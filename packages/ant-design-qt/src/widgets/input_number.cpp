#include "input_number.h"

#include "icons.h"
#include "input_number_style.h"
#include "interaction_overlay_manager.h"
#include "theme/theme.h"

#include <QApplication>
#include <QEnterEvent>
#include <QEvent>
#include <QFocusEvent>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QRegion>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <utility>

namespace adqt::widgets {

namespace {

namespace outlined_icons = adqt::icons::outlined;

bool iconStylesEqual(const adqt::icons::IconStyle& lhs, const adqt::icons::IconStyle& rhs) {
  return lhs.hasPrimary == rhs.hasPrimary && lhs.hasSecondary == rhs.hasSecondary &&
         lhs.hasTertiary == rhs.hasTertiary && lhs.primary == rhs.primary &&
         lhs.secondary == rhs.secondary && lhs.tertiary == rhs.tertiary;
}

bool iconTokensEqual(const adqt::icons::IconToken& lhs, const adqt::icons::IconToken& rhs) {
  return lhs.index == rhs.index && iconStylesEqual(lhs.style, rhs.style);
}

QPoint mouseEventPos(const QMouseEvent* event) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  return event ? event->position().toPoint() : QPoint();
#else
  return event ? event->pos() : QPoint();
#endif
}

bool isLeftMouseActivationEvent(const QEvent* event) {
  if (!event) {
    return false;
  }
  if (event->type() != QEvent::MouseButtonPress &&
      event->type() != QEvent::MouseButtonDblClick) {
    return false;
  }
  const auto* mouseEvent = static_cast<const QMouseEvent*>(event);
  return mouseEvent->button() == Qt::LeftButton;
}

QString cssRgba(const QColor& color) {
  if (!color.isValid()) {
    return QStringLiteral("rgba(0,0,0,0)");
  }
  return QStringLiteral("rgba(%1,%2,%3,%4)")
      .arg(color.red())
      .arg(color.green())
      .arg(color.blue())
      .arg(QString::number(color.alphaF(), 'f', 3));
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

QRectF joinedBorderRect(const QRect& bounds, qreal borderWidth, bool joinedLeft, bool joinedRight) {
  const qreal half = std::max<qreal>(0.0, borderWidth / 2.0);
  qreal leftInset = half + 0.5;
  qreal rightInset = half + 0.5;
  if (joinedLeft) {
    leftInset = half;
  }
  if (joinedRight) {
    rightInset = half;
  }
  return QRectF(bounds).adjusted(leftInset, half, -rightInset, -half);
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

  return QRectF(left, top, std::max(minSize, right - left), std::max(minSize, bottom - top));
}

QPixmap renderTintedIcon(const adqt::icons::IconToken& token, const QColor& color, int side, qreal dpr) {
  if (!adqt::icons::isValid(token) || side <= 0) {
    return QPixmap();
  }
  adqt::icons::IconToken tinted = token;
  tinted.style.primary = color;
  tinted.style.hasPrimary = true;
  return adqt::icons::renderIconPixmap(tinted, QSize(side, side), dpr, QIcon::Normal, QIcon::Off);
}

bool fuzzyEqual(long double lhs, long double rhs) {
  return std::fabs(lhs - rhs) <= 1e-12L;
}

QString trimNumberText(QString text) {
  while (text.contains(QLatin1Char('.')) &&
         (text.endsWith(QLatin1Char('0')) || text.endsWith(QLatin1Char('.')))) {
    text.chop(1);
    if (text.endsWith(QLatin1Char('.'))) {
      text.chop(1);
      break;
    }
  }
  if (text.isEmpty() || text == QStringLiteral("-0")) {
    return QStringLiteral("0");
  }
  return text;
}

QString numberToString(long double value, int precision = -1) {
  if (!std::isfinite(static_cast<double>(value))) {
    return QStringLiteral("0");
  }
  if (precision >= 0) {
    return QString::number(static_cast<double>(value), 'f', precision);
  }
  return trimNumberText(QString::number(static_cast<double>(value), 'f', 16));
}

QVariant variantFromNumber(long double value, bool stringMode, int precision) {
  if (stringMode) {
    return QVariant(numberToString(value, precision));
  }
  return QVariant(static_cast<double>(value));
}

}  // namespace

AdInputNumber::AdInputNumber(QWidget* parent) : QWidget(parent) {
  setAttribute(Qt::WA_Hover, true);
  setMouseTracking(true);
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

  rootLayout_ = new QHBoxLayout(this);
  rootLayout_->setContentsMargins(0, 0, 0, 0);
  rootLayout_->setSpacing(0);

  prefixIconLabel_ = new QLabel(this);
  prefixIconLabel_->setAlignment(Qt::AlignCenter);
  prefixIconLabel_->setVisible(false);
  prefixIconLabel_->installEventFilter(this);

  prefixLabel_ = new QLabel(this);
  prefixLabel_->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
  prefixLabel_->setVisible(false);
  prefixLabel_->installEventFilter(this);

  lineEdit_ = new QLineEdit(this);
  lineEdit_->setFrame(false);
  lineEdit_->setClearButtonEnabled(false);
  lineEdit_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  lineEdit_->installEventFilter(this);
  setFocusProxy(lineEdit_);

  suffixLabel_ = new QLabel(this);
  suffixLabel_->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
  suffixLabel_->setVisible(false);
  suffixLabel_->installEventFilter(this);

  suffixIconLabel_ = new QLabel(this);
  suffixIconLabel_->setAlignment(Qt::AlignCenter);
  suffixIconLabel_->setVisible(false);
  suffixIconLabel_->installEventFilter(this);

  inputActionsWidget_ = new QWidget(this);
  inputActionsWidget_->installEventFilter(this);
  inputActionsWidget_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
  inputActionsLayout_ = new QVBoxLayout(inputActionsWidget_);
  inputActionsLayout_->setContentsMargins(0, 0, 0, 0);
  inputActionsLayout_->setSpacing(0);

  inputUpButton_ = new QToolButton(inputActionsWidget_);
  inputDownButton_ = new QToolButton(inputActionsWidget_);
  inputUpButton_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  inputDownButton_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  inputUpButton_->setAutoRepeat(true);
  inputDownButton_->setAutoRepeat(true);
  inputUpButton_->setFocusPolicy(Qt::NoFocus);
  inputDownButton_->setFocusPolicy(Qt::NoFocus);
  inputUpButton_->installEventFilter(this);
  inputDownButton_->installEventFilter(this);
  inputActionsLayout_->addWidget(inputUpButton_, 1);
  inputActionsLayout_->addWidget(inputDownButton_, 1);

  spinnerDownButton_ = new QToolButton(this);
  spinnerUpButton_ = new QToolButton(this);
  spinnerDownButton_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
  spinnerUpButton_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
  spinnerDownButton_->setAutoRepeat(true);
  spinnerUpButton_->setAutoRepeat(true);
  spinnerDownButton_->setFocusPolicy(Qt::NoFocus);
  spinnerUpButton_->setFocusPolicy(Qt::NoFocus);
  spinnerDownButton_->installEventFilter(this);
  spinnerUpButton_->installEventFilter(this);

  connect(inputUpButton_, &QToolButton::clicked, this,
          [this]() { stepBy(+1, StepEmitter::Handler); });
  connect(inputDownButton_, &QToolButton::clicked, this,
          [this]() { stepBy(-1, StepEmitter::Handler); });
  connect(spinnerUpButton_, &QToolButton::clicked, this,
          [this]() { stepBy(+1, StepEmitter::Handler); });
  connect(spinnerDownButton_, &QToolButton::clicked, this,
          [this]() { stepBy(-1, StepEmitter::Handler); });

  connect(lineEdit_, &QLineEdit::textEdited, this, [this](const QString& text) {
    if (internalTextUpdate_) {
      return;
    }

    userTyping_ = true;
    const QVariant parsed = parsedFromText(text, true);
    if (parsed.isValid() || text.trimmed().isEmpty()) {
      setValueInternal(parsed, true, true);
    } else {
      updateVisualState();
    }
  });

  connect(lineEdit_, &QLineEdit::editingFinished, this, [this]() {
    userTyping_ = false;
    commitFromText(true);
  });
  connect(lineEdit_, &QLineEdit::returnPressed, this, &AdInputNumber::returnPressed);

  connect(&adqt::theme::ThemeManager::instance(), &adqt::theme::ThemeManager::themeChanged, this,
          [this]() { updateVisualState(); });

  syncLayoutForMode();
  updateAffixVisual();
  updateActionVisibility();
  updateActionIcons();
  updateReadOnlyState();
  updateVisualState();
}

AdInputNumber::~AdInputNumber() {
  stopInteractionFocusForOwner(this);

  // QWidget teardown can dispatch late change/hide events while child widgets and
  // layouts are being destroyed. Nulling raw caches prevents stale dereferences.
  rootLayout_ = nullptr;
  prefixIconLabel_ = nullptr;
  prefixLabel_ = nullptr;
  lineEdit_ = nullptr;
  suffixLabel_ = nullptr;
  suffixIconLabel_ = nullptr;
  inputActionsWidget_ = nullptr;
  inputActionsLayout_ = nullptr;
  inputUpButton_ = nullptr;
  inputDownButton_ = nullptr;
  spinnerDownButton_ = nullptr;
  spinnerUpButton_ = nullptr;
}

AdInputNumber::Size AdInputNumber::size() const {
  return size_;
}

void AdInputNumber::setSize(Size value) {
  if (size_ == value) {
    return;
  }
  size_ = value;
  updateVisualState();
  updateGeometry();
  emit sizeChanged(size_);
}

AdInputNumber::Variant AdInputNumber::variant() const {
  return variant_;
}

void AdInputNumber::setVariant(Variant value) {
  if (variant_ == value) {
    return;
  }
  variant_ = value;
  updateVisualState();
  emit variantChanged(variant_);
}

AdInputNumber::Status AdInputNumber::status() const {
  return status_;
}

void AdInputNumber::setStatus(Status value) {
  if (status_ == value) {
    return;
  }
  status_ = value;
  updateVisualState();
  emit statusChanged(status_);
}

AdInputNumber::Mode AdInputNumber::mode() const {
  return mode_;
}

void AdInputNumber::setMode(Mode value) {
  if (mode_ == value) {
    return;
  }
  mode_ = value;
  syncLayoutForMode();
  updateVisualState();
  emit modeChanged(mode_);
}

bool AdInputNumber::disabled() const {
  return !isEnabled();
}

void AdInputNumber::setDisabled(bool value) {
  if (disabled() == value) {
    return;
  }
  QWidget::setDisabled(value);
  updateReadOnlyState();
  updateActionVisibility();
  updateVisualState();
  emit disabledChanged(value);
}

bool AdInputNumber::readOnly() const {
  return readOnly_;
}

void AdInputNumber::setReadOnly(bool value) {
  if (readOnly_ == value) {
    return;
  }
  readOnly_ = value;
  updateReadOnlyState();
  updateActionVisibility();
  updateVisualState();
  emit readOnlyChanged(readOnly_);
}

bool AdInputNumber::controls() const {
  return controls_;
}

void AdInputNumber::setControls(bool value) {
  if (controls_ == value) {
    return;
  }
  controls_ = value;
  updateActionVisibility();
  updateVisualState();
  emit controlsChanged(controls_);
}

bool AdInputNumber::keyboardEnabled() const {
  return keyboardEnabled_;
}

void AdInputNumber::setKeyboardEnabled(bool value) {
  if (keyboardEnabled_ == value) {
    return;
  }
  keyboardEnabled_ = value;
  emit keyboardEnabledChanged(keyboardEnabled_);
}

bool AdInputNumber::changeOnBlur() const {
  return changeOnBlur_;
}

void AdInputNumber::setChangeOnBlur(bool value) {
  if (changeOnBlur_ == value) {
    return;
  }
  changeOnBlur_ = value;
  emit changeOnBlurChanged(changeOnBlur_);
}

bool AdInputNumber::changeOnWheel() const {
  return changeOnWheel_;
}

void AdInputNumber::setChangeOnWheel(bool value) {
  if (changeOnWheel_ == value) {
    return;
  }
  changeOnWheel_ = value;
  emit changeOnWheelChanged(changeOnWheel_);
}

bool AdInputNumber::stringMode() const {
  return stringMode_;
}

void AdInputNumber::setStringMode(bool value) {
  if (stringMode_ == value) {
    return;
  }

  const QVariant previous = value_;
  stringMode_ = value;
  QVariant converted = normalizeParsedVariant(previous);
  converted = applyPrecisionVariant(converted);
  const bool valueTypeChanged = previous.userType() != converted.userType();
  const bool changed = valueTypeChanged || !valueEquals(previous, converted);
  value_ = converted;

  updateLineEditTextFromValue(false);
  updateVisualState();

  emit stringModeChanged(stringMode_);
  if (changed) {
    emit valueChanged(value_);
  }
}

QVariant AdInputNumber::value() const {
  return value_;
}

void AdInputNumber::setValue(const QVariant& value) {
  setValueInternal(value, true, false);
}

QVariant AdInputNumber::min() const {
  return min_;
}

void AdInputNumber::setMin(const QVariant& value) {
  const QVariant normalized = normalizeParsedVariant(value);
  if (valueEquals(min_, normalized) && min_.userType() == normalized.userType()) {
    return;
  }
  min_ = normalized;
  updateVisualState();
  emit minChanged(min_);
}

QVariant AdInputNumber::max() const {
  return max_;
}

void AdInputNumber::setMax(const QVariant& value) {
  const QVariant normalized = normalizeParsedVariant(value);
  if (valueEquals(max_, normalized) && max_.userType() == normalized.userType()) {
    return;
  }
  max_ = normalized;
  updateVisualState();
  emit maxChanged(max_);
}

QVariant AdInputNumber::step() const {
  return step_;
}

void AdInputNumber::setStep(const QVariant& value) {
  QVariant normalized = normalizeParsedVariant(value);
  long double number = 0.0L;
  if (!normalized.isValid() || !tryVariantToNumber(normalized, &number) || number <= 0.0L) {
    normalized = variantFromNumber(1.0L, stringMode_, precision_);
  }

  if (valueEquals(step_, normalized) && step_.userType() == normalized.userType()) {
    return;
  }

  step_ = normalized;
  emit stepChanged(step_);
}

int AdInputNumber::precision() const {
  return precision_;
}

void AdInputNumber::setPrecision(int value) {
  const int normalized = std::max(-1, value);
  if (precision_ == normalized) {
    return;
  }

  precision_ = normalized;

  const QVariant previous = value_;
  value_ = applyPrecisionVariant(value_);
  updateLineEditTextFromValue(false);
  updateVisualState();

  emit precisionChanged(precision_);
  if (!valueEquals(previous, value_) || previous.userType() != value_.userType()) {
    emit valueChanged(value_);
  }
}

QString AdInputNumber::placeholder() const {
  return lineEdit_ ? lineEdit_->placeholderText() : QString();
}

void AdInputNumber::setPlaceholder(const QString& value) {
  if (!lineEdit_ || lineEdit_->placeholderText() == value) {
    return;
  }
  lineEdit_->setPlaceholderText(value);
  updateVisualState();
  emit placeholderChanged(value);
}

QString AdInputNumber::prefixText() const {
  return prefixText_;
}

void AdInputNumber::setPrefixText(const QString& value) {
  if (prefixText_ == value) {
    return;
  }
  prefixText_ = value;
  updateAffixVisual();
  updateGeometry();
  emit prefixTextChanged(prefixText_);
}

QString AdInputNumber::suffixText() const {
  return suffixText_;
}

void AdInputNumber::setSuffixText(const QString& value) {
  if (suffixText_ == value) {
    return;
  }
  suffixText_ = value;
  updateAffixVisual();
  updateGeometry();
  emit suffixTextChanged(suffixText_);
}

bool AdInputNumber::joinedLeft() const {
  return joinedLeft_;
}

void AdInputNumber::setJoinedLeft(bool value) {
  if (joinedLeft_ == value) {
    return;
  }
  joinedLeft_ = value;
  updateVisualState();
}

bool AdInputNumber::joinedRight() const {
  return joinedRight_;
}

void AdInputNumber::setJoinedRight(bool value) {
  if (joinedRight_ == value) {
    return;
  }
  joinedRight_ = value;
  updateVisualState();
}

adqt::icons::IconToken AdInputNumber::prefixIconToken() const {
  return prefixIconToken_;
}

void AdInputNumber::setPrefixIconToken(const adqt::icons::IconToken& token) {
  if (iconTokensEqual(prefixIconToken_, token)) {
    return;
  }
  prefixIconToken_ = token;
  updateAffixVisual();
  updateVisualState();
  emit prefixIconTokenChanged(prefixIconToken_);
}

adqt::icons::IconToken AdInputNumber::suffixIconToken() const {
  return suffixIconToken_;
}

void AdInputNumber::setSuffixIconToken(const adqt::icons::IconToken& token) {
  if (iconTokensEqual(suffixIconToken_, token)) {
    return;
  }
  suffixIconToken_ = token;
  updateAffixVisual();
  updateVisualState();
  emit suffixIconTokenChanged(suffixIconToken_);
}

adqt::icons::IconToken AdInputNumber::upIconToken() const {
  return upIconToken_;
}

void AdInputNumber::setUpIconToken(const adqt::icons::IconToken& token) {
  if (iconTokensEqual(upIconToken_, token)) {
    return;
  }
  upIconToken_ = token;
  updateActionIcons();
  emit upIconTokenChanged(upIconToken_);
}

adqt::icons::IconToken AdInputNumber::downIconToken() const {
  return downIconToken_;
}

void AdInputNumber::setDownIconToken(const adqt::icons::IconToken& token) {
  if (iconTokensEqual(downIconToken_, token)) {
    return;
  }
  downIconToken_ = token;
  updateActionIcons();
  emit downIconTokenChanged(downIconToken_);
}

AdInputNumber::Formatter AdInputNumber::formatter() const {
  return formatter_;
}

void AdInputNumber::setFormatter(Formatter value) {
  formatter_ = std::move(value);
  updateLineEditTextFromValue(false);
}

AdInputNumber::Parser AdInputNumber::parser() const {
  return parser_;
}

void AdInputNumber::setParser(Parser value) {
  parser_ = std::move(value);
}

AdInputNumber::ComponentTokens AdInputNumber::componentTokens() const {
  return componentTokens_;
}

void AdInputNumber::setComponentTokens(const ComponentTokens& tokens) {
  componentTokens_ = tokens;
  updateVisualState();
  emit componentTokensChanged();
}

void AdInputNumber::resetComponentTokens() {
  componentTokens_ = ComponentTokens();
  updateVisualState();
  emit componentTokensChanged();
}

AdInputNumber::SemanticStyles AdInputNumber::semanticStyles() const {
  return semanticStyles_;
}

void AdInputNumber::setSemanticStyles(const SemanticStyles& styles) {
  semanticStyles_ = styles;
  updateVisualState();
  emit semanticStylesChanged();
}

void AdInputNumber::setSemanticStyleResolver(SemanticStyleResolver resolver) {
  semanticStyleResolver_ = std::move(resolver);
  updateVisualState();
  emit semanticStylesChanged();
}

QSize AdInputNumber::sizeHint() const {
  StyleContext context;
  context.size = size_;
  context.variant = variant_;
  context.status = status_;
  context.mode = mode_;
  context.disabled = disabled();
  context.readOnly = readOnly_;
  context.focused = focused_;
  context.hovered = hovered_;
  context.controls = controls_;
  context.outOfRange = hasOutOfRangeValue();

  SemanticStyles effectiveSemantic = semanticStyles_;
  if (semanticStyleResolver_) {
    effectiveSemantic = semanticStyleResolver_(context);
  }

  detail::InputNumberStyleInput styleInput;
  styleInput.size = size_;
  styleInput.variant = variant_;
  styleInput.status = status_;
  styleInput.mode = mode_;
  styleInput.disabled = disabled();
  styleInput.readOnly = readOnly_;
  styleInput.focused = focused_;
  styleInput.hovered = hovered_;
  styleInput.controlsVisible = controls_;
  styleInput.outOfRange = hasOutOfRangeValue();
  styleInput.baseFont = font();
  styleInput.componentTokens = componentTokens_;
  styleInput.semanticStyles = effectiveSemantic;
  const detail::InputNumberVisualStyle style = detail::resolveInputNumberVisualStyle(styleInput);

  QFontMetrics fm(style.metrics.font);
  int widthHint = std::max(style.metrics.width, style.metrics.height * 3);
  if (lineEdit_) {
    const QString basis = !lineEdit_->text().isEmpty() ? lineEdit_->text() : lineEdit_->placeholderText();
    widthHint = std::max(widthHint, fm.horizontalAdvance(basis) + 96);
  }
  if (!prefixText_.isEmpty()) {
    widthHint += fm.horizontalAdvance(prefixText_) + std::max(4, style.metrics.horizontalPadding / 2);
  }
  if (!suffixText_.isEmpty()) {
    widthHint += fm.horizontalAdvance(suffixText_) + std::max(4, style.metrics.horizontalPadding / 2);
  }
  if (mode_ == Mode::Spinner && controls_) {
    widthHint += std::max(20, style.metrics.handleWidth) * 2;
  }

  return QSize(widthHint, style.metrics.height);
}

QSize AdInputNumber::minimumSizeHint() const {
  const QSize hint = sizeHint();
  return QSize(std::min(96, hint.width()), hint.height());
}

void AdInputNumber::focusInput(FocusCursor cursor, bool preventScroll) {
  Q_UNUSED(preventScroll)
  if (!lineEdit_) {
    return;
  }

  lineEdit_->setFocus(Qt::OtherFocusReason);
  if (joinedLeft_ || joinedRight_) {
    raise();
  }

  const QString text = lineEdit_->text();
  if (cursor == FocusCursor::Start) {
    lineEdit_->setCursorPosition(0);
  } else if (cursor == FocusCursor::End) {
    lineEdit_->setCursorPosition(text.size());
  } else if (cursor == FocusCursor::All) {
    lineEdit_->selectAll();
  }
}

void AdInputNumber::blurInput() {
  if (lineEdit_) {
    lineEdit_->clearFocus();
  }
}

QLineEdit* AdInputNumber::lineEdit() const {
  return lineEdit_;
}

bool AdInputNumber::eventFilter(QObject* watched, QEvent* event) {
  if (!event) {
    return QWidget::eventFilter(watched, event);
  }

  if (watched == lineEdit_) {
    if (event->type() == QEvent::FocusIn) {
      focused_ = true;
      bumpJoinedZOrder();
      updateVisualState();
      updateInteractionFocusOverlay();
    } else if (event->type() == QEvent::FocusOut) {
      focused_ = false;
      userTyping_ = false;
      updateVisualState();
      updateInteractionFocusOverlay();
    } else if (event->type() == QEvent::KeyPress) {
      auto* keyEvent = static_cast<QKeyEvent*>(event);
      if (!disabled() && !readOnly_ && keyboardEnabled_) {
        if (keyEvent->key() == Qt::Key_Up) {
          stepBy(+1, StepEmitter::KeyDown);
          return true;
        }
        if (keyEvent->key() == Qt::Key_Down) {
          stepBy(-1, StepEmitter::KeyDown);
          return true;
        }
      }
    } else if (event->type() == QEvent::Wheel) {
      if (!disabled() && !readOnly_ && changeOnWheel_) {
        auto* wheel = static_cast<QWheelEvent*>(event);
        const int delta = wheel->angleDelta().y();
        if (delta != 0) {
          stepBy(delta > 0 ? +1 : -1, StepEmitter::Wheel);
          wheel->accept();
          return true;
        }
      }
    }
  } else if (watched == inputUpButton_ || watched == inputDownButton_ || watched == spinnerUpButton_ ||
             watched == spinnerDownButton_) {
    if (event->type() == QEvent::Enter || event->type() == QEvent::Leave ||
        event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease) {
      updateActionIcons();
    }
  } else if (watched == prefixLabel_ || watched == suffixLabel_ || watched == prefixIconLabel_ ||
             watched == suffixIconLabel_ || watched == inputActionsWidget_) {
    if (isLeftMouseActivationEvent(event)) {
      if (disabled() || !lineEdit_) {
        return true;
      }
      if (QWidget* source = qobject_cast<QWidget*>(watched)) {
        const auto* mouseEvent = static_cast<const QMouseEvent*>(event);
        focusFromMouseGlobalPos(source->mapToGlobal(mouseEventPos(mouseEvent)), Qt::MouseFocusReason);
      }
      return true;
    }
  }

  return QWidget::eventFilter(watched, event);
}

void AdInputNumber::enterEvent(QEnterEvent* event) {
  hovered_ = true;
  bumpJoinedZOrder();
  updateVisualState();
  QWidget::enterEvent(event);
}

void AdInputNumber::leaveEvent(QEvent* event) {
  hovered_ = false;
  updateVisualState();
  QWidget::leaveEvent(event);
}

void AdInputNumber::paintEvent(QPaintEvent* event) {
  QWidget::paintEvent(event);

  StyleContext context;
  context.size = size_;
  context.variant = variant_;
  context.status = status_;
  context.mode = mode_;
  context.disabled = disabled();
  context.readOnly = readOnly_;
  context.focused = focused_;
  context.hovered = hovered_;
  context.controls = controls_;
  context.outOfRange = hasOutOfRangeValue();

  SemanticStyles effectiveSemantic = semanticStyles_;
  if (semanticStyleResolver_) {
    effectiveSemantic = semanticStyleResolver_(context);
  }

  detail::InputNumberStyleInput styleInput;
  styleInput.size = size_;
  styleInput.variant = variant_;
  styleInput.status = status_;
  styleInput.mode = mode_;
  styleInput.disabled = disabled();
  styleInput.readOnly = readOnly_;
  styleInput.focused = focused_;
  styleInput.hovered = hovered_;
  styleInput.controlsVisible = controls_;
  styleInput.outOfRange = hasOutOfRangeValue();
  styleInput.baseFont = font();
  styleInput.componentTokens = componentTokens_;
  styleInput.semanticStyles = effectiveSemantic;
  const detail::InputNumberVisualStyle style = detail::resolveInputNumberVisualStyle(styleInput);

  QColor background = style.selectorBg;
  QColor border = style.selectorBorderColor;
  if (!disabled()) {
    if (focused_) {
      background = style.selectorActiveBg;
      border = style.selectorActiveBorderColor;
    } else if (hovered_) {
      background = style.selectorHoverBg;
      border = style.selectorHoverBorderColor;
    }
  }

  const qreal borderWidth = std::max<qreal>(0.0, style.metrics.borderWidth);
  const bool hasVisibleBorder = borderWidth > 0.0 && border.alpha() > 0;
  const QRectF fillRect(rect());
  const QRectF rawBorderRect = joinedBorderRect(rect(), borderWidth, joinedLeft_, joinedRight_);
  if (!fillRect.isValid() || fillRect.width() <= 0.0 || fillRect.height() <= 0.0) {
    return;
  }
  if (hasVisibleBorder &&
      (!rawBorderRect.isValid() || rawBorderRect.width() <= 0.0 || rawBorderRect.height() <= 0.0)) {
    return;
  }

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, true);
  const qreal dpr = painter.device() ? painter.device()->devicePixelRatioF() : devicePixelRatioF();
  const QRectF borderRect = snapRectToDevicePixels(rawBorderRect, dpr);
  const int joinedEdgeClearWidth = 1;

  const qreal cornerRadius = style.underlined ? 0.0 : std::max<qreal>(0.0, style.metrics.borderRadius);
  const qreal topLeftRadius = joinedLeft_ ? 0.0 : cornerRadius;
  const qreal topRightRadius = joinedRight_ ? 0.0 : cornerRadius;
  const qreal bottomRightRadius = joinedRight_ ? 0.0 : cornerRadius;
  const qreal bottomLeftRadius = joinedLeft_ ? 0.0 : cornerRadius;

  if (style.underlined) {
    if (background.alpha() > 0) {
      painter.fillRect(fillRect, background);
    }
    if (hasVisibleBorder) {
      QPen underlinePen(border, borderWidth, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
      painter.setPen(underlinePen);
      painter.setBrush(Qt::NoBrush);
      const qreal underlineY = snapToDevicePixelCoord(borderRect.bottom(), dpr);
      painter.drawLine(QPointF(fillRect.left(), underlineY), QPointF(fillRect.right(), underlineY));
    }
    return;
  }

  const QPainterPath fillPath =
      roundedRectPath(fillRect, topLeftRadius, topRightRadius, bottomRightRadius, bottomLeftRadius);
  if (background.alpha() > 0) {
    painter.fillPath(fillPath, background);

    if (!hasVisibleBorder && (joinedLeft_ || joinedRight_)) {
      // Keep joined edges opaque to avoid anti-aliased seams with adjacent controls.
      painter.save();
      painter.setRenderHint(QPainter::Antialiasing, false);
      painter.setPen(Qt::NoPen);
      painter.setBrush(background);
      if (joinedLeft_) {
        painter.drawRect(QRect(0, 0, joinedEdgeClearWidth, height()));
      }
      if (joinedRight_) {
        painter.drawRect(QRect(std::max(0, width() - joinedEdgeClearWidth), 0, joinedEdgeClearWidth,
                               height()));
      }
      painter.restore();
    }
  }

  if (hasVisibleBorder) {
    const QPainterPath borderPath =
        roundedRectPath(borderRect, topLeftRadius, topRightRadius, bottomRightRadius, bottomLeftRadius);
    QPen borderPen(border, borderWidth, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
    painter.setPen(borderPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(borderPath);
  }
}

void AdInputNumber::changeEvent(QEvent* event) {
  QWidget::changeEvent(event);
  if (!event) {
    return;
  }

  if (event->type() == QEvent::Hide) {
    hovered_ = false;
    stopInteractionFocusForOwner(this);
    return;
  }

  if (event->type() == QEvent::EnabledChange || event->type() == QEvent::FontChange ||
      event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange) {
    updateReadOnlyState();
    updateActionVisibility();
    updateVisualState();
  }
}

void AdInputNumber::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  updateInteractionFocusOverlay();
}

void AdInputNumber::moveEvent(QMoveEvent* event) {
  QWidget::moveEvent(event);
  updateInteractionFocusOverlay();
}

void AdInputNumber::showEvent(QShowEvent* event) {
  QWidget::showEvent(event);
  updateInteractionFocusOverlay();
}

void AdInputNumber::hideEvent(QHideEvent* event) {
  QWidget::hideEvent(event);
  stopInteractionFocusForOwner(this);
}

void AdInputNumber::wheelEvent(QWheelEvent* event) {
  if (!event) {
    QWidget::wheelEvent(event);
    return;
  }

  if (!disabled() && !readOnly_ && changeOnWheel_) {
    const int delta = event->angleDelta().y();
    if (delta != 0) {
      stepBy(delta > 0 ? +1 : -1, StepEmitter::Wheel);
      event->accept();
      return;
    }
  }

  QWidget::wheelEvent(event);
}

void AdInputNumber::mousePressEvent(QMouseEvent* event) {
  if (event && event->button() == Qt::LeftButton && !disabled() && lineEdit_) {
    focusFromMouseGlobalPos(mapToGlobal(mouseEventPos(event)), Qt::MouseFocusReason);
    event->accept();
    return;
  }
  QWidget::mousePressEvent(event);
}

void AdInputNumber::updateInteractionFocusOverlay() {
  if (!focused_ || disabled() || !isVisible()) {
    stopInteractionFocusForOwner(this);
    return;
  }

  StyleContext context;
  context.size = size_;
  context.variant = variant_;
  context.status = status_;
  context.mode = mode_;
  context.disabled = disabled();
  context.readOnly = readOnly_;
  context.focused = focused_;
  context.hovered = hovered_;
  context.controls = controls_;
  context.outOfRange = hasOutOfRangeValue();

  SemanticStyles effectiveSemantic = semanticStyles_;
  if (semanticStyleResolver_) {
    effectiveSemantic = semanticStyleResolver_(context);
  }

  detail::InputNumberStyleInput styleInput;
  styleInput.size = size_;
  styleInput.variant = variant_;
  styleInput.status = status_;
  styleInput.mode = mode_;
  styleInput.disabled = disabled();
  styleInput.readOnly = readOnly_;
  styleInput.focused = focused_;
  styleInput.hovered = hovered_;
  styleInput.controlsVisible = controls_;
  styleInput.outOfRange = hasOutOfRangeValue();
  styleInput.baseFont = font();
  styleInput.componentTokens = componentTokens_;
  styleInput.semanticStyles = effectiveSemantic;
  const detail::InputNumberVisualStyle style = detail::resolveInputNumberVisualStyle(styleInput);

  if (style.underlined || style.selectorFocusOutlineColor.alpha() <= 0 ||
      style.metrics.focusOutlineWidth <= 0.0) {
    stopInteractionFocusForOwner(this);
    return;
  }

  const qreal borderWidth = std::max<qreal>(0.0, style.metrics.borderWidth);
  const QRectF shellRect = joinedBorderRect(rect(), borderWidth, joinedLeft_, joinedRight_);
  if (!shellRect.isValid() || shellRect.width() <= 0.0 || shellRect.height() <= 0.0) {
    stopInteractionFocusForOwner(this);
    return;
  }

  QWidget* hostWindow = window();
  if (!hostWindow) {
    stopInteractionFocusForOwner(this);
    return;
  }

  const qreal cornerRadius = std::max<qreal>(0.0, style.metrics.borderRadius);
  InteractionFocusRequest request;
  request.owner = this;
  const QPoint origin = mapTo(hostWindow, QPoint(0, 0));
  request.baseRectInWindow = shellRect.translated(origin.x(), origin.y());
  request.topLeft = joinedLeft_ ? 0.0 : cornerRadius;
  request.topRight = joinedRight_ ? 0.0 : cornerRadius;
  request.bottomRight = joinedRight_ ? 0.0 : cornerRadius;
  request.bottomLeft = joinedLeft_ ? 0.0 : cornerRadius;
  request.color = style.selectorFocusOutlineColor;
  request.strokeWidth = std::max<qreal>(1.0, style.metrics.focusOutlineWidth);
  request.offset = std::max<qreal>(0.0, style.metrics.focusOutlineOffset);
  triggerInteractionFocus(request);
}

void AdInputNumber::syncLayoutForMode() {
  if (!rootLayout_) {
    return;
  }

  while (QLayoutItem* item = rootLayout_->takeAt(0)) {
    delete item;
  }

  if (mode_ == Mode::Spinner) {
    rootLayout_->addWidget(spinnerDownButton_);
    rootLayout_->addWidget(lineEdit_, 1);
    rootLayout_->addWidget(spinnerUpButton_);

    if (inputActionsWidget_) {
      inputActionsWidget_->setVisible(false);
    }
    if (prefixIconLabel_) {
      prefixIconLabel_->setVisible(false);
    }
    if (prefixLabel_) {
      prefixLabel_->setVisible(false);
    }
    if (suffixLabel_) {
      suffixLabel_->setVisible(false);
    }
    if (suffixIconLabel_) {
      suffixIconLabel_->setVisible(false);
    }
    if (lineEdit_) {
      lineEdit_->setAlignment(Qt::AlignCenter);
    }
  } else {
    rootLayout_->addWidget(prefixIconLabel_);
    rootLayout_->addWidget(prefixLabel_);
    rootLayout_->addWidget(lineEdit_, 1);
    rootLayout_->addWidget(suffixLabel_);
    rootLayout_->addWidget(suffixIconLabel_);
    rootLayout_->addWidget(inputActionsWidget_);

    if (spinnerDownButton_) {
      spinnerDownButton_->setVisible(false);
    }
    if (spinnerUpButton_) {
      spinnerUpButton_->setVisible(false);
    }
    if (lineEdit_) {
      lineEdit_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    }
  }

  updateAffixVisual();
  updateActionVisibility();
}

void AdInputNumber::updateVisualState() {
  if (!rootLayout_ || !lineEdit_) {
    return;
  }

  StyleContext context;
  context.size = size_;
  context.variant = variant_;
  context.status = status_;
  context.mode = mode_;
  context.disabled = disabled();
  context.readOnly = readOnly_;
  context.focused = focused_;
  context.hovered = hovered_;
  context.controls = controls_;
  context.outOfRange = hasOutOfRangeValue();

  SemanticStyles effectiveSemantic = semanticStyles_;
  if (semanticStyleResolver_) {
    effectiveSemantic = semanticStyleResolver_(context);
  }

  detail::InputNumberStyleInput styleInput;
  styleInput.size = size_;
  styleInput.variant = variant_;
  styleInput.status = status_;
  styleInput.mode = mode_;
  styleInput.disabled = disabled();
  styleInput.readOnly = readOnly_;
  styleInput.focused = focused_;
  styleInput.hovered = hovered_;
  styleInput.controlsVisible = controls_;
  styleInput.outOfRange = hasOutOfRangeValue();
  styleInput.baseFont = font();
  styleInput.componentTokens = componentTokens_;
  styleInput.semanticStyles = effectiveSemantic;
  const detail::InputNumberVisualStyle style = detail::resolveInputNumberVisualStyle(styleInput);

  setFont(style.metrics.font);
  lineEdit_->setFont(style.metrics.font);
  if (prefixLabel_) {
    prefixLabel_->setFont(style.metrics.font);
  }
  if (suffixLabel_) {
    suffixLabel_->setFont(style.metrics.font);
  }

  const int borderInset = std::max(0, style.metrics.borderWidth);
  const int hp = std::max(0, style.metrics.horizontalPadding);
  const int handleWidth = std::max(0, style.metrics.handleWidth);
  const int visibleHandleWidth = std::max(0, style.metrics.handleVisibleWidth);

  int leftInset = hp + borderInset;
  int rightInset = hp + borderInset;
  int verticalInset = 0;
  if (mode_ == Mode::Spinner) {
    leftInset = borderInset;
    rightInset = borderInset;
  } else if (mode_ == Mode::Input && visibleHandleWidth > 0) {
    // Keep action buttons flush to the right border while preserving text padding when controls are hidden.
    rightInset = borderInset;
  }
  if (controls_) {
    // Action buttons paint above the parent border; keep them inside the inner content box.
    verticalInset = borderInset;
  }

  rootLayout_->setContentsMargins(leftInset, verticalInset, rightInset, verticalInset);
  rootLayout_->setSpacing(mode_ == Mode::Input ? std::max(4, hp / 2) : 0);

  const int fixedHeight = std::max(20, style.metrics.height);
  setFixedHeight(fixedHeight);

  if (lineEdit_) {
    lineEdit_->setMinimumHeight(std::max(1, fixedHeight - borderInset * 2));
    lineEdit_->setStyleSheet(
        QStringLiteral("QLineEdit { border: none; background: transparent; padding: 0; }")
    );

    QColor textColor = style.selectorTextColor;
    if (!disabled() && hasOutOfRangeValue()) {
      textColor = style.outOfRangeTextColor;
    }

    QPalette linePalette = lineEdit_->palette();
    linePalette.setColor(QPalette::Text, textColor);
    linePalette.setColor(QPalette::Disabled, QPalette::Text, style.disabledTextColor);
    linePalette.setColor(QPalette::PlaceholderText, style.placeholderColor);
    lineEdit_->setPalette(linePalette);
  }

  if (prefixLabel_) {
    QPalette palette = prefixLabel_->palette();
    palette.setColor(QPalette::WindowText, style.prefixColor);
    prefixLabel_->setPalette(palette);
  }
  if (suffixLabel_) {
    QPalette palette = suffixLabel_->palette();
    palette.setColor(QPalette::WindowText, style.suffixColor);
    suffixLabel_->setPalette(palette);
  }

  const int iconSide = std::max(8, style.metrics.iconSize);
  const qreal dpr = devicePixelRatioF();

  if (prefixIconLabel_) {
    prefixIconLabel_->setFixedSize(iconSide, iconSide);
    if (mode_ == Mode::Input && adqt::icons::isValid(prefixIconToken_)) {
      prefixIconLabel_->setPixmap(renderTintedIcon(prefixIconToken_, style.prefixColor, iconSide, dpr));
    } else {
      prefixIconLabel_->setPixmap(QPixmap());
    }
  }

  if (suffixIconLabel_) {
    suffixIconLabel_->setFixedSize(iconSide, iconSide);
    if (mode_ == Mode::Input && adqt::icons::isValid(suffixIconToken_)) {
      suffixIconLabel_->setPixmap(renderTintedIcon(suffixIconToken_, style.suffixColor, iconSide, dpr));
    } else {
      suffixIconLabel_->setPixmap(QPixmap());
    }
  }

  const int actionCornerRadius = std::max(0, style.metrics.borderRadius - borderInset);
  const int rightActionCornerRadius = joinedRight_ ? 0 : actionCornerRadius;
  const int leftActionCornerRadius = joinedLeft_ ? 0 : actionCornerRadius;

  if (inputActionsWidget_) {
    const int width = (mode_ == Mode::Input) ? visibleHandleWidth : 0;
    inputActionsWidget_->setFixedWidth(width);
    inputActionsWidget_->setFixedHeight(std::max(1, fixedHeight - borderInset * 2));
    if (mode_ == Mode::Input && width > 0 && inputActionsWidget_->height() > 0) {
      const qreal clipWidth = std::max<qreal>(1.0, inputActionsWidget_->width() - 1.0);
      const qreal clipHeight = std::max<qreal>(1.0, inputActionsWidget_->height() - 1.0);
      const QPainterPath actionsPath = roundedRectPath(
          QRectF(0.0, 0.0, clipWidth, clipHeight), 0.0,
          static_cast<qreal>(rightActionCornerRadius),
          static_cast<qreal>(rightActionCornerRadius), 0.0);
      inputActionsWidget_->setMask(QRegion(actionsPath.toFillPolygon().toPolygon()));
    } else {
      inputActionsWidget_->clearMask();
    }
  }

  const QString inputUpCss =
      QStringLiteral(
          "QToolButton { border: none; border-left: %1px solid %2; border-top-right-radius: %5px; background: %3; padding: 0; }"
          "QToolButton:hover { background: %3; }"
          "QToolButton:pressed { background: %4; }")
          .arg(borderInset)
          .arg(cssRgba(style.handleBorderColor))
          .arg(cssRgba(style.handleBg))
          .arg(cssRgba(style.handleActiveBg))
          .arg(rightActionCornerRadius);
  const QString inputDownCss =
      QStringLiteral(
          "QToolButton { border: none; border-left: %1px solid %2; border-top: %1px solid %2; background: %3; "
          "padding: 0; border-bottom-right-radius: %5px; }"
          "QToolButton:hover { background: %3; }"
          "QToolButton:pressed { background: %4; }")
          .arg(borderInset)
          .arg(cssRgba(style.handleBorderColor))
          .arg(cssRgba(style.handleBg))
          .arg(cssRgba(style.handleActiveBg))
          .arg(rightActionCornerRadius);

  const int handleIconSide = std::max(6, style.metrics.handleIconSize);
  if (inputUpButton_) {
    inputUpButton_->setStyleSheet(inputUpCss);
    inputUpButton_->setMinimumWidth(std::max(handleWidth, visibleHandleWidth));
    inputUpButton_->setIconSize(QSize(handleIconSide, handleIconSide));
  }
  if (inputDownButton_) {
    inputDownButton_->setStyleSheet(inputDownCss);
    inputDownButton_->setMinimumWidth(std::max(handleWidth, visibleHandleWidth));
    inputDownButton_->setIconSize(QSize(handleIconSide, handleIconSide));
  }

  const QString spinnerDownCss =
      QStringLiteral(
          "QToolButton { border: none; border-right: %1px solid %2; border-top-left-radius: %5px; border-bottom-left-radius: %5px; background: %3; padding: 0; }"
          "QToolButton:hover { background: %3; }"
          "QToolButton:pressed { background: %4; }")
          .arg(borderInset)
          .arg(cssRgba(style.handleBorderColor))
          .arg(cssRgba(style.handleBg))
          .arg(cssRgba(style.handleActiveBg))
          .arg(leftActionCornerRadius);
  const QString spinnerUpCss =
      QStringLiteral(
          "QToolButton { border: none; border-left: %1px solid %2; border-top-right-radius: %5px; border-bottom-right-radius: %5px; background: %3; padding: 0; }"
          "QToolButton:hover { background: %3; }"
          "QToolButton:pressed { background: %4; }")
          .arg(borderInset)
          .arg(cssRgba(style.handleBorderColor))
          .arg(cssRgba(style.handleBg))
          .arg(cssRgba(style.handleActiveBg))
          .arg(rightActionCornerRadius);

  if (spinnerDownButton_) {
    spinnerDownButton_->setStyleSheet(spinnerDownCss);
    spinnerDownButton_->setFixedWidth(std::max(20, handleWidth));
    spinnerDownButton_->setFixedHeight(std::max(1, fixedHeight - borderInset * 2));
    spinnerDownButton_->setIconSize(QSize(handleIconSide, handleIconSide));
  }
  if (spinnerUpButton_) {
    spinnerUpButton_->setStyleSheet(spinnerUpCss);
    spinnerUpButton_->setFixedWidth(std::max(20, handleWidth));
    spinnerUpButton_->setFixedHeight(std::max(1, fixedHeight - borderInset * 2));
    spinnerUpButton_->setIconSize(QSize(handleIconSide, handleIconSide));
  }

  updateAffixVisual();
  updateActionVisibility();
  updateActionIcons();
  updateInteractiveCursor();
  updateGeometry();
  updateInteractionFocusOverlay();
  update();
}

void AdInputNumber::updateAffixVisual() {
  if (!prefixLabel_ || !suffixLabel_ || !prefixIconLabel_ || !suffixIconLabel_) {
    return;
  }

  const bool inputMode = mode_ == Mode::Input;

  prefixLabel_->setText(prefixText_);
  suffixLabel_->setText(suffixText_);

  const bool showPrefixText = inputMode && !prefixText_.trimmed().isEmpty();
  const bool showSuffixText = inputMode && !suffixText_.trimmed().isEmpty();
  const bool showPrefixIcon = inputMode && adqt::icons::isValid(prefixIconToken_);
  const bool showSuffixIcon = inputMode && adqt::icons::isValid(suffixIconToken_);

  prefixLabel_->setVisible(showPrefixText);
  suffixLabel_->setVisible(showSuffixText);
  prefixIconLabel_->setVisible(showPrefixIcon);
  suffixIconLabel_->setVisible(showSuffixIcon);
}

void AdInputNumber::updateActionIcons() {
  StyleContext context;
  context.size = size_;
  context.variant = variant_;
  context.status = status_;
  context.mode = mode_;
  context.disabled = disabled();
  context.readOnly = readOnly_;
  context.focused = focused_;
  context.hovered = hovered_;
  context.controls = controls_;
  context.outOfRange = hasOutOfRangeValue();

  SemanticStyles effectiveSemantic = semanticStyles_;
  if (semanticStyleResolver_) {
    effectiveSemantic = semanticStyleResolver_(context);
  }

  detail::InputNumberStyleInput styleInput;
  styleInput.size = size_;
  styleInput.variant = variant_;
  styleInput.status = status_;
  styleInput.mode = mode_;
  styleInput.disabled = disabled();
  styleInput.readOnly = readOnly_;
  styleInput.focused = focused_;
  styleInput.hovered = hovered_;
  styleInput.controlsVisible = controls_;
  styleInput.outOfRange = hasOutOfRangeValue();
  styleInput.baseFont = font();
  styleInput.componentTokens = componentTokens_;
  styleInput.semanticStyles = effectiveSemantic;
  const detail::InputNumberVisualStyle style = detail::resolveInputNumberVisualStyle(styleInput);

  const adqt::icons::IconToken upToken =
      adqt::icons::isValid(upIconToken_)
          ? upIconToken_
          : (mode_ == Mode::Spinner ? outlined_icons::Plus() : outlined_icons::Up());
  const adqt::icons::IconToken downToken =
      adqt::icons::isValid(downIconToken_)
          ? downIconToken_
          : (mode_ == Mode::Spinner ? outlined_icons::Minus() : outlined_icons::Down());

  const bool interactive = controls_ && !readOnly_ && !disabled();

  QColor upColor = style.handleIconColor;
  QColor downColor = style.handleIconColor;
  if (interactive) {
    const bool upHovered =
        (mode_ == Mode::Spinner && spinnerUpButton_ && spinnerUpButton_->underMouse()) ||
        (mode_ == Mode::Input && inputUpButton_ && inputUpButton_->underMouse());
    const bool downHovered =
        (mode_ == Mode::Spinner && spinnerDownButton_ && spinnerDownButton_->underMouse()) ||
        (mode_ == Mode::Input && inputDownButton_ && inputDownButton_->underMouse());
    if (upHovered) {
      upColor = style.handleHoverColor;
    }
    if (downHovered) {
      downColor = style.handleHoverColor;
    }
  }

  const int handleIconSide = std::max(6, style.metrics.handleIconSize);
  const qreal dpr = devicePixelRatioF();

  const QIcon upIcon(renderTintedIcon(upToken, upColor, handleIconSide, dpr));
  const QIcon downIcon(renderTintedIcon(downToken, downColor, handleIconSide, dpr));

  if (inputUpButton_) {
    inputUpButton_->setIcon(upIcon);
    inputUpButton_->setIconSize(QSize(handleIconSide, handleIconSide));
  }
  if (inputDownButton_) {
    inputDownButton_->setIcon(downIcon);
    inputDownButton_->setIconSize(QSize(handleIconSide, handleIconSide));
  }
  if (spinnerUpButton_) {
    spinnerUpButton_->setIcon(upIcon);
    spinnerUpButton_->setIconSize(QSize(handleIconSide, handleIconSide));
  }
  if (spinnerDownButton_) {
    spinnerDownButton_->setIcon(downIcon);
    spinnerDownButton_->setIconSize(QSize(handleIconSide, handleIconSide));
  }
}

void AdInputNumber::updateActionVisibility() {
  const bool interactive = controls_ && !readOnly_ && !disabled();
  const bool inputMode = mode_ == Mode::Input;
  const bool spinnerMode = mode_ == Mode::Spinner;

  if (inputActionsWidget_) {
    inputActionsWidget_->setVisible(inputMode);
  }
  if (inputUpButton_) {
    inputUpButton_->setVisible(inputMode && interactive);
    inputUpButton_->setEnabled(inputMode && interactive);
  }
  if (inputDownButton_) {
    inputDownButton_->setVisible(inputMode && interactive);
    inputDownButton_->setEnabled(inputMode && interactive);
  }
  if (spinnerUpButton_) {
    spinnerUpButton_->setVisible(spinnerMode && interactive);
    spinnerUpButton_->setEnabled(spinnerMode && interactive);
  }
  if (spinnerDownButton_) {
    spinnerDownButton_->setVisible(spinnerMode && interactive);
    spinnerDownButton_->setEnabled(spinnerMode && interactive);
  }
}

void AdInputNumber::updateReadOnlyState() {
  const bool disabledNow = disabled();
  const bool readOnlyNow = readOnly_ || disabledNow;

  if (lineEdit_) {
    lineEdit_->setReadOnly(readOnlyNow);
    lineEdit_->setEnabled(!disabledNow);
  }

  const bool buttonEnabled = controls_ && !readOnlyNow;
  if (inputUpButton_) {
    inputUpButton_->setEnabled(buttonEnabled);
  }
  if (inputDownButton_) {
    inputDownButton_->setEnabled(buttonEnabled);
  }
  if (spinnerUpButton_) {
    spinnerUpButton_->setEnabled(buttonEnabled);
  }
  if (spinnerDownButton_) {
    spinnerDownButton_->setEnabled(buttonEnabled);
  }
}

void AdInputNumber::updateInteractiveCursor() {
  const bool disabledNow = disabled();
  const Qt::CursorShape defaultCursor = disabledNow ? Qt::ForbiddenCursor : Qt::ArrowCursor;

  setCursor(defaultCursor);
  if (prefixLabel_) {
    prefixLabel_->setCursor(defaultCursor);
  }
  if (suffixLabel_) {
    suffixLabel_->setCursor(defaultCursor);
  }
  if (prefixIconLabel_) {
    prefixIconLabel_->setCursor(defaultCursor);
  }
  if (suffixIconLabel_) {
    suffixIconLabel_->setCursor(defaultCursor);
  }
  if (inputActionsWidget_) {
    inputActionsWidget_->setCursor(defaultCursor);
  }

  if (lineEdit_) {
    if (disabledNow) {
      lineEdit_->setCursor(Qt::ForbiddenCursor);
    } else if (readOnly_) {
      lineEdit_->setCursor(Qt::ArrowCursor);
    } else {
      lineEdit_->setCursor(Qt::IBeamCursor);
    }
  }

  auto applyButtonCursor = [defaultCursor, disabledNow](QToolButton* button) {
    if (!button) {
      return;
    }
    const Qt::CursorShape buttonCursor = (!disabledNow && button->isVisible() && button->isEnabled())
                                             ? Qt::PointingHandCursor
                                             : defaultCursor;
    button->setCursor(buttonCursor);
  };

  applyButtonCursor(inputUpButton_);
  applyButtonCursor(inputDownButton_);
  applyButtonCursor(spinnerUpButton_);
  applyButtonCursor(spinnerDownButton_);
}

void AdInputNumber::updateLineEditTextFromValue(bool userTyping) {
  if (!lineEdit_ || internalTextUpdate_) {
    return;
  }

  const QString currentInput = lineEdit_->text();
  const QString nextText = formattedForDisplay(value_, userTyping, currentInput);
  if (currentInput == nextText) {
    return;
  }

  const int oldCursor = lineEdit_->cursorPosition();
  {
    QSignalBlocker blocker(lineEdit_);
    internalTextUpdate_ = true;
    lineEdit_->setText(nextText);
    internalTextUpdate_ = false;
  }

  if (userTyping) {
    lineEdit_->setCursorPosition(std::clamp(oldCursor, 0, static_cast<int>(nextText.size())));
  } else {
    lineEdit_->setCursorPosition(nextText.size());
  }
}

void AdInputNumber::commitFromText(bool triggerBlurAdjust) {
  if (!lineEdit_) {
    return;
  }

  const QString text = lineEdit_->text();
  const QVariant parsed = parsedFromText(text, false);

  if (!text.trimmed().isEmpty() && !parsed.isValid()) {
    updateLineEditTextFromValue(false);
    updateVisualState();
    return;
  }

  QVariant next = parsed;
  if (triggerBlurAdjust && changeOnBlur_) {
    next = clampedVariant(next);
  }
  next = applyPrecisionVariant(next);

  setValueInternal(next, true, false);
  updateLineEditTextFromValue(false);
}

bool AdInputNumber::stepBy(int direction, StepEmitter emitter) {
  if (direction == 0 || disabled() || readOnly_) {
    return false;
  }

  long double stepValue = 1.0L;
  if (!tryVariantToNumber(step_, &stepValue) || fuzzyEqual(stepValue, 0.0L)) {
    stepValue = 1.0L;
  }
  stepValue = std::fabs(stepValue);

  long double current = 0.0L;
  const bool hasCurrent = tryVariantToNumber(value_, &current);
  if (!hasCurrent) {
    if (direction > 0) {
      if (!hasMinNumber(&current)) {
        current = 0.0L;
      }
    } else {
      if (!hasMaxNumber(&current)) {
        current = 0.0L;
      }
    }
  }

  long double nextNumber = current + static_cast<long double>(direction) * stepValue;
  long double minNumber = 0.0L;
  if (hasMinNumber(&minNumber) && nextNumber < minNumber) {
    nextNumber = minNumber;
  }
  long double maxNumber = 0.0L;
  if (hasMaxNumber(&maxNumber) && nextNumber > maxNumber) {
    nextNumber = maxNumber;
  }

  QVariant nextVariant = variantFromNumber(nextNumber, stringMode_, precision_);
  nextVariant = applyPrecisionVariant(nextVariant);

  const bool changed = !valueEquals(value_, nextVariant) || value_.userType() != nextVariant.userType();
  if (!changed) {
    return false;
  }

  setValueInternal(nextVariant, true, false);

  const StepType type = direction > 0 ? StepType::Up : StepType::Down;
  emit stepped(value_, direction, type, emitter);
  return true;
}

void AdInputNumber::setValueInternal(const QVariant& value, bool emitSignal, bool userTyping) {
  QVariant normalized = normalizeParsedVariant(value);
  if (!normalized.isValid()) {
    normalized = QVariant();
  }

  if (!userTyping) {
    normalized = applyPrecisionVariant(normalized);
  }

  const bool sameType = value_.userType() == normalized.userType();
  const bool changed = !valueEquals(value_, normalized) || !sameType;

  if (!changed) {
    if (!userTyping) {
      updateLineEditTextFromValue(userTyping);
    }
    updateVisualState();
    return;
  }

  value_ = normalized;
  if (!userTyping) {
    updateLineEditTextFromValue(userTyping);
  }
  updateVisualState();

  if (emitSignal) {
    emit valueChanged(value_);
  }
}

bool AdInputNumber::valueEquals(const QVariant& lhs, const QVariant& rhs) const {
  if (!lhs.isValid() && !rhs.isValid()) {
    return true;
  }
  if (lhs.isValid() != rhs.isValid()) {
    return false;
  }
  if (lhs.userType() == rhs.userType()) {
    return lhs == rhs;
  }

  long double lhsNumber = 0.0L;
  long double rhsNumber = 0.0L;
  if (tryVariantToNumber(lhs, &lhsNumber) && tryVariantToNumber(rhs, &rhsNumber)) {
    return fuzzyEqual(lhsNumber, rhsNumber);
  }

  return lhs.toString() == rhs.toString();
}

bool AdInputNumber::tryVariantToNumber(const QVariant& value, long double* out) const {
  if (!out || !value.isValid()) {
    return false;
  }

  switch (value.userType()) {
    case QMetaType::Double:
    case QMetaType::Float:
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::LongLong:
    case QMetaType::ULongLong: {
      bool ok = false;
      const double number = value.toDouble(&ok);
      if (!ok || !std::isfinite(number)) {
        return false;
      }
      *out = static_cast<long double>(number);
      return true;
    }
    default:
      break;
  }

  const QString raw = value.toString();
  QString text = raw.trimmed();
  if (text.isEmpty()) {
    return false;
  }

  text.remove(QChar(0x00A0));
  text.remove(QLatin1Char(','));
  if (text == QStringLiteral("-") || text == QStringLiteral("+") || text == QStringLiteral(".") ||
      text == QStringLiteral("-.") || text == QStringLiteral("+.")) {
    return false;
  }

  bool ok = false;
  double number = text.toDouble(&ok);
  if (!ok) {
    number = QLocale::c().toDouble(text, &ok);
  }
  if (!ok || !std::isfinite(number)) {
    return false;
  }

  *out = static_cast<long double>(number);
  return true;
}

QVariant AdInputNumber::normalizeParsedVariant(const QVariant& parsed) const {
  if (!parsed.isValid()) {
    return QVariant();
  }

  if (parsed.userType() == QMetaType::QString && parsed.toString().trimmed().isEmpty()) {
    return QVariant();
  }

  if (stringMode_ && parsed.userType() == QMetaType::QString && precision_ < 0) {
    const QString text = parsed.toString().trimmed();
    long double number = 0.0L;
    if (!tryVariantToNumber(text, &number)) {
      return QVariant();
    }
    return QVariant(text);
  }

  long double number = 0.0L;
  if (!tryVariantToNumber(parsed, &number)) {
    return QVariant();
  }

  return variantFromNumber(number, stringMode_, precision_);
}

QVariant AdInputNumber::parsedFromText(const QString& text, bool userTyping) const {
  const QString trimmed = text.trimmed();
  if (trimmed.isEmpty()) {
    return QVariant();
  }

  QVariant parsed;
  if (parser_) {
    parsed = parser_(text);
  } else {
    parsed = QVariant(text);
  }

  QVariant normalized = normalizeParsedVariant(parsed);
  if (!normalized.isValid()) {
    return QVariant();
  }

  if (!userTyping) {
    normalized = applyPrecisionVariant(normalized);
  }
  return normalized;
}

QVariant AdInputNumber::clampedVariant(const QVariant& value) const {
  if (!value.isValid()) {
    return QVariant();
  }

  long double number = 0.0L;
  if (!tryVariantToNumber(value, &number)) {
    return QVariant();
  }

  long double minNumber = 0.0L;
  if (hasMinNumber(&minNumber) && number < minNumber) {
    number = minNumber;
  }

  long double maxNumber = 0.0L;
  if (hasMaxNumber(&maxNumber) && number > maxNumber) {
    number = maxNumber;
  }

  return variantFromNumber(number, stringMode_, precision_);
}

QVariant AdInputNumber::applyPrecisionVariant(const QVariant& value) const {
  if (!value.isValid() || precision_ < 0) {
    return value;
  }

  long double number = 0.0L;
  if (!tryVariantToNumber(value, &number)) {
    return QVariant();
  }

  const long double scale = std::pow(10.0L, static_cast<long double>(precision_));
  if (std::isfinite(static_cast<double>(scale)) && scale > 0.0L) {
    number = std::round(number * scale) / scale;
  }

  return variantFromNumber(number, stringMode_, precision_);
}

QString AdInputNumber::defaultFormatForValue(const QVariant& value) const {
  if (!value.isValid()) {
    return QString();
  }

  if (value.userType() == QMetaType::QString) {
    const QString text = value.toString();
    if (text.trimmed().isEmpty()) {
      return QString();
    }
    if (precision_ >= 0) {
      long double number = 0.0L;
      if (tryVariantToNumber(text, &number)) {
        return numberToString(number, precision_);
      }
    }
    return text;
  }

  long double number = 0.0L;
  if (tryVariantToNumber(value, &number)) {
    return numberToString(number, precision_);
  }

  return value.toString();
}

QString AdInputNumber::formattedForDisplay(const QVariant& value,
                                           bool userTyping,
                                           const QString& input) const {
  if (formatter_) {
    return formatter_(value, userTyping, input);
  }

  if (!value.isValid()) {
    return userTyping ? input : QString();
  }

  return defaultFormatForValue(value);
}

bool AdInputNumber::hasMinNumber(long double* out) const {
  return tryVariantToNumber(min_, out);
}

bool AdInputNumber::hasMaxNumber(long double* out) const {
  return tryVariantToNumber(max_, out);
}

bool AdInputNumber::hasOutOfRangeValue() const {
  long double current = 0.0L;
  if (!tryVariantToNumber(value_, &current)) {
    return false;
  }

  long double minNumber = 0.0L;
  if (hasMinNumber(&minNumber) && current < minNumber) {
    return true;
  }

  long double maxNumber = 0.0L;
  if (hasMaxNumber(&maxNumber) && current > maxNumber) {
    return true;
  }

  return false;
}

int AdInputNumber::cursorPositionForClickX(int x) const {
  if (!lineEdit_) {
    return 0;
  }

  const QPoint local = lineEdit_->mapFrom(this, QPoint(x, lineEdit_->height() / 2));
  const int pos = lineEdit_->cursorPositionAt(local);
  return std::clamp(pos, 0, static_cast<int>(lineEdit_->text().size()));
}

void AdInputNumber::focusFromMouseGlobalPos(const QPoint& globalPos, Qt::FocusReason reason) {
  if (disabled() || !lineEdit_) {
    return;
  }

  const QPoint editPos = lineEdit_->mapFromGlobal(globalPos);
  if (lineEdit_->rect().contains(editPos)) {
    lineEdit_->setCursorPosition(lineEdit_->cursorPositionAt(editPos));
  } else if (editPos.x() <= 0) {
    lineEdit_->setCursorPosition(0);
  } else if (editPos.x() >= lineEdit_->width()) {
    lineEdit_->setCursorPosition(lineEdit_->text().size());
  }

  lineEdit_->setFocus(reason);
  bumpJoinedZOrder();
}

void AdInputNumber::bumpJoinedZOrder() {
  if (!(joinedLeft_ || joinedRight_)) {
    return;
  }
  if (!(focused_ || hovered_)) {
    return;
  }
  raise();
}

}  // namespace adqt::widgets
