#include "input_number_style.h"

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

QColor resolveTokenColor(const std::optional<QString>& token, const QColor& fallback) {
  if (!token.has_value()) {
    return fallback;
  }
  return toColor(token.value(), fallback);
}

QColor withAlpha(const QColor& color, qreal alpha) {
  QColor result = color;
  result.setAlphaF(std::clamp(alpha, 0.0, 1.0));
  return result;
}

QColor themeOutlineFromBg(const QColor& bg, const QColor& fallback) {
  if (!bg.isValid()) {
    return fallback;
  }

  QColor out = bg;
  if (out.alpha() == 255) {
    out.setAlphaF(0.45);
  }
  return out;
}

void applySemanticSlot(const AdInputNumber::SemanticSlotStyle& slot,
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

InputNumberVisualStyle resolveInputNumberVisualStyle(const InputNumberStyleInput& input) {
  const adqt::theme::ThemeMapToken& map = adqt::theme::ThemeManager::instance().currentMapToken();
  const adqt::theme::GlobalPaletteToken& global = adqt::theme::ThemeManager::instance().currentToken();
  const QColor transparent(0, 0, 0, 0);

  InputNumberVisualStyle style;
  style.selectorBg = toColor(map.colorBgContainer, QColor("#ffffff"));
  style.selectorHoverBg = style.selectorBg;
  style.selectorActiveBg = style.selectorBg;
  style.selectorBorderColor = toColor(map.colorBorder, QColor("#d9d9d9"));
  style.selectorHoverBorderColor = toColor(map.colorPrimaryHover, QColor("#4096ff"));
  style.selectorActiveBorderColor = toColor(map.colorPrimary, QColor("#1677ff"));
  style.selectorFocusOutlineColor =
      themeOutlineFromBg(toColor(map.colorPrimaryBg, QColor("#e6f4ff")), QColor("#91caff"));
  style.selectorTextColor = toColor(map.colorText, QColor("#141414"));
  style.placeholderColor = toColor(global.colorTextPlaceholder, QColor("#bfbfbf"));
  style.prefixColor = style.selectorTextColor;
  style.suffixColor = style.selectorTextColor;
  style.handleBg = toColor(map.colorBgContainer, QColor("#ffffff"));
  style.handleActiveBg = toColor(global.colorFillAlter, QColor("#f5f5f5"));
  style.handleBorderColor = toColor(map.colorBorder, QColor("#d9d9d9"));
  style.handleHoverColor = toColor(map.colorPrimary, QColor("#1677ff"));
  style.handleIconColor = toColor(map.colorTextQuaternary, QColor("#bfbfbf"));
  style.outOfRangeTextColor = toColor(map.colorError, QColor("#ff4d4f"));
  style.disabledTextColor = toColor(global.colorTextDisabled, QColor("#bfbfbf"));
  style.disabledBg = toColor(global.colorBgContainerDisabled, QColor("#f5f5f5"));
  style.disabledBorderColor = toColor(map.colorBorderDisabled, QColor("#d9d9d9"));

  style.metrics.font = input.baseFont;
  style.metrics.font.setPixelSize(std::max(12, qRound(map.fontSize)));
  style.metrics.inputFontSize = std::max(12, qRound(map.fontSize));
  style.metrics.height = std::max(24, qRound(map.controlHeight));
  style.metrics.width = 90;
  style.metrics.borderRadius = std::max(0, qRound(map.borderRadius));
  style.metrics.borderWidth = std::max(1, qRound(map.lineWidth));
  style.metrics.horizontalPadding = std::max(8, qRound(map.sizeSM - map.lineWidth));
  style.metrics.iconSize = std::max(10, qRound(map.fontSizeSM));
  style.metrics.handleIconSize = std::max(6, qRound(map.fontSize / 2.0));
  style.metrics.handleWidth = std::max(16, qRound(map.controlHeightSM - map.lineWidth * 2.0));
  style.metrics.handleVisibleWidth =
      input.controlsVisible
          ? ((input.mode == AdInputNumber::Mode::Spinner || input.focused || input.hovered)
                 ? style.metrics.handleWidth
                 : 0)
          : 0;
  style.metrics.focusOutlineWidth = std::max<qreal>(1.0, map.lineWidth * 3.0);
  style.metrics.focusOutlineOffset = 0.0;

  if (input.size == AdInputNumber::Size::Large) {
    style.metrics.height = std::max(28, qRound(map.controlHeightLG));
    style.metrics.borderRadius = std::max(0, qRound(map.borderRadiusLG));
    style.metrics.inputFontSize = std::max(12, qRound(map.fontSizeLG));
    style.metrics.font.setPixelSize(style.metrics.inputFontSize);
  } else if (input.size == AdInputNumber::Size::Small) {
    style.metrics.height = std::max(22, qRound(map.controlHeightSM));
    style.metrics.borderRadius = std::max(0, qRound(map.borderRadiusSM));
    style.metrics.inputFontSize = std::max(12, qRound(map.fontSizeSM));
    style.metrics.font.setPixelSize(style.metrics.inputFontSize);
    style.metrics.horizontalPadding = std::max(6, qRound(map.sizeXS - map.lineWidth));
  }

  if (input.variant == AdInputNumber::Variant::Filled) {
    style.selectorBg = toColor(map.colorFillTertiary, QColor("#f5f5f5"));
    style.selectorHoverBg = toColor(map.colorFillSecondary, QColor("#f0f0f0"));
    style.selectorActiveBg = toColor(map.colorBgContainer, QColor("#ffffff"));
    style.selectorBorderColor = transparent;
    style.selectorHoverBorderColor = transparent;
    style.selectorFocusOutlineColor = transparent;
    style.handleBg = toColor(map.colorFillSecondary, QColor("#f0f0f0"));
  } else if (input.variant == AdInputNumber::Variant::Borderless) {
    style.selectorBg = transparent;
    style.selectorHoverBg = transparent;
    style.selectorActiveBg = transparent;
    style.selectorBorderColor = transparent;
    style.selectorHoverBorderColor = transparent;
    style.selectorActiveBorderColor = transparent;
    style.selectorFocusOutlineColor = transparent;
    style.handleBg = transparent;
    style.handleBorderColor = transparent;
  } else if (input.variant == AdInputNumber::Variant::Underlined) {
    style.underlined = true;
    style.metrics.borderRadius = 0;
    style.selectorFocusOutlineColor = transparent;
  }

  if (input.status == AdInputNumber::Status::Error) {
    const QColor statusColor = toColor(map.colorError, QColor("#ff4d4f"));
    style.outOfRangeTextColor = statusColor;
    if (input.variant == AdInputNumber::Variant::Filled) {
      style.selectorBg = toColor(map.colorErrorBg, QColor("#fff2f0"));
      style.selectorHoverBg = toColor(map.colorErrorBgHover, QColor("#fff1f0"));
      style.selectorActiveBorderColor = statusColor;
      style.selectorTextColor = toColor(map.colorErrorText, QColor("#ff4d4f"));
      style.prefixColor = statusColor;
      style.suffixColor = statusColor;
    } else if (input.variant == AdInputNumber::Variant::Borderless) {
      style.selectorTextColor = statusColor;
      style.prefixColor = statusColor;
      style.suffixColor = statusColor;
    } else {
      style.selectorBorderColor = statusColor;
      style.selectorHoverBorderColor = toColor(map.colorErrorBorderHover, QColor("#ff7875"));
      style.selectorActiveBorderColor = statusColor;
      style.prefixColor = statusColor;
      style.suffixColor = statusColor;
      if (input.variant == AdInputNumber::Variant::Outlined) {
        style.selectorFocusOutlineColor = withAlpha(toColor(map.colorErrorBg, QColor("#fff2f0")), 0.6);
      }
    }
  } else if (input.status == AdInputNumber::Status::Warning) {
    const QColor statusColor = toColor(map.colorWarning, QColor("#faad14"));
    if (input.variant == AdInputNumber::Variant::Filled) {
      style.selectorBg = toColor(map.colorWarningBg, QColor("#fffbe6"));
      style.selectorHoverBg = toColor(map.colorWarningBgHover, QColor("#fffbe6"));
      style.selectorActiveBorderColor = statusColor;
      style.selectorTextColor = toColor(map.colorWarningText, QColor("#d48806"));
      style.prefixColor = statusColor;
      style.suffixColor = statusColor;
    } else if (input.variant == AdInputNumber::Variant::Borderless) {
      style.selectorTextColor = statusColor;
      style.prefixColor = statusColor;
      style.suffixColor = statusColor;
    } else {
      style.selectorBorderColor = statusColor;
      style.selectorHoverBorderColor = toColor(map.colorWarningBorderHover, QColor("#ffd666"));
      style.selectorActiveBorderColor = statusColor;
      style.prefixColor = statusColor;
      style.suffixColor = statusColor;
      if (input.variant == AdInputNumber::Variant::Outlined) {
        style.selectorFocusOutlineColor =
            withAlpha(toColor(map.colorWarningBg, QColor("#fffbe6")), 0.6);
      }
    }
  }

  const auto& tokens = input.componentTokens;
  if (tokens.controlWidth.has_value()) {
    style.metrics.width = std::max(48, tokens.controlWidth.value());
  }
  if (tokens.controlHeight.has_value()) {
    style.metrics.height = std::max(20, tokens.controlHeight.value());
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
  if (tokens.iconSize.has_value()) {
    const int resolvedSize = std::max(8, tokens.iconSize.value());
    style.metrics.iconSize = resolvedSize;
    style.metrics.handleIconSize = resolvedSize;
  }
  if (tokens.handleWidth.has_value()) {
    style.metrics.handleWidth = std::max(12, tokens.handleWidth.value());
  }
  if (tokens.handleVisibleWidth.has_value()) {
    style.metrics.handleVisibleWidth =
        input.controlsVisible
            ? std::max(0, tokens.handleVisibleWidth.value())
            : 0;
  }

  if (tokens.inputFontSize.has_value()) {
    style.metrics.inputFontSize = std::max(8, tokens.inputFontSize.value());
    style.metrics.font.setPixelSize(style.metrics.inputFontSize);
  }
  if (input.size == AdInputNumber::Size::Small && tokens.inputFontSizeSM.has_value()) {
    style.metrics.inputFontSize = std::max(8, tokens.inputFontSizeSM.value());
    style.metrics.font.setPixelSize(style.metrics.inputFontSize);
  }
  if (input.size == AdInputNumber::Size::Large && tokens.inputFontSizeLG.has_value()) {
    style.metrics.inputFontSize = std::max(8, tokens.inputFontSizeLG.value());
    style.metrics.font.setPixelSize(style.metrics.inputFontSize);
  }
  if (input.size == AdInputNumber::Size::Large && tokens.paddingInlineLG.has_value()) {
    style.metrics.horizontalPadding = std::max(0, tokens.paddingInlineLG.value());
  }

  if (tokens.selectorBg.has_value()) {
    style.selectorBg = toColor(tokens.selectorBg.value(), style.selectorBg);
    if (input.variant != AdInputNumber::Variant::Filled) {
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
  style.prefixColor = resolveTokenColor(tokens.prefixColor, style.prefixColor);
  style.suffixColor = resolveTokenColor(tokens.suffixColor, style.suffixColor);
  style.handleBg = resolveTokenColor(tokens.handleBg, style.handleBg);
  style.handleActiveBg = resolveTokenColor(tokens.handleActiveBg, style.handleActiveBg);
  style.handleBorderColor = resolveTokenColor(tokens.handleBorderColor, style.handleBorderColor);
  style.handleHoverColor = resolveTokenColor(tokens.handleHoverColor, style.handleHoverColor);
  style.handleIconColor = resolveTokenColor(tokens.handleIconColor, style.handleIconColor);

  const auto& semantic = input.semanticStyles;
  applySemanticSlot(semantic.root, nullptr, &style.selectorBg, &style.selectorBorderColor);
  applySemanticSlot(semantic.input, &style.selectorTextColor, &style.selectorBg, &style.selectorBorderColor);
  applySemanticSlot(semantic.prefix, &style.prefixColor, nullptr, nullptr);
  applySemanticSlot(semantic.suffix, &style.suffixColor, nullptr, nullptr);
  applySemanticSlot(semantic.actions, &style.handleIconColor, &style.handleBg, &style.handleBorderColor);
  applySemanticSlot(semantic.action, &style.handleIconColor, &style.handleActiveBg, nullptr);

  if (input.variant != AdInputNumber::Variant::Filled) {
    style.selectorHoverBg = style.selectorBg;
    style.selectorActiveBg = style.selectorBg;
  }

  if (input.disabled) {
    if (input.variant == AdInputNumber::Variant::Borderless) {
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
      if (input.variant != AdInputNumber::Variant::Underlined) {
        style.selectorBorderColor = style.disabledBorderColor;
      }
      style.selectorHoverBorderColor = style.selectorBorderColor;
      style.selectorActiveBorderColor = style.selectorBorderColor;
      style.selectorFocusOutlineColor = transparent;
    }

    style.selectorTextColor = style.disabledTextColor;
    style.placeholderColor = style.disabledTextColor;
    style.prefixColor = style.disabledTextColor;
    style.suffixColor = style.disabledTextColor;
    style.handleIconColor = style.disabledTextColor;
    style.handleHoverColor = style.disabledTextColor;
    style.handleBg = style.disabledBg;
    style.handleActiveBg = style.disabledBg;
    style.handleBorderColor = style.disabledBorderColor;
  }

  return style;
}

}  // namespace adqt::widgets::detail
