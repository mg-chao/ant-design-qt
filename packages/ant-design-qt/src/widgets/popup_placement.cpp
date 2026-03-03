#include "popup_placement.h"

#include <QCursor>
#include <QGuiApplication>
#include <QScreen>
#include <QWidget>

#include <algorithm>

namespace adqt::widgets::detail {

namespace {

QRect widgetGlobalRect(const QWidget* widget) {
  if (!widget) {
    return QRect();
  }
  return QRect(widget->mapToGlobal(QPoint(0, 0)), widget->size());
}

QPoint placementTopLeft(PopupPlacement placement,
                        const QPoint& anchorTopLeft,
                        const QSize& anchorSize,
                        const QSize& popupSize) {
  const int anchorWidth = std::max(0, anchorSize.width());
  const int anchorHeight = std::max(0, anchorSize.height());
  const int popupWidth = std::max(1, popupSize.width());
  const int popupHeight = std::max(1, popupSize.height());

  switch (placement) {
    case PopupPlacement::BottomLeft:
      return QPoint(anchorTopLeft.x(), anchorTopLeft.y() + anchorHeight);
    case PopupPlacement::BottomRight:
      return QPoint(anchorTopLeft.x() + anchorWidth - popupWidth, anchorTopLeft.y() + anchorHeight);
    case PopupPlacement::TopLeft:
      return QPoint(anchorTopLeft.x(), anchorTopLeft.y() - popupHeight);
    case PopupPlacement::TopRight:
      return QPoint(anchorTopLeft.x() + anchorWidth - popupWidth, anchorTopLeft.y() - popupHeight);
    case PopupPlacement::RightTop:
      return QPoint(anchorTopLeft.x() + anchorWidth, anchorTopLeft.y());
    case PopupPlacement::LeftTop:
      return QPoint(anchorTopLeft.x() - popupWidth, anchorTopLeft.y());
  }
  return anchorTopLeft;
}

int overflowCost(const QPoint& topLeft, const QSize& popupSize, const QRect& bounds) {
  if (!bounds.isValid()) {
    return 0;
  }

  const int popupWidth = std::max(1, popupSize.width());
  const int popupHeight = std::max(1, popupSize.height());

  const int leftOverflow = std::max(0, bounds.left() - topLeft.x());
  const int topOverflow = std::max(0, bounds.top() - topLeft.y());
  const int rightOverflow =
      std::max(0, topLeft.x() + popupWidth - (bounds.right() + 1));
  const int bottomOverflow =
      std::max(0, topLeft.y() + popupHeight - (bounds.bottom() + 1));

  return leftOverflow + topOverflow + rightOverflow + bottomOverflow;
}

QRect fallbackBounds() {
  QScreen* screen = QGuiApplication::screenAt(QCursor::pos());
  if (!screen) {
    screen = QGuiApplication::primaryScreen();
  }
  return screen ? screen->availableGeometry() : QRect(0, 0, 1920, 1080);
}

}  // namespace

QWidget* resolvePopupScopeWindow(const QWidget* owner) {
  if (!owner) {
    return nullptr;
  }
  QWidget* scopeWindow = owner->window();
  return scopeWindow ? scopeWindow : const_cast<QWidget*>(owner);
}

QRect popupBoundsInGlobal(const QWidget* scopeWindow) {
  if (scopeWindow) {
    const QRect scopeRect = widgetGlobalRect(scopeWindow);
    if (scopeRect.isValid()) {
      return scopeRect;
    }
  }
  return fallbackBounds();
}

QPoint clampPopupTopLeft(const QPoint& topLeft, const QSize& popupSize, const QRect& bounds) {
  if (!bounds.isValid()) {
    return topLeft;
  }

  const int popupWidth = std::max(1, popupSize.width());
  const int popupHeight = std::max(1, popupSize.height());

  const int minX = bounds.left();
  const int minY = bounds.top();
  const int maxX = std::max(minX, bounds.right() - popupWidth + 1);
  const int maxY = std::max(minY, bounds.bottom() - popupHeight + 1);
  return QPoint(std::clamp(topLeft.x(), minX, maxX), std::clamp(topLeft.y(), minY, maxY));
}

PopupPlacement oppositePopupPlacement(PopupPlacement placement) {
  switch (placement) {
    case PopupPlacement::BottomLeft:
      return PopupPlacement::TopLeft;
    case PopupPlacement::BottomRight:
      return PopupPlacement::TopRight;
    case PopupPlacement::TopLeft:
      return PopupPlacement::BottomLeft;
    case PopupPlacement::TopRight:
      return PopupPlacement::BottomRight;
    case PopupPlacement::RightTop:
      return PopupPlacement::LeftTop;
    case PopupPlacement::LeftTop:
      return PopupPlacement::RightTop;
  }
  return placement;
}

PopupPlacementOutput resolvePopupPlacement(const PopupPlacementInput& input) {
  PopupPlacementOutput out;
  out.placement = input.preferredPlacement;

  const QSize popupSize(std::max(1, input.popupSize.width()), std::max(1, input.popupSize.height()));
  const QRect bounds = input.bounds.isValid() ? input.bounds : fallbackBounds();

  QPoint preferredTopLeft = placementTopLeft(
      input.preferredPlacement, input.anchorTopLeft, input.anchorSize, popupSize);
  preferredTopLeft += input.offset;

  QPoint selectedTopLeft = preferredTopLeft;

  if (input.allowFallback) {
    const PopupPlacement fallbackPlacement = oppositePopupPlacement(input.preferredPlacement);
    if (fallbackPlacement != input.preferredPlacement) {
      QPoint fallbackTopLeft =
          placementTopLeft(fallbackPlacement, input.anchorTopLeft, input.anchorSize, popupSize);
      fallbackTopLeft += input.offset;

      const int preferredCost = overflowCost(preferredTopLeft, popupSize, bounds);
      const int fallbackCost = overflowCost(fallbackTopLeft, popupSize, bounds);
      if (fallbackCost < preferredCost) {
        out.placement = fallbackPlacement;
        selectedTopLeft = fallbackTopLeft;
      }
    }
  }

  out.topLeft = clampPopupTopLeft(selectedTopLeft, popupSize, bounds);
  return out;
}

}  // namespace adqt::widgets::detail
