#pragma once

#include <QColor>
#include <QFont>

#include "alert.h"

namespace adqt::widgets::detail {

struct AlertMetrics {
  int borderWidth = 1;
  int borderRadius = 8;
  int defaultPaddingHorizontal = 12;
  int defaultPaddingVertical = 8;
  int withDescriptionPadding = 16;
  int iconSize = 14;
  int withDescriptionIconSize = 24;
  int titleDescriptionGap = 4;
  int iconGap = 8;
  int actionGap = 8;
  int closeGap = 8;
  int closeIconSize = 14;
  int closeButtonSize = 20;
  int animationDurationMs = 300;
  QFont titleFont;
  QFont titleWithDescriptionFont;
  QFont descriptionFont;
};

struct AlertVisualStyle {
  QColor background;
  QColor border;
  QColor titleColor;
  QColor descriptionColor;
  QColor iconColor;
  QColor closeColor;
  QColor closeHoverColor;
  QColor actionTextColor;
  AlertMetrics metrics;
};

struct AlertStyleInput {
  AdAlert::Type type = AdAlert::Type::Info;
  bool banner = false;
  bool showIcon = false;
  bool closable = false;
  bool hasDescription = false;
  bool open = true;
  QFont baseFont;
  AdAlert::ComponentTokens componentTokens;
  AdAlert::SemanticStyles semanticStyles;
};

AlertVisualStyle resolveAlertVisualStyle(const AlertStyleInput& input);

}  // namespace adqt::widgets::detail
