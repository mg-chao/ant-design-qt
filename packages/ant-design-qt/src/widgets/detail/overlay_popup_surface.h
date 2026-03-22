#pragma once

#include "../popup_placement.h"

#include <QColor>
#include <QPointer>
#include <QWidget>

namespace adqt::widgets::detail {

struct OverlayPopupSurfaceMetrics {
  int borderRadius = 8;
  int borderWidth = 1;
  int arrowSize = 8;
};

struct OverlayPopupSurfaceStyle {
  QColor background;
  QColor borderColor;
  QColor arrowBackground;
  QColor arrowBorderColor;
  OverlayPopupSurfaceMetrics metrics;
};

class OverlayPopupSurface final : public QWidget {
 public:
  explicit OverlayPopupSurface(QWidget* parent = nullptr);

  QWidget* bodyWidget() const { return bodyWidget_; }

  OverlayPopupSurfaceStyle surfaceStyle() const { return style_; }
  void setSurfaceStyle(const OverlayPopupSurfaceStyle& style);

  bool arrowVisible() const { return arrowVisible_; }
  void setArrowVisible(bool visible);

  OverlayPopupPlacement placement() const { return placement_; }
  void setPlacement(OverlayPopupPlacement placement);

  qreal arrowCenter() const { return arrowCenter_; }
  void setArrowCenter(qreal center);

  QSize sizeHint() const override;

 protected:
  void resizeEvent(QResizeEvent* event) override;
  void paintEvent(QPaintEvent* event) override;

 private:
  enum class ArrowSide {
    None,
    Top,
    Bottom,
    Left,
    Right,
  };

  static ArrowSide arrowSideForPlacement(OverlayPopupPlacement placement);
  int arrowProjection() const;
  qreal arrowBaseHalfWidth() const;
  qreal arrowBaseInsetForUnion() const;
  QRectF bubbleRectForPaint() const;
  qreal clampedArrowCenter(const QRectF& bubbleRect) const;
  QPolygonF arrowPolygon(const QRectF& bubbleRect) const;
  void updateBodyGeometry();

  QPointer<QWidget> bodyWidget_;
  OverlayPopupSurfaceStyle style_;
  OverlayPopupPlacement placement_ = OverlayPopupPlacement::Top;
  ArrowSide arrowSide_ = ArrowSide::Bottom;
  bool arrowVisible_ = true;
  qreal arrowCenter_ = 0.0;
};

}  // namespace adqt::widgets::detail
