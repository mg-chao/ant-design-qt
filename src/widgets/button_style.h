#pragma once

#include "button.h"

#include <QColor>
#include <QFont>

namespace adqt::widgets::detail {

struct ButtonStateStyle {
  QColor text;
  QColor background;
  QColor border;
  QColor shadow;
  Qt::PenStyle borderStyle = Qt::SolidLine;
};

struct ButtonMetrics {
  int height = 32;
  int horizontalPadding = 14;
  int borderRadius = 6;
  int borderWidth = 1;
  int shadowOffsetY = 2;
  int iconGap = 8;
  QFont font;
  QColor focusOutline;
  qreal focusOutlineWidth = 3.0;
  qreal focusOutlineOffset = 1.0;
};

struct ResolvedRole {
  AdButton::Color color = AdButton::Color::Default;
  AdButton::Variant variant = AdButton::Variant::Outlined;
  bool ghost = false;
  bool unbordered = false;
};

struct ButtonVisualStyle {
  ButtonStateStyle normal;
  ButtonStateStyle hover;
  ButtonStateStyle active;
  ButtonStateStyle disabled;
  ButtonMetrics metrics;
  ResolvedRole role;
};

struct ButtonStyleInput {
  AdButton::Type type = AdButton::Type::Default;
  AdButton::Color color = AdButton::Color::Default;
  AdButton::Variant variant = AdButton::Variant::Outlined;
  AdButton::Size size = AdButton::Size::Middle;

  bool colorExplicit = false;
  bool variantExplicit = false;
  bool danger = false;
  bool ghost = false;

  QFont baseFont;
};

ResolvedRole resolveRole(const ButtonStyleInput& input);
ButtonVisualStyle resolveButtonVisualStyle(const ButtonStyleInput& input);

}  // namespace adqt::widgets::detail
