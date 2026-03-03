#pragma once

#include <QPoint>
#include <QRect>
#include <QSize>

class QWidget;

namespace adqt::widgets::detail {

enum class PopupPlacement {
  BottomLeft,
  BottomRight,
  TopLeft,
  TopRight,
  RightTop,
  LeftTop,
};

struct PopupPlacementInput {
  QPoint anchorTopLeft;
  QSize anchorSize;
  QSize popupSize;
  QRect bounds;
  PopupPlacement preferredPlacement = PopupPlacement::BottomLeft;
  QPoint offset;
  bool allowFallback = true;
};

struct PopupPlacementOutput {
  QPoint topLeft;
  PopupPlacement placement = PopupPlacement::BottomLeft;
};

QWidget* resolvePopupScopeWindow(const QWidget* owner);
QRect popupBoundsInGlobal(const QWidget* scopeWindow);
QPoint clampPopupTopLeft(const QPoint& topLeft, const QSize& popupSize, const QRect& bounds);
PopupPlacement oppositePopupPlacement(PopupPlacement placement);
PopupPlacementOutput resolvePopupPlacement(const PopupPlacementInput& input);

}  // namespace adqt::widgets::detail
