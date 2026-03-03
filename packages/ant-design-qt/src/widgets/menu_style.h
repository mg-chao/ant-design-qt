#pragma once

#include "menu.h"

#include <QColor>
#include <QFont>

namespace adqt::widgets::detail {

struct MenuStateStyle {
  QColor text;
  QColor background;
};

struct MenuMetrics {
  int itemHeight = 40;
  int horizontalLineHeight = 46;
  int itemPaddingInline = 16;
  int itemMarginInline = 4;
  int itemMarginBlock = 4;
  int itemBorderRadius = 8;
  int horizontalItemBorderRadius = 0;
  int subMenuItemBorderRadius = 6;
  int popupBorderRadius = 8;
  int inlineIndent = 24;

  int iconSize = 14;
  int iconMarginInlineEnd = 10;
  int activeBarWidth = 0;
  int activeBarHeight = 2;

  int borderWidth = 1;
  int dividerMarginBlock = 1;
  int groupTitleFontSize = 14;
  int groupTitleLineHeight = 22;
  int groupTitleHorizontalPadding = 16;
  int groupTitleVerticalPadding = 8;
  int popupPlacementGap = 8;
  int horizontalSpacing = 0;
  QFont font;
};

struct MenuVisualStyle {
  QColor menuBackground;
  QColor borderColor;
  QColor dividerColor;
  QColor groupTitleColor;
  QColor subMenuItemSelectedColor;
  QColor subMenuBackground;
  QColor popupBackground;
  QColor popupBorderColor;

  MenuStateStyle normal;
  MenuStateStyle hover;
  MenuStateStyle active;
  MenuStateStyle selected;
  MenuStateStyle disabled;
  MenuStateStyle danger;
  MenuStateStyle dangerHover;
  MenuStateStyle dangerActive;
  MenuStateStyle dangerSelected;

  MenuStateStyle horizontalNormal;
  MenuStateStyle horizontalHover;
  MenuStateStyle horizontalActive;
  MenuStateStyle horizontalSelected;

  MenuMetrics metrics;
};

struct MenuStyleInput {
  AdMenu::Mode mode = AdMenu::Mode::Vertical;
  AdMenu::MenuTheme theme = AdMenu::MenuTheme::Light;
  bool inlineCollapsed = false;
  QFont baseFont;
  AdMenu::ComponentTokens componentTokens;
  AdMenu::SemanticStyles semanticStyles;
};

MenuVisualStyle resolveMenuVisualStyle(const MenuStyleInput& input);

}  // namespace adqt::widgets::detail
