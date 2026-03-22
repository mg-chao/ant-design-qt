#include <QMetaProperty>
#include <QSignalSpy>
#include <QtTest>

#include "widgets/date_picker.h"

namespace {

class DatePickerTests final : public QObject {
  Q_OBJECT

 private slots:
  void exposesQtProperties();
  void singleValueCanonicalizesByMode();
  void multipleValuesNormalizeAndClear();
  void popupVisibleRoundTrips();
  void rangeValueSortsAndAllowsEmpty();
};

void DatePickerTests::exposesQtProperties() {
  adqt::widgets::AdDatePicker picker;
  const QMetaObject* meta = picker.metaObject();

  QVERIFY(meta->indexOfProperty("pickerMode") >= 0);
  QVERIFY(meta->indexOfProperty("selectionMode") >= 0);
  QVERIFY(meta->indexOfProperty("showTime") >= 0);
  QVERIFY(meta->indexOfProperty("displayFormat") >= 0);

  QVERIFY(picker.setProperty(
      "selectionMode",
      QVariant::fromValue(adqt::widgets::AdDatePicker::SelectionMode::Multiple)));
  QCOMPARE(picker.selectionMode(), adqt::widgets::AdDatePicker::SelectionMode::Multiple);

  QVERIFY(
      picker.setProperty("pickerMode",
                         QVariant::fromValue(adqt::widgets::AdDatePicker::PickerMode::Quarter)));
  QCOMPARE(picker.pickerMode(), adqt::widgets::AdDatePicker::PickerMode::Quarter);
}

void DatePickerTests::singleValueCanonicalizesByMode() {
  adqt::widgets::AdDatePicker picker;

  picker.setPickerMode(adqt::widgets::AdDatePicker::PickerMode::Month);
  picker.setValue(QDateTime(QDate(2026, 3, 22), QTime(13, 45, 12)));
  QCOMPARE(picker.value().date(), QDate(2026, 3, 1));
  QCOMPARE(picker.value().time(), QTime(0, 0, 0));

  picker.setShowTime(true);
  picker.setPickerMode(adqt::widgets::AdDatePicker::PickerMode::Date);
  picker.setValue(QDateTime(QDate(2026, 3, 22), QTime(13, 45, 12)));
  QCOMPARE(picker.value(), QDateTime(QDate(2026, 3, 22), QTime(13, 45, 12)));
}

void DatePickerTests::multipleValuesNormalizeAndClear() {
  adqt::widgets::AdDatePicker picker;
  picker.setSelectionMode(adqt::widgets::AdDatePicker::SelectionMode::Multiple);

  const QVector<QDateTime> rawValues = {
      QDateTime(QDate(2026, 3, 10), QTime(0, 0, 0)),
      QDateTime(QDate(2026, 3, 8), QTime(0, 0, 0)),
      QDateTime(QDate(2026, 3, 10), QTime(12, 0, 0)),
  };

  picker.setValues(rawValues);
  QCOMPARE(picker.values().size(), 2);
  QCOMPARE(picker.values().at(0).date(), QDate(2026, 3, 8));
  QCOMPARE(picker.values().at(1).date(), QDate(2026, 3, 10));

  QSignalSpy clearedSpy(&picker, &adqt::widgets::AdDatePicker::cleared);
  picker.clearSelection();
  QCOMPARE(picker.values().size(), 0);
  QCOMPARE(clearedSpy.count(), 1);
}

void DatePickerTests::popupVisibleRoundTrips() {
  QWidget host;
  host.resize(320, 120);

  adqt::widgets::AdDatePicker picker(&host);
  picker.move(24, 24);
  picker.resize(240, 32);

  host.show();
  QTest::qWait(1);

  picker.setPopupVisible(true);
  QTRY_VERIFY(picker.popupVisible());

  picker.setPopupVisible(false);
  QTRY_VERIFY(!picker.popupVisible());
}

void DatePickerTests::rangeValueSortsAndAllowsEmpty() {
  adqt::widgets::AdDateRangePicker picker;

  adqt::widgets::AdDateTimeRangeValue reversed;
  reversed.start = QDateTime(QDate(2026, 3, 22), QTime(9, 0, 0));
  reversed.end = QDateTime(QDate(2026, 3, 18), QTime(18, 0, 0));
  picker.setRangeValue(reversed);

  QVERIFY(picker.rangeValue().start <= picker.rangeValue().end);
  QCOMPARE(picker.rangeValue().start.date(), QDate(2026, 3, 18));
  QCOMPARE(picker.rangeValue().end.date(), QDate(2026, 3, 22));

  picker.setAllowEmptyStart(true);
  adqt::widgets::AdDateTimeRangeValue openInterval;
  openInterval.end = QDateTime(QDate(2026, 3, 30), QTime(12, 0, 0));
  picker.setRangeValue(openInterval);
  QVERIFY(!picker.rangeValue().start.isValid());
  QCOMPARE(picker.rangeValue().end.date(), QDate(2026, 3, 30));
}

}  // namespace

int runDatePickerTests(int argc, char** argv) {
  DatePickerTests tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "date_picker_tests.moc"
