#pragma once

#include "select.h"

#include <QColor>
#include <QFont>

namespace adqt::widgets::detail {

struct SelectMetrics {
  int height = 32;
  int borderRadius = 6;
  int borderWidth = 1;
  int horizontalPadding = 11;
  int popupMaxHeight = 256;
  int optionHeight = 32;
  int tagHeight = 20;
  int iconSize = 14;
  int spacing = 6;
  QFont font;
};

struct SelectVisualStyle {
  QColor selectorBg;
  QColor selectorBorderColor;
  QColor selectorHoverBorderColor;
  QColor selectorActiveBorderColor;
  QColor selectorTextColor;
  QColor placeholderColor;
  QColor popupBg;
  QColor popupBorderColor;
  QColor optionTextColor;
  QColor optionHoverBg;
  QColor optionSelectedBg;
  QColor optionSelectedColor;
  QColor tagBg;
  QColor tagTextColor;
  QColor clearColor;
  QColor prefixColor;
  QColor suffixColor;
  QColor disabledTextColor;
  QColor disabledBg;
  QColor disabledBorderColor;
  SelectMetrics metrics;
};

struct SelectStyleInput {
  AdSelect::Mode mode = AdSelect::Mode::Single;
  AdSelect::Size size = AdSelect::Size::Middle;
  AdSelect::Variant variant = AdSelect::Variant::Outlined;
  AdSelect::Status status = AdSelect::Status::None;
  bool disabled = false;
  QFont baseFont;
  AdSelect::ComponentTokens componentTokens;
  AdSelect::SemanticStyles semanticStyles;
};

SelectVisualStyle resolveSelectVisualStyle(const SelectStyleInput& input);

}  // namespace adqt::widgets::detail
