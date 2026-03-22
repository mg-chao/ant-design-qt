#include <QApplication>
#include <QLabel>
#include <QMetaProperty>
#include <QPointer>
#include <QPushButton>
#include <QSignalSpy>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QtTest>

#include <algorithm>

#include "theme/theme.h"
#include "widgets/popup_types.h"
#include "widgets/popconfirm.h"
#include "widgets/popover.h"
#include "widgets/tooltip.h"

namespace {

using adqt::widgets::AdPopconfirm;
using adqt::widgets::AdPopover;
using adqt::widgets::AdPopupPlacement;
using adqt::widgets::AdTooltip;

}  // namespace

class PopupTests final : public QObject {
  Q_OBJECT

 private slots:
  void tooltipExposesQtStyleControllerProperties();
  void tooltipTriggerRectLimitsClickActivation();
  void tooltipSemanticResolverTracksLogicalVisibility();
  void tooltipTargetDestructionClosesOpenPopup();
  void popoverClickTriggerUsesSourceWidget();
  void popoverContentWidgetOwnershipRoundTrips();
  void popoverManualContentRefreshDoesNotRequestClose();
  void popoverWidgetOnlyContentUsesAccessibleFallback();
  void popconfirmExposesQtStyleControllerProperties();
  void popconfirmManualVisibilityRequestAndAcceptFlow();
  void popconfirmDefaultButtonAndEscapeButtonBehaviors();
  void popconfirmButtonConfigControlsAutoClose();
  void popconfirmUsesQtLayoutSpacingForContent();
  void popconfirmPrefersIntrinsicWidthBeforeWrapping();
  void popconfirmMatchesQtButtonOrderAndRoleSignals();
};

void PopupTests::tooltipExposesQtStyleControllerProperties() {
  AdTooltip tooltip;
  const QMetaObject* meta = tooltip.metaObject();

  QVERIFY(meta != nullptr);
  QVERIFY(QString::fromLatin1(meta->superClass()->className()) == QStringLiteral("QObject"));

  const int visibleIndex = meta->indexOfProperty("visible");
  QVERIFY(visibleIndex >= 0);
  const QMetaProperty visibleProperty = meta->property(visibleIndex);
  QVERIFY(visibleProperty.isReadable());
  QVERIFY(visibleProperty.isWritable());

  const int targetWidgetIndex = meta->indexOfProperty("targetWidget");
  QVERIFY(targetWidgetIndex >= 0);
  const QMetaProperty targetWidgetProperty = meta->property(targetWidgetIndex);
  QVERIFY(targetWidgetProperty.isReadable());
  QVERIFY(targetWidgetProperty.isWritable());
  QCOMPARE(targetWidgetProperty.metaType().id(), QMetaType::fromType<QWidget*>().id());

  const int activationModeIndex = meta->indexOfProperty("activationMode");
  QVERIFY(activationModeIndex >= 0);
  QVERIFY(meta->property(activationModeIndex).isWritable());

  const int popupLifetimeIndex = meta->indexOfProperty("popupLifetime");
  QVERIFY(popupLifetimeIndex >= 0);
  QVERIFY(meta->property(popupLifetimeIndex).isWritable());

  const int triggerRectIndex = meta->indexOfProperty("triggerRect");
  QVERIFY(triggerRectIndex >= 0);
  QVERIFY(meta->property(triggerRectIndex).isWritable());

  QVERIFY(tooltip.setProperty("placement", QVariant::fromValue(AdPopupPlacement::BottomRight)));
  QCOMPARE(tooltip.placement(), AdPopupPlacement::BottomRight);

  QVERIFY(tooltip.setProperty("activationMode", QVariant::fromValue(AdTooltip::ActivationMode::Manual)));
  QCOMPARE(tooltip.activationMode(), AdTooltip::ActivationMode::Manual);

  QVERIFY(tooltip.setProperty("popupLifetime", QVariant::fromValue(AdTooltip::PopupLifetime::RecreateOnOpen)));
  QCOMPARE(tooltip.popupLifetime(), AdTooltip::PopupLifetime::RecreateOnOpen);

  QVERIFY(tooltip.setProperty("enabled", false));
  QVERIFY(!tooltip.isEnabled());
}

void PopupTests::tooltipTriggerRectLimitsClickActivation() {
  QWidget window;
  auto* layout = new QVBoxLayout(&window);
  layout->setContentsMargins(0, 0, 0, 0);
  auto* trigger = new QPushButton(QStringLiteral("Trigger"), &window);
  trigger->setFixedSize(120, 40);
  layout->addWidget(trigger);

  window.resize(240, 120);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));

  AdTooltip tooltip;
  tooltip.setText(QStringLiteral("Prompt text"));
  tooltip.setTargetWidget(trigger);
  tooltip.setTriggers(AdTooltip::Trigger::Click);
  tooltip.setTriggerRect(QRect(40, 0, 40, trigger->height()));

  QTest::mouseClick(trigger, Qt::LeftButton, Qt::NoModifier, QPoint(10, 20));
  QCoreApplication::processEvents();
  QVERIFY(!tooltip.isVisible());

  QTest::mouseClick(trigger, Qt::LeftButton, Qt::NoModifier, QPoint(60, 20));
  QTRY_VERIFY(tooltip.isVisible());
}

void PopupTests::tooltipSemanticResolverTracksLogicalVisibility() {
  QWidget window;
  auto* layout = new QVBoxLayout(&window);
  layout->setContentsMargins(0, 0, 0, 0);
  auto* trigger = new QPushButton(QStringLiteral("Trigger"), &window);
  layout->addWidget(trigger);

  window.resize(240, 120);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));

  AdTooltip tooltip;
  QList<bool> resolverVisibleStates;
  tooltip.setSemanticStyleResolver([&resolverVisibleStates](const AdTooltip::StyleContext& context) {
    resolverVisibleStates.append(context.visible);
    return AdTooltip::SemanticStyles{};
  });
  tooltip.setText(QStringLiteral("Prompt text"));
  tooltip.setTargetWidget(trigger);
  tooltip.setTriggers(AdTooltip::Trigger::Click);

  QTest::mouseClick(trigger, Qt::LeftButton);
  QTRY_VERIFY(tooltip.isVisible());
  QVERIFY(resolverVisibleStates.contains(true));

  tooltip.hide();
  QTRY_VERIFY(!tooltip.isVisible());
  QVERIFY(resolverVisibleStates.contains(false));
}

void PopupTests::tooltipTargetDestructionClosesOpenPopup() {
  QWidget window;
  auto* layout = new QVBoxLayout(&window);
  layout->setContentsMargins(0, 0, 0, 0);
  auto* trigger = new QPushButton(QStringLiteral("Trigger"), &window);
  layout->addWidget(trigger);

  window.resize(240, 120);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));

  AdTooltip tooltip;
  tooltip.setText(QStringLiteral("Prompt text"));
  tooltip.setTargetWidget(trigger);
  tooltip.setTriggers(AdTooltip::Trigger::Click);

  QSignalSpy visibleSpy(&tooltip, &AdTooltip::visibleChanged);

  QTest::mouseClick(trigger, Qt::LeftButton);
  QTRY_VERIFY(tooltip.isVisible());
  QVERIFY(visibleSpy.count() >= 1);

  delete trigger;
  QCoreApplication::processEvents();

  QTRY_VERIFY(!tooltip.isVisible());
  QCOMPARE(tooltip.targetWidget(), nullptr);
  QCOMPARE(tooltip.anchorWidget(), nullptr);
}

void PopupTests::popoverClickTriggerUsesSourceWidget() {
  QWidget window;
  auto* layout = new QVBoxLayout(&window);
  layout->setContentsMargins(0, 0, 0, 0);
  auto* trigger = new QPushButton(QStringLiteral("Open popover"), &window);
  layout->addWidget(trigger);

  window.resize(280, 140);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));

  AdPopover popover(&window);
  popover.setSourceWidget(trigger);
  popover.setTriggers(AdPopover::Trigger::Click);
  popover.setTitle(QStringLiteral("Header"));
  popover.setText(QStringLiteral("Body"));

  QSignalSpy visibleSpy(&popover, &AdPopover::visibleChanged);

  QTest::mouseClick(trigger, Qt::LeftButton);
  QTRY_VERIFY(popover.isVisible());
  QCOMPARE(popover.sourceWidget(), trigger);
  QCOMPARE(popover.anchorWidget(), trigger);
  QVERIFY(visibleSpy.count() >= 1);

  popover.hide();
  QTRY_VERIFY(!popover.isVisible());
}

void PopupTests::popoverContentWidgetOwnershipRoundTrips() {
  AdPopover popover;

  auto* firstContent = new QLabel(QStringLiteral("First content"));
  QPointer<QWidget> firstContentGuard(firstContent);
  popover.setContentWidget(firstContent);
  QCOMPARE(popover.contentWidget(), firstContent);
  QVERIFY(firstContent->parentWidget() != nullptr);

  auto* secondContent = new QLabel(QStringLiteral("Second content"));
  popover.setContentWidget(secondContent);
  QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
  QCoreApplication::processEvents();

  QVERIFY(firstContentGuard.isNull());
  QCOMPARE(popover.contentWidget(), secondContent);
  QVERIFY(secondContent->parentWidget() != nullptr);

  QWidget* takenContent = popover.takeContentWidget();
  QCOMPARE(takenContent, static_cast<QWidget*>(secondContent));
  QCOMPARE(popover.contentWidget(), nullptr);
  QCOMPARE(takenContent->parentWidget(), nullptr);

  delete takenContent;
}

void PopupTests::popoverManualContentRefreshDoesNotRequestClose() {
  QWidget window;
  auto* layout = new QVBoxLayout(&window);
  layout->setContentsMargins(0, 0, 0, 0);
  auto* trigger = new QPushButton(QStringLiteral("Trigger"), &window);
  layout->addWidget(trigger);

  window.resize(280, 140);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));

  AdPopover popover(&window);
  popover.setSourceWidget(trigger);
  popover.setTriggers(AdPopover::Trigger::Click);
  popover.setVisibilityPolicy(AdPopover::VisibilityPolicy::Manual);
  popover.setTitle(QStringLiteral("Header"));
  popover.setText(QStringLiteral("Body"));
  popover.setVisible(true);
  QTRY_VERIFY(popover.isVisible());

  QSignalSpy requestSpy(&popover, &AdPopover::visibilityRequested);

  popover.setText(QStringLiteral("Body changed"));
  QCoreApplication::processEvents();

  QCOMPARE(requestSpy.count(), 0);
  QVERIFY(popover.isVisible());
}

void PopupTests::popoverWidgetOnlyContentUsesAccessibleFallback() {
  QWidget window;
  auto* layout = new QVBoxLayout(&window);
  layout->setContentsMargins(0, 0, 0, 0);
  auto* trigger = new QPushButton(QStringLiteral("Trigger"), &window);
  layout->addWidget(trigger);

  window.resize(280, 140);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));

  AdPopover popover(&window);
  popover.setSourceWidget(trigger);
  popover.setTriggers(AdPopover::Trigger::Click);

  auto* content = new QLabel(QStringLiteral("Body widget only"));
  content->setAccessibleName(QStringLiteral("Widget only title"));
  content->setAccessibleDescription(QStringLiteral("Widget only description"));
  popover.setContentWidget(content);
  popover.setVisible(true);

  QTRY_VERIFY(popover.isVisible());
  QWidget* popupBody = nullptr;
  QTRY_VERIFY((popupBody = window.findChild<QWidget*>(QStringLiteral("adpopover-popup"),
                                                      Qt::FindChildrenRecursively)));
  QCOMPARE(popupBody->accessibleName(), QStringLiteral("Widget only title"));
  QCOMPARE(popupBody->accessibleDescription(), QStringLiteral("Widget only description"));
  QCOMPARE(trigger->accessibleDescription(), QStringLiteral("Widget only description"));
}

void PopupTests::popconfirmExposesQtStyleControllerProperties() {
  qRegisterMetaType<AdPopconfirm::StandardButton>();
  qRegisterMetaType<AdPopconfirm::StandardButtons>();
  qRegisterMetaType<AdPopconfirm::Icon>();

  AdPopconfirm popconfirm;
  const QMetaObject* meta = popconfirm.metaObject();

  QVERIFY(meta != nullptr);
  QVERIFY(QString::fromLatin1(meta->superClass()->className()) == QStringLiteral("QObject"));

  const int visibleIndex = meta->indexOfProperty("visible");
  QVERIFY(visibleIndex >= 0);
  QVERIFY(meta->property(visibleIndex).isWritable());

  const int sourceWidgetIndex = meta->indexOfProperty("sourceWidget");
  QVERIFY(sourceWidgetIndex >= 0);
  QVERIFY(meta->property(sourceWidgetIndex).isWritable());

  const int visibilityModeIndex = meta->indexOfProperty("visibilityMode");
  QVERIFY(visibilityModeIndex >= 0);
  QVERIFY(meta->property(visibilityModeIndex).isWritable());

  const int standardButtonsIndex = meta->indexOfProperty("standardButtons");
  QVERIFY(standardButtonsIndex >= 0);
  QVERIFY(meta->property(standardButtonsIndex).isWritable());

  const int defaultButtonIndex = meta->indexOfProperty("defaultButton");
  QVERIFY(defaultButtonIndex >= 0);
  QVERIFY(meta->property(defaultButtonIndex).isWritable());

  const int escapeButtonIndex = meta->indexOfProperty("escapeButton");
  QVERIFY(escapeButtonIndex >= 0);
  QVERIFY(meta->property(escapeButtonIndex).isWritable());

  const int iconIndex = meta->indexOfProperty("icon");
  QVERIFY(iconIndex >= 0);
  QVERIFY(meta->property(iconIndex).isWritable());

  QCOMPARE(meta->indexOfProperty("open"), -1);
  QCOMPARE(meta->indexOfProperty("targetWidget"), -1);
  QCOMPARE(meta->indexOfProperty("activationMode"), -1);

  QWidget widget;
  QVERIFY(popconfirm.setProperty("sourceWidget", QVariant::fromValue(&widget)));
  QCOMPARE(popconfirm.sourceWidget(), &widget);

  QVERIFY(popconfirm.setProperty("visibilityMode",
                                 QVariant::fromValue(adqt::widgets::AdPopupActivationMode::Manual)));
  QCOMPARE(popconfirm.visibilityMode(), AdPopconfirm::VisibilityMode::Manual);

  QVERIFY(popconfirm.setProperty("standardButtons",
                                 QVariant::fromValue(QDialogButtonBox::StandardButtons(
                                     QDialogButtonBox::Ok | QDialogButtonBox::Cancel))));
  QCOMPARE(popconfirm.standardButtons(), QDialogButtonBox::StandardButtons(QDialogButtonBox::Ok |
                                                                           QDialogButtonBox::Cancel));

  QVERIFY(popconfirm.setProperty("defaultButton", QVariant::fromValue(QDialogButtonBox::StandardButton::Ok)));
  QCOMPARE(popconfirm.defaultButton(), QDialogButtonBox::StandardButton::Ok);

  QVERIFY(popconfirm.setProperty("escapeButton",
                                 QVariant::fromValue(QDialogButtonBox::StandardButton::Cancel)));
  QCOMPARE(popconfirm.escapeButton(), QDialogButtonBox::StandardButton::Cancel);

  QVERIFY(popconfirm.setProperty("icon", QVariant::fromValue(QMessageBox::Icon::Question)));
  QCOMPARE(popconfirm.icon(), QMessageBox::Icon::Question);
}

void PopupTests::popconfirmManualVisibilityRequestAndAcceptFlow() {
  QWidget window;
  auto* layout = new QVBoxLayout(&window);
  layout->setContentsMargins(0, 0, 0, 0);
  auto* trigger = new QPushButton(QStringLiteral("Delete"), &window);
  layout->addWidget(trigger);

  window.resize(260, 140);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));

  AdPopconfirm popconfirm(&window);
  popconfirm.setSourceWidget(trigger);
  popconfirm.setTriggers(AdPopconfirm::Trigger::Click);
  popconfirm.setText(QStringLiteral("Delete item"));
  popconfirm.setInformativeText(QStringLiteral("Are you sure?"));
  popconfirm.setButtonText(QDialogButtonBox::StandardButton::Ok, QStringLiteral("Yes"));
  popconfirm.setButtonText(QDialogButtonBox::StandardButton::Cancel, QStringLiteral("No"));
  popconfirm.setVisibilityMode(AdPopconfirm::VisibilityMode::Manual);

  int requestCount = 0;
  bool lastRequestedOpen = false;
  int clickedCount = 0;
  AdPopconfirm::StandardButton lastClickedButton = QDialogButtonBox::StandardButton::NoButton;
  int acceptedCount = 0;

  connect(&popconfirm, &AdPopconfirm::visibilityRequested, this, [&](bool nextOpen) {
    ++requestCount;
    lastRequestedOpen = nextOpen;
  });
  connect(&popconfirm, &AdPopconfirm::clicked, this, [&](AdPopconfirm::StandardButton button) {
    ++clickedCount;
    lastClickedButton = button;
  });
  connect(&popconfirm, &AdPopconfirm::accepted, this, [&]() { ++acceptedCount; });

  QTest::mouseClick(trigger, Qt::LeftButton);
  QTRY_COMPARE(requestCount, 1);
  QVERIFY(lastRequestedOpen);
  QVERIFY(!popconfirm.isVisible());

  popconfirm.setVisibilityMode(AdPopconfirm::VisibilityMode::Automatic);
  popconfirm.setVisible(true);
  QTRY_VERIFY(popconfirm.isVisible());
  QVERIFY(popconfirm.button(QDialogButtonBox::StandardButton::Ok) != nullptr);
  QTRY_VERIFY(popconfirm.button(QDialogButtonBox::StandardButton::Ok)->isVisible());

  QTest::mouseClick(popconfirm.button(QDialogButtonBox::StandardButton::Ok), Qt::LeftButton);
  QTRY_COMPARE(clickedCount, 1);
  QCOMPARE(lastClickedButton, QDialogButtonBox::StandardButton::Ok);
  QTRY_COMPARE(acceptedCount, 1);
  QTRY_VERIFY(!popconfirm.isVisible());
}

void PopupTests::popconfirmDefaultButtonAndEscapeButtonBehaviors() {
  qRegisterMetaType<AdPopconfirm::StandardButton>();

  QWidget window;
  auto* layout = new QVBoxLayout(&window);
  layout->setContentsMargins(0, 0, 0, 0);
  auto* trigger = new QPushButton(QStringLiteral("Delete"), &window);
  layout->addWidget(trigger);

  window.resize(280, 160);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));

  AdPopconfirm popconfirm(&window);
  popconfirm.setSourceWidget(trigger);
  popconfirm.setTriggers(AdPopconfirm::Trigger::Click);
  popconfirm.setText(QStringLiteral("Delete item"));
  popconfirm.setInformativeText(QStringLiteral("Are you sure?"));
  popconfirm.setButtonText(QDialogButtonBox::StandardButton::Ok, QStringLiteral("Yes"));
  popconfirm.setButtonText(QDialogButtonBox::StandardButton::Cancel, QStringLiteral("No"));
  popconfirm.setDefaultButton(QDialogButtonBox::StandardButton::Ok);
  popconfirm.setEscapeButton(QDialogButtonBox::StandardButton::Cancel);

  QSignalSpy acceptedSpy(&popconfirm, &AdPopconfirm::accepted);
  QSignalSpy rejectedSpy(&popconfirm, &AdPopconfirm::rejected);
  QSignalSpy clickedSpy(&popconfirm, &AdPopconfirm::clicked);

  popconfirm.setVisible(true);
  QTRY_VERIFY(popconfirm.isVisible());
  QTRY_COMPARE(QApplication::focusWidget(), static_cast<QWidget*>(popconfirm.button(QDialogButtonBox::Ok)));

  QTest::keyClick(popconfirm.button(QDialogButtonBox::Ok), Qt::Key_Return);
  QTRY_COMPARE(clickedSpy.count(), 1);
  QCOMPARE(clickedSpy.at(0).at(0).value<AdPopconfirm::StandardButton>(), QDialogButtonBox::StandardButton::Ok);
  QTRY_COMPARE(acceptedSpy.count(), 1);
  QTRY_VERIFY(!popconfirm.isVisible());

  popconfirm.setVisible(true);
  QTRY_VERIFY(popconfirm.isVisible());
  QTRY_COMPARE(QApplication::focusWidget(), static_cast<QWidget*>(popconfirm.button(QDialogButtonBox::Ok)));
  QTest::keyClick(popconfirm.button(QDialogButtonBox::StandardButton::Ok), Qt::Key_Escape);
  QTRY_COMPARE(clickedSpy.count(), 2);
  QCOMPARE(clickedSpy.at(1).at(0).value<AdPopconfirm::StandardButton>(),
           QDialogButtonBox::StandardButton::Cancel);
  QTRY_COMPARE(rejectedSpy.count(), 1);
  QTRY_VERIFY(!popconfirm.isVisible());
}

void PopupTests::popconfirmButtonConfigControlsAutoClose() {
  QWidget window;
  auto* layout = new QVBoxLayout(&window);
  layout->setContentsMargins(0, 0, 0, 0);
  auto* trigger = new QPushButton(QStringLiteral("Delete"), &window);
  layout->addWidget(trigger);

  window.resize(280, 160);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));

  AdPopconfirm popconfirm(&window);
  popconfirm.setSourceWidget(trigger);
  popconfirm.setTriggers(AdPopconfirm::Trigger::Click);
  popconfirm.setText(QStringLiteral("Delete item"));
  popconfirm.setInformativeText(QStringLiteral("Are you sure?"));
  popconfirm.setButtonText(QDialogButtonBox::StandardButton::Ok, QStringLiteral("Proceed"));
  popconfirm.setButtonText(QDialogButtonBox::StandardButton::Cancel, QStringLiteral("Back"));
  popconfirm.setAutoCloseButtons(QDialogButtonBox::StandardButton::Cancel);

  int acceptedCount = 0;
  int rejectedCount = 0;
  connect(&popconfirm, &AdPopconfirm::accepted, this, [&]() { ++acceptedCount; });
  connect(&popconfirm, &AdPopconfirm::rejected, this, [&]() { ++rejectedCount; });

  popconfirm.setVisible(true);
  QTRY_VERIFY(popconfirm.isVisible());
  QCOMPARE(popconfirm.buttonText(QDialogButtonBox::StandardButton::Ok), QStringLiteral("Proceed"));
  QCOMPARE(popconfirm.buttonText(QDialogButtonBox::StandardButton::Cancel), QStringLiteral("Back"));

  popconfirm.setButtonBusy(QDialogButtonBox::StandardButton::Ok, true);
  QTRY_VERIFY(popconfirm.button(QDialogButtonBox::StandardButton::Ok) != nullptr);
  QVERIFY(popconfirm.button(QDialogButtonBox::StandardButton::Ok)->busy());
  popconfirm.setButtonBusy(QDialogButtonBox::StandardButton::Ok, false);

  QTest::mouseClick(popconfirm.button(QDialogButtonBox::StandardButton::Ok), Qt::LeftButton);
  QTRY_COMPARE(acceptedCount, 1);
  QVERIFY(popconfirm.isVisible());

  QTest::mouseClick(popconfirm.button(QDialogButtonBox::StandardButton::Cancel), Qt::LeftButton);
  QTRY_COMPARE(rejectedCount, 1);
  QTRY_VERIFY(!popconfirm.isVisible());
}

void PopupTests::popconfirmUsesQtLayoutSpacingForContent() {
  QWidget window;
  auto* layout = new QVBoxLayout(&window);
  layout->setContentsMargins(0, 0, 0, 0);
  auto* trigger = new QPushButton(QStringLiteral("Delete"), &window);
  layout->addWidget(trigger);

  window.resize(320, 180);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));

  const QString titleText = QStringLiteral("Delete item");
  const QString descriptionText = QStringLiteral("This action cannot be undone.");

  AdPopconfirm popconfirm(&window);
  popconfirm.setSourceWidget(trigger);
  popconfirm.setTriggers(AdPopconfirm::Trigger::Click);
  popconfirm.setText(titleText);
  popconfirm.setInformativeText(descriptionText);
  popconfirm.setVisible(true);
  QTRY_VERIFY(popconfirm.isVisible());

  auto findPopupBody = []() -> QWidget* {
    const QWidgetList widgets = QApplication::allWidgets();
    for (QWidget* widget : widgets) {
      if (widget && widget->isVisible() && widget->objectName() == QStringLiteral("adpopover-popup")) {
        return widget;
      }
    }
    return nullptr;
  };

  QWidget* popupBody = nullptr;
  QTRY_VERIFY((popupBody = findPopupBody()));

  auto findLabel = [&](const QString& labelText) -> QLabel* {
    const QList<QLabel*> labels = popupBody->findChildren<QLabel*>();
    for (QLabel* label : labels) {
      if (label && label->isVisible() && label->text() == labelText) {
        return label;
      }
    }
    return nullptr;
  };

  QLabel* titleLabel = nullptr;
  QLabel* descriptionLabel = nullptr;
  QTRY_VERIFY((titleLabel = findLabel(titleText)));
  QTRY_VERIFY((descriptionLabel = findLabel(descriptionText)));

  auto* confirmButton = popconfirm.button(QDialogButtonBox::StandardButton::Ok);
  QVERIFY(confirmButton != nullptr);

  const adqt::theme::ThemeMapToken map = adqt::theme::ThemeManager::instance().resolveTheme(trigger);
  const int expectedDescriptionGap = std::max(0, qRound(map.sizeXXS));
  const int expectedMessageBottom = std::max(0, qRound(map.sizeXS));

  const int actualDescriptionGap = descriptionLabel->geometry().top() - titleLabel->geometry().bottom() - 1;
  QCOMPARE(actualDescriptionGap, expectedDescriptionGap);
  QCOMPARE(descriptionLabel->contentsMargins(), QMargins());

  const QRect descriptionRect =
      QRect(descriptionLabel->mapTo(popupBody, QPoint(0, 0)), descriptionLabel->size());
  const QRect buttonRect = QRect(confirmButton->mapTo(popupBody, QPoint(0, 0)), confirmButton->size());
  const int actualMessageBottom = buttonRect.top() - descriptionRect.bottom() - 1;
  QCOMPARE(actualMessageBottom, expectedMessageBottom);
}

void PopupTests::popconfirmPrefersIntrinsicWidthBeforeWrapping() {
  QWidget window;
  auto* layout = new QVBoxLayout(&window);
  layout->setContentsMargins(0, 0, 0, 0);
  auto* trigger = new QPushButton(QStringLiteral("Delete"), &window);
  layout->addWidget(trigger);

  window.resize(860, 220);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));

  const QString titleText =
      QStringLiteral("Delete the selected repository and all generated artifacts permanently?");

  AdPopconfirm::ComponentTokens tokens;
  tokens.popupMaximumWidth = 600;

  AdPopconfirm popconfirm(&window);
  popconfirm.setSourceWidget(trigger);
  popconfirm.setTriggers(AdPopconfirm::Trigger::Click);
  popconfirm.setIcon(QMessageBox::NoIcon);
  popconfirm.setText(titleText);
  popconfirm.setInformativeText(QString());
  popconfirm.setComponentTokens(tokens);
  popconfirm.setVisible(true);
  QTRY_VERIFY(popconfirm.isVisible());

  auto findPopupBody = []() -> QWidget* {
    const QWidgetList widgets = QApplication::allWidgets();
    for (QWidget* widget : widgets) {
      if (widget && widget->isVisible() && widget->objectName() == QStringLiteral("adpopover-popup")) {
        return widget;
      }
    }
    return nullptr;
  };

  QWidget* popupBody = nullptr;
  QTRY_VERIFY((popupBody = findPopupBody()));

  auto findTitleLabel = [&]() -> QLabel* {
    const QList<QLabel*> labels = popupBody->findChildren<QLabel*>();
    for (QLabel* label : labels) {
      if (label && label->isVisible() && label->text() == titleText) {
        return label;
      }
    }
    return nullptr;
  };

  QLabel* titleLabel = nullptr;
  QTRY_VERIFY((titleLabel = findTitleLabel()));

  const int naturalTextWidth = titleLabel->fontMetrics().horizontalAdvance(titleText);
  QVERIFY2(naturalTextWidth > 400, "The title text must be wide enough to exercise the width regression.");

  const int initialBodyWidth = popupBody->width();
  const int initialLabelWidth = titleLabel->width();
  QVERIFY2(initialBodyWidth > 400,
           qPrintable(QStringLiteral("popup width stayed too small on first show: %1").arg(initialBodyWidth)));
  QVERIFY2(initialLabelWidth > 380,
           qPrintable(QStringLiteral("title label width stayed too small on first show: %1").arg(initialLabelWidth)));

  for (int index = 0; index < 5; ++index) {
    QCoreApplication::processEvents();
    QTest::qWait(10);
    QCOMPARE(popupBody->width(), initialBodyWidth);
    QCOMPARE(titleLabel->width(), initialLabelWidth);
  }
}

void PopupTests::popconfirmMatchesQtButtonOrderAndRoleSignals() {
  QWidget window;
  auto* layout = new QVBoxLayout(&window);
  layout->setContentsMargins(0, 0, 0, 0);
  auto* trigger = new QPushButton(QStringLiteral("More actions"), &window);
  auto* referenceBox = new QDialogButtonBox(Qt::Horizontal, &window);
  layout->addWidget(trigger);
  layout->addWidget(referenceBox);

  const AdPopconfirm::StandardButtons buttons = QDialogButtonBox::Yes | QDialogButtonBox::No |
                                                QDialogButtonBox::Cancel | QDialogButtonBox::Discard |
                                                QDialogButtonBox::Help | QDialogButtonBox::Apply |
                                                QDialogButtonBox::Reset;
  referenceBox->setStandardButtons(buttons);

  window.resize(520, 220);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));

  AdPopconfirm popconfirm(&window);
  popconfirm.setSourceWidget(trigger);
  popconfirm.setTriggers(AdPopconfirm::Trigger::Click);
  popconfirm.setText(QStringLiteral("Choose an action"));
  popconfirm.setInformativeText(QStringLiteral("This popup mirrors Qt button semantics."));
  popconfirm.setStandardButtons(buttons);
  popconfirm.setAutoCloseButtons(QDialogButtonBox::StandardButton::NoButton);
  popconfirm.setVisible(true);
  QTRY_VERIFY(popconfirm.isVisible());

  auto orderedPopconfirmButtons = [&](const QList<AdPopconfirm::StandardButton>& candidates) {
    QVector<QPair<int, AdPopconfirm::StandardButton>> positions;
    for (AdPopconfirm::StandardButton buttonType : candidates) {
      if (auto* button = popconfirm.button(buttonType)) {
        positions.append(qMakePair(button->mapToGlobal(QPoint(0, 0)).x(), buttonType));
      }
    }
    std::sort(positions.begin(), positions.end(), [](const auto& lhs, const auto& rhs) {
      return lhs.first < rhs.first;
    });
    QList<AdPopconfirm::StandardButton> ordered;
    for (const auto& entry : positions) {
      ordered.append(entry.second);
    }
    return ordered;
  };

  auto orderedReferenceButtons = [&]() {
    QVector<QPair<int, AdPopconfirm::StandardButton>> positions;
    for (QAbstractButton* button : referenceBox->buttons()) {
      const auto standardButton = referenceBox->standardButton(button);
      positions.append(qMakePair(button->mapToGlobal(QPoint(0, 0)).x(), standardButton));
    }
    std::sort(positions.begin(), positions.end(), [](const auto& lhs, const auto& rhs) {
      return lhs.first < rhs.first;
    });
    QList<AdPopconfirm::StandardButton> ordered;
    for (const auto& entry : positions) {
      ordered.append(entry.second);
    }
    return ordered;
  };

  const QList<AdPopconfirm::StandardButton> candidates = {QDialogButtonBox::Yes,     QDialogButtonBox::No,
                                                          QDialogButtonBox::Cancel,  QDialogButtonBox::Discard,
                                                          QDialogButtonBox::Help,    QDialogButtonBox::Apply,
                                                          QDialogButtonBox::Reset};
  QCOMPARE(orderedPopconfirmButtons(candidates), orderedReferenceButtons());

  QSignalSpy acceptedSpy(&popconfirm, &AdPopconfirm::accepted);
  QSignalSpy rejectedSpy(&popconfirm, &AdPopconfirm::rejected);
  QSignalSpy helpSpy(&popconfirm, &AdPopconfirm::helpRequested);
  QSignalSpy clickedSpy(&popconfirm, &AdPopconfirm::clicked);

  QTest::mouseClick(popconfirm.button(QDialogButtonBox::Discard), Qt::LeftButton);
  QTRY_COMPARE(clickedSpy.count(), 1);
  QCOMPARE(clickedSpy.at(0).at(0).value<AdPopconfirm::StandardButton>(), QDialogButtonBox::Discard);
  QCOMPARE(acceptedSpy.count(), 0);
  QCOMPARE(rejectedSpy.count(), 0);
  QCOMPARE(helpSpy.count(), 0);
  QVERIFY(popconfirm.isVisible());

  QTest::mouseClick(popconfirm.button(QDialogButtonBox::Help), Qt::LeftButton);
  QTRY_COMPARE(clickedSpy.count(), 2);
  QCOMPARE(clickedSpy.at(1).at(0).value<AdPopconfirm::StandardButton>(), QDialogButtonBox::Help);
  QCOMPARE(acceptedSpy.count(), 0);
  QCOMPARE(rejectedSpy.count(), 0);
  QCOMPARE(helpSpy.count(), 1);
  QVERIFY(popconfirm.isVisible());

  QTest::mouseClick(popconfirm.button(QDialogButtonBox::Apply), Qt::LeftButton);
  QTRY_COMPARE(clickedSpy.count(), 3);
  QCOMPARE(clickedSpy.at(2).at(0).value<AdPopconfirm::StandardButton>(), QDialogButtonBox::Apply);
  QCOMPARE(acceptedSpy.count(), 0);
  QCOMPARE(rejectedSpy.count(), 0);
  QCOMPARE(helpSpy.count(), 1);
  QVERIFY(popconfirm.isVisible());

  QTest::mouseClick(popconfirm.button(QDialogButtonBox::Reset), Qt::LeftButton);
  QTRY_COMPARE(clickedSpy.count(), 4);
  QCOMPARE(clickedSpy.at(3).at(0).value<AdPopconfirm::StandardButton>(), QDialogButtonBox::Reset);
  QCOMPARE(acceptedSpy.count(), 0);
  QCOMPARE(rejectedSpy.count(), 0);
  QCOMPARE(helpSpy.count(), 1);
  QVERIFY(popconfirm.isVisible());

  QTest::mouseClick(popconfirm.button(QDialogButtonBox::Yes), Qt::LeftButton);
  QTRY_COMPARE(clickedSpy.count(), 5);
  QCOMPARE(clickedSpy.at(4).at(0).value<AdPopconfirm::StandardButton>(), QDialogButtonBox::Yes);
  QCOMPARE(acceptedSpy.count(), 1);
  QCOMPARE(rejectedSpy.count(), 0);
  QCOMPARE(helpSpy.count(), 1);
  QVERIFY(popconfirm.isVisible());

  QTest::mouseClick(popconfirm.button(QDialogButtonBox::No), Qt::LeftButton);
  QTRY_COMPARE(clickedSpy.count(), 6);
  QCOMPARE(clickedSpy.at(5).at(0).value<AdPopconfirm::StandardButton>(), QDialogButtonBox::No);
  QCOMPARE(acceptedSpy.count(), 1);
  QCOMPARE(rejectedSpy.count(), 1);
  QCOMPARE(helpSpy.count(), 1);
  QVERIFY(popconfirm.isVisible());
}

QObject* createPopupTests() { return new PopupTests(); }

#include "popup_tests.moc"
