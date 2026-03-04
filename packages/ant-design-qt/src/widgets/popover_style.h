#pragma once

#include "popover.h"

#include <QColor>
#include <QFont>

namespace adqt::widgets::detail {

struct PopoverMetrics {
  int titleMinWidth = 177;
  int zIndexPopup = 1030;
  int borderRadius = 8;
  int borderWidth = 1;
  int arrowSize = 8;
  int popupOffset = 8;
  int popupPadding = 12;
  int titlePaddingHorizontal = 0;
  int titlePaddingVertical = 0;
  int titleMarginBottom = 8;
  int contentPaddingHorizontal = 0;
  int contentPaddingVertical = 0;
  QFont titleFont;
  QFont contentFont;
};

struct PopoverVisualStyle {
  QColor rootBackground;
  QColor containerBackground;
  QColor borderColor;
  QColor titleColor;
  QColor contentColor;
  QColor arrowColor;
  PopoverMetrics metrics;
};

struct PopoverStyleInput {
  AdPopover::Placement placement = AdPopover::Placement::Top;
  bool open = false;
  bool disabled = false;
  bool arrowVisible = true;
  QFont baseFont;
  AdPopover::ComponentTokens componentTokens;
  AdPopover::SemanticStyles semanticStyles;
};

PopoverVisualStyle resolvePopoverVisualStyle(const PopoverStyleInput& input);

}  // namespace adqt::widgets::detail
