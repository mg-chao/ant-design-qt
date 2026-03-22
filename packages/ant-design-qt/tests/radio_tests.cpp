#include <QAccessible>
#include <QApplication>
#include <QColor>
#include <QHBoxLayout>
#include <QImage>
#include <QMetaProperty>
#include <QSignalSpy>
#include <QVBoxLayout>
#include <QtTest>

#include "widgets/radio.h"
#include "widgets/radio_button_group.h"

namespace {

using adqt::widgets::AdRadio;
using adqt::widgets::AdRadioButtonGroup;

struct ManagedRadioGroup {
  QWidget host;
  QBoxLayout* layout = nullptr;
  AdRadioButtonGroup controller;

  explicit ManagedRadioGroup(Qt::Orientation orientation = Qt::Horizontal) : controller(&host) {
    if (orientation == Qt::Vertical) {
      layout = new QVBoxLayout(&host);
    } else {
      layout = new QHBoxLayout(&host);
    }
    layout->setContentsMargins(0, 0, 0, 0);
    controller.setManagedLayout(layout);
  }

  AdRadio* addRadio(const QString& text, int id = -1) {
    auto* radio = new AdRadio(text, &host);
    layout->addWidget(radio);
    controller.addButton(radio, id);
    return radio;
  }
};

void showAndWait(QWidget* widget) {
  if (!widget) {
    return;
  }
  widget->show();
  QTest::qWait(1);
  QCoreApplication::processEvents();
}

QColor sampleCenterColor(QWidget* widget) {
  const QImage image = widget->grab().toImage();
  return image.pixelColor(image.width() / 2, image.height() / 2);
}

}  // namespace

class RadioTests final : public QObject {
  Q_OBJECT

 private slots:
  void exposesQtStyleProperties();
  void siblingRadiosRemainExclusive();
  void controllerTracksIdsAndCheckedState();
  void keyboardNavigationSkipsDisabledAndHidden();
  void disabledBlocksMouseAndKeyboardInteraction();
  void buttonVariantDistributionAndControlSizeAffectLayout();
  void joinedButtonLayoutReflowsAroundHiddenRadios();
  void localOverridesResetBackToControllerDefaults();
  void componentTokensAndResolverAffectRendering();
  void explicitCursorOverrideIsPreserved();
  void accessibilityReflectsQtRadioContract();
};

void RadioTests::exposesQtStyleProperties() {
  AdRadio radio;
  const QMetaObject* radioMeta = radio.metaObject();

  const int radioControlSizeIndex = radioMeta->indexOfProperty("controlSize");
  QVERIFY(radioControlSizeIndex >= 0);
  const QMetaProperty radioControlSizeProperty = radioMeta->property(radioControlSizeIndex);
  QVERIFY(radioControlSizeProperty.isReadable());
  QVERIFY(radioControlSizeProperty.isWritable());

  QVERIFY(radio.setProperty("controlSize", QVariant::fromValue(AdRadio::ControlSize::Small)));
  QCOMPARE(radio.controlSize(), AdRadio::ControlSize::Small);
  QVERIFY(radio.hasControlSizeOverride());

  AdRadioButtonGroup group;
  const QMetaObject* groupMeta = group.metaObject();

  const int groupControlSizeIndex = groupMeta->indexOfProperty("controlSize");
  QVERIFY(groupControlSizeIndex >= 0);
  const QMetaProperty groupControlSizeProperty = groupMeta->property(groupControlSizeIndex);
  QVERIFY(groupControlSizeProperty.isReadable());
  QVERIFY(groupControlSizeProperty.isWritable());

  const int groupDistributionIndex = groupMeta->indexOfProperty("distribution");
  QVERIFY(groupDistributionIndex >= 0);
  const QMetaProperty groupDistributionProperty = groupMeta->property(groupDistributionIndex);
  QVERIFY(groupDistributionProperty.isReadable());
  QVERIFY(groupDistributionProperty.isWritable());

  QVERIFY(group.setProperty("controlSize", QVariant::fromValue(AdRadio::ControlSize::Large)));
  QCOMPARE(group.controlSize(), AdRadio::ControlSize::Large);
}

void RadioTests::siblingRadiosRemainExclusive() {
  QWidget host;
  auto* layout = new QHBoxLayout(&host);
  layout->setContentsMargins(0, 0, 0, 0);

  auto* first = new AdRadio(QStringLiteral("Alpha"), &host);
  auto* second = new AdRadio(QStringLiteral("Beta"), &host);
  auto* third = new AdRadio(QStringLiteral("Gamma"), &host);
  first->setChecked(true);

  layout->addWidget(first);
  layout->addWidget(second);
  layout->addWidget(third);

  showAndWait(&host);
  QVERIFY(host.isVisible());

  QTest::mouseClick(second, Qt::LeftButton, Qt::NoModifier, second->rect().center());

  QTRY_VERIFY(second->isChecked());
  QVERIFY(!first->isChecked());
  QVERIFY(!third->isChecked());
}

void RadioTests::controllerTracksIdsAndCheckedState() {
  ManagedRadioGroup group;
  auto* first = group.addRadio(QStringLiteral("First"), 10);
  auto* second = group.addRadio(QStringLiteral("Second"));
  auto* third = new AdRadio(QStringLiteral("Third"), &group.host);
  group.layout->insertWidget(1, third);
  group.controller.addButton(third, 30);

  QCOMPARE(group.layout->indexOf(first), 0);
  QCOMPARE(group.layout->indexOf(third), 1);
  QCOMPARE(group.layout->indexOf(second), 2);
  QCOMPARE(group.controller.id(first), 10);
  QCOMPARE(group.controller.id(third), 30);
  QCOMPARE(group.controller.id(second), -2);

  group.controller.setCheckedId(30);
  QCOMPARE(group.controller.checkedId(), 30);
  QCOMPARE(group.controller.checkedRadio(), third);

  group.controller.setId(second, 40);
  QCOMPARE(group.controller.id(second), 40);

  group.controller.setCheckedId(10);
  QCOMPARE(group.controller.checkedRadio(), first);
  group.controller.setId(first, 15);
  QCOMPARE(group.controller.checkedId(), 15);
  QCOMPARE(group.controller.id(first), 15);

  QSignalSpy checkedSpy(&group.controller, &AdRadioButtonGroup::checkedIdChanged);
  group.controller.removeButton(first);
  QCOMPARE(group.controller.checkedId(), -1);
  QVERIFY(group.controller.checkedRadio() == nullptr);
  QVERIFY(!group.controller.buttons().contains(first));
  QVERIFY(checkedSpy.count() >= 1);

  group.layout->removeWidget(first);
  delete first;
}

void RadioTests::keyboardNavigationSkipsDisabledAndHidden() {
  ManagedRadioGroup group;
  auto* first = group.addRadio(QStringLiteral("A"), 1);
  auto* second = group.addRadio(QStringLiteral("B"), 2);
  auto* third = group.addRadio(QStringLiteral("C"), 3);
  auto* fourth = group.addRadio(QStringLiteral("D"), 4);

  second->setDisabled(true);
  third->hide();
  group.controller.setCheckedId(1);

  showAndWait(&group.host);
  QVERIFY(group.host.isVisible());

  first->setFocus(Qt::TabFocusReason);
  QVERIFY(first->hasFocus());

  QTest::keyClick(first, Qt::Key_Right);

  QTRY_COMPARE(group.controller.checkedId(), 4);
  QVERIFY(fourth->hasFocus());
}

void RadioTests::disabledBlocksMouseAndKeyboardInteraction() {
  AdRadio radio(QStringLiteral("Disabled"));
  radio.setDisabled(true);
  showAndWait(&radio);
  QVERIFY(radio.isVisible());

  QSignalSpy clickedSpy(&radio, &QAbstractButton::clicked);

  QTest::mouseClick(&radio, Qt::LeftButton, Qt::NoModifier, radio.rect().center());
  QTest::ignoreMessage(QtWarningMsg, "Keyboard event not accepted by receiving widget");
  QTest::ignoreMessage(QtWarningMsg, "Keyboard event not accepted by receiving widget");
  QTest::keyClick(&radio, Qt::Key_Space);

  QCOMPARE(clickedSpy.count(), 0);
  QVERIFY(!radio.isChecked());
}

void RadioTests::buttonVariantDistributionAndControlSizeAffectLayout() {
  ManagedRadioGroup group;
  group.controller.setVariant(AdRadio::Variant::Button);
  group.controller.setDistribution(AdRadioButtonGroup::Distribution::Fill);
  group.controller.setControlSize(AdRadio::ControlSize::Large);
  group.host.setFixedWidth(320);

  auto* first = group.addRadio(QStringLiteral("Alpha"), 1);
  auto* second = group.addRadio(QStringLiteral("Beta"), 2);
  group.controller.setCheckedId(1);

  showAndWait(&group.host);

  QCOMPARE(first->sizePolicy().horizontalPolicy(), QSizePolicy::Expanding);
  QCOMPARE(second->sizePolicy().horizontalPolicy(), QSizePolicy::Expanding);
  QVERIFY(first->width() > first->sizeHint().width());
  QVERIFY(second->width() > second->sizeHint().width());
  QVERIFY(second->x() < first->geometry().right());

  ManagedRadioGroup verticalGroup(Qt::Vertical);
  verticalGroup.controller.setVariant(AdRadio::Variant::Button);
  verticalGroup.controller.setDistribution(AdRadioButtonGroup::Distribution::Fill);
  verticalGroup.controller.setControlSize(AdRadio::ControlSize::Large);
  verticalGroup.host.setFixedWidth(320);
  auto* top = verticalGroup.addRadio(QStringLiteral("Top"), 1);
  auto* bottom = verticalGroup.addRadio(QStringLiteral("Bottom"), 2);
  verticalGroup.controller.setCheckedId(1);

  showAndWait(&verticalGroup.host);
  QTRY_VERIFY(bottom->y() > top->y());
  QCOMPARE(verticalGroup.controller.controlSize(), AdRadio::ControlSize::Large);
}

void RadioTests::joinedButtonLayoutReflowsAroundHiddenRadios() {
  ManagedRadioGroup group;
  group.controller.setVariant(AdRadio::Variant::Button);
  group.host.setFixedWidth(360);

  auto* first = group.addRadio(QStringLiteral("Alpha"), 1);
  auto* second = group.addRadio(QStringLiteral("Beta"), 2);
  auto* third = group.addRadio(QStringLiteral("Gamma"), 3);
  Q_UNUSED(first)

  showAndWait(&group.host);
  QVERIFY(group.host.isVisible());

  const int thirdXWithMiddleVisible = third->x();
  second->hide();
  QCoreApplication::processEvents();

  QTRY_VERIFY(third->x() < thirdXWithMiddleVisible);
}

void RadioTests::localOverridesResetBackToControllerDefaults() {
  ManagedRadioGroup group;
  group.controller.setVariant(AdRadio::Variant::Button);
  group.controller.setButtonStyle(AdRadio::ButtonStyle::Solid);
  group.controller.setControlSize(AdRadio::ControlSize::Large);
  group.controller.setDistribution(AdRadioButtonGroup::Distribution::Fill);

  auto* radio = group.addRadio(QStringLiteral("Alpha"), 1);
  radio->setControlSize(AdRadio::ControlSize::Small);
  radio->setVariant(AdRadio::Variant::Default);
  radio->setButtonStyle(AdRadio::ButtonStyle::Outline);

  showAndWait(&group.host);

  QCOMPARE(radio->controlSize(), AdRadio::ControlSize::Small);
  QCOMPARE(radio->variant(), AdRadio::Variant::Default);
  QCOMPARE(radio->buttonStyle(), AdRadio::ButtonStyle::Outline);
  QVERIFY(radio->hasControlSizeOverride());
  QVERIFY(radio->hasVariantOverride());
  QVERIFY(radio->hasButtonStyleOverride());
  QCOMPARE(radio->sizePolicy().horizontalPolicy(), QSizePolicy::Expanding);

  radio->resetControlSize();
  radio->resetVariant();
  radio->resetButtonStyle();

  QCOMPARE(radio->controlSize(), AdRadio::ControlSize::Large);
  QCOMPARE(radio->variant(), AdRadio::Variant::Button);
  QCOMPARE(radio->buttonStyle(), AdRadio::ButtonStyle::Solid);
  QVERIFY(!radio->hasControlSizeOverride());
  QVERIFY(!radio->hasVariantOverride());
  QVERIFY(!radio->hasButtonStyleOverride());

  group.controller.removeButton(radio);
  QCOMPARE(radio->sizePolicy().horizontalPolicy(), QSizePolicy::Preferred);
  QCOMPARE(radio->controlSize(), AdRadio::ControlSize::Medium);
  QCOMPARE(radio->variant(), AdRadio::Variant::Default);
  QCOMPARE(radio->buttonStyle(), AdRadio::ButtonStyle::Outline);
}

void RadioTests::componentTokensAndResolverAffectRendering() {
  ManagedRadioGroup group;
  group.controller.setVariant(AdRadio::Variant::Button);
  group.controller.setButtonStyle(AdRadio::ButtonStyle::Solid);

  auto* radio = group.addRadio(QString(), 1);
  group.controller.setCheckedId(1);

  showAndWait(&group.host);
  QVERIFY(group.host.isVisible());

  const QColor baseColor = sampleCenterColor(radio);

  AdRadioButtonGroup::ComponentTokens tokens;
  tokens.colors.buttonFillColor = QColor(QStringLiteral("#ff4d4f"));
  tokens.colors.buttonBorderColor = QColor(QStringLiteral("#ff4d4f"));
  group.controller.setComponentTokens(tokens);
  QTest::qWait(1);
  const QColor overrideColor = sampleCenterColor(radio);

  QCOMPARE(overrideColor, QColor(QStringLiteral("#ff4d4f")));
  QVERIFY(baseColor != overrideColor);

  group.controller.resetComponentTokens();
  group.controller.setComponentTokenResolver([](const AdRadio::ComponentTokenContext& state) {
    AdRadioButtonGroup::ComponentTokens resolved;
    if (state.checked) {
      resolved.colors.buttonFillColor = QColor(QStringLiteral("#52c41a"));
      resolved.colors.buttonBorderColor = QColor(QStringLiteral("#52c41a"));
    }
    return resolved;
  });
  QTest::qWait(1);
  const QColor resolvedColor = sampleCenterColor(radio);

  QCOMPARE(resolvedColor, QColor(QStringLiteral("#52c41a")));
  QVERIFY(resolvedColor != overrideColor);
}

void RadioTests::explicitCursorOverrideIsPreserved() {
  AdRadio radio(QStringLiteral("Cursor"));
  showAndWait(&radio);
  QVERIFY(radio.isVisible());

  QCOMPARE(radio.cursor().shape(), Qt::PointingHandCursor);

  radio.setCursor(Qt::WaitCursor);
  QCOMPARE(radio.cursor().shape(), Qt::WaitCursor);

  radio.setDisabled(true);
  QCOMPARE(radio.cursor().shape(), Qt::WaitCursor);

  radio.setDisabled(false);
  QCOMPARE(radio.cursor().shape(), Qt::WaitCursor);
}

void RadioTests::accessibilityReflectsQtRadioContract() {
  AdRadio radio(QStringLiteral("&Power"));
  QAccessibleInterface* iface = QAccessible::queryAccessibleInterface(&radio);
  QVERIFY(iface);
  QCOMPARE(iface->role(), QAccessible::RadioButton);

  QAccessible::State state = iface->state();
  QVERIFY(state.checkable);
  QVERIFY(!state.checked);

  radio.setChecked(true);
  state = iface->state();
  QVERIFY(state.checked);
}

int runRadioTests(int argc, char** argv) {
  qRegisterMetaType<AdRadio::ControlSize>();
  qRegisterMetaType<AdRadio::Variant>();
  qRegisterMetaType<AdRadio::ButtonStyle>();
  qRegisterMetaType<AdRadioButtonGroup::Distribution>();

  RadioTests tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "radio_tests.moc"
