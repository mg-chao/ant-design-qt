#pragma once

#include <QColor>
#include <QFont>

#include "color_picker.h"

namespace adqt::widgets::detail {

struct ColorPickerMetrics {
  int controlHeightSM = 24;
  int controlHeight = 32;
  int controlHeightLG = 40;
  int triggerMinWidth = 44;
  int triggerRadius = 8;
  int triggerPadding = 4;
  int triggerTextGap = 8;
  int swatchSizeSM = 14;
  int swatchSize = 20;
  int swatchSizeLG = 26;
  int swatchRadius = 4;
  int panelWidth = 234;
  int panelPadding = 8;
  int panelSpacing = 8;
  int presetSwatchSize = 24;
  int inputHeight = 26;
  int saturationPanelHeight = 160;
  int saturationPanelRadius = 4;
  int sliderHeight = 8;
  int previewSwatchSize = 20;
  int previewSwatchRadius = 4;
  int alphaInputWidth = 44;
  qreal borderWidth = 1.0;
  QFont font;
};

struct ColorPickerVisualStyle {
  QColor triggerBackground;
  QColor triggerBackgroundDisabled;
  QColor triggerBorder;
  QColor triggerBorderHover;
  QColor triggerBorderActive;
  QColor triggerText;
  QColor triggerTextDisabled;
  QColor panelBackground;
  QColor panelBorder;
  QColor panelText;
  QColor swatchBorder;
  QColor presetBorder;
  QColor presetBorderHover;
  QColor transparentCellA;
  QColor transparentCellB;
  ColorPickerMetrics metrics;
};

struct ColorPickerStyleInput {
  AdColorPicker::Size size = AdColorPicker::Size::Middle;
  bool open = false;
  bool disabled = false;
  bool showText = false;
  bool cleared = false;
  QFont baseFont;
  AdColorPicker::ComponentTokens componentTokens;
  AdColorPicker::SemanticStyles semanticStyles;
};

ColorPickerVisualStyle resolveColorPickerVisualStyle(const ColorPickerStyleInput& input);

}  // namespace adqt::widgets::detail
