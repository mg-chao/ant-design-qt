#include "popover.h"

#include "detail/timing_hub.h"
#include "popover_style.h"
#include "popup_placement.h"

#include <QApplication>
#include <QChildEvent>
#include <QContextMenuEvent>
#include <QCursor>
#include <QEvent>
#include <QFocusEvent>
#include <QHideEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QKeyEvent>
#include <QLayoutItem>
#include <QMouseEvent>
#include <QMoveEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPointer>
#include <QResizeEvent>
#include <QShowEvent>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>

namespace adqt::widgets {

namespace {

constexpr char kHoverOpenTaskKey[] = "AdPopover.HoverOpen";
constexpr char kHoverCloseTaskKey[] = "AdPopover.HoverClose";
constexpr char kFocusRecheckTaskKey[] = "AdPopover.FocusRecheck";

QRect widgetGlobalRect(const QWidget* widget) {
  if (!widget) {
    return QRect();
  }
  return QRect(widget->mapToGlobal(QPoint(0, 0)), widget->size());
}

bool widgetContainsGlobalPos(const QWidget* widget, const QPoint& globalPos) {
  const QRect rect = widgetGlobalRect(widget);
  return rect.isValid() && rect.contains(globalPos);
}

bool widgetInTree(const QWidget* candidate, const QWidget* root) {
  if (!candidate || !root) {
    return false;
  }
  return candidate == root || root->isAncestorOf(const_cast<QWidget*>(candidate));
}

enum class ArrowSide {
  None,
  Top,
  Bottom,
  Left,
  Right,
};

ArrowSide arrowSideForPlacement(AdPopover::Placement placement) {
  switch (placement) {
    case AdPopover::Placement::Top:
    case AdPopover::Placement::TopLeft:
    case AdPopover::Placement::TopRight:
      return ArrowSide::Bottom;
    case AdPopover::Placement::Bottom:
    case AdPopover::Placement::BottomLeft:
    case AdPopover::Placement::BottomRight:
      return ArrowSide::Top;
    case AdPopover::Placement::Left:
    case AdPopover::Placement::LeftTop:
    case AdPopover::Placement::LeftBottom:
      return ArrowSide::Right;
    case AdPopover::Placement::Right:
    case AdPopover::Placement::RightTop:
    case AdPopover::Placement::RightBottom:
      return ArrowSide::Left;
  }
  return ArrowSide::None;
}

bool isVerticalPlacement(AdPopover::Placement placement) {
  switch (placement) {
    case AdPopover::Placement::Top:
    case AdPopover::Placement::TopLeft:
    case AdPopover::Placement::TopRight:
    case AdPopover::Placement::Bottom:
    case AdPopover::Placement::BottomLeft:
    case AdPopover::Placement::BottomRight:
      return true;
    default:
      return false;
  }
}

bool supportsCrossAxisAutoShift(AdPopover::Placement placement) {
  return placement == AdPopover::Placement::Top || placement == AdPopover::Placement::Bottom ||
         placement == AdPopover::Placement::Left || placement == AdPopover::Placement::Right;
}

AdPopover::Placement oppositePlacement(AdPopover::Placement placement) {
  switch (placement) {
    case AdPopover::Placement::Top:
      return AdPopover::Placement::Bottom;
    case AdPopover::Placement::TopLeft:
      return AdPopover::Placement::BottomLeft;
    case AdPopover::Placement::TopRight:
      return AdPopover::Placement::BottomRight;
    case AdPopover::Placement::Bottom:
      return AdPopover::Placement::Top;
    case AdPopover::Placement::BottomLeft:
      return AdPopover::Placement::TopLeft;
    case AdPopover::Placement::BottomRight:
      return AdPopover::Placement::TopRight;
    case AdPopover::Placement::Left:
      return AdPopover::Placement::Right;
    case AdPopover::Placement::LeftTop:
      return AdPopover::Placement::RightTop;
    case AdPopover::Placement::LeftBottom:
      return AdPopover::Placement::RightBottom;
    case AdPopover::Placement::Right:
      return AdPopover::Placement::Left;
    case AdPopover::Placement::RightTop:
      return AdPopover::Placement::LeftTop;
    case AdPopover::Placement::RightBottom:
      return AdPopover::Placement::LeftBottom;
  }
  return placement;
}

QPoint placementTopLeft(AdPopover::Placement placement,
                        const QRect& anchorRect,
                        const QSize& popupSize) {
  const int popupWidth = std::max(1, popupSize.width());
  const int popupHeight = std::max(1, popupSize.height());
  switch (placement) {
    case AdPopover::Placement::Top:
      return QPoint(anchorRect.center().x() - popupWidth / 2, anchorRect.top() - popupHeight);
    case AdPopover::Placement::TopLeft:
      return QPoint(anchorRect.left(), anchorRect.top() - popupHeight);
    case AdPopover::Placement::TopRight:
      return QPoint(anchorRect.right() - popupWidth + 1, anchorRect.top() - popupHeight);
    case AdPopover::Placement::Bottom:
      return QPoint(anchorRect.center().x() - popupWidth / 2, anchorRect.bottom() + 1);
    case AdPopover::Placement::BottomLeft:
      return QPoint(anchorRect.left(), anchorRect.bottom() + 1);
    case AdPopover::Placement::BottomRight:
      return QPoint(anchorRect.right() - popupWidth + 1, anchorRect.bottom() + 1);
    case AdPopover::Placement::Left:
      return QPoint(anchorRect.left() - popupWidth, anchorRect.center().y() - popupHeight / 2);
    case AdPopover::Placement::LeftTop:
      return QPoint(anchorRect.left() - popupWidth, anchorRect.top());
    case AdPopover::Placement::LeftBottom:
      return QPoint(anchorRect.left() - popupWidth, anchorRect.bottom() - popupHeight + 1);
    case AdPopover::Placement::Right:
      return QPoint(anchorRect.right() + 1, anchorRect.center().y() - popupHeight / 2);
    case AdPopover::Placement::RightTop:
      return QPoint(anchorRect.right() + 1, anchorRect.top());
    case AdPopover::Placement::RightBottom:
      return QPoint(anchorRect.right() + 1, anchorRect.bottom() - popupHeight + 1);
  }
  return anchorRect.topLeft();
}

int overflowCost(const QPoint& topLeft, const QSize& popupSize, const QRect& bounds) {
  if (!bounds.isValid()) {
    return 0;
  }

  const int popupWidth = std::max(1, popupSize.width());
  const int popupHeight = std::max(1, popupSize.height());
  const int leftOverflow = std::max(0, bounds.left() - topLeft.x());
  const int topOverflow = std::max(0, bounds.top() - topLeft.y());
  const int rightOverflow = std::max(0, topLeft.x() + popupWidth - (bounds.right() + 1));
  const int bottomOverflow = std::max(0, topLeft.y() + popupHeight - (bounds.bottom() + 1));
  return leftOverflow + topOverflow + rightOverflow + bottomOverflow;
}

QPoint applyPopupOffset(AdPopover::Placement placement, QPoint point, int offset) {
  if (offset <= 0) {
    return point;
  }

  switch (placement) {
    case AdPopover::Placement::Top:
    case AdPopover::Placement::TopLeft:
    case AdPopover::Placement::TopRight:
      point.ry() -= offset;
      break;
    case AdPopover::Placement::Bottom:
    case AdPopover::Placement::BottomLeft:
    case AdPopover::Placement::BottomRight:
      point.ry() += offset;
      break;
    case AdPopover::Placement::Left:
    case AdPopover::Placement::LeftTop:
    case AdPopover::Placement::LeftBottom:
      point.rx() -= offset;
      break;
    case AdPopover::Placement::Right:
    case AdPopover::Placement::RightTop:
    case AdPopover::Placement::RightBottom:
      point.rx() += offset;
      break;
  }
  return point;
}

QPoint applyCrossAxisShift(AdPopover::Placement placement, QPoint point, const QSize& popupSize, const QRect& bounds) {
  if (!bounds.isValid()) {
    return point;
  }

  const int popupWidth = std::max(1, popupSize.width());
  const int popupHeight = std::max(1, popupSize.height());
  if (placement == AdPopover::Placement::Top || placement == AdPopover::Placement::Bottom) {
    const int minX = bounds.left();
    const int maxX = std::max(minX, bounds.right() - popupWidth + 1);
    point.setX(std::clamp(point.x(), minX, maxX));
  } else if (placement == AdPopover::Placement::Left || placement == AdPopover::Placement::Right) {
    const int minY = bounds.top();
    const int maxY = std::max(minY, bounds.bottom() - popupHeight + 1);
    point.setY(std::clamp(point.y(), minY, maxY));
  }
  return point;
}

QPoint clampMainAxis(AdPopover::Placement placement,
                     QPoint point,
                     const QSize& popupSize,
                     const QRect& bounds) {
  if (!bounds.isValid()) {
    return point;
  }

  const int popupWidth = std::max(1, popupSize.width());
  const int popupHeight = std::max(1, popupSize.height());
  const int minX = bounds.left();
  const int maxX = std::max(minX, bounds.right() - popupWidth + 1);
  const int minY = bounds.top();
  const int maxY = std::max(minY, bounds.bottom() - popupHeight + 1);

  if (isVerticalPlacement(placement)) {
    point.setY(std::clamp(point.y(), minY, maxY));
  } else {
    point.setX(std::clamp(point.x(), minX, maxX));
  }
  return point;
}

qreal anchorCoordForArrow(AdPopover::Placement placement, const QRect& anchorRect, bool pointAtCenter) {
  const int maxInset = 24;
  switch (placement) {
    case AdPopover::Placement::Top:
    case AdPopover::Placement::Bottom:
      return anchorRect.center().x();
    case AdPopover::Placement::TopLeft:
    case AdPopover::Placement::BottomLeft:
      return pointAtCenter ? anchorRect.center().x()
                           : (anchorRect.left() + std::min(maxInset, std::max(0, anchorRect.width() / 2)));
    case AdPopover::Placement::TopRight:
    case AdPopover::Placement::BottomRight:
      return pointAtCenter ? anchorRect.center().x()
                           : (anchorRect.right() - std::min(maxInset, std::max(0, anchorRect.width() / 2)));
    case AdPopover::Placement::Left:
    case AdPopover::Placement::Right:
      return anchorRect.center().y();
    case AdPopover::Placement::LeftTop:
    case AdPopover::Placement::RightTop:
      return pointAtCenter ? anchorRect.center().y()
                           : (anchorRect.top() + std::min(maxInset, std::max(0, anchorRect.height() / 2)));
    case AdPopover::Placement::LeftBottom:
    case AdPopover::Placement::RightBottom:
      return pointAtCenter ? anchorRect.center().y()
                           : (anchorRect.bottom() - std::min(maxInset, std::max(0, anchorRect.height() / 2)));
  }
  return 0;
}

class PopoverPopupWidget final : public QWidget {
 public:
  explicit PopoverPopupWidget(QWidget* parent = nullptr) : QWidget(parent) {
    setAttribute(Qt::WA_Hover, true);
    setAutoFillBackground(false);

    bodyWidget_ = new QWidget(this);
    bodyLayout_ = new QVBoxLayout(bodyWidget_);
    bodyLayout_->setContentsMargins(0, 0, 0, 0);
    bodyLayout_->setSpacing(0);
  }

  QWidget* bodyWidget() const { return bodyWidget_; }

  void setVisualStyle(const detail::PopoverVisualStyle& style) {
    style_ = style;
    bodyPadding_ = std::max(0, style.metrics.popupPadding);
    updateBodyGeometry();
    updateGeometry();
    update();
  }

  void setArrowVisible(bool visible) {
    if (arrowVisible_ == visible) {
      return;
    }
    arrowVisible_ = visible;
    updateBodyGeometry();
    updateGeometry();
    update();
  }

  void setPlacement(AdPopover::Placement placement) {
    const ArrowSide next = arrowSideForPlacement(placement);
    if (arrowSide_ == next) {
      return;
    }
    arrowSide_ = next;
    updateBodyGeometry();
    updateGeometry();
    update();
  }

  void setArrowCenter(qreal center) {
    if (qFuzzyCompare(static_cast<double>(arrowCenter_), static_cast<double>(center))) {
      return;
    }
    arrowCenter_ = center;
    update();
  }

  QSize sizeHint() const override {
    const QSize bodyHint = bodyWidget_ ? bodyWidget_->sizeHint() : QSize(120, 32);
    const int arrowSize = arrowVisible_ ? std::max(0, style_.metrics.arrowSize) : 0;
    const int widthPadding =
        bodyPadding_ * 2 + ((arrowSide_ == ArrowSide::Left || arrowSide_ == ArrowSide::Right) ? arrowSize : 0);
    const int heightPadding =
        bodyPadding_ * 2 + ((arrowSide_ == ArrowSide::Top || arrowSide_ == ArrowSide::Bottom) ? arrowSize : 0);
    return QSize(std::max(1, bodyHint.width() + widthPadding),
                 std::max(1, bodyHint.height() + heightPadding));
  }

 protected:
  void resizeEvent(QResizeEvent* event) override {
    QWidget::resizeEvent(event);
    updateBodyGeometry();
  }

  void paintEvent(QPaintEvent* event) override {
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF bubbleRect = bubbleRectForPaint();
    if (!bubbleRect.isValid()) {
      return;
    }

    QPainterPath bubblePath;
    bubblePath.addRoundedRect(bubbleRect, style_.metrics.borderRadius, style_.metrics.borderRadius);
    const QPolygonF arrow = arrowPolygon(bubbleRect);
    if (!arrow.isEmpty()) {
      bubblePath.addPolygon(arrow);
    }

    painter.fillPath(bubblePath, style_.containerBackground);
    if (style_.metrics.borderWidth > 0 && style_.borderColor.alpha() > 0) {
      QPen pen(style_.borderColor, style_.metrics.borderWidth);
      painter.setPen(pen);
      painter.setBrush(Qt::NoBrush);
      painter.drawPath(bubblePath);
    }
  }

 private:
  QRectF bubbleRectForPaint() const {
    const int arrow = arrowVisible_ ? std::max(0, style_.metrics.arrowSize) : 0;
    QRectF rectf(rect());
    rectf.adjust(0.5, 0.5, -0.5, -0.5);
    switch (arrowSide_) {
      case ArrowSide::Top:
        rectf.adjust(0.0, arrow, 0.0, 0.0);
        break;
      case ArrowSide::Bottom:
        rectf.adjust(0.0, 0.0, 0.0, -arrow);
        break;
      case ArrowSide::Left:
        rectf.adjust(arrow, 0.0, 0.0, 0.0);
        break;
      case ArrowSide::Right:
        rectf.adjust(0.0, 0.0, -arrow, 0.0);
        break;
      case ArrowSide::None:
        break;
    }
    return rectf;
  }

  qreal clampedArrowCenter(const QRectF& bubbleRect) const {
    const int arrow = arrowVisible_ ? std::max(0, style_.metrics.arrowSize) : 0;
    if (arrow <= 0) {
      return 0.0;
    }
    const qreal half = static_cast<qreal>(arrow) / 2.0;
    const qreal radius = std::max(0.0, static_cast<qreal>(style_.metrics.borderRadius));
    if (arrowSide_ == ArrowSide::Top || arrowSide_ == ArrowSide::Bottom) {
      const qreal minX = bubbleRect.left() + radius + half + 1.0;
      const qreal maxX = bubbleRect.right() - radius - half - 1.0;
      const qreal fallback = bubbleRect.center().x();
      const qreal target = (arrowCenter_ > 0.0) ? arrowCenter_ : fallback;
      return std::clamp(target, minX, maxX);
    }
    if (arrowSide_ == ArrowSide::Left || arrowSide_ == ArrowSide::Right) {
      const qreal minY = bubbleRect.top() + radius + half + 1.0;
      const qreal maxY = bubbleRect.bottom() - radius - half - 1.0;
      const qreal fallback = bubbleRect.center().y();
      const qreal target = (arrowCenter_ > 0.0) ? arrowCenter_ : fallback;
      return std::clamp(target, minY, maxY);
    }
    return 0.0;
  }

  QPolygonF arrowPolygon(const QRectF& bubbleRect) const {
    const int arrow = arrowVisible_ ? std::max(0, style_.metrics.arrowSize) : 0;
    if (arrow <= 0) {
      return {};
    }

    const qreal center = clampedArrowCenter(bubbleRect);
    const qreal half = static_cast<qreal>(arrow) / 2.0;
    QPolygonF polygon;
    switch (arrowSide_) {
      case ArrowSide::Top:
        polygon << QPointF(center, bubbleRect.top() - arrow) << QPointF(center - half, bubbleRect.top())
                << QPointF(center + half, bubbleRect.top());
        break;
      case ArrowSide::Bottom:
        polygon << QPointF(center, bubbleRect.bottom() + arrow) << QPointF(center - half, bubbleRect.bottom())
                << QPointF(center + half, bubbleRect.bottom());
        break;
      case ArrowSide::Left:
        polygon << QPointF(bubbleRect.left() - arrow, center) << QPointF(bubbleRect.left(), center - half)
                << QPointF(bubbleRect.left(), center + half);
        break;
      case ArrowSide::Right:
        polygon << QPointF(bubbleRect.right() + arrow, center) << QPointF(bubbleRect.right(), center - half)
                << QPointF(bubbleRect.right(), center + half);
        break;
      case ArrowSide::None:
        break;
    }
    return polygon;
  }

  void updateBodyGeometry() {
    if (!bodyWidget_) {
      return;
    }
    const int arrow = arrowVisible_ ? std::max(0, style_.metrics.arrowSize) : 0;
    int left = bodyPadding_;
    int top = bodyPadding_;
    int right = bodyPadding_;
    int bottom = bodyPadding_;
    switch (arrowSide_) {
      case ArrowSide::Top:
        top += arrow;
        break;
      case ArrowSide::Bottom:
        bottom += arrow;
        break;
      case ArrowSide::Left:
        left += arrow;
        break;
      case ArrowSide::Right:
        right += arrow;
        break;
      case ArrowSide::None:
        break;
    }

    const QRect bodyRect = rect().adjusted(left, top, -right, -bottom);
    bodyWidget_->setGeometry(bodyRect);
  }

  QPointer<QWidget> bodyWidget_;
  QPointer<QVBoxLayout> bodyLayout_;
  detail::PopoverVisualStyle style_;
  ArrowSide arrowSide_ = ArrowSide::Bottom;
  bool arrowVisible_ = true;
  int bodyPadding_ = 12;
  qreal arrowCenter_ = 0.0;
};

struct PlacementComputation {
  QPoint topLeft;
  AdPopover::Placement placement = AdPopover::Placement::Top;
  qreal arrowCenterCoord = 0.0;
};

PlacementComputation resolvePlacement(AdPopover::Placement preferredPlacement,
                                      const QRect& anchorRect,
                                      const QSize& popupSize,
                                      const QRect& bounds,
                                      bool autoAdjustOverflow,
                                      int popupOffset,
                                      bool pointAtCenter) {
  auto computeForPlacement = [&](AdPopover::Placement placement) {
    QPoint pos = placementTopLeft(placement, anchorRect, popupSize);
    pos = applyPopupOffset(placement, pos, popupOffset);
    if (autoAdjustOverflow && supportsCrossAxisAutoShift(placement)) {
      pos = applyCrossAxisShift(placement, pos, popupSize, bounds);
    }
    return pos;
  };

  const QPoint preferredPos = computeForPlacement(preferredPlacement);
  QPoint chosenPos = preferredPos;
  AdPopover::Placement chosenPlacement = preferredPlacement;

  if (autoAdjustOverflow) {
    const AdPopover::Placement fallbackPlacement = oppositePlacement(preferredPlacement);
    const QPoint fallbackPos = computeForPlacement(fallbackPlacement);
    const int preferredCost = overflowCost(preferredPos, popupSize, bounds);
    const int fallbackCost = overflowCost(fallbackPos, popupSize, bounds);
    if (fallbackCost < preferredCost) {
      chosenPos = fallbackPos;
      chosenPlacement = fallbackPlacement;
    }
  }

  if (autoAdjustOverflow && bounds.isValid()) {
    if (supportsCrossAxisAutoShift(chosenPlacement)) {
      chosenPos = detail::clampPopupTopLeft(chosenPos, popupSize, bounds);
    } else {
      // Keep edge-aligned placements stable on cross-axis. Only clamp along main axis.
      chosenPos = clampMainAxis(chosenPlacement, chosenPos, popupSize, bounds);
    }
  }

  PlacementComputation out;
  out.topLeft = chosenPos;
  out.placement = chosenPlacement;
  const QRect popupRect(chosenPos, popupSize);
  out.arrowCenterCoord = anchorCoordForArrow(chosenPlacement, anchorRect, pointAtCenter) -
                         (isVerticalPlacement(chosenPlacement) ? popupRect.left() : popupRect.top());
  return out;
}

void clearLayoutChildren(QLayout* layout) {
  if (!layout) {
    return;
  }
  while (QLayoutItem* item = layout->takeAt(0)) {
    if (QWidget* widget = item->widget()) {
      widget->hide();
    }
    delete item;
  }
}

template <typename Callback>
void traverseObjectTree(QObject* root, Callback&& callback) {
  if (!root) {
    return;
  }

  callback(root);
  const QObjectList children = root->children();
  for (QObject* child : children) {
    traverseObjectTree(child, callback);
  }
}

}  // namespace

AdPopover::AdPopover(QWidget* parent) : QWidget(parent) {
  setAttribute(Qt::WA_Hover, true);
  setMouseTracking(true);

  rootLayout_ = new QVBoxLayout(this);
  rootLayout_->setContentsMargins(0, 0, 0, 0);
  rootLayout_->setSpacing(0);
}

AdPopover::~AdPopover() {
  clearHoverTasks();
  detail::setInWindowPopupHostOpen(this, false);
  clearTriggerWatchers();
  clearPopupWatchers();
  releasePopup();
}

AdPopover::Placement AdPopover::placement() const { return placement_; }

void AdPopover::setPlacement(Placement value) {
  if (placement_ == value) {
    return;
  }
  placement_ = value;
  emit placementChanged(placement_);
  if (open_) {
    syncPopupGeometry();
  }
}

AdPopover::Triggers AdPopover::triggerModes() const { return triggerModes_; }

void AdPopover::setTriggerModes(Triggers value) {
  if (triggerModes_ == value) {
    return;
  }
  triggerModes_ = value;
  emit triggerModesChanged(triggerModes_);

  if (!hasTrigger(Trigger::Hover)) {
    setReasonOpen(InternalOpenReason::Hover, false);
    hoverTriggerActive_ = false;
    hoverPopupActive_ = false;
    clearHoverTasks();
  }
  if (!hasTrigger(Trigger::Focus)) {
    setReasonOpen(InternalOpenReason::Focus, false);
    focusTriggerActive_ = false;
    focusPopupActive_ = false;
  }
  if (!hasTrigger(Trigger::Click)) {
    setReasonOpen(InternalOpenReason::Click, false);
    triggerPressActive_ = false;
    triggerKeyPressActive_ = false;
  }
  if (!hasTrigger(Trigger::ContextMenu)) {
    setReasonOpen(InternalOpenReason::ContextMenu, false);
    contextMenuGlobalPos_.reset();
  }

  updateOpenState(true);
}

bool AdPopover::open() const { return open_; }

void AdPopover::setOpen(bool value) {
  explicitOpenSet_ = true;
  if (openControlled_) {
    clearAllOpenReasons();
    setOpenInternal(value, true, false);
    return;
  }

  const bool previousSuppress = suppressOnOpenChangeEmission_;
  suppressOnOpenChangeEmission_ = true;
  if (value) {
    setReasonOpen(InternalOpenReason::Programmatic, true);
  } else {
    clearAllOpenReasons();
  }
  updateOpenState(true);
  suppressOnOpenChangeEmission_ = previousSuppress;
}

bool AdPopover::openControlled() const { return openControlled_; }

void AdPopover::setOpenControlled(bool value) {
  if (openControlled_ == value) {
    return;
  }

  openControlled_ = value;
  emit openControlledChanged(openControlled_);

  if (openControlled_) {
    clearAllOpenReasons();
    return;
  }

  clearAllOpenReasons();
  if (open_) {
    setReasonOpen(InternalOpenReason::Programmatic, true);
  }
}

bool AdPopover::defaultOpen() const { return defaultOpen_; }

void AdPopover::setDefaultOpen(bool value) {
  if (defaultOpen_ == value) {
    return;
  }
  defaultOpen_ = value;
  emit defaultOpenChanged(defaultOpen_);
}

bool AdPopover::autoAdjustOverflow() const { return autoAdjustOverflow_; }

void AdPopover::setAutoAdjustOverflow(bool value) {
  if (autoAdjustOverflow_ == value) {
    return;
  }
  autoAdjustOverflow_ = value;
  emit autoAdjustOverflowChanged(autoAdjustOverflow_);
  if (open_) {
    syncPopupGeometry();
  }
}

bool AdPopover::arrowVisible() const { return arrowVisible_; }

void AdPopover::setArrowVisible(bool value) {
  if (arrowVisible_ == value) {
    return;
  }
  arrowVisible_ = value;
  emit arrowVisibleChanged(arrowVisible_);
  updateStyle();
  if (open_) {
    syncPopupGeometry();
  }
}

bool AdPopover::arrowPointAtCenter() const { return arrowPointAtCenter_; }

void AdPopover::setArrowPointAtCenter(bool value) {
  if (arrowPointAtCenter_ == value) {
    return;
  }
  arrowPointAtCenter_ = value;
  emit arrowPointAtCenterChanged(arrowPointAtCenter_);
  if (open_) {
    syncPopupGeometry();
  }
}

bool AdPopover::destroyOnHidden() const { return destroyOnHidden_; }

void AdPopover::setDestroyOnHidden(bool value) {
  if (destroyOnHidden_ == value) {
    return;
  }
  destroyOnHidden_ = value;
  emit destroyOnHiddenChanged(destroyOnHidden_);
  if (!open_ && destroyOnHidden_) {
    releasePopup();
  }
}

bool AdPopover::disabled() const { return disabled_; }

void AdPopover::setDisabled(bool value) {
  if (disabled_ == value) {
    return;
  }
  disabled_ = value;
  emit disabledChanged(disabled_);
  if (disabled_) {
    clearAllOpenReasons();
  }
  updateOpenState(true);
}

int AdPopover::mouseEnterDelayMs() const { return mouseEnterDelayMs_; }

void AdPopover::setMouseEnterDelayMs(int value) {
  const int normalized = std::max(0, value);
  if (mouseEnterDelayMs_ == normalized) {
    return;
  }
  mouseEnterDelayMs_ = normalized;
  emit mouseEnterDelayMsChanged(mouseEnterDelayMs_);
}

int AdPopover::mouseLeaveDelayMs() const { return mouseLeaveDelayMs_; }

void AdPopover::setMouseLeaveDelayMs(int value) {
  const int normalized = std::max(0, value);
  if (mouseLeaveDelayMs_ == normalized) {
    return;
  }
  mouseLeaveDelayMs_ = normalized;
  emit mouseLeaveDelayMsChanged(mouseLeaveDelayMs_);
}

QString AdPopover::titleText() const { return titleText_; }

void AdPopover::setTitleText(const QString& value) {
  if (titleText_ == value) {
    return;
  }
  titleText_ = value;
  emit titleTextChanged(titleText_);
  refreshPopupContent();
  updateOpenState(true);
}

QString AdPopover::contentText() const { return contentText_; }

void AdPopover::setContentText(const QString& value) {
  if (contentText_ == value) {
    return;
  }
  contentText_ = value;
  emit contentTextChanged(contentText_);
  refreshPopupContent();
  updateOpenState(true);
}

QWidget* AdPopover::triggerWidget() const { return triggerWidget_; }

void AdPopover::setTriggerWidget(QWidget* widget) {
  if (triggerWidget_ == widget) {
    return;
  }

  clearTriggerWatchers();
  if (triggerWidget_ && rootLayout_) {
    rootLayout_->removeWidget(triggerWidget_);
    triggerWidget_->hide();
    triggerWidget_->setParent(nullptr);
  }

  triggerWidget_ = widget;
  if (triggerWidget_) {
    triggerWidget_->setParent(this);
    if (rootLayout_) {
      rootLayout_->addWidget(triggerWidget_);
    }
  }
  refreshTriggerWatchers();
  emit triggerWidgetChanged(triggerWidget_);

  if (open_) {
    syncPopupGeometry();
  }
}

QWidget* AdPopover::titleWidget() const { return titleWidget_; }

void AdPopover::setTitleWidget(QWidget* widget) {
  if (titleWidget_ == widget) {
    return;
  }
  if (titleWidget_) {
    titleWidget_->hide();
    titleWidget_->setParent(nullptr);
  }

  titleWidget_ = widget;
  emit titleWidgetChanged(titleWidget_);
  refreshPopupContent();
  updateOpenState(true);
}

QWidget* AdPopover::contentWidget() const { return contentWidget_; }

void AdPopover::setContentWidget(QWidget* widget) {
  if (contentWidget_ == widget) {
    return;
  }
  if (contentWidget_) {
    contentWidget_->hide();
    contentWidget_->setParent(nullptr);
  }

  contentWidget_ = widget;
  emit contentWidgetChanged(contentWidget_);
  refreshPopupContent();
  updateOpenState(true);
}

AdPopover::ComponentTokens AdPopover::componentTokens() const { return componentTokens_; }

void AdPopover::setComponentTokens(const ComponentTokens& tokens) {
  componentTokens_ = tokens;
  emit componentTokensChanged();
  updateStyle();
  if (open_) {
    syncPopupGeometry();
  }
}

void AdPopover::resetComponentTokens() {
  componentTokens_ = ComponentTokens{};
  emit componentTokensChanged();
  updateStyle();
  if (open_) {
    syncPopupGeometry();
  }
}

AdPopover::SemanticStyles AdPopover::semanticStyles() const { return semanticStyles_; }

void AdPopover::setSemanticStyles(const SemanticStyles& styles) {
  semanticStyles_ = styles;
  emit semanticStylesChanged();
  updateStyle();
  if (open_) {
    syncPopupGeometry();
  }
}

void AdPopover::setSemanticStyleResolver(SemanticStyleResolver resolver) {
  semanticStyleResolver_ = std::move(resolver);
  emit semanticStylesChanged();
  updateStyle();
  if (open_) {
    syncPopupGeometry();
  }
}

bool AdPopover::eventFilter(QObject* watched, QEvent* event) {
  if (!watched || !event) {
    return QWidget::eventFilter(watched, event);
  }

  if (watchedByTrigger(watched)) {
    switch (event->type()) {
      case QEvent::Enter:
      case QEvent::HoverEnter:
        handleTriggerHoverEnter();
        break;
      case QEvent::Leave:
      case QEvent::HoverLeave:
        handleTriggerHoverLeave();
        break;
      case QEvent::FocusIn:
        focusTriggerActive_ = true;
        if (hasTrigger(Trigger::Focus)) {
          setReasonOpen(InternalOpenReason::Focus, true);
          updateOpenState(true);
        }
        break;
      case QEvent::FocusOut:
        handleTriggerFocusOutDeferred();
        break;
      case QEvent::MouseButtonPress:
        handleTriggerPress(event);
        break;
      case QEvent::MouseButtonRelease:
        handleTriggerRelease(event);
        break;
      case QEvent::KeyPress: {
        handleTriggerKeyPress(event);
        break;
      }
      case QEvent::KeyRelease: {
        handleTriggerKeyRelease(event);
        break;
      }
      case QEvent::ContextMenu:
        handleTriggerContextMenu(event);
        break;
      case QEvent::Move:
      case QEvent::Resize:
      case QEvent::LayoutRequest:
      case QEvent::Show:
        if (open_) {
          syncPopupGeometry();
        }
        break;
      case QEvent::Hide:
        triggerPressActive_ = false;
        triggerKeyPressActive_ = false;
        if (open_) {
          clearAllOpenReasons();
          updateOpenState(true);
        }
        break;
      case QEvent::ChildAdded: {
        auto* childEvent = static_cast<QChildEvent*>(event);
        if (childEvent->child()) {
          traverseObjectTree(childEvent->child(), [this](QObject* object) {
            if (!object || watchedTriggerObjects_.contains(object)) {
              return;
            }
            object->installEventFilter(this);
            watchedTriggerObjects_.insert(object);
          });
        }
        break;
      }
      default:
        break;
    }
  } else if (watchedByPopup(watched)) {
    switch (event->type()) {
      case QEvent::Enter:
      case QEvent::HoverEnter:
        handlePopupHoverEnter();
        break;
      case QEvent::Leave:
      case QEvent::HoverLeave:
        handlePopupHoverLeave();
        break;
      case QEvent::FocusIn:
        focusPopupActive_ = true;
        if (hasTrigger(Trigger::Focus)) {
          setReasonOpen(InternalOpenReason::Focus, true);
          updateOpenState(true);
        }
        break;
      case QEvent::FocusOut:
        handleTriggerFocusOutDeferred();
        break;
      case QEvent::KeyPress: {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Escape && open_) {
          clearAllOpenReasons();
          updateOpenState(true);
          keyEvent->accept();
        }
        break;
      }
      case QEvent::ChildAdded: {
        auto* childEvent = static_cast<QChildEvent*>(event);
        if (childEvent->child()) {
          traverseObjectTree(childEvent->child(), [this](QObject* object) {
            if (!object || watchedPopupObjects_.contains(object)) {
              return;
            }
            object->installEventFilter(this);
            watchedPopupObjects_.insert(object);
          });
        }
        break;
      }
      default:
        break;
    }
  }

  return QWidget::eventFilter(watched, event);
}

void AdPopover::showEvent(QShowEvent* event) {
  QWidget::showEvent(event);
  if (!defaultOpenApplied_) {
    defaultOpenApplied_ = true;
    if (defaultOpen_ && !explicitOpenSet_ && !openControlled_) {
      const bool previousSuppress = suppressOnOpenChangeEmission_;
      suppressOnOpenChangeEmission_ = true;
      setReasonOpen(InternalOpenReason::Programmatic, true);
      updateOpenState(true);
      suppressOnOpenChangeEmission_ = previousSuppress;
    }
  }
  if (open_) {
    syncPopupGeometry();
  }
}

void AdPopover::hideEvent(QHideEvent* event) {
  QWidget::hideEvent(event);
  clearHoverTasks();
  detail::setInWindowPopupHostOpen(this, false);
  if (popup_) {
    popup_->hide();
  }
}

void AdPopover::moveEvent(QMoveEvent* event) {
  QWidget::moveEvent(event);
  if (open_) {
    syncPopupGeometry();
  }
}

void AdPopover::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  if (open_) {
    syncPopupGeometry();
  }
}

void AdPopover::changeEvent(QEvent* event) {
  QWidget::changeEvent(event);
  if (!event) {
    return;
  }
  if (event->type() == QEvent::EnabledChange) {
    setDisabled(!isEnabled());
  } else if (event->type() == QEvent::ParentChange || event->type() == QEvent::ParentAboutToChange) {
    if (open_) {
      syncPopupGeometry();
    }
  }
}

void AdPopover::ensurePopup() {
  if (popup_) {
    return;
  }

  QWidget* scopeWindow = detail::resolvePopupScopeWindow(this);
  auto* popup = new PopoverPopupWidget(scopeWindow);
  popup->setObjectName(QStringLiteral("adpopover-popup"));
  popup->setAttribute(Qt::WA_DeleteOnClose, false);
  popup->setAttribute(Qt::WA_Hover, true);
  popup->installEventFilter(this);

  popup_ = popup;
  popupBody_ = popup->bodyWidget();
  popupBodyLayout_ = qobject_cast<QVBoxLayout*>(popupBody_->layout());

  titleContainer_ = new QWidget(popupBody_);
  titleContainerLayout_ = new QVBoxLayout(titleContainer_);
  titleContainerLayout_->setContentsMargins(0, 0, 0, 0);
  titleContainerLayout_->setSpacing(0);
  titleLabel_ = new QLabel(titleContainer_);
  titleLabel_->setWordWrap(true);
  titleContainerLayout_->addWidget(titleLabel_);

  contentContainer_ = new QWidget(popupBody_);
  contentContainerLayout_ = new QVBoxLayout(contentContainer_);
  contentContainerLayout_->setContentsMargins(0, 0, 0, 0);
  contentContainerLayout_->setSpacing(0);
  contentLabel_ = new QLabel(contentContainer_);
  contentLabel_->setWordWrap(true);
  contentContainerLayout_->addWidget(contentLabel_);

  if (popupBodyLayout_) {
    popupBodyLayout_->setContentsMargins(0, 0, 0, 0);
    popupBodyLayout_->setSpacing(0);
    popupBodyLayout_->addWidget(titleContainer_);
    popupBodyLayout_->addWidget(contentContainer_);
  }

  refreshPopupWatchers();
  refreshPopupContent();
  updateStyle();
}

void AdPopover::releasePopup() {
  clearPopupWatchers();
  if (popup_) {
    popup_->removeEventFilter(this);
    popup_->hide();
    popup_->deleteLater();
  }

  popup_.clear();
  popupBody_.clear();
  popupBodyLayout_.clear();
  titleContainer_.clear();
  contentContainer_.clear();
  titleContainerLayout_.clear();
  contentContainerLayout_.clear();
  titleLabel_.clear();
  contentLabel_.clear();
}

void AdPopover::refreshPopupContent() {
  if (!popup_) {
    return;
  }

  const bool hasTitleWidget = titleWidget_;
  const bool hasTitleText = !titleText_.trimmed().isEmpty();
  const bool hasContentWidget = contentWidget_;
  const bool hasContentText = !contentText_.trimmed().isEmpty();

  if (titleContainerLayout_) {
    clearLayoutChildren(titleContainerLayout_);
    if (hasTitleWidget) {
      if (titleWidget_->parentWidget() != titleContainer_) {
        titleWidget_->setParent(titleContainer_);
      }
      titleContainerLayout_->addWidget(titleWidget_);
      titleWidget_->show();
    } else if (titleLabel_) {
      titleLabel_->setText(titleText_);
      titleContainerLayout_->addWidget(titleLabel_);
      titleLabel_->setVisible(hasTitleText);
    }
  }

  if (contentContainerLayout_) {
    clearLayoutChildren(contentContainerLayout_);
    if (hasContentWidget) {
      if (contentWidget_->parentWidget() != contentContainer_) {
        contentWidget_->setParent(contentContainer_);
      }
      contentContainerLayout_->addWidget(contentWidget_);
      contentWidget_->show();
    } else if (contentLabel_) {
      contentLabel_->setText(contentText_);
      contentContainerLayout_->addWidget(contentLabel_);
      contentLabel_->setVisible(hasContentText);
    }
  }

  if (titleContainer_) {
    titleContainer_->setVisible(hasTitleWidget || hasTitleText);
  }
  if (contentContainer_) {
    contentContainer_->setVisible(hasContentWidget || hasContentText);
  }

  updateStyle();
  if (open_) {
    syncPopupGeometry();
  }
}

void AdPopover::syncPopupGeometry() {
  if (!open_ || !popup_) {
    return;
  }

  QWidget* popupParent = popup_->parentWidget();
  if (!popupParent) {
    popupParent = detail::resolvePopupScopeWindow(this);
    if (popupParent) {
      popup_->setParent(popupParent);
    }
  }
  if (!popupParent) {
    return;
  }

  QRect anchorRect = widgetGlobalRect(popupAnchorWidget());
  if (contextMenuGlobalPos_.has_value() && reasonOpen(InternalOpenReason::ContextMenu)) {
    anchorRect = QRect(contextMenuGlobalPos_.value(), QSize(1, 1));
  }

  if (!anchorRect.isValid()) {
    return;
  }

  popup_->adjustSize();
  QSize popupSize = popup_->sizeHint();
  popupSize.setWidth(std::max(1, popupSize.width()));
  popupSize.setHeight(std::max(1, popupSize.height()));
  popup_->resize(popupSize);

  StyleContext ctx;
  ctx.placement = placement_;
  ctx.triggerModes = triggerModes_;
  ctx.open = open_;
  ctx.disabled = disabled_;
  ctx.arrowVisible = arrowVisible_;
  const SemanticStyles effectiveSemantic =
      semanticStyleResolver_ ? semanticStyleResolver_(ctx) : semanticStyles_;

  detail::PopoverStyleInput styleInput;
  styleInput.placement = placement_;
  styleInput.open = open_;
  styleInput.disabled = disabled_;
  styleInput.arrowVisible = arrowVisible_;
  styleInput.baseFont = font();
  styleInput.componentTokens = componentTokens_;
  styleInput.semanticStyles = effectiveSemantic;
  const detail::PopoverVisualStyle style = detail::resolvePopoverVisualStyle(styleInput);

  const QRect bounds = detail::popupBoundsInGlobal(popupParent);
  const PlacementComputation placementResult =
      resolvePlacement(placement_, anchorRect, popupSize, bounds, autoAdjustOverflow_,
                       style.metrics.popupOffset, arrowPointAtCenter_);

  QPoint popupTopLeft = placementResult.topLeft;
  popupTopLeft = popupParent->mapFromGlobal(popupTopLeft);
  popup_->move(popupTopLeft);

  auto* popupWidget = static_cast<PopoverPopupWidget*>(popup_.data());
  popupWidget->setPlacement(placementResult.placement);
  popupWidget->setArrowVisible(arrowVisible_);
  popupWidget->setArrowCenter(placementResult.arrowCenterCoord);
  popupWidget->update();
}

bool AdPopover::hasOverlayContent() const {
  return titleWidget_ || contentWidget_ || !titleText_.trimmed().isEmpty() ||
         !contentText_.trimmed().isEmpty();
}

bool AdPopover::hasTrigger(Trigger trigger) const { return triggerModes_.testFlag(trigger); }

bool AdPopover::shouldBeOpen() const {
  if (disabled_ || !hasOverlayContent()) {
    return false;
  }
  return openByHover_ || openByFocus_ || openByClick_ || openByContextMenu_ || openByProgrammatic_;
}

bool AdPopover::isHoveringTriggerTree() const {
  if (!triggerWidget_) {
    return false;
  }
  const QPoint globalPos = QCursor::pos();
  QWidget* hovered = QApplication::widgetAt(globalPos);
  return widgetInTree(hovered, triggerWidget_);
}

bool AdPopover::isHoveringPopupTree() const {
  if (!popup_) {
    return false;
  }
  const QPoint globalPos = QCursor::pos();
  QWidget* hovered = QApplication::widgetAt(globalPos);
  return widgetInTree(hovered, popup_);
}

void AdPopover::scheduleHoverOpen() {
  detail::cancelTimingTask(this, QString::fromLatin1(kHoverCloseTaskKey));
  const int delay = std::max(0, mouseEnterDelayMs_);
  if (delay == 0) {
    if ((hoverTriggerActive_ || hoverPopupActive_) && hasTrigger(Trigger::Hover)) {
      setReasonOpen(InternalOpenReason::Hover, true);
      updateOpenState(true);
    }
    return;
  }

  detail::scheduleTimingTask(this, QString::fromLatin1(kHoverOpenTaskKey), delay, [this]() {
    if ((isHoveringTriggerTree() || isHoveringPopupTree()) && hasTrigger(Trigger::Hover)) {
      setReasonOpen(InternalOpenReason::Hover, true);
      updateOpenState(true);
    }
  });
}

void AdPopover::scheduleHoverClose() {
  detail::cancelTimingTask(this, QString::fromLatin1(kHoverOpenTaskKey));
  const int delay = std::max(0, mouseLeaveDelayMs_);
  if (delay == 0) {
    if (!isHoveringTriggerTree() && !isHoveringPopupTree()) {
      setReasonOpen(InternalOpenReason::Hover, false);
      updateOpenState(true);
    }
    return;
  }

  detail::scheduleTimingTask(this, QString::fromLatin1(kHoverCloseTaskKey), delay, [this]() {
    if (!isHoveringTriggerTree() && !isHoveringPopupTree()) {
      setReasonOpen(InternalOpenReason::Hover, false);
      updateOpenState(true);
    }
  });
}

void AdPopover::clearHoverTasks() {
  detail::cancelTimingTask(this, QString::fromLatin1(kHoverOpenTaskKey));
  detail::cancelTimingTask(this, QString::fromLatin1(kHoverCloseTaskKey));
}

void AdPopover::setReasonOpen(InternalOpenReason reason, bool enabled) {
  switch (reason) {
    case InternalOpenReason::Hover:
      openByHover_ = enabled;
      break;
    case InternalOpenReason::Focus:
      openByFocus_ = enabled;
      break;
    case InternalOpenReason::Click:
      openByClick_ = enabled;
      break;
    case InternalOpenReason::ContextMenu:
      openByContextMenu_ = enabled;
      if (!enabled) {
        contextMenuGlobalPos_.reset();
      }
      break;
    case InternalOpenReason::Programmatic:
      openByProgrammatic_ = enabled;
      break;
  }
}

bool AdPopover::reasonOpen(InternalOpenReason reason) const {
  switch (reason) {
    case InternalOpenReason::Hover:
      return openByHover_;
    case InternalOpenReason::Focus:
      return openByFocus_;
    case InternalOpenReason::Click:
      return openByClick_;
    case InternalOpenReason::ContextMenu:
      return openByContextMenu_;
    case InternalOpenReason::Programmatic:
      return openByProgrammatic_;
  }
  return false;
}

void AdPopover::clearAllOpenReasons() {
  openByHover_ = false;
  openByFocus_ = false;
  openByClick_ = false;
  openByContextMenu_ = false;
  openByProgrammatic_ = false;
  triggerPressActive_ = false;
  triggerKeyPressActive_ = false;
  contextMenuGlobalPos_.reset();
}

void AdPopover::emitControlledOpenRequest(bool requestedOpen) {
  if (!openControlled_) {
    return;
  }

  if (requestedOpen && (disabled_ || !hasOverlayContent())) {
    return;
  }

  if (requestedOpen == open_) {
    return;
  }

  emit onOpenChange(requestedOpen);
}

void AdPopover::updateOpenState(bool emitSignal) {
  if (updatingOpen_) {
    return;
  }
  const bool shouldOpen = shouldBeOpen();
  if (openControlled_) {
    if (!emitSignal) {
      return;
    }
    emitControlledOpenRequest(shouldOpen);
    return;
  }
  setOpenInternal(shouldOpen, emitSignal);
}

void AdPopover::setOpenInternal(bool open, bool emitSignal, bool emitOnOpenChangeSignal) {
  if (updatingOpen_) {
    return;
  }
  updatingOpen_ = true;

  if (open_ == open) {
    if (open_) {
      ensurePopup();
      updateStyle();
      syncPopupGeometry();
      if (popup_) {
        popup_->show();
        popup_->raise();
      }
    }
    updatingOpen_ = false;
    return;
  }

  open_ = open;
  detail::setInWindowPopupHostOpen(this, open_);
  if (open_) {
    ensurePopup();
    refreshPopupContent();
    updateStyle();
    syncPopupGeometry();
    if (popup_) {
      popup_->show();
      popup_->raise();
    }
  } else {
    clearHoverTasks();
    if (popup_) {
      popup_->hide();
    }
    if (destroyOnHidden_) {
      releasePopup();
    }
  }

  if (emitSignal) {
    emit openChanged(open_);
    if (emitOnOpenChangeSignal && !suppressOnOpenChangeEmission_) {
      emit onOpenChange(open_);
    }
  }
  updatingOpen_ = false;
}

void AdPopover::refreshTriggerWatchers() {
  clearTriggerWatchers();
  if (!triggerWidget_) {
    return;
  }

  watchedTriggerRoot_ = triggerWidget_;
  traverseObjectTree(triggerWidget_, [this](QObject* object) {
    if (!object) {
      return;
    }
    object->installEventFilter(this);
    watchedTriggerObjects_.insert(object);
  });
}

void AdPopover::clearTriggerWatchers() {
  for (auto it = watchedTriggerObjects_.begin(); it != watchedTriggerObjects_.end(); ++it) {
    if (it->data()) {
      it->data()->removeEventFilter(this);
    }
  }
  watchedTriggerObjects_.clear();
  watchedTriggerRoot_.clear();
}

void AdPopover::refreshPopupWatchers() {
  clearPopupWatchers();
  if (!popup_) {
    return;
  }
  traverseObjectTree(popup_, [this](QObject* object) {
    if (!object) {
      return;
    }
    object->installEventFilter(this);
    watchedPopupObjects_.insert(object);
  });
}

void AdPopover::clearPopupWatchers() {
  for (auto it = watchedPopupObjects_.begin(); it != watchedPopupObjects_.end(); ++it) {
    if (it->data()) {
      it->data()->removeEventFilter(this);
    }
  }
  watchedPopupObjects_.clear();
}

bool AdPopover::watchedByTrigger(QObject* watched) const {
  if (!watched) {
    return false;
  }
  for (auto it = watchedTriggerObjects_.constBegin(); it != watchedTriggerObjects_.constEnd(); ++it) {
    if (it->data() == watched) {
      return true;
    }
  }
  return false;
}

bool AdPopover::watchedByPopup(QObject* watched) const {
  if (!watched) {
    return false;
  }
  for (auto it = watchedPopupObjects_.constBegin(); it != watchedPopupObjects_.constEnd(); ++it) {
    if (it->data() == watched) {
      return true;
    }
  }
  return false;
}

void AdPopover::handleTriggerPress(QEvent* event) {
  if (!event || disabled_ || !hasTrigger(Trigger::Click)) {
    return;
  }

  if (event->type() != QEvent::MouseButtonPress) {
    return;
  }

  auto* mouseEvent = static_cast<QMouseEvent*>(event);
  if (mouseEvent->button() != Qt::LeftButton) {
    return;
  }
  if (triggerPressActive_) {
    return;
  }
  triggerPressActive_ = true;
}

void AdPopover::handleTriggerRelease(QEvent* event) {
  if (!event) {
    return;
  }
  if (event->type() != QEvent::MouseButtonRelease) {
    return;
  }
  auto* mouseEvent = static_cast<QMouseEvent*>(event);
  if (mouseEvent->button() != Qt::LeftButton) {
    return;
  }
  if (!triggerPressActive_) {
    return;
  }

  triggerPressActive_ = false;
  QWidget* hovered = QApplication::widgetAt(mouseEvent->globalPosition().toPoint());
  if (!widgetInTree(hovered, triggerWidget_)) {
    return;
  }

  if (openControlled_) {
    emitControlledOpenRequest(!open_);
    return;
  }

  setReasonOpen(InternalOpenReason::Click, !reasonOpen(InternalOpenReason::Click));
  if (!reasonOpen(InternalOpenReason::Click)) {
    contextMenuGlobalPos_.reset();
  }
  updateOpenState(true);
}

void AdPopover::handleTriggerKeyPress(QEvent* event) {
  if (!event) {
    return;
  }
  if (event->type() != QEvent::KeyPress) {
    return;
  }

  auto* keyEvent = static_cast<QKeyEvent*>(event);
  if (keyEvent->key() == Qt::Key_Escape && open_) {
    clearAllOpenReasons();
    updateOpenState(true);
    keyEvent->accept();
    return;
  }

  if (disabled_ || !hasTrigger(Trigger::Click) || keyEvent->isAutoRepeat()) {
    return;
  }
  if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter ||
      keyEvent->key() == Qt::Key_Space) {
    triggerKeyPressActive_ = true;
  }
}

void AdPopover::handleTriggerKeyRelease(QEvent* event) {
  if (!event) {
    return;
  }
  if (event->type() != QEvent::KeyRelease) {
    return;
  }

  auto* keyEvent = static_cast<QKeyEvent*>(event);
  if (keyEvent->isAutoRepeat()) {
    return;
  }

  const bool activationKey =
      keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter ||
      keyEvent->key() == Qt::Key_Space;
  if (!activationKey) {
    return;
  }

  if (!triggerKeyPressActive_) {
    return;
  }
  triggerKeyPressActive_ = false;

  if (disabled_ || !hasTrigger(Trigger::Click)) {
    return;
  }
  if (!widgetInTree(QApplication::focusWidget(), triggerWidget_)) {
    return;
  }

  if (openControlled_) {
    emitControlledOpenRequest(!open_);
    keyEvent->accept();
    return;
  }

  setReasonOpen(InternalOpenReason::Click, !reasonOpen(InternalOpenReason::Click));
  if (!reasonOpen(InternalOpenReason::Click)) {
    contextMenuGlobalPos_.reset();
  }
  updateOpenState(true);
  keyEvent->accept();
}

void AdPopover::handleTriggerContextMenu(QEvent* event) {
  if (!event || disabled_ || !hasTrigger(Trigger::ContextMenu)) {
    return;
  }

  if (event->type() != QEvent::ContextMenu) {
    return;
  }

  auto* contextEvent = static_cast<QContextMenuEvent*>(event);
  contextMenuGlobalPos_ = contextEvent->globalPos();
  setReasonOpen(InternalOpenReason::ContextMenu, true);
  updateOpenState(true);
  contextEvent->accept();
}

void AdPopover::handleTriggerFocusOutDeferred() {
  detail::deferTimingTask(this, QString::fromLatin1(kFocusRecheckTaskKey), [this]() {
    QWidget* focused = QApplication::focusWidget();
    focusTriggerActive_ = widgetInTree(focused, triggerWidget_);
    focusPopupActive_ = widgetInTree(focused, popup_);
    if (hasTrigger(Trigger::Focus)) {
      setReasonOpen(InternalOpenReason::Focus, focusTriggerActive_ || focusPopupActive_);
      updateOpenState(true);
    }
  });
}

void AdPopover::handleTriggerHoverEnter() {
  const bool wasActive = hoverTriggerActive_;
  hoverTriggerActive_ = true;
  if (wasActive == hoverTriggerActive_) {
    return;
  }
  if (hasTrigger(Trigger::Hover)) {
    scheduleHoverOpen();
  }
}

void AdPopover::handleTriggerHoverLeave() {
  const bool wasActive = hoverTriggerActive_;
  hoverTriggerActive_ = isHoveringTriggerTree();
  if (wasActive == hoverTriggerActive_) {
    return;
  }
  if (hasTrigger(Trigger::Hover)) {
    scheduleHoverClose();
  }
}

void AdPopover::handlePopupHoverEnter() {
  const bool wasActive = hoverPopupActive_;
  hoverPopupActive_ = true;
  if (wasActive == hoverPopupActive_) {
    return;
  }
  if (hasTrigger(Trigger::Hover)) {
    scheduleHoverOpen();
  }
}

void AdPopover::handlePopupHoverLeave() {
  const bool wasActive = hoverPopupActive_;
  hoverPopupActive_ = isHoveringPopupTree();
  if (wasActive == hoverPopupActive_) {
    return;
  }
  if (hasTrigger(Trigger::Hover)) {
    scheduleHoverClose();
  }
}

void AdPopover::updateStyle() {
  if (!popup_) {
    return;
  }

  StyleContext ctx;
  ctx.placement = placement_;
  ctx.triggerModes = triggerModes_;
  ctx.open = open_;
  ctx.disabled = disabled_;
  ctx.arrowVisible = arrowVisible_;
  const SemanticStyles effectiveSemantic =
      semanticStyleResolver_ ? semanticStyleResolver_(ctx) : semanticStyles_;

  detail::PopoverStyleInput styleInput;
  styleInput.placement = placement_;
  styleInput.open = open_;
  styleInput.disabled = disabled_;
  styleInput.arrowVisible = arrowVisible_;
  styleInput.baseFont = font();
  styleInput.componentTokens = componentTokens_;
  styleInput.semanticStyles = effectiveSemantic;
  const detail::PopoverVisualStyle style = detail::resolvePopoverVisualStyle(styleInput);

  auto* popupWidget = static_cast<PopoverPopupWidget*>(popup_.data());
  popupWidget->setVisualStyle(style);
  popupWidget->setArrowVisible(arrowVisible_);
  popupWidget->setPlacement(placement_);

  if (popupBodyLayout_) {
    const int padding = std::max(0, style.metrics.popupPadding);
    popupBodyLayout_->setContentsMargins(padding, padding, padding, padding);
    const bool hasTitle = titleContainer_ && titleContainer_->isVisible();
    const bool hasContent = contentContainer_ && contentContainer_->isVisible();
    popupBodyLayout_->setSpacing((hasTitle && hasContent) ? std::max(0, style.metrics.titleMarginBottom) : 0);
  }

  if (titleContainerLayout_) {
    titleContainerLayout_->setContentsMargins(std::max(0, style.metrics.titlePaddingHorizontal),
                                              std::max(0, style.metrics.titlePaddingVertical),
                                              std::max(0, style.metrics.titlePaddingHorizontal),
                                              std::max(0, style.metrics.titlePaddingVertical));
  }
  if (contentContainerLayout_) {
    contentContainerLayout_->setContentsMargins(std::max(0, style.metrics.contentPaddingHorizontal),
                                                std::max(0, style.metrics.contentPaddingVertical),
                                                std::max(0, style.metrics.contentPaddingHorizontal),
                                                std::max(0, style.metrics.contentPaddingVertical));
  }

  if (titleContainer_) {
    titleContainer_->setMinimumWidth(std::max(0, style.metrics.titleMinWidth));
  }

  if (titleLabel_) {
    titleLabel_->setFont(style.metrics.titleFont);
    QPalette palette = titleLabel_->palette();
    palette.setColor(QPalette::WindowText, style.titleColor);
    titleLabel_->setPalette(palette);
  }
  if (contentLabel_) {
    contentLabel_->setFont(style.metrics.contentFont);
    QPalette palette = contentLabel_->palette();
    palette.setColor(QPalette::WindowText, style.contentColor);
    contentLabel_->setPalette(palette);
  }
}

QObject* AdPopover::popupOwnerObject() const { return const_cast<AdPopover*>(this); }

QWidget* AdPopover::popupAnchorWidget() const {
  if (triggerWidget_) {
    return triggerWidget_;
  }
  return const_cast<AdPopover*>(this);
}

QWidget* AdPopover::popupScopeWindow() const { return detail::resolvePopupScopeWindow(this); }

bool AdPopover::popupIsVisible() const { return open_ && popup_ && popup_->isVisible(); }

bool AdPopover::popupContainsGlobalPos(const QPoint& globalPos) const {
  if (widgetContainsGlobalPos(triggerWidget_, globalPos)) {
    return true;
  }
  return widgetContainsGlobalPos(popup_, globalPos);
}

void AdPopover::popupCloseFromHost(detail::PopupCloseReason reason) {
  Q_UNUSED(reason)
  if (closingFromHost_) {
    return;
  }
  closingFromHost_ = true;
  clearAllOpenReasons();
  updateOpenState(true);
  closingFromHost_ = false;
}

void AdPopover::popupRelayoutFromHost() {
  if (open_) {
    syncPopupGeometry();
  }
}

}  // namespace adqt::widgets
