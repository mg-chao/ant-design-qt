#include "button_style.h"

#include "theme/fast_color_lite.h"
#include "theme/palette_generate.h"
#include "theme/theme.h"
#include "theme/theme_algorithms.h"

#include <QHash>

#include <algorithm>
#include <cmath>

namespace adqt::widgets::detail {

namespace {

using adqt::theme::ThemeAlgorithm;
using adqt::theme::ThemeConfig;
using adqt::theme::ThemeManager;
using adqt::theme::ThemeMapToken;
using adqt::theme::ThemeSeedToken;

struct ColorFamily {
  QColor base;
  QColor hover;
  QColor active;
  QColor light;
  QColor lightHover;
  QColor lightActive;

  QColor outlinedText;
  QColor outlinedTextHover;
  QColor outlinedTextActive;

  QColor filledText;
  QColor filledTextHover;
  QColor filledTextActive;

  QColor textText;
  QColor textTextHover;
  QColor textTextActive;

  QColor solidBg;
  QColor solidBgHover;
  QColor solidBgActive;
  QColor solidText;
  QColor shadow;
};

constexpr int kStyleCacheMaxEntries = 512;
constexpr int kPresetPaletteCacheMaxEntries = 128;

struct PresetPaletteKey {
  QString base;
  QString background;
  bool darkTheme = false;
};

bool operator==(const PresetPaletteKey& lhs, const PresetPaletteKey& rhs) {
  return lhs.base == rhs.base && lhs.background == rhs.background && lhs.darkTheme == rhs.darkTheme;
}

uint qHash(const PresetPaletteKey& key, uint seed) {
  seed = ::qHash(key.base, seed);
  seed = ::qHash(key.background, seed);
  seed = ::qHash(key.darkTheme, seed);
  return seed;
}

struct StyleCacheKey {
  quint64 themeRevision = 0;
  AdButton::Type type = AdButton::Type::Default;
  AdButton::Color color = AdButton::Color::Default;
  AdButton::Variant variant = AdButton::Variant::Outlined;
  AdButton::Size size = AdButton::Size::Middle;
  bool colorExplicit = false;
  bool variantExplicit = false;
  bool danger = false;
  bool ghost = false;
  QString fontKey;
};

bool operator==(const StyleCacheKey& lhs, const StyleCacheKey& rhs) {
  return lhs.themeRevision == rhs.themeRevision && lhs.type == rhs.type && lhs.color == rhs.color &&
         lhs.variant == rhs.variant && lhs.size == rhs.size &&
         lhs.colorExplicit == rhs.colorExplicit && lhs.variantExplicit == rhs.variantExplicit &&
         lhs.danger == rhs.danger && lhs.ghost == rhs.ghost && lhs.fontKey == rhs.fontKey;
}

uint qHash(const StyleCacheKey& key, uint seed) {
  seed = ::qHash(key.themeRevision, seed);
  seed = ::qHash(static_cast<int>(key.type), seed);
  seed = ::qHash(static_cast<int>(key.color), seed);
  seed = ::qHash(static_cast<int>(key.variant), seed);
  seed = ::qHash(static_cast<int>(key.size), seed);
  seed = ::qHash(key.colorExplicit, seed);
  seed = ::qHash(key.variantExplicit, seed);
  seed = ::qHash(key.danger, seed);
  seed = ::qHash(key.ghost, seed);
  seed = ::qHash(key.fontKey, seed);
  return seed;
}

QHash<PresetPaletteKey, QVector<QString>>& presetPaletteCache() {
  static QHash<PresetPaletteKey, QVector<QString>> cache;
  return cache;
}

QHash<StyleCacheKey, ButtonVisualStyle>& styleCache() {
  static QHash<StyleCacheKey, ButtonVisualStyle> cache;
  return cache;
}

StyleCacheKey makeStyleCacheKey(const ButtonStyleInput& input, quint64 revision) {
  StyleCacheKey key;
  key.themeRevision = revision;
  key.type = input.type;
  key.color = input.color;
  key.variant = input.variant;
  key.size = input.size;
  key.colorExplicit = input.colorExplicit;
  key.variantExplicit = input.variantExplicit;
  key.danger = input.danger;
  key.ghost = input.ghost;
  key.fontKey = input.baseFont.key();
  return key;
}

QColor toColor(const QString& value, const QColor& fallback) {
  const adqt::theme::FastColorLite parsed(value);
  if (!parsed.isValid()) {
    return fallback;
  }

  QColor color;
  color.setRed(parsed.red());
  color.setGreen(parsed.green());
  color.setBlue(parsed.blue());
  color.setAlphaF(parsed.alpha());
  return color;
}

QColor withAlpha(const QColor& color, double alpha) {
  QColor copy = color;
  copy.setAlphaF(std::clamp(alpha, 0.0, 1.0));
  return copy;
}

bool isStableChannel(int value) { return value >= 0 && value <= 255; }

QColor resolveAlphaColor(const QColor& frontColor, const QColor& backgroundColor) {
  if (frontColor.alphaF() < 1.0) {
    return frontColor;
  }

  const int fR = frontColor.red();
  const int fG = frontColor.green();
  const int fB = frontColor.blue();

  const int bR = backgroundColor.red();
  const int bG = backgroundColor.green();
  const int bB = backgroundColor.blue();

  for (int i = 1; i <= 100; ++i) {
    const double alpha = i / 100.0;
    const int r = qRound((fR - bR * (1.0 - alpha)) / alpha);
    const int g = qRound((fG - bG * (1.0 - alpha)) / alpha);
    const int b = qRound((fB - bB * (1.0 - alpha)) / alpha);
    if (isStableChannel(r) && isStableChannel(g) && isStableChannel(b)) {
      return QColor(r, g, b, qRound(alpha * 255.0));
    }
  }

  return QColor(fR, fG, fB);
}

bool isBright(const QColor& color) {
  const int brightness = qGray(color.rgb());
  return brightness >= 186;
}

QString presetSeed(AdButton::Color color, const ThemeSeedToken& seed) {
  switch (color) {
    case AdButton::Color::Blue:
      return seed.blue;
    case AdButton::Color::Purple:
      return seed.purple;
    case AdButton::Color::Cyan:
      return seed.cyan;
    case AdButton::Color::Green:
      return seed.green;
    case AdButton::Color::Magenta:
      return seed.magenta;
    case AdButton::Color::Pink:
      return seed.pink;
    case AdButton::Color::Red:
      return seed.red;
    case AdButton::Color::Orange:
      return seed.orange;
    case AdButton::Color::Yellow:
      return seed.yellow;
    case AdButton::Color::Volcano:
      return seed.volcano;
    case AdButton::Color::Geekblue:
      return seed.geekblue;
    case AdButton::Color::Lime:
      return seed.lime;
    case AdButton::Color::Gold:
      return seed.gold;
    case AdButton::Color::Default:
    case AdButton::Color::Primary:
    case AdButton::Color::Danger:
    default:
      return QString();
  }
}

QVector<QString> toMappedDefault(const QVector<QString>& colors) {
  QVector<QString> mapped(11);
  if (colors.size() < 7) {
    return mapped;
  }
  mapped[1] = colors[0];
  mapped[2] = colors[1];
  mapped[3] = colors[2];
  mapped[4] = colors[3];
  mapped[5] = colors[4];
  mapped[6] = colors[5];
  mapped[7] = colors[6];
  mapped[8] = colors[4];
  mapped[9] = colors[5];
  mapped[10] = colors[6];
  return mapped;
}

QVector<QString> toMappedDark(const QVector<QString>& colors) {
  QVector<QString> mapped(11);
  if (colors.size() < 7) {
    return mapped;
  }
  mapped[1] = colors[0];
  mapped[2] = colors[1];
  mapped[3] = colors[2];
  mapped[4] = colors[3];
  mapped[5] = colors[6];
  mapped[6] = colors[5];
  mapped[7] = colors[4];
  mapped[8] = colors[6];
  mapped[9] = colors[5];
  mapped[10] = colors[4];
  return mapped;
}

QVector<QString> generateMappedPreset(const QString& base,
                                      bool darkTheme,
                                      const QString& backgroundColor) {
  PresetPaletteKey key;
  key.base = base;
  key.background = backgroundColor;
  key.darkTheme = darkTheme;

  auto& cache = presetPaletteCache();
  const auto cached = cache.constFind(key);
  if (cached != cache.constEnd()) {
    return cached.value();
  }

  const QVector<QString> raw =
      adqt::theme::generatePalette(base, darkTheme, backgroundColor);
  const QVector<QString> mapped = darkTheme ? toMappedDark(raw) : toMappedDefault(raw);

  if (cache.size() >= kPresetPaletteCacheMaxEntries) {
    cache.clear();
  }
  cache.insert(key, mapped);
  return mapped;
}

AdButton::Variant variantForType(AdButton::Type type) {
  switch (type) {
    case AdButton::Type::Primary:
      return AdButton::Variant::Solid;
    case AdButton::Type::Dashed:
      return AdButton::Variant::Dashed;
    case AdButton::Type::Link:
      return AdButton::Variant::Link;
    case AdButton::Type::Text:
      return AdButton::Variant::Text;
    case AdButton::Type::Default:
    default:
      return AdButton::Variant::Outlined;
  }
}

AdButton::Color colorForType(AdButton::Type type) {
  switch (type) {
    case AdButton::Type::Primary:
      return AdButton::Color::Primary;
    case AdButton::Type::Dashed:
    case AdButton::Type::Link:
    case AdButton::Type::Text:
    case AdButton::Type::Default:
    default:
      return AdButton::Color::Default;
  }
}

ColorFamily makeDefaultFamily(const ThemeMapToken& map) {
  ColorFamily family;

  family.base = toColor(map.colorBorder, QColor("#d9d9d9"));
  family.hover = toColor(map.colorPrimaryHover, QColor("#4096ff"));
  family.active = toColor(map.colorPrimaryActive, QColor("#0958d9"));
  family.light = toColor(map.colorFillTertiary, QColor("#f5f5f5"));
  family.lightHover = toColor(map.colorFillSecondary, QColor("#f0f0f0"));
  family.lightActive = toColor(map.colorFill, QColor("#d9d9d9"));

  family.outlinedText = toColor(map.colorText, QColor("#141414"));
  family.outlinedTextHover = family.hover;
  family.outlinedTextActive = family.active;

  family.filledText = toColor(map.colorText, QColor("#141414"));
  family.filledTextHover = family.filledText;
  family.filledTextActive = family.filledText;

  family.textText = toColor(map.colorText, QColor("#141414"));
  family.textTextHover = family.textText;
  family.textTextActive = family.textText;

  family.solidBg = toColor(map.colorBgSolid, QColor("#141414"));
  family.solidBgHover = toColor(map.colorBgSolidHover, QColor("#303030"));
  family.solidBgActive = toColor(map.colorBgSolidActive, QColor("#000000"));
  family.solidText = isBright(family.solidBg) ? QColor("#000000") : QColor("#ffffff");

  family.shadow = toColor(map.colorFillQuaternary, withAlpha(QColor("#000000"), 0.02));

  return family;
}

ColorFamily makePrimaryFamily(const ThemeMapToken& map) {
  ColorFamily family;
  const QColor containerBg = toColor(map.colorBgContainer, QColor("#ffffff"));

  family.base = toColor(map.colorPrimary, QColor("#1677ff"));
  family.hover = toColor(map.colorPrimaryHover, QColor("#4096ff"));
  family.active = toColor(map.colorPrimaryActive, QColor("#0958d9"));
  family.light = toColor(map.colorPrimaryBg, QColor("#e6f4ff"));
  family.lightHover = toColor(map.colorPrimaryBgHover, QColor("#bae0ff"));
  family.lightActive = toColor(map.colorPrimaryBorder, QColor("#91caff"));

  family.outlinedText = family.base;
  family.outlinedTextHover = family.hover;
  family.outlinedTextActive = family.active;

  family.filledText = family.base;
  family.filledTextHover = family.hover;
  family.filledTextActive = family.active;

  family.textText = family.base;
  family.textTextHover = family.hover;
  family.textTextActive = family.active;

  family.solidBg = family.base;
  family.solidBgHover = family.hover;
  family.solidBgActive = family.active;
  family.solidText = toColor(map.colorWhite, QColor("#ffffff"));

  family.shadow = resolveAlphaColor(toColor(map.colorPrimaryBg, QColor("#e6f4ff")), containerBg);

  return family;
}

ColorFamily makeDangerFamily(const ThemeMapToken& map) {
  ColorFamily family;
  const QColor containerBg = toColor(map.colorBgContainer, QColor("#ffffff"));

  family.base = toColor(map.colorError, QColor("#ff4d4f"));
  family.hover = toColor(map.colorErrorHover, QColor("#ff7875"));
  family.active = toColor(map.colorErrorActive, QColor("#cf1322"));
  family.light = toColor(map.colorErrorBg, QColor("#fff2f0"));
  family.lightHover = toColor(map.colorErrorBgFilledHover, QColor("#fff1f0"));
  family.lightActive = toColor(map.colorErrorBgActive, QColor("#ffccc7"));

  family.outlinedText = family.base;
  family.outlinedTextHover = family.hover;
  family.outlinedTextActive = family.active;

  family.filledText = family.base;
  family.filledTextHover = family.hover;
  family.filledTextActive = family.active;

  family.textText = family.base;
  family.textTextHover = family.hover;
  family.textTextActive = family.active;

  family.solidBg = family.base;
  family.solidBgHover = family.hover;
  family.solidBgActive = family.active;
  family.solidText = toColor(map.colorWhite, QColor("#ffffff"));

  family.shadow = resolveAlphaColor(toColor(map.colorErrorBg, QColor("#fff2f0")), containerBg);

  return family;
}

ColorFamily makePresetFamily(AdButton::Color color,
                             const ThemeMapToken& map,
                             const ThemeConfig& config) {
  const QString preset = presetSeed(color, config.seed);
  const bool darkTheme = adqt::theme::hasAlgorithm(config, ThemeAlgorithm::Dark);
  const QVector<QString> mapped = generateMappedPreset(preset, darkTheme, map.colorBgBase);
  const QColor containerBg = toColor(map.colorBgContainer, QColor("#ffffff"));

  ColorFamily family;
  family.base = toColor(mapped[6], QColor("#1677ff"));
  family.hover = toColor(mapped[5], family.base.lighter(112));
  family.active = toColor(mapped[7], family.base.darker(112));
  family.light = toColor(mapped[1], family.base.lighter(170));
  family.lightHover = toColor(mapped[2], family.base.lighter(160));
  family.lightActive = toColor(mapped[3], family.base.lighter(145));

  family.outlinedText = family.base;
  family.outlinedTextHover = family.hover;
  family.outlinedTextActive = family.active;

  family.filledText = family.base;
  family.filledTextHover = family.hover;
  family.filledTextActive = family.active;

  family.textText = family.base;
  family.textTextHover = family.hover;
  family.textTextActive = family.active;

  family.solidBg = family.base;
  family.solidBgHover = family.hover;
  family.solidBgActive = family.active;
  family.solidText = toColor(map.colorWhite, QColor("#ffffff"));

  family.shadow = resolveAlphaColor(toColor(mapped[1], family.base.lighter(170)), containerBg);

  return family;
}

ColorFamily makeFamily(AdButton::Color color, const ThemeMapToken& map, const ThemeConfig& config) {
  switch (color) {
    case AdButton::Color::Primary:
      return makePrimaryFamily(map);
    case AdButton::Color::Danger:
      return makeDangerFamily(map);
    case AdButton::Color::Blue:
    case AdButton::Color::Purple:
    case AdButton::Color::Cyan:
    case AdButton::Color::Green:
    case AdButton::Color::Magenta:
    case AdButton::Color::Pink:
    case AdButton::Color::Red:
    case AdButton::Color::Orange:
    case AdButton::Color::Yellow:
    case AdButton::Color::Volcano:
    case AdButton::Color::Geekblue:
    case AdButton::Color::Lime:
    case AdButton::Color::Gold:
      return makePresetFamily(color, map, config);
    case AdButton::Color::Default:
    default:
      return makeDefaultFamily(map);
  }
}

void applyVariantStyle(ButtonVisualStyle& style,
                       const ResolvedRole& role,
                       const ColorFamily& family,
                       const ThemeMapToken& map) {
  const QColor containerBg = toColor(map.colorBgContainer, QColor("#ffffff"));
  const QColor transparent = QColor(0, 0, 0, 0);
  const QColor linkBase = toColor(map.colorLink, family.base);
  const QColor linkHover = toColor(map.colorLinkHover, family.hover);
  const QColor linkActive = toColor(map.colorLinkActive, family.active);

  style.normal.shadow = transparent;
  style.hover.shadow = transparent;
  style.active.shadow = transparent;
  style.disabled.shadow = transparent;

  style.normal.borderStyle = role.variant == AdButton::Variant::Dashed ? Qt::DashLine : Qt::SolidLine;
  style.hover.borderStyle = style.normal.borderStyle;
  style.active.borderStyle = style.normal.borderStyle;
  style.disabled.borderStyle = style.normal.borderStyle;

  switch (role.variant) {
    case AdButton::Variant::Solid:
      style.normal.shadow = family.shadow;
      style.hover.shadow = family.shadow;
      style.active.shadow = family.shadow;

      style.normal.background = family.solidBg;
      style.hover.background = family.solidBgHover;
      style.active.background = family.solidBgActive;

      style.normal.border = transparent;
      style.hover.border = transparent;
      style.active.border = transparent;

      style.normal.text = family.solidText;
      style.hover.text = family.solidText;
      style.active.text = family.solidText;
      break;
    case AdButton::Variant::Filled:
      style.normal.background = family.light;
      style.hover.background = family.lightHover;
      style.active.background = family.lightActive;

      style.normal.border = transparent;
      style.hover.border = transparent;
      style.active.border = transparent;

      style.normal.text = family.filledText;
      style.hover.text = family.filledTextHover;
      style.active.text = family.filledTextActive;
      break;
    case AdButton::Variant::Text:
      style.normal.background = transparent;
      style.hover.background = family.light;
      style.active.background = family.lightActive;

      style.normal.border = transparent;
      style.hover.border = transparent;
      style.active.border = transparent;

      style.normal.text = family.textText;
      style.hover.text = family.textTextHover;
      style.active.text = family.textTextActive;
      break;
    case AdButton::Variant::Link:
      style.normal.background = transparent;
      style.hover.background = transparent;
      style.active.background = transparent;

      style.normal.border = transparent;
      style.hover.border = transparent;
      style.active.border = transparent;

      style.normal.text = (role.color == AdButton::Color::Default) ? linkBase : family.textText;
      style.hover.text = (role.color == AdButton::Color::Default) ? linkHover : family.textTextHover;
      style.active.text = (role.color == AdButton::Color::Default) ? linkActive : family.textTextActive;
      break;
    case AdButton::Variant::Dashed:
    case AdButton::Variant::Outlined:
    default:
      style.normal.shadow = family.shadow;
      style.hover.shadow = family.shadow;
      style.active.shadow = family.shadow;

      style.normal.background = containerBg;
      style.hover.background = containerBg;
      style.active.background = containerBg;

      style.normal.border = family.base;
      style.hover.border = family.hover;
      style.active.border = family.active;

      style.normal.text = family.outlinedText;
      style.hover.text = family.outlinedTextHover;
      style.active.text = family.outlinedTextActive;
      break;
  }
}

void applyGhostStyle(ButtonVisualStyle& style, const ResolvedRole& role, const ThemeMapToken& map) {
  if (!role.ghost) {
    return;
  }

  const QColor transparent = QColor(0, 0, 0, 0);
  const QColor defaultGhost = toColor(map.colorBgContainer, QColor("#ffffff"));

  style.normal.background = transparent;
  style.hover.background = transparent;
  style.active.background = transparent;

  style.normal.shadow = transparent;
  style.hover.shadow = transparent;
  style.active.shadow = transparent;

  if (role.variant == AdButton::Variant::Outlined || role.variant == AdButton::Variant::Dashed) {
    if (role.color == AdButton::Color::Default) {
      style.normal.text = defaultGhost;
      style.hover.text = defaultGhost;
      style.active.text = defaultGhost;
      style.normal.border = defaultGhost;
      style.hover.border = defaultGhost;
      style.active.border = defaultGhost;
    }
  }
}

void applyDisabledStyle(ButtonVisualStyle& style, const ResolvedRole& role, const ThemeMapToken& map) {
  const QColor disabledText = toColor(map.colorTextQuaternary, QColor("#bfbfbf"));
  const QColor disabledBorder = toColor(map.colorBorderDisabled, QColor("#d9d9d9"));
  const QColor disabledBg = toColor(map.colorFillTertiary, QColor("#f5f5f5"));
  const QColor transparent = QColor(0, 0, 0, 0);

  style.disabled.text = disabledText;

  if (role.variant == AdButton::Variant::Text || role.variant == AdButton::Variant::Link) {
    style.disabled.border = transparent;
    style.disabled.background = transparent;
  } else {
    style.disabled.border = disabledBorder;
    style.disabled.background = role.ghost ? transparent : disabledBg;
  }
}

ButtonMetrics resolveMetrics(const ButtonStyleInput& input, const ThemeMapToken& map) {
  ButtonMetrics metrics;
  const int lineWidth = std::max(1, qRound(map.lineWidth));
  metrics.borderWidth = lineWidth;
  metrics.shadowOffsetY = std::max(1, qRound(map.lineWidth * 2.0));
  metrics.iconGap = std::max(4, qRound(map.sizeXS));

  int fontSize = qRound(map.fontSize);

  switch (input.size) {
    case AdButton::Size::Small:
      metrics.height = std::max(20, qRound(map.controlHeightSM));
      metrics.borderRadius = std::max(0, qRound(map.borderRadiusSM));
      // Align with antd Button token defaults: contentFontSizeSM falls back to base fontSize.
      fontSize = qRound(map.fontSize);
      break;
    case AdButton::Size::Large:
      metrics.height = std::max(28, qRound(map.controlHeightLG));
      metrics.borderRadius = std::max(0, qRound(map.borderRadiusLG));
      fontSize = qRound(map.fontSizeLG);
      break;
    case AdButton::Size::Middle:
    default:
      metrics.height = std::max(24, qRound(map.controlHeight));
      metrics.borderRadius = std::max(0, qRound(map.borderRadius));
      fontSize = qRound(map.fontSize);
      break;
  }

  // Align with antd Button component token:
  // middle/large: paddingInline = sizeMS - lineWidth, small: paddingInlineSM = 8 - lineWidth.
  const int inlinePadding =
      (input.size == AdButton::Size::Small) ? qRound(8.0 - map.lineWidth)
                                            : qRound(map.sizeMS - map.lineWidth);
  metrics.horizontalPadding = std::max(0, inlinePadding);

  metrics.font = input.baseFont;
  if (fontSize > 0) {
    metrics.font.setPixelSize(fontSize);
  }

  // Align with antd focus-visible outline:
  // outline: lineWidthFocus (lineWidth * 3) solid colorPrimaryBorder; outline-offset: 1px.
  metrics.focusOutline = toColor(map.colorPrimaryBorder, QColor("#91caff"));
  metrics.focusOutlineWidth = std::max<qreal>(1.0, map.lineWidth * 3.0);
  metrics.focusOutlineOffset = 1.0;

  return metrics;
}

}  // namespace

ResolvedRole resolveRole(const ButtonStyleInput& input) {
  ResolvedRole role;

  const AdButton::Color typeColor = colorForType(input.type);
  const AdButton::Variant typeVariant = variantForType(input.type);

  role.color = input.colorExplicit ? input.color : typeColor;
  role.variant = input.variantExplicit ? input.variant : typeVariant;

  if (input.danger && !input.colorExplicit) {
    role.color = AdButton::Color::Danger;
  }

  if (input.ghost && role.variant == AdButton::Variant::Solid) {
    role.variant = AdButton::Variant::Outlined;
  }

  role.unbordered = role.variant == AdButton::Variant::Text || role.variant == AdButton::Variant::Link;
  role.ghost = input.ghost && !role.unbordered;

  return role;
}

ButtonVisualStyle resolveButtonVisualStyle(const ButtonStyleInput& input) {
  const ThemeManager& manager = ThemeManager::instance();
  const quint64 revision = manager.themeRevision();
  const StyleCacheKey cacheKey = makeStyleCacheKey(input, revision);
  auto& cache = styleCache();

  const auto cached = cache.constFind(cacheKey);
  if (cached != cache.constEnd()) {
    return cached.value();
  }

  ButtonVisualStyle style;
  const ThemeMapToken& map = manager.currentMapToken();
  const ThemeConfig& config = manager.currentConfig();

  style.role = resolveRole(input);
  style.metrics = resolveMetrics(input, map);

  const ColorFamily family = makeFamily(style.role.color, map, config);
  applyVariantStyle(style, style.role, family, map);
  applyGhostStyle(style, style.role, map);
  applyDisabledStyle(style, style.role, map);

  if (cache.size() >= kStyleCacheMaxEntries) {
    cache.clear();
  }
  cache.insert(cacheKey, style);

  return style;
}

}  // namespace adqt::widgets::detail
