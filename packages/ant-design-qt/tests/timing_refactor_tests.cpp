#include <QtTest/QtTest>

#include <QBuffer>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QCursor>
#include <QElapsedTimer>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QIcon>
#include <QImage>
#include <QInputMethodEvent>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QListView>
#include <QMouseEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalSpy>
#include <QStackedWidget>
#include <QToolButton>
#include <QTextEdit>
#include <QWidget>
#include <QWheelEvent>

#include <algorithm>
#include <numeric>

#include "icons.h"
#include "theme/fast_color_lite.h"
#include "theme/theme_manager.h"
#include "widgets/button.h"
#include "widgets/color_picker.h"
#include "widgets/detail/timing_hub.h"
#include "widgets/input.h"
#include "widgets/input_style.h"
#include "widgets/in_window_popup_host.h"
#include "widgets/interaction_overlay_manager.h"
#include "widgets/menu.h"
#include "widgets/popover.h"
#include "widgets/radio.h"
#include "widgets/radio_group.h"
#include "widgets/radio_style.h"
#define private public
#include "widgets/select.h"
#include "widgets/slider.h"
#undef private
#include "widgets/select_style.h"
#include "widgets/slider_style.h"
#include "widgets/switch.h"
#include "widgets/switch_style.h"
#include "widgets/tooltip.h"

namespace {

using adqt::theme::ThemeConfig;
using adqt::theme::ThemeManager;
using adqt::widgets::AdButton;
using adqt::widgets::AdColorPicker;
using adqt::widgets::AdInput;
using adqt::widgets::AdInputOtp;
using adqt::widgets::AdInputPassword;
using adqt::widgets::AdInputSearch;
using adqt::widgets::AdInputTextArea;
using adqt::widgets::AdMenu;
using adqt::widgets::AdPopover;
using adqt::widgets::AdRadio;
using adqt::widgets::AdRadioGroup;
using adqt::widgets::AdSelect;
using adqt::widgets::AdSlider;
using adqt::widgets::AdSwitch;
using adqt::widgets::AdTooltip;
using adqt::widgets::InteractionWaveRequest;

QByteArray pngDigest(const QImage& image) {
  if (image.isNull()) {
    return {};
  }

  QByteArray bytes;
  QBuffer buffer(&bytes);
  if (!buffer.open(QIODevice::WriteOnly)) {
    return {};
  }
  image.save(&buffer, "PNG");
  return QCryptographicHash::hash(bytes, QCryptographicHash::Sha1);
}

QByteArray iconDigest(const QIcon& icon, const QSize& size = QSize(18, 18)) {
  if (icon.isNull()) {
    return {};
  }
  const QPixmap pixmap = icon.pixmap(size, QIcon::Normal, QIcon::Off);
  return pngDigest(pixmap.toImage());
}

QColor parseThemeColor(const QString& value, const QColor& fallback) {
  const adqt::theme::FastColorLite parsed(value);
  if (!parsed.isValid()) {
    return fallback;
  }

  QColor color;
  color.setRed(parsed.red());
  color.setGreen(parsed.green());
  color.setBlue(parsed.blue());
  color.setAlphaF(parsed.alpha());
  return color;
}

void sendMouseClick(QWidget* widget,
                    Qt::MouseButton button = Qt::LeftButton,
                    Qt::KeyboardModifiers modifiers = Qt::NoModifier,
                    const QPoint& localPos = QPoint()) {
  if (!widget) {
    return;
  }

  const QPoint pos = localPos.isNull() ? widget->rect().center() : localPos;
  const QPointF localPoint(pos);
  const QPointF scenePoint(widget->mapTo(widget->window(), pos));
  const QPointF globalPoint(widget->mapToGlobal(pos));

  QMouseEvent press(QEvent::MouseButtonPress, localPoint, scenePoint, globalPoint, button, button,
                    modifiers);
  QCoreApplication::sendEvent(widget, &press);

  QMouseEvent release(QEvent::MouseButtonRelease, localPoint, scenePoint, globalPoint, button,
                      Qt::NoButton, modifiers);
  QCoreApplication::sendEvent(widget, &release);
  QCoreApplication::processEvents();
}

void sendMousePress(QWidget* widget,
                    Qt::MouseButton button = Qt::LeftButton,
                    Qt::KeyboardModifiers modifiers = Qt::NoModifier,
                    const QPoint& localPos = QPoint()) {
  if (!widget) {
    return;
  }
  const QPoint pos = localPos.isNull() ? widget->rect().center() : localPos;
  const QPointF localPoint(pos);
  const QPointF scenePoint(widget->mapTo(widget->window(), pos));
  const QPointF globalPoint(widget->mapToGlobal(pos));
  QMouseEvent press(QEvent::MouseButtonPress, localPoint, scenePoint, globalPoint, button, button,
                    modifiers);
  QCoreApplication::sendEvent(widget, &press);
  QCoreApplication::processEvents();
}

void sendMouseRelease(QWidget* widget,
                      Qt::MouseButton button = Qt::LeftButton,
                      Qt::KeyboardModifiers modifiers = Qt::NoModifier,
                      const QPoint& localPos = QPoint()) {
  if (!widget) {
    return;
  }
  const QPoint pos = localPos.isNull() ? widget->rect().center() : localPos;
  const QPointF localPoint(pos);
  const QPointF scenePoint(widget->mapTo(widget->window(), pos));
  const QPointF globalPoint(widget->mapToGlobal(pos));
  QMouseEvent release(QEvent::MouseButtonRelease, localPoint, scenePoint, globalPoint, button,
                      Qt::NoButton, modifiers);
  QCoreApplication::sendEvent(widget, &release);
  QCoreApplication::processEvents();
}

void sendMouseWheel(QWidget* widget, const QPoint& localPos = QPoint(), int angleDeltaY = -120) {
  if (!widget) {
    return;
  }
  const QPoint pos = localPos.isNull() ? widget->rect().center() : localPos;
  const QPointF localPoint(pos);
  const QPointF globalPoint(widget->mapToGlobal(pos));
  QWheelEvent wheel(localPoint, globalPoint, QPoint(0, 0), QPoint(0, angleDeltaY), Qt::NoButton,
                    Qt::NoModifier, Qt::NoScrollPhase, false);
  QCoreApplication::sendEvent(widget, &wheel);
  QCoreApplication::processEvents();
}

ThemeConfig buildTimingConfig(int frameIntervalMs,
                              int spinnerCycleMs,
                              int waveDurationMs,
                              int menuOpenDelayMs,
                              int menuCloseDelayMs,
                              int loadingDelayMs,
                              bool motion = true) {
  ThemeConfig config = ThemeManager::instance().currentConfig();
  config.seed.motion = motion;
  config.seed.timingFrameIntervalMs = frameIntervalMs;
  config.seed.timingSpinnerCycleMs = spinnerCycleMs;
  config.seed.timingWaveDurationMs = waveDurationMs;
  config.seed.timingMenuOpenDelayMs = menuOpenDelayMs;
  config.seed.timingMenuCloseDelayMs = menuCloseDelayMs;
  config.seed.timingLoadingDelayMs = loadingDelayMs;
  return config;
}

void applyTimingConfig(const ThemeConfig& config) {
  ThemeManager::instance().setTheme(config);
  QCoreApplication::sendPostedEvents(nullptr, 0);
  QCoreApplication::processEvents();
}

QToolButton* findVisibleIconButton(QWidget* root) {
  if (!root) {
    return nullptr;
  }
  const QList<QToolButton*> candidates = root->findChildren<QToolButton*>();
  for (QToolButton* button : candidates) {
    if (button && button->isVisible() && !button->icon().isNull()) {
      return button;
    }
  }
  return nullptr;
}

QToolButton* findToolButton(QWidget* root, const QString& objectName) {
  if (!root) {
    return nullptr;
  }
  return root->findChild<QToolButton*>(objectName, Qt::FindChildrenRecursively);
}

QWidget* findInteractionOverlayWidget(QWidget* window) {
  if (!window) {
    return nullptr;
  }
  const QList<QWidget*> children = window->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
  for (QWidget* child : children) {
    if (!child) {
      continue;
    }
    if (!child->testAttribute(Qt::WA_TransparentForMouseEvents)) {
      continue;
    }
    if (!child->testAttribute(Qt::WA_TranslucentBackground)) {
      continue;
    }
    return child;
  }
  return nullptr;
}

class FakePopupOwner final : public QObject, public adqt::widgets::detail::InWindowPopupOwner {
 public:
  explicit FakePopupOwner(QWidget* scopeWindow, QWidget* anchorParent = nullptr)
      : scopeWindow_(scopeWindow) {
    anchorWidget_ = new QWidget(anchorParent ? anchorParent : scopeWindow_.data());
    anchorWidget_->setGeometry(24, 24, 120, 32);
    anchorWidget_->show();
    popupVisible_ = true;
    refreshPopupRect();
  }

  QWidget* anchorWidget() const { return anchorWidget_; }
  void resetRelayoutCount() { relayoutCount_ = 0; }
  int relayoutCount() const { return relayoutCount_; }
  int closeCount() const { return closeReasons_.size(); }
  const QVector<adqt::widgets::detail::PopupCloseReason>& closeReasons() const { return closeReasons_; }

  QObject* popupOwnerObject() const override { return const_cast<FakePopupOwner*>(this); }
  QWidget* popupAnchorWidget() const override { return anchorWidget_; }
  QWidget* popupScopeWindow() const override { return scopeWindow_; }
  bool popupIsVisible() const override { return popupVisible_; }
  bool popupContainsGlobalPos(const QPoint& globalPos) const override {
    return popupGlobalRect_.contains(globalPos);
  }
  void popupCloseFromHost(adqt::widgets::detail::PopupCloseReason reason) override {
    popupVisible_ = false;
    closeReasons_.push_back(reason);
  }
  void popupRelayoutFromHost() override {
    ++relayoutCount_;
    refreshPopupRect();
  }

 private:
  void refreshPopupRect() {
    if (!anchorWidget_) {
      popupGlobalRect_ = QRect();
      return;
    }
    const QPoint topLeft = anchorWidget_->mapToGlobal(QPoint(0, anchorWidget_->height()));
    popupGlobalRect_ = QRect(topLeft, QSize(180, 160));
  }

  QPointer<QWidget> scopeWindow_;
  QPointer<QWidget> anchorWidget_;
  bool popupVisible_ = false;
  int relayoutCount_ = 0;
  QVector<adqt::widgets::detail::PopupCloseReason> closeReasons_;
  QRect popupGlobalRect_;
};

class TimingRefactorTests final : public QObject {
  Q_OBJECT

 private slots:
  void initTestCase() { originalConfig_ = ThemeManager::instance().currentConfig(); }

  void cleanup() { applyTimingConfig(originalConfig_); }

  void cleanupTestCase() { applyTimingConfig(originalConfig_); }

  void timingHub_deduplicatesByContextAndKey() {
    QObject context;
    int hitCount = 0;
    QString marker;

    adqt::widgets::detail::scheduleTimingTask(&context, QStringLiteral("dedupe-key"), 40, [&]() {
      ++hitCount;
      marker = QStringLiteral("first");
    });
    adqt::widgets::detail::scheduleTimingTask(&context, QStringLiteral("dedupe-key"), 5, [&]() {
      ++hitCount;
      marker = QStringLiteral("second");
    });

    QTRY_COMPARE_WITH_TIMEOUT(hitCount, 1, 400);
    QCOMPARE(marker, QStringLiteral("second"));

    QTest::qWait(80);
    QCOMPARE(hitCount, 1);
  }

  void timingHub_deferPreservesOrderForQueuedCallbacks() {
    QObject context;
    QStringList order;

    adqt::widgets::detail::deferTimingTask(&context, QStringLiteral("d1"),
                                           [&]() { order.push_back(QStringLiteral("first")); });
    adqt::widgets::detail::deferTimingTask(&context, QStringLiteral("d2"),
                                           [&]() { order.push_back(QStringLiteral("second")); });
    adqt::widgets::detail::deferTimingTask(&context, QStringLiteral("d3"),
                                           [&]() { order.push_back(QStringLiteral("third")); });

    QTRY_COMPARE_WITH_TIMEOUT(order.size(), 3, 300);
    QCOMPARE(order, QStringList({QStringLiteral("first"), QStringLiteral("second"),
                                 QStringLiteral("third")}));
  }

  void timingHub_cancelPreventsExecution() {
    QObject context;
    int hitCount = 0;

    adqt::widgets::detail::scheduleTimingTask(&context, QStringLiteral("cancel-key"), 25,
                                              [&]() { ++hitCount; });
    adqt::widgets::detail::cancelTimingTask(&context, QStringLiteral("cancel-key"));

    QTest::qWait(100);
    QCOMPARE(hitCount, 0);
  }

  void timingHub_contextDestroyClearsPendingWork() {
    int taskHits = 0;
    int frameHits = 0;

    QObject* context = new QObject();
    adqt::widgets::detail::scheduleTimingTask(context, QStringLiteral("destroy-task"), 15,
                                              [&]() { ++taskHits; });
    adqt::widgets::detail::setFrameSubscription(context, QStringLiteral("destroy-frame"), true,
                                                [&](qint64, qint64) { ++frameHits; });
    delete context;

    QTest::qWait(120);
    QCOMPARE(taskHits, 0);
    QCOMPARE(frameHits, 0);
  }

  void timingHub_frameSubscriptionReplaceAndClear() {
    applyTimingConfig(buildTimingConfig(10, 1000, 560, 0, 100, 0));

    QObject context;
    int firstCallbackHits = 0;
    int secondCallbackHits = 0;

    adqt::widgets::detail::setFrameSubscription(&context, QStringLiteral("frame-key"), true,
                                                [&](qint64, qint64) { ++firstCallbackHits; });
    QTRY_VERIFY_WITH_TIMEOUT(firstCallbackHits > 0, 500);

    adqt::widgets::detail::setFrameSubscription(&context, QStringLiteral("frame-key"), true,
                                                [&](qint64, qint64) { ++secondCallbackHits; });
    QTRY_VERIFY_WITH_TIMEOUT(secondCallbackHits > 0, 500);

    QTest::qWait(30);
    const int firstStable = firstCallbackHits;
    QTest::qWait(80);
    QCOMPARE(firstCallbackHits, firstStable);

    adqt::widgets::detail::clearFrameSubscription(&context, QStringLiteral("frame-key"));
    const int secondStable = secondCallbackHits;
    QTest::qWait(80);
    QCOMPARE(secondCallbackHits, secondStable);
  }

  void timingHub_themeTimingResolutionAndMotionOff() {
    applyTimingConfig(buildTimingConfig(18, 920, 480, 35, 90, 60, true));

    const adqt::widgets::detail::TimingConfig timing = adqt::widgets::detail::currentTimingConfig();
    QCOMPARE(timing.frameIntervalMs, 18);
    QCOMPARE(timing.spinnerCycleMs, 920);
    QCOMPARE(timing.waveDurationMs, 480);
    QCOMPARE(timing.menuOpenDelayMs, 35);
    QCOMPARE(timing.menuCloseDelayMs, 90);
    QCOMPARE(timing.loadingDelayMs, 60);

    QCOMPARE(adqt::widgets::detail::resolveLoadingDelayMs(-1), 60);
    QCOMPARE(adqt::widgets::detail::resolveLoadingDelayMs(4), 4);
    QCOMPARE(adqt::widgets::detail::resolveMenuOpenDelayMs(-1), 35);
    QCOMPARE(adqt::widgets::detail::resolveMenuOpenDelayMs(7), 7);
    QCOMPARE(adqt::widgets::detail::resolveMenuCloseDelayMs(-1), 90);
    QCOMPARE(adqt::widgets::detail::resolveMenuCloseDelayMs(8), 8);

    applyTimingConfig(buildTimingConfig(18, 920, 480, 35, 90, 60, false));
    const adqt::widgets::detail::TimingConfig reduced = adqt::widgets::detail::currentTimingConfig();
    QCOMPARE(reduced.frameIntervalMs, 0);
    QCOMPARE(reduced.spinnerCycleMs, 0);
    QCOMPARE(reduced.waveDurationMs, 0);
    QCOMPARE(reduced.menuOpenDelayMs, 0);
    QCOMPARE(reduced.menuCloseDelayMs, 0);
    QCOMPARE(reduced.loadingDelayMs, 0);
  }

  void timingHub_stressBurstKeepsSingleTaskPerKey() {
    applyTimingConfig(buildTimingConfig(8, 1000, 560, 0, 100, 0));

    QObject context;
    int frameHits = 0;
    adqt::widgets::detail::setFrameSubscription(&context, QStringLiteral("stress-frame"), true,
                                                [&](qint64, qint64) { ++frameHits; });

    constexpr int kKeyCount = 64;
    constexpr int kRounds = 30;
    QVector<int> latestValue(kKeyCount, -1);
    QVector<int> observedValue(kKeyCount, -1);
    QVector<int> seenCount(kKeyCount, 0);

    for (int round = 0; round < kRounds; ++round) {
      for (int keyIndex = 0; keyIndex < kKeyCount; ++keyIndex) {
        const int payload = round;
        latestValue[keyIndex] = payload;
        const QString key = QStringLiteral("stress-%1").arg(keyIndex);
        adqt::widgets::detail::scheduleTimingTask(&context, key, 12, [&, keyIndex, payload]() {
          ++seenCount[keyIndex];
          observedValue[keyIndex] = payload;
        });
      }
    }

    auto totalHits = [&seenCount]() { return std::accumulate(seenCount.cbegin(), seenCount.cend(), 0); };
    QTRY_COMPARE_WITH_TIMEOUT(totalHits(), kKeyCount, 3000);
    QVERIFY(frameHits > 0);

    for (int keyIndex = 0; keyIndex < kKeyCount; ++keyIndex) {
      QCOMPARE(seenCount[keyIndex], 1);
      QCOMPARE(observedValue[keyIndex], latestValue[keyIndex]);
    }

    adqt::widgets::detail::clearFrameSubscription(&context, QStringLiteral("stress-frame"));
  }

  void button_loadingDelayUsesThemeSentinel() {
    applyTimingConfig(buildTimingConfig(10, 300, 560, 0, 100, 70));

    AdButton button;
    button.resize(140, 36);
    button.show();

    QCOMPARE(button.loadingDelay(), -1);
    QVERIFY(!button.isLoadingVisible());

    button.setLoading(true);
    QVERIFY(!button.isLoadingVisible());

    QTest::qWait(25);
    QVERIFY(!button.isLoadingVisible());
    QTRY_VERIFY_WITH_TIMEOUT(button.isLoadingVisible(), 500);

    button.setLoading(false);
    QVERIFY(!button.isLoadingVisible());

    button.setLoadingDelay(-9);
    QCOMPARE(button.loadingDelay(), -1);
    button.setLoadingDelay(0);
    QCOMPARE(button.loadingDelay(), 0);
    button.setLoading(true);
    QVERIFY(button.isLoadingVisible());
  }

  void menu_delaySentinelAndClampBehavior() {
    applyTimingConfig(buildTimingConfig(12, 1000, 560, 80, 90, 0));

    AdMenu menu;
    QCOMPARE(menu.subMenuOpenDelayMs(), -1);
    QCOMPARE(menu.subMenuCloseDelayMs(), -1);

    menu.setSubMenuOpenDelayMs(-3);
    menu.setSubMenuCloseDelayMs(-7);
    QCOMPARE(menu.subMenuOpenDelayMs(), -1);
    QCOMPARE(menu.subMenuCloseDelayMs(), -1);

    QCOMPARE(adqt::widgets::detail::resolveMenuOpenDelayMs(menu.subMenuOpenDelayMs()), 80);
    QCOMPARE(adqt::widgets::detail::resolveMenuCloseDelayMs(menu.subMenuCloseDelayMs()), 90);

    menu.setSubMenuOpenDelayMs(18);
    menu.setSubMenuCloseDelayMs(22);
    QCOMPARE(menu.subMenuOpenDelayMs(), 18);
    QCOMPARE(menu.subMenuCloseDelayMs(), 22);
    QCOMPARE(adqt::widgets::detail::resolveMenuOpenDelayMs(menu.subMenuOpenDelayMs()), 18);
    QCOMPARE(adqt::widgets::detail::resolveMenuCloseDelayMs(menu.subMenuCloseDelayMs()), 22);
  }

  void menu_inlineCollapsedTooltipUsesTooltipComponent() {
    AdMenu menu;
    menu.setMode(AdMenu::Mode::Inline);
    menu.setInlineCollapsed(true);

    AdMenu::Item item;
    item.key = QStringLiteral("item-1");
    item.label = QStringLiteral("Navigation One");
    item.type = AdMenu::ItemType::Item;
    menu.setItems({item});
    menu.resize(80, 48);
    menu.show();
    QTRY_VERIFY_WITH_TIMEOUT(menu.isVisible(), 400);

    QTest::mouseMove(&menu, menu.rect().center());

    QList<AdTooltip*> tooltipHosts =
        menu.findChildren<AdTooltip*>(QString(), Qt::FindDirectChildrenOnly);
    QTRY_VERIFY_WITH_TIMEOUT(!tooltipHosts.isEmpty(), 400);

    AdTooltip* tooltip = tooltipHosts.constFirst();
    QVERIFY(tooltip != nullptr);
    QTRY_VERIFY_WITH_TIMEOUT(tooltip->open(), 400);
    QVERIFY(tooltip->openControlled());
    QCOMPARE(tooltip->placement(), AdTooltip::Placement::Right);
    QCOMPARE(tooltip->titleText(), QStringLiteral("Navigation One"));
    QVERIFY(tooltip->triggerWidget() != nullptr);

    menu.setTooltipEnabled(false);
    QTRY_VERIFY_WITH_TIMEOUT(!tooltip->open(), 400);
  }

  void menu_inlineCollapsedTooltipPlacementFollowsLayoutDirection() {
    AdMenu menu;
    menu.setMode(AdMenu::Mode::Inline);
    menu.setInlineCollapsed(true);

    AdMenu::Item item;
    item.key = QStringLiteral("item-rtl");
    item.label = QStringLiteral("RTL Item");
    item.type = AdMenu::ItemType::Item;
    menu.setItems({item});
    menu.resize(80, 48);
    menu.show();
    QTRY_VERIFY_WITH_TIMEOUT(menu.isVisible(), 400);

    QTest::mouseMove(&menu, QPoint(4, 4));
    QTest::mouseMove(&menu, menu.rect().center());

    QList<AdTooltip*> tooltipHosts =
        menu.findChildren<AdTooltip*>(QString(), Qt::FindDirectChildrenOnly);
    QTRY_VERIFY_WITH_TIMEOUT(!tooltipHosts.isEmpty(), 400);

    AdTooltip* tooltip = tooltipHosts.constFirst();
    QVERIFY(tooltip != nullptr);
    QTRY_VERIFY_WITH_TIMEOUT(tooltip->open(), 400);
    QCOMPARE(tooltip->placement(), AdTooltip::Placement::Right);

    menu.setLayoutDirection(Qt::RightToLeft);
    QCoreApplication::processEvents();
    QTRY_COMPARE_WITH_TIMEOUT(tooltip->placement(), AdTooltip::Placement::Left, 400);
  }

  void select_loadingSpinnerUsesSharedFrameTicker() {
    applyTimingConfig(buildTimingConfig(10, 120, 560, 0, 100, 0));

    AdSelect select;
    select.resize(280, 40);
    select.show();
    select.setLoading(true);

    QToolButton* spinnerButton = nullptr;
    QElapsedTimer findTimer;
    findTimer.start();
    while (!spinnerButton && findTimer.elapsed() < 500) {
      spinnerButton = findVisibleIconButton(&select);
      if (spinnerButton) {
        break;
      }
      QTest::qWait(20);
    }
    QVERIFY(spinnerButton != nullptr);

    const QByteArray firstDigest = iconDigest(spinnerButton->icon());
    QVERIFY(!firstDigest.isEmpty());

    bool changed = false;
    QElapsedTimer spinTimer;
    spinTimer.start();
    while (!changed && spinTimer.elapsed() < 800) {
      QTest::qWait(30);
      const QByteArray currentDigest = iconDigest(spinnerButton->icon());
      if (!currentDigest.isEmpty() && currentDigest != firstDigest) {
        changed = true;
      }
    }
    QVERIFY(changed);

    select.setLoading(false);
    QTRY_VERIFY_WITH_TIMEOUT(!spinnerButton->icon().isNull(), 300);
    const QByteArray settledDigest = iconDigest(spinnerButton->icon());
    QVERIFY(!settledDigest.isEmpty());
    QTest::qWait(120);
    QCOMPARE(iconDigest(spinnerButton->icon()), settledDigest);
  }

  void select_clearOverlayOccupiesSuffixSlot() {
    AdSelect select;
    AdSelect::Option option;
    option.value = QStringLiteral("lucy");
    option.label = QStringLiteral("Lucy");
    select.setOptions({option});
    select.setAllowClear(true);
    select.setValue(option.value);
    select.resize(280, 40);
    select.show();

    QTest::qWait(30);
    QToolButton* suffixButton = findToolButton(&select, QStringLiteral("adselect-suffix"));
    QToolButton* clearButton = findToolButton(&select, QStringLiteral("adselect-clear"));
    QVERIFY(suffixButton != nullptr);
    QVERIFY(clearButton != nullptr);

    QTest::mouseMove(&select, select.rect().center());
    QTRY_VERIFY_WITH_TIMEOUT(clearButton->isVisible(), 400);
    QVERIFY(suffixButton->isVisible());

    QCOMPARE(clearButton->size(), suffixButton->size());
    QCOMPARE(clearButton->geometry(), suffixButton->geometry());
  }

  void select_popupMatchSelectWidthFalseExpandsPopupToContent() {
    AdSelect select;
    select.resize(120, 32);
    select.setPopupMatchSelectWidth(false);

    AdSelect::Option shortOption;
    shortOption.value = QStringLiteral("short");
    shortOption.label = QStringLiteral("Short");

    AdSelect::Option longOption;
    longOption.value = QStringLiteral("long");
    longOption.label = QStringLiteral("This is a very long option label for placement popup width");

    select.setOptions({shortOption, longOption});
    select.setValue(shortOption.value);
    select.show();
    QTRY_VERIFY_WITH_TIMEOUT(select.isVisible(), 400);

    select.setOpen(true);
    QTRY_VERIFY_WITH_TIMEOUT(select.open(), 400);

    QWidget* scopeWindow = select.window();
    QVERIFY(scopeWindow != nullptr);
    QWidget* popup = scopeWindow->findChild<QWidget*>(QStringLiteral("adselect-popup"),
                                                      Qt::FindChildrenRecursively);
    QTRY_VERIFY_WITH_TIMEOUT(popup != nullptr, 400);
    QTRY_VERIFY_WITH_TIMEOUT(popup->isVisible(), 400);

    QListView* listView =
        scopeWindow->findChild<QListView*>(QStringLiteral("adselect-list"), Qt::FindChildrenRecursively);
    QVERIFY(listView != nullptr);
    const QAbstractItemModel* model = listView->model();
    QVERIFY(model != nullptr);

    QModelIndex selectedIndex;
    for (int row = 0; row < model->rowCount(); ++row) {
      const QModelIndex index = model->index(row, 0);
      if (index.data(Qt::UserRole).toString() == shortOption.value) {
        selectedIndex = index;
        break;
      }
    }
    QVERIFY(selectedIndex.isValid());

    const QString selectedText = selectedIndex.data(Qt::DisplayRole).toString();
    QFont selectedFont = selectedIndex.data(Qt::FontRole).value<QFont>();
    if (selectedFont.family().isEmpty()) {
      selectedFont = listView->font();
    }
    const QFontMetrics selectedMetrics(selectedFont);
    const int textWidth = std::max(selectedMetrics.horizontalAdvance(selectedText),
                                   selectedMetrics.boundingRect(selectedText).width());
    const int optionPadding = std::max(0, select.visualStyle_->metrics.optionPaddingHorizontal);
    const int popupContentWidth = textWidth + optionPadding * 2 + 2;
    const QMargins popupMargins =
        popup->layout() ? popup->layout()->contentsMargins() : QMargins(0, 0, 0, 0);
    const int expectedPopupWidth = popupContentWidth + popupMargins.left() + popupMargins.right();

    QVERIFY(popup->width() > select.width());
    QVERIFY(popup->width() >= expectedPopupWidth);
  }

  void select_multipleWrapsAndCapsTagWidth() {
    QWidget host;
    host.resize(320, 220);

    AdSelect select(&host);
    select.setMode(AdSelect::Mode::Multiple);
    select.setGeometry(20, 20, 180, 32);

    QVector<AdSelect::Option> options;
    QStringList values;
    for (int i = 0; i < 6; ++i) {
      AdSelect::Option option;
      option.value = QStringLiteral("value-%1").arg(i);
      option.label = QStringLiteral("This is a very long selected label %1").arg(i);
      options.push_back(option);
      values.push_back(option.value);
    }
    select.setOptions(options);
    select.setValues({values.constFirst()});
    host.show();

    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(select.isVisible(), 400);
    QCoreApplication::processEvents();
    const int singleLineHeight = select.height();
    QVERIFY(singleLineHeight > 0);

    select.setValues(values);
    QTRY_VERIFY_WITH_TIMEOUT(select.height() > singleLineHeight, 400);

    QWidget* tagsContainer = select.findChild<QWidget*>(QStringLiteral("adselect-tags"));
    QVERIFY(tagsContainer != nullptr);
    QTRY_VERIFY_WITH_TIMEOUT(tagsContainer->isVisible(), 400);

    const int containerWidth = tagsContainer->contentsRect().width();
    QVERIFY(containerWidth > 0);
    const QList<QWidget*> tagItems = tagsContainer->findChildren<QWidget*>(
        QStringLiteral("adselect-tag-item"), Qt::FindChildrenRecursively);
    QVERIFY(!tagItems.isEmpty());
    for (QWidget* tagItem : tagItems) {
      QVERIFY(tagItem != nullptr);
      QVERIFY(tagItem->width() <= containerWidth);
      QLabel* tagLabel = tagItem->findChild<QLabel*>(QStringLiteral("adselect-tag-text"),
                                                      Qt::FindChildrenRecursively);
      QVERIFY(tagLabel != nullptr);
      QVERIFY(!tagLabel->text().isEmpty());
    }
  }

  void select_multipleOptionClickRestoresInputFocus() {
    AdSelect select;
    select.setMode(AdSelect::Mode::Multiple);

    AdSelect::Option lucy;
    lucy.value = QStringLiteral("lucy");
    lucy.label = QStringLiteral("Lucy");

    AdSelect::Option jack;
    jack.value = QStringLiteral("jack");
    jack.label = QStringLiteral("Jack");

    select.setOptions({lucy, jack});
    select.resize(280, 40);
    select.show();

    QLineEdit* input = select.findChild<QLineEdit*>(QStringLiteral("adselect-input"));
    QVERIFY(input != nullptr);

    select.setOpen(true);

    QWidget* scopeWindow = select.window();
    QVERIFY(scopeWindow != nullptr);
    QListView* listView =
        scopeWindow->findChild<QListView*>(QStringLiteral("adselect-list"), Qt::FindChildrenRecursively);
    QTRY_VERIFY_WITH_TIMEOUT(listView != nullptr, 400);

    auto* model = listView->model();
    QVERIFY(model != nullptr);

    QModelIndex targetIndex;
    for (int row = 0; row < model->rowCount(); ++row) {
      const QModelIndex index = model->index(row, 0);
      if ((model->flags(index) & Qt::ItemIsSelectable) != 0) {
        targetIndex = index;
        break;
      }
    }
    QVERIFY(targetIndex.isValid());

    QTRY_VERIFY_WITH_TIMEOUT(listView->visualRect(targetIndex).isValid(), 400);
    QTest::mouseClick(listView->viewport(), Qt::LeftButton, Qt::NoModifier,
                      listView->visualRect(targetIndex).center());

    QTRY_VERIFY_WITH_TIMEOUT(input->hasFocus(), 400);
    QCOMPARE(select.values(), QStringList({QStringLiteral("lucy")}));
  }

  void select_multipleImePreeditExpandsInputWidth() {
    AdSelect select;
    select.setMode(AdSelect::Mode::Multiple);
    select.resize(280, 40);
    select.show();

    QLineEdit* input = select.findChild<QLineEdit*>(QStringLiteral("adselect-input"));
    QVERIFY(input != nullptr);

    select.setOpen(true);
    QTRY_VERIFY_WITH_TIMEOUT(select.open(), 400);

    input->setFocus();
    QTRY_VERIFY_WITH_TIMEOUT(input->hasFocus(), 400);
    QCoreApplication::processEvents();

    const int baseWidth = input->width();
    QVERIFY(baseWidth >= 4);

    const QList<QInputMethodEvent::Attribute> emptyAttributes;
    QInputMethodEvent composingEvent(QStringLiteral("pinyinshurufangfa"), emptyAttributes);
    QCoreApplication::sendEvent(input, &composingEvent);
    QCoreApplication::processEvents();
    const int composingWidth = input->width();
    QVERIFY(composingWidth > baseWidth);

    QInputMethodEvent clearComposingEvent(QString(), emptyAttributes);
    QCoreApplication::sendEvent(input, &clearComposingEvent);
    QCoreApplication::processEvents();
    QCOMPARE(input->width(), baseWidth);
  }

  void select_filledMultipleSelectorRepaintsAfterPopupClose() {
    QWidget host;
    host.resize(440, 200);

    AdSelect select(&host);
    select.setMode(AdSelect::Mode::Multiple);
    select.setVariant(AdSelect::Variant::Filled);

    AdSelect::Option lucy;
    lucy.value = QStringLiteral("lucy");
    lucy.label = QStringLiteral("Lucy");

    AdSelect::Option jack;
    jack.value = QStringLiteral("jack");
    jack.label = QStringLiteral("Jack");

    select.setOptions({lucy, jack});
    select.setGeometry(40, 48, 320, 40);

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(select.isVisible(), 400);

    QTest::mouseMove(&host, QPoint(5, 5));
    QCoreApplication::processEvents();

    auto colorDistance = [](const QColor& lhs, const QColor& rhs) -> int {
      return std::abs(lhs.red() - rhs.red()) + std::abs(lhs.green() - rhs.green()) +
             std::abs(lhs.blue() - rhs.blue()) + std::abs(lhs.alpha() - rhs.alpha());
    };

    auto sampleSelectorPixel = [&select]() -> QColor {
      const QImage image = select.grab().toImage();
      if (image.isNull()) {
        return QColor();
      }

      const qreal dpr = std::max<qreal>(1.0, image.devicePixelRatio());
      const int logicalX = std::clamp(select.width() - 100, 0, std::max(0, select.width() - 1));
      const int logicalY = std::clamp(select.height() / 2, 0, std::max(0, select.height() - 1));
      const int px =
          std::clamp(qRound(static_cast<qreal>(logicalX) * dpr), 0, image.width() - 1);
      const int py =
          std::clamp(qRound(static_cast<qreal>(logicalY) * dpr), 0, image.height() - 1);
      return QColor::fromRgba(image.pixel(px, py));
    };

    const QColor closedBeforeOpen = sampleSelectorPixel();

    select.setOpen(true);
    QTRY_VERIFY_WITH_TIMEOUT(select.open(), 400);

    QWidget* scopeWindow = select.window();
    QVERIFY(scopeWindow != nullptr);
    QListView* listView =
        scopeWindow->findChild<QListView*>(QStringLiteral("adselect-list"), Qt::FindChildrenRecursively);
    QTRY_VERIFY_WITH_TIMEOUT(listView != nullptr, 400);
    QTRY_VERIFY_WITH_TIMEOUT(listView->isVisible(), 400);

    const QColor openedColor = sampleSelectorPixel();

    const QAbstractItemModel* model = listView->model();
    QVERIFY(model != nullptr);

    QModelIndex targetIndex;
    for (int row = 0; row < model->rowCount(); ++row) {
      const QModelIndex index = model->index(row, 0);
      if ((model->flags(index) & Qt::ItemIsSelectable) != 0) {
        targetIndex = index;
        break;
      }
    }
    QVERIFY(targetIndex.isValid());
    QTRY_VERIFY_WITH_TIMEOUT(listView->visualRect(targetIndex).isValid(), 400);
    QTest::mouseClick(listView->viewport(), Qt::LeftButton, Qt::NoModifier,
                      listView->visualRect(targetIndex).center());

    select.setOpen(false);
    QTRY_VERIFY_WITH_TIMEOUT(!select.open(), 400);
    QCoreApplication::processEvents();

    const QColor closedAfterClose = sampleSelectorPixel();
    const int distToClosed = colorDistance(closedAfterClose, closedBeforeOpen);
    const int distToOpen = colorDistance(closedAfterClose, openedColor);

    QVERIFY(distToOpen > 10);
    QVERIFY(distToClosed + 6 < distToOpen);
  }

  void select_underlinedFocusedBorderUsesActiveColor() {
    AdSelect select;
    select.setVariant(AdSelect::Variant::Underlined);
    select.setStatus(AdSelect::Status::Error);
    select.resize(280, 40);
    select.show();
    QTRY_VERIFY_WITH_TIMEOUT(select.isVisible(), 400);

    QVERIFY(select.visualStyle_ != nullptr);

    select.hovered_ = true;
    select.hasFocusWithin_ = false;
    select.open_ = false;
    const QColor hoveredBorder = select.resolveSelectorBorderColor();
    QCOMPARE(hoveredBorder, select.visualStyle_->selectorHoverBorderColor);

    select.hasFocusWithin_ = true;
    const QColor focusedBorder = select.resolveSelectorBorderColor();
    QCOMPARE(focusedBorder, select.visualStyle_->selectorActiveBorderColor);

    select.hasFocusWithin_ = false;
    select.open_ = true;
    const QColor openedBorder = select.resolveSelectorBorderColor();
    QCOMPARE(openedBorder, select.visualStyle_->selectorActiveBorderColor);
  }

  void input_filledVariantBackgroundStateFollowsAntdTokens() {
    adqt::widgets::detail::InputStyleInput input;
    input.variant = AdInput::Variant::Filled;
    input.baseFont = QFont();

    const adqt::widgets::detail::InputVisualStyle style =
        adqt::widgets::detail::resolveInputVisualStyle(input);
    const auto& map = ThemeManager::instance().currentMapToken();

    const QColor expectedBg = parseThemeColor(map.colorFillTertiary, QColor("#f5f5f5"));
    const QColor expectedHoverBg = parseThemeColor(map.colorFillSecondary, QColor("#f0f0f0"));
    const QColor expectedActiveBg = parseThemeColor(map.colorBgContainer, QColor("#ffffff"));
    const QColor transparent(0, 0, 0, 0);

    QCOMPARE(style.selectorBg, expectedBg);
    QCOMPARE(style.selectorHoverBg, expectedHoverBg);
    QCOMPARE(style.selectorActiveBg, expectedActiveBg);
    QCOMPARE(style.selectorBorderColor, transparent);
    QCOMPARE(style.selectorHoverBorderColor, transparent);
  }

  void input_filledStatusFocusBgMatchesAntdAndIdleBorderIsTransparent() {
    adqt::widgets::detail::InputStyleInput input;
    input.variant = AdInput::Variant::Filled;
    input.status = AdInput::Status::Error;
    input.baseFont = QFont();

    const adqt::widgets::detail::InputVisualStyle style =
        adqt::widgets::detail::resolveInputVisualStyle(input);
    const auto& map = ThemeManager::instance().currentMapToken();

    const QColor expectedBg = parseThemeColor(map.colorErrorBg, QColor("#fff2f0"));
    const QColor expectedHoverBg = parseThemeColor(map.colorErrorBgHover, QColor("#fff1f0"));
    const QColor expectedActiveBg = parseThemeColor(map.colorBgContainer, QColor("#ffffff"));
    const QColor expectedActiveBorder = parseThemeColor(map.colorError, QColor("#ff4d4f"));
    const QColor transparent(0, 0, 0, 0);

    QCOMPARE(style.selectorBg, expectedBg);
    QCOMPARE(style.selectorHoverBg, expectedHoverBg);
    QCOMPARE(style.selectorActiveBg, expectedActiveBg);
    QCOMPARE(style.selectorBorderColor, transparent);
    QCOMPARE(style.selectorHoverBorderColor, transparent);
    QCOMPARE(style.selectorActiveBorderColor, expectedActiveBorder);
  }

  void select_underlinedDisabledStatusPreservesStatusBorderColor() {
    adqt::widgets::detail::SelectStyleInput enabledInput;
    enabledInput.variant = AdSelect::Variant::Underlined;
    enabledInput.status = AdSelect::Status::Error;
    enabledInput.baseFont = QFont();

    adqt::widgets::detail::SelectStyleInput disabledInput = enabledInput;
    disabledInput.disabled = true;

    const adqt::widgets::detail::SelectVisualStyle enabledStyle =
        adqt::widgets::detail::resolveSelectVisualStyle(enabledInput);
    const adqt::widgets::detail::SelectVisualStyle disabledStyle =
        adqt::widgets::detail::resolveSelectVisualStyle(disabledInput);

    QCOMPARE(disabledStyle.selectorBorderColor, enabledStyle.selectorBorderColor);
    QCOMPARE(disabledStyle.selectorHoverBorderColor, enabledStyle.selectorBorderColor);
    QCOMPARE(disabledStyle.selectorActiveBorderColor, enabledStyle.selectorBorderColor);
  }

  void popupHost_relayoutBurstIsDeferredAndDeduplicated() {
    QWidget scope;
    scope.resize(400, 260);
    scope.show();

    FakePopupOwner owner(&scope);

    adqt::widgets::detail::setInWindowPopupHostOpen(&owner, true);
    QTRY_VERIFY_WITH_TIMEOUT(owner.relayoutCount() >= 1, 400);
    owner.resetRelayoutCount();

    for (int i = 0; i < 10; ++i) {
      owner.anchorWidget()->move(24 + i, 24 + i);
      owner.anchorWidget()->resize(120 + i, 32);
    }

    QTRY_COMPARE_WITH_TIMEOUT(owner.relayoutCount(), 1, 500);
    adqt::widgets::detail::setInWindowPopupHostOpen(&owner, false);
  }

  void popupHost_escapeKeyClosesActiveOwner() {
    QWidget scope;
    scope.resize(400, 260);
    scope.setFocusPolicy(Qt::StrongFocus);
    scope.show();
    QTRY_VERIFY_WITH_TIMEOUT(scope.isVisible(), 400);

    FakePopupOwner owner(&scope);
    adqt::widgets::detail::setInWindowPopupHostOpen(&owner, true);
    QTRY_VERIFY_WITH_TIMEOUT(owner.relayoutCount() >= 1, 400);

    scope.setFocus();
    QTest::keyClick(&scope, Qt::Key_Escape);
    QTRY_COMPARE_WITH_TIMEOUT(owner.closeCount(), 1, 400);
    QCOMPARE(owner.closeReasons().constLast(),
             adqt::widgets::detail::PopupCloseReason::EscapeKeyPress);
  }

  void popupHost_wheelScrollRelayoutsActiveOwner() {
    QWidget scope;
    scope.resize(400, 260);
    scope.show();
    QTRY_VERIFY_WITH_TIMEOUT(scope.isVisible(), 400);

    FakePopupOwner owner(&scope);
    adqt::widgets::detail::setInWindowPopupHostOpen(&owner, true);
    QTRY_VERIFY_WITH_TIMEOUT(owner.relayoutCount() >= 1, 400);
    owner.resetRelayoutCount();

    sendMouseWheel(owner.anchorWidget());
    QTRY_VERIFY_WITH_TIMEOUT(owner.relayoutCount() >= 1, 400);
    QCOMPARE(owner.closeCount(), 0);
  }

  void popupHost_scopeStyleChangeRelayoutsActiveOwner() {
    QWidget scope;
    scope.resize(400, 260);
    scope.show();
    QTRY_VERIFY_WITH_TIMEOUT(scope.isVisible(), 400);

    FakePopupOwner owner(&scope);
    adqt::widgets::detail::setInWindowPopupHostOpen(&owner, true);
    QTRY_VERIFY_WITH_TIMEOUT(owner.relayoutCount() >= 1, 400);
    owner.resetRelayoutCount();

    QEvent styleChangeEvent(QEvent::StyleChange);
    QCoreApplication::sendEvent(&scope, &styleChangeEvent);
    QCoreApplication::processEvents();
    QTRY_VERIFY_WITH_TIMEOUT(owner.relayoutCount() >= 1, 400);
    QCOMPARE(owner.closeCount(), 0);
  }

  void popupHost_scrollBarStyleChangeRelayoutsActiveOwner() {
    QWidget scope;
    scope.resize(420, 320);

    QScrollArea area(&scope);
    area.setGeometry(20, 20, 220, 180);
    auto* content = new QWidget();
    content->resize(200, 900);
    area.setWidget(content);
    area.setWidgetResizable(false);

    scope.show();
    QTRY_VERIFY_WITH_TIMEOUT(scope.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(area.isVisible(), 400);

    FakePopupOwner owner(&scope, &area);
    adqt::widgets::detail::setInWindowPopupHostOpen(&owner, true);
    QTRY_VERIFY_WITH_TIMEOUT(owner.relayoutCount() >= 1, 400);
    owner.resetRelayoutCount();

    QScrollBar* verticalBar = area.verticalScrollBar();
    QVERIFY(verticalBar != nullptr);
    QVERIFY(verticalBar->maximum() > verticalBar->minimum());
    QEvent styleChangeEvent(QEvent::StyleChange);
    QCoreApplication::sendEvent(verticalBar, &styleChangeEvent);
    QCoreApplication::processEvents();

    QTRY_VERIFY_WITH_TIMEOUT(owner.relayoutCount() >= 1, 400);
    QCOMPARE(owner.closeCount(), 0);
  }

  void interactionOverlay_waveStopsAtThemeDuration() {
    applyTimingConfig(buildTimingConfig(12, 1000, 90, 0, 100, 0));

    QWidget window;
    window.resize(420, 300);
    window.show();

    QPushButton owner(QStringLiteral("owner"), &window);
    owner.setGeometry(80, 90, 140, 40);
    owner.show();

    InteractionWaveRequest request;
    request.owner = &owner;
    request.baseRectInWindow = owner.geometry();
    request.topLeft = 8.0;
    request.topRight = 8.0;
    request.bottomRight = 8.0;
    request.bottomLeft = 8.0;
    request.color = QColor(QStringLiteral("#1677ff"));
    adqt::widgets::triggerInteractionWave(request);

    QWidget* overlay = nullptr;
    QElapsedTimer findOverlayTimer;
    findOverlayTimer.start();
    while (!overlay && findOverlayTimer.elapsed() < 400) {
      overlay = findInteractionOverlayWidget(&window);
      if (!overlay) {
        QTest::qWait(10);
      }
    }

    QVERIFY(overlay != nullptr);
    QTRY_VERIFY_WITH_TIMEOUT(overlay->isVisible(), 250);
    QTRY_VERIFY_WITH_TIMEOUT(!overlay->isVisible(), 800);
  }

  void select_adjacentSelectedOptionsMergeHighlight() {
    AdSelect select;
    select.setMode(AdSelect::Mode::Multiple);

    QVector<AdSelect::Option> options;
    for (int i = 0; i < 8; ++i) {
      AdSelect::Option option;
      option.value = QString::number(i);
      option.label = QStringLiteral("Option %1").arg(i);
      options.push_back(option);
    }
    select.setOptions(options);
    select.setValues({QStringLiteral("2"), QStringLiteral("3"), QStringLiteral("4")});
    select.resize(280, 40);
    select.show();
    select.setOpen(true);

    QTRY_VERIFY_WITH_TIMEOUT(select.open(), 400);
    QWidget* scopeWindow = select.window();
    QVERIFY(scopeWindow != nullptr);
    QListView* listView =
        scopeWindow->findChild<QListView*>(QStringLiteral("adselect-list"), Qt::FindChildrenRecursively);
    QVERIFY(listView != nullptr);
    QTRY_VERIFY_WITH_TIMEOUT(listView->isVisible(), 400);

    QTest::qWait(80);
    QCoreApplication::processEvents();

    const QImage image = listView->viewport()->grab().toImage();
    QVERIFY(!image.isNull());

    const QAbstractItemModel* model = listView->model();
    QVERIFY(model != nullptr);

    auto rowOfValue = [model](const QString& value) -> int {
      for (int row = 0; row < model->rowCount(); ++row) {
        if (model->index(row, 0).data(Qt::UserRole).toString() == value) {
          return row;
        }
      }
      return -1;
    };

    const int row2 = rowOfValue(QStringLiteral("2"));
    const int row3 = rowOfValue(QStringLiteral("3"));
    const int row4 = rowOfValue(QStringLiteral("4"));
    QVERIFY(row2 >= 0);
    QVERIFY(row3 == row2 + 1);
    QVERIFY(row4 == row3 + 1);

    const int rowHeight = listView->sizeHintForRow(row2);
    QVERIFY(rowHeight > 0);

    const qreal imageDpr = std::max<qreal>(1.0, image.devicePixelRatio());
    auto pixelAt = [&image, imageDpr](int logicalX, int logicalY) -> QColor {
      const int px = std::clamp(qRound(static_cast<qreal>(logicalX) * imageDpr), 0, image.width() - 1);
      const int py = std::clamp(qRound(static_cast<qreal>(logicalY) * imageDpr), 0, image.height() - 1);
      return QColor::fromRgba(image.pixel(px, py));
    };

    auto colorDistance = [](const QColor& lhs, const QColor& rhs) -> int {
      return std::abs(lhs.red() - rhs.red()) + std::abs(lhs.green() - rhs.green()) +
             std::abs(lhs.blue() - rhs.blue()) + std::abs(lhs.alpha() - rhs.alpha());
    };

    const QColor selectedColor = pixelAt(4, row3 * rowHeight + rowHeight / 2);
    const QColor popupColor = pixelAt(4, (row2 - 1) * rowHeight + rowHeight / 2);
    const QColor seam23TopLeft = pixelAt(1, row3 * rowHeight);
    const QColor seam23BottomLeft = pixelAt(1, row3 * rowHeight - 1);
    const QColor seam34TopLeft = pixelAt(1, row4 * rowHeight);
    const QColor seam34BottomLeft = pixelAt(1, row4 * rowHeight - 1);

    QVERIFY(colorDistance(seam23TopLeft, selectedColor) < colorDistance(seam23TopLeft, popupColor));
    QVERIFY(colorDistance(seam23BottomLeft, selectedColor) <
            colorDistance(seam23BottomLeft, popupColor));
    QVERIFY(colorDistance(seam34TopLeft, selectedColor) < colorDistance(seam34TopLeft, popupColor));
    QVERIFY(colorDistance(seam34BottomLeft, selectedColor) <
            colorDistance(seam34BottomLeft, popupColor));
  }

  void slider_singleAndRange_emitSignalsOnUserAction() {
    AdSlider single;
    single.resize(320, 56);
    single.show();
    QTRY_VERIFY_WITH_TIMEOUT(single.isVisible(), 400);

    QSignalSpy singleValueSpy(&single, &AdSlider::valueChanged);
    QSignalSpy singleCompleteSpy(&single, &AdSlider::valueChangeCompleted);

    QTest::mouseClick(&single, Qt::LeftButton, Qt::NoModifier, QPoint(single.width() - 10, single.height() / 2));
    QTRY_VERIFY_WITH_TIMEOUT(singleValueSpy.count() >= 1, 300);
    QTRY_VERIFY_WITH_TIMEOUT(singleCompleteSpy.count() >= 1, 300);

    AdSlider range;
    range.setMode(AdSlider::Mode::Range);
    range.setValues({20, 50});
    range.resize(320, 56);
    range.show();
    QTRY_VERIFY_WITH_TIMEOUT(range.isVisible(), 400);

    QSignalSpy rangeChangedSpy(&range, &AdSlider::valuesChanged);
    QSignalSpy rangeCompleteSpy(&range, &AdSlider::valuesChangeCompleted);

    const QPoint pressPoint(64, range.height() / 2);
    QTest::mousePress(&range, Qt::LeftButton, Qt::NoModifier, pressPoint);
    QTest::mouseMove(&range, QPoint(120, range.height() / 2));
    QTest::mouseRelease(&range, Qt::LeftButton, Qt::NoModifier, QPoint(120, range.height() / 2));

    QTRY_VERIFY_WITH_TIMEOUT(rangeChangedSpy.count() >= 1, 300);
    QTRY_VERIFY_WITH_TIMEOUT(rangeCompleteSpy.count() >= 1, 300);
  }

  void slider_marksOnly_snapToNearestMark() {
    AdSlider slider;
    AdSlider::MarkMap marks;
    marks.insert(0, {QStringLiteral("0"), std::nullopt, std::nullopt});
    marks.insert(48, {QStringLiteral("48"), std::nullopt, std::nullopt});
    marks.insert(100, {QStringLiteral("100"), std::nullopt, std::nullopt});

    slider.setMarks(marks);
    slider.setMarksOnly(true);
    slider.setValue(40);
    QCOMPARE(slider.value(), 48.0);
  }

  void slider_markActive_singleMatchesAntdIncludedBehavior() {
    AdSlider slider;
    slider.setMinimum(0);
    slider.setMaximum(100);
    slider.setValue(37);

    slider.setIncluded(true);
    QVERIFY(slider.isMarkActive(0));
    QVERIFY(slider.isMarkActive(26));
    QVERIFY(slider.isMarkActive(37));
    QVERIFY(!slider.isMarkActive(100));

    slider.setIncluded(false);
    QVERIFY(!slider.isMarkActive(0));
    QVERIFY(!slider.isMarkActive(26));
    QVERIFY(!slider.isMarkActive(37));
    QVERIFY(!slider.isMarkActive(100));
  }

  void slider_markActive_rangeMatchesAntdIncludedBehavior() {
    AdSlider slider;
    slider.setMode(AdSlider::Mode::Range);
    slider.setMinimum(0);
    slider.setMaximum(100);
    slider.setValues({26, 37});

    slider.setIncluded(true);
    QVERIFY(!slider.isMarkActive(0));
    QVERIFY(slider.isMarkActive(26));
    QVERIFY(slider.isMarkActive(30));
    QVERIFY(slider.isMarkActive(37));
    QVERIFY(!slider.isMarkActive(100));

    slider.setIncluded(false);
    QVERIFY(!slider.isMarkActive(0));
    QVERIFY(!slider.isMarkActive(26));
    QVERIFY(!slider.isMarkActive(30));
    QVERIFY(!slider.isMarkActive(37));
    QVERIFY(!slider.isMarkActive(100));
  }

  void slider_markColor_overrideDoesNotTintDotBorder() {
    auto reddishPixelCountInTrackBand = [](AdSlider& slider) -> int {
      const QImage image = slider.grab().toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
      if (image.isNull()) {
        return -1;
      }

      const int scanTop = image.height() / 3;
      int reddishPixels = 0;
      for (int y = 0; y < scanTop; ++y) {
        for (int x = 0; x < image.width(); ++x) {
          const QColor px = QColor::fromRgba(image.pixel(x, y));
          if (px.alpha() > 100 && px.red() > 200 && px.green() < 120 && px.blue() < 120) {
            ++reddishPixels;
          }
        }
      }
      return reddishPixels;
    };

    auto configureSlider = [](AdSlider* slider, std::optional<QColor> secondMarkColor) {
      slider->setMinimum(0);
      slider->setMaximum(100);
      slider->setIncluded(false);
      slider->setValue(50);

      AdSlider::MarkMap marks;
      marks.insert(25.0, {QStringLiteral(" "), std::nullopt, std::nullopt});
      marks.insert(75.0, {QStringLiteral(" "), secondMarkColor, std::nullopt});
      slider->setMarks(marks);

      AdSlider::ComponentTokens tokens;
      tokens.dotSize = 16;
      tokens.handleLineWidth = 4;
      tokens.dotBorderColor = QStringLiteral("#52c41a");
      tokens.dotActiveBorderColor = QStringLiteral("#1677ff");
      slider->setComponentTokens(tokens);

      slider->resize(360, 70);
      slider->show();
    };

    AdSlider baselineSlider;
    configureSlider(&baselineSlider, std::nullopt);
    QTRY_VERIFY_WITH_TIMEOUT(baselineSlider.isVisible(), 400);
    QCoreApplication::processEvents();
    const int baselineReddishPixels = reddishPixelCountInTrackBand(baselineSlider);
    QVERIFY(baselineReddishPixels >= 0);

    AdSlider coloredSlider;
    configureSlider(&coloredSlider, QColor(QStringLiteral("#ff4d4f")));
    QTRY_VERIFY_WITH_TIMEOUT(coloredSlider.isVisible(), 400);
    QCoreApplication::processEvents();
    const int coloredReddishPixels = reddishPixelCountInTrackBand(coloredSlider);
    QVERIFY(coloredReddishPixels >= 0);

    // Match antd: per-mark color should not change dot border rendering.
    const int reddishDelta = std::abs(coloredReddishPixels - baselineReddishPixels);
    QVERIFY2(reddishDelta < 30,
             qPrintable(QStringLiteral("dot border picked up mark color: baseline=%1 colored=%2 delta=%3")
                            .arg(baselineReddishPixels)
                            .arg(coloredReddishPixels)
                            .arg(reddishDelta)));
  }

  void slider_verticalLongMarkLabel_expandsSizeHintWidth() {
    AdSlider baseline;
    baseline.setOrientation(Qt::Vertical);
    baseline.resize(80, 280);

    AdSlider withLongMark;
    withLongMark.setOrientation(Qt::Vertical);
    withLongMark.resize(80, 280);

    AdSlider::MarkMap marks;
    marks.insert(0.0, {QStringLiteral("A very very very long mark description for testing"), std::nullopt,
                       std::nullopt});
    marks.insert(100.0, {QStringLiteral("100"), std::nullopt, std::nullopt});
    withLongMark.setMarks(marks);

    QVERIFY(withLongMark.sizeHint().width() > baseline.sizeHint().width());
  }

  void slider_handlesAtExtremes_doNotClipVisualRing() {
    auto edgeContainsHandleColor = [](const QImage& image, Qt::Orientation orientation,
                                      bool minSide) {
      auto isHandlePixel = [](const QColor& pixel) {
        return pixel.alpha() > 32 && pixel.red() > 170 && pixel.green() < 120 && pixel.blue() < 120;
      };

      if (orientation == Qt::Horizontal) {
        const int x = minSide ? 0 : image.width() - 1;
        for (int y = 0; y < image.height(); ++y) {
          if (isHandlePixel(QColor::fromRgba(image.pixel(x, y)))) {
            return true;
          }
        }
        return false;
      }

      const int y = minSide ? image.height() - 1 : 0;
      for (int x = 0; x < image.width(); ++x) {
        if (isHandlePixel(QColor::fromRgba(image.pixel(x, y)))) {
          return true;
        }
      }
      return false;
    };

    auto assertNoHandleClipAtEdge = [&edgeContainsHandleColor](AdSlider& slider, const char* axisTag) {
      QCoreApplication::processEvents();
      const QImage image = slider.grab().toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
      QVERIFY(!image.isNull());

      const bool minEdgeHasHandle =
          edgeContainsHandleColor(image, slider.orientation(), true);
      const bool maxEdgeHasHandle =
          edgeContainsHandleColor(image, slider.orientation(), false);

      QVERIFY2(!minEdgeHasHandle,
               qPrintable(QStringLiteral("%1 min-edge clipped").arg(QString::fromLatin1(axisTag))));
      QVERIFY2(!maxEdgeHasHandle,
               qPrintable(QStringLiteral("%1 max-edge clipped").arg(QString::fromLatin1(axisTag))));
    };

    AdSlider horizontal;
    AdSlider::ComponentTokens horizontalTokens;
    horizontalTokens.handleColor = QStringLiteral("#ff4d4f");
    horizontalTokens.handleActiveColor = QStringLiteral("#ff4d4f");
    horizontalTokens.handleActiveOutlineColor = QStringLiteral("#ff4d4f");
    horizontalTokens.handleColorDisabled = QStringLiteral("#ff4d4f");
    horizontalTokens.railBg = QStringLiteral("rgba(0,0,0,0)");
    horizontalTokens.railHoverBg = QStringLiteral("rgba(0,0,0,0)");
    horizontalTokens.trackBg = QStringLiteral("rgba(0,0,0,0)");
    horizontalTokens.trackHoverBg = QStringLiteral("rgba(0,0,0,0)");
    horizontalTokens.trackBgDisabled = QStringLiteral("rgba(0,0,0,0)");
    horizontal.setComponentTokens(horizontalTokens);
    horizontal.resize(320, 56);
    horizontal.show();
    QTRY_VERIFY_WITH_TIMEOUT(horizontal.isVisible(), 400);
    horizontal.setValue(horizontal.minimum());
    assertNoHandleClipAtEdge(horizontal, "horizontal-min");
    horizontal.dragging_ = true;
    horizontal.dragHandleIndex_ = 0;
    assertNoHandleClipAtEdge(horizontal, "horizontal-min-dragging");
    horizontal.setValue(horizontal.maximum());
    assertNoHandleClipAtEdge(horizontal, "horizontal-max");
    assertNoHandleClipAtEdge(horizontal, "horizontal-max-dragging");
    horizontal.dragging_ = false;
    horizontal.dragHandleIndex_ = -1;

    AdSlider vertical;
    AdSlider::ComponentTokens verticalTokens;
    verticalTokens.handleColor = QStringLiteral("#ff4d4f");
    verticalTokens.handleActiveColor = QStringLiteral("#ff4d4f");
    verticalTokens.handleActiveOutlineColor = QStringLiteral("#ff4d4f");
    verticalTokens.handleColorDisabled = QStringLiteral("#ff4d4f");
    verticalTokens.railBg = QStringLiteral("rgba(0,0,0,0)");
    verticalTokens.railHoverBg = QStringLiteral("rgba(0,0,0,0)");
    verticalTokens.trackBg = QStringLiteral("rgba(0,0,0,0)");
    verticalTokens.trackHoverBg = QStringLiteral("rgba(0,0,0,0)");
    verticalTokens.trackBgDisabled = QStringLiteral("rgba(0,0,0,0)");
    vertical.setComponentTokens(verticalTokens);
    vertical.setOrientation(Qt::Vertical);
    vertical.resize(80, 280);
    vertical.show();
    QTRY_VERIFY_WITH_TIMEOUT(vertical.isVisible(), 400);
    vertical.setValue(vertical.minimum());
    assertNoHandleClipAtEdge(vertical, "vertical-min");
    vertical.dragging_ = true;
    vertical.dragHandleIndex_ = 0;
    assertNoHandleClipAtEdge(vertical, "vertical-min-dragging");
    vertical.setValue(vertical.maximum());
    assertNoHandleClipAtEdge(vertical, "vertical-max");
    assertNoHandleClipAtEdge(vertical, "vertical-max-dragging");
    vertical.dragging_ = false;
    vertical.dragHandleIndex_ = -1;
  }

  void slider_verticalReverseValueMapping() {
    AdSlider slider;
    slider.setMinimum(0);
    slider.setMaximum(100);
    slider.setOrientation(Qt::Vertical);
    slider.resize(80, 280);
    slider.show();
    QTRY_VERIFY_WITH_TIMEOUT(slider.isVisible(), 400);

    QTest::mouseClick(&slider, Qt::LeftButton, Qt::NoModifier, QPoint(slider.width() / 2, 10));
    QTRY_VERIFY_WITH_TIMEOUT(slider.value() > 80.0, 300);

    slider.setReverse(true);
    QTest::mouseClick(&slider, Qt::LeftButton, Qt::NoModifier, QPoint(slider.width() / 2, 10));
    QTRY_VERIFY_WITH_TIMEOUT(slider.value() < 20.0, 300);
  }

  void slider_draggableTrack_movesBothHandles() {
    AdSlider slider;
    slider.setMode(AdSlider::Mode::Range);
    slider.setValues({20, 40});
    slider.setDraggableTrack(true);
    slider.resize(320, 56);
    slider.show();
    QTRY_VERIFY_WITH_TIMEOUT(slider.isVisible(), 400);

    const QList<double> before = slider.values();
    QCOMPARE(before.size(), 2);
    const double widthBefore = before.at(1) - before.at(0);

    const QPoint pressPoint(94, slider.height() / 2);
    QTest::mousePress(&slider, Qt::LeftButton, Qt::NoModifier, pressPoint);
    QTest::mouseMove(&slider, QPoint(150, slider.height() / 2));
    QTest::mouseRelease(&slider, Qt::LeftButton, Qt::NoModifier, QPoint(150, slider.height() / 2));

    const QList<double> after = slider.values();
    QCOMPARE(after.size(), 2);
    QVERIFY(after.at(0) > before.at(0));
    QCOMPARE(qRound(after.at(1) - after.at(0)), qRound(widthBefore));
  }

  void slider_rangeDragCross_keepsTooltipBoundToDraggedHandle() {
    AdSlider slider;
    slider.setMode(AdSlider::Mode::Range);
    slider.setValues({20, 50});
    slider.setTooltipVisibleMode(AdSlider::TooltipVisibleMode::Auto);
    slider.resize(320, 56);
    slider.show();
    QTRY_VERIFY_WITH_TIMEOUT(slider.isVisible(), 400);

    adqt::widgets::detail::SliderStyleInput styleInput;
    styleInput.mode = slider.mode();
    styleInput.orientation = slider.orientation();
    styleInput.baseFont = slider.font();
    styleInput.componentTokens = slider.componentTokens();
    const auto style = adqt::widgets::detail::resolveSliderVisualStyle(styleInput);
    const qreal valueSpan = std::max<qreal>(1.0, slider.maximum() - slider.minimum());
    const qreal handleRatio =
        (slider.values().constFirst() - slider.minimum()) / valueSpan;
    const qreal axisStart = style.metrics.marginMain;
    const qreal axisLength = std::max<qreal>(1.0, slider.width() - style.metrics.marginMain * 2.0);
    const int startX = qRound(axisStart + handleRatio * axisLength);
    const QPoint startPoint(startX, slider.height() / 2);
    const QPoint crossPoint(250, slider.height() / 2);
    QTest::mousePress(&slider, Qt::LeftButton, Qt::NoModifier, startPoint);
    QTest::mouseMove(&slider, crossPoint);
    QCOMPARE(slider.dragMode_, AdSlider::DragMode::Handle);
    QVERIFY(slider.dragHandleIndex_ >= 0);
    QCOMPARE(slider.hoverHandleIndex_, slider.dragHandleIndex_);
    QCOMPARE(slider.focusHandleIndex_, slider.dragHandleIndex_);

    QList<AdTooltip*> tooltipHosts =
        slider.findChildren<AdTooltip*>(QString(), Qt::FindDirectChildrenOnly);
    QTRY_COMPARE_WITH_TIMEOUT(tooltipHosts.size(), slider.values().size(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(std::any_of(tooltipHosts.cbegin(), tooltipHosts.cend(),
                                         [](AdTooltip* tooltip) { return tooltip && tooltip->open(); }),
                             400);

    AdTooltip* openTooltip = nullptr;
    for (AdTooltip* tooltip : tooltipHosts) {
      if (tooltip && tooltip->open()) {
        openTooltip = tooltip;
        break;
      }
    }
    QVERIFY(openTooltip != nullptr);

    bool parsed = false;
    const double tooltipValue = openTooltip->titleText().toDouble(&parsed);
    QVERIFY(parsed);
    const QList<double> currentValues = slider.values();
    QVERIFY(!currentValues.isEmpty());
    const QString debugMessage =
        QStringLiteral("tooltip=%1 values=%2")
            .arg(openTooltip->titleText(),
                 QStringLiteral("[%1]")
                     .arg([&currentValues]() {
                       QStringList parts;
                       parts.reserve(currentValues.size());
                       for (double value : currentValues) {
                         parts.append(QString::number(value, 'f', 3));
                       }
                       return parts.join(QStringLiteral(", "));
                     }()));
    QVERIFY2(std::abs(currentValues.constFirst() - 50.0) <= 0.6, qPrintable(debugMessage));
    QVERIFY2(std::abs(tooltipValue - currentValues.constLast()) <= 0.6,
             qPrintable(debugMessage));

    QTest::mouseRelease(&slider, Qt::LeftButton, Qt::NoModifier, crossPoint);
  }

  void slider_editable_addDeleteRespectsBounds() {
    AdSlider slider;
    slider.setMode(AdSlider::Mode::Range);
    slider.setEditableHandles(true);
    slider.setMinHandleCount(1);
    slider.setMaxHandleCount(3);
    slider.setValues({20, 80});
    slider.resize(340, 64);
    slider.show();
    QTRY_VERIFY_WITH_TIMEOUT(slider.isVisible(), 400);

    QCOMPARE(slider.values().size(), 2);
    QTest::mouseClick(&slider, Qt::LeftButton, Qt::NoModifier, QPoint(slider.width() / 2, slider.height() / 2));
    QTRY_COMPARE_WITH_TIMEOUT(slider.values().size(), 3, 300);

    QTest::mouseClick(&slider, Qt::LeftButton, Qt::NoModifier, QPoint(slider.width() - 20, slider.height() / 2));
    QTest::qWait(30);
    QCOMPARE(slider.values().size(), 3);

    slider.setFocus();
    QTest::mouseClick(&slider, Qt::LeftButton, Qt::NoModifier, QPoint(slider.width() / 2, slider.height() / 2));
    QTest::keyClick(&slider, Qt::Key_Delete);
    QTRY_VERIFY_WITH_TIMEOUT(slider.values().size() <= 2, 300);

    QTest::keyClick(&slider, Qt::Key_Delete);
    QTest::keyClick(&slider, Qt::Key_Delete);
    QTest::qWait(30);
    QVERIFY(slider.values().size() >= 1);
  }

  void slider_tooltipModesAndFormatter_doNotCrashAndRespectOpenPolicy() {
    AdSlider slider;
    slider.setMode(AdSlider::Mode::Range);
    slider.setValues({20, 50});
    slider.setTooltipVisibleMode(AdSlider::TooltipVisibleMode::Never);
    slider.setTooltipFormatter([](double value) { return QStringLiteral("%1%%").arg(qRound(value)); });
    slider.resize(300, 56);
    slider.show();
    QTRY_VERIFY_WITH_TIMEOUT(slider.isVisible(), 400);

    QTest::mouseMove(&slider, QPoint(64, slider.height() / 2));
    QTest::mousePress(&slider, Qt::LeftButton, Qt::NoModifier, QPoint(64, slider.height() / 2));
    QTest::mouseMove(&slider, QPoint(110, slider.height() / 2));
    QTest::mouseRelease(&slider, Qt::LeftButton, Qt::NoModifier, QPoint(110, slider.height() / 2));

    slider.setTooltipVisibleMode(AdSlider::TooltipVisibleMode::Always);
    slider.update();
    QTest::qWait(20);
    QVERIFY(slider.values().size() >= 2);
  }

  void slider_tooltipUsesTooltipComponentHierarchy() {
    AdSlider slider;
    slider.setMode(AdSlider::Mode::Range);
    slider.setValues({20, 50});
    slider.setTooltipVisibleMode(AdSlider::TooltipVisibleMode::Always);
    slider.setTooltipFormatter([](double value) { return QStringLiteral("v=%1").arg(qRound(value)); });
    slider.resize(320, 56);
    slider.show();
    QTRY_VERIFY_WITH_TIMEOUT(slider.isVisible(), 400);

    QList<AdTooltip*> tooltipHosts =
        slider.findChildren<AdTooltip*>(QString(), Qt::FindDirectChildrenOnly);
    QTRY_COMPARE_WITH_TIMEOUT(tooltipHosts.size(), slider.values().size(), 400);
    QVERIFY(!tooltipHosts.isEmpty());
    QTRY_VERIFY_WITH_TIMEOUT(std::all_of(tooltipHosts.cbegin(), tooltipHosts.cend(), [](AdTooltip* tooltip) {
                               return tooltip && tooltip->openControlled() && tooltip->open();
                             }),
                             400);
    for (AdTooltip* tooltip : tooltipHosts) {
      QVERIFY(tooltip != nullptr);
      QVERIFY(tooltip->openControlled());
      QVERIFY(!tooltip->titleText().isEmpty());
    }

    slider.setTooltipVisibleMode(AdSlider::TooltipVisibleMode::Never);
    QTRY_VERIFY_WITH_TIMEOUT(std::all_of(tooltipHosts.cbegin(), tooltipHosts.cend(),
                                         [](AdTooltip* tooltip) { return tooltip && !tooltip->open(); }),
                             400);
  }

  void slider_tooltipAutoHidesWhenSliderScrollsOutOfViewport() {
    QScrollArea area;
    area.resize(340, 180);
    area.show();
    QTRY_VERIFY_WITH_TIMEOUT(area.isVisible(), 400);

    auto* content = new QWidget();
    content->resize(320, 920);
    area.setWidget(content);
    area.setWidgetResizable(false);
    QTRY_VERIFY_WITH_TIMEOUT(content->isVisible(), 400);

    AdSlider slider(content);
    slider.setGeometry(24, 24, 240, 56);
    slider.setValue(30);
    slider.setTooltipVisibleMode(AdSlider::TooltipVisibleMode::Always);
    slider.show();
    QTRY_VERIFY_WITH_TIMEOUT(slider.isVisible(), 400);

    QList<AdTooltip*> tooltipHosts =
        slider.findChildren<AdTooltip*>(QString(), Qt::FindDirectChildrenOnly);
    QTRY_COMPARE_WITH_TIMEOUT(tooltipHosts.size(), 1, 400);
    AdTooltip* tooltip = tooltipHosts.constFirst();
    QVERIFY(tooltip != nullptr);
    QTRY_VERIFY_WITH_TIMEOUT(tooltip->open(), 400);

    QWidget* popup = area.findChild<QWidget*>(QStringLiteral("adpopover-popup"), Qt::FindChildrenRecursively);
    QTRY_VERIFY_WITH_TIMEOUT(popup != nullptr && popup->isVisible(), 400);

    QScrollBar* verticalBar = area.verticalScrollBar();
    QVERIFY(verticalBar != nullptr);
    verticalBar->setValue(verticalBar->maximum());
    QCoreApplication::processEvents();

    QTRY_VERIFY_WITH_TIMEOUT(tooltip->open(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(popup != nullptr && !popup->isVisible(), 400);
    const QRect areaRect = QRect(area.mapToGlobal(QPoint(0, 0)), area.size());

    verticalBar->setValue(verticalBar->minimum());
    QCoreApplication::processEvents();
    QTRY_VERIFY_WITH_TIMEOUT(popup != nullptr && popup->isVisible(), 400);
    const QRect popupRectAfterRestore = QRect(popup->mapToGlobal(QPoint(0, 0)), popup->size());
    QVERIFY(popupRectAfterRestore.intersects(areaRect));
  }

  void slider_tooltipAlwaysModeHidesInNestedDocsLikeScrollOnFastScroll() {
    QWidget host;
    host.resize(860, 620);

    QScrollArea area(&host);
    area.setGeometry(12, 12, 640, 460);
    area.setWidgetResizable(true);

    auto* stack = new QStackedWidget();
    auto* page = new QWidget();
    auto* pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(16, 16, 16, 16);
    pageLayout->setSpacing(14);

    auto addFillerSection = [pageLayout](const QString& title) {
      auto* section = new QFrame();
      section->setFrameShape(QFrame::StyledPanel);
      auto* sectionLayout = new QVBoxLayout(section);
      sectionLayout->setContentsMargins(12, 12, 12, 12);
      sectionLayout->setSpacing(8);
      auto* label = new QLabel(title);
      auto* filler = new QWidget();
      filler->setMinimumHeight(140);
      sectionLayout->addWidget(label);
      sectionLayout->addWidget(filler);
      pageLayout->addWidget(section);
    };

    addFillerSection(QStringLiteral("Section A"));

    auto* tooltipSection = new QFrame();
    tooltipSection->setFrameShape(QFrame::StyledPanel);
    auto* tooltipSectionLayout = new QVBoxLayout(tooltipSection);
    tooltipSectionLayout->setContentsMargins(12, 12, 12, 12);
    tooltipSectionLayout->setSpacing(8);
    tooltipSectionLayout->addWidget(new QLabel(QStringLiteral("Control visibility of Tooltip")));
    auto* slider = new AdSlider();
    slider->setValue(30);
    slider->setTooltipVisibleMode(AdSlider::TooltipVisibleMode::Always);
    tooltipSectionLayout->addWidget(slider);
    pageLayout->addWidget(tooltipSection);

    for (int i = 0; i < 6; ++i) {
      addFillerSection(QStringLiteral("Section %1").arg(i + 1));
    }

    stack->addWidget(page);
    stack->setCurrentWidget(page);
    area.setWidget(stack);

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(area.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(slider->isVisible(), 400);

    QList<AdTooltip*> tooltipHosts =
        slider->findChildren<AdTooltip*>(QString(), Qt::FindDirectChildrenOnly);
    QTRY_COMPARE_WITH_TIMEOUT(tooltipHosts.size(), 1, 400);
    AdTooltip* tooltip = tooltipHosts.constFirst();
    QVERIFY(tooltip != nullptr);
    QTRY_VERIFY_WITH_TIMEOUT(tooltip->open(), 400);

    QWidget* popup = host.findChild<QWidget*>(QStringLiteral("adpopover-popup"), Qt::FindChildrenRecursively);
    QTRY_VERIFY_WITH_TIMEOUT(popup != nullptr && popup->isVisible(), 400);

    QScrollBar* verticalBar = area.verticalScrollBar();
    QVERIFY(verticalBar != nullptr);
    QVERIFY(verticalBar->maximum() > verticalBar->minimum());

    for (int i = 0; i < 8; ++i) {
      verticalBar->setValue((i % 2 == 0) ? verticalBar->maximum() : verticalBar->minimum());
      QCoreApplication::processEvents();
    }
    verticalBar->setValue(verticalBar->maximum());
    QCoreApplication::processEvents();

    QTRY_VERIFY_WITH_TIMEOUT(popup != nullptr && !popup->isVisible(), 500);
  }

  void slider_tooltipAlwaysMode_scrollBurstCoalescesRelayouts() {
    QWidget host;
    host.resize(860, 620);

    QScrollArea area(&host);
    area.setGeometry(12, 12, 640, 460);
    area.setWidgetResizable(true);

    auto* stack = new QStackedWidget();
    auto* page = new QWidget();
    auto* pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(16, 16, 16, 16);
    pageLayout->setSpacing(14);

    auto addFillerSection = [pageLayout](const QString& title) {
      auto* section = new QFrame();
      section->setFrameShape(QFrame::StyledPanel);
      auto* sectionLayout = new QVBoxLayout(section);
      sectionLayout->setContentsMargins(12, 12, 12, 12);
      sectionLayout->setSpacing(8);
      auto* label = new QLabel(title);
      auto* filler = new QWidget();
      filler->setMinimumHeight(160);
      sectionLayout->addWidget(label);
      sectionLayout->addWidget(filler);
      pageLayout->addWidget(section);
    };

    addFillerSection(QStringLiteral("Section A"));

    auto* tooltipSection = new QFrame();
    tooltipSection->setFrameShape(QFrame::StyledPanel);
    auto* tooltipSectionLayout = new QVBoxLayout(tooltipSection);
    tooltipSectionLayout->setContentsMargins(12, 12, 12, 12);
    tooltipSectionLayout->setSpacing(8);
    tooltipSectionLayout->addWidget(new QLabel(QStringLiteral("Control visibility of Tooltip")));
    auto* slider = new AdSlider();
    slider->setValue(30);
    slider->setTooltipVisibleMode(AdSlider::TooltipVisibleMode::Always);
    tooltipSectionLayout->addWidget(slider);
    pageLayout->addWidget(tooltipSection);

    for (int i = 0; i < 8; ++i) {
      addFillerSection(QStringLiteral("Section %1").arg(i + 1));
    }

    stack->addWidget(page);
    stack->setCurrentWidget(page);
    area.setWidget(stack);

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(area.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(slider->isVisible(), 400);

    QList<AdTooltip*> tooltipHosts =
        slider->findChildren<AdTooltip*>(QString(), Qt::FindDirectChildrenOnly);
    QTRY_COMPARE_WITH_TIMEOUT(tooltipHosts.size(), 1, 400);
    AdTooltip* tooltip = tooltipHosts.constFirst();
    QVERIFY(tooltip != nullptr);
    QTRY_VERIFY_WITH_TIMEOUT(tooltip->open(), 400);

    QWidget* popup = host.findChild<QWidget*>(QStringLiteral("adpopover-popup"), Qt::FindChildrenRecursively);
    QTRY_VERIFY_WITH_TIMEOUT(popup != nullptr && popup->isVisible(), 400);

    QScrollBar* verticalBar = area.verticalScrollBar();
    QVERIFY(verticalBar != nullptr);
    QVERIFY(verticalBar->maximum() > verticalBar->minimum());

    AdPopover::resetSyncPopupGeometryCountersForTesting();
    for (int i = 0; i < 240; ++i) {
      verticalBar->setValue((i % 2 == 0) ? verticalBar->maximum() : verticalBar->minimum());
    }
    verticalBar->setValue(verticalBar->maximum());
    QCoreApplication::sendPostedEvents(nullptr, 0);
    QCoreApplication::processEvents();

    const qint64 syncCalls = AdPopover::syncPopupGeometryCallCountForTesting();
    const qint64 shortCircuitCalls = AdPopover::syncPopupGeometryShortCircuitCountForTesting();
    QVERIFY2(syncCalls <= 32,
             qPrintable(QStringLiteral("expected coalesced popup relayouts in scroll burst, calls=%1 shortCircuits=%2")
                            .arg(syncCalls)
                            .arg(shortCircuitCalls)));

    QTRY_VERIFY_WITH_TIMEOUT(popup != nullptr && !popup->isVisible(), 500);
  }

  void slider_tooltipAlwaysMode_worksInNestedScrollAreas() {
    QWidget host;
    host.resize(920, 700);

    QScrollArea outer(&host);
    outer.setGeometry(12, 12, 720, 560);
    outer.setWidgetResizable(true);

    auto* outerContent = new QWidget();
    auto* outerLayout = new QVBoxLayout(outerContent);
    outerLayout->setContentsMargins(16, 16, 16, 16);
    outerLayout->setSpacing(14);

    auto* outerTop = new QWidget();
    outerTop->setMinimumHeight(260);
    outerLayout->addWidget(outerTop);

    auto* inner = new QScrollArea();
    inner->setFixedHeight(260);
    inner->setWidgetResizable(false);

    auto* innerContent = new QWidget();
    innerContent->resize(560, 960);
    auto* slider = new AdSlider(innerContent);
    slider->setGeometry(24, 24, 320, 56);
    slider->setValue(42);
    slider->setTooltipVisibleMode(AdSlider::TooltipVisibleMode::Always);
    slider->show();
    inner->setWidget(innerContent);
    outerLayout->addWidget(inner);

    auto* outerBottom = new QWidget();
    outerBottom->setMinimumHeight(640);
    outerLayout->addWidget(outerBottom);

    outer.setWidget(outerContent);

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(outer.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(inner->isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(slider->isVisible(), 400);

    QList<AdTooltip*> tooltipHosts =
        slider->findChildren<AdTooltip*>(QString(), Qt::FindDirectChildrenOnly);
    QTRY_COMPARE_WITH_TIMEOUT(tooltipHosts.size(), 1, 400);
    AdTooltip* tooltip = tooltipHosts.constFirst();
    QVERIFY(tooltip != nullptr);
    QTRY_VERIFY_WITH_TIMEOUT(tooltip->open(), 400);

    QWidget* popup = host.findChild<QWidget*>(QStringLiteral("adpopover-popup"), Qt::FindChildrenRecursively);
    QTRY_VERIFY_WITH_TIMEOUT(popup != nullptr && popup->isVisible(), 400);

    QScrollBar* innerBar = inner->verticalScrollBar();
    QVERIFY(innerBar != nullptr);
    QVERIFY(innerBar->maximum() > innerBar->minimum());
    innerBar->setValue(innerBar->maximum());
    QCoreApplication::sendPostedEvents(nullptr, 0);
    QCoreApplication::processEvents();

    const QRect innerViewportGlobal = QRect(inner->viewport()->mapToGlobal(QPoint(0, 0)), inner->viewport()->size());
    const QRect sliderGlobalAfterInner = QRect(slider->mapToGlobal(QPoint(0, 0)), slider->size());
    QVERIFY(!sliderGlobalAfterInner.intersects(innerViewportGlobal));
    QTRY_VERIFY_WITH_TIMEOUT(popup != nullptr && !popup->isVisible(), 500);

    innerBar->setValue(innerBar->minimum());
    QCoreApplication::sendPostedEvents(nullptr, 0);
    QCoreApplication::processEvents();
    QTRY_VERIFY_WITH_TIMEOUT(popup != nullptr && popup->isVisible(), 500);

    QScrollBar* outerBar = outer.verticalScrollBar();
    QVERIFY(outerBar != nullptr);
    QVERIFY(outerBar->maximum() > outerBar->minimum());
    outerBar->setValue(outerBar->maximum());
    QCoreApplication::sendPostedEvents(nullptr, 0);
    QCoreApplication::processEvents();

    const QRect outerViewportGlobal = QRect(outer.viewport()->mapToGlobal(QPoint(0, 0)), outer.viewport()->size());
    const QRect sliderGlobalAfterOuter = QRect(slider->mapToGlobal(QPoint(0, 0)), slider->size());
    QVERIFY(!sliderGlobalAfterOuter.intersects(outerViewportGlobal));
    QTRY_VERIFY_WITH_TIMEOUT(popup != nullptr && !popup->isVisible(), 500);
  }

  void slider_multipleAlwaysTooltips_scrollBurstCoalescesRelayouts() {
    QScrollArea area;
    area.resize(420, 240);
    area.show();
    QTRY_VERIFY_WITH_TIMEOUT(area.isVisible(), 400);

    auto* content = new QWidget();
    content->resize(360, 1080);
    area.setWidget(content);
    area.setWidgetResizable(false);
    QTRY_VERIFY_WITH_TIMEOUT(content->isVisible(), 400);

    AdSlider topSlider(content);
    topSlider.setGeometry(24, 24, 280, 56);
    topSlider.setValue(25);
    topSlider.setTooltipVisibleMode(AdSlider::TooltipVisibleMode::Always);
    topSlider.show();

    AdSlider bottomSlider(content);
    bottomSlider.setGeometry(24, 116, 280, 56);
    bottomSlider.setValue(75);
    bottomSlider.setTooltipVisibleMode(AdSlider::TooltipVisibleMode::Always);
    bottomSlider.show();

    QTRY_VERIFY_WITH_TIMEOUT(topSlider.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(bottomSlider.isVisible(), 400);

    QList<AdTooltip*> topTips =
        topSlider.findChildren<AdTooltip*>(QString(), Qt::FindDirectChildrenOnly);
    QList<AdTooltip*> bottomTips =
        bottomSlider.findChildren<AdTooltip*>(QString(), Qt::FindDirectChildrenOnly);
    QTRY_COMPARE_WITH_TIMEOUT(topTips.size(), 1, 400);
    QTRY_COMPARE_WITH_TIMEOUT(bottomTips.size(), 1, 400);
    QTRY_VERIFY_WITH_TIMEOUT(topTips.constFirst()->open(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(bottomTips.constFirst()->open(), 400);

    QWidget* topPopup = nullptr;
    QWidget* bottomPopup = nullptr;
    auto popups = area.findChildren<QWidget*>(QStringLiteral("adpopover-popup"), Qt::FindChildrenRecursively);
    QTRY_VERIFY_WITH_TIMEOUT(popups.size() >= 2, 400);
    topPopup = popups.at(0);
    bottomPopup = popups.at(1);
    QVERIFY(topPopup != nullptr);
    QVERIFY(bottomPopup != nullptr);

    QScrollBar* verticalBar = area.verticalScrollBar();
    QVERIFY(verticalBar != nullptr);
    QVERIFY(verticalBar->maximum() > verticalBar->minimum());

    AdPopover::resetSyncPopupGeometryCountersForTesting();
    for (int i = 0; i < 260; ++i) {
      verticalBar->setValue((i % 2 == 0) ? verticalBar->maximum() : verticalBar->minimum());
    }
    verticalBar->setValue(verticalBar->maximum());
    QCoreApplication::sendPostedEvents(nullptr, 0);
    QCoreApplication::processEvents();

    const qint64 syncCalls = AdPopover::syncPopupGeometryCallCountForTesting();
    const qint64 shortCircuitCalls = AdPopover::syncPopupGeometryShortCircuitCountForTesting();
    QVERIFY2(syncCalls <= 64,
             qPrintable(QStringLiteral("expected coalesced relayout for multiple always tooltips, calls=%1 shortCircuits=%2")
                            .arg(syncCalls)
                            .arg(shortCircuitCalls)));
    QTRY_VERIFY_WITH_TIMEOUT(topPopup != nullptr && !topPopup->isVisible(), 500);
    QTRY_VERIFY_WITH_TIMEOUT(bottomPopup != nullptr && !bottomPopup->isVisible(), 500);
  }

  void popover_anchorDescendantMoveBurstSkipsRelayoutWhenAnchorStatic() {
    QWidget host;
    host.resize(760, 420);

    AdPopover popover(&host);
    popover.setGeometry(40, 48, 320, 120);

    auto* trigger = new QWidget();
    trigger->setFixedSize(220, 56);
    trigger->setAttribute(Qt::WA_Hover, true);
    trigger->setMouseTracking(true);

    auto* movingChild = new QWidget(trigger);
    movingChild->setGeometry(8, 10, 26, 26);
    movingChild->show();

    popover.setTriggerWidget(trigger);
    popover.setContentText(QStringLiteral("Descendant move perf"));

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(trigger->isVisible(), 400);

    popover.setOpen(true);
    QTRY_VERIFY_WITH_TIMEOUT(popover.open(), 400);
    QWidget* popup = host.findChild<QWidget*>(QStringLiteral("adpopover-popup"), Qt::FindChildrenRecursively);
    QTRY_VERIFY_WITH_TIMEOUT(popup != nullptr && popup->isVisible(), 400);

    // Let the initial frame-sync tail drain so the counter reflects descendant motion only.
    QTest::qWait(200);
    QCoreApplication::sendPostedEvents(nullptr, 0);
    QCoreApplication::processEvents();

    AdPopover::resetSyncPopupGeometryCountersForTesting();
    for (int i = 0; i < 120; ++i) {
      movingChild->move(8 + (i % 48), 10 + ((i / 48) % 2));
      QCoreApplication::sendPostedEvents(nullptr, 0);
      QCoreApplication::processEvents();
    }
    QCoreApplication::sendPostedEvents(nullptr, 0);
    QCoreApplication::processEvents();

    const qint64 syncCalls = AdPopover::syncPopupGeometryCallCountForTesting();
    const qint64 shortCircuitCalls = AdPopover::syncPopupGeometryShortCircuitCountForTesting();
    QVERIFY2(syncCalls <= 4,
             qPrintable(QStringLiteral("expected descendant move burst to skip anchor relayout, calls=%1 shortCircuits=%2")
                            .arg(syncCalls)
                            .arg(shortCircuitCalls)));
    QVERIFY(popup != nullptr && popup->isVisible());
  }

  void slider_multipleAlwaysTooltips_hideOutOfViewportPerAnchor() {
    QScrollArea area;
    area.resize(360, 220);
    area.show();
    QTRY_VERIFY_WITH_TIMEOUT(area.isVisible(), 400);

    auto* content = new QWidget();
    content->resize(320, 960);
    area.setWidget(content);
    area.setWidgetResizable(false);
    QTRY_VERIFY_WITH_TIMEOUT(content->isVisible(), 400);

    AdSlider topSlider(content);
    topSlider.setGeometry(24, 24, 240, 56);
    topSlider.setValue(20);
    topSlider.setTooltipVisibleMode(AdSlider::TooltipVisibleMode::Always);
    topSlider.show();

    AdSlider bottomSlider(content);
    bottomSlider.setGeometry(24, 140, 240, 56);
    bottomSlider.setValue(80);
    bottomSlider.setTooltipVisibleMode(AdSlider::TooltipVisibleMode::Always);
    bottomSlider.show();

    QTRY_VERIFY_WITH_TIMEOUT(topSlider.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(bottomSlider.isVisible(), 400);

    QList<AdTooltip*> topTips =
        topSlider.findChildren<AdTooltip*>(QString(), Qt::FindDirectChildrenOnly);
    QList<AdTooltip*> bottomTips =
        bottomSlider.findChildren<AdTooltip*>(QString(), Qt::FindDirectChildrenOnly);
    QTRY_COMPARE_WITH_TIMEOUT(topTips.size(), 1, 400);
    QTRY_COMPARE_WITH_TIMEOUT(bottomTips.size(), 1, 400);
    QTRY_VERIFY_WITH_TIMEOUT(topTips.constFirst()->open(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(bottomTips.constFirst()->open(), 400);

    // Nudge bottom slider so its tooltip host is the most recently synced popup owner.
    bottomSlider.setValue(81);
    bottomSlider.setValue(80);
    QCoreApplication::processEvents();

    auto nearestPopupToSlider = [&area](AdSlider* slider) -> QWidget* {
      if (!slider) {
        return nullptr;
      }
      QWidget* best = nullptr;
      qint64 bestDist = std::numeric_limits<qint64>::max();
      const QRect sliderRect = QRect(slider->mapToGlobal(QPoint(0, 0)), slider->size());
      const QPoint sliderCenter = sliderRect.center();
      const QList<QWidget*> popups =
          area.findChildren<QWidget*>(QStringLiteral("adpopover-popup"), Qt::FindChildrenRecursively);
      for (QWidget* popup : popups) {
        if (!popup || !popup->isVisible()) {
          continue;
        }
        const QRect popupRect = QRect(popup->mapToGlobal(QPoint(0, 0)), popup->size());
        const QPoint popupCenter = popupRect.center();
        const qint64 dist =
            std::llabs(static_cast<qint64>(popupCenter.x() - sliderCenter.x())) +
            std::llabs(static_cast<qint64>(popupCenter.y() - sliderCenter.y()));
        if (dist < bestDist) {
          bestDist = dist;
          best = popup;
        }
      }
      return best;
    };

    QWidget* topPopup = nullptr;
    QWidget* bottomPopup = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT((topPopup = nearestPopupToSlider(&topSlider)) != nullptr, 400);
    QTRY_VERIFY_WITH_TIMEOUT((bottomPopup = nearestPopupToSlider(&bottomSlider)) != nullptr, 400);
    QVERIFY(topPopup != bottomPopup);
    QVERIFY(topPopup->isVisible());
    QVERIFY(bottomPopup->isVisible());

    QScrollBar* verticalBar = area.verticalScrollBar();
    QVERIFY(verticalBar != nullptr);
    verticalBar->setValue(90);
    QCoreApplication::processEvents();

    const QRect viewportGlobal = QRect(area.viewport()->mapToGlobal(QPoint(0, 0)), area.viewport()->size());
    const QRect topSliderGlobal = QRect(topSlider.mapToGlobal(QPoint(0, 0)), topSlider.size());
    const QRect bottomSliderGlobal = QRect(bottomSlider.mapToGlobal(QPoint(0, 0)), bottomSlider.size());
    QVERIFY(!topSliderGlobal.intersects(viewportGlobal));
    QVERIFY(bottomSliderGlobal.intersects(viewportGlobal));

    QTRY_VERIFY_WITH_TIMEOUT(topPopup != nullptr && !topPopup->isVisible(), 500);
    QTRY_VERIFY_WITH_TIMEOUT(bottomPopup != nullptr && bottomPopup->isVisible(), 500);
  }

  void slider_tooltipVisualTokensAndFontFollowAntdTooltipModel() {
    QWidget host;
    host.resize(420, 280);

    AdSlider slider(&host);
    slider.setValue(30);
    slider.setTooltipVisibleMode(AdSlider::TooltipVisibleMode::Always);
    slider.resize(320, 56);
    slider.show();

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(slider.isVisible(), 400);

    QList<AdTooltip*> tooltipHosts =
        slider.findChildren<AdTooltip*>(QString(), Qt::FindDirectChildrenOnly);
    QTRY_COMPARE_WITH_TIMEOUT(tooltipHosts.size(), 1, 400);
    AdTooltip* tooltip = tooltipHosts.constFirst();
    QVERIFY(tooltip != nullptr);
    QTRY_VERIFY_WITH_TIMEOUT(tooltip->open(), 400);

    auto* popover = tooltip->findChild<AdPopover*>();
    QVERIFY(popover != nullptr);

    const AdPopover::ComponentTokens tokens = popover->componentTokens();
    const auto& map = ThemeManager::instance().currentMapToken();
    const auto& seed = ThemeManager::instance().currentConfig().seed;

    const int expectedBorderRadius = std::max(0, qRound(map.borderRadius));
    const int expectedArrowSize = std::max(0, qRound(seed.sizePopupArrow / 2.0));
    const int expectedPopupOffset = std::max(0, qRound(map.sizeXXS));
    const int expectedPaddingHorizontal = std::max(0, qRound(map.sizeXS));
    const int expectedPaddingVertical = std::max(0, qRound(map.sizeSM / 2.0));
    const int expectedFontSize = std::max(12, qRound(map.fontSize));

    QVERIFY(tokens.borderRadius.has_value());
    QCOMPARE(tokens.borderRadius.value(), expectedBorderRadius);
    QVERIFY(tokens.arrowSize.has_value());
    QCOMPARE(tokens.arrowSize.value(), expectedArrowSize);
    QVERIFY(tokens.popupOffset.has_value());
    QCOMPARE(tokens.popupOffset.value(), expectedPopupOffset);
    QVERIFY(tokens.contentPaddingHorizontal.has_value());
    QCOMPARE(tokens.contentPaddingHorizontal.value(), expectedPaddingHorizontal);
    QVERIFY(tokens.contentPaddingVertical.has_value());
    QCOMPARE(tokens.contentPaddingVertical.value(), expectedPaddingVertical);

    QWidget* contentHost = popover->contentWidget();
    QVERIFY(contentHost != nullptr);
    QLabel* tooltipLabel = contentHost->findChild<QLabel*>();
    QVERIFY(tooltipLabel != nullptr);
    QCOMPARE(tooltipLabel->font().pixelSize(), expectedFontSize);
  }

  void slider_semanticTrackAndRailOverrideAlsoAffectHoverState() {
    adqt::widgets::detail::SliderStyleInput input;
    input.baseFont = QFont();
    input.semanticStyles.rail.backgroundColor = QColor("#d3adf7");
    input.semanticStyles.track.backgroundColor = QColor("#722ed1");

    const auto style = adqt::widgets::detail::resolveSliderVisualStyle(input);
    QCOMPARE(style.railBg, QColor("#d3adf7"));
    QCOMPARE(style.railHoverBg, QColor("#d3adf7"));
    QCOMPARE(style.trackBg, QColor("#722ed1"));
    QCOMPARE(style.trackHoverBg, QColor("#722ed1"));

    adqt::widgets::detail::SliderStyleInput tracksInput;
    tracksInput.baseFont = QFont();
    tracksInput.semanticStyles.tracks.backgroundColor = QColor("#531dab");
    const auto tracksStyle = adqt::widgets::detail::resolveSliderVisualStyle(tracksInput);
    QCOMPARE(tracksStyle.trackBg, QColor("#531dab"));
    QCOMPARE(tracksStyle.trackHoverBg, QColor("#531dab"));
  }

  void slider_disabledTrackAndRailFollowAntdTokenMapping() {
    adqt::widgets::detail::SliderStyleInput input;
    input.disabled = true;
    input.baseFont = QFont();

    const auto style = adqt::widgets::detail::resolveSliderVisualStyle(input);
    const auto& map = ThemeManager::instance().currentMapToken();
    const adqt::theme::FastColorLite parsed(map.colorFillTertiary);
    QVERIFY(parsed.isValid());

    QColor expectedFillTertiary;
    expectedFillTertiary.setRed(parsed.red());
    expectedFillTertiary.setGreen(parsed.green());
    expectedFillTertiary.setBlue(parsed.blue());
    expectedFillTertiary.setAlphaF(parsed.alpha());

    QCOMPARE(style.railBg, expectedFillTertiary);
    QCOMPARE(style.trackBgDisabled, expectedFillTertiary);
    QCOMPARE(style.railHoverBg, style.railBg);
    QCOMPARE(style.trackBg, style.trackBgDisabled);
    QCOMPARE(style.trackHoverBg, style.trackBgDisabled);
  }

  void switch_toggleAndValueAliasEmitConsistentSignals() {
    AdSwitch sw;
    sw.resize(80, 32);
    sw.show();
    QTRY_VERIFY_WITH_TIMEOUT(sw.isVisible(), 400);

    QSignalSpy checkedSpy(&sw, &AdSwitch::checkedChanged);
    QSignalSpy valueSpy(&sw, &AdSwitch::valueChanged);
    QSignalSpy changedSpy(&sw, &AdSwitch::changed);

    QVERIFY(!sw.isChecked());
    QVERIFY(!sw.value());

    sw.setValue(true);
    QTRY_VERIFY_WITH_TIMEOUT(sw.isChecked(), 300);
    QVERIFY(sw.value());
    QCOMPARE(checkedSpy.count(), 1);
    QCOMPARE(valueSpy.count(), 1);
    QCOMPARE(changedSpy.count(), 1);

    QTest::mouseClick(&sw, Qt::LeftButton, Qt::NoModifier, sw.rect().center());
    QTRY_VERIFY_WITH_TIMEOUT(!sw.isChecked(), 300);
    QVERIFY(!sw.value());
    QCOMPARE(checkedSpy.count(), 2);
    QCOMPARE(valueSpy.count(), 2);
    QCOMPARE(changedSpy.count(), 2);
  }

  void switch_loadingBlocksInteractionLikeDisabled() {
    AdSwitch sw;
    sw.resize(80, 32);
    sw.setChecked(false);
    sw.setLoading(true);
    sw.show();
    QTRY_VERIFY_WITH_TIMEOUT(sw.isVisible(), 400);

    QTest::mouseClick(&sw, Qt::LeftButton, Qt::NoModifier, sw.rect().center());
    QTest::qWait(20);
    QVERIFY(!sw.isChecked());

    sw.setLoading(false);
    QTest::mouseClick(&sw, Qt::LeftButton, Qt::NoModifier, sw.rect().center());
    QTRY_VERIFY_WITH_TIMEOUT(sw.isChecked(), 300);
  }

  void switch_sizeHintRespondsToSizeAndContent() {
    AdSwitch plain;
    const QSize plainDefault = plain.sizeHint();
    adqt::widgets::detail::SwitchStyleInput defaultInput;
    defaultInput.size = AdSwitch::Size::Default;
    const auto defaultStyle = adqt::widgets::detail::resolveSwitchVisualStyle(defaultInput);
    QCOMPARE(plainDefault.width(), defaultStyle.metrics.trackMinWidth);
    QCOMPARE(plainDefault.height(), defaultStyle.metrics.trackHeight);

    plain.setSize(AdSwitch::Size::Small);
    const QSize plainSmall = plain.sizeHint();
    adqt::widgets::detail::SwitchStyleInput smallInput;
    smallInput.size = AdSwitch::Size::Small;
    const auto smallStyle = adqt::widgets::detail::resolveSwitchVisualStyle(smallInput);
    QCOMPARE(plainSmall.width(), smallStyle.metrics.trackMinWidthSM);
    QCOMPARE(plainSmall.height(), smallStyle.metrics.trackHeightSM);
    QVERIFY(plainSmall.height() < plainDefault.height());

    AdSwitch rich;
    rich.setCheckedChildren(QStringLiteral("ON"));
    rich.setUnCheckedChildren(QStringLiteral("OFF"));
    rich.setCheckedChildrenIconToken(adqt::icons::outlined::Check());
    rich.setUnCheckedChildrenIconToken(adqt::icons::outlined::Close());
    const QSize richSize = rich.sizeHint();
    QVERIFY(richSize.width() >= plainDefault.width());
  }

  void switch_sizeHintKeepsLargeHandleVisibleWithComponentTokens() {
    AdSwitch sw;
    AdSwitch::ComponentTokens tokens;
    tokens.trackHeight = 14;
    tokens.trackMinWidth = 32;
    tokens.trackPadding = 0;
    tokens.handleSize = 20;
    sw.setComponentTokens(tokens);

    const QSize hinted = sw.sizeHint();
    QVERIFY(hinted.height() >= 20);
    QVERIFY(hinted.width() >= 32);
  }

  void switch_styleResolverAppliesComponentAndSemanticOverrides() {
    adqt::widgets::detail::SwitchStyleInput input;
    input.size = AdSwitch::Size::Default;
    input.checked = true;
    input.componentTokens.colorPrimary = QStringLiteral("#123456");
    input.componentTokens.handleBg = QStringLiteral("#fedcba");
    input.componentTokens.loadingIconColor = QStringLiteral("#654321");
    input.componentTokens.disabledOpacity = 0.2;
    input.semanticStyles.content.textColor = QColor(QStringLiteral("#101010"));
    input.semanticStyles.indicator.borderColor = QColor(QStringLiteral("#222222"));

    const auto style = adqt::widgets::detail::resolveSwitchVisualStyle(input);
    QCOMPARE(style.trackCheckedBg, QColor(QStringLiteral("#123456")));
    QCOMPARE(style.handleBg, QColor(QStringLiteral("#fedcba")));
    QCOMPARE(style.loadingIconColor, QColor(QStringLiteral("#654321")));
    QCOMPARE(style.metrics.disabledOpacity, 0.2);
    QCOMPARE(style.contentColor, QColor(QStringLiteral("#101010")));
    QCOMPARE(style.handleBorderColor, QColor(QStringLiteral("#222222")));
  }

  void radio_standaloneCheckedClickDoesNotToggleOff() {
    AdRadio radio;
    radio.setText(QStringLiteral("Standalone"));
    radio.setValue(QStringLiteral("standalone"));
    radio.resize(180, 40);
    radio.show();
    QTRY_VERIFY_WITH_TIMEOUT(radio.isVisible(), 400);

    QSignalSpy toggledSpy(&radio, &QAbstractButton::toggled);
    QVERIFY(!radio.isChecked());

    QTest::mouseClick(&radio, Qt::LeftButton, Qt::NoModifier, radio.rect().center());
    QTRY_VERIFY_WITH_TIMEOUT(radio.isChecked(), 300);
    QCOMPARE(toggledSpy.count(), 1);

    QTest::mouseClick(&radio, Qt::LeftButton, Qt::NoModifier, radio.rect().center());
    QTest::qWait(20);
    QVERIFY(radio.isChecked());
    QCOMPARE(toggledSpy.count(), 1);
  }

  void radio_standaloneWithoutValueStaysUnchecked() {
    AdRadio radio;
    radio.setText(QStringLiteral("Standalone"));
    radio.resize(180, 40);
    radio.show();
    QTRY_VERIFY_WITH_TIMEOUT(radio.isVisible(), 400);

    QSignalSpy toggledSpy(&radio, &QAbstractButton::toggled);
    QVERIFY(!radio.value().isValid());
    QVERIFY(!radio.isChecked());

    QTest::mouseClick(&radio, Qt::LeftButton, Qt::NoModifier, radio.rect().center());
    QTest::qWait(20);
    QVERIFY(!radio.isChecked());
    QCOMPARE(toggledSpy.count(), 0);
  }

  void radio_groupValueChangeEmitsOnceOnRealChange() {
    AdRadioGroup group;

    AdRadioGroup::Option optA;
    optA.value = QStringLiteral("a");
    optA.label = QStringLiteral("A");
    AdRadioGroup::Option optB;
    optB.value = QStringLiteral("b");
    optB.label = QStringLiteral("B");
    AdRadioGroup::Option optC;
    optC.value = QStringLiteral("c");
    optC.label = QStringLiteral("C");
    group.setOptions({optA, optB, optC});
    group.setValue(optA.value);
    group.resize(280, 44);
    group.show();
    QTRY_VERIFY_WITH_TIMEOUT(group.isVisible(), 400);

    const QList<AdRadio*> radios = group.findChildren<AdRadio*>(QString(), Qt::FindDirectChildrenOnly);
    QCOMPARE(radios.size(), 3);

    QSignalSpy changedSpy(&group, &AdRadioGroup::changed);
    QSignalSpy valueChangedSpy(&group, &AdRadioGroup::valueChanged);

    QTest::mouseClick(radios.at(0), Qt::LeftButton, Qt::NoModifier, radios.at(0)->rect().center());
    QTest::qWait(20);
    QCOMPARE(changedSpy.count(), 0);
    QCOMPARE(valueChangedSpy.count(), 0);

    QTest::mouseClick(radios.at(1), Qt::LeftButton, Qt::NoModifier, radios.at(1)->rect().center());
    QTRY_COMPARE_WITH_TIMEOUT(changedSpy.count(), 1, 300);
    QTRY_COMPARE_WITH_TIMEOUT(valueChangedSpy.count(), 1, 300);
    QCOMPARE(group.value(), optB.value);

    QTest::mouseClick(radios.at(1), Qt::LeftButton, Qt::NoModifier, radios.at(1)->rect().center());
    QTest::qWait(20);
    QCOMPARE(changedSpy.count(), 1);
    QCOMPARE(valueChangedSpy.count(), 1);
  }

  void radio_groupDisabledAndOptionDisabledBothRespected() {
    AdRadioGroup group;

    AdRadioGroup::Option enabledA;
    enabledA.value = QStringLiteral("a");
    enabledA.label = QStringLiteral("Enabled A");
    AdRadioGroup::Option disabledB;
    disabledB.value = QStringLiteral("b");
    disabledB.label = QStringLiteral("Disabled B");
    disabledB.disabled = true;
    AdRadioGroup::Option enabledC;
    enabledC.value = QStringLiteral("c");
    enabledC.label = QStringLiteral("Enabled C");

    group.setOptions({enabledA, disabledB, enabledC});
    group.setValue(enabledA.value);
    group.resize(320, 44);
    group.show();
    QTRY_VERIFY_WITH_TIMEOUT(group.isVisible(), 400);

    const QList<AdRadio*> radios = group.findChildren<AdRadio*>(QString(), Qt::FindDirectChildrenOnly);
    QCOMPARE(radios.size(), 3);

    QVERIFY(!radios.at(0)->disabled());
    QVERIFY(radios.at(1)->disabled());
    QVERIFY(!radios.at(2)->disabled());

    group.setDisabled(true);
    QTRY_VERIFY_WITH_TIMEOUT(group.disabled(), 300);
    QVERIFY(radios.at(0)->disabled());
    QVERIFY(radios.at(1)->disabled());
    QVERIFY(radios.at(2)->disabled());

    group.setDisabled(false);
    QTRY_VERIFY_WITH_TIMEOUT(!group.disabled(), 300);
    QVERIFY(!radios.at(0)->disabled());
    QVERIFY(radios.at(1)->disabled());
    QVERIFY(!radios.at(2)->disabled());
  }

  void radio_groupBlockAppliesExpandingPolicy() {
    AdRadioGroup group;

    AdRadioGroup::Option option1;
    option1.value = QStringLiteral("1");
    option1.label = QStringLiteral("One");
    AdRadioGroup::Option option2;
    option2.value = QStringLiteral("2");
    option2.label = QStringLiteral("Two");

    group.setOptions({option1, option2});
    group.setBlock(true);
    group.resize(320, 44);
    group.show();
    QTRY_VERIFY_WITH_TIMEOUT(group.isVisible(), 400);

    const QList<AdRadio*> radios = group.findChildren<AdRadio*>(QString(), Qt::FindDirectChildrenOnly);
    QCOMPARE(radios.size(), 2);
    for (AdRadio* radio : radios) {
      QVERIFY(radio != nullptr);
      QCOMPARE(radio->sizePolicy().horizontalPolicy(), QSizePolicy::Expanding);
    }

    group.setBlock(false);
    for (AdRadio* radio : radios) {
      QVERIFY(radio != nullptr);
      QCOMPARE(radio->sizePolicy().horizontalPolicy(), QSizePolicy::Preferred);
    }
  }

  void radio_buttonGroupUsesCollapsedSpacing() {
    AdRadioGroup group;
    group.setOptionType(AdRadio::OptionType::Button);

    AdRadioGroup::Option option1;
    option1.value = QStringLiteral("1");
    option1.label = QStringLiteral("One");
    AdRadioGroup::Option option2;
    option2.value = QStringLiteral("2");
    option2.label = QStringLiteral("Two");
    AdRadioGroup::Option option3;
    option3.value = QStringLiteral("3");
    option3.label = QStringLiteral("Three");

    group.setOptions({option1, option2, option3});
    group.resize(420, 44);
    group.show();
    QTRY_VERIFY_WITH_TIMEOUT(group.isVisible(), 400);

    QLayout* layout = group.layout();
    QVERIFY(layout != nullptr);
    QCOMPARE(layout->spacing(), 0);
  }

  void radio_buttonGroupOverlapsAdjacentBordersByLineWidth() {
    AdRadioGroup group;
    group.setOptionType(AdRadio::OptionType::Button);

    AdRadioGroup::Option option1;
    option1.value = QStringLiteral("1");
    option1.label = QStringLiteral("One");
    AdRadioGroup::Option option2;
    option2.value = QStringLiteral("2");
    option2.label = QStringLiteral("Two");
    AdRadioGroup::Option option3;
    option3.value = QStringLiteral("3");
    option3.label = QStringLiteral("Three");

    group.setOptions({option1, option2, option3});
    group.resize(420, 44);
    group.show();
    QTRY_VERIFY_WITH_TIMEOUT(group.isVisible(), 400);

    QList<AdRadio*> radios = group.findChildren<AdRadio*>(QString(), Qt::FindDirectChildrenOnly);
    QCOMPARE(radios.size(), 3);
    std::sort(radios.begin(), radios.end(), [](const AdRadio* lhs, const AdRadio* rhs) {
      return lhs->geometry().x() < rhs->geometry().x();
    });

    const int lineWidth = std::max(1, qRound(ThemeManager::instance().currentMapToken().lineWidth));
    const int expectedOverlap = lineWidth + 1;
    for (int i = 1; i < radios.size(); ++i) {
      const QRect previous = radios.at(i - 1)->geometry();
      const QRect current = radios.at(i)->geometry();
      const int overlapPixels = previous.right() - current.left() + 1;
      QCOMPARE(overlapPixels, expectedOverlap);
    }
  }

  void radio_buttonGroupHoverDoesNotOverrideCheckedSharedBorderLayer() {
    AdRadioGroup group;
    group.setOptionType(AdRadio::OptionType::Button);

    AdRadioGroup::Option option1;
    option1.value = QStringLiteral("1");
    option1.label = QStringLiteral("One");
    AdRadioGroup::Option option2;
    option2.value = QStringLiteral("2");
    option2.label = QStringLiteral("Two");

    group.setOptions({option1, option2});
    group.setValue(option2.value);
    group.resize(320, 44);
    group.show();
    QTRY_VERIFY_WITH_TIMEOUT(group.isVisible(), 400);

    QList<AdRadio*> radios = group.findChildren<AdRadio*>(QString(), Qt::FindDirectChildrenOnly);
    QCOMPARE(radios.size(), 2);
    std::sort(radios.begin(), radios.end(), [](const AdRadio* lhs, const AdRadio* rhs) {
      return lhs->geometry().x() < rhs->geometry().x();
    });

    AdRadio* first = radios.at(0);
    AdRadio* second = radios.at(1);
    QVERIFY(first != nullptr);
    QVERIFY(second != nullptr);
    QVERIFY(second->isChecked());

    const int lineWidth = std::max(1, qRound(ThemeManager::instance().currentMapToken().lineWidth));
    const int overlap = lineWidth + 1;
    const int seamX = second->geometry().left() + (overlap - 1) / 2;
    const int seamY = second->geometry().center().y();
    const QPoint seamPoint(seamX, seamY);

    QWidget* topBeforeHover = group.childAt(seamPoint);
    QVERIFY(topBeforeHover == second || second->isAncestorOf(topBeforeHover));

    QTest::mouseMove(first, first->rect().center());
    QCoreApplication::processEvents();

    QWidget* topAfterHover = group.childAt(seamPoint);
    QVERIFY(topAfterHover == second || second->isAncestorOf(topAfterHover));
  }

  void radio_styleResolverRespectsButtonSolidAndDisabledChecked() {
    adqt::widgets::detail::RadioStyleInput input;
    input.baseFont = QFont();
    input.optionType = AdRadio::OptionType::Button;
    input.buttonStyle = AdRadio::ButtonStyle::Solid;
    input.checked = true;
    input.componentTokens.buttonSolidCheckedBg = QStringLiteral("#112233");
    input.componentTokens.buttonSolidCheckedColor = QStringLiteral("#fefefe");
    input.componentTokens.buttonCheckedBgDisabled = QStringLiteral("#334455");
    input.componentTokens.buttonCheckedColorDisabled = QStringLiteral("#778899");

    const adqt::widgets::detail::RadioVisualStyle enabledStyle =
        adqt::widgets::detail::resolveRadioVisualStyle(input);
    QCOMPARE(enabledStyle.buttonChecked.backgroundColor, QColor(QStringLiteral("#112233")));
    QCOMPARE(enabledStyle.buttonChecked.textColor, QColor(QStringLiteral("#fefefe")));

    input.disabled = true;
    const adqt::widgets::detail::RadioVisualStyle disabledStyle =
        adqt::widgets::detail::resolveRadioVisualStyle(input);
    QCOMPARE(disabledStyle.buttonChecked.backgroundColor, QColor(QStringLiteral("#334455")));
    QCOMPARE(disabledStyle.buttonChecked.textColor, QColor(QStringLiteral("#778899")));
    QCOMPARE(disabledStyle.buttonChecked.backgroundColor,
             disabledStyle.buttonCheckedDisabled.backgroundColor);
    QCOMPARE(disabledStyle.buttonChecked.textColor, disabledStyle.buttonCheckedDisabled.textColor);
  }

  void radio_styleResolverMatchesAntdHoverActiveBehavior() {
    adqt::widgets::detail::RadioStyleInput dotInput;
    dotInput.baseFont = QFont();
    dotInput.optionType = AdRadio::OptionType::Default;

    const adqt::widgets::detail::RadioVisualStyle dotStyle =
        adqt::widgets::detail::resolveRadioVisualStyle(dotInput);
    QCOMPARE(dotStyle.dotActive.borderColor, dotStyle.dotHover.borderColor);
    QCOMPARE(dotStyle.dotActive.backgroundColor, dotStyle.dotHover.backgroundColor);
    QCOMPARE(dotStyle.dotActive.dotColor, dotStyle.dotHover.dotColor);

    adqt::widgets::detail::RadioStyleInput buttonInput;
    buttonInput.baseFont = QFont();
    buttonInput.optionType = AdRadio::OptionType::Button;

    const adqt::widgets::detail::RadioVisualStyle buttonStyle =
        adqt::widgets::detail::resolveRadioVisualStyle(buttonInput);
    QCOMPARE(buttonStyle.buttonHover.borderColor, buttonStyle.buttonNormal.borderColor);
    QCOMPARE(buttonStyle.buttonActive.borderColor, buttonStyle.buttonHover.borderColor);
    QCOMPARE(buttonStyle.buttonActive.textColor, buttonStyle.buttonHover.textColor);
  }

  void radio_checkedHoverKeepsBorderVisibleToAvoidVisualShrink() {
    adqt::widgets::detail::RadioStyleInput input;
    input.baseFont = QFont();
    input.optionType = AdRadio::OptionType::Default;
    input.checked = true;
    input.hovered = true;

    const adqt::widgets::detail::RadioVisualStyle style =
        adqt::widgets::detail::resolveRadioVisualStyle(input);
    QVERIFY(style.dotCheckedHover.borderColor.alpha() > 0);
    QCOMPARE(style.dotCheckedHover.borderColor, style.dotCheckedHover.backgroundColor);
  }

  void radio_semanticStyleResolverCanOverrideCheckedVisual() {
    const auto resolver = [](const AdRadio::StyleContext& ctx) {
      AdRadio::SemanticStyles styles;
      if (ctx.checked) {
        styles.icon.borderColor = QColor(QStringLiteral("#fa541c"));
        styles.icon.backgroundColor = QColor(QStringLiteral("#fff2e8"));
        styles.label.textColor = QColor(QStringLiteral("#d4380d"));
      } else {
        styles.icon.borderColor = QColor(QStringLiteral("#8c8c8c"));
        styles.icon.backgroundColor = QColor(QStringLiteral("#fafafa"));
        styles.label.textColor = QColor(QStringLiteral("#595959"));
      }
      return styles;
    };

    AdRadio::StyleContext checkedContext;
    checkedContext.checked = true;
    checkedContext.optionType = AdRadio::OptionType::Default;
    checkedContext.buttonStyle = AdRadio::ButtonStyle::Outline;

    adqt::widgets::detail::RadioStyleInput checkedInput;
    checkedInput.baseFont = QFont();
    checkedInput.checked = true;
    checkedInput.semanticStyles = resolver(checkedContext);
    const adqt::widgets::detail::RadioVisualStyle checkedStyle =
        adqt::widgets::detail::resolveRadioVisualStyle(checkedInput);

    QCOMPARE(checkedStyle.dotChecked.borderColor, QColor(QStringLiteral("#fa541c")));
    QCOMPARE(checkedStyle.dotChecked.backgroundColor, QColor(QStringLiteral("#fff2e8")));
    QCOMPARE(checkedStyle.dotChecked.labelColor, QColor(QStringLiteral("#d4380d")));

    AdRadio::StyleContext uncheckedContext;
    uncheckedContext.checked = false;
    uncheckedContext.optionType = AdRadio::OptionType::Default;
    uncheckedContext.buttonStyle = AdRadio::ButtonStyle::Outline;

    adqt::widgets::detail::RadioStyleInput uncheckedInput;
    uncheckedInput.baseFont = QFont();
    uncheckedInput.checked = false;
    uncheckedInput.semanticStyles = resolver(uncheckedContext);
    const adqt::widgets::detail::RadioVisualStyle uncheckedStyle =
        adqt::widgets::detail::resolveRadioVisualStyle(uncheckedInput);

    QCOMPARE(uncheckedStyle.dotNormal.borderColor, QColor(QStringLiteral("#8c8c8c")));
    QCOMPARE(uncheckedStyle.dotNormal.backgroundColor, QColor(QStringLiteral("#fafafa")));
    QCOMPARE(uncheckedStyle.dotNormal.labelColor, QColor(QStringLiteral("#595959")));
  }

  void tooltip_hoverTriggerOpensAndCloses() {
    QWidget host;
    host.resize(420, 280);

    AdTooltip tooltip(&host);
    tooltip.setGeometry(24, 40, 180, 40);
    tooltip.setTitleText(QStringLiteral("prompt text"));
    tooltip.setTriggerModes(AdTooltip::Trigger::Hover);
    tooltip.setMouseEnterDelayMs(0);
    tooltip.setMouseLeaveDelayMs(0);

    auto* trigger = new QPushButton(QStringLiteral("trigger"), &tooltip);
    tooltip.setTriggerWidget(trigger);
    tooltip.show();

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(trigger->isVisible(), 400);

    QCursor::setPos(trigger->mapToGlobal(trigger->rect().center()));
    QEvent enterEvent(QEvent::Enter);
    QCoreApplication::sendEvent(trigger, &enterEvent);
    QTRY_VERIFY_WITH_TIMEOUT(tooltip.open(), 400);

    QCursor::setPos(host.mapToGlobal(QPoint(host.width() - 4, host.height() - 4)));
    QEvent leaveEvent(QEvent::Leave);
    QCoreApplication::sendEvent(trigger, &leaveEvent);
    QTRY_VERIFY_WITH_TIMEOUT(!tooltip.open(), 400);
  }

  void tooltip_destroyOnHiddenHoverCanReopenAfterClose() {
    QWidget host;
    host.resize(420, 280);

    AdTooltip tooltip(&host);
    tooltip.setGeometry(24, 40, 220, 40);
    tooltip.setTitleText(QStringLiteral("prompt text"));
    tooltip.setTriggerModes(AdTooltip::Trigger::Hover);
    tooltip.setDestroyOnHidden(true);
    tooltip.setMouseEnterDelayMs(0);
    tooltip.setMouseLeaveDelayMs(0);

    auto* trigger = new QPushButton(QStringLiteral("trigger"), &tooltip);
    tooltip.setTriggerWidget(trigger);
    tooltip.show();

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(trigger->isVisible(), 400);

    auto* popover = tooltip.findChild<AdPopover*>();
    QVERIFY(popover != nullptr);

    QCursor::setPos(trigger->mapToGlobal(trigger->rect().center()));
    QEvent firstEnter(QEvent::Enter);
    QCoreApplication::sendEvent(trigger, &firstEnter);
    QTRY_VERIFY_WITH_TIMEOUT(tooltip.open(), 400);

    QCursor::setPos(host.mapToGlobal(QPoint(host.width() - 4, host.height() - 4)));
    QEvent firstLeave(QEvent::Leave);
    QCoreApplication::sendEvent(trigger, &firstLeave);
    QTRY_VERIFY_WITH_TIMEOUT(!tooltip.open(), 400);

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
    QVERIFY(popover->contentWidget() != nullptr);

    QCursor::setPos(trigger->mapToGlobal(trigger->rect().center()));
    QEvent secondEnter(QEvent::Enter);
    QCoreApplication::sendEvent(trigger, &secondEnter);
    QTRY_VERIFY_WITH_TIMEOUT(tooltip.open(), 400);
  }

  void tooltip_contentHostDoesNotBlockTriggerHitTest() {
    QWidget host;
    host.resize(420, 280);

    AdTooltip tooltip(&host);
    tooltip.setGeometry(24, 40, 180, 40);
    tooltip.setTitleText(QStringLiteral("prompt text"));

    auto* trigger = new QPushButton(QStringLiteral("trigger"), &tooltip);
    tooltip.setTriggerWidget(trigger);
    tooltip.show();

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(trigger->isVisible(), 400);

    const QPoint triggerCenterInTooltip = trigger->mapTo(&tooltip, trigger->rect().center());
    QWidget* directHit = tooltip.childAt(triggerCenterInTooltip);
    QVERIFY(directHit != nullptr);
    QVERIFY(directHit == trigger || trigger->isAncestorOf(directHit));
  }

  void tooltip_emptyTitleStaysClosed() {
    QWidget host;
    host.resize(360, 220);

    AdTooltip tooltip(&host);
    tooltip.setGeometry(20, 24, 160, 40);
    tooltip.setTriggerModes(AdTooltip::Trigger::Click);

    auto* trigger = new QPushButton(QStringLiteral("trigger"), &tooltip);
    tooltip.setTriggerWidget(trigger);
    tooltip.show();

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(trigger->isVisible(), 400);

    sendMouseClick(trigger);
    QTest::qWait(80);
    QVERIFY(!tooltip.open());
  }

  void tooltip_programmaticOpenAndClose() {
    QWidget host;
    host.resize(420, 280);

    AdTooltip tooltip(&host);
    tooltip.setGeometry(24, 40, 180, 40);
    tooltip.setTitleText(QStringLiteral("prompt text"));
    tooltip.show();

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(tooltip.isVisible(), 400);

    tooltip.setOpen(true);
    QTRY_VERIFY_WITH_TIMEOUT(tooltip.open(), 400);

    QWidget* popup = host.findChild<QWidget*>(QStringLiteral("adpopover-popup"), Qt::FindChildrenRecursively);
    QTRY_VERIFY_WITH_TIMEOUT(popup != nullptr && popup->isVisible(), 400);

    tooltip.setOpen(false);
    QTRY_VERIFY_WITH_TIMEOUT(!tooltip.open(), 400);
  }

  void tooltip_controlledProgrammaticSetOpenUpdatesVisibleState() {
    QWidget host;
    host.resize(420, 280);

    AdTooltip tooltip(&host);
    tooltip.setGeometry(24, 40, 180, 40);
    tooltip.setTitleText(QStringLiteral("prompt text"));
    tooltip.setOpenControlled(true);
    tooltip.setOpen(false);
    tooltip.show();

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);

    QSignalSpy openChangedSpy(&tooltip, &AdTooltip::openChanged);

    tooltip.setOpen(true);
    QTRY_VERIFY_WITH_TIMEOUT(tooltip.open(), 400);
    QCOMPARE(openChangedSpy.count(), 1);

    tooltip.setOpen(false);
    QTRY_VERIFY_WITH_TIMEOUT(!tooltip.open(), 400);
    QCOMPARE(openChangedSpy.count(), 2);
  }

  void tooltip_colorAutoContrastForCustomColor() {
    QWidget host;
    host.resize(420, 280);

    AdTooltip tooltip(&host);
    tooltip.setGeometry(24, 40, 180, 40);
    tooltip.setTitleText(QStringLiteral("Contrast text"));
    tooltip.setColor(QStringLiteral("#003366"));
    tooltip.setOpen(true);

    auto* trigger = new QPushButton(QStringLiteral("trigger"), &tooltip);
    tooltip.setTriggerWidget(trigger);
    tooltip.show();

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(tooltip.open(), 400);

    QWidget* popup = host.findChild<QWidget*>(QStringLiteral("adpopover-popup"), Qt::FindChildrenRecursively);
    QTRY_VERIFY_WITH_TIMEOUT(popup != nullptr && popup->isVisible(), 400);

    QLabel* contentLabel = nullptr;
    const QList<QLabel*> labels = popup->findChildren<QLabel*>();
    for (QLabel* label : labels) {
      if (!label) {
        continue;
      }
      if (label->text() == QStringLiteral("Contrast text")) {
        contentLabel = label;
        break;
      }
    }
    QVERIFY(contentLabel != nullptr);

    const QColor textColor = contentLabel->palette().color(QPalette::WindowText);
    QVERIFY(textColor.red() >= 240);
    QVERIFY(textColor.green() >= 240);
    QVERIFY(textColor.blue() >= 240);
  }

  void tooltip_translucentColorPreservesPopupBackgroundOpacity() {
    AdTooltip tooltip;
    tooltip.setTitleText(QStringLiteral("prompt text"));
    tooltip.setColor(QStringLiteral("rgba(20, 20, 20, 0.85)"));

    auto* popover = tooltip.findChild<AdPopover*>();
    QVERIFY(popover != nullptr);

    const AdPopover::ComponentTokens tokens = popover->componentTokens();
    QVERIFY(tokens.popupBg.has_value());

    const adqt::theme::FastColorLite parsed(tokens.popupBg.value());
    QVERIFY(parsed.isValid());
    QCOMPARE(parsed.red(), 20);
    QCOMPARE(parsed.green(), 20);
    QCOMPARE(parsed.blue(), 20);
    QVERIFY(parsed.alpha() > 0.80);
    QVERIFY(parsed.alpha() < 0.90);
  }

  void tooltip_defaultVisualTokensFollowAntdSizingModel() {
    QWidget host;
    host.resize(420, 280);

    AdTooltip tooltip(&host);
    tooltip.setGeometry(24, 40, 180, 40);
    tooltip.setTitleText(QStringLiteral("prompt text"));

    auto* trigger = new QPushButton(QStringLiteral("trigger"), &tooltip);
    tooltip.setTriggerWidget(trigger);
    tooltip.show();

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(tooltip.isVisible(), 400);

    auto* popover = tooltip.findChild<AdPopover*>();
    QVERIFY(popover != nullptr);

    const AdPopover::ComponentTokens tokens = popover->componentTokens();
    const auto& map = ThemeManager::instance().currentMapToken();
    const auto& seed = ThemeManager::instance().currentConfig().seed;

    const int expectedBorderRadius = std::max(0, qRound(map.borderRadius));
    const int expectedArrowSize = std::max(0, qRound(seed.sizePopupArrow / 2.0));
    const int expectedPopupOffset = std::max(0, qRound(map.sizeXXS));
    const int expectedPaddingHorizontal = std::max(0, qRound(map.sizeXS));
    const int expectedPaddingVertical = std::max(0, qRound(map.sizeSM / 2.0));
    const int expectedBodyMinWidth = std::max(1, expectedBorderRadius * 2 + expectedArrowSize * 2);
    const int expectedBodyMinHeight = std::max(0, qRound(map.controlHeight));
    const int expectedContentMinWidth =
        std::max(1, expectedBodyMinWidth - expectedPaddingHorizontal * 2);
    const int expectedContentMinHeight =
        std::max(0, expectedBodyMinHeight - expectedPaddingVertical * 2);

    QVERIFY(tokens.borderRadius.has_value());
    QCOMPARE(tokens.borderRadius.value(), expectedBorderRadius);
    QVERIFY(tokens.arrowSize.has_value());
    QCOMPARE(tokens.arrowSize.value(), expectedArrowSize);
    QVERIFY(tokens.popupOffset.has_value());
    QCOMPARE(tokens.popupOffset.value(), expectedPopupOffset);
    QVERIFY(tokens.contentPaddingHorizontal.has_value());
    QCOMPARE(tokens.contentPaddingHorizontal.value(), expectedPaddingHorizontal);
    QVERIFY(tokens.contentPaddingVertical.has_value());
    QCOMPARE(tokens.contentPaddingVertical.value(), expectedPaddingVertical);

    QWidget* contentHost = popover->contentWidget();
    QVERIFY(contentHost != nullptr);
    QCOMPARE(contentHost->minimumWidth(), expectedContentMinWidth);
    QCOMPARE(contentHost->minimumHeight(), expectedContentMinHeight);
  }

  void tooltip_autoAdjustKeepsPopupInBounds() {
    QWidget host;
    host.resize(420, 260);

    AdTooltip tooltip(&host);
    tooltip.setGeometry(4, 4, 140, 40);
    tooltip.setTitleText(QStringLiteral("A short message to measure popup bounds"));
    tooltip.setTriggerModes(AdTooltip::Trigger::Click);
    tooltip.setPlacement(AdTooltip::Placement::TopLeft);
    tooltip.setAutoAdjustOverflow(true);

    auto* trigger = new QPushButton(QStringLiteral("trigger"), &tooltip);
    tooltip.setTriggerWidget(trigger);
    tooltip.show();

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(tooltip.isVisible(), 400);

    tooltip.setOpen(true);
    QTRY_VERIFY_WITH_TIMEOUT(tooltip.open(), 400);

    QWidget* popup = host.findChild<QWidget*>(QStringLiteral("adpopover-popup"), Qt::FindChildrenRecursively);
    QTRY_VERIFY_WITH_TIMEOUT(popup != nullptr && popup->isVisible(), 400);

    const QRect hostRect = QRect(host.mapToGlobal(QPoint(0, 0)), host.size());
    const QRect popupRect = QRect(popup->mapToGlobal(QPoint(0, 0)), popup->size());
    const QRect tolerantHostRect = hostRect.adjusted(-2, -2, 2, 2);
    QVERIFY(tolerantHostRect.contains(popupRect.topLeft()));
    QVERIFY(tolerantHostRect.contains(popupRect.topRight()));
    QVERIFY(tolerantHostRect.contains(popupRect.bottomLeft()));
    QVERIFY(tolerantHostRect.contains(popupRect.bottomRight()));
  }

  void popover_clickTriggerTogglesOpen() {
    QWidget host;
    host.resize(480, 320);

    AdPopover popover(&host);
    popover.setGeometry(48, 64, 160, 40);
    popover.setTitleText(QStringLiteral("Title"));
    popover.setContentText(QStringLiteral("Content"));
    popover.setTriggerModes(AdPopover::Trigger::Click);
    QVERIFY(popover.triggerModes().testFlag(AdPopover::Trigger::Click));

    auto* trigger = new QPushButton(QStringLiteral("trigger"), &popover);
    popover.setTriggerWidget(trigger);
    popover.show();

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(popover.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(trigger->isVisible(), 400);
    QVERIFY(trigger->width() > 0);
    QVERIFY(trigger->height() > 0);

    sendMouseClick(trigger);
    QTRY_VERIFY_WITH_TIMEOUT(popover.open(), 400);

    QWidget* popup = host.findChild<QWidget*>(QStringLiteral("adpopover-popup"), Qt::FindChildrenRecursively);
    QTRY_VERIFY_WITH_TIMEOUT(popup != nullptr && popup->isVisible(), 400);

    sendMouseClick(trigger);
    QTRY_VERIFY_WITH_TIMEOUT(!popover.open(), 400);
  }

  void popover_defaultPopupPaddingMatchesAntdInnerSpacing() {
    QWidget host;
    host.resize(420, 280);

    AdPopover popover(&host);
    popover.setGeometry(48, 64, 180, 40);
    popover.setPlacement(AdPopover::Placement::Top);
    popover.setTriggerModes(AdPopover::Trigger::Click);
    popover.setContentText(QStringLiteral("Padding probe content"));

    auto* trigger = new QPushButton(QStringLiteral("trigger"), &popover);
    popover.setTriggerWidget(trigger);
    popover.show();

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(popover.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(trigger->isVisible(), 400);

    popover.setOpen(true);
    QTRY_VERIFY_WITH_TIMEOUT(popover.open(), 400);

    QWidget* popup = host.findChild<QWidget*>(QStringLiteral("adpopover-popup"), Qt::FindChildrenRecursively);
    QTRY_VERIFY_WITH_TIMEOUT(popup != nullptr && popup->isVisible(), 400);

    QLabel* contentLabel = nullptr;
    const QList<QLabel*> labels = popup->findChildren<QLabel*>();
    for (QLabel* label : labels) {
      if (!label || !label->isVisible()) {
        continue;
      }
      if (label->text() == QStringLiteral("Padding probe content")) {
        contentLabel = label;
        break;
      }
    }
    QVERIFY(contentLabel != nullptr);

    const QRect popupRect = QRect(popup->mapToGlobal(QPoint(0, 0)), popup->size());
    const QRect contentRect = QRect(contentLabel->mapToGlobal(QPoint(0, 0)), contentLabel->size());
    const int topInset = contentRect.top() - popupRect.top();
    const int leftInset = contentRect.left() - popupRect.left();

    QCOMPARE(topInset, 12);
    QCOMPARE(leftInset, 12);
  }

  void popover_titleContentGapDoesNotJumpOnPopupHover() {
    QWidget host;
    host.resize(420, 280);

    AdPopover popover(&host);
    popover.setGeometry(48, 64, 180, 40);
    popover.setPlacement(AdPopover::Placement::Top);
    popover.setTriggerModes(AdPopover::Trigger::Hover);
    popover.setMouseEnterDelayMs(0);
    popover.setTitleText(QStringLiteral("Title"));
    popover.setContentText(QStringLiteral("Content"));

    auto* trigger = new QPushButton(QStringLiteral("trigger"), &popover);
    popover.setTriggerWidget(trigger);
    popover.show();

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(popover.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(trigger->isVisible(), 400);

    popover.setOpen(true);
    QTRY_VERIFY_WITH_TIMEOUT(popover.open(), 400);

    QWidget* popup = host.findChild<QWidget*>(QStringLiteral("adpopover-popup"), Qt::FindChildrenRecursively);
    QTRY_VERIFY_WITH_TIMEOUT(popup != nullptr && popup->isVisible(), 400);

    auto findVisibleLabelByText = [popup](const QString& text) -> QLabel* {
      if (!popup) {
        return nullptr;
      }
      const QList<QLabel*> labels = popup->findChildren<QLabel*>();
      for (QLabel* label : labels) {
        if (!label || !label->isVisible()) {
          continue;
        }
        if (label->text() == text) {
          return label;
        }
      }
      return nullptr;
    };

    QLabel* titleLabel = nullptr;
    QLabel* contentLabel = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT((titleLabel = findVisibleLabelByText(QStringLiteral("Title"))) != nullptr, 400);
    QTRY_VERIFY_WITH_TIMEOUT((contentLabel = findVisibleLabelByText(QStringLiteral("Content"))) != nullptr, 400);

    auto titleContentGap = [](QLabel* title, QLabel* content) {
      if (!title || !content) {
        return -1;
      }
      const QRect titleRect = QRect(title->mapToGlobal(QPoint(0, 0)), title->size());
      const QRect contentRect = QRect(content->mapToGlobal(QPoint(0, 0)), content->size());
      return contentRect.top() - titleRect.bottom() - 1;
    };

    const int gapBeforeHover = titleContentGap(titleLabel, contentLabel);
    QVERIFY(gapBeforeHover >= 0);

    QEvent popupEnterEvent(QEvent::Enter);
    QCoreApplication::sendEvent(popup, &popupEnterEvent);
    QCoreApplication::processEvents();

    const int gapAfterHover = titleContentGap(titleLabel, contentLabel);
    QCOMPARE(gapAfterHover, gapBeforeHover);
  }

  void popover_clickOpensOnReleaseNotPress() {
    QWidget host;
    host.resize(420, 280);

    AdPopover popover(&host);
    popover.setGeometry(32, 50, 180, 40);
    popover.setTitleText(QStringLiteral("Title"));
    popover.setContentText(QStringLiteral("Content"));
    popover.setTriggerModes(AdPopover::Trigger::Click);

    auto* trigger = new QPushButton(QStringLiteral("trigger"), &popover);
    popover.setTriggerWidget(trigger);
    popover.show();

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(trigger->isVisible(), 400);

    sendMousePress(trigger);
    QTest::qWait(10);
    QVERIFY(!popover.open());

    sendMouseRelease(trigger);
    QTRY_VERIFY_WITH_TIMEOUT(popover.open(), 400);
  }

  void popover_clickTriggerKeyboardActivationTogglesOpen() {
    QWidget host;
    host.resize(480, 320);

    AdPopover popover(&host);
    popover.setGeometry(48, 64, 160, 40);
    popover.setTitleText(QStringLiteral("Title"));
    popover.setContentText(QStringLiteral("Content"));
    popover.setTriggerModes(AdPopover::Trigger::Click);

    auto* trigger = new QPushButton(QStringLiteral("trigger"), &popover);
    popover.setTriggerWidget(trigger);
    popover.show();

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(popover.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(trigger->isVisible(), 400);

    trigger->setFocus();
    QTRY_VERIFY_WITH_TIMEOUT(trigger->hasFocus(), 400);
    QTest::keyClick(trigger, Qt::Key_Space);
    QTRY_VERIFY_WITH_TIMEOUT(popover.open(), 400);

    QTest::keyClick(trigger, Qt::Key_Space);
    QTRY_VERIFY_WITH_TIMEOUT(!popover.open(), 400);
  }

  void popover_controlledModeRequestsButDoesNotMutateOpen() {
    QWidget host;
    host.resize(480, 320);

    AdPopover popover(&host);
    popover.setGeometry(48, 64, 160, 40);
    popover.setTitleText(QStringLiteral("Title"));
    popover.setContentText(QStringLiteral("Content"));
    popover.setTriggerModes(AdPopover::Trigger::Click);
    popover.setOpenControlled(true);
    popover.setOpen(false);

    auto* trigger = new QPushButton(QStringLiteral("trigger"), &popover);
    popover.setTriggerWidget(trigger);
    popover.show();

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(popover.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(trigger->isVisible(), 400);

    QSignalSpy openChangedSpy(&popover, &AdPopover::openChanged);
    QSignalSpy requestSpy(&popover, &AdPopover::onOpenChange);

    sendMouseClick(trigger);
    QVERIFY(!popover.open());
    QTRY_COMPARE_WITH_TIMEOUT(requestSpy.count(), 1, 400);
    QCOMPARE(requestSpy.at(0).at(0).toBool(), true);
    QCOMPARE(openChangedSpy.count(), 0);

    popover.setOpen(true);
    QTRY_VERIFY_WITH_TIMEOUT(popover.open(), 400);
    QCOMPARE(openChangedSpy.count(), 1);

    sendMouseClick(trigger);
    QVERIFY(popover.open());
    QTRY_COMPARE_WITH_TIMEOUT(requestSpy.count(), 2, 400);
    QCOMPARE(requestSpy.at(1).at(0).toBool(), false);
    QCOMPARE(openChangedSpy.count(), 1);
  }

  void popover_controlledHoverDoesNotRequestCloseWhenAlreadyClosed() {
    QWidget host;
    host.resize(480, 320);

    AdPopover popover(&host);
    popover.setGeometry(48, 64, 160, 40);
    popover.setTitleText(QStringLiteral("Title"));
    popover.setContentText(QStringLiteral("Content"));
    popover.setTriggerModes(AdPopover::Trigger::Hover);
    popover.setMouseEnterDelayMs(0);
    popover.setMouseLeaveDelayMs(0);
    popover.setOpenControlled(true);
    popover.setOpen(false);

    auto* trigger = new QPushButton(QStringLiteral("trigger"), &popover);
    popover.setTriggerWidget(trigger);
    popover.show();

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(popover.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(trigger->isVisible(), 400);

    QSignalSpy requestSpy(&popover, &AdPopover::onOpenChange);

    QCursor::setPos(host.mapToGlobal(QPoint(host.width() - 8, host.height() - 8)));
    QCursor::setPos(trigger->mapToGlobal(trigger->rect().center()));
    QEvent enterEvent(QEvent::Enter);
    QCoreApplication::sendEvent(trigger, &enterEvent);
    QTRY_COMPARE_WITH_TIMEOUT(requestSpy.count(), 1, 400);
    QCOMPARE(requestSpy.at(0).at(0).toBool(), true);
    QVERIFY(!popover.open());

    const int requestCountAfterEnter = requestSpy.count();
    QCursor::setPos(host.mapToGlobal(QPoint(host.width() - 8, host.height() - 8)));
    QEvent leaveEvent(QEvent::Leave);
    QCoreApplication::sendEvent(trigger, &leaveEvent);
    QTest::qWait(60);
    QCOMPARE(requestSpy.count(), requestCountAfterEnter);
    QVERIFY(!popover.open());
  }

  void popover_controlledHoverDelayedOpenUsesHoverStateNotOnlyCursorPos() {
    QWidget host;
    host.resize(480, 320);

    AdPopover popover(&host);
    popover.setGeometry(48, 64, 160, 40);
    popover.setTitleText(QStringLiteral("Title"));
    popover.setContentText(QStringLiteral("Content"));
    popover.setTriggerModes(AdPopover::Trigger::Hover);
    popover.setMouseEnterDelayMs(80);
    popover.setMouseLeaveDelayMs(80);
    popover.setOpenControlled(true);
    popover.setOpen(false);

    auto* trigger = new QPushButton(QStringLiteral("trigger"), &popover);
    popover.setTriggerWidget(trigger);
    popover.show();

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(popover.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(trigger->isVisible(), 400);

    QSignalSpy requestSpy(&popover, &AdPopover::onOpenChange);

    // Keep system cursor away from trigger to verify delayed open is driven by hover state.
    QCursor::setPos(host.mapToGlobal(QPoint(host.width() - 8, host.height() - 8)));
    QEvent enterEvent(QEvent::Enter);
    QCoreApplication::sendEvent(trigger, &enterEvent);
    QTRY_COMPARE_WITH_TIMEOUT(requestSpy.count(), 1, 500);
    QCOMPARE(requestSpy.at(0).at(0).toBool(), true);
  }

  void popover_controlledModeWithEmptyContentDoesNotRequestOpen() {
    QWidget host;
    host.resize(420, 280);

    AdPopover popover(&host);
    popover.setGeometry(32, 50, 180, 40);
    popover.setTriggerModes(AdPopover::Trigger::Click);
    popover.setOpenControlled(true);
    popover.setOpen(false);

    auto* trigger = new QPushButton(QStringLiteral("trigger"), &popover);
    popover.setTriggerWidget(trigger);
    popover.show();

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(popover.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(trigger->isVisible(), 400);

    QSignalSpy requestSpy(&popover, &AdPopover::onOpenChange);
    QSignalSpy openChangedSpy(&popover, &AdPopover::openChanged);

    sendMouseClick(trigger);
    QTest::qWait(60);
    QCOMPARE(requestSpy.count(), 0);
    QCOMPARE(openChangedSpy.count(), 0);
    QVERIFY(!popover.open());
  }

  void popover_programmaticSetOpenDoesNotEmitOnOpenChange() {
    QWidget host;
    host.resize(420, 280);

    AdPopover popover(&host);
    popover.setGeometry(32, 50, 180, 40);
    popover.setTitleText(QStringLiteral("Title"));
    popover.setContentText(QStringLiteral("Content"));
    popover.setTriggerModes(AdPopover::Trigger::Click);

    auto* trigger = new QPushButton(QStringLiteral("trigger"), &popover);
    popover.setTriggerWidget(trigger);
    popover.show();

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(popover.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(trigger->isVisible(), 400);

    QSignalSpy openChangedSpy(&popover, &AdPopover::openChanged);
    QSignalSpy onOpenChangeSpy(&popover, &AdPopover::onOpenChange);

    popover.setOpen(true);
    QTRY_VERIFY_WITH_TIMEOUT(popover.open(), 400);
    QTRY_COMPARE_WITH_TIMEOUT(openChangedSpy.count(), 1, 400);
    QCOMPARE(openChangedSpy.at(0).at(0).toBool(), true);
    QCOMPARE(onOpenChangeSpy.count(), 0);

    popover.setOpen(false);
    QTRY_VERIFY_WITH_TIMEOUT(!popover.open(), 400);
    QTRY_COMPARE_WITH_TIMEOUT(openChangedSpy.count(), 2, 400);
    QCOMPARE(openChangedSpy.at(1).at(0).toBool(), false);
    QCOMPARE(onOpenChangeSpy.count(), 0);
  }

  void popover_defaultOpenShowsPopup() {
    QWidget host;
    host.resize(420, 280);

    AdPopover popover(&host);
    popover.setGeometry(30, 40, 160, 40);
    popover.setTitleText(QStringLiteral("Title"));
    popover.setContentText(QStringLiteral("Content"));
    popover.setDefaultOpen(true);

    auto* trigger = new QPushButton(QStringLiteral("trigger"), &popover);
    popover.setTriggerWidget(trigger);
    QSignalSpy openChangedSpy(&popover, &AdPopover::openChanged);
    QSignalSpy onOpenChangeSpy(&popover, &AdPopover::onOpenChange);
    popover.show();

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(popover.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(popover.open(), 400);
    QTRY_COMPARE_WITH_TIMEOUT(openChangedSpy.count(), 1, 400);
    QCOMPARE(openChangedSpy.at(0).at(0).toBool(), true);
    QCOMPARE(onOpenChangeSpy.count(), 0);
  }

  void popover_controlledOpenAutoHidesWhenAnchorScrollsOutOfViewport() {
    QScrollArea area;
    area.resize(320, 180);
    area.show();
    QTRY_VERIFY_WITH_TIMEOUT(area.isVisible(), 400);

    auto* content = new QWidget();
    content->resize(280, 900);
    area.setWidget(content);
    area.setWidgetResizable(false);
    QTRY_VERIFY_WITH_TIMEOUT(content->isVisible(), 400);

    AdPopover popover(content);
    popover.setGeometry(24, 24, 180, 40);
    popover.setTitleText(QStringLiteral("scroll anchor"));
    popover.setContentText(QStringLiteral("popover content"));
    popover.setOpenControlled(true);

    auto* trigger = new QPushButton(QStringLiteral("trigger"), &popover);
    popover.setTriggerWidget(trigger);
    popover.show();
    QTRY_VERIFY_WITH_TIMEOUT(trigger->isVisible(), 400);

    popover.setOpen(true);
    QTRY_VERIFY_WITH_TIMEOUT(popover.open(), 400);

    QWidget* popup = area.findChild<QWidget*>(QStringLiteral("adpopover-popup"), Qt::FindChildrenRecursively);
    QTRY_VERIFY_WITH_TIMEOUT(popup != nullptr && popup->isVisible(), 400);

    QScrollBar* verticalBar = area.verticalScrollBar();
    QVERIFY(verticalBar != nullptr);
    verticalBar->setValue(verticalBar->maximum());
    QCoreApplication::processEvents();
    QTRY_VERIFY_WITH_TIMEOUT(popup != nullptr && !popup->isVisible(), 400);
    QVERIFY(popover.open());

    verticalBar->setValue(verticalBar->minimum());
    QCoreApplication::processEvents();
    QTRY_VERIFY_WITH_TIMEOUT(popup != nullptr && popup->isVisible(), 400);
    const QRect areaRect = QRect(area.mapToGlobal(QPoint(0, 0)), area.size());
    const QRect popupRectAfterRestore = QRect(popup->mapToGlobal(QPoint(0, 0)), popup->size());
    QVERIFY(popupRectAfterRestore.intersects(areaRect));
  }

  void popover_openBeforeAttachReparentsPopupToScopeWindow() {
    QWidget host;
    host.resize(480, 320);

    auto* popover = new AdPopover();
    popover->setPlacement(AdPopover::Placement::Top);
    popover->setTriggerModes(AdPopover::Trigger::Click);
    popover->setTitleText(QStringLiteral("Placement"));
    popover->setContentText(QStringLiteral("Use combo box to change placement."));

    auto* trigger = new QPushButton(QStringLiteral("Open popover"), popover);
    popover->setTriggerWidget(trigger);

    // Reproduces docs demo flow: open first, mount into host afterwards.
    popover->setOpen(true);
    QTRY_VERIFY_WITH_TIMEOUT(popover->open(), 400);

    popover->setParent(&host);
    popover->setGeometry(150, 170, 180, 40);
    popover->show();

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(popover->isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(trigger->isVisible(), 400);

    QWidget* popup = host.findChild<QWidget*>(QStringLiteral("adpopover-popup"), Qt::FindChildrenRecursively);
    QTRY_VERIFY_WITH_TIMEOUT(popup != nullptr && popup->isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(popup->parentWidget() == &host, 400);

    const QRect popupRect = QRect(popup->mapToGlobal(QPoint(0, 0)), popup->size());
    const QRect triggerRect = QRect(trigger->mapToGlobal(QPoint(0, 0)), trigger->size());
    QVERIFY(popupRect.bottom() <= triggerRect.top() - 1);
  }

  void popover_emptyContentStaysClosed() {
    QWidget host;
    host.resize(360, 220);

    AdPopover popover(&host);
    popover.setGeometry(40, 40, 160, 40);
    popover.setTriggerModes(AdPopover::Trigger::Click);

    auto* trigger = new QPushButton(QStringLiteral("trigger"), &popover);
    popover.setTriggerWidget(trigger);
    popover.show();

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(popover.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(trigger->isVisible(), 400);

    QSignalSpy openSpy(&popover, &AdPopover::openChanged);
    sendMouseClick(trigger);
    QTest::qWait(80);
    QVERIFY(!popover.open());
    QCOMPARE(openSpy.count(), 0);
  }

  void popover_autoAdjustKeepsPopupInBounds() {
    QWidget host;
    host.resize(420, 260);

    AdPopover popover(&host);
    popover.setGeometry(4, 4, 140, 40);
    popover.setTriggerModes(AdPopover::Trigger::Click);
    popover.setPlacement(AdPopover::Placement::TopLeft);
    popover.setTitleText(QStringLiteral("Title"));
    popover.setContentText(QStringLiteral("A short message to measure popup bounds"));
    popover.setAutoAdjustOverflow(true);

    auto* trigger = new QPushButton(QStringLiteral("trigger"), &popover);
    popover.setTriggerWidget(trigger);
    popover.show();

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(popover.isVisible(), 400);

    popover.setOpen(true);
    QTRY_VERIFY_WITH_TIMEOUT(popover.open(), 400);

    QWidget* popup = host.findChild<QWidget*>(QStringLiteral("adpopover-popup"), Qt::FindChildrenRecursively);
    QTRY_VERIFY_WITH_TIMEOUT(popup != nullptr && popup->isVisible(), 400);

    const QRect hostRect = QRect(host.mapToGlobal(QPoint(0, 0)), host.size());
    const QRect popupRect = QRect(popup->mapToGlobal(QPoint(0, 0)), popup->size());
    const QRect tolerantHostRect = hostRect.adjusted(-2, -2, 2, 2);
    QVERIFY(tolerantHostRect.contains(popupRect.topLeft()));
    QVERIFY(tolerantHostRect.contains(popupRect.topRight()));
    QVERIFY(tolerantHostRect.contains(popupRect.bottomLeft()));
    QVERIFY(tolerantHostRect.contains(popupRect.bottomRight()));
  }

  void popover_edgePlacementDoesNotCrossAxisShift() {
    QWidget host;
    host.resize(320, 240);

    AdPopover popover(&host);
    popover.setGeometry(272, 8, 40, 30);
    popover.setTriggerModes(AdPopover::Trigger::Click);
    popover.setPlacement(AdPopover::Placement::TopLeft);
    popover.setTitleText(QStringLiteral("Title"));
    popover.setContentText(
        QStringLiteral("Edge placement should flip but keep cross-axis alignment stable."));
    popover.setAutoAdjustOverflow(true);

    auto* trigger = new QPushButton(QStringLiteral("trigger"), &popover);
    popover.setTriggerWidget(trigger);
    popover.show();

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(popover.isVisible(), 400);

    popover.setOpen(true);
    QTRY_VERIFY_WITH_TIMEOUT(popover.open(), 400);

    QWidget* popup = host.findChild<QWidget*>(QStringLiteral("adpopover-popup"), Qt::FindChildrenRecursively);
    QTRY_VERIFY_WITH_TIMEOUT(popup != nullptr && popup->isVisible(), 400);

    const QRect hostRect = QRect(host.mapToGlobal(QPoint(0, 0)), host.size());
    const QRect triggerRect = QRect(trigger->mapToGlobal(QPoint(0, 0)), trigger->size());
    const QRect popupRect = QRect(popup->mapToGlobal(QPoint(0, 0)), popup->size());

    QVERIFY(popupRect.top() >= triggerRect.bottom() - 1);
    QVERIFY(qAbs(popupRect.left() - triggerRect.left()) <= 1);
    QVERIFY(popupRect.right() > hostRect.right());
  }

  void popover_autoAdjustDisabledAllowsOverflow() {
    QWidget host;
    host.resize(320, 220);

    AdPopover popover(&host);
    popover.setGeometry(120, 2, 100, 30);
    popover.setPlacement(AdPopover::Placement::Top);
    popover.setTriggerModes(AdPopover::Trigger::Click);
    popover.setTitleText(QStringLiteral("Title"));
    popover.setContentText(QStringLiteral("No auto adjust should keep original placement even if clipped."));
    popover.setAutoAdjustOverflow(false);

    auto* trigger = new QPushButton(QStringLiteral("trigger"), &popover);
    popover.setTriggerWidget(trigger);
    popover.show();

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(popover.isVisible(), 400);

    popover.setOpen(true);
    QTRY_VERIFY_WITH_TIMEOUT(popover.open(), 400);

    QWidget* popup = host.findChild<QWidget*>(QStringLiteral("adpopover-popup"), Qt::FindChildrenRecursively);
    QTRY_VERIFY_WITH_TIMEOUT(popup != nullptr && popup->isVisible(), 400);

    const QRect hostRect = QRect(host.mapToGlobal(QPoint(0, 0)), host.size());
    const QRect popupRect = QRect(popup->mapToGlobal(QPoint(0, 0)), popup->size());
    QVERIFY(popupRect.top() < hostRect.top());
  }

  void popover_outsideClickClosesPopup() {
    QWidget host;
    host.resize(420, 260);

    AdPopover popover(&host);
    popover.setGeometry(24, 30, 180, 40);
    popover.setTitleText(QStringLiteral("Title"));
    popover.setContentText(QStringLiteral("Outside click should close"));
    popover.setTriggerModes(AdPopover::Trigger::Click);
    QVERIFY(popover.triggerModes().testFlag(AdPopover::Trigger::Click));

    auto* trigger = new QPushButton(QStringLiteral("trigger"), &popover);
    popover.setTriggerWidget(trigger);
    popover.show();

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(popover.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(trigger->isVisible(), 400);

    sendMouseClick(trigger);
    QTRY_VERIFY_WITH_TIMEOUT(popover.open(), 400);

    sendMouseClick(&host, Qt::LeftButton, Qt::NoModifier,
                   QPoint(host.width() - 10, host.height() - 10));
    QTRY_VERIFY_WITH_TIMEOUT(!popover.open(), 400);
  }

  void popover_escapeKeyClosesPopup() {
    QWidget host;
    host.resize(420, 280);

    AdPopover popover(&host);
    popover.setGeometry(32, 50, 180, 40);
    popover.setTitleText(QStringLiteral("Title"));
    popover.setContentText(QStringLiteral("Escape should close popover"));
    popover.setTriggerModes(AdPopover::Trigger::Click);
    QVERIFY(popover.triggerModes().testFlag(AdPopover::Trigger::Click));

    auto* trigger = new QPushButton(QStringLiteral("trigger"), &popover);
    popover.setTriggerWidget(trigger);
    popover.show();

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(popover.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(trigger->isVisible(), 400);

    sendMouseClick(trigger);
    QTRY_VERIFY_WITH_TIMEOUT(popover.open(), 400);

    trigger->setFocus();
    QTest::keyClick(trigger, Qt::Key_Escape);
    QTRY_VERIFY_WITH_TIMEOUT(!popover.open(), 400);
  }

  void popover_escapeKeyOnWindowClosesPopup() {
    QWidget host;
    host.resize(420, 280);
    host.setFocusPolicy(Qt::StrongFocus);

    AdPopover popover(&host);
    popover.setGeometry(32, 50, 180, 40);
    popover.setTitleText(QStringLiteral("Title"));
    popover.setContentText(QStringLiteral("Escape on window should close popover"));
    popover.setTriggerModes(AdPopover::Trigger::Click);

    auto* trigger = new QPushButton(QStringLiteral("trigger"), &popover);
    popover.setTriggerWidget(trigger);
    popover.show();

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(popover.isVisible(), 400);

    sendMouseClick(trigger);
    QTRY_VERIFY_WITH_TIMEOUT(popover.open(), 400);

    host.setFocus();
    QTest::keyClick(&host, Qt::Key_Escape);
    QTRY_VERIFY_WITH_TIMEOUT(!popover.open(), 400);
  }

  void colorPicker_basicValueParsingAndFormatSwitch() {
    AdColorPicker picker;
    picker.resize(220, 36);
    picker.show();
    QTRY_VERIFY_WITH_TIMEOUT(picker.isVisible(), 400);

    picker.setValue(QStringLiteral("#ff0000"));
    QCOMPARE(picker.value(), QStringLiteral("#ff0000"));

    picker.setValue(QStringLiteral("rgb(0, 128, 255)"));
    QCOMPARE(picker.value(), QStringLiteral("#0080ff"));

    picker.setValue(QStringLiteral("hsb(120, 100%, 50%)"));
    QCOMPARE(picker.value(), QStringLiteral("#008000"));

    picker.setFormat(AdColorPicker::Format::Rgb);
    QCOMPARE(picker.format(), AdColorPicker::Format::Rgb);
  }

  void colorPicker_openAndClearSignals() {
    AdColorPicker picker;
    picker.resize(260, 38);
    picker.setAllowClear(true);
    picker.setValue(QStringLiteral("#1677ff"));
    picker.show();
    QTRY_VERIFY_WITH_TIMEOUT(picker.isVisible(), 400);

    QSignalSpy openSpy(&picker, &AdColorPicker::openChanged);
    QSignalSpy onClearSpy(&picker, &AdColorPicker::onClear);
    QSignalSpy clearedSpy(&picker, &AdColorPicker::cleared);
    QSignalSpy valueSpy(&picker, &AdColorPicker::valueChanged);

    picker.setOpen(true);
    QTRY_VERIFY_WITH_TIMEOUT(picker.open(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(openSpy.count() >= 1, 400);

    QPushButton* clearButton = nullptr;
    const QList<QPushButton*> buttons = picker.findChildren<QPushButton*>();
    for (QPushButton* candidate : buttons) {
      if (candidate && candidate->text() == QStringLiteral("Clear")) {
        clearButton = candidate;
        break;
      }
    }
    QVERIFY(clearButton != nullptr);

    sendMouseClick(clearButton);
    QTRY_COMPARE_WITH_TIMEOUT(onClearSpy.count(), 1, 400);
    QTRY_COMPARE_WITH_TIMEOUT(clearedSpy.count(), 1, 400);
    QCOMPARE(picker.value(), QString());
    QVERIFY(valueSpy.count() >= 1);
    QCOMPARE(valueSpy.last().at(0).toString(), QString());
  }

  void colorPicker_gradientModeAndPresets() {
    AdColorPicker picker;
    picker.resize(260, 38);
    picker.setModeOptions({AdColorPicker::Mode::Single, AdColorPicker::Mode::Gradient});
    picker.setMode(AdColorPicker::Mode::Gradient);
    picker.show();
    QTRY_VERIFY_WITH_TIMEOUT(picker.isVisible(), 400);

    AdColorPicker::ColorValue gradientValue;
    gradientValue.gradientStops = {
        AdColorPicker::GradientStop{QStringLiteral("rgb(16, 142, 233)"), 0},
        AdColorPicker::GradientStop{QStringLiteral("rgb(135, 208, 104)"), 100},
    };
    picker.setColorValue(gradientValue);
    QVERIFY(picker.value().startsWith(QStringLiteral("linear-gradient(")));

    picker.setMode(AdColorPicker::Mode::Single);

    AdColorPicker::PresetItem preset;
    preset.label = QStringLiteral("test");
    preset.colors = {
        AdColorPicker::ColorValue{false, QStringLiteral("#f5222d"), {}},
        AdColorPicker::ColorValue{false, QStringLiteral("#52c41a"), {}},
    };
    picker.setPresets({preset});
    picker.setOpen(true);
    QTRY_VERIFY_WITH_TIMEOUT(picker.open(), 400);

    QPushButton* swatch = picker.findChild<QPushButton*>(QStringLiteral("ad-color-picker-preset-swatch"),
                                                         Qt::FindChildrenRecursively);
    QVERIFY(swatch != nullptr);
    sendMouseClick(swatch);
    QTRY_COMPARE_WITH_TIMEOUT(picker.value(), QStringLiteral("#f5222d"), 400);
  }

  void colorPicker_customTriggerPlacementAndModeOptions() {
    AdColorPicker picker;
    picker.resize(260, 38);
    picker.show();
    QTRY_VERIFY_WITH_TIMEOUT(picker.isVisible(), 400);

    auto* customTrigger = new QPushButton(QStringLiteral("open"));
    picker.setTriggerWidget(customTrigger);
    QCOMPARE(picker.triggerWidget(), customTrigger);

    picker.setTrigger(AdColorPicker::Trigger::Hover);
    QCOMPARE(picker.trigger(), AdColorPicker::Trigger::Hover);

    picker.setPlacement(AdColorPicker::Placement::TopRight);
    QCOMPARE(picker.placement(), AdColorPicker::Placement::TopRight);

    picker.setModeOptions({AdColorPicker::Mode::Gradient});
    QCOMPARE(picker.modeOptions(), QVector<AdColorPicker::Mode>({AdColorPicker::Mode::Gradient}));
    QCOMPARE(picker.mode(), AdColorPicker::Mode::Gradient);

    picker.setTriggerWidget(nullptr);
    QVERIFY(picker.triggerWidget() == nullptr);
  }

  void input_clearAndFocusBehaviors() {
    AdInput input;
    input.resize(260, 40);
    input.setAllowClear(true);
    input.setValue(QStringLiteral("hello"));
    input.show();
    QTRY_VERIFY_WITH_TIMEOUT(input.isVisible(), 400);

    input.focusInput(AdInput::FocusCursor::End, false);
    QTRY_VERIFY_WITH_TIMEOUT(input.lineEdit() && input.lineEdit()->hasFocus(), 400);
    QVERIFY(input.lineEdit()->cursorPosition() >= input.value().size());

    QSignalSpy clearedSpy(&input, &AdInput::cleared);
    const QList<QToolButton*> buttons = input.findChildren<QToolButton*>();
    QVERIFY(!buttons.isEmpty());
    QToolButton* clear = nullptr;
    for (QToolButton* candidate : buttons) {
      if (candidate && candidate->isVisible()) {
        clear = candidate;
        break;
      }
    }
    QVERIFY(clear != nullptr);

    sendMouseClick(clear);
    QTRY_COMPARE_WITH_TIMEOUT(clearedSpy.count(), 1, 400);
    QCOMPARE(input.value(), QString());
  }

  void inputSearch_sourceSignals() {
    AdInputSearch search;
    search.resize(360, 40);
    search.setAllowClear(true);
    search.show();
    QTRY_VERIFY_WITH_TIMEOUT(search.isVisible(), 400);

    QSignalSpy searchSpy(&search, &AdInputSearch::searchTriggered);

    search.setValue(QStringLiteral("abc"));
    QVERIFY(search.input() != nullptr);
    search.input()->focusInput();
    QTest::keyClick(search.input()->lineEdit(), Qt::Key_Return);
    QTRY_VERIFY_WITH_TIMEOUT(searchSpy.count() >= 1, 400);
    QCOMPARE(searchSpy.last().at(0).toString(), QStringLiteral("abc"));
    QCOMPARE(searchSpy.last().at(1).value<AdInputSearch::SearchSource>(),
             AdInputSearch::SearchSource::Input);

    const QList<QToolButton*> clearButtons = search.findChildren<QToolButton*>();
    QToolButton* clear = nullptr;
    for (QToolButton* candidate : clearButtons) {
      if (candidate && candidate->isVisible()) {
        clear = candidate;
        break;
      }
    }
    QVERIFY(clear != nullptr);

    sendMouseClick(clear);
    QTRY_VERIFY_WITH_TIMEOUT(searchSpy.count() >= 2, 400);
    QCOMPARE(searchSpy.last().at(1).value<AdInputSearch::SearchSource>(),
             AdInputSearch::SearchSource::Clear);
  }

  void input_compactJoin_noBackgroundSeamBetweenInputAndButton() {
    const int overlap = std::max(1, qRound(ThemeManager::instance().currentMapToken().lineWidth));
    const QColor hostBg(QStringLiteral("#ff00ff"));

    struct SeamStats {
      int bgLeakPixels = 0;
      int brightEdgeRows = 0;
      int joinDelta = 0;
    };

    auto measure = [&](bool applyOverlap) -> SeamStats {
      SeamStats stats;

      QWidget host;
      host.resize(520, 120);
      host.setAutoFillBackground(true);
      QPalette hostPalette = host.palette();
      hostPalette.setColor(QPalette::Window, hostBg);
      host.setPalette(hostPalette);

      auto* row = new QWidget(&host);
      row->setGeometry(24, 36, 460, 32);
      auto* rowLayout = new QHBoxLayout(row);
      rowLayout->setContentsMargins(0, 0, 0, 0);
      rowLayout->setSpacing(0);

      auto* input = new AdInput(row);
      input->setValue(QStringLiteral("Combine input and button"));
      input->setJoinedRight(true);
      input->setFixedWidth(340);

      auto* button = new AdButton(QStringLiteral("Submit"), row);
      button->setType(AdButton::Type::Primary);
      button->setJoinedLeft(true);

      rowLayout->addWidget(input);
      if (applyOverlap) {
        rowLayout->addSpacing(-overlap);
      }
      rowLayout->addWidget(button);
      rowLayout->addStretch();

      host.show();
      QCoreApplication::processEvents();
      QTest::qWait(20);
      if (!host.isVisible() || !row->isVisible() || !input->isVisible() || !button->isVisible()) {
        stats.bgLeakPixels = -1;
        stats.brightEdgeRows = -1;
        stats.joinDelta = 0;
        return stats;
      }
      button->setFocus(Qt::OtherFocusReason);
      QCoreApplication::processEvents();

      stats.joinDelta = button->x() - (input->x() + input->width());

      const QImage image = host.grab().toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
      if (image.isNull()) {
        stats.bgLeakPixels = -1;
        stats.brightEdgeRows = -1;
        return stats;
      }
      const qreal dpr = std::max<qreal>(1.0, image.devicePixelRatio());
      const int seamLogicalX = row->x() + button->x();
      const int scanTop = row->y();
      const int scanBottom = row->y() + row->height() - 1;

      auto colorDistance = [](const QColor& lhs, const QColor& rhs) -> int {
        return std::abs(lhs.red() - rhs.red()) + std::abs(lhs.green() - rhs.green()) +
               std::abs(lhs.blue() - rhs.blue()) + std::abs(lhs.alpha() - rhs.alpha());
      };
      auto pixelAtLogical = [&image, dpr](int logicalX, int logicalY) -> QColor {
        const int px = std::clamp(qFloor((static_cast<qreal>(logicalX) + 0.5) * dpr), 0, image.width() - 1);
        const int py = std::clamp(qFloor((static_cast<qreal>(logicalY) + 0.5) * dpr), 0, image.height() - 1);
        return QColor::fromRgba(image.pixel(px, py));
      };

      for (int y = scanTop; y <= scanBottom; ++y) {
        for (int x = seamLogicalX - 1; x <= seamLogicalX + 1; ++x) {
          const QColor px = pixelAtLogical(x, y);
          if (colorDistance(px, hostBg) <= 30) {
            ++stats.bgLeakPixels;
          }
        }
      }

      const int coreTop = row->y() + 4;
      const int coreBottom = row->y() + row->height() - 5;
      const int buttonEdgeX = seamLogicalX;
      const int buttonInnerX = seamLogicalX + 6;
      for (int y = coreTop; y <= coreBottom; ++y) {
        const QColor edge = pixelAtLogical(buttonEdgeX, y);
        const QColor inner = pixelAtLogical(buttonInnerX, y);
        if (colorDistance(edge, inner) > 85) {
          ++stats.brightEdgeRows;
        }
      }

      return stats;
    };

    const SeamStats compact = measure(true);

    QVERIFY2(compact.brightEdgeRows >= 0, QStringLiteral("failed to capture compact join seam stats").toUtf8().constData());
    QVERIFY2(compact.joinDelta == -overlap,
             qPrintable(QStringLiteral("compact join delta=%1 overlap=%2")
                            .arg(compact.joinDelta)
                            .arg(overlap)));
    QVERIFY2(compact.bgLeakPixels == 0,
             qPrintable(QStringLiteral("compact join leaked host background pixels=%1")
                            .arg(compact.bgLeakPixels)));
    QVERIFY2(compact.brightEdgeRows <= 1,
             qPrintable(QStringLiteral("compact join seam rows=%1")
                            .arg(compact.brightEdgeRows)));
  }

  void input_compactJoin_inputInputRetainsDividerBorder() {
    QWidget host;
    host.resize(560, 120);

    auto* row = new QWidget(&host);
    row->setGeometry(24, 36, 500, 32);
    auto* rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(0);

    auto* left = new AdInput(row);
    left->setValue(QStringLiteral("0571"));
    left->setJoinedRight(true);
    left->setFixedWidth(120);

    auto* right = new AdInput(row);
    right->setValue(QStringLiteral("26888888"));
    right->setJoinedLeft(true);
    right->setFixedWidth(300);

    rowLayout->addWidget(left);
    rowLayout->addWidget(right);
    rowLayout->addStretch();

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(left->isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(right->isVisible(), 400);
    QCoreApplication::processEvents();

    const int joinDelta = right->x() - (left->x() + left->width());
    QCOMPARE(joinDelta, 0);

    const QImage leftImage = left->grab().toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
    const QImage rightImage = right->grab().toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
    QVERIFY(!leftImage.isNull());
    QVERIFY(!rightImage.isNull());

    const qreal leftDpr = std::max<qreal>(1.0, leftImage.devicePixelRatio());
    const qreal rightDpr = std::max<qreal>(1.0, rightImage.devicePixelRatio());
    const int leftY = std::clamp(qFloor((left->height() / 2.0 + 0.5) * leftDpr), 0, leftImage.height() - 1);
    const int rightY =
        std::clamp(qFloor((right->height() / 2.0 + 0.5) * rightDpr), 0, rightImage.height() - 1);

    const int leftEdgeX = std::max(0, leftImage.width() - 1);
    const int leftInnerX = std::max(0, leftImage.width() - std::max(2, qRound(6.0 * leftDpr)));
    const int rightEdgeX = 0;
    const int rightInnerX = std::clamp(qRound(6.0 * rightDpr), 0, rightImage.width() - 1);

    const QColor leftEdge = QColor::fromRgba(leftImage.pixel(leftEdgeX, leftY));
    const QColor leftInner = QColor::fromRgba(leftImage.pixel(leftInnerX, leftY));
    const QColor rightEdge = QColor::fromRgba(rightImage.pixel(rightEdgeX, rightY));
    const QColor rightInner = QColor::fromRgba(rightImage.pixel(rightInnerX, rightY));

    auto colorDistance = [](const QColor& lhs, const QColor& rhs) -> int {
      return std::abs(lhs.red() - rhs.red()) + std::abs(lhs.green() - rhs.green()) +
             std::abs(lhs.blue() - rhs.blue()) + std::abs(lhs.alpha() - rhs.alpha());
    };

    const int leftEdgeDelta = colorDistance(leftEdge, leftInner);
    const int rightEdgeDelta = colorDistance(rightEdge, rightInner);
    QVERIFY2(leftEdgeDelta > 10,
             qPrintable(QStringLiteral("left divider border missing, delta=%1").arg(leftEdgeDelta)));
    QVERIFY2(rightEdgeDelta > 10,
             qPrintable(QStringLiteral("right divider border missing, delta=%1").arg(rightEdgeDelta)));
  }

  void inputPassword_toggleVisibility() {
    AdInputPassword password;
    password.resize(320, 40);
    password.setValue(QStringLiteral("secret"));
    password.show();
    QTRY_VERIFY_WITH_TIMEOUT(password.isVisible(), 400);

    QVERIFY(password.input() != nullptr);
    QCOMPARE(password.input()->echoMode(), QLineEdit::Password);

    QSignalSpy visibleSpy(&password, &AdInputPassword::passwordVisibleChanged);
    const QList<QToolButton*> buttons = password.findChildren<QToolButton*>();
    QVERIFY(!buttons.isEmpty());
    QToolButton* toggle = nullptr;
    for (QToolButton* button : buttons) {
      if (button && button->isVisible()) {
        toggle = button;
        break;
      }
    }
    QVERIFY(toggle != nullptr);
    sendMouseClick(toggle);
    QTRY_COMPARE_WITH_TIMEOUT(visibleSpy.count(), 1, 400);
    QCOMPARE(password.input()->echoMode(), QLineEdit::Normal);
  }

  void inputTextArea_autoSizeBounds() {
    AdInputTextArea textArea;
    textArea.resize(360, 120);
    textArea.setAutoSizeEnabled(true);
    textArea.setAutoSizeMinRows(2);
    textArea.setAutoSizeMaxRows(4);
    textArea.show();
    QTRY_VERIFY_WITH_TIMEOUT(textArea.isVisible(), 400);

    QVERIFY(textArea.textEdit() != nullptr);
    const int before = textArea.textEdit()->height();
    textArea.setValue(QStringLiteral("line1\nline2\nline3\nline4\nline5\nline6"));
    QCoreApplication::processEvents();
    const int after = textArea.textEdit()->height();
    QVERIFY(after >= before);
    QVERIFY(after <= textArea.maximumHeight() || textArea.maximumHeight() == QWIDGETSIZE_MAX);
  }

  void inputTextArea_maxLengthCountFollowsShowCount() {
    AdInputTextArea textArea;
    textArea.resize(360, 120);
    textArea.setMaxLength(6);
    textArea.show();
    QTRY_VERIFY_WITH_TIMEOUT(textArea.isVisible(), 400);

    const QList<QLabel*> labels = textArea.findChildren<QLabel*>();
    bool hasVisibleCount = false;
    for (QLabel* label : labels) {
      if (label && label->isVisible() && label->text().contains(QStringLiteral("/"))) {
        hasVisibleCount = true;
        break;
      }
    }
    QVERIFY(!hasVisibleCount);

    textArea.setShowCount(true);
    QCoreApplication::processEvents();

    bool hasExpectedCount = false;
    for (QLabel* label : labels) {
      if (label && label->isVisible() && label->text() == QStringLiteral("0 / 6")) {
        hasExpectedCount = true;
        break;
      }
    }
    QVERIFY(hasExpectedCount);
  }

  void inputTextArea_shellDoesNotStretchToWidgetHeight() {
    AdInputTextArea textArea;
    textArea.resize(360, 320);
    textArea.setAutoSizeEnabled(true);
    textArea.setAutoSizeMinRows(4);
    textArea.setAutoSizeMaxRows(4);
    textArea.show();
    QTRY_VERIFY_WITH_TIMEOUT(textArea.isVisible(), 400);

    QVERIFY(textArea.textEdit() != nullptr);
    QWidget* shell = textArea.textEdit()->parentWidget();
    QVERIFY(shell != nullptr);
    QVERIFY(shell->height() < textArea.height());
    QVERIFY(shell->height() <= textArea.textEdit()->height() + 8);
  }

  void inputTextArea_shellHeightFitsWidgetWhenUsingSizeHint() {
    QWidget host;
    host.resize(440, 280);
    auto* layout = new QVBoxLayout(&host);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto* textArea = new AdInputTextArea(&host);
    textArea->setAutoSizeEnabled(true);
    textArea->setAutoSizeMinRows(4);
    textArea->setAutoSizeMaxRows(4);
    textArea->setFixedWidth(360);
    layout->addWidget(textArea, 0, Qt::AlignLeft);

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(textArea->isVisible(), 400);

    QVERIFY(textArea->textEdit() != nullptr);
    QWidget* shell = textArea->textEdit()->parentWidget();
    QVERIFY(shell != nullptr);

    QTRY_VERIFY_WITH_TIMEOUT(shell->height() > 0, 400);
    QVERIFY2(shell->height() <= textArea->height(),
             qPrintable(QStringLiteral("shell is taller than textarea widget: shell=%1 textarea=%2 hint=%3 minHint=%4 shellMin=%5 shellMax=%6")
                            .arg(shell->height())
                            .arg(textArea->height())
                            .arg(textArea->sizeHint().height())
                            .arg(textArea->minimumSizeHint().height())
                            .arg(shell->minimumHeight())
                            .arg(shell->maximumHeight())));
  }

  void inputOtp_completionAndPasteDistribution() {
    AdInputOtp otp;
    otp.setLength(4);
    otp.show();
    QTRY_VERIFY_WITH_TIMEOUT(otp.isVisible(), 400);

    QSignalSpy completeSpy(&otp, &AdInputOtp::completed);
    otp.setValue(QStringLiteral("12"));
    QCOMPARE(completeSpy.count(), 0);

    const QList<QLineEdit*> cells = otp.findChildren<QLineEdit*>();
    QVERIFY(cells.size() >= 4);
    cells.first()->setFocus();
    QTest::keyClicks(cells.first(), QStringLiteral("9876"));

    QTRY_VERIFY_WITH_TIMEOUT(completeSpy.count() >= 1, 400);
    QCOMPARE(completeSpy.last().at(0).toString(), QStringLiteral("9876"));
    QCOMPARE(otp.value(), QStringLiteral("9876"));
  }

 private:
  ThemeConfig originalConfig_;
};

}  // namespace

QTEST_MAIN(TimingRefactorTests)
#include "timing_refactor_tests.moc"
