#include <QtTest/QtTest>

#include <QBuffer>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QIcon>
#include <QImage>
#include <QInputMethodEvent>
#include <QLineEdit>
#include <QListView>
#include <QPushButton>
#include <QToolButton>
#include <QWidget>

#include <algorithm>
#include <numeric>

#include "theme/theme_manager.h"
#include "widgets/button.h"
#include "widgets/detail/timing_hub.h"
#include "widgets/in_window_popup_host.h"
#include "widgets/interaction_overlay_manager.h"
#include "widgets/menu.h"
#include "widgets/select.h"

namespace {

using adqt::theme::ThemeConfig;
using adqt::theme::ThemeManager;
using adqt::widgets::AdButton;
using adqt::widgets::AdMenu;
using adqt::widgets::AdSelect;
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
  explicit FakePopupOwner(QWidget* scopeWindow) : scopeWindow_(scopeWindow) {
    anchorWidget_ = new QWidget(scopeWindow_);
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

 private:
  ThemeConfig originalConfig_;
};

}  // namespace

QTEST_MAIN(TimingRefactorTests)
#include "timing_refactor_tests.moc"
