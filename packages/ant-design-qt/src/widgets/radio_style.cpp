#include "radio_style.h"

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

void applySemanticSlot(const AdRadio::SemanticSlotStyle& slot,
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

void applyLabelOverride(const AdRadio::SemanticSlotStyle& slot,
                        RadioDotStateStyle* dotState,
                        RadioButtonStateStyle* buttonState) {
  if (!slot.textColor.has_value()) {
    return;
  }

  if (dotState) {
    dotState->labelColor = slot.textColor.value();
  }
  if (buttonState) {
    buttonState->textColor = slot.textColor.value();
  }
}

}  // namespace

RadioVisualStyle resolveRadioVisualStyle(const RadioStyleInput& input) {
  const adqt::theme::ThemeMapToken& map = adqt::theme::ThemeManager::instance().currentMapToken();
  const adqt::theme::GlobalPaletteToken& global = adqt::theme::ThemeManager::instance().currentToken();
  const bool wireframe = adqt::theme::ThemeManager::instance().currentConfig().seed.wireframe;

  const QColor transparent(0, 0, 0, 0);
  const QColor colorText = toColor(map.colorText, QColor("#141414"));
  const QColor colorTextDisabled = toColor(global.colorTextDisabled, QColor("#bfbfbf"));
  const QColor colorBorder = toColor(map.colorBorder, QColor("#d9d9d9"));
  const QColor colorBgContainer = toColor(map.colorBgContainer, QColor("#ffffff"));
  const QColor colorBgContainerDisabled =
      toColor(global.colorBgContainerDisabled, QColor("#f5f5f5"));
  const QColor colorPrimary = toColor(map.colorPrimary, QColor("#1677ff"));
  const QColor colorPrimaryHover = toColor(map.colorPrimaryHover, QColor("#4096ff"));
  const QColor colorPrimaryActive = toColor(map.colorPrimaryActive, QColor("#0958d9"));
  const QColor colorWhite = toColor(map.colorWhite, QColor("#ffffff"));
  const QColor colorPrimaryBorder = toColor(map.colorPrimaryBorder, QColor("#91caff"));
  const QColor controlItemBgActiveDisabled = toColor(map.colorFill, QColor("#d9d9d9"));

  RadioVisualStyle style;

  style.metrics.borderWidth = std::max(1, qRound(map.lineWidth));
  style.metrics.radioSize = std::max(12, qRound(map.fontSizeLG));
  const int dotPadding = 4;
  const int radioDotSize = wireframe ? (style.metrics.radioSize - dotPadding * 2)
                                     : (style.metrics.radioSize -
                                        (dotPadding + style.metrics.borderWidth) * 2);
  style.metrics.dotSize = std::max(4, radioDotSize);
  style.metrics.labelPaddingInlineStart = std::max(4, qRound(map.sizeXS));
  style.metrics.labelPaddingInlineEnd = std::max(4, qRound(map.sizeXS));
  style.metrics.wrapperMarginInlineEnd = std::max(0, qRound(map.sizeXS));
  style.metrics.buttonPaddingInline =
      std::max(4, qRound(map.sizeMS - map.lineWidth));
  style.metrics.buttonBorderRadius = std::max(0, qRound(map.borderRadius));
  style.metrics.focusOutlineColor = colorPrimaryBorder;
  style.metrics.focusOutlineWidth = std::max<qreal>(1.0, map.lineWidth * 3.0);
  style.metrics.focusOutlineOffset = 1.0;

  int fontPixelSize = std::max(12, qRound(map.fontSize));
  if (input.size == AdRadio::Size::Large) {
    style.metrics.buttonHeight = std::max(28, qRound(map.controlHeightLG));
    style.metrics.buttonBorderRadius = std::max(0, qRound(map.borderRadiusLG));
    fontPixelSize = std::max(12, qRound(map.fontSizeLG));
  } else if (input.size == AdRadio::Size::Small) {
    style.metrics.buttonHeight = std::max(24, qRound(map.controlHeightSM));
    style.metrics.buttonBorderRadius = std::max(0, qRound(map.borderRadiusSM));
    style.metrics.buttonPaddingInline =
        std::max(4, qRound(map.sizeXS - map.lineWidth));
    fontPixelSize = std::max(12, qRound(map.fontSize));
  } else {
    style.metrics.buttonHeight = std::max(24, qRound(map.controlHeight));
  }

  style.metrics.font = input.baseFont;
  style.metrics.font.setPixelSize(fontPixelSize);
  style.metrics.textLineHeight =
      std::max(style.metrics.radioSize, qRound(map.lineHeight * fontPixelSize));

  const auto& tokens = input.componentTokens;
  if (tokens.radioSize.has_value()) {
    style.metrics.radioSize = std::max(12, tokens.radioSize.value());
  }
  if (tokens.dotSize.has_value()) {
    style.metrics.dotSize = std::max(4, tokens.dotSize.value());
  }
  if (tokens.wrapperMarginInlineEnd.has_value()) {
    style.metrics.wrapperMarginInlineEnd = std::max(0, tokens.wrapperMarginInlineEnd.value());
  }
  if (tokens.buttonPaddingInline.has_value()) {
    style.metrics.buttonPaddingInline = std::max(0, tokens.buttonPaddingInline.value());
  }
  style.metrics.textLineHeight = std::max(style.metrics.textLineHeight, style.metrics.radioSize);

  QColor dotColor = wireframe ? colorPrimary : colorWhite;
  QColor checkedBg = wireframe ? colorBgContainer : colorPrimary;
  QColor checkedHoverBg = colorPrimaryHover;

  style.dotNormal = {colorBorder, colorBgContainer, dotColor, colorText};
  style.dotHover = {colorPrimary, colorBgContainer, dotColor, colorText};
  // antd default Radio does not define a distinct active state for unchecked items.
  style.dotActive = style.dotHover;
  style.dotChecked = {colorPrimary, checkedBg, dotColor, colorText};
  style.dotCheckedHover = {transparent, checkedHoverBg, dotColor, colorText};
  style.dotDisabled = {colorBorder, colorBgContainerDisabled, colorTextDisabled, colorTextDisabled};
  style.dotCheckedDisabled =
      {colorBorder, colorBgContainerDisabled, colorTextDisabled, colorTextDisabled};

  QColor buttonBg = colorBgContainer;
  QColor buttonCheckedBg = colorBgContainer;
  QColor buttonColor = colorText;
  QColor buttonCheckedBgDisabled = controlItemBgActiveDisabled;
  QColor buttonCheckedColorDisabled = colorTextDisabled;
  QColor buttonSolidCheckedColor = colorWhite;
  QColor buttonSolidCheckedBg = colorPrimary;
  QColor buttonSolidCheckedHoverBg = colorPrimaryHover;
  QColor buttonSolidCheckedActiveBg = colorPrimaryActive;
  QColor dotColorDisabled = colorTextDisabled;

  buttonBg = resolveTokenColor(tokens.buttonBg, buttonBg);
  buttonCheckedBg = resolveTokenColor(tokens.buttonCheckedBg, buttonCheckedBg);
  buttonColor = resolveTokenColor(tokens.buttonColor, buttonColor);
  buttonCheckedBgDisabled =
      resolveTokenColor(tokens.buttonCheckedBgDisabled, buttonCheckedBgDisabled);
  buttonCheckedColorDisabled =
      resolveTokenColor(tokens.buttonCheckedColorDisabled, buttonCheckedColorDisabled);
  buttonSolidCheckedColor =
      resolveTokenColor(tokens.buttonSolidCheckedColor, buttonSolidCheckedColor);
  buttonSolidCheckedBg = resolveTokenColor(tokens.buttonSolidCheckedBg, buttonSolidCheckedBg);
  buttonSolidCheckedHoverBg =
      resolveTokenColor(tokens.buttonSolidCheckedHoverBg, buttonSolidCheckedHoverBg);
  buttonSolidCheckedActiveBg =
      resolveTokenColor(tokens.buttonSolidCheckedActiveBg, buttonSolidCheckedActiveBg);

  if (tokens.dotColorDisabled.has_value()) {
    dotColorDisabled = resolveTokenColor(tokens.dotColorDisabled, dotColorDisabled);
    style.dotDisabled.dotColor = dotColorDisabled;
    style.dotCheckedDisabled.dotColor = dotColorDisabled;
  }

  style.buttonNormal = {buttonColor, buttonBg, colorBorder};
  // antd Radio.Button hover (unchecked): text turns primary, border stays default.
  style.buttonHover = {colorPrimary, buttonBg, colorBorder};
  // antd does not define a separate unchecked active style for Radio.Button.
  style.buttonActive = style.buttonHover;
  style.buttonChecked = {colorPrimary, buttonCheckedBg, colorPrimary};
  style.buttonCheckedHover = {colorPrimaryHover, buttonCheckedBg, colorPrimaryHover};
  style.buttonCheckedActive = {colorPrimaryActive, buttonCheckedBg, colorPrimaryActive};
  style.buttonDisabled = {colorTextDisabled, colorBgContainerDisabled, colorBorder};
  style.buttonCheckedDisabled =
      {buttonCheckedColorDisabled, buttonCheckedBgDisabled, colorBorder};

  if (input.buttonStyle == AdRadio::ButtonStyle::Solid) {
    style.buttonChecked = {buttonSolidCheckedColor, buttonSolidCheckedBg, buttonSolidCheckedBg};
    style.buttonCheckedHover =
        {buttonSolidCheckedColor, buttonSolidCheckedHoverBg, buttonSolidCheckedHoverBg};
    style.buttonCheckedActive =
        {buttonSolidCheckedColor, buttonSolidCheckedActiveBg, buttonSolidCheckedActiveBg};
  }

  if (tokens.radioBgColor.has_value()) {
    const QColor radioBgColor = resolveTokenColor(tokens.radioBgColor, style.dotChecked.backgroundColor);
    style.dotChecked.backgroundColor = radioBgColor;
    style.dotCheckedHover.backgroundColor = radioBgColor;
  }
  if (tokens.radioColor.has_value()) {
    const QColor radioColor = resolveTokenColor(tokens.radioColor, style.dotChecked.dotColor);
    style.dotNormal.dotColor = radioColor;
    style.dotHover.dotColor = radioColor;
    style.dotActive.dotColor = radioColor;
    style.dotChecked.dotColor = radioColor;
    style.dotCheckedHover.dotColor = radioColor;
  }

  const AdRadio::SemanticStyles& semantic = input.semanticStyles;

  applySemanticSlot(semantic.root, &style.dotNormal.labelColor, &style.dotNormal.backgroundColor,
                    &style.dotNormal.borderColor);
  applySemanticSlot(semantic.root, &style.buttonNormal.textColor, &style.buttonNormal.backgroundColor,
                    &style.buttonNormal.borderColor);

  applySemanticSlot(semantic.icon, nullptr, &style.dotNormal.backgroundColor, &style.dotNormal.borderColor);
  applySemanticSlot(semantic.icon, nullptr, &style.dotHover.backgroundColor, &style.dotHover.borderColor);
  applySemanticSlot(semantic.icon, nullptr, &style.dotActive.backgroundColor, &style.dotActive.borderColor);
  applySemanticSlot(semantic.icon, nullptr, &style.dotChecked.backgroundColor, &style.dotChecked.borderColor);
  applySemanticSlot(semantic.icon, nullptr, &style.dotCheckedHover.backgroundColor,
                    &style.dotCheckedHover.borderColor);
  applySemanticSlot(semantic.icon, nullptr, &style.dotDisabled.backgroundColor,
                    &style.dotDisabled.borderColor);
  applySemanticSlot(semantic.icon, nullptr, &style.dotCheckedDisabled.backgroundColor,
                    &style.dotCheckedDisabled.borderColor);

  applyLabelOverride(semantic.label, &style.dotNormal, &style.buttonNormal);
  applyLabelOverride(semantic.label, &style.dotHover, &style.buttonHover);
  applyLabelOverride(semantic.label, &style.dotActive, &style.buttonActive);
  applyLabelOverride(semantic.label, &style.dotChecked, &style.buttonChecked);
  applyLabelOverride(semantic.label, &style.dotCheckedHover, &style.buttonCheckedHover);
  applyLabelOverride(semantic.label, &style.dotDisabled, &style.buttonDisabled);
  applyLabelOverride(semantic.label, &style.dotCheckedDisabled, &style.buttonCheckedDisabled);
  applyLabelOverride(semantic.label, nullptr, &style.buttonCheckedActive);

  if (input.disabled) {
    if (input.checked) {
      style.dotChecked = style.dotCheckedDisabled;
      style.buttonChecked = style.buttonCheckedDisabled;
    } else {
      style.dotNormal = style.dotDisabled;
      style.buttonNormal = style.buttonDisabled;
    }
  }

  return style;
}

}  // namespace adqt::widgets::detail
