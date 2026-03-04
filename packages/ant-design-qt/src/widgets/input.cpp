#include "input.h"

#include "button.h"
#include "icons.h"
#include "input_style.h"
#include "interaction_overlay_manager.h"
#include "theme/fast_color_lite.h"
#include "theme/theme.h"

#include <QApplication>
#include <QCursor>
#include <QEvent>
#include <QFontMetrics>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLayoutItem>
#include <QLineEdit>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QTextEdit>
#include <QTextDocument>
#include <QTextOption>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <limits>

namespace adqt::widgets {

namespace {

namespace outlined_icons = adqt::icons::outlined;
namespace filled_icons = adqt::icons::filled;

bool iconStylesEqual(const adqt::icons::IconStyle& lhs, const adqt::icons::IconStyle& rhs) {
  return lhs.hasPrimary == rhs.hasPrimary && lhs.hasSecondary == rhs.hasSecondary &&
         lhs.hasTertiary == rhs.hasTertiary && lhs.primary == rhs.primary &&
         lhs.secondary == rhs.secondary && lhs.tertiary == rhs.tertiary;
}

bool iconTokensEqual(const adqt::icons::IconToken& lhs, const adqt::icons::IconToken& rhs) {
  return lhs.index == rhs.index && iconStylesEqual(lhs.style, rhs.style);
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

QColor parseThemeColor(const QString& value, const QColor& fallback) {
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

QPixmap renderTintedIcon(const adqt::icons::IconToken& token, const QColor& color, int side, qreal dpr) {
  if (!adqt::icons::isValid(token) || side <= 0) {
    return QPixmap();
  }
  adqt::icons::IconToken tinted = token;
  tinted.style.primary = color;
  tinted.style.hasPrimary = true;
  return adqt::icons::renderIconPixmap(tinted, QSize(side, side), dpr, QIcon::Normal, QIcon::Off);
}

int boundedCursorPosition(const QString& text, int requested) {
  return std::clamp(requested, 0, static_cast<int>(text.size()));
}

int lineHeightForFont(const QFont& font) {
  QFontMetrics fm(font);
  return std::max(1, fm.lineSpacing());
}

int textAreaRowPaddingExtra(const detail::InputVisualStyle& style) {
  const int verticalPadding = std::max(0, style.metrics.verticalPadding);
  return std::max(0, verticalPadding * 2 + 6);
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
  return QRectF(bounds).adjusted(leftInset, half + 0.5, -rightInset, -half - 0.5);
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

struct InputShellPaintStyle {
  QColor background;
  QColor border;
  qreal borderWidth = 1.0;
  qreal topLeftRadius = 0.0;
  qreal topRightRadius = 0.0;
  qreal bottomRightRadius = 0.0;
  qreal bottomLeftRadius = 0.0;
  bool underlined = false;
  bool joinedLeft = false;
  bool joinedRight = false;
};

class InputShellWidget final : public QWidget {
 public:
  explicit InputShellWidget(QWidget* parent = nullptr) : QWidget(parent) {
    setAttribute(Qt::WA_Hover, true);
    setMouseTracking(true);
  }

  void setPaintStyle(const InputShellPaintStyle& style) {
    paintStyle_ = style;
    update();
  }

 protected:
  void paintEvent(QPaintEvent* event) override {
    Q_UNUSED(event)

    const qreal borderWidth = std::max<qreal>(0.0, paintStyle_.borderWidth);
    const bool hasVisibleBorder = borderWidth > 0.0 && paintStyle_.border.alpha() > 0;
    const QRectF fillRect(rect());
    const QRectF rawBorderRect =
        joinedBorderRect(rect(), borderWidth, paintStyle_.joinedLeft, paintStyle_.joinedRight);

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

    if (paintStyle_.underlined) {
      if (paintStyle_.background.alpha() > 0) {
        painter.fillRect(fillRect, paintStyle_.background);
      }
      if (hasVisibleBorder) {
        QPen underlinePen(paintStyle_.border, borderWidth, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
        painter.setPen(underlinePen);
        painter.setBrush(Qt::NoBrush);
        const qreal underlineY = snapToDevicePixelCoord(borderRect.bottom(), dpr);
        painter.drawLine(QPointF(fillRect.left(), underlineY), QPointF(fillRect.right(), underlineY));
      }
      return;
    }

    const QPainterPath fillPath =
        roundedRectPath(fillRect, paintStyle_.topLeftRadius, paintStyle_.topRightRadius,
                        paintStyle_.bottomRightRadius, paintStyle_.bottomLeftRadius);

    if (paintStyle_.background.alpha() > 0) {
      painter.fillPath(fillPath, paintStyle_.background);

      if (!hasVisibleBorder && (paintStyle_.joinedLeft || paintStyle_.joinedRight)) {
        // Keep joined edges fully opaque to avoid anti-aliased seams with adjacent controls.
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.setPen(Qt::NoPen);
        painter.setBrush(paintStyle_.background);
        if (paintStyle_.joinedLeft) {
          painter.drawRect(QRect(0, 0, joinedEdgeClearWidth, height()));
        }
        if (paintStyle_.joinedRight) {
          painter.drawRect(QRect(std::max(0, width() - joinedEdgeClearWidth), 0, joinedEdgeClearWidth,
                                 height()));
        }
        painter.restore();
      }
    }

    if (hasVisibleBorder) {
      const QPainterPath borderPath =
          roundedRectPath(borderRect, paintStyle_.topLeftRadius, paintStyle_.topRightRadius,
                          paintStyle_.bottomRightRadius, paintStyle_.bottomLeftRadius);
      QPen borderPen(paintStyle_.border, borderWidth, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
      painter.setPen(borderPen);
      painter.setBrush(Qt::NoBrush);
      painter.drawPath(borderPath);

    }
  }

 private:
  InputShellPaintStyle paintStyle_;
};

}  // namespace

AdInput::AdInput(QWidget* parent) : QWidget(parent) {
  setAttribute(Qt::WA_Hover, true);
  setMouseTracking(true);

  rootLayout_ = new QVBoxLayout(this);
  rootLayout_->setContentsMargins(0, 0, 0, 0);
  rootLayout_->setSpacing(0);

  shell_ = new InputShellWidget(this);
  shell_->setObjectName(QStringLiteral("adinput-shell"));

  shellLayout_ = new QHBoxLayout(shell_);
  shellLayout_->setContentsMargins(0, 0, 0, 0);
  shellLayout_->setSpacing(6);

  prefixIconLabel_ = new QLabel(shell_);
  prefixIconLabel_->setAlignment(Qt::AlignCenter);
  prefixIconLabel_->setVisible(false);

  prefixLabel_ = new QLabel(shell_);
  prefixLabel_->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
  prefixLabel_->setVisible(false);

  lineEdit_ = new QLineEdit(shell_);
  lineEdit_->setFrame(false);
  lineEdit_->setClearButtonEnabled(false);
  lineEdit_->setAlignment(textAlignment_);
  lineEdit_->installEventFilter(this);

  clearButton_ = new QToolButton(shell_);
  clearButton_->setAutoRaise(true);
  clearButton_->setFocusPolicy(Qt::NoFocus);
  clearButton_->setVisible(false);
  clearButton_->installEventFilter(this);

  suffixLabel_ = new QLabel(shell_);
  suffixLabel_->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
  suffixLabel_->setVisible(false);

  suffixIconLabel_ = new QLabel(shell_);
  suffixIconLabel_->setAlignment(Qt::AlignCenter);
  suffixIconLabel_->setVisible(false);

  shellLayout_->addWidget(prefixIconLabel_);
  shellLayout_->addWidget(prefixLabel_);
  shellLayout_->addWidget(lineEdit_, 1);
  shellLayout_->addWidget(clearButton_);
  shellLayout_->addWidget(suffixLabel_);
  shellLayout_->addWidget(suffixIconLabel_);

  countLabel_ = new QLabel(this);
  countLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  countLabel_->setVisible(false);

  rootLayout_->addWidget(shell_);
  rootLayout_->addWidget(countLabel_);

  connect(lineEdit_, &QLineEdit::textChanged, this, [this](const QString& text) {
    updateCountLabel();
    updateClearButton();
    updateAccessoryVisibility();
    applyVisualStyle();

    if (lastValue_ != text) {
      lastValue_ = text;
      emit valueChanged(text);
    }
  });

  connect(lineEdit_, &QLineEdit::textEdited, this, [this](const QString& text) {
    if (countMax_ > 0 && exceedFormatter_ && effectiveCount(text) > countMax_) {
      const QString clipped = exceedFormatter_(text, countMax_);
      if (clipped != text) {
        internalTextUpdate_ = true;
        lineEdit_->setText(clipped);
        internalTextUpdate_ = false;
      }
    }
    emit textEdited(lineEdit_->text());
  });

  connect(lineEdit_, &QLineEdit::returnPressed, this, &AdInput::returnPressed);

  connect(clearButton_, &QToolButton::clicked, this, [this]() {
    if (!lineEdit_ || lineEdit_->text().isEmpty()) {
      return;
    }
    internalTextUpdate_ = true;
    lineEdit_->clear();
    internalTextUpdate_ = false;
    emit cleared();
    focusInput(FocusCursor::Start);
  });

  connect(&adqt::theme::ThemeManager::instance(), &adqt::theme::ThemeManager::themeChanged, this,
          [this]() { applyVisualStyle(); });

  if (lineEdit_) {
    lineEdit_->setMaxLength(std::numeric_limits<int>::max());
  }

  lastValue_ = value();
  updateAccessoryVisibility();
  updateCountLabel();
  applyVisualStyle();
}

AdInput::~AdInput() { stopInteractionFocusForOwner(this); }

AdInput::Size AdInput::size() const { return size_; }

void AdInput::setSize(Size value) {
  if (size_ == value) {
    return;
  }
  size_ = value;
  applyVisualStyle();
  updateGeometry();
  emit sizeChanged(size_);
}

AdInput::Variant AdInput::variant() const { return variant_; }

void AdInput::setVariant(Variant value) {
  if (variant_ == value) {
    return;
  }
  variant_ = value;
  applyVisualStyle();
  update();
  emit variantChanged(variant_);
}

AdInput::Status AdInput::status() const { return status_; }

void AdInput::setStatus(Status value) {
  if (status_ == value) {
    return;
  }
  status_ = value;
  applyVisualStyle();
  update();
  emit statusChanged(status_);
}

bool AdInput::disabled() const { return !isEnabled(); }

void AdInput::setDisabled(bool value) {
  if (disabled() == value) {
    return;
  }
  QWidget::setDisabled(value);
  if (lineEdit_) {
    lineEdit_->setDisabled(value);
  }
  if (clearButton_) {
    clearButton_->setDisabled(value);
  }
  updateClearButton();
  applyVisualStyle();
  emit disabledChanged(value);
}

bool AdInput::allowClear() const { return allowClear_; }

void AdInput::setAllowClear(bool value) {
  if (allowClear_ == value) {
    return;
  }
  allowClear_ = value;
  updateClearButton();
  updateAccessoryVisibility();
  emit allowClearChanged(allowClear_);
}

QString AdInput::placeholder() const { return lineEdit_ ? lineEdit_->placeholderText() : QString(); }

void AdInput::setPlaceholder(const QString& value) {
  if (!lineEdit_ || lineEdit_->placeholderText() == value) {
    return;
  }
  lineEdit_->setPlaceholderText(value);
  applyVisualStyle();
  emit placeholderChanged(value);
}

QString AdInput::value() const { return lineEdit_ ? lineEdit_->text() : QString(); }

void AdInput::setValue(const QString& value) {
  if (!lineEdit_) {
    return;
  }

  QString next = value;
  if (maxLength_ > 0 && next.size() > maxLength_) {
    next = next.left(maxLength_);
  }
  if (countMax_ > 0 && exceedFormatter_ && effectiveCount(next) > countMax_) {
    next = exceedFormatter_(next, countMax_);
  }

  if (lineEdit_->text() == next) {
    return;
  }

  internalTextUpdate_ = true;
  lineEdit_->setText(next);
  internalTextUpdate_ = false;
}

int AdInput::maxLength() const { return maxLength_; }

void AdInput::setMaxLength(int value) {
  const int normalized = value < 0 ? -1 : value;
  if (maxLength_ == normalized) {
    return;
  }
  maxLength_ = normalized;

  if (lineEdit_) {
    lineEdit_->setMaxLength(maxLength_ > 0 ? maxLength_ : std::numeric_limits<int>::max());
    if (maxLength_ > 0 && lineEdit_->text().size() > maxLength_) {
      setValue(lineEdit_->text().left(maxLength_));
    }
  }

  updateCountLabel();
  applyVisualStyle();
  emit maxLengthChanged(maxLength_);
}

QString AdInput::prefixText() const { return prefixText_; }

void AdInput::setPrefixText(const QString& value) {
  if (prefixText_ == value) {
    return;
  }
  prefixText_ = value;
  updatePrefixVisual();
  updateAccessoryVisibility();
  emit prefixTextChanged(prefixText_);
}

QString AdInput::suffixText() const { return suffixText_; }

void AdInput::setSuffixText(const QString& value) {
  if (suffixText_ == value) {
    return;
  }
  suffixText_ = value;
  updateSuffixVisual();
  updateAccessoryVisibility();
  emit suffixTextChanged(suffixText_);
}

bool AdInput::showCount() const { return showCount_; }

void AdInput::setShowCount(bool value) {
  if (showCount_ == value) {
    return;
  }
  showCount_ = value;
  updateCountLabel();
  applyVisualStyle();
  emit showCountChanged(showCount_);
}

int AdInput::countMax() const { return countMax_; }

void AdInput::setCountMax(int value) {
  const int normalized = value < 0 ? -1 : value;
  if (countMax_ == normalized) {
    return;
  }
  countMax_ = normalized;
  updateCountLabel();
  applyVisualStyle();
  emit countMaxChanged(countMax_);
}

Qt::Alignment AdInput::textAlignment() const { return textAlignment_; }

void AdInput::setTextAlignment(Qt::Alignment value) {
  if (textAlignment_ == value) {
    return;
  }
  textAlignment_ = value;
  if (lineEdit_) {
    lineEdit_->setAlignment(textAlignment_);
  }
  emit textAlignmentChanged(textAlignment_);
}

bool AdInput::joinedLeft() const { return joinedLeft_; }

void AdInput::setJoinedLeft(bool value) {
  if (joinedLeft_ == value) {
    return;
  }
  joinedLeft_ = value;
  applyVisualStyle();
}

bool AdInput::joinedRight() const { return joinedRight_; }

void AdInput::setJoinedRight(bool value) {
  if (joinedRight_ == value) {
    return;
  }
  joinedRight_ = value;
  applyVisualStyle();
}

QLineEdit::EchoMode AdInput::echoMode() const {
  return lineEdit_ ? lineEdit_->echoMode() : QLineEdit::Normal;
}

void AdInput::setEchoMode(QLineEdit::EchoMode value) {
  if (!lineEdit_ || lineEdit_->echoMode() == value) {
    return;
  }
  lineEdit_->setEchoMode(value);
  emit echoModeChanged(value);
}

adqt::icons::IconToken AdInput::prefixIconToken() const { return prefixIconToken_; }

void AdInput::setPrefixIconToken(const adqt::icons::IconToken& token) {
  if (iconTokensEqual(prefixIconToken_, token)) {
    return;
  }
  prefixIconToken_ = token;
  updatePrefixVisual();
  updateAccessoryVisibility();
  emit prefixIconTokenChanged(prefixIconToken_);
}

adqt::icons::IconToken AdInput::suffixIconToken() const { return suffixIconToken_; }

void AdInput::setSuffixIconToken(const adqt::icons::IconToken& token) {
  if (iconTokensEqual(suffixIconToken_, token)) {
    return;
  }
  suffixIconToken_ = token;
  updateSuffixVisual();
  updateAccessoryVisibility();
  emit suffixIconTokenChanged(suffixIconToken_);
}

AdInput::CountStrategy AdInput::countStrategy() const { return countStrategy_; }

void AdInput::setCountStrategy(CountStrategy value) {
  countStrategy_ = std::move(value);
  updateCountLabel();
  applyVisualStyle();
}

AdInput::CountFormatter AdInput::countFormatter() const { return countFormatter_; }

void AdInput::setCountFormatter(CountFormatter value) {
  countFormatter_ = std::move(value);
  updateCountLabel();
}

AdInput::ExceedFormatter AdInput::exceedFormatter() const { return exceedFormatter_; }

void AdInput::setExceedFormatter(ExceedFormatter value) { exceedFormatter_ = std::move(value); }

AdInput::ComponentTokens AdInput::componentTokens() const { return componentTokens_; }

void AdInput::setComponentTokens(const ComponentTokens& tokens) {
  componentTokens_ = tokens;
  applyVisualStyle();
  emit componentTokensChanged();
}

void AdInput::resetComponentTokens() {
  componentTokens_ = ComponentTokens();
  applyVisualStyle();
  emit componentTokensChanged();
}

AdInput::SemanticStyles AdInput::semanticStyles() const { return semanticStyles_; }

void AdInput::setSemanticStyles(const SemanticStyles& styles) {
  semanticStyles_ = styles;
  applyVisualStyle();
  emit semanticStylesChanged();
}

void AdInput::setSemanticStyleResolver(SemanticStyleResolver resolver) {
  semanticStyleResolver_ = std::move(resolver);
  applyVisualStyle();
  emit semanticStylesChanged();
}

QSize AdInput::sizeHint() const {
  detail::InputStyleInput styleInput;
  styleInput.size = size_;
  styleInput.variant = variant_;
  styleInput.status = status_;
  styleInput.disabled = disabled();
  styleInput.focused = focused_;
  styleInput.hovered = hovered_;
  styleInput.baseFont = font();
  styleInput.componentTokens = componentTokens_;
  styleInput.semanticStyles = semanticStyles_;
  const detail::InputVisualStyle style = detail::resolveInputVisualStyle(styleInput);

  const bool showCountRow = showCount_ || effectiveCountMax() > 0;
  const int countHeight = showCountRow ? style.metrics.countTopMargin + style.metrics.countHeight : 0;

  int widthHint = std::max(160, style.metrics.height * 4);
  if (lineEdit_) {
    QFontMetrics fm(style.metrics.font);
    widthHint = std::max(widthHint, fm.horizontalAdvance(lineEdit_->placeholderText()) + 96);
  }

  return QSize(widthHint, style.metrics.height + countHeight);
}

QSize AdInput::minimumSizeHint() const {
  const QSize hint = sizeHint();
  return QSize(std::min(96, hint.width()), hint.height());
}

void AdInput::focusInput(FocusCursor cursor, bool preventScroll) {
  Q_UNUSED(preventScroll)
  if (!lineEdit_) {
    return;
  }

  lineEdit_->setFocus(Qt::OtherFocusReason);
  const QString text = lineEdit_->text();
  if (cursor == FocusCursor::Start) {
    lineEdit_->setCursorPosition(0);
  } else if (cursor == FocusCursor::End) {
    lineEdit_->setCursorPosition(text.size());
  } else if (cursor == FocusCursor::All) {
    lineEdit_->selectAll();
  }
}

void AdInput::blurInput() {
  if (lineEdit_) {
    lineEdit_->clearFocus();
  }
}

QLineEdit* AdInput::lineEdit() const { return lineEdit_; }

bool AdInput::eventFilter(QObject* watched, QEvent* event) {
  if (!event) {
    return QWidget::eventFilter(watched, event);
  }

  if (watched == lineEdit_) {
    if (event->type() == QEvent::FocusIn) {
      focused_ = true;
      updateClearButton();
      applyVisualStyle();
    } else if (event->type() == QEvent::FocusOut) {
      focused_ = false;
      updateClearButton();
      applyVisualStyle();
    }
  } else if (watched == clearButton_) {
    if (event->type() == QEvent::Enter || event->type() == QEvent::Leave) {
      updateClearButton();
    }
  }

  return QWidget::eventFilter(watched, event);
}

void AdInput::enterEvent(QEnterEvent* event) {
  hovered_ = true;
  updateClearButton();
  applyVisualStyle();
  QWidget::enterEvent(event);
}

void AdInput::leaveEvent(QEvent* event) {
  hovered_ = false;
  updateClearButton();
  applyVisualStyle();
  QWidget::leaveEvent(event);
}

void AdInput::paintEvent(QPaintEvent* event) { QWidget::paintEvent(event); }

void AdInput::changeEvent(QEvent* event) {
  QWidget::changeEvent(event);
  if (!event) {
    return;
  }

  if (event->type() == QEvent::EnabledChange || event->type() == QEvent::FontChange ||
      event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange) {
    updateClearButton();
    applyVisualStyle();
  }
}

void AdInput::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  updateInteractionFocusOverlay();
}

void AdInput::updateCountLabel() {
  if (!countLabel_ || !lineEdit_) {
    return;
  }

  const int count = effectiveCount(lineEdit_->text());
  const int max = effectiveCountMax();
  const bool visible = showCount_ || max > 0;
  countLabel_->setVisible(visible);
  if (!visible) {
    return;
  }

  QString formatted;
  if (countFormatter_) {
    formatted = countFormatter_(lineEdit_->text(), count, max);
  } else if (max > 0) {
    formatted = QStringLiteral("%1 / %2").arg(count).arg(max);
  } else {
    formatted = QString::number(count);
  }
  countLabel_->setText(formatted);
}

void AdInput::updateClearButton() {
  if (!clearButton_ || !lineEdit_) {
    return;
  }

  const bool canShow =
      allowClear_ && !disabled() && !lineEdit_->text().isEmpty();
  clearButton_->setVisible(canShow);

  StyleContext context;
  context.size = size_;
  context.variant = variant_;
  context.status = status_;
  context.disabled = disabled();
  context.focused = focused_;
  context.hovered = hovered_;
  context.showCount = showCount_;
  context.valueLength = value().size();
  context.count = effectiveCount(value());
  context.countMax = effectiveCountMax();

  SemanticStyles effectiveSemantic = semanticStyles_;
  if (semanticStyleResolver_) {
    effectiveSemantic = semanticStyleResolver_(context);
  }

  detail::InputStyleInput styleInput;
  styleInput.size = size_;
  styleInput.variant = variant_;
  styleInput.status = status_;
  styleInput.disabled = disabled();
  styleInput.focused = focused_;
  styleInput.hovered = hovered_;
  styleInput.baseFont = font();
  styleInput.componentTokens = componentTokens_;
  styleInput.semanticStyles = effectiveSemantic;
  const detail::InputVisualStyle style = detail::resolveInputVisualStyle(styleInput);

  const int iconSide = std::max(10, style.metrics.iconSize);
  const QColor color =
      (clearButton_->underMouse() && canShow && !disabled()) ? style.clearHoverColor : style.clearColor;
  const QPixmap px = renderTintedIcon(filled_icons::CloseCircle(), color, iconSide, devicePixelRatioF());
  clearButton_->setIcon(QIcon(px));
  clearButton_->setIconSize(QSize(iconSide, iconSide));
  clearButton_->setFixedSize(iconSide, iconSide);
  clearButton_->setText(px.isNull() ? QStringLiteral("x") : QString());
  clearButton_->setStyleSheet(
      QStringLiteral("QToolButton { border: none; padding: 0; background: transparent; }"));
}

void AdInput::updateAccessoryVisibility() {
  if (!prefixLabel_ || !prefixIconLabel_ || !suffixLabel_ || !suffixIconLabel_) {
    return;
  }

  prefixLabel_->setVisible(!prefixText_.trimmed().isEmpty());
  prefixIconLabel_->setVisible(adqt::icons::isValid(prefixIconToken_));
  suffixLabel_->setVisible(!suffixText_.trimmed().isEmpty());
  suffixIconLabel_->setVisible(adqt::icons::isValid(suffixIconToken_));
}

void AdInput::updatePrefixVisual() {
  if (!prefixLabel_ || !prefixIconLabel_) {
    return;
  }

  StyleContext context;
  context.size = size_;
  context.variant = variant_;
  context.status = status_;
  context.disabled = disabled();
  context.focused = focused_;
  context.hovered = hovered_;
  context.showCount = showCount_;
  context.valueLength = value().size();
  context.count = effectiveCount(value());
  context.countMax = effectiveCountMax();

  SemanticStyles effectiveSemantic = semanticStyles_;
  if (semanticStyleResolver_) {
    effectiveSemantic = semanticStyleResolver_(context);
  }

  detail::InputStyleInput styleInput;
  styleInput.size = size_;
  styleInput.variant = variant_;
  styleInput.status = status_;
  styleInput.disabled = disabled();
  styleInput.focused = focused_;
  styleInput.hovered = hovered_;
  styleInput.baseFont = font();
  styleInput.componentTokens = componentTokens_;
  styleInput.semanticStyles = effectiveSemantic;
  const detail::InputVisualStyle style = detail::resolveInputVisualStyle(styleInput);

  prefixLabel_->setText(prefixText_);
  QPalette labelPalette = prefixLabel_->palette();
  labelPalette.setColor(QPalette::WindowText, style.prefixColor);
  prefixLabel_->setPalette(labelPalette);

  int iconSide = std::max(10, style.metrics.font.pixelSize());
  if (componentTokens_.iconSize.has_value()) {
    iconSide = std::max(8, componentTokens_.iconSize.value());
  }
  const QPixmap px = renderTintedIcon(prefixIconToken_, style.prefixColor, iconSide, devicePixelRatioF());
  prefixIconLabel_->setPixmap(px);
  prefixIconLabel_->setFixedSize(iconSide, iconSide);
}

void AdInput::updateSuffixVisual() {
  if (!suffixLabel_ || !suffixIconLabel_) {
    return;
  }

  StyleContext context;
  context.size = size_;
  context.variant = variant_;
  context.status = status_;
  context.disabled = disabled();
  context.focused = focused_;
  context.hovered = hovered_;
  context.showCount = showCount_;
  context.valueLength = value().size();
  context.count = effectiveCount(value());
  context.countMax = effectiveCountMax();

  SemanticStyles effectiveSemantic = semanticStyles_;
  if (semanticStyleResolver_) {
    effectiveSemantic = semanticStyleResolver_(context);
  }

  detail::InputStyleInput styleInput;
  styleInput.size = size_;
  styleInput.variant = variant_;
  styleInput.status = status_;
  styleInput.disabled = disabled();
  styleInput.focused = focused_;
  styleInput.hovered = hovered_;
  styleInput.baseFont = font();
  styleInput.componentTokens = componentTokens_;
  styleInput.semanticStyles = effectiveSemantic;
  const detail::InputVisualStyle style = detail::resolveInputVisualStyle(styleInput);

  suffixLabel_->setText(suffixText_);
  QPalette labelPalette = suffixLabel_->palette();
  labelPalette.setColor(QPalette::WindowText, style.suffixColor);
  suffixLabel_->setPalette(labelPalette);

  int iconSide = std::max(10, style.metrics.font.pixelSize());
  if (componentTokens_.iconSize.has_value()) {
    iconSide = std::max(8, componentTokens_.iconSize.value());
  }
  const QPixmap px = renderTintedIcon(suffixIconToken_, style.suffixColor, iconSide, devicePixelRatioF());
  suffixIconLabel_->setPixmap(px);
  suffixIconLabel_->setFixedSize(iconSide, iconSide);
}

void AdInput::applyVisualStyle() {
  if (!shell_ || !shellLayout_ || !lineEdit_ || !countLabel_) {
    return;
  }

  StyleContext context;
  context.size = size_;
  context.variant = variant_;
  context.status = status_;
  context.disabled = disabled();
  context.focused = focused_;
  context.hovered = hovered_;
  context.showCount = showCount_;
  context.valueLength = value().size();
  context.count = effectiveCount(value());
  context.countMax = effectiveCountMax();

  SemanticStyles effectiveSemantic = semanticStyles_;
  if (semanticStyleResolver_) {
    effectiveSemantic = semanticStyleResolver_(context);
  }

  detail::InputStyleInput styleInput;
  styleInput.size = size_;
  styleInput.variant = variant_;
  styleInput.status = status_;
  styleInput.disabled = disabled();
  styleInput.focused = focused_;
  styleInput.hovered = hovered_;
  styleInput.baseFont = font();
  styleInput.componentTokens = componentTokens_;
  styleInput.semanticStyles = effectiveSemantic;

  if (status_ == Status::None && effectiveCountMax() > 0 && effectiveCount(value()) > effectiveCountMax()) {
    styleInput.status = Status::Warning;
  }

  const detail::InputVisualStyle style = detail::resolveInputVisualStyle(styleInput);

  setFont(style.metrics.font);
  lineEdit_->setFont(style.metrics.font);
  prefixLabel_->setFont(style.metrics.font);
  suffixLabel_->setFont(style.metrics.font);

  const int borderInset = std::max(0, style.metrics.borderWidth);
  const int hp = std::max(0, style.metrics.horizontalPadding);
  shellLayout_->setContentsMargins(hp + borderInset, 0, hp + borderInset, 0);
  shellLayout_->setSpacing(std::max(4, hp / 2));
  shell_->setFixedHeight(style.metrics.height);

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

  InputShellPaintStyle shellPaintStyle;
  shellPaintStyle.background = background;
  shellPaintStyle.border = border;
  shellPaintStyle.borderWidth = std::max(0, style.metrics.borderWidth);
  const qreal cornerRadius = style.underlined ? 0.0 : std::max<qreal>(0.0, style.metrics.borderRadius);
  shellPaintStyle.topLeftRadius = joinedLeft_ ? 0.0 : cornerRadius;
  shellPaintStyle.topRightRadius = joinedRight_ ? 0.0 : cornerRadius;
  shellPaintStyle.bottomRightRadius = joinedRight_ ? 0.0 : cornerRadius;
  shellPaintStyle.bottomLeftRadius = joinedLeft_ ? 0.0 : cornerRadius;
  shellPaintStyle.underlined = style.underlined;
  shellPaintStyle.joinedLeft = joinedLeft_;
  shellPaintStyle.joinedRight = joinedRight_;
  static_cast<InputShellWidget*>(shell_)->setPaintStyle(shellPaintStyle);
  if (!shell_->styleSheet().isEmpty()) {
    shell_->setStyleSheet(QString());
  }

  QPalette linePalette = lineEdit_->palette();
  linePalette.setColor(QPalette::Text, style.selectorTextColor);
  linePalette.setColor(QPalette::Disabled, QPalette::Text, style.disabledTextColor);
  linePalette.setColor(QPalette::PlaceholderText, style.placeholderColor);
  lineEdit_->setPalette(linePalette);
  lineEdit_->setStyleSheet(QStringLiteral("QLineEdit { border: none; background: transparent; padding: 0; }"));

  QFont countFont = style.metrics.font;
  countFont.setPixelSize(std::max(10, style.metrics.font.pixelSize() - 1));
  countLabel_->setFont(countFont);
  countLabel_->setMinimumHeight(style.metrics.countHeight);
  QPalette countPalette = countLabel_->palette();
  countPalette.setColor(QPalette::WindowText, style.countColor);
  countLabel_->setPalette(countPalette);

  updatePrefixVisual();
  updateSuffixVisual();
  updateClearButton();
  updateCountLabel();
  updateInteractionFocusOverlay();
  update();
}

void AdInput::updateInteractionFocusOverlay() {
  if (!focused_ || disabled() || !isVisible() || !shell_) {
    stopInteractionFocusForOwner(this);
    return;
  }

  StyleContext context;
  context.size = size_;
  context.variant = variant_;
  context.status = status_;
  context.disabled = disabled();
  context.focused = focused_;
  context.hovered = hovered_;
  context.showCount = showCount_;
  context.valueLength = value().size();
  context.count = effectiveCount(value());
  context.countMax = effectiveCountMax();

  SemanticStyles effectiveSemantic = semanticStyles_;
  if (semanticStyleResolver_) {
    effectiveSemantic = semanticStyleResolver_(context);
  }

  detail::InputStyleInput styleInput;
  styleInput.size = size_;
  styleInput.variant = variant_;
  styleInput.status = status_;
  styleInput.disabled = disabled();
  styleInput.focused = focused_;
  styleInput.hovered = hovered_;
  styleInput.baseFont = font();
  styleInput.componentTokens = componentTokens_;
  styleInput.semanticStyles = effectiveSemantic;
  const detail::InputVisualStyle style = detail::resolveInputVisualStyle(styleInput);

  if (style.selectorFocusOutlineColor.alpha() <= 0 || style.metrics.focusOutlineWidth <= 0.0) {
    stopInteractionFocusForOwner(this);
    return;
  }

  QWidget* hostWindow = window();
  if (!hostWindow) {
    return;
  }

  const QPoint origin = mapTo(hostWindow, shell_->geometry().topLeft());
  QRectF baseRect =
      joinedBorderRect(QRect(QPoint(0, 0), shell_->size()), style.metrics.borderWidth, joinedLeft_, joinedRight_);
  baseRect.translate(origin.x(), origin.y());

  InteractionFocusRequest request;
  request.owner = this;
  request.baseRectInWindow = baseRect;
  const qreal focusRadius = style.underlined ? 0.0 : std::max<qreal>(0.0, style.metrics.borderRadius);
  request.topLeft = joinedLeft_ ? 0.0 : focusRadius;
  request.topRight = joinedRight_ ? 0.0 : focusRadius;
  request.bottomRight = joinedRight_ ? 0.0 : focusRadius;
  request.bottomLeft = joinedLeft_ ? 0.0 : focusRadius;
  request.color = style.selectorFocusOutlineColor;
  request.strokeWidth = std::max<qreal>(1.0, style.metrics.focusOutlineWidth);
  request.offset = std::max<qreal>(0.0, style.metrics.focusOutlineOffset);
  triggerInteractionFocus(request);
}

int AdInput::effectiveCount(const QString& text) const {
  if (countStrategy_) {
    return std::max(0, countStrategy_(text));
  }
  return text.size();
}

int AdInput::effectiveCountMax() const {
  if (countMax_ > 0) {
    return countMax_;
  }
  if (maxLength_ > 0) {
    return maxLength_;
  }
  return -1;
}

AdInputTextArea::AdInputTextArea(QWidget* parent) : QWidget(parent) {
  setAttribute(Qt::WA_Hover, true);
  setMouseTracking(true);
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

  rootLayout_ = new QVBoxLayout(this);
  rootLayout_->setContentsMargins(0, 0, 0, 0);
  rootLayout_->setSpacing(0);

  shell_ = new InputShellWidget(this);
  shell_->setObjectName(QStringLiteral("adinput-textarea-shell"));
  shell_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

  shellLayout_ = new QHBoxLayout(shell_);
  shellLayout_->setContentsMargins(0, 0, 0, 0);
  shellLayout_->setSpacing(6);

  textEdit_ = new QTextEdit(shell_);
  textEdit_->setFrameStyle(QFrame::NoFrame);
  textEdit_->setAcceptRichText(false);
  textEdit_->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
  textEdit_->setAttribute(Qt::WA_Hover, true);
  textEdit_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  textEdit_->installEventFilter(this);

  clearButton_ = new QToolButton(shell_);
  clearButton_->setAutoRaise(true);
  clearButton_->setFocusPolicy(Qt::NoFocus);
  clearButton_->setVisible(false);
  clearButton_->installEventFilter(this);

  shellLayout_->addWidget(textEdit_, 1);
  shellLayout_->addWidget(clearButton_);

  countLabel_ = new QLabel(this);
  countLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  countLabel_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  countLabel_->setVisible(false);

  rootLayout_->addWidget(shell_);
  rootLayout_->addWidget(countLabel_);

  connect(textEdit_, &QTextEdit::textChanged, this, [this]() {
    QString text = textEdit_->toPlainText();
    bool clipped = false;

    if (!internalTextUpdate_ && maxLength_ > 0 && text.size() > maxLength_) {
      text = text.left(maxLength_);
      clipped = true;
    }
    if (!internalTextUpdate_ && countMax_ > 0 && exceedFormatter_ && effectiveCount(text) > countMax_) {
      text = exceedFormatter_(text, countMax_);
      clipped = true;
    }

    if (clipped) {
      internalTextUpdate_ = true;
      const int oldPos = textEdit_->textCursor().position();
      textEdit_->setPlainText(text);
      QTextCursor cursor = textEdit_->textCursor();
      cursor.setPosition(boundedCursorPosition(text, oldPos));
      textEdit_->setTextCursor(cursor);
      internalTextUpdate_ = false;
    }

    updateCountLabel();
    updateClearButton();
    updateAutoSize();
    applyVisualStyle();

    const QString current = textEdit_->toPlainText();
    if (lastValue_ != current) {
      lastValue_ = current;
      emit valueChanged(current);
    }
    if (!internalTextUpdate_) {
      emit textEdited(current);
    }
  });

  connect(clearButton_, &QToolButton::clicked, this, [this]() {
    if (!textEdit_ || textEdit_->toPlainText().isEmpty()) {
      return;
    }
    internalTextUpdate_ = true;
    textEdit_->clear();
    internalTextUpdate_ = false;
    emit cleared();
    focusInput(FocusCursor::Start);
  });

  connect(&adqt::theme::ThemeManager::instance(), &adqt::theme::ThemeManager::themeChanged, this,
          [this]() { applyVisualStyle(); });

  lastValue_ = value();
  updateCountLabel();
  updateClearButton();
  updateAutoSize();
  applyVisualStyle();
}

AdInputTextArea::~AdInputTextArea() { stopInteractionFocusForOwner(this); }

AdInputTextArea::Size AdInputTextArea::size() const { return size_; }

void AdInputTextArea::setSize(Size value) {
  if (size_ == value) {
    return;
  }
  size_ = value;
  updateAutoSize();
  applyVisualStyle();
  updateGeometry();
  emit sizeChanged(size_);
}

AdInputTextArea::Variant AdInputTextArea::variant() const { return variant_; }

void AdInputTextArea::setVariant(Variant value) {
  if (variant_ == value) {
    return;
  }
  variant_ = value;
  applyVisualStyle();
  emit variantChanged(variant_);
}

AdInputTextArea::Status AdInputTextArea::status() const { return status_; }

void AdInputTextArea::setStatus(Status value) {
  if (status_ == value) {
    return;
  }
  status_ = value;
  applyVisualStyle();
  emit statusChanged(status_);
}

bool AdInputTextArea::disabled() const { return !isEnabled(); }

void AdInputTextArea::setDisabled(bool value) {
  if (disabled() == value) {
    return;
  }
  QWidget::setDisabled(value);
  if (textEdit_) {
    textEdit_->setDisabled(value);
  }
  if (clearButton_) {
    clearButton_->setDisabled(value);
  }
  updateClearButton();
  applyVisualStyle();
  emit disabledChanged(value);
}

bool AdInputTextArea::allowClear() const { return allowClear_; }

void AdInputTextArea::setAllowClear(bool value) {
  if (allowClear_ == value) {
    return;
  }
  allowClear_ = value;
  updateClearButton();
  emit allowClearChanged(allowClear_);
}

QString AdInputTextArea::placeholder() const { return textEdit_ ? textEdit_->placeholderText() : QString(); }

void AdInputTextArea::setPlaceholder(const QString& value) {
  if (!textEdit_ || textEdit_->placeholderText() == value) {
    return;
  }
  textEdit_->setPlaceholderText(value);
  applyVisualStyle();
  emit placeholderChanged(value);
}

QString AdInputTextArea::value() const { return textEdit_ ? textEdit_->toPlainText() : QString(); }

void AdInputTextArea::setValue(const QString& value) {
  if (!textEdit_) {
    return;
  }

  QString next = value;
  if (maxLength_ > 0 && next.size() > maxLength_) {
    next = next.left(maxLength_);
  }
  if (countMax_ > 0 && exceedFormatter_ && effectiveCount(next) > countMax_) {
    next = exceedFormatter_(next, countMax_);
  }

  if (textEdit_->toPlainText() == next) {
    return;
  }

  internalTextUpdate_ = true;
  textEdit_->setPlainText(next);
  internalTextUpdate_ = false;
}

int AdInputTextArea::maxLength() const { return maxLength_; }

void AdInputTextArea::setMaxLength(int value) {
  const int normalized = value < 0 ? -1 : value;
  if (maxLength_ == normalized) {
    return;
  }
  maxLength_ = normalized;

  if (maxLength_ > 0 && textEdit_ && textEdit_->toPlainText().size() > maxLength_) {
    setValue(textEdit_->toPlainText().left(maxLength_));
  }

  updateCountLabel();
  applyVisualStyle();
  emit maxLengthChanged(maxLength_);
}

bool AdInputTextArea::showCount() const { return showCount_; }

void AdInputTextArea::setShowCount(bool value) {
  if (showCount_ == value) {
    return;
  }
  showCount_ = value;
  updateCountLabel();
  applyVisualStyle();
  updateGeometry();
  emit showCountChanged(showCount_);
}

int AdInputTextArea::countMax() const { return countMax_; }

void AdInputTextArea::setCountMax(int value) {
  const int normalized = value < 0 ? -1 : value;
  if (countMax_ == normalized) {
    return;
  }
  countMax_ = normalized;
  updateCountLabel();
  applyVisualStyle();
  emit countMaxChanged(countMax_);
}

bool AdInputTextArea::autoSizeEnabled() const { return autoSizeEnabled_; }

void AdInputTextArea::setAutoSizeEnabled(bool value) {
  if (autoSizeEnabled_ == value) {
    return;
  }
  autoSizeEnabled_ = value;
  updateAutoSize();
  applyVisualStyle();
  emit autoSizeEnabledChanged(autoSizeEnabled_);
}

int AdInputTextArea::autoSizeMinRows() const { return autoSizeMinRows_; }

void AdInputTextArea::setAutoSizeMinRows(int value) {
  const int normalized = std::max(1, value);
  if (autoSizeMinRows_ == normalized) {
    return;
  }
  autoSizeMinRows_ = normalized;
  if (autoSizeMaxRows_ < autoSizeMinRows_) {
    autoSizeMaxRows_ = autoSizeMinRows_;
    emit autoSizeMaxRowsChanged(autoSizeMaxRows_);
  }
  updateAutoSize();
  emit autoSizeMinRowsChanged(autoSizeMinRows_);
}

int AdInputTextArea::autoSizeMaxRows() const { return autoSizeMaxRows_; }

void AdInputTextArea::setAutoSizeMaxRows(int value) {
  const int normalized = std::max(autoSizeMinRows_, value);
  if (autoSizeMaxRows_ == normalized) {
    return;
  }
  autoSizeMaxRows_ = normalized;
  updateAutoSize();
  emit autoSizeMaxRowsChanged(autoSizeMaxRows_);
}

AdInputTextArea::CountStrategy AdInputTextArea::countStrategy() const { return countStrategy_; }

void AdInputTextArea::setCountStrategy(CountStrategy value) {
  countStrategy_ = std::move(value);
  updateCountLabel();
  applyVisualStyle();
}

AdInputTextArea::CountFormatter AdInputTextArea::countFormatter() const { return countFormatter_; }

void AdInputTextArea::setCountFormatter(CountFormatter value) {
  countFormatter_ = std::move(value);
  updateCountLabel();
}

AdInputTextArea::ExceedFormatter AdInputTextArea::exceedFormatter() const { return exceedFormatter_; }

void AdInputTextArea::setExceedFormatter(ExceedFormatter value) { exceedFormatter_ = std::move(value); }

AdInputTextArea::ComponentTokens AdInputTextArea::componentTokens() const { return componentTokens_; }

void AdInputTextArea::setComponentTokens(const ComponentTokens& tokens) {
  componentTokens_ = tokens;
  applyVisualStyle();
  emit componentTokensChanged();
}

void AdInputTextArea::resetComponentTokens() {
  componentTokens_ = ComponentTokens();
  applyVisualStyle();
  emit componentTokensChanged();
}

AdInputTextArea::SemanticStyles AdInputTextArea::semanticStyles() const { return semanticStyles_; }

void AdInputTextArea::setSemanticStyles(const SemanticStyles& styles) {
  semanticStyles_ = styles;
  applyVisualStyle();
  emit semanticStylesChanged();
}

void AdInputTextArea::setSemanticStyleResolver(SemanticStyleResolver resolver) {
  semanticStyleResolver_ = std::move(resolver);
  applyVisualStyle();
  emit semanticStylesChanged();
}

QSize AdInputTextArea::sizeHint() const {
  detail::InputStyleInput styleInput;
  styleInput.size = size_;
  styleInput.variant = variant_;
  styleInput.status = status_;
  styleInput.disabled = disabled();
  styleInput.focused = focused_;
  styleInput.hovered = hovered_;
  styleInput.multiline = true;
  styleInput.baseFont = font();
  styleInput.componentTokens = componentTokens_;
  styleInput.semanticStyles = semanticStyles_;
  const detail::InputVisualStyle style = detail::resolveInputVisualStyle(styleInput);

  const int lineHeight = lineHeightForFont(style.metrics.font);
  const int minRows = std::max(1, autoSizeMinRows_);
  const int maxRows = std::max(minRows, autoSizeMaxRows_);
  const int rowPaddingExtra = textAreaRowPaddingExtra(style);

  int desired = std::max(style.metrics.height * 2, minRows * lineHeight + rowPaddingExtra);
  if (autoSizeEnabled_) {
    desired = minRows * lineHeight + rowPaddingExtra;
  }
  const int minHeight = minRows * lineHeight + rowPaddingExtra;
  const int maxHeight = maxRows * lineHeight + rowPaddingExtra;
  desired = std::clamp(desired, minHeight, maxHeight);
  desired = std::max(desired, style.metrics.height);

  const int borderInset = std::max(0, style.metrics.borderWidth);
  int shellHeight = desired + borderInset * 2;
  if (shell_ && shell_->minimumHeight() > 0) {
    shellHeight = std::max(shellHeight, shell_->minimumHeight());
  }

  const int countHeight = showCount_ ? style.metrics.countTopMargin + style.metrics.countHeight : 0;
  return QSize(std::max(220, style.metrics.height * 5), shellHeight + countHeight);
}

QSize AdInputTextArea::minimumSizeHint() const {
  const QSize hint = sizeHint();
  return QSize(std::min(120, hint.width()), hint.height());
}

void AdInputTextArea::focusInput(FocusCursor cursor, bool preventScroll) {
  Q_UNUSED(preventScroll)
  if (!textEdit_) {
    return;
  }

  textEdit_->setFocus(Qt::OtherFocusReason);
  QTextCursor cursorObj = textEdit_->textCursor();
  const QString text = textEdit_->toPlainText();

  if (cursor == FocusCursor::Start) {
    cursorObj.setPosition(0);
    textEdit_->setTextCursor(cursorObj);
  } else if (cursor == FocusCursor::End) {
    cursorObj.setPosition(text.size());
    textEdit_->setTextCursor(cursorObj);
  } else if (cursor == FocusCursor::All) {
    textEdit_->selectAll();
  }
}

void AdInputTextArea::blurInput() {
  if (textEdit_) {
    textEdit_->clearFocus();
  }
}

QTextEdit* AdInputTextArea::textEdit() const { return textEdit_; }

bool AdInputTextArea::eventFilter(QObject* watched, QEvent* event) {
  if (!event) {
    return QWidget::eventFilter(watched, event);
  }

  if (watched == textEdit_) {
    if (event->type() == QEvent::FocusIn) {
      focused_ = true;
      updateClearButton();
      applyVisualStyle();
    } else if (event->type() == QEvent::FocusOut) {
      focused_ = false;
      updateClearButton();
      applyVisualStyle();
    }
  } else if (watched == clearButton_) {
    if (event->type() == QEvent::Enter || event->type() == QEvent::Leave) {
      updateClearButton();
    }
  }

  return QWidget::eventFilter(watched, event);
}

void AdInputTextArea::enterEvent(QEnterEvent* event) {
  hovered_ = true;
  updateClearButton();
  applyVisualStyle();
  QWidget::enterEvent(event);
}

void AdInputTextArea::leaveEvent(QEvent* event) {
  hovered_ = false;
  updateClearButton();
  applyVisualStyle();
  QWidget::leaveEvent(event);
}

void AdInputTextArea::paintEvent(QPaintEvent* event) { QWidget::paintEvent(event); }

void AdInputTextArea::changeEvent(QEvent* event) {
  QWidget::changeEvent(event);
  if (!event) {
    return;
  }

  if (event->type() == QEvent::EnabledChange || event->type() == QEvent::FontChange ||
      event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange) {
    updateClearButton();
    updateAutoSize();
    applyVisualStyle();
  }
}

void AdInputTextArea::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  updateAutoSize();
  updateInteractionFocusOverlay();
}

void AdInputTextArea::updateCountLabel() {
  if (!countLabel_ || !textEdit_) {
    return;
  }

  const int count = effectiveCount(textEdit_->toPlainText());
  const int max = effectiveCountMax();
  const bool visible = showCount_;
  countLabel_->setVisible(visible);
  if (!visible) {
    return;
  }

  QString formatted;
  if (countFormatter_) {
    formatted = countFormatter_(textEdit_->toPlainText(), count, max);
  } else if (max > 0) {
    formatted = QStringLiteral("%1 / %2").arg(count).arg(max);
  } else {
    formatted = QString::number(count);
  }
  countLabel_->setText(formatted);
}

void AdInputTextArea::updateClearButton() {
  if (!clearButton_ || !textEdit_) {
    return;
  }

  const bool canShow =
      allowClear_ && !disabled() && !textEdit_->toPlainText().isEmpty();
  clearButton_->setVisible(canShow);

  StyleContext context;
  context.size = size_;
  context.variant = variant_;
  context.status = status_;
  context.disabled = disabled();
  context.focused = focused_;
  context.hovered = hovered_;
  context.showCount = showCount_;
  context.valueLength = value().size();
  context.count = effectiveCount(value());
  context.countMax = effectiveCountMax();

  SemanticStyles effectiveSemantic = semanticStyles_;
  if (semanticStyleResolver_) {
    effectiveSemantic = semanticStyleResolver_(context);
  }

  detail::InputStyleInput styleInput;
  styleInput.size = size_;
  styleInput.variant = variant_;
  styleInput.status = status_;
  styleInput.disabled = disabled();
  styleInput.focused = focused_;
  styleInput.hovered = hovered_;
  styleInput.multiline = true;
  styleInput.baseFont = font();
  styleInput.componentTokens = componentTokens_;
  styleInput.semanticStyles = effectiveSemantic;
  const detail::InputVisualStyle style = detail::resolveInputVisualStyle(styleInput);

  const int iconSide = std::max(10, style.metrics.iconSize);
  const QColor color =
      (clearButton_->underMouse() && canShow && !disabled()) ? style.clearHoverColor : style.clearColor;
  const QPixmap px = renderTintedIcon(filled_icons::CloseCircle(), color, iconSide, devicePixelRatioF());
  clearButton_->setIcon(QIcon(px));
  clearButton_->setIconSize(QSize(iconSide, iconSide));
  clearButton_->setFixedSize(iconSide, iconSide);
  clearButton_->setText(px.isNull() ? QStringLiteral("x") : QString());
  clearButton_->setStyleSheet(
      QStringLiteral("QToolButton { border: none; padding: 0; background: transparent; }"));
}

void AdInputTextArea::updateAutoSize() {
  if (!shell_ || !textEdit_) {
    return;
  }

  detail::InputStyleInput styleInput;
  styleInput.size = size_;
  styleInput.variant = variant_;
  styleInput.status = status_;
  styleInput.disabled = disabled();
  styleInput.focused = focused_;
  styleInput.hovered = hovered_;
  styleInput.multiline = true;
  styleInput.baseFont = font();
  styleInput.componentTokens = componentTokens_;
  styleInput.semanticStyles = semanticStyles_;
  const detail::InputVisualStyle style = detail::resolveInputVisualStyle(styleInput);

  const int lineHeight = lineHeightForFont(style.metrics.font);
  const int minRows = std::max(1, autoSizeMinRows_);
  const int maxRows = std::max(minRows, autoSizeMaxRows_);
  const int rowPaddingExtra = textAreaRowPaddingExtra(style);
  const int autoSizeExtra = std::max(2, std::max(0, style.metrics.verticalPadding) * 2 + 2);

  int desired = std::max(style.metrics.height * 2, minRows * lineHeight + rowPaddingExtra);
  if (autoSizeEnabled_ && textEdit_->document()) {
    const qreal availableWidth =
        std::max<qreal>(1.0, textEdit_->viewport()->width() > 0 ? textEdit_->viewport()->width() : this->width());
    textEdit_->document()->setTextWidth(availableWidth);
    desired = std::max(1, qRound(textEdit_->document()->size().height()) + autoSizeExtra);
  }

  const int minHeight = minRows * lineHeight + rowPaddingExtra;
  const int maxHeight = maxRows * lineHeight + rowPaddingExtra;
  desired = std::clamp(desired, minHeight, maxHeight);
  desired = std::max(desired, style.metrics.height);

  const int borderInset = std::max(0, style.metrics.borderWidth);
  const int shellHeight = desired + borderInset * 2;
  textEdit_->setMinimumHeight(desired);
  textEdit_->setMaximumHeight(desired);
  shell_->setMinimumHeight(shellHeight);
  shell_->setMaximumHeight(shellHeight);
  updateGeometry();
}

void AdInputTextArea::applyVisualStyle() {
  if (!shell_ || !shellLayout_ || !textEdit_ || !countLabel_) {
    return;
  }

  StyleContext context;
  context.size = size_;
  context.variant = variant_;
  context.status = status_;
  context.disabled = disabled();
  context.focused = focused_;
  context.hovered = hovered_;
  context.showCount = showCount_;
  context.valueLength = value().size();
  context.count = effectiveCount(value());
  context.countMax = effectiveCountMax();

  SemanticStyles effectiveSemantic = semanticStyles_;
  if (semanticStyleResolver_) {
    effectiveSemantic = semanticStyleResolver_(context);
  }

  detail::InputStyleInput styleInput;
  styleInput.size = size_;
  styleInput.variant = variant_;
  styleInput.status = status_;
  styleInput.disabled = disabled();
  styleInput.focused = focused_;
  styleInput.hovered = hovered_;
  styleInput.multiline = true;
  styleInput.baseFont = font();
  styleInput.componentTokens = componentTokens_;
  styleInput.semanticStyles = effectiveSemantic;

  if (status_ == Status::None && effectiveCountMax() > 0 && effectiveCount(value()) > effectiveCountMax()) {
    styleInput.status = Status::Warning;
  }

  const detail::InputVisualStyle style = detail::resolveInputVisualStyle(styleInput);

  setFont(style.metrics.font);
  textEdit_->setFont(style.metrics.font);

  const int borderInset = std::max(0, style.metrics.borderWidth);
  const int hp = std::max(0, style.metrics.horizontalPadding);
  const int vp = std::max(0, style.metrics.verticalPadding);
  const int docMargin = vp;
  const int insetHorizontal = std::max(0, hp + borderInset - docMargin);
  shellLayout_->setContentsMargins(insetHorizontal, borderInset, insetHorizontal, borderInset);
  shellLayout_->setSpacing(std::max(4, hp / 2));
  rootLayout_->setSpacing(showCount_ ? style.metrics.countTopMargin : 0);

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

  InputShellPaintStyle shellPaintStyle;
  shellPaintStyle.background = background;
  shellPaintStyle.border = border;
  shellPaintStyle.borderWidth = std::max(0, style.metrics.borderWidth);
  const qreal cornerRadius = style.underlined ? 0.0 : std::max<qreal>(0.0, style.metrics.borderRadius);
  shellPaintStyle.topLeftRadius = cornerRadius;
  shellPaintStyle.topRightRadius = cornerRadius;
  shellPaintStyle.bottomRightRadius = cornerRadius;
  shellPaintStyle.bottomLeftRadius = cornerRadius;
  shellPaintStyle.underlined = style.underlined;
  static_cast<InputShellWidget*>(shell_)->setPaintStyle(shellPaintStyle);
  if (!shell_->styleSheet().isEmpty()) {
    shell_->setStyleSheet(QString());
  }

  QPalette editPalette = textEdit_->palette();
  editPalette.setColor(QPalette::Text, style.selectorTextColor);
  editPalette.setColor(QPalette::Disabled, QPalette::Text, style.disabledTextColor);
  editPalette.setColor(QPalette::PlaceholderText, style.placeholderColor);
  textEdit_->setPalette(editPalette);
  textEdit_->setStyleSheet(
      QStringLiteral("QTextEdit { border: none; background: transparent; padding: 0; }"));
  if (textEdit_->document()) {
    textEdit_->document()->setDocumentMargin(docMargin);
  }

  QFont countFont = style.metrics.font;
  countFont.setPixelSize(std::max(10, style.metrics.font.pixelSize() - 1));
  countLabel_->setFont(countFont);
  countLabel_->setMinimumHeight(style.metrics.countHeight);
  QPalette countPalette = countLabel_->palette();
  countPalette.setColor(QPalette::WindowText, style.countColor);
  countLabel_->setPalette(countPalette);

  updateAutoSize();
  updateClearButton();
  updateCountLabel();
  updateInteractionFocusOverlay();
  update();
}

void AdInputTextArea::updateInteractionFocusOverlay() {
  if (!focused_ || disabled() || !isVisible() || !shell_) {
    stopInteractionFocusForOwner(this);
    return;
  }

  detail::InputStyleInput styleInput;
  styleInput.size = size_;
  styleInput.variant = variant_;
  styleInput.status = status_;
  styleInput.disabled = disabled();
  styleInput.focused = focused_;
  styleInput.hovered = hovered_;
  styleInput.multiline = true;
  styleInput.baseFont = font();
  styleInput.componentTokens = componentTokens_;
  styleInput.semanticStyles = semanticStyles_;
  const detail::InputVisualStyle style = detail::resolveInputVisualStyle(styleInput);

  if (style.selectorFocusOutlineColor.alpha() <= 0 || style.metrics.focusOutlineWidth <= 0.0) {
    stopInteractionFocusForOwner(this);
    return;
  }

  QWidget* hostWindow = window();
  if (!hostWindow) {
    return;
  }

  const QPoint origin = mapTo(hostWindow, shell_->geometry().topLeft());
  QRectF baseRect(origin, shell_->size());
  const qreal half = std::max<qreal>(0.0, style.metrics.borderWidth / 2.0);
  baseRect.adjust(half + 0.5, half + 0.5, -half - 0.5, -half - 0.5);

  InteractionFocusRequest request;
  request.owner = this;
  request.baseRectInWindow = baseRect;
  request.topLeft = style.underlined ? 0.0 : std::max<qreal>(0.0, style.metrics.borderRadius);
  request.topRight = request.topLeft;
  request.bottomRight = request.topLeft;
  request.bottomLeft = request.topLeft;
  request.color = style.selectorFocusOutlineColor;
  request.strokeWidth = std::max<qreal>(1.0, style.metrics.focusOutlineWidth);
  request.offset = std::max<qreal>(0.0, style.metrics.focusOutlineOffset);
  triggerInteractionFocus(request);
}

int AdInputTextArea::effectiveCount(const QString& text) const {
  if (countStrategy_) {
    return std::max(0, countStrategy_(text));
  }
  return text.size();
}

int AdInputTextArea::effectiveCountMax() const {
  if (countMax_ > 0) {
    return countMax_;
  }
  if (maxLength_ > 0) {
    return maxLength_;
  }
  return -1;
}

AdInputSearch::AdInputSearch(QWidget* parent) : QWidget(parent) {
  rootLayout_ = new QHBoxLayout(this);
  rootLayout_->setContentsMargins(0, 0, 0, 0);
  rootLayout_->setSpacing(0);

  input_ = new AdInput(this);
  input_->setJoinedRight(true);

  button_ = new AdButton(this);
  button_->setJoinedLeft(true);
  button_->setIconToken(outlined_icons::Search());
  button_->setText(QString());

  rootLayout_->addWidget(input_, 1);
  rootLayout_->addWidget(button_);

  connect(input_, &AdInput::valueChanged, this, [this](const QString& text) { emit valueChanged(text); });
  connect(input_, &AdInput::cleared, this, [this]() { emit searchTriggered(input_->value(), SearchSource::Clear); });
  connect(input_, &AdInput::returnPressed, this, [this]() {
    if (!loading_ && !disabled()) {
      emit searchTriggered(input_->value(), SearchSource::Input);
    }
  });

  connect(button_, &QPushButton::clicked, this, [this]() {
    if (!loading_ && !disabled()) {
      emit searchTriggered(input_->value(), SearchSource::Input);
    }
  });

  connect(&adqt::theme::ThemeManager::instance(), &adqt::theme::ThemeManager::themeChanged, this,
          [this]() { updateButtonVisual(); });

  updateButtonVisual();
}

AdInputSearch::~AdInputSearch() = default;

AdInputSearch::Size AdInputSearch::size() const {
  return input_ ? input_->size() : AdInput::Size::Middle;
}

void AdInputSearch::setSize(Size value) {
  if (!input_ || input_->size() == value) {
    return;
  }
  input_->setSize(value);
  updateButtonVisual();
  emit sizeChanged(value);
}

AdInputSearch::Variant AdInputSearch::variant() const {
  return input_ ? input_->variant() : AdInput::Variant::Outlined;
}

void AdInputSearch::setVariant(Variant value) {
  if (!input_ || input_->variant() == value) {
    return;
  }
  input_->setVariant(value);
  updateButtonVisual();
  emit variantChanged(value);
}

AdInputSearch::Status AdInputSearch::status() const {
  return input_ ? input_->status() : AdInput::Status::None;
}

void AdInputSearch::setStatus(Status value) {
  if (!input_ || input_->status() == value) {
    return;
  }
  input_->setStatus(value);
  emit statusChanged(value);
}

bool AdInputSearch::disabled() const { return !isEnabled(); }

void AdInputSearch::setDisabled(bool value) {
  if (disabled() == value) {
    return;
  }
  QWidget::setDisabled(value);
  if (input_) {
    input_->setDisabled(value);
  }
  if (button_) {
    button_->setDisabled(value);
  }
  updateButtonVisual();
  emit disabledChanged(value);
}

bool AdInputSearch::allowClear() const { return input_ ? input_->allowClear() : false; }

void AdInputSearch::setAllowClear(bool value) {
  if (!input_ || input_->allowClear() == value) {
    return;
  }
  input_->setAllowClear(value);
  emit allowClearChanged(value);
}

QString AdInputSearch::placeholder() const { return input_ ? input_->placeholder() : QString(); }

void AdInputSearch::setPlaceholder(const QString& value) {
  if (!input_ || input_->placeholder() == value) {
    return;
  }
  input_->setPlaceholder(value);
  emit placeholderChanged(value);
}

QString AdInputSearch::value() const { return input_ ? input_->value() : QString(); }

void AdInputSearch::setValue(const QString& value) {
  if (!input_ || input_->value() == value) {
    return;
  }
  input_->setValue(value);
}

bool AdInputSearch::loading() const { return loading_; }

void AdInputSearch::setLoading(bool value) {
  if (loading_ == value) {
    return;
  }
  loading_ = value;
  updateButtonVisual();
  emit loadingChanged(loading_);
}

bool AdInputSearch::enterButton() const { return enterButton_; }

void AdInputSearch::setEnterButton(bool value) {
  if (enterButton_ == value) {
    return;
  }
  enterButton_ = value;
  updateButtonVisual();
  emit enterButtonChanged(enterButton_);
}

QString AdInputSearch::enterButtonText() const { return enterButtonText_; }

void AdInputSearch::setEnterButtonText(const QString& value) {
  if (enterButtonText_ == value) {
    return;
  }
  enterButtonText_ = value;
  updateButtonVisual();
  emit enterButtonTextChanged(enterButtonText_);
}

AdInput* AdInputSearch::input() const { return input_; }

void AdInputSearch::updateButtonVisual() {
  if (!button_ || !input_) {
    return;
  }

  const adqt::theme::ThemeMapToken& map = adqt::theme::ThemeManager::instance().currentMapToken();
  const int lineWidth = std::max(1, qRound(map.lineWidth));

  button_->setLoading(loading_);
  button_->setDisabled(disabled());
  button_->setSize(static_cast<AdButton::Size>(static_cast<int>(input_->size())));

  const Variant inputVariant = variant();
  const bool unborderedVariant =
      inputVariant == Variant::Borderless || inputVariant == Variant::Underlined;

  AdButton::Color buttonColor = AdButton::Color::Default;
  AdButton::Variant buttonVariant = AdButton::Variant::Outlined;

  if (enterButton_) {
    buttonColor = AdButton::Color::Primary;
    buttonVariant =
        (inputVariant == Variant::Filled || unborderedVariant) ? AdButton::Variant::Text
                                                                : AdButton::Variant::Solid;
    button_->setIconToken(adqt::icons::IconToken());
    button_->setText(enterButtonText_.isEmpty() ? QStringLiteral("Search") : enterButtonText_);
  } else {
    if (inputVariant == Variant::Filled) {
      buttonVariant = AdButton::Variant::Filled;
    } else if (unborderedVariant) {
      buttonVariant = AdButton::Variant::Text;
    } else {
      buttonVariant = AdButton::Variant::Outlined;
    }
    button_->setIconToken(outlined_icons::Search());
    button_->setText(QString());
  }

  button_->setColor(buttonColor);
  button_->setVariant(buttonVariant);

  if (rootLayout_) {
    rootLayout_->setSpacing(0);
  }

  const bool showSeparator = !enterButton_ && inputVariant == Variant::Filled;
  button_->setLeadingSeparatorVisible(showSeparator);
  button_->setLeadingSeparatorWidth(lineWidth);
  button_->setLeadingSeparatorColor(parseThemeColor(map.colorBorder, QColor("#d9d9d9")));

  const int controlHeight = std::max(1, button_->minimumHeight());
  input_->setFixedHeight(controlHeight);
  button_->setFixedHeight(controlHeight);
  setFixedHeight(controlHeight);
}

AdInputPassword::AdInputPassword(QWidget* parent) : QWidget(parent) {
  rootLayout_ = new QHBoxLayout(this);
  rootLayout_->setContentsMargins(0, 0, 0, 0);
  rootLayout_->setSpacing(0);

  input_ = new AdInput(this);
  input_->setEchoMode(QLineEdit::Password);

  toggleButton_ = new QToolButton(this);
  toggleButton_->setAutoRaise(true);
  toggleButton_->setFocusPolicy(Qt::NoFocus);

  rootLayout_->addWidget(input_, 1);
  rootLayout_->addWidget(toggleButton_);

  visibleIconToken_ = outlined_icons::Eye();
  hiddenIconToken_ = outlined_icons::EyeInvisible();

  connect(input_, &AdInput::valueChanged, this, [this](const QString& text) { emit valueChanged(text); });
  connect(toggleButton_, &QToolButton::clicked, this, [this]() {
    if (!visibilityToggle_ || disabled()) {
      return;
    }
    setPasswordVisible(!passwordVisible_);
  });

  connect(&adqt::theme::ThemeManager::instance(), &adqt::theme::ThemeManager::themeChanged, this,
          [this]() { updateToggleVisual(); });

  updateToggleVisual();
}

AdInputPassword::~AdInputPassword() = default;

AdInputPassword::Size AdInputPassword::size() const {
  return input_ ? input_->size() : AdInput::Size::Middle;
}

void AdInputPassword::setSize(Size value) {
  if (!input_ || input_->size() == value) {
    return;
  }
  input_->setSize(value);
  updateToggleVisual();
  emit sizeChanged(value);
}

AdInputPassword::Variant AdInputPassword::variant() const {
  return input_ ? input_->variant() : AdInput::Variant::Outlined;
}

void AdInputPassword::setVariant(Variant value) {
  if (!input_ || input_->variant() == value) {
    return;
  }
  input_->setVariant(value);
  emit variantChanged(value);
}

AdInputPassword::Status AdInputPassword::status() const {
  return input_ ? input_->status() : AdInput::Status::None;
}

void AdInputPassword::setStatus(Status value) {
  if (!input_ || input_->status() == value) {
    return;
  }
  input_->setStatus(value);
  emit statusChanged(value);
}

bool AdInputPassword::disabled() const { return !isEnabled(); }

void AdInputPassword::setDisabled(bool value) {
  if (disabled() == value) {
    return;
  }
  QWidget::setDisabled(value);
  if (input_) {
    input_->setDisabled(value);
  }
  if (toggleButton_) {
    toggleButton_->setDisabled(value);
  }
  updateToggleVisual();
  emit disabledChanged(value);
}

QString AdInputPassword::placeholder() const { return input_ ? input_->placeholder() : QString(); }

void AdInputPassword::setPlaceholder(const QString& value) {
  if (!input_ || input_->placeholder() == value) {
    return;
  }
  input_->setPlaceholder(value);
  emit placeholderChanged(value);
}

QString AdInputPassword::value() const { return input_ ? input_->value() : QString(); }

void AdInputPassword::setValue(const QString& value) {
  if (!input_ || input_->value() == value) {
    return;
  }
  input_->setValue(value);
}

bool AdInputPassword::visibilityToggle() const { return visibilityToggle_; }

void AdInputPassword::setVisibilityToggle(bool value) {
  if (visibilityToggle_ == value) {
    return;
  }
  visibilityToggle_ = value;
  updateToggleVisual();
  emit visibilityToggleChanged(visibilityToggle_);
}

bool AdInputPassword::passwordVisible() const { return passwordVisible_; }

void AdInputPassword::setPasswordVisible(bool value) {
  if (passwordVisible_ == value) {
    return;
  }
  passwordVisible_ = value;
  if (input_) {
    input_->setEchoMode(passwordVisible_ ? QLineEdit::Normal : QLineEdit::Password);
  }
  updateToggleVisual();
  emit passwordVisibleChanged(passwordVisible_);
}

adqt::icons::IconToken AdInputPassword::visibleIconToken() const { return visibleIconToken_; }

void AdInputPassword::setVisibleIconToken(const adqt::icons::IconToken& value) {
  if (iconTokensEqual(visibleIconToken_, value)) {
    return;
  }
  visibleIconToken_ = value;
  updateToggleVisual();
}

adqt::icons::IconToken AdInputPassword::hiddenIconToken() const { return hiddenIconToken_; }

void AdInputPassword::setHiddenIconToken(const adqt::icons::IconToken& value) {
  if (iconTokensEqual(hiddenIconToken_, value)) {
    return;
  }
  hiddenIconToken_ = value;
  updateToggleVisual();
}

AdInput* AdInputPassword::input() const { return input_; }

void AdInputPassword::updateToggleVisual() {
  if (!toggleButton_) {
    return;
  }

  const bool showToggle = visibilityToggle_;
  toggleButton_->setVisible(showToggle);
  if (!showToggle) {
    return;
  }

  const adqt::icons::IconToken token = passwordVisible_ ? visibleIconToken_ : hiddenIconToken_;
  const QColor color = disabled() ? QColor("#bfbfbf") : QColor("#8c8c8c");
  const int iconSide = 14;
  const QPixmap px = renderTintedIcon(token, color, iconSide, devicePixelRatioF());
  toggleButton_->setIcon(QIcon(px));
  toggleButton_->setIconSize(QSize(iconSide, iconSide));
  toggleButton_->setFixedSize(28, 28);
  toggleButton_->setStyleSheet(
      QStringLiteral("QToolButton { border: none; background: transparent; padding: 0; }"));
}

AdInputOtp::AdInputOtp(QWidget* parent) : QWidget(parent) {
  rootLayout_ = new QHBoxLayout(this);
  rootLayout_->setContentsMargins(0, 0, 0, 0);
  rootLayout_->setSpacing(8);

  connect(&adqt::theme::ThemeManager::instance(), &adqt::theme::ThemeManager::themeChanged, this,
          [this]() { applyVisualStyle(); });

  rebuildCells();
}

AdInputOtp::~AdInputOtp() = default;

int AdInputOtp::length() const { return length_; }

void AdInputOtp::setLength(int value) {
  const int normalized = std::max(1, value);
  if (length_ == normalized) {
    return;
  }
  length_ = normalized;

  if (value_.size() > length_) {
    value_ = value_.left(length_);
    emit valueChanged(value_);
  }

  rebuildCells();
  emit lengthChanged(length_);
}

QString AdInputOtp::value() const { return value_; }

void AdInputOtp::setValue(const QString& value) {
  QString next = value.left(length_);
  if (formatter_) {
    next = formatter_(next).left(length_);
  }

  if (value_ == next) {
    return;
  }

  value_ = next;
  applyValueToCells(value_);
  emit valueChanged(value_);
  emitInputState();
}

AdInputOtp::Size AdInputOtp::size() const { return size_; }

void AdInputOtp::setSize(Size value) {
  if (size_ == value) {
    return;
  }
  size_ = value;
  applyVisualStyle();
  emit sizeChanged(size_);
}

AdInputOtp::Variant AdInputOtp::variant() const { return variant_; }

void AdInputOtp::setVariant(Variant value) {
  if (variant_ == value) {
    return;
  }
  variant_ = value;
  applyVisualStyle();
  emit variantChanged(variant_);
}

AdInputOtp::Status AdInputOtp::status() const { return status_; }

void AdInputOtp::setStatus(Status value) {
  if (status_ == value) {
    return;
  }
  status_ = value;
  applyVisualStyle();
  emit statusChanged(status_);
}

bool AdInputOtp::disabled() const { return !isEnabled(); }

void AdInputOtp::setDisabled(bool value) {
  if (disabled() == value) {
    return;
  }
  QWidget::setDisabled(value);
  for (QLineEdit* cell : cells_) {
    if (cell) {
      cell->setDisabled(value);
    }
  }
  applyVisualStyle();
  emit disabledChanged(value);
}

bool AdInputOtp::maskEnabled() const { return maskEnabled_; }

void AdInputOtp::setMaskEnabled(bool value) {
  if (maskEnabled_ == value) {
    return;
  }
  maskEnabled_ = value;
  updateEchoModes();
  emit maskEnabledChanged(maskEnabled_);
}

QString AdInputOtp::maskCharacter() const { return maskCharacter_; }

void AdInputOtp::setMaskCharacter(const QString& value) {
  if (maskCharacter_ == value) {
    return;
  }
  maskCharacter_ = value;
  updateEchoModes();
  emit maskCharacterChanged(maskCharacter_);
}

AdInputOtp::Formatter AdInputOtp::formatter() const { return formatter_; }

void AdInputOtp::setFormatter(Formatter value) {
  formatter_ = std::move(value);
  setValue(value_);
}

QString AdInputOtp::separatorText() const { return separatorText_; }

void AdInputOtp::setSeparatorText(const QString& value) {
  if (separatorText_ == value) {
    return;
  }
  separatorText_ = value;
  rebuildCells();
}

AdInputOtp::SeparatorFactory AdInputOtp::separatorFactory() const { return separatorFactory_; }

void AdInputOtp::setSeparatorFactory(SeparatorFactory value) {
  separatorFactory_ = std::move(value);
  rebuildCells();
}

bool AdInputOtp::eventFilter(QObject* watched, QEvent* event) {
  if (!event) {
    return QWidget::eventFilter(watched, event);
  }

  const int index = cells_.indexOf(qobject_cast<QLineEdit*>(watched));
  if (index < 0) {
    return QWidget::eventFilter(watched, event);
  }

  if (event->type() == QEvent::FocusIn) {
    QLineEdit* cell = cells_.at(index);
    if (cell) {
      cell->selectAll();
    }
    applyVisualStyle();
  } else if (event->type() == QEvent::FocusOut) {
    applyVisualStyle();
  } else if (event->type() == QEvent::KeyPress) {
    auto* keyEvent = static_cast<QKeyEvent*>(event);
    if (keyEvent->key() == Qt::Key_Left) {
      focusCell(index - 1);
      return true;
    }
    if (keyEvent->key() == Qt::Key_Right) {
      focusCell(index + 1);
      return true;
    }
    if (keyEvent->key() == Qt::Key_Backspace && cells_.at(index)->text().isEmpty()) {
      focusCell(index - 1);
    }
  }

  return QWidget::eventFilter(watched, event);
}

void AdInputOtp::rebuildCells() {
  if (!rootLayout_) {
    return;
  }

  while (QLayoutItem* item = rootLayout_->takeAt(0)) {
    if (item->widget()) {
      item->widget()->deleteLater();
    }
    delete item;
  }

  cells_.clear();
  separators_.clear();

  for (int i = 0; i < length_; ++i) {
    auto* cell = new QLineEdit(this);
    cell->setAlignment(Qt::AlignCenter);
    cell->setMaxLength(length_);
    cell->setFrame(false);
    cell->setDisabled(disabled());
    cell->installEventFilter(this);

    connect(cell, &QLineEdit::textEdited, this, [this, i](const QString& text) { handleCellEdited(i, text); });

    cells_.append(cell);
    rootLayout_->addWidget(cell);

    if (i == length_ - 1) {
      continue;
    }

    QWidget* separator = nullptr;
    if (separatorFactory_) {
      separator = separatorFactory_(i, this);
    }
    if (!separator && !separatorText_.isEmpty()) {
      auto* label = new QLabel(separatorText_, this);
      label->setAlignment(Qt::AlignCenter);
      separator = label;
    }
    if (separator) {
      separators_.append(separator);
      rootLayout_->addWidget(separator);
    }
  }

  applyValueToCells(value_);
  updateEchoModes();
  applyVisualStyle();
}

void AdInputOtp::applyVisualStyle() {
  detail::InputStyleInput styleInput;
  styleInput.size = size_;
  styleInput.variant = variant_;
  styleInput.status = status_;
  styleInput.disabled = disabled();
  styleInput.focused = false;
  styleInput.hovered = false;
  styleInput.baseFont = font();
  styleInput.componentTokens = AdInput::ComponentTokens();
  styleInput.semanticStyles = AdInput::SemanticStyles();
  const detail::InputVisualStyle baseStyle = detail::resolveInputVisualStyle(styleInput);

  const int cellSide = std::max(22, baseStyle.metrics.height);

  for (QLineEdit* cell : cells_) {
    if (!cell) {
      continue;
    }

    QColor background = baseStyle.selectorBg;
    QColor border = baseStyle.selectorBorderColor;
    if (!disabled()) {
      if (cell->hasFocus()) {
        background = baseStyle.selectorActiveBg;
        border = baseStyle.selectorActiveBorderColor;
      } else if (cell->underMouse()) {
        background = baseStyle.selectorHoverBg;
        border = baseStyle.selectorHoverBorderColor;
      }
    }

    QString cellCss;
    if (baseStyle.underlined) {
      cellCss = QStringLiteral("QLineEdit { background: %1; color: %2; border: none; border-bottom: %3px solid %4; }")
                    .arg(cssRgba(background))
                    .arg(cssRgba(baseStyle.selectorTextColor))
                    .arg(std::max(1, baseStyle.metrics.borderWidth))
                    .arg(cssRgba(border));
    } else {
      cellCss = QStringLiteral("QLineEdit { background: %1; color: %2; border: %3px solid %4; border-radius: %5px; }")
                    .arg(cssRgba(background))
                    .arg(cssRgba(baseStyle.selectorTextColor))
                    .arg(std::max(0, baseStyle.metrics.borderWidth))
                    .arg(cssRgba(border))
                    .arg(std::max(0, baseStyle.metrics.borderRadius));
    }

    cell->setStyleSheet(cellCss);
    cell->setFont(baseStyle.metrics.font);
    cell->setFixedSize(cellSide, cellSide);

    QPalette palette = cell->palette();
    palette.setColor(QPalette::Text, baseStyle.selectorTextColor);
    palette.setColor(QPalette::Disabled, QPalette::Text, baseStyle.disabledTextColor);
    cell->setPalette(palette);
  }

  for (QWidget* separator : separators_) {
    if (auto* label = qobject_cast<QLabel*>(separator)) {
      QPalette palette = label->palette();
      palette.setColor(QPalette::WindowText, baseStyle.suffixColor);
      label->setPalette(palette);
    }
  }

  if (rootLayout_) {
    rootLayout_->setSpacing(std::max(4, baseStyle.metrics.horizontalPadding / 2));
  }
}

void AdInputOtp::applyValueToCells(const QString& value) {
  internalUpdate_ = true;
  for (int i = 0; i < cells_.size(); ++i) {
    QLineEdit* cell = cells_.at(i);
    if (!cell) {
      continue;
    }
    const QString v = (i < value.size()) ? QString(value.at(i)) : QString();
    cell->setText(v);
  }
  internalUpdate_ = false;
}

QString AdInputOtp::combinedValueFromCells() const {
  QString out;
  out.reserve(cells_.size());
  for (QLineEdit* cell : cells_) {
    if (!cell) {
      continue;
    }
    const QString t = cell->text();
    out += (t.isEmpty() ? QString() : QString(t.at(0)));
  }
  while (!out.isEmpty() && out.endsWith(QLatin1Char(' '))) {
    out.chop(1);
  }
  return out;
}

void AdInputOtp::emitInputState() {
  QStringList pieces;
  pieces.reserve(cells_.size());
  for (QLineEdit* cell : cells_) {
    pieces.push_back(cell ? (cell->text().isEmpty() ? QString() : QString(cell->text().at(0))) : QString());
  }
  emit inputChanged(pieces);

  bool allFilled = !pieces.isEmpty();
  QString joined;
  joined.reserve(pieces.size());
  for (const QString& piece : pieces) {
    if (piece.isEmpty()) {
      allFilled = false;
    }
    joined += piece;
  }

  if (value_ != joined) {
    value_ = joined;
    emit valueChanged(value_);
  }
  if (allFilled && pieces.size() == length_) {
    emit completed(joined);
  }
}

void AdInputOtp::handleCellEdited(int index, const QString& text) {
  if (internalUpdate_ || index < 0 || index >= cells_.size()) {
    return;
  }

  QStringList parts;
  parts.reserve(cells_.size());
  for (QLineEdit* cell : cells_) {
    parts.push_back(cell ? (cell->text().isEmpty() ? QString() : QString(cell->text().at(0))) : QString());
  }

  if (text.size() <= 1) {
    parts[index] = text;
    if (!text.isEmpty()) {
      focusCell(index + 1);
    }
  } else {
    int cursor = index;
    for (int i = 0; i < text.size() && cursor < parts.size(); ++i, ++cursor) {
      parts[cursor] = QString(text.at(i));
    }
    focusCell(std::min(cursor, static_cast<int>(parts.size()) - 1));
  }

  QString joined;
  joined.reserve(parts.size());
  for (const QString& piece : parts) {
    joined += piece;
  }

  if (formatter_) {
    joined = formatter_(joined).left(length_);
  }

  value_ = joined;
  applyValueToCells(value_);
  emit valueChanged(value_);
  emitInputState();
}

void AdInputOtp::focusCell(int index) {
  if (index < 0 || index >= cells_.size()) {
    return;
  }
  QLineEdit* cell = cells_.at(index);
  if (!cell || !cell->isEnabled()) {
    return;
  }
  cell->setFocus(Qt::OtherFocusReason);
  cell->selectAll();
}

void AdInputOtp::updateEchoModes() {
  for (QLineEdit* cell : cells_) {
    if (!cell) {
      continue;
    }
    cell->setEchoMode(maskEnabled_ ? QLineEdit::Password : QLineEdit::Normal);
  }
}

}  // namespace adqt::widgets
