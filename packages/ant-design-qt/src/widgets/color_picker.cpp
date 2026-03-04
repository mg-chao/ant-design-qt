#include "color_picker.h"

#include "color_picker_style.h"
#include "slider.h"
#include "theme/theme_manager.h"

#include <QButtonGroup>
#include <QBrush>
#include <QComboBox>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLayout>
#include <QMap>
#include <QMetaType>
#include <QMouseEvent>
#include <QPalette>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPushButton>
#include <QRegularExpression>
#include <QScopedValueRollback>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include <algorithm>
#include <limits>
#include <utility>

namespace adqt::widgets {

class ColorSaturationPanel final : public QWidget {
 public:
  explicit ColorSaturationPanel(QWidget* parent = nullptr) : QWidget(parent) {
    setAttribute(Qt::WA_Hover, true);
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  }

  void setHue(int value) {
    const int clamped = ((value % 360) + 360) % 360;
    if (hue_ == clamped) {
      return;
    }
    hue_ = clamped;
    update();
  }

  void setSaturationBrightness(double saturation, double brightness) {
    const double sat = std::clamp(saturation, 0.0, 1.0);
    const double bri = std::clamp(brightness, 0.0, 1.0);
    const bool changed = !qFuzzyCompare(saturation_ + 1.0, sat + 1.0) ||
                         !qFuzzyCompare(brightness_ + 1.0, bri + 1.0);
    saturation_ = sat;
    brightness_ = bri;
    if (changed) {
      update();
    }
  }

  int hue() const { return hue_; }
  double saturation() const { return saturation_; }
  double brightness() const { return brightness_; }

  void setChangeCallback(std::function<void(double saturation, double brightness, bool completed)> callback) {
    changeCallback_ = std::move(callback);
  }

 protected:
  void paintEvent(QPaintEvent* event) override {
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF panelRect = rect().adjusted(0.5, 0.5, -0.5, -0.5);
    if (panelRect.width() <= 1.0 || panelRect.height() <= 1.0) {
      return;
    }

  // Use small radius matching borderRadiusXS in theme tokens
  // Handler size from Ant Design tokens (colorPickerHandlerSize = 16)
  constexpr qreal kSaturationPanelRadius = 4.0;
  constexpr qreal kHandlerOuterRadius = 8.0;
  constexpr qreal kHandlerInnerRadius = 5.0;
  constexpr qreal kBorderAlpha = 0.08;

  QPainterPath clipPath;
  clipPath.addRoundedRect(panelRect, kSaturationPanelRadius, kSaturationPanelRadius);
  painter.setClipPath(clipPath);

    painter.fillRect(panelRect, QColor::fromHsv(hue_, 255, 255));

    QLinearGradient whiteOverlay(panelRect.topLeft(), panelRect.topRight());
    whiteOverlay.setColorAt(0.0, QColor(255, 255, 255));
    whiteOverlay.setColorAt(1.0, QColor(255, 255, 255, 0));
    painter.fillRect(panelRect, whiteOverlay);

    QLinearGradient blackOverlay(panelRect.topLeft(), panelRect.bottomLeft());
    blackOverlay.setColorAt(0.0, QColor(0, 0, 0, 0));
    blackOverlay.setColorAt(1.0, QColor(0, 0, 0));
    painter.fillRect(panelRect, blackOverlay);

    painter.setClipping(false);
    painter.setPen(QPen(QColor(0, 0, 0, static_cast<int>(255 * kBorderAlpha)), 1.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(panelRect, kSaturationPanelRadius, kSaturationPanelRadius);

    const qreal handleX = panelRect.left() + saturation_ * panelRect.width();
    const qreal handleY = panelRect.top() + (1.0 - brightness_) * panelRect.height();
    const QPointF handleCenter(handleX, handleY);

    // Draw outer shadow/border effect (AntD uses box-shadow)
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 25));
    painter.drawEllipse(handleCenter, kHandlerOuterRadius, kHandlerOuterRadius);

    // Draw inner white ring
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor("#ffffff"), 1.5));
    painter.drawEllipse(handleCenter, kHandlerInnerRadius, kHandlerInnerRadius);
  }

  void mousePressEvent(QMouseEvent* event) override {
    QWidget::mousePressEvent(event);
    if (!event || event->button() != Qt::LeftButton || !isEnabled()) {
      return;
    }
    dragging_ = true;
    updateFromPoint(event->position(), false);
  }

  void mouseMoveEvent(QMouseEvent* event) override {
    QWidget::mouseMoveEvent(event);
    if (!event || !dragging_ || !isEnabled()) {
      return;
    }
    updateFromPoint(event->position(), false);
  }

  void mouseReleaseEvent(QMouseEvent* event) override {
    QWidget::mouseReleaseEvent(event);
    if (!event || event->button() != Qt::LeftButton || !isEnabled()) {
      return;
    }
    const bool wasDragging = dragging_;
    dragging_ = false;
    if (!wasDragging) {
      return;
    }
    updateFromPoint(event->position(), true);
  }

 private:
  void updateFromPoint(const QPointF& point, bool completed) {
    const QRectF panelRect = rect().adjusted(0.5, 0.5, -0.5, -0.5);
    if (panelRect.width() <= 1.0 || panelRect.height() <= 1.0) {
      return;
    }

    const qreal clampedX = std::clamp(point.x(), panelRect.left(), panelRect.right());
    const qreal clampedY = std::clamp(point.y(), panelRect.top(), panelRect.bottom());
    const double nextSat = std::clamp((clampedX - panelRect.left()) / panelRect.width(), 0.0, 1.0);
    const double nextBri = std::clamp(1.0 - (clampedY - panelRect.top()) / panelRect.height(), 0.0, 1.0);

    const bool changed = !qFuzzyCompare(saturation_ + 1.0, nextSat + 1.0) ||
                         !qFuzzyCompare(brightness_ + 1.0, nextBri + 1.0);
    saturation_ = nextSat;
    brightness_ = nextBri;
    if (changed) {
      update();
    }

    if (changeCallback_ && (changed || completed)) {
      changeCallback_(saturation_, brightness_, completed);
    }
  }

  int hue_ = 215;
  double saturation_ = 0.91;
  double brightness_ = 1.0;
  bool dragging_ = false;
  std::function<void(double saturation, double brightness, bool completed)> changeCallback_;
};

namespace {

constexpr char kPresetSwatchObjectName[] = "ad-color-picker-preset-swatch";
constexpr int kTransparencyCell = 6;

// Default values - will be overridden by style metrics when available
int g_saturationPanelHeight = 160;
int g_sliderHeight = 8;
int g_previewSwatchSize = 20;
int g_alphaInputWidth = 44;

void updateInternalMetrics(const detail::ColorPickerMetrics& metrics) {
  g_saturationPanelHeight = metrics.saturationPanelHeight;
  g_sliderHeight = metrics.sliderHeight;
  g_previewSwatchSize = metrics.previewSwatchSize;
  g_alphaInputWidth = metrics.alphaInputWidth;
}

QString formatPercent(double value) {
  QString text = QString::number(value, 'f', 3);
  while (text.contains(QLatin1Char('.')) &&
         (text.endsWith(QLatin1Char('0')) || text.endsWith(QLatin1Char('.')))) {
    text.chop(1);
    if (text.endsWith(QLatin1Char('.'))) {
      text.chop(1);
      break;
    }
  }
  if (text.isEmpty()) {
    return QStringLiteral("0");
  }
  return text;
}

QString colorToHexRgbLower(const QColor& color) {
  return color.name(QColor::HexRgb).toLower();
}

QBrush makeCheckerBrush(int cellSize, const QColor& light = QColor(255, 255, 255),
                        const QColor& dark = QColor(0, 0, 0, 20)) {
  const int cell = std::max(2, cellSize);
  QPixmap pixmap(cell * 2, cell * 2);
  pixmap.fill(light);

  QPainter painter(&pixmap);
  painter.fillRect(QRect(0, 0, cell, cell), dark);
  painter.fillRect(QRect(cell, cell, cell, cell), dark);
  painter.end();

  QBrush brush(pixmap);
  brush.setStyle(Qt::TexturePattern);
  return brush;
}

QBrush makeHueBrush() {
  QLinearGradient gradient(0.0, 0.0, 1.0, 0.0);
  gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
  gradient.setColorAt(0.0, QColor("#ff0000"));
  gradient.setColorAt(1.0 / 6.0, QColor("#ffff00"));
  gradient.setColorAt(2.0 / 6.0, QColor("#00ff00"));
  gradient.setColorAt(3.0 / 6.0, QColor("#00ffff"));
  gradient.setColorAt(4.0 / 6.0, QColor("#0000ff"));
  gradient.setColorAt(5.0 / 6.0, QColor("#ff00ff"));
  gradient.setColorAt(1.0, QColor("#ff0000"));
  return QBrush(gradient);
}

QBrush makeAlphaBrush(const QColor& color) {
  QColor transparent = color;
  transparent.setAlpha(0);
  QColor opaque = color;
  opaque.setAlpha(255);

  QLinearGradient gradient(0.0, 0.0, 1.0, 0.0);
  gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
  gradient.setColorAt(0.0, transparent);
  gradient.setColorAt(1.0, opaque);
  return QBrush(gradient);
}

int controlHeightForSize(AdColorPicker::Size size,
                         const detail::ColorPickerVisualStyle& style) {
  switch (size) {
    case AdColorPicker::Size::Small:
      return style.metrics.controlHeightSM;
    case AdColorPicker::Size::Large:
      return style.metrics.controlHeightLG;
    case AdColorPicker::Size::Middle:
    default:
      return style.metrics.controlHeight;
  }
}

int swatchSizeForSize(AdColorPicker::Size size,
                      const detail::ColorPickerVisualStyle& style) {
  switch (size) {
    case AdColorPicker::Size::Small:
      return style.metrics.swatchSizeSM;
    case AdColorPicker::Size::Large:
      return style.metrics.swatchSizeLG;
    case AdColorPicker::Size::Middle:
    default:
      return style.metrics.swatchSize;
  }
}

bool modeListContains(const QVector<AdColorPicker::Mode>& modes, AdColorPicker::Mode value) {
  return std::find(modes.cbegin(), modes.cend(), value) != modes.cend();
}

QVector<AdColorPicker::Mode> normalizeModeOptions(const QVector<AdColorPicker::Mode>& options) {
  QVector<AdColorPicker::Mode> normalized;
  normalized.reserve(options.size());
  for (AdColorPicker::Mode value : options) {
    if (modeListContains(normalized, value)) {
      continue;
    }
    normalized.append(value);
  }
  if (normalized.isEmpty()) {
    normalized.append(AdColorPicker::Mode::Single);
  }
  return normalized;
}

void clearLayout(QLayout* layout) {
  if (!layout) {
    return;
  }

  while (QLayoutItem* item = layout->takeAt(0)) {
    if (QWidget* widget = item->widget()) {
      widget->setParent(nullptr);
      widget->hide();
    }
    delete item;
  }
}

}  // namespace

AdColorPicker::AdColorPicker(QWidget* parent) : QWidget(parent) {
  setAttribute(Qt::WA_Hover, true);
  setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

  qRegisterMetaType<AdColorPicker::GradientStop>("adqt::widgets::AdColorPicker::GradientStop");
  qRegisterMetaType<AdColorPicker::ColorValue>("adqt::widgets::AdColorPicker::ColorValue");

  gradientStops_ = {
      InternalGradientStop{solidColor_, 0},
      InternalGradientStop{solidColor_, 100},
  };

  ensureUi();

  connect(&adqt::theme::ThemeManager::instance(), &adqt::theme::ThemeManager::themeChanged, this,
          [this]() { refreshStyle(); });

  refreshPanelControlsFromState();
  refreshTriggerDisplay();
  refreshStyle();
}

AdColorPicker::~AdColorPicker() = default;

AdColorPicker::Size AdColorPicker::size() const { return size_; }

void AdColorPicker::setSize(Size value) {
  if (size_ == value) {
    return;
  }
  size_ = value;
  emit sizeChanged(size_);
  refreshStyle();
}

AdColorPicker::Mode AdColorPicker::mode() const { return mode_; }

void AdColorPicker::setMode(Mode value) {
  const QVector<Mode> options = normalizeModeOptions(modeOptions_);
  const Mode resolved = modeListContains(options, value) ? value : options.constFirst();
  if (mode_ == resolved) {
    return;
  }

  mode_ = resolved;
  if (mode_ == Mode::Gradient && gradientStops_.isEmpty()) {
    gradientStops_ = {
        InternalGradientStop{solidColor_, 0},
        InternalGradientStop{solidColor_, 100},
    };
  }

  emit modeChanged(mode_);
  refreshPanelControlsFromState();
  refreshTriggerDisplay();
  emit valueChanged(this->value());
}

QVector<AdColorPicker::Mode> AdColorPicker::modeOptions() const { return modeOptions_; }

void AdColorPicker::setModeOptions(const QVector<Mode>& options) {
  const QVector<Mode> normalized = normalizeModeOptions(options);
  if (modeOptions_ == normalized) {
    return;
  }

  modeOptions_ = normalized;
  emit modeOptionsChanged(modeOptions_);

  if (!modeListContains(modeOptions_, mode_)) {
    mode_ = modeOptions_.constFirst();
    emit modeChanged(mode_);
    emit valueChanged(this->value());
  }

  updateModeComboOptions();
  refreshPanelControlsFromState();
}

AdColorPicker::Format AdColorPicker::format() const { return format_; }

void AdColorPicker::setFormat(Format value) {
  if (format_ == value) {
    return;
  }

  format_ = value;
  emit formatChanged(format_);
  updateFormatInputText();
  refreshTriggerDisplay();
}

bool AdColorPicker::allowClear() const { return allowClear_; }

void AdColorPicker::setAllowClear(bool value) {
  if (allowClear_ == value) {
    return;
  }
  allowClear_ = value;
  emit allowClearChanged(allowClear_);
  refreshStyle();
}

bool AdColorPicker::showText() const { return showText_; }

void AdColorPicker::setShowText(bool value) {
  if (showText_ == value) {
    return;
  }
  showText_ = value;
  emit showTextChanged(showText_);
  refreshStyle();
}

bool AdColorPicker::open() const { return popover_ && popover_->open(); }

void AdColorPicker::setOpen(bool value) {
  ensureUi();
  if (!popover_) {
    return;
  }
  if (open() == value) {
    return;
  }
  popover_->setOpen(value);
}

bool AdColorPicker::disabled() const { return !isEnabled(); }

void AdColorPicker::setDisabled(bool value) {
  if (disabled() == value) {
    return;
  }
  setEnabled(!value);
  if (popover_) {
    popover_->setDisabled(value);
  }
  if (value && open()) {
    setOpen(false);
  }
}

bool AdColorPicker::disabledAlpha() const { return disabledAlpha_; }

void AdColorPicker::setDisabledAlpha(bool value) {
  if (disabledAlpha_ == value) {
    return;
  }
  disabledAlpha_ = value;
  emit disabledAlphaChanged(disabledAlpha_);
  refreshPanelControlsFromState();
}

bool AdColorPicker::disabledFormat() const { return disabledFormat_; }

void AdColorPicker::setDisabledFormat(bool value) {
  if (disabledFormat_ == value) {
    return;
  }
  disabledFormat_ = value;
  emit disabledFormatChanged(disabledFormat_);
  refreshPanelControlsFromState();
}

AdColorPicker::Trigger AdColorPicker::trigger() const {
  if (!popover_) {
    return Trigger::Click;
  }
  return fromPopoverTriggers(popover_->triggerModes(), Trigger::Click);
}

void AdColorPicker::setTrigger(Trigger value) {
  ensureUi();
  if (!popover_) {
    return;
  }
  if (trigger() == value) {
    return;
  }
  popover_->setTriggerModes(toPopoverTriggers(value));
  emit triggerChanged(value);
}

AdColorPicker::Placement AdColorPicker::placement() const {
  if (!popover_) {
    return Placement::BottomLeft;
  }
  return fromPopoverPlacement(popover_->placement());
}

void AdColorPicker::setPlacement(Placement value) {
  ensureUi();
  if (!popover_) {
    return;
  }
  if (placement() == value) {
    return;
  }
  popover_->setPlacement(toPopoverPlacement(value));
  emit placementChanged(value);
}

QString AdColorPicker::value() const { return colorValueToCss(exportColorValue()); }

void AdColorPicker::setValue(const QString& value) {
  const QString trimmed = value.trimmed();
  if (trimmed.isEmpty()) {
    ColorValue clear;
    clear.cleared = true;
    setColorValue(clear);
    return;
  }

  const QRegularExpression gradientStopRe(
      QStringLiteral("(#(?:[0-9a-fA-F]{3,8})|rgba?\\([^\\)]+\\)|hsb\\([^\\)]+\\)|[a-zA-Z]+)"
                     "\\s*([0-9]+(?:\\.[0-9]+)?)%"),
      QRegularExpression::CaseInsensitiveOption);

  ColorValue next;
  if (trimmed.startsWith(QStringLiteral("linear-gradient"), Qt::CaseInsensitive)) {
    QRegularExpressionMatchIterator it = gradientStopRe.globalMatch(trimmed);
    while (it.hasNext()) {
      const QRegularExpressionMatch match = it.next();
      if (!match.hasMatch()) {
        continue;
      }
      GradientStop stop;
      stop.color = match.captured(1).trimmed();
      bool ok = false;
      const double percent = match.captured(2).toDouble(&ok);
      if (!ok) {
        continue;
      }
      stop.percent = std::clamp(qRound(percent), 0, 100);
      next.gradientStops.append(stop);
    }

    if (!next.gradientStops.isEmpty()) {
      setColorValue(next);
      return;
    }
  }

  bool ok = false;
  QColor parsed = parseColorString(trimmed, &ok);
  if (!ok || !parsed.isValid()) {
    return;
  }

  next.solidColor = trimmed;
  setColorValue(next);
}

AdColorPicker::ColorValue AdColorPicker::colorValue() const { return exportColorValue(); }

void AdColorPicker::setColorValue(const ColorValue& value) {
  importColorValue(value, false, true, true);
}

QVector<AdColorPicker::PresetItem> AdColorPicker::presets() const { return presets_; }

void AdColorPicker::setPresets(const QVector<PresetItem>& presets) {
  presets_ = presets;
  emit presetsChanged();
  rebuildPresetsPanel();
  rebuildPanelComposition();
}

QWidget* AdColorPicker::triggerWidget() const { return customTrigger_; }

void AdColorPicker::setTriggerWidget(QWidget* widget) {
  if (customTrigger_ == widget) {
    return;
  }

  ensureUi();
  customTrigger_ = widget;
  if (customTrigger_ && customTrigger_->parentWidget() != popover_) {
    customTrigger_->setParent(popover_);
  }

  if (popover_) {
    popover_->setTriggerWidget(customTrigger_ ? customTrigger_.data() : defaultTrigger_.data());
  }

  emit triggerWidgetChanged(customTrigger_);
  refreshStyle();
}

AdColorPicker::ShowTextFormatter AdColorPicker::showTextFormatter() const { return showTextFormatter_; }

void AdColorPicker::setShowTextFormatter(ShowTextFormatter formatter) {
  showTextFormatter_ = std::move(formatter);
  emit showTextFormatterChanged();
  refreshTriggerDisplay();
}

AdColorPicker::PanelRenderFactory AdColorPicker::panelRenderFactory() const { return panelRenderFactory_; }

void AdColorPicker::setPanelRenderFactory(PanelRenderFactory factory) {
  panelRenderFactory_ = std::move(factory);
  emit panelRenderFactoryChanged();
  rebuildPanelComposition();
}

AdColorPicker::ComponentTokens AdColorPicker::componentTokens() const { return componentTokens_; }

void AdColorPicker::setComponentTokens(const ComponentTokens& tokens) {
  componentTokens_ = tokens;
  emit componentTokensChanged();
  refreshStyle();
}

void AdColorPicker::resetComponentTokens() {
  componentTokens_ = {};
  emit componentTokensChanged();
  refreshStyle();
}

AdColorPicker::SemanticStyles AdColorPicker::semanticStyles() const { return semanticStyles_; }

void AdColorPicker::setSemanticStyles(const SemanticStyles& styles) {
  semanticStyles_ = styles;
  emit semanticStylesChanged();
  refreshStyle();
}

void AdColorPicker::setSemanticStyleResolver(SemanticStyleResolver resolver) {
  semanticStyleResolver_ = std::move(resolver);
  emit semanticStylesChanged();
  refreshStyle();
}

void AdColorPicker::changeEvent(QEvent* event) {
  QWidget::changeEvent(event);
  if (!event) {
    return;
  }

  if (event->type() == QEvent::EnabledChange) {
    if (popover_) {
      popover_->setDisabled(disabled());
    }
    emit disabledChanged(disabled());
    refreshStyle();
  } else if (event->type() == QEvent::FontChange || event->type() == QEvent::PaletteChange) {
    refreshStyle();
  }
}

QString AdColorPicker::formatName(Format value) {
  switch (value) {
    case Format::Hex:
      return QStringLiteral("hex");
    case Format::Rgb:
      return QStringLiteral("rgb");
    case Format::Hsb:
      return QStringLiteral("hsb");
  }
  return QStringLiteral("hex");
}

AdColorPicker::Format AdColorPicker::parseFormatName(const QString& value, Format fallback) {
  const QString normalized = value.trimmed().toLower();
  if (normalized == QStringLiteral("hex")) {
    return Format::Hex;
  }
  if (normalized == QStringLiteral("rgb")) {
    return Format::Rgb;
  }
  if (normalized == QStringLiteral("hsb")) {
    return Format::Hsb;
  }
  return fallback;
}

AdPopover::Placement AdColorPicker::toPopoverPlacement(Placement value) {
  switch (value) {
    case Placement::Top:
      return AdPopover::Placement::Top;
    case Placement::TopLeft:
      return AdPopover::Placement::TopLeft;
    case Placement::TopRight:
      return AdPopover::Placement::TopRight;
    case Placement::Bottom:
      return AdPopover::Placement::Bottom;
    case Placement::BottomLeft:
      return AdPopover::Placement::BottomLeft;
    case Placement::BottomRight:
      return AdPopover::Placement::BottomRight;
    case Placement::Left:
      return AdPopover::Placement::Left;
    case Placement::LeftTop:
      return AdPopover::Placement::LeftTop;
    case Placement::LeftBottom:
      return AdPopover::Placement::LeftBottom;
    case Placement::Right:
      return AdPopover::Placement::Right;
    case Placement::RightTop:
      return AdPopover::Placement::RightTop;
    case Placement::RightBottom:
      return AdPopover::Placement::RightBottom;
  }
  return AdPopover::Placement::BottomLeft;
}

AdColorPicker::Placement AdColorPicker::fromPopoverPlacement(AdPopover::Placement value) {
  switch (value) {
    case AdPopover::Placement::Top:
      return Placement::Top;
    case AdPopover::Placement::TopLeft:
      return Placement::TopLeft;
    case AdPopover::Placement::TopRight:
      return Placement::TopRight;
    case AdPopover::Placement::Bottom:
      return Placement::Bottom;
    case AdPopover::Placement::BottomLeft:
      return Placement::BottomLeft;
    case AdPopover::Placement::BottomRight:
      return Placement::BottomRight;
    case AdPopover::Placement::Left:
      return Placement::Left;
    case AdPopover::Placement::LeftTop:
      return Placement::LeftTop;
    case AdPopover::Placement::LeftBottom:
      return Placement::LeftBottom;
    case AdPopover::Placement::Right:
      return Placement::Right;
    case AdPopover::Placement::RightTop:
      return Placement::RightTop;
    case AdPopover::Placement::RightBottom:
      return Placement::RightBottom;
  }
  return Placement::BottomLeft;
}

AdPopover::Triggers AdColorPicker::toPopoverTriggers(Trigger value) {
  switch (value) {
    case Trigger::Hover:
      return AdPopover::Trigger::Hover;
    case Trigger::Click:
    default:
      return AdPopover::Trigger::Click;
  }
}

AdColorPicker::Trigger AdColorPicker::fromPopoverTriggers(AdPopover::Triggers value, Trigger fallback) {
  if (value.testFlag(AdPopover::Trigger::Hover)) {
    return Trigger::Hover;
  }
  if (value.testFlag(AdPopover::Trigger::Click)) {
    return Trigger::Click;
  }
  return fallback;
}

void AdColorPicker::ensureUi() {
  if (popover_) {
    return;
  }

  auto* root = new QHBoxLayout(this);
  root->setContentsMargins(0, 0, 0, 0);
  root->setSpacing(0);

  popover_ = new AdPopover(this);
  popover_->setPlacement(AdPopover::Placement::BottomLeft);
  popover_->setTriggerModes(AdPopover::Trigger::Click);
  popover_->setArrowVisible(true);
  popover_->setDisabled(disabled());
  root->addWidget(popover_);

  triggerFrame_ = new QFrame(popover_);
  triggerFrame_->setObjectName(QStringLiteral("ad-color-picker-trigger-frame"));
  triggerFrame_->setAttribute(Qt::WA_StyledBackground, true);
  auto* triggerLayout = new QHBoxLayout(triggerFrame_);
  triggerLayout->setContentsMargins(8, 4, 8, 4);
  triggerLayout->setSpacing(8);

  triggerSwatch_ = new QWidget(triggerFrame_);
  triggerSwatch_->setObjectName(QStringLiteral("ad-color-picker-trigger-swatch"));
  triggerSwatch_->setAttribute(Qt::WA_StyledBackground, true);

  triggerTextLabel_ = new QLabel(triggerFrame_);
  triggerTextLabel_->setObjectName(QStringLiteral("ad-color-picker-trigger-text"));
  triggerTextLabel_->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

  triggerLayout->addWidget(triggerSwatch_);
  triggerLayout->addWidget(triggerTextLabel_);

  defaultTrigger_ = triggerFrame_;
  popover_->setTriggerWidget(defaultTrigger_);

  panelHost_ = new QWidget(popover_);
  panelHost_->setObjectName(QStringLiteral("ad-color-picker-panel-host"));
  panelHost_->setAttribute(Qt::WA_StyledBackground, true);
  popover_->setContentWidget(panelHost_);

  connect(popover_, &AdPopover::openChanged, this, [this](bool openValue) {
    emit openChanged(openValue);
    emit onOpenChange(openValue);
    refreshStyle();
  });

  ensurePickerPanel();
  rebuildPanelComposition();
}

void AdColorPicker::ensurePickerPanel() {
  if (pickerPanel_) {
    return;
  }

  pickerPanel_ = new QWidget(panelHost_);
  pickerPanel_->setObjectName(QStringLiteral("ad-color-picker-picker-panel"));
  pickerPanel_->setAttribute(Qt::WA_StyledBackground, true);

  auto* pickerLayout = new QVBoxLayout(pickerPanel_);
  pickerLayout->setContentsMargins(0, 0, 0, 0);
  pickerLayout->setSpacing(8);

  operationRow_ = new QWidget(pickerPanel_);
  operationRow_->setObjectName(QStringLiteral("ad-color-picker-operation-row"));
  auto* operationRow = new QHBoxLayout(operationRow_);
  operationRow->setContentsMargins(0, 0, 0, 0);
  operationRow->setSpacing(8);
  modeCombo_ = new QComboBox(operationRow_);
  clearButton_ = new QPushButton(QStringLiteral("Clear"), operationRow_);
  modeCombo_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  operationRow->addWidget(modeCombo_, 1);
  operationRow->addWidget(clearButton_, 0, Qt::AlignRight);
  pickerLayout->addWidget(operationRow_);

  gradientSection_ = new QWidget(pickerPanel_);
  gradientSection_->setObjectName(QStringLiteral("ad-color-picker-gradient-section"));
  auto* gradientLayout = new QVBoxLayout(gradientSection_);
  gradientLayout->setContentsMargins(0, 0, 0, 0);
  gradientLayout->setSpacing(0);

  gradientSlider_ = new AdSlider(gradientSection_);
  gradientSlider_->setMode(AdSlider::Mode::Range);
  gradientSlider_->setMinimum(0);
  gradientSlider_->setMaximum(100);
  gradientSlider_->setStep(1);
  gradientSlider_->setTooltipEnabled(false);
  gradientSlider_->setIncluded(false);
  gradientSlider_->setEditableHandles(true);
  gradientSlider_->setMinHandleCount(2);
  gradientSlider_->setValues({0.0, 100.0});
  AdSlider::ComponentTokens gradientTokens;
  gradientTokens.railSize = g_sliderHeight;
  gradientTokens.handleSize = 8;
  gradientTokens.handleSizeHover = 10;
  gradientTokens.handleLineWidth = 2;
  gradientTokens.handleLineWidthHover = 2;
  gradientSlider_->setComponentTokens(gradientTokens);

  gradientLayout->addWidget(gradientSlider_);
  pickerLayout->addWidget(gradientSection_);

  saturationPanel_ = new ColorSaturationPanel(pickerPanel_);
  saturationPanel_->setObjectName(QStringLiteral("ad-color-picker-saturation-panel"));
  saturationPanel_->setMinimumHeight(g_saturationPanelHeight);
  saturationPanel_->setMaximumHeight(g_saturationPanelHeight);
  pickerLayout->addWidget(saturationPanel_);

  sliderContainer_ = new QWidget(pickerPanel_);
  sliderContainer_->setObjectName(QStringLiteral("ad-color-picker-slider-container"));
  auto* sliderContainerLayout = new QHBoxLayout(sliderContainer_);
  sliderContainerLayout->setContentsMargins(0, 0, 0, 0);
  sliderContainerLayout->setSpacing(8);

  sliderGroup_ = new QWidget(sliderContainer_);
  auto* sliderGroupLayout = new QVBoxLayout(sliderGroup_);
  sliderGroupLayout->setContentsMargins(0, 0, 0, 0);
  sliderGroupLayout->setSpacing(8);

  hueSlider_ = new AdSlider(sliderGroup_);
  hueSlider_->setMinimum(0);
  hueSlider_->setMaximum(359);
  hueSlider_->setStep(1);
  hueSlider_->setIncluded(false);
  hueSlider_->setTooltipEnabled(false);
  AdSlider::ComponentTokens hueTokens;
  hueTokens.railSize = g_sliderHeight;
  hueTokens.handleSize = 8;
  hueTokens.handleSizeHover = 10;
  hueTokens.handleLineWidth = 2;
  hueTokens.handleLineWidthHover = 2;
  hueSlider_->setComponentTokens(hueTokens);
  sliderGroupLayout->addWidget(hueSlider_);

  alphaSection_ = new QWidget(sliderGroup_);
  alphaSection_->setObjectName(QStringLiteral("ad-color-picker-alpha-section"));
  auto* alphaRow = new QHBoxLayout(alphaSection_);
  alphaRow->setContentsMargins(0, 0, 0, 0);
  alphaRow->setSpacing(0);
  alphaSlider_ = new AdSlider(alphaSection_);
  alphaSlider_->setMinimum(0);
  alphaSlider_->setMaximum(100);
  alphaSlider_->setStep(1);
  alphaSlider_->setIncluded(false);
  alphaSlider_->setTooltipEnabled(false);
  AdSlider::ComponentTokens alphaTokens;
  alphaTokens.railSize = g_sliderHeight;
  alphaTokens.handleSize = 8;
  alphaTokens.handleSizeHover = 10;
  alphaTokens.handleLineWidth = 2;
  alphaTokens.handleLineWidthHover = 2;
  alphaSlider_->setComponentTokens(alphaTokens);
  alphaRow->addWidget(alphaSlider_);
  sliderGroupLayout->addWidget(alphaSection_);

  sliderContainerLayout->addWidget(sliderGroup_, 1);

  previewSwatch_ = new QWidget(sliderContainer_);
  previewSwatch_->setObjectName(QStringLiteral("ad-color-picker-preview-swatch"));
  previewSwatch_->setAttribute(Qt::WA_StyledBackground, true);
  previewSwatch_->setFixedSize(g_previewSwatchSize, g_previewSwatchSize);
  sliderContainerLayout->addWidget(previewSwatch_, 0, Qt::AlignBottom);
  pickerLayout->addWidget(sliderContainer_);

  auto* formatRowWidget = new QWidget(pickerPanel_);
  formatRowWidget->setObjectName(QStringLiteral("ad-color-picker-format-row"));
  auto* formatRow = new QHBoxLayout(formatRowWidget);
  formatRow->setContentsMargins(0, 0, 0, 0);
  formatRow->setSpacing(8);
  formatCombo_ = new QComboBox(formatRowWidget);
  formatCombo_->addItem(QStringLiteral("HEX"), QStringLiteral("hex"));
  formatCombo_->addItem(QStringLiteral("RGB"), QStringLiteral("rgb"));
  formatCombo_->addItem(QStringLiteral("HSB"), QStringLiteral("hsb"));
  formatInput_ = new QLineEdit(formatRowWidget);
  alphaInput_ = new QLineEdit(formatRowWidget);
  alphaInput_->setAlignment(Qt::AlignCenter);
  alphaInput_->setFixedWidth(g_alphaInputWidth);
  alphaInput_->setPlaceholderText(QStringLiteral("100%"));
  formatRow->addWidget(formatCombo_);
  formatRow->addWidget(formatInput_, 1);
  formatRow->addWidget(alphaInput_);
  pickerLayout->addWidget(formatRowWidget);

  presetsPanel_ = new QWidget(panelHost_);
  presetsPanel_->setObjectName(QStringLiteral("ad-color-picker-presets-panel"));
  presetsPanel_->setAttribute(Qt::WA_StyledBackground, true);
  presetsLayout_ = new QVBoxLayout(presetsPanel_);
  presetsLayout_->setContentsMargins(0, 0, 0, 0);
  presetsLayout_->setSpacing(8);

  connect(modeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
    if (syncingControls_ || !modeCombo_ || index < 0) {
      return;
    }
    const QVariant data = modeCombo_->itemData(index);
    setMode(static_cast<Mode>(data.toInt()));
  });

  connect(clearButton_, &QPushButton::clicked, this, [this]() {
    if (!allowClear_ || cleared_) {
      return;
    }
    cleared_ = true;
    refreshPanelControlsFromState();
    refreshTriggerDisplay();
    emit cleared();
    emit onClear();
    emitChangeSignals(true, true);
  });

  connect(gradientSlider_, &AdSlider::valuesChanged, this,
          [this](const QList<double>& values) { setGradientStopsFromSlider(values, false); });
  connect(gradientSlider_, &AdSlider::valuesChangeCompleted, this,
          [this](const QList<double>& values) { setGradientStopsFromSlider(values, true); });

  if (saturationPanel_) {
    saturationPanel_->setChangeCallback([this](double, double, bool completed) {
      if (syncingControls_) {
        return;
      }
      setCurrentFromControls(completed);
    });
  }

  auto bindChannel = [this](AdSlider* slider) {
    connect(slider, &AdSlider::valueChanged, this, [this](double) {
      if (syncingControls_) {
        return;
      }
      setCurrentFromControls(false);
    });
    connect(slider, &AdSlider::valueChangeCompleted, this, [this](double) {
      if (syncingControls_) {
        return;
      }
      setCurrentFromControls(true);
    });
  };

  bindChannel(hueSlider_);
  bindChannel(alphaSlider_);

  connect(formatCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
    if (syncingControls_ || !formatCombo_ || index < 0) {
      return;
    }
    setFormat(parseFormatName(formatCombo_->itemData(index).toString(), format_));
  });

  connect(formatInput_, &QLineEdit::editingFinished, this, [this]() {
    if (syncingControls_ || !formatInput_) {
      return;
    }
    bool ok = false;
    const QColor color = parseColorString(formatInput_->text(), &ok);
    if (!ok || !color.isValid()) {
      updateFormatInputText();
      return;
    }
    setCurrentEditableColor(color, true, true, true);
  });

  connect(alphaInput_, &QLineEdit::editingFinished, this, [this]() {
    if (syncingControls_ || !alphaInput_) {
      return;
    }
    QString text = alphaInput_->text().trimmed();
    if (text.endsWith(QLatin1Char('%'))) {
      text.chop(1);
    }

    bool ok = false;
    int percent = qRound(text.toDouble(&ok));
    if (!ok) {
      refreshPanelControlsFromState();
      return;
    }
    percent = std::clamp(percent, 0, 100);

    QColor current = currentEditableColor().toHsv();
    int hue = current.hue();
    if (hue < 0) {
      hue = 0;
    }
    const int alpha = std::clamp(qRound(percent * 255.0 / 100.0), 0, 255);
    const QColor next = QColor::fromHsv(hue, current.saturation(), current.value(), alpha);
    setCurrentEditableColor(next, true, true, true);
  });

  updateModeComboOptions();
  rebuildPresetsPanel();
}

void AdColorPicker::rebuildPanelComposition() {
  ensureUi();
  if (!panelHost_ || !pickerPanel_ || !presetsPanel_) {
    return;
  }

  if (QLayout* layout = panelHost_->layout()) {
    clearLayout(layout);
    delete layout;
  }

  QWidget* nextPanel = nullptr;
  if (panelRenderFactory_) {
    nextPanel = panelRenderFactory_(panelHost_, pickerPanel_, presetsPanel_);
    if (nextPanel && nextPanel->parentWidget() != panelHost_) {
      nextPanel->setParent(panelHost_);
    }
  }

  if (!nextPanel) {
    auto* defaultPanel = new QWidget(panelHost_);
    auto* layout = new QVBoxLayout(defaultPanel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    layout->addWidget(pickerPanel_);
    if (!presets_.isEmpty()) {
      layout->addWidget(presetsPanel_);
    }
    nextPanel = defaultPanel;
  }

  composedPanel_ = nextPanel;

  auto* hostLayout = new QVBoxLayout(panelHost_);
  hostLayout->setContentsMargins(0, 0, 0, 0);
  hostLayout->setSpacing(0);
  hostLayout->addWidget(composedPanel_);

  refreshStyle();
}

void AdColorPicker::rebuildPresetsPanel() {
  if (!presetsPanel_ || !presetsLayout_) {
    return;
  }

  while (QLayoutItem* item = presetsLayout_->takeAt(0)) {
    if (QWidget* widget = item->widget()) {
      delete widget;
    }
    delete item;
  }

  if (presets_.isEmpty()) {
    presetsPanel_->setVisible(false);
    return;
  }

  for (const PresetItem& preset : presets_) {
    auto* group = new QWidget(presetsPanel_);
    auto* groupLayout = new QVBoxLayout(group);
    groupLayout->setContentsMargins(0, 0, 0, 0);
    groupLayout->setSpacing(4);

    if (!preset.label.trimmed().isEmpty()) {
      auto* label = new QLabel(preset.label, group);
      QFont labelFont = label->font();
      labelFont.setBold(true);
      label->setFont(labelFont);
      groupLayout->addWidget(label);
    }

    auto* swatchRow = new QHBoxLayout();
    swatchRow->setContentsMargins(0, 0, 0, 0);
    swatchRow->setSpacing(6);

    for (const ColorValue& value : preset.colors) {
      auto* swatch = new QPushButton(group);
      swatch->setObjectName(QString::fromLatin1(kPresetSwatchObjectName));
      swatch->setCursor(Qt::PointingHandCursor);
      swatch->setCheckable(false);
      swatch->setToolTip(colorValueToCss(value));
      swatch->setProperty("ad-color-picker-css", colorValueToCss(value));
      connect(swatch, &QPushButton::clicked, this, [this, value]() { applyPreset(value); });
      swatchRow->addWidget(swatch);
    }

    swatchRow->addStretch();
    groupLayout->addLayout(swatchRow);
    presetsLayout_->addWidget(group);
  }

  presetsPanel_->setVisible(true);
}

void AdColorPicker::refreshStyle() {
  if (!popover_) {
    return;
  }

  SemanticStyles mergedSemantic = semanticStyles_;
  if (semanticStyleResolver_) {
    StyleContext context;
    context.mode = mode_;
    context.format = format_;
    context.open = open();
    context.disabled = disabled();
    context.cleared = cleared_;
    const SemanticStyles resolved = semanticStyleResolver_(context);

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

    mergeSlot(&mergedSemantic.root, resolved.root);
    mergeSlot(&mergedSemantic.body, resolved.body);
    mergeSlot(&mergedSemantic.content, resolved.content);
    mergeSlot(&mergedSemantic.description, resolved.description);
    mergeSlot(&mergedSemantic.popup, resolved.popup);
  }

  detail::ColorPickerStyleInput input;
  input.size = size_;
  input.open = open();
  input.disabled = disabled();
  input.showText = showText_;
  input.cleared = cleared_;
  input.baseFont = font();
  input.componentTokens = componentTokens_;
  input.semanticStyles = mergedSemantic;
  const detail::ColorPickerVisualStyle style = detail::resolveColorPickerVisualStyle(input);

  // Update global internal metrics for use in non-refreshStyle contexts
  updateInternalMetrics(style.metrics);

  const int controlHeight = controlHeightForSize(size_, style);
  const int swatchSize = swatchSizeForSize(size_, style);

  if (defaultTrigger_) {
    defaultTrigger_->setMinimumHeight(controlHeight);
    defaultTrigger_->setMaximumHeight(controlHeight);
    defaultTrigger_->setMinimumWidth(style.metrics.triggerMinWidth);
    defaultTrigger_->setFont(style.metrics.font);
  }

  if (triggerFrame_) {
    const QColor border = disabled()
                              ? style.triggerBorder
                              : (open() ? style.triggerBorderActive : style.triggerBorder);

    if (auto* triggerLayout = qobject_cast<QHBoxLayout*>(triggerFrame_->layout())) {
      const int padH = std::max(0, style.metrics.triggerPadding * 2);
      const int padV = std::max(0, style.metrics.triggerPadding);
      triggerLayout->setContentsMargins(padH, padV, padH, padV);
      triggerLayout->setSpacing(std::max(0, style.metrics.triggerTextGap));
    }

    triggerFrame_->setStyleSheet(
        QStringLiteral("background:%1; border:%2px solid %3; border-radius:%4px;")
            .arg(disabled() ? style.triggerBackgroundDisabled.name(QColor::HexArgb)
                            : style.triggerBackground.name(QColor::HexArgb))
            .arg(QString::number(style.metrics.borderWidth, 'f', 1))
            .arg(border.name(QColor::HexArgb))
            .arg(style.metrics.triggerRadius));
  }

  if (triggerSwatch_) {
    triggerSwatch_->setFixedSize(swatchSize, swatchSize);
  }

  if (triggerTextLabel_) {
    QPalette palette = triggerTextLabel_->palette();
    palette.setColor(QPalette::WindowText,
                     disabled() ? style.triggerTextDisabled : style.triggerText);
    triggerTextLabel_->setPalette(palette);
    triggerTextLabel_->setVisible(showText_);
  }

  if (panelHost_) {
    panelHost_->setFixedWidth(style.metrics.panelWidth);
    panelHost_->setStyleSheet(
        QStringLiteral("background:%1; border:%2px solid %3; border-radius:%4px; padding:%5px;")
            .arg(style.panelBackground.name(QColor::HexArgb))
            .arg(QString::number(style.metrics.borderWidth, 'f', 1))
            .arg(style.panelBorder.name(QColor::HexArgb))
            .arg(style.metrics.triggerRadius)
            .arg(style.metrics.panelPadding));
  }

  if (pickerPanel_) {
    pickerPanel_->setFont(style.metrics.font);
    if (auto* pickerLayout = qobject_cast<QVBoxLayout*>(pickerPanel_->layout())) {
      pickerLayout->setSpacing(std::max(0, style.metrics.panelSpacing));
    }
  }
  if (presetsPanel_) {
    presetsPanel_->setFont(style.metrics.font);
    if (presetsLayout_) {
      presetsLayout_->setSpacing(std::max(0, style.metrics.panelSpacing));
    }
  }

  if (composedPanel_) {
    if (auto* composedLayout = qobject_cast<QVBoxLayout*>(composedPanel_->layout())) {
      composedLayout->setSpacing(std::max(0, style.metrics.panelSpacing));
    }
  }

  const QVector<Mode> normalizedModes = normalizeModeOptions(modeOptions_);
  const bool showModeSwitch = normalizedModes.size() > 1;

  if (operationRow_) {
    operationRow_->setVisible(showModeSwitch || allowClear_);
  }
  if (modeCombo_) {
    modeCombo_->setVisible(showModeSwitch);
    modeCombo_->setMinimumHeight(style.metrics.inputHeight);
    modeCombo_->setMaximumHeight(style.metrics.inputHeight);
    modeCombo_->setEnabled(showModeSwitch && !disabled());
    modeCombo_->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  }
  if (formatCombo_) {
    formatCombo_->setVisible(!disabledFormat_);
    formatCombo_->setMinimumHeight(style.metrics.inputHeight);
    formatCombo_->setMaximumHeight(style.metrics.inputHeight);
    formatCombo_->setEnabled(!disabledFormat_ && !disabled());
  }
  if (formatInput_) {
    formatInput_->setMinimumHeight(style.metrics.inputHeight);
    formatInput_->setMaximumHeight(style.metrics.inputHeight);
    formatInput_->setEnabled(!disabled());
  }
  if (alphaInput_) {
    alphaInput_->setMinimumHeight(style.metrics.inputHeight);
    alphaInput_->setMaximumHeight(style.metrics.inputHeight);
    alphaInput_->setEnabled(!disabled() && !disabledAlpha_);
    alphaInput_->setVisible(!disabledAlpha_);
  }

  if (clearButton_) {
    clearButton_->setVisible(allowClear_);
    clearButton_->setEnabled(!disabled());
    clearButton_->setMinimumHeight(style.metrics.inputHeight);
    clearButton_->setMaximumHeight(style.metrics.inputHeight);
  }

  if (gradientSection_) {
    gradientSection_->setVisible(mode_ == Mode::Gradient);
  }
  if (gradientSlider_) {
    gradientSlider_->setEnabled(!disabled());
  }

  if (saturationPanel_) {
    saturationPanel_->setEnabled(!disabled());
    saturationPanel_->setMinimumHeight(g_saturationPanelHeight);
    saturationPanel_->setMaximumHeight(g_saturationPanelHeight);
  }

  if (alphaSection_) {
    alphaSection_->setVisible(!disabledAlpha_);
  }
  if (hueSlider_) {
    hueSlider_->setEnabled(!disabled());
  }
  if (alphaSlider_) {
    alphaSlider_->setEnabled(!disabledAlpha_ && !disabled());
  }
  if (previewSwatch_) {
    previewSwatch_->setFixedSize(g_previewSwatchSize, g_previewSwatchSize);
  }

  AdPopover::ComponentTokens popoverTokens;
  popoverTokens.popupBg = style.panelBackground.name(QColor::HexArgb);
  popoverTokens.popupBorderColor = style.panelBorder.name(QColor::HexArgb);
  popoverTokens.borderRadius = style.metrics.triggerRadius;
  popoverTokens.borderWidth = qRound(style.metrics.borderWidth);
  popoverTokens.popupPadding = style.metrics.panelPadding;
  popover_->setComponentTokens(popoverTokens);
  popover_->setPlacement(toPopoverPlacement(placement()));
  popover_->setTriggerModes(toPopoverTriggers(trigger()));
  popover_->setDisabled(disabled());

  const QList<QPushButton*> swatches =
      presetsPanel_ ? presetsPanel_->findChildren<QPushButton*>(QString::fromLatin1(kPresetSwatchObjectName),
                                                                 Qt::FindChildrenRecursively)
                    : QList<QPushButton*>();
  for (QPushButton* swatch : swatches) {
    if (!swatch) {
      continue;
    }
    swatch->setFixedSize(style.metrics.presetSwatchSize, style.metrics.presetSwatchSize);
    const QString css = swatch->property("ad-color-picker-css").toString();
    if (css.startsWith(QStringLiteral("linear-gradient"), Qt::CaseInsensitive)) {
      const QRegularExpression stopRe(
          QStringLiteral("(#(?:[0-9a-fA-F]{3,8})|rgba?\\([^\\)]+\\)|hsb\\([^\\)]+\\)|[a-zA-Z]+)"
                         "\\s*([0-9]+(?:\\.[0-9]+)?)%"),
          QRegularExpression::CaseInsensitiveOption);
      QStringList stops;
      QRegularExpressionMatchIterator it = stopRe.globalMatch(css);
      while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        if (!match.hasMatch()) {
          continue;
        }
        bool ok = false;
        const double percent = match.captured(2).toDouble(&ok);
        if (!ok) {
          continue;
        }
        const double stopPos = std::clamp(percent / 100.0, 0.0, 1.0);
        stops.append(QStringLiteral("stop:%1 %2")
                         .arg(formatPercent(stopPos))
                         .arg(match.captured(1).trimmed()));
      }
      const QString gradient = stops.isEmpty() ? QStringLiteral("#1677ff") : stops.join(QStringLiteral(", "));
      swatch->setStyleSheet(QStringLiteral("border:1px solid %1; border-radius:%2px;"
                                           "background:qlineargradient(x1:0,y1:0,x2:1,y2:0,%3);")
                                .arg(style.presetBorder.name(QColor::HexArgb))
                                .arg(style.metrics.swatchRadius)
                                .arg(gradient));
    } else {
      swatch->setStyleSheet(
          QStringLiteral("border:1px solid %1; border-radius:%2px; background:%3;")
              .arg(style.presetBorder.name(QColor::HexArgb))
              .arg(style.metrics.swatchRadius)
              .arg(css.isEmpty() ? QStringLiteral("transparent") : css));
    }
  }

  refreshChannelVisuals();
  refreshPreviewSwatch();
  refreshTriggerDisplay();
  refreshPanelControlsFromState();
}

void AdColorPicker::refreshTriggerDisplay() {
  if (!triggerSwatch_) {
    return;
  }

  detail::ColorPickerStyleInput input;
  input.size = size_;
  input.open = open();
  input.disabled = disabled();
  input.showText = showText_;
  input.cleared = cleared_;
  input.baseFont = font();
  input.componentTokens = componentTokens_;
  input.semanticStyles = semanticStyles_;
  const detail::ColorPickerVisualStyle style = detail::resolveColorPickerVisualStyle(input);

  if (cleared_) {
    triggerSwatch_->setStyleSheet(QStringLiteral("border:1px dashed %1; border-radius:%2px; background:transparent;")
                                      .arg(style.swatchBorder.name(QColor::HexArgb))
                                      .arg(style.metrics.swatchRadius));
  } else if (mode_ == Mode::Gradient && !gradientStops_.isEmpty()) {
    QStringList stops;
    const QVector<InternalGradientStop> normalized = normalizeGradientStops(gradientStops_);
    for (const InternalGradientStop& stop : normalized) {
      const double stopPos = std::clamp(static_cast<double>(stop.percent) / 100.0, 0.0, 1.0);
      stops.append(QStringLiteral("stop:%1 %2")
                       .arg(formatPercent(stopPos))
                       .arg(colorToCss(stop.color)));
    }
    triggerSwatch_->setStyleSheet(
        QStringLiteral("border:1px solid %1; border-radius:%2px;"
                       "background:qlineargradient(x1:0,y1:0,x2:1,y2:0,%3);")
            .arg(style.swatchBorder.name(QColor::HexArgb))
            .arg(style.metrics.swatchRadius)
            .arg(stops.join(QStringLiteral(", "))));
  } else {
    triggerSwatch_->setStyleSheet(QStringLiteral("border:1px solid %1; border-radius:%2px; background:%3;")
                                      .arg(style.swatchBorder.name(QColor::HexArgb))
                                      .arg(style.metrics.swatchRadius)
                                      .arg(colorToCss(solidColor_)));
  }

  if (triggerTextLabel_) {
    if (!showText_) {
      triggerTextLabel_->clear();
      return;
    }

    const ColorValue displayValue = exportColorValue();
    QString text;
    if (showTextFormatter_) {
      text = showTextFormatter_(displayValue, format_, activeStopIndex_);
    }

    if (text.trimmed().isEmpty()) {
      if (displayValue.cleared) {
        text = QStringLiteral("none");
      } else if (mode_ == Mode::Gradient && !displayValue.gradientStops.isEmpty()) {
        text = colorValueToCss(displayValue);
      } else {
        text = colorToString(currentEditableColor(), format_);
      }
    }
    triggerTextLabel_->setText(text);
  }
}

void AdColorPicker::refreshPanelControlsFromState() {
  if (!pickerPanel_) {
    return;
  }

  QScopedValueRollback<bool> guard(syncingControls_, true);

  updateModeComboOptions();
  if (modeCombo_) {
    const int index = modeCombo_->findData(static_cast<int>(mode_));
    if (index >= 0 && modeCombo_->currentIndex() != index) {
      modeCombo_->setCurrentIndex(index);
    }
  }

  const QVector<InternalGradientStop> normalized = normalizeGradientStops(gradientStops_);
  if (gradientSlider_) {
    QList<double> values;
    values.reserve(normalized.size());
    for (const InternalGradientStop& stop : normalized) {
      values.append(stop.percent);
    }
    gradientSlider_->setValues(values);
    gradientSlider_->setEnabled(!disabled());
  }

  const QColor color = currentEditableColor().toHsv();
  int hue = color.hue();
  if (hue < 0) {
    hue = 0;
  }
  const int sat = qRound(color.saturationF() * 100.0);
  const int bri = qRound(color.valueF() * 100.0);
  const int alpha = qRound(color.alphaF() * 100.0);

  if (hueSlider_) {
    hueSlider_->setValue(hue);
    hueSlider_->setEnabled(!disabled());
  }
  if (saturationPanel_) {
    saturationPanel_->setHue(hue);
    saturationPanel_->setSaturationBrightness(sat / 100.0, bri / 100.0);
    saturationPanel_->setEnabled(!disabled());
  }
  if (alphaSlider_) {
    alphaSlider_->setValue(alpha);
    alphaSlider_->setEnabled(!disabledAlpha_ && !disabled());
  }

  const bool showModeSwitch = normalizeModeOptions(modeOptions_).size() > 1;
  if (operationRow_) {
    operationRow_->setVisible(showModeSwitch || allowClear_);
  }
  if (gradientSection_) {
    gradientSection_->setVisible(mode_ == Mode::Gradient);
  }
  if (alphaSection_) {
    alphaSection_->setVisible(!disabledAlpha_);
  }
  if (formatCombo_) {
    const int idx = formatCombo_->findData(formatName(format_));
    if (idx >= 0) {
      formatCombo_->setCurrentIndex(idx);
    }
    formatCombo_->setEnabled(!disabledFormat_ && !disabled());
    formatCombo_->setVisible(!disabledFormat_);
  }
  if (alphaInput_) {
    if (cleared_) {
      alphaInput_->clear();
    } else {
      alphaInput_->setText(QStringLiteral("%1%").arg(alpha));
    }
    alphaInput_->setEnabled(!disabled() && !disabledAlpha_);
    alphaInput_->setVisible(!disabledAlpha_);
  }

  refreshChannelVisuals();
  refreshPreviewSwatch();
  updateFormatInputText();
}

void AdColorPicker::updateModeComboOptions() {
  if (!modeCombo_) {
    return;
  }

  const QVector<Mode> normalized = normalizeModeOptions(modeOptions_);
  QSignalBlocker blocker(modeCombo_);
  modeCombo_->clear();
  for (Mode value : normalized) {
    modeCombo_->addItem(value == Mode::Gradient ? QStringLiteral("gradient") : QStringLiteral("single"),
                        static_cast<int>(value));
  }
  modeCombo_->setEnabled(normalized.size() > 1 && !disabled());

  if (!modeListContains(normalized, mode_)) {
    mode_ = normalized.constFirst();
    emit modeChanged(mode_);
  }

  const int index = modeCombo_->findData(static_cast<int>(mode_));
  if (index >= 0) {
    modeCombo_->setCurrentIndex(index);
  }
}

void AdColorPicker::refreshChannelVisuals() {
  if (hueSlider_) {
    AdSlider::SemanticStyles hueStyles;
    hueStyles.rail.brush = makeHueBrush();
    hueStyles.handle.borderColor = QColor("#ffffff");
    hueStyles.handle.backgroundColor = QColor(0, 0, 0, 0);
    hueSlider_->setSemanticStyles(hueStyles);
  }

  if (alphaSlider_) {
    AdSlider::SemanticStyles alphaStyles;
    alphaStyles.rail.brush = makeCheckerBrush(kTransparencyCell);
    alphaStyles.tracks.brush = makeAlphaBrush(currentEditableColor());
    alphaStyles.handle.borderColor = QColor("#ffffff");
    alphaStyles.handle.backgroundColor = QColor(0, 0, 0, 0);
    alphaSlider_->setSemanticStyles(alphaStyles);
  }

  if (gradientSlider_) {
    const QVector<InternalGradientStop> normalized = normalizeGradientStops(gradientStops_);
    QLinearGradient gradient(0.0, 0.0, 1.0, 0.0);
    gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    for (const InternalGradientStop& stop : normalized) {
      const double stopPos = std::clamp(static_cast<double>(stop.percent) / 100.0, 0.0, 1.0);
      gradient.setColorAt(stopPos, stop.color);
    }

    AdSlider::SemanticStyles gradientStyles;
    gradientStyles.rail.brush = QBrush(gradient);
    gradientStyles.handle.borderColor = QColor("#ffffff");
    gradientStyles.handle.backgroundColor = QColor(0, 0, 0, 0);
    gradientSlider_->setSemanticStyles(gradientStyles);
  }
}

void AdColorPicker::refreshPreviewSwatch() {
  if (!previewSwatch_) {
    return;
  }

  detail::ColorPickerStyleInput input;
  input.size = size_;
  input.open = open();
  input.disabled = disabled();
  input.showText = showText_;
  input.cleared = cleared_;
  input.baseFont = font();
  input.componentTokens = componentTokens_;
  input.semanticStyles = semanticStyles_;
  const detail::ColorPickerVisualStyle style = detail::resolveColorPickerVisualStyle(input);

  const int radius = style.metrics.previewSwatchRadius;
  if (cleared_) {
    previewSwatch_->setStyleSheet(QStringLiteral("border:1px dashed %1; border-radius:%2px; background:transparent;")
                                      .arg(style.swatchBorder.name(QColor::HexArgb))
                                      .arg(radius));
    return;
  }

  const QString css = colorToCss(currentEditableColor());
  previewSwatch_->setStyleSheet(
      QStringLiteral("border:1px solid %1; border-radius:%2px; background:%3;")
          .arg(style.swatchBorder.name(QColor::HexArgb))
          .arg(radius)
          .arg(css));
}

void AdColorPicker::updateFormatInputText() {
  if (!formatInput_) {
    return;
  }

  if (cleared_) {
    formatInput_->clear();
    return;
  }

  QString text = colorToString(currentEditableColor(), format_);
  if (format_ == Format::Hex) {
    text = text.toUpper();
  }
  formatInput_->setText(text);
}

QColor AdColorPicker::currentEditableColor() const {
  if (mode_ == Mode::Gradient && !gradientStops_.isEmpty()) {
    const int maxIndex = std::max(0, static_cast<int>(gradientStops_.size()) - 1);
    const int index = std::clamp(activeStopIndex_, 0, maxIndex);
    return gradientStops_.at(index).color;
  }
  return solidColor_;
}

void AdColorPicker::setCurrentEditableColor(const QColor& color,
                                            bool fromUser,
                                            bool emitCompleted,
                                            bool emitValueSignal) {
  Q_UNUSED(fromUser)
  if (!color.isValid()) {
    return;
  }

  cleared_ = false;
  if (mode_ == Mode::Gradient) {
    if (gradientStops_.isEmpty()) {
      gradientStops_ = {
          InternalGradientStop{color, 0},
          InternalGradientStop{color, 100},
      };
      activeStopIndex_ = 0;
    }
    const int maxIndex = std::max(0, static_cast<int>(gradientStops_.size()) - 1);
    activeStopIndex_ = std::clamp(activeStopIndex_, 0, maxIndex);
    gradientStops_[activeStopIndex_].color = color;
  } else {
    solidColor_ = color;
  }

  refreshPanelControlsFromState();
  refreshTriggerDisplay();
  emitChangeSignals(emitCompleted, emitValueSignal);
}

void AdColorPicker::setCurrentFromControls(bool emitCompleted) {
  if (syncingControls_ || !hueSlider_ || !saturationPanel_) {
    return;
  }

  const int hue = std::clamp(qRound(hueSlider_->value()), 0, 359);
  const int sat = std::clamp(qRound(saturationPanel_->saturation() * 255.0), 0, 255);
  const int bri = std::clamp(qRound(saturationPanel_->brightness() * 255.0), 0, 255);
  const int alpha =
      alphaSlider_ ? std::clamp(qRound(alphaSlider_->value() * 255.0 / 100.0), 0, 255) : 255;

  QColor color = QColor::fromHsv(hue, sat, bri, alpha);
  setCurrentEditableColor(color, true, emitCompleted, true);
}

void AdColorPicker::setGradientStopsFromSlider(const QList<double>& values, bool emitCompleted) {
  if (syncingControls_ || values.isEmpty()) {
    return;
  }

  QVector<int> percents;
  percents.reserve(values.size());
  for (double value : values) {
    percents.append(std::clamp(qRound(value), 0, 100));
  }
  std::sort(percents.begin(), percents.end());

  QVector<InternalGradientStop> next;
  next.reserve(percents.size());
  const QVector<InternalGradientStop> current = normalizeGradientStops(gradientStops_);
  for (int i = 0; i < percents.size(); ++i) {
    InternalGradientStop stop;
    stop.percent = percents.at(i);
    if (i < current.size()) {
      stop.color = current.at(i).color;
    } else if (!current.isEmpty()) {
      stop.color = current.constLast().color;
    } else {
      stop.color = solidColor_;
    }
    next.append(stop);
  }

  const int currentPercent = !gradientStops_.isEmpty()
                                 ? gradientStops_.at(std::clamp(
                                       activeStopIndex_, 0,
                                       std::max(0, static_cast<int>(gradientStops_.size()) - 1)))
                                       .percent
                                 : 0;
  gradientStops_ = normalizeGradientStops(next);

  int nearest = 0;
  int nearestDistance = std::numeric_limits<int>::max();
  for (int i = 0; i < gradientStops_.size(); ++i) {
    const int distance = std::abs(gradientStops_.at(i).percent - currentPercent);
    if (distance < nearestDistance) {
      nearestDistance = distance;
      nearest = i;
    }
  }
  activeStopIndex_ = nearest;

  cleared_ = false;
  refreshPanelControlsFromState();
  refreshTriggerDisplay();
  emitChangeSignals(emitCompleted, true);
}

QColor AdColorPicker::parseColorString(const QString& value, bool* ok) const {
  const QString text = value.trimmed();
  QColor parsed(text);
  if (parsed.isValid()) {
    if (ok) {
      *ok = true;
    }
    return parsed;
  }

  const QRegularExpression rgbRegex(
      QStringLiteral("^rgba?\\s*\\(\\s*([0-9]{1,3})\\s*,\\s*([0-9]{1,3})\\s*,\\s*([0-9]{1,3})"
                     "(?:\\s*,\\s*([0-9]+(?:\\.[0-9]+)?%?))?\\s*\\)$"),
      QRegularExpression::CaseInsensitiveOption);
  const QRegularExpressionMatch rgbMatch = rgbRegex.match(text);
  if (rgbMatch.hasMatch()) {
    bool rOk = false;
    bool gOk = false;
    bool bOk = false;
    const int red = std::clamp(rgbMatch.captured(1).toInt(&rOk), 0, 255);
    const int green = std::clamp(rgbMatch.captured(2).toInt(&gOk), 0, 255);
    const int blue = std::clamp(rgbMatch.captured(3).toInt(&bOk), 0, 255);
    if (rOk && gOk && bOk) {
      int alpha = 255;
      if (!rgbMatch.captured(4).trimmed().isEmpty()) {
        bool aOk = false;
        const QString alphaText = rgbMatch.captured(4).trimmed();
        if (alphaText.endsWith(QLatin1Char('%'))) {
          const double percent = alphaText.left(alphaText.size() - 1).toDouble(&aOk);
          alpha = std::clamp(qRound(percent * 255.0 / 100.0), 0, 255);
        } else {
          double alphaRaw = alphaText.toDouble(&aOk);
          if (alphaRaw > 1.0) {
            alphaRaw = alphaRaw / 255.0;
          }
          alpha = std::clamp(qRound(alphaRaw * 255.0), 0, 255);
        }
        if (!aOk) {
          if (ok) {
            *ok = false;
          }
          return QColor();
        }
      }
      const QColor color(red, green, blue, alpha);
      if (ok) {
        *ok = color.isValid();
      }
      return color;
    }
  }

  const QRegularExpression hsbRegex(
      QStringLiteral("^hsb\\s*\\(\\s*([-+]?[0-9]+(?:\\.[0-9]+)?)\\s*,\\s*([0-9]+(?:\\.[0-9]+)?)%"
                     "\\s*,\\s*([0-9]+(?:\\.[0-9]+)?)%"
                     "(?:\\s*,\\s*([0-9]+(?:\\.[0-9]+)?%?))?\\s*\\)$"),
      QRegularExpression::CaseInsensitiveOption);
  const QRegularExpressionMatch match = hsbRegex.match(text);
  if (match.hasMatch()) {
    bool hOk = false;
    bool sOk = false;
    bool bOk = false;

    const double hueRaw = match.captured(1).toDouble(&hOk);
    const double satRaw = match.captured(2).toDouble(&sOk);
    const double briRaw = match.captured(3).toDouble(&bOk);
    if (hOk && sOk && bOk) {
      double alphaRaw = 1.0;
      if (!match.captured(4).trimmed().isEmpty()) {
        bool aOk = false;
        const QString alphaText = match.captured(4).trimmed();
        if (alphaText.endsWith(QLatin1Char('%'))) {
          alphaRaw = alphaText.left(alphaText.size() - 1).toDouble(&aOk) / 100.0;
        } else {
          alphaRaw = alphaText.toDouble(&aOk);
          if (alphaRaw > 1.0) {
            alphaRaw = alphaRaw / 100.0;
          }
        }
        if (!aOk) {
          if (ok) {
            *ok = false;
          }
          return QColor();
        }
      }

      const int hue = ((qRound(hueRaw) % 360) + 360) % 360;
      const int sat = std::clamp(qRound(satRaw * 255.0 / 100.0), 0, 255);
      const int bri = std::clamp(qRound(briRaw * 255.0 / 100.0), 0, 255);
      const int alpha = std::clamp(qRound(alphaRaw * 255.0), 0, 255);
      const QColor color = QColor::fromHsv(hue, sat, bri, alpha);
      if (ok) {
        *ok = color.isValid();
      }
      return color;
    }
  }

  if (ok) {
    *ok = false;
  }
  return QColor();
}

QString AdColorPicker::colorToString(const QColor& color, Format format) const {
  if (!color.isValid()) {
    return QString();
  }

  if (format == Format::Hex) {
    if (color.alpha() >= 255) {
      return colorToHexRgbLower(color);
    }
    return QStringLiteral("rgba(%1, %2, %3, %4)")
        .arg(color.red())
        .arg(color.green())
        .arg(color.blue())
        .arg(formatPercent(color.alphaF()));
  }

  if (format == Format::Rgb) {
    if (color.alpha() >= 255) {
      return QStringLiteral("rgb(%1, %2, %3)").arg(color.red()).arg(color.green()).arg(color.blue());
    }
    return QStringLiteral("rgba(%1, %2, %3, %4)")
        .arg(color.red())
        .arg(color.green())
        .arg(color.blue())
        .arg(formatPercent(color.alphaF()));
  }

  int hue = color.hsvHue();
  if (hue < 0) {
    hue = 0;
  }
  const int sat = qRound(color.saturationF() * 100.0);
  const int bri = qRound(color.valueF() * 100.0);
  if (color.alpha() >= 255) {
    return QStringLiteral("hsb(%1, %2%, %3%)").arg(hue).arg(sat).arg(bri);
  }
  return QStringLiteral("hsb(%1, %2%, %3%, %4)")
      .arg(hue)
      .arg(sat)
      .arg(bri)
      .arg(formatPercent(color.alphaF()));
}

QString AdColorPicker::colorToCss(const QColor& color) const {
  if (!color.isValid()) {
    return QString();
  }
  if (color.alpha() >= 255) {
    return colorToHexRgbLower(color);
  }
  return QStringLiteral("rgba(%1, %2, %3, %4)")
      .arg(color.red())
      .arg(color.green())
      .arg(color.blue())
      .arg(formatPercent(color.alphaF()));
}

QString AdColorPicker::colorValueToCss(const ColorValue& value) const {
  if (value.cleared) {
    return QString();
  }

  if (!value.gradientStops.isEmpty()) {
    QStringList parts;
    parts.reserve(value.gradientStops.size());
    for (const GradientStop& stop : value.gradientStops) {
      parts.append(QStringLiteral("%1 %2%").arg(stop.color).arg(stop.percent));
    }
    return QStringLiteral("linear-gradient(90deg, %1)").arg(parts.join(QStringLiteral(", ")));
  }

  if (!value.solidColor.trimmed().isEmpty()) {
    return value.solidColor.trimmed();
  }

  return colorToCss(solidColor_);
}

AdColorPicker::ColorValue AdColorPicker::exportColorValue() const {
  ColorValue out;
  out.cleared = cleared_;
  if (cleared_) {
    return out;
  }

  if (mode_ == Mode::Gradient && !gradientStops_.isEmpty()) {
    const QVector<InternalGradientStop> normalized = normalizeGradientStops(gradientStops_);
    out.gradientStops.reserve(normalized.size());
    for (const InternalGradientStop& stop : normalized) {
      out.gradientStops.append(GradientStop{colorToCss(stop.color), stop.percent});
    }
    return out;
  }

  out.solidColor = colorToCss(solidColor_);
  return out;
}

void AdColorPicker::importColorValue(const ColorValue& value,
                                     bool fromUser,
                                     bool emitCompleted,
                                     bool emitValueSignal) {
  Q_UNUSED(fromUser)

  if (value.cleared) {
    if (!cleared_) {
      cleared_ = true;
      refreshPanelControlsFromState();
      refreshTriggerDisplay();
      emitChangeSignals(emitCompleted, emitValueSignal);
    }
    return;
  }

  bool updated = false;
  if (!value.gradientStops.isEmpty()) {
    QVector<InternalGradientStop> incoming;
    incoming.reserve(value.gradientStops.size());
    for (const GradientStop& stop : value.gradientStops) {
      bool ok = false;
      const QColor color = parseColorString(stop.color, &ok);
      if (!ok || !color.isValid()) {
        continue;
      }
      incoming.append(InternalGradientStop{color, std::clamp(stop.percent, 0, 100)});
    }

    if (!incoming.isEmpty()) {
      gradientStops_ = normalizeGradientStops(incoming);
      const int maxIndex = std::max(0, static_cast<int>(gradientStops_.size()) - 1);
      activeStopIndex_ = std::clamp(activeStopIndex_, 0, maxIndex);
      solidColor_ = gradientStops_.at(activeStopIndex_).color;
      cleared_ = false;
      if (modeListContains(modeOptions_, Mode::Gradient) && mode_ != Mode::Gradient) {
        mode_ = Mode::Gradient;
        emit modeChanged(mode_);
      }
      updated = true;
    }
  }

  if (!updated && !value.solidColor.trimmed().isEmpty()) {
    bool ok = false;
    const QColor color = parseColorString(value.solidColor, &ok);
    if (ok && color.isValid()) {
      solidColor_ = color;
      cleared_ = false;
      updated = true;
    }
  }

  if (!updated) {
    return;
  }

  refreshPanelControlsFromState();
  refreshTriggerDisplay();
  emitChangeSignals(emitCompleted, emitValueSignal);
}

QVector<AdColorPicker::InternalGradientStop> AdColorPicker::normalizeGradientStops(
    const QVector<InternalGradientStop>& stops) const {
  QMap<int, QColor> ordered;
  for (const InternalGradientStop& stop : stops) {
    if (!stop.color.isValid()) {
      continue;
    }
    ordered.insert(std::clamp(stop.percent, 0, 100), stop.color);
  }

  QVector<InternalGradientStop> normalized;
  normalized.reserve(ordered.size() + 2);
  for (auto it = ordered.cbegin(); it != ordered.cend(); ++it) {
    normalized.append(InternalGradientStop{it.value(), it.key()});
  }

  if (normalized.isEmpty()) {
    normalized = {
        InternalGradientStop{solidColor_.isValid() ? solidColor_ : QColor("#1677ff"), 0},
        InternalGradientStop{solidColor_.isValid() ? solidColor_ : QColor("#1677ff"), 100},
    };
    return normalized;
  }

  if (normalized.constFirst().percent != 0) {
    normalized.prepend(InternalGradientStop{normalized.constFirst().color, 0});
  }
  if (normalized.constLast().percent != 100) {
    normalized.append(InternalGradientStop{normalized.constLast().color, 100});
  }

  if (normalized.size() == 1) {
    normalized.append(InternalGradientStop{normalized.constFirst().color, 100});
  }

  return normalized;
}

void AdColorPicker::emitChangeSignals(bool emitCompleted, bool emitValueSignal) {
  const ColorValue exported = exportColorValue();
  const QString css = colorValueToCss(exported);

  emit colorValueChanged(exported);
  if (emitValueSignal) {
    emit valueChanged(css);
  }
  emit changed(exported, css);
  if (emitCompleted) {
    emit changeCompleted(exported);
  }
}

void AdColorPicker::applyPreset(const ColorValue& value) {
  importColorValue(value, true, true, true);
}

}  // namespace adqt::widgets
