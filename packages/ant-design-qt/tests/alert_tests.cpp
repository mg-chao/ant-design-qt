#include <QAccessible>
#include <QCoreApplication>
#include <QEvent>
#include <QImage>
#include <QLabel>
#include <QPointer>
#include <QSignalSpy>
#include <QToolButton>
#include <QtTest>

#include "widgets/alert.h"

class AlertTests final : public QObject {
  Q_OBJECT

 private slots:
  void exposesQtFirstProperties();
  void closeButtonUsesCloseLifecycle();
  void programmaticCloseUsesProgrammaticReason();
  void hideDoesNotEmitCloseSignals();
  void deleteOnCloseWorksWithAnimation();
  void hostedWidgetsTransferOwnership();
  void externallyDestroyedHostedWidgetsClearState();
  void accessibleTextFallsBackToHostedAndPlainText();
  void inheritedPaletteOverridesApplyWithoutLocalPalette();
  void inheritedFontChangesRefreshVisuals();
  void bannerAutoIconFollowsDisplayMode();
};

void AlertTests::exposesQtFirstProperties() {
  adqt::widgets::AdAlert alert;
  const QMetaObject* meta = alert.metaObject();

  const int textIndex = meta->indexOfProperty("text");
  QVERIFY(textIndex >= 0);
  QVERIFY(meta->property(textIndex).isWritable());

  const int informativeIndex = meta->indexOfProperty("informativeText");
  QVERIFY(informativeIndex >= 0);
  QVERIFY(meta->property(informativeIndex).isWritable());

  const int animatedIndex = meta->indexOfProperty("animated");
  QVERIFY(animatedIndex >= 0);
  QVERIFY(meta->property(animatedIndex).isWritable());

  const int displayModeIndex = meta->indexOfProperty("displayMode");
  QVERIFY(displayModeIndex >= 0);
  QVERIFY(meta->property(displayModeIndex).isWritable());

  const int closableIndex = meta->indexOfProperty("closable");
  QVERIFY(closableIndex >= 0);
  QVERIFY(meta->property(closableIndex).isWritable());

  const int leadingWidgetIndex = meta->indexOfProperty("leadingWidget");
  QVERIFY(leadingWidgetIndex >= 0);
  QVERIFY(meta->property(leadingWidgetIndex).isWritable());

  const int textWidgetIndex = meta->indexOfProperty("textWidget");
  QVERIFY(textWidgetIndex >= 0);
  QVERIFY(meta->property(textWidgetIndex).isWritable());

  const int informativeWidgetIndex = meta->indexOfProperty("informativeWidget");
  QVERIFY(informativeWidgetIndex >= 0);
  QVERIFY(meta->property(informativeWidgetIndex).isWritable());

  const int actionsWidgetIndex = meta->indexOfProperty("actionsWidget");
  QVERIFY(actionsWidgetIndex >= 0);
  QVERIFY(meta->property(actionsWidgetIndex).isWritable());

  QCOMPARE(meta->indexOfProperty("iconRef"), -1);
  QCOMPARE(meta->indexOfProperty("dismissIconRef"), -1);

  QVERIFY(alert.setProperty("animated", false));
  QVERIFY(!alert.animated());

  QVERIFY(alert.setProperty("displayMode",
                            QVariant::fromValue(adqt::widgets::AdAlert::DisplayMode::Banner)));
  QCOMPARE(alert.displayMode(), adqt::widgets::AdAlert::DisplayMode::Banner);

  auto* propertyWidget = new QLabel(QStringLiteral("Property widget"));
  QVERIFY(alert.setProperty("textWidget", QVariant::fromValue(static_cast<QWidget*>(propertyWidget))));
  QCOMPARE(alert.textWidget(), propertyWidget);
}

void AlertTests::closeButtonUsesCloseLifecycle() {
  adqt::widgets::AdAlert alert;
  alert.setText(QStringLiteral("Closeable"));
  alert.setClosable(true);
  alert.setAnimated(false);
  alert.show();
  QTest::qWait(1);

  auto* closeButton = alert.findChild<QToolButton*>(QStringLiteral("ad-alert-close"));
  QVERIFY(closeButton);
  QVERIFY(closeButton->isVisible());

  QSignalSpy requestedSpy(&alert, &adqt::widgets::AdAlert::closeRequested);
  QSignalSpy closedSpy(&alert, &adqt::widgets::AdAlert::closed);

  QTest::mouseClick(closeButton, Qt::LeftButton);

  QTRY_COMPARE(requestedSpy.count(), 1);
  QTRY_COMPARE(closedSpy.count(), 1);
  QVERIFY(!alert.isVisible());

  QCOMPARE(qvariant_cast<adqt::widgets::AdAlert::CloseReason>(requestedSpy.at(0).at(0)),
           adqt::widgets::AdAlert::CloseReason::CloseButton);
  QCOMPARE(qvariant_cast<adqt::widgets::AdAlert::CloseReason>(closedSpy.at(0).at(0)),
           adqt::widgets::AdAlert::CloseReason::CloseButton);
}

void AlertTests::programmaticCloseUsesProgrammaticReason() {
  adqt::widgets::AdAlert alert;
  alert.setText(QStringLiteral("Programmatic"));
  alert.setClosable(true);
  alert.setAnimated(false);
  alert.show();
  QTest::qWait(1);

  QSignalSpy requestedSpy(&alert, &adqt::widgets::AdAlert::closeRequested);
  QSignalSpy closedSpy(&alert, &adqt::widgets::AdAlert::closed);

  QVERIFY(alert.close());

  QTRY_COMPARE(requestedSpy.count(), 1);
  QTRY_COMPARE(closedSpy.count(), 1);
  QVERIFY(!alert.isVisible());

  QCOMPARE(qvariant_cast<adqt::widgets::AdAlert::CloseReason>(requestedSpy.at(0).at(0)),
           adqt::widgets::AdAlert::CloseReason::Programmatic);
  QCOMPARE(qvariant_cast<adqt::widgets::AdAlert::CloseReason>(closedSpy.at(0).at(0)),
           adqt::widgets::AdAlert::CloseReason::Programmatic);
}

void AlertTests::hideDoesNotEmitCloseSignals() {
  adqt::widgets::AdAlert alert;
  alert.setText(QStringLiteral("Manual hide"));
  alert.setClosable(true);
  alert.show();
  QTest::qWait(1);

  QSignalSpy requestedSpy(&alert, &adqt::widgets::AdAlert::closeRequested);
  QSignalSpy closedSpy(&alert, &adqt::widgets::AdAlert::closed);

  alert.hide();
  QTest::qWait(20);

  QCOMPARE(requestedSpy.count(), 0);
  QCOMPARE(closedSpy.count(), 0);
}

void AlertTests::deleteOnCloseWorksWithAnimation() {
  auto* rawAlert = new adqt::widgets::AdAlert();
  rawAlert->setText(QStringLiteral("Animated close"));
  rawAlert->setClosable(true);
  rawAlert->setAttribute(Qt::WA_DeleteOnClose, true);
  rawAlert->show();
  QTest::qWait(1);

  QPointer<adqt::widgets::AdAlert> alert(rawAlert);
  QSignalSpy requestedSpy(rawAlert, &adqt::widgets::AdAlert::closeRequested);
  QSignalSpy closedSpy(rawAlert, &adqt::widgets::AdAlert::closed);
  QSignalSpy destroyedSpy(rawAlert, &QObject::destroyed);

  rawAlert->close();

  QTRY_VERIFY(alert.isNull());
  QCOMPARE(requestedSpy.count(), 1);
  QCOMPARE(closedSpy.count(), 1);
  QCOMPARE(destroyedSpy.count(), 1);
}

void AlertTests::hostedWidgetsTransferOwnership() {
  adqt::widgets::AdAlert alert;
  QPointer<QLabel> first = new QLabel(QStringLiteral("First"));
  alert.setTextWidget(first);
  QVERIFY(first);
  QVERIFY(first->parentWidget() != nullptr);

  QPointer<QLabel> second = new QLabel(QStringLiteral("Second"));
  alert.setTextWidget(second);
  QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
  QTRY_VERIFY(first.isNull());
  QVERIFY(second);
  QVERIFY(second->parentWidget() != nullptr);

  QWidget* taken = alert.takeTextWidget();
  QCOMPARE(taken, second.data());
  QCOMPARE(taken->parentWidget(), nullptr);
  delete taken;
}

void AlertTests::externallyDestroyedHostedWidgetsClearState() {
  adqt::widgets::AdAlert alert;
  auto* hostedText = new QLabel(QStringLiteral("Hosted body"));
  hostedText->setAccessibleName(QStringLiteral("Hosted name"));
  auto* hostedActions = new QLabel(QStringLiteral("Hosted action"));

  alert.setTextWidget(hostedText);
  alert.setActionsWidget(hostedActions);
  alert.show();
  QTest::qWait(1);

  QAccessibleInterface* accessible = QAccessible::queryAccessibleInterface(&alert);
  QVERIFY(accessible);
  QCOMPARE(accessible->text(QAccessible::Name), QStringLiteral("Hosted name"));

  QSignalSpy textWidgetChangedSpy(&alert, &adqt::widgets::AdAlert::textWidgetChanged);
  QSignalSpy actionsWidgetChangedSpy(&alert, &adqt::widgets::AdAlert::actionsWidgetChanged);

  delete hostedText;
  delete hostedActions;

  QTRY_COMPARE(textWidgetChangedSpy.count(), 1);
  QTRY_COMPARE(actionsWidgetChangedSpy.count(), 1);
  QCOMPARE(alert.textWidget(), nullptr);
  QCOMPARE(alert.actionsWidget(), nullptr);
  QCOMPARE(accessible->text(QAccessible::Name), QString());
  QCOMPARE(textWidgetChangedSpy.at(0).at(0).value<QWidget*>(), nullptr);
  QCOMPARE(actionsWidgetChangedSpy.at(0).at(0).value<QWidget*>(), nullptr);
}

void AlertTests::accessibleTextFallsBackToHostedAndPlainText() {
  adqt::widgets::AdAlert plainAlert;
  plainAlert.setText(QStringLiteral("Plain name"));
  plainAlert.setInformativeText(QStringLiteral("Plain description"));

  QAccessibleInterface* plainInterface = QAccessible::queryAccessibleInterface(&plainAlert);
  QVERIFY(plainInterface);
  QCOMPARE(plainInterface->text(QAccessible::Name), QStringLiteral("Plain name"));
  QCOMPARE(plainInterface->text(QAccessible::Description), QStringLiteral("Plain description"));

  adqt::widgets::AdAlert hostedAlert;
  auto* hostedText = new QLabel(QStringLiteral("Ignored body"));
  hostedText->setAccessibleName(QStringLiteral("Hosted name"));
  auto* hostedDescription = new QLabel(QStringLiteral("Ignored details"));
  hostedDescription->setAccessibleDescription(QStringLiteral("Hosted description"));
  hostedAlert.setTextWidget(hostedText);
  hostedAlert.setInformativeWidget(hostedDescription);

  QAccessibleInterface* hostedInterface = QAccessible::queryAccessibleInterface(&hostedAlert);
  QVERIFY(hostedInterface);
  QCOMPARE(hostedInterface->text(QAccessible::Name), QStringLiteral("Hosted name"));
  QCOMPARE(hostedInterface->text(QAccessible::Description), QStringLiteral("Hosted description"));

  hostedAlert.setAccessibleName(QStringLiteral("Explicit name"));
  hostedAlert.setAccessibleDescription(QStringLiteral("Explicit description"));
  QCOMPARE(hostedInterface->text(QAccessible::Name), QStringLiteral("Explicit name"));
  QCOMPARE(hostedInterface->text(QAccessible::Description), QStringLiteral("Explicit description"));
}

void AlertTests::inheritedPaletteOverridesApplyWithoutLocalPalette() {
  QWidget host;
  host.resize(420, 160);

  const QColor background(QStringLiteral("#f7e8ff"));
  const QColor border(QStringLiteral("#d3adf7"));
  const QColor textColor(QStringLiteral("#531dab"));
  const QColor descriptionColor(QStringLiteral("#722ed1"));

  QPalette palette = host.palette();
  palette.setColor(QPalette::Window, background);
  palette.setColor(QPalette::Mid, border);
  palette.setColor(QPalette::WindowText, textColor);
  palette.setColor(QPalette::Text, descriptionColor);
  palette.setColor(QPalette::ButtonText, textColor);
  palette.setColor(QPalette::AlternateBase, QColor(QStringLiteral("#efdbff")));
  palette.setColor(QPalette::Button, QColor(QStringLiteral("#e2c8ff")));
  palette.setColor(QPalette::Highlight, QColor(QStringLiteral("#9254de")));
  host.setPalette(palette);

  adqt::widgets::AdAlert alert(&host);
  alert.setText(QStringLiteral("Inherited palette"));
  alert.setInformativeText(QStringLiteral("Description"));
  alert.resize(360, alert.sizeHint().height());

  host.show();
  alert.show();
  QTest::qWait(1);

  QVERIFY(!alert.testAttribute(Qt::WA_SetPalette));

  auto* textLabel = alert.findChild<QLabel*>(QStringLiteral("ad-alert-text"));
  auto* informativeLabel = alert.findChild<QLabel*>(QStringLiteral("ad-alert-informative-text"));
  QVERIFY(textLabel);
  QVERIFY(informativeLabel);

  QCOMPARE(textLabel->palette().color(QPalette::WindowText), textColor);
  QCOMPARE(informativeLabel->palette().color(QPalette::WindowText), descriptionColor);

  const QImage image = alert.grab().toImage();
  const QColor sampledBackground = image.pixelColor(alert.width() - 12, alert.height() / 2);
  QCOMPARE(sampledBackground, background);
}

void AlertTests::inheritedFontChangesRefreshVisuals() {
  QWidget host;
  host.resize(320, 120);

  adqt::widgets::AdAlert alert(&host);
  alert.setText(QStringLiteral("Font refresh"));

  host.show();
  alert.show();
  QTest::qWait(1);

  auto* textLabel = alert.findChild<QLabel*>(QStringLiteral("ad-alert-text"));
  QVERIFY(textLabel);
  QVERIFY(!textLabel->font().italic());

  QFont font = host.font();
  font.setItalic(true);
  host.setFont(font);

  QTRY_VERIFY(textLabel->font().italic());
}

void AlertTests::bannerAutoIconFollowsDisplayMode() {
  adqt::widgets::AdAlert alert;
  alert.setText(QStringLiteral("Icon mode auto"));
  alert.show();
  QTest::qWait(1);

  auto* iconLabel = alert.findChild<QLabel*>(QStringLiteral("ad-alert-icon"));
  QVERIFY(iconLabel);
  QVERIFY(!iconLabel->isVisible());

  alert.setDisplayMode(adqt::widgets::AdAlert::DisplayMode::Banner);
  QTRY_VERIFY(iconLabel->isVisible());

  alert.setDisplayMode(adqt::widgets::AdAlert::DisplayMode::Inline);
  QTRY_VERIFY(!iconLabel->isVisible());

  alert.setIconMode(adqt::widgets::AdAlert::IconMode::Visible);
  QTRY_VERIFY(iconLabel->isVisible());
}

QObject* createAlertTests() { return new AlertTests(); }

#include "alert_tests.moc"
