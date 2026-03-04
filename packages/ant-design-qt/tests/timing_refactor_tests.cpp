#include <QtTest/QtTest>

#include <QBuffer>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QCursor>
#include <QElapsedTimer>
#include <QFontMetrics>
#include <QIcon>
#include <QImage>
#include <QInputMethodEvent>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QListView>
#include <QMouseEvent>
#include <QPushButton>
#include <QSignalSpy>
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
#include "widgets/popover.h"
#include "widgets/radio.h"
#include "widgets/radio_group.h"
#include "widgets/radio_style.h"
#define private public
#include "widgets/select.h"
#undef private
#include "widgets/select_style.h"
#include "widgets/slider.h"
#include "widgets/tooltip.h"

namespace {

using adqt::theme::ThemeConfig;
using adqt::theme::ThemeManager;
using adqt::widgets::AdButton;
using adqt::widgets::AdMenu;
using adqt::widgets::AdPopover;
using adqt::widgets::AdRadio;
using adqt::widgets::AdRadioGroup;
using adqt::widgets::AdSelect;
using adqt::widgets::AdSlider;
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
    for (int i = 1; i < radios.size(); ++i) {
      const QRect previous = radios.at(i - 1)->geometry();
      const QRect current = radios.at(i)->geometry();
      const int overlapPixels = previous.right() - current.left() + 1;
      QCOMPARE(overlapPixels, lineWidth);
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
    const int seamX = second->geometry().left() + (lineWidth - 1) / 2;
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

 private:
  ThemeConfig originalConfig_;
};

}  // namespace

QTEST_MAIN(TimingRefactorTests)
#include "timing_refactor_tests.moc"
