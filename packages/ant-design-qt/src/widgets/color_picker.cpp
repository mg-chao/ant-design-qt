#include "color_picker.h"

#include "color_picker_style.h"
#include "interaction_overlay_manager.h"
#include "input.h"
#include "input_number.h"
#include "select.h"
#include "slider.h"
#include "theme/theme_manager.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QBrush>
#include <QEvent>
#include <QFrame>
#include <QFontMetrics>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLayout>
#include <QMap>
#include <QMetaType>
#include <QMoveEvent>
#include <QMouseEvent>
#include <QPalette>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QResizeEvent>
#include <QScopedValueRollback>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include <algorithm>
#include <limits>
#include <utility>

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

}  // namespace

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

class ColorPickerTriggerFrame final : public QFrame {
 public:
  explicit ColorPickerTriggerFrame(QWidget* parent = nullptr) : QFrame(parent) {
    setAttribute(Qt::WA_Hover, true);
    setMouseTracking(true);
  }

  void setVisualStyle(const QColor& background, const QColor& border, qreal borderWidth, qreal radius) {
    const qreal normalizedWidth = std::max<qreal>(0.0, borderWidth);
    const qreal normalizedRadius = std::max<qreal>(0.0, radius);
    const bool changed = background_ != background || border_ != border ||
                         !qFuzzyCompare(borderWidth_ + 1.0, normalizedWidth + 1.0) ||
                         !qFuzzyCompare(radius_ + 1.0, normalizedRadius + 1.0);
    background_ = background;
    border_ = border;
    borderWidth_ = normalizedWidth;
    radius_ = normalizedRadius;
    if (changed) {
      update();
    }
  }

  QColor borderColor() const { return border_; }
  qreal borderWidth() const { return borderWidth_; }
  qreal radius() const { return radius_; }

 protected:
  void paintEvent(QPaintEvent* event) override {
    Q_UNUSED(event)

    const QRectF fillRect(rect());
    const qreal borderWidth = std::max<qreal>(0.0, borderWidth_);
    const bool hasVisibleBorder = borderWidth > 0.0 && border_.alpha() > 0;
    const qreal half = borderWidth / 2.0;
    const QRectF rawBorderRect =
        fillRect.adjusted(half + 0.5, half + 0.5, -half - 0.5, -half - 0.5);
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
    const QRectF borderRect = hasVisibleBorder ? snapRectToDevicePixels(rawBorderRect, dpr) : rawBorderRect;

    const qreal topLeft = std::max<qreal>(0.0, radius_);
    const qreal topRight = std::max<qreal>(0.0, radius_);
    const qreal bottomRight = std::max<qreal>(0.0, radius_);
    const qreal bottomLeft = std::max<qreal>(0.0, radius_);

    const QRectF shapeRect = hasVisibleBorder ? borderRect : fillRect;
    const QPainterPath fillPath = roundedRectPath(shapeRect, topLeft, topRight, bottomRight, bottomLeft);
    painter.fillPath(fillPath, background_);

    if (hasVisibleBorder) {
      QPen borderPen(border_, borderWidth, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
      painter.setPen(borderPen);
      painter.setBrush(Qt::NoBrush);
      painter.drawPath(roundedRectPath(borderRect, topLeft, topRight, bottomRight, bottomLeft));
    }
  }

 private:
  QColor background_ = QColor(255, 255, 255);
  QColor border_ = QColor(217, 217, 217);
  qreal borderWidth_ = 1.0;
  qreal radius_ = 6.0;
};

constexpr char kPresetSwatchObjectName[] = "ad-color-picker-preset-swatch";
constexpr char kFormatSelectObjectName[] = "ad-color-picker-format-select";
constexpr int kTransparencyCell = 6;
constexpr int kFormatSelectPopupWidth = 68;

// Default values - will be overridden by style metrics when available
int g_saturationPanelHeight = 160;
int g_sliderHeight = 8;
int g_sliderControlSize = 8;
int g_sliderHandleSize = 8;
int g_sliderHandleSizeHover = 8;
int g_sliderHandleLineWidth = 2;
int g_sliderHandleLineWidthHover = 2;
int g_sliderMarginMain = 6;
int g_sliderMarginCross = 2;
int g_sliderWidgetHeight = 12;
int g_gradientHandleSize = 6;
int g_gradientHandleSizeHover = 8;
int g_marginSM = 12;
int g_previewSwatchSize = 28;
int g_alphaInputWidth = 44;

int formatSelectIconSizeFromMap(const adqt::theme::ThemeMapToken& map) {
  return std::max(10, qRound(map.fontSizeSM));
}

int sliderGroupGapFromMetrics(int marginSM, int sliderMarginCross) {
  // AntD slider root height is rail-height-based and handle ring can overflow.
  // Qt sliders include cross padding in widget height, so subtract it here.
  return std::max(0, marginSM - sliderMarginCross * 2);
}

int sliderSectionGapFromMetrics(int marginSM, int sliderMarginCross) {
  // Keep perceived block spacing consistent with AntD's overflow visuals.
  return std::max(0, marginSM - sliderMarginCross);
}

int sliderContainerHeightFromMetrics(int previewSwatchSize,
                                     int sliderVisualHeight,
                                     int sliderGroupGap,
                                     bool disabledAlpha) {
  const int visibleSliderCount = disabledAlpha ? 1 : 2;
  const int groupHeight = sliderVisualHeight * visibleSliderCount +
                          ((visibleSliderCount > 1) ? sliderGroupGap : 0);
  return std::max(previewSwatchSize, groupHeight);
}

void updateInternalMetrics(const detail::ColorPickerMetrics& metrics) {
  g_saturationPanelHeight = metrics.saturationPanelHeight;
  g_sliderHeight = metrics.sliderHeight;
  g_sliderControlSize = metrics.sliderControlSize;
  g_sliderHandleSize = metrics.sliderHandleSize;
  g_sliderHandleSizeHover = metrics.sliderHandleSizeHover;
  g_sliderHandleLineWidth = metrics.sliderHandleLineWidth;
  g_sliderHandleLineWidthHover = metrics.sliderHandleLineWidthHover;
  g_sliderMarginMain = metrics.sliderMarginMain;
  g_sliderMarginCross = metrics.sliderMarginCross;
  g_sliderWidgetHeight = metrics.sliderVisualHeight;
  g_gradientHandleSize = metrics.gradientHandleSize;
  g_gradientHandleSizeHover = metrics.gradientHandleSizeHover;
  g_marginSM = metrics.marginSM;
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

QString colorToHexRgbaLower(const QColor& color) {
  return QStringLiteral("#%1%2%3%4")
      .arg(color.red(), 2, 16, QChar('0'))
      .arg(color.green(), 2, 16, QChar('0'))
      .arg(color.blue(), 2, 16, QChar('0'))
      .arg(color.alpha(), 2, 16, QChar('0'))
      .toLower();
}

bool parseCssHexColor(const QString& input, QColor* out) {
  if (!out) {
    return false;
  }

  const QString trimmed = input.trimmed();
  if (!trimmed.startsWith(QLatin1Char('#'))) {
    return false;
  }

  const QString hex = trimmed.mid(1);
  const int length = hex.size();
  if (length != 3 && length != 4 && length != 6 && length != 8) {
    return false;
  }

  static const QRegularExpression kHexPattern(QStringLiteral("^[0-9a-fA-F]+$"));
  if (!kHexPattern.match(hex).hasMatch()) {
    return false;
  }

  bool ok = false;
  int red = 0;
  int green = 0;
  int blue = 0;
  int alpha = 255;

  if (length == 3 || length == 4) {
    red = QString(hex.at(0)).repeated(2).toInt(&ok, 16);
    if (!ok) {
      return false;
    }
    green = QString(hex.at(1)).repeated(2).toInt(&ok, 16);
    if (!ok) {
      return false;
    }
    blue = QString(hex.at(2)).repeated(2).toInt(&ok, 16);
    if (!ok) {
      return false;
    }
    if (length == 4) {
      alpha = QString(hex.at(3)).repeated(2).toInt(&ok, 16);
      if (!ok) {
        return false;
      }
    }
  } else {
    red = hex.mid(0, 2).toInt(&ok, 16);
    if (!ok) {
      return false;
    }
    green = hex.mid(2, 2).toInt(&ok, 16);
    if (!ok) {
      return false;
    }
    blue = hex.mid(4, 2).toInt(&ok, 16);
    if (!ok) {
      return false;
    }
    if (length == 8) {
      alpha = hex.mid(6, 2).toInt(&ok, 16);
      if (!ok) {
        return false;
      }
    }
  }

  const QColor parsed(red, green, blue, alpha);
  if (!parsed.isValid()) {
    return false;
  }
  *out = parsed;
  return true;
}

QString colorToRgbCssCompact(const QColor& color) {
  if (!color.isValid()) {
    return QString();
  }
  if (color.alpha() >= 255) {
    return QStringLiteral("rgb(%1,%2,%3)").arg(color.red()).arg(color.green()).arg(color.blue());
  }
  return QStringLiteral("rgba(%1,%2,%3,%4)")
      .arg(color.red())
      .arg(color.green())
      .arg(color.blue())
      .arg(formatPercent(color.alphaF()));
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

class ColorPickerSwatch final : public QWidget {
 public:
  explicit ColorPickerSwatch(QWidget* parent = nullptr) : QWidget(parent) {
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  }

  void setFrameStyle(const QColor& border, qreal borderWidth, qreal radius, bool dashedBorder = false) {
    const qreal normalizedWidth = std::max<qreal>(0.0, borderWidth);
    const qreal normalizedRadius = std::max<qreal>(0.0, radius);
    const bool changed = border_ != border || dashedBorder_ != dashedBorder ||
                         !qFuzzyCompare(borderWidth_ + 1.0, normalizedWidth + 1.0) ||
                         !qFuzzyCompare(radius_ + 1.0, normalizedRadius + 1.0);
    border_ = border;
    borderWidth_ = normalizedWidth;
    radius_ = normalizedRadius;
    dashedBorder_ = dashedBorder;
    if (changed) {
      update();
    }
  }

  void setCheckerColors(const QColor& light, const QColor& dark, int cellSize) {
    const int normalizedCell = std::max(2, cellSize);
    const bool changed = checkerLight_ != light || checkerDark_ != dark || checkerCellSize_ != normalizedCell;
    checkerLight_ = light;
    checkerDark_ = dark;
    checkerCellSize_ = normalizedCell;
    if (changed) {
      update();
    }
  }

  void setSolidFill(const QColor& color) {
    const bool changed = fillMode_ != FillMode::Solid || solidFill_ != color;
    fillMode_ = FillMode::Solid;
    solidFill_ = color;
    if (changed) {
      update();
    }
  }

  void setGradientFill(const QVector<QPair<qreal, QColor>>& stops) {
    const bool changed = fillMode_ != FillMode::Gradient || gradientStops_ != stops;
    fillMode_ = FillMode::Gradient;
    gradientStops_ = stops;
    if (changed) {
      update();
    }
  }

  void setClearedTriggerFill() {
    if (fillMode_ == FillMode::ClearTrigger) {
      return;
    }
    fillMode_ = FillMode::ClearTrigger;
    update();
  }

  void setClearedPreviewFill() {
    if (fillMode_ == FillMode::ClearPreview) {
      return;
    }
    fillMode_ = FillMode::ClearPreview;
    update();
  }

 protected:
  void paintEvent(QPaintEvent* event) override {
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF fillRect(rect());
    if (!fillRect.isValid() || fillRect.width() <= 0.0 || fillRect.height() <= 0.0) {
      return;
    }

    const qreal borderWidth = std::max<qreal>(0.0, borderWidth_);
    const bool hasVisibleBorder = borderWidth > 0.0 && border_.alpha() > 0;
    const qreal half = borderWidth / 2.0;
    const QRectF rawBorderRect = fillRect.adjusted(half + 0.5, half + 0.5, -half - 0.5, -half - 0.5);
    if (hasVisibleBorder &&
        (!rawBorderRect.isValid() || rawBorderRect.width() <= 0.0 || rawBorderRect.height() <= 0.0)) {
      return;
    }

    const qreal dpr = painter.device() ? painter.device()->devicePixelRatioF() : devicePixelRatioF();
    const QRectF borderRect = hasVisibleBorder ? snapRectToDevicePixels(rawBorderRect, dpr) : rawBorderRect;
    const QRectF shapeRect = hasVisibleBorder ? borderRect : fillRect;
    const qreal radius = std::max<qreal>(0.0, radius_);
    const QPainterPath fillPath = roundedRectPath(shapeRect, radius, radius, radius, radius);

    switch (fillMode_) {
      case FillMode::Solid: {
        painter.fillPath(fillPath, makeCheckerBrush(checkerCellSize_, checkerLight_, checkerDark_));
        painter.fillPath(fillPath, solidFill_);
        break;
      }
      case FillMode::Gradient: {
        painter.fillPath(fillPath, makeCheckerBrush(checkerCellSize_, checkerLight_, checkerDark_));
        if (!gradientStops_.isEmpty()) {
          QLinearGradient gradient(0.0, 0.0, 1.0, 0.0);
          gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
          for (const auto& stop : gradientStops_) {
            gradient.setColorAt(std::clamp(static_cast<double>(stop.first), 0.0, 1.0), stop.second);
          }
          painter.fillPath(fillPath, QBrush(gradient));
        }
        break;
      }
      case FillMode::ClearTrigger: {
        painter.fillPath(fillPath, QColor("#ffffff"));
        painter.save();
        painter.setClipPath(fillPath);
        const qreal inset = std::max<qreal>(1.0, std::round(std::min(shapeRect.width(), shapeRect.height()) * 0.12));
        const qreal slashWidth =
            std::max<qreal>(2.0, std::round(std::min(shapeRect.width(), shapeRect.height()) * 0.14));
        QPen slashPen(QColor("#ff4d4f"), slashWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(slashPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawLine(QPointF(shapeRect.left() + inset, shapeRect.top() + inset),
                         QPointF(shapeRect.right() - inset, shapeRect.bottom() - inset));
        painter.restore();
        break;
      }
      case FillMode::ClearPreview:
      default:
        break;
    }

    if (hasVisibleBorder) {
      QPen borderPen(border_, borderWidth,
                     dashedBorder_ ? Qt::DashLine : Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
      if (dashedBorder_) {
        borderPen.setDashPattern({2.0, 2.0});
      }
      painter.setPen(borderPen);
      painter.setBrush(Qt::NoBrush);
      painter.drawPath(roundedRectPath(borderRect, radius, radius, radius, radius));
    }
  }

 private:
  enum class FillMode {
    Solid,
    Gradient,
    ClearTrigger,
    ClearPreview,
  };

  QColor border_ = QColor("#f0f0f0");
  qreal borderWidth_ = 1.0;
  qreal radius_ = 4.0;
  bool dashedBorder_ = false;

  QColor checkerLight_ = QColor("#ffffff");
  QColor checkerDark_ = QColor("#f0f0f0");
  int checkerCellSize_ = 6;

  FillMode fillMode_ = FillMode::Solid;
  QColor solidFill_ = QColor("#1677ff");
  QVector<QPair<qreal, QColor>> gradientStops_;
};

class ColorPickerClearButton final : public QAbstractButton {
 public:
  explicit ColorPickerClearButton(QWidget* parent = nullptr) : QAbstractButton(parent) {
    setObjectName(QStringLiteral("ad-color-picker-clear"));
    setCursor(Qt::ArrowCursor);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_Hover, true);
  }

  void setVisualStyle(const QColor& background,
                      const QColor& border,
                      const QColor& borderHover,
                      const QColor& slash,
                      qreal borderWidth,
                      int radius) {
    const qreal normalizedBorder = std::max<qreal>(1.0, borderWidth);
    const int normalizedRadius = std::max(0, radius);
    const bool changed = background_ != background || border_ != border ||
                         borderHover_ != borderHover || slash_ != slash ||
                         !qFuzzyCompare(borderWidth_ + 1.0, normalizedBorder + 1.0) ||
                         radius_ != normalizedRadius;
    background_ = background;
    border_ = border;
    borderHover_ = borderHover;
    slash_ = slash;
    borderWidth_ = normalizedBorder;
    radius_ = normalizedRadius;
    if (changed) {
      update();
    }
  }

 protected:
  void enterEvent(QEnterEvent* event) override {
    hovered_ = true;
    update();
    QAbstractButton::enterEvent(event);
  }

  void leaveEvent(QEvent* event) override {
    hovered_ = false;
    update();
    QAbstractButton::leaveEvent(event);
  }

  void paintEvent(QPaintEvent* event) override {
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF fillRect(rect());
    if (!fillRect.isValid() || fillRect.width() <= 0.0 || fillRect.height() <= 0.0) {
      return;
    }

    const qreal borderWidth = std::max<qreal>(1.0, borderWidth_);
    const qreal half = borderWidth / 2.0;
    const QRectF rawBorderRect = fillRect.adjusted(half + 0.5, half + 0.5, -half - 0.5, -half - 0.5);
    if (!rawBorderRect.isValid() || rawBorderRect.width() <= 0.0 || rawBorderRect.height() <= 0.0) {
      return;
    }

    const qreal dpr = painter.device() ? painter.device()->devicePixelRatioF() : devicePixelRatioF();
    const QRectF borderRect = snapRectToDevicePixels(rawBorderRect, dpr);
    const qreal radius = std::max<qreal>(0.0, radius_);
    const QPainterPath fillPath = roundedRectPath(borderRect, radius, radius, radius, radius);

    QColor borderColor = hovered_ && isEnabled() ? borderHover_ : border_;
    QColor slashColor = slash_;
    if (!isEnabled()) {
      borderColor.setAlphaF(borderColor.alphaF() * 0.8);
      slashColor.setAlphaF(slashColor.alphaF() * 0.45);
    }

    painter.fillPath(fillPath, background_);
    painter.setPen(QPen(borderColor, borderWidth, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(roundedRectPath(borderRect, radius, radius, radius, radius));

    painter.save();
    painter.setClipPath(fillPath);
    const qreal inset = std::max<qreal>(1.0, std::round(std::min(borderRect.width(), borderRect.height()) * 0.2));
    const qreal slashWidth =
        std::max<qreal>(2.0, std::round(std::min(borderRect.width(), borderRect.height()) * 0.12));
    painter.setPen(QPen(slashColor, slashWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawLine(QPointF(borderRect.right() - inset, borderRect.top() + inset),
                     QPointF(borderRect.left() + inset, borderRect.bottom() - inset));
    painter.restore();
  }

 private:
  QColor background_ = QColor("#ffffff");
  QColor border_ = QColor("#f0f0f0");
  QColor borderHover_ = QColor("#d9d9d9");
  QColor slash_ = QColor("#ff4d4f");
  qreal borderWidth_ = 1.0;
  int radius_ = 4;
  bool hovered_ = false;
};

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

int triggerRadiusForSize(AdColorPicker::Size size,
                         const detail::ColorPickerVisualStyle& style) {
  switch (size) {
    case AdColorPicker::Size::Small:
      return style.metrics.triggerRadiusSM;
    case AdColorPicker::Size::Large:
      return style.metrics.triggerRadiusLG;
    case AdColorPicker::Size::Middle:
    default:
      return style.metrics.triggerRadius;
  }
}

int swatchRadiusForSize(AdColorPicker::Size size,
                        const detail::ColorPickerVisualStyle& style) {
  switch (size) {
    case AdColorPicker::Size::Small:
      return style.metrics.swatchRadiusSM;
    case AdColorPicker::Size::Large:
      return style.metrics.swatchRadiusLG;
    case AdColorPicker::Size::Middle:
    default:
      return style.metrics.swatchRadius;
  }
}

bool modeListContains(const QVector<AdColorPicker::Mode>& modes, AdColorPicker::Mode value) {
  return std::find(modes.cbegin(), modes.cend(), value) != modes.cend();
}

int formatSelectWidthHint(const QFont& font, int iconSize, int arrowGap) {
  const QFontMetrics metrics(font);
  int widestLabel = 0;
  for (const QString& label :
       {QStringLiteral("HEX"), QStringLiteral("RGB"), QStringLiteral("HSB")}) {
    widestLabel =
        std::max(widestLabel,
                 std::max(metrics.horizontalAdvance(label), metrics.boundingRect(label).width()));
  }

  constexpr int kWidthSafety = 2;
  return std::max(40, widestLabel + std::max(10, iconSize) + std::max(0, arrowGap) + kWidthSafety);
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

QString sanitizeHexInput(QString text) {
  text = text.trimmed().toUpper();
  if (text.startsWith(QLatin1Char('#'))) {
    text.remove(0, 1);
  }

  QString filtered;
  filtered.reserve(text.size());
  for (const QChar ch : text) {
    if (ch.isDigit() || (ch >= QLatin1Char('A') && ch <= QLatin1Char('F'))) {
      filtered.append(ch);
    }
  }

  if (filtered.size() > 8) {
    filtered.truncate(8);
  }
  return filtered;
}

bool parseBoundedInt(const QString& text, int minValue, int maxValue, int* out) {
  if (!out) {
    return false;
  }

  bool ok = false;
  const int parsed = text.trimmed().toInt(&ok);
  if (!ok) {
    return false;
  }
  if (parsed < minValue || parsed > maxValue) {
    return false;
  }
  *out = parsed;
  return true;
}

bool parseBoundedInt(const QVariant& value, int minValue, int maxValue, int* out) {
  if (!out) {
    return false;
  }

  bool ok = false;
  const int parsed = value.toInt(&ok);
  if (!ok) {
    const QString text = value.toString();
    if (text.trimmed().isEmpty()) {
      return false;
    }
    return parseBoundedInt(text, minValue, maxValue, out);
  }
  if (parsed < minValue || parsed > maxValue) {
    return false;
  }
  *out = parsed;
  return true;
}

QString modeLabel(AdColorPicker::Mode value) {
  switch (value) {
    case AdColorPicker::Mode::Single:
      return QStringLiteral("Single");
    case AdColorPicker::Mode::Gradient:
      return QStringLiteral("Gradient");
  }
  return QStringLiteral("Single");
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

AdColorPicker::~AdColorPicker() {
  stopInteractionWaveForOwner(this);
  stopInteractionFocusForOwner(this);
}

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

  updateModeSegmentedOptions();
  refreshPanelControlsFromState();
}

AdColorPicker::Format AdColorPicker::format() const { return format_; }

void AdColorPicker::setFormat(Format value) {
  if (format_ == value) {
    return;
  }

  format_ = value;
  emit formatChanged(format_);
  updateFormatInputVisibility();
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
    if (disabled()) {
      stopInteractionWaveForOwner(this);
      stopInteractionFocusForOwner(this);
    }
    if (popover_) {
      popover_->setDisabled(disabled());
    }
    emit disabledChanged(disabled());
    refreshStyle();
  } else if (event->type() == QEvent::Hide) {
    triggerHovered_ = false;
    stopInteractionFocusForOwner(this);
  } else if (event->type() == QEvent::FontChange || event->type() == QEvent::PaletteChange) {
    refreshStyle();
  } else if (event->type() == QEvent::Show) {
    updateTriggerFocusOverlay();
  }
}

void AdColorPicker::moveEvent(QMoveEvent* event) {
  QWidget::moveEvent(event);
  updateTriggerFocusOverlay();
}

void AdColorPicker::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  updateTriggerFocusOverlay();
}

bool AdColorPicker::eventFilter(QObject* watched, QEvent* event) {
  const bool watchedDefaultTrigger =
      watched == triggerFrame_ || watched == triggerSwatch_ || watched == triggerTextLabel_;
  if (watchedDefaultTrigger && event) {
    const QEvent::Type type = event->type();
    if (watched == triggerFrame_) {
      if (type == QEvent::Enter || type == QEvent::HoverEnter) {
        if (!triggerHovered_) {
          triggerHovered_ = true;
          refreshStyle();
        }
      } else if (type == QEvent::Leave || type == QEvent::HoverLeave) {
        if (triggerHovered_) {
          triggerHovered_ = false;
          refreshStyle();
        }
      }
    }
  }
  return QWidget::eventFilter(watched, event);
}

void AdColorPicker::updateTriggerFocusOverlay() {
  if (disabled() || customTrigger_ || !triggerFrame_ || !open() || !isVisible()) {
    stopInteractionFocusForOwner(this);
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
  // Preview swatch visuals do not depend on trigger text mode. Keep this path
  // independent from showText to avoid extra style work during drag updates.
  input.showText = false;
  input.cleared = cleared_;
  input.baseFont = font();
  input.componentTokens = componentTokens_;
  input.semanticStyles = mergedSemantic;
  const detail::ColorPickerVisualStyle style = detail::resolveColorPickerVisualStyle(input);
  if (style.triggerFocusOutline.alpha() <= 0 || style.metrics.focusOutlineWidth <= 0.0) {
    stopInteractionFocusForOwner(this);
    return;
  }

  const qreal borderWidth = std::max<qreal>(0.0, style.metrics.borderWidth);
  const qreal half = borderWidth / 2.0;
  QRectF baseRect = QRectF(triggerFrame_->rect()).adjusted(half + 0.5, half + 0.5, -half - 0.5, -half - 0.5);
  if (!baseRect.isValid() || baseRect.width() <= 0.0 || baseRect.height() <= 0.0) {
    baseRect = QRectF(triggerFrame_->rect());
  }

  QWidget* hostWindow = window();
  if (!hostWindow) {
    stopInteractionFocusForOwner(this);
    return;
  }
  const QPoint origin = triggerFrame_->mapTo(hostWindow, QPoint(0, 0));
  const QRectF baseRectInWindow = baseRect.translated(origin.x(), origin.y());

  const qreal radius = std::max<qreal>(0.0, triggerRadiusForSize(size_, style));
  InteractionFocusRequest request;
  request.owner = this;
  request.baseRectInWindow = baseRectInWindow;
  request.topLeft = radius;
  request.topRight = radius;
  request.bottomRight = radius;
  request.bottomLeft = radius;
  request.color = style.triggerFocusOutline;
  request.strokeWidth = std::max<qreal>(1.0, style.metrics.focusOutlineWidth);
  request.offset = std::max<qreal>(0.0, style.metrics.focusOutlineOffset);
  triggerInteractionFocus(request);
}

QString AdColorPicker::modeName(Mode value) {
  switch (value) {
    case Mode::Single:
      return QStringLiteral("single");
    case Mode::Gradient:
      return QStringLiteral("gradient");
  }
  return QStringLiteral("single");
}

AdColorPicker::Mode AdColorPicker::parseModeName(const QString& value, Mode fallback) {
  const QString normalized = value.trimmed().toLower();
  if (normalized == QStringLiteral("single")) {
    return Mode::Single;
  }
  if (normalized == QStringLiteral("gradient")) {
    return Mode::Gradient;
  }
  return fallback;
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

  triggerFrame_ = new ColorPickerTriggerFrame(popover_);
  triggerFrame_->setObjectName(QStringLiteral("ad-color-picker-trigger-frame"));
  triggerFrame_->setAttribute(Qt::WA_Hover, true);
  triggerFrame_->setMouseTracking(true);
  triggerFrame_->setCursor(Qt::PointingHandCursor);
  // Match Ant Design trigger behavior: intrinsic-width inline trigger that does not
  // stretch with parent layout width.
  triggerFrame_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
  triggerFrame_->installEventFilter(this);
  auto* triggerLayout = new QHBoxLayout(triggerFrame_);
  triggerLayout->setContentsMargins(8, 4, 8, 4);
  triggerLayout->setSpacing(8);
  triggerLayout->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

  triggerSwatch_ = new ColorPickerSwatch(triggerFrame_);
  triggerSwatch_->setObjectName(QStringLiteral("ad-color-picker-trigger-swatch"));
  triggerSwatch_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  triggerSwatch_->installEventFilter(this);

  triggerTextLabel_ = new QLabel(triggerFrame_);
  triggerTextLabel_->setObjectName(QStringLiteral("ad-color-picker-trigger-text"));
  triggerTextLabel_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  triggerTextLabel_->setWordWrap(false);
  triggerTextLabel_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
  triggerTextLabel_->installEventFilter(this);

  triggerLayout->addWidget(triggerSwatch_);
  triggerLayout->addWidget(triggerTextLabel_);
  triggerLayout->setAlignment(triggerSwatch_, Qt::AlignVCenter);
  triggerLayout->setAlignment(triggerTextLabel_, Qt::AlignVCenter);

  defaultTrigger_ = triggerFrame_;
  popover_->setTriggerWidget(defaultTrigger_);

  panelHost_ = new QWidget(popover_);
  panelHost_->setObjectName(QStringLiteral("ad-color-picker-panel-host"));
  panelHost_->setAttribute(Qt::WA_StyledBackground, true);
  popover_->setContentWidget(panelHost_);

  connect(popover_, &AdPopover::openChanged, this, [this](bool openValue) {
    if (!openValue) {
      resumeTriggerUpdatesAfterInteraction();
    }
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
  pickerLayout->setSpacing(0);

  operationRow_ = new QWidget(pickerPanel_);
  operationRow_->setObjectName(QStringLiteral("ad-color-picker-operation-row"));
  auto* operationRow = new QHBoxLayout(operationRow_);
  operationRow->setContentsMargins(0, 0, 0, 0);
  operationRow->setSpacing(8);
  modeSegmented_ = new QWidget(operationRow_);
  modeSegmented_->setObjectName(QStringLiteral("ad-color-picker-mode-segmented"));
  modeSegmented_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
  auto* modeLayout = new QHBoxLayout(modeSegmented_);
  modeLayout->setContentsMargins(2, 2, 2, 2);
  modeLayout->setSpacing(2);
  modeButtonGroup_ = new QButtonGroup(modeSegmented_);
  modeButtonGroup_->setExclusive(true);
  clearButton_ = new ColorPickerClearButton(operationRow_);
  operationRow->addWidget(modeSegmented_, 0);
  operationRow->addStretch(1);
  operationRow->addWidget(clearButton_, 0, Qt::AlignRight);
  pickerLayout->addWidget(operationRow_);
  operationGap_ = new QWidget(pickerPanel_);
  operationGap_->setObjectName(QStringLiteral("ad-color-picker-operation-gap"));
  operationGap_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  operationGap_->setFixedHeight(8);
  pickerLayout->addWidget(operationGap_);

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
  gradientTokens.controlSize = g_sliderControlSize;
  gradientTokens.railSize = g_sliderHeight;
  gradientTokens.handleSize = g_gradientHandleSize;
  gradientTokens.handleSizeHover = g_gradientHandleSizeHover;
  gradientTokens.handleLineWidth = g_sliderHandleLineWidth;
  gradientTokens.handleLineWidthHover = g_sliderHandleLineWidthHover;
  gradientTokens.marginMain = g_sliderMarginMain;
  gradientTokens.marginCross = g_sliderMarginCross;
  gradientTokens.focusOutlineSize = 0;
  gradientTokens.markGap = 0;
  gradientSlider_->setComponentTokens(gradientTokens);
  gradientSlider_->setMinimumHeight(g_sliderWidgetHeight);
  gradientSlider_->setMaximumHeight(g_sliderWidgetHeight);
  gradientSlider_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

  gradientLayout->addWidget(gradientSlider_);
  pickerLayout->addWidget(gradientSection_);
  gradientGap_ = new QWidget(pickerPanel_);
  gradientGap_->setObjectName(QStringLiteral("ad-color-picker-gradient-gap"));
  gradientGap_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  gradientGap_->setFixedHeight(8);
  pickerLayout->addWidget(gradientGap_);

  saturationPanel_ = new ColorSaturationPanel(pickerPanel_);
  saturationPanel_->setObjectName(QStringLiteral("ad-color-picker-saturation-panel"));
  saturationPanel_->setMinimumHeight(g_saturationPanelHeight);
  saturationPanel_->setMaximumHeight(g_saturationPanelHeight);
  pickerLayout->addWidget(saturationPanel_);
  saturationGap_ = new QWidget(pickerPanel_);
  saturationGap_->setObjectName(QStringLiteral("ad-color-picker-saturation-gap"));
  saturationGap_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  saturationGap_->setFixedHeight(sliderSectionGapFromMetrics(g_marginSM, g_sliderMarginCross));
  pickerLayout->addWidget(saturationGap_);

  sliderContainer_ = new QWidget(pickerPanel_);
  sliderContainer_->setObjectName(QStringLiteral("ad-color-picker-slider-container"));
  auto* sliderContainerLayout = new QHBoxLayout(sliderContainer_);
  sliderContainerLayout->setContentsMargins(0, 0, 0, 0);
  sliderContainerLayout->setSpacing(g_marginSM);

  sliderGroup_ = new QWidget(sliderContainer_);
  auto* sliderGroupLayout = new QVBoxLayout(sliderGroup_);
  sliderGroupLayout->setContentsMargins(0, 0, 0, 0);
  sliderGroupLayout->setSpacing(sliderGroupGapFromMetrics(g_marginSM, g_sliderMarginCross));

  hueSlider_ = new AdSlider(sliderGroup_);
  hueSlider_->setMinimum(0);
  hueSlider_->setMaximum(359);
  hueSlider_->setStep(1);
  hueSlider_->setIncluded(false);
  hueSlider_->setTooltipEnabled(false);
  AdSlider::ComponentTokens hueTokens;
  hueTokens.controlSize = g_sliderControlSize;
  hueTokens.railSize = g_sliderHeight;
  hueTokens.handleSize = g_sliderHandleSize;
  hueTokens.handleSizeHover = g_sliderHandleSizeHover;
  hueTokens.handleLineWidth = g_sliderHandleLineWidth;
  hueTokens.handleLineWidthHover = g_sliderHandleLineWidthHover;
  hueTokens.marginMain = g_sliderMarginMain;
  hueTokens.marginCross = g_sliderMarginCross;
  hueTokens.focusOutlineSize = 0;
  hueTokens.markGap = 0;
  hueSlider_->setComponentTokens(hueTokens);
  hueSlider_->setMinimumHeight(g_sliderWidgetHeight);
  hueSlider_->setMaximumHeight(g_sliderWidgetHeight);
  hueSlider_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
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
  alphaTokens.controlSize = g_sliderControlSize;
  alphaTokens.railSize = g_sliderHeight;
  alphaTokens.handleSize = g_sliderHandleSize;
  alphaTokens.handleSizeHover = g_sliderHandleSizeHover;
  alphaTokens.handleLineWidth = g_sliderHandleLineWidth;
  alphaTokens.handleLineWidthHover = g_sliderHandleLineWidthHover;
  alphaTokens.marginMain = g_sliderMarginMain;
  alphaTokens.marginCross = g_sliderMarginCross;
  alphaTokens.focusOutlineSize = 0;
  alphaTokens.markGap = 0;
  alphaSlider_->setComponentTokens(alphaTokens);
  alphaSlider_->setMinimumHeight(g_sliderWidgetHeight);
  alphaSlider_->setMaximumHeight(g_sliderWidgetHeight);
  alphaSlider_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  alphaRow->addWidget(alphaSlider_);
  sliderGroupLayout->addWidget(alphaSection_);

  sliderContainerLayout->addWidget(sliderGroup_, 1);

  previewSwatch_ = new ColorPickerSwatch(sliderContainer_);
  previewSwatch_->setObjectName(QStringLiteral("ad-color-picker-preview-swatch"));
  previewSwatch_->setFixedSize(g_previewSwatchSize, g_previewSwatchSize);
  sliderContainerLayout->addWidget(previewSwatch_, 0, Qt::AlignCenter);
  sliderContainer_->setFixedHeight(sliderContainerHeightFromMetrics(
      g_previewSwatchSize, g_sliderWidgetHeight,
      sliderGroupGapFromMetrics(g_marginSM, g_sliderMarginCross), disabledAlpha_));
  pickerLayout->addWidget(sliderContainer_);
  sliderGap_ = new QWidget(pickerPanel_);
  sliderGap_->setObjectName(QStringLiteral("ad-color-picker-slider-gap"));
  sliderGap_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  sliderGap_->setFixedHeight(sliderSectionGapFromMetrics(g_marginSM, g_sliderMarginCross));
  pickerLayout->addWidget(sliderGap_);

  auto* formatRowWidget = new QWidget(pickerPanel_);
  formatRowWidget->setObjectName(QStringLiteral("ad-color-picker-format-row"));
  auto* formatRow = new QHBoxLayout(formatRowWidget);
  if (formatRowWidget->layoutDirection() == Qt::RightToLeft) {
    formatRow->setContentsMargins(0, 0, g_sliderMarginMain, 0);
  } else {
    formatRow->setContentsMargins(g_sliderMarginMain, 0, 0, 0);
  }
  formatRow->setSpacing(8);
  formatCombo_ = new AdSelect(formatRowWidget);
  formatCombo_->setObjectName(QString::fromLatin1(kFormatSelectObjectName));
  formatCombo_->setMode(AdSelect::Mode::Single);
  formatCombo_->setSize(AdSelect::Size::Small);
  formatCombo_->setVariant(AdSelect::Variant::Borderless);
  formatCombo_->setSearchEnabled(false);
  formatCombo_->setAllowClear(false);
  formatCombo_->setPlacement(AdSelect::Placement::BottomRight);
  formatCombo_->setPopupMatchSelectWidth(false);
  formatCombo_->setPopupWidth(kFormatSelectPopupWidth);
  const auto& mapForFormat = adqt::theme::ThemeManager::instance().currentMapToken();
  AdSelect::ComponentTokens formatSelectTokens;
  formatSelectTokens.horizontalPadding = 0;
  formatSelectTokens.borderWidth = 0;
  formatSelectTokens.iconSize = formatSelectIconSizeFromMap(mapForFormat);
  formatCombo_->setComponentTokens(formatSelectTokens);
  formatCombo_->setOptions({
      AdSelect::Option{QStringLiteral("hex"), QStringLiteral("HEX"), false, QString(), QVariantMap()},
      AdSelect::Option{QStringLiteral("hsb"), QStringLiteral("HSB"), false, QString(), QVariantMap()},
      AdSelect::Option{QStringLiteral("rgb"), QStringLiteral("RGB"), false, QString(), QVariantMap()},
  });
  formatInputHost_ = new QWidget(formatRowWidget);
  formatInputHost_->setObjectName(QStringLiteral("ad-color-picker-format-input-host"));
  auto* formatInputHostLayout = new QHBoxLayout(formatInputHost_);
  formatInputHostLayout->setContentsMargins(0, 0, 0, 0);
  formatInputHostLayout->setSpacing(4);

  hexInput_ = new AdInput(formatInputHost_);
  hexInput_->setObjectName(QStringLiteral("ad-color-picker-hex-input"));
  hexInput_->setTextAlignment(Qt::AlignCenter);
  hexInput_->setPrefixText(QStringLiteral("#"));
  hexInput_->setMaxLength(8);
  if (hexInput_->lineEdit()) {
    auto* validator = new QRegularExpressionValidator(
        QRegularExpression(QStringLiteral("^[0-9a-fA-F]{0,8}$")), hexInput_->lineEdit());
    hexInput_->lineEdit()->setValidator(validator);
  }
  formatInputHostLayout->addWidget(hexInput_);

  rgbInputHost_ = new QWidget(formatInputHost_);
  rgbInputHost_->setObjectName(QStringLiteral("ad-color-picker-rgb-input-host"));
  auto* rgbRow = new QHBoxLayout(rgbInputHost_);
  rgbRow->setContentsMargins(0, 0, 0, 0);
  rgbRow->setSpacing(4);
  rgbInputR_ = new AdInputNumber(rgbInputHost_);
  rgbInputR_->setObjectName(QStringLiteral("ad-color-picker-rgb-r-input"));
  rgbInputR_->setMin(0);
  rgbInputR_->setMax(255);
  rgbInputR_->setStep(1);
  rgbInputR_->setPrecision(0);
  rgbInputG_ = new AdInputNumber(rgbInputHost_);
  rgbInputG_->setObjectName(QStringLiteral("ad-color-picker-rgb-g-input"));
  rgbInputG_->setMin(0);
  rgbInputG_->setMax(255);
  rgbInputG_->setStep(1);
  rgbInputG_->setPrecision(0);
  rgbInputB_ = new AdInputNumber(rgbInputHost_);
  rgbInputB_->setObjectName(QStringLiteral("ad-color-picker-rgb-b-input"));
  rgbInputB_->setMin(0);
  rgbInputB_->setMax(255);
  rgbInputB_->setStep(1);
  rgbInputB_->setPrecision(0);
  if (rgbInputR_->lineEdit()) {
    rgbInputR_->lineEdit()->setAlignment(Qt::AlignCenter);
  }
  if (rgbInputG_->lineEdit()) {
    rgbInputG_->lineEdit()->setAlignment(Qt::AlignCenter);
  }
  if (rgbInputB_->lineEdit()) {
    rgbInputB_->lineEdit()->setAlignment(Qt::AlignCenter);
  }
  rgbRow->addWidget(rgbInputR_, 1);
  rgbRow->addWidget(rgbInputG_, 1);
  rgbRow->addWidget(rgbInputB_, 1);
  formatInputHostLayout->addWidget(rgbInputHost_);

  hsbInputHost_ = new QWidget(formatInputHost_);
  hsbInputHost_->setObjectName(QStringLiteral("ad-color-picker-hsb-input-host"));
  auto* hsbRow = new QHBoxLayout(hsbInputHost_);
  hsbRow->setContentsMargins(0, 0, 0, 0);
  hsbRow->setSpacing(4);
  hsbInputH_ = new AdInputNumber(hsbInputHost_);
  hsbInputH_->setObjectName(QStringLiteral("ad-color-picker-hsb-h-input"));
  hsbInputH_->setMin(0);
  hsbInputH_->setMax(360);
  hsbInputH_->setStep(1);
  hsbInputH_->setPrecision(0);
  hsbInputS_ = new AdInputNumber(hsbInputHost_);
  hsbInputS_->setObjectName(QStringLiteral("ad-color-picker-hsb-s-input"));
  hsbInputS_->setMin(0);
  hsbInputS_->setMax(100);
  hsbInputS_->setStep(1);
  hsbInputS_->setPrecision(0);
  hsbInputS_->setSuffixText(QStringLiteral("%"));
  hsbInputB_ = new AdInputNumber(hsbInputHost_);
  hsbInputB_->setObjectName(QStringLiteral("ad-color-picker-hsb-b-input"));
  hsbInputB_->setMin(0);
  hsbInputB_->setMax(100);
  hsbInputB_->setStep(1);
  hsbInputB_->setPrecision(0);
  hsbInputB_->setSuffixText(QStringLiteral("%"));
  if (hsbInputH_->lineEdit()) {
    hsbInputH_->lineEdit()->setAlignment(Qt::AlignCenter);
  }
  if (hsbInputS_->lineEdit()) {
    hsbInputS_->lineEdit()->setAlignment(Qt::AlignCenter);
  }
  if (hsbInputB_->lineEdit()) {
    hsbInputB_->lineEdit()->setAlignment(Qt::AlignCenter);
  }
  hsbRow->addWidget(hsbInputH_, 1);
  hsbRow->addWidget(hsbInputS_, 1);
  hsbRow->addWidget(hsbInputB_, 1);
  formatInputHostLayout->addWidget(hsbInputHost_);

  alphaInput_ = new AdInputNumber(formatRowWidget);
  alphaInput_->setObjectName(QStringLiteral("ad-color-picker-alpha-input"));
  alphaInput_->setMin(0);
  alphaInput_->setMax(100);
  alphaInput_->setStep(1);
  alphaInput_->setPrecision(0);
  alphaInput_->setSuffixText(QStringLiteral("%"));
  alphaInput_->setFixedWidth(g_alphaInputWidth);
  alphaInput_->setPlaceholder(QStringLiteral("100"));
  if (alphaInput_->lineEdit()) {
    alphaInput_->lineEdit()->setAlignment(Qt::AlignCenter);
  }
  formatRow->addWidget(formatCombo_);
  formatRow->addWidget(formatInputHost_, 1);
  formatRow->addWidget(alphaInput_);
  pickerLayout->addWidget(formatRowWidget);

  presetsPanel_ = new QWidget(panelHost_);
  presetsPanel_->setObjectName(QStringLiteral("ad-color-picker-presets-panel"));
  presetsPanel_->setAttribute(Qt::WA_StyledBackground, true);
  presetsLayout_ = new QVBoxLayout(presetsPanel_);
  presetsLayout_->setContentsMargins(0, 0, 0, 0);
  presetsLayout_->setSpacing(8);

  connect(modeButtonGroup_, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked), this,
          [this](QAbstractButton* button) {
            if (syncingControls_ || !button) {
              return;
            }
            const QString value = button->property("ad-color-picker-mode-value").toString();
            if (value.isEmpty()) {
              return;
            }
            setMode(parseModeName(value, mode_));
          });

  connect(clearButton_, &QAbstractButton::clicked, this, [this]() {
    if (!allowClear_ || cleared_) {
      return;
    }
    QColor clearColor = currentEditableColor().toHsv();
    if (!clearColor.isValid()) {
      clearColor = QColor(0, 0, 0, 0);
    }
    clearColor.setAlpha(0);
    if (mode_ == Mode::Gradient) {
      gradientStops_ = {
          InternalGradientStop{clearColor, 0},
          InternalGradientStop{clearColor, 100},
      };
      activeStopIndex_ = 0;
    } else {
      solidColor_ = clearColor;
    }
    cleared_ = true;
    refreshPanelControlsFromState();
    refreshTriggerDisplay();
    emit cleared();
    emit onClear();
    emitChangeSignals(true, true);
  });

  connect(gradientSlider_, &AdSlider::activeHandleIndexChanged, this, [this](int index) {
    if (syncingControls_ || mode_ != Mode::Gradient) {
      return;
    }
    const QVector<InternalGradientStop> normalized = normalizeGradientStops(gradientStops_);
    if (normalized.isEmpty()) {
      return;
    }
    const int nextIndex = std::clamp(index, 0, static_cast<int>(normalized.size()) - 1);
    if (activeStopIndex_ == nextIndex) {
      return;
    }
    activeStopIndex_ = nextIndex;
    refreshPanelControlsFromState(true);
    refreshTriggerDisplay();
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

  connect(formatCombo_, &AdSelect::valueChanged, this, [this](const QString& value) {
    if (syncingControls_ || !formatCombo_) {
      return;
    }
    setFormat(parseFormatName(value, format_));
  });
  connect(formatCombo_, &AdSelect::openChanged, this, [this](bool) {
    if (!formatCombo_) {
      return;
    }
    refreshStyle();
  });

  auto commitFormatInputs = [this]() {
    if (syncingControls_ || cleared_) {
      updateFormatInputText();
      return;
    }

    if (format_ == Format::Hex) {
      if (!hexInput_) {
        return;
      }
      const QString sanitized = sanitizeHexInput(hexInput_->value());
      if (hexInput_->value() != sanitized) {
        hexInput_->setValue(sanitized);
      }
      if (sanitized.size() != 6 && sanitized.size() != 8) {
        updateFormatInputText();
        return;
      }
      QColor parsed;
      if (!parseCssHexColor(QStringLiteral("#%1").arg(sanitized), &parsed) || !parsed.isValid()) {
        updateFormatInputText();
        return;
      }
      setCurrentEditableColor(parsed, true, true, true);
      return;
    }

    if (format_ == Format::Rgb) {
      if (!rgbInputR_ || !rgbInputG_ || !rgbInputB_) {
        return;
      }
      int r = 0;
      int g = 0;
      int b = 0;
      if (!parseBoundedInt(rgbInputR_->value(), 0, 255, &r) ||
          !parseBoundedInt(rgbInputG_->value(), 0, 255, &g) ||
          !parseBoundedInt(rgbInputB_->value(), 0, 255, &b)) {
        updateFormatInputText();
        return;
      }
      QColor next(r, g, b, currentEditableColor().alpha());
      setCurrentEditableColor(next, true, true, true);
      return;
    }

    if (!hsbInputH_ || !hsbInputS_ || !hsbInputB_) {
      return;
    }
    int h = 0;
    int s = 0;
    int v = 0;
    if (!parseBoundedInt(hsbInputH_->value(), 0, 360, &h) ||
        !parseBoundedInt(hsbInputS_->value(), 0, 100, &s) ||
        !parseBoundedInt(hsbInputB_->value(), 0, 100, &v)) {
      updateFormatInputText();
      return;
    }
    const int normalizedHue = (h == 360) ? 0 : h;
    const int sat = std::clamp(qRound(static_cast<double>(s) * 255.0 / 100.0), 0, 255);
    const int bri = std::clamp(qRound(static_cast<double>(v) * 255.0 / 100.0), 0, 255);
    QColor current = currentEditableColor().toHsv();
    const QColor next = QColor::fromHsv(normalizedHue, sat, bri, current.alpha());
    setCurrentEditableColor(next, true, true, true);
  };

  if (hexInput_ && hexInput_->lineEdit()) {
    connect(hexInput_->lineEdit(), &QLineEdit::textEdited, this, [this](const QString& text) {
      if (syncingControls_ || !hexInput_ || format_ != Format::Hex) {
        return;
      }
      const QString sanitized = sanitizeHexInput(text);
      if (sanitized != text) {
        hexInput_->setValue(sanitized);
      }
      if (sanitized.size() != 6 && sanitized.size() != 8) {
        return;
      }
      QColor parsed;
      if (!parseCssHexColor(QStringLiteral("#%1").arg(sanitized), &parsed) || !parsed.isValid()) {
        return;
      }
      setCurrentEditableColor(parsed, true, false, true);
    });
    connect(hexInput_->lineEdit(), &QLineEdit::editingFinished, this, commitFormatInputs);
  }

  auto bindFormatEdit = [this, &commitFormatInputs](AdInputNumber* input) {
    if (!input || !input->lineEdit()) {
      return;
    }
    connect(input->lineEdit(), &QLineEdit::editingFinished, this, commitFormatInputs);
  };
  bindFormatEdit(rgbInputR_);
  bindFormatEdit(rgbInputG_);
  bindFormatEdit(rgbInputB_);
  bindFormatEdit(hsbInputH_);
  bindFormatEdit(hsbInputS_);
  bindFormatEdit(hsbInputB_);

  if (alphaInput_ && alphaInput_->lineEdit()) {
    connect(alphaInput_->lineEdit(), &QLineEdit::editingFinished, this, [this]() {
      if (syncingControls_ || !alphaInput_) {
        return;
      }
      int percent = 0;
      if (!parseBoundedInt(alphaInput_->value(), 0, 100, &percent)) {
        refreshPanelControlsFromState();
        return;
      }

      QColor current = currentEditableColor().toHsv();
      int hue = current.hue();
      if (hue < 0) {
        hue = 0;
      }
      const int alpha = std::clamp(qRound(percent * 255.0 / 100.0), 0, 255);
      const QColor next = QColor::fromHsv(hue, current.saturation(), current.value(), alpha);
      setCurrentEditableColor(next, true, true, true);
    });
  }

  updateModeSegmentedOptions();
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
  const int triggerRadius = triggerRadiusForSize(size_, style);
  const int triggerMinWidth = componentTokens_.triggerMinWidth.has_value()
                                  ? style.metrics.triggerMinWidth
                                  : controlHeight;

  if (defaultTrigger_) {
    defaultTrigger_->setMinimumHeight(controlHeight);
    defaultTrigger_->setMaximumHeight(controlHeight);
    defaultTrigger_->setMinimumWidth(triggerMinWidth);
    defaultTrigger_->setFont(style.metrics.font);
  }

  if (triggerFrame_) {
    QColor border = style.triggerBorder;
    if (!disabled()) {
      border = open() ? style.triggerBorderActive
                      : (triggerHovered_ ? style.triggerBorderHover : style.triggerBorder);
    }
    triggerFrame_->setProperty("ad-color-picker-border-color", border.name(QColor::HexArgb));

    if (auto* triggerLayout = qobject_cast<QHBoxLayout*>(triggerFrame_->layout())) {
      const int pad = std::max(0, style.metrics.triggerPadding);
      // Keep the swatch leading inset consistent between color-only and showText modes.
      const int startPad = showText_
                               ? std::max(pad, std::max(0, (controlHeight - swatchSize) / 2))
                               : pad;
      const int endPad = showText_
                             ? std::max(0, pad + std::max(0, style.metrics.triggerTextMarginEnd))
                             : pad;
      triggerLayout->setContentsMargins(startPad, pad, endPad, pad);
      triggerLayout->setSpacing(showText_ ? std::max(0, style.metrics.triggerTextGap) : 0);
      triggerLayout->setAlignment(showText_ ? (Qt::AlignLeft | Qt::AlignVCenter)
                                            : (Qt::AlignHCenter | Qt::AlignVCenter));
    }

    triggerFrame_->setCursor(disabled() ? Qt::ForbiddenCursor : Qt::PointingHandCursor);
    if (auto* paintedTrigger = dynamic_cast<ColorPickerTriggerFrame*>(triggerFrame_.data())) {
      paintedTrigger->setVisualStyle(
          disabled() ? style.triggerBackgroundDisabled : style.triggerBackground, border,
          style.metrics.borderWidth, triggerRadius);
      // ColorPickerTriggerFrame paints its own border/background. Keep stylesheet empty
      // to avoid a second border from style-engine painting.
      triggerFrame_->setStyleSheet(QString());
    } else {
      const QString triggerFrameStyle =
          QStringLiteral("background:%1; border:%2px solid %3; border-radius:%4px;")
              .arg(disabled() ? style.triggerBackgroundDisabled.name(QColor::HexArgb)
                              : style.triggerBackground.name(QColor::HexArgb))
              .arg(QString::number(style.metrics.borderWidth, 'f', 1))
              .arg(border.name(QColor::HexArgb))
              .arg(triggerRadius);
      triggerFrame_->setStyleSheet(triggerFrameStyle);
    }
  }

  if (triggerSwatch_) {
    triggerSwatch_->setFixedSize(swatchSize, swatchSize);
  }

  if (triggerTextLabel_) {
    QFont textFont = style.metrics.font;
    if (size_ == Size::Large) {
      textFont.setPixelSize(std::max(textFont.pixelSize(), style.metrics.triggerTextFontSizeLG));
    }
    triggerTextLabel_->setFont(textFont);
    triggerTextLabel_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    if (size_ == Size::Small) {
      const int lineHeight = std::max(0, style.metrics.triggerTextLineHeightSM);
      triggerTextLabel_->setMinimumHeight(lineHeight);
      triggerTextLabel_->setMaximumHeight(lineHeight);
    } else {
      triggerTextLabel_->setMinimumHeight(0);
      triggerTextLabel_->setMaximumHeight(QWIDGETSIZE_MAX);
    }

    QPalette palette = triggerTextLabel_->palette();
    palette.setColor(QPalette::WindowText,
                     disabled() ? style.triggerTextDisabled : style.triggerText);
    triggerTextLabel_->setPalette(palette);
    triggerTextLabel_->setVisible(showText_);
  }

  if (panelHost_) {
    int contentWidth = style.metrics.panelWidth;
    if (componentTokens_.panelWidth.has_value()) {
      // Align with antd `styles.popupOverlayInner.width`: explicit width targets
      // popup container width, so content width should exclude popup paddings.
      const int popupPadding = std::max(0, style.metrics.panelPadding);
      contentWidth = std::max(1, style.metrics.panelWidth - popupPadding * 2);
    }
    panelHost_->setFixedWidth(contentWidth);
    panelHost_->setStyleSheet(
        QStringLiteral("#ad-color-picker-panel-host {"
                       " background:transparent; border:none; border-radius:0px; padding:0px;"
                       " }")
            );
  }

  if (pickerPanel_) {
    pickerPanel_->setFont(style.metrics.font);
    if (auto* pickerLayout = qobject_cast<QVBoxLayout*>(pickerPanel_->layout())) {
      pickerLayout->setSpacing(0);
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

  if (sliderContainer_) {
    const int sliderGroupGap =
        sliderGroupGapFromMetrics(style.metrics.marginSM, style.metrics.sliderMarginCross);
    const int sliderContainerHeight = sliderContainerHeightFromMetrics(
        style.metrics.previewSwatchSize, style.metrics.sliderVisualHeight, sliderGroupGap, disabledAlpha_);
    sliderContainer_->setMinimumHeight(sliderContainerHeight);
    sliderContainer_->setMaximumHeight(sliderContainerHeight);
    if (auto* sliderContainerLayout = qobject_cast<QHBoxLayout*>(sliderContainer_->layout())) {
      sliderContainerLayout->setSpacing(std::max(0, style.metrics.marginSM));
    }
  }

  if (sliderGroup_) {
    if (auto* sliderGroupLayout = qobject_cast<QVBoxLayout*>(sliderGroup_->layout())) {
      const int sliderGap =
          sliderGroupGapFromMetrics(style.metrics.marginSM, style.metrics.sliderMarginCross);
      sliderGroupLayout->setSpacing(sliderGap);
      if (hueSlider_) {
        sliderGroupLayout->setAlignment(hueSlider_, disabledAlpha_ ? Qt::AlignVCenter : Qt::Alignment());
      }
    }
  }

  const QVector<Mode> normalizedModes = normalizeModeOptions(modeOptions_);
  const bool showModeSwitch = normalizedModes.size() > 1;

  if (operationRow_) {
    operationRow_->setVisible(showModeSwitch || allowClear_);
  }
  if (modeSegmented_) {
    modeSegmented_->setVisible(showModeSwitch);
    modeSegmented_->setDisabled(!(showModeSwitch && !disabled()));
    modeSegmented_->setMinimumHeight(style.metrics.inputHeight);
    modeSegmented_->setMaximumHeight(style.metrics.inputHeight);

    const auto& mapToken = adqt::theme::ThemeManager::instance().currentMapToken();
    const int segmentedTrackPadding = std::max(1, qRound(mapToken.lineWidthBold));
    const int segmentedRadius = std::max(0, qRound(mapToken.borderRadiusSM));
    const int segmentedItemRadius = std::max(0, qRound(mapToken.borderRadiusXS));
    const int segmentedPadding = std::max(4, qRound(mapToken.sizeXS - mapToken.lineWidth));
    const int modeFontSize = std::max(10, qRound(mapToken.fontSize));

    if (auto* modeLayout = qobject_cast<QHBoxLayout*>(modeSegmented_->layout())) {
      modeLayout->setContentsMargins(segmentedTrackPadding, segmentedTrackPadding, segmentedTrackPadding,
                                     segmentedTrackPadding);
      modeLayout->setSpacing(0);
      const int buttonHeight = std::max(
          12, style.metrics.inputHeight - modeLayout->contentsMargins().top() -
                  modeLayout->contentsMargins().bottom());
      for (QPushButton* button :
           modeSegmented_->findChildren<QPushButton*>(QString(), Qt::FindDirectChildrenOnly)) {
        if (!button) {
          continue;
        }
        QFont modeFont = style.metrics.font;
        modeFont.setPixelSize(modeFontSize);
        button->setFont(modeFont);
        button->setMinimumHeight(buttonHeight);
        button->setMaximumHeight(buttonHeight);
      }
    }

    const QColor segmentedBg =
        QColor(mapToken.colorBgLayout).isValid() ? QColor(mapToken.colorBgLayout) : QColor("#f5f5f5");
    const QColor segmentedItemBg = style.panelBackground;
    const QColor segmentedHoverBg =
        QColor(mapToken.colorFillSecondary).isValid() ? QColor(mapToken.colorFillSecondary) : QColor("#f0f0f0");
    const QColor segmentedBorder = style.panelBorder;
    const QColor segmentedText =
        QColor(mapToken.colorTextSecondary).isValid() ? QColor(mapToken.colorTextSecondary) : style.panelText;
    const QColor segmentedTextDisabled = QColor(mapToken.colorTextQuaternary).isValid()
                                             ? QColor(mapToken.colorTextQuaternary)
                                             : QColor("#bfbfbf");
    modeSegmented_->setStyleSheet(
        QStringLiteral(
            "#ad-color-picker-mode-segmented {"
            " background:%1; border-radius:%2px;"
            " }"
            "#ad-color-picker-mode-segmented QPushButton {"
            " border:none;"
            " background:transparent;"
            " border-radius:%3px;"
            " color:%4;"
            " padding:0 %5px;"
            " }"
            "#ad-color-picker-mode-segmented QPushButton:hover:!checked {"
            " background:%6;"
            " }"
            "#ad-color-picker-mode-segmented QPushButton:checked {"
            " background:%7;"
            " border:1px solid %8;"
            " color:%9;"
            " }"
            "#ad-color-picker-mode-segmented QPushButton:disabled {"
            " color:%10;"
            " }")
            .arg(segmentedBg.name(QColor::HexArgb))
            .arg(segmentedRadius)
            .arg(segmentedItemRadius)
            .arg(segmentedText.name(QColor::HexArgb))
            .arg(segmentedPadding)
            .arg(segmentedHoverBg.name(QColor::HexArgb))
            .arg(segmentedItemBg.name(QColor::HexArgb))
            .arg(segmentedBorder.name(QColor::HexArgb))
            .arg(style.panelText.name(QColor::HexArgb))
            .arg(segmentedTextDisabled.name(QColor::HexArgb)));
  }
  if (formatCombo_) {
    const auto& mapForFormat = adqt::theme::ThemeManager::instance().currentMapToken();
    const int formatSelectFontSize = std::max(8, qRound(mapForFormat.fontSizeSM));
    const int formatSelectIconSize = formatSelectIconSizeFromMap(mapForFormat);
    const int formatSelectArrowGap = std::max(0, qRound(mapForFormat.sizeXXS));
    QFont formatSelectFont = style.metrics.font;
    formatSelectFont.setPixelSize(formatSelectFontSize);

    AdSelect::ComponentTokens formatTokens = formatCombo_->componentTokens();
    bool formatTokensChanged = false;
    const bool formatControlHeightSynced =
        formatTokens.controlHeight.has_value() &&
        formatTokens.controlHeight.value() == style.metrics.inputHeight;
    if (!formatControlHeightSynced) {
      formatTokens.controlHeight = style.metrics.inputHeight;
      formatTokensChanged = true;
    }
    const bool formatSelectorFontSynced =
        formatTokens.selectorFontSize.has_value() &&
        formatTokens.selectorFontSize.value() == formatSelectFontSize;
    if (!formatSelectorFontSynced) {
      formatTokens.selectorFontSize = formatSelectFontSize;
      formatTokensChanged = true;
    }
    const bool formatOptionFontSynced =
        formatTokens.optionFontSize.has_value() &&
        formatTokens.optionFontSize.value() == formatSelectFontSize;
    if (!formatOptionFontSynced) {
      formatTokens.optionFontSize = formatSelectFontSize;
      formatTokensChanged = true;
    }
    const bool formatIconSynced =
        formatTokens.iconSize.has_value() && formatTokens.iconSize.value() == formatSelectIconSize;
    if (!formatIconSynced) {
      formatTokens.iconSize = formatSelectIconSize;
      formatTokensChanged = true;
    }
    if (formatTokensChanged) {
      formatCombo_->setComponentTokens(formatTokens);
    }
    formatCombo_->setSize(AdSelect::Size::Small);
    formatCombo_->setVisible(!disabledFormat_);
    formatCombo_->setMinimumHeight(style.metrics.inputHeight);
    formatCombo_->setMaximumHeight(style.metrics.inputHeight);
    const int formatWidth =
        formatSelectWidthHint(formatSelectFont, formatSelectIconSize, formatSelectArrowGap);
    if (formatCombo_->minimumWidth() != formatWidth ||
        formatCombo_->maximumWidth() != formatWidth) {
      formatCombo_->setFixedWidth(formatWidth);
    }
    formatCombo_->setDisabled(disabledFormat_ || disabled());
  }
  if (auto* formatRowWidget =
          pickerPanel_ ? pickerPanel_->findChild<QWidget*>(QStringLiteral("ad-color-picker-format-row"),
                                                           Qt::FindDirectChildrenOnly)
                       : nullptr) {
    if (auto* formatRowLayout = qobject_cast<QHBoxLayout*>(formatRowWidget->layout())) {
      // AdSlider keeps a main-axis inset so handle rings are not clipped by QWidget bounds.
      // Compensate the input row inline-start with the same inset to align with AntD visuals.
      const int sliderInlineStartInset = std::max(0, style.metrics.sliderMarginMain);
      if (formatRowWidget->layoutDirection() == Qt::RightToLeft) {
        formatRowLayout->setContentsMargins(0, 0, sliderInlineStartInset, 0);
      } else {
        formatRowLayout->setContentsMargins(sliderInlineStartInset, 0, 0, 0);
      }
      formatRowLayout->setSpacing(std::max(0, style.metrics.marginXS));
    }
  }
  const auto& mapForInput = adqt::theme::ThemeManager::instance().currentMapToken();
  const int colorInputFontSize = std::max(8, qRound(mapForInput.fontSizeSM));
  const int colorHexHorizontalPadding = std::max(0, qRound(mapForInput.sizeXS));
  const int compactHorizontalPadding = std::max(0, qRound(mapForInput.sizeXXS));
  const int inputNumberHandleWidth = 16;

  auto applyFormatInputStyle = [&](AdInput* input, int horizontalPadding) {
    if (!input) {
      return;
    }
    AdInput::ComponentTokens inputTokens = input->componentTokens();
    bool inputTokensChanged = false;
    if (!inputTokens.controlHeight.has_value() ||
        inputTokens.controlHeight.value() != style.metrics.inputHeight) {
      inputTokens.controlHeight = style.metrics.inputHeight;
      inputTokensChanged = true;
    }
    if (!inputTokens.horizontalPadding.has_value() ||
        inputTokens.horizontalPadding.value() != horizontalPadding) {
      inputTokens.horizontalPadding = horizontalPadding;
      inputTokensChanged = true;
    }
    if (!inputTokens.inputFontSize.has_value() ||
        inputTokens.inputFontSize.value() != colorInputFontSize) {
      inputTokens.inputFontSize = colorInputFontSize;
      inputTokensChanged = true;
    }
    if (inputTokensChanged) {
      input->setComponentTokens(inputTokens);
    }

    // Ant Design keeps color picker panel inputs compact regardless of trigger size.
    input->setSize(AdInput::Size::Small);
    input->setMinimumHeight(style.metrics.inputHeight);
    input->setMaximumHeight(style.metrics.inputHeight);
    input->setDisabled(disabled());
  };

  auto applyFormatNumberInputStyle = [&](AdInputNumber* input, bool fixedAlphaWidth) {
    if (!input) {
      return;
    }

    AdInputNumber::ComponentTokens inputTokens = input->componentTokens();
    bool inputTokensChanged = false;
    if (!inputTokens.controlHeight.has_value() ||
        inputTokens.controlHeight.value() != style.metrics.inputHeight) {
      inputTokens.controlHeight = style.metrics.inputHeight;
      inputTokensChanged = true;
    }
    if (!inputTokens.horizontalPadding.has_value() ||
        inputTokens.horizontalPadding.value() != compactHorizontalPadding) {
      inputTokens.horizontalPadding = compactHorizontalPadding;
      inputTokensChanged = true;
    }
    if (!inputTokens.inputFontSize.has_value() ||
        inputTokens.inputFontSize.value() != colorInputFontSize) {
      inputTokens.inputFontSize = colorInputFontSize;
      inputTokensChanged = true;
    }
    if (!inputTokens.controlWidth.has_value() || inputTokens.controlWidth.value() != 48) {
      inputTokens.controlWidth = 48;
      inputTokensChanged = true;
    }
    if (!inputTokens.handleWidth.has_value() ||
        inputTokens.handleWidth.value() != inputNumberHandleWidth) {
      inputTokens.handleWidth = inputNumberHandleWidth;
      inputTokensChanged = true;
    }
    if (!inputTokens.handleVisibleWidth.has_value() ||
        inputTokens.handleVisibleWidth.value() != inputNumberHandleWidth) {
      inputTokens.handleVisibleWidth = inputNumberHandleWidth;
      inputTokensChanged = true;
    }
    if (inputTokensChanged) {
      input->setComponentTokens(inputTokens);
    }

    input->setSize(AdInputNumber::Size::Small);
    input->setMinimumHeight(style.metrics.inputHeight);
    input->setMaximumHeight(style.metrics.inputHeight);
    input->setDisabled(disabled() || (fixedAlphaWidth && disabledAlpha_));
    input->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    input->setMinimumWidth(0);
    if (fixedAlphaWidth) {
      input->setFixedWidth(style.metrics.alphaInputWidth);
      input->setVisible(!disabledAlpha_);
    }
  };

  applyFormatInputStyle(hexInput_, colorHexHorizontalPadding);
  applyFormatNumberInputStyle(rgbInputR_, false);
  applyFormatNumberInputStyle(rgbInputG_, false);
  applyFormatNumberInputStyle(rgbInputB_, false);
  applyFormatNumberInputStyle(hsbInputH_, false);
  applyFormatNumberInputStyle(hsbInputS_, false);
  applyFormatNumberInputStyle(hsbInputB_, false);
  applyFormatNumberInputStyle(alphaInput_, true);

  if (formatInputHost_) {
    if (auto* hostLayout = qobject_cast<QHBoxLayout*>(formatInputHost_->layout())) {
      hostLayout->setSpacing(std::max(2, qRound(mapForInput.sizeXXS)));
    }
  }
  if (rgbInputHost_) {
    if (auto* rgbLayout = qobject_cast<QHBoxLayout*>(rgbInputHost_->layout())) {
      rgbLayout->setSpacing(std::max(2, qRound(mapForInput.sizeXXS)));
    }
  }
  if (hsbInputHost_) {
    if (auto* hsbLayout = qobject_cast<QHBoxLayout*>(hsbInputHost_->layout())) {
      hsbLayout->setSpacing(std::max(2, qRound(mapForInput.sizeXXS)));
    }
  }

  updateFormatInputVisibility();

  if (clearButton_) {
    clearButton_->setVisible(allowClear_);
    clearButton_->setDisabled(disabled());
    const int clearSize = std::max(12, style.metrics.presetSwatchSize);
    clearButton_->setMinimumSize(clearSize, clearSize);
    clearButton_->setMaximumSize(clearSize, clearSize);
    if (auto* clear = dynamic_cast<ColorPickerClearButton*>(clearButton_.data())) {
      const auto& mapForClear = adqt::theme::ThemeManager::instance().currentMapToken();
      QColor slashColor(mapForClear.colorError);
      if (!slashColor.isValid()) {
        slashColor = QColor("#ff4d4f");
      }
      clear->setVisualStyle(style.panelBackground, style.panelBorder, style.triggerBorder, slashColor,
                            style.metrics.borderWidth, style.metrics.swatchRadius);
    }
  }

  if (gradientSection_) {
    gradientSection_->setVisible(mode_ == Mode::Gradient);
  }

  auto updateGapWidget = [](QWidget* gap, int height, bool visible) {
    if (!gap) {
      return;
    }
    const int targetHeight = std::max(0, height);
    if (gap->minimumHeight() != targetHeight || gap->maximumHeight() != targetHeight) {
      gap->setFixedHeight(targetHeight);
    }
    gap->setVisible(visible && targetHeight > 0);
  };
  const int sliderSectionGap =
      sliderSectionGapFromMetrics(style.metrics.marginSM, style.metrics.sliderMarginCross);
  updateGapWidget(operationGap_, style.metrics.marginXS,
                  operationRow_ && operationRow_->isVisible());
  updateGapWidget(gradientGap_, style.metrics.marginXS,
                  gradientSection_ && gradientSection_->isVisible());
  updateGapWidget(saturationGap_, sliderSectionGap,
                  saturationPanel_ && saturationPanel_->isVisible());
  updateGapWidget(sliderGap_, sliderSectionGap,
                  sliderContainer_ && sliderContainer_->isVisible());

  const auto& mapToken = adqt::theme::ThemeManager::instance().currentMapToken();
  auto buildCommonSliderTokens = [&style, &mapToken]() {
    AdSlider::ComponentTokens tokens;
    tokens.controlSize = style.metrics.sliderControlSize;
    tokens.railSize = style.metrics.sliderHeight;
    tokens.handleLineWidth = style.metrics.sliderHandleLineWidth;
    tokens.handleLineWidthHover = style.metrics.sliderHandleLineWidthHover;
    tokens.handleShadowColor = mapToken.colorFillSecondary;
    tokens.handleActiveShadowColor = mapToken.colorPrimaryActive;
    tokens.marginMain = style.metrics.sliderMarginMain;
    tokens.marginCross = style.metrics.sliderMarginCross;
    tokens.markGap = 0;
    tokens.focusOutlineSize = 0;
    return tokens;
  };

  if (gradientSlider_) {
    AdSlider::ComponentTokens tokens = buildCommonSliderTokens();
    tokens.handleSize = style.metrics.gradientHandleSize;
    tokens.handleSizeHover = style.metrics.gradientHandleSizeHover;
    gradientSlider_->setComponentTokens(tokens);
    gradientSlider_->setMinimumHeight(style.metrics.sliderVisualHeight);
    gradientSlider_->setMaximumHeight(style.metrics.sliderVisualHeight);
    gradientSlider_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
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
    AdSlider::ComponentTokens tokens = buildCommonSliderTokens();
    tokens.handleSize = style.metrics.sliderHandleSize;
    tokens.handleSizeHover = style.metrics.sliderHandleSizeHover;
    hueSlider_->setComponentTokens(tokens);
    hueSlider_->setMinimumHeight(style.metrics.sliderVisualHeight);
    hueSlider_->setMaximumHeight(style.metrics.sliderVisualHeight);
    hueSlider_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    hueSlider_->setEnabled(!disabled());
  }
  if (alphaSlider_) {
    AdSlider::ComponentTokens tokens = buildCommonSliderTokens();
    tokens.handleSize = style.metrics.sliderHandleSize;
    tokens.handleSizeHover = style.metrics.sliderHandleSizeHover;
    alphaSlider_->setComponentTokens(tokens);
    alphaSlider_->setMinimumHeight(style.metrics.sliderVisualHeight);
    alphaSlider_->setMaximumHeight(style.metrics.sliderVisualHeight);
    alphaSlider_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    alphaSlider_->setEnabled(!disabledAlpha_ && !disabled());
  }
  if (previewSwatch_) {
    previewSwatch_->setFixedSize(style.metrics.previewSwatchSize, style.metrics.previewSwatchSize);
  }

  AdPopover::ComponentTokens popoverTokens;
  // AdPopover token parser follows CSS color grammar (#RRGGBB[#AA] / rgb[a]).
  // Avoid Qt #AARRGGBB serialization here to prevent channel-order mismatch.
  popoverTokens.popupBg = colorToCss(style.panelBackground);
  popoverTokens.popupBorderColor = colorToCss(style.panelBorder);
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
  updateTriggerFocusOverlay();
}

void AdColorPicker::suppressTriggerUpdatesDuringInteraction() {
  if (triggerUpdatesSuppressed_) {
    return;
  }

  triggerUpdatesSuppressed_ = true;
  if (triggerFrame_) {
    triggerFrame_->setUpdatesEnabled(false);
  }
  if (triggerSwatch_) {
    triggerSwatch_->setUpdatesEnabled(false);
  }
  if (triggerTextLabel_) {
    triggerTextLabel_->setUpdatesEnabled(false);
  }
}

void AdColorPicker::resumeTriggerUpdatesAfterInteraction() {
  if (!triggerUpdatesSuppressed_) {
    return;
  }

  triggerUpdatesSuppressed_ = false;
  if (triggerFrame_) {
    triggerFrame_->setUpdatesEnabled(true);
    triggerFrame_->update();
  }
  if (triggerSwatch_) {
    triggerSwatch_->setUpdatesEnabled(true);
    triggerSwatch_->update();
  }
  if (triggerTextLabel_) {
    triggerTextLabel_->setUpdatesEnabled(true);
    triggerTextLabel_->update();
  }
}

void AdColorPicker::refreshTriggerDisplay(bool deferTextUpdate) {
  if (triggerUpdatesSuppressed_) {
    return;
  }

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
  const int swatchRadius = swatchRadiusForSize(size_, style);
  triggerSwatch_->setProperty("ad-color-picker-swatch-radius", swatchRadius);

  if (auto* swatch = dynamic_cast<ColorPickerSwatch*>(triggerSwatch_.data())) {
    swatch->setFrameStyle(style.swatchBorder, style.metrics.borderWidth, swatchRadius);
    const QColor checkerLight = disabled() ? style.triggerBackgroundDisabled : style.triggerBackground;
    swatch->setCheckerColors(checkerLight, style.transparentCellB, kTransparencyCell);
    if (cleared_) {
      swatch->setClearedTriggerFill();
    } else if (mode_ == Mode::Gradient && !gradientStops_.isEmpty()) {
      QVector<QPair<qreal, QColor>> stops;
      const QVector<InternalGradientStop> normalized = normalizeGradientStops(gradientStops_);
      stops.reserve(normalized.size());
      for (const InternalGradientStop& stop : normalized) {
        const qreal stopPos =
            static_cast<qreal>(std::clamp(static_cast<double>(stop.percent) / 100.0, 0.0, 1.0));
        stops.append(qMakePair(stopPos, stop.color));
      }
      swatch->setGradientFill(stops);
    } else {
      swatch->setSolidFill(solidColor_);
    }
  } else if (cleared_) {
    triggerSwatch_->setStyleSheet(
        QStringLiteral("border:1px solid %1; border-radius:%2px;"
                       "background:qlineargradient(x1:0,y1:0,x2:1,y2:1,"
                       "stop:0 #ffffff, stop:0.46 #ffffff, stop:0.47 #ff4d4f,"
                       "stop:0.53 #ff4d4f, stop:0.54 #ffffff, stop:1 #ffffff);")
            .arg(style.swatchBorder.name(QColor::HexArgb))
            .arg(swatchRadius));
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
            .arg(swatchRadius)
            .arg(stops.join(QStringLiteral(", "))));
  } else {
    triggerSwatch_->setStyleSheet(QStringLiteral("border:1px solid %1; border-radius:%2px; background:%3;")
                                      .arg(style.swatchBorder.name(QColor::HexArgb))
                                      .arg(swatchRadius)
                                      .arg(colorToCss(solidColor_)));
  }

  if (triggerTextLabel_) {
    if (!showText_) {
      triggerTextLabel_->clear();
      return;
    }

    // During high-frequency drag updates, defer trigger text mutation to the
    // completed step to avoid per-move relayout/paint churn.
    if (deferTextUpdate) {
      return;
    }

    const ColorValue displayValue = exportColorValue();
    QString text;
    bool useRichText = false;
    if (showTextFormatter_) {
      text = showTextFormatter_(displayValue, format_, activeStopIndex_);
    }

    if (text.trimmed().isEmpty()) {
      if (displayValue.cleared) {
        text = QStringLiteral("none");
      } else if (mode_ == Mode::Gradient && !displayValue.gradientStops.isEmpty()) {
        QStringList cells;
        const QVector<InternalGradientStop> normalized = normalizeGradientStops(gradientStops_);
        cells.reserve(normalized.size());
        // Ant Design only dims inactive gradient stops while the popup is open.
        const bool markInactive = open() && activeStopIndex_ >= 0 && activeStopIndex_ < normalized.size();
        if (markInactive) {
          useRichText = true;
          for (int i = 0; i < normalized.size(); ++i) {
            const InternalGradientStop& stop = normalized.at(i);
            const QString cellText =
                QStringLiteral("%1 %2%").arg(colorToRgbCssCompact(stop.color)).arg(stop.percent);
            const QColor cellColor = (i == activeStopIndex_) ? style.triggerText : style.triggerTextDisabled;
            cells.append(QStringLiteral("<span style=\"color:%1;\">%2</span>")
                             .arg(cellColor.name(QColor::HexArgb))
                             .arg(cellText.toHtmlEscaped()));
          }
        } else {
          for (const InternalGradientStop& stop : normalized) {
            cells.append(QStringLiteral("%1 %2%").arg(colorToRgbCssCompact(stop.color)).arg(stop.percent));
          }
        }
        text = cells.join(QStringLiteral(", "));
      } else {
        text = colorToString(currentEditableColor(), format_);
      }
    }
    Qt::TextFormat textFormat = useRichText ? Qt::RichText : Qt::AutoText;

    if (triggerTextLabel_->textFormat() != textFormat) {
      triggerTextLabel_->setTextFormat(textFormat);
    }
    if (triggerTextLabel_->text() != text) {
      triggerTextLabel_->setText(text);
    }
  }

}

void AdColorPicker::refreshPanelControlsFromState(bool minimal) {
  if (!pickerPanel_) {
    return;
  }

  QScopedValueRollback<bool> guard(syncingControls_, true);

  if (minimal) {
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
    }
    if (saturationPanel_) {
      saturationPanel_->setHue(hue);
      saturationPanel_->setSaturationBrightness(sat / 100.0, bri / 100.0);
    }
    if (alphaSlider_) {
      alphaSlider_->setValue(alpha);
    }
    if (alphaInput_) {
      if (cleared_) {
        alphaInput_->setValue(QVariant());
      } else {
        alphaInput_->setValue(alpha);
      }
    }

    refreshChannelVisuals();
    refreshPreviewSwatch();
    updateFormatInputText();
    return;
  }

  updateModeSegmentedOptions();
  if (modeButtonGroup_) {
    const QString currentMode = modeName(mode_);
    const QList<QAbstractButton*> buttons = modeButtonGroup_->buttons();
    for (QAbstractButton* button : buttons) {
      if (!button) {
        continue;
      }
      const bool shouldCheck = button->property("ad-color-picker-mode-value").toString() == currentMode;
      if (button->isChecked() != shouldCheck) {
        button->setChecked(shouldCheck);
      }
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
  if (modeSegmented_) {
    modeSegmented_->setVisible(showModeSwitch);
    modeSegmented_->setDisabled(!(showModeSwitch && !disabled()));
  }
  if (gradientSection_) {
    gradientSection_->setVisible(mode_ == Mode::Gradient);
  }
  if (alphaSection_) {
    alphaSection_->setVisible(!disabledAlpha_);
  }
  if (formatCombo_) {
    const QString currentFormat = formatName(format_);
    if (formatCombo_->value() != currentFormat) {
      formatCombo_->setValue(currentFormat);
    }
    formatCombo_->setDisabled(disabledFormat_ || disabled());
    formatCombo_->setVisible(!disabledFormat_);
  }
  if (alphaInput_) {
    if (cleared_) {
      alphaInput_->setValue(QVariant());
    } else {
      alphaInput_->setValue(alpha);
    }
    alphaInput_->setDisabled(disabled() || disabledAlpha_);
    alphaInput_->setVisible(!disabledAlpha_);
  }

  refreshChannelVisuals();
  refreshPreviewSwatch();
  updateFormatInputText();
}

void AdColorPicker::updateModeSegmentedOptions() {
  if (!modeSegmented_ || !modeButtonGroup_) {
    return;
  }

  const QVector<Mode> normalized = normalizeModeOptions(modeOptions_);
  if (!modeListContains(normalized, mode_)) {
    mode_ = normalized.constFirst();
    emit modeChanged(mode_);
  }

  auto* modeLayout = qobject_cast<QHBoxLayout*>(modeSegmented_->layout());
  if (!modeLayout) {
    return;
  }

  QStringList expectedValues;
  expectedValues.reserve(normalized.size());
  for (Mode value : normalized) {
    expectedValues.append(modeName(value));
  }

  QStringList existingValues;
  const QList<QPushButton*> existingButtons =
      modeSegmented_->findChildren<QPushButton*>(QString(), Qt::FindDirectChildrenOnly);
  existingValues.reserve(existingButtons.size());
  for (QPushButton* button : existingButtons) {
    if (!button) {
      continue;
    }
    existingValues.append(button->property("ad-color-picker-mode-value").toString());
  }

  if (existingValues != expectedValues) {
    while (QLayoutItem* item = modeLayout->takeAt(0)) {
      QWidget* widget = item->widget();
      if (auto* button = qobject_cast<QAbstractButton*>(widget)) {
        modeButtonGroup_->removeButton(button);
      }
      if (widget) {
        delete widget;
      }
      delete item;
    }

    for (Mode value : normalized) {
      const QString modeValue = modeName(value);
      auto* button = new QPushButton(modeLabel(value), modeSegmented_);
      button->setCheckable(true);
      button->setCursor(Qt::PointingHandCursor);
      button->setFocusPolicy(Qt::NoFocus);
      button->setProperty("ad-color-picker-mode-value", modeValue);
      modeButtonGroup_->addButton(button);
      modeLayout->addWidget(button);
    }
  }

  const bool canInteract = normalized.size() > 1 && !disabled();
  const QString currentMode = modeName(mode_);
  const QList<QAbstractButton*> buttons = modeButtonGroup_->buttons();
  for (QAbstractButton* button : buttons) {
    if (!button) {
      continue;
    }
    const Mode buttonMode =
        parseModeName(button->property("ad-color-picker-mode-value").toString(), Mode::Single);
    if (auto* push = qobject_cast<QPushButton*>(button)) {
      const QString label = modeLabel(buttonMode);
      if (push->text() != label) {
        push->setText(label);
      }
    }
    button->setEnabled(canInteract);
    const bool shouldCheck = button->property("ad-color-picker-mode-value").toString() == currentMode;
    if (button->isChecked() != shouldCheck) {
      button->setChecked(shouldCheck);
    }
  }
}

void AdColorPicker::refreshChannelVisuals() {
  if (hueSlider_) {
    AdSlider::SemanticStyles hueStyles;
    hueStyles.rail.brush = makeHueBrush();
    hueStyles.handle.borderColor = QColor("#ffffff");
    int hue = currentEditableColor().hsvHue();
    if (hue < 0) {
      hue = 0;
    }
    hueStyles.handle.backgroundColor = QColor::fromHsv(hue, 255, 255);
    hueSlider_->setSemanticStyles(hueStyles);
  }

  if (alphaSlider_) {
    AdSlider::SemanticStyles alphaStyles;
    alphaStyles.rail.brush = makeCheckerBrush(kTransparencyCell);
    alphaStyles.tracks.brush = makeAlphaBrush(currentEditableColor());
    alphaStyles.handle.borderColor = QColor("#ffffff");
    QColor alphaHandleColor = currentEditableColor().toRgb();
    alphaHandleColor.setAlpha(255);
    alphaStyles.handle.backgroundColor = alphaHandleColor;
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
  // Preview swatch visuals are independent from trigger text mode; keeping this
  // false avoids extra style work in high-frequency drag updates.
  input.showText = false;
  input.cleared = cleared_;
  input.baseFont = font();
  input.componentTokens = componentTokens_;
  input.semanticStyles = semanticStyles_;
  const detail::ColorPickerVisualStyle style = detail::resolveColorPickerVisualStyle(input);

  const int radius = style.metrics.previewSwatchRadius;
  previewSwatch_->setProperty("ad-color-picker-swatch-radius", radius);
  if (auto* swatch = dynamic_cast<ColorPickerSwatch*>(previewSwatch_.data())) {
    swatch->setFrameStyle(style.swatchBorder, style.metrics.borderWidth, radius, cleared_);
    swatch->setCheckerColors(style.panelBackground, style.transparentCellB, kTransparencyCell);
    if (cleared_) {
      swatch->setClearedPreviewFill();
    } else {
      swatch->setSolidFill(currentEditableColor());
    }
    return;
  }

  if (cleared_) {
    previewSwatch_->setStyleSheet(QStringLiteral("border:1px dashed %1; border-radius:%2px; background:transparent;")
                                      .arg(style.swatchBorder.name(QColor::HexArgb))
                                      .arg(radius));
  } else {
    const QString css = colorToCss(currentEditableColor());
    previewSwatch_->setStyleSheet(
        QStringLiteral("border:1px solid %1; border-radius:%2px; background:%3;")
            .arg(style.swatchBorder.name(QColor::HexArgb))
            .arg(radius)
            .arg(css));
  }
}

void AdColorPicker::updateFormatInputText() {
  if (cleared_) {
    if (hexInput_) {
      hexInput_->setValue(QString());
    }
    if (rgbInputR_) {
      rgbInputR_->setValue(QVariant());
    }
    if (rgbInputG_) {
      rgbInputG_->setValue(QVariant());
    }
    if (rgbInputB_) {
      rgbInputB_->setValue(QVariant());
    }
    if (hsbInputH_) {
      hsbInputH_->setValue(QVariant());
    }
    if (hsbInputS_) {
      hsbInputS_->setValue(QVariant());
    }
    if (hsbInputB_) {
      hsbInputB_->setValue(QVariant());
    }
    return;
  }

  const QColor color = currentEditableColor();
  if (!color.isValid()) {
    return;
  }

  if (format_ == Format::Hex) {
    if (!hexInput_) {
      return;
    }
    QString text = colorToString(color, Format::Hex).toUpper();
    if (text.startsWith(QLatin1Char('#'))) {
      text.remove(0, 1);
    }
    hexInput_->setValue(text);
    return;
  }

  if (format_ == Format::Rgb) {
    if (rgbInputR_) {
      rgbInputR_->setValue(color.red());
    }
    if (rgbInputG_) {
      rgbInputG_->setValue(color.green());
    }
    if (rgbInputB_) {
      rgbInputB_->setValue(color.blue());
    }
    return;
  }

  int hue = color.hsvHue();
  if (hue < 0) {
    hue = 0;
  }
  const int sat = std::clamp(qRound(color.saturationF() * 100.0), 0, 100);
  const int bri = std::clamp(qRound(color.valueF() * 100.0), 0, 100);
  if (hsbInputH_) {
    hsbInputH_->setValue(hue);
  }
  if (hsbInputS_) {
    hsbInputS_->setValue(sat);
  }
  if (hsbInputB_) {
    hsbInputB_->setValue(bri);
  }
}

void AdColorPicker::updateFormatInputVisibility() {
  if (hexInput_) {
    hexInput_->setVisible(format_ == Format::Hex);
  }
  if (rgbInputHost_) {
    rgbInputHost_->setVisible(format_ == Format::Rgb);
  }
  if (hsbInputHost_) {
    hsbInputHost_->setVisible(format_ == Format::Hsb);
  }
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

  const bool deferTriggerRefresh = fromUser && !emitCompleted && open();
  refreshPanelControlsFromState(deferTriggerRefresh);
  if (deferTriggerRefresh) {
    suppressTriggerUpdatesDuringInteraction();
  } else {
    resumeTriggerUpdatesAfterInteraction();
    refreshTriggerDisplay();
  }
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

  const QVector<InternalGradientStop> current = normalizeGradientStops(gradientStops_);
  auto mixColor = [](const QColor& lhs, const QColor& rhs, double ratio) {
    const double t = std::clamp(ratio, 0.0, 1.0);
    return QColor(
        std::clamp(qRound(lhs.red() + (rhs.red() - lhs.red()) * t), 0, 255),
        std::clamp(qRound(lhs.green() + (rhs.green() - lhs.green()) * t), 0, 255),
        std::clamp(qRound(lhs.blue() + (rhs.blue() - lhs.blue()) * t), 0, 255),
        std::clamp(qRound(lhs.alpha() + (rhs.alpha() - lhs.alpha()) * t), 0, 255));
  };
  auto sampleColorAtPercent = [&](int percent) -> QColor {
    if (current.isEmpty()) {
      return solidColor_.isValid() ? solidColor_ : QColor("#1677ff");
    }
    const int target = std::clamp(percent, 0, 100);
    if (target <= current.constFirst().percent) {
      return current.constFirst().color;
    }
    if (target >= current.constLast().percent) {
      return current.constLast().color;
    }
    for (int i = 0; i + 1 < current.size(); ++i) {
      const InternalGradientStop& lhs = current.at(i);
      const InternalGradientStop& rhs = current.at(i + 1);
      if (target < lhs.percent || target > rhs.percent) {
        continue;
      }
      const int span = rhs.percent - lhs.percent;
      if (span <= 0) {
        return rhs.color;
      }
      return mixColor(lhs.color, rhs.color, static_cast<double>(target - lhs.percent) / span);
    }
    return current.constLast().color;
  };
  QVector<int> oldToNew(current.size(), -1);
  QVector<int> newToOld(percents.size(), -1);
  QVector<bool> oldUsed(current.size(), false);

  for (int newIndex = 0; newIndex < percents.size(); ++newIndex) {
    const int percent = percents.at(newIndex);
    for (int oldIndex = 0; oldIndex < current.size(); ++oldIndex) {
      if (oldUsed.at(oldIndex) || current.at(oldIndex).percent != percent) {
        continue;
      }
      oldUsed[oldIndex] = true;
      oldToNew[oldIndex] = newIndex;
      newToOld[newIndex] = oldIndex;
      break;
    }
  }

  QVector<int> unmatchedOld;
  unmatchedOld.reserve(current.size());
  for (int oldIndex = 0; oldIndex < current.size(); ++oldIndex) {
    if (oldToNew.at(oldIndex) < 0) {
      unmatchedOld.append(oldIndex);
    }
  }

  QVector<int> unmatchedNew;
  unmatchedNew.reserve(percents.size());
  for (int newIndex = 0; newIndex < percents.size(); ++newIndex) {
    if (newToOld.at(newIndex) < 0) {
      unmatchedNew.append(newIndex);
    }
  }

  int addedIndex = -1;
  int removedOldIndex = -1;
  if (unmatchedOld.size() == 1 && unmatchedNew.size() == 1) {
    // Drag move: preserve the moved stop color by index rather than by position.
    const int oldIndex = unmatchedOld.constFirst();
    const int newIndex = unmatchedNew.constFirst();
    oldToNew[oldIndex] = newIndex;
    newToOld[newIndex] = oldIndex;
  } else if (unmatchedOld.isEmpty() && unmatchedNew.size() == 1) {
    // Add: the new stop should use the interpolated color at insertion percent.
    addedIndex = unmatchedNew.constFirst();
  } else if (unmatchedOld.size() == 1 && unmatchedNew.isEmpty()) {
    // Delete: remaining stops keep their colors; only the removed index disappears.
    removedOldIndex = unmatchedOld.constFirst();
  } else {
    // Fallback for non-standard transitions: pair remaining indexes in order.
    const int pairCount = std::min(unmatchedOld.size(), unmatchedNew.size());
    for (int i = 0; i < pairCount; ++i) {
      const int oldIndex = unmatchedOld.at(i);
      const int newIndex = unmatchedNew.at(i);
      oldToNew[oldIndex] = newIndex;
      newToOld[newIndex] = oldIndex;
    }
  }

  QVector<InternalGradientStop> next;
  next.reserve(percents.size());
  const QColor fallbackColor = solidColor_.isValid() ? solidColor_ : QColor("#1677ff");
  for (int newIndex = 0; newIndex < percents.size(); ++newIndex) {
    InternalGradientStop stop;
    stop.percent = percents.at(newIndex);
    const int mappedOldIndex = newToOld.at(newIndex);
    if (mappedOldIndex >= 0 && mappedOldIndex < current.size()) {
      stop.color = current.at(mappedOldIndex).color;
    } else {
      stop.color = sampleColorAtPercent(stop.percent);
    }
    if (!stop.color.isValid()) {
      stop.color = fallbackColor;
    }
    next.append(stop);
  }

  gradientStops_ = normalizeGradientStops(next);

  int nextActiveIndex = activeStopIndex_;
  if (addedIndex >= 0) {
    nextActiveIndex = addedIndex;
  } else if (activeStopIndex_ >= 0 && activeStopIndex_ < oldToNew.size() &&
             oldToNew.at(activeStopIndex_) >= 0) {
    nextActiveIndex = oldToNew.at(activeStopIndex_);
  } else if (removedOldIndex >= 0 && activeStopIndex_ >= 0) {
    if (activeStopIndex_ > removedOldIndex) {
      nextActiveIndex = activeStopIndex_ - 1;
    } else if (activeStopIndex_ == removedOldIndex) {
      nextActiveIndex = removedOldIndex;
    }
  }
  if (!gradientStops_.isEmpty()) {
    nextActiveIndex = std::clamp(nextActiveIndex, 0, static_cast<int>(gradientStops_.size()) - 1);
  } else {
    nextActiveIndex = 0;
  }
  activeStopIndex_ = nextActiveIndex;

  cleared_ = false;
  const bool deferTriggerRefresh = !emitCompleted && open();
  refreshPanelControlsFromState(deferTriggerRefresh);
  if (deferTriggerRefresh) {
    suppressTriggerUpdatesDuringInteraction();
  } else {
    resumeTriggerUpdatesAfterInteraction();
    refreshTriggerDisplay();
  }
  emitChangeSignals(emitCompleted, true);
}

QColor AdColorPicker::parseColorString(const QString& value, bool* ok) const {
  const QString text = value.trimmed();
  QColor cssHex;
  if (parseCssHexColor(text, &cssHex)) {
    if (ok) {
      *ok = true;
    }
    return cssHex;
  }

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
    return colorToHexRgbaLower(color);
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
  QVector<InternalGradientStop> normalized;
  normalized.reserve(stops.size());
  for (const InternalGradientStop& stop : stops) {
    if (!stop.color.isValid()) {
      continue;
    }
    normalized.append(InternalGradientStop{stop.color, std::clamp(stop.percent, 0, 100)});
  }

  std::stable_sort(normalized.begin(), normalized.end(),
                   [](const InternalGradientStop& lhs, const InternalGradientStop& rhs) {
                     return lhs.percent < rhs.percent;
                   });

  if (normalized.isEmpty()) {
    normalized = {
        InternalGradientStop{solidColor_.isValid() ? solidColor_ : QColor("#1677ff"), 0},
        InternalGradientStop{solidColor_.isValid() ? solidColor_ : QColor("#1677ff"), 100},
    };
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
