#include "select_style.h"

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

QColor withAlpha(const QColor& color, qreal alpha) {
  QColor updated = color;
  updated.setAlphaF(std::clamp(alpha, 0.0, 1.0));
  return updated;
}

QColor compositeOn(const QColor& foreground, const QColor& background) {
  if (!foreground.isValid()) {
    return background;
  }
  if (!background.isValid()) {
    QColor opaque = foreground;
    opaque.setAlpha(255);
    return opaque;
  }

  const qreal alpha = std::clamp(static_cast<qreal>(foreground.alphaF()), qreal(0.0), qreal(1.0));
  if (alpha >= 0.999) {
    return foreground;
  }

  QColor mixed;
  mixed.setRedF(foreground.redF() * alpha + background.redF() * (1.0 - alpha));
  mixed.setGreenF(foreground.greenF() * alpha + background.greenF() * (1.0 - alpha));
  mixed.setBlueF(foreground.blueF() * alpha + background.blueF() * (1.0 - alpha));
  mixed.setAlpha(255);
  return mixed;
}

void applySemanticSlot(const AdSelect::SemanticSlotStyle& slot,
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

SelectVisualStyle resolveSelectVisualStyle(const SelectStyleInput& input) {
  const adqt::theme::ThemeMapToken& map = adqt::theme::ThemeManager::instance().currentMapToken();
  const QColor transparent(0, 0, 0, 0);

  SelectVisualStyle style;
  style.selectorBg = toColor(map.colorBgContainer, QColor("#ffffff"));
  style.selectorHoverBg = style.selectorBg;
  style.selectorActiveBg = style.selectorBg;
  style.selectorBorderColor = toColor(map.colorBorder, QColor("#d9d9d9"));
  style.selectorHoverBorderColor = toColor(map.colorPrimaryHover, QColor("#4096ff"));
  style.selectorActiveBorderColor = toColor(map.colorPrimary, QColor("#1677ff"));
  style.selectorFocusOutlineColor = withAlpha(toColor(map.colorPrimary, QColor("#1677ff")), 0.12);
  style.selectorTextColor = toColor(map.colorText, QColor("#141414"));
  style.placeholderColor = toColor(map.colorTextQuaternary, QColor("#bfbfbf"));
  style.popupBg = toColor(map.colorBgElevated, QColor("#ffffff"));
  style.popupBorderColor = toColor(map.colorBorderSecondary, QColor("#f0f0f0"));
  style.optionTextColor = style.selectorTextColor;
  style.optionHoverBg = toColor(map.colorFillSecondary, QColor("#f5f5f5"));
  style.optionSelectedBg = toColor(map.colorPrimaryBg, QColor("#e6f4ff"));
  style.optionSelectedColor = toColor(map.colorText, QColor("#141414"));
  style.tagBg = toColor(map.colorFillSecondary, QColor("#f5f5f5"));
  style.tagTextColor = style.selectorTextColor;
  style.clearColor = toColor(map.colorTextTertiary, QColor("#8c8c8c"));
  style.prefixColor = toColor(map.colorTextSecondary, QColor("#595959"));
  style.suffixColor = toColor(map.colorTextQuaternary, QColor("#bfbfbf"));
  style.disabledTextColor = toColor(map.colorTextQuaternary, QColor("#bfbfbf"));
  style.disabledBg = toColor(map.colorFillTertiary, QColor("#f5f5f5"));
  style.disabledBorderColor = toColor(map.colorBorderDisabled, QColor("#d9d9d9"));

  style.metrics.font = input.baseFont;
  style.metrics.font.setPixelSize(std::max(12, qRound(map.fontSize)));
  style.metrics.height = std::max(24, qRound(map.controlHeight));
  style.metrics.borderRadius = std::max(0, qRound(map.borderRadius));
  style.metrics.popupBorderRadius = std::max(0, qRound(map.borderRadiusLG));
  style.metrics.optionBorderRadius = std::max(0, qRound(map.borderRadiusSM));
  style.metrics.borderWidth = std::max(1, qRound(map.lineWidth));
  style.metrics.focusOutlineWidth = std::max<qreal>(1.0, map.lineWidth * 3.0);
  style.metrics.focusOutlineOffset = 1.0;
  style.metrics.horizontalPadding = std::max(8, qRound(map.sizeSM - map.lineWidth));
  style.metrics.popupPadding = std::max(2, qRound(map.sizeXXS));
  style.metrics.popupOffset = std::max(2, qRound(map.sizeXXS));
  style.metrics.popupMaxHeight = 256;
  style.metrics.optionHeight = std::max(24, qRound(map.controlHeight));
  style.metrics.optionPaddingHorizontal = std::max(8, qRound(map.sizeSM));
  style.metrics.tagHeight = std::max(18, qRound(map.controlHeightSM));
  style.metrics.iconSize = std::max(10, qRound(map.fontSizeSM));
  style.metrics.spacing = std::max(4, qRound(map.sizeXXS * 1.5));

  double optionLineHeight = map.lineHeight;
  auto recomputeOptionPadding = [&style, &optionLineHeight]() {
    const int fontPixelSize = style.metrics.font.pixelSize();
    const double textHeight = static_cast<double>(std::max(1, fontPixelSize)) * optionLineHeight;
    style.metrics.optionPaddingVertical =
        std::max(0, qRound((style.metrics.optionHeight - textHeight) / 2.0));
  };
  recomputeOptionPadding();

  if (input.size == AdSelect::Size::Large) {
    style.metrics.height = std::max(style.metrics.height, qRound(map.controlHeightLG));
    style.metrics.optionHeight = std::max(style.metrics.optionHeight, qRound(map.controlHeightLG));
    optionLineHeight = map.lineHeightLG;
  } else if (input.size == AdSelect::Size::Small) {
    style.metrics.height = std::max(24, qRound(map.controlHeightSM));
    style.metrics.optionHeight = std::max(24, qRound(map.controlHeightSM));
    style.metrics.font.setPixelSize(std::max(12, qRound(map.fontSizeSM)));
    optionLineHeight = map.lineHeightSM;
  }
  recomputeOptionPadding();

  if (input.variant == AdSelect::Variant::Filled) {
    style.selectorBg = toColor(map.colorFillTertiary, QColor("#f5f5f5"));
    style.selectorHoverBg = toColor(map.colorFillSecondary, QColor("#f5f5f5"));
    style.selectorActiveBg = toColor(map.colorBgContainer, QColor("#ffffff"));
    style.selectorBorderColor = transparent;
    style.selectorHoverBorderColor = transparent;
    style.selectorFocusOutlineColor = transparent;
  } else if (input.variant == AdSelect::Variant::Borderless) {
    style.selectorBg = transparent;
    style.selectorHoverBg = transparent;
    style.selectorActiveBg = transparent;
    style.selectorBorderColor = transparent;
    style.selectorHoverBorderColor = transparent;
    style.selectorActiveBorderColor = transparent;
    style.selectorFocusOutlineColor = transparent;
  } else if (input.variant == AdSelect::Variant::Underlined) {
    style.selectorBg = toColor(map.colorBgContainer, QColor("#ffffff"));
    style.selectorHoverBg = style.selectorBg;
    style.selectorActiveBg = style.selectorBg;
    style.selectorFocusOutlineColor = transparent;
  }

  if (input.status == AdSelect::Status::Error) {
    if (input.variant == AdSelect::Variant::Filled) {
      style.selectorBg = toColor(map.colorErrorBg, QColor("#fff2f0"));
      style.selectorHoverBg = toColor(map.colorErrorBgHover, QColor("#fff1f0"));
      style.selectorActiveBorderColor = toColor(map.colorError, QColor("#ff4d4f"));
    } else if (input.variant != AdSelect::Variant::Borderless) {
      style.selectorBorderColor = toColor(map.colorError, QColor("#ff4d4f"));
      style.selectorHoverBorderColor = toColor(map.colorErrorHover, QColor("#ff7875"));
      style.selectorActiveBorderColor = toColor(map.colorError, QColor("#ff4d4f"));
      style.selectorFocusOutlineColor =
          (input.variant == AdSelect::Variant::Outlined)
              ? withAlpha(toColor(map.colorError, QColor("#ff4d4f")), 0.12)
              : transparent;
    }
  } else if (input.status == AdSelect::Status::Warning) {
    if (input.variant == AdSelect::Variant::Filled) {
      style.selectorBg = toColor(map.colorWarningBg, QColor("#fffbe6"));
      style.selectorHoverBg = toColor(map.colorWarningBgHover, QColor("#fffbe6"));
      style.selectorActiveBorderColor = toColor(map.colorWarning, QColor("#faad14"));
    } else if (input.variant != AdSelect::Variant::Borderless) {
      style.selectorBorderColor = toColor(map.colorWarning, QColor("#faad14"));
      style.selectorHoverBorderColor = toColor(map.colorWarningHover, QColor("#ffd666"));
      style.selectorActiveBorderColor = toColor(map.colorWarning, QColor("#faad14"));
      style.selectorFocusOutlineColor =
          (input.variant == AdSelect::Variant::Outlined)
              ? withAlpha(toColor(map.colorWarning, QColor("#faad14")), 0.12)
              : transparent;
    }
  }

  const AdSelect::ComponentTokens& tokens = input.componentTokens;
  if (tokens.controlHeight.has_value()) {
    style.metrics.height = std::max(24, tokens.controlHeight.value());
  }
  if (tokens.borderRadius.has_value()) {
    style.metrics.borderRadius = std::max(0, tokens.borderRadius.value());
  }
  if (tokens.borderWidth.has_value()) {
    style.metrics.borderWidth = std::max(0, tokens.borderWidth.value());
  }
  if (tokens.horizontalPadding.has_value()) {
    style.metrics.horizontalPadding = std::max(0, tokens.horizontalPadding.value());
  }
  if (tokens.popupMaxHeight.has_value()) {
    style.metrics.popupMaxHeight = std::max(80, tokens.popupMaxHeight.value());
  }
  if (tokens.optionHeight.has_value()) {
    style.metrics.optionHeight = std::max(20, tokens.optionHeight.value());
  }
  if (tokens.tagHeight.has_value()) {
    style.metrics.tagHeight = std::max(16, tokens.tagHeight.value());
  }
  if (tokens.iconSize.has_value()) {
    style.metrics.iconSize = std::max(10, tokens.iconSize.value());
  }
  recomputeOptionPadding();

  if (tokens.selectorBg.has_value()) {
    style.selectorBg = toColor(tokens.selectorBg.value(), style.selectorBg);
    if (input.variant != AdSelect::Variant::Filled) {
      style.selectorHoverBg = style.selectorBg;
      style.selectorActiveBg = style.selectorBg;
    }
  }
  style.selectorBorderColor = resolveTokenColor(tokens.selectorBorderColor, style.selectorBorderColor);
  style.selectorHoverBorderColor =
      resolveTokenColor(tokens.selectorHoverBorderColor, style.selectorHoverBorderColor);
  style.selectorActiveBorderColor =
      resolveTokenColor(tokens.selectorActiveBorderColor, style.selectorActiveBorderColor);
  style.selectorTextColor = resolveTokenColor(tokens.selectorTextColor, style.selectorTextColor);
  style.placeholderColor = resolveTokenColor(tokens.placeholderColor, style.placeholderColor);
  style.popupBg = resolveTokenColor(tokens.popupBg, style.popupBg);
  style.popupBorderColor = resolveTokenColor(tokens.popupBorderColor, style.popupBorderColor);
  style.optionTextColor = resolveTokenColor(tokens.optionTextColor, style.optionTextColor);
  style.optionHoverBg = resolveTokenColor(tokens.optionHoverBg, style.optionHoverBg);
  style.optionSelectedBg = resolveTokenColor(tokens.optionSelectedBg, style.optionSelectedBg);
  style.optionSelectedColor = resolveTokenColor(tokens.optionSelectedColor, style.optionSelectedColor);
  style.tagBg = resolveTokenColor(tokens.tagBg, style.tagBg);
  style.tagTextColor = resolveTokenColor(tokens.tagTextColor, style.tagTextColor);
  style.clearColor = resolveTokenColor(tokens.clearColor, style.clearColor);
  style.prefixColor = resolveTokenColor(tokens.prefixColor, style.prefixColor);
  style.suffixColor = resolveTokenColor(tokens.suffixColor, style.suffixColor);

  const AdSelect::SemanticStyles& semantic = input.semanticStyles;
  applySemanticSlot(semantic.root, nullptr, &style.selectorBg, &style.selectorBorderColor);
  applySemanticSlot(semantic.selector, &style.selectorTextColor, &style.selectorBg, &style.selectorBorderColor);
  applySemanticSlot(semantic.placeholder, &style.placeholderColor, nullptr, nullptr);
  applySemanticSlot(semantic.tag, &style.tagTextColor, &style.tagBg, nullptr);
  applySemanticSlot(semantic.popup, nullptr, &style.popupBg, &style.popupBorderColor);
  applySemanticSlot(semantic.option, &style.optionTextColor, nullptr, nullptr);
  applySemanticSlot(semantic.optionHover, nullptr, &style.optionHoverBg, nullptr);
  applySemanticSlot(semantic.optionSelected, &style.optionSelectedColor, &style.optionSelectedBg, nullptr);
  applySemanticSlot(semantic.prefix, &style.prefixColor, nullptr, nullptr);
  applySemanticSlot(semantic.suffix, &style.suffixColor, nullptr, nullptr);

  if (input.variant != AdSelect::Variant::Filled) {
    style.selectorHoverBg = style.selectorBg;
    style.selectorActiveBg = style.selectorBg;
  }

  if (input.disabled) {
    if (input.variant == AdSelect::Variant::Borderless) {
      style.selectorBg = transparent;
      style.selectorHoverBg = transparent;
      style.selectorActiveBg = transparent;
      style.selectorBorderColor = transparent;
      style.selectorHoverBorderColor = transparent;
      style.selectorActiveBorderColor = transparent;
      style.selectorFocusOutlineColor = transparent;
    } else {
      style.selectorBg = style.disabledBg;
      style.selectorHoverBg = style.disabledBg;
      style.selectorActiveBg = style.disabledBg;
      style.selectorBorderColor = style.disabledBorderColor;
      style.selectorHoverBorderColor = style.disabledBorderColor;
      style.selectorActiveBorderColor = style.disabledBorderColor;
      style.selectorFocusOutlineColor = transparent;
    }
    style.selectorTextColor = style.disabledTextColor;
    style.placeholderColor = style.disabledTextColor;
    style.optionTextColor = style.disabledTextColor;
    style.tagTextColor = style.disabledTextColor;
    style.prefixColor = style.disabledTextColor;
    style.suffixColor = style.disabledTextColor;
    style.clearColor = style.disabledTextColor;
  }

  const QColor containerBg = toColor(map.colorBgContainer, QColor("#ffffff"));
  style.popupBg = compositeOn(style.popupBg, containerBg);
  style.popupBorderColor = compositeOn(style.popupBorderColor, style.popupBg);
  style.optionHoverBg = compositeOn(style.optionHoverBg, style.popupBg);
  style.optionSelectedBg = compositeOn(style.optionSelectedBg, style.popupBg);
  style.tagBg = compositeOn(style.tagBg, containerBg);

  return style;
}

}  // namespace adqt::widgets::detail
