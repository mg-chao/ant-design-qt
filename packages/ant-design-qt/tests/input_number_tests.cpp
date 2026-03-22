#include <QAccessible>
#include <QEnterEvent>
#include <QLineEdit>
#include <QSignalSpy>
#include <QtTest>

#include "widgets/input_number.h"

namespace {

using adqt::widgets::AdInputNumber;

void showAndStabilize(QWidget* widget) {
  QVERIFY(widget != nullptr);
  widget->show();
  QTest::qWait(1);
  QCoreApplication::processEvents();
}

Qt::Alignment horizontalAlignmentOf(const AdInputNumber& input) {
  return input.textAlignment() & Qt::AlignHorizontal_Mask;
}

QLineEdit* editorOf(AdInputNumber& input) {
  return input.findChild<QLineEdit*>();
}

}  // namespace

class InputNumberTests final : public QObject {
  Q_OBJECT

 private slots:
  void keyboardTrackingOffDefersCommitUntilFinish();
  void keyboardTrackingOnCommitsImmediately();
  void exactModeStepPreservesCanonicalDecimal();
  void clearResetsValueAndHasValueState();
  void compactLayoutKeepsBalancedPaddingAndHoverDoesNotShiftEditor();
  void stepButtonLayoutUpdatesDefaultAlignment();
  void strictRangeModeClampsCommittedValues();
  void permissiveRangeModePreservesOutOfRangeValues();
  void accessibleInterfaceExposesSpinBoxValue();
};

void InputNumberTests::keyboardTrackingOffDefersCommitUntilFinish() {
  AdInputNumber input;
  input.setRange(0.0, 10.0);
  input.setValue(1.0);
  input.setKeyboardTracking(false);
  input.resize(180, input.sizeHint().height());
  showAndStabilize(&input);

  QSignalSpy valueSpy(&input, &AdInputNumber::valueChanged);
  QSignalSpy textSpy(&input, &AdInputNumber::textChanged);

  input.focusEditor(AdInputNumber::FocusSelection::SelectAll);
  QLineEdit* editor = editorOf(input);
  QVERIFY(editor != nullptr);
  QTest::keyClicks(editor, "5");

  QTRY_VERIFY(textSpy.count() >= 1);
  QCOMPARE(input.value(), 1.0);
  QCOMPARE(input.displayText(), QStringLiteral("5"));
  QCOMPARE(valueSpy.count(), 0);

  QTest::keyClick(editor, Qt::Key_Return);
  QTRY_COMPARE(input.value(), 5.0);
  QTRY_VERIFY(valueSpy.count() >= 1);
}

void InputNumberTests::keyboardTrackingOnCommitsImmediately() {
  AdInputNumber input;
  input.setRange(0.0, 10.0);
  input.setValue(1.0);
  input.setKeyboardTracking(true);
  input.resize(180, input.sizeHint().height());
  showAndStabilize(&input);

  QSignalSpy valueSpy(&input, &AdInputNumber::valueChanged);

  input.focusEditor(AdInputNumber::FocusSelection::SelectAll);
  QLineEdit* editor = editorOf(input);
  QVERIFY(editor != nullptr);
  QTest::keyClicks(editor, "5");

  QTRY_COMPARE(input.value(), 5.0);
  QTRY_VERIFY(valueSpy.count() >= 1);
  QCOMPARE(input.displayText(), QStringLiteral("5"));
}

void InputNumberTests::exactModeStepPreservesCanonicalDecimal() {
  AdInputNumber input;
  input.setValueMode(AdInputNumber::ValueMode::ExactDecimal);
  input.setExactRange(QStringLiteral("0"), QStringLiteral("2"));
  input.setExactValue(QStringLiteral("1"));
  input.setExactSingleStep(QStringLiteral("0.00000000000001"));
  input.resize(260, input.sizeHint().height());
  showAndStabilize(&input);

  QSignalSpy exactSpy(&input, &AdInputNumber::exactValueChanged);

  input.focusEditor();
  QLineEdit* editor = editorOf(input);
  QVERIFY(editor != nullptr);
  QTest::keyClick(editor, Qt::Key_Up);

  QTRY_COMPARE(input.exactValue(), QStringLiteral("1.00000000000001"));
  QTRY_VERIFY(exactSpy.count() >= 1);
  QCOMPARE(input.displayText(), QStringLiteral("1.00000000000001"));
}

void InputNumberTests::clearResetsValueAndHasValueState() {
  AdInputNumber input;
  input.setValue(3.0);
  input.resize(180, input.sizeHint().height());
  showAndStabilize(&input);

  QSignalSpy hasValueSpy(&input, &AdInputNumber::hasValueChanged);
  QSignalSpy exactSpy(&input, &AdInputNumber::exactValueChanged);

  QVERIFY(input.hasValue());
  input.clear();

  QTRY_VERIFY(hasValueSpy.count() >= 1);
  QTRY_VERIFY(exactSpy.count() >= 1);
  QVERIFY(!input.hasValue());
  QCOMPARE(input.exactValue(), QString());
  QCOMPARE(input.displayText(), QString());
}

void InputNumberTests::compactLayoutKeepsBalancedPaddingAndHoverDoesNotShiftEditor() {
  AdInputNumber input;
  AdInputNumber::AppearanceOverrides overrides;
  overrides.metrics.handleWidth = 50;
  input.setAppearanceOverrides(overrides);
  input.resize(200, input.sizeHint().height());
  showAndStabilize(&input);

  QLineEdit* editor = editorOf(input);
  QVERIFY(editor != nullptr);
  QEvent leaveEditor(QEvent::Leave);
  QCoreApplication::sendEvent(editor, &leaveEditor);
  QEvent leaveInput(QEvent::Leave);
  QCoreApplication::sendEvent(&input, &leaveInput);
  QCoreApplication::processEvents();

  const QRect restingGeometry = editor->geometry();
  const int leftGap = restingGeometry.left();
  const int rightGap = input.width() - restingGeometry.right() - 1;
  QVERIFY2(qAbs(leftGap - rightGap) <= 1,
           qPrintable(QStringLiteral("Unbalanced inline padding: left=%1 right=%2")
                          .arg(leftGap)
                          .arg(rightGap)));

  auto* actions = input.findChild<QWidget*>(QStringLiteral("ad-input-number-actions"));
  QVERIFY(actions != nullptr);

  const QPointF hoverPoint(input.rect().center());
  QEnterEvent enterInput(hoverPoint, hoverPoint, hoverPoint);
  QCoreApplication::sendEvent(&input, &enterInput);
  QCoreApplication::processEvents();

  QVERIFY(actions->isVisible());
  QVERIFY(actions->width() > 0);
  QCOMPARE(editor->geometry(), restingGeometry);
}

void InputNumberTests::stepButtonLayoutUpdatesDefaultAlignment() {
  AdInputNumber input;

  QCOMPARE(horizontalAlignmentOf(input), Qt::AlignLeft);

  input.setStepButtonLayout(AdInputNumber::StepButtonLayout::Split);
  QCOMPARE(input.textAlignment(), Qt::AlignCenter);

  input.setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
  input.setStepButtonLayout(AdInputNumber::StepButtonLayout::Compact);
  QCOMPARE(input.textAlignment(), Qt::AlignRight | Qt::AlignVCenter);
}

void InputNumberTests::strictRangeModeClampsCommittedValues() {
  AdInputNumber input;
  input.setRange(0.0, 10.0);
  input.setValue(99.0);

  QCOMPARE(input.rangeMode(), AdInputNumber::RangeMode::Strict);
  QCOMPARE(input.value(), 10.0);
  QCOMPARE(input.exactValue(), QStringLiteral("10"));
}

void InputNumberTests::permissiveRangeModePreservesOutOfRangeValues() {
  AdInputNumber input;
  input.setRange(0.0, 10.0);
  input.setRangeMode(AdInputNumber::RangeMode::Permissive);
  input.setExactValue(QStringLiteral("99"));

  QCOMPARE(input.value(), 99.0);
  QCOMPARE(input.exactValue(), QStringLiteral("99"));
  QCOMPARE(input.displayText(), QStringLiteral("99"));
}

void InputNumberTests::accessibleInterfaceExposesSpinBoxValue() {
  AdInputNumber input;
  input.setRange(0.0, 10.0);
  input.setValue(7.0);
  input.setPlaceholderText(QStringLiteral("Count"));
  input.resize(180, input.sizeHint().height());
  showAndStabilize(&input);

  QAccessibleInterface* iface = QAccessible::queryAccessibleInterface(&input);
  QVERIFY(iface != nullptr);
  QCOMPARE(iface->role(), QAccessible::SpinBox);

  auto* valueIface =
      static_cast<QAccessibleValueInterface*>(iface->interface_cast(QAccessible::ValueInterface));
  QVERIFY(valueIface != nullptr);
  QCOMPARE(valueIface->currentValue().toDouble(), 7.0);
  QCOMPARE(valueIface->minimumValue().toDouble(), 0.0);
  QCOMPARE(valueIface->maximumValue().toDouble(), 10.0);
  QCOMPARE(valueIface->minimumStepSize().toDouble(), 1.0);
  QVERIFY(iface->text(QAccessible::Description).contains(QStringLiteral("Range 0 to 10")));
}

QObject* createInputNumberTests() { return new InputNumberTests(); }

#include "input_number_tests.moc"
