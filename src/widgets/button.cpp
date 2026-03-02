#include "button.h"

#include "button_style.h"
#include "interaction_overlay_manager.h"
#include "theme/theme.h"

#include <QApplication>
#include <QEvent>
#include <QFocusEvent>
#include <QFontMetrics>
#include <QHideEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QMoveEvent>
#include <QPainter>
#include <QPainterPath>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QSet>
#include <QShowEvent>
#include <QStyle>

#include <algorithm>

namespace adqt::widgets {

namespace {

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

bool isTwoChineseCharacters(const QString& text) {
  static const QRegularExpression re(QStringLiteral("^[\\x{4e00}-\\x{9fa5}]{2}$"));
  return re.match(text).hasMatch();
}

bool isKeyboardFocusReason(Qt::FocusReason reason) {
  return reason == Qt::TabFocusReason || reason == Qt::BacktabFocusReason ||
         reason == Qt::ShortcutFocusReason;
}

QPoint mouseEventPos(const QMouseEvent* event) {
  if (!event) {
    return QPoint();
  }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  return event->position().toPoint();
#else
  return event->pos();
#endif
}

int resolveIconSide(const QPushButton* button, const QFontMetrics& fm, const QFont& contentFont) {
  int iconSide = contentFont.pixelSize();
  if (iconSide <= 0) {
    const qreal pointSize = contentFont.pointSizeF();
    if (pointSize > 0.0) {
      iconSide = qRound(pointSize);
    }
  }
  if (iconSide <= 0) {
    iconSide = fm.height();
  }
  iconSide = std::max(10, iconSide);

  const QSize requestedIconSize = button->iconSize();
  if (!requestedIconSize.isValid() || requestedIconSize.isEmpty()) {
    return iconSide;
  }

  const int requestedSide = std::max(requestedIconSize.width(), requestedIconSize.height());
  int defaultStyleSide = -1;
  if (button->style()) {
    defaultStyleSide = button->style()->pixelMetric(QStyle::PM_ButtonIconSize, nullptr, button);
  }
  if (defaultStyleSide > 0 && requestedSide == defaultStyleSide) {
    return iconSide;
  }

  return requestedSide;
}

int measureTextWidth(const QFontMetrics& fm, const QString& text) {
  if (text.isEmpty()) {
    return 0;
  }
  // CSS inline layout uses glyph advance widths for line breaking and intrinsic width.
  // Prefer horizontalAdvance to match antd/browser width behavior.
  const int advance = fm.horizontalAdvance(text);
  if (advance > 0) {
    return advance;
  }
  return fm.boundingRect(text).width();
}

int resolveContentPixelSize(const QFontMetrics& fm, const QFont& contentFont) {
  int contentPx = contentFont.pixelSize();
  if (contentPx <= 0) {
    const qreal pointSize = contentFont.pointSizeF();
    if (pointSize > 0.0) {
      contentPx = qRound(pointSize);
    }
  }
  if (contentPx <= 0) {
    contentPx = fm.height();
  }
  return std::max(contentPx, 1);
}

int twoCnLetterSpacingPx(const QFontMetrics& fm, const QFont& contentFont) {
  // antd uses letter-spacing: 0.34em for two Chinese characters.
  return std::max(1, qRound(resolveContentPixelSize(fm, contentFont) * 0.34));
}

int measureDisplayTextWidth(const QFontMetrics& fm,
                            const QFont& contentFont,
                            const QString& text,
                            bool twoCnAutoSpacing) {
  if (text.isEmpty()) {
    return 0;
  }
  if (!twoCnAutoSpacing || !isTwoChineseCharacters(text)) {
    return measureTextWidth(fm, text);
  }

  const int first = fm.horizontalAdvance(QString(text.at(0)));
  const int second = fm.horizontalAdvance(QString(text.at(1)));
  return std::max(0, first) + std::max(0, second) + twoCnLetterSpacingPx(fm, contentFont);
}

int& sharedSpinnerAngle() {
  static int angle = 0;
  return angle;
}

QSet<QWidget*>& sharedSpinnerWidgets() {
  static QSet<QWidget*> widgets;
  return widgets;
}

QTimer& sharedSpinnerTimer() {
  static QTimer timer;
  static bool initialized = false;

  if (!initialized) {
    timer.setInterval(60);
    QObject::connect(&timer, &QTimer::timeout, []() {
      auto& widgets = sharedSpinnerWidgets();
      if (widgets.isEmpty()) {
        return;
      }

      sharedSpinnerAngle() = (sharedSpinnerAngle() + 24) % 360;

      for (QWidget* widget : widgets) {
        if (widget && widget->isVisible()) {
          widget->update();
        }
      }
    });
    initialized = true;
  }

  return timer;
}

void subscribeSharedSpinner(QWidget* widget) {
  if (!widget) {
    return;
  }

  auto& widgets = sharedSpinnerWidgets();
  if (widgets.contains(widget)) {
    return;
  }

  widgets.insert(widget);

  QTimer& timer = sharedSpinnerTimer();
  if (!timer.isActive()) {
    timer.start();
  }
}

void unsubscribeSharedSpinner(QWidget* widget) {
  if (!widget) {
    return;
  }

  auto& widgets = sharedSpinnerWidgets();
  widgets.remove(widget);

  if (widgets.isEmpty()) {
    QTimer& timer = sharedSpinnerTimer();
    if (timer.isActive()) {
      timer.stop();
    }
    sharedSpinnerAngle() = 0;
  }
}

bool isValidWaveColor(const QColor& color) {
  if (!color.isValid() || color.alpha() <= 0) {
    return false;
  }

  // Align with antd wave util semantics:
  // reject opaque white, but allow non-opaque colors.
  if (color.red() == 255 && color.green() == 255 && color.blue() == 255 &&
      color.alpha() == 255) {
    return false;
  }

  return true;
}

}  // namespace

struct AdButton::ContentLayout {
  QRect iconRect;
  QRect textRect;
  QString text;
  bool hasIcon = false;
  bool hasText = false;
};

AdButton::AdButton(QWidget* parent) : QPushButton(parent), baseSizePolicy_(sizePolicy()) {
  setAttribute(Qt::WA_Hover, true);
  setAutoDefault(false);
  setDefault(false);

  QSizePolicy policy = sizePolicy();
  policy.setHorizontalPolicy(QSizePolicy::Fixed);
  setSizePolicy(policy);
  baseSizePolicy_ = policy;

  loadingDelayTimer_.setSingleShot(true);
  connect(&loadingDelayTimer_, &QTimer::timeout, this, [this]() {
    if (!loading_) {
      return;
    }
    loadingVisible_ = true;
    updateSpinnerState();
    refreshAfterPropertyChange();
  });

  connect(&adqt::theme::ThemeManager::instance(), &adqt::theme::ThemeManager::themeChanged, this,
          [this]() { refreshAfterPropertyChange(); });

  refreshAfterPropertyChange();
}

AdButton::AdButton(const QString& text, QWidget* parent) : AdButton(parent) { setText(text); }

AdButton::~AdButton() {
  stopWaveEffect();
  stopInteractionFocusForOwner(this);

  if (spinnerSubscribed_) {
    unsubscribeSharedSpinner(this);
    spinnerSubscribed_ = false;
  }
}

AdButton::Type AdButton::type() const { return type_; }

void AdButton::setType(Type value) {
  if (type_ == value) {
    return;
  }
  type_ = value;
  refreshAfterPropertyChange();
  emit typeChanged(type_);
}

AdButton::Color AdButton::color() const { return color_; }

void AdButton::setColor(Color value) {
  if (color_ == value && colorExplicit_) {
    return;
  }
  color_ = value;
  colorExplicit_ = true;
  refreshAfterPropertyChange();
  emit colorChanged(color_);
}

AdButton::Variant AdButton::variant() const { return variant_; }

void AdButton::setVariant(Variant value) {
  if (variant_ == value && variantExplicit_) {
    return;
  }
  variant_ = value;
  variantExplicit_ = true;
  refreshAfterPropertyChange();
  emit variantChanged(variant_);
}

AdButton::Shape AdButton::shape() const { return shape_; }

void AdButton::setShape(Shape value) {
  if (shape_ == value) {
    return;
  }
  shape_ = value;
  refreshAfterPropertyChange();
  emit shapeChanged(shape_);
}

AdButton::Size AdButton::size() const { return size_; }

void AdButton::setSize(Size value) {
  if (size_ == value && sizeExplicit_) {
    return;
  }
  size_ = value;
  sizeExplicit_ = true;
  refreshAfterPropertyChange();
  emit sizeChanged(size_);
}

bool AdButton::danger() const { return danger_; }

void AdButton::setDanger(bool value) {
  if (danger_ == value) {
    return;
  }
  danger_ = value;
  refreshAfterPropertyChange();
  emit dangerChanged(danger_);
}

bool AdButton::ghost() const { return ghost_; }

void AdButton::setGhost(bool value) {
  if (ghost_ == value) {
    return;
  }
  ghost_ = value;
  refreshAfterPropertyChange();
  emit ghostChanged(ghost_);
}

bool AdButton::block() const { return block_; }

void AdButton::setBlock(bool value) {
  if (block_ == value) {
    return;
  }
  block_ = value;
  applyBlockSizePolicy();
  refreshAfterPropertyChange();
  emit blockChanged(block_);
}

bool AdButton::loading() const { return loading_; }

void AdButton::setLoading(bool value) {
  if (loading_ == value) {
    return;
  }
  loading_ = value;
  updateLoadingVisualState();
  updateCursorForRole();
  emit loadingChanged(loading_);
}

int AdButton::loadingDelay() const { return loadingDelay_; }

void AdButton::setLoadingDelay(int value) {
  const int normalized = std::max(0, value);
  if (loadingDelay_ == normalized) {
    return;
  }
  loadingDelay_ = normalized;
  updateLoadingVisualState();
  emit loadingDelayChanged(loadingDelay_);
}

AdButton::IconPlacement AdButton::iconPlacement() const { return iconPlacement_; }

void AdButton::setIconPlacement(IconPlacement value) {
  if (iconPlacement_ == value) {
    return;
  }
  iconPlacement_ = value;
  refreshAfterPropertyChange(false);
  emit iconPlacementChanged(iconPlacement_);
}

bool AdButton::autoInsertSpace() const { return autoInsertSpace_; }

void AdButton::setAutoInsertSpace(bool value) {
  if (autoInsertSpace_ == value) {
    return;
  }
  autoInsertSpace_ = value;
  refreshAfterPropertyChange();
  emit autoInsertSpaceChanged(autoInsertSpace_);
}

QIcon AdButton::loadingIcon() const { return loadingIcon_; }

void AdButton::setLoadingIcon(const QIcon& value) {
  loadingIcon_ = value;
  updateSpinnerState();
  refreshAfterPropertyChange(false);
  emit loadingIconChanged(loadingIcon_);
}

bool AdButton::isLoadingVisible() const { return loadingVisible_; }

void AdButton::resetSizeOverride() {
  if (!sizeExplicit_) {
    return;
  }
  sizeExplicit_ = false;
  refreshAfterPropertyChange();
}

void AdButton::paintEvent(QPaintEvent* event) {
  Q_UNUSED(event)

  detail::ButtonStyleInput input;
  input.type = type_;
  input.color = color_;
  input.variant = variant_;
  input.size = effectiveSize();
  input.colorExplicit = colorExplicit_;
  input.variantExplicit = variantExplicit_;
  input.danger = danger_;
  input.ghost = ghost_;
  input.baseFont = font();

  const detail::ButtonVisualStyle style = detail::resolveButtonVisualStyle(input);

  const bool enabled = isEnabled();
  const bool pressed = isDown();
  const bool hovered = hovered_ && enabled;

  detail::ButtonStateStyle state = style.normal;
  if (!enabled) {
    state = style.disabled;
  } else if (pressed) {
    state = style.active;
  } else if (hovered) {
    state = style.hover;
  }

  qreal radius = static_cast<qreal>(style.metrics.borderRadius);
  if (shape_ == Shape::Square) {
    radius = 0.0;
  } else if (shape_ == Shape::Round || shape_ == Shape::Circle) {
    radius = style.metrics.height / 2.0;
  }

  qreal topLeft = radius;
  qreal topRight = radius;
  qreal bottomRight = radius;
  qreal bottomLeft = radius;

  switch (groupPosition_) {
    case GroupPosition::First:
      topRight = 0.0;
      bottomRight = 0.0;
      break;
    case GroupPosition::Middle:
      topLeft = 0.0;
      topRight = 0.0;
      bottomRight = 0.0;
      bottomLeft = 0.0;
      break;
    case GroupPosition::Last:
      topLeft = 0.0;
      bottomLeft = 0.0;
      break;
    case GroupPosition::Only:
    case GroupPosition::None:
    default:
      break;
  }

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, true);

  const qreal borderHalf = style.metrics.borderWidth / 2.0;
  const QRectF buttonRect = rect().adjusted(borderHalf + 0.5, borderHalf + 0.5, -borderHalf - 0.5,
                                            -borderHalf - 0.5);
  const QPainterPath path = roundedRectPath(buttonRect, topLeft, topRight, bottomRight, bottomLeft);

  if (enabled && state.shadow.alpha() > 0 && !interactionBlocked()) {
    const qreal shadowOffsetY = std::max<qreal>(0.0, style.metrics.shadowOffsetY);
    QPainterPath shadowPath = roundedRectPath(buttonRect.translated(0.0, shadowOffsetY), topLeft,
                                              topRight, bottomRight, bottomLeft);
    painter.fillPath(shadowPath, state.shadow);
  }

  painter.fillPath(path, state.background);

  QPen borderPen(state.border, style.metrics.borderWidth, state.borderStyle, Qt::SquareCap,
                 Qt::MiterJoin);
  if (state.borderStyle == Qt::DashLine) {
    borderPen.setStyle(Qt::CustomDashLine);

    // Match CSS dashed border rhythm more closely than Qt's default DashLine.
    const qreal penWidth = std::max<qreal>(1.0, borderPen.widthF());
    borderPen.setDashPattern({3.0 / penWidth, 2.0 / penWidth});
    borderPen.setCapStyle(Qt::FlatCap);
    borderPen.setJoinStyle(Qt::RoundJoin);
  }
  painter.setPen(borderPen);
  painter.setBrush(Qt::NoBrush);
  painter.drawPath(path);

  if (hasFocus() && enabled && focusVisible_) {
    const qreal focusWidth = std::max<qreal>(1.0, style.metrics.focusOutlineWidth);
    const qreal focusOffset = std::max<qreal>(0.0, style.metrics.focusOutlineOffset);

    QRectF focusBaseRectInWindow = buttonRect;
    QWidget* hostWindow = window();
    if (hostWindow) {
      const QPoint origin = mapTo(hostWindow, QPoint(0, 0));
      focusBaseRectInWindow = buttonRect.translated(origin.x(), origin.y());
    }

    InteractionFocusRequest request;
    request.owner = this;
    request.baseRectInWindow = focusBaseRectInWindow;
    request.topLeft = topLeft;
    request.topRight = topRight;
    request.bottomRight = bottomRight;
    request.bottomLeft = bottomLeft;
    request.color = style.metrics.focusOutline;
    request.strokeWidth = focusWidth;
    request.offset = focusOffset;
    triggerInteractionFocus(request);
  } else {
    stopInteractionFocusForOwner(this);
  }

  painter.setFont(style.metrics.font);
  QFontMetrics fm(style.metrics.font);

  const QString textToRender = renderText();
  const bool twoCnAutoSpacing = shouldApplyTwoCnSpacing(textToRender);
  const bool spinnerOnly = loadingVisible_ && loadingIcon_.isNull();
  const QIcon iconToRender = loadingVisible_ ? loadingIcon_ : icon();
  const bool hasIcon = spinnerOnly || !iconToRender.isNull();
  const bool iconOnly = hasIcon && textToRender.isEmpty();

  int iconSide = resolveIconSide(this, fm, style.metrics.font);

  if ((shape_ == Shape::Circle || iconOnly) && groupPosition_ == GroupPosition::None) {
    iconSide = std::min(iconSide, std::max(8, height() - 8));
  }

  const int horizontalPadding = (iconOnly || shape_ == Shape::Circle) ? 0 : style.metrics.horizontalPadding;

  QRect contentRect = rect().adjusted(horizontalPadding + style.metrics.borderWidth,
                                      style.metrics.borderWidth,
                                      -(horizontalPadding + style.metrics.borderWidth),
                                      -style.metrics.borderWidth);
  if (contentRect.width() < 0 || contentRect.height() < 0) {
    return;
  }

  const QSize layoutIconSize = hasIcon ? QSize(iconSide, iconSide) : QSize();
  const ContentLayout layout = computeContentLayout(contentRect, layoutIconSize, textToRender, fm,
                                                    style.metrics.iconGap, style.metrics.font,
                                                    twoCnAutoSpacing);

  QColor contentColor = state.text;
  if (loadingVisible_) {
    contentColor.setAlphaF(contentColor.alphaF() * 0.72);
  }

  painter.setPen(contentColor);

  if (layout.hasIcon) {
    if (spinnerOnly) {
      drawSpinner(painter, layout.iconRect, contentColor);
    } else {
      const QIcon::Mode mode = enabled ? QIcon::Normal : QIcon::Disabled;
      const QPixmap pixmap = iconToRender.pixmap(layout.iconRect.size(), mode, QIcon::Off);
      painter.drawPixmap(layout.iconRect, pixmap);
    }
  }

  if (layout.hasText) {
    if (twoCnAutoSpacing && isTwoChineseCharacters(layout.text)) {
      const int spacingPx = twoCnLetterSpacingPx(fm, style.metrics.font);
      const int firstWidth = measureTextWidth(fm, QString(layout.text.at(0)));
      const int baseline = layout.textRect.top() + fm.ascent();

      painter.drawText(layout.textRect.left(), baseline, QString(layout.text.at(0)));
      painter.drawText(layout.textRect.left() + firstWidth + spacingPx, baseline,
                       QString(layout.text.at(1)));
    } else {
      painter.drawText(layout.textRect, Qt::AlignLeft | Qt::AlignVCenter, layout.text);
    }
  }
}

QSize AdButton::sizeHint() const {
  detail::ButtonStyleInput input;
  input.type = type_;
  input.color = color_;
  input.variant = variant_;
  input.size = effectiveSize();
  input.colorExplicit = colorExplicit_;
  input.variantExplicit = variantExplicit_;
  input.danger = danger_;
  input.ghost = ghost_;
  input.baseFont = font();

  const detail::ButtonVisualStyle style = detail::resolveButtonVisualStyle(input);
  QFontMetrics fm(style.metrics.font);

  const QString textToMeasure = renderText();
  const bool twoCnAutoSpacing = shouldApplyTwoCnSpacing(textToMeasure);
  const bool hasIcon = loadingVisible_ || hasUserIcon();
  const bool iconOnly = hasIcon && textToMeasure.isEmpty();

  const int iconSide = resolveIconSide(this, fm, style.metrics.font);

  const int textWidth = textToMeasure.isEmpty()
                            ? 0
                            : measureDisplayTextWidth(fm, style.metrics.font, textToMeasure,
                                                      twoCnAutoSpacing);
  const int horizontalPadding = (iconOnly || shape_ == Shape::Circle) ? 0 : style.metrics.horizontalPadding;

  int width = horizontalPadding * 2 + style.metrics.borderWidth * 2 + textWidth;
  if (hasIcon) {
    width += iconSide;
  }
  if (hasIcon && !textToMeasure.isEmpty()) {
    width += style.metrics.iconGap;
  }

  int height = style.metrics.height;

  if (iconOnly) {
    width = height;
  } else if (shape_ == Shape::Circle) {
    width = std::max(width, height);
  }

  return QSize(width, height);
}

QSize AdButton::minimumSizeHint() const { return sizeHint(); }

void AdButton::changeEvent(QEvent* event) {
  QPushButton::changeEvent(event);
  if (!event) {
    return;
  }

  switch (event->type()) {
    case QEvent::EnabledChange:
      if (!isEnabled()) {
        stopWaveEffect();
        stopInteractionFocusForOwner(this);
      }
      updateCursorForRole();
      update();
      break;
    case QEvent::FontChange:
      refreshAfterPropertyChange();
      break;
    default:
      break;
  }
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void AdButton::enterEvent(QEnterEvent* event) {
  QPushButton::enterEvent(event);
#else
void AdButton::enterEvent(QEvent* event) {
  QPushButton::enterEvent(event);
#endif
  hovered_ = true;
  bumpGroupZOrder();
  update();
}

void AdButton::leaveEvent(QEvent* event) {
  QPushButton::leaveEvent(event);
  hovered_ = false;
  update();
}

void AdButton::mousePressEvent(QMouseEvent* event) {
  if (interactionBlocked()) {
    event->ignore();
    return;
  }
  focusVisible_ = false;
  QPushButton::mousePressEvent(event);
  bumpGroupZOrder();
}

void AdButton::mouseReleaseEvent(QMouseEvent* event) {
  if (interactionBlocked()) {
    event->ignore();
    return;
  }

  const bool shouldStartWave =
      event && event->button() == Qt::LeftButton && rect().contains(mouseEventPos(event));
  QPushButton::mouseReleaseEvent(event);
  if (shouldStartWave && isEnabled() && !interactionBlocked()) {
    startWaveEffect();
  }
}

void AdButton::keyPressEvent(QKeyEvent* event) {
  if (interactionBlocked() &&
      (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return ||
       event->key() == Qt::Key_Enter)) {
    event->ignore();
    return;
  }
  QPushButton::keyPressEvent(event);
}

void AdButton::keyReleaseEvent(QKeyEvent* event) {
  if (interactionBlocked() &&
      (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return ||
       event->key() == Qt::Key_Enter)) {
    event->ignore();
    return;
  }
  QPushButton::keyReleaseEvent(event);
  if (!event->isAutoRepeat() &&
      (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return ||
       event->key() == Qt::Key_Enter) &&
      isEnabled() && !interactionBlocked()) {
    startWaveEffect();
  }
}

void AdButton::focusInEvent(QFocusEvent* event) {
  QPushButton::focusInEvent(event);
  focusVisible_ = event && isKeyboardFocusReason(event->reason());
  update();
}

void AdButton::focusOutEvent(QFocusEvent* event) {
  QPushButton::focusOutEvent(event);
  focusVisible_ = false;
  stopInteractionFocusForOwner(this);
  update();
}

void AdButton::moveEvent(QMoveEvent* event) {
  QPushButton::moveEvent(event);
  if (hasFocus() && isEnabled() && focusVisible_) {
    update();
  } else {
    stopInteractionFocusForOwner(this);
  }
}

void AdButton::resizeEvent(QResizeEvent* event) {
  QPushButton::resizeEvent(event);
  if (hasFocus() && isEnabled() && focusVisible_) {
    update();
  } else {
    stopInteractionFocusForOwner(this);
  }
}

void AdButton::showEvent(QShowEvent* event) {
  QPushButton::showEvent(event);
  if (hasFocus() && isEnabled() && focusVisible_) {
    update();
  }
}

void AdButton::hideEvent(QHideEvent* event) {
  QPushButton::hideEvent(event);
  stopInteractionFocusForOwner(this);
}

bool AdButton::interactionBlocked() const { return loadingVisible_; }

bool AdButton::hasUserIcon() const { return !icon().isNull(); }

bool AdButton::shouldApplyTwoCnSpacing(const QString& sourceText) const {
  if (!autoInsertSpace_ || sourceText.isEmpty()) {
    return false;
  }

  detail::ButtonStyleInput input;
  input.type = type_;
  input.color = color_;
  input.variant = variant_;
  input.size = effectiveSize();
  input.colorExplicit = colorExplicit_;
  input.variantExplicit = variantExplicit_;
  input.danger = danger_;
  input.ghost = ghost_;
  input.baseFont = font();

  const detail::ResolvedRole role = detail::resolveRole(input);
  if (role.unbordered || hasUserIcon()) {
    return false;
  }

  return isTwoChineseCharacters(sourceText);
}

QString AdButton::renderText() const {
  return text();
}

void AdButton::refreshAfterPropertyChange(bool updateGeometryHint) {
  detail::ButtonStyleInput input;
  input.type = type_;
  input.color = color_;
  input.variant = variant_;
  input.size = effectiveSize();
  input.colorExplicit = colorExplicit_;
  input.variantExplicit = variantExplicit_;
  input.danger = danger_;
  input.ghost = ghost_;
  input.baseFont = font();

  const detail::ButtonVisualStyle style = detail::resolveButtonVisualStyle(input);
  setMinimumHeight(style.metrics.height);
  updateSpinnerState();
  updateCursorForRole();

  if (updateGeometryHint) {
    updateGeometry();
  }
  update();
}

void AdButton::updateLoadingVisualState() {
  loadingDelayTimer_.stop();

  if (!loading_) {
    loadingVisible_ = false;
  } else if (loadingDelay_ <= 0) {
    loadingVisible_ = true;
  } else {
    loadingVisible_ = false;
    loadingDelayTimer_.start(loadingDelay_);
  }

  updateSpinnerState();
  if (interactionBlocked()) {
    stopWaveEffect();
  }
  refreshAfterPropertyChange();
}

void AdButton::updateSpinnerState() {
  const bool spinning = loadingVisible_ && loadingIcon_.isNull();
  if (spinning && !spinnerSubscribed_) {
    subscribeSharedSpinner(this);
    spinnerSubscribed_ = true;
  } else if (!spinning && spinnerSubscribed_) {
    unsubscribeSharedSpinner(this);
    spinnerSubscribed_ = false;
  }
}

void AdButton::applyBlockSizePolicy() {
  if (block_) {
    QSizePolicy policy = sizePolicy();
    policy.setHorizontalPolicy(QSizePolicy::Expanding);
    setSizePolicy(policy);
  } else {
    setSizePolicy(baseSizePolicy_);
  }
}

void AdButton::bumpGroupZOrder() {
  if (groupPosition_ != GroupPosition::None) {
    raise();
  }
}

void AdButton::updateCursorForRole() {
  if (!isEnabled() || interactionBlocked()) {
    unsetCursor();
    return;
  }
  setCursor(Qt::PointingHandCursor);
}

void AdButton::startWaveEffect() {
  if (!isEnabled() || interactionBlocked()) {
    return;
  }
  if (type_ == Type::Text) {
    return;
  }

  detail::ButtonStyleInput input;
  input.type = type_;
  input.color = color_;
  input.variant = variant_;
  input.size = effectiveSize();
  input.colorExplicit = colorExplicit_;
  input.variantExplicit = variantExplicit_;
  input.danger = danger_;
  input.ghost = ghost_;
  input.baseFont = font();

  const detail::ButtonVisualStyle style = detail::resolveButtonVisualStyle(input);
  detail::ButtonStateStyle state = style.normal;
  if (isDown()) {
    state = style.active;
  } else if (hovered_) {
    state = style.hover;
  }

  qreal radius = static_cast<qreal>(style.metrics.borderRadius);
  if (shape_ == Shape::Square) {
    radius = 0.0;
  } else if (shape_ == Shape::Round || shape_ == Shape::Circle) {
    radius = style.metrics.height / 2.0;
  }

  qreal waveTopLeft = radius;
  qreal waveTopRight = radius;
  qreal waveBottomRight = radius;
  qreal waveBottomLeft = radius;

  switch (groupPosition_) {
    case GroupPosition::First:
      waveTopRight = 0.0;
      waveBottomRight = 0.0;
      break;
    case GroupPosition::Middle:
      waveTopLeft = 0.0;
      waveTopRight = 0.0;
      waveBottomRight = 0.0;
      waveBottomLeft = 0.0;
      break;
    case GroupPosition::Last:
      waveTopLeft = 0.0;
      waveBottomLeft = 0.0;
      break;
    case GroupPosition::Only:
    case GroupPosition::None:
    default:
      break;
  }

  const qreal borderHalf = style.metrics.borderWidth / 2.0;
  const QRectF buttonRect =
      rect().adjusted(borderHalf + 0.5, borderHalf + 0.5, -borderHalf - 0.5, -borderHalf - 0.5);
  QRectF waveBaseRectInWindow = buttonRect;
  QWidget* hostWindow = window();
  if (hostWindow) {
    const QPoint origin = mapTo(hostWindow, QPoint(0, 0));
    waveBaseRectInWindow = buttonRect.translated(origin.x(), origin.y());
  }

  QColor waveColor;
  if (isValidWaveColor(state.border)) {
    waveColor = state.border;
  } else if (isValidWaveColor(state.background)) {
    waveColor = state.background;
  } else {
    return;
  }

  InteractionWaveRequest request;
  request.owner = this;
  request.baseRectInWindow = waveBaseRectInWindow;
  request.topLeft = waveTopLeft;
  request.topRight = waveTopRight;
  request.bottomRight = waveBottomRight;
  request.bottomLeft = waveBottomLeft;
  request.color = waveColor;
  triggerInteractionWave(request);
}

void AdButton::stopWaveEffect() { stopInteractionWaveForOwner(this); }

AdButton::Size AdButton::effectiveSize() const {
  if (usesExplicitSize()) {
    return size_;
  }
  if (hasGroupSizeContext_) {
    return groupSizeContext_;
  }
  return size_;
}

bool AdButton::usesExplicitSize() const { return sizeExplicit_; }

void AdButton::setGroupPosition(GroupPosition position) {
  if (groupPosition_ == position) {
    return;
  }
  groupPosition_ = position;
  update();
}

void AdButton::setGroupSizeContext(Size size, bool enabled) {
  const bool changed = hasGroupSizeContext_ != enabled || groupSizeContext_ != size;
  hasGroupSizeContext_ = enabled;
  groupSizeContext_ = size;
  if (!changed) {
    return;
  }
  if (!usesExplicitSize()) {
    refreshAfterPropertyChange();
  } else {
    update();
  }
}

AdButton::ContentLayout AdButton::computeContentLayout(const QRect& contentRect, const QSize& iconSize,
                                                       const QString& displayText,
                                                       const QFontMetrics& fm,
                                                       int iconGap,
                                                       const QFont& contentFont,
                                                       bool twoCnAutoSpacing) const {
  ContentLayout layout;
  layout.hasText = !displayText.isEmpty();
  layout.hasIcon = iconSize.isValid() && !iconSize.isEmpty();

  const int gap = (layout.hasIcon && layout.hasText) ? iconGap : 0;
  const int availableTextWidth =
      std::max(0, contentRect.width() - (layout.hasIcon ? iconSize.width() + gap : 0));
  if (layout.hasText) {
    const int fullTextWidth =
        measureDisplayTextWidth(fm, contentFont, displayText, twoCnAutoSpacing);
    if (fullTextWidth <= availableTextWidth) {
      layout.text = displayText;
    } else {
      layout.text = fm.elidedText(displayText, Qt::ElideRight, availableTextWidth);
    }
  }
  layout.hasText = !layout.text.isEmpty();

  const int textWidth = layout.hasText
                            ? measureDisplayTextWidth(fm, contentFont, layout.text,
                                                      twoCnAutoSpacing)
                            : 0;
  const int textHeight = layout.hasText ? fm.height() : 0;

  int contentWidth = 0;
  if (layout.hasIcon) {
    contentWidth += iconSize.width();
  }
  if (layout.hasIcon && layout.hasText) {
    contentWidth += gap;
  }
  if (layout.hasText) {
    contentWidth += textWidth;
  }

  int startX = contentRect.left();
  if (contentRect.width() > contentWidth) {
    startX = contentRect.left() + (contentRect.width() - contentWidth) / 2;
  }

  if (!layout.hasIcon && !layout.hasText) {
    return layout;
  }

  if (layout.hasIcon && layout.hasText) {
    const int iconY = contentRect.top() + (contentRect.height() - iconSize.height()) / 2;
    const int textY = contentRect.top() + (contentRect.height() - textHeight) / 2;
    if (iconPlacement_ == IconPlacement::Start) {
      layout.iconRect = QRect(startX, iconY, iconSize.width(), iconSize.height());
      layout.textRect = QRect(layout.iconRect.right() + 1 + gap, textY, textWidth, textHeight);
    } else {
      layout.textRect = QRect(startX, textY, textWidth, textHeight);
      layout.iconRect =
          QRect(layout.textRect.right() + 1 + gap, iconY, iconSize.width(), iconSize.height());
    }
    return layout;
  }

  if (layout.hasIcon) {
    const int iconX = contentRect.left() + (contentRect.width() - iconSize.width()) / 2;
    const int iconY = contentRect.top() + (contentRect.height() - iconSize.height()) / 2;
    layout.iconRect = QRect(iconX, iconY, iconSize.width(), iconSize.height());
  }

  if (layout.hasText) {
    const int textX = contentRect.left() + (contentRect.width() - textWidth) / 2;
    const int textY = contentRect.top() + (contentRect.height() - textHeight) / 2;
    layout.textRect = QRect(textX, textY, textWidth, textHeight);
  }

  return layout;
}

void AdButton::drawSpinner(QPainter& painter, const QRect& iconRect, const QColor& color) const {
  const int side = std::max(8, std::min(iconRect.width(), iconRect.height()) - 2);
  const QRectF spinnerRect(iconRect.center().x() - side / 2.0, iconRect.center().y() - side / 2.0, side, side);

  QPen pen(color, 1.8, Qt::SolidLine, Qt::RoundCap);
  painter.setPen(pen);
  painter.setBrush(Qt::NoBrush);
  painter.drawArc(spinnerRect, (90 - sharedSpinnerAngle()) * 16, -270 * 16);
}

}  // namespace adqt::widgets
