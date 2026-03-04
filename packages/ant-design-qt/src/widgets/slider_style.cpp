#include "slider_style.h"

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
  QColor updated = color;
  updated.setAlphaF(std::clamp(alpha, 0.0, 1.0));
  return updated;
}

void applySemanticSlotColor(const AdSlider::SemanticSlotStyle& slot,
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

SliderVisualStyle resolveSliderVisualStyle(const SliderStyleInput& input) {
  const adqt::theme::ThemeMapToken& map = adqt::theme::ThemeManager::instance().currentMapToken();
  const adqt::theme::GlobalPaletteToken& global = adqt::theme::ThemeManager::instance().currentToken();

  SliderVisualStyle style;
  style.rootBg = QColor(0, 0, 0, 0);
  style.railBg = toColor(map.colorFillTertiary, QColor("#f5f5f5"));
  style.railHoverBg = toColor(map.colorFillSecondary, QColor("#f0f0f0"));
  style.trackBg = toColor(map.colorPrimaryBorder, QColor("#91caff"));
  style.trackHoverBg = toColor(map.colorPrimaryBorderHover, QColor("#69b1ff"));
  style.handleColor = toColor(map.colorPrimaryBorder, QColor("#91caff"));
  style.handleActiveColor = toColor(map.colorPrimary, QColor("#1677ff"));
  style.handleActiveOutlineColor = withAlpha(style.handleActiveColor, 0.2);
  style.handleColorDisabled = toColor(global.colorTextDisabled, QColor("#bfbfbf"));
  style.dotBorderColor = toColor(map.colorBorderSecondary, QColor("#f0f0f0"));
  style.dotActiveBorderColor = toColor(map.colorPrimaryBorder, QColor("#91caff"));
  style.trackBgDisabled = toColor(global.colorBgContainerDisabled, QColor("#f5f5f5"));
  style.markColor = toColor(map.colorTextSecondary, QColor("#8c8c8c"));
  style.markActiveColor = toColor(map.colorText, QColor("#141414"));
  style.tooltipBg = toColor(map.colorText, QColor("#141414"));
  style.tooltipText = toColor(map.colorBgBase, QColor("#ffffff"));
  style.useTracksBrush = false;

  const int defaultControlSize = std::max(16, qRound(map.controlHeightLG / 4.0));
  const int defaultControlSizeHover = std::max(defaultControlSize, qRound(map.controlHeightSM / 2.0));
  style.metrics.controlSize = defaultControlSize;
  style.metrics.railSize = 4;
  style.metrics.handleSize = defaultControlSize;
  style.metrics.handleSizeHover = defaultControlSizeHover;
  style.metrics.handleLineWidth = std::max(1, qRound(map.lineWidth + 1.0));
  style.metrics.handleLineWidthHover = std::max(1, qRound(map.lineWidth + 2.0));
  style.metrics.dotSize = 8;
  style.metrics.marginMain = std::max(4, style.metrics.handleSize / 2);
  style.metrics.marginCross = std::max(8, qRound(map.controlHeight - style.metrics.controlSize));
  style.metrics.markGap = std::max(8, qRound(map.sizeLG));
  style.metrics.focusOutlineSize = std::max(4, qRound(map.lineWidthBold * 2.0));
  style.metrics.tooltipPaddingH = std::max(6, qRound(map.sizeSM));
  style.metrics.tooltipPaddingV = std::max(3, qRound(map.sizeXXS));
  style.metrics.tooltipRadius = std::max(4, qRound(map.borderRadiusSM));
  style.metrics.tooltipOffset = std::max(6, qRound(map.sizeXS));
  style.metrics.tooltipArrowSize = std::max(4, qRound(map.sizeXXS));
  style.metrics.font = input.baseFont;
  style.metrics.font.setPixelSize(std::max(12, qRound(map.fontSize)));

  const auto& tokens = input.componentTokens;
  if (tokens.controlSize.has_value()) {
    style.metrics.controlSize = std::max(8, tokens.controlSize.value());
  }
  if (tokens.railSize.has_value()) {
    style.metrics.railSize = std::max(2, tokens.railSize.value());
  }
  if (tokens.handleSize.has_value()) {
    style.metrics.handleSize = std::max(8, tokens.handleSize.value());
  }
  if (tokens.handleSizeHover.has_value()) {
    style.metrics.handleSizeHover = std::max(style.metrics.handleSize, tokens.handleSizeHover.value());
  }
  if (tokens.handleLineWidth.has_value()) {
    style.metrics.handleLineWidth = std::max(1, tokens.handleLineWidth.value());
  }
  if (tokens.handleLineWidthHover.has_value()) {
    style.metrics.handleLineWidthHover = std::max(1, tokens.handleLineWidthHover.value());
  }
  if (tokens.dotSize.has_value()) {
    style.metrics.dotSize = std::max(4, tokens.dotSize.value());
  }

  style.railBg = resolveTokenColor(tokens.railBg, style.railBg);
  style.railHoverBg = resolveTokenColor(tokens.railHoverBg, style.railHoverBg);
  style.trackBg = resolveTokenColor(tokens.trackBg, style.trackBg);
  style.trackHoverBg = resolveTokenColor(tokens.trackHoverBg, style.trackHoverBg);
  style.handleColor = resolveTokenColor(tokens.handleColor, style.handleColor);
  style.handleActiveColor = resolveTokenColor(tokens.handleActiveColor, style.handleActiveColor);
  style.handleActiveOutlineColor =
      resolveTokenColor(tokens.handleActiveOutlineColor, style.handleActiveOutlineColor);
  style.handleColorDisabled = resolveTokenColor(tokens.handleColorDisabled, style.handleColorDisabled);
  style.dotBorderColor = resolveTokenColor(tokens.dotBorderColor, style.dotBorderColor);
  style.dotActiveBorderColor = resolveTokenColor(tokens.dotActiveBorderColor, style.dotActiveBorderColor);
  style.trackBgDisabled = resolveTokenColor(tokens.trackBgDisabled, style.trackBgDisabled);

  const auto& semantic = input.semanticStyles;
  applySemanticSlotColor(semantic.root, nullptr, &style.rootBg, nullptr);
  applySemanticSlotColor(semantic.rail, nullptr, &style.railBg, nullptr);
  applySemanticSlotColor(semantic.track, nullptr, &style.trackBg, nullptr);
  applySemanticSlotColor(semantic.tracks, nullptr, &style.trackBg, nullptr);
  applySemanticSlotColor(semantic.handle, nullptr, nullptr, &style.handleColor);
  applySemanticSlotColor(semantic.mark, &style.markColor, nullptr, nullptr);
  applySemanticSlotColor(semantic.markActive, &style.markActiveColor, nullptr, nullptr);
  if (semantic.tracks.brush.has_value()) {
    style.tracksBrush = semantic.tracks.brush.value();
    style.useTracksBrush = true;
  }

  if (input.disabled) {
    style.railBg = style.trackBgDisabled;
    style.railHoverBg = style.trackBgDisabled;
    style.trackBg = style.trackBgDisabled;
    style.trackHoverBg = style.trackBgDisabled;
    style.handleColor = style.handleColorDisabled;
    style.handleActiveColor = style.handleColorDisabled;
    style.handleActiveOutlineColor = QColor(0, 0, 0, 0);
    style.dotBorderColor = style.trackBgDisabled;
    style.dotActiveBorderColor = style.trackBgDisabled;
    style.markColor = style.handleColorDisabled;
    style.markActiveColor = style.handleColorDisabled;
    style.tooltipBg = toColor(map.colorTextQuaternary, QColor("#bfbfbf"));
    style.tooltipText = toColor(map.colorBgBase, QColor("#ffffff"));
    style.useTracksBrush = false;
  }

  return style;
}

}  // namespace adqt::widgets::detail
