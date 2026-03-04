#pragma once

#include <QColor>
#include <QFont>

#include "input.h"

namespace adqt::widgets::detail {

struct InputMetrics {
  int height = 32;
  int borderRadius = 6;
  int borderWidth = 1;
  int horizontalPadding = 11;
  int verticalPadding = 4;
  int iconSize = 14;
  int countTopMargin = 4;
  int countHeight = 20;
  qreal focusOutlineWidth = 3.0;
  qreal focusOutlineOffset = 0.0;
  QFont font;
};

struct InputVisualStyle {
  QColor selectorBg;
  QColor selectorHoverBg;
  QColor selectorActiveBg;
  QColor selectorBorderColor;
  QColor selectorHoverBorderColor;
  QColor selectorActiveBorderColor;
  QColor selectorFocusOutlineColor;
  QColor selectorTextColor;
  QColor placeholderColor;
  QColor clearColor;
  QColor clearHoverColor;
  QColor prefixColor;
  QColor suffixColor;
  QColor countColor;
  QColor disabledTextColor;
  QColor disabledBg;
  QColor disabledBorderColor;
  bool underlined = false;
  InputMetrics metrics;
};

struct InputStyleInput {
  AdInput::Size size = AdInput::Size::Middle;
  AdInput::Variant variant = AdInput::Variant::Outlined;
  AdInput::Status status = AdInput::Status::None;
  bool disabled = false;
  bool focused = false;
  bool hovered = false;
  bool multiline = false;
  QFont baseFont;
  AdInput::ComponentTokens componentTokens;
  AdInput::SemanticStyles semanticStyles;
};

InputVisualStyle resolveInputVisualStyle(const InputStyleInput& input);

}  // namespace adqt::widgets::detail
