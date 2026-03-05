#include "switch.h"

#include "detail/timing_hub.h"
#include "generated/icon_manifest.h"
#include "icons.h"
#include "interaction_overlay_manager.h"
#include "switch_style.h"
#include "theme/theme.h"

#include <QEnterEvent>
#include <QEvent>
#include <QFocusEvent>
#include <QFontMetrics>
#include <QHideEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QMoveEvent>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QShowEvent>

#include <algorithm>
#include <cmath>

namespace adqt::widgets {

namespace {

constexpr char kThumbFrameKey[] = "AdSwitch.ThumbFrame";
constexpr char kPressStateFrameKey[] = "AdSwitch.PressStateFrame";
constexpr char kSpinnerFrameKey[] = "AdSwitch.SpinnerFrame";

bool iconStylesEqual(const adqt::icons::IconStyle& lhs, const adqt::icons::IconStyle& rhs) {
  return lhs.hasPrimary == rhs.hasPrimary && lhs.hasSecondary == rhs.hasSecondary &&
         lhs.hasTertiary == rhs.hasTertiary && lhs.primary == rhs.primary &&
         lhs.secondary == rhs.secondary && lhs.tertiary == rhs.tertiary;
}

bool iconTokensEqual(const adqt::icons::IconToken& lhs, const adqt::icons::IconToken& rhs) {
  return lhs.index == rhs.index && iconStylesEqual(lhs.style, rhs.style);
}

bool isKeyboardFocusReason(Qt::FocusReason reason) {
  return reason != Qt::MouseFocusReason && reason != Qt::NoFocusReason;
}

qreal clamp01(qreal value) { return std::clamp(value, 0.0, 1.0); }

qreal cubicBezierCoordinate(qreal p1, qreal p2, qreal t) {
  const qreal oneMinusT = 1.0 - t;
  return 3.0 * oneMinusT * oneMinusT * t * p1 + 3.0 * oneMinusT * t * t * p2 + t * t * t;
}

qreal cubicBezierSlope(qreal p1, qreal p2, qreal t) {
  const qreal oneMinusT = 1.0 - t;
  return 3.0 * oneMinusT * oneMinusT * p1 + 6.0 * oneMinusT * t * (p2 - p1) +
         3.0 * t * t * (1.0 - p2);
}

qreal cubicBezierEase(qreal progress, qreal x1, qreal y1, qreal x2, qreal y2) {
  const qreal targetX = clamp01(progress);
  qreal curveT = targetX;

  for (int iteration = 0; iteration < 6; ++iteration) {
    const qreal xEstimate = cubicBezierCoordinate(x1, x2, curveT) - targetX;
    const qreal slope = cubicBezierSlope(x1, x2, curveT);
    if (std::abs(slope) < 1e-6) {
      break;
    }
    curveT = clamp01(curveT - xEstimate / slope);
  }

  qreal lower = 0.0;
  qreal upper = 1.0;
  for (int iteration = 0; iteration < 10; ++iteration) {
    const qreal xEstimate = cubicBezierCoordinate(x1, x2, curveT);
    if (std::abs(xEstimate - targetX) < 1e-5) {
      break;
    }
    if (xEstimate < targetX) {
      lower = curveT;
    } else {
      upper = curveT;
    }
    curveT = (lower + upper) / 2.0;
  }

  return cubicBezierCoordinate(y1, y2, curveT);
}

qreal easeInOut(qreal t) {
  // Match CSS `ease-in-out` used by Ant Design switch handle transitions.
  return cubicBezierEase(t, 0.42, 0.0, 0.58, 1.0);
}

QColor blendColor(const QColor& from, const QColor& to, qreal t) {
  const qreal x = clamp01(t);
  return QColor::fromRgbF(from.redF() + (to.redF() - from.redF()) * x,
                          from.greenF() + (to.greenF() - from.greenF()) * x,
                          from.blueF() + (to.blueF() - from.blueF()) * x,
                          from.alphaF() + (to.alphaF() - from.alphaF()) * x);
}

QPainterPath roundedRectPath(const QRectF& rect,
                             qreal topLeft,
                             qreal topRight,
                             qreal bottomRight,
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
    path.arcTo(QRectF(right - 2.0 * bottomRight,
                      bottom - 2.0 * bottomRight,
                      2.0 * bottomRight,
                      2.0 * bottomRight),
               0.0,
               -90.0);
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

bool shouldInheritCurrentColor(const adqt::icons::IconToken& icon) {
  if (!adqt::icons::isValid(icon)) {
    return false;
  }
  if (icon.style.hasPrimary || icon.style.hasSecondary || icon.style.hasTertiary) {
    return false;
  }
  const adqt::icons::detail::IconEntry& entry = adqt::icons::detail::iconEntryAt(icon.index);
  return entry.theme != adqt::icons::IconTheme::TwoTone;
}

int sharedSpinnerAngle() {
  const int cycleMs = detail::spinnerCycleDurationMs();
  if (cycleMs <= 0) {
    return 0;
  }
  qint64 phaseMs = detail::timingNowMs() % cycleMs;
  if (phaseMs < 0) {
    phaseMs += cycleMs;
  }
  return static_cast<int>((phaseMs * 360) / cycleMs);
}

}  // namespace

AdSwitch::AdSwitch(QWidget* parent) : QAbstractButton(parent) {
  setCheckable(true);
  setFocusPolicy(Qt::StrongFocus);
  setAttribute(Qt::WA_Hover, true);

  connect(this, &QAbstractButton::toggled, this, [this](bool checked) {
    emit checkedChanged(checked);
    emit valueChanged(checked);
    emit changed(checked);
    updateThumbAnimationTarget(false);
    updateCursorForState();
    updateInteractionFocusOverlay();
    update();
  });

  connect(&adqt::theme::ThemeManager::instance(), &adqt::theme::ThemeManager::themeChanged, this,
          [this]() { refreshAfterPropertyChange(); });

  thumbPosition_ = isChecked() ? 1.0 : 0.0;
  thumbTargetPosition_ = thumbPosition_;
  pressStateProgress_ = 0.0;
  pressStateTargetProgress_ = 0.0;
  pressStateDirection_ = isChecked() ? -1.0 : 1.0;
  updateCursorForState();
}

AdSwitch::~AdSwitch() {
  stopInteractionWaveForOwner(this);
  stopInteractionFocusForOwner(this);
  stopAnimations();
}

bool AdSwitch::value() const { return isChecked(); }

void AdSwitch::setValue(bool value) { setChecked(value); }

bool AdSwitch::disabled() const { return !isEnabled(); }

void AdSwitch::setDisabled(bool value) {
  if (disabled() == value) {
    return;
  }
  setEnabled(!value);
  if (value) {
    setPressedState(false);
    stopInteractionWaveForOwner(this);
    stopInteractionFocusForOwner(this);
  }
  refreshAfterPropertyChange(false);
  emit disabledChanged(value);
}

bool AdSwitch::loading() const { return loading_; }

void AdSwitch::setLoading(bool value) {
  if (loading_ == value) {
    return;
  }
  loading_ = value;
  if (loading_) {
    setPressedState(false);
  }
  updateLoadingSpinnerState();
  updateCursorForState();
  updateInteractionFocusOverlay();
  refreshAfterPropertyChange(false);
  emit loadingChanged(loading_);
}

AdSwitch::Size AdSwitch::size() const { return size_; }

void AdSwitch::setSize(Size value) {
  if (size_ == value) {
    return;
  }
  size_ = value;
  refreshAfterPropertyChange();
  emit sizeChanged(size_);
}

QString AdSwitch::checkedChildren() const { return checkedChildren_; }

void AdSwitch::setCheckedChildren(const QString& value) {
  if (checkedChildren_ == value) {
    return;
  }
  checkedChildren_ = value;
  refreshAfterPropertyChange();
  emit checkedChildrenChanged(checkedChildren_);
}

QString AdSwitch::unCheckedChildren() const { return unCheckedChildren_; }

void AdSwitch::setUnCheckedChildren(const QString& value) {
  if (unCheckedChildren_ == value) {
    return;
  }
  unCheckedChildren_ = value;
  refreshAfterPropertyChange();
  emit unCheckedChildrenChanged(unCheckedChildren_);
}

adqt::icons::IconToken AdSwitch::checkedChildrenIconToken() const { return checkedChildrenIconToken_; }

void AdSwitch::setCheckedChildrenIconToken(const adqt::icons::IconToken& value) {
  if (iconTokensEqual(checkedChildrenIconToken_, value)) {
    return;
  }
  checkedChildrenIconToken_ = value;
  refreshAfterPropertyChange();
  emit checkedChildrenIconTokenChanged(checkedChildrenIconToken_);
}

adqt::icons::IconToken AdSwitch::unCheckedChildrenIconToken() const {
  return unCheckedChildrenIconToken_;
}

void AdSwitch::setUnCheckedChildrenIconToken(const adqt::icons::IconToken& value) {
  if (iconTokensEqual(unCheckedChildrenIconToken_, value)) {
    return;
  }
  unCheckedChildrenIconToken_ = value;
  refreshAfterPropertyChange();
  emit unCheckedChildrenIconTokenChanged(unCheckedChildrenIconToken_);
}

AdSwitch::ComponentTokens AdSwitch::componentTokens() const { return componentTokens_; }

void AdSwitch::setComponentTokens(const ComponentTokens& tokens) {
  componentTokens_ = tokens;
  refreshAfterPropertyChange();
  emit componentTokensChanged();
}

void AdSwitch::resetComponentTokens() {
  componentTokens_ = {};
  refreshAfterPropertyChange();
  emit componentTokensChanged();
}

AdSwitch::SemanticStyles AdSwitch::semanticStyles() const { return semanticStyles_; }

void AdSwitch::setSemanticStyles(const SemanticStyles& styles) {
  semanticStyles_ = styles;
  refreshAfterPropertyChange(false);
  emit semanticStylesChanged();
}

void AdSwitch::setSemanticStyleResolver(SemanticStyleResolver resolver) {
  semanticStyleResolver_ = std::move(resolver);
  refreshAfterPropertyChange(false);
  emit semanticStylesChanged();
}

QSize AdSwitch::sizeHint() const {
  detail::SwitchStyleInput input;
  input.size = size_;
  input.checked = isChecked();
  input.loading = loading_;
  input.disabled = effectiveDisabled();
  input.hovered = hovered_;
  input.pressed = pressed_;
  input.focused = hasFocus() && focusVisible_;
  input.componentTokens = componentTokens_;
  input.semanticStyles = resolvedSemanticStyles();
  const detail::SwitchVisualStyle style = detail::resolveSwitchVisualStyle(input);

  const bool small = size_ == Size::Small;
  const int trackHeight = small ? style.metrics.trackHeightSM : style.metrics.trackHeight;
  const int handleSize = small ? style.metrics.handleSizeSM : style.metrics.handleSize;
  const int height = std::max(trackHeight, handleSize);
  const int minWidth = small ? style.metrics.trackMinWidthSM : style.metrics.trackMinWidth;
  const int minTrackWidth = handleSize + style.metrics.trackPadding * 2;
  const int contentWidth = std::max(contentWidthForState(false), contentWidthForState(true));
  const int innerMinMargin = small ? style.metrics.innerMinMarginSM : style.metrics.innerMinMargin;
  const int innerMaxMargin = small ? style.metrics.innerMaxMarginSM : style.metrics.innerMaxMargin;
  const int width =
      std::max({minWidth, minTrackWidth, contentWidth + innerMinMargin + innerMaxMargin});
  return QSize(std::max(width, height), height);
}

QSize AdSwitch::minimumSizeHint() const { return sizeHint(); }

void AdSwitch::paintEvent(QPaintEvent* event) {
  Q_UNUSED(event)

  detail::SwitchStyleInput input;
  input.size = size_;
  input.checked = isChecked();
  input.loading = loading_;
  input.disabled = effectiveDisabled();
  input.hovered = hovered_;
  input.pressed = pressed_;
  input.focused = hasFocus() && focusVisible_;
  input.componentTokens = componentTokens_;
  input.semanticStyles = resolvedSemanticStyles();
  const detail::SwitchVisualStyle style = detail::resolveSwitchVisualStyle(input);

  const bool small = size_ == Size::Small;
  const int trackHeight = small ? style.metrics.trackHeightSM : style.metrics.trackHeight;
  const int handleSize = small ? style.metrics.handleSizeSM : style.metrics.handleSize;
  const int trackPadding = style.metrics.trackPadding;
  const int innerMinMargin = small ? style.metrics.innerMinMarginSM : style.metrics.innerMinMargin;
  const int innerMaxMargin = small ? style.metrics.innerMaxMarginSM : style.metrics.innerMaxMargin;
  const qreal activePressProgress = effectiveDisabled() ? 0.0 : pressStateProgress_;
  const int activeContentOffset =
      small ? style.metrics.innerContentActiveOffsetSM : style.metrics.innerContentActiveOffset;

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, true);
  if (effectiveDisabled()) {
    painter.setOpacity(style.metrics.disabledOpacity);
  }

  QRectF trackRect = QRectF(rect());
  if (trackRect.height() > trackHeight) {
    const qreal top = trackRect.top() + (trackRect.height() - trackHeight) / 2.0;
    trackRect.setTop(top);
    trackRect.setHeight(trackHeight);
  }
  trackRect = trackRect.adjusted(0.5, 0.5, -0.5, -0.5);

  const QColor offColor = hovered_ && !effectiveDisabled() ? style.trackHoverBg : style.trackBg;
  const QColor onColor =
      hovered_ && !effectiveDisabled() ? style.trackCheckedHoverBg : style.trackCheckedBg;
  const QColor trackColor = blendColor(offColor, onColor, thumbPosition_);

  const qreal radius = trackRect.height() / 2.0;
  const QPainterPath trackPath = roundedRectPath(trackRect, radius, radius, radius, radius);
  painter.fillPath(trackPath, trackColor);
  if (style.trackBorderColor.alpha() > 0) {
    painter.setPen(QPen(style.trackBorderColor, 1.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(trackPath);
  }

  auto drawContent = [&](bool checkedState, qreal alpha) {
    if (alpha <= 0.0) {
      return;
    }

    QString text = checkedState ? checkedChildren_ : unCheckedChildren_;
    adqt::icons::IconToken icon =
        checkedState ? checkedChildrenIconToken_ : unCheckedChildrenIconToken_;
    const bool hasIcon = adqt::icons::isValid(icon);
    const bool hasText = !text.trimmed().isEmpty();
    if (!hasIcon && !hasText) {
      return;
    }

    QFont contentFont = font();
    contentFont.setPixelSize(style.metrics.fontSize);
    painter.setFont(contentFont);
    const QFontMetrics fm(contentFont);
    const int iconSide = std::max(10, style.metrics.fontSize);
    const int textWidth = hasText ? std::max(0, fm.horizontalAdvance(text)) : 0;
    const int gap = hasIcon && hasText ? 4 : 0;
    const int contentWidth = (hasIcon ? iconSide : 0) + gap + textWidth;

    qreal x =
        checkedState ? trackRect.left() + innerMinMargin
                     : trackRect.right() - innerMinMargin - contentWidth;
    if (checkedState) {
      x += (1.0 - thumbPosition_) * (innerMaxMargin - innerMinMargin);
    } else {
      x -= thumbPosition_ * (innerMaxMargin - innerMinMargin);
    }
    if (activePressProgress > 0.0 && checkedState == isChecked()) {
      x += pressStateDirection_ * activeContentOffset * activePressProgress;
    }
    const qreal y = trackRect.top() + (trackRect.height() - std::max(iconSide, fm.height())) / 2.0;

    const qreal oldOpacity = painter.opacity();
    painter.setOpacity(oldOpacity * alpha);
    painter.setPen(style.contentColor);

    int cursorX = static_cast<int>(std::round(x));
    if (hasIcon) {
      adqt::icons::IconToken iconToRender = icon;
      if (shouldInheritCurrentColor(iconToRender)) {
        iconToRender.style.primary = style.contentColor;
        iconToRender.style.hasPrimary = true;
      }
      const QPixmap pixmap = adqt::icons::renderIconPixmap(
          iconToRender, QSize(iconSide, iconSide), devicePixelRatioF(), QIcon::Normal, QIcon::Off);
      if (!pixmap.isNull()) {
        painter.drawPixmap(cursorX, static_cast<int>(std::round(trackRect.center().y() - iconSide / 2.0)),
                           pixmap);
      }
      cursorX += iconSide + gap;
    }
    if (hasText) {
      const QRect textRect(cursorX,
                           static_cast<int>(std::round(y)),
                           textWidth,
                           std::max(iconSide, fm.height()));
      painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);
    }
    painter.setOpacity(oldOpacity);
  };

  drawContent(false, 1.0 - thumbPosition_);
  drawContent(true, thumbPosition_);

  const qreal handleLeftOff = trackRect.left() + trackPadding;
  const qreal handleLeftOn = trackRect.right() - trackPadding - handleSize;
  const qreal handleLeft = handleLeftOff + (handleLeftOn - handleLeftOff) * thumbPosition_;

  QRectF handleRect(handleLeft,
                    trackRect.top() + (trackRect.height() - handleSize) / 2.0,
                    handleSize,
                    handleSize);
  handleRect = handleRect.adjusted(0.5, 0.5, -0.5, -0.5);
  QRectF handleVisualRect = handleRect;
  if (activePressProgress > 0.0 && style.metrics.handleActiveInsetRatio > 0.0) {
    const qreal activeInset = handleSize * style.metrics.handleActiveInsetRatio * activePressProgress;
    if (pressStateDirection_ < 0.0) {
      handleVisualRect = handleVisualRect.adjusted(-activeInset, 0.0, 0.0, 0.0);
    } else {
      handleVisualRect = handleVisualRect.adjusted(0.0, 0.0, activeInset, 0.0);
    }
  }

  if (style.metrics.handleShadowColor.alpha() > 0) {
    QColor shadowColor = style.metrics.handleShadowColor;
    const qreal shadowOffset = style.metrics.handleShadowOffsetY;
    QRectF shadowRect = handleVisualRect.translated(0.0, shadowOffset);
    painter.setPen(Qt::NoPen);
    painter.setBrush(shadowColor);
    painter.drawEllipse(shadowRect);
  }

  if (style.handleBorderColor.alpha() > 0) {
    painter.setPen(QPen(style.handleBorderColor, 1.0));
  } else {
    painter.setPen(Qt::NoPen);
  }
  painter.setBrush(style.handleBg);
  painter.drawEllipse(handleVisualRect);

  if (loading_) {
    const QColor spinnerColor =
        thumbPosition_ >= 0.5 ? style.checkedLoadingIconColor : style.loadingIconColor;
    drawSpinner(&painter, handleRect, spinnerColor, style.metrics.loadingIconSize);
  }
}

void AdSwitch::nextCheckState() {
  if (effectiveDisabled()) {
    return;
  }
  QAbstractButton::nextCheckState();
}

void AdSwitch::enterEvent(QEnterEvent* event) {
  hovered_ = true;
  update();
  QAbstractButton::enterEvent(event);
}

void AdSwitch::leaveEvent(QEvent* event) {
  hovered_ = false;
  setPressedState(false);
  update();
  QAbstractButton::leaveEvent(event);
}

void AdSwitch::mousePressEvent(QMouseEvent* event) {
  if (event && event->button() == Qt::LeftButton && !effectiveDisabled()) {
    setPressedState(true);
    update();
  }
  QAbstractButton::mousePressEvent(event);
}

void AdSwitch::mouseReleaseEvent(QMouseEvent* event) {
  const bool triggerWave = event && event->button() == Qt::LeftButton && pressed_ &&
                           rect().contains(event->position().toPoint()) && !effectiveDisabled();
  setPressedState(false);
  QAbstractButton::mouseReleaseEvent(event);
  if (triggerWave) {
    triggerInteractionWaveOverlay();
  }
  update();
}

void AdSwitch::keyPressEvent(QKeyEvent* event) {
  if (!effectiveDisabled() && event &&
      (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return ||
       event->key() == Qt::Key_Enter)) {
    setPressedState(true);
    update();
  }
  QAbstractButton::keyPressEvent(event);
}

void AdSwitch::keyReleaseEvent(QKeyEvent* event) {
  const bool interactiveKey =
      event && (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return ||
                event->key() == Qt::Key_Enter);
  const bool triggerWave = !effectiveDisabled() && interactiveKey;
  setPressedState(false);
  QAbstractButton::keyReleaseEvent(event);
  if (triggerWave) {
    triggerInteractionWaveOverlay();
  }
  update();
}

void AdSwitch::focusInEvent(QFocusEvent* event) {
  focusVisible_ = event && isKeyboardFocusReason(event->reason());
  QAbstractButton::focusInEvent(event);
  updateInteractionFocusOverlay();
  update();
}

void AdSwitch::focusOutEvent(QFocusEvent* event) {
  focusVisible_ = false;
  QAbstractButton::focusOutEvent(event);
  stopInteractionFocusForOwner(this);
  update();
}

void AdSwitch::moveEvent(QMoveEvent* event) {
  QAbstractButton::moveEvent(event);
  updateInteractionFocusOverlay();
}

void AdSwitch::resizeEvent(QResizeEvent* event) {
  QAbstractButton::resizeEvent(event);
  updateInteractionFocusOverlay();
}

void AdSwitch::showEvent(QShowEvent* event) {
  QAbstractButton::showEvent(event);
  updateLoadingSpinnerState();
  updateThumbAnimationTarget(true);
  updatePressAnimationTarget(true);
  updateInteractionFocusOverlay();
}

void AdSwitch::hideEvent(QHideEvent* event) {
  QAbstractButton::hideEvent(event);
  stopInteractionFocusForOwner(this);
  stopInteractionWaveForOwner(this);
  stopAnimations();
}

void AdSwitch::changeEvent(QEvent* event) {
  QAbstractButton::changeEvent(event);
  if (!event) {
    return;
  }

  if (event->type() == QEvent::EnabledChange) {
    if (effectiveDisabled()) {
      setPressedState(false);
      stopInteractionFocusForOwner(this);
      stopInteractionWaveForOwner(this);
    }
    updateCursorForState();
    updateInteractionFocusOverlay();
    update();
  } else if (event->type() == QEvent::FontChange) {
    refreshAfterPropertyChange();
  }
}

bool AdSwitch::effectiveDisabled() const { return !isEnabled() || loading_; }

AdSwitch::SemanticStyles AdSwitch::resolvedSemanticStyles() const {
  SemanticStyles merged = semanticStyles_;
  if (!semanticStyleResolver_) {
    return merged;
  }

  StyleContext ctx;
  ctx.size = size_;
  ctx.checked = isChecked();
  ctx.loading = loading_;
  ctx.disabled = effectiveDisabled();
  ctx.hovered = hovered_;
  ctx.pressed = pressed_;
  ctx.focused = hasFocus() && focusVisible_;

  const SemanticStyles resolved = semanticStyleResolver_(ctx);
  auto mergeSlot = [](SemanticSlotStyle* target, const SemanticSlotStyle& source) {
    if (source.textColor.has_value()) {
      target->textColor = source.textColor;
    }
    if (source.backgroundColor.has_value()) {
      target->backgroundColor = source.backgroundColor;
    }
    if (source.borderColor.has_value()) {
      target->borderColor = source.borderColor;
    }
  };
  mergeSlot(&merged.root, resolved.root);
  mergeSlot(&merged.content, resolved.content);
  mergeSlot(&merged.indicator, resolved.indicator);
  return merged;
}

void AdSwitch::refreshAfterPropertyChange(bool updateGeometryHint) {
  if (updateGeometryHint) {
    QWidget::updateGeometry();
  }
  updateThumbAnimationTarget(true);
  updatePressAnimationTarget(true);
  updateLoadingSpinnerState();
  updateCursorForState();
  updateInteractionFocusOverlay();
  update();
}

void AdSwitch::updateCursorForState() {
  setCursor(effectiveDisabled() ? Qt::ForbiddenCursor : Qt::PointingHandCursor);
}

void AdSwitch::setPressedState(bool value, bool immediate) {
  if (pressed_ == value) {
    if (immediate) {
      updatePressAnimationTarget(true);
    }
    return;
  }
  if (value) {
    pressStateDirection_ = isChecked() ? -1.0 : 1.0;
  }
  pressed_ = value;
  updatePressAnimationTarget(immediate);
}

void AdSwitch::updateThumbAnimationTarget(bool immediate) {
  const qreal target = isChecked() ? 1.0 : 0.0;
  thumbTargetPosition_ = target;

  detail::SwitchStyleInput input;
  input.size = size_;
  input.checked = isChecked();
  input.loading = loading_;
  input.disabled = effectiveDisabled();
  input.hovered = hovered_;
  input.pressed = pressed_;
  input.focused = hasFocus() && focusVisible_;
  input.componentTokens = componentTokens_;
  input.semanticStyles = resolvedSemanticStyles();
  const detail::SwitchVisualStyle style = detail::resolveSwitchVisualStyle(input);

  if (immediate || style.metrics.animationDurationMs <= 0 || !isVisible()) {
    thumbPosition_ = target;
    thumbStartPosition_ = target;
    thumbAnimationStartMs_ = 0;
    if (thumbAnimationSubscribed_) {
      detail::clearFrameSubscription(this, QString::fromLatin1(kThumbFrameKey));
      thumbAnimationSubscribed_ = false;
    }
    return;
  }

  if (std::abs(thumbPosition_ - target) < 0.0001) {
    thumbPosition_ = target;
    if (thumbAnimationSubscribed_) {
      detail::clearFrameSubscription(this, QString::fromLatin1(kThumbFrameKey));
      thumbAnimationSubscribed_ = false;
    }
    return;
  }

  thumbStartPosition_ = thumbPosition_;
  thumbAnimationStartMs_ = detail::timingNowMs();
  if (!thumbAnimationSubscribed_) {
    detail::setFrameSubscription(this, QString::fromLatin1(kThumbFrameKey), true, [this](qint64, qint64) {
      updateThumbAnimationState();
    });
    thumbAnimationSubscribed_ = true;
  }
}

void AdSwitch::updateThumbAnimationState() {
  if (!thumbAnimationSubscribed_) {
    return;
  }

  detail::SwitchStyleInput input;
  input.size = size_;
  input.checked = isChecked();
  input.loading = loading_;
  input.disabled = effectiveDisabled();
  input.hovered = hovered_;
  input.pressed = pressed_;
  input.focused = hasFocus() && focusVisible_;
  input.componentTokens = componentTokens_;
  input.semanticStyles = resolvedSemanticStyles();
  const detail::SwitchVisualStyle style = detail::resolveSwitchVisualStyle(input);
  const int durationMs = style.metrics.animationDurationMs;

  if (durationMs <= 0) {
    thumbPosition_ = thumbTargetPosition_;
    detail::clearFrameSubscription(this, QString::fromLatin1(kThumbFrameKey));
    thumbAnimationSubscribed_ = false;
    update();
    return;
  }

  const qint64 elapsed = std::max<qint64>(0, detail::timingNowMs() - thumbAnimationStartMs_);
  const qreal progress = clamp01(static_cast<qreal>(elapsed) / durationMs);
  thumbPosition_ =
      thumbStartPosition_ + (thumbTargetPosition_ - thumbStartPosition_) * easeInOut(progress);
  update();

  if (progress >= 1.0) {
    thumbPosition_ = thumbTargetPosition_;
    detail::clearFrameSubscription(this, QString::fromLatin1(kThumbFrameKey));
    thumbAnimationSubscribed_ = false;
  }
}

void AdSwitch::updatePressAnimationTarget(bool immediate) {
  const qreal target = (pressed_ && !effectiveDisabled()) ? 1.0 : 0.0;
  pressStateTargetProgress_ = target;

  detail::SwitchStyleInput input;
  input.size = size_;
  input.checked = isChecked();
  input.loading = loading_;
  input.disabled = effectiveDisabled();
  input.hovered = hovered_;
  input.pressed = pressed_;
  input.focused = hasFocus() && focusVisible_;
  input.componentTokens = componentTokens_;
  input.semanticStyles = resolvedSemanticStyles();
  const detail::SwitchVisualStyle style = detail::resolveSwitchVisualStyle(input);

  if (immediate || style.metrics.animationDurationMs <= 0 || !isVisible()) {
    pressStateProgress_ = target;
    pressStateStartProgress_ = target;
    pressStateAnimationStartMs_ = 0;
    if (pressStateAnimationSubscribed_) {
      detail::clearFrameSubscription(this, QString::fromLatin1(kPressStateFrameKey));
      pressStateAnimationSubscribed_ = false;
    }
    return;
  }

  if (std::abs(pressStateProgress_ - target) < 0.0001) {
    pressStateProgress_ = target;
    if (pressStateAnimationSubscribed_) {
      detail::clearFrameSubscription(this, QString::fromLatin1(kPressStateFrameKey));
      pressStateAnimationSubscribed_ = false;
    }
    return;
  }

  pressStateStartProgress_ = pressStateProgress_;
  pressStateAnimationStartMs_ = detail::timingNowMs();
  if (!pressStateAnimationSubscribed_) {
    detail::setFrameSubscription(this, QString::fromLatin1(kPressStateFrameKey), true, [this](qint64, qint64) {
      updatePressAnimationState();
    });
    pressStateAnimationSubscribed_ = true;
  }
}

void AdSwitch::updatePressAnimationState() {
  if (!pressStateAnimationSubscribed_) {
    return;
  }

  detail::SwitchStyleInput input;
  input.size = size_;
  input.checked = isChecked();
  input.loading = loading_;
  input.disabled = effectiveDisabled();
  input.hovered = hovered_;
  input.pressed = pressed_;
  input.focused = hasFocus() && focusVisible_;
  input.componentTokens = componentTokens_;
  input.semanticStyles = resolvedSemanticStyles();
  const detail::SwitchVisualStyle style = detail::resolveSwitchVisualStyle(input);
  const int durationMs = style.metrics.animationDurationMs;

  if (durationMs <= 0) {
    pressStateProgress_ = pressStateTargetProgress_;
    detail::clearFrameSubscription(this, QString::fromLatin1(kPressStateFrameKey));
    pressStateAnimationSubscribed_ = false;
    update();
    return;
  }

  const qint64 elapsed = std::max<qint64>(0, detail::timingNowMs() - pressStateAnimationStartMs_);
  const qreal progress = clamp01(static_cast<qreal>(elapsed) / durationMs);
  pressStateProgress_ =
      pressStateStartProgress_ +
      (pressStateTargetProgress_ - pressStateStartProgress_) * easeInOut(progress);
  update();

  if (progress >= 1.0) {
    pressStateProgress_ = pressStateTargetProgress_;
    detail::clearFrameSubscription(this, QString::fromLatin1(kPressStateFrameKey));
    pressStateAnimationSubscribed_ = false;
  }
}

void AdSwitch::updateLoadingSpinnerState() {
  if (loading_ && isVisible()) {
    if (!spinnerSubscribed_) {
      detail::setFrameSubscription(this, QString::fromLatin1(kSpinnerFrameKey), true, [this](qint64, qint64) {
        if (!loading_) {
          return;
        }
        update();
      });
      spinnerSubscribed_ = true;
    }
    return;
  }

  if (spinnerSubscribed_) {
    detail::clearFrameSubscription(this, QString::fromLatin1(kSpinnerFrameKey));
    spinnerSubscribed_ = false;
  }
}

void AdSwitch::updateInteractionFocusOverlay() {
  if (!(hasFocus() && focusVisible_) || effectiveDisabled() || !isVisible()) {
    stopInteractionFocusForOwner(this);
    return;
  }

  detail::SwitchStyleInput input;
  input.size = size_;
  input.checked = isChecked();
  input.loading = loading_;
  input.disabled = effectiveDisabled();
  input.hovered = hovered_;
  input.pressed = pressed_;
  input.focused = hasFocus() && focusVisible_;
  input.componentTokens = componentTokens_;
  input.semanticStyles = resolvedSemanticStyles();
  const detail::SwitchVisualStyle style = detail::resolveSwitchVisualStyle(input);

  const bool small = size_ == Size::Small;
  const int trackHeight = small ? style.metrics.trackHeightSM : style.metrics.trackHeight;
  QRectF trackRect = QRectF(rect());
  if (trackRect.height() > trackHeight) {
    const qreal top = trackRect.top() + (trackRect.height() - trackHeight) / 2.0;
    trackRect.setTop(top);
    trackRect.setHeight(trackHeight);
  }

  QWidget* hostWindow = window();
  if (!hostWindow) {
    return;
  }

  InteractionFocusRequest request;
  request.owner = this;
  const QPoint origin = mapTo(hostWindow, QPoint(0, 0));
  request.baseRectInWindow = trackRect.translated(origin.x(), origin.y());
  const qreal radius = trackRect.height() / 2.0;
  request.topLeft = radius;
  request.topRight = radius;
  request.bottomRight = radius;
  request.bottomLeft = radius;
  request.color = style.metrics.focusOutlineColor;
  request.strokeWidth = style.metrics.focusOutlineWidth;
  request.offset = style.metrics.focusOutlineOffset;
  triggerInteractionFocus(request);
}

void AdSwitch::triggerInteractionWaveOverlay() {
  if (effectiveDisabled() || !isVisible()) {
    return;
  }

  detail::SwitchStyleInput input;
  input.size = size_;
  input.checked = isChecked();
  input.loading = loading_;
  input.disabled = effectiveDisabled();
  input.hovered = hovered_;
  input.pressed = pressed_;
  input.focused = hasFocus() && focusVisible_;
  input.componentTokens = componentTokens_;
  input.semanticStyles = resolvedSemanticStyles();
  const detail::SwitchVisualStyle style = detail::resolveSwitchVisualStyle(input);

  const bool small = size_ == Size::Small;
  const int trackHeight = small ? style.metrics.trackHeightSM : style.metrics.trackHeight;
  QRectF trackRect = QRectF(rect());
  if (trackRect.height() > trackHeight) {
    const qreal top = trackRect.top() + (trackRect.height() - trackHeight) / 2.0;
    trackRect.setTop(top);
    trackRect.setHeight(trackHeight);
  }

  QWidget* hostWindow = window();
  if (!hostWindow) {
    return;
  }

  InteractionWaveRequest request;
  request.owner = this;
  const QPoint origin = mapTo(hostWindow, QPoint(0, 0));
  request.baseRectInWindow = trackRect.translated(origin.x(), origin.y());
  const qreal radius = trackRect.height() / 2.0;
  request.topLeft = radius;
  request.topRight = radius;
  request.bottomRight = radius;
  request.bottomLeft = radius;
  request.color = style.waveColor;
  triggerInteractionWave(request);
}

void AdSwitch::stopAnimations() {
  if (thumbAnimationSubscribed_) {
    detail::clearFrameSubscription(this, QString::fromLatin1(kThumbFrameKey));
    thumbAnimationSubscribed_ = false;
  }
  if (pressStateAnimationSubscribed_) {
    detail::clearFrameSubscription(this, QString::fromLatin1(kPressStateFrameKey));
    pressStateAnimationSubscribed_ = false;
  }
  if (spinnerSubscribed_) {
    detail::clearFrameSubscription(this, QString::fromLatin1(kSpinnerFrameKey));
    spinnerSubscribed_ = false;
  }
}

void AdSwitch::drawSpinner(QPainter* painter,
                           const QRectF& rect,
                           const QColor& color,
                           qreal preferredSize) const {
  if (!painter || !rect.isValid()) {
    return;
  }

  const qreal side = std::clamp(preferredSize, 6.0, std::min(rect.width(), rect.height()));
  const QPointF center = rect.center();
  const QRectF spinnerRect(center.x() - side / 2.0, center.y() - side / 2.0, side, side);
  const qreal strokeWidth = std::clamp(side * 0.11, 1.0, 2.0);
  QPen pen(color, strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
  painter->setPen(pen);
  painter->setBrush(Qt::NoBrush);
  painter->drawArc(spinnerRect, (90 - sharedSpinnerAngle()) * 16, -270 * 16);
}

int AdSwitch::contentWidthForState(bool checkedState) const {
  QString text = checkedState ? checkedChildren_ : unCheckedChildren_;
  const adqt::icons::IconToken icon =
      checkedState ? checkedChildrenIconToken_ : unCheckedChildrenIconToken_;
  const bool hasIcon = adqt::icons::isValid(icon);
  const bool hasText = !text.trimmed().isEmpty();
  if (!hasIcon && !hasText) {
    return 0;
  }

  detail::SwitchStyleInput input;
  input.size = size_;
  input.checked = isChecked();
  input.loading = loading_;
  input.disabled = effectiveDisabled();
  input.hovered = hovered_;
  input.pressed = pressed_;
  input.focused = hasFocus() && focusVisible_;
  input.componentTokens = componentTokens_;
  input.semanticStyles = resolvedSemanticStyles();
  const detail::SwitchVisualStyle style = detail::resolveSwitchVisualStyle(input);

  QFont contentFont = font();
  contentFont.setPixelSize(style.metrics.fontSize);
  const QFontMetrics fm(contentFont);
  const int iconSide = std::max(10, style.metrics.fontSize);
  const int textWidth = hasText ? std::max(0, fm.horizontalAdvance(text)) : 0;
  const int gap = hasIcon && hasText ? 4 : 0;
  return (hasIcon ? iconSide : 0) + gap + textWidth;
}

}  // namespace adqt::widgets
