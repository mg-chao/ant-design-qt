#include "scroll_area.h"

#include "theme/fast_color_lite.h"
#include "theme/theme.h"

#include <QEvent>
#include <QFrame>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSignalBlocker>

#include <algorithm>

namespace adqt::widgets {

namespace {

QColor toColor(const QString& value, const QColor& fallback) {
  const adqt::theme::FastColorLite parsed(value);
  if (!parsed.isValid()) {
    return fallback;
  }

  QColor color;
  color.setRed(parsed.red());
  color.setGreen(parsed.green());
  color.setBlue(parsed.blue());
  color.setAlphaF(parsed.alpha());
  return color;
}

QColor withAlpha(const QColor& color, double alpha) {
  QColor copy = color;
  copy.setAlphaF(std::clamp(alpha, 0.0, 1.0));
  return copy;
}

}  // namespace

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

  connect(&adqt::theme::ThemeManager::instance(), &adqt::theme::ThemeManager::themeChanged, this, [this]() {
    applyScrollBarStyle();
    syncContentSize();
  });

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

void AdScrollArea::resizeEvent(QResizeEvent* event) {
  QScrollArea::resizeEvent(event);
  syncContentSize();
  syncOverlayScrollBar();
}

void AdScrollArea::applyScrollBarStyle() {
  const adqt::theme::ThemeMapToken& map = adqt::theme::ThemeManager::instance().currentMapToken();

  const QColor trackColor = withAlpha(toColor(map.colorTextQuaternary, QColor("#8c8c8c")), 0.18);
  const QColor handleColor = withAlpha(toColor(map.colorTextSecondary, QColor("#595959")), 0.45);
  const QColor handleHoverColor = withAlpha(toColor(map.colorText, QColor("#141414")), 0.60);
  const QColor handlePressedColor = withAlpha(toColor(map.colorText, QColor("#141414")), 0.75);
  const int collapsedVisualThickness = std::max(1, std::min(2, scrollBarThickness_));
  const int collapsedInset = std::max(0, scrollBarThickness_ - collapsedVisualThickness);
  const int visualInset = overlayHovered_ ? 0 : collapsedInset;
  const int visualWidth = std::max(1, scrollBarThickness_ - visualInset);
  const int visualRadius = overlayHovered_ ? scrollBarRadius_ : std::max(1, visualWidth / 2);

  const QString sheet = QStringLiteral(
                            "QScrollArea#adscrollarea {"
                            "  border: none;"
                            "  background: transparent;"
                            "}"
                            "QScrollBar#adscrollarea-overlay-vbar:vertical {"
                            "  background: %1;"
                            "  width: %2px;"
                            "  margin: 0px 0px 0px %7px;"
                            "  border: none;"
                            "  border-radius: %8px;"
                            "}"
                            "QScrollBar#adscrollarea-overlay-vbar::handle:vertical {"
                            "  background: %4;"
                            "  min-height: 24px;"
                            "  margin: 0px;"
                            "  border-radius: %8px;"
                            "}"
                            "QScrollBar#adscrollarea-overlay-vbar::handle:vertical:hover {"
                            "  background: %5;"
                            "}"
                            "QScrollBar#adscrollarea-overlay-vbar::handle:vertical:pressed {"
                            "  background: %6;"
                            "}"
                            "QScrollBar#adscrollarea-overlay-vbar::add-line:vertical,"
                            "QScrollBar#adscrollarea-overlay-vbar::sub-line:vertical {"
                            "  width: 0px;"
                            "  height: 0px;"
                            "  border: none;"
                            "  margin: 0px;"
                            "}"
                            "QScrollBar#adscrollarea-overlay-vbar::add-page:vertical,"
                            "QScrollBar#adscrollarea-overlay-vbar::sub-page:vertical {"
                            "  background: transparent;"
                            "  margin: 0px;"
                            "  border-radius: 0px;"
                            "}"
                            "QScrollBar:horizontal {"
                            "  background: %1;"
                            "  height: %2px;"
                            "  margin: 0px 2px 0px 2px;"
                            "  border: none;"
                            "  border-radius: %3px;"
                            "}"
                            "QScrollBar::handle:horizontal {"
                            "  background: %4;"
                            "  min-width: 24px;"
                            "  border-radius: %3px;"
                            "}"
                            "QScrollBar::handle:horizontal:hover {"
                            "  background: %5;"
                            "}"
                            "QScrollBar::handle:horizontal:pressed {"
                            "  background: %6;"
                            "}"
                            "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {"
                            "  width: 0px;"
                            "  height: 0px;"
                            "  border: none;"
                            "  margin: 0px;"
                            "}"
                            "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {"
                            "  background: transparent;"
                            "}")
                            .arg(trackColor.name(QColor::HexArgb))
                            .arg(scrollBarThickness_)
                            .arg(scrollBarRadius_)
                            .arg(handleColor.name(QColor::HexArgb))
                            .arg(handleHoverColor.name(QColor::HexArgb))
                            .arg(handlePressedColor.name(QColor::HexArgb))
                            .arg(visualInset)
                            .arg(visualRadius);

  setStyleSheet(sheet);
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
  const int height = std::max(0, viewport()->height() - margin * 2);
  const int x = std::max(0, viewport()->width() - thickness - margin);
  overlayVerticalScrollBar_->setGeometry(x, margin, thickness, height);
}

}  // namespace adqt::widgets
