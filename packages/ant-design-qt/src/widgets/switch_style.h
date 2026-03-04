#pragma once

#include "switch.h"

#include <QColor>

namespace adqt::widgets::detail {

struct SwitchMetrics {
  int trackHeight = 22;
  int trackHeightSM = 16;
  int trackMinWidth = 44;
  int trackMinWidthSM = 28;
  int trackPadding = 2;
  int handleSize = 18;
  int handleSizeSM = 12;
  int innerMinMargin = 9;
  int innerMaxMargin = 24;
  int innerMinMarginSM = 6;
  int innerMaxMarginSM = 18;
  int loadingIconSize = 10;
  int fontSize = 12;
  int animationDurationMs = 200;
  qreal disabledOpacity = 0.65;
  QColor focusOutlineColor;
  qreal focusOutlineWidth = 3.0;
  qreal focusOutlineOffset = 1.0;
  QColor handleShadowColor;
  qreal handleShadowBlur = 4.0;
  qreal handleShadowOffsetY = 2.0;
  qreal handleActiveInsetRatio = 0.3;
  int innerContentActiveOffset = 4;
  int innerContentActiveOffsetSM = 2;
};

struct SwitchVisualStyle {
  QColor trackBg;
  QColor trackHoverBg;
  QColor trackCheckedBg;
  QColor trackCheckedHoverBg;
  QColor trackBorderColor;
  QColor handleBg;
  QColor handleBorderColor;
  QColor contentColor;
  QColor loadingIconColor;
  QColor checkedLoadingIconColor;
  QColor waveColor;
  SwitchMetrics metrics;
};

struct SwitchStyleInput {
  AdSwitch::Size size = AdSwitch::Size::Default;
  bool checked = false;
  bool loading = false;
  bool disabled = false;
  bool hovered = false;
  bool pressed = false;
  bool focused = false;
  AdSwitch::ComponentTokens componentTokens;
  AdSwitch::SemanticStyles semanticStyles;
};

SwitchVisualStyle resolveSwitchVisualStyle(const SwitchStyleInput& input);

}  // namespace adqt::widgets::detail
