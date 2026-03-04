#include "color_picker_style.h"

#include <algorithm>

#include "theme/fast_color_lite.h"
#include "theme/theme.h"

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

QColor resolveTokenColor(const std::optional<QString>& value, const QColor& fallback) {
  if (!value.has_value()) {
    return fallback;
  }
  return toColor(value.value(), fallback);
}

void applySemanticSlot(const AdColorPicker::SemanticSlotStyle& slot,
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

ColorPickerVisualStyle resolveColorPickerVisualStyle(const ColorPickerStyleInput& input) {
  const adqt::theme::ThemeMapToken& map = adqt::theme::ThemeManager::instance().currentMapToken();

  ColorPickerVisualStyle style;
  style.triggerBackground = toColor(map.colorBgElevated, QColor("#ffffff"));
  style.triggerBackgroundDisabled = toColor(map.colorFillTertiary, QColor("#f5f5f5"));
  style.triggerBorder = toColor(map.colorBorder, QColor("#d9d9d9"));
  style.triggerBorderHover = toColor(map.colorPrimaryHover, QColor("#4096ff"));
  style.triggerBorderActive = toColor(map.colorPrimary, QColor("#1677ff"));
  style.triggerText = toColor(map.colorText, QColor("#141414"));
  style.triggerTextDisabled = toColor(map.colorTextQuaternary, QColor("#bfbfbf"));
  style.panelBackground = toColor(map.colorBgElevated, QColor("#ffffff"));
  style.panelBorder = toColor(map.colorBorderSecondary, QColor("#f0f0f0"));
  style.panelText = toColor(map.colorText, QColor("#141414"));
  style.swatchBorder = toColor(map.colorBorderSecondary, QColor("#f0f0f0"));
  style.presetBorder = toColor(map.colorBorderSecondary, QColor("#f0f0f0"));
  style.presetBorderHover = toColor(map.colorPrimaryHover, QColor("#4096ff"));
  style.transparentCellA = QColor(255, 255, 255);
  style.transparentCellB = QColor(230, 230, 230);

  style.metrics.controlHeightSM = std::max(20, qRound(map.controlHeightSM));
  style.metrics.controlHeight = std::max(24, qRound(map.controlHeight));
  style.metrics.controlHeightLG = std::max(28, qRound(map.controlHeightLG));
  style.metrics.triggerMinWidth = std::max(34, qRound(map.controlHeight));
  style.metrics.triggerRadius = std::max(4, qRound(map.borderRadius));
  style.metrics.triggerPadding = std::max(2, qRound(map.sizeXXS));
  style.metrics.triggerTextGap = std::max(4, qRound(map.sizeXS));
  style.metrics.swatchSizeSM = std::max(10, qRound(map.controlHeightSM / 2.0));
  style.metrics.swatchSize = std::max(14, qRound(map.controlHeight / 2.0));
  style.metrics.swatchSizeLG = std::max(16, qRound(map.controlHeightLG / 2.0));
  style.metrics.panelWidth = 234;
  style.metrics.panelPadding = std::max(8, qRound(map.sizeSM));
  style.metrics.panelSpacing = std::max(8, qRound(map.sizeXS));
  style.metrics.presetSwatchSize = 24;
  style.metrics.inputHeight = std::max(24, qRound(map.controlHeightSM));
  style.metrics.borderWidth = std::max<qreal>(1.0, map.lineWidth);
  style.metrics.font = input.baseFont;
  style.metrics.font.setPixelSize(std::max(12, qRound(map.fontSize)));

  // Derived sizes for color picker internals
  style.metrics.saturationPanelHeight = 160;
  style.metrics.saturationPanelRadius = qMax(2, qRound(map.borderRadiusXS / 2.0));
  style.metrics.sliderHeight = 8;
  style.metrics.previewSwatchSize = 20;
  style.metrics.previewSwatchRadius = qMax(2, qRound(map.borderRadiusXS / 2.0));
  style.metrics.alphaInputWidth = 44;

  // Swatch radius derived from trigger radius for consistency
  style.metrics.swatchRadius = qMax(2, qRound(style.metrics.triggerRadius / 2.0));

  const auto& tokens = input.componentTokens;
  if (tokens.controlHeight.has_value()) {
    style.metrics.controlHeight = std::max(20, tokens.controlHeight.value());
  }
  if (tokens.triggerMinWidth.has_value()) {
    style.metrics.triggerMinWidth = std::max(24, tokens.triggerMinWidth.value());
  }
  if (tokens.triggerRadius.has_value()) {
    style.metrics.triggerRadius = std::max(0, tokens.triggerRadius.value());
  }
  if (tokens.panelWidth.has_value()) {
    style.metrics.panelWidth = std::max(220, tokens.panelWidth.value());
  }
  if (tokens.swatchSize.has_value()) {
    style.metrics.swatchSize = std::max(10, tokens.swatchSize.value());
    style.metrics.swatchSizeSM = std::max(8, style.metrics.swatchSize - 4);
    style.metrics.swatchSizeLG = std::max(style.metrics.swatchSize, style.metrics.swatchSize + 4);
  }
  if (tokens.presetSwatchSize.has_value()) {
    style.metrics.presetSwatchSize = std::max(12, tokens.presetSwatchSize.value());
  }
  if (tokens.panelPadding.has_value()) {
    style.metrics.panelPadding = std::max(0, tokens.panelPadding.value());
  }
  if (tokens.inputHeight.has_value()) {
    style.metrics.inputHeight = std::max(18, tokens.inputHeight.value());
  }

  style.triggerBackground = resolveTokenColor(tokens.triggerBackground, style.triggerBackground);
  style.triggerBorder = resolveTokenColor(tokens.triggerBorderColor, style.triggerBorder);
  style.triggerBorderHover = resolveTokenColor(tokens.triggerBorderHoverColor, style.triggerBorderHover);
  style.triggerText = resolveTokenColor(tokens.triggerTextColor, style.triggerText);
  style.panelBackground = resolveTokenColor(tokens.panelBackground, style.panelBackground);
  style.panelBorder = resolveTokenColor(tokens.panelBorderColor, style.panelBorder);

  applySemanticSlot(input.semanticStyles.root, nullptr, &style.triggerBackground, &style.triggerBorder);
  applySemanticSlot(input.semanticStyles.description, &style.triggerText, nullptr, nullptr);
  applySemanticSlot(input.semanticStyles.popup, &style.panelText, &style.panelBackground, &style.panelBorder);
  applySemanticSlot(input.semanticStyles.body, nullptr, nullptr, &style.swatchBorder);

  if (input.disabled) {
    style.triggerText = style.triggerTextDisabled;
    style.triggerBorderHover = style.triggerBorder;
    style.triggerBorderActive = style.triggerBorder;
  }

  return style;
}

}  // namespace adqt::widgets::detail
