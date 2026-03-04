#pragma once

#include "radio.h"

#include <QColor>
#include <QFont>

namespace adqt::widgets::detail {

struct RadioDotStateStyle {
  QColor borderColor;
  QColor backgroundColor;
  QColor dotColor;
  QColor labelColor;
};

struct RadioButtonStateStyle {
  QColor textColor;
  QColor backgroundColor;
  QColor borderColor;
};

struct RadioMetrics {
  int radioSize = 16;
  int dotSize = 8;
  int borderWidth = 1;
  int labelPaddingInlineStart = 8;
  int labelPaddingInlineEnd = 8;
  int textLineHeight = 22;
  int wrapperMarginInlineEnd = 8;
  int buttonHeight = 32;
  int buttonPaddingInline = 12;
  int buttonBorderRadius = 6;
  QFont font;
  QColor focusOutlineColor;
  qreal focusOutlineWidth = 3.0;
  qreal focusOutlineOffset = 1.0;
};

struct RadioVisualStyle {
  RadioDotStateStyle dotNormal;
  RadioDotStateStyle dotHover;
  RadioDotStateStyle dotActive;
  RadioDotStateStyle dotChecked;
  RadioDotStateStyle dotCheckedHover;
  RadioDotStateStyle dotDisabled;
  RadioDotStateStyle dotCheckedDisabled;

  RadioButtonStateStyle buttonNormal;
  RadioButtonStateStyle buttonHover;
  RadioButtonStateStyle buttonActive;
  RadioButtonStateStyle buttonChecked;
  RadioButtonStateStyle buttonCheckedHover;
  RadioButtonStateStyle buttonCheckedActive;
  RadioButtonStateStyle buttonDisabled;
  RadioButtonStateStyle buttonCheckedDisabled;

  RadioMetrics metrics;
};

struct RadioStyleInput {
  AdRadio::Size size = AdRadio::Size::Middle;
  AdRadio::OptionType optionType = AdRadio::OptionType::Default;
  AdRadio::ButtonStyle buttonStyle = AdRadio::ButtonStyle::Outline;
  bool checked = false;
  bool hovered = false;
  bool pressed = false;
  bool disabled = false;
  bool focused = false;
  bool block = false;
  QFont baseFont;
  AdRadio::ComponentTokens componentTokens;
  AdRadio::SemanticStyles semanticStyles;
};

RadioVisualStyle resolveRadioVisualStyle(const RadioStyleInput& input);

}  // namespace adqt::widgets::detail
