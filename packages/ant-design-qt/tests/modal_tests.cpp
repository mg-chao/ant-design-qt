#include <QCoreApplication>
#include <QEvent>
#include <QLabel>
#include <QMetaProperty>
#include <QPointer>
#include <QSignalSpy>
#include <QToolButton>
#include <QtTest>

#include "widgets/modal.h"

class ModalTests final : public QObject {
  Q_OBJECT

 private slots:
 void exposesQtDialogProperties();
  void ownerWindowNormalizesToTopLevelWindow();
  void programmaticCloseUsesClosedSignalNotCloseRequested();
  void manualClosePolicyKeepsModalOpenOnCloseButton();
  void manualClosePolicyHandlerCanResolveAndPreservesReason();
  void hostedWidgetsTransferOwnershipAndCanBeTaken();
  void closeAllClosesStaticServiceModals();
};

void ModalTests::exposesQtDialogProperties() {
  adqt::widgets::AdModal modal;
  const QMetaObject* meta = modal.metaObject();

  QVERIFY(meta->indexOfProperty("open") >= 0);
  QVERIFY(meta->indexOfProperty("closePolicy") >= 0);
  QVERIFY(meta->indexOfProperty("windowTitle") >= 0);
  QVERIFY(meta->indexOfProperty("preferredWidth") >= 0);
  QVERIFY(meta->indexOfProperty("standardButtons") >= 0);
  QVERIFY(meta->indexOfProperty("text") >= 0);
  QVERIFY(meta->indexOfProperty("ownerWindow") >= 0);
  QVERIFY(meta->indexOfProperty("contentWidget") >= 0);
  QVERIFY(meta->indexOfProperty("footerWidget") >= 0);

  QCOMPARE(meta->indexOfProperty("hostWindow"), -1);
  QCOMPARE(meta->indexOfProperty("bodyWidget"), -1);
  QCOMPARE(meta->indexOfProperty("visible"), -1);
  QCOMPARE(meta->indexOfProperty("acceptAutoClose"), -1);

  QVERIFY(modal.setProperty("windowTitle", QStringLiteral("Dialog title")));
  QCOMPARE(modal.windowTitle(), QStringLiteral("Dialog title"));

  const auto policy = adqt::widgets::AdModal::ClosePolicy::Manual;
  QVERIFY(modal.setProperty("closePolicy", QVariant::fromValue(policy)));
  QCOMPARE(modal.closePolicy(), policy);

  const auto buttons = adqt::widgets::AdModal::StandardButtons(adqt::widgets::AdModal::StandardButton::Ok);
  QVERIFY(modal.setProperty("standardButtons", QVariant::fromValue(buttons)));
  QCOMPARE(modal.standardButtons(), buttons);
}

void ModalTests::ownerWindowNormalizesToTopLevelWindow() {
  QWidget host;
  host.resize(640, 480);
  QWidget child(&host);
  child.resize(160, 80);
  host.show();
  child.show();
  QTest::qWait(1);

  adqt::widgets::AdModal modal;
  modal.setOwnerWindow(&child);

  QCOMPARE(modal.ownerWindow(), &host);

  modal.open();
  QTRY_VERIFY(modal.isOpen());

  QWidget* overlay = host.findChild<QWidget*>(QStringLiteral("ad-modal-overlay"));
  QVERIFY(overlay != nullptr);
  QCOMPARE(overlay->parentWidget(), &host);
}

void ModalTests::programmaticCloseUsesClosedSignalNotCloseRequested() {
  QWidget host;
  host.resize(640, 480);
  host.show();
  QTest::qWait(1);

  adqt::widgets::AdModal modal(&host);
  modal.setOwnerWindow(&host);
  modal.setWindowTitle(QStringLiteral("Programmatic"));
  modal.setText(QStringLiteral("Programmatic close"));

  QSignalSpy closeRequestedSpy(&modal, &adqt::widgets::AdModal::closeRequested);
  QSignalSpy openSpy(&modal, &adqt::widgets::AdModal::openChanged);
  QSignalSpy closedSpy(&modal, &adqt::widgets::AdModal::closed);
  QSignalSpy rejectedSpy(&modal, &adqt::widgets::AdModal::rejected);
  QSignalSpy finishedSpy(&modal, &adqt::widgets::AdModal::finished);

  modal.open();
  QTRY_VERIFY(modal.isOpen());
  QTRY_COMPARE(openSpy.count(), 1);

  QVERIFY(modal.close());

  QTRY_COMPARE(openSpy.count(), 2);
  QCOMPARE(closeRequestedSpy.count(), 0);
  QTRY_COMPARE(closedSpy.count(), 1);
  QTRY_COMPARE(rejectedSpy.count(), 1);
  QTRY_COMPARE(finishedSpy.count(), 1);
  QVERIFY(!modal.isOpen());

  QCOMPARE(qvariant_cast<bool>(openSpy.at(0).at(0)), true);
  QCOMPARE(qvariant_cast<bool>(openSpy.at(1).at(0)), false);
  QCOMPARE(qvariant_cast<adqt::widgets::AdModal::CloseReason>(closedSpy.at(0).at(0)),
           adqt::widgets::AdModal::CloseReason::Programmatic);
  QCOMPARE(qvariant_cast<adqt::widgets::AdModal::DialogCode>(finishedSpy.at(0).at(0)),
           adqt::widgets::AdModal::DialogCode::Rejected);
}

void ModalTests::manualClosePolicyKeepsModalOpenOnCloseButton() {
  QWidget host;
  host.resize(640, 480);
  host.show();
  QTest::qWait(1);

  adqt::widgets::AdModal modal(&host);
  modal.setOwnerWindow(&host);
  modal.setWindowTitle(QStringLiteral("Manual"));
  modal.setText(QStringLiteral("Manual mode"));
  modal.setClosePolicy(adqt::widgets::AdModal::ClosePolicy::Manual);
  modal.open();

  QTRY_VERIFY(modal.isOpen());
  QTRY_VERIFY(modal.closeButton() != nullptr);

  QSignalSpy closeRequestedSpy(&modal, &adqt::widgets::AdModal::closeRequested);
  QSignalSpy closedSpy(&modal, &adqt::widgets::AdModal::closed);
  QSignalSpy finishedSpy(&modal, &adqt::widgets::AdModal::finished);

  QTest::mouseClick(modal.closeButton(), Qt::LeftButton);

  QTRY_COMPARE(closeRequestedSpy.count(), 1);
  QCOMPARE(closedSpy.count(), 0);
  QCOMPARE(finishedSpy.count(), 0);
  QVERIFY(modal.isOpen());
  QCOMPARE(qvariant_cast<adqt::widgets::AdModal::CloseReason>(closeRequestedSpy.at(0).at(0)),
           adqt::widgets::AdModal::CloseReason::CloseButton);
}

void ModalTests::manualClosePolicyHandlerCanResolveAndPreservesReason() {
  QWidget host;
  host.resize(640, 480);
  host.show();
  QTest::qWait(1);

  adqt::widgets::AdModal modal(&host);
  modal.setOwnerWindow(&host);
  modal.setWindowTitle(QStringLiteral("Manual close handler"));
  modal.setText(QStringLiteral("Manual mode close from handler"));
  modal.setClosePolicy(adqt::widgets::AdModal::ClosePolicy::Manual);
  modal.open();

  QTRY_VERIFY(modal.isOpen());
  QTRY_VERIFY(modal.acceptButton() != nullptr);

  QSignalSpy closeRequestedSpy(&modal, &adqt::widgets::AdModal::closeRequested);
  QSignalSpy closedSpy(&modal, &adqt::widgets::AdModal::closed);
  QSignalSpy acceptedSpy(&modal, &adqt::widgets::AdModal::accepted);
  QSignalSpy finishedSpy(&modal, &adqt::widgets::AdModal::finished);

  connect(&modal, &adqt::widgets::AdModal::closeRequested, &modal,
          [&modal](adqt::widgets::AdModal::CloseReason reason) {
            if (reason == adqt::widgets::AdModal::CloseReason::OkAction) {
              modal.accept();
            } else {
              modal.reject();
            }
          });

  QTest::mouseClick(modal.acceptButton(), Qt::LeftButton);

  QTRY_COMPARE(closeRequestedSpy.count(), 1);
  QTRY_COMPARE(closedSpy.count(), 1);
  QTRY_COMPARE(acceptedSpy.count(), 1);
  QTRY_COMPARE(finishedSpy.count(), 1);
  QVERIFY(!modal.isOpen());
  QCOMPARE(qvariant_cast<adqt::widgets::AdModal::CloseReason>(closeRequestedSpy.at(0).at(0)),
           adqt::widgets::AdModal::CloseReason::OkAction);
  QCOMPARE(qvariant_cast<adqt::widgets::AdModal::CloseReason>(closedSpy.at(0).at(0)),
           adqt::widgets::AdModal::CloseReason::OkAction);
  QCOMPARE(qvariant_cast<adqt::widgets::AdModal::DialogCode>(finishedSpy.at(0).at(0)),
           adqt::widgets::AdModal::DialogCode::Accepted);
}

void ModalTests::hostedWidgetsTransferOwnershipAndCanBeTaken() {
  adqt::widgets::AdModal modal;

  QPointer<QLabel> firstBody = new QLabel(QStringLiteral("First body"));
  modal.setContentWidget(firstBody);
  QVERIFY(firstBody);
  QVERIFY(firstBody->parentWidget() != nullptr);

  QPointer<QLabel> secondBody = new QLabel(QStringLiteral("Second body"));
  modal.setContentWidget(secondBody);
  QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
  QTRY_VERIFY(firstBody.isNull());
  QVERIFY(secondBody);

  QWidget* takenBody = modal.takeContentWidget();
  QCOMPARE(takenBody, secondBody.data());
  QVERIFY(takenBody != nullptr);
  QCOMPARE(takenBody->parentWidget(), nullptr);
  delete takenBody;

  QPointer<QLabel> firstFooter = new QLabel(QStringLiteral("First footer"));
  modal.setFooterWidget(firstFooter);
  QVERIFY(firstFooter);
  QVERIFY(firstFooter->parentWidget() != nullptr);

  QPointer<QLabel> secondFooter = new QLabel(QStringLiteral("Second footer"));
  modal.setFooterWidget(secondFooter);
  QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
  QTRY_VERIFY(firstFooter.isNull());
  QVERIFY(secondFooter);

  QWidget* takenFooter = modal.takeFooterWidget();
  QCOMPARE(takenFooter, secondFooter.data());
  QVERIFY(takenFooter != nullptr);
  QCOMPARE(takenFooter->parentWidget(), nullptr);
  delete takenFooter;
}

void ModalTests::closeAllClosesStaticServiceModals() {
  QWidget host;
  host.resize(640, 480);
  host.show();
  QTest::qWait(1);

  adqt::widgets::AdModalService::Request confirmConfig;
  confirmConfig.title = QStringLiteral("Confirm");
  confirmConfig.text = QStringLiteral("Confirm body");
  confirmConfig.standardButtons = adqt::widgets::AdModal::StandardButton::Ok |
                                  adqt::widgets::AdModal::StandardButton::Cancel;

  QPointer<adqt::widgets::AdModal> infoModal(
      adqt::widgets::AdModalService::showInfo(adqt::widgets::AdModalService::Request{}, &host));
  QPointer<adqt::widgets::AdModal> confirmModal(
      adqt::widgets::AdModalService::showConfirm(confirmConfig, &host));

  QTRY_VERIFY(infoModal);
  QTRY_VERIFY(confirmModal);
  QTRY_VERIFY(infoModal->isOpen());
  QTRY_VERIFY(confirmModal->isOpen());

  adqt::widgets::AdModalService::closeAll();

  QTRY_VERIFY(infoModal.isNull());
  QTRY_VERIFY(confirmModal.isNull());
}

QObject* createModalTests() { return new ModalTests(); }

#include "modal_tests.moc"
