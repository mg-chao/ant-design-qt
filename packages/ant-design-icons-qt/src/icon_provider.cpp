#include "icon_provider.h"

#include "icon_engine.h"
#include "icon_render.h"

#include <QFile>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QPainter>
#include <QPixmapCache>
#include <QStringList>
#include <QSvgRenderer>
#include <QVector>
#include <QtMath>

#include <mutex>
#include <utility>

int qInitResources_ant_design_icons();

namespace adqt::icons::detail {

namespace {

constexpr int kDefaultPixmapCacheLimitKB = 8 * 1024;
constexpr qreal kDprQuantizeStep = 0.25;
constexpr qreal kDprMin = 1.0;
constexpr qreal kDprMax = 4.0;
constexpr qint64 kMaxCacheablePixmapBytes = 256 * 1024;

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

void ensureIconResources() {
  static std::once_flag once;
  std::call_once(once, []() {
    ::qInitResources_ant_design_icons();
  });
}

struct ResolvedColors {
  QColor primary;
  QColor secondary;
  QColor tertiary;
  quint64 revision = 1;
};

struct ResolvedIconSource {
  IconTheme theme = IconTheme::Outlined;
  QString name;
  QByteArray svg;
};

class IconRuntime final {
 public:
  static IconRuntime& instance() {
    static IconRuntime runtime;
    return runtime;
  }

  QIcon makeIcon(int index, const IconStyle& style) {
    if (!isKnownIndex(index)) {
      return QIcon();
    }

    return QIcon(new SvgIconEngine(index, style));
  }

  IconToken registerCustomIcon(const CustomIconSource& source, const IconStyle& style) {
    if (!source.isValid()) {
      return IconToken();
    }

    QMutexLocker locker(&mutex_);
    const int index = iconEntryCount() + customEntries_.size();
    customEntries_.append(source);

    IconToken token;
    token.index = index;
    token.style = style;
    return token;
  }

  IconMetadata metadata(int index) {
    IconMetadata metadata;
    if (index < 0) {
      return metadata;
    }

    if (index < iconEntryCount()) {
      ensureIconResources();
      const IconEntry& entry = iconEntryAt(index);
      metadata.valid = true;
      metadata.custom = false;
      metadata.theme = entry.theme;
      metadata.name = QString::fromLatin1(entry.name);
      return metadata;
    }

    QMutexLocker locker(&mutex_);
    const int offset = customEntryOffset(index);
    if (offset < 0 || offset >= customEntries_.size()) {
      return metadata;
    }

    const CustomIconSource& source = customEntries_.at(offset);
    metadata.valid = true;
    metadata.custom = true;
    metadata.theme = source.theme;
    metadata.name = source.name;
    return metadata;
  }

  QPixmap renderPixmapByIndex(int index,
                              const IconStyle& style,
                              const QSize& logicalSize,
                              qreal devicePixelRatio,
                              QIcon::Mode mode,
                              QIcon::State state) {
    const ResolvedIconSource source = resolveIconSource(index);
    if (source.svg.isEmpty()) {
      return QPixmap();
    }

    const QSize effectiveSize = logicalSize.isValid() && !logicalSize.isEmpty()
                                    ? logicalSize
                                    : QSize(16, 16);
    const qreal dpr = normalizeDevicePixelRatio(devicePixelRatio);
    const int pixelW = qMax(1, qRound(effectiveSize.width() * dpr));
    const int pixelH = qMax(1, qRound(effectiveSize.height() * dpr));
    const bool enablePixmapCache = shouldCachePixmap(pixelW, pixelH);

    const IconThemeSnapshot snapshot = resolveThemeSnapshot();
    const ResolvedColors colors = resolveColors(source.theme, style, snapshot, mode);
    const QString key = cacheKey(index, effectiveSize, dpr, mode, state, colors);

    if (enablePixmapCache) {
      QPixmap cached;
      if (QPixmapCache::find(key, &cached)) {
        return cached;
      }
    }

    const QByteArray coloredSvg =
        applyColorsToSvg(source.svg,
                         source.theme,
                         source.name.toUtf8().constData(),
                         colors.primary,
                         colors.secondary,
                         colors.tertiary);
    QSvgRenderer renderer(coloredSvg);
    if (!renderer.isValid()) {
      return QPixmap();
    }

    QPixmap pm(pixelW, pixelH);
    pm.fill(Qt::transparent);
    {
      QPainter painter(&pm);
      painter.setRenderHint(QPainter::Antialiasing, true);
      renderer.render(&painter, QRectF(0, 0, pixelW, pixelH));
    }

    // QtSvg may ignore alpha in `fill` color strings. Keep rgb replacement stable and
    // apply final opacity on the rendered pixmap for non-twotone single-color icons.
    if (source.theme != IconTheme::TwoTone) {
      applyGlobalOpacity(pm, colors.primary.alphaF());
    }

    pm.setDevicePixelRatio(dpr);

    if (enablePixmapCache) {
      QPixmapCache::insert(key, pm);
    }
    return pm;
  }

  void setResolver(IconThemeResolver resolver) {
    QMutexLocker locker(&mutex_);
    resolver_ = std::move(resolver);
    clearSvgSourceCacheLocked();
    QPixmapCache::clear();
  }

  void clearResolver() {
    QMutexLocker locker(&mutex_);
    resolver_ = IconThemeResolver();
    clearSvgSourceCacheLocked();
    QPixmapCache::clear();
  }

  void setPixmapCacheLimit(int kb) {
    const int safe = qMax(1024, kb);
    QMutexLocker locker(&mutex_);
    pixmapCacheLimitKB_ = safe;
    QPixmapCache::setCacheLimit(pixmapCacheLimitKB_);
    QPixmapCache::clear();
  }

  void clearCache() {
    QMutexLocker locker(&mutex_);
    clearSvgSourceCacheLocked();
    QPixmapCache::clear();
  }

 private:
  IconRuntime() {
    QPixmapCache::setCacheLimit(pixmapCacheLimitKB_);
  }

  static IconThemeSnapshot defaultSnapshot() {
    IconThemeSnapshot snapshot;
    snapshot.text = QColor(QStringLiteral("#1F1F1F"));
    snapshot.textDisabled = QColor(QStringLiteral("#BFBFBF"));
    snapshot.primary = QColor(QStringLiteral("#1677FF"));
    snapshot.twoToneSecondary = QColor(QStringLiteral("#E6F4FF"));
    snapshot.revision = 1;
    return snapshot;
  }

  IconThemeSnapshot resolveThemeSnapshot() {
    IconThemeResolver resolverCopy;
    {
      QMutexLocker locker(&mutex_);
      resolverCopy = resolver_;
    }

    if (!resolverCopy) {
      return defaultSnapshot();
    }

    IconThemeSnapshot snapshot = resolverCopy();
    if (!snapshot.text.isValid()) {
      snapshot.text = QColor(QStringLiteral("#1F1F1F"));
    }
    if (!snapshot.textDisabled.isValid()) {
      snapshot.textDisabled = QColor(QStringLiteral("#BFBFBF"));
    }
    if (!snapshot.primary.isValid()) {
      snapshot.primary = QColor(QStringLiteral("#1677FF"));
    }
    if (!snapshot.twoToneSecondary.isValid()) {
      snapshot.twoToneSecondary = QColor(QStringLiteral("#E6F4FF"));
    }
    if (snapshot.revision == 0) {
      snapshot.revision = 1;
    }
    return snapshot;
  }

  static ResolvedColors resolveColors(IconTheme theme,
                                      const IconStyle& style,
                                      const IconThemeSnapshot& snapshot,
                                      QIcon::Mode mode) {
    ResolvedColors resolved;
    resolved.revision = snapshot.revision;

    const bool disabled = mode == QIcon::Disabled;
    if (theme == IconTheme::TwoTone) {
      resolved.primary = disabled ? snapshot.textDisabled : snapshot.primary;
      resolved.secondary = disabled ? deriveSecondaryColor(snapshot.textDisabled)
                                    : snapshot.twoToneSecondary;
      resolved.tertiary = resolved.secondary;
    } else {
      resolved.primary = disabled ? snapshot.textDisabled : snapshot.text;
      resolved.secondary = resolved.primary;
      resolved.tertiary = resolved.secondary;
    }

    if (style.hasPrimary && style.primary.isValid()) {
      resolved.primary = style.primary;
      if (theme == IconTheme::TwoTone && !style.hasSecondary) {
        resolved.secondary = deriveSecondaryColor(style.primary);
      }
    }
    if (style.hasSecondary && style.secondary.isValid()) {
      resolved.secondary = style.secondary;
    }
    if (style.hasTertiary && style.tertiary.isValid()) {
      resolved.tertiary = style.tertiary;
    }

    return resolved;
  }

  static QString cacheKey(int index,
                          const QSize& logicalSize,
                          qreal dpr,
                          QIcon::Mode mode,
                          QIcon::State state,
                          const ResolvedColors& colors) {
    return QStringLiteral("adqt:icon:%1:%2x%3:%4:%5:%6:%7:%8:%9:%10")
        .arg(index)
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

  QByteArray loadSvgSource(const QString& qrcPath) {
    {
      QMutexLocker locker(&mutex_);
      const auto it = svgSourceCache_.constFind(qrcPath);
      if (it != svgSourceCache_.constEnd()) {
        return it.value();
      }
    }

    QFile file(qrcPath);
    if (!file.open(QIODevice::ReadOnly)) {
      return QByteArray();
    }

    const QByteArray payload = file.readAll();
    if (payload.isEmpty()) {
      return payload;
    }

    QMutexLocker locker(&mutex_);
    putSvgSourceCacheLocked(qrcPath, payload);
    return payload;
  }

  bool isKnownIndex(int index) {
    if (index < 0) {
      return false;
    }
    if (index < iconEntryCount()) {
      ensureIconResources();
      return true;
    }

    QMutexLocker locker(&mutex_);
    return customEntryOffset(index) >= 0 && customEntryOffset(index) < customEntries_.size();
  }

  ResolvedIconSource resolveIconSource(int index) {
    ResolvedIconSource source;
    if (index < 0) {
      return source;
    }

    if (index < iconEntryCount()) {
      ensureIconResources();
      const IconEntry& entry = iconEntryAt(index);
      source.theme = entry.theme;
      source.name = QString::fromLatin1(entry.name);
      source.svg = loadSvgSource(QString::fromLatin1(entry.qrcPath));
      return source;
    }

    CustomIconSource custom;
    {
      QMutexLocker locker(&mutex_);
      const int offset = customEntryOffset(index);
      if (offset < 0 || offset >= customEntries_.size()) {
        return source;
      }
      custom = customEntries_.at(offset);
    }

    source.theme = custom.theme;
    source.name = custom.name;
    if (custom.sourceType == IconSourceType::SvgBytes) {
      source.svg = custom.svg;
    } else {
      source.svg = loadSvgSource(custom.path);
    }
    return source;
  }

  static int customEntryOffset(int index) {
    return index - iconEntryCount();
  }

  void putSvgSourceCacheLocked(const QString& key, const QByteArray& value) {
    if (svgSourceCache_.contains(key)) {
      svgSourceOrder_.removeAll(key);
    }
    svgSourceCache_.insert(key, value);
    svgSourceOrder_.append(key);

    while (svgSourceOrder_.size() > svgSourceCacheMaxEntries_) {
      const QString oldest = svgSourceOrder_.takeFirst();
      svgSourceCache_.remove(oldest);
    }
  }

  void clearSvgSourceCacheLocked() {
    svgSourceCache_.clear();
    svgSourceOrder_.clear();
  }

  QMutex mutex_;
  IconThemeResolver resolver_;
  int pixmapCacheLimitKB_ = kDefaultPixmapCacheLimitKB;
  int svgSourceCacheMaxEntries_ = 128;
  QHash<QString, QByteArray> svgSourceCache_;
  QStringList svgSourceOrder_;
  QVector<CustomIconSource> customEntries_;
};

}  // namespace

IconToken makeTokenByIndex(int index, const IconStyle& style) {
  IconToken token;
  token.index = index;
  token.style = style;
  return token;
}

QIcon makeIconByIndex(int index, const IconStyle& style) {
  return IconRuntime::instance().makeIcon(index, style);
}

QIcon makeIcon(const IconToken& token) {
  if (!token.isValid()) {
    return QIcon();
  }
  return makeIconByIndex(token.index, token.style);
}

IconMetadata iconMetadata(const IconToken& token) {
  if (!token.isValid()) {
    return IconMetadata();
  }
  return IconRuntime::instance().metadata(token.index);
}

bool isTwoTone(const IconToken& token) {
  const IconMetadata metadata = iconMetadata(token);
  return metadata.valid && metadata.theme == IconTheme::TwoTone;
}

bool isSingleTone(const IconToken& token) {
  const IconMetadata metadata = iconMetadata(token);
  return metadata.valid && metadata.theme != IconTheme::TwoTone;
}

IconToken registerCustomIcon(const CustomIconSource& source, const IconStyle& style) {
  return IconRuntime::instance().registerCustomIcon(source, style);
}

QPixmap renderIconPixmapByIndex(int index,
                                const IconStyle& style,
                                const QSize& logicalSize,
                                qreal devicePixelRatio,
                                QIcon::Mode mode,
                                QIcon::State state) {
  return IconRuntime::instance().renderPixmapByIndex(
      index, style, logicalSize, devicePixelRatio, mode, state);
}

QPixmap renderIconPixmap(const IconToken& token,
                         const QSize& logicalSize,
                         qreal devicePixelRatio,
                         QIcon::Mode mode,
                         QIcon::State state) {
  if (!token.isValid()) {
    return QPixmap();
  }
  return renderIconPixmapByIndex(token.index, token.style, logicalSize, devicePixelRatio, mode,
                                 state);
}

void setThemeResolver(IconThemeResolver resolver) {
  IconRuntime::instance().setResolver(std::move(resolver));
}

void clearThemeResolver() {
  IconRuntime::instance().clearResolver();
}

void setPixmapCacheLimitKB(int kb) {
  IconRuntime::instance().setPixmapCacheLimit(kb);
}

void clearIconCache() {
  IconRuntime::instance().clearCache();
}

}  // namespace adqt::icons::detail
