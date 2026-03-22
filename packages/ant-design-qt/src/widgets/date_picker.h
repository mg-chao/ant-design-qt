#pragma once

#include <QColor>
#include <QDate>
#include <QDateTime>
#include <QEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPointer>
#include <QResizeEvent>
#include <QString>
#include <QStringList>
#include <QTime>
#include <QVector>
#include <QWidget>

#include <functional>
#include <memory>
#include <optional>

#include "icon_core.h"

namespace adqt::widgets {

struct AdDisabledTimeSpec {
  QVector<int> disabledHours;
  QVector<int> disabledMinutes;
  QVector<int> disabledSeconds;

  bool operator==(const AdDisabledTimeSpec& other) const {
    return disabledHours == other.disabledHours &&
           disabledMinutes == other.disabledMinutes &&
           disabledSeconds == other.disabledSeconds;
  }

  bool operator!=(const AdDisabledTimeSpec& other) const { return !(*this == other); }
};

struct AdDateTimeRangeValue {
  QDateTime start;
  QDateTime end;

  bool isEmpty() const { return !start.isValid() && !end.isValid(); }

  bool operator==(const AdDateTimeRangeValue& other) const {
    return start == other.start && end == other.end;
  }

  bool operator!=(const AdDateTimeRangeValue& other) const { return !(*this == other); }
};

struct AdDatePresetItem {
  QString label;
  QDateTime value;

  bool operator==(const AdDatePresetItem& other) const {
    return label == other.label && value == other.value;
  }

  bool operator!=(const AdDatePresetItem& other) const { return !(*this == other); }
};

struct AdDateRangePresetItem {
  QString label;
  AdDateTimeRangeValue value;

  bool operator==(const AdDateRangePresetItem& other) const {
    return label == other.label && value == other.value;
  }

  bool operator!=(const AdDateRangePresetItem& other) const { return !(*this == other); }
};

struct AdDateTimePanelOptions {
  bool showHour = true;
  bool showMinute = true;
  bool showSecond = false;
  bool use12Hour = false;
  bool hideDisabledOptions = true;
  QTime defaultOpenTime = QTime(0, 0, 0);

  bool operator==(const AdDateTimePanelOptions& other) const {
    return showHour == other.showHour && showMinute == other.showMinute &&
           showSecond == other.showSecond && use12Hour == other.use12Hour &&
           hideDisabledOptions == other.hideDisabledOptions &&
           defaultOpenTime == other.defaultOpenTime;
  }

  bool operator!=(const AdDateTimePanelOptions& other) const { return !(*this == other); }
};

class AdDatePicker final : public QWidget {
  Q_OBJECT

  Q_PROPERTY(PickerMode pickerMode READ pickerMode WRITE setPickerMode NOTIFY pickerModeChanged)
  Q_PROPERTY(SelectionMode selectionMode READ selectionMode WRITE setSelectionMode
                 NOTIFY selectionModeChanged)
  Q_PROPERTY(ControlSize controlSize READ controlSize WRITE setControlSize NOTIFY controlSizeChanged)
  Q_PROPERTY(Variant variant READ variant WRITE setVariant NOTIFY variantChanged)
  Q_PROPERTY(Status status READ status WRITE setStatus NOTIFY statusChanged)
  Q_PROPERTY(bool allowClear READ allowClear WRITE setAllowClear NOTIFY allowClearChanged)
  Q_PROPERTY(bool popupVisible READ popupVisible WRITE setPopupVisible NOTIFY popupVisibleChanged)
  Q_PROPERTY(bool showTime READ showTime WRITE setShowTime NOTIFY showTimeChanged)
  Q_PROPERTY(bool needConfirm READ needConfirm WRITE setNeedConfirm NOTIFY needConfirmChanged)
  Q_PROPERTY(bool autoSortSelections READ autoSortSelections WRITE setAutoSortSelections
                 NOTIFY autoSortSelectionsChanged)
  Q_PROPERTY(bool disabled READ disabled WRITE setDisabled NOTIFY disabledChanged)
  Q_PROPERTY(Placement placement READ placement WRITE setPlacement NOTIFY placementChanged)
  Q_PROPERTY(QString placeholder READ placeholder WRITE setPlaceholder NOTIFY placeholderChanged)
  Q_PROPERTY(QString displayFormat READ displayFormat WRITE setDisplayFormat NOTIFY displayFormatChanged)
  Q_PROPERTY(QStringList acceptedInputFormats READ acceptedInputFormats WRITE setAcceptedInputFormats
                 NOTIFY acceptedInputFormatsChanged)
  Q_PROPERTY(int maxVisibleTags READ maxVisibleTags WRITE setMaxVisibleTags NOTIFY maxVisibleTagsChanged)
  Q_PROPERTY(bool responsiveMaxTagCount READ responsiveMaxTagCount WRITE setResponsiveMaxTagCount
                 NOTIFY responsiveMaxTagCountChanged)
  Q_PROPERTY(QDate minDate READ minDate WRITE setMinDate NOTIFY minDateChanged)
  Q_PROPERTY(QDate maxDate READ maxDate WRITE setMaxDate NOTIFY maxDateChanged)
  Q_PROPERTY(QString prefixText READ prefixText WRITE setPrefixText NOTIFY prefixTextChanged)
  Q_PROPERTY(QString suffixText READ suffixText WRITE setSuffixText NOTIFY suffixTextChanged)
  Q_PROPERTY(adqt::icons::IconRef prefixIconRef READ prefixIconRef WRITE setPrefixIconRef
                 NOTIFY prefixIconRefChanged)
  Q_PROPERTY(adqt::icons::IconRef suffixIconRef READ suffixIconRef WRITE setSuffixIconRef
                 NOTIFY suffixIconRefChanged)
  Q_PROPERTY(QString extraFooterText READ extraFooterText WRITE setExtraFooterText
                 NOTIFY extraFooterTextChanged)
  Q_PROPERTY(adqt::widgets::AdDateTimePanelOptions timePanelOptions READ timePanelOptions
                 WRITE setTimePanelOptions NOTIFY timePanelOptionsChanged)
  Q_PROPERTY(QDateTime value READ value WRITE setValue NOTIFY valueChanged)
  Q_PROPERTY(QVector<QDateTime> values READ values WRITE setValues NOTIFY valuesChanged)
  Q_PROPERTY(QVector<adqt::widgets::AdDatePresetItem> presets READ presets WRITE setPresets
                 NOTIFY presetsChanged)

 public:
  enum class PickerMode {
    Date,
    Week,
    Month,
    Quarter,
    Year,
  };
  Q_ENUM(PickerMode)

  enum class SelectionMode {
    Single,
    Multiple,
  };
  Q_ENUM(SelectionMode)

  enum class ControlSize {
    Large,
    Middle,
    Small,
  };
  Q_ENUM(ControlSize)

  enum class Variant {
    Outlined,
    Filled,
    Borderless,
    Underlined,
  };
  Q_ENUM(Variant)

  enum class Status {
    None,
    Error,
    Warning,
  };
  Q_ENUM(Status)

  enum class Placement {
    BottomLeft,
    BottomRight,
    TopLeft,
    TopRight,
  };
  Q_ENUM(Placement)

  enum class RangePart {
    Single,
    Start,
    End,
  };
  Q_ENUM(RangePart)

  struct ComponentTokens {
    std::optional<int> controlHeight;
    std::optional<int> borderRadius;
    std::optional<int> popupWidth;
    std::optional<int> popupPadding;
    std::optional<int> cellSize;
    std::optional<int> timeColumnWidth;
    std::optional<int> timeCellHeight;
    std::optional<int> presetRailWidth;
    std::optional<int> chipHeight;
    std::optional<QColor> selectorBackground;
    std::optional<QColor> selectorBorderColor;
    std::optional<QColor> selectorHoverBorderColor;
    std::optional<QColor> selectorActiveBorderColor;
    std::optional<QColor> popupBackground;
    std::optional<QColor> popupBorderColor;
    std::optional<QColor> cellHoverBackground;
    std::optional<QColor> cellRangeBackground;
    std::optional<QColor> cellSelectedBackground;
    std::optional<QColor> chipBackground;
    std::optional<QColor> chipBorderColor;
  };

  using DisabledDateEvaluator = std::function<bool(const QDate& date)>;
  using DisabledTimeEvaluator =
      std::function<AdDisabledTimeSpec(const QDate& date, RangePart part,
                                       const std::optional<QDate>& counterpartDate)>;

  explicit AdDatePicker(QWidget* parent = nullptr);
  ~AdDatePicker() override;

  PickerMode pickerMode() const;
  void setPickerMode(PickerMode value);

  SelectionMode selectionMode() const;
  void setSelectionMode(SelectionMode value);

  ControlSize controlSize() const;
  void setControlSize(ControlSize value);

  Variant variant() const;
  void setVariant(Variant value);

  Status status() const;
  void setStatus(Status value);

  bool allowClear() const;
  void setAllowClear(bool value);

  bool popupVisible() const;
  void setPopupVisible(bool value);

  bool showTime() const;
  void setShowTime(bool value);

  bool needConfirm() const;
  void setNeedConfirm(bool value);

  bool autoSortSelections() const;
  void setAutoSortSelections(bool value);

  bool disabled() const;
  void setDisabled(bool value);

  Placement placement() const;
  void setPlacement(Placement value);

  QString placeholder() const;
  void setPlaceholder(const QString& value);

  QString displayFormat() const;
  void setDisplayFormat(const QString& value);

  QStringList acceptedInputFormats() const;
  void setAcceptedInputFormats(const QStringList& value);

  int maxVisibleTags() const;
  void setMaxVisibleTags(int value);

  bool responsiveMaxTagCount() const;
  void setResponsiveMaxTagCount(bool value);

  QDate minDate() const;
  void setMinDate(const QDate& value);

  QDate maxDate() const;
  void setMaxDate(const QDate& value);

  QString prefixText() const;
  void setPrefixText(const QString& value);

  QString suffixText() const;
  void setSuffixText(const QString& value);

  adqt::icons::IconRef prefixIconRef() const;
  void setPrefixIconRef(const adqt::icons::IconRef& value);

  adqt::icons::IconRef suffixIconRef() const;
  void setSuffixIconRef(const adqt::icons::IconRef& value);

  QString extraFooterText() const;
  void setExtraFooterText(const QString& value);

  QWidget* extraFooterWidget() const;
  void setExtraFooterWidget(QWidget* widget);
  QWidget* takeExtraFooterWidget();

  AdDateTimePanelOptions timePanelOptions() const;
  void setTimePanelOptions(const AdDateTimePanelOptions& value);

  QDateTime value() const;
  void setValue(const QDateTime& value);

  QVector<QDateTime> values() const;
  void setValues(const QVector<QDateTime>& values);

  QVector<AdDatePresetItem> presets() const;
  void setPresets(const QVector<AdDatePresetItem>& value);

  DisabledDateEvaluator disabledDateEvaluator() const;
  void setDisabledDateEvaluator(DisabledDateEvaluator evaluator);

  DisabledTimeEvaluator disabledTimeEvaluator() const;
  void setDisabledTimeEvaluator(DisabledTimeEvaluator evaluator);

  ComponentTokens componentTokens() const;
  void setComponentTokens(const ComponentTokens& tokens);
  void resetComponentTokens();

  void clearSelection();
  void focus();

 signals:
  void pickerModeChanged(PickerMode value);
  void selectionModeChanged(SelectionMode value);
  void controlSizeChanged(ControlSize value);
  void variantChanged(Variant value);
  void statusChanged(Status value);
  void allowClearChanged(bool value);
  void popupVisibleChanged(bool value);
  void showTimeChanged(bool value);
  void needConfirmChanged(bool value);
  void autoSortSelectionsChanged(bool value);
  void disabledChanged(bool value);
  void placementChanged(Placement value);
  void placeholderChanged(const QString& value);
  void displayFormatChanged(const QString& value);
  void acceptedInputFormatsChanged(const QStringList& value);
  void maxVisibleTagsChanged(int value);
  void responsiveMaxTagCountChanged(bool value);
  void minDateChanged(const QDate& value);
  void maxDateChanged(const QDate& value);
  void prefixTextChanged(const QString& value);
  void suffixTextChanged(const QString& value);
  void prefixIconRefChanged(const adqt::icons::IconRef& value);
  void suffixIconRefChanged(const adqt::icons::IconRef& value);
  void extraFooterTextChanged(const QString& value);
  void extraFooterWidgetChanged(QWidget* widget);
  void timePanelOptionsChanged(const adqt::widgets::AdDateTimePanelOptions& value);
  void valueChanged(const QDateTime& value);
  void valuesChanged(const QVector<QDateTime>& value);
  void presetsChanged();
  void componentTokensChanged();
  void cleared();
  void editingFinished(const QDateTime& value);

 protected:
  bool eventFilter(QObject* watched, QEvent* event) override;
  void paintEvent(QPaintEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void changeEvent(QEvent* event) override;

 private:
  class Private;
  std::unique_ptr<Private> d_;
};

class AdDateRangePicker final : public QWidget {
  Q_OBJECT

  Q_PROPERTY(PickerMode pickerMode READ pickerMode WRITE setPickerMode NOTIFY pickerModeChanged)
  Q_PROPERTY(ControlSize controlSize READ controlSize WRITE setControlSize NOTIFY controlSizeChanged)
  Q_PROPERTY(Variant variant READ variant WRITE setVariant NOTIFY variantChanged)
  Q_PROPERTY(Status status READ status WRITE setStatus NOTIFY statusChanged)
  Q_PROPERTY(bool allowClear READ allowClear WRITE setAllowClear NOTIFY allowClearChanged)
  Q_PROPERTY(bool popupVisible READ popupVisible WRITE setPopupVisible NOTIFY popupVisibleChanged)
  Q_PROPERTY(bool showTime READ showTime WRITE setShowTime NOTIFY showTimeChanged)
  Q_PROPERTY(bool needConfirm READ needConfirm WRITE setNeedConfirm NOTIFY needConfirmChanged)
  Q_PROPERTY(bool disabled READ disabled WRITE setDisabled NOTIFY disabledChanged)
  Q_PROPERTY(bool startDisabled READ startDisabled WRITE setStartDisabled NOTIFY startDisabledChanged)
  Q_PROPERTY(bool endDisabled READ endDisabled WRITE setEndDisabled NOTIFY endDisabledChanged)
  Q_PROPERTY(bool allowEmptyStart READ allowEmptyStart WRITE setAllowEmptyStart
                 NOTIFY allowEmptyStartChanged)
  Q_PROPERTY(bool allowEmptyEnd READ allowEmptyEnd WRITE setAllowEmptyEnd
                 NOTIFY allowEmptyEndChanged)
  Q_PROPERTY(Placement placement READ placement WRITE setPlacement NOTIFY placementChanged)
  Q_PROPERTY(QString startPlaceholder READ startPlaceholder WRITE setStartPlaceholder
                 NOTIFY startPlaceholderChanged)
  Q_PROPERTY(QString endPlaceholder READ endPlaceholder WRITE setEndPlaceholder
                 NOTIFY endPlaceholderChanged)
  Q_PROPERTY(QString separatorText READ separatorText WRITE setSeparatorText NOTIFY separatorTextChanged)
  Q_PROPERTY(QString displayFormat READ displayFormat WRITE setDisplayFormat NOTIFY displayFormatChanged)
  Q_PROPERTY(QStringList acceptedInputFormats READ acceptedInputFormats WRITE setAcceptedInputFormats
                 NOTIFY acceptedInputFormatsChanged)
  Q_PROPERTY(QDate minDate READ minDate WRITE setMinDate NOTIFY minDateChanged)
  Q_PROPERTY(QDate maxDate READ maxDate WRITE setMaxDate NOTIFY maxDateChanged)
  Q_PROPERTY(QString prefixText READ prefixText WRITE setPrefixText NOTIFY prefixTextChanged)
  Q_PROPERTY(QString suffixText READ suffixText WRITE setSuffixText NOTIFY suffixTextChanged)
  Q_PROPERTY(adqt::icons::IconRef prefixIconRef READ prefixIconRef WRITE setPrefixIconRef
                 NOTIFY prefixIconRefChanged)
  Q_PROPERTY(adqt::icons::IconRef suffixIconRef READ suffixIconRef WRITE setSuffixIconRef
                 NOTIFY suffixIconRefChanged)
  Q_PROPERTY(QString extraFooterText READ extraFooterText WRITE setExtraFooterText
                 NOTIFY extraFooterTextChanged)
  Q_PROPERTY(adqt::widgets::AdDateTimePanelOptions timePanelOptions READ timePanelOptions
                 WRITE setTimePanelOptions NOTIFY timePanelOptionsChanged)
  Q_PROPERTY(adqt::widgets::AdDateTimeRangeValue rangeValue READ rangeValue WRITE setRangeValue
                 NOTIFY rangeValueChanged)
  Q_PROPERTY(QVector<adqt::widgets::AdDateRangePresetItem> presets READ presets WRITE setPresets
                 NOTIFY presetsChanged)

 public:
  using PickerMode = AdDatePicker::PickerMode;
  using ControlSize = AdDatePicker::ControlSize;
  using Variant = AdDatePicker::Variant;
  using Status = AdDatePicker::Status;
  using Placement = AdDatePicker::Placement;
  using RangePart = AdDatePicker::RangePart;
  using ComponentTokens = AdDatePicker::ComponentTokens;
  using DisabledDateEvaluator = AdDatePicker::DisabledDateEvaluator;
  using DisabledTimeEvaluator = AdDatePicker::DisabledTimeEvaluator;

  explicit AdDateRangePicker(QWidget* parent = nullptr);
  ~AdDateRangePicker() override;

  PickerMode pickerMode() const;
  void setPickerMode(PickerMode value);

  ControlSize controlSize() const;
  void setControlSize(ControlSize value);

  Variant variant() const;
  void setVariant(Variant value);

  Status status() const;
  void setStatus(Status value);

  bool allowClear() const;
  void setAllowClear(bool value);

  bool popupVisible() const;
  void setPopupVisible(bool value);

  bool showTime() const;
  void setShowTime(bool value);

  bool needConfirm() const;
  void setNeedConfirm(bool value);

  bool disabled() const;
  void setDisabled(bool value);

  bool startDisabled() const;
  void setStartDisabled(bool value);

  bool endDisabled() const;
  void setEndDisabled(bool value);

  bool allowEmptyStart() const;
  void setAllowEmptyStart(bool value);

  bool allowEmptyEnd() const;
  void setAllowEmptyEnd(bool value);

  Placement placement() const;
  void setPlacement(Placement value);

  QString startPlaceholder() const;
  void setStartPlaceholder(const QString& value);

  QString endPlaceholder() const;
  void setEndPlaceholder(const QString& value);

  QString separatorText() const;
  void setSeparatorText(const QString& value);

  QString displayFormat() const;
  void setDisplayFormat(const QString& value);

  QStringList acceptedInputFormats() const;
  void setAcceptedInputFormats(const QStringList& value);

  QDate minDate() const;
  void setMinDate(const QDate& value);

  QDate maxDate() const;
  void setMaxDate(const QDate& value);

  QString prefixText() const;
  void setPrefixText(const QString& value);

  QString suffixText() const;
  void setSuffixText(const QString& value);

  adqt::icons::IconRef prefixIconRef() const;
  void setPrefixIconRef(const adqt::icons::IconRef& value);

  adqt::icons::IconRef suffixIconRef() const;
  void setSuffixIconRef(const adqt::icons::IconRef& value);

  QString extraFooterText() const;
  void setExtraFooterText(const QString& value);

  QWidget* extraFooterWidget() const;
  void setExtraFooterWidget(QWidget* widget);
  QWidget* takeExtraFooterWidget();

  AdDateTimePanelOptions timePanelOptions() const;
  void setTimePanelOptions(const AdDateTimePanelOptions& value);

  AdDateTimeRangeValue rangeValue() const;
  void setRangeValue(const AdDateTimeRangeValue& value);

  QVector<AdDateRangePresetItem> presets() const;
  void setPresets(const QVector<AdDateRangePresetItem>& value);

  DisabledDateEvaluator disabledDateEvaluator() const;
  void setDisabledDateEvaluator(DisabledDateEvaluator evaluator);

  DisabledTimeEvaluator disabledTimeEvaluator() const;
  void setDisabledTimeEvaluator(DisabledTimeEvaluator evaluator);

  ComponentTokens componentTokens() const;
  void setComponentTokens(const ComponentTokens& tokens);
  void resetComponentTokens();

  void clearSelection();
  void focus();

 signals:
  void pickerModeChanged(PickerMode value);
  void controlSizeChanged(ControlSize value);
  void variantChanged(Variant value);
  void statusChanged(Status value);
  void allowClearChanged(bool value);
  void popupVisibleChanged(bool value);
  void showTimeChanged(bool value);
  void needConfirmChanged(bool value);
  void disabledChanged(bool value);
  void startDisabledChanged(bool value);
  void endDisabledChanged(bool value);
  void allowEmptyStartChanged(bool value);
  void allowEmptyEndChanged(bool value);
  void placementChanged(Placement value);
  void startPlaceholderChanged(const QString& value);
  void endPlaceholderChanged(const QString& value);
  void separatorTextChanged(const QString& value);
  void displayFormatChanged(const QString& value);
  void acceptedInputFormatsChanged(const QStringList& value);
  void minDateChanged(const QDate& value);
  void maxDateChanged(const QDate& value);
  void prefixTextChanged(const QString& value);
  void suffixTextChanged(const QString& value);
  void prefixIconRefChanged(const adqt::icons::IconRef& value);
  void suffixIconRefChanged(const adqt::icons::IconRef& value);
  void extraFooterTextChanged(const QString& value);
  void extraFooterWidgetChanged(QWidget* widget);
  void timePanelOptionsChanged(const adqt::widgets::AdDateTimePanelOptions& value);
  void rangeValueChanged(const adqt::widgets::AdDateTimeRangeValue& value);
  void presetsChanged();
  void componentTokensChanged();
  void cleared();
  void editingFinished(const adqt::widgets::AdDateTimeRangeValue& value);

 protected:
  bool eventFilter(QObject* watched, QEvent* event) override;
  void paintEvent(QPaintEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void changeEvent(QEvent* event) override;

 private:
  class Private;
  std::unique_ptr<Private> d_;
};

}  // namespace adqt::widgets

Q_DECLARE_METATYPE(adqt::widgets::AdDisabledTimeSpec)
Q_DECLARE_METATYPE(adqt::widgets::AdDateTimeRangeValue)
Q_DECLARE_METATYPE(adqt::widgets::AdDatePresetItem)
Q_DECLARE_METATYPE(adqt::widgets::AdDateRangePresetItem)
Q_DECLARE_METATYPE(adqt::widgets::AdDateTimePanelOptions)
Q_DECLARE_METATYPE(QVector<QDateTime>)
Q_DECLARE_METATYPE(QVector<adqt::widgets::AdDatePresetItem>)
Q_DECLARE_METATYPE(QVector<adqt::widgets::AdDateRangePresetItem>)
