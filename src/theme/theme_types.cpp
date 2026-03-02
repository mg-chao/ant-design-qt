#include "theme_types.h"

namespace adqt::theme {

QString algorithmName(ThemeAlgorithm algorithm) {
  switch (algorithm) {
    case ThemeAlgorithm::Default:
      return "default";
    case ThemeAlgorithm::Dark:
      return "dark";
    case ThemeAlgorithm::Compact:
      return "compact";
  }

  return "default";
}

ThemeSeedToken defaultSeedToken() {
  ThemeSeedToken token;

  token.blue = "#1677FF";
  token.purple = "#722ED1";
  token.cyan = "#13C2C2";
  token.green = "#52C41A";
  token.magenta = "#EB2F96";
  token.pink = "#EB2F96";
  token.red = "#F5222D";
  token.orange = "#FA8C16";
  token.yellow = "#FADB14";
  token.volcano = "#FA541C";
  token.geekblue = "#2F54EB";
  token.gold = "#FAAD14";
  token.lime = "#A0D911";

  token.colorPrimary = "#1677ff";
  token.colorSuccess = "#52c41a";
  token.colorWarning = "#faad14";
  token.colorError = "#ff4d4f";
  token.colorInfo = "#1677ff";
  token.colorLink = "";
  token.colorTextBase = "";
  token.colorBgBase = "";

  token.fontFamily =
      "-apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, "
      "'Noto Sans', sans-serif, 'Apple Color Emoji', 'Segoe UI Emoji', 'Segoe UI Symbol', "
      "'Noto Color Emoji'";
  token.fontFamilyCode = "'SFMono-Regular', Consolas, 'Liberation Mono', Menlo, Courier, monospace";
  token.fontSize = 14.0;

  token.lineWidth = 1.0;
  token.lineType = "solid";

  token.motionUnit = 0.1;
  token.motionBase = 0.0;
  token.motionEaseOutCirc = "cubic-bezier(0.08, 0.82, 0.17, 1)";
  token.motionEaseInOutCirc = "cubic-bezier(0.78, 0.14, 0.15, 0.86)";
  token.motionEaseOut = "cubic-bezier(0.215, 0.61, 0.355, 1)";
  token.motionEaseInOut = "cubic-bezier(0.645, 0.045, 0.355, 1)";
  token.motionEaseOutBack = "cubic-bezier(0.12, 0.4, 0.29, 1.46)";
  token.motionEaseInBack = "cubic-bezier(0.71, -0.46, 0.88, 0.6)";
  token.motionEaseInQuint = "cubic-bezier(0.755, 0.05, 0.855, 0.06)";
  token.motionEaseOutQuint = "cubic-bezier(0.23, 1, 0.32, 1)";

  token.borderRadius = 6.0;

  token.sizeUnit = 4.0;
  token.sizeStep = 4.0;
  token.sizePopupArrow = 16.0;

  token.controlHeight = 32.0;

  token.zIndexBase = 0.0;
  token.zIndexPopupBase = 1000.0;

  token.opacityImage = 1.0;

  token.wireframe = false;
  token.motion = true;

  return token;
}

ThemeConfig defaultThemeConfig() {
  ThemeConfig config;
  config.seed = defaultSeedToken();
  config.algorithms = {ThemeAlgorithm::Default};
  config.loadAntdFont = false;
  return config;
}

bool operator==(const ThemeSeedToken& lhs, const ThemeSeedToken& rhs) {
  return lhs.blue == rhs.blue && lhs.purple == rhs.purple && lhs.cyan == rhs.cyan &&
         lhs.green == rhs.green && lhs.magenta == rhs.magenta && lhs.pink == rhs.pink &&
         lhs.red == rhs.red && lhs.orange == rhs.orange && lhs.yellow == rhs.yellow &&
         lhs.volcano == rhs.volcano && lhs.geekblue == rhs.geekblue && lhs.gold == rhs.gold &&
         lhs.lime == rhs.lime && lhs.colorPrimary == rhs.colorPrimary &&
         lhs.colorSuccess == rhs.colorSuccess && lhs.colorWarning == rhs.colorWarning &&
         lhs.colorError == rhs.colorError && lhs.colorInfo == rhs.colorInfo &&
         lhs.colorLink == rhs.colorLink && lhs.colorTextBase == rhs.colorTextBase &&
         lhs.colorBgBase == rhs.colorBgBase && lhs.fontFamily == rhs.fontFamily &&
         lhs.fontFamilyCode == rhs.fontFamilyCode && lhs.fontSize == rhs.fontSize &&
         lhs.lineWidth == rhs.lineWidth && lhs.lineType == rhs.lineType &&
         lhs.motionUnit == rhs.motionUnit && lhs.motionBase == rhs.motionBase &&
         lhs.motionEaseOutCirc == rhs.motionEaseOutCirc &&
         lhs.motionEaseInOutCirc == rhs.motionEaseInOutCirc &&
         lhs.motionEaseOut == rhs.motionEaseOut && lhs.motionEaseInOut == rhs.motionEaseInOut &&
         lhs.motionEaseOutBack == rhs.motionEaseOutBack &&
         lhs.motionEaseInBack == rhs.motionEaseInBack &&
         lhs.motionEaseInQuint == rhs.motionEaseInQuint &&
         lhs.motionEaseOutQuint == rhs.motionEaseOutQuint &&
         lhs.borderRadius == rhs.borderRadius && lhs.sizeUnit == rhs.sizeUnit &&
         lhs.sizeStep == rhs.sizeStep && lhs.sizePopupArrow == rhs.sizePopupArrow &&
         lhs.controlHeight == rhs.controlHeight && lhs.zIndexBase == rhs.zIndexBase &&
         lhs.zIndexPopupBase == rhs.zIndexPopupBase && lhs.opacityImage == rhs.opacityImage &&
         lhs.wireframe == rhs.wireframe && lhs.motion == rhs.motion;
}

bool operator==(const ThemeConfig& lhs, const ThemeConfig& rhs) {
  return lhs.seed == rhs.seed && lhs.algorithms == rhs.algorithms &&
         lhs.loadAntdFont == rhs.loadAntdFont;
}

}  // namespace adqt::theme
