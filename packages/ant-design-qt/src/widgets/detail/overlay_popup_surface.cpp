#include "overlay_popup_surface.h"

#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QResizeEvent>

#include <algorithm>

namespace adqt::widgets::detail {

OverlayPopupSurface::OverlayPopupSurface(QWidget* parent) : QWidget(parent) {
  setAttribute(Qt::WA_Hover, true);
  setAutoFillBackground(false);

  bodyWidget_ = new QWidget(this);
}

void OverlayPopupSurface::setSurfaceStyle(const OverlayPopupSurfaceStyle& style) {
  style_ = style;
  updateBodyGeometry();
  updateGeometry();
  update();
}

void OverlayPopupSurface::setArrowVisible(bool visible) {
  if (arrowVisible_ == visible) {
    return;
  }
  arrowVisible_ = visible;
  updateBodyGeometry();
  updateGeometry();
  update();
}

void OverlayPopupSurface::setPlacement(OverlayPopupPlacement placement) {
  if (placement_ == placement) {
    return;
  }
  placement_ = placement;
  arrowSide_ = arrowSideForPlacement(placement_);
  updateBodyGeometry();
  updateGeometry();
  update();
}

void OverlayPopupSurface::setArrowCenter(qreal center) {
  if (qFuzzyCompare(static_cast<double>(arrowCenter_), static_cast<double>(center))) {
    return;
  }
  arrowCenter_ = center;
  update();
}

QSize OverlayPopupSurface::sizeHint() const {
  const QSize bodyHint = bodyWidget_ ? bodyWidget_->sizeHint() : QSize(120, 32);
  const int arrowSize = arrowProjection();
  const int widthPadding =
      (arrowSide_ == ArrowSide::Left || arrowSide_ == ArrowSide::Right) ? arrowSize : 0;
  const int heightPadding =
      (arrowSide_ == ArrowSide::Top || arrowSide_ == ArrowSide::Bottom) ? arrowSize : 0;
  return QSize(std::max(1, bodyHint.width() + widthPadding),
               std::max(1, bodyHint.height() + heightPadding));
}

void OverlayPopupSurface::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  updateBodyGeometry();
}

void OverlayPopupSurface::paintEvent(QPaintEvent* event) {
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
  QPainterPath arrowPath;
  if (!arrow.isEmpty()) {
    arrowPath.addPolygon(arrow);
  }

  const QColor resolvedArrowBackground =
      style_.arrowBackground.isValid() ? style_.arrowBackground : style_.background;
  const QColor resolvedArrowBorder =
      style_.arrowBorderColor.isValid() ? style_.arrowBorderColor : style_.borderColor;
  const bool unifiedArrowFill = arrowPath.isEmpty() || resolvedArrowBackground == style_.background;
  const bool unifiedArrowBorder = arrowPath.isEmpty() || resolvedArrowBorder == style_.borderColor;

  if (unifiedArrowFill) {
    QPainterPath fillPath = bubblePath;
    if (!arrowPath.isEmpty()) {
      fillPath = fillPath.united(arrowPath);
    }
    painter.fillPath(fillPath, style_.background);
  } else {
    painter.fillPath(bubblePath, style_.background);
    painter.fillPath(arrowPath, resolvedArrowBackground);
  }

  if (style_.metrics.borderWidth <= 0) {
    return;
  }

  if (unifiedArrowBorder) {
    if (style_.borderColor.alpha() <= 0) {
      return;
    }
    QPainterPath strokePath = bubblePath;
    if (!arrowPath.isEmpty()) {
      strokePath = strokePath.united(arrowPath);
    }
    QPen pen(style_.borderColor, style_.metrics.borderWidth);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(strokePath);
    return;
  }

  painter.setBrush(Qt::NoBrush);
  if (style_.borderColor.alpha() > 0) {
    QPen pen(style_.borderColor, style_.metrics.borderWidth);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);
    painter.drawPath(bubblePath);
  }
  if (!arrowPath.isEmpty() && resolvedArrowBorder.alpha() > 0) {
    QPen pen(resolvedArrowBorder, style_.metrics.borderWidth);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);
    painter.drawPath(arrowPath);
  }
}

OverlayPopupSurface::ArrowSide OverlayPopupSurface::arrowSideForPlacement(
    OverlayPopupPlacement placement) {
  switch (placement) {
    case OverlayPopupPlacement::Top:
    case OverlayPopupPlacement::TopLeft:
    case OverlayPopupPlacement::TopRight:
      return ArrowSide::Bottom;
    case OverlayPopupPlacement::Bottom:
    case OverlayPopupPlacement::BottomLeft:
    case OverlayPopupPlacement::BottomRight:
      return ArrowSide::Top;
    case OverlayPopupPlacement::Left:
    case OverlayPopupPlacement::LeftTop:
    case OverlayPopupPlacement::LeftBottom:
      return ArrowSide::Right;
    case OverlayPopupPlacement::Right:
    case OverlayPopupPlacement::RightTop:
    case OverlayPopupPlacement::RightBottom:
      return ArrowSide::Left;
  }
  return ArrowSide::None;
}

int OverlayPopupSurface::arrowProjection() const {
  return arrowVisible_ ? std::max(0, style_.metrics.arrowSize) : 0;
}

qreal OverlayPopupSurface::arrowBaseHalfWidth() const { return static_cast<qreal>(arrowProjection()); }

qreal OverlayPopupSurface::arrowBaseInsetForUnion() const {
  return std::max(1.0, static_cast<qreal>(style_.metrics.borderWidth));
}

QRectF OverlayPopupSurface::bubbleRectForPaint() const {
  const int arrow = arrowProjection();
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

qreal OverlayPopupSurface::clampedArrowCenter(const QRectF& bubbleRect) const {
  const int projection = arrowProjection();
  if (projection <= 0) {
    return 0.0;
  }
  const qreal half = arrowBaseHalfWidth();
  const qreal radius = std::max(0.0, static_cast<qreal>(style_.metrics.borderRadius));
  if (arrowSide_ == ArrowSide::Top || arrowSide_ == ArrowSide::Bottom) {
    const qreal minX = bubbleRect.left() + radius + half + 1.0;
    const qreal maxX = bubbleRect.right() - radius - half - 1.0;
    if (minX > maxX) {
      return bubbleRect.center().x();
    }
    const qreal fallback = bubbleRect.center().x();
    const qreal target = (arrowCenter_ > 0.0) ? arrowCenter_ : fallback;
    return std::clamp(target, minX, maxX);
  }
  if (arrowSide_ == ArrowSide::Left || arrowSide_ == ArrowSide::Right) {
    const qreal minY = bubbleRect.top() + radius + half + 1.0;
    const qreal maxY = bubbleRect.bottom() - radius - half - 1.0;
    if (minY > maxY) {
      return bubbleRect.center().y();
    }
    const qreal fallback = bubbleRect.center().y();
    const qreal target = (arrowCenter_ > 0.0) ? arrowCenter_ : fallback;
    return std::clamp(target, minY, maxY);
  }
  return 0.0;
}

QPolygonF OverlayPopupSurface::arrowPolygon(const QRectF& bubbleRect) const {
  const int projection = arrowProjection();
  if (projection <= 0) {
    return {};
  }

  const qreal center = clampedArrowCenter(bubbleRect);
  const qreal half = arrowBaseHalfWidth();
  const qreal baseInset = arrowBaseInsetForUnion();
  QPolygonF polygon;
  switch (arrowSide_) {
    case ArrowSide::Top:
      polygon << QPointF(center, bubbleRect.top() - projection)
              << QPointF(center - half, bubbleRect.top() + baseInset)
              << QPointF(center + half, bubbleRect.top() + baseInset);
      break;
    case ArrowSide::Bottom:
      polygon << QPointF(center, bubbleRect.bottom() + projection)
              << QPointF(center - half, bubbleRect.bottom() - baseInset)
              << QPointF(center + half, bubbleRect.bottom() - baseInset);
      break;
    case ArrowSide::Left:
      polygon << QPointF(bubbleRect.left() - projection, center)
              << QPointF(bubbleRect.left() + baseInset, center - half)
              << QPointF(bubbleRect.left() + baseInset, center + half);
      break;
    case ArrowSide::Right:
      polygon << QPointF(bubbleRect.right() + projection, center)
              << QPointF(bubbleRect.right() - baseInset, center - half)
              << QPointF(bubbleRect.right() - baseInset, center + half);
      break;
    case ArrowSide::None:
      break;
  }
  return polygon;
}

void OverlayPopupSurface::updateBodyGeometry() {
  if (!bodyWidget_) {
    return;
  }
  const int arrow = arrowProjection();
  int left = 0;
  int top = 0;
  int right = 0;
  int bottom = 0;
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

}  // namespace adqt::widgets::detail
