#include "popover_style.h"

#include "theme/fast_color_lite.h"
#include "theme/theme.h"

#include <algorithm>

namespace adqt::widgets::detail {

namespace {

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

QColor resolveTokenColor(const std::optional<QString>& token, const QColor& fallback) {
  if (!token.has_value()) {
    return fallback;
  }
  return toColor(token.value(), fallback);
}

void applySemanticSlot(const AdPopover::SemanticSlotStyle& slot,
                       QColor* textColor,
                       QColor* backgroundColor,
                       QColor* borderColor) {
  if (textColor && slot.textColor.has_value()) {
    *textColor = slot.textColor.value();
  }
  if (backgroundColor && slot.backgroundColor.has_value()) {
    *backgroundColor = slot.backgroundColor.value();
  }
  if (borderColor && slot.borderColor.has_value()) {
    *borderColor = slot.borderColor.value();
  }
}

}  // namespace

PopoverVisualStyle resolvePopoverVisualStyle(const PopoverStyleInput& input) {
  const adqt::theme::ThemeManager& themeManager = adqt::theme::ThemeManager::instance();
  const adqt::theme::ThemeMapToken& map = themeManager.currentMapToken();
  const adqt::theme::ThemeSeedToken& seed = themeManager.currentConfig().seed;

  PopoverVisualStyle style;
  style.rootBackground = QColor(0, 0, 0, 0);
  style.containerBackground = toColor(map.colorBgElevated, QColor("#ffffff"));
  style.borderColor = toColor(map.colorBorderSecondary, QColor("#f0f0f0"));
  style.titleColor = toColor(map.colorText, QColor("#141414"));
  style.contentColor = toColor(map.colorText, QColor("#141414"));
  style.arrowColor = style.containerBackground;

  style.metrics.titleMinWidth = 177;
  style.metrics.zIndexPopup = static_cast<int>(std::round(seed.zIndexPopupBase + 30.0));
  style.metrics.borderRadius = std::max(0, qRound(map.borderRadiusLG));
  style.metrics.borderWidth = std::max(0, qRound(map.lineWidth));
  style.metrics.arrowSize = std::max(6, qRound(seed.sizePopupArrow / 2.0));
  style.metrics.popupOffset = std::max(2, qRound(map.sizeXXS));
  style.metrics.popupPadding = 12;
  style.metrics.titlePaddingHorizontal = 0;
  style.metrics.titlePaddingVertical = 0;
  style.metrics.titleMarginBottom = std::max(0, qRound(map.sizeXS));
  style.metrics.contentPaddingHorizontal = 0;
  style.metrics.contentPaddingVertical = 0;
  style.metrics.titleFont = input.baseFont;
  style.metrics.contentFont = input.baseFont;
  style.metrics.titleFont.setPixelSize(std::max(12, qRound(map.fontSize)));
  style.metrics.titleFont.setWeight(QFont::DemiBold);
  style.metrics.contentFont.setPixelSize(std::max(12, qRound(map.fontSize)));
  style.metrics.contentFont.setWeight(QFont::Normal);

  const AdPopover::ComponentTokens& tokens = input.componentTokens;
  if (tokens.titleMinWidth.has_value()) {
    style.metrics.titleMinWidth = std::max(0, tokens.titleMinWidth.value());
  }
  if (tokens.zIndexPopup.has_value()) {
    style.metrics.zIndexPopup = std::max(0, tokens.zIndexPopup.value());
  }
  if (tokens.borderRadius.has_value()) {
    style.metrics.borderRadius = std::max(0, tokens.borderRadius.value());
  }
  if (tokens.borderWidth.has_value()) {
    style.metrics.borderWidth = std::max(0, tokens.borderWidth.value());
  }
  if (tokens.arrowSize.has_value()) {
    style.metrics.arrowSize = std::max(0, tokens.arrowSize.value());
  }
  if (tokens.popupOffset.has_value()) {
    style.metrics.popupOffset = std::max(0, tokens.popupOffset.value());
  }
  if (tokens.popupPadding.has_value()) {
    style.metrics.popupPadding = std::max(0, tokens.popupPadding.value());
  }
  if (tokens.titlePaddingHorizontal.has_value()) {
    style.metrics.titlePaddingHorizontal = std::max(0, tokens.titlePaddingHorizontal.value());
  }
  if (tokens.titlePaddingVertical.has_value()) {
    style.metrics.titlePaddingVertical = std::max(0, tokens.titlePaddingVertical.value());
  }
  if (tokens.titleMarginBottom.has_value()) {
    style.metrics.titleMarginBottom = std::max(0, tokens.titleMarginBottom.value());
  }
  if (tokens.contentPaddingHorizontal.has_value()) {
    style.metrics.contentPaddingHorizontal = std::max(0, tokens.contentPaddingHorizontal.value());
  }
  if (tokens.contentPaddingVertical.has_value()) {
    style.metrics.contentPaddingVertical = std::max(0, tokens.contentPaddingVertical.value());
  }

  style.containerBackground = resolveTokenColor(tokens.popupBg, style.containerBackground);
  style.borderColor = resolveTokenColor(tokens.popupBorderColor, style.borderColor);
  style.titleColor = resolveTokenColor(tokens.titleColor, style.titleColor);
  style.contentColor = resolveTokenColor(tokens.contentColor, style.contentColor);
  style.arrowColor = style.containerBackground;

  const AdPopover::SemanticStyles& semantic = input.semanticStyles;
  applySemanticSlot(semantic.root, nullptr, &style.rootBackground, &style.borderColor);
  applySemanticSlot(semantic.container, nullptr, &style.containerBackground, &style.borderColor);
  applySemanticSlot(semantic.title, &style.titleColor, nullptr, nullptr);
  applySemanticSlot(semantic.content, &style.contentColor, nullptr, nullptr);
  applySemanticSlot(semantic.arrow, nullptr, &style.arrowColor, nullptr);

  if (input.disabled) {
    const QColor disabledText = toColor(map.colorTextQuaternary, QColor("#bfbfbf"));
    style.titleColor = disabledText;
    style.contentColor = disabledText;
  }

  return style;
}

}  // namespace adqt::widgets::detail
