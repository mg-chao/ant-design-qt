#include <QAbstractButton>
#include <QAbstractSlider>
#include <QDialog>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMetaProperty>
#include <QMenu>
#include <QSignalSpy>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QtTest>

#include "antd_icons.h"

#define protected public
#include "widgets/alert.h"
#include "widgets/button.h"
#undef protected
#include "widgets/color_picker.h"
#include "widgets/date_picker.h"
#include "widgets/detail/button_grouping.h"
#include "widgets/detail/color_picker_value_model.h"
#include "widgets/input_number.h"
#include "widgets/input_search_edit.h"
#include "widgets/modal.h"
#include "widgets/popup_types.h"

namespace {

bool iconRefsEqual(const adqt::icons::IconRef& lhs, const adqt::icons::IconRef& rhs) {
  return lhs == rhs;
}

void sendReturnKey(QWidget* widget) {
  if (!widget) {
    return;
  }

  QKeyEvent press(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
  QKeyEvent release(QEvent::KeyRelease, Qt::Key_Return, Qt::NoModifier);
  QApplication::sendEvent(widget, &press);
  QApplication::sendEvent(widget, &release);
  QApplication::processEvents();
}

bool hasVisibleInteractionOverlay(const QWidget* root) {
  if (!root) {
    return false;
  }

  if (root->property("adqt.interaction.overlay").toBool() && root->isVisible()) {
    return true;
  }

  const auto overlays = root->findChildren<QWidget*>();
  for (QWidget* overlay : overlays) {
    if (overlay && overlay->property("adqt.interaction.overlay").toBool() && overlay->isVisible()) {
      return true;
    }
  }
  return false;
}

QWidget* findVisibleButtonCursorOverlay(const QWidget* root) {
  if (!root) {
    return nullptr;
  }

  const auto overlays = root->findChildren<QWidget*>(QStringLiteral("ad-button-disabled-cursor-overlay"));
  for (QWidget* overlay : overlays) {
    if (overlay && overlay->isVisible()) {
      return overlay;
    }
  }
  return nullptr;
}

}  // namespace

QObject* createAlertTests();
QObject* createInputTests();
QObject* createInputNumberTests();
QObject* createModalTests();
QObject* createPopupTests();
QObject* createSliderTests();
QObject* createColorPickerTests();
int runImageTests(int argc, char** argv);
int runDatePickerTests(int argc, char** argv);
int runMenuTests(int argc, char** argv);
int runRadioTests(int argc, char** argv);
int runSelectTests(int argc, char** argv);
int runSwitchTests(int argc, char** argv);
int runTagTests(int argc, char** argv);

class ButtonTests final : public QObject {
  Q_OBJECT

 private slots:
  void exposesQtProperties();
  void sizeHintRemainsPublic();
  void explicitMinimumHeightIsPreserved();
  void controlSizeUpdatesMetrics();
  void cursorTracksInteractionState();
  void circleShapeUsesRoundHitTarget();
  void circleShapeFallsBackToPillForText();
  void menuButtonReservesIndicatorSpace();
  void accessibleNameIsExplicitOnly();
  void loadingBlocksActivation();
  void loadingSuspendsDefaultDialogActivation();
  void clickActivationShowsInteractionWave();
  void textAndLinkSkipInteractionWave();
  void searchEditTracksButtonContract();
};

void ButtonTests::exposesQtProperties() {
  adqt::widgets::AdButton button;
  const QMetaObject* meta = button.metaObject();

  const int sizeClassIndex = meta->indexOfProperty("sizeClass");
  QVERIFY(sizeClassIndex >= 0);
  const QMetaProperty sizeClassProperty = meta->property(sizeClassIndex);
  QVERIFY(sizeClassProperty.isReadable());
  QVERIFY(sizeClassProperty.isWritable());

  const int iconPositionIndex = meta->indexOfProperty("iconPosition");
  QVERIFY(iconPositionIndex >= 0);
  QVERIFY(meta->property(iconPositionIndex).isWritable());

  const int busyDelayIndex = meta->indexOfProperty("busyDelayMs");
  QVERIFY(busyDelayIndex >= 0);
  QVERIFY(meta->property(busyDelayIndex).isWritable());

  const int iconRefIndex = meta->indexOfProperty("iconRef");
  QVERIFY(iconRefIndex >= 0);
  const QMetaProperty iconRefProperty = meta->property(iconRefIndex);
  QVERIFY(iconRefProperty.isReadable());
  QVERIFY(iconRefProperty.isWritable());

  const int busyIconRefIndex = meta->indexOfProperty("busyIconRef");
  QVERIFY(busyIconRefIndex >= 0);
  QVERIFY(meta->property(busyIconRefIndex).isWritable());

  QCOMPARE(meta->indexOfProperty("controlSize"), -1);
  QCOMPARE(meta->indexOfProperty("iconPlacement"), -1);
  QCOMPARE(meta->indexOfProperty("busyIndicatorDelayMs"), -1);
  QCOMPARE(meta->indexOfProperty("glyph"), -1);
  QCOMPARE(meta->indexOfProperty("busyGlyph"), -1);
  QCOMPARE(meta->indexOfProperty("autoCjkSpacing"), -1);

  QVERIFY(button.setProperty("sizeClass",
                             QVariant::fromValue(adqt::widgets::AdButton::SizeClass::Large)));
  QCOMPARE(button.sizeClass(), adqt::widgets::AdButton::SizeClass::Large);

  QVERIFY(button.setProperty(
      "iconPosition",
      QVariant::fromValue(adqt::widgets::AdButton::IconPosition::Trailing)));
  QCOMPARE(button.iconPosition(), adqt::widgets::AdButton::IconPosition::Trailing);

  QVERIFY(button.setProperty("busyDelayMs", 120));
  QCOMPARE(button.busyDelayMs(), 120);

  const adqt::icons::IconRef iconRef = adqt::icons::antd::outlined::Search();
  QVERIFY(button.setProperty("iconRef", QVariant::fromValue(iconRef)));
  QVERIFY(iconRefsEqual(button.iconRef(), iconRef));

  const adqt::icons::IconRef loadingIconRef = adqt::icons::antd::outlined::Loading();
  QVERIFY(button.setProperty("busyIconRef", QVariant::fromValue(loadingIconRef)));
  QVERIFY(iconRefsEqual(button.busyIconRef(), loadingIconRef));
}

void ButtonTests::explicitMinimumHeightIsPreserved() {
  adqt::widgets::AdButton button("Submit");
  button.setMinimumHeight(60);

  button.setButtonStyle(adqt::widgets::AdButton::ButtonStyle::Solid);
  button.setAccentRole(adqt::widgets::AdButton::AccentRole::Primary);
  button.setSizeClass(adqt::widgets::AdButton::SizeClass::Small);
  button.setBusy(true);

  QCOMPARE(button.minimumHeight(), 60);
}

void ButtonTests::sizeHintRemainsPublic() {
  adqt::widgets::AdButton button("Submit");
  QVERIFY(button.sizeHint().isValid());
  QCOMPARE(button.minimumSizeHint(), button.sizeHint());
}

void ButtonTests::controlSizeUpdatesMetrics() {
  adqt::widgets::AdButton button("Submit");

  button.setSizeClass(adqt::widgets::AdButton::SizeClass::Small);
  const int smallHeight = button.sizeHint().height();
  button.adjustSize();
  const int smallRenderedHeight = button.height();

  button.setSizeClass(adqt::widgets::AdButton::SizeClass::Medium);
  const int mediumHeight = button.sizeHint().height();
  button.adjustSize();
  const int mediumRenderedHeight = button.height();

  button.setSizeClass(adqt::widgets::AdButton::SizeClass::Large);
  const int largeHeight = button.sizeHint().height();
  button.adjustSize();
  const int largeRenderedHeight = button.height();

  QCOMPARE(smallRenderedHeight, smallHeight);
  QCOMPARE(mediumRenderedHeight, mediumHeight);
  QCOMPARE(largeRenderedHeight, largeHeight);
  QVERIFY(smallHeight < mediumHeight);
  QVERIFY(mediumHeight < largeHeight);
}

void ButtonTests::cursorTracksInteractionState() {
  QWidget host;
  host.resize(240, 120);

  adqt::widgets::AdButton button("Submit", &host);
  button.move(24, 24);
  button.adjustSize();

  host.show();
  QTest::qWait(1);

  const QList<adqt::widgets::AdButton::ButtonStyle> defaultCursorVariants = {
      adqt::widgets::AdButton::ButtonStyle::Outline,
      adqt::widgets::AdButton::ButtonStyle::Dashed,
      adqt::widgets::AdButton::ButtonStyle::Solid,
      adqt::widgets::AdButton::ButtonStyle::Tonal,
      adqt::widgets::AdButton::ButtonStyle::Text,
      adqt::widgets::AdButton::ButtonStyle::Link,
  };

  for (const adqt::widgets::AdButton::ButtonStyle style : defaultCursorVariants) {
    button.setButtonStyle(style);
    QCOMPARE(button.cursor().shape(), Qt::PointingHandCursor);
  }

  button.setBusy(true);
  QCOMPARE(button.cursor().shape(), Qt::ArrowCursor);

  button.setEnabled(false);
  QCOMPARE(button.cursor().shape(), Qt::ForbiddenCursor);
  QTRY_VERIFY(findVisibleButtonCursorOverlay(&host) != nullptr);
  QCOMPARE(findVisibleButtonCursorOverlay(&host)->cursor().shape(), Qt::ForbiddenCursor);

  button.setEnabled(true);
  QCOMPARE(button.cursor().shape(), Qt::ArrowCursor);
  QTRY_VERIFY(findVisibleButtonCursorOverlay(&host) == nullptr);

  button.setButtonStyle(adqt::widgets::AdButton::ButtonStyle::Link);
  button.setBusy(true);
  QCOMPARE(button.cursor().shape(), Qt::ArrowCursor);

  button.setBusy(false);
  QCOMPARE(button.cursor().shape(), Qt::PointingHandCursor);

  button.setCursor(Qt::CrossCursor);
  QCOMPARE(button.cursor().shape(), Qt::CrossCursor);

  button.setBusy(true);
  QCOMPARE(button.cursor().shape(), Qt::CrossCursor);

  button.setEnabled(false);
  QCOMPARE(button.cursor().shape(), Qt::CrossCursor);
  QTRY_VERIFY(findVisibleButtonCursorOverlay(&host) != nullptr);
  QCOMPARE(findVisibleButtonCursorOverlay(&host)->cursor().shape(), Qt::CrossCursor);

  button.setEnabled(true);
  button.setBusy(false);
  button.unsetCursor();
  QCOMPARE(button.cursor().shape(), Qt::PointingHandCursor);

  button.setEnabled(false);
  button.unsetCursor();
  QCOMPARE(button.cursor().shape(), Qt::ForbiddenCursor);
  QTRY_VERIFY(findVisibleButtonCursorOverlay(&host) != nullptr);
  QCOMPARE(findVisibleButtonCursorOverlay(&host)->cursor().shape(), Qt::ForbiddenCursor);
}

void ButtonTests::circleShapeUsesRoundHitTarget() {
  QWidget host;
  host.resize(120, 80);

  adqt::widgets::AdButton button(&host);
  button.setShape(adqt::widgets::AdButton::Shape::Circle);
  button.move(10, 10);
  button.resize(40, 40);
  host.show();
  QTest::qWait(1);

  QVERIFY(button.hitButton(QPoint(20, 20)));
  QVERIFY(!button.hitButton(QPoint(0, 0)));

  QSignalSpy clickedSpy(&button, &QAbstractButton::clicked);
  QTest::mouseClick(&button, Qt::LeftButton, Qt::NoModifier, QPoint(20, 20));
  QCOMPARE(clickedSpy.count(), 1);

  button.setShape(adqt::widgets::AdButton::Shape::Rectangle);
  QVERIFY(button.hitButton(QPoint(0, 0)));
}

void ButtonTests::circleShapeFallsBackToPillForText() {
  QWidget host;
  host.resize(240, 120);

  adqt::widgets::AdButton button(QStringLiteral("Download"), &host);
  button.setShape(adqt::widgets::AdButton::Shape::Circle);

  const QSize hint = button.sizeHint();
  QVERIFY(hint.width() > hint.height());

  button.adjustSize();
  QVERIFY(button.width() > button.height());
  QVERIFY(button.hitButton(QPoint(0, 0)));
  QVERIFY(button.hitButton(button.rect().center()));
}

void ButtonTests::menuButtonReservesIndicatorSpace() {
  QWidget host;
  host.resize(320, 120);

  adqt::widgets::AdButton plain(QStringLiteral("Actions"), &host);
  adqt::widgets::AdButton withMenu(QStringLiteral("Actions"), &host);
  QMenu menu(&withMenu);
  menu.addAction(QStringLiteral("Rename"));
  withMenu.setMenu(&menu);

  plain.move(16, 24);
  withMenu.move(160, 24);
  plain.adjustSize();
  withMenu.adjustSize();
  host.show();
  QTest::qWait(1);

  QVERIFY(withMenu.sizeHint().width() > plain.sizeHint().width());

  QSignalSpy aboutToShowSpy(&menu, &QMenu::aboutToShow);
  QTest::mouseClick(&withMenu, Qt::LeftButton, Qt::NoModifier, withMenu.rect().center());
  QTRY_COMPARE(aboutToShowSpy.count(), 1);
  menu.hide();
}

void ButtonTests::accessibleNameIsExplicitOnly() {
  adqt::widgets::AdButton button(QStringLiteral("&Save"));
  QCOMPARE(button.accessibleName(), QString());

  button.setToolTip(QStringLiteral("Run action"));
  QApplication::processEvents();
  QCOMPARE(button.accessibleName(), QString());

  button.setAccessibleName(QStringLiteral("Confirm save"));
  QCOMPARE(button.accessibleName(), QStringLiteral("Confirm save"));

  button.setText(QStringLiteral("&Commit"));
  QCOMPARE(button.accessibleName(), QStringLiteral("Confirm save"));

  button.setAccessibleName(QString());
  QCOMPARE(button.accessibleName(), QString());

  adqt::widgets::AdButton iconOnly;
  QCOMPARE(iconOnly.accessibleName(), QString());
}

void ButtonTests::loadingBlocksActivation() {
  adqt::widgets::AdButton button("Submit");
  button.adjustSize();
  button.show();
  QTest::qWait(1);
  QVERIFY(button.isVisible());

  QSignalSpy clickedSpy(&button, &QAbstractButton::clicked);

  button.setBusy(true);
  button.setFocus();
  QTest::mouseClick(&button, Qt::LeftButton, Qt::NoModifier, button.rect().center());
  QTest::keyClick(&button, Qt::Key_Space);
  QCOMPARE(clickedSpy.count(), 0);

  button.setBusy(false);
  button.setFocus();
  QTest::mouseClick(&button, Qt::LeftButton, Qt::NoModifier, button.rect().center());
  QTest::keyClick(&button, Qt::Key_Space);
  QCOMPARE(clickedSpy.count(), 2);
}

void ButtonTests::loadingSuspendsDefaultDialogActivation() {
  QDialog dialog;
  dialog.resize(240, 120);
  QVBoxLayout layout(&dialog);

  QLineEdit editor(&dialog);
  adqt::widgets::AdButton button(QStringLiteral("Submit"), &dialog);
  layout.addWidget(&editor);
  layout.addWidget(&button);

  QPushButton* baseButton = &button;
  baseButton->setDefault(true);
  QVERIFY(button.isDefault());

  button.setBusy(true);
  QVERIFY(!button.isDefault());

  dialog.show();
  QTest::qWait(1);
  editor.setFocus();
  QApplication::processEvents();

  QSignalSpy clickedSpy(&button, &QAbstractButton::clicked);
  sendReturnKey(&dialog);
  QCOMPARE(clickedSpy.count(), 0);

  button.setBusy(false);
  QVERIFY(button.isDefault());

  sendReturnKey(&dialog);
  QCOMPARE(clickedSpy.count(), 1);
}

void ButtonTests::clickActivationShowsInteractionWave() {
  QWidget host;
  host.resize(220, 120);
  QWidget focusSink(&host);
  focusSink.setFocusPolicy(Qt::StrongFocus);
  focusSink.setGeometry(0, 0, 1, 1);
  focusSink.show();

  adqt::widgets::AdButton button(QStringLiteral("Submit"), &host);
  button.setButtonStyle(adqt::widgets::AdButton::ButtonStyle::Solid);
  button.setAccentRole(adqt::widgets::AdButton::AccentRole::Primary);
  button.move(24, 24);
  button.adjustSize();

  host.show();
  QTest::qWait(1);
  QVERIFY(button.isVisible());
  focusSink.setFocus(Qt::MouseFocusReason);
  QApplication::processEvents();
  QTRY_VERIFY(!hasVisibleInteractionOverlay(&host));

  QTest::mouseClick(&button, Qt::LeftButton, Qt::NoModifier, button.rect().center());
  QTRY_VERIFY(hasVisibleInteractionOverlay(&host));
  QTRY_VERIFY(!hasVisibleInteractionOverlay(&host));

  button.setFocus(Qt::MouseFocusReason);
  QApplication::processEvents();
  QTRY_VERIFY(!hasVisibleInteractionOverlay(&host));

  QTest::keyClick(&button, Qt::Key_Space);
  QTRY_VERIFY(hasVisibleInteractionOverlay(&host));
  QTRY_VERIFY(!hasVisibleInteractionOverlay(&host));
}

void ButtonTests::textAndLinkSkipInteractionWave() {
  QWidget host;
  host.resize(260, 140);
  QWidget focusSink(&host);
  focusSink.setFocusPolicy(Qt::StrongFocus);
  focusSink.setGeometry(0, 0, 1, 1);
  focusSink.show();

  const QList<adqt::widgets::AdButton::ButtonStyle> noWaveVariants = {
      adqt::widgets::AdButton::ButtonStyle::Text,
      adqt::widgets::AdButton::ButtonStyle::Link,
  };

  int y = 24;
  for (const adqt::widgets::AdButton::ButtonStyle style : noWaveVariants) {
    adqt::widgets::AdButton button(QStringLiteral("Action"), &host);
    button.setButtonStyle(style);
    button.move(24, y);
    button.adjustSize();
    button.show();
    y += button.height() + 12;

    host.show();
    QTest::qWait(1);
    QVERIFY(button.isVisible());

    focusSink.setFocus(Qt::MouseFocusReason);
    QApplication::processEvents();
    QTRY_VERIFY(!hasVisibleInteractionOverlay(&host));

    QTest::mouseClick(&button, Qt::LeftButton, Qt::NoModifier, button.rect().center());
    QTest::qWait(50);
    QVERIFY(!hasVisibleInteractionOverlay(&host));

    button.setFocus(Qt::MouseFocusReason);
    QApplication::processEvents();
    QTest::keyClick(&button, Qt::Key_Space);
    QTest::qWait(50);
    QVERIFY(!hasVisibleInteractionOverlay(&host));
  }
}

void ButtonTests::searchEditTracksButtonContract() {
  adqt::widgets::AdSearchEdit search;
  auto* lineEdit = search.lineEdit();
  auto* button = search.findChild<adqt::widgets::AdButton*>();

  QVERIFY(lineEdit);
  QVERIFY(button);
  QCOMPARE(adqt::widgets::detail::buttonSegmentPosition(button),
           adqt::widgets::detail::SegmentPosition::Trailing);
  QVERIFY(lineEdit->joinedRight());

  search.setControlSize(adqt::widgets::AdSearchEdit::ControlSize::Large);
  QCOMPARE(button->sizeClass(), adqt::widgets::AdButton::SizeClass::Large);

  search.setVariant(adqt::widgets::AdSearchEdit::Variant::Filled);
  search.setSearchButtonText(QStringLiteral("Go"));
  QCOMPARE(button->text(), QStringLiteral("Go"));
  QCOMPARE(button->buttonStyle(), adqt::widgets::AdButton::ButtonStyle::Text);
  QCOMPARE(button->accentRole(), adqt::widgets::AdButton::AccentRole::Primary);

  search.setSearchButtonText(QString());
  QCOMPARE(button->text(), QString());
  QCOMPARE(button->buttonStyle(), adqt::widgets::AdButton::ButtonStyle::Tonal);

  search.setVariant(adqt::widgets::AdSearchEdit::Variant::Borderless);
  QCOMPARE(button->buttonStyle(), adqt::widgets::AdButton::ButtonStyle::Text);
}

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  qRegisterMetaType<adqt::widgets::AdAlert::CloseReason>();
  qRegisterMetaType<adqt::widgets::AdModal::ClosePolicy>();
  qRegisterMetaType<adqt::widgets::AdModal::DialogCode>();
  qRegisterMetaType<adqt::widgets::AdModal::CloseReason>();
  qRegisterMetaType<adqt::widgets::AdModal::StandardButtons>();
  qRegisterMetaType<adqt::widgets::AdColorSelection>();
  qRegisterMetaType<adqt::widgets::AdColorValue>();
  qRegisterMetaType<adqt::widgets::AdColorGradientStop>();
  qRegisterMetaType<adqt::widgets::AdColorPicker::PresetItem>();
  qRegisterMetaType<QVector<adqt::widgets::AdColorPicker::Mode>>();
  qRegisterMetaType<QVector<adqt::widgets::AdColorPicker::PresetItem>>();
  qRegisterMetaType<adqt::widgets::AdPopupPlacement>();
  qRegisterMetaType<adqt::widgets::AdPopupTrigger>();
  qRegisterMetaType<adqt::widgets::AdPopupTriggers>();
  qRegisterMetaType<adqt::widgets::AdPopupActivationMode>();
  qRegisterMetaType<adqt::widgets::AdPopupLifetime>();
  qRegisterMetaType<adqt::widgets::AdInputNumber::StepButtonLayout>();
  qRegisterMetaType<adqt::widgets::AdInputNumber::ValueMode>();
  qRegisterMetaType<adqt::widgets::AdInputNumber::RangeMode>();
  qRegisterMetaType<adqt::widgets::AdInputNumber::StepType>();
  qRegisterMetaType<adqt::widgets::AdInputNumber::StepEmitter>();
  qRegisterMetaType<adqt::widgets::AdDisabledTimeSpec>();
  qRegisterMetaType<adqt::widgets::AdDateTimeRangeValue>();
  qRegisterMetaType<adqt::widgets::AdDatePresetItem>();
  qRegisterMetaType<adqt::widgets::AdDateRangePresetItem>();
  qRegisterMetaType<adqt::widgets::AdDateTimePanelOptions>();
  qRegisterMetaType<QVector<QDateTime>>();
  qRegisterMetaType<QVector<adqt::widgets::AdDatePresetItem>>();
  qRegisterMetaType<QVector<adqt::widgets::AdDateRangePresetItem>>();
  qRegisterMetaType<QAbstractSlider::SliderAction>();

  QObject* alertTests = createAlertTests();
  QObject* inputTests = createInputTests();
  QObject* inputNumberTests = createInputNumberTests();
  QObject* modalTests = createModalTests();
  QObject* popupTests = createPopupTests();
  QObject* sliderTests = createSliderTests();
  QObject* colorPickerTests = createColorPickerTests();
  ButtonTests buttonTests;

  int status = 0;
  status |= QTest::qExec(alertTests, argc, argv);
  status |= QTest::qExec(inputTests, argc, argv);
  status |= QTest::qExec(inputNumberTests, argc, argv);
  status |= QTest::qExec(modalTests, argc, argv);
  status |= QTest::qExec(popupTests, argc, argv);
  status |= QTest::qExec(sliderTests, argc, argv);
  status |= QTest::qExec(&buttonTests, argc, argv);
  status |= QTest::qExec(colorPickerTests, argc, argv);
  status |= runDatePickerTests(argc, argv);
  status |= runImageTests(argc, argv);
  status |= runMenuTests(argc, argv);
  status |= runRadioTests(argc, argv);
  status |= runSelectTests(argc, argv);
  status |= runSwitchTests(argc, argv);
  status |= runTagTests(argc, argv);
  delete alertTests;
  delete inputTests;
  delete inputNumberTests;
  delete modalTests;
  delete popupTests;
  delete sliderTests;
  delete colorPickerTests;
  return status;
}

#include "button_tests.moc"
