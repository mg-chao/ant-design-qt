#include "interaction_overlay_manager.h"
#include "detail/timing_hub.h"

#include <QHash>
#include <QPainter>
#include <QPainterPath>
#include <QPointer>
#include <QString>
#include <QWidget>

#include <algorithm>

namespace adqt::widgets {

namespace {

constexpr int kInteractionWaveSpreadDurationMs = 300;
constexpr int kInteractionWaveThickenDurationMs = 180;
constexpr qreal kInteractionWaveInitialOpacity = 0.16;
constexpr qreal kInteractionWaveStrokeWidth = 1.6;
constexpr char kWaveFrameKey[] = "SharedInteractionOverlay.WaveFrame";

qreal clampUnit(qreal value) { return std::clamp(value, 0.0, 1.0); }

qreal easeOutCirc(qreal t) {
  const qreal x = clampUnit(t) - 1.0;
  return std::sqrt(std::max<qreal>(0.0, 1.0 - x * x));
}

qreal lerp(qreal a, qreal b, qreal t) { return a + (b - a) * clampUnit(t); }

qreal expandedCornerRadius(qreal baseRadius, qreal outwardOffset) {
  if (baseRadius <= 0.0) {
    return 0.0;
  }
  return baseRadius + std::max<qreal>(0.0, outwardOffset);
}

QPainterPath roundedRectPath(const QRectF& rect, qreal topLeft, qreal topRight, qreal bottomRight,
                             qreal bottomLeft) {
  const qreal w = std::max(rect.width(), 0.0);
  const qreal h = std::max(rect.height(), 0.0);
  const qreal maxRadius = std::min(w, h) / 2.0;

  topLeft = std::clamp(topLeft, 0.0, maxRadius);
  topRight = std::clamp(topRight, 0.0, maxRadius);
  bottomRight = std::clamp(bottomRight, 0.0, maxRadius);
  bottomLeft = std::clamp(bottomLeft, 0.0, maxRadius);

  const qreal left = rect.left();
  const qreal top = rect.top();
  const qreal right = left + rect.width();
  const qreal bottom = top + rect.height();

  QPainterPath path;
  path.moveTo(left + topLeft, top);
  path.lineTo(right - topRight, top);
  if (topRight > 0.0) {
    path.arcTo(QRectF(right - 2.0 * topRight, top, 2.0 * topRight, 2.0 * topRight), 90.0, -90.0);
  }
  path.lineTo(right, bottom - bottomRight);
  if (bottomRight > 0.0) {
    path.arcTo(QRectF(right - 2.0 * bottomRight, bottom - 2.0 * bottomRight, 2.0 * bottomRight,
                      2.0 * bottomRight),
               0.0, -90.0);
  }
  path.lineTo(left + bottomLeft, bottom);
  if (bottomLeft > 0.0) {
    path.arcTo(QRectF(left, bottom - 2.0 * bottomLeft, 2.0 * bottomLeft, 2.0 * bottomLeft), 270.0,
               -90.0);
  }
  path.lineTo(left, top + topLeft);
  if (topLeft > 0.0) {
    path.arcTo(QRectF(left, top, 2.0 * topLeft, 2.0 * topLeft), 180.0, -90.0);
  }
  path.closeSubpath();
  return path;
}

bool isValidInteractionWaveRequest(const InteractionWaveRequest& request) {
  if (!request.owner) {
    return false;
  }
  if (!request.baseRectInWindow.isValid() || request.baseRectInWindow.width() <= 0.0 ||
      request.baseRectInWindow.height() <= 0.0) {
    return false;
  }
  if (!request.color.isValid() || request.color.alpha() <= 0) {
    return false;
  }
  if (request.strokeWidthScale <= 0.0) {
    return false;
  }
  return true;
}

bool isValidInteractionFocusRequest(const InteractionFocusRequest& request) {
  if (!request.owner) {
    return false;
  }
  if (!request.baseRectInWindow.isValid() || request.baseRectInWindow.width() <= 0.0 ||
      request.baseRectInWindow.height() <= 0.0) {
    return false;
  }
  if (!request.color.isValid() || request.color.alpha() <= 0) {
    return false;
  }
  if (request.strokeWidth <= 0.0) {
    return false;
  }
  return true;
}

class SharedInteractionOverlay final : public QWidget {
 public:
  explicit SharedInteractionOverlay(QWidget* parent = nullptr) : QWidget(parent) {
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    hide();
  }

  void triggerInteractionWave(const InteractionWaveRequest& request) {
    if (!isValidInteractionWaveRequest(request)) {
      return;
    }

    interactionWaveOwner_ = request.owner;
    interactionWaveRequest_ = request;
    interactionWaveStartMs_ = detail::timingNowMs();
    interactionWaveActive_ = true;
    syncGeometryToParent();
    ensureVisibleAndRaised();
    update();
    detail::setFrameSubscription(this, QString::fromLatin1(kWaveFrameKey), true, [this](qint64, qint64) {
      advanceInteractionWaveFrame();
    });
  }

  void triggerInteractionFocus(const InteractionFocusRequest& request) {
    if (!isValidInteractionFocusRequest(request)) {
      return;
    }

    interactionFocusOwner_ = request.owner;
    interactionFocusRequest_ = request;
    interactionFocusActive_ = true;
    syncGeometryToParent();
    ensureVisibleAndRaised();
    update();
  }

  void stopInteractionWaveIfOwner(const QWidget* owner) {
    if (!owner || interactionWaveOwner_ != owner) {
      return;
    }
    resetInteractionWave();
  }

  void stopInteractionFocusIfOwner(const QWidget* owner) {
    if (!owner || interactionFocusOwner_ != owner) {
      return;
    }
    resetInteractionFocus();
  }

 protected:
  void paintEvent(QPaintEvent* event) override {
    Q_UNUSED(event)

    if (!interactionWaveActive_ && !interactionFocusActive_) {
      return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    if (interactionWaveActive_) {
      const int totalWaveDurationMs = std::max(0, detail::waveDurationMs());
      const qint64 elapsedMs = std::max<qint64>(0, detail::timingNowMs() - interactionWaveStartMs_);
      const qreal spreadProgress =
          easeOutCirc(static_cast<qreal>(elapsedMs) / kInteractionWaveSpreadDurationMs);
      const qreal fadeProgress =
          totalWaveDurationMs > 0
              ? clampUnit(static_cast<qreal>(elapsedMs) / totalWaveDurationMs)
              : 1.0;
      const qreal thickenProgress =
          easeOutCirc(static_cast<qreal>(elapsedMs) / kInteractionWaveThickenDurationMs);
      const qreal opacity = kInteractionWaveInitialOpacity * (1.0 - fadeProgress);

      if (opacity > 0.0) {
        // antd wave is perceived as a thin ring that quickly thickens (box-shadow 0 -> 6px).
        // Keep this approximation low-cost by animating pen width with a fast ease-out ramp.
        const qreal strokeScale = std::max<qreal>(0.1, interactionWaveRequest_.strokeWidthScale);
        const qreal startStroke =
            std::max<qreal>(0.6, (kInteractionWaveStrokeWidth * 0.32) * strokeScale);
        const qreal endStroke = std::max<qreal>(startStroke, 6.0 * strokeScale);
        const qreal strokeProgress = std::max(spreadProgress, thickenProgress);
        const qreal strokeWidth = lerp(startStroke, endStroke, strokeProgress);
        const qreal outwardOffset = strokeWidth * 0.5;

        // Keep the inner edge of the wave attached to button bounds, while it expands outward.
        const QRectF waveRect = interactionWaveRequest_.baseRectInWindow.adjusted(
            -outwardOffset, -outwardOffset, outwardOffset, outwardOffset);
        const QPainterPath wavePath =
            roundedRectPath(waveRect,
                            expandedCornerRadius(interactionWaveRequest_.topLeft, outwardOffset),
                            expandedCornerRadius(interactionWaveRequest_.topRight, outwardOffset),
                            expandedCornerRadius(interactionWaveRequest_.bottomRight, outwardOffset),
                            expandedCornerRadius(interactionWaveRequest_.bottomLeft, outwardOffset));

        QColor waveColor = interactionWaveRequest_.color;
        // Keep source alpha and apply animation opacity on top to match antd behavior.
        const qreal baseAlpha = clampUnit(waveColor.alphaF());
        waveColor.setAlphaF(baseAlpha * clampUnit(opacity));
        if (waveColor.alpha() > 0) {
          QPen wavePen(waveColor, strokeWidth, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
          painter.setPen(wavePen);
          painter.setBrush(Qt::NoBrush);
          painter.drawPath(wavePath);
        }
      }
    }

    if (interactionFocusActive_) {
      const qreal strokeWidth = std::max<qreal>(1.0, interactionFocusRequest_.strokeWidth);
      const qreal outwardOffset =
          std::max<qreal>(0.0, interactionFocusRequest_.offset) + strokeWidth / 2.0;
      const QRectF focusRect = interactionFocusRequest_.baseRectInWindow.adjusted(
          -outwardOffset, -outwardOffset, outwardOffset, outwardOffset);
      const QPainterPath focusPath =
          roundedRectPath(focusRect,
                          expandedCornerRadius(interactionFocusRequest_.topLeft, outwardOffset),
                          expandedCornerRadius(interactionFocusRequest_.topRight, outwardOffset),
                          expandedCornerRadius(interactionFocusRequest_.bottomRight, outwardOffset),
                          expandedCornerRadius(interactionFocusRequest_.bottomLeft, outwardOffset));

      QPen focusPen(interactionFocusRequest_.color, strokeWidth, Qt::SolidLine, Qt::SquareCap,
                    Qt::MiterJoin);
      painter.setPen(focusPen);
      painter.setBrush(Qt::NoBrush);
      painter.drawPath(focusPath);
    }
  }

 private:
  void advanceInteractionWaveFrame() {
    if (!interactionWaveActive_) {
      detail::clearFrameSubscription(this, QString::fromLatin1(kWaveFrameKey));
      return;
    }

    const int totalWaveDurationMs = std::max(0, detail::waveDurationMs());
    if (totalWaveDurationMs <= 0) {
      resetInteractionWave();
      return;
    }

    const qint64 elapsedMs = std::max<qint64>(0, detail::timingNowMs() - interactionWaveStartMs_);
    if (elapsedMs >= totalWaveDurationMs) {
      resetInteractionWave();
      return;
    }

    syncGeometryToParent();
    update();
  }

  void ensureVisibleAndRaised() {
    if (!isVisible()) {
      show();
    }
    raise();
  }

  void syncGeometryToParent() {
    QWidget* p = parentWidget();
    if (!p) {
      return;
    }
    const QRect parentRect = p->rect();
    if (geometry() != parentRect) {
      setGeometry(parentRect);
    }
  }

  void resetInteractionWave() {
    interactionWaveActive_ = false;
    interactionWaveOwner_.clear();
    interactionWaveStartMs_ = 0;
    detail::clearFrameSubscription(this, QString::fromLatin1(kWaveFrameKey));
    if (!interactionFocusActive_) {
      hide();
    }
    update();
  }

  void resetInteractionFocus() {
    interactionFocusActive_ = false;
    interactionFocusOwner_.clear();
    if (!interactionWaveActive_) {
      hide();
    }
    update();
  }

  QPointer<const QWidget> interactionWaveOwner_;
  InteractionWaveRequest interactionWaveRequest_;
  QPointer<const QWidget> interactionFocusOwner_;
  InteractionFocusRequest interactionFocusRequest_;
  qint64 interactionWaveStartMs_ = 0;
  bool interactionWaveActive_ = false;
  bool interactionFocusActive_ = false;
};

QHash<QWidget*, SharedInteractionOverlay*>& interactionOverlayMap() {
  static QHash<QWidget*, SharedInteractionOverlay*> overlays;
  return overlays;
}

SharedInteractionOverlay* ensureInteractionOverlay(QWidget* hostWindow) {
  if (!hostWindow) {
    return nullptr;
  }

  auto& overlays = interactionOverlayMap();
  const auto existing = overlays.constFind(hostWindow);
  if (existing != overlays.constEnd() && existing.value()) {
    return existing.value();
  }

  auto* overlay = new SharedInteractionOverlay(hostWindow);
  overlays.insert(hostWindow, overlay);

  QObject::connect(hostWindow, &QObject::destroyed,
                   [hostWindow]() { interactionOverlayMap().remove(hostWindow); });

  return overlay;
}

QWidget* resolveInteractionHostWindow(const QWidget* owner) {
  if (!owner) {
    return nullptr;
  }

  QWidget* hostWindow = owner->window();
  if (hostWindow) {
    return hostWindow;
  }

  return const_cast<QWidget*>(owner);
}

}  // namespace

void triggerInteractionWave(const InteractionWaveRequest& request) {
  if (!isValidInteractionWaveRequest(request)) {
    return;
  }

  QWidget* hostWindow = resolveInteractionHostWindow(request.owner);
  SharedInteractionOverlay* overlay = ensureInteractionOverlay(hostWindow);
  if (!overlay) {
    return;
  }

  overlay->triggerInteractionWave(request);
}

void stopInteractionWaveForOwner(const QWidget* owner) {
  if (!owner) {
    return;
  }

  auto& overlays = interactionOverlayMap();
  for (auto it = overlays.begin(); it != overlays.end(); ++it) {
    if (it.value()) {
      it.value()->stopInteractionWaveIfOwner(owner);
    }
  }
}

void triggerInteractionFocus(const InteractionFocusRequest& request) {
  if (!isValidInteractionFocusRequest(request)) {
    return;
  }

  QWidget* hostWindow = resolveInteractionHostWindow(request.owner);
  SharedInteractionOverlay* overlay = ensureInteractionOverlay(hostWindow);
  if (!overlay) {
    return;
  }

  overlay->triggerInteractionFocus(request);
}

void stopInteractionFocusForOwner(const QWidget* owner) {
  if (!owner) {
    return;
  }

  auto& overlays = interactionOverlayMap();
  for (auto it = overlays.begin(); it != overlays.end(); ++it) {
    if (it.value()) {
      it.value()->stopInteractionFocusIfOwner(owner);
    }
  }
}

}  // namespace adqt::widgets

