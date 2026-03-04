#include "radio.h"

#include "interaction_overlay_manager.h"
#include "radio_style.h"

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

namespace adqt::widgets {

namespace {

bool isKeyboardFocusReason(Qt::FocusReason reason) {
  return reason == Qt::TabFocusReason || reason == Qt::BacktabFocusReason ||
         reason == Qt::ShortcutFocusReason;
}

qreal snapToDevicePixelSize(qreal value, qreal dpr) {
  if (dpr <= 0.0) {
    return value;
  }
  const qreal snapped = qRound(value * dpr) / dpr;
  return std::max(snapped, 1.0 / dpr);
}

qreal snapToDevicePixelCoord(qreal value, qreal dpr) {
  if (dpr <= 0.0) {
    return value;
  }
  return qRound(value * dpr) / dpr;
}

QRectF snapRectToDevicePixels(const QRectF& rect, qreal dpr) {
  if (dpr <= 0.0) {
    return rect;
  }

  const qreal left = snapToDevicePixelCoord(rect.left(), dpr);
  const qreal top = snapToDevicePixelCoord(rect.top(), dpr);
  const qreal right = snapToDevicePixelCoord(rect.left() + rect.width(), dpr);
  const qreal bottom = snapToDevicePixelCoord(rect.top() + rect.height(), dpr);
  const qreal minSize = 1.0 / dpr;

  return QRectF(left,
                top,
                std::max(minSize, right - left),
                std::max(minSize, bottom - top));
}

QRectF centeredSquare(const QPointF& center, qreal size) {
  const qreal halfSize = size / 2.0;
  return QRectF(center.x() - halfSize, center.y() - halfSize, size, size);
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

QRectF defaultRadioIconRect(const QRectF& contentRect,
                            const detail::RadioVisualStyle& style,
                            int textWidth,
                            bool hasText,
                            bool block) {
  qreal totalWidth = static_cast<qreal>(style.metrics.radioSize);
  if (hasText) {
    totalWidth += static_cast<qreal>(style.metrics.labelPaddingInlineStart + textWidth +
                                     style.metrics.labelPaddingInlineEnd);
  }

  const qreal startX =
      block ? contentRect.x() + std::max<qreal>(0.0, (contentRect.width() - totalWidth) / 2.0)
            : contentRect.x();
  const qreal startY =
      contentRect.y() +
      std::max<qreal>(0.0, (contentRect.height() - static_cast<qreal>(style.metrics.radioSize)) / 2.0);

  return QRectF(startX,
                startY,
                static_cast<qreal>(style.metrics.radioSize),
                static_cast<qreal>(style.metrics.radioSize));
}

constexpr qreal kDefaultRadioWaveStrokeScale = 0.6;

}  // namespace

AdRadio::AdRadio(QWidget* parent) : QAbstractButton(parent) {
  setCheckable(true);
  setCursor(Qt::PointingHandCursor);
  setFocusPolicy(Qt::StrongFocus);
  setAttribute(Qt::WA_Hover, true);

  connect(this, &QAbstractButton::toggled, this, [this](bool checked) {
    emit checkedChanged(checked);
    emit changed(value_, checked);
    if (checked) {
      bumpButtonGroupZOrder();
    }
    updateInteractionFocusOverlay();
    update();
  });
}

AdRadio::AdRadio(const QString& text, QWidget* parent) : AdRadio(parent) { setText(text); }

AdRadio::~AdRadio() {
  stopWaveEffect();
  stopInteractionFocusForOwner(this);
}

bool AdRadio::disabled() const { return !isEnabled(); }

void AdRadio::setDisabled(bool value) {
  if (disabled() == value) {
    return;
  }
  setEnabled(!value);
  if (value) {
    stopWaveEffect();
    stopInteractionFocusForOwner(this);
  }
  refreshAfterPropertyChange(false);
  emit disabledChanged(value);
}

QVariant AdRadio::value() const { return value_; }

void AdRadio::setValue(const QVariant& value) {
  if (value_ == value) {
    return;
  }
  value_ = value;
  emit valueChanged(value_);
}

AdRadio::Size AdRadio::size() const { return size_; }

void AdRadio::setSize(Size value) {
  if (size_ == value) {
    return;
  }
  size_ = value;
  refreshAfterPropertyChange();
  emit sizeChanged(size_);
}

AdRadio::OptionType AdRadio::optionType() const { return optionType_; }

void AdRadio::setOptionType(OptionType value) {
  if (optionType_ == value) {
    return;
  }
  optionType_ = value;
  if (optionType_ == OptionType::Button && isChecked()) {
    bumpButtonGroupZOrder();
  }
  refreshAfterPropertyChange();
  emit optionTypeChanged(optionType_);
}

AdRadio::ButtonStyle AdRadio::buttonStyle() const { return buttonStyle_; }

void AdRadio::setButtonStyle(ButtonStyle value) {
  if (buttonStyle_ == value) {
    return;
  }
  buttonStyle_ = value;
  refreshAfterPropertyChange(false);
  emit buttonStyleChanged(buttonStyle_);
}

bool AdRadio::block() const { return block_; }

void AdRadio::setBlock(bool value) {
  if (block_ == value) {
    return;
  }
  block_ = value;
  refreshAfterPropertyChange();
  emit blockChanged(block_);
}

AdRadio::ComponentTokens AdRadio::componentTokens() const { return componentTokens_; }

void AdRadio::setComponentTokens(const ComponentTokens& tokens) {
  componentTokens_ = tokens;
  refreshAfterPropertyChange();
  emit componentTokensChanged();
}

void AdRadio::resetComponentTokens() {
  componentTokens_ = {};
  refreshAfterPropertyChange();
  emit componentTokensChanged();
}

AdRadio::SemanticStyles AdRadio::semanticStyles() const { return semanticStyles_; }

void AdRadio::setSemanticStyles(const SemanticStyles& styles) {
  semanticStyles_ = styles;
  refreshAfterPropertyChange(false);
  emit semanticStylesChanged();
}

void AdRadio::setSemanticStyleResolver(SemanticStyleResolver resolver) {
  semanticStyleResolver_ = std::move(resolver);
  refreshAfterPropertyChange(false);
  emit semanticStylesChanged();
}

QSize AdRadio::sizeHint() const {
  detail::RadioStyleInput input;
  input.size = size_;
  input.optionType = optionType_;
  input.buttonStyle = buttonStyle_;
  input.checked = isChecked();
  input.hovered = hovered_;
  input.pressed = pressed_;
  input.disabled = effectiveDisabled();
  input.focused = hasFocus() && focusVisible_;
  input.block = block_;
  input.baseFont = font();
  input.componentTokens = componentTokens_;
  input.semanticStyles = resolvedSemanticStyles();

  const detail::RadioVisualStyle style = detail::resolveRadioVisualStyle(input);
  const QFontMetrics fm(style.metrics.font);
  const int textW = textWidth(fm);
  const bool hasText = !text().isEmpty();

  if (optionType_ == OptionType::Button) {
    int width = textW + style.metrics.buttonPaddingInline * 2 + style.metrics.borderWidth * 2;
    width = std::max(width, 0);
    return QSize(width, style.metrics.buttonHeight);
  }

  int width = style.metrics.radioSize;
  if (hasText) {
    width += style.metrics.labelPaddingInlineStart + textW + style.metrics.labelPaddingInlineEnd;
  }
  const int height = std::max(style.metrics.radioSize, style.metrics.textLineHeight);
  return QSize(width, height);
}

QSize AdRadio::minimumSizeHint() const { return sizeHint(); }

void AdRadio::paintEvent(QPaintEvent* event) {
  Q_UNUSED(event)

  detail::RadioStyleInput input;
  input.size = size_;
  input.optionType = optionType_;
  input.buttonStyle = buttonStyle_;
  input.checked = isChecked();
  input.hovered = hovered_;
  input.pressed = pressed_;
  input.disabled = effectiveDisabled();
  input.focused = hasFocus() && focusVisible_;
  input.block = block_;
  input.baseFont = font();
  input.componentTokens = componentTokens_;
  input.semanticStyles = resolvedSemanticStyles();

  const detail::RadioVisualStyle style = detail::resolveRadioVisualStyle(input);

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.setFont(style.metrics.font);

  const bool disabledNow = effectiveDisabled();
  const bool checkedNow = isChecked();
  const QRectF contentRect = QRectF(rect());
  const qreal dpr = painter.device() ? painter.device()->devicePixelRatioF() : 1.0;

  if (optionType_ == OptionType::Button) {
    detail::RadioButtonStateStyle buttonState = style.buttonNormal;
    if (disabledNow) {
      buttonState = checkedNow ? style.buttonCheckedDisabled : style.buttonDisabled;
    } else if (checkedNow) {
      if (pressed_) {
        buttonState = style.buttonCheckedActive;
      } else if (hovered_) {
        buttonState = style.buttonCheckedHover;
      } else {
        buttonState = style.buttonChecked;
      }
    } else if (pressed_) {
      buttonState = style.buttonActive;
    } else if (hovered_) {
      buttonState = style.buttonHover;
    }

    qreal topLeft = 0.0;
    qreal topRight = 0.0;
    qreal bottomRight = 0.0;
    qreal bottomLeft = 0.0;
    resolveButtonCornerRadii(&topLeft, &topRight, &bottomRight, &bottomLeft);

    const qreal borderHalf = style.metrics.borderWidth / 2.0;
    const QRectF buttonRect = contentRect.adjusted(borderHalf + 0.5,
                                                   borderHalf + 0.5,
                                                   -borderHalf - 0.5,
                                                   -borderHalf - 0.5);
    const QPainterPath path =
        roundedRectPath(buttonRect, topLeft, topRight, bottomRight, bottomLeft);
    painter.fillPath(path, buttonState.backgroundColor);
    QPen borderPen(buttonState.borderColor,
                   style.metrics.borderWidth,
                   Qt::SolidLine,
                   Qt::SquareCap,
                   Qt::MiterJoin);
    painter.setPen(borderPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);

    painter.setPen(buttonState.textColor);
    painter.drawText(contentRect, Qt::AlignCenter, text());
    return;
  }

  detail::RadioDotStateStyle dotState = style.dotNormal;
  if (disabledNow) {
    dotState = checkedNow ? style.dotCheckedDisabled : style.dotDisabled;
  } else if (checkedNow) {
    if (hovered_ || pressed_) {
      dotState = style.dotCheckedHover;
    } else {
      dotState = style.dotChecked;
    }
  } else if (pressed_) {
    dotState = style.dotActive;
  } else if (hovered_) {
    dotState = style.dotHover;
  }

  const bool hasText = !text().isEmpty();
  const QFontMetrics fm(style.metrics.font);
  const int textW = textWidth(fm);
  const QRectF iconRect = defaultRadioIconRect(contentRect, style, textW, hasText, block_);
  const QRectF iconBackgroundRect =
      snapRectToDevicePixels(iconRect.adjusted(0.5, 0.5, -0.5, -0.5), dpr);
  const qreal iconStrokeInset = style.metrics.borderWidth / 2.0 + 0.5;
  const QRectF iconBorderRect =
      snapRectToDevicePixels(iconRect.adjusted(iconStrokeInset,
                                               iconStrokeInset,
                                               -iconStrokeInset,
                                               -iconStrokeInset),
                             dpr);

  // CSS paints the background across the full border box, even when border is transparent.
  painter.setPen(Qt::NoPen);
  painter.setBrush(dotState.backgroundColor);
  painter.drawEllipse(iconBackgroundRect);

  if (dotState.borderColor.alpha() > 0 && style.metrics.borderWidth > 0) {
    QPen borderPen(dotState.borderColor,
                   style.metrics.borderWidth,
                   Qt::SolidLine,
                   Qt::RoundCap,
                   Qt::RoundJoin);
    painter.setPen(borderPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(iconBorderRect);
  }

  if (checkedNow) {
    const qreal dotSize = static_cast<qreal>(std::min(style.metrics.dotSize, style.metrics.radioSize - 4));
    const qreal alignedDotSize = snapToDevicePixelSize(dotSize, dpr);
    const QRectF dotRect =
        snapRectToDevicePixels(centeredSquare(iconBackgroundRect.center(), alignedDotSize), dpr);
    painter.setPen(Qt::NoPen);
    painter.setBrush(dotState.dotColor);
    painter.drawEllipse(dotRect);
  }

  if (hasText) {
    const qreal textX =
        iconRect.x() + iconRect.width() + static_cast<qreal>(style.metrics.labelPaddingInlineStart);
    const qreal textRight = contentRect.x() + contentRect.width() -
                            static_cast<qreal>(style.metrics.labelPaddingInlineEnd);
    QRectF textRect(textX,
                    contentRect.y(),
                    std::max<qreal>(0.0, textRight - textX),
                    contentRect.height());
    painter.setPen(dotState.labelColor);
    painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, text());
  }
}

void AdRadio::nextCheckState() {
  if (!value_.isValid()) {
    return;
  }
  if (!isChecked()) {
    setChecked(true);
  }
}

void AdRadio::enterEvent(QEnterEvent* event) {
  hovered_ = true;
  if (!effectiveDisabled()) {
    bumpButtonGroupZOrder();
  }
  update();
  QAbstractButton::enterEvent(event);
}

void AdRadio::leaveEvent(QEvent* event) {
  hovered_ = false;
  pressed_ = false;
  update();
  QAbstractButton::leaveEvent(event);
}

void AdRadio::mousePressEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton && !effectiveDisabled() && value_.isValid()) {
    pressed_ = true;
    bumpButtonGroupZOrder();
    update();
  }
  QAbstractButton::mousePressEvent(event);
}

void AdRadio::mouseReleaseEvent(QMouseEvent* event) {
  const bool hasValue = value_.isValid();
  const bool triggerWave =
      event->button() == Qt::LeftButton && pressed_ && rect().contains(event->position().toPoint()) &&
      !effectiveDisabled() && hasValue;
  pressed_ = false;
  if (event->button() == Qt::LeftButton && !hasValue) {
    update();
    event->accept();
    return;
  }
  if (triggerWave) {
    startWaveEffect();
  }
  QAbstractButton::mouseReleaseEvent(event);
  update();
}

void AdRadio::keyPressEvent(QKeyEvent* event) {
  if (!effectiveDisabled() && value_.isValid() &&
      (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)) {
    pressed_ = true;
    bumpButtonGroupZOrder();
    update();
  }
  QAbstractButton::keyPressEvent(event);
}

void AdRadio::keyReleaseEvent(QKeyEvent* event) {
  const bool hasValue = value_.isValid();
  const bool interactiveKey =
      event->key() == Qt::Key_Space || event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter;
  const bool triggerWave =
      !effectiveDisabled() && hasValue && interactiveKey;
  pressed_ = false;
  if (!hasValue && interactiveKey) {
    update();
    event->accept();
    return;
  }
  if (triggerWave) {
    startWaveEffect();
  }
  QAbstractButton::keyReleaseEvent(event);
  update();
}

void AdRadio::focusInEvent(QFocusEvent* event) {
  focusVisible_ = isKeyboardFocusReason(event->reason());
  if (!effectiveDisabled()) {
    bumpButtonGroupZOrder();
  }
  QAbstractButton::focusInEvent(event);
  updateInteractionFocusOverlay();
  update();
}

void AdRadio::focusOutEvent(QFocusEvent* event) {
  focusVisible_ = false;
  QAbstractButton::focusOutEvent(event);
  stopInteractionFocusForOwner(this);
  update();
}

void AdRadio::moveEvent(QMoveEvent* event) {
  QAbstractButton::moveEvent(event);
  updateInteractionFocusOverlay();
}

void AdRadio::resizeEvent(QResizeEvent* event) {
  QAbstractButton::resizeEvent(event);
  updateInteractionFocusOverlay();
}

void AdRadio::showEvent(QShowEvent* event) {
  QAbstractButton::showEvent(event);
  updateInteractionFocusOverlay();
}

void AdRadio::hideEvent(QHideEvent* event) {
  QAbstractButton::hideEvent(event);
  stopInteractionFocusForOwner(this);
  stopWaveEffect();
}

void AdRadio::changeEvent(QEvent* event) {
  QAbstractButton::changeEvent(event);
  if (!event) {
    return;
  }

  if (event->type() == QEvent::EnabledChange) {
    if (effectiveDisabled()) {
      stopInteractionFocusForOwner(this);
      stopWaveEffect();
    }
    emit disabledChanged(disabled());
    update();
  } else if (event->type() == QEvent::FontChange) {
    refreshAfterPropertyChange(true);
  }
}

bool AdRadio::effectiveDisabled() const { return !isEnabled(); }

AdRadio::SemanticStyles AdRadio::resolvedSemanticStyles() const {
  SemanticStyles merged = semanticStyles_;
  if (!semanticStyleResolver_) {
    return merged;
  }

  StyleContext ctx;
  ctx.size = size_;
  ctx.optionType = optionType_;
  ctx.buttonStyle = buttonStyle_;
  ctx.checked = isChecked();
  ctx.disabled = effectiveDisabled();
  ctx.hovered = hovered_;
  ctx.focused = hasFocus() && focusVisible_;
  ctx.block = block_;

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
  mergeSlot(&merged.icon, resolved.icon);
  mergeSlot(&merged.label, resolved.label);

  return merged;
}

void AdRadio::refreshAfterPropertyChange(bool updateGeometry) {
  if (updateGeometry) {
    QWidget::updateGeometry();
  }
  updateInteractionFocusOverlay();
  update();
}

void AdRadio::updateInteractionFocusOverlay() {
  if (!(hasFocus() && focusVisible_) || effectiveDisabled() || !isVisible()) {
    stopInteractionFocusForOwner(this);
    return;
  }

  detail::RadioStyleInput input;
  input.size = size_;
  input.optionType = optionType_;
  input.buttonStyle = buttonStyle_;
  input.checked = isChecked();
  input.hovered = hovered_;
  input.pressed = pressed_;
  input.disabled = effectiveDisabled();
  input.focused = hasFocus() && focusVisible_;
  input.block = block_;
  input.baseFont = font();
  input.componentTokens = componentTokens_;
  input.semanticStyles = resolvedSemanticStyles();
  const detail::RadioVisualStyle style = detail::resolveRadioVisualStyle(input);

  QWidget* hostWindow = window();
  if (!hostWindow) {
    return;
  }

  InteractionFocusRequest request;
  request.owner = this;
  const QPoint widgetOriginInWindow = mapTo(hostWindow, QPoint(0, 0));
  if (optionType_ == OptionType::Default) {
    const QFontMetrics fm(style.metrics.font);
    const QRectF contentRect = QRectF(rect());
    const QRectF iconRect =
        defaultRadioIconRect(contentRect, style, textWidth(fm), !text().isEmpty(), block_);
    request.baseRectInWindow = iconRect.translated(widgetOriginInWindow.x(), widgetOriginInWindow.y());
    const qreal radius = iconRect.width() / 2.0;
    request.topLeft = radius;
    request.topRight = radius;
    request.bottomRight = radius;
    request.bottomLeft = radius;
  } else {
    request.baseRectInWindow = QRectF(QPointF(widgetOriginInWindow), QSizeF(QWidget::size()));
    resolveButtonCornerRadii(&request.topLeft,
                             &request.topRight,
                             &request.bottomRight,
                             &request.bottomLeft);
  }
  request.color = style.metrics.focusOutlineColor;
  request.strokeWidth = style.metrics.focusOutlineWidth;
  request.offset = style.metrics.focusOutlineOffset;

  triggerInteractionFocus(request);
}

void AdRadio::startWaveEffect() {
  if (effectiveDisabled() || !isVisible()) {
    return;
  }

  detail::RadioStyleInput input;
  input.size = size_;
  input.optionType = optionType_;
  input.buttonStyle = buttonStyle_;
  input.checked = isChecked();
  input.hovered = hovered_;
  input.pressed = pressed_;
  input.disabled = effectiveDisabled();
  input.focused = hasFocus() && focusVisible_;
  input.block = block_;
  input.baseFont = font();
  input.componentTokens = componentTokens_;
  input.semanticStyles = resolvedSemanticStyles();
  const detail::RadioVisualStyle style = detail::resolveRadioVisualStyle(input);

  QWidget* hostWindow = window();
  if (!hostWindow) {
    return;
  }

  InteractionWaveRequest request;
  request.owner = this;
  const QPoint widgetOriginInWindow = mapTo(hostWindow, QPoint(0, 0));
  if (optionType_ == OptionType::Default) {
    const QFontMetrics fm(style.metrics.font);
    const QRectF contentRect = QRectF(rect());
    const QRectF iconRect =
        defaultRadioIconRect(contentRect, style, textWidth(fm), !text().isEmpty(), block_);
    request.baseRectInWindow = iconRect.translated(widgetOriginInWindow.x(), widgetOriginInWindow.y());
    const qreal radius = iconRect.width() / 2.0;
    request.topLeft = radius;
    request.topRight = radius;
    request.bottomRight = radius;
    request.bottomLeft = radius;
    request.color = style.dotChecked.borderColor;
    request.strokeWidthScale = kDefaultRadioWaveStrokeScale;
  } else {
    request.baseRectInWindow = QRectF(QPointF(widgetOriginInWindow), QSizeF(QWidget::size()));
    request.color = style.buttonChecked.borderColor;
    resolveButtonCornerRadii(&request.topLeft,
                             &request.topRight,
                             &request.bottomRight,
                             &request.bottomLeft);
  }

  triggerInteractionWave(request);
}

void AdRadio::stopWaveEffect() { stopInteractionWaveForOwner(this); }

void AdRadio::bumpButtonGroupZOrder() {
  if (optionType_ != OptionType::Button || groupPosition_ == GroupPosition::None || !isVisible()) {
    return;
  }
  if (!isChecked() || effectiveDisabled()) {
    return;
  }
  raise();
}

int AdRadio::textWidth(const QFontMetrics& metrics) const {
  if (text().isEmpty()) {
    return 0;
  }
  const int advance = metrics.horizontalAdvance(text());
  if (advance > 0) {
    return advance;
  }
  return metrics.boundingRect(text()).width();
}

qreal AdRadio::cornerRadius() const {
  if (optionType_ == OptionType::Default) {
    detail::RadioStyleInput input;
    input.size = size_;
    input.optionType = optionType_;
    input.buttonStyle = buttonStyle_;
    input.checked = isChecked();
    input.hovered = hovered_;
    input.pressed = pressed_;
    input.disabled = effectiveDisabled();
    input.focused = hasFocus() && focusVisible_;
    input.block = block_;
    input.baseFont = font();
    input.componentTokens = componentTokens_;
    input.semanticStyles = resolvedSemanticStyles();
    const detail::RadioVisualStyle style = detail::resolveRadioVisualStyle(input);
    return style.metrics.radioSize / 2.0;
  }

  detail::RadioStyleInput input;
  input.size = size_;
  input.optionType = optionType_;
  input.buttonStyle = buttonStyle_;
  input.baseFont = font();
  input.componentTokens = componentTokens_;
  input.semanticStyles = resolvedSemanticStyles();
  const detail::RadioVisualStyle style = detail::resolveRadioVisualStyle(input);
  return style.metrics.buttonBorderRadius;
}

void AdRadio::resolveButtonCornerRadii(qreal* topLeft,
                                       qreal* topRight,
                                       qreal* bottomRight,
                                       qreal* bottomLeft) const {
  qreal tl = cornerRadius();
  qreal tr = tl;
  qreal br = tl;
  qreal bl = tl;

  if (optionType_ == OptionType::Button &&
      groupPosition_ != GroupPosition::None &&
      groupPosition_ != GroupPosition::Only) {
    if (groupVertical_) {
      if (groupPosition_ == GroupPosition::First) {
        bl = 0.0;
        br = 0.0;
      } else if (groupPosition_ == GroupPosition::Middle) {
        tl = 0.0;
        tr = 0.0;
        bl = 0.0;
        br = 0.0;
      } else if (groupPosition_ == GroupPosition::Last) {
        tl = 0.0;
        tr = 0.0;
      }
    } else {
      if (groupPosition_ == GroupPosition::First) {
        tr = 0.0;
        br = 0.0;
      } else if (groupPosition_ == GroupPosition::Middle) {
        tl = 0.0;
        tr = 0.0;
        bl = 0.0;
        br = 0.0;
      } else if (groupPosition_ == GroupPosition::Last) {
        tl = 0.0;
        bl = 0.0;
      }
    }
  }

  if (topLeft) {
    *topLeft = tl;
  }
  if (topRight) {
    *topRight = tr;
  }
  if (bottomRight) {
    *bottomRight = br;
  }
  if (bottomLeft) {
    *bottomLeft = bl;
  }
}

void AdRadio::setGroupPosition(GroupPosition position) {
  if (groupPosition_ == position) {
    return;
  }
  groupPosition_ = position;
  if (isChecked()) {
    bumpButtonGroupZOrder();
  }
  update();
}

void AdRadio::setGroupVertical(bool vertical) {
  if (groupVertical_ == vertical) {
    return;
  }
  groupVertical_ = vertical;
  update();
}

void AdRadio::setGroupName(const QString& name) {
  groupName_ = name;
  if (!groupName_.isEmpty()) {
    setProperty("name", groupName_);
  } else {
    setProperty("name", QVariant());
  }
}

AdRadioButton::AdRadioButton(QWidget* parent) : AdRadio(parent) {
  setOptionType(AdRadio::OptionType::Button);
}

AdRadioButton::AdRadioButton(const QString& text, QWidget* parent) : AdRadio(text, parent) {
  setOptionType(AdRadio::OptionType::Button);
}

}  // namespace adqt::widgets
