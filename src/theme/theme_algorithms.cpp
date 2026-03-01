#include "theme_algorithms.h"

#include "fast_color_lite.h"
#include "palette_generate.h"

#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <algorithm>
#include <array>
#include <cmath>

namespace adqt::theme {

namespace {

QString resolveColor(const QString& color, const QString& fallback) {
  FastColorLite parsed(color);
  if (!parsed.isValid()) {
    return FastColorLite(fallback).toHexString();
  }
  return parsed.toHexString();
}

QString alphaColor(const QString& baseColor, double alpha) {
  return FastColorLite(baseColor).setAlpha(alpha).toRgbString();
}

QString solidColorDefault(const QString& baseColor, double brightness) {
  return FastColorLite(baseColor).darken(brightness).toHexString();
}

QString solidColorDark(const QString& baseColor, double brightness) {
  return FastColorLite(baseColor).lighten(brightness).toHexString();
}

QVector<QString> toMappedDefault(const QVector<QString>& colors) {
  QVector<QString> mapped(11);
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

QVector<QString> generateColorPalettes(const QString& baseColor, bool darkTheme) {
  const QString safe = resolveColor(baseColor, "#1677ff");
  const QVector<QString> generated = generatePalette(safe, darkTheme);
  return darkTheme ? toMappedDark(generated) : toMappedDefault(generated);
}

void applyNeutralDefault(const ThemeSeedToken& seed, ThemeMapToken& map) {
  const QString colorBgBase = seed.colorBgBase.isEmpty() ? "#fff" : resolveColor(seed.colorBgBase, "#fff");
  const QString colorTextBase =
      seed.colorTextBase.isEmpty() ? "#000" : resolveColor(seed.colorTextBase, "#000");

  map.colorBgBase = colorBgBase;
  map.colorTextBase = colorTextBase;

  map.colorText = alphaColor(colorTextBase, 0.88);
  map.colorTextSecondary = alphaColor(colorTextBase, 0.65);
  map.colorTextTertiary = alphaColor(colorTextBase, 0.45);
  map.colorTextQuaternary = alphaColor(colorTextBase, 0.25);

  map.colorFill = alphaColor(colorTextBase, 0.15);
  map.colorFillSecondary = alphaColor(colorTextBase, 0.06);
  map.colorFillTertiary = alphaColor(colorTextBase, 0.04);
  map.colorFillQuaternary = alphaColor(colorTextBase, 0.02);

  map.colorBgSolid = alphaColor(colorTextBase, 1.0);
  map.colorBgSolidHover = alphaColor(colorTextBase, 0.75);
  map.colorBgSolidActive = alphaColor(colorTextBase, 0.95);

  map.colorBgLayout = solidColorDefault(colorBgBase, 4.0);
  map.colorBgContainer = solidColorDefault(colorBgBase, 0.0);
  map.colorBgElevated = solidColorDefault(colorBgBase, 0.0);
  map.colorBgSpotlight = alphaColor(colorTextBase, 0.85);
  map.colorBgBlur = "transparent";

  map.colorBorder = solidColorDefault(colorBgBase, 15.0);
  map.colorBorderDisabled = solidColorDefault(colorBgBase, 15.0);
  map.colorBorderSecondary = solidColorDefault(colorBgBase, 6.0);
}

void applyNeutralDark(const ThemeSeedToken& seed, ThemeMapToken& map) {
  const QString colorBgBase = seed.colorBgBase.isEmpty() ? "#000" : resolveColor(seed.colorBgBase, "#000");
  const QString colorTextBase =
      seed.colorTextBase.isEmpty() ? "#fff" : resolveColor(seed.colorTextBase, "#fff");

  map.colorBgBase = colorBgBase;
  map.colorTextBase = colorTextBase;

  map.colorText = alphaColor(colorTextBase, 0.85);
  map.colorTextSecondary = alphaColor(colorTextBase, 0.65);
  map.colorTextTertiary = alphaColor(colorTextBase, 0.45);
  map.colorTextQuaternary = alphaColor(colorTextBase, 0.25);

  map.colorFill = alphaColor(colorTextBase, 0.18);
  map.colorFillSecondary = alphaColor(colorTextBase, 0.12);
  map.colorFillTertiary = alphaColor(colorTextBase, 0.08);
  map.colorFillQuaternary = alphaColor(colorTextBase, 0.04);

  map.colorBgSolid = alphaColor(colorTextBase, 0.95);
  map.colorBgSolidHover = alphaColor(colorTextBase, 1.0);
  map.colorBgSolidActive = alphaColor(colorTextBase, 0.9);

  map.colorBgElevated = solidColorDark(colorBgBase, 12.0);
  map.colorBgContainer = solidColorDark(colorBgBase, 8.0);
  map.colorBgLayout = solidColorDark(colorBgBase, 0.0);
  map.colorBgSpotlight = solidColorDark(colorBgBase, 26.0);
  map.colorBgBlur = alphaColor(colorTextBase, 0.04);

  map.colorBorder = solidColorDark(colorBgBase, 26.0);
  map.colorBorderDisabled = solidColorDark(colorBgBase, 26.0);
  map.colorBorderSecondary = solidColorDark(colorBgBase, 19.0);
}

void applyCoreColorMap(const ThemeSeedToken& seed, ThemeMapToken& map, bool darkTheme) {
  const QVector<QString> primary = generateColorPalettes(seed.colorPrimary, darkTheme);
  const QVector<QString> success = generateColorPalettes(seed.colorSuccess, darkTheme);
  const QVector<QString> warning = generateColorPalettes(seed.colorWarning, darkTheme);
  const QVector<QString> error = generateColorPalettes(seed.colorError, darkTheme);
  const QVector<QString> info = generateColorPalettes(seed.colorInfo, darkTheme);

  if (darkTheme) {
    applyNeutralDark(seed, map);
  } else {
    applyNeutralDefault(seed, map);
  }

  const QString linkBase = seed.colorLink.isEmpty() ? seed.colorInfo : seed.colorLink;
  const QVector<QString> link = generateColorPalettes(linkBase, darkTheme);

  map.colorPrimaryBg = primary[1];
  map.colorPrimaryBgHover = primary[2];
  map.colorPrimaryBorder = primary[3];
  map.colorPrimaryBorderHover = primary[4];
  map.colorPrimaryHover = primary[5];
  map.colorPrimary = primary[6];
  map.colorPrimaryActive = primary[7];
  map.colorPrimaryTextHover = primary[8];
  map.colorPrimaryText = primary[9];
  map.colorPrimaryTextActive = primary[10];

  map.colorSuccessBg = success[1];
  map.colorSuccessBgHover = success[2];
  map.colorSuccessBorder = success[3];
  map.colorSuccessBorderHover = success[4];
  map.colorSuccessHover = success[4];
  map.colorSuccess = success[6];
  map.colorSuccessActive = success[7];
  map.colorSuccessTextHover = success[8];
  map.colorSuccessText = success[9];
  map.colorSuccessTextActive = success[10];

  map.colorErrorBg = error[1];
  map.colorErrorBgHover = error[2];
  map.colorErrorBgFilledHover = FastColorLite(error[1]).mix(FastColorLite(error[3]), 50).toHexString();
  map.colorErrorBgActive = error[3];
  map.colorErrorBorder = error[3];
  map.colorErrorBorderHover = error[4];
  map.colorErrorHover = error[5];
  map.colorError = error[6];
  map.colorErrorActive = error[7];
  map.colorErrorTextHover = error[8];
  map.colorErrorText = error[9];
  map.colorErrorTextActive = error[10];

  map.colorWarningBg = warning[1];
  map.colorWarningBgHover = warning[2];
  map.colorWarningBorder = warning[3];
  map.colorWarningBorderHover = warning[4];
  map.colorWarningHover = warning[4];
  map.colorWarning = warning[6];
  map.colorWarningActive = warning[7];
  map.colorWarningTextHover = warning[8];
  map.colorWarningText = warning[9];
  map.colorWarningTextActive = warning[10];

  map.colorInfoBg = info[1];
  map.colorInfoBgHover = info[2];
  map.colorInfoBorder = info[3];
  map.colorInfoBorderHover = info[4];
  map.colorInfoHover = info[4];
  map.colorInfo = info[6];
  map.colorInfoActive = info[7];
  map.colorInfoTextHover = info[8];
  map.colorInfoText = info[9];
  map.colorInfoTextActive = info[10];

  map.colorLinkHover = link[4];
  map.colorLink = link[6];
  map.colorLinkActive = link[7];

  map.colorBgMask = FastColorLite("#000").setAlpha(0.45).toRgbString();
  map.colorWhite = "#fff";
}

struct RadiusValues {
  double borderRadius;
  double borderRadiusXS;
  double borderRadiusSM;
  double borderRadiusLG;
  double borderRadiusOuter;
};

RadiusValues genRadius(double radiusBase) {
  double radiusLG = radiusBase;
  double radiusSM = radiusBase;
  double radiusXS = radiusBase;
  double radiusOuter = radiusBase;

  if (radiusBase < 6.0 && radiusBase >= 5.0) {
    radiusLG = radiusBase + 1.0;
  } else if (radiusBase < 16.0 && radiusBase >= 6.0) {
    radiusLG = radiusBase + 2.0;
  } else if (radiusBase >= 16.0) {
    radiusLG = 16.0;
  }

  if (radiusBase < 7.0 && radiusBase >= 5.0) {
    radiusSM = 4.0;
  } else if (radiusBase < 8.0 && radiusBase >= 7.0) {
    radiusSM = 5.0;
  } else if (radiusBase < 14.0 && radiusBase >= 8.0) {
    radiusSM = 6.0;
  } else if (radiusBase < 16.0 && radiusBase >= 14.0) {
    radiusSM = 7.0;
  } else if (radiusBase >= 16.0) {
    radiusSM = 8.0;
  }

  if (radiusBase < 6.0 && radiusBase >= 2.0) {
    radiusXS = 1.0;
  } else if (radiusBase >= 6.0) {
    radiusXS = 2.0;
  }

  if (radiusBase > 4.0 && radiusBase < 8.0) {
    radiusOuter = 4.0;
  } else if (radiusBase >= 8.0) {
    radiusOuter = 6.0;
  }

  return RadiusValues{radiusBase, radiusXS, radiusSM, radiusLG, radiusOuter};
}

double lineHeightFromSize(double fontSize) {
  return (fontSize + 8.0) / fontSize;
}

void applyFontMap(double baseFontSize, ThemeMapToken& map) {
  std::array<double, 10> fontSizes{};
  std::array<double, 10> lineHeights{};

  for (int index = 0; index < 10; ++index) {
    const int i = index - 1;
    const double baseSize = baseFontSize * std::exp(i / 5.0);
    const double intSize = index > 1 ? std::floor(baseSize) : std::ceil(baseSize);
    const double evenSize = std::floor(intSize / 2.0) * 2.0;
    fontSizes[index] = evenSize;
  }

  fontSizes[1] = baseFontSize;

  for (int i = 0; i < 10; ++i) {
    lineHeights[i] = lineHeightFromSize(fontSizes[i]);
  }

  map.fontSizeSM = fontSizes[0];
  map.fontSize = fontSizes[1];
  map.fontSizeLG = fontSizes[2];
  map.fontSizeXL = fontSizes[3];

  map.fontSizeHeading1 = fontSizes[6];
  map.fontSizeHeading2 = fontSizes[5];
  map.fontSizeHeading3 = fontSizes[4];
  map.fontSizeHeading4 = fontSizes[3];
  map.fontSizeHeading5 = fontSizes[2];

  map.lineHeight = lineHeights[1];
  map.lineHeightLG = lineHeights[2];
  map.lineHeightSM = lineHeights[0];

  map.fontHeight = std::round(map.lineHeight * map.fontSize);
  map.fontHeightLG = std::round(map.lineHeightLG * map.fontSizeLG);
  map.fontHeightSM = std::round(map.lineHeightSM * map.fontSizeSM);

  map.lineHeightHeading1 = lineHeights[6];
  map.lineHeightHeading2 = lineHeights[5];
  map.lineHeightHeading3 = lineHeights[4];
  map.lineHeightHeading4 = lineHeights[3];
  map.lineHeightHeading5 = lineHeights[2];
}

void applyDefaultSizeMap(const ThemeSeedToken& seed, ThemeMapToken& map) {
  const double u = seed.sizeUnit;
  const double step = seed.sizeStep;

  map.sizeXXL = u * (step + 8.0);
  map.sizeXL = u * (step + 4.0);
  map.sizeLG = u * (step + 2.0);
  map.sizeMD = u * (step + 1.0);
  map.sizeMS = u * step;
  map.size = u * step;
  map.sizeSM = u * (step - 1.0);
  map.sizeXS = u * (step - 2.0);
  map.sizeXXS = u * (step - 3.0);
}

void applyCompactSizeMap(double sizeUnit, double sizeStep, ThemeMapToken& map) {
  const double compactStep = sizeStep - 2.0;

  map.sizeXXL = sizeUnit * (compactStep + 10.0);
  map.sizeXL = sizeUnit * (compactStep + 6.0);
  map.sizeLG = sizeUnit * (compactStep + 2.0);
  map.sizeMD = sizeUnit * (compactStep + 2.0);
  map.sizeMS = sizeUnit * (compactStep + 1.0);
  map.size = sizeUnit * compactStep;
  map.sizeSM = sizeUnit * compactStep;
  map.sizeXS = sizeUnit * (compactStep - 1.0);
  map.sizeXXS = sizeUnit * (compactStep - 1.0);
}

void applyControlHeight(double controlHeight, ThemeMapToken& map) {
  map.controlHeight = controlHeight;
  map.controlHeightSM = controlHeight * 0.75;
  map.controlHeightXS = controlHeight * 0.5;
  map.controlHeightLG = controlHeight * 1.25;
}

void applyCommonMap(const ThemeSeedToken& seed, ThemeMapToken& map) {
  map.motionDurationFast = QString::number(seed.motionBase + seed.motionUnit, 'f', 1) + "s";
  map.motionDurationMid = QString::number(seed.motionBase + seed.motionUnit * 2.0, 'f', 1) + "s";
  map.motionDurationSlow = QString::number(seed.motionBase + seed.motionUnit * 3.0, 'f', 1) + "s";

  map.lineWidthBold = seed.lineWidth + 1.0;

  const RadiusValues radius = genRadius(seed.borderRadius);
  map.borderRadius = radius.borderRadius;
  map.borderRadiusXS = radius.borderRadiusXS;
  map.borderRadiusSM = radius.borderRadiusSM;
  map.borderRadiusLG = radius.borderRadiusLG;
  map.borderRadiusOuter = radius.borderRadiusOuter;

  if (!seed.motion) {
    map.motionDurationFast = "0s";
    map.motionDurationMid = "0s";
    map.motionDurationSlow = "0s";
  }
}

ThemeMapToken computeDefaultMapToken(const ThemeSeedToken& seed) {
  ThemeMapToken map;

  map.lineWidth = seed.lineWidth;
  map.lineType = seed.lineType;
  map.controlHeight = seed.controlHeight;
  map.sizeUnit = seed.sizeUnit;
  map.sizeStep = seed.sizeStep;
  map.motion = seed.motion;

  applyCoreColorMap(seed, map, false);
  applyFontMap(seed.fontSize, map);
  applyDefaultSizeMap(seed, map);
  applyControlHeight(seed.controlHeight, map);
  applyCommonMap(seed, map);

  return map;
}

ThemeMapToken applyDarkAlgorithm(const ThemeSeedToken& seed, const ThemeMapToken& baseMap) {
  ThemeMapToken map = baseMap;

  ThemeMapToken darkMap = map;
  applyCoreColorMap(seed, darkMap, true);

  map.colorBgBase = darkMap.colorBgBase;
  map.colorTextBase = darkMap.colorTextBase;
  map.colorText = darkMap.colorText;
  map.colorTextSecondary = darkMap.colorTextSecondary;
  map.colorTextTertiary = darkMap.colorTextTertiary;
  map.colorTextQuaternary = darkMap.colorTextQuaternary;
  map.colorFill = darkMap.colorFill;
  map.colorFillSecondary = darkMap.colorFillSecondary;
  map.colorFillTertiary = darkMap.colorFillTertiary;
  map.colorFillQuaternary = darkMap.colorFillQuaternary;
  map.colorBgSolid = darkMap.colorBgSolid;
  map.colorBgSolidHover = darkMap.colorBgSolidHover;
  map.colorBgSolidActive = darkMap.colorBgSolidActive;
  map.colorBgLayout = darkMap.colorBgLayout;
  map.colorBgContainer = darkMap.colorBgContainer;
  map.colorBgElevated = darkMap.colorBgElevated;
  map.colorBgSpotlight = darkMap.colorBgSpotlight;
  map.colorBgBlur = darkMap.colorBgBlur;
  map.colorBorder = darkMap.colorBorder;
  map.colorBorderDisabled = darkMap.colorBorderDisabled;
  map.colorBorderSecondary = darkMap.colorBorderSecondary;

  map.colorPrimaryBg = darkMap.colorPrimaryBorder;
  map.colorPrimaryBgHover = darkMap.colorPrimaryBorderHover;
  map.colorPrimaryBorder = darkMap.colorPrimaryBorder;
  map.colorPrimaryBorderHover = darkMap.colorPrimaryBorderHover;
  map.colorPrimaryHover = darkMap.colorPrimaryHover;
  map.colorPrimary = darkMap.colorPrimary;
  map.colorPrimaryActive = darkMap.colorPrimaryActive;
  map.colorPrimaryTextHover = darkMap.colorPrimaryTextHover;
  map.colorPrimaryText = darkMap.colorPrimaryText;
  map.colorPrimaryTextActive = darkMap.colorPrimaryTextActive;

  map.colorSuccessBg = darkMap.colorSuccessBg;
  map.colorSuccessBgHover = darkMap.colorSuccessBgHover;
  map.colorSuccessBorder = darkMap.colorSuccessBorder;
  map.colorSuccessBorderHover = darkMap.colorSuccessBorderHover;
  map.colorSuccessHover = darkMap.colorSuccessHover;
  map.colorSuccess = darkMap.colorSuccess;
  map.colorSuccessActive = darkMap.colorSuccessActive;
  map.colorSuccessTextHover = darkMap.colorSuccessTextHover;
  map.colorSuccessText = darkMap.colorSuccessText;
  map.colorSuccessTextActive = darkMap.colorSuccessTextActive;

  map.colorErrorBg = darkMap.colorErrorBg;
  map.colorErrorBgHover = darkMap.colorErrorBgHover;
  map.colorErrorBgFilledHover = darkMap.colorErrorBgFilledHover;
  map.colorErrorBgActive = darkMap.colorErrorBgActive;
  map.colorErrorBorder = darkMap.colorErrorBorder;
  map.colorErrorBorderHover = darkMap.colorErrorBorderHover;
  map.colorErrorHover = darkMap.colorErrorHover;
  map.colorError = darkMap.colorError;
  map.colorErrorActive = darkMap.colorErrorActive;
  map.colorErrorTextHover = darkMap.colorErrorTextHover;
  map.colorErrorText = darkMap.colorErrorText;
  map.colorErrorTextActive = darkMap.colorErrorTextActive;

  map.colorWarningBg = darkMap.colorWarningBg;
  map.colorWarningBgHover = darkMap.colorWarningBgHover;
  map.colorWarningBorder = darkMap.colorWarningBorder;
  map.colorWarningBorderHover = darkMap.colorWarningBorderHover;
  map.colorWarningHover = darkMap.colorWarningHover;
  map.colorWarning = darkMap.colorWarning;
  map.colorWarningActive = darkMap.colorWarningActive;
  map.colorWarningTextHover = darkMap.colorWarningTextHover;
  map.colorWarningText = darkMap.colorWarningText;
  map.colorWarningTextActive = darkMap.colorWarningTextActive;

  map.colorInfoBg = darkMap.colorInfoBg;
  map.colorInfoBgHover = darkMap.colorInfoBgHover;
  map.colorInfoBorder = darkMap.colorInfoBorder;
  map.colorInfoBorderHover = darkMap.colorInfoBorderHover;
  map.colorInfoHover = darkMap.colorInfoHover;
  map.colorInfo = darkMap.colorInfo;
  map.colorInfoActive = darkMap.colorInfoActive;
  map.colorInfoTextHover = darkMap.colorInfoTextHover;
  map.colorInfoText = darkMap.colorInfoText;
  map.colorInfoTextActive = darkMap.colorInfoTextActive;

  map.colorLinkHover = darkMap.colorLinkHover;
  map.colorLink = darkMap.colorLink;
  map.colorLinkActive = darkMap.colorLinkActive;

  map.colorBgMask = darkMap.colorBgMask;
  map.colorWhite = darkMap.colorWhite;

  return map;
}

ThemeMapToken applyCompactAlgorithm(const ThemeSeedToken& seed, const ThemeMapToken& baseMap) {
  ThemeMapToken map = baseMap;

  applyCompactSizeMap(seed.sizeUnit, seed.sizeStep, map);

  const double compactFontBase = baseMap.fontSizeSM;
  applyFontMap(compactFontBase, map);

  const double compactControlHeight = baseMap.controlHeight - 4.0;
  applyControlHeight(compactControlHeight, map);

  return map;
}

QJsonObject seedToJson(const ThemeSeedToken& seed) {
  QJsonObject obj;

  obj["blue"] = seed.blue;
  obj["purple"] = seed.purple;
  obj["cyan"] = seed.cyan;
  obj["green"] = seed.green;
  obj["magenta"] = seed.magenta;
  obj["pink"] = seed.pink;
  obj["red"] = seed.red;
  obj["orange"] = seed.orange;
  obj["yellow"] = seed.yellow;
  obj["volcano"] = seed.volcano;
  obj["geekblue"] = seed.geekblue;
  obj["gold"] = seed.gold;
  obj["lime"] = seed.lime;

  obj["colorPrimary"] = seed.colorPrimary;
  obj["colorSuccess"] = seed.colorSuccess;
  obj["colorWarning"] = seed.colorWarning;
  obj["colorError"] = seed.colorError;
  obj["colorInfo"] = seed.colorInfo;
  obj["colorLink"] = seed.colorLink;
  obj["colorTextBase"] = seed.colorTextBase;
  obj["colorBgBase"] = seed.colorBgBase;

  obj["fontFamily"] = seed.fontFamily;
  obj["fontFamilyCode"] = seed.fontFamilyCode;
  obj["fontSize"] = seed.fontSize;

  obj["lineWidth"] = seed.lineWidth;
  obj["lineType"] = seed.lineType;

  obj["motionUnit"] = seed.motionUnit;
  obj["motionBase"] = seed.motionBase;
  obj["motionEaseOutCirc"] = seed.motionEaseOutCirc;
  obj["motionEaseInOutCirc"] = seed.motionEaseInOutCirc;
  obj["motionEaseOut"] = seed.motionEaseOut;
  obj["motionEaseInOut"] = seed.motionEaseInOut;
  obj["motionEaseOutBack"] = seed.motionEaseOutBack;
  obj["motionEaseInBack"] = seed.motionEaseInBack;
  obj["motionEaseInQuint"] = seed.motionEaseInQuint;
  obj["motionEaseOutQuint"] = seed.motionEaseOutQuint;

  obj["borderRadius"] = seed.borderRadius;
  obj["sizeUnit"] = seed.sizeUnit;
  obj["sizeStep"] = seed.sizeStep;
  obj["sizePopupArrow"] = seed.sizePopupArrow;
  obj["controlHeight"] = seed.controlHeight;
  obj["zIndexBase"] = seed.zIndexBase;
  obj["zIndexPopupBase"] = seed.zIndexPopupBase;
  obj["opacityImage"] = seed.opacityImage;
  obj["wireframe"] = seed.wireframe;
  obj["motion"] = seed.motion;

  return obj;
}

}  // namespace

ThemeConfig normalizeThemeConfig(const ThemeConfig& config) {
  ThemeConfig normalized = config;
  if (normalized.algorithms.isEmpty()) {
    normalized.algorithms = {ThemeAlgorithm::Default};
  }
  return normalized;
}

bool hasAlgorithm(const ThemeConfig& config, ThemeAlgorithm algorithm) {
  const ThemeConfig normalized = normalizeThemeConfig(config);
  return normalized.algorithms.contains(algorithm);
}

ThemeMapToken computeMapToken(const ThemeConfig& config) {
  const ThemeConfig normalized = normalizeThemeConfig(config);

  ThemeMapToken map = computeDefaultMapToken(normalized.seed);

  if (normalized.algorithms.contains(ThemeAlgorithm::Dark)) {
    map = applyDarkAlgorithm(normalized.seed, map);
  }

  if (normalized.algorithms.contains(ThemeAlgorithm::Compact)) {
    map = applyCompactAlgorithm(normalized.seed, map);
  }

  return map;
}

GlobalPaletteToken toGlobalPaletteToken(const ThemeMapToken& mapToken) {
  GlobalPaletteToken token;

  token.colorBgLayout = mapToken.colorBgLayout;
  token.colorBgContainer = mapToken.colorBgContainer;
  token.colorBgContainerDisabled = mapToken.colorFillTertiary;
  token.colorFillAlter = mapToken.colorFillQuaternary;
  token.colorBgElevated = mapToken.colorBgElevated;

  token.colorText = mapToken.colorText;
  token.colorTextDisabled = mapToken.colorTextQuaternary;
  token.colorTextPlaceholder = mapToken.colorTextQuaternary;
  token.colorTextLightSolid = mapToken.colorWhite;

  token.colorPrimary = mapToken.colorPrimary;
  token.colorPrimaryHover = mapToken.colorPrimaryHover;
  token.colorPrimaryActive = mapToken.colorPrimaryActive;

  token.colorLink = mapToken.colorLink;
  token.colorLinkHover = mapToken.colorLinkHover;
  token.colorLinkActive = mapToken.colorLinkActive;

  return token;
}

QByteArray themeConfigKey(const ThemeConfig& config) {
  const ThemeConfig normalized = normalizeThemeConfig(config);

  QJsonObject obj;
  obj["seed"] = seedToJson(normalized.seed);

  QStringList algorithms;
  algorithms.reserve(normalized.algorithms.size());
  for (ThemeAlgorithm algorithm : normalized.algorithms) {
    algorithms.append(algorithmName(algorithm));
  }
  std::sort(algorithms.begin(), algorithms.end());

  QJsonArray arr;
  for (const QString& algorithm : algorithms) {
    arr.append(algorithm);
  }
  obj["algorithms"] = arr;

  const QByteArray json = QJsonDocument(obj).toJson(QJsonDocument::Compact);
  return QCryptographicHash::hash(json, QCryptographicHash::Sha256).toHex();
}

}  // namespace adqt::theme
