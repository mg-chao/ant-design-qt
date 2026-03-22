#include "scroll_area.h"

#include "detail/themed_scrollbar.h"

#include <QEvent>
#include <QFrame>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSignalBlocker>

#include <algorithm>

namespace adqt::widgets {

void AdScrollArea::applyThemedScrollBar(QScrollBar* bar,
                                        int extent,
                                        int radius,
                                        int inset,
                                        int marginStart,
                                        int marginEnd) {
  detail::applyThemedScrollBar(bar, extent, radius, inset, marginStart, marginEnd);
}

AdScrollArea::AdScrollArea(QWidget* parent) : QScrollArea(parent) {
  setObjectName(QStringLiteral("adscrollarea"));
  setFrameShape(QFrame::NoFrame);
  setAlignment(Qt::AlignTop | Qt::AlignLeft);
  setWidgetResizable(false);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  if (viewport()) {
    viewport()->installEventFilter(this);
    overlayVerticalScrollBar_ = new QScrollBar(Qt::Vertical, viewport());
    overlayVerticalScrollBar_->setObjectName(QStringLiteral("adscrollarea-overlay-vbar"));
    overlayVerticalScrollBar_->setFocusPolicy(Qt::NoFocus);
    overlayVerticalScrollBar_->installEventFilter(this);
    overlayVerticalScrollBar_->hide();
    overlayVerticalScrollBar_->raise();
  }

  if (verticalScrollBar()) {
    connect(verticalScrollBar(), &QScrollBar::rangeChanged, this, [this](int, int) { syncOverlayScrollBar(); });
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int) { syncOverlayScrollBar(); });
  }
  if (overlayVerticalScrollBar_) {
    connect(overlayVerticalScrollBar_, &QScrollBar::valueChanged, this, [this](int value) {
      QScrollBar* source = verticalScrollBar();
      if (!source || source->value() == value) {
        return;
      }
      source->setValue(value);
    });
  }

  applyScrollBarStyle();
  syncOverlayScrollBar();
}

void AdScrollArea::setContentWidget(QWidget* widget) {
  if (contentWidget_ == widget) {
    return;
  }

  if (contentWidget_) {
    contentWidget_->removeEventFilter(this);
  }

  QWidget* previous = takeWidget();
  if (previous && previous != widget) {
    previous->removeEventFilter(this);
  }

  contentWidget_ = widget;
  if (contentWidget_) {
    contentWidget_->installEventFilter(this);
    setWidget(contentWidget_);
    syncContentSize();
    return;
  }
  syncOverlayScrollBar();
}

QWidget* AdScrollArea::contentWidget() const { return contentWidget_; }

bool AdScrollArea::fitToWidth() const { return fitToWidth_; }

void AdScrollArea::setFitToWidth(bool value) {
  if (fitToWidth_ == value) {
    return;
  }
  fitToWidth_ = value;
  syncContentSize();
  emit fitToWidthChanged(fitToWidth_);
}

int AdScrollArea::scrollBarThickness() const { return scrollBarThickness_; }

void AdScrollArea::setScrollBarThickness(int value) {
  const int normalized = std::max(6, value);
  if (scrollBarThickness_ == normalized) {
    return;
  }
  scrollBarThickness_ = normalized;
  applyScrollBarStyle();
  updateOverlayGeometry();
  emit scrollBarThicknessChanged(scrollBarThickness_);
}

int AdScrollArea::scrollBarRadius() const { return scrollBarRadius_; }

void AdScrollArea::setScrollBarRadius(int value) {
  const int normalized = std::max(0, value);
  if (scrollBarRadius_ == normalized) {
    return;
  }
  scrollBarRadius_ = normalized;
  applyScrollBarStyle();
  emit scrollBarRadiusChanged(scrollBarRadius_);
}

bool AdScrollArea::eventFilter(QObject* watched, QEvent* event) {
  if (watched == overlayVerticalScrollBar_ && event) {
    switch (event->type()) {
      case QEvent::Enter:
      case QEvent::HoverEnter:
        if (!overlayHovered_) {
          overlayHovered_ = true;
          applyScrollBarStyle();
        }
        break;
      case QEvent::Leave:
      case QEvent::HoverLeave:
      case QEvent::Hide:
        if (overlayHovered_) {
          overlayHovered_ = false;
          applyScrollBarStyle();
        }
        break;
      default:
        break;
    }
  }

  if (event && (watched == contentWidget_ || watched == viewport())) {
    switch (event->type()) {
      case QEvent::LayoutRequest:
      case QEvent::Resize:
      case QEvent::Show:
      case QEvent::StyleChange:
      case QEvent::FontChange:
        syncContentSize();
        if (watched == viewport()) {
          updateOverlayGeometry();
        }
        break;
      default:
        break;
    }
  }
  return QScrollArea::eventFilter(watched, event);
}

void AdScrollArea::changeEvent(QEvent* event) {
  QScrollArea::changeEvent(event);
  if (!event) {
    return;
  }

  switch (event->type()) {
    case QEvent::PaletteChange:
    case QEvent::StyleChange:
    case QEvent::ApplicationPaletteChange:
    case QEvent::FontChange:
    case QEvent::ApplicationFontChange:
      applyScrollBarStyle();
      syncContentSize();
      break;
    default:
      break;
  }
}

void AdScrollArea::resizeEvent(QResizeEvent* event) {
  QScrollArea::resizeEvent(event);
  syncContentSize();
  syncOverlayScrollBar();
}

void AdScrollArea::applyScrollBarStyle() {
  const int thickness = std::max(6, scrollBarThickness_);
  const int hoverThickness = thickness + std::max(1, thickness / 2);
  const int verticalExtent = overlayHovered_ ? hoverThickness : thickness;
  const int collapsedVisualThickness = 3;
  const int collapsedInset = std::max(0, thickness - collapsedVisualThickness);
  const int visualInset = overlayHovered_ ? 0 : collapsedInset;
  const int visualWidth = std::max(1, verticalExtent - visualInset);
  const int visualRadius = std::max(1, (visualWidth + 1) / 2);

  if (overlayVerticalScrollBar_) {
    applyThemedScrollBar(overlayVerticalScrollBar_, verticalExtent, visualRadius, visualInset);
  }

  if (QScrollBar* hBar = horizontalScrollBar()) {
    applyThemedScrollBar(hBar, thickness, std::max(1, thickness / 2), 0);
  }

  updateOverlayGeometry();
  if (viewport()) {
    viewport()->update();
  }
}

void AdScrollArea::syncContentSize() {
  if (!contentWidget_ || syncingContentSize_) {
    return;
  }

  syncingContentSize_ = true;

  QSize hint = contentWidget_->sizeHint();
  if (!hint.isValid()) {
    hint = contentWidget_->minimumSizeHint();
  }
  if (!hint.isValid()) {
    hint = contentWidget_->size();
  }

  int targetWidth = hint.width();
  if (fitToWidth_) {
    targetWidth = viewport()->width();
  } else {
    targetWidth = std::max(targetWidth, contentWidget_->minimumSizeHint().width());
  }

  int targetHeight = std::max(hint.height(), contentWidget_->minimumSizeHint().height());
  targetWidth = std::max(0, targetWidth);
  targetHeight = std::max(0, targetHeight);

  const QSize nextSize(targetWidth, targetHeight);
  if (contentWidget_->size() != nextSize) {
    contentWidget_->resize(nextSize);
  }

  syncingContentSize_ = false;
  syncOverlayScrollBar();
}

void AdScrollArea::syncOverlayScrollBar() {
  if (!overlayVerticalScrollBar_) {
    return;
  }

  QScrollBar* source = verticalScrollBar();
  if (!source) {
    const bool wasHovered = overlayHovered_;
    overlayHovered_ = false;
    if (wasHovered) {
      applyScrollBarStyle();
    }
    overlayVerticalScrollBar_->hide();
    return;
  }

  {
    QSignalBlocker blocker(overlayVerticalScrollBar_);
    overlayVerticalScrollBar_->setRange(source->minimum(), source->maximum());
    overlayVerticalScrollBar_->setPageStep(source->pageStep());
    overlayVerticalScrollBar_->setSingleStep(source->singleStep());
    overlayVerticalScrollBar_->setValue(source->value());
  }

  const bool visible = source->maximum() > source->minimum();
  if (!visible && overlayHovered_) {
    overlayHovered_ = false;
    applyScrollBarStyle();
  }
  overlayVerticalScrollBar_->setVisible(visible);
  updateOverlayGeometry();
  if (visible) {
    overlayVerticalScrollBar_->raise();
  }
}

void AdScrollArea::updateOverlayGeometry() {
  if (!overlayVerticalScrollBar_ || !viewport()) {
    return;
  }

  const int margin = 2;
  const int thickness = std::max(6, scrollBarThickness_);
  const int hoverThickness = thickness + std::max(1, thickness / 2);
  const int overlayWidth = overlayHovered_ ? hoverThickness : thickness;
  const int height = std::max(0, viewport()->height() - margin * 2);
  const int x = std::max(0, viewport()->width() - overlayWidth - margin);
  overlayVerticalScrollBar_->setGeometry(x, margin, overlayWidth, height);
}


}  // namespace adqt::widgets
