#pragma once

#include <QColor>
#include <QSet>
#include <QString>

namespace adqt::theme {

enum class ThemeAlgorithm {
  Default,
  Dark,
  Compact,
};

QString algorithmName(ThemeAlgorithm algorithm);

struct ThemeSeedToken {
  QString blue;
  QString purple;
  QString cyan;
  QString green;
  QString magenta;
  QString pink;
  QString red;
  QString orange;
  QString yellow;
  QString volcano;
  QString geekblue;
  QString gold;
  QString lime;

  QString colorPrimary;
  QString colorSuccess;
  QString colorWarning;
  QString colorError;
  QString colorInfo;
  QString colorLink;
  QString colorTextBase;
  QString colorBgBase;

  QString fontFamily;
  QString fontFamilyCode;
  double fontSize;

  double lineWidth;
  QString lineType;

  double motionUnit;
  double motionBase;
  QString motionEaseOutCirc;
  QString motionEaseInOutCirc;
  QString motionEaseOut;
  QString motionEaseInOut;
  QString motionEaseOutBack;
  QString motionEaseInBack;
  QString motionEaseInQuint;
  QString motionEaseOutQuint;

  double borderRadius;

  double sizeUnit;
  double sizeStep;
  double sizePopupArrow;

  double controlHeight;

  double zIndexBase;
  double zIndexPopupBase;

  double opacityImage;

  bool wireframe;
  bool motion;
};

struct ThemeConfig {
  ThemeSeedToken seed;
  QSet<ThemeAlgorithm> algorithms;
};

struct ThemeMapToken {
  QString colorBgBase;
  QString colorTextBase;

  QString colorText;
  QString colorTextSecondary;
  QString colorTextTertiary;
  QString colorTextQuaternary;

  QString colorFill;
  QString colorFillSecondary;
  QString colorFillTertiary;
  QString colorFillQuaternary;

  QString colorBgSolid;
  QString colorBgSolidHover;
  QString colorBgSolidActive;

  QString colorBgLayout;
  QString colorBgContainer;
  QString colorBgElevated;
  QString colorBgSpotlight;
  QString colorBgBlur;

  QString colorBorder;
  QString colorBorderDisabled;
  QString colorBorderSecondary;

  QString colorPrimaryBg;
  QString colorPrimaryBgHover;
  QString colorPrimaryBorder;
  QString colorPrimaryBorderHover;
  QString colorPrimaryHover;
  QString colorPrimary;
  QString colorPrimaryActive;
  QString colorPrimaryTextHover;
  QString colorPrimaryText;
  QString colorPrimaryTextActive;

  QString colorSuccessBg;
  QString colorSuccessBgHover;
  QString colorSuccessBorder;
  QString colorSuccessBorderHover;
  QString colorSuccessHover;
  QString colorSuccess;
  QString colorSuccessActive;
  QString colorSuccessTextHover;
  QString colorSuccessText;
  QString colorSuccessTextActive;

  QString colorErrorBg;
  QString colorErrorBgHover;
  QString colorErrorBgFilledHover;
  QString colorErrorBgActive;
  QString colorErrorBorder;
  QString colorErrorBorderHover;
  QString colorErrorHover;
  QString colorError;
  QString colorErrorActive;
  QString colorErrorTextHover;
  QString colorErrorText;
  QString colorErrorTextActive;

  QString colorWarningBg;
  QString colorWarningBgHover;
  QString colorWarningBorder;
  QString colorWarningBorderHover;
  QString colorWarningHover;
  QString colorWarning;
  QString colorWarningActive;
  QString colorWarningTextHover;
  QString colorWarningText;
  QString colorWarningTextActive;

  QString colorInfoBg;
  QString colorInfoBgHover;
  QString colorInfoBorder;
  QString colorInfoBorderHover;
  QString colorInfoHover;
  QString colorInfo;
  QString colorInfoActive;
  QString colorInfoTextHover;
  QString colorInfoText;
  QString colorInfoTextActive;

  QString colorLinkHover;
  QString colorLink;
  QString colorLinkActive;

  QString colorBgMask;
  QString colorWhite;

  double sizeXXL;
  double sizeXL;
  double sizeLG;
  double sizeMD;
  double sizeMS;
  double size;
  double sizeSM;
  double sizeXS;
  double sizeXXS;

  double fontSizeSM;
  double fontSize;
  double fontSizeLG;
  double fontSizeXL;
  double fontSizeHeading1;
  double fontSizeHeading2;
  double fontSizeHeading3;
  double fontSizeHeading4;
  double fontSizeHeading5;

  double lineHeight;
  double lineHeightLG;
  double lineHeightSM;
  double lineHeightHeading1;
  double lineHeightHeading2;
  double lineHeightHeading3;
  double lineHeightHeading4;
  double lineHeightHeading5;

  double fontHeight;
  double fontHeightLG;
  double fontHeightSM;

  QString motionDurationFast;
  QString motionDurationMid;
  QString motionDurationSlow;

  double lineWidth;
  double lineWidthBold;
  QString lineType;

  double borderRadius;
  double borderRadiusXS;
  double borderRadiusSM;
  double borderRadiusLG;
  double borderRadiusOuter;

  double controlHeight;
  double controlHeightSM;
  double controlHeightXS;
  double controlHeightLG;

  double sizeUnit;
  double sizeStep;

  bool motion;
};

struct GlobalPaletteToken {
  QString colorBgLayout;
  QString colorBgContainer;
  QString colorBgContainerDisabled;
  QString colorFillAlter;
  QString colorBgElevated;

  QString colorText;
  QString colorTextDisabled;
  QString colorTextPlaceholder;
  QString colorTextLightSolid;

  QString colorPrimary;
  QString colorPrimaryHover;
  QString colorPrimaryActive;

  QString colorLink;
  QString colorLinkHover;
  QString colorLinkActive;
};

ThemeSeedToken defaultSeedToken();
ThemeConfig defaultThemeConfig();

bool operator==(const ThemeSeedToken& lhs, const ThemeSeedToken& rhs);
bool operator==(const ThemeConfig& lhs, const ThemeConfig& rhs);

}  // namespace adqt::theme
