#include <algorithm>

#include <QAccessible>
#include <QApplication>
#include <QMetaProperty>
#include <QSignalSpy>
#include <QtTest>

#include "widgets/switch.h"

namespace {

using adqt::widgets::AdSwitch;

QPoint indicatorPressPoint(const AdSwitch& sw) {
  return QPoint(std::max(2, sw.height() / 2), std::max(2, sw.height() / 2));
}

QPoint indicatorReleasePoint(const AdSwitch& sw) {
  return QPoint(std::max(indicatorPressPoint(sw).x() + QApplication::startDragDistance() + 2,
                         sw.width() - std::max(3, sw.height() / 2)),
                std::max(2, sw.height() / 2));
}

QPoint rtlIndicatorPressPoint(const AdSwitch& sw) {
  return QPoint(std::max(2, sw.width() - std::max(3, sw.height() / 2)),
                std::max(2, sw.height() / 2));
}

QPoint rtlIndicatorReleasePoint(const AdSwitch& sw) {
  return QPoint(std::max(2, rtlIndicatorPressPoint(sw).x() - QApplication::startDragDistance() - 2),
                std::max(2, sw.height() / 2));
}

}  // namespace

class SwitchTests final : public QObject {
  Q_OBJECT

 private slots:
  void exposesQtStyleProperties();
  void labelTextExtendsSizeHint();
  void spaceKeyTogglesCheckedState();
  void clickOnVisibleLabelTogglesSwitch();
  void dragGestureCommitsThumbPosition();
  void rtlDragGestureCommitsThumbPosition();
  void loadingBlocksUserToggle();
  void explicitCursorOverrideIsPreserved();
  void accessibilityReflectsQtStateAndFallbacks();
};

void SwitchTests::exposesQtStyleProperties() {
  AdSwitch sw;
  const QMetaObject* meta = sw.metaObject();

  const int controlSizeIndex = meta->indexOfProperty("controlSize");
  QVERIFY(controlSizeIndex >= 0);
  const QMetaProperty controlSizeProperty = meta->property(controlSizeIndex);
  QVERIFY(controlSizeProperty.isReadable());
  QVERIFY(controlSizeProperty.isWritable());

  const int sizeIndex = meta->indexOfProperty("size");
  QVERIFY(sizeIndex >= 0);
  const QMetaProperty sizeProperty = meta->property(sizeIndex);
  QCOMPARE(sizeProperty.metaType().id(), QMetaType::QSize);

  QVERIFY(sw.setProperty("controlSize", QVariant::fromValue(AdSwitch::ControlSize::Small)));
  QCOMPARE(sw.controlSize(), AdSwitch::ControlSize::Small);
}

void SwitchTests::labelTextExtendsSizeHint() {
  AdSwitch plain;
  plain.adjustSize();

  AdSwitch labeled;
  labeled.setText(QStringLiteral("Power"));
  labeled.adjustSize();

  QVERIFY(labeled.sizeHint().width() > plain.sizeHint().width());
  QCOMPARE(labeled.minimumSizeHint(), labeled.sizeHint());
}

void SwitchTests::spaceKeyTogglesCheckedState() {
  AdSwitch sw;
  sw.adjustSize();
  sw.show();
  QTest::qWait(1);
  QVERIFY(sw.isVisible());

  QSignalSpy clickedSpy(&sw, &QAbstractButton::clicked);
  sw.setFocus();
  QVERIFY(sw.hasFocus());

  QTest::keyClick(&sw, Qt::Key_Space);

  QTRY_COMPARE(clickedSpy.count(), 1);
  QVERIFY(sw.isChecked());
}

void SwitchTests::clickOnVisibleLabelTogglesSwitch() {
  AdSwitch sw;
  sw.setText(QStringLiteral("Power"));
  sw.adjustSize();
  sw.show();
  QTest::qWait(1);
  QVERIFY(sw.isVisible());

  const QPoint labelPoint(sw.width() - 3, sw.height() / 2);
  QVERIFY(labelPoint.x() > sw.height());

  QSignalSpy clickedSpy(&sw, &QAbstractButton::clicked);
  QTest::mouseClick(&sw, Qt::LeftButton, Qt::NoModifier, labelPoint);

  QTRY_COMPARE(clickedSpy.count(), 1);
  QVERIFY(sw.isChecked());
}

void SwitchTests::dragGestureCommitsThumbPosition() {
  AdSwitch sw;
  sw.adjustSize();
  sw.show();
  QTest::qWait(1);
  QVERIFY(sw.isVisible());

  const QPoint pressPoint = indicatorPressPoint(sw);
  const QPoint releasePoint = indicatorReleasePoint(sw);
  QVERIFY(releasePoint.x() > pressPoint.x());

  QSignalSpy toggledSpy(&sw, &QAbstractButton::toggled);
  QTest::mousePress(&sw, Qt::LeftButton, Qt::NoModifier, pressPoint);
  QTest::mouseMove(&sw, releasePoint, 5);
  QTest::mouseRelease(&sw, Qt::LeftButton, Qt::NoModifier, releasePoint);

  QTRY_COMPARE(toggledSpy.count(), 1);
  QVERIFY(sw.isChecked());
}

void SwitchTests::rtlDragGestureCommitsThumbPosition() {
  AdSwitch sw;
  sw.setLayoutDirection(Qt::RightToLeft);
  sw.adjustSize();
  sw.show();
  QTest::qWait(1);
  QVERIFY(sw.isVisible());

  const QPoint pressPoint = rtlIndicatorPressPoint(sw);
  const QPoint releasePoint = rtlIndicatorReleasePoint(sw);
  QVERIFY(releasePoint.x() < pressPoint.x());

  QSignalSpy toggledSpy(&sw, &QAbstractButton::toggled);
  QTest::mousePress(&sw, Qt::LeftButton, Qt::NoModifier, pressPoint);
  QTest::mouseMove(&sw, releasePoint, 5);
  QTest::mouseRelease(&sw, Qt::LeftButton, Qt::NoModifier, releasePoint);

  QTRY_COMPARE(toggledSpy.count(), 1);
  QVERIFY(sw.isChecked());
}

void SwitchTests::loadingBlocksUserToggle() {
  AdSwitch sw;
  sw.adjustSize();
  sw.show();
  QTest::qWait(1);
  QVERIFY(sw.isVisible());

  const QPoint pressPoint = indicatorPressPoint(sw);
  QSignalSpy clickedSpy(&sw, &QAbstractButton::clicked);

  sw.setLoading(true);
  sw.setFocus();
  QTest::mouseClick(&sw, Qt::LeftButton, Qt::NoModifier, pressPoint);
  QTest::keyClick(&sw, Qt::Key_Space);
  QCOMPARE(clickedSpy.count(), 0);
  QVERIFY(!sw.isChecked());

  sw.setLoading(false);
  sw.setFocus();
  QTest::mouseClick(&sw, Qt::LeftButton, Qt::NoModifier, pressPoint);
  QTest::keyClick(&sw, Qt::Key_Space);
  QTRY_COMPARE(clickedSpy.count(), 2);
}

void SwitchTests::explicitCursorOverrideIsPreserved() {
  AdSwitch sw;
  sw.adjustSize();
  sw.show();
  QTest::qWait(1);
  QVERIFY(sw.isVisible());

  sw.setCursor(Qt::WaitCursor);
  QCOMPARE(sw.cursor().shape(), Qt::WaitCursor);

  sw.setLoading(true);
  QCOMPARE(sw.cursor().shape(), Qt::WaitCursor);

  sw.setLoading(false);
  QCOMPARE(sw.cursor().shape(), Qt::WaitCursor);
}

void SwitchTests::accessibilityReflectsQtStateAndFallbacks() {
  AdSwitch sw;
  sw.setCheckedText(QStringLiteral("Enabled"));
  sw.setUncheckedText(QStringLiteral("Disabled"));

  QAccessibleInterface* iface = QAccessible::queryAccessibleInterface(&sw);
  QVERIFY(iface);
  QCOMPARE(iface->role(), QAccessible::CheckBox);
  QCOMPARE(iface->text(QAccessible::Name), QStringLiteral("Disabled"));
  QCOMPARE(iface->text(QAccessible::Value), QStringLiteral("Disabled"));

  QAccessible::State state = iface->state();
  QVERIFY(state.checkable);
  QVERIFY(!state.checked);
  QVERIFY(!state.busy);
  QVERIFY(!state.disabled);

  sw.setText(QStringLiteral("&Power"));
  QCOMPARE(iface->text(QAccessible::Name), QStringLiteral("Power"));

  sw.setAccessibleName(QStringLiteral("Main power"));
  QCOMPARE(iface->text(QAccessible::Name), QStringLiteral("Main power"));

  sw.setAccessibleName(QString());
  QCOMPARE(iface->text(QAccessible::Name), QStringLiteral("Power"));

  sw.setText(QString());
  QCOMPARE(iface->text(QAccessible::Name), QStringLiteral("Disabled"));

  sw.setChecked(true);
  QCOMPARE(iface->text(QAccessible::Value), QStringLiteral("Enabled"));
  state = iface->state();
  QVERIFY(state.checked);

  sw.setLoading(true);
  state = iface->state();
  QVERIFY(state.busy);

  sw.setEnabled(false);
  state = iface->state();
  QVERIFY(state.disabled);

  AdSwitch fallback;
  QAccessibleInterface* fallbackIface = QAccessible::queryAccessibleInterface(&fallback);
  QVERIFY(fallbackIface);
  QCOMPARE(fallbackIface->text(QAccessible::Value), QStringLiteral("Off"));

  fallback.setChecked(true);
  QCOMPARE(fallbackIface->text(QAccessible::Value), QStringLiteral("On"));
}

int runSwitchTests(int argc, char** argv) {
  SwitchTests tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "switch_tests.moc"
