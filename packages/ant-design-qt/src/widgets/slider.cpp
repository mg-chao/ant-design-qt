#include "slider.h"

#include "slider_style.h"
#include "theme/theme.h"

#include <QEnterEvent>
#include <QEvent>
#include <QFocusEvent>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>

#include <algorithm>
#include <cmath>
#include <limits>

namespace adqt::widgets {

namespace {

constexpr double kEpsilon = 1e-6;

bool fuzzyEq(double lhs, double rhs) { return std::abs(lhs - rhs) <= kEpsilon; }

double clampValue(double value, double minValue, double maxValue) {
  if (maxValue < minValue) {
    return minValue;
  }
  return std::clamp(value, minValue, maxValue);
}

bool listFuzzyEquals(const QList<double>& lhs, const QList<double>& rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (int i = 0; i < lhs.size(); ++i) {
    if (!fuzzyEq(lhs.at(i), rhs.at(i))) {
      return false;
    }
  }
  return true;
}

bool marksEqual(const AdSlider::MarkMap& lhs, const AdSlider::MarkMap& rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (auto it = lhs.cbegin(); it != lhs.cend(); ++it) {
    if (!rhs.contains(it.key())) {
      return false;
    }
    if (!(it.value() == rhs.value(it.key()))) {
      return false;
    }
  }
  return true;
}

bool isKeyboardFocusReason(Qt::FocusReason reason) {
  return reason == Qt::TabFocusReason || reason == Qt::BacktabFocusReason ||
         reason == Qt::ShortcutFocusReason;
}

QString formatNumber(double value) {
  if (!std::isfinite(value)) {
    return QStringLiteral("0");
  }
  QString text = QString::number(value, 'f', 4);
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

}  // namespace

struct AdSlider::LayoutInfo {
  detail::SliderVisualStyle style;
  QRectF contentRect;
  QRectF railRect;
  QList<QRectF> handleRects;
  QList<QPointF> markCenters;
  QList<double> markValues;
  bool vertical = false;
  qreal axisStart = 0.0;
  qreal axisLength = 1.0;
  qreal crossCenter = 0.0;
  int activeHandleSize = 0;
  int normalHandleSize = 0;
  int markLabelOffset = 0;
};

AdSlider::AdSlider(QWidget* parent) : QWidget(parent) {
  setAttribute(Qt::WA_Hover, true);
  setFocusPolicy(Qt::StrongFocus);
  setMouseTracking(true);
  handles_ = {minimum_};

  connect(&adqt::theme::ThemeManager::instance(), &adqt::theme::ThemeManager::themeChanged, this,
          [this]() { refreshAfterPropertyChange(); });
}

AdSlider::~AdSlider() = default;

AdSlider::Mode AdSlider::mode() const { return mode_; }

void AdSlider::setMode(Mode value) {
  if (mode_ == value) {
    return;
  }
  mode_ = value;
  setHandlesInternal(handles_, true, false);
  emit modeChanged(mode_);
  refreshAfterPropertyChange();
}

double AdSlider::minimum() const { return minimum_; }

void AdSlider::setMinimum(double value) {
  if (fuzzyEq(minimum_, value)) {
    return;
  }
  minimum_ = value;
  if (maximum_ < minimum_) {
    maximum_ = minimum_;
    emit maximumChanged(maximum_);
  }
  setHandlesInternal(handles_, true, false);
  emit minimumChanged(minimum_);
  refreshAfterPropertyChange();
}

double AdSlider::maximum() const { return maximum_; }

void AdSlider::setMaximum(double value) {
  if (fuzzyEq(maximum_, value)) {
    return;
  }
  maximum_ = value;
  if (minimum_ > maximum_) {
    minimum_ = maximum_;
    emit minimumChanged(minimum_);
  }
  setHandlesInternal(handles_, true, false);
  emit maximumChanged(maximum_);
  refreshAfterPropertyChange();
}

double AdSlider::step() const { return step_; }

void AdSlider::setStep(double value) {
  const double normalized = value <= 0.0 ? 0.0 : value;
  if (fuzzyEq(step_, normalized)) {
    return;
  }
  step_ = normalized;
  setHandlesInternal(handles_, true, false);
  emit stepChanged(step_);
  refreshAfterPropertyChange(false);
}

bool AdSlider::marksOnly() const { return marksOnly_; }

void AdSlider::setMarksOnly(bool value) {
  if (marksOnly_ == value) {
    return;
  }
  marksOnly_ = value;
  setHandlesInternal(handles_, true, false);
  emit marksOnlyChanged(marksOnly_);
  refreshAfterPropertyChange(false);
}

bool AdSlider::dots() const { return dots_; }

void AdSlider::setDots(bool value) {
  if (dots_ == value) {
    return;
  }
  dots_ = value;
  emit dotsChanged(dots_);
  update();
}

bool AdSlider::included() const { return included_; }

void AdSlider::setIncluded(bool value) {
  if (included_ == value) {
    return;
  }
  included_ = value;
  emit includedChanged(included_);
  update();
}

bool AdSlider::reverse() const { return reverse_; }

void AdSlider::setReverse(bool value) {
  if (reverse_ == value) {
    return;
  }
  reverse_ = value;
  emit reverseChanged(reverse_);
  update();
}

Qt::Orientation AdSlider::orientation() const { return orientation_; }

void AdSlider::setOrientation(Qt::Orientation value) {
  if (orientation_ == value) {
    return;
  }
  orientation_ = value;
  emit orientationChanged(orientation_);
  refreshAfterPropertyChange();
}

bool AdSlider::disabled() const { return !isEnabled(); }

void AdSlider::setDisabled(bool value) {
  if (disabled() == value) {
    return;
  }
  setEnabled(!value);
  hovered_ = false;
  hoverHandleIndex_ = -1;
  dragMode_ = DragMode::None;
  dragging_ = false;
  dragHandleIndex_ = -1;
  emit disabledChanged(value);
  update();
}

bool AdSlider::keyboardEnabled() const { return keyboardEnabled_; }

void AdSlider::setKeyboardEnabled(bool value) {
  if (keyboardEnabled_ == value) {
    return;
  }
  keyboardEnabled_ = value;
  emit keyboardEnabledChanged(keyboardEnabled_);
}

double AdSlider::value() const { return handles_.isEmpty() ? minimum_ : handles_.constFirst(); }

void AdSlider::setValue(double value) {
  if (mode_ == Mode::Single) {
    setHandlesInternal({value}, true, false);
    return;
  }
  QList<double> next = handles_;
  if (next.isEmpty()) {
    next = {value};
  } else {
    next[0] = value;
  }
  setHandlesInternal(next, true, false);
}

QList<double> AdSlider::values() const { return handles_; }

void AdSlider::setValues(const QList<double>& values) { setHandlesInternal(values, true, false); }

bool AdSlider::draggableTrack() const { return draggableTrack_; }

void AdSlider::setDraggableTrack(bool value) {
  if (draggableTrack_ == value) {
    return;
  }
  draggableTrack_ = value;
  if (draggableTrack_ && editableHandles_) {
    editableHandles_ = false;
    emit editableHandlesChanged(editableHandles_);
  }
  emit draggableTrackChanged(draggableTrack_);
  update();
}

bool AdSlider::editableHandles() const { return editableHandles_; }

void AdSlider::setEditableHandles(bool value) {
  if (editableHandles_ == value) {
    return;
  }
  editableHandles_ = value;
  if (editableHandles_ && draggableTrack_) {
    draggableTrack_ = false;
    emit draggableTrackChanged(draggableTrack_);
  }
  setHandlesInternal(handles_, true, false);
  emit editableHandlesChanged(editableHandles_);
  update();
}

int AdSlider::minHandleCount() const { return minHandleCount_; }

void AdSlider::setMinHandleCount(int value) {
  const int normalized = std::max(0, value);
  if (minHandleCount_ == normalized) {
    return;
  }
  minHandleCount_ = normalized;
  setHandlesInternal(handles_, true, false);
  emit minHandleCountChanged(minHandleCount_);
  update();
}

int AdSlider::maxHandleCount() const { return maxHandleCount_; }

void AdSlider::setMaxHandleCount(int value) {
  int normalized = value;
  if (normalized == 0) {
    normalized = -1;
  }
  if (normalized < -1) {
    normalized = -1;
  }
  if (maxHandleCount_ == normalized) {
    return;
  }
  maxHandleCount_ = normalized;
  setHandlesInternal(handles_, true, false);
  emit maxHandleCountChanged(maxHandleCount_);
  update();
}

bool AdSlider::tooltipEnabled() const { return tooltipEnabled_; }

void AdSlider::setTooltipEnabled(bool value) {
  if (tooltipEnabled_ == value) {
    return;
  }
  tooltipEnabled_ = value;
  emit tooltipEnabledChanged(tooltipEnabled_);
  update();
}

AdSlider::TooltipVisibleMode AdSlider::tooltipVisibleMode() const { return tooltipVisibleMode_; }

void AdSlider::setTooltipVisibleMode(TooltipVisibleMode value) {
  if (tooltipVisibleMode_ == value) {
    return;
  }
  tooltipVisibleMode_ = value;
  emit tooltipVisibleModeChanged(tooltipVisibleMode_);
  update();
}

AdSlider::TooltipPlacement AdSlider::tooltipPlacement() const { return tooltipPlacement_; }

void AdSlider::setTooltipPlacement(TooltipPlacement value) {
  if (tooltipPlacement_ == value) {
    return;
  }
  tooltipPlacement_ = value;
  emit tooltipPlacementChanged(tooltipPlacement_);
  update();
}

AdSlider::MarkMap AdSlider::marks() const { return marks_; }

void AdSlider::setMarks(const MarkMap& marks) {
  if (marksEqual(marks_, marks)) {
    return;
  }
  marks_ = marks;
  setHandlesInternal(handles_, true, false);
  emit marksChanged();
  refreshAfterPropertyChange();
}

void AdSlider::clearMarks() {
  if (marks_.isEmpty()) {
    return;
  }
  marks_.clear();
  setHandlesInternal(handles_, true, false);
  emit marksChanged();
  refreshAfterPropertyChange();
}

AdSlider::TooltipFormatter AdSlider::tooltipFormatter() const { return tooltipFormatter_; }

void AdSlider::setTooltipFormatter(TooltipFormatter formatter) {
  tooltipFormatter_ = std::move(formatter);
  update();
}

AdSlider::ComponentTokens AdSlider::componentTokens() const { return componentTokens_; }

void AdSlider::setComponentTokens(const ComponentTokens& tokens) {
  componentTokens_ = tokens;
  emit componentTokensChanged();
  refreshAfterPropertyChange();
}

void AdSlider::resetComponentTokens() {
  componentTokens_ = {};
  emit componentTokensChanged();
  refreshAfterPropertyChange();
}

AdSlider::SemanticStyles AdSlider::semanticStyles() const { return semanticStyles_; }

void AdSlider::setSemanticStyles(const SemanticStyles& styles) {
  semanticStyles_ = styles;
  emit semanticStylesChanged();
  update();
}

void AdSlider::setSemanticStyleResolver(SemanticStyleResolver resolver) {
  semanticStyleResolver_ = std::move(resolver);
  emit semanticStylesChanged();
  update();
}

QSize AdSlider::sizeHint() const {
  const LayoutInfo layout = buildLayout();
  const bool hasMarks = !marks_.isEmpty();
  if (orientation_ == Qt::Horizontal) {
    const int height = std::max(34, layout.style.metrics.controlSize + layout.style.metrics.marginCross * 2 +
                                        (hasMarks ? layout.style.metrics.markGap + QFontMetrics(layout.style.metrics.font).height()
                                                  : 0));
    return QSize(260, height);
  }
  const int width = std::max(52, layout.style.metrics.controlSize + layout.style.metrics.marginCross * 2 +
                                     (hasMarks ? layout.style.metrics.markGap + QFontMetrics(layout.style.metrics.font).horizontalAdvance(QStringLiteral("100"))
                                               : 0));
  return QSize(width, 260);
}

QSize AdSlider::minimumSizeHint() const {
  const QSize hint = sizeHint();
  if (orientation_ == Qt::Horizontal) {
    return QSize(120, hint.height());
  }
  return QSize(hint.width(), 120);
}

AdSlider::MarkMap AdSlider::effectiveMarks() const {
  MarkMap out = marks_;
  for (auto it = out.begin(); it != out.end(); ++it) {
    if (it->label.trimmed().isEmpty()) {
      it->label = formatNumber(it.key());
    }
  }
  return out;
}

AdSlider::SemanticStyles AdSlider::resolvedSemanticStyles() const {
  SemanticStyles merged = semanticStyles_;
  if (!semanticStyleResolver_) {
    return merged;
  }

  StyleContext ctx;
  ctx.mode = mode_;
  ctx.orientation = orientation_;
  ctx.reverse = reverse_;
  ctx.disabled = disabled();
  ctx.dragging = dragging_;
  ctx.hovered = hovered_;
  ctx.focused = hasFocus() && focusVisible_;
  ctx.values = handles_;
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
    if (source.brush.has_value()) {
      target->brush = source.brush;
    }
  };

  mergeSlot(&merged.root, resolved.root);
  mergeSlot(&merged.rail, resolved.rail);
  mergeSlot(&merged.track, resolved.track);
  mergeSlot(&merged.tracks, resolved.tracks);
  mergeSlot(&merged.handle, resolved.handle);
  mergeSlot(&merged.mark, resolved.mark);
  mergeSlot(&merged.markActive, resolved.markActive);
  return merged;
}

QList<double> AdSlider::normalizedValues(const QList<double>& values, bool forceRangeMode) const {
  const bool rangeMode = forceRangeMode || mode_ == Mode::Range;
  QList<double> normalized = values;

  if (!rangeMode) {
    if (normalized.isEmpty()) {
      normalized = {minimum_};
    } else {
      normalized = {normalized.constFirst()};
    }
    normalized[0] = normalizeValue(normalized.constFirst());
    return normalized;
  }

  if (normalized.isEmpty()) {
    normalized = {minimum_, minimum_};
  }

  const int requiredMinCount = editableHandles_ ? std::max(1, minHandleCount_) : 2;
  if (normalized.size() < requiredMinCount) {
    const double fillValue = normalized.isEmpty() ? minimum_ : normalized.constLast();
    while (normalized.size() < requiredMinCount) {
      normalized.append(fillValue);
    }
  }

  if (maxHandleCount_ > 0 && normalized.size() > maxHandleCount_) {
    normalized = normalized.mid(0, maxHandleCount_);
  }

  for (double& value : normalized) {
    value = normalizeValue(value);
  }
  std::sort(normalized.begin(), normalized.end());
  return normalized;
}

QList<double> AdSlider::snapPoints() const {
  QList<double> points;
  points.reserve(marks_.size() + 2);
  points.append(minimum_);
  points.append(maximum_);
  for (auto it = marks_.cbegin(); it != marks_.cend(); ++it) {
    points.append(clampValue(it.key(), minimum_, maximum_));
  }
  std::sort(points.begin(), points.end());
  points.erase(std::unique(points.begin(), points.end(), [](double lhs, double rhs) {
                 return std::abs(lhs - rhs) <= kEpsilon;
               }),
               points.end());
  return points;
}

double AdSlider::normalizeValue(double value) const {
  if (maximum_ <= minimum_) {
    return minimum_;
  }

  double normalized = clampValue(value, minimum_, maximum_);

  if (marksOnly_) {
    const QList<double> points = snapPoints();
    if (!points.isEmpty()) {
      double nearest = points.constFirst();
      double bestDistance = std::abs(normalized - nearest);
      for (double point : points) {
        const double dist = std::abs(normalized - point);
        if (dist < bestDistance) {
          bestDistance = dist;
          nearest = point;
        }
      }
      normalized = nearest;
    }
  } else if (step_ > 0.0) {
    const double snapped = minimum_ + std::round((normalized - minimum_) / step_) * step_;
    normalized = clampValue(snapped, minimum_, maximum_);
  }

  if (dots_ && !marks_.isEmpty()) {
    const QList<double> points = snapPoints();
    if (!points.isEmpty()) {
      double nearest = points.constFirst();
      double bestDistance = std::abs(normalized - nearest);
      for (double point : points) {
        const double dist = std::abs(normalized - point);
        if (dist < bestDistance) {
          bestDistance = dist;
          nearest = point;
        }
      }
      normalized = nearest;
    }
  }

  normalized = std::round(normalized * 1000000.0) / 1000000.0;
  return clampValue(normalized, minimum_, maximum_);
}

void AdSlider::setHandlesInternal(const QList<double>& handles,
                                  bool emitValueChangedSignal,
                                  bool fromUserAction) {
  const QList<double> normalized = normalizedValues(handles, mode_ == Mode::Range);
  if (listFuzzyEquals(handles_, normalized)) {
    return;
  }

  handles_ = normalized;

  if (focusHandleIndex_ >= handles_.size()) {
    focusHandleIndex_ = handles_.isEmpty() ? -1 : handles_.size() - 1;
  } else if (focusHandleIndex_ < 0 && !handles_.isEmpty()) {
    focusHandleIndex_ = 0;
  }

  if (dragHandleIndex_ >= handles_.size()) {
    dragHandleIndex_ = handles_.size() - 1;
  }

  if (emitValueChangedSignal) {
    emitChangedSignalsForCurrentMode();
    if (fromUserAction) {
      dragChanged_ = true;
    }
  }

  update();
}

void AdSlider::emitChangedSignalsForCurrentMode() {
  if (mode_ == Mode::Single) {
    emit valueChanged(value());
    return;
  }
  emit valuesChanged(handles_);
}

void AdSlider::emitCompletedSignalsForCurrentMode() {
  if (mode_ == Mode::Single) {
    emit valueChangeCompleted(value());
    return;
  }
  emit valuesChangeCompleted(handles_);
}

void AdSlider::refreshAfterPropertyChange(bool updateGeometryHint) {
  if (updateGeometryHint) {
    updateGeometry();
  }
  update();
}

AdSlider::LayoutInfo AdSlider::buildLayout() const {
  LayoutInfo layout;

  detail::SliderStyleInput input;
  input.mode = mode_;
  input.orientation = orientation_;
  input.hovered = hovered_;
  input.dragging = dragging_;
  input.focused = hasFocus() && focusVisible_;
  input.disabled = disabled();
  input.reverse = reverse_;
  input.baseFont = font();
  input.componentTokens = componentTokens_;
  input.semanticStyles = resolvedSemanticStyles();
  layout.style = detail::resolveSliderVisualStyle(input);

  const QFontMetrics markMetrics(layout.style.metrics.font);
  const bool hasMarks = !marks_.isEmpty();
  const int markSpan = hasMarks ? layout.style.metrics.markGap + markMetrics.height() : 0;

  layout.vertical = orientation_ == Qt::Vertical;
  if (!layout.vertical) {
    const qreal left = layout.style.metrics.marginMain;
    const qreal right = width() - layout.style.metrics.marginMain;
    const qreal top = layout.style.metrics.marginCross;
    const qreal bottom = height() - layout.style.metrics.marginCross - markSpan;
    layout.contentRect = QRectF(left, top, std::max<qreal>(1.0, right - left),
                                std::max<qreal>(1.0, bottom - top));
    layout.crossCenter = layout.contentRect.center().y();
    layout.railRect =
        QRectF(layout.contentRect.left(),
               layout.crossCenter - layout.style.metrics.railSize / 2.0,
               layout.contentRect.width(),
               layout.style.metrics.railSize);
    layout.axisStart = layout.railRect.left();
    layout.axisLength = std::max<qreal>(1.0, layout.railRect.width());
  } else {
    const qreal left = layout.style.metrics.marginCross;
    const qreal right = width() - layout.style.metrics.marginCross - markSpan;
    const qreal top = layout.style.metrics.marginMain;
    const qreal bottom = height() - layout.style.metrics.marginMain;
    layout.contentRect = QRectF(left, top, std::max<qreal>(1.0, right - left),
                                std::max<qreal>(1.0, bottom - top));
    layout.crossCenter = layout.contentRect.center().x();
    layout.railRect =
        QRectF(layout.crossCenter - layout.style.metrics.railSize / 2.0,
               layout.contentRect.top(),
               layout.style.metrics.railSize,
               layout.contentRect.height());
    layout.axisStart = layout.railRect.top();
    layout.axisLength = std::max<qreal>(1.0, layout.railRect.height());
  }

  const int activeHandleSize =
      disabled() ? layout.style.metrics.handleSize : layout.style.metrics.handleSizeHover;
  layout.activeHandleSize = activeHandleSize;
  layout.normalHandleSize = layout.style.metrics.handleSize;
  layout.markLabelOffset = layout.style.metrics.markGap;

  layout.handleRects.reserve(handles_.size());
  for (int i = 0; i < handles_.size(); ++i) {
    const bool active = (i == dragHandleIndex_) || (i == hoverHandleIndex_) ||
                        ((hasFocus() && focusVisible_) && i == focusHandleIndex_);
    const int handleSize = active ? activeHandleSize : layout.style.metrics.handleSize;
    const int half = handleSize / 2;
    const int axisPos = positionFromValue(handles_.at(i), layout);
    if (!layout.vertical) {
      layout.handleRects.append(
          QRectF(axisPos - half, layout.crossCenter - half, handleSize, handleSize));
    } else {
      layout.handleRects.append(
          QRectF(layout.crossCenter - half, axisPos - half, handleSize, handleSize));
    }
  }

  const MarkMap marks = effectiveMarks();
  layout.markCenters.reserve(marks.size());
  layout.markValues.reserve(marks.size());
  for (auto it = marks.cbegin(); it != marks.cend(); ++it) {
    const double markValue = clampValue(it.key(), minimum_, maximum_);
    const int axisPos = positionFromValue(markValue, layout);
    if (!layout.vertical) {
      layout.markCenters.append(QPointF(axisPos, layout.crossCenter));
    } else {
      layout.markCenters.append(QPointF(layout.crossCenter, axisPos));
    }
    layout.markValues.append(markValue);
  }

  return layout;
}

int AdSlider::hitTestHandle(const QPoint& pos, const LayoutInfo& layout) const {
  for (int i = layout.handleRects.size() - 1; i >= 0; --i) {
    const QRectF hitRect = layout.handleRects.at(i).adjusted(-2, -2, 2, 2);
    if (hitRect.contains(QPointF(pos))) {
      return i;
    }
  }
  return -1;
}

int AdSlider::nearestHandleIndex(double value) const {
  if (handles_.isEmpty()) {
    return -1;
  }
  int bestIndex = 0;
  double bestDistance = std::abs(handles_.constFirst() - value);
  for (int i = 1; i < handles_.size(); ++i) {
    const double distance = std::abs(handles_.at(i) - value);
    if (distance < bestDistance) {
      bestDistance = distance;
      bestIndex = i;
    }
  }
  return bestIndex;
}

double AdSlider::valueFromPosition(const QPoint& pos, const LayoutInfo& layout) const {
  if (maximum_ <= minimum_) {
    return minimum_;
  }

  double ratio = 0.0;
  if (!layout.vertical) {
    ratio = (pos.x() - layout.axisStart) / layout.axisLength;
    ratio = std::clamp(ratio, 0.0, 1.0);
    if (reverse_) {
      ratio = 1.0 - ratio;
    }
  } else {
    ratio = 1.0 - ((pos.y() - layout.axisStart) / layout.axisLength);
    ratio = std::clamp(ratio, 0.0, 1.0);
    if (reverse_) {
      ratio = 1.0 - ratio;
    }
  }
  const double raw = minimum_ + ratio * (maximum_ - minimum_);
  return normalizeValue(raw);
}

int AdSlider::positionFromValue(double value, const LayoutInfo& layout) const {
  if (maximum_ <= minimum_) {
    return !layout.vertical ? qRound(layout.axisStart)
                            : qRound(layout.axisStart + layout.axisLength);
  }

  double ratio = (value - minimum_) / (maximum_ - minimum_);
  ratio = std::clamp(ratio, 0.0, 1.0);
  if (reverse_) {
    ratio = 1.0 - ratio;
  }

  if (!layout.vertical) {
    return qRound(layout.axisStart + ratio * layout.axisLength);
  }
  return qRound(layout.axisStart + (1.0 - ratio) * layout.axisLength);
}

double AdSlider::clampTrackDelta(double delta) const {
  if (dragStartValues_.isEmpty()) {
    return 0.0;
  }
  double minValue = std::numeric_limits<double>::max();
  double maxValue = std::numeric_limits<double>::lowest();
  for (double value : dragStartValues_) {
    minValue = std::min(minValue, value);
    maxValue = std::max(maxValue, value);
  }

  const double minDelta = minimum_ - minValue;
  const double maxDelta = maximum_ - maxValue;
  return std::clamp(delta, minDelta, maxDelta);
}

void AdSlider::handleRailAction(const QPoint& pos, const LayoutInfo& layout) {
  const double nextValue = valueFromPosition(pos, layout);
  if (mode_ == Mode::Single) {
    const double previous = value();
    setHandlesInternal({nextValue}, true, true);
    focusHandleIndex_ = 0;
    if (!fuzzyEq(previous, value())) {
      emitCompletedSignalsForCurrentMode();
    }
    return;
  }

  if (editableHandles_) {
    int insertedIndex = -1;
    if (addHandleAt(nextValue, &insertedIndex)) {
      focusHandleIndex_ = insertedIndex;
      emitCompletedSignalsForCurrentMode();
      return;
    }
  }

  const int index = nearestHandleIndex(nextValue);
  if (index < 0) {
    return;
  }

  QList<double> nextValues = handles_;
  nextValues[index] = nextValue;
  const QList<double> before = handles_;
  setHandlesInternal(nextValues, true, true);
  if (!listFuzzyEquals(before, handles_)) {
    focusHandleIndex_ = nearestHandleIndex(nextValue);
    emitCompletedSignalsForCurrentMode();
  }
}

bool AdSlider::deleteHandleAt(int index) {
  if (mode_ != Mode::Range || index < 0 || index >= handles_.size()) {
    return false;
  }
  const int requiredMinCount = editableHandles_ ? std::max(1, minHandleCount_) : 2;
  if (handles_.size() <= requiredMinCount) {
    return false;
  }

  QList<double> next = handles_;
  next.removeAt(index);
  const QList<double> before = handles_;
  setHandlesInternal(next, true, true);
  if (listFuzzyEquals(before, handles_)) {
    return false;
  }

  const int maxIndex = static_cast<int>(handles_.size()) - 1;
  focusHandleIndex_ = std::clamp(index, 0, maxIndex);
  return true;
}

bool AdSlider::addHandleAt(double value, int* insertedIndex) {
  if (mode_ != Mode::Range) {
    return false;
  }
  if (maxHandleCount_ > 0 && handles_.size() >= maxHandleCount_) {
    return false;
  }

  QList<double> next = handles_;
  next.append(normalizeValue(value));
  std::sort(next.begin(), next.end());

  const QList<double> before = handles_;
  setHandlesInternal(next, true, true);
  if (listFuzzyEquals(before, handles_)) {
    return false;
  }

  const int index = nearestHandleIndex(value);
  if (insertedIndex) {
    *insertedIndex = index;
  }
  return true;
}

QList<int> AdSlider::tooltipHandleIndexes() const {
  QList<int> indexes;
  if (!tooltipEnabled_ || handles_.isEmpty()) {
    return indexes;
  }

  if (tooltipVisibleMode_ == TooltipVisibleMode::Never) {
    return indexes;
  }

  if (tooltipVisibleMode_ == TooltipVisibleMode::Always) {
    indexes.reserve(handles_.size());
    for (int i = 0; i < handles_.size(); ++i) {
      indexes.append(i);
    }
    return indexes;
  }

  const int primary =
      dragHandleIndex_ >= 0
          ? dragHandleIndex_
          : (hoverHandleIndex_ >= 0 ? hoverHandleIndex_
                                    : ((hasFocus() && focusVisible_) ? focusHandleIndex_ : -1));
  if (primary >= 0 && primary < handles_.size()) {
    indexes.append(primary);
  }
  return indexes;
}

QString AdSlider::tooltipText(double value) const {
  if (tooltipFormatter_) {
    return tooltipFormatter_(value);
  }
  return formatNumber(value);
}

void AdSlider::paintEvent(QPaintEvent* event) {
  Q_UNUSED(event)

  const LayoutInfo layout = buildLayout();

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.setFont(layout.style.metrics.font);

  if (layout.style.rootBg.isValid() && layout.style.rootBg.alpha() > 0) {
    painter.fillRect(rect(), layout.style.rootBg);
  }

  const bool isDisabled = disabled();
  const QColor railColor = (!isDisabled && hovered_) ? layout.style.railHoverBg : layout.style.railBg;
  const QColor trackColor =
      isDisabled ? layout.style.trackBgDisabled
                 : ((hovered_ || dragging_) ? layout.style.trackHoverBg : layout.style.trackBg);

  painter.setPen(Qt::NoPen);
  painter.setBrush(railColor);
  painter.drawRoundedRect(layout.railRect, layout.style.metrics.railSize / 2.0,
                          layout.style.metrics.railSize / 2.0);

  auto drawTrackSegment = [&](double fromValue, double toValue) {
    const int fromPos = positionFromValue(fromValue, layout);
    const int toPos = positionFromValue(toValue, layout);
    if (!layout.vertical) {
      const qreal left = std::min(fromPos, toPos);
      const qreal width =
          std::max<qreal>(1.0, static_cast<qreal>(std::abs(toPos - fromPos)));
      const QRectF segment(left, layout.crossCenter - layout.style.metrics.railSize / 2.0, width,
                           layout.style.metrics.railSize);
      painter.setBrush(layout.style.useTracksBrush ? layout.style.tracksBrush : QBrush(trackColor));
      painter.drawRoundedRect(segment, layout.style.metrics.railSize / 2.0,
                              layout.style.metrics.railSize / 2.0);
    } else {
      const qreal top = std::min(fromPos, toPos);
      const qreal height =
          std::max<qreal>(1.0, static_cast<qreal>(std::abs(toPos - fromPos)));
      const QRectF segment(layout.crossCenter - layout.style.metrics.railSize / 2.0, top,
                           layout.style.metrics.railSize, height);
      painter.setBrush(layout.style.useTracksBrush ? layout.style.tracksBrush : QBrush(trackColor));
      painter.drawRoundedRect(segment, layout.style.metrics.railSize / 2.0,
                              layout.style.metrics.railSize / 2.0);
    }
  };

  if (mode_ == Mode::Single) {
    if (included_ && !handles_.isEmpty()) {
      drawTrackSegment(minimum_, handles_.constFirst());
    }
  } else if (handles_.size() >= 2) {
    for (int i = 0; i + 1 < handles_.size(); ++i) {
      drawTrackSegment(handles_.at(i), handles_.at(i + 1));
    }
  }

  const MarkMap marks = effectiveMarks();
  if (!marks.isEmpty()) {
    const QFontMetrics fm(layout.style.metrics.font);
    int markIndex = 0;
    for (auto it = marks.cbegin(); it != marks.cend(); ++it, ++markIndex) {
      if (markIndex < 0 || markIndex >= layout.markCenters.size() ||
          markIndex >= layout.markValues.size()) {
        continue;
      }

      const QPointF center = layout.markCenters.at(markIndex);
      const double markValue = layout.markValues.at(markIndex);
      const bool active = [this, markValue]() {
        if (handles_.isEmpty()) {
          return false;
        }
        if (mode_ == Mode::Single) {
          if (!included_) {
            return fuzzyEq(markValue, handles_.constFirst());
          }
          return markValue <= handles_.constFirst() + kEpsilon;
        }
        const double low = handles_.constFirst();
        const double high = handles_.constLast();
        return markValue >= low - kEpsilon && markValue <= high + kEpsilon;
      }();

      QColor dotBorder = active ? layout.style.dotActiveBorderColor : layout.style.dotBorderColor;
      if (it->color.has_value()) {
        dotBorder = it->color.value();
      }

      const int dotSize = layout.style.metrics.dotSize;
      const QRectF dotRect(center.x() - dotSize / 2.0, center.y() - dotSize / 2.0, dotSize, dotSize);
      painter.setBrush(palette().base());
      painter.setPen(QPen(dotBorder, std::max(1, layout.style.metrics.handleLineWidth)));
      painter.drawEllipse(dotRect);

      QFont markFont = layout.style.metrics.font;
      if (it->font.has_value()) {
        markFont = it->font.value();
      }
      painter.setFont(markFont);
      QColor textColor = active ? layout.style.markActiveColor : layout.style.markColor;
      if (it->color.has_value()) {
        textColor = it->color.value();
      }
      painter.setPen(textColor);

      const QString label = it->label.isEmpty() ? formatNumber(it.key()) : it->label;
      if (!layout.vertical) {
        const int textWidth = QFontMetrics(markFont).horizontalAdvance(label);
        const QPointF textPos(center.x() - textWidth / 2.0,
                              center.y() + layout.markLabelOffset + fm.ascent());
        painter.drawText(textPos, label);
      } else {
        const QPointF textPos(center.x() + layout.markLabelOffset, center.y() + fm.ascent() / 2.0);
        painter.drawText(textPos, label);
      }
    }
    painter.setFont(layout.style.metrics.font);
  }

  for (int i = 0; i < layout.handleRects.size(); ++i) {
    const QRectF handleRect = layout.handleRects.at(i);
    const bool active = i == hoverHandleIndex_ || i == dragHandleIndex_ ||
                        ((hasFocus() && focusVisible_) && i == focusHandleIndex_);
    QColor borderColor = isDisabled ? layout.style.handleColorDisabled
                                    : (active ? layout.style.handleActiveColor : layout.style.handleColor);
    QColor outlineColor = isDisabled ? QColor(0, 0, 0, 0) : layout.style.handleActiveOutlineColor;

    if (active && outlineColor.isValid() && outlineColor.alpha() > 0) {
      painter.setPen(Qt::NoPen);
      painter.setBrush(outlineColor);
      const qreal expand = layout.style.metrics.focusOutlineSize / 2.0;
      painter.drawEllipse(handleRect.adjusted(-expand, -expand, expand, expand));
    }

    painter.setBrush(palette().base());
    painter.setPen(QPen(borderColor, std::max(1, layout.style.metrics.handleLineWidth)));
    painter.drawEllipse(handleRect);
  }

  const QList<int> tooltipIndexes = tooltipHandleIndexes();
  if (tooltipIndexes.isEmpty()) {
    return;
  }

  const QFontMetrics fm(layout.style.metrics.font);
  auto drawTooltipAt = [&](int handleIndex) {
    if (handleIndex < 0 || handleIndex >= layout.handleRects.size() || handleIndex >= handles_.size()) {
      return;
    }

    const QString text = tooltipText(handles_.at(handleIndex));
    if (text.isEmpty()) {
      return;
    }

    const QRectF handleRect = layout.handleRects.at(handleIndex);
    const int textWidth = fm.horizontalAdvance(text);
    const int bubbleW = textWidth + layout.style.metrics.tooltipPaddingH * 2;
    const int bubbleH = fm.height() + layout.style.metrics.tooltipPaddingV * 2;
    const int arrowSize = layout.style.metrics.tooltipArrowSize;
    const int offset = layout.style.metrics.tooltipOffset;

    QRectF bubbleRect;
    QPolygonF arrow;

    if (tooltipPlacement_ == TooltipPlacement::Top) {
      bubbleRect = QRectF(handleRect.center().x() - bubbleW / 2.0,
                          handleRect.top() - offset - bubbleH - arrowSize, bubbleW, bubbleH);
      arrow << QPointF(handleRect.center().x(), bubbleRect.bottom() + arrowSize)
            << QPointF(handleRect.center().x() - arrowSize, bubbleRect.bottom())
            << QPointF(handleRect.center().x() + arrowSize, bubbleRect.bottom());
    } else if (tooltipPlacement_ == TooltipPlacement::Bottom) {
      bubbleRect =
          QRectF(handleRect.center().x() - bubbleW / 2.0, handleRect.bottom() + offset + arrowSize, bubbleW,
                 bubbleH);
      arrow << QPointF(handleRect.center().x(), bubbleRect.top() - arrowSize)
            << QPointF(handleRect.center().x() - arrowSize, bubbleRect.top())
            << QPointF(handleRect.center().x() + arrowSize, bubbleRect.top());
    } else if (tooltipPlacement_ == TooltipPlacement::Left) {
      bubbleRect =
          QRectF(handleRect.left() - offset - bubbleW - arrowSize, handleRect.center().y() - bubbleH / 2.0,
                 bubbleW, bubbleH);
      arrow << QPointF(bubbleRect.right() + arrowSize, handleRect.center().y())
            << QPointF(bubbleRect.right(), handleRect.center().y() - arrowSize)
            << QPointF(bubbleRect.right(), handleRect.center().y() + arrowSize);
    } else {
      bubbleRect = QRectF(handleRect.right() + offset + arrowSize, handleRect.center().y() - bubbleH / 2.0,
                          bubbleW, bubbleH);
      arrow << QPointF(bubbleRect.left() - arrowSize, handleRect.center().y())
            << QPointF(bubbleRect.left(), handleRect.center().y() - arrowSize)
            << QPointF(bubbleRect.left(), handleRect.center().y() + arrowSize);
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(layout.style.tooltipBg);
    painter.drawRoundedRect(bubbleRect, layout.style.metrics.tooltipRadius, layout.style.metrics.tooltipRadius);
    painter.drawPolygon(arrow);
    painter.setPen(layout.style.tooltipText);
    painter.drawText(bubbleRect, Qt::AlignCenter, text);
  };

  for (int index : tooltipIndexes) {
    drawTooltipAt(index);
  }
}

void AdSlider::enterEvent(QEnterEvent* event) {
  hovered_ = true;
  update();
  QWidget::enterEvent(event);
}

void AdSlider::leaveEvent(QEvent* event) {
  hovered_ = false;
  if (dragMode_ == DragMode::None) {
    hoverHandleIndex_ = -1;
  }
  update();
  QWidget::leaveEvent(event);
}

void AdSlider::mousePressEvent(QMouseEvent* event) {
  QWidget::mousePressEvent(event);
  if (!event || event->button() != Qt::LeftButton || disabled()) {
    return;
  }

  setFocus(Qt::MouseFocusReason);
  pressValuesSnapshot_ = handles_;
  dragChanged_ = false;

  const LayoutInfo layout = buildLayout();
  const int hitIndex = hitTestHandle(event->position().toPoint(), layout);
  if (hitIndex >= 0) {
    dragMode_ = DragMode::Handle;
    dragHandleIndex_ = hitIndex;
    focusHandleIndex_ = hitIndex;
    dragStartPos_ = event->position().toPoint();
    dragStartValues_ = handles_;
    dragging_ = true;
    update();
    return;
  }

  if (mode_ == Mode::Range && draggableTrack_ && handles_.size() >= 2) {
    const int p1 = positionFromValue(handles_.constFirst(), layout);
    const int p2 = positionFromValue(handles_.constLast(), layout);
    const int trackHalf = std::max(layout.style.metrics.railSize, layout.activeHandleSize / 2);
    QRectF trackRect;
    if (!layout.vertical) {
      trackRect = QRectF(std::min(p1, p2), layout.crossCenter - trackHalf, std::abs(p2 - p1), trackHalf * 2.0);
    } else {
      trackRect = QRectF(layout.crossCenter - trackHalf, std::min(p1, p2), trackHalf * 2.0, std::abs(p2 - p1));
    }
    if (trackRect.adjusted(-2, -2, 2, 2).contains(event->position())) {
      dragMode_ = DragMode::Track;
      dragHandleIndex_ = -1;
      dragStartPos_ = event->position().toPoint();
      dragStartValues_ = handles_;
      dragging_ = true;
      update();
      return;
    }
  }

  handleRailAction(event->position().toPoint(), layout);
}

void AdSlider::mouseMoveEvent(QMouseEvent* event) {
  QWidget::mouseMoveEvent(event);
  if (!event || disabled()) {
    return;
  }

  const LayoutInfo layout = buildLayout();
  const int hitIndex = hitTestHandle(event->position().toPoint(), layout);
  if (hoverHandleIndex_ != hitIndex && dragMode_ == DragMode::None) {
    hoverHandleIndex_ = hitIndex;
    update();
  }

  if (dragMode_ == DragMode::None) {
    return;
  }

  if (dragMode_ == DragMode::Handle && dragHandleIndex_ >= 0 && dragHandleIndex_ < handles_.size()) {
    QList<double> next = dragStartValues_;
    if (dragHandleIndex_ >= next.size()) {
      return;
    }
    const double nextValue = valueFromPosition(event->position().toPoint(), layout);
    next[dragHandleIndex_] = nextValue;
    setHandlesInternal(next, true, true);
    focusHandleIndex_ = nearestHandleIndex(nextValue);
    return;
  }

  if (dragMode_ == DragMode::Track && !dragStartValues_.isEmpty()) {
    double pixelDelta = 0.0;
    if (!layout.vertical) {
      pixelDelta = event->position().x() - dragStartPos_.x();
    } else {
      pixelDelta = dragStartPos_.y() - event->position().y();
    }
    if (reverse_) {
      pixelDelta *= -1.0;
    }

    const double valueDelta = clampTrackDelta((pixelDelta / layout.axisLength) * (maximum_ - minimum_));
    QList<double> next = dragStartValues_;
    for (double& value : next) {
      value = normalizeValue(value + valueDelta);
    }
    setHandlesInternal(next, true, true);
  }
}

void AdSlider::mouseReleaseEvent(QMouseEvent* event) {
  QWidget::mouseReleaseEvent(event);
  if (!event || event->button() != Qt::LeftButton) {
    return;
  }

  const bool hadDrag = dragMode_ != DragMode::None;
  dragMode_ = DragMode::None;
  dragging_ = false;
  dragStartValues_.clear();
  dragHandleIndex_ = -1;

  if (hadDrag && dragChanged_) {
    emitCompletedSignalsForCurrentMode();
  }

  dragChanged_ = false;
  if (!rect().contains(event->position().toPoint())) {
    hoverHandleIndex_ = -1;
  }
  update();
}

void AdSlider::keyPressEvent(QKeyEvent* event) {
  if (!event) {
    QWidget::keyPressEvent(event);
    return;
  }

  if (disabled() || !keyboardEnabled_) {
    QWidget::keyPressEvent(event);
    return;
  }

  const int key = event->key();
  if (mode_ == Mode::Range && editableHandles_ &&
      (key == Qt::Key_Delete || key == Qt::Key_Backspace)) {
    if (focusHandleIndex_ < 0 || focusHandleIndex_ >= handles_.size()) {
      focusHandleIndex_ = handles_.isEmpty() ? -1 : handles_.size() - 1;
    }
    if (deleteHandleAt(focusHandleIndex_)) {
      emitCompletedSignalsForCurrentMode();
      event->accept();
      return;
    }
  }

  if (handles_.isEmpty()) {
    QWidget::keyPressEvent(event);
    return;
  }

  int index = focusHandleIndex_;
  if (index < 0 || index >= handles_.size()) {
    index = 0;
  }
  const double stepValue = step_ > 0.0 ? step_ : std::max(1.0, (maximum_ - minimum_) / 100.0);
  double nextValue = handles_.at(index);
  bool handled = true;

  switch (key) {
    case Qt::Key_Left:
    case Qt::Key_Down:
      nextValue -= stepValue;
      break;
    case Qt::Key_Right:
    case Qt::Key_Up:
      nextValue += stepValue;
      break;
    case Qt::Key_Home:
      nextValue = minimum_;
      break;
    case Qt::Key_End:
      nextValue = maximum_;
      break;
    default:
      handled = false;
      break;
  }

  if (!handled) {
    QWidget::keyPressEvent(event);
    return;
  }

  nextValue = normalizeValue(nextValue);
  QList<double> next = handles_;
  next[index] = nextValue;
  const QList<double> before = handles_;
  setHandlesInternal(next, true, true);
  focusHandleIndex_ = nearestHandleIndex(nextValue);
  if (!listFuzzyEquals(before, handles_)) {
    dragChanged_ = true;
  }
  event->accept();
}

void AdSlider::keyReleaseEvent(QKeyEvent* event) {
  QWidget::keyReleaseEvent(event);
  if (!event || disabled() || !keyboardEnabled_) {
    return;
  }

  switch (event->key()) {
    case Qt::Key_Left:
    case Qt::Key_Right:
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_Home:
    case Qt::Key_End:
      if (dragChanged_) {
        emitCompletedSignalsForCurrentMode();
        dragChanged_ = false;
      }
      break;
    default:
      break;
  }
}

void AdSlider::focusInEvent(QFocusEvent* event) {
  focusVisible_ = event && isKeyboardFocusReason(event->reason());
  if (focusHandleIndex_ < 0 && !handles_.isEmpty()) {
    focusHandleIndex_ = 0;
  }
  QWidget::focusInEvent(event);
  update();
}

void AdSlider::focusOutEvent(QFocusEvent* event) {
  focusVisible_ = false;
  QWidget::focusOutEvent(event);
  update();
}

void AdSlider::changeEvent(QEvent* event) {
  QWidget::changeEvent(event);
  if (!event) {
    return;
  }

  if (event->type() == QEvent::EnabledChange) {
    hovered_ = false;
    hoverHandleIndex_ = -1;
    dragMode_ = DragMode::None;
    dragging_ = false;
    dragHandleIndex_ = -1;
    update();
  } else if (event->type() == QEvent::FontChange) {
    refreshAfterPropertyChange();
  }
}

}  // namespace adqt::widgets
