#include "alert_style.h"

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

int parseDurationMs(const QString& value, int fallbackMs) {
  const QString trimmed = value.trimmed();
  if (trimmed.isEmpty()) {
    return fallbackMs;
  }

  QString numericPart = trimmed;
  double multiplier = 1.0;
  if (numericPart.endsWith(QStringLiteral("ms"), Qt::CaseInsensitive)) {
    numericPart.chop(2);
  } else if (numericPart.endsWith(QStringLiteral("s"), Qt::CaseInsensitive)) {
    numericPart.chop(1);
    multiplier = 1000.0;
  }

  bool ok = false;
  const double numeric = numericPart.trimmed().toDouble(&ok);
  if (!ok || !std::isfinite(numeric)) {
    return fallbackMs;
  }

  return std::max(0, static_cast<int>(std::round(numeric * multiplier)));
}

void applySemanticSlot(const AdAlert::SemanticSlotStyle& slot,
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

AlertVisualStyle resolveAlertVisualStyle(const AlertStyleInput& input) {
  const adqt::theme::ThemeMapToken& map = adqt::theme::ThemeManager::instance().currentMapToken();
  AlertVisualStyle style;

  style.background = toColor(map.colorInfoBg, QColor("#e6f4ff"));
  style.border = toColor(map.colorInfoBorder, QColor("#91caff"));
  style.iconColor = toColor(map.colorInfo, QColor("#1677ff"));
  style.titleColor = toColor(map.colorText, QColor("#141414"));
  style.descriptionColor = toColor(map.colorText, QColor("#141414"));
  style.closeColor = toColor(map.colorTextQuaternary, QColor("#bfbfbf"));
  style.closeHoverColor = toColor(map.colorTextTertiary, QColor("#8c8c8c"));
  style.actionTextColor = style.titleColor;

  switch (input.type) {
    case AdAlert::Type::Success:
      style.background = toColor(map.colorSuccessBg, style.background);
      style.border = toColor(map.colorSuccessBorder, style.border);
      style.iconColor = toColor(map.colorSuccess, style.iconColor);
      break;
    case AdAlert::Type::Info:
      style.background = toColor(map.colorInfoBg, style.background);
      style.border = toColor(map.colorInfoBorder, style.border);
      style.iconColor = toColor(map.colorInfo, style.iconColor);
      break;
    case AdAlert::Type::Warning:
      style.background = toColor(map.colorWarningBg, style.background);
      style.border = toColor(map.colorWarningBorder, style.border);
      style.iconColor = toColor(map.colorWarning, style.iconColor);
      break;
    case AdAlert::Type::Error:
      style.background = toColor(map.colorErrorBg, style.background);
      style.border = toColor(map.colorErrorBorder, style.border);
      style.iconColor = toColor(map.colorError, style.iconColor);
      break;
  }

  style.metrics.borderWidth = std::max(0, qRound(map.lineWidth));
  style.metrics.borderRadius = std::max(0, qRound(map.borderRadiusLG));
  style.metrics.defaultPaddingHorizontal = 12;
  style.metrics.defaultPaddingVertical = std::max(4, qRound(map.sizeXS));
  style.metrics.withDescriptionPadding = std::max(8, qRound(map.sizeMD));
  style.metrics.iconSize = std::max(12, qRound(map.fontSize));
  style.metrics.withDescriptionIconSize = std::max(12, qRound(map.fontSizeHeading3));
  style.metrics.titleDescriptionGap = std::max(2, qRound(map.sizeXXS));
  style.metrics.iconGap = std::max(4, qRound(map.sizeXS));
  style.metrics.actionGap = std::max(4, qRound(map.sizeXS));
  style.metrics.closeGap = std::max(4, qRound(map.sizeXS));
  style.metrics.closeIconSize = std::max(12, qRound(map.fontSize));
  style.metrics.closeButtonSize = std::max(style.metrics.closeIconSize, qRound(map.controlHeightSM));
  style.metrics.animationDurationMs = parseDurationMs(map.motionDurationSlow, 300);

  style.metrics.titleFont = input.baseFont;
  style.metrics.titleFont.setPixelSize(std::max(12, qRound(map.fontSize)));
  style.metrics.titleFont.setWeight(QFont::Normal);

  style.metrics.titleWithDescriptionFont = input.baseFont;
  style.metrics.titleWithDescriptionFont.setPixelSize(std::max(12, qRound(map.fontSizeLG)));
  style.metrics.titleWithDescriptionFont.setWeight(QFont::DemiBold);

  style.metrics.descriptionFont = input.baseFont;
  style.metrics.descriptionFont.setPixelSize(std::max(12, qRound(map.fontSize)));
  style.metrics.descriptionFont.setWeight(QFont::Normal);

  if (input.banner) {
    style.metrics.borderRadius = 0;
    style.metrics.borderWidth = 0;
  }

  if (!map.motion) {
    style.metrics.animationDurationMs = 0;
  }

  const auto& tokens = input.componentTokens;
  if (tokens.defaultPaddingHorizontal.has_value()) {
    style.metrics.defaultPaddingHorizontal = std::max(0, tokens.defaultPaddingHorizontal.value());
  }
  if (tokens.defaultPaddingVertical.has_value()) {
    style.metrics.defaultPaddingVertical = std::max(0, tokens.defaultPaddingVertical.value());
  }
  if (tokens.withDescriptionIconSize.has_value()) {
    style.metrics.withDescriptionIconSize = std::max(8, tokens.withDescriptionIconSize.value());
  }
  if (tokens.withDescriptionPadding.has_value()) {
    style.metrics.withDescriptionPadding = std::max(0, tokens.withDescriptionPadding.value());
  }
  if (tokens.iconSize.has_value()) {
    style.metrics.iconSize = std::max(8, tokens.iconSize.value());
  }
  if (tokens.borderRadius.has_value()) {
    style.metrics.borderRadius = std::max(0, tokens.borderRadius.value());
  }
  if (tokens.actionGap.has_value()) {
    style.metrics.actionGap = std::max(0, tokens.actionGap.value());
  }
  if (tokens.closeGap.has_value()) {
    style.metrics.closeGap = std::max(0, tokens.closeGap.value());
  }
  if (tokens.closeButtonSize.has_value()) {
    style.metrics.closeButtonSize = std::max(8, tokens.closeButtonSize.value());
  }

  const auto& semantic = input.semanticStyles;
  applySemanticSlot(semantic.root, nullptr, &style.background, &style.border);
  applySemanticSlot(semantic.section, &style.titleColor, nullptr, nullptr);
  applySemanticSlot(semantic.icon, &style.iconColor, nullptr, nullptr);
  applySemanticSlot(semantic.title, &style.titleColor, nullptr, nullptr);
  applySemanticSlot(semantic.description, &style.descriptionColor, nullptr, nullptr);
  applySemanticSlot(semantic.actions, &style.actionTextColor, nullptr, nullptr);
  applySemanticSlot(semantic.close, &style.closeColor, nullptr, nullptr);

  return style;
}

}  // namespace adqt::widgets::detail
