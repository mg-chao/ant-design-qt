#include "icon_registry.h"

#include <QCache>
#include <QIconEngine>
#include <QMutex>
#include <QMutexLocker>
#include <QPaintDevice>
#include <QPainter>
#include <QRegularExpression>
#include <QSvgRenderer>
#include <QtMath>

#include <algorithm>
#include <memory>
#include <utility>

namespace adqt::icons::detail {

struct StoredIcon {
  IconMetadata metadata;
  QByteArray svgTemplate;
};

struct IconRegistryImpl {
  static constexpr int defaultCacheLimitKB = 8 * 1024;

  IconRegistryImpl()
      : pixmapCache(defaultCacheLimitKB) {}

  QMutex mutex;
  QHash<IconId, StoredIcon> icons;
  IconPaletteResolver resolver;
  QCache<QString, QPixmap> pixmapCache;
  int cacheLimitKB = defaultCacheLimitKB;

};

}  // namespace adqt::icons::detail

namespace adqt::icons {
namespace {

using detail::IconRegistryImpl;
using detail::StoredIcon;

constexpr qreal kDprQuantizeStep = 0.25;
constexpr qreal kDprMin = 1.0;
constexpr qreal kDprMax = 4.0;
constexpr qint64 kMaxCacheablePixmapBytes = 256 * 1024;
constexpr auto kPrimaryPlaceholder = "__ADQT_SLOT_PRIMARY__";
constexpr auto kSecondaryPlaceholder = "__ADQT_SLOT_SECONDARY__";
constexpr auto kTertiaryPlaceholder = "__ADQT_SLOT_TERTIARY__";

QString placeholderForSlot(const QString& slotName) {
  if (slotName.compare(QStringLiteral("secondary"), Qt::CaseInsensitive) == 0) {
    return QString::fromLatin1(kSecondaryPlaceholder);
  }
  if (slotName.compare(QStringLiteral("tertiary"), Qt::CaseInsensitive) == 0) {
    return QString::fromLatin1(kTertiaryPlaceholder);
  }
  return QString::fromLatin1(kPrimaryPlaceholder);
}

QString toSvgColor(const QColor& color) {
  QColor safe = color.isValid() ? color : QColor(QStringLiteral("#000000"));
  return safe.name(QColor::HexRgb);
}

qreal normalizeDevicePixelRatio(qreal devicePixelRatio) {
  const qreal safe = devicePixelRatio > 0.0 ? devicePixelRatio : 1.0;
  const qreal clamped = qBound(kDprMin, safe, kDprMax);
  const qreal quantized = qRound(clamped / kDprQuantizeStep) * kDprQuantizeStep;
  return qBound(kDprMin, quantized, kDprMax);
}

bool shouldCachePixmap(int pixelWidth, int pixelHeight) {
  const qint64 safeW = qMax(1, pixelWidth);
  const qint64 safeH = qMax(1, pixelHeight);
  const qint64 approxBytes = safeW * safeH * 4;
  return approxBytes <= kMaxCacheablePixmapBytes;
}

int pixmapCostKB(const QPixmap& pixmap) {
  const qreal dpr = pixmap.devicePixelRatio();
  const qint64 pixelW = qMax<qint64>(1, qRound(pixmap.width() * dpr));
  const qint64 pixelH = qMax<qint64>(1, qRound(pixmap.height() * dpr));
  const qint64 bytes = pixelW * pixelH * 4;
  return qMax<int>(1, static_cast<int>((bytes + 1023) / 1024));
}

void applyGlobalOpacity(QPixmap& pixmap, qreal opacity) {
  const qreal clamped = qBound(0.0, opacity, 1.0);
  if (clamped >= 1.0) {
    return;
  }
  if (clamped <= 0.0) {
    pixmap.fill(Qt::transparent);
    return;
  }

  QPainter painter(&pixmap);
  painter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
  const int alpha = qBound(0, qRound(clamped * 255.0), 255);
  painter.fillRect(pixmap.rect(), QColor(0, 0, 0, alpha));
}

QColor deriveSecondaryColor(const QColor& primary) {
  QColor safe = primary.isValid() ? primary : QColor(QStringLiteral("#1677FF"));
  QColor hsl = safe.toHsl();
  int sat = hsl.hslSaturation();
  int light = hsl.lightness();

  sat = qMax(8, qRound(sat * 0.22));
  light = qMin(245, qRound(light + (255 - light) * 0.82));
  hsl.setHsl(hsl.hslHue(), sat, light, safe.alpha());
  return hsl.toRgb();
}

IconPalette defaultPalette() {
  IconPalette palette;
  palette.text = QColor(QStringLiteral("#1F1F1F"));
  palette.textDisabled = QColor(QStringLiteral("#BFBFBF"));
  palette.primary = QColor(QStringLiteral("#1677FF"));
  palette.twoToneSecondary = QColor(QStringLiteral("#E6F4FF"));
  palette.revision = 1;
  return palette;
}

IconPalette resolvePalette(const IconPaletteResolver& resolver) {
  if (!resolver) {
    return defaultPalette();
  }

  IconPalette palette = resolver();
  if (!palette.text.isValid()) {
    palette.text = QColor(QStringLiteral("#1F1F1F"));
  }
  if (!palette.textDisabled.isValid()) {
    palette.textDisabled = QColor(QStringLiteral("#BFBFBF"));
  }
  if (!palette.primary.isValid()) {
    palette.primary = QColor(QStringLiteral("#1677FF"));
  }
  if (!palette.twoToneSecondary.isValid()) {
    palette.twoToneSecondary = QColor(QStringLiteral("#E6F4FF"));
  }
  if (palette.revision == 0) {
    palette.revision = 1;
  }
  return palette;
}

struct ResolvedColors {
  QColor primary;
  QColor secondary;
  QColor tertiary;
  quint64 revision = 1;
};

ResolvedColors resolveColors(IconRenderModel model,
                             const IconColorOverrides& overrides,
                             const IconPalette& palette,
                             QIcon::Mode mode) {
  ResolvedColors resolved;
  resolved.revision = palette.revision;

  const bool disabled = mode == QIcon::Disabled;
  if (model == IconRenderModel::Monochrome) {
    resolved.primary = disabled ? palette.textDisabled : palette.text;
    resolved.secondary = resolved.primary;
    resolved.tertiary = resolved.secondary;
  } else {
    resolved.primary = disabled ? palette.textDisabled : palette.primary;
    resolved.secondary = disabled ? deriveSecondaryColor(palette.textDisabled)
                                  : palette.twoToneSecondary;
    resolved.tertiary = resolved.secondary;
  }

  if (overrides.hasPrimary && overrides.primary.isValid()) {
    resolved.primary = overrides.primary;
    if (model != IconRenderModel::Monochrome && !overrides.hasSecondary) {
      resolved.secondary = deriveSecondaryColor(overrides.primary);
      resolved.tertiary = resolved.secondary;
    }
  }
  if (overrides.hasSecondary && overrides.secondary.isValid()) {
    resolved.secondary = overrides.secondary;
    resolved.tertiary = overrides.secondary;
  }
  if (overrides.hasTertiary && overrides.tertiary.isValid()) {
    resolved.tertiary = overrides.tertiary;
  }

  return resolved;
}

QString cacheKey(const IconMetadata& metadata,
                 const QSize& logicalSize,
                 qreal dpr,
                 QIcon::Mode mode,
                 QIcon::State state,
                 const ResolvedColors& colors) {
  return QStringLiteral("adqt:%1:%2:%3:%4x%5:%6:%7:%8:%9:%10:%11:%12")
      .arg(metadata.id.pack)
      .arg(QString::fromLatin1(iconThemeKey(metadata.id.theme)))
      .arg(metadata.id.name)
      .arg(logicalSize.width())
      .arg(logicalSize.height())
      .arg(qRound(dpr * 1000))
      .arg(static_cast<int>(mode))
      .arg(static_cast<int>(state))
      .arg(static_cast<unsigned long long>(colors.revision))
      .arg(colors.primary.rgba(), 8, 16, QLatin1Char('0'))
      .arg(colors.secondary.rgba(), 8, 16, QLatin1Char('0'))
      .arg(colors.tertiary.rgba(), 8, 16, QLatin1Char('0'));
}

bool hasAttribute(const QString& tag, const QString& attrName) {
  const QRegularExpression expr(
      QStringLiteral(R"(\b%1\s*=)").arg(attrName),
      QRegularExpression::CaseInsensitiveOption);
  return expr.match(tag).hasMatch();
}

bool replaceColorAttribute(QString& tag, const QString& attrName, const QString& placeholder) {
  const QRegularExpression expr(
      QStringLiteral(R"(\b%1\s*=\s*(['"])([^'"]*)\1)").arg(attrName),
      QRegularExpression::CaseInsensitiveOption);
  const QRegularExpressionMatch match = expr.match(tag);
  if (!match.hasMatch()) {
    return false;
  }

  const QString value = match.captured(2);
  if (value.compare(QStringLiteral("none"), Qt::CaseInsensitive) == 0) {
    return false;
  }

  const QString replacement = QStringLiteral("%1=\"%2\"").arg(attrName, placeholder);
  tag.replace(match.capturedStart(0), match.capturedLength(0), replacement);
  return true;
}

void insertAttribute(QString& tag, const QString& attributeFragment) {
  const int closePos = tag.lastIndexOf(QLatin1Char('>'));
  if (closePos <= 0) {
    return;
  }
  int insertPos = closePos;
  if (insertPos > 0 && tag.at(insertPos - 1) == QLatin1Char('/')) {
    --insertPos;
  }
  tag.insert(insertPos, attributeFragment);
}

QByteArray normalizeSvgTemplate(const IconDefinition& definition) {
  QString svg = QString::fromUtf8(definition.svgTemplate);
  if (!svg.contains(QStringLiteral("<svg"), Qt::CaseInsensitive)) {
    return QByteArray();
  }

  const QRegularExpression slotTagExpr(
      QStringLiteral(
          R"(<([A-Za-z_:][A-Za-z0-9:._-]*)([^>]*)\bdata-adqt-slot\s*=\s*(['"])(primary|secondary|tertiary)\3([^>]*)>)"),
      QRegularExpression::CaseInsensitiveOption);
  const QRegularExpression slotAttrExpr(
      QStringLiteral(R"(\s*data-adqt-slot\s*=\s*(['"])(primary|secondary|tertiary)\1)"),
      QRegularExpression::CaseInsensitiveOption);
  const QRegularExpression currentColorExpr(QStringLiteral("currentColor"),
                                            QRegularExpression::CaseInsensitiveOption);

  QString normalized;
  normalized.reserve(svg.size() + 64);
  int cursor = 0;
  QRegularExpressionMatchIterator it = slotTagExpr.globalMatch(svg);
  while (it.hasNext()) {
    const QRegularExpressionMatch match = it.next();
    normalized += svg.mid(cursor, match.capturedStart(0) - cursor);

    QString tag = match.captured(0);
    const QString slotName = match.captured(4).toLower();
    const QString placeholder = placeholderForSlot(slotName);
    tag.remove(slotAttrExpr);

    bool changed = false;
    changed = replaceColorAttribute(tag, QStringLiteral("fill"), placeholder) || changed;
    changed = replaceColorAttribute(tag, QStringLiteral("stroke"), placeholder) || changed;
    if (tag.contains(QStringLiteral("currentColor"), Qt::CaseInsensitive)) {
      tag.replace(currentColorExpr, placeholder);
      changed = true;
    }
    if (!changed && !hasAttribute(tag, QStringLiteral("fill")) &&
        !hasAttribute(tag, QStringLiteral("stroke"))) {
      insertAttribute(tag, QStringLiteral(" fill=\"") + placeholder + QStringLiteral("\""));
    }

    normalized += tag;
    cursor = match.capturedEnd(0);
  }
  normalized += svg.mid(cursor);
  normalized.replace(currentColorExpr, QString::fromLatin1(kPrimaryPlaceholder));

  const bool hasPrimary = normalized.contains(QString::fromLatin1(kPrimaryPlaceholder));
  const bool hasSecondary = normalized.contains(QString::fromLatin1(kSecondaryPlaceholder));
  const bool hasTertiary = normalized.contains(QString::fromLatin1(kTertiaryPlaceholder));

  switch (definition.model) {
    case IconRenderModel::Monochrome:
      if (!hasPrimary || hasSecondary || hasTertiary) {
        return QByteArray();
      }
      break;
    case IconRenderModel::TwoTone:
      if (!hasPrimary || !hasSecondary || hasTertiary) {
        return QByteArray();
      }
      break;
    case IconRenderModel::ThreeTone:
      if (!hasPrimary || !hasSecondary || !hasTertiary) {
        return QByteArray();
      }
      break;
  }

  return normalized.toUtf8();
}

QByteArray renderSvgTemplate(const QByteArray& svgTemplate,
                             const QColor& primary,
                             const QColor& secondary,
                             const QColor& tertiary) {
  QString svg = QString::fromUtf8(svgTemplate);
  svg.replace(QString::fromLatin1(kPrimaryPlaceholder), toSvgColor(primary), Qt::CaseSensitive);
  svg.replace(QString::fromLatin1(kSecondaryPlaceholder), toSvgColor(secondary), Qt::CaseSensitive);
  svg.replace(QString::fromLatin1(kTertiaryPlaceholder), toSvgColor(tertiary), Qt::CaseSensitive);
  return svg.toUtf8();
}

bool lookupStoredIcon(const std::shared_ptr<IconRegistryImpl>& impl,
                      const IconId& id,
                      StoredIcon* stored,
                      IconPaletteResolver* resolver) {
  QMutexLocker locker(&impl->mutex);
  const auto it = impl->icons.constFind(id);
  if (it == impl->icons.constEnd()) {
    return false;
  }
  if (stored) {
    *stored = it.value();
  }
  if (resolver) {
    *resolver = impl->resolver;
  }
  return true;
}

QPixmap renderPixmapFromImpl(const std::shared_ptr<IconRegistryImpl>& impl,
                             const IconRef& ref,
                             const QSize& logicalSize,
                             qreal devicePixelRatio,
                             QIcon::Mode mode,
                             QIcon::State state) {
  if (!ref.isValid()) {
    return QPixmap();
  }

  StoredIcon stored;
  IconPaletteResolver resolver;
  if (!lookupStoredIcon(impl, ref.id, &stored, &resolver)) {
    return QPixmap();
  }

  const QSize effectiveSize = logicalSize.isValid() && !logicalSize.isEmpty()
                                  ? logicalSize
                                  : QSize(16, 16);
  const qreal dpr = normalizeDevicePixelRatio(devicePixelRatio);
  const int pixelW = qMax(1, qRound(effectiveSize.width() * dpr));
  const int pixelH = qMax(1, qRound(effectiveSize.height() * dpr));
  const bool enableCache = shouldCachePixmap(pixelW, pixelH);

  const IconPalette palette = resolvePalette(resolver);
  const ResolvedColors colors = resolveColors(stored.metadata.model, ref.colors, palette, mode);
  const QString key = cacheKey(stored.metadata, effectiveSize, dpr, mode, state, colors);

  if (enableCache) {
    QMutexLocker locker(&impl->mutex);
    if (QPixmap* cached = impl->pixmapCache.object(key)) {
      return *cached;
    }
  }

  const QByteArray coloredSvg =
      renderSvgTemplate(stored.svgTemplate, colors.primary, colors.secondary, colors.tertiary);
  QSvgRenderer renderer(coloredSvg);
  if (!renderer.isValid()) {
    return QPixmap();
  }

  QPixmap pixmap(pixelW, pixelH);
  pixmap.fill(Qt::transparent);
  {
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    renderer.render(&painter, QRectF(0, 0, pixelW, pixelH));
  }

  if (stored.metadata.model == IconRenderModel::Monochrome) {
    applyGlobalOpacity(pixmap, colors.primary.alphaF());
  }

  pixmap.setDevicePixelRatio(dpr);

  if (enableCache) {
    QMutexLocker locker(&impl->mutex);
    impl->pixmapCache.insert(key, new QPixmap(pixmap), pixmapCostKB(pixmap));
  }
  return pixmap;
}

class RegistryIconEngine final : public QIconEngine {
 public:
  RegistryIconEngine(std::shared_ptr<IconRegistryImpl> impl, const IconRef& ref)
      : impl_(std::move(impl)), ref_(ref) {}

  QIconEngine* clone() const override {
    return new RegistryIconEngine(impl_, ref_);
  }

  QString key() const override {
    return QStringLiteral("adqt.registry.icon.engine");
  }

  QPixmap pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) override {
    return renderPixmapFromImpl(impl_, ref_, size, 1.0, mode, state);
  }

  QPixmap scaledPixmap(const QSize& size,
                       QIcon::Mode mode,
                       QIcon::State state,
                       qreal scale) override {
    return renderPixmapFromImpl(impl_, ref_, size, scale > 0.0 ? scale : 1.0, mode, state);
  }

  void paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state) override {
    if (!painter || rect.isEmpty()) {
      return;
    }
    const qreal dpr = painter->device() ? painter->device()->devicePixelRatioF() : 1.0;
    const QPixmap pixmap = renderPixmapFromImpl(impl_, ref_, rect.size(), dpr, mode, state);
    if (!pixmap.isNull()) {
      painter->drawPixmap(rect, pixmap);
    }
  }

 private:
  std::shared_ptr<IconRegistryImpl> impl_;
  IconRef ref_;
};

}  // namespace

const char* iconThemeKey(IconTheme theme) {
  switch (theme) {
    case IconTheme::Outlined:
      return "outlined";
    case IconTheme::Filled:
      return "filled";
    case IconTheme::TwoTone:
      return "twotone";
  }
  return "outlined";
}

IconRegistry::IconRegistry()
    : impl_(std::make_shared<IconRegistryImpl>()) {}

IconRegistry::~IconRegistry() = default;

bool IconRegistry::registerIcon(const IconDefinition& definition) {
  if (!definition.isValid()) {
    return false;
  }

  IconDefinition normalized = definition;
  normalized.svgTemplate = normalizeSvgTemplate(definition);
  if (normalized.svgTemplate.isEmpty()) {
    return false;
  }

  StoredIcon stored;
  stored.metadata.id = normalized.id;
  stored.metadata.model = normalized.model;
  stored.svgTemplate = normalized.svgTemplate;

  QMutexLocker locker(&impl_->mutex);
  if (impl_->icons.contains(normalized.id)) {
    return false;
  }
  impl_->icons.insert(normalized.id, stored);
  return true;
}

bool IconRegistry::containsIcon(const IconId& id) const {
  if (!id.isValid()) {
    return false;
  }
  QMutexLocker locker(&impl_->mutex);
  return impl_->icons.contains(id);
}

IconMetadata IconRegistry::describeIcon(const IconId& id) const {
  if (!id.isValid()) {
    return IconMetadata();
  }
  QMutexLocker locker(&impl_->mutex);
  const auto it = impl_->icons.constFind(id);
  return it == impl_->icons.constEnd() ? IconMetadata() : it.value().metadata;
}

QList<IconMetadata> IconRegistry::listIcons(const QString& pack) const {
  QList<IconMetadata> icons;
  {
    QMutexLocker locker(&impl_->mutex);
    icons.reserve(impl_->icons.size());
    for (auto it = impl_->icons.constBegin(); it != impl_->icons.constEnd(); ++it) {
      if (!pack.isEmpty() && it.key().pack != pack) {
        continue;
      }
      icons.append(it.value().metadata);
    }
  }
  std::sort(icons.begin(), icons.end(), [](const IconMetadata& lhs, const IconMetadata& rhs) {
    if (lhs.id.pack != rhs.id.pack) {
      return lhs.id.pack < rhs.id.pack;
    }
    if (lhs.id.theme != rhs.id.theme) {
      return static_cast<int>(lhs.id.theme) < static_cast<int>(rhs.id.theme);
    }
    return lhs.id.name < rhs.id.name;
  });
  return icons;
}

QList<IconMetadata> IconRegistry::listIcons(const QString& pack, IconTheme theme) const {
  QList<IconMetadata> icons;
  {
    QMutexLocker locker(&impl_->mutex);
    icons.reserve(impl_->icons.size());
    for (auto it = impl_->icons.constBegin(); it != impl_->icons.constEnd(); ++it) {
      if (!pack.isEmpty() && it.key().pack != pack) {
        continue;
      }
      if (it.key().theme != theme) {
        continue;
      }
      icons.append(it.value().metadata);
    }
  }
  std::sort(icons.begin(), icons.end(), [](const IconMetadata& lhs, const IconMetadata& rhs) {
    return lhs.id.name < rhs.id.name;
  });
  return icons;
}

QIcon IconRegistry::makeIcon(const IconRef& ref) const {
  if (!containsIcon(ref.id)) {
    return QIcon();
  }
  return QIcon(new RegistryIconEngine(impl_, ref));
}

QPixmap IconRegistry::renderIconPixmap(const IconRef& ref,
                                       const QSize& logicalSize,
                                       qreal devicePixelRatio,
                                       QIcon::Mode mode,
                                       QIcon::State state) const {
  return renderPixmapFromImpl(impl_, ref, logicalSize, devicePixelRatio, mode, state);
}

void IconRegistry::paintIcon(QPainter* painter,
                             const IconRef& ref,
                             const QRectF& rect,
                             QIcon::Mode mode,
                             QIcon::State state) const {
  if (!painter || rect.isEmpty()) {
    return;
  }

  const qreal dpr = painter->device() ? painter->device()->devicePixelRatioF() : 1.0;
  const QPixmap pixmap = renderPixmapFromImpl(impl_, ref, rect.size().toSize(), dpr, mode, state);
  if (pixmap.isNull()) {
    return;
  }
  painter->drawPixmap(rect, pixmap, QRectF(0, 0, pixmap.width(), pixmap.height()));
}

void IconRegistry::setPaletteResolver(IconPaletteResolver resolver) {
  QMutexLocker locker(&impl_->mutex);
  impl_->resolver = std::move(resolver);
  impl_->pixmapCache.clear();
}

void IconRegistry::clearPaletteResolver() {
  QMutexLocker locker(&impl_->mutex);
  impl_->resolver = IconPaletteResolver();
  impl_->pixmapCache.clear();
}

void IconRegistry::setCacheLimitKB(int kb) {
  const int safe = qMax(1024, kb);
  QMutexLocker locker(&impl_->mutex);
  impl_->cacheLimitKB = safe;
  impl_->pixmapCache.setMaxCost(safe);
  impl_->pixmapCache.clear();
}

void IconRegistry::clearCache() {
  QMutexLocker locker(&impl_->mutex);
  impl_->pixmapCache.clear();
}

IconRegistry& defaultRegistry() {
  static IconRegistry registry;
  return registry;
}

bool registerIcon(const IconDefinition& definition) {
  return defaultRegistry().registerIcon(definition);
}

bool containsIcon(const IconId& id) {
  return defaultRegistry().containsIcon(id);
}

IconMetadata describeIcon(const IconId& id) {
  return defaultRegistry().describeIcon(id);
}

QList<IconMetadata> listIcons(const QString& pack) {
  return defaultRegistry().listIcons(pack);
}

QList<IconMetadata> listIcons(const QString& pack, IconTheme theme) {
  return defaultRegistry().listIcons(pack, theme);
}

QIcon makeIcon(const IconRef& ref) {
  return defaultRegistry().makeIcon(ref);
}

QPixmap renderIconPixmap(const IconRef& ref,
                         const QSize& logicalSize,
                         qreal devicePixelRatio,
                         QIcon::Mode mode,
                         QIcon::State state) {
  return defaultRegistry().renderIconPixmap(ref, logicalSize, devicePixelRatio, mode, state);
}

void paintIcon(QPainter* painter,
               const IconRef& ref,
               const QRectF& rect,
               QIcon::Mode mode,
               QIcon::State state) {
  defaultRegistry().paintIcon(painter, ref, rect, mode, state);
}

void setPaletteResolver(IconPaletteResolver resolver) {
  defaultRegistry().setPaletteResolver(std::move(resolver));
}

void clearPaletteResolver() {
  defaultRegistry().clearPaletteResolver();
}

void setCacheLimitKB(int kb) {
  defaultRegistry().setCacheLimitKB(kb);
}

void clearCache() {
  defaultRegistry().clearCache();
}

}  // namespace adqt::icons
