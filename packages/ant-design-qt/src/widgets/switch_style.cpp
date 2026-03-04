#include "switch_style.h"

#include "theme/fast_color_lite.h"
#include "theme/theme.h"

#include <QRegularExpression>

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

int parseDurationMs(const QString& source, int fallbackMs) {
  const QString trimmed = source.trimmed();
  if (trimmed.isEmpty()) {
    return fallbackMs;
  }

  QString numberPart = trimmed;
  double multiplier = 1.0;
  if (numberPart.endsWith(QStringLiteral("ms"), Qt::CaseInsensitive)) {
    numberPart.chop(2);
  } else if (numberPart.endsWith(QStringLiteral("s"), Qt::CaseInsensitive)) {
    numberPart.chop(1);
    multiplier = 1000.0;
  }

  bool ok = false;
  const double value = numberPart.trimmed().toDouble(&ok);
  if (!ok || !std::isfinite(value)) {
    return fallbackMs;
  }

  return std::max(0, static_cast<int>(std::round(value * multiplier)));
}

std::optional<QColor> parseShadowColor(const QString& source) {
  if (source.trimmed().isEmpty()) {
    return std::nullopt;
  }

  static const QRegularExpression kRgbaPattern(QStringLiteral("(rgba?\\([^\\)]*\\))"),
                                               QRegularExpression::CaseInsensitiveOption);
  static const QRegularExpression kHexPattern(QStringLiteral("(#[0-9a-fA-F]{3,8})"));

  QRegularExpressionMatch match = kRgbaPattern.match(source);
  if (!match.hasMatch()) {
    match = kHexPattern.match(source);
  }
  if (!match.hasMatch()) {
    return std::nullopt;
  }

  const QColor parsed = toColor(match.captured(1), QColor());
  if (!parsed.isValid()) {
    return std::nullopt;
  }
  return parsed;
}

void applySemanticSlot(const AdSwitch::SemanticSlotStyle& slot,
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

SwitchVisualStyle resolveSwitchVisualStyle(const SwitchStyleInput& input) {
  const adqt::theme::ThemeMapToken& map = adqt::theme::ThemeManager::instance().currentMapToken();

  constexpr int kTrackPadding = 2;
  const int trackHeight = std::max(12, qRound(map.fontSize * map.lineHeight));
  const int trackHeightSM = std::max(10, qRound(map.controlHeight / 2.0));
  const int handleSize = std::max(8, trackHeight - kTrackPadding * 2);
  const int handleSizeSM = std::max(6, trackHeightSM - kTrackPadding * 2);
  const QColor primaryColor = toColor(map.colorPrimary, QColor("#1677ff"));

  SwitchVisualStyle style;
  style.trackBg = toColor(map.colorTextQuaternary, QColor("#bfbfbf"));
  style.trackHoverBg = toColor(map.colorTextTertiary, QColor("#8c8c8c"));
  style.trackCheckedBg = primaryColor;
  style.trackCheckedHoverBg = toColor(map.colorPrimaryHover, QColor("#4096ff"));
  style.trackBorderColor = QColor(0, 0, 0, 0);
  style.handleBg = toColor(map.colorWhite, QColor("#ffffff"));
  style.handleBorderColor = QColor(0, 0, 0, 0);
  style.contentColor = toColor(map.colorWhite, QColor("#ffffff"));
  style.loadingIconColor = QColor(0, 0, 0, 166);
  style.checkedLoadingIconColor = primaryColor;
  style.waveColor = primaryColor;

  style.metrics.trackHeight = trackHeight;
  style.metrics.trackHeightSM = trackHeightSM;
  style.metrics.trackPadding = kTrackPadding;
  style.metrics.handleSize = handleSize;
  style.metrics.handleSizeSM = handleSizeSM;
  style.metrics.trackMinWidth = handleSize * 2 + kTrackPadding * 4;
  style.metrics.trackMinWidthSM = handleSizeSM * 2 + kTrackPadding * 2;
  style.metrics.innerMinMargin = handleSize / 2;
  style.metrics.innerMaxMargin = handleSize + kTrackPadding + kTrackPadding * 2;
  style.metrics.innerMinMarginSM = handleSizeSM / 2;
  style.metrics.innerMaxMarginSM = handleSizeSM + kTrackPadding + kTrackPadding * 2;
  style.metrics.loadingIconSize = std::max(8, qRound(map.fontSizeSM * 0.75));
  style.metrics.fontSize = std::max(10, qRound(map.fontSizeSM));
  style.metrics.animationDurationMs = parseDurationMs(map.motionDurationMid, 200);
  style.metrics.disabledOpacity = 0.65;
  style.metrics.focusOutlineColor = toColor(map.colorPrimaryBorder, QColor("#91caff"));
  style.metrics.focusOutlineWidth = std::max<qreal>(1.0, map.lineWidth * 3.0);
  style.metrics.focusOutlineOffset = 1.0;
  style.metrics.handleShadowColor = QColor(0, 35, 11, 51);
  style.metrics.handleShadowBlur = 4.0;
  style.metrics.handleShadowOffsetY = 2.0;
  style.metrics.handleActiveInsetRatio = 0.3;
  style.metrics.innerContentActiveOffset = style.metrics.trackPadding * 2;
  style.metrics.innerContentActiveOffsetSM = std::max(1, qRound(map.sizeXXS / 2.0));

  if (!map.motion) {
    style.metrics.animationDurationMs = 0;
  }

  const auto& tokens = input.componentTokens;
  if (tokens.trackHeight.has_value()) {
    style.metrics.trackHeight = std::max(10, tokens.trackHeight.value());
  }
  if (tokens.trackHeightSM.has_value()) {
    style.metrics.trackHeightSM = std::max(8, tokens.trackHeightSM.value());
  }
  if (tokens.trackMinWidth.has_value()) {
    style.metrics.trackMinWidth = std::max(16, tokens.trackMinWidth.value());
  }
  if (tokens.trackMinWidthSM.has_value()) {
    style.metrics.trackMinWidthSM = std::max(12, tokens.trackMinWidthSM.value());
  }
  if (tokens.trackPadding.has_value()) {
    style.metrics.trackPadding = std::max(0, tokens.trackPadding.value());
  }
  if (tokens.handleSize.has_value()) {
    style.metrics.handleSize = std::max(6, tokens.handleSize.value());
  }
  if (tokens.handleSizeSM.has_value()) {
    style.metrics.handleSizeSM = std::max(4, tokens.handleSizeSM.value());
  }
  if (tokens.innerMinMargin.has_value()) {
    style.metrics.innerMinMargin = std::max(0, tokens.innerMinMargin.value());
  }
  if (tokens.innerMaxMargin.has_value()) {
    style.metrics.innerMaxMargin = std::max(0, tokens.innerMaxMargin.value());
  }
  if (tokens.innerMinMarginSM.has_value()) {
    style.metrics.innerMinMarginSM = std::max(0, tokens.innerMinMarginSM.value());
  }
  if (tokens.innerMaxMarginSM.has_value()) {
    style.metrics.innerMaxMarginSM = std::max(0, tokens.innerMaxMarginSM.value());
  }
  if (tokens.loadingIconSize.has_value()) {
    style.metrics.loadingIconSize = std::max(6, tokens.loadingIconSize.value());
  }
  if (tokens.disabledOpacity.has_value()) {
    style.metrics.disabledOpacity = std::clamp(tokens.disabledOpacity.value(), 0.0, 1.0);
  }

  style.metrics.innerContentActiveOffset = style.metrics.trackPadding * 2;
  style.metrics.innerContentActiveOffsetSM = std::max(1, qRound(map.sizeXXS / 2.0));
  style.loadingIconColor = QColor(0, 0, 0, qRound(255.0 * style.metrics.disabledOpacity));

  style.handleBg = resolveTokenColor(tokens.handleBg, style.handleBg);
  style.loadingIconColor = resolveTokenColor(tokens.loadingIconColor, style.loadingIconColor);
  if (tokens.colorPrimary.has_value()) {
    const QColor resolvedPrimary = resolveTokenColor(tokens.colorPrimary, style.trackCheckedBg);
    style.trackCheckedBg = resolvedPrimary;
    style.checkedLoadingIconColor = resolvedPrimary;
    style.waveColor = resolvedPrimary;
  }
  if (tokens.handleShadow.has_value()) {
    const std::optional<QColor> parsed = parseShadowColor(tokens.handleShadow.value());
    if (parsed.has_value()) {
      style.metrics.handleShadowColor = parsed.value();
    }
  }

  const auto& semantic = input.semanticStyles;
  if (semantic.root.backgroundColor.has_value()) {
    style.trackBg = semantic.root.backgroundColor.value();
    style.trackHoverBg = style.trackBg;
    style.trackCheckedBg = style.trackBg;
    style.trackCheckedHoverBg = style.trackBg;
  }
  applySemanticSlot(semantic.root, &style.contentColor, nullptr, &style.trackBorderColor);
  applySemanticSlot(semantic.content, &style.contentColor, nullptr, nullptr);
  applySemanticSlot(semantic.indicator, &style.loadingIconColor, &style.handleBg,
                    &style.handleBorderColor);
  if (semantic.indicator.textColor.has_value()) {
    style.checkedLoadingIconColor = semantic.indicator.textColor.value();
  }

  if (input.disabled || input.loading) {
    style.trackHoverBg = style.trackBg;
    style.trackCheckedHoverBg = style.trackCheckedBg;
  }

  style.waveColor = input.checked ? style.trackCheckedBg : style.trackBg;
  return style;
}

}  // namespace adqt::widgets::detail
