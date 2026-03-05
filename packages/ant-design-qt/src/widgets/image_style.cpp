#include "image_style.h"

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

QColor withAlpha(const QColor& color, qreal alpha) {
  QColor output = color;
  output.setAlphaF(std::clamp(alpha, 0.0, 1.0));
  return output;
}

void applySemanticSlot(const AdImage::SemanticSlotStyle& slot,
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

void applySemanticSlot(const AdImagePreviewGroup::SemanticSlotStyle& slot,
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

void applyComponentTokenColors(const AdImage::ComponentTokens& tokens, ImageVisualStyle* style) {
  if (!style) {
    return;
  }
  if (tokens.placeholderBg.has_value()) {
    style->placeholderBackground = toColor(tokens.placeholderBg.value(), style->placeholderBackground);
  }
  if (tokens.placeholderIconColor.has_value()) {
    style->placeholderIcon = toColor(tokens.placeholderIconColor.value(), style->placeholderIcon);
  }
  if (tokens.coverBg.has_value()) {
    style->coverBackground = toColor(tokens.coverBg.value(), style->coverBackground);
  }
  if (tokens.coverColor.has_value()) {
    style->coverText = toColor(tokens.coverColor.value(), style->coverText);
  }
  if (tokens.previewOperationColor.has_value()) {
    style->operationColor = toColor(tokens.previewOperationColor.value(), style->operationColor);
  }
  if (tokens.previewOperationHoverColor.has_value()) {
    style->operationHoverColor =
        toColor(tokens.previewOperationHoverColor.value(), style->operationHoverColor);
  }
  if (tokens.previewOperationColorDisabled.has_value()) {
    style->operationDisabledColor =
        toColor(tokens.previewOperationColorDisabled.value(), style->operationDisabledColor);
  }
}

void applyComponentTokenColors(const AdImagePreviewGroup::ComponentTokens& tokens,
                               ImageVisualStyle* style) {
  if (!style) {
    return;
  }
  if (tokens.previewOperationColor.has_value()) {
    style->operationColor = toColor(tokens.previewOperationColor.value(), style->operationColor);
  }
  if (tokens.previewOperationHoverColor.has_value()) {
    style->operationHoverColor =
        toColor(tokens.previewOperationHoverColor.value(), style->operationHoverColor);
  }
  if (tokens.previewOperationColorDisabled.has_value()) {
    style->operationDisabledColor =
        toColor(tokens.previewOperationColorDisabled.value(), style->operationDisabledColor);
  }
}

ImageVisualStyle baseImageVisualStyle() {
  const adqt::theme::ThemeMapToken& map = adqt::theme::ThemeManager::instance().currentMapToken();

  // Match Ant Design alias token mapping:
  // paddingSM -> sizeSM, paddingLG -> sizeLG, marginSM -> sizeSM, margin -> size.
  const int paddingSM = std::max(4, qRound(map.sizeSM));
  const int paddingLG = std::max(paddingSM, qRound(map.sizeLG));
  const int marginSM = std::max(4, qRound(map.sizeSM));
  const int margin = std::max(8, qRound(map.size));
  const int marginXL = std::max(margin, qRound(map.sizeXL));

  ImageVisualStyle style;
  style.rootBackground = QColor(Qt::transparent);
  style.rootBorder = toColor(map.colorBorderSecondary, QColor(Qt::transparent));
  style.placeholderBackground = toColor(map.colorFillTertiary, QColor("#f5f5f5"));
  style.placeholderIcon = toColor(map.colorTextQuaternary, QColor("#bfbfbf"));
  style.coverBackground = QColor(0, 0, 0, 76);
  style.coverText = toColor(map.colorWhite, QColor("#ffffff"));

  QColor mask = toColor(map.colorBgMask, QColor(0, 0, 0, 115));
  if (mask.alpha() <= 0) {
    mask.setAlpha(115);
  }
  style.popupMask = mask;
  style.popupBodyBackground = QColor(Qt::transparent);
  style.popupFooterText = toColor(map.colorWhite, QColor("#ffffff"));
  style.popupActionsBackground = withAlpha(mask, 0.1);
  style.operationColor = withAlpha(style.popupFooterText, 0.65);
  style.operationHoverColor = withAlpha(style.popupFooterText, 0.85);
  style.operationDisabledColor = withAlpha(style.popupFooterText, 0.25);
  style.operationBorder = QColor(Qt::transparent);

  // Ant Design Image root does not define a default border radius.
  style.metrics.borderRadius = 0;
  style.metrics.coverPadding = std::max(0, qRound(map.sizeSM));
  style.metrics.operationIconSize = std::max(10, qRound(map.fontSizeSM * 1.5));
  style.metrics.footerPadding = paddingSM;
  style.metrics.actionsHorizontalPadding = paddingLG;
  style.metrics.actionsGap = paddingSM;
  style.metrics.footerGap = margin;
  style.metrics.controlOffset = marginSM;
  style.metrics.footerBottomOffset = marginXL;
  style.metrics.operationButtonSize = style.metrics.operationIconSize + paddingSM * 2;
  style.metrics.switchButtonSize = style.metrics.operationButtonSize;
  style.metrics.closeButtonSize = style.metrics.operationButtonSize;
  style.metrics.zIndexPopup = 1080;

  return style;
}

void applyImageComponentTokens(const AdImage::ComponentTokens& tokens, ImageVisualStyle* style) {
  if (!style) {
    return;
  }
  if (tokens.borderRadius.has_value()) {
    style->metrics.borderRadius = std::max(0, tokens.borderRadius.value());
  }
  if (tokens.previewOperationSize.has_value()) {
    const int size = std::max(8, tokens.previewOperationSize.value());
    const int padding = std::max(4, style->metrics.footerPadding);
    const int buttonSize = size + padding * 2;
    style->metrics.operationIconSize = size;
    style->metrics.operationButtonSize = buttonSize;
    style->metrics.switchButtonSize = buttonSize;
    style->metrics.closeButtonSize = buttonSize;
  }
  if (tokens.zIndexPopup.has_value()) {
    style->metrics.zIndexPopup = tokens.zIndexPopup.value();
  }

  applyComponentTokenColors(tokens, style);
}

void applyImageGroupTokens(const AdImagePreviewGroup::ComponentTokens& tokens, ImageVisualStyle* style) {
  if (!style) {
    return;
  }
  if (tokens.previewOperationSize.has_value()) {
    const int size = std::max(8, tokens.previewOperationSize.value());
    const int padding = std::max(4, style->metrics.footerPadding);
    const int buttonSize = size + padding * 2;
    style->metrics.operationIconSize = size;
    style->metrics.operationButtonSize = buttonSize;
    style->metrics.switchButtonSize = buttonSize;
    style->metrics.closeButtonSize = buttonSize;
  }
  if (tokens.zIndexPopup.has_value()) {
    style->metrics.zIndexPopup = tokens.zIndexPopup.value();
  }

  applyComponentTokenColors(tokens, style);
}

}  // namespace

ImageVisualStyle resolveImageVisualStyle(const ImageStyleInput& input) {
  ImageVisualStyle style = baseImageVisualStyle();
  applyImageComponentTokens(input.componentTokens, &style);

  if (!input.previewMaskVisible) {
    style.popupMask = QColor(Qt::transparent);
  } else if (input.previewMaskBlur) {
    QColor mask = style.popupMask;
    mask.setAlpha(std::max(mask.alpha(), 176));
    style.popupMask = mask;
  }

  applySemanticSlot(input.semanticStyles.root, nullptr, &style.rootBackground, &style.rootBorder);
  applySemanticSlot(input.semanticStyles.image, nullptr, nullptr, &style.rootBorder);
  applySemanticSlot(input.semanticStyles.cover, &style.coverText, &style.coverBackground, nullptr);
  applySemanticSlot(input.semanticStyles.popupRoot, nullptr, &style.popupBodyBackground, nullptr);
  applySemanticSlot(input.semanticStyles.popupMask, nullptr, &style.popupMask, nullptr);
  applySemanticSlot(input.semanticStyles.popupBody, nullptr, &style.popupBodyBackground, nullptr);
  applySemanticSlot(input.semanticStyles.popupFooter, &style.popupFooterText, nullptr, nullptr);
  applySemanticSlot(input.semanticStyles.popupActions,
                    &style.operationColor,
                    &style.popupActionsBackground,
                    &style.operationBorder);

  return style;
}

ImageVisualStyle resolveImageGroupVisualStyle(const ImageGroupStyleInput& input) {
  ImageVisualStyle style = baseImageVisualStyle();
  applyImageGroupTokens(input.componentTokens, &style);

  if (!input.previewMaskVisible) {
    style.popupMask = QColor(Qt::transparent);
  } else if (input.previewMaskBlur) {
    QColor mask = style.popupMask;
    mask.setAlpha(std::max(mask.alpha(), 176));
    style.popupMask = mask;
  }

  applySemanticSlot(input.semanticStyles.popupRoot, nullptr, &style.popupBodyBackground, nullptr);
  applySemanticSlot(input.semanticStyles.popupMask, nullptr, &style.popupMask, nullptr);
  applySemanticSlot(input.semanticStyles.popupBody, nullptr, &style.popupBodyBackground, nullptr);
  applySemanticSlot(input.semanticStyles.popupFooter, &style.popupFooterText, nullptr, nullptr);
  applySemanticSlot(input.semanticStyles.popupActions,
                    &style.operationColor,
                    &style.popupActionsBackground,
                    &style.operationBorder);

  return style;
}

}  // namespace adqt::widgets::detail
