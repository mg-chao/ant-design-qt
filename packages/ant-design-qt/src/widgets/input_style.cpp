#include "input_style.h"

#include "theme/fast_color_lite.h"
#include "theme/theme.h"

#include <algorithm>
#include <cmath>

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
  QColor out = color;
  out.setAlphaF(std::clamp(alpha, 0.0, 1.0));
  return out;
}

double roundToSingleDecimal(double value) {
  return std::round(value * 10.0) / 10.0;
}

double ceilToSingleDecimal(double value) {
  return std::ceil(value * 10.0) / 10.0;
}

int resolvePaddingBlock(double controlHeight, double fontSize, double lineHeight, double lineWidth,
                        bool useCeil) {
  const double base = (controlHeight - fontSize * lineHeight) / 2.0;
  const double rounded = useCeil ? ceilToSingleDecimal(base) : roundToSingleDecimal(base);
  return std::max(0, qRound(rounded - lineWidth));
}

void applySemanticSlot(const AdInput::SemanticSlotStyle& slot,
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

}  // namespace

InputVisualStyle resolveInputVisualStyle(const InputStyleInput& input) {
  const adqt::theme::ThemeMapToken& map = adqt::theme::ThemeManager::instance().currentMapToken();
  const adqt::theme::GlobalPaletteToken& global = adqt::theme::ThemeManager::instance().currentToken();
  const QColor transparent(0, 0, 0, 0);

  InputVisualStyle style;
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
  style.clearColor = toColor(map.colorTextQuaternary, QColor("#bfbfbf"));
  style.clearHoverColor = toColor(map.colorTextTertiary, QColor("#8c8c8c"));
  style.prefixColor = style.selectorTextColor;
  style.suffixColor = toColor(map.colorTextSecondary, QColor("#8c8c8c"));
  style.countColor = toColor(map.colorTextSecondary, QColor("#8c8c8c"));
  style.disabledTextColor = toColor(global.colorTextDisabled, QColor("#bfbfbf"));
  style.disabledBg = toColor(global.colorBgContainerDisabled, QColor("#f5f5f5"));
  style.disabledBorderColor = toColor(map.colorBorderDisabled, QColor("#d9d9d9"));

  style.metrics.font = input.baseFont;
  style.metrics.font.setPixelSize(std::max(12, qRound(map.fontSize)));
  style.metrics.height = std::max(24, qRound(map.controlHeight));
  style.metrics.borderRadius = std::max(0, qRound(map.borderRadius));
  style.metrics.borderWidth = std::max(1, qRound(map.lineWidth));
  style.metrics.horizontalPadding = std::max(8, qRound(map.sizeSM - map.lineWidth));
  style.metrics.verticalPadding =
      resolvePaddingBlock(map.controlHeight, map.fontSize, map.lineHeight, map.lineWidth, false);
  style.metrics.iconSize = std::max(10, qRound(map.fontSizeSM));
  style.metrics.countTopMargin = std::max(2, qRound(map.sizeXXS));
  style.metrics.countHeight = std::max(16, qRound(map.controlHeightSM));
  style.metrics.focusOutlineWidth = std::max<qreal>(1.0, map.lineWidth * 3.0);
  style.metrics.focusOutlineOffset = 0.0;

  if (input.size == AdInput::Size::Large) {
    style.metrics.height = std::max(28, qRound(map.controlHeightLG));
    style.metrics.borderRadius = std::max(0, qRound(map.borderRadiusLG));
    style.metrics.font.setPixelSize(std::max(12, qRound(map.fontSizeLG)));
  } else if (input.size == AdInput::Size::Small) {
    style.metrics.height = std::max(22, qRound(map.controlHeightSM));
    style.metrics.borderRadius = std::max(0, qRound(map.borderRadiusSM));
    style.metrics.horizontalPadding = std::max(6, qRound(map.sizeXS - map.lineWidth));
    style.metrics.verticalPadding =
        resolvePaddingBlock(map.controlHeightSM, map.fontSize, map.lineHeight, map.lineWidth, false);
  } else {
    style.metrics.verticalPadding =
        resolvePaddingBlock(map.controlHeight, map.fontSize, map.lineHeight, map.lineWidth, false);
  }

  if (input.size == AdInput::Size::Large) {
    style.metrics.verticalPadding =
        resolvePaddingBlock(map.controlHeightLG, map.fontSizeLG, map.lineHeightLG, map.lineWidth, true);
  }

  if (input.multiline) {
    style.metrics.height = std::max(style.metrics.height, qRound(map.controlHeightLG));
  }

  if (input.variant == AdInput::Variant::Filled) {
    style.selectorBg = toColor(map.colorFillTertiary, QColor("#f5f5f5"));
    style.selectorHoverBg = toColor(map.colorFillSecondary, QColor("#f0f0f0"));
    style.selectorActiveBg = toColor(map.colorBgContainer, QColor("#ffffff"));
    style.selectorBorderColor = transparent;
    style.selectorHoverBorderColor = transparent;
    style.selectorFocusOutlineColor = transparent;
  } else if (input.variant == AdInput::Variant::Borderless) {
    style.selectorBg = transparent;
    style.selectorHoverBg = transparent;
    style.selectorActiveBg = transparent;
    style.selectorBorderColor = transparent;
    style.selectorHoverBorderColor = transparent;
    style.selectorActiveBorderColor = transparent;
    style.selectorFocusOutlineColor = transparent;
  } else if (input.variant == AdInput::Variant::Underlined) {
    style.underlined = true;
    style.metrics.borderRadius = 0;
    style.selectorFocusOutlineColor = transparent;
  }

  if (input.status == AdInput::Status::Error) {
    const QColor statusColor = toColor(map.colorError, QColor("#ff4d4f"));
    style.countColor = statusColor;
    if (input.variant == AdInput::Variant::Filled) {
      style.selectorBg = toColor(map.colorErrorBg, QColor("#fff2f0"));
      style.selectorHoverBg = toColor(map.colorErrorBgHover, QColor("#fff1f0"));
      style.selectorActiveBorderColor = statusColor;
      style.selectorTextColor = toColor(map.colorErrorText, QColor("#ff4d4f"));
      style.prefixColor = statusColor;
      style.suffixColor = statusColor;
    } else if (input.variant == AdInput::Variant::Borderless) {
      style.selectorTextColor = statusColor;
      style.prefixColor = statusColor;
      style.suffixColor = statusColor;
    } else {
      style.selectorBorderColor = statusColor;
      style.selectorHoverBorderColor = toColor(map.colorErrorBorderHover, QColor("#ff7875"));
      style.selectorActiveBorderColor = statusColor;
      style.prefixColor = statusColor;
      style.suffixColor = statusColor;
      if (input.variant == AdInput::Variant::Outlined) {
        const QColor statusBg = toColor(map.colorErrorBg, QColor("#fff2f0"));
        style.selectorFocusOutlineColor = withAlpha(statusBg, 0.6);
      }
    }
  } else if (input.status == AdInput::Status::Warning) {
    const QColor statusColor = toColor(map.colorWarning, QColor("#faad14"));
    style.countColor = statusColor;
    if (input.variant == AdInput::Variant::Filled) {
      style.selectorBg = toColor(map.colorWarningBg, QColor("#fffbe6"));
      style.selectorHoverBg = toColor(map.colorWarningBgHover, QColor("#fffbe6"));
      style.selectorActiveBorderColor = statusColor;
      style.selectorTextColor = toColor(map.colorWarningText, QColor("#d48806"));
      style.prefixColor = statusColor;
      style.suffixColor = statusColor;
    } else if (input.variant == AdInput::Variant::Borderless) {
      style.selectorTextColor = statusColor;
      style.prefixColor = statusColor;
      style.suffixColor = statusColor;
    } else {
      style.selectorBorderColor = statusColor;
      style.selectorHoverBorderColor = toColor(map.colorWarningBorderHover, QColor("#ffd666"));
      style.selectorActiveBorderColor = statusColor;
      style.prefixColor = statusColor;
      style.suffixColor = statusColor;
      if (input.variant == AdInput::Variant::Outlined) {
        const QColor statusBg = toColor(map.colorWarningBg, QColor("#fffbe6"));
        style.selectorFocusOutlineColor = withAlpha(statusBg, 0.6);
      }
    }
  }

  const auto& tokens = input.componentTokens;
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
    style.metrics.iconSize = std::max(8, tokens.iconSize.value());
  }

  if (tokens.selectorBg.has_value()) {
    style.selectorBg = toColor(tokens.selectorBg.value(), style.selectorBg);
    if (input.variant != AdInput::Variant::Filled) {
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
  style.clearColor = resolveTokenColor(tokens.clearColor, style.clearColor);
  style.prefixColor = resolveTokenColor(tokens.prefixColor, style.prefixColor);
  style.suffixColor = resolveTokenColor(tokens.suffixColor, style.suffixColor);
  style.countColor = resolveTokenColor(tokens.countColor, style.countColor);

  const auto& semantic = input.semanticStyles;
  applySemanticSlot(semantic.root, nullptr, &style.selectorBg, &style.selectorBorderColor);
  applySemanticSlot(semantic.input, &style.selectorTextColor, &style.selectorBg, &style.selectorBorderColor);
  applySemanticSlot(semantic.prefix, &style.prefixColor, nullptr, nullptr);
  applySemanticSlot(semantic.suffix, &style.suffixColor, nullptr, nullptr);
  applySemanticSlot(semantic.clear, &style.clearColor, nullptr, nullptr);
  applySemanticSlot(semantic.count, &style.countColor, nullptr, nullptr);

  if (input.variant != AdInput::Variant::Filled) {
    style.selectorHoverBg = style.selectorBg;
    style.selectorActiveBg = style.selectorBg;
  }

  if (input.disabled) {
    if (input.variant == AdInput::Variant::Borderless) {
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
      if (input.variant != AdInput::Variant::Underlined) {
        style.selectorBorderColor = style.disabledBorderColor;
      }
      style.selectorHoverBorderColor = style.selectorBorderColor;
      style.selectorActiveBorderColor = style.selectorBorderColor;
      style.selectorFocusOutlineColor = transparent;
    }

    style.selectorTextColor = style.disabledTextColor;
    style.placeholderColor = style.disabledTextColor;
    style.clearColor = style.disabledTextColor;
    style.clearHoverColor = style.disabledTextColor;
    style.prefixColor = style.disabledTextColor;
    style.suffixColor = style.disabledTextColor;
    style.countColor = style.disabledTextColor;
  }

  return style;
}

}  // namespace adqt::widgets::detail
