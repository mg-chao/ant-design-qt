#pragma once

#include <QColor>
#include <QFont>

#include "input_number.h"

namespace adqt::widgets::detail {

struct InputNumberMetrics {
  int height = 32;
  int width = 90;
  int borderRadius = 6;
  int borderWidth = 1;
  int horizontalPadding = 11;
  int iconSize = 12;
  int handleIconSize = 7;
  int inputFontSize = 14;
  int handleWidth = 22;
  int handleVisibleWidth = 0;
  qreal focusOutlineWidth = 3.0;
  qreal focusOutlineOffset = 0.0;
  QFont font;
};

struct InputNumberVisualStyle {
  QColor selectorBg;
  QColor selectorHoverBg;
  QColor selectorActiveBg;
  QColor selectorBorderColor;
  QColor selectorHoverBorderColor;
  QColor selectorActiveBorderColor;
  QColor selectorFocusOutlineColor;
  QColor selectorTextColor;
  QColor placeholderColor;
  QColor prefixColor;
  QColor suffixColor;
  QColor handleBg;
  QColor handleActiveBg;
  QColor handleBorderColor;
  QColor handleHoverColor;
  QColor handleIconColor;
  QColor outOfRangeTextColor;
  QColor disabledTextColor;
  QColor disabledBg;
  QColor disabledBorderColor;
  bool underlined = false;
  InputNumberMetrics metrics;
};

struct InputNumberStyleInput {
  AdInputNumber::Size size = AdInputNumber::Size::Middle;
  AdInputNumber::Variant variant = AdInputNumber::Variant::Outlined;
  AdInputNumber::Status status = AdInputNumber::Status::None;
  AdInputNumber::Mode mode = AdInputNumber::Mode::Input;
  bool disabled = false;
  bool readOnly = false;
  bool focused = false;
  bool hovered = false;
  bool controlsVisible = true;
  bool outOfRange = false;
  QFont baseFont;
  AdInputNumber::ComponentTokens componentTokens;
  AdInputNumber::SemanticStyles semanticStyles;
};

InputNumberVisualStyle resolveInputNumberVisualStyle(const InputNumberStyleInput& input);

}  // namespace adqt::widgets::detail
