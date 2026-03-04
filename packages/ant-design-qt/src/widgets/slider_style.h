#pragma once

#include <QBrush>
#include <QColor>
#include <QFont>

#include "slider.h"

namespace adqt::widgets::detail {

struct SliderMetrics {
  int controlSize = 20;
  int railSize = 4;
  int handleSize = 10;
  int handleSizeHover = 12;
  int handleLineWidth = 2;
  int handleLineWidthHover = 2;
  int dotSize = 8;
  int marginMain = 8;
  int marginCross = 10;
  int markGap = 10;
  int focusOutlineSize = 6;
  int tooltipPaddingH = 8;
  int tooltipPaddingV = 4;
  int tooltipRadius = 6;
  int tooltipOffset = 8;
  int tooltipArrowSize = 5;
  QFont font;
};

struct SliderVisualStyle {
  QColor rootBg;
  QColor railBg;
  QColor railHoverBg;
  QColor trackBg;
  QColor trackHoverBg;
  QColor handleColor;
  QColor handleActiveColor;
  QColor handleActiveOutlineColor;
  QColor handleColorDisabled;
  QColor dotBorderColor;
  QColor dotActiveBorderColor;
  QColor trackBgDisabled;
  QColor markColor;
  QColor markActiveColor;
  QColor tooltipBg;
  QColor tooltipText;
  QBrush tracksBrush;
  bool useTracksBrush = false;
  SliderMetrics metrics;
};

struct SliderStyleInput {
  AdSlider::Mode mode = AdSlider::Mode::Single;
  Qt::Orientation orientation = Qt::Horizontal;
  bool hovered = false;
  bool dragging = false;
  bool focused = false;
  bool disabled = false;
  bool reverse = false;
  QFont baseFont;
  AdSlider::ComponentTokens componentTokens;
  AdSlider::SemanticStyles semanticStyles;
};

SliderVisualStyle resolveSliderVisualStyle(const SliderStyleInput& input);

}  // namespace adqt::widgets::detail

