#pragma once

#include <QColor>
#include <QPointer>
#include <QVector>
#include <QWidget>

#include <functional>
#include <optional>

#include "popover.h"

class QComboBox;
class QFrame;
class QGridLayout;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QPushButton;
class QScrollArea;
class QVBoxLayout;

namespace adqt::widgets {

class AdSlider;
class ColorSaturationPanel;

class AdColorPicker final : public QWidget {
  Q_OBJECT

  Q_PROPERTY(Size size READ size WRITE setSize NOTIFY sizeChanged)
  Q_PROPERTY(Mode mode READ mode WRITE setMode NOTIFY modeChanged)
  Q_PROPERTY(Format format READ format WRITE setFormat NOTIFY formatChanged)
  Q_PROPERTY(bool allowClear READ allowClear WRITE setAllowClear NOTIFY allowClearChanged)
  Q_PROPERTY(bool showText READ showText WRITE setShowText NOTIFY showTextChanged)
  Q_PROPERTY(bool open READ open WRITE setOpen NOTIFY openChanged)
  Q_PROPERTY(bool disabled READ disabled WRITE setDisabled NOTIFY disabledChanged)
  Q_PROPERTY(bool disabledAlpha READ disabledAlpha WRITE setDisabledAlpha NOTIFY disabledAlphaChanged)
  Q_PROPERTY(bool disabledFormat READ disabledFormat WRITE setDisabledFormat NOTIFY disabledFormatChanged)
  Q_PROPERTY(Trigger trigger READ trigger WRITE setTrigger NOTIFY triggerChanged)
  Q_PROPERTY(Placement placement READ placement WRITE setPlacement NOTIFY placementChanged)
  Q_PROPERTY(QString value READ value WRITE setValue NOTIFY valueChanged)

 public:
  enum class Size {
    Small,
    Middle,
    Large,
  };
  Q_ENUM(Size)

  enum class Mode {
    Single,
    Gradient,
  };
  Q_ENUM(Mode)

  enum class Format {
    Hex,
    Rgb,
    Hsb,
  };
  Q_ENUM(Format)

  enum class Trigger {
    Click,
    Hover,
  };
  Q_ENUM(Trigger)

  enum class Placement {
    Top,
    TopLeft,
    TopRight,
    Bottom,
    BottomLeft,
    BottomRight,
    Left,
    LeftTop,
    LeftBottom,
    Right,
    RightTop,
    RightBottom,
  };
  Q_ENUM(Placement)

  struct GradientStop {
    QString color;
    int percent = 0;
  };

  struct ColorValue {
    bool cleared = false;
    QString solidColor;
    QVector<GradientStop> gradientStops;
  };

  struct PresetItem {
    QString label;
    QVector<ColorValue> colors;
    bool defaultOpen = true;
    QString key;
  };

  struct ComponentTokens {
    std::optional<int> controlHeight;
    std::optional<int> triggerMinWidth;
    std::optional<int> triggerRadius;
    std::optional<int> panelWidth;
    std::optional<int> swatchSize;
    std::optional<int> presetSwatchSize;
    std::optional<int> panelPadding;
    std::optional<int> inputHeight;
    std::optional<QString> triggerBackground;
    std::optional<QString> triggerBorderColor;
    std::optional<QString> triggerBorderHoverColor;
    std::optional<QString> triggerTextColor;
    std::optional<QString> panelBackground;
    std::optional<QString> panelBorderColor;
  };

  struct SemanticSlotStyle {
    std::optional<QColor> textColor;
    std::optional<QColor> backgroundColor;
    std::optional<QColor> borderColor;
  };

  struct SemanticStyles {
    SemanticSlotStyle root;
    SemanticSlotStyle body;
    SemanticSlotStyle content;
    SemanticSlotStyle description;
    SemanticSlotStyle popup;
  };

  struct StyleContext {
    Mode mode = Mode::Single;
    Format format = Format::Hex;
    bool open = false;
    bool disabled = false;
    bool cleared = false;
  };

  using SemanticStyleResolver = std::function<SemanticStyles(const StyleContext&)>;
  using ShowTextFormatter = std::function<QString(const ColorValue&, Format, int activeIndex)>;
  using PanelRenderFactory =
      std::function<QWidget*(QWidget* parent, QWidget* pickerPanel, QWidget* presetsPanel)>;

  explicit AdColorPicker(QWidget* parent = nullptr);
  ~AdColorPicker() override;

  Size size() const;
  void setSize(Size value);

  Mode mode() const;
  void setMode(Mode value);

  QVector<Mode> modeOptions() const;
  void setModeOptions(const QVector<Mode>& options);

  Format format() const;
  void setFormat(Format value);

  bool allowClear() const;
  void setAllowClear(bool value);

  bool showText() const;
  void setShowText(bool value);

  bool open() const;
  void setOpen(bool value);

  bool disabled() const;
  void setDisabled(bool value);

  bool disabledAlpha() const;
  void setDisabledAlpha(bool value);

  bool disabledFormat() const;
  void setDisabledFormat(bool value);

  Trigger trigger() const;
  void setTrigger(Trigger value);

  Placement placement() const;
  void setPlacement(Placement value);

  QString value() const;
  void setValue(const QString& value);

  ColorValue colorValue() const;
  void setColorValue(const ColorValue& value);

  QVector<PresetItem> presets() const;
  void setPresets(const QVector<PresetItem>& presets);

  QWidget* triggerWidget() const;
  void setTriggerWidget(QWidget* widget);

  ShowTextFormatter showTextFormatter() const;
  void setShowTextFormatter(ShowTextFormatter formatter);

  PanelRenderFactory panelRenderFactory() const;
  void setPanelRenderFactory(PanelRenderFactory factory);

  ComponentTokens componentTokens() const;
  void setComponentTokens(const ComponentTokens& tokens);
  void resetComponentTokens();

  SemanticStyles semanticStyles() const;
  void setSemanticStyles(const SemanticStyles& styles);
  void setSemanticStyleResolver(SemanticStyleResolver resolver);

 signals:
  void sizeChanged(Size value);
  void modeChanged(Mode value);
  void modeOptionsChanged(const QVector<Mode>& options);
  void formatChanged(Format value);
  void allowClearChanged(bool value);
  void showTextChanged(bool value);
  void openChanged(bool value);
  void onOpenChange(bool value);
  void disabledChanged(bool value);
  void disabledAlphaChanged(bool value);
  void disabledFormatChanged(bool value);
  void triggerChanged(Trigger value);
  void placementChanged(Placement value);
  void valueChanged(const QString& value);
  void colorValueChanged(const AdColorPicker::ColorValue& value);
  void presetsChanged();
  void triggerWidgetChanged(QWidget* widget);
  void showTextFormatterChanged();
  void panelRenderFactoryChanged();
  void componentTokensChanged();
  void semanticStylesChanged();
  void cleared();
  void onClear();
  void changed(const AdColorPicker::ColorValue& value, const QString& css);
  void changeCompleted(const AdColorPicker::ColorValue& value);

 protected:
  void changeEvent(QEvent* event) override;

 private:
  struct InternalGradientStop {
    QColor color;
    int percent = 0;
  };

  static QString formatName(Format value);
  static Format parseFormatName(const QString& value, Format fallback);
  static AdPopover::Placement toPopoverPlacement(Placement value);
  static Placement fromPopoverPlacement(AdPopover::Placement value);
  static AdPopover::Triggers toPopoverTriggers(Trigger value);
  static Trigger fromPopoverTriggers(AdPopover::Triggers value, Trigger fallback);

  void ensureUi();
  void ensurePickerPanel();
  void rebuildPanelComposition();
  void rebuildPresetsPanel();
  void refreshStyle();
  void refreshTriggerDisplay();
  void refreshPanelControlsFromState();
  void refreshChannelVisuals();
  void refreshPreviewSwatch();
  void updateFormatInputText();
  void updateModeComboOptions();

  QColor currentEditableColor() const;
  void setCurrentEditableColor(const QColor& color,
                               bool fromUser,
                               bool emitCompleted,
                               bool emitValueSignal = true);
  void setCurrentFromControls(bool emitCompleted);
  void setGradientStopsFromSlider(const QList<double>& values, bool emitCompleted);
  QColor parseColorString(const QString& value, bool* ok) const;
  QString colorToString(const QColor& color, Format format) const;
  QString colorToCss(const QColor& color) const;
  QString colorValueToCss(const ColorValue& value) const;

  ColorValue exportColorValue() const;
  void importColorValue(const ColorValue& value,
                        bool fromUser,
                        bool emitCompleted,
                        bool emitValueSignal = true);
  QVector<InternalGradientStop> normalizeGradientStops(const QVector<InternalGradientStop>& stops) const;

  void emitChangeSignals(bool emitCompleted, bool emitValueSignal);
  void applyPreset(const ColorValue& value);

  Size size_ = Size::Middle;
  Mode mode_ = Mode::Single;
  QVector<Mode> modeOptions_ = {Mode::Single};
  Format format_ = Format::Hex;
  bool allowClear_ = false;
  bool showText_ = false;
  bool disabledAlpha_ = false;
  bool disabledFormat_ = false;
  bool cleared_ = false;

  QColor solidColor_ = QColor("#1677ff");
  QVector<InternalGradientStop> gradientStops_;
  int activeStopIndex_ = 0;

  QVector<PresetItem> presets_;
  ShowTextFormatter showTextFormatter_;
  PanelRenderFactory panelRenderFactory_;
  ComponentTokens componentTokens_;
  SemanticStyles semanticStyles_;
  SemanticStyleResolver semanticStyleResolver_;

  bool syncingControls_ = false;

  QPointer<AdPopover> popover_;
  QPointer<QWidget> defaultTrigger_;
  QPointer<QWidget> customTrigger_;
  QPointer<QWidget> panelHost_;
  QPointer<QWidget> pickerPanel_;
  QPointer<QWidget> presetsPanel_;
  QPointer<QWidget> composedPanel_;

  QPointer<QFrame> triggerFrame_;
  QPointer<QWidget> triggerSwatch_;
  QPointer<QLabel> triggerTextLabel_;

  QPointer<QWidget> operationRow_;
  QPointer<QComboBox> modeCombo_;
  QPointer<QPushButton> clearButton_;
  QPointer<QWidget> gradientSection_;
  QPointer<AdSlider> gradientSlider_;
  QPointer<ColorSaturationPanel> saturationPanel_;
  QPointer<QWidget> sliderContainer_;
  QPointer<QWidget> sliderGroup_;
  QPointer<QWidget> alphaSection_;
  QPointer<AdSlider> hueSlider_;
  QPointer<AdSlider> alphaSlider_;
  QPointer<QWidget> previewSwatch_;

  QPointer<QComboBox> formatCombo_;
  QPointer<QLineEdit> formatInput_;
  QPointer<QLineEdit> alphaInput_;

  QPointer<QVBoxLayout> presetsLayout_;
};

}  // namespace adqt::widgets

Q_DECLARE_METATYPE(adqt::widgets::AdColorPicker::GradientStop)
Q_DECLARE_METATYPE(adqt::widgets::AdColorPicker::ColorValue)
Q_DECLARE_METATYPE(adqt::widgets::AdColorPicker::Mode)
Q_DECLARE_METATYPE(adqt::widgets::AdColorPicker::Format)
