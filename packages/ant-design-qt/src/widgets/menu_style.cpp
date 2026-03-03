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

QColor resolveTokenColor(const std::optional<QString>& token, const QColor& fallback) {
  if (!token.has_value()) {
    return fallback;
  }
  return toColor(token.value(), fallback);
}

QColor resolveTokenColorChain(const std::optional<QString>& primaryToken,
                             const std::optional<QString>& secondaryToken,
                             const QColor& fallback) {
  if (primaryToken.has_value()) {
    return toColor(primaryToken.value(), fallback);
  }
  if (secondaryToken.has_value()) {
    return toColor(secondaryToken.value(), fallback);
  }
  return fallback;
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
    metrics.groupTitleLineHeight = std::max(metrics.groupTitleFontSize, tokens.groupTitleLineHeight.value());
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
  if (semantic.subMenuItemTitle.textColor.has_value()) {
    style.subMenuItemSelectedColor = semantic.subMenuItemTitle.textColor.value();
  }

  if (semantic.subMenuList.backgroundColor.has_value()) {
    style.subMenuBackground = semantic.subMenuList.backgroundColor.value();
  }
  if (semantic.popup.backgroundColor.has_value()) {
    style.popupBackground = semantic.popup.backgroundColor.value();
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
  metrics.horizontalLineHeight = std::max(metrics.itemHeight, qRound(map.controlHeightLG * 1.15));
  metrics.itemPaddingInline = std::max(8, qRound(map.sizeMS));
  metrics.itemMarginInline = std::max(2, qRound(map.sizeXXS));
  metrics.itemMarginBlock = std::max(1, qRound(map.sizeXXS));
  metrics.itemBorderRadius = std::max(0, qRound(map.borderRadiusLG));
  // antd default for horizontal mode is radius 0.
  metrics.horizontalItemBorderRadius = 0;
  metrics.subMenuItemBorderRadius = std::max(0, qRound(map.borderRadiusSM));
  metrics.popupBorderRadius = std::max(0, qRound(map.borderRadiusLG));
  metrics.inlineIndent = 24;

  const int itemFontSize = std::max(12, qRound(map.fontSize));
  metrics.font = baseFont;
  metrics.font.setPixelSize(itemFontSize);

  metrics.iconSize = std::max(12, itemFontSize);
  metrics.iconMarginInlineEnd = std::max(6, qRound(map.controlHeightSM - map.fontSize));
  metrics.activeBarWidth = 0;
  metrics.activeBarHeight = std::max(1, qRound(map.lineWidthBold));

  metrics.borderWidth = std::max(1, qRound(map.lineWidth));
  metrics.dividerMarginBlock = metrics.borderWidth;

  metrics.groupTitleFontSize = std::max(10, qRound(map.fontSize));
  metrics.groupTitleLineHeight =
      std::max(metrics.groupTitleFontSize, qRound(map.lineHeight * metrics.groupTitleFontSize));
  metrics.groupTitleHorizontalPadding = metrics.itemPaddingInline;
  metrics.groupTitleVerticalPadding = std::max(4, qRound(map.sizeXS));
  metrics.popupPlacementGap = std::max(0, qRound(map.sizeXS));
  metrics.horizontalSpacing = 0;

  applyTokenOverrides(metrics, tokenOverrides);
  // Keep horizontal line-height at least item height, matching antd horizontal defaults.
  metrics.horizontalLineHeight =
      std::max(metrics.itemHeight, std::max(1, qRound(map.controlHeightLG * 1.15)));
  return metrics;
}

MenuVisualStyle makeLightStyle(const ThemeMapToken& map,
                               const QFont& baseFont,
                               const AdMenu::ComponentTokens& tokens) {
  MenuVisualStyle style;
  style.metrics = resolveMetrics(map, baseFont, tokens);

  style.menuBackground =
      resolveTokenColor(tokens.itemBg, toColor(map.colorBgContainer, QColor("#ffffff")));
  style.borderColor = toColor(map.colorBorderSecondary, QColor("#f0f0f0"));
  style.dividerColor = style.borderColor;
  style.groupTitleColor = toColor(map.colorTextTertiary, QColor("#8c8c8c"));
  style.subMenuBackground =
      resolveTokenColor(tokens.subMenuItemBg, toColor(map.colorFillQuaternary, QColor("#fafafa")));
  style.popupBackground = resolveTokenColor(tokens.popupBg, toColor(map.colorBgElevated, QColor("#ffffff")));
  style.popupBorderColor = style.borderColor;

  style.normal.text = toColor(map.colorText, QColor("#141414"));
  style.normal.background = QColor(0, 0, 0, 0);

  style.hover.text = resolveTokenColor(tokens.itemHoverColor, toColor(map.colorText, QColor("#141414")));
  style.hover.background =
      resolveTokenColor(tokens.itemHoverBg, toColor(map.colorFillSecondary, QColor("#f5f5f5")));

  style.active.text = toColor(map.colorText, QColor("#141414"));
  style.active.background =
      resolveTokenColor(tokens.itemActiveBg, toColor(map.colorFillTertiary, QColor("#f0f0f0")));

  style.selected.text =
      resolveTokenColor(tokens.itemSelectedColor, toColor(map.colorPrimary, QColor("#1677ff")));
  style.selected.background =
      resolveTokenColor(tokens.itemSelectedBg, toColor(map.colorPrimaryBg, QColor("#e6f4ff")));
  style.subMenuItemSelectedColor = resolveTokenColor(tokens.subMenuItemSelectedColor, style.selected.text);

  style.disabled.text =
      resolveTokenColor(tokens.itemDisabledColor, toColor(map.colorTextQuaternary, QColor("#bfbfbf")));
  style.disabled.background = QColor(0, 0, 0, 0);

  style.danger.text =
      resolveTokenColor(tokens.dangerItemColor, toColor(map.colorError, QColor("#ff4d4f")));
  style.danger.background = QColor(0, 0, 0, 0);

  style.dangerHover.text = resolveTokenColor(tokens.dangerItemHoverColor, style.danger.text);
  style.dangerHover.background = style.hover.background;

  style.dangerActive.text = style.danger.text;
  style.dangerActive.background =
      resolveTokenColor(tokens.dangerItemActiveBg, toColor(map.colorErrorBg, QColor("#fff2f0")));

  style.dangerSelected.text = resolveTokenColor(tokens.dangerItemSelectedColor, style.danger.text);
  style.dangerSelected.background =
      resolveTokenColor(tokens.dangerItemSelectedBg, toColor(map.colorErrorBg, QColor("#fff2f0")));

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

  const QColor primaryColor = toColor(map.colorPrimary, QColor("#1677ff"));
  const QColor textLight = toColor(map.colorWhite, QColor("#ffffff"));
  const QColor textDark = withAlpha(textLight, 0.65);
  const QColor dangerColor = toColor(map.colorError, QColor("#ff4d4f"));
  const QColor dangerHoverColor = toColor(map.colorErrorHover, QColor("#ff7875"));
  const QColor transparent(0, 0, 0, 0);

  style.menuBackground = resolveTokenColor(tokens.darkItemBg, QColor("#001529"));
  style.borderColor = transparent;
  style.dividerColor = withAlpha(textLight, 0.12);
  style.groupTitleColor = resolveTokenColor(tokens.darkGroupTitleColor, textDark);
  style.subMenuBackground =
      resolveTokenColorChain(tokens.darkSubMenuItemBg, tokens.subMenuItemBg, QColor("#000c17"));
  style.popupBackground = resolveTokenColorChain(tokens.darkPopupBg, tokens.popupBg, style.menuBackground);
  style.popupBorderColor = toColor(map.colorBorderSecondary, withAlpha(textLight, 0.12));

  style.normal.text = resolveTokenColor(tokens.darkItemColor, textDark);
  style.normal.background = transparent;

  style.hover.text = resolveTokenColorChain(tokens.darkItemHoverColor, tokens.itemHoverColor, textLight);
  style.hover.background =
      resolveTokenColorChain(tokens.darkItemHoverBg, tokens.itemHoverBg, transparent);

  style.active.text = textLight;
  style.active.background = resolveTokenColor(tokens.itemActiveBg, transparent);

  style.selected.text =
      resolveTokenColorChain(tokens.darkItemSelectedColor, tokens.itemSelectedColor, textLight);
  style.selected.background =
      resolveTokenColorChain(tokens.darkItemSelectedBg, tokens.itemSelectedBg, primaryColor);
  style.subMenuItemSelectedColor =
      resolveTokenColor(tokens.subMenuItemSelectedColor, style.selected.text);

  style.disabled.text =
      resolveTokenColorChain(tokens.darkItemDisabledColor, tokens.itemDisabledColor, withAlpha(textLight, 0.25));
  style.disabled.background = transparent;

  style.danger.text = resolveTokenColorChain(tokens.darkDangerItemColor, tokens.dangerItemColor, dangerColor);
  style.danger.background = transparent;

  style.dangerHover.text = resolveTokenColorChain(tokens.darkDangerItemHoverColor,
                                                  tokens.dangerItemHoverColor,
                                                  dangerHoverColor);
  style.dangerHover.background = style.hover.background;

  style.dangerActive.text =
      resolveTokenColorChain(tokens.darkDangerItemSelectedColor, tokens.dangerItemSelectedColor, textLight);
  style.dangerActive.background =
      resolveTokenColorChain(tokens.darkDangerItemActiveBg, tokens.dangerItemActiveBg, dangerColor);

  style.dangerSelected.text = style.dangerActive.text;
  style.dangerSelected.background =
      resolveTokenColorChain(tokens.darkDangerItemSelectedBg, tokens.dangerItemSelectedBg, dangerColor);

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

  // Match antd dark token override: menuDarkToken.activeBarHeight = 0.
  style.metrics.activeBarHeight = 0;

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
