#include <QAccessible>
#include <QCoreApplication>
#include <QMetaProperty>
#include <QSignalSpy>
#include <QtTest>

#include "widgets/slider.h"

namespace {

using adqt::widgets::AdMultiSlider;
using adqt::widgets::AdRangeSlider;
using adqt::widgets::AdSlider;

void showAndStabilize(QWidget* widget) {
  QVERIFY(widget != nullptr);
  widget->show();
  QTest::qWait(1);
  QCoreApplication::processEvents();
}

void sendWheel(QWidget* widget, int angleDeltaY) {
  QVERIFY(widget != nullptr);
  const QPointF localPos(widget->rect().center());
  const QPoint globalPos = widget->mapToGlobal(localPos.toPoint());
  QWheelEvent event(localPos,
                    QPointF(globalPos),
                    QPoint(),
                    QPoint(0, angleDeltaY),
                    Qt::NoButton,
                    Qt::NoModifier,
                    Qt::NoScrollPhase,
                    false);
  QCoreApplication::sendEvent(widget, &event);
  QCoreApplication::processEvents();
}

}  // namespace

class SliderTests final : public QObject {
  Q_OBJECT

 private slots:
  void initTestCase();
  void singleSliderUsesQtStyleRangeApi();
  void rangeSliderSeparatesBoundsAndSelectedValues();
  void trackingFalseSeparatesSliderPositionFromValue();
  void singleSliderSignalsFollowQtStyleContract();
  void multiSliderTabCyclesCurrentHandle();
  void rtlArrowKeysFollowLayoutDirection();
  void wheelInteractionRequiresOptIn();
  void accessibilityReflectsSliderRoleAndDescription();
};

void SliderTests::initTestCase() {
  qRegisterMetaType<QAbstractSlider::SliderAction>();
}

void SliderTests::singleSliderUsesQtStyleRangeApi() {
  AdSlider slider;
  QSignalSpy rangeSpy(&slider, &AdMultiSlider::rangeChanged);
  slider.setRange(10.0, 20.0);
  slider.setValue(15.0);

  QCOMPARE(slider.minimum(), 10.0);
  QCOMPARE(slider.maximum(), 20.0);
  QCOMPARE(slider.value(), 15.0);
  QCOMPARE(slider.sliderPosition(), 15.0);
  QVERIFY(rangeSpy.count() >= 1);

  const QMetaObject* meta = slider.metaObject();
  QVERIFY(meta->indexOfProperty("wheelEnabled") >= 0);
  QVERIFY(meta->indexOfProperty("markSnapEnabled") >= 0);
  QVERIFY(meta->indexOfProperty("selectionHighlightVisible") >= 0);
  QVERIFY(meta->indexOfProperty("sliderPosition") >= 0);
  QVERIFY(meta->indexOfProperty("sliderDown") >= 0);
  QVERIFY(meta->indexOfProperty("markIndicatorsVisible") >= 0);
  QVERIFY(meta->indexOfProperty("markStepSnapEnabled") >= 0);
}

void SliderTests::rangeSliderSeparatesBoundsAndSelectedValues() {
  AdRangeSlider slider;
  QSignalSpy boundsSpy(&slider, &AdMultiSlider::rangeChanged);
  QSignalSpy valuesSpy(&slider, &AdRangeSlider::valuesChanged);
  QSignalSpy lowerSpy(&slider, &AdRangeSlider::lowerValueChanged);
  QSignalSpy upperSpy(&slider, &AdRangeSlider::upperValueChanged);

  slider.setRange(10.0, 100.0);
  QCOMPARE(slider.lowerValue(), 10.0);
  QCOMPARE(slider.upperValue(), 10.0);
  QCOMPARE(boundsSpy.count(), 1);
  QCOMPARE(valuesSpy.count(), 1);
  QCOMPARE(lowerSpy.count(), 1);
  QCOMPARE(upperSpy.count(), 1);

  slider.setValues(20.0, 50.0);

  QCOMPARE(slider.lowerValue(), 20.0);
  QCOMPARE(slider.upperValue(), 50.0);
  QCOMPARE(boundsSpy.count(), 1);
  QCOMPARE(valuesSpy.count(), 2);
  QCOMPARE(lowerSpy.count(), 2);
  QCOMPARE(upperSpy.count(), 2);

  slider.setUpperValue(60.0);
  QCOMPARE(slider.lowerValue(), 20.0);
  QCOMPARE(slider.upperValue(), 60.0);
  QCOMPARE(valuesSpy.count(), 3);
  QCOMPARE(lowerSpy.count(), 2);
  QCOMPARE(upperSpy.count(), 3);
}

void SliderTests::trackingFalseSeparatesSliderPositionFromValue() {
  AdSlider slider;
  slider.setRange(0.0, 10.0);
  slider.setValue(5.0);
  slider.setTracking(false);
  slider.resize(260, slider.sizeHint().height());
  showAndStabilize(&slider);

  QSignalSpy valueSpy(&slider, &AdMultiSlider::valueChanged);
  QSignalSpy finishedSpy(&slider, &AdSlider::editingFinished);

  slider.setFocus();
  QVERIFY(slider.hasFocus());

  QTest::keyPress(&slider, Qt::Key_Right);
  QTRY_COMPARE(slider.sliderPosition(), 6.0);
  QCOMPARE(slider.value(), 5.0);
  QCOMPARE(valueSpy.count(), 0);

  QTest::keyRelease(&slider, Qt::Key_Right);
  QTRY_COMPARE(slider.value(), 6.0);
  QTRY_COMPARE(valueSpy.count(), 1);
  QTRY_COMPARE(finishedSpy.count(), 1);
}

void SliderTests::singleSliderSignalsFollowQtStyleContract() {
  AdSlider slider;
  slider.setRange(0.0, 10.0);
  slider.setValue(5.0);
  slider.resize(260, slider.sizeHint().height());
  showAndStabilize(&slider);

  QSignalSpy pressedSpy(&slider, &AdSlider::sliderPressed);
  QSignalSpy movedSpy(&slider, &AdSlider::sliderMoved);
  QSignalSpy releasedSpy(&slider, &AdSlider::sliderReleased);
  QSignalSpy actionSpy(&slider, &AdSlider::actionTriggered);
  QSignalSpy finishedSpy(&slider, &AdSlider::editingFinished);

  const QPoint center = slider.rect().center();
  QTest::mousePress(&slider, Qt::LeftButton, Qt::NoModifier, center);
  QTest::mouseMove(&slider, center + QPoint(24, 0));
  QTest::mouseRelease(&slider, Qt::LeftButton, Qt::NoModifier, center + QPoint(24, 0));

  QTRY_COMPARE(pressedSpy.count(), 1);
  QVERIFY(movedSpy.count() >= 1);
  QTRY_COMPARE(releasedSpy.count(), 1);
  QTRY_COMPARE(finishedSpy.count(), 1);

  slider.setFocus();
  QVERIFY(slider.hasFocus());
  QTest::keyClick(&slider, Qt::Key_Right);
  QTRY_COMPARE(actionSpy.count(), 1);
}

void SliderTests::multiSliderTabCyclesCurrentHandle() {
  AdMultiSlider slider;
  slider.setHandleValues({10.0, 40.0, 70.0});
  slider.resize(260, slider.sizeHint().height());
  showAndStabilize(&slider);

  slider.setFocus();
  QVERIFY(slider.hasFocus());
  QTRY_COMPARE(slider.currentHandle(), 0);

  QTest::keyClick(&slider, Qt::Key_Tab);
  QTRY_COMPARE(slider.currentHandle(), 1);

  QTest::keyClick(&slider, Qt::Key_Backtab);
  QTRY_COMPARE(slider.currentHandle(), 0);
}

void SliderTests::rtlArrowKeysFollowLayoutDirection() {
  AdSlider slider;
  slider.setRange(0.0, 100.0);
  slider.setValue(50.0);
  slider.resize(260, slider.sizeHint().height());
  slider.setLayoutDirection(Qt::RightToLeft);
  showAndStabilize(&slider);

  slider.setFocus();
  QVERIFY(slider.hasFocus());

  QTest::keyClick(&slider, Qt::Key_Left);
  QTRY_COMPARE(slider.value(), 51.0);
}

void SliderTests::wheelInteractionRequiresOptIn() {
  AdSlider slider;
  slider.setRange(0.0, 10.0);
  slider.setValue(5.0);
  slider.resize(260, slider.sizeHint().height());
  showAndStabilize(&slider);

  QSignalSpy actionSpy(&slider, &AdSlider::actionTriggered);

  sendWheel(&slider, 120);
  QCOMPARE(slider.value(), 5.0);
  QCOMPARE(actionSpy.count(), 0);

  slider.setWheelEnabled(true);
  sendWheel(&slider, 120);
  QTRY_COMPARE(slider.value(), 6.0);
  QTRY_COMPARE(actionSpy.count(), 1);
}

void SliderTests::accessibilityReflectsSliderRoleAndDescription() {
  AdRangeSlider slider;
  slider.setRange(0.0, 100.0);
  slider.setValues(20.0, 60.0);

  QAccessibleInterface* iface = QAccessible::queryAccessibleInterface(&slider);
  QVERIFY(iface != nullptr);
  QCOMPARE(iface->role(), QAccessible::Slider);
  QCOMPARE(iface->text(QAccessible::Name), QStringLiteral("Slider"));
  QVERIFY(iface->text(QAccessible::Description).contains(QStringLiteral("Range 0 to 100")));
}

QObject* createSliderTests() { return new SliderTests(); }

#include "slider_tests.moc"
