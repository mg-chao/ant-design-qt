#include "date_picker_docs_page.h"

#include <QDateTime>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

using adqt::widgets::AdDatePicker;
using adqt::widgets::AdDateRangePicker;
using adqt::widgets::AdDatePresetItem;
using adqt::widgets::AdDateRangePresetItem;
using adqt::widgets::AdDateTimeRangeValue;

namespace {

QWidget* wrapRows(const QList<QWidget*>& rows) {
  auto* host = new QWidget();
  auto* layout = new QVBoxLayout(host);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(10);
  for (QWidget* row : rows) {
    layout->addWidget(row);
  }
  return host;
}

QWidget* makeRow(const QList<QWidget*>& widgets) {
  auto* host = new QWidget();
  auto* layout = new QHBoxLayout(host);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(12);
  for (QWidget* widget : widgets) {
    layout->addWidget(widget);
  }
  layout->addStretch();
  return host;
}

QLabel* makeHint(const QString& text) {
  auto* label = new QLabel(text);
  label->setWordWrap(true);
  return label;
}

}  // namespace

DatePickerDocsPage::DatePickerDocsPage(QWidget* parent) : QWidget(parent) {
  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(16, 16, 16, 24);
  root->setSpacing(16);

  auto* title = new QLabel("DatePicker");
  QFont titleFont = title->font();
  titleFont.setPointSize(titleFont.pointSize() + 8);
  titleFont.setBold(true);
  title->setFont(titleFont);
  root->addWidget(title);

  auto* subtitle = new QLabel(
      "Qt-native DatePicker and RangePicker rebuilt against `components/date-picker/index.en-US.md` "
      "and the Ant Design demos under `components/date-picker/demo`, with multiple-selection chip "
      "behavior cross-checked against `components/tag/index.en-US.md`.");
  subtitle->setWordWrap(true);
  root->addWidget(subtitle);

  addSection(root, "Basic",
             "Aligned with `demo/basic.tsx` and `demo/range-picker.tsx`: click the field to open "
             "a calendar popover and select a date or range.",
             buildBasicDemo());
  addSection(root, "Range Picker",
             "Cross-checked with `demo/range-picker.tsx` and `demo/allow-empty.tsx`: the Qt "
             "version keeps split start/end inputs and supports open intervals.",
             buildRangeDemo());
  addSection(root, "Multiple",
             "Cross-checked with `demo/multiple.tsx` plus the Ant Design Tag visual language: "
             "selected values render as compact removable chips instead of plain comma text.",
             buildMultipleDemo());
  addSection(root, "Picker Modes",
             "Matches the core picker modes described in `index.en-US.md`: date, week, month, "
             "quarter, and year.",
             buildModeDemo());
  addSection(root, "Formatting",
             "Cross-checked with `demo/format.tsx`: `displayFormat` controls rendering while "
             "`acceptedInputFormats` allows alternate typed input.",
             buildFormatDemo());
  addSection(root, "Time Selection",
             "Aligned with `demo/time.tsx`: optional time controls appear inside the popup and "
             "can be paired with explicit confirmation.",
             buildTimeDemo());
  addSection(root, "Constraints",
             "Cross-checked with `demo/date-range.tsx` and `demo/disabled-date.tsx`: min/max "
             "bounds and custom disabled-date rules are both available.",
             buildConstraintsDemo());
  addSection(root, "Presets",
             "Aligned with `demo/preset-ranges.tsx`: quick actions can inject a single date or a "
             "date range from the preset rail.",
             buildPresetsDemo());
  addSection(root, "Extra Footer",
             "Cross-checked with `demo/extra-footer.tsx` and `demo/needConfirm.tsx`: extra footer "
             "content and explicit OK confirmation are both supported.",
             buildFooterDemo());
  addSection(root, "Size, Status, Variant",
             "Cross-checked with `demo/size.tsx`, `demo/status.tsx`, `demo/variant.tsx`, "
             "`demo/placement.tsx`, and `demo/suffix.tsx`.",
             buildSizeStatusVariantDemo());

  root->addStretch();
}

const QVector<QWidget*>& DatePickerDocsPage::sectionAnchors() const { return anchors_; }

const QStringList& DatePickerDocsPage::sectionTitles() const { return titles_; }

void DatePickerDocsPage::addSection(QVBoxLayout* root,
                                    const QString& title,
                                    const QString& description,
                                    QWidget* content) {
  auto* panel = new QFrame();
  panel->setFrameShape(QFrame::StyledPanel);
  auto* layout = new QVBoxLayout(panel);
  layout->setContentsMargins(14, 14, 14, 14);
  layout->setSpacing(10);

  auto* titleLabel = new QLabel(title);
  QFont titleFont = titleLabel->font();
  titleFont.setBold(true);
  titleFont.setPointSize(titleFont.pointSize() + 1);
  titleLabel->setFont(titleFont);

  auto* descLabel = new QLabel(description);
  descLabel->setWordWrap(true);

  layout->addWidget(titleLabel);
  layout->addWidget(descLabel);
  layout->addWidget(content);

  root->addWidget(panel);
  anchors_.append(panel);
  titles_.append(title);
}

QWidget* DatePickerDocsPage::buildBasicDemo() {
  auto* single = new AdDatePicker();
  single->setValue(QDateTime(QDate::currentDate(), QTime(0, 0, 0)));

  auto* range = new AdDateRangePicker();
  AdDateTimeRangeValue rangeValue;
  rangeValue.start = QDateTime(QDate::currentDate().addDays(-6), QTime(0, 0, 0));
  rangeValue.end = QDateTime(QDate::currentDate(), QTime(0, 0, 0));
  range->setRangeValue(rangeValue);

  return makeRow({single, range});
}

QWidget* DatePickerDocsPage::buildRangeDemo() {
  auto* requiredRange = new AdDateRangePicker();
  requiredRange->setSeparatorText("to");
  requiredRange->setRangeValue(
      {QDateTime(QDate::currentDate().addDays(-14), QTime(0, 0, 0)),
       QDateTime(QDate::currentDate().addDays(-2), QTime(0, 0, 0))});

  auto* openRange = new AdDateRangePicker();
  openRange->setAllowEmptyStart(true);
  openRange->setRangeValue({QDateTime(), QDateTime(QDate::currentDate().addDays(10), QTime(0, 0, 0))});

  return wrapRows({makeRow({requiredRange}), makeRow({openRange}),
                   makeHint("The second example mirrors Ant Design's open-interval behavior by "
                            "allowing an unset start value while still showing the end selection.")});
}

QWidget* DatePickerDocsPage::buildMultipleDemo() {
  const QVector<QDateTime> defaults = {
      QDateTime(QDate(2026, 3, 1), QTime(0, 0, 0)),
      QDateTime(QDate(2026, 3, 3), QTime(0, 0, 0)),
      QDateTime(QDate(2026, 3, 5), QTime(0, 0, 0)),
  };

  auto* small = new AdDatePicker();
  small->setSelectionMode(AdDatePicker::SelectionMode::Multiple);
  small->setControlSize(AdDatePicker::ControlSize::Small);
  small->setResponsiveMaxTagCount(true);
  small->setValues(defaults);

  auto* middle = new AdDatePicker();
  middle->setSelectionMode(AdDatePicker::SelectionMode::Multiple);
  middle->setResponsiveMaxTagCount(true);
  middle->setValues(defaults);

  auto* large = new AdDatePicker();
  large->setSelectionMode(AdDatePicker::SelectionMode::Multiple);
  large->setControlSize(AdDatePicker::ControlSize::Large);
  large->setResponsiveMaxTagCount(true);
  large->setValues(defaults);

  return wrapRows({makeRow({small}), makeRow({middle}), makeRow({large}),
                   makeHint("The chip container intentionally follows the compact Tag proportions "
                            "instead of rendering multiple dates as plain text, matching the "
                            "combination of `demo/multiple.tsx` and the Tag component docs.")});
}

QWidget* DatePickerDocsPage::buildModeDemo() {
  auto* grid = new QWidget();
  auto* layout = new QGridLayout(grid);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setHorizontalSpacing(12);
  layout->setVerticalSpacing(12);

  const QList<QPair<QString, AdDatePicker::PickerMode>> items = {
      {"Date", AdDatePicker::PickerMode::Date},
      {"Week", AdDatePicker::PickerMode::Week},
      {"Month", AdDatePicker::PickerMode::Month},
      {"Quarter", AdDatePicker::PickerMode::Quarter},
      {"Year", AdDatePicker::PickerMode::Year},
  };

  int row = 0;
  for (const auto& item : items) {
    auto* label = new QLabel(item.first);
    auto* picker = new AdDatePicker();
    picker->setPickerMode(item.second);
    layout->addWidget(label, row, 0);
    layout->addWidget(picker, row, 1);
    ++row;
  }

  return grid;
}

QWidget* DatePickerDocsPage::buildFormatDemo() {
  auto* compact = new AdDatePicker();
  compact->setDisplayFormat("dd/MM/yyyy");
  compact->setAcceptedInputFormats({"dd/MM/yyyy", "yyyy-MM-dd"});
  compact->setValue(QDateTime(QDate(2026, 3, 22), QTime(0, 0, 0)));

  auto* month = new AdDatePicker();
  month->setPickerMode(AdDatePicker::PickerMode::Month);
  month->setDisplayFormat("yyyy.MM");
  month->setValue(QDateTime(QDate(2026, 7, 1), QTime(0, 0, 0)));

  return wrapRows({makeRow({compact}), makeRow({month}),
                   makeHint("You can still type either `22/03/2026` or `2026-03-22` into the "
                            "first field because `acceptedInputFormats` keeps multi-format parsing.")});
}

QWidget* DatePickerDocsPage::buildTimeDemo() {
  auto* single = new AdDatePicker();
  single->setShowTime(true);
  single->setNeedConfirm(true);
  single->setDisplayFormat("yyyy-MM-dd HH:mm");
  single->setAcceptedInputFormats({"yyyy-MM-dd HH:mm"});
  single->setValue(QDateTime(QDate::currentDate(), QTime::currentTime()));

  auto* range = new AdDateRangePicker();
  range->setShowTime(true);
  range->setNeedConfirm(true);
  range->setDisplayFormat("yyyy-MM-dd HH:mm");
  range->setRangeValue(
      {QDateTime(QDate::currentDate().addDays(-1), QTime(9, 30, 0)),
       QDateTime(QDate::currentDate(), QTime(17, 45, 0))});

  return wrapRows({makeRow({single}), makeRow({range})});
}

QWidget* DatePickerDocsPage::buildConstraintsDemo() {
  auto* bounded = new AdDatePicker();
  bounded->setMinDate(QDate::currentDate().addDays(-7));
  bounded->setMaxDate(QDate::currentDate().addDays(14));

  auto* noWeekend = new AdDatePicker();
  noWeekend->setDisabledDateEvaluator([](const QDate& date) {
    return date.dayOfWeek() == Qt::Saturday || date.dayOfWeek() == Qt::Sunday;
  });

  auto* range = new AdDateRangePicker();
  range->setMinDate(QDate::currentDate().addDays(-30));
  range->setMaxDate(QDate::currentDate().addDays(30));

  return wrapRows({makeRow({bounded, noWeekend}), makeRow({range})});
}

QWidget* DatePickerDocsPage::buildPresetsDemo() {
  auto* single = new AdDatePicker();
  single->setPresets({
      {"Yesterday", QDateTime(QDate::currentDate().addDays(-1), QTime(0, 0, 0))},
      {"Last Week", QDateTime(QDate::currentDate().addDays(-7), QTime(0, 0, 0))},
      {"Last Month", QDateTime(QDate::currentDate().addMonths(-1), QTime(0, 0, 0))},
  });

  auto* range = new AdDateRangePicker();
  range->setPresets({
      {"Last 7 Days",
       {QDateTime(QDate::currentDate().addDays(-7), QTime(0, 0, 0)),
        QDateTime(QDate::currentDate(), QTime(0, 0, 0))}},
      {"Last 30 Days",
       {QDateTime(QDate::currentDate().addDays(-30), QTime(0, 0, 0)),
        QDateTime(QDate::currentDate(), QTime(0, 0, 0))}},
  });

  return makeRow({single, range});
}

QWidget* DatePickerDocsPage::buildFooterDemo() {
  auto* withFooter = new AdDatePicker();
  withFooter->setNeedConfirm(true);
  withFooter->setExtraFooterText("Cross-checked with demo/extra-footer.tsx");
  withFooter->setExtraFooterWidget(makeHint("Custom footer widgets can be injected directly into "
                                            "the popup body when a plain text note is not enough."));

  auto* withRangeFooter = new AdDateRangePicker();
  withRangeFooter->setNeedConfirm(true);
  withRangeFooter->setExtraFooterText("The OK button mirrors demo/needConfirm.tsx.");

  return wrapRows({makeRow({withFooter}), makeRow({withRangeFooter})});
}

QWidget* DatePickerDocsPage::buildSizeStatusVariantDemo() {
  auto* large = new AdDatePicker();
  large->setControlSize(AdDatePicker::ControlSize::Large);
  large->setPrefixText("Booking");

  auto* small = new AdDatePicker();
  small->setControlSize(AdDatePicker::ControlSize::Small);
  small->setStatus(AdDatePicker::Status::Warning);

  auto* filled = new AdDatePicker();
  filled->setVariant(AdDatePicker::Variant::Filled);
  filled->setPlacement(AdDatePicker::Placement::TopRight);

  auto* underlined = new AdDatePicker();
  underlined->setVariant(AdDatePicker::Variant::Underlined);
  underlined->setStatus(AdDatePicker::Status::Error);
  underlined->setSuffixText("UTC+8");

  return wrapRows({makeRow({large, small}), makeRow({filled, underlined})});
}
