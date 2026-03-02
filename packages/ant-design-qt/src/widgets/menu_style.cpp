#include "menu_style.h"

#include "theme/fast_color_lite.h"
#include "theme/theme.h"

#include <algorithm>

namespace adqt::widgets::detail {

namespace {

using adqt::theme::ThemeMapToken;
using adqt::theme::ThemeManager;

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

QColor withAlpha(const QColor& color, double alpha) {
  QColor copy = color;
  copy.setAlphaF(std::clamp(alpha, 0.0, 1.0));
  return copy;
}

void applyTokenOverrides(MenuMetrics& metrics, const AdMenu::ComponentTokens& tokens) {
  if (tokens.itemHeight.has_value()) {
    metrics.itemHeight = std::max(20, tokens.itemHeight.value());
  }
  if (tokens.itemPaddingInline.has_value()) {
    metrics.itemPaddingInline = std::max(0, tokens.itemPaddingInline.value());
  }
  if (tokens.itemMarginInline.has_value()) {
    metrics.itemMarginInline = std::max(0, tokens.itemMarginInline.value());
  }
  if (tokens.itemMarginBlock.has_value()) {
    metrics.itemMarginBlock = std::max(0, tokens.itemMarginBlock.value());
  }
  if (tokens.itemBorderRadius.has_value()) {
    metrics.itemBorderRadius = std::max(0, tokens.itemBorderRadius.value());
  }
  if (tokens.horizontalItemBorderRadius.has_value()) {
    metrics.horizontalItemBorderRadius = std::max(0, tokens.horizontalItemBorderRadius.value());
  }
  if (tokens.subMenuItemBorderRadius.has_value()) {
    metrics.subMenuItemBorderRadius = std::max(0, tokens.subMenuItemBorderRadius.value());
  }
  if (tokens.inlineIndent.has_value()) {
    metrics.inlineIndent = std::max(0, tokens.inlineIndent.value());
  }
  if (tokens.iconSize.has_value()) {
    metrics.iconSize = std::max(10, tokens.iconSize.value());
  }
  if (tokens.iconMarginInlineEnd.has_value()) {
    metrics.iconMarginInlineEnd = std::max(0, tokens.iconMarginInlineEnd.value());
  }
  if (tokens.activeBarWidth.has_value()) {
    metrics.activeBarWidth = std::max(0, tokens.activeBarWidth.value());
  }
  if (tokens.borderWidth.has_value()) {
    metrics.borderWidth = std::max(0, tokens.borderWidth.value());
  }
  if (tokens.groupTitleFontSize.has_value()) {
    metrics.groupTitleFontSize = std::max(10, tokens.groupTitleFontSize.value());
  }
  if (tokens.groupTitleLineHeight.has_value()) {
    metrics.groupTitleLineHeight =
        std::max(metrics.groupTitleFontSize + 2, tokens.groupTitleLineHeight.value());
  }
}

void applySemanticSlot(AdMenu::SemanticSlotStyle slot, MenuStateStyle& state) {
  if (slot.textColor.has_value()) {
    state.text = slot.textColor.value();
  }
  if (slot.backgroundColor.has_value()) {
    state.background = slot.backgroundColor.value();
  }
}

void applySemanticStyles(const AdMenu::SemanticStyles& semantic, MenuVisualStyle& style) {
  if (semantic.root.backgroundColor.has_value()) {
    style.menuBackground = semantic.root.backgroundColor.value();
  }
  if (semantic.root.borderColor.has_value()) {
    style.borderColor = semantic.root.borderColor.value();
    style.popupBorderColor = semantic.root.borderColor.value();
    style.dividerColor = semantic.root.borderColor.value();
  }

  applySemanticSlot(semantic.item, style.normal);
  applySemanticSlot(semantic.item, style.horizontalNormal);
  applySemanticSlot(semantic.itemContent, style.hover);
  applySemanticSlot(semantic.subMenuItem, style.active);
  applySemanticSlot(semantic.subMenuItem, style.dangerActive);
  if (semantic.itemTitle.textColor.has_value()) {
    style.groupTitleColor = semantic.itemTitle.textColor.value();
  }

  if (semantic.popup.backgroundColor.has_value()) {
    style.popupBackground = semantic.popup.backgroundColor.value();
    style.subMenuBackground = semantic.popup.backgroundColor.value();
  }
  if (semantic.popup.borderColor.has_value()) {
    style.popupBorderColor = semantic.popup.borderColor.value();
  }
}

MenuMetrics resolveMetrics(const ThemeMapToken& map,
                           const QFont& baseFont,
                           const AdMenu::ComponentTokens& tokenOverrides) {
  MenuMetrics metrics;

  metrics.itemHeight = std::max(28, qRound(map.controlHeightLG));
  metrics.itemPaddingInline = std::max(8, qRound(map.sizeMS));
  metrics.itemMarginInline = std::max(2, qRound(map.sizeXXS));
  metrics.itemMarginBlock = std::max(1, qRound(map.sizeXXS));
  metrics.itemBorderRadius = std::max(0, qRound(map.borderRadiusLG));
  metrics.horizontalItemBorderRadius = metrics.itemBorderRadius;
  metrics.subMenuItemBorderRadius = std::max(0, qRound(map.borderRadiusSM));
  metrics.inlineIndent = 24;

  const int itemFontSize = std::max(12, qRound(map.fontSize));
  metrics.font = baseFont;
  metrics.font.setPixelSize(itemFontSize);

  metrics.iconSize = std::max(12, itemFontSize);
  metrics.iconMarginInlineEnd = std::max(6, qRound(map.controlHeightSM - map.fontSize));
  metrics.activeBarWidth = 0;

  metrics.borderWidth = std::max(1, qRound(map.lineWidth));
  metrics.dividerMarginBlock = metrics.borderWidth;

  metrics.groupTitleFontSize = std::max(11, qRound(map.fontSize));
  metrics.groupTitleLineHeight =
      std::max(metrics.groupTitleFontSize + 4, qRound(map.lineHeight * map.fontSize));
  metrics.groupTitleHorizontalPadding = metrics.itemPaddingInline;
  metrics.groupTitleVerticalPadding = std::max(4, qRound(map.sizeXS));
  metrics.horizontalSpacing = std::max(2, qRound(map.sizeXXS));

  applyTokenOverrides(metrics, tokenOverrides);
  return metrics;
}

MenuVisualStyle makeLightStyle(const ThemeMapToken& map,
                               const QFont& baseFont,
                               const AdMenu::ComponentTokens& tokens) {
  MenuVisualStyle style;
  style.metrics = resolveMetrics(map, baseFont, tokens);

  style.menuBackground = toColor(map.colorBgContainer, QColor("#ffffff"));
  style.borderColor = toColor(map.colorBorderSecondary, QColor("#f0f0f0"));
  style.dividerColor = style.borderColor;
  style.groupTitleColor = toColor(map.colorTextTertiary, QColor("#8c8c8c"));
  style.subMenuBackground = toColor(map.colorFillQuaternary, QColor(0, 0, 0, 5));
  style.popupBackground =
      tokens.popupBg.has_value() ? toColor(tokens.popupBg.value(), style.menuBackground)
                                 : toColor(map.colorBgElevated, QColor("#ffffff"));
  style.popupBorderColor = style.borderColor;

  style.normal.text = toColor(map.colorText, QColor("#141414"));
  style.normal.background = QColor(0, 0, 0, 0);

  style.hover.text = tokens.itemHoverColor.has_value()
                         ? toColor(tokens.itemHoverColor.value(), toColor(map.colorText, QColor("#141414")))
                         : toColor(map.colorText, QColor("#141414"));
  style.hover.background = tokens.itemHoverBg.has_value()
                               ? toColor(tokens.itemHoverBg.value(),
                                         toColor(map.colorFillSecondary, QColor("#f5f5f5")))
                               : toColor(map.colorFillSecondary, QColor("#f5f5f5"));

  style.active.text = toColor(map.colorText, QColor("#141414"));
  style.active.background = toColor(map.colorPrimaryBg, QColor("#e6f4ff"));

  style.selected.text = tokens.itemSelectedColor.has_value()
                            ? toColor(tokens.itemSelectedColor.value(), QColor("#1677ff"))
                            : toColor(map.colorPrimary, QColor("#1677ff"));
  style.selected.background = tokens.itemSelectedBg.has_value()
                                  ? toColor(tokens.itemSelectedBg.value(), QColor("#e6f4ff"))
                                  : toColor(map.colorPrimaryBg, QColor("#e6f4ff"));

  style.disabled.text = toColor(map.colorTextQuaternary, QColor("#bfbfbf"));
  style.disabled.background = QColor(0, 0, 0, 0);

  style.danger.text = toColor(map.colorError, QColor("#ff4d4f"));
  style.danger.background = QColor(0, 0, 0, 0);

  style.dangerHover.text = toColor(map.colorError, QColor("#ff4d4f"));
  style.dangerHover.background = style.hover.background;

  style.dangerActive.text = toColor(map.colorError, QColor("#ff4d4f"));
  style.dangerActive.background = toColor(map.colorErrorBg, QColor("#fff2f0"));

  style.dangerSelected.text = toColor(map.colorError, QColor("#ff4d4f"));
  style.dangerSelected.background = toColor(map.colorErrorBg, QColor("#fff2f0"));

  style.horizontalNormal = style.normal;
  style.horizontalHover = style.hover;
  style.horizontalActive = style.horizontalHover;
  const QColor horizontalHoverFallback = style.horizontalHover.text;
  style.horizontalHover.text = tokens.horizontalItemHoverColor.has_value()
                                   ? toColor(tokens.horizontalItemHoverColor.value(),
                                             horizontalHoverFallback)
                                   : horizontalHoverFallback;
  style.horizontalSelected.text = tokens.horizontalItemSelectedColor.has_value()
                                      ? toColor(tokens.horizontalItemSelectedColor.value(),
                                                style.selected.text)
                                      : style.selected.text;
  style.horizontalSelected.background =
      tokens.horizontalItemSelectedBg.has_value()
          ? toColor(tokens.horizontalItemSelectedBg.value(), QColor(0, 0, 0, 0))
          : QColor(0, 0, 0, 0);

  if (tokens.horizontalItemHoverBg.has_value()) {
    style.horizontalHover.background =
        toColor(tokens.horizontalItemHoverBg.value(), style.horizontalHover.background);
  } else {
    style.horizontalHover.background = QColor(0, 0, 0, 0);
  }

  return style;
}

MenuVisualStyle makeDarkStyle(const ThemeMapToken& map,
                              const QFont& baseFont,
                              const AdMenu::ComponentTokens& tokens) {
  MenuVisualStyle style;
  style.metrics = resolveMetrics(map, baseFont, tokens);

  const QColor textDark = toColor(map.colorTextSecondary, withAlpha(QColor("#ffffff"), 0.65));
  const QColor textLight = toColor(map.colorWhite, QColor("#ffffff"));
  const QColor dangerColor = toColor(map.colorError, QColor("#ff4d4f"));

  style.menuBackground = tokens.darkItemBg.has_value() ? toColor(tokens.darkItemBg.value(), QColor("#001529"))
                                                        : QColor("#001529");
  style.borderColor = QColor(0, 0, 0, 0);
  style.dividerColor = withAlpha(QColor("#ffffff"), 0.12);
  style.groupTitleColor = textDark;
  style.subMenuBackground =
      tokens.darkSubMenuItemBg.has_value() ? toColor(tokens.darkSubMenuItemBg.value(), QColor("#000c17"))
                                           : QColor("#000c17");
  style.popupBackground = tokens.darkPopupBg.has_value()
                              ? toColor(tokens.darkPopupBg.value(), style.subMenuBackground)
                              : style.subMenuBackground;
  style.popupBorderColor = QColor(0, 0, 0, 0);

  style.normal.text =
      tokens.darkItemColor.has_value() ? toColor(tokens.darkItemColor.value(), textDark) : textDark;
  style.normal.background = QColor(0, 0, 0, 0);

  style.hover.text = tokens.itemHoverColor.has_value() ? toColor(tokens.itemHoverColor.value(), textLight)
                                                        : textLight;
  style.hover.background = tokens.itemHoverBg.has_value()
                               ? toColor(tokens.itemHoverBg.value(), QColor(0, 0, 0, 0))
                               : QColor(0, 0, 0, 0);

  style.active.text = textLight;
  style.active.background = QColor(0, 0, 0, 0);

  style.selected.text =
      tokens.darkItemSelectedColor.has_value()
          ? toColor(tokens.darkItemSelectedColor.value(), textLight)
          : (tokens.itemSelectedColor.has_value() ? toColor(tokens.itemSelectedColor.value(), textLight)
                                                  : textLight);
  style.selected.background =
      tokens.darkItemSelectedBg.has_value()
          ? toColor(tokens.darkItemSelectedBg.value(), toColor(map.colorPrimary, QColor("#1677ff")))
          : (tokens.itemSelectedBg.has_value()
                 ? toColor(tokens.itemSelectedBg.value(), toColor(map.colorPrimary, QColor("#1677ff")))
                 : toColor(map.colorPrimary, QColor("#1677ff")));

  style.disabled.text = withAlpha(QColor("#ffffff"), 0.25);
  style.disabled.background = QColor(0, 0, 0, 0);

  style.danger.text = dangerColor;
  style.danger.background = QColor(0, 0, 0, 0);

  style.dangerHover.text = toColor(map.colorErrorHover, QColor("#ff7875"));
  style.dangerHover.background = QColor(0, 0, 0, 0);

  style.dangerActive.text = textLight;
  style.dangerActive.background = dangerColor;

  style.dangerSelected.text = textLight;
  style.dangerSelected.background = dangerColor;

  style.horizontalNormal = style.normal;
  style.horizontalHover = style.hover;
  style.horizontalActive = style.active;
  style.horizontalHover.text = tokens.horizontalItemHoverColor.has_value()
                                   ? toColor(tokens.horizontalItemHoverColor.value(), style.horizontalHover.text)
                                   : style.horizontalHover.text;
  style.horizontalSelected.text = tokens.horizontalItemSelectedColor.has_value()
                                      ? toColor(tokens.horizontalItemSelectedColor.value(),
                                                style.selected.text)
                                      : style.selected.text;
  style.horizontalSelected.background =
      tokens.horizontalItemSelectedBg.has_value()
          ? toColor(tokens.horizontalItemSelectedBg.value(), style.selected.background)
          : style.selected.background;

  if (tokens.horizontalItemHoverBg.has_value()) {
    style.horizontalHover.background =
        toColor(tokens.horizontalItemHoverBg.value(), style.horizontalHover.background);
  }

  return style;
}

}  // namespace

MenuVisualStyle resolveMenuVisualStyle(const MenuStyleInput& input) {
  const ThemeMapToken& map = ThemeManager::instance().currentMapToken();
  MenuVisualStyle style = input.theme == AdMenu::MenuTheme::Dark
                              ? makeDarkStyle(map, input.baseFont, input.componentTokens)
                              : makeLightStyle(map, input.baseFont, input.componentTokens);

  style.metrics.inlineIndent = std::max(0, input.componentTokens.inlineIndent.value_or(style.metrics.inlineIndent));

  applySemanticStyles(input.semanticStyles, style);
  return style;
}

}  // namespace adqt::widgets::detail
