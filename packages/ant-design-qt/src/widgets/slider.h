#pragma once

#include <QBrush>
#include <QColor>
#include <QMap>
#include <QList>
#include <QString>
#include <QWidget>

#include <functional>
#include <optional>

namespace adqt::widgets {

class AdSlider final : public QWidget {
  Q_OBJECT

  Q_PROPERTY(Mode mode READ mode WRITE setMode NOTIFY modeChanged)
  Q_PROPERTY(double minimum READ minimum WRITE setMinimum NOTIFY minimumChanged)
  Q_PROPERTY(double maximum READ maximum WRITE setMaximum NOTIFY maximumChanged)
  Q_PROPERTY(double step READ step WRITE setStep NOTIFY stepChanged)
  Q_PROPERTY(bool marksOnly READ marksOnly WRITE setMarksOnly NOTIFY marksOnlyChanged)
  Q_PROPERTY(bool dots READ dots WRITE setDots NOTIFY dotsChanged)
  Q_PROPERTY(bool included READ included WRITE setIncluded NOTIFY includedChanged)
  Q_PROPERTY(bool reverse READ reverse WRITE setReverse NOTIFY reverseChanged)
  Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation NOTIFY orientationChanged)
  Q_PROPERTY(bool disabled READ disabled WRITE setDisabled NOTIFY disabledChanged)
  Q_PROPERTY(bool keyboardEnabled READ keyboardEnabled WRITE setKeyboardEnabled NOTIFY keyboardEnabledChanged)
  Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged)
  Q_PROPERTY(QList<double> values READ values WRITE setValues NOTIFY valuesChanged)
  Q_PROPERTY(bool draggableTrack READ draggableTrack WRITE setDraggableTrack NOTIFY draggableTrackChanged)
  Q_PROPERTY(bool editableHandles READ editableHandles WRITE setEditableHandles NOTIFY editableHandlesChanged)
  Q_PROPERTY(int minHandleCount READ minHandleCount WRITE setMinHandleCount NOTIFY minHandleCountChanged)
  Q_PROPERTY(int maxHandleCount READ maxHandleCount WRITE setMaxHandleCount NOTIFY maxHandleCountChanged)
  Q_PROPERTY(bool tooltipEnabled READ tooltipEnabled WRITE setTooltipEnabled NOTIFY tooltipEnabledChanged)
  Q_PROPERTY(TooltipVisibleMode tooltipVisibleMode READ tooltipVisibleMode WRITE setTooltipVisibleMode
                 NOTIFY tooltipVisibleModeChanged)
  Q_PROPERTY(TooltipPlacement tooltipPlacement READ tooltipPlacement WRITE setTooltipPlacement
                 NOTIFY tooltipPlacementChanged)

 public:
  enum class Mode {
    Single,
    Range,
  };
  Q_ENUM(Mode)

  enum class TooltipVisibleMode {
    Auto,
    Always,
    Never,
  };
  Q_ENUM(TooltipVisibleMode)

  enum class TooltipPlacement {
    Top,
    Bottom,
    Left,
    Right,
  };
  Q_ENUM(TooltipPlacement)

  struct Mark {
    QString label;
    std::optional<QColor> color;
    std::optional<QFont> font;

    bool operator==(const Mark& other) const {
      return label == other.label && color == other.color && font == other.font;
    }
  };
  using MarkMap = QMap<double, Mark>;

  struct ComponentTokens {
    std::optional<int> controlSize;
    std::optional<int> railSize;
    std::optional<int> handleSize;
    std::optional<int> handleSizeHover;
    std::optional<int> handleLineWidth;
    std::optional<int> handleLineWidthHover;
    std::optional<int> dotSize;
    std::optional<QString> railBg;
    std::optional<QString> railHoverBg;
    std::optional<QString> trackBg;
    std::optional<QString> trackHoverBg;
    std::optional<QString> handleColor;
    std::optional<QString> handleActiveColor;
    std::optional<QString> handleActiveOutlineColor;
    std::optional<QString> handleColorDisabled;
    std::optional<QString> dotBorderColor;
    std::optional<QString> dotActiveBorderColor;
    std::optional<QString> trackBgDisabled;
  };

  struct SemanticSlotStyle {
    std::optional<QColor> textColor;
    std::optional<QColor> backgroundColor;
    std::optional<QColor> borderColor;
    std::optional<QBrush> brush;
  };

  struct SemanticStyles {
    SemanticSlotStyle root;
    SemanticSlotStyle rail;
    SemanticSlotStyle track;
    SemanticSlotStyle tracks;
    SemanticSlotStyle handle;
    SemanticSlotStyle mark;
    SemanticSlotStyle markActive;
  };

  struct StyleContext {
    Mode mode = Mode::Single;
    Qt::Orientation orientation = Qt::Horizontal;
    bool reverse = false;
    bool disabled = false;
    bool dragging = false;
    bool hovered = false;
    bool focused = false;
    QList<double> values;
  };

  using TooltipFormatter = std::function<QString(double value)>;
  using SemanticStyleResolver = std::function<SemanticStyles(const StyleContext&)>;

  explicit AdSlider(QWidget* parent = nullptr);
  ~AdSlider() override;

  Mode mode() const;
  void setMode(Mode value);

  double minimum() const;
  void setMinimum(double value);

  double maximum() const;
  void setMaximum(double value);

  double step() const;
  void setStep(double value);

  bool marksOnly() const;
  void setMarksOnly(bool value);

  bool dots() const;
  void setDots(bool value);

  bool included() const;
  void setIncluded(bool value);

  bool reverse() const;
  void setReverse(bool value);

  Qt::Orientation orientation() const;
  void setOrientation(Qt::Orientation value);

  bool disabled() const;
  void setDisabled(bool value);

  bool keyboardEnabled() const;
  void setKeyboardEnabled(bool value);

  double value() const;
  void setValue(double value);

  QList<double> values() const;
  void setValues(const QList<double>& values);

  bool draggableTrack() const;
  void setDraggableTrack(bool value);

  bool editableHandles() const;
  void setEditableHandles(bool value);

  int minHandleCount() const;
  void setMinHandleCount(int value);

  int maxHandleCount() const;
  void setMaxHandleCount(int value);

  bool tooltipEnabled() const;
  void setTooltipEnabled(bool value);

  TooltipVisibleMode tooltipVisibleMode() const;
  void setTooltipVisibleMode(TooltipVisibleMode value);

  TooltipPlacement tooltipPlacement() const;
  void setTooltipPlacement(TooltipPlacement value);

  MarkMap marks() const;
  void setMarks(const MarkMap& marks);
  void clearMarks();

  TooltipFormatter tooltipFormatter() const;
  void setTooltipFormatter(TooltipFormatter formatter);

  ComponentTokens componentTokens() const;
  void setComponentTokens(const ComponentTokens& tokens);
  void resetComponentTokens();

  SemanticStyles semanticStyles() const;
  void setSemanticStyles(const SemanticStyles& styles);
  void setSemanticStyleResolver(SemanticStyleResolver resolver);

  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;

 signals:
  void modeChanged(Mode value);
  void minimumChanged(double value);
  void maximumChanged(double value);
  void stepChanged(double value);
  void marksOnlyChanged(bool value);
  void dotsChanged(bool value);
  void includedChanged(bool value);
  void reverseChanged(bool value);
  void orientationChanged(Qt::Orientation value);
  void disabledChanged(bool value);
  void keyboardEnabledChanged(bool value);
  void valueChanged(double value);
  void valuesChanged(const QList<double>& values);
  void draggableTrackChanged(bool value);
  void editableHandlesChanged(bool value);
  void minHandleCountChanged(int value);
  void maxHandleCountChanged(int value);
  void tooltipEnabledChanged(bool value);
  void tooltipVisibleModeChanged(TooltipVisibleMode value);
  void tooltipPlacementChanged(TooltipPlacement value);
  void marksChanged();
  void componentTokensChanged();
  void semanticStylesChanged();
  void valueChangeCompleted(double value);
  void valuesChangeCompleted(const QList<double>& values);

 protected:
  void paintEvent(QPaintEvent* event) override;
  void enterEvent(QEnterEvent* event) override;
  void leaveEvent(QEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
  void keyReleaseEvent(QKeyEvent* event) override;
  void focusInEvent(QFocusEvent* event) override;
  void focusOutEvent(QFocusEvent* event) override;
  void changeEvent(QEvent* event) override;

 private:
  enum class DragMode {
    None,
    Handle,
    Track,
  };

  struct LayoutInfo;

  MarkMap effectiveMarks() const;
  SemanticStyles resolvedSemanticStyles() const;
  QList<double> normalizedValues(const QList<double>& values, bool forceRangeMode) const;
  QList<double> snapPoints() const;
  double normalizeValue(double value) const;
  void setHandlesInternal(const QList<double>& handles, bool emitValueChangedSignal, bool fromUserAction);
  void emitChangedSignalsForCurrentMode();
  void emitCompletedSignalsForCurrentMode();
  void refreshAfterPropertyChange(bool updateGeometryHint = true);

  LayoutInfo buildLayout() const;
  int hitTestHandle(const QPoint& pos, const LayoutInfo& layout) const;
  int nearestHandleIndex(double value) const;
  double valueFromPosition(const QPoint& pos, const LayoutInfo& layout) const;
  int positionFromValue(double value, const LayoutInfo& layout) const;
  double clampTrackDelta(double delta) const;
  void handleRailAction(const QPoint& pos, const LayoutInfo& layout);
  bool deleteHandleAt(int index);
  bool addHandleAt(double value, int* insertedIndex = nullptr);
  QList<int> tooltipHandleIndexes() const;
  QString tooltipText(double value) const;

  Mode mode_ = Mode::Single;
  double minimum_ = 0.0;
  double maximum_ = 100.0;
  double step_ = 1.0;
  bool marksOnly_ = false;
  bool dots_ = false;
  bool included_ = true;
  bool reverse_ = false;
  Qt::Orientation orientation_ = Qt::Horizontal;
  bool keyboardEnabled_ = true;
  bool draggableTrack_ = false;
  bool editableHandles_ = false;
  int minHandleCount_ = 0;
  int maxHandleCount_ = -1;
  bool tooltipEnabled_ = true;
  TooltipVisibleMode tooltipVisibleMode_ = TooltipVisibleMode::Auto;
  TooltipPlacement tooltipPlacement_ = TooltipPlacement::Top;

  QList<double> handles_;
  MarkMap marks_;
  TooltipFormatter tooltipFormatter_;
  ComponentTokens componentTokens_;
  SemanticStyles semanticStyles_;
  SemanticStyleResolver semanticStyleResolver_;

  bool hovered_ = false;
  bool focusVisible_ = false;
  bool dragging_ = false;
  bool dragChanged_ = false;
  DragMode dragMode_ = DragMode::None;
  int dragHandleIndex_ = -1;
  int hoverHandleIndex_ = -1;
  int focusHandleIndex_ = -1;
  QPoint dragStartPos_;
  QList<double> dragStartValues_;
  QList<double> pressValuesSnapshot_;
};

}  // namespace adqt::widgets

