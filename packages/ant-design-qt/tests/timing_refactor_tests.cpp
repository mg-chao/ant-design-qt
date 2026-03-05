#include <QtTest/QtTest>

#include <QBuffer>
#include <QAbstractButton>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QCursor>
#include <QElapsedTimer>
#include <QFrame>
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
#include <QRegularExpression>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalSpy>
#include <QStackedWidget>
#include <QToolButton>
#include <QTextEdit>
#include <QWidget>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <numeric>

#include "icons.h"
#include "theme/fast_color_lite.h"
#include "theme/theme_manager.h"
#include "widgets/alert.h"
#include "widgets/button.h"
#include "widgets/color_picker.h"
#include "widgets/detail/timing_hub.h"
#include "widgets/input.h"
#include "widgets/input_number.h"
#include "widgets/input_style.h"
#include "widgets/in_window_popup_host.h"
#include "widgets/interaction_overlay_manager.h"
#include "widgets/image_style.h"
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
using adqt::widgets::AdAlert;
using adqt::widgets::AdInput;
using adqt::widgets::AdInputNumber;
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

bool labelHasPixmap(const QLabel* label) {
  if (!label) {
    return false;
  }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  return !label->pixmap(Qt::ReturnByValue).isNull();
#else
  const QPixmap* pixmap = label->pixmap();
  return pixmap && !pixmap->isNull();
#endif
}

QString colorToString(const QColor& color) {
  return QStringLiteral("rgba(%1, %2, %3, %4)")
      .arg(color.red())
      .arg(color.green())
      .arg(color.blue())
      .arg(color.alpha());
}

bool colorClose(const QColor& actual, const QColor& expected, int tolerance) {
  return std::abs(actual.red() - expected.red()) <= tolerance &&
         std::abs(actual.green() - expected.green()) <= tolerance &&
         std::abs(actual.blue() - expected.blue()) <= tolerance &&
         std::abs(actual.alpha() - expected.alpha()) <= tolerance;
}

struct RadiusValues {
  int borderRadius = 0;
  int borderRadiusXS = 0;
  int borderRadiusSM = 0;
  int borderRadiusLG = 0;
};

RadiusValues deriveRadiusValuesForBase(int radiusBase) {
  const double base = std::max(0.0, static_cast<double>(radiusBase));
  double radiusLG = base;
  double radiusSM = base;
  double radiusXS = base;

  if (base < 6.0 && base >= 5.0) {
    radiusLG = base + 1.0;
  } else if (base < 16.0 && base >= 6.0) {
    radiusLG = base + 2.0;
  } else if (base >= 16.0) {
    radiusLG = 16.0;
  }

  if (base < 7.0 && base >= 5.0) {
    radiusSM = 4.0;
  } else if (base < 8.0 && base >= 7.0) {
    radiusSM = 5.0;
  } else if (base < 14.0 && base >= 8.0) {
    radiusSM = 6.0;
  } else if (base < 16.0 && base >= 14.0) {
    radiusSM = 7.0;
  } else if (base >= 16.0) {
    radiusSM = 8.0;
  }

  if (base < 6.0 && base >= 2.0) {
    radiusXS = 1.0;
  } else if (base >= 6.0) {
    radiusXS = 2.0;
  }

  RadiusValues values;
  values.borderRadius = qRound(base);
  values.borderRadiusXS = qRound(radiusXS);
  values.borderRadiusSM = qRound(radiusSM);
  values.borderRadiusLG = qRound(radiusLG);
  return values;
}

int extractBorderRadiusPx(const QString& styleSheet) {
  const QRegularExpression re(QStringLiteral("border-radius\\s*:\\s*([0-9]+)px"),
                              QRegularExpression::CaseInsensitiveOption);
  const QRegularExpressionMatch match = re.match(styleSheet);
  if (!match.hasMatch()) {
    return -1;
  }
  bool ok = false;
  const int value = match.captured(1).toInt(&ok);
  return ok ? value : -1;
}

QColor sampleWidgetPixel(QWidget* widget, const QPoint& logicalPoint) {
  if (!widget || widget->width() <= 0 || widget->height() <= 0) {
    return QColor();
  }

  const QImage image = widget->grab().toImage().convertToFormat(QImage::Format_ARGB32);
  if (image.isNull()) {
    return QColor();
  }

  const double scaleX = static_cast<double>(image.width()) / static_cast<double>(widget->width());
  const double scaleY = static_cast<double>(image.height()) / static_cast<double>(widget->height());
  const int x = std::clamp(static_cast<int>(std::round(logicalPoint.x() * scaleX)), 0, image.width() - 1);
  const int y =
      std::clamp(static_cast<int>(std::round(logicalPoint.y() * scaleY)), 0, image.height() - 1);
  return image.pixelColor(x, y);
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

void sendMouseMove(QWidget* widget, const QPoint& localPos = QPoint()) {
  if (!widget) {
    return;
  }
  const QPoint pos = localPos.isNull() ? widget->rect().center() : localPos;
  const QPointF localPoint(pos);
  const QPointF scenePoint(widget->mapTo(widget->window(), pos));
  const QPointF globalPoint(widget->mapToGlobal(pos));
  QMouseEvent move(QEvent::MouseMove, localPoint, scenePoint, globalPoint, Qt::NoButton,
                   Qt::NoButton, Qt::NoModifier);
  QCoreApplication::sendEvent(widget, &move);
  QCoreApplication::processEvents();
}

void sendMouseMoveWithButtons(QWidget* widget,
                              const QPoint& localPos,
                              Qt::MouseButtons buttons,
                              Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
  if (!widget) {
    return;
  }
  const QPoint pos = localPos.isNull() ? widget->rect().center() : localPos;
  const QPointF localPoint(pos);
  const QPointF scenePoint(widget->mapTo(widget->window(), pos));
  const QPointF globalPoint(widget->mapToGlobal(pos));
  QMouseEvent move(QEvent::MouseMove, localPoint, scenePoint, globalPoint, Qt::NoButton,
                   buttons, modifiers);
  QCoreApplication::sendEvent(widget, &move);
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
    if (child && child->property("adqt.interaction.overlay").toBool()) {
      return child;
    }
  }
  return nullptr;
}

QWidget* findInteractionOverlayWidgetInParent(QWidget* parent) {
  if (!parent) {
    return nullptr;
  }
  const QList<QWidget*> children =
      parent->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
  for (QWidget* child : children) {
    if (child && child->property("adqt.interaction.overlay").toBool()) {
      return child;
    }
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

  void image_defaultVisualStyleHasNoRoundedCornersByDefault() {
    adqt::widgets::detail::ImageStyleInput input;
    const auto style = adqt::widgets::detail::resolveImageVisualStyle(input);
    QCOMPARE(style.metrics.borderRadius, 0);

    input.componentTokens.borderRadius = 6;
    const auto overridden = adqt::widgets::detail::resolveImageVisualStyle(input);
    QCOMPARE(overridden.metrics.borderRadius, 6);
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

  void menu_hoverOpenDelayNotRestartedByIntraItemMouseMoves() {
    applyTimingConfig(buildTimingConfig(12, 1000, 560, 80, 90, 0));

    AdMenu menu;
    menu.setMode(AdMenu::Mode::Vertical);

    AdMenu::Item child;
    child.key = QStringLiteral("child-1");
    child.label = QStringLiteral("Child Item");
    child.type = AdMenu::ItemType::Item;

    AdMenu::Item rootSubMenu;
    rootSubMenu.key = QStringLiteral("root-submenu");
    rootSubMenu.label = QStringLiteral("Root Submenu");
    rootSubMenu.type = AdMenu::ItemType::SubMenu;
    rootSubMenu.children = {child};

    menu.setItems({rootSubMenu});
    menu.resize(220, 48);
    menu.show();
    QTRY_VERIFY_WITH_TIMEOUT(menu.isVisible(), 400);

    const QPoint baseHoverPos = menu.rect().center();
    sendMouseMove(&menu, baseHoverPos);

    // Keep pointer inside the same row while dispatching multiple move events.
    // Hover-open delay should run from the initial row enter, not be restarted.
    for (int i = 0; i < 6; ++i) {
      QTest::qWait(20);
      const int xOffset = (i % 2 == 0) ? -1 : 1;
      sendMouseMove(&menu, baseHoverPos + QPoint(xOffset, 0));
    }
    QTest::qWait(10);

    const bool openedDuringHoverBurst = menu.openKeys().contains(QStringLiteral("root-submenu"));
    QTRY_VERIFY_WITH_TIMEOUT(menu.openKeys().contains(QStringLiteral("root-submenu")), 300);

    const QStringList openKeys = menu.openKeys();
    QVERIFY2(openedDuringHoverBurst,
             qPrintable(QStringLiteral("submenu opened too late after continuous same-row hover, openKeys=%1")
                            .arg(openKeys.join(QStringLiteral(",")))));
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

    sendMouseMove(&menu, menu.rect().center());

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

    sendMouseMove(&menu, QPoint(4, 4));
    sendMouseMove(&menu, menu.rect().center());

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

  void menu_topNavigationPopupHoverLatencyNotMuchSlowerThanInline() {
    applyTimingConfig(buildTimingConfig(12, 1000, 560, 0, 90, 0));

    auto makeLeaf = [](const QString& key, const QString& label) {
      AdMenu::Item item;
      item.key = key;
      item.label = label;
      item.type = AdMenu::ItemType::Item;
      return item;
    };

    QVector<AdMenu::Item> children;
    children.reserve(14);
    for (int i = 0; i < 14; ++i) {
      children.push_back(makeLeaf(QStringLiteral("child-%1").arg(i),
                                  QStringLiteral("Option %1").arg(i + 1)));
    }

    AdMenu::Item subMenu;
    subMenu.key = QStringLiteral("sub-root");
    subMenu.label = QStringLiteral("Navigation Three - Submenu");
    subMenu.type = AdMenu::ItemType::SubMenu;
    subMenu.children = children;

    QWidget host;
    host.resize(860, 760);
    auto* layout = new QVBoxLayout(&host);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(16);

    auto* top = new AdMenu(&host);
    top->setMode(AdMenu::Mode::Horizontal);
    top->setItems({
        makeLeaf(QStringLiteral("nav-1"), QStringLiteral("Navigation One")),
        makeLeaf(QStringLiteral("nav-2"), QStringLiteral("Navigation Two")),
        subMenu,
        makeLeaf(QStringLiteral("nav-4"), QStringLiteral("Navigation Four")),
    });
    top->setMinimumWidth(780);
    layout->addWidget(top);

    auto* inlineMenu = new AdMenu(&host);
    inlineMenu->setMode(AdMenu::Mode::Inline);
    inlineMenu->setFixedWidth(280);
    inlineMenu->setItems({subMenu});
    layout->addWidget(inlineMenu);

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 500);
    QTRY_VERIFY_WITH_TIMEOUT(top->isVisible(), 500);
    QTRY_VERIFY_WITH_TIMEOUT(inlineMenu->isVisible(), 500);
    top->setOpenKeys({QStringLiteral("sub-root")});
    inlineMenu->setOpenKeys({QStringLiteral("sub-root")});
    QCoreApplication::processEvents();

    QWidget* popupWindow = nullptr;
    QElapsedTimer findPopupTimer;
    findPopupTimer.start();
    while (!popupWindow && findPopupTimer.elapsed() < 1000) {
      popupWindow = host.findChild<QWidget*>(QStringLiteral("admenu-shared-popup-window"),
                                             Qt::FindChildrenRecursively);
      if (!popupWindow || !popupWindow->isVisible()) {
        popupWindow = nullptr;
        QTest::qWait(10);
      }
    }
    QVERIFY2(popupWindow != nullptr, "failed to find visible shared popup window for top navigation");

    AdMenu* popupMenu = popupWindow->findChild<AdMenu*>(QString(), Qt::FindChildrenRecursively);
    QVERIFY2(popupMenu != nullptr, "failed to find popup menu instance");
    QTRY_VERIFY_WITH_TIMEOUT(popupMenu->isVisible(), 400);

    const auto buildSweepPoints = [](QWidget* widget,
                                     int pointCount,
                                     int xInset,
                                     int topInset,
                                     int bottomInset) {
      QVector<QPoint> points;
      if (!widget || pointCount <= 0) {
        return points;
      }

      const QRect r = widget->rect();
      const int left = std::clamp(xInset, 0, std::max(0, r.width() - 1));
      const int right = std::clamp(r.width() - xInset - 1, 0, std::max(0, r.width() - 1));
      const int x = std::clamp((left + right) / 2, 0, std::max(0, r.width() - 1));

      const int top = std::clamp(topInset, 0, std::max(0, r.height() - 1));
      const int bottom = std::clamp(r.height() - bottomInset - 1, 0, std::max(0, r.height() - 1));
      const int span = std::max(1, bottom - top + 1);

      points.reserve(pointCount);
      for (int i = 0; i < pointCount; ++i) {
        const double t = pointCount == 1 ? 0.5 : static_cast<double>(i) / static_cast<double>(pointCount - 1);
        const int y = top + static_cast<int>(std::round(t * static_cast<double>(span - 1)));
        points.push_back(QPoint(x, y));
      }
      return points;
    };

    const auto collectMoveDurationsUs = [](QWidget* widget,
                                           const QVector<QPoint>& points,
                                           int rounds) {
      QVector<qint64> samplesUs;
      if (!widget || points.isEmpty() || rounds <= 0) {
        return samplesUs;
      }
      samplesUs.reserve(rounds * points.size());

      for (int r = 0; r < rounds; ++r) {
        for (const QPoint& p : points) {
          QElapsedTimer timer;
          timer.start();
          QTest::mouseMove(widget, p);
          QCoreApplication::processEvents();
          samplesUs.push_back(timer.nsecsElapsed() / 1000);
        }
      }
      return samplesUs;
    };

    const auto averageUs = [](const QVector<qint64>& samples) -> double {
      if (samples.isEmpty()) {
        return 0.0;
      }
      const qint64 sum = std::accumulate(samples.cbegin(), samples.cend(), static_cast<qint64>(0));
      return static_cast<double>(sum) / static_cast<double>(samples.size());
    };

    const auto p95Us = [](QVector<qint64> samples) -> qint64 {
      if (samples.isEmpty()) {
        return 0;
      }
      std::sort(samples.begin(), samples.end());
      const int lastIndex = static_cast<int>(samples.size()) - 1;
      const int index = std::clamp(static_cast<int>(std::ceil(samples.size() * 0.95)) - 1,
                                   0,
                                   lastIndex);
      return samples.at(index);
    };

    const QVector<QPoint> popupPoints = buildSweepPoints(popupMenu, 10, 16, 10, 10);
    const QVector<QPoint> inlinePoints = buildSweepPoints(inlineMenu, 10, 16, 42, 10);
    QVERIFY(!popupPoints.isEmpty());
    QVERIFY(!inlinePoints.isEmpty());

    // Warm up caches and first-paint paths.
    (void)collectMoveDurationsUs(popupMenu, popupPoints, 2);
    (void)collectMoveDurationsUs(inlineMenu, inlinePoints, 2);

    const QVector<qint64> popupSamples = collectMoveDurationsUs(popupMenu, popupPoints, 16);
    const QVector<qint64> inlineSamples = collectMoveDurationsUs(inlineMenu, inlinePoints, 16);
    QVERIFY(!popupSamples.isEmpty());
    QVERIFY(!inlineSamples.isEmpty());

    const double popupAvg = averageUs(popupSamples);
    const double inlineAvg = averageUs(inlineSamples);
    const qint64 popupP95 = p95Us(popupSamples);
    const qint64 inlineP95 = p95Us(inlineSamples);

    qInfo().noquote()
        << QStringLiteral("menu hover latency us: popup avg=%1 p95=%2, inline avg=%3 p95=%4")
               .arg(popupAvg, 0, 'f', 1)
               .arg(popupP95)
               .arg(inlineAvg, 0, 'f', 1)
               .arg(inlineP95);

    const double avgRatio = inlineAvg <= 0.0 ? popupAvg : popupAvg / inlineAvg;
    const double p95Ratio =
        inlineP95 <= 0 ? static_cast<double>(popupP95) : static_cast<double>(popupP95) / static_cast<double>(inlineP95);

    QVERIFY2(avgRatio <= 2.2,
             qPrintable(QStringLiteral("popup hover avg latency too high, ratio=%1 popupAvgUs=%2 inlineAvgUs=%3")
                            .arg(avgRatio, 0, 'f', 3)
                            .arg(popupAvg, 0, 'f', 1)
                            .arg(inlineAvg, 0, 'f', 1)));
    QVERIFY2(p95Ratio <= 2.6,
             qPrintable(QStringLiteral("popup hover p95 latency too high, ratio=%1 popupP95Us=%2 inlineP95Us=%3")
                            .arg(p95Ratio, 0, 'f', 3)
                            .arg(popupP95)
                            .arg(inlineP95)));
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

  void select_singleReadOnlyInputClickTogglesPopupReliably() {
    AdSelect select;

    AdSelect::Option lucy;
    lucy.value = QStringLiteral("lucy");
    lucy.label = QStringLiteral("Lucy");
    select.setOptions({lucy});
    select.setValue(lucy.value);
    select.resize(260, 40);
    select.show();

    QTRY_VERIFY_WITH_TIMEOUT(select.isVisible(), 400);

    QLineEdit* input = select.findChild<QLineEdit*>(QStringLiteral("adselect-input"));
    QVERIFY(input != nullptr);
    QVERIFY(input->isReadOnly());

    QSignalSpy openChangedSpy(&select, &AdSelect::openChanged);
    const QPoint clickPos(input->width() / 2, std::max(1, input->height() / 2));

    for (int i = 0; i < 8; ++i) {
      const bool expectedOpen = (i % 2) == 0;
      QTest::mouseClick(input, Qt::LeftButton, Qt::NoModifier, clickPos);
      QTRY_COMPARE_WITH_TIMEOUT(select.open(), expectedOpen, 400);
      QTRY_COMPARE_WITH_TIMEOUT(openChangedSpy.count(), i + 1, 400);
      QCOMPARE(openChangedSpy.at(i).at(0).toBool(), expectedOpen);
    }
  }

  void select_singleSearchInputClickTogglesPopupReliably() {
    AdSelect select;

    AdSelect::Option lucy;
    lucy.value = QStringLiteral("lucy");
    lucy.label = QStringLiteral("Lucy");
    select.setOptions({lucy});
    select.setSearchEnabled(true);
    select.resize(260, 40);
    select.show();

    QTRY_VERIFY_WITH_TIMEOUT(select.isVisible(), 400);

    QLineEdit* input = select.findChild<QLineEdit*>(QStringLiteral("adselect-input"));
    QVERIFY(input != nullptr);
    QVERIFY(!input->isReadOnly());

    QSignalSpy openChangedSpy(&select, &AdSelect::openChanged);
    const QPoint clickPos(input->width() / 2, std::max(1, input->height() / 2));

    for (int i = 0; i < 8; ++i) {
      const bool expectedOpen = (i % 2) == 0;
      QTest::mouseClick(input, Qt::LeftButton, Qt::NoModifier, clickPos);
      QTRY_COMPARE_WITH_TIMEOUT(select.open(), expectedOpen, 400);
      QTRY_COMPARE_WITH_TIMEOUT(openChangedSpy.count(), i + 1, 400);
      QCOMPARE(openChangedSpy.at(i).at(0).toBool(), expectedOpen);
    }
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

  void popupHost_nestedOwnerDoesNotCloseParentAndRestoresOnChildClose() {
    QWidget scope;
    scope.resize(420, 320);
    scope.show();
    QTRY_VERIFY_WITH_TIMEOUT(scope.isVisible(), 400);

    FakePopupOwner parentOwner(&scope);
    parentOwner.anchorWidget()->setGeometry(24, 24, 120, 32);
    adqt::widgets::detail::setInWindowPopupHostOpen(&parentOwner, true);
    QTRY_VERIFY_WITH_TIMEOUT(parentOwner.relayoutCount() >= 1, 400);
    parentOwner.resetRelayoutCount();
    QCOMPARE(parentOwner.closeCount(), 0);
    QVERIFY(parentOwner.popupIsVisible());

    FakePopupOwner childOwner(&scope);
    const QPoint parentPopupTopLeft =
        parentOwner.anchorWidget()->mapToGlobal(QPoint(0, parentOwner.anchorWidget()->height()));
    const QPoint childCenterGlobal = parentPopupTopLeft + QPoint(40, 40);
    const QPoint childTopLeftInScope = scope.mapFromGlobal(childCenterGlobal - QPoint(60, 16));
    childOwner.anchorWidget()->setGeometry(childTopLeftInScope.x(), childTopLeftInScope.y(), 120, 32);
    QVERIFY(parentOwner.popupContainsGlobalPos(
        childOwner.anchorWidget()->mapToGlobal(childOwner.anchorWidget()->rect().center())));
    adqt::widgets::detail::setInWindowPopupHostOpen(&childOwner, true);
    QTRY_VERIFY_WITH_TIMEOUT(childOwner.relayoutCount() >= 1, 400);
    QVERIFY(parentOwner.popupContainsGlobalPos(
        childOwner.anchorWidget()->mapToGlobal(childOwner.anchorWidget()->rect().center())));
    QCOMPARE(parentOwner.closeCount(), 0);
    QCOMPARE(childOwner.closeCount(), 0);

    // Click inside parent popup but outside child popup: close only child.
    sendMousePress(&scope, Qt::LeftButton, Qt::NoModifier, QPoint(30, 70));
    QTRY_COMPARE_WITH_TIMEOUT(childOwner.closeCount(), 1, 400);
    QCOMPARE(childOwner.closeReasons().constLast(),
             adqt::widgets::detail::PopupCloseReason::OutsidePressInScope);
    QCOMPARE(parentOwner.closeCount(), 0);

    // Child closes and parent should be restored as active owner.
    adqt::widgets::detail::setInWindowPopupHostOpen(&childOwner, false);
    sendMousePress(&scope, Qt::LeftButton, Qt::NoModifier, QPoint(scope.width() - 8, scope.height() - 8));
    QTRY_COMPARE_WITH_TIMEOUT(parentOwner.closeCount(), 1, 400);
    QCOMPARE(parentOwner.closeReasons().constLast(),
             adqt::widgets::detail::PopupCloseReason::OutsidePressInScope);
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

  void interactionOverlay_buttonWaveInSurfaceUsesSurfaceOverlay() {
    applyTimingConfig(buildTimingConfig(12, 1000, 180, 0, 100, 0));

    QWidget host;
    host.resize(420, 280);
    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);

    QWidget surface(&host);
    surface.setProperty("adqt.interaction.surface", true);
    surface.setGeometry(120, 40, 220, 160);
    surface.setAutoFillBackground(true);
    QPalette surfacePalette = surface.palette();
    surfacePalette.setColor(QPalette::Window, QColor(210, 64, 64));
    surface.setPalette(surfacePalette);
    surface.show();
    surface.raise();
    QTRY_VERIFY_WITH_TIMEOUT(surface.isVisible(), 400);

    AdButton button(QStringLiteral("Surface"), &surface);
    button.setGeometry(24, 48, 140, 40);
    button.show();
    QTRY_VERIFY_WITH_TIMEOUT(button.isVisible(), 400);

    sendMouseClick(&button);

    QWidget* overlay = nullptr;
    QElapsedTimer timer;
    timer.start();
    while (!overlay && timer.elapsed() < 500) {
      overlay = findInteractionOverlayWidgetInParent(&surface);
      if (!overlay) {
        QTest::qWait(10);
      }
    }

    QVERIFY2(overlay != nullptr, "failed to find interaction overlay under surface root");
    QVERIFY(overlay->parentWidget() == &surface);
    QTRY_VERIFY_WITH_TIMEOUT(overlay->isVisible(), 250);
  }

  void interactionOverlay_baseWaveDoesNotDrawAcrossSurface() {
    applyTimingConfig(buildTimingConfig(12, 1000, 200, 0, 100, 0));

    QWidget host;
    host.resize(440, 280);
    host.setAutoFillBackground(true);
    QPalette hostPalette = host.palette();
    hostPalette.setColor(QPalette::Window, QColor("#ffffff"));
    host.setPalette(hostPalette);
    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);

    QPushButton owner(QStringLiteral("owner"), &host);
    owner.setGeometry(70, 80, 140, 44);
    owner.show();
    QTRY_VERIFY_WITH_TIMEOUT(owner.isVisible(), 400);

    QWidget surface(&host);
    surface.setProperty("adqt.interaction.surface", true);
    surface.setGeometry(150, 36, 220, 180);
    surface.setAutoFillBackground(true);
    QPalette surfacePalette = surface.palette();
    surfacePalette.setColor(QPalette::Window, QColor(225, 58, 58));
    surface.setPalette(surfacePalette);
    surface.show();
    surface.raise();
    QTRY_VERIFY_WITH_TIMEOUT(surface.isVisible(), 400);

    const QPoint samplePoint(209, 102);
    const QColor before = sampleWidgetPixel(&host, samplePoint);

    InteractionWaveRequest request;
    request.owner = &owner;
    request.baseRectInWindow = owner.geometry();
    request.topLeft = 8.0;
    request.topRight = 8.0;
    request.bottomRight = 8.0;
    request.bottomLeft = 8.0;
    request.color = QColor("#1677ff");
    adqt::widgets::triggerInteractionWave(request);

    QWidget* baseOverlay = nullptr;
    QElapsedTimer overlayTimer;
    overlayTimer.start();
    while (!baseOverlay && overlayTimer.elapsed() < 500) {
      baseOverlay = findInteractionOverlayWidget(&host);
      if (!baseOverlay) {
        QTest::qWait(10);
      }
    }
    QVERIFY(baseOverlay != nullptr);
    QTRY_VERIFY_WITH_TIMEOUT(baseOverlay->isVisible(), 250);

    QTest::qWait(40);
    const QColor after = sampleWidgetPixel(&host, samplePoint);
    const int colorDelta = std::abs(after.red() - before.red()) +
                           std::abs(after.green() - before.green()) +
                           std::abs(after.blue() - before.blue()) +
                           std::abs(after.alpha() - before.alpha());
    QVERIFY2(colorDelta < 24,
             qPrintable(QStringLiteral("base overlay leaked across surface, delta=%1 before=%2 after=%3")
                            .arg(colorDelta)
                            .arg(colorToString(before))
                            .arg(colorToString(after))));
  }

  void interactionOverlay_buttonFocusInSurfaceUsesSurfaceOverlay() {
    QWidget host;
    host.resize(420, 280);
    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);

    QWidget surface(&host);
    surface.setProperty("adqt.interaction.surface", true);
    surface.setGeometry(96, 40, 220, 160);
    surface.show();
    surface.raise();
    QTRY_VERIFY_WITH_TIMEOUT(surface.isVisible(), 400);

    AdButton button(QStringLiteral("Focus"), &surface);
    button.setGeometry(26, 48, 140, 40);
    button.show();
    QTRY_VERIFY_WITH_TIMEOUT(button.isVisible(), 400);

    button.setFocus(Qt::TabFocusReason);
    QTRY_VERIFY_WITH_TIMEOUT(button.hasFocus(), 300);

    QWidget* overlay = nullptr;
    QElapsedTimer timer;
    timer.start();
    while (!overlay && timer.elapsed() < 500) {
      overlay = findInteractionOverlayWidgetInParent(&surface);
      if (!overlay) {
        QTest::qWait(10);
      }
    }

    QVERIFY2(overlay != nullptr, "failed to find focus overlay for surface button");
    QVERIFY(overlay->parentWidget() == &surface);
    QTRY_VERIFY_WITH_TIMEOUT(overlay->isVisible(), 250);

    host.setFocus(Qt::OtherFocusReason);
    QTRY_VERIFY_WITH_TIMEOUT(!button.hasFocus(), 300);
    QTRY_VERIFY_WITH_TIMEOUT(!overlay->isVisible(), 400);
  }

  void interactionOverlay_switchWaveInSurfaceUsesSurfaceOverlay() {
    applyTimingConfig(buildTimingConfig(12, 1000, 180, 0, 100, 0));

    QWidget host;
    host.resize(420, 280);
    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);

    QWidget surface(&host);
    surface.setProperty("adqt.interaction.surface", true);
    surface.setGeometry(100, 40, 220, 160);
    surface.show();
    surface.raise();
    QTRY_VERIFY_WITH_TIMEOUT(surface.isVisible(), 400);

    AdSwitch sw(&surface);
    sw.setGeometry(24, 54, 64, 28);
    sw.show();
    QTRY_VERIFY_WITH_TIMEOUT(sw.isVisible(), 400);

    sendMousePress(&sw);
    sendMouseRelease(&sw);

    QWidget* overlay = nullptr;
    QElapsedTimer timer;
    timer.start();
    while (!overlay && timer.elapsed() < 500) {
      overlay = findInteractionOverlayWidgetInParent(&surface);
      if (!overlay) {
        QTest::qWait(10);
      }
    }

    QVERIFY2(overlay != nullptr, "failed to find interaction overlay for switch inside surface");
    QVERIFY(overlay->parentWidget() == &surface);
    QTRY_VERIFY_WITH_TIMEOUT(overlay->isVisible(), 250);
  }

  void interactionOverlay_radioFocusInSurfaceUsesSurfaceOverlay() {
    QWidget host;
    host.resize(420, 280);
    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);

    QWidget surface(&host);
    surface.setProperty("adqt.interaction.surface", true);
    surface.setGeometry(88, 40, 220, 160);
    surface.show();
    surface.raise();
    QTRY_VERIFY_WITH_TIMEOUT(surface.isVisible(), 400);

    AdRadio radio(QStringLiteral("Option"), &surface);
    radio.setValue(QStringLiteral("v1"));
    radio.setGeometry(22, 50, 140, 32);
    radio.show();
    QTRY_VERIFY_WITH_TIMEOUT(radio.isVisible(), 400);

    radio.setFocus(Qt::TabFocusReason);
    QTRY_VERIFY_WITH_TIMEOUT(radio.hasFocus(), 300);

    QWidget* overlay = nullptr;
    QElapsedTimer timer;
    timer.start();
    while (!overlay && timer.elapsed() < 500) {
      overlay = findInteractionOverlayWidgetInParent(&surface);
      if (!overlay) {
        QTest::qWait(10);
      }
    }

    QVERIFY2(overlay != nullptr, "failed to find interaction overlay for radio inside surface");
    QVERIFY(overlay->parentWidget() == &surface);
    QTRY_VERIFY_WITH_TIMEOUT(overlay->isVisible(), 250);

    host.setFocus(Qt::OtherFocusReason);
    QTRY_VERIFY_WITH_TIMEOUT(!radio.hasFocus(), 300);
    QTRY_VERIFY_WITH_TIMEOUT(!overlay->isVisible(), 400);
  }

  void interactionOverlay_focusFollowsScrollOffset() {
    QWidget host;
    host.resize(460, 320);
    host.setAutoFillBackground(true);
    QPalette hostPalette = host.palette();
    hostPalette.setColor(QPalette::Window, QColor("#ffffff"));
    host.setPalette(hostPalette);

    auto* area = new QScrollArea(&host);
    area->setGeometry(0, 0, host.width(), host.height());
    area->setWidgetResizable(false);
    area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    area->setFrameShape(QFrame::NoFrame);
    area->setAutoFillBackground(true);
    QPalette areaPalette = area->palette();
    areaPalette.setColor(QPalette::Window, QColor("#ffffff"));
    area->setPalette(areaPalette);

    auto* content = new QWidget();
    content->setAutoFillBackground(true);
    QPalette contentPalette = content->palette();
    contentPalette.setColor(QPalette::Window, QColor("#ffffff"));
    content->setPalette(contentPalette);
    content->resize(420, 960);
    area->setWidget(content);

    AdButton button(QStringLiteral("Scroll Focus"), content);
    button.setGeometry(64, 260, 150, 38);
    button.show();

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(button.isVisible(), 400);
    QVERIFY(area->verticalScrollBar() != nullptr);
    QVERIFY(area->verticalScrollBar()->maximum() > area->verticalScrollBar()->minimum());

    button.setFocus(Qt::TabFocusReason);
    QTRY_VERIFY_WITH_TIMEOUT(button.hasFocus(), 300);

    QWidget* overlay = nullptr;
    QElapsedTimer timer;
    timer.start();
    while (!overlay && timer.elapsed() < 500) {
      overlay = findInteractionOverlayWidget(&host);
      if (!overlay) {
        QTest::qWait(10);
      }
    }
    QVERIFY(overlay != nullptr);
    QTRY_VERIFY_WITH_TIMEOUT(overlay->isVisible(), 250);

    const QRect oldGlobalRect(button.mapToGlobal(QPoint(0, 0)), button.rect().size());
    const QPoint oldSampleGlobal(oldGlobalRect.left() - 2, oldGlobalRect.center().y());
    const QPoint oldSampleInHost = host.mapFromGlobal(oldSampleGlobal);
    const QColor oldBefore = sampleWidgetPixel(&host, oldSampleInHost);

    const int scrollValue =
        std::min(area->verticalScrollBar()->maximum(), area->verticalScrollBar()->value() + 120);
    area->verticalScrollBar()->setValue(scrollValue);
    QTRY_VERIFY_WITH_TIMEOUT(area->verticalScrollBar()->value() == scrollValue, 300);

    const QRect newGlobalRect(button.mapToGlobal(QPoint(0, 0)), button.rect().size());
    QVERIFY(newGlobalRect.top() < oldGlobalRect.top());
    const QPoint newSampleGlobal(newGlobalRect.left() - 2, newGlobalRect.center().y());
    const QPoint newSampleInHost = host.mapFromGlobal(newSampleGlobal);

    QTest::qWait(60);
    QCoreApplication::processEvents();

    const QColor oldAfter = sampleWidgetPixel(&host, oldSampleInHost);
    const QColor newAfter = sampleWidgetPixel(&host, newSampleInHost);
    const QColor background = QColor("#ffffff");
    auto distanceToBg = [&background](const QColor& color) {
      return std::abs(color.red() - background.red()) + std::abs(color.green() - background.green()) +
             std::abs(color.blue() - background.blue()) + std::abs(color.alpha() - background.alpha());
    };

    const int oldDistance = distanceToBg(oldAfter);
    const int newDistance = distanceToBg(newAfter);
    QVERIFY2(newDistance > oldDistance + 10,
             qPrintable(QStringLiteral("focus overlay did not follow scroll: oldDistance=%1 newDistance=%2 oldBefore=%3 oldAfter=%4 newAfter=%5")
                            .arg(oldDistance)
                            .arg(newDistance)
                            .arg(colorToString(oldBefore))
                            .arg(colorToString(oldAfter))
                            .arg(colorToString(newAfter))));
  }

  void interactionOverlay_inputFocusFollowsScrollOffset() {
    QWidget host;
    host.resize(460, 320);
    host.setAutoFillBackground(true);
    QPalette hostPalette = host.palette();
    hostPalette.setColor(QPalette::Window, QColor("#ffffff"));
    host.setPalette(hostPalette);

    auto* area = new QScrollArea(&host);
    area->setGeometry(0, 0, host.width(), host.height());
    area->setWidgetResizable(false);
    area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    area->setFrameShape(QFrame::NoFrame);
    area->setAutoFillBackground(true);
    QPalette areaPalette = area->palette();
    areaPalette.setColor(QPalette::Window, QColor("#ffffff"));
    area->setPalette(areaPalette);

    auto* content = new QWidget();
    content->setAutoFillBackground(true);
    QPalette contentPalette = content->palette();
    contentPalette.setColor(QPalette::Window, QColor("#ffffff"));
    content->setPalette(contentPalette);
    content->resize(420, 960);
    area->setWidget(content);

    AdInput input(content);
    input.setGeometry(64, 260, 220, 38);
    input.show();

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(input.isVisible(), 400);
    QVERIFY(area->verticalScrollBar() != nullptr);
    QVERIFY(area->verticalScrollBar()->maximum() > area->verticalScrollBar()->minimum());

    input.focusInput(AdInput::FocusCursor::End, false);
    QTRY_VERIFY_WITH_TIMEOUT(input.lineEdit() && input.lineEdit()->hasFocus(), 300);

    QWidget* overlay = nullptr;
    QElapsedTimer timer;
    timer.start();
    while (!overlay && timer.elapsed() < 500) {
      overlay = findInteractionOverlayWidget(&host);
      if (!overlay) {
        QTest::qWait(10);
      }
    }
    QVERIFY(overlay != nullptr);
    QTRY_VERIFY_WITH_TIMEOUT(overlay->isVisible(), 250);

    const QRect oldGlobalRect(input.mapToGlobal(QPoint(0, 0)), input.rect().size());
    const QPoint oldSampleGlobal(oldGlobalRect.left() - 2, oldGlobalRect.center().y());
    const QPoint oldSampleInHost = host.mapFromGlobal(oldSampleGlobal);
    const QColor oldBefore = sampleWidgetPixel(&host, oldSampleInHost);

    const int scrollValue =
        std::min(area->verticalScrollBar()->maximum(), area->verticalScrollBar()->value() + 120);
    area->verticalScrollBar()->setValue(scrollValue);
    QTRY_VERIFY_WITH_TIMEOUT(area->verticalScrollBar()->value() == scrollValue, 300);

    const QRect newGlobalRect(input.mapToGlobal(QPoint(0, 0)), input.rect().size());
    QVERIFY(newGlobalRect.top() < oldGlobalRect.top());
    const QPoint newSampleGlobal(newGlobalRect.left() - 2, newGlobalRect.center().y());
    const QPoint newSampleInHost = host.mapFromGlobal(newSampleGlobal);

    QTest::qWait(60);
    QCoreApplication::processEvents();

    const QColor oldAfter = sampleWidgetPixel(&host, oldSampleInHost);
    const QColor newAfter = sampleWidgetPixel(&host, newSampleInHost);
    const QColor background = QColor("#ffffff");
    auto distanceToBg = [&background](const QColor& color) {
      return std::abs(color.red() - background.red()) + std::abs(color.green() - background.green()) +
             std::abs(color.blue() - background.blue()) + std::abs(color.alpha() - background.alpha());
    };

    const int oldDistance = distanceToBg(oldAfter);
    const int newDistance = distanceToBg(newAfter);
    QVERIFY2(newDistance > oldDistance + 10,
             qPrintable(QStringLiteral("input focus overlay did not follow scroll: oldDistance=%1 newDistance=%2 oldBefore=%3 oldAfter=%4 newAfter=%5")
                            .arg(oldDistance)
                            .arg(newDistance)
                            .arg(colorToString(oldBefore))
                            .arg(colorToString(oldAfter))
                            .arg(colorToString(newAfter))));
  }

  void interactionOverlay_inputNumberFocusFollowsScrollOffset() {
    QWidget host;
    host.resize(460, 320);
    host.setAutoFillBackground(true);
    QPalette hostPalette = host.palette();
    hostPalette.setColor(QPalette::Window, QColor("#ffffff"));
    host.setPalette(hostPalette);

    auto* area = new QScrollArea(&host);
    area->setGeometry(0, 0, host.width(), host.height());
    area->setWidgetResizable(false);
    area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    area->setFrameShape(QFrame::NoFrame);
    area->setAutoFillBackground(true);
    QPalette areaPalette = area->palette();
    areaPalette.setColor(QPalette::Window, QColor("#ffffff"));
    area->setPalette(areaPalette);

    auto* content = new QWidget();
    content->setAutoFillBackground(true);
    QPalette contentPalette = content->palette();
    contentPalette.setColor(QPalette::Window, QColor("#ffffff"));
    content->setPalette(contentPalette);
    content->resize(420, 960);
    area->setWidget(content);

    AdInputNumber inputNumber(content);
    inputNumber.setGeometry(64, 260, 220, 38);
    inputNumber.show();

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(inputNumber.isVisible(), 400);
    QVERIFY(area->verticalScrollBar() != nullptr);
    QVERIFY(area->verticalScrollBar()->maximum() > area->verticalScrollBar()->minimum());

    inputNumber.focusInput(AdInputNumber::FocusCursor::End, false);
    QTRY_VERIFY_WITH_TIMEOUT(inputNumber.lineEdit() && inputNumber.lineEdit()->hasFocus(), 300);

    QWidget* overlay = nullptr;
    QElapsedTimer timer;
    timer.start();
    while (!overlay && timer.elapsed() < 500) {
      overlay = findInteractionOverlayWidget(&host);
      if (!overlay) {
        QTest::qWait(10);
      }
    }
    QVERIFY(overlay != nullptr);
    QTRY_VERIFY_WITH_TIMEOUT(overlay->isVisible(), 250);

    const QRect oldGlobalRect(inputNumber.mapToGlobal(QPoint(0, 0)), inputNumber.rect().size());
    const QPoint oldSampleGlobal(oldGlobalRect.left() - 2, oldGlobalRect.center().y());
    const QPoint oldSampleInHost = host.mapFromGlobal(oldSampleGlobal);
    const QColor oldBefore = sampleWidgetPixel(&host, oldSampleInHost);

    const int scrollValue =
        std::min(area->verticalScrollBar()->maximum(), area->verticalScrollBar()->value() + 120);
    area->verticalScrollBar()->setValue(scrollValue);
    QTRY_VERIFY_WITH_TIMEOUT(area->verticalScrollBar()->value() == scrollValue, 300);

    const QRect newGlobalRect(inputNumber.mapToGlobal(QPoint(0, 0)), inputNumber.rect().size());
    QVERIFY(newGlobalRect.top() < oldGlobalRect.top());
    const QPoint newSampleGlobal(newGlobalRect.left() - 2, newGlobalRect.center().y());
    const QPoint newSampleInHost = host.mapFromGlobal(newSampleGlobal);

    QTest::qWait(60);
    QCoreApplication::processEvents();

    const QColor oldAfter = sampleWidgetPixel(&host, oldSampleInHost);
    const QColor newAfter = sampleWidgetPixel(&host, newSampleInHost);
    const QColor background = QColor("#ffffff");
    auto distanceToBg = [&background](const QColor& color) {
      return std::abs(color.red() - background.red()) + std::abs(color.green() - background.green()) +
             std::abs(color.blue() - background.blue()) + std::abs(color.alpha() - background.alpha());
    };

    const int oldDistance = distanceToBg(oldAfter);
    const int newDistance = distanceToBg(newAfter);
    QVERIFY2(newDistance > oldDistance + 10,
             qPrintable(QStringLiteral("input number focus overlay did not follow scroll: oldDistance=%1 newDistance=%2 oldBefore=%3 oldAfter=%4 newAfter=%5")
                            .arg(oldDistance)
                            .arg(newDistance)
                            .arg(colorToString(oldBefore))
                            .arg(colorToString(oldAfter))
                            .arg(colorToString(newAfter))));
  }

  void interactionOverlay_selectFocusFollowsScrollOffset() {
    QWidget host;
    host.resize(460, 320);
    host.setAutoFillBackground(true);
    QPalette hostPalette = host.palette();
    hostPalette.setColor(QPalette::Window, QColor("#ffffff"));
    host.setPalette(hostPalette);

    auto* area = new QScrollArea(&host);
    area->setGeometry(0, 0, host.width(), host.height());
    area->setWidgetResizable(false);
    area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    area->setFrameShape(QFrame::NoFrame);
    area->setAutoFillBackground(true);
    QPalette areaPalette = area->palette();
    areaPalette.setColor(QPalette::Window, QColor("#ffffff"));
    area->setPalette(areaPalette);

    auto* content = new QWidget();
    content->setAutoFillBackground(true);
    QPalette contentPalette = content->palette();
    contentPalette.setColor(QPalette::Window, QColor("#ffffff"));
    content->setPalette(contentPalette);
    content->resize(420, 960);
    area->setWidget(content);

    AdSelect select(content);
    select.setGeometry(64, 260, 220, 38);
    QVector<AdSelect::Option> options;
    {
      AdSelect::Option option;
      option.value = QStringLiteral("v1");
      option.label = QStringLiteral("Option 1");
      options.push_back(option);
    }
    select.setOptions(options);
    select.show();

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(select.isVisible(), 400);
    QVERIFY(area->verticalScrollBar() != nullptr);
    QVERIFY(area->verticalScrollBar()->maximum() > area->verticalScrollBar()->minimum());
    QVERIFY(select.lineEdit_ != nullptr);

    select.lineEdit_->setFocus(Qt::TabFocusReason);
    QTRY_VERIFY_WITH_TIMEOUT(select.lineEdit_->hasFocus(), 300);
    QTRY_VERIFY_WITH_TIMEOUT(select.hasFocusWithin_, 300);

    QWidget* overlay = nullptr;
    QElapsedTimer timer;
    timer.start();
    while (!overlay && timer.elapsed() < 500) {
      overlay = findInteractionOverlayWidget(&host);
      if (!overlay) {
        QTest::qWait(10);
      }
    }
    QVERIFY(overlay != nullptr);
    QTRY_VERIFY_WITH_TIMEOUT(overlay->isVisible(), 250);

    const QRect oldGlobalRect(select.mapToGlobal(QPoint(0, 0)), select.rect().size());
    const QPoint oldSampleGlobal(oldGlobalRect.left() - 2, oldGlobalRect.center().y());
    const QPoint oldSampleInHost = host.mapFromGlobal(oldSampleGlobal);
    const QColor oldBefore = sampleWidgetPixel(&host, oldSampleInHost);

    const int scrollValue =
        std::min(area->verticalScrollBar()->maximum(), area->verticalScrollBar()->value() + 120);
    area->verticalScrollBar()->setValue(scrollValue);
    QTRY_VERIFY_WITH_TIMEOUT(area->verticalScrollBar()->value() == scrollValue, 300);

    const QRect newGlobalRect(select.mapToGlobal(QPoint(0, 0)), select.rect().size());
    QVERIFY(newGlobalRect.top() < oldGlobalRect.top());
    const QPoint newSampleGlobal(newGlobalRect.left() - 2, newGlobalRect.center().y());
    const QPoint newSampleInHost = host.mapFromGlobal(newSampleGlobal);

    QTest::qWait(60);
    QCoreApplication::processEvents();

    const QColor oldAfter = sampleWidgetPixel(&host, oldSampleInHost);
    const QColor newAfter = sampleWidgetPixel(&host, newSampleInHost);
    const QColor background = QColor("#ffffff");
    auto distanceToBg = [&background](const QColor& color) {
      return std::abs(color.red() - background.red()) + std::abs(color.green() - background.green()) +
             std::abs(color.blue() - background.blue()) + std::abs(color.alpha() - background.alpha());
    };

    const int oldDistance = distanceToBg(oldAfter);
    const int newDistance = distanceToBg(newAfter);
    QVERIFY2(newDistance > oldDistance + 10,
             qPrintable(QStringLiteral("select focus overlay did not follow scroll: oldDistance=%1 newDistance=%2 oldBefore=%3 oldAfter=%4 newAfter=%5")
                            .arg(oldDistance)
                            .arg(newDistance)
                            .arg(colorToString(oldBefore))
                            .arg(colorToString(oldAfter))
                            .arg(colorToString(newAfter))));
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

  void slider_clickRailStartsHandleDrag_likeAntd() {
    AdSlider slider;
    slider.setValue(20);
    slider.resize(320, 56);
    slider.show();
    QTRY_VERIFY_WITH_TIMEOUT(slider.isVisible(), 400);

    const QPoint pressPoint(slider.width() - 16, slider.height() / 2);
    const double beforePress = slider.value();
    QTest::mousePress(&slider, Qt::LeftButton, Qt::NoModifier, pressPoint);

    QCOMPARE(slider.dragMode_, AdSlider::DragMode::Handle);
    QCOMPARE(slider.dragHandleIndex_, 0);
    QCOMPARE(slider.hoverHandleIndex_, 0);
    QCOMPARE(slider.focusHandleIndex_, 0);
    QVERIFY(slider.dragging_);
    QVERIFY(slider.value() > beforePress + 1.0);

    const double afterPress = slider.value();
    const QPoint movePoint(slider.width() / 2, slider.height() / 2);
    QTest::mouseMove(&slider, movePoint);
    QVERIFY(std::abs(slider.value() - afterPress) > 1.0);

    QTest::mouseRelease(&slider, Qt::LeftButton, Qt::NoModifier, movePoint);
    QCOMPARE(slider.dragMode_, AdSlider::DragMode::None);
    QVERIFY(!slider.dragging_);
    QCOMPARE(slider.dragHandleIndex_, -1);
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

  void slider_editable_dragOutRemovesHandle() {
    AdSlider slider;
    slider.setMode(AdSlider::Mode::Range);
    slider.setEditableHandles(true);
    slider.setMinHandleCount(1);
    slider.setMaxHandleCount(4);
    slider.setValues({20, 50, 80});
    slider.resize(340, 64);
    slider.show();
    QTRY_VERIFY_WITH_TIMEOUT(slider.isVisible(), 400);

    adqt::widgets::detail::SliderStyleInput styleInput;
    styleInput.mode = slider.mode();
    styleInput.orientation = slider.orientation();
    styleInput.baseFont = slider.font();
    styleInput.componentTokens = slider.componentTokens();
    const auto style = adqt::widgets::detail::resolveSliderVisualStyle(styleInput);

    const qreal valueSpan = std::max<qreal>(1.0, slider.maximum() - slider.minimum());
    const qreal axisStart = style.metrics.marginMain;
    const qreal axisLength = std::max<qreal>(1.0, slider.width() - style.metrics.marginMain * 2.0);
    const qreal ratio = (80.0 - slider.minimum()) / valueSpan;
    const int startX = qRound(axisStart + ratio * axisLength);
    const QPoint startPoint(startX, slider.height() / 2);

    QTest::mousePress(&slider, Qt::LeftButton, Qt::NoModifier, startPoint);
    QTest::mouseMove(&slider, QPoint(slider.width() + 240, slider.height() / 2));
    QTest::mouseRelease(&slider, Qt::LeftButton, Qt::NoModifier,
                        QPoint(slider.width() + 240, slider.height() / 2));

    QTRY_COMPARE_WITH_TIMEOUT(slider.values().size(), 2, 400);
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

  void slider_handleShadowTokens_followColorPickerShadowModel() {
    adqt::widgets::detail::SliderStyleInput input;
    input.baseFont = QFont();
    input.componentTokens.handleShadowColor = QStringLiteral("#d9d9d9");
    input.componentTokens.handleActiveShadowColor = QStringLiteral("#0958d9");

    const auto style = adqt::widgets::detail::resolveSliderVisualStyle(input);
    QCOMPARE(style.handleShadowColor, QColor(QStringLiteral("#d9d9d9")));
    QCOMPARE(style.handleActiveShadowColor, QColor(QStringLiteral("#0958d9")));

    adqt::widgets::detail::SliderStyleInput fallbackInput;
    fallbackInput.baseFont = QFont();
    fallbackInput.componentTokens.handleShadowColor = QStringLiteral("#f0f0f0");
    const auto fallbackStyle = adqt::widgets::detail::resolveSliderVisualStyle(fallbackInput);
    QCOMPARE(fallbackStyle.handleShadowColor, QColor(QStringLiteral("#f0f0f0")));
    QCOMPARE(fallbackStyle.handleActiveShadowColor, QColor(QStringLiteral("#f0f0f0")));

    adqt::widgets::detail::SliderStyleInput disabledInput;
    disabledInput.baseFont = QFont();
    disabledInput.disabled = true;
    disabledInput.componentTokens.handleShadowColor = QStringLiteral("#d9d9d9");
    disabledInput.componentTokens.handleActiveShadowColor = QStringLiteral("#0958d9");
    const auto disabledStyle = adqt::widgets::detail::resolveSliderVisualStyle(disabledInput);
    QCOMPARE(disabledStyle.handleShadowColor, QColor(0, 0, 0, 0));
    QCOMPARE(disabledStyle.handleActiveShadowColor, QColor(0, 0, 0, 0));
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

  void colorPicker_hexFormatWithAlpha_doesNotFallbackToRgb() {
    AdColorPicker picker;
    picker.resize(300, 44);
    picker.show();
    QTRY_VERIFY_WITH_TIMEOUT(picker.isVisible(), 400);

    picker.setFormat(AdColorPicker::Format::Hex);
    QCOMPARE(picker.format(), AdColorPicker::Format::Hex);

    picker.setOpen(true);
    QTRY_VERIFY_WITH_TIMEOUT(picker.open(), 400);

    picker.setValue(QStringLiteral("rgba(22, 119, 255, 0.5)"));
    QCoreApplication::sendPostedEvents(nullptr, 0);
    QCoreApplication::processEvents();
    QCOMPARE(picker.format(), AdColorPicker::Format::Hex);

    QWidget* formatRowWidget =
        picker.findChild<QWidget*>(QStringLiteral("ad-color-picker-format-row"),
                                   Qt::FindChildrenRecursively);
    QVERIFY(formatRowWidget != nullptr);
    AdInput* hexInput = nullptr;
    const QList<AdInput*> allInputs =
        formatRowWidget->findChildren<AdInput*>(QString(), Qt::FindChildrenRecursively);
    for (AdInput* input : allInputs) {
      if (input && input->prefixText() == QStringLiteral("#")) {
        hexInput = input;
        break;
      }
    }
    QVERIFY(hexInput != nullptr);
    QCOMPARE(hexInput->value().trimmed(), QStringLiteral("1677FF80"));

    picker.setValue(QStringLiteral("#1677ff80"));
    QCoreApplication::sendPostedEvents(nullptr, 0);
    QCoreApplication::processEvents();
    QCOMPARE(picker.format(), AdColorPicker::Format::Hex);
    QCOMPARE(hexInput->value().trimmed(), QStringLiteral("1677FF80"));
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

    QAbstractButton* clearButton =
        picker.findChild<QAbstractButton*>(QStringLiteral("ad-color-picker-clear"),
                                           Qt::FindChildrenRecursively);
    QVERIFY(clearButton != nullptr);
    QVERIFY(clearButton->isVisible());

    sendMouseClick(clearButton);
    QTRY_COMPARE_WITH_TIMEOUT(onClearSpy.count(), 1, 400);
    QTRY_COMPARE_WITH_TIMEOUT(clearedSpy.count(), 1, 400);
    QCOMPARE(picker.value(), QString());
    QVERIFY(valueSpy.count() >= 1);
    QCOMPARE(valueSpy.last().at(0).toString(), QString());
  }

  void colorPicker_openStateKeepsFocusOverlayVisible() {
    applyTimingConfig(buildTimingConfig(12, 1000, 70, 0, 100, 0));

    QWidget host;
    host.resize(420, 280);

    AdColorPicker picker(&host);
    picker.setGeometry(24, 24, 260, 38);
    picker.show();
    host.show();

    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(picker.isVisible(), 400);

    auto* triggerFrame =
        picker.findChild<QFrame*>(QStringLiteral("ad-color-picker-trigger-frame"),
                                  Qt::FindChildrenRecursively);
    QVERIFY(triggerFrame != nullptr);

    sendMouseClick(triggerFrame);
    QTRY_VERIFY_WITH_TIMEOUT(picker.open(), 400);

    QWidget* overlay = nullptr;
    QElapsedTimer findOverlayTimer;
    findOverlayTimer.start();
    while (!overlay && findOverlayTimer.elapsed() < 300) {
      overlay = findInteractionOverlayWidget(&host);
      if (!overlay) {
        QTest::qWait(10);
      }
    }
    QVERIFY(overlay != nullptr);
    QTRY_VERIFY_WITH_TIMEOUT(overlay->isVisible(), 200);

    // Focus overlay should stay visible for the whole open state, not fade like click-wave.
    QTest::qWait(180);
    QCoreApplication::processEvents();
    QVERIFY(overlay->isVisible());

    picker.setOpen(false);
    QTRY_VERIFY_WITH_TIMEOUT(!picker.open(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(!overlay->isVisible(), 400);
  }

  void colorPicker_gradientModeAndPresets() {
    AdColorPicker picker;
    picker.resize(260, 38);
    picker.setModeOptions({AdColorPicker::Mode::Single, AdColorPicker::Mode::Gradient});
    picker.setMode(AdColorPicker::Mode::Gradient);
    picker.setShowText(true);
    picker.show();
    QTRY_VERIFY_WITH_TIMEOUT(picker.isVisible(), 400);

    AdColorPicker::ColorValue gradientValue;
    gradientValue.gradientStops = {
        AdColorPicker::GradientStop{QStringLiteral("rgb(16, 142, 233)"), 0},
        AdColorPicker::GradientStop{QStringLiteral("rgb(135, 208, 104)"), 100},
    };
    picker.setColorValue(gradientValue);
    QVERIFY(picker.value().startsWith(QStringLiteral("linear-gradient(")));
    auto* triggerText = picker.findChild<QLabel*>(QStringLiteral("ad-color-picker-trigger-text"),
                                                  Qt::FindChildrenRecursively);
    QVERIFY(triggerText != nullptr);
    QCOMPARE(triggerText->text(),
             QStringLiteral("rgb(16,142,233) 0%, rgb(135,208,104) 100%"));
    QCOMPARE(triggerText->textFormat(), Qt::AutoText);

    picker.setOpen(true);
    QTRY_VERIFY_WITH_TIMEOUT(picker.open(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(triggerText->textFormat() == Qt::RichText, 400);
    QVERIFY(triggerText->text().contains(QStringLiteral("<span"), Qt::CaseInsensitive));
    QVERIFY(triggerText->text().contains(QStringLiteral("rgb(16,142,233) 0%")));
    QVERIFY(triggerText->text().contains(QStringLiteral("rgb(135,208,104) 100%")));

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

  void colorPicker_gradientSliderMoveCrossKeepsDraggedStopColor() {
    AdColorPicker picker;
    picker.resize(260, 38);
    picker.setModeOptions({AdColorPicker::Mode::Gradient});
    picker.setMode(AdColorPicker::Mode::Gradient);
    picker.show();
    QTRY_VERIFY_WITH_TIMEOUT(picker.isVisible(), 400);
    picker.setOpen(true);
    QTRY_VERIFY_WITH_TIMEOUT(picker.open(), 400);

    AdColorPicker::ColorValue value;
    value.gradientStops = {
        AdColorPicker::GradientStop{QStringLiteral("#ff0000"), 0},
        AdColorPicker::GradientStop{QStringLiteral("#00ff00"), 50},
        AdColorPicker::GradientStop{QStringLiteral("#0000ff"), 100},
    };
    picker.setColorValue(value);

    AdSlider* gradientSlider = nullptr;
    const QList<AdSlider*> sliders =
        picker.findChildren<AdSlider*>(QString(), Qt::FindChildrenRecursively);
    for (AdSlider* slider : sliders) {
      if (!slider) {
        continue;
      }
      if (slider->mode() == AdSlider::Mode::Range && qRound(slider->minimum()) == 0 &&
          qRound(slider->maximum()) == 100 && slider->editableHandles()) {
        gradientSlider = slider;
        break;
      }
    }
    QVERIFY(gradientSlider != nullptr);

    gradientSlider->setValues({50, 80, 100});
    QCoreApplication::processEvents();

    const AdColorPicker::ColorValue out = picker.colorValue();
    QCOMPARE(out.gradientStops.size(), 3);
    QCOMPARE(out.gradientStops.at(0).percent, 50);
    QCOMPARE(out.gradientStops.at(1).percent, 80);
    QCOMPARE(out.gradientStops.at(2).percent, 100);

    const QColor stop0 = parseThemeColor(out.gradientStops.at(0).color, QColor());
    const QColor stop1 = parseThemeColor(out.gradientStops.at(1).color, QColor());
    const QColor stop2 = parseThemeColor(out.gradientStops.at(2).color, QColor());
    QVERIFY(stop0.isValid());
    QVERIFY(stop1.isValid());
    QVERIFY(stop2.isValid());
    QVERIFY(colorClose(stop0, QColor(QStringLiteral("#00ff00")), 2));
    QVERIFY(colorClose(stop1, QColor(QStringLiteral("#ff0000")), 2));
    QVERIFY(colorClose(stop2, QColor(QStringLiteral("#0000ff")), 2));
  }

  void colorPicker_gradientSliderAddMoveDeleteMatchesAntdModel() {
    AdColorPicker picker;
    picker.resize(260, 38);
    picker.setModeOptions({AdColorPicker::Mode::Gradient});
    picker.setMode(AdColorPicker::Mode::Gradient);
    picker.show();
    QTRY_VERIFY_WITH_TIMEOUT(picker.isVisible(), 400);
    picker.setOpen(true);
    QTRY_VERIFY_WITH_TIMEOUT(picker.open(), 400);

    AdColorPicker::ColorValue value;
    value.gradientStops = {
        AdColorPicker::GradientStop{QStringLiteral("#ff0000"), 0},
        AdColorPicker::GradientStop{QStringLiteral("#0000ff"), 100},
    };
    picker.setColorValue(value);

    AdSlider* gradientSlider = nullptr;
    const QList<AdSlider*> sliders =
        picker.findChildren<AdSlider*>(QString(), Qt::FindChildrenRecursively);
    for (AdSlider* slider : sliders) {
      if (!slider) {
        continue;
      }
      if (slider->mode() == AdSlider::Mode::Range && qRound(slider->minimum()) == 0 &&
          qRound(slider->maximum()) == 100 && slider->editableHandles()) {
        gradientSlider = slider;
        break;
      }
    }
    QVERIFY(gradientSlider != nullptr);

    gradientSlider->setValues({0, 20, 100});
    QCoreApplication::processEvents();

    const AdColorPicker::ColorValue afterAdd = picker.colorValue();
    QCOMPARE(afterAdd.gradientStops.size(), 3);
    QCOMPARE(afterAdd.gradientStops.at(0).percent, 0);
    QCOMPARE(afterAdd.gradientStops.at(1).percent, 20);
    QCOMPARE(afterAdd.gradientStops.at(2).percent, 100);

    const QColor mixedAdd = parseThemeColor(afterAdd.gradientStops.at(1).color, QColor());
    QVERIFY(mixedAdd.isValid());
    QVERIFY(colorClose(mixedAdd, QColor(204, 0, 51), 2));

    gradientSlider->setValues({0, 30, 100});
    QCoreApplication::processEvents();

    const AdColorPicker::ColorValue afterMove = picker.colorValue();
    QCOMPARE(afterMove.gradientStops.size(), 3);
    QCOMPARE(afterMove.gradientStops.at(1).percent, 30);
    const QColor mixedMove = parseThemeColor(afterMove.gradientStops.at(1).color, QColor());
    QVERIFY(mixedMove.isValid());
    QVERIFY(colorClose(mixedMove, mixedAdd, 2));

    gradientSlider->setValues({0, 30});
    QCoreApplication::processEvents();

    const AdColorPicker::ColorValue afterDelete = picker.colorValue();
    QCOMPARE(afterDelete.gradientStops.size(), 2);
    QCOMPARE(afterDelete.gradientStops.at(0).percent, 0);
    QCOMPARE(afterDelete.gradientStops.at(1).percent, 30);
    const QColor mixedDelete = parseThemeColor(afterDelete.gradientStops.at(1).color, QColor());
    QVERIFY(mixedDelete.isValid());
    QVERIFY(colorClose(mixedDelete, mixedAdd, 2));
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

  void colorPicker_formatSelectMatchesAntdInputModel() {
    AdColorPicker picker;
    picker.resize(300, 44);
    picker.show();
    QTRY_VERIFY_WITH_TIMEOUT(picker.isVisible(), 400);
    picker.setOpen(true);
    QTRY_VERIFY_WITH_TIMEOUT(picker.open(), 400);

    AdSelect* formatCombo =
        picker.findChild<AdSelect*>(QStringLiteral("ad-color-picker-format-select"),
                                    Qt::FindChildrenRecursively);
    QVERIFY(formatCombo != nullptr);
    const QVector<AdSelect::Option> formatOptions = formatCombo->options();
    QCOMPARE(formatOptions.size(), 3);
    QCOMPARE(formatOptions.at(0).value, QStringLiteral("hex"));
    QCOMPARE(formatOptions.at(1).value, QStringLiteral("hsb"));
    QCOMPARE(formatOptions.at(2).value, QStringLiteral("rgb"));

    QCOMPARE(formatCombo->variant(), AdSelect::Variant::Borderless);
    QCOMPARE(formatCombo->size(), AdSelect::Size::Small);
    QCOMPARE(formatCombo->placement(), AdSelect::Placement::BottomRight);
    QVERIFY(!formatCombo->popupMatchSelectWidth());
    QCOMPARE(formatCombo->popupWidth(), 68);

    const AdSelect::ComponentTokens tokens = formatCombo->componentTokens();
    QVERIFY(tokens.horizontalPadding.has_value());
    QCOMPARE(tokens.horizontalPadding.value(), 0);
    QVERIFY(tokens.borderWidth.has_value());
    QCOMPARE(tokens.borderWidth.value(), 0);
    QVERIFY(tokens.iconSize.has_value());
    const auto& map = ThemeManager::instance().currentMapToken();
    const int expectedIconSize = std::max(10, qRound(map.fontSizeSM));
    QCOMPARE(tokens.iconSize.value(), expectedIconSize);

    QVERIFY(formatCombo->width() < 120);
    const QFontMetrics metrics(formatCombo->font());
    const int minExpectedWidth = std::max(metrics.horizontalAdvance(QStringLiteral("HEX")),
                                          metrics.horizontalAdvance(QStringLiteral("HSB"))) +
                                 expectedIconSize;
    QVERIFY(formatCombo->width() >= minExpectedWidth);
    const int formatWidthBeforeOpen = formatCombo->width();
    const int formatHeightBeforeOpen = formatCombo->height();

    QWidget* formatRowWidget =
        picker.findChild<QWidget*>(QStringLiteral("ad-color-picker-format-row"),
                                   Qt::FindChildrenRecursively);
    QVERIFY(formatRowWidget != nullptr);
    AdInput* hexInput =
        formatRowWidget->findChild<AdInput*>(QStringLiteral("ad-color-picker-hex-input"),
                                             Qt::FindChildrenRecursively);
    QVERIFY(hexInput != nullptr);
    const QList<AdInputNumber*> numericInputs =
        formatRowWidget->findChildren<AdInputNumber*>(QString(), Qt::FindChildrenRecursively);
    QVERIFY(numericInputs.size() >= 7);

    AdInputNumber* rgbInputR =
        formatRowWidget->findChild<AdInputNumber*>(QStringLiteral("ad-color-picker-rgb-r-input"),
                                                   Qt::FindChildrenRecursively);
    AdInputNumber* rgbInputG =
        formatRowWidget->findChild<AdInputNumber*>(QStringLiteral("ad-color-picker-rgb-g-input"),
                                                   Qt::FindChildrenRecursively);
    AdInputNumber* rgbInputB =
        formatRowWidget->findChild<AdInputNumber*>(QStringLiteral("ad-color-picker-rgb-b-input"),
                                                   Qt::FindChildrenRecursively);
    AdInputNumber* hsbInputH =
        formatRowWidget->findChild<AdInputNumber*>(QStringLiteral("ad-color-picker-hsb-h-input"),
                                                   Qt::FindChildrenRecursively);
    AdInputNumber* hsbInputS =
        formatRowWidget->findChild<AdInputNumber*>(QStringLiteral("ad-color-picker-hsb-s-input"),
                                                   Qt::FindChildrenRecursively);
    AdInputNumber* hsbInputB =
        formatRowWidget->findChild<AdInputNumber*>(QStringLiteral("ad-color-picker-hsb-b-input"),
                                                   Qt::FindChildrenRecursively);
    AdInputNumber* alphaInput =
        formatRowWidget->findChild<AdInputNumber*>(QStringLiteral("ad-color-picker-alpha-input"),
                                                   Qt::FindChildrenRecursively);
    QVERIFY(rgbInputR != nullptr);
    QVERIFY(rgbInputG != nullptr);
    QVERIFY(rgbInputB != nullptr);
    QVERIFY(hsbInputH != nullptr);
    QVERIFY(hsbInputS != nullptr);
    QVERIFY(hsbInputB != nullptr);
    QVERIFY(alphaInput != nullptr);

    for (AdInputNumber* input : numericInputs) {
      if (!input) {
        continue;
      }
      QCOMPARE(input->size(), AdInputNumber::Size::Small);
    }
    QVERIFY(hexInput->isVisible());
    QVERIFY(alphaInput->isVisible());
    QVERIFY(!rgbInputR->isVisible());
    QVERIFY(!rgbInputG->isVisible());
    QVERIFY(!rgbInputB->isVisible());
    QVERIFY(!hsbInputH->isVisible());
    QVERIFY(!hsbInputS->isVisible());
    QVERIFY(!hsbInputB->isVisible());

    picker.setFormat(AdColorPicker::Format::Rgb);
    QCoreApplication::processEvents();
    QVERIFY(!hexInput->isVisible());
    QVERIFY(rgbInputR->isVisible());
    QVERIFY(rgbInputG->isVisible());
    QVERIFY(rgbInputB->isVisible());
    QVERIFY(!hsbInputH->isVisible());
    QVERIFY(!hsbInputS->isVisible());
    QVERIFY(!hsbInputB->isVisible());

    picker.setFormat(AdColorPicker::Format::Hsb);
    QCoreApplication::processEvents();
    QVERIFY(!hexInput->isVisible());
    QVERIFY(!rgbInputR->isVisible());
    QVERIFY(!rgbInputG->isVisible());
    QVERIFY(!rgbInputB->isVisible());
    QVERIFY(hsbInputH->isVisible());
    QVERIFY(hsbInputS->isVisible());
    QVERIFY(hsbInputB->isVisible());

    formatCombo->setOpen(true);
    QTRY_VERIFY_WITH_TIMEOUT(formatCombo->open(), 400);
    QCOMPARE(formatCombo->width(), formatWidthBeforeOpen);
    QCOMPARE(formatCombo->height(), formatHeightBeforeOpen);

    QWidget* scopeWindow = formatCombo->window();
    QVERIFY(scopeWindow != nullptr);
    QWidget* popup = scopeWindow->findChild<QWidget*>(QStringLiteral("adselect-popup"),
                                                      Qt::FindChildrenRecursively);
    QTRY_VERIFY_WITH_TIMEOUT(popup != nullptr, 400);
    QTRY_VERIFY_WITH_TIMEOUT(popup->isVisible(), 400);
    QCOMPARE(popup->width(), 68);
    const QRect comboRectGlobal =
        QRect(formatCombo->mapToGlobal(QPoint(0, 0)), formatCombo->rect().size());
    const QRect popupRectGlobal =
        QRect(popup->mapToGlobal(QPoint(0, 0)), popup->size());
    const int expectedPopupOffset = std::max(2, qRound(map.sizeXXS));
    QVERIFY(qAbs(popupRectGlobal.left() -
                 (comboRectGlobal.right() - popupRectGlobal.width())) <= 1);
    QVERIFY(qAbs(popupRectGlobal.top() -
                 (comboRectGlobal.bottom() + expectedPopupOffset)) <= 1);
    QVERIFY(picker.open());

    formatCombo->setOpen(false);
    QTRY_VERIFY_WITH_TIMEOUT(!formatCombo->open(), 400);
    QVERIFY(picker.open());

    sendMouseClick(formatCombo);
    QTRY_VERIFY_WITH_TIMEOUT(formatCombo->open(), 400);
    QVERIFY(picker.open());
    QCOMPARE(formatCombo->width(), formatWidthBeforeOpen);
    QCOMPARE(formatCombo->height(), formatHeightBeforeOpen);

    formatCombo->setOpen(false);
    QTRY_VERIFY_WITH_TIMEOUT(!formatCombo->open(), 400);
    QCOMPARE(formatCombo->width(), formatWidthBeforeOpen);
    QCOMPARE(formatCombo->height(), formatHeightBeforeOpen);

    picker.setSize(AdColorPicker::Size::Large);
    QCoreApplication::sendPostedEvents(nullptr, 0);
    QCoreApplication::processEvents();
    QCOMPARE(formatCombo->size(), AdSelect::Size::Small);
    QCOMPARE(hexInput->size(), AdInput::Size::Small);
    for (AdInputNumber* input : numericInputs) {
      if (input) {
        QCOMPARE(input->size(), AdInputNumber::Size::Small);
      }
    }
    QCOMPARE(alphaInput->size(), AdInputNumber::Size::Small);
  }

  void colorPicker_formatInputsApplyNumericConstraintsLikeAntd() {
    AdColorPicker picker;
    picker.resize(300, 44);
    picker.setValue(QStringLiteral("rgba(22, 119, 255, 0.5)"));
    picker.show();
    QTRY_VERIFY_WITH_TIMEOUT(picker.isVisible(), 400);
    picker.setOpen(true);
    QTRY_VERIFY_WITH_TIMEOUT(picker.open(), 400);

    QWidget* formatRowWidget =
        picker.findChild<QWidget*>(QStringLiteral("ad-color-picker-format-row"),
                                   Qt::FindChildrenRecursively);
    QVERIFY(formatRowWidget != nullptr);
    const QList<AdInputNumber*> numericInputs =
        formatRowWidget->findChildren<AdInputNumber*>(QString(), Qt::FindChildrenRecursively);
    QVERIFY(numericInputs.size() >= 7);

    AdInputNumber* rgbInputR =
        formatRowWidget->findChild<AdInputNumber*>(QStringLiteral("ad-color-picker-rgb-r-input"),
                                                   Qt::FindChildrenRecursively);
    AdInputNumber* rgbInputG =
        formatRowWidget->findChild<AdInputNumber*>(QStringLiteral("ad-color-picker-rgb-g-input"),
                                                   Qt::FindChildrenRecursively);
    AdInputNumber* rgbInputB =
        formatRowWidget->findChild<AdInputNumber*>(QStringLiteral("ad-color-picker-rgb-b-input"),
                                                   Qt::FindChildrenRecursively);
    AdInputNumber* hsbInputH =
        formatRowWidget->findChild<AdInputNumber*>(QStringLiteral("ad-color-picker-hsb-h-input"),
                                                   Qt::FindChildrenRecursively);
    AdInputNumber* hsbInputS =
        formatRowWidget->findChild<AdInputNumber*>(QStringLiteral("ad-color-picker-hsb-s-input"),
                                                   Qt::FindChildrenRecursively);
    AdInputNumber* hsbInputB =
        formatRowWidget->findChild<AdInputNumber*>(QStringLiteral("ad-color-picker-hsb-b-input"),
                                                   Qt::FindChildrenRecursively);
    AdInputNumber* alphaInput =
        formatRowWidget->findChild<AdInputNumber*>(QStringLiteral("ad-color-picker-alpha-input"),
                                                   Qt::FindChildrenRecursively);
    QVERIFY(rgbInputR != nullptr);
    QVERIFY(rgbInputG != nullptr);
    QVERIFY(rgbInputB != nullptr);
    QVERIFY(hsbInputH != nullptr);
    QVERIFY(hsbInputS != nullptr);
    QVERIFY(hsbInputB != nullptr);
    QVERIFY(alphaInput != nullptr);

    picker.setFormat(AdColorPicker::Format::Rgb);
    QCoreApplication::processEvents();
    QVERIFY(rgbInputR->isVisible());
    QVERIFY(rgbInputG->isVisible());
    QVERIFY(rgbInputB->isVisible());
    QCOMPARE(rgbInputR->value().toInt(), 22);
    QCOMPARE(rgbInputG->value().toInt(), 119);
    QCOMPARE(rgbInputB->value().toInt(), 255);

    const QColor colorBeforeInvalidRgb = parseThemeColor(picker.value(), QColor());
    QVERIFY(colorBeforeInvalidRgb.isValid());
    rgbInputR->setValue(999);
    QVERIFY(rgbInputR->lineEdit() != nullptr);
    QMetaObject::invokeMethod(rgbInputR->lineEdit(), "editingFinished", Qt::DirectConnection);
    QCOMPARE(rgbInputR->value().toInt(), 255);
    const QColor colorAfterInvalidRgb = parseThemeColor(picker.value(), QColor());
    QVERIFY(colorAfterInvalidRgb.isValid());
    QCOMPARE(colorAfterInvalidRgb.red(), 255);
    QCOMPARE(colorAfterInvalidRgb.green(), 119);
    QCOMPARE(colorAfterInvalidRgb.blue(), 255);

    rgbInputR->setValue(255);
    QMetaObject::invokeMethod(rgbInputR->lineEdit(), "editingFinished", Qt::DirectConnection);
    const QColor colorAfterRgb = parseThemeColor(picker.value(), QColor());
    QVERIFY(colorAfterRgb.isValid());
    QCOMPARE(colorAfterRgb.red(), 255);
    QCOMPARE(colorAfterRgb.green(), 119);
    QCOMPARE(colorAfterRgb.blue(), 255);

    picker.setFormat(AdColorPicker::Format::Hsb);
    QCoreApplication::processEvents();
    QVERIFY(hsbInputH->isVisible());
    QVERIFY(hsbInputS->isVisible());
    QVERIFY(hsbInputB->isVisible());

    hsbInputS->setValue(101);
    QVERIFY(hsbInputS->lineEdit() != nullptr);
    QMetaObject::invokeMethod(hsbInputS->lineEdit(), "editingFinished", Qt::DirectConnection);
    QVERIFY(hsbInputS->value().toInt() <= 100);

    hsbInputH->setValue(120);
    hsbInputS->setValue(100);
    hsbInputB->setValue(50);
    QMetaObject::invokeMethod(hsbInputB->lineEdit(), "editingFinished", Qt::DirectConnection);
    const QColor colorAfterHsb = parseThemeColor(picker.value(), QColor());
    QVERIFY(colorAfterHsb.isValid());
    QCOMPARE(colorAfterHsb.red(), 0);
    QCOMPARE(colorAfterHsb.green(), 128);
    QCOMPARE(colorAfterHsb.blue(), 0);

    alphaInput->setValue(101);
    QVERIFY(alphaInput->lineEdit() != nullptr);
    QMetaObject::invokeMethod(alphaInput->lineEdit(), "editingFinished", Qt::DirectConnection);
    QCOMPARE(alphaInput->value().toInt(), 100);
  }

  void colorPicker_modeSegmentedKeepsCompactHeightAfterClick() {
    AdColorPicker picker;
    picker.setModeOptions({AdColorPicker::Mode::Single, AdColorPicker::Mode::Gradient});
    picker.resize(300, 44);
    picker.show();
    QTRY_VERIFY_WITH_TIMEOUT(picker.isVisible(), 400);
    picker.setOpen(true);
    QTRY_VERIFY_WITH_TIMEOUT(picker.open(), 400);

    QWidget* modeSegmented =
        picker.findChild<QWidget*>(QStringLiteral("ad-color-picker-mode-segmented"),
                                   Qt::FindChildrenRecursively);
    QVERIFY(modeSegmented != nullptr);
    QTRY_VERIFY_WITH_TIMEOUT(modeSegmented->isVisible(), 400);

    QPushButton* singleButton = nullptr;
    QPushButton* gradientButton = nullptr;
    const QList<QPushButton*> buttons =
        modeSegmented->findChildren<QPushButton*>(QString(), Qt::FindDirectChildrenOnly);
    for (QPushButton* button : buttons) {
      if (!button) {
        continue;
      }
      const QString modeValue = button->property("ad-color-picker-mode-value").toString();
      if (modeValue == QStringLiteral("single")) {
        singleButton = button;
      } else if (modeValue == QStringLiteral("gradient")) {
        gradientButton = button;
      }
    }
    QVERIFY(singleButton != nullptr);
    QVERIFY(gradientButton != nullptr);

    const int beforeHeight = modeSegmented->height();
    QVERIFY(beforeHeight > 0);

    sendMouseClick(gradientButton);
    QTRY_COMPARE_WITH_TIMEOUT(picker.mode(), AdColorPicker::Mode::Gradient, 400);
    QCOMPARE(modeSegmented->height(), beforeHeight);
    singleButton = nullptr;
    for (QPushButton* button :
         modeSegmented->findChildren<QPushButton*>(QString(), Qt::FindDirectChildrenOnly)) {
      if (button && button->property("ad-color-picker-mode-value").toString() == QStringLiteral("single")) {
        singleButton = button;
        break;
      }
    }
    QVERIFY(singleButton != nullptr);
    sendMouseClick(singleButton);
    QTRY_COMPARE_WITH_TIMEOUT(picker.mode(), AdColorPicker::Mode::Single, 400);
    QCOMPARE(modeSegmented->height(), beforeHeight);
  }

  void colorPicker_sliderHandleFillMatchesAntdColorModel() {
    AdColorPicker picker;
    picker.resize(300, 44);
    picker.show();
    QTRY_VERIFY_WITH_TIMEOUT(picker.isVisible(), 400);

    picker.setOpen(true);
    QTRY_VERIFY_WITH_TIMEOUT(picker.open(), 400);
    picker.setValue(QStringLiteral("rgba(255, 0, 0, 0.5)"));
    QCoreApplication::sendPostedEvents(nullptr, 0);
    QCoreApplication::processEvents();

    AdSlider* hueSlider = nullptr;
    AdSlider* alphaSlider = nullptr;
    const QList<AdSlider*> sliders =
        picker.findChildren<AdSlider*>(QString(), Qt::FindChildrenRecursively);
    for (AdSlider* slider : sliders) {
      if (!slider) {
        continue;
      }
      if (qRound(slider->minimum()) == 0 && qRound(slider->maximum()) == 359) {
        hueSlider = slider;
      } else if (qRound(slider->minimum()) == 0 && qRound(slider->maximum()) == 100) {
        alphaSlider = slider;
      }
    }

    QVERIFY(hueSlider != nullptr);
    QVERIFY(alphaSlider != nullptr);

    const AdSlider::SemanticStyles hueStyles = hueSlider->semanticStyles();
    QVERIFY(hueStyles.handle.backgroundColor.has_value());
    QCOMPARE(hueStyles.handle.backgroundColor.value(), QColor::fromHsv(0, 255, 255));

    const AdSlider::SemanticStyles alphaStyles = alphaSlider->semanticStyles();
    QVERIFY(alphaStyles.handle.backgroundColor.has_value());
    QCOMPARE(alphaStyles.handle.backgroundColor.value(), QColor(255, 0, 0));
  }

  void colorPicker_transparentSwatchesRenderCheckerboardForTriggerAndPreview() {
    AdColorPicker picker;
    picker.resize(300, 44);
    picker.show();
    QTRY_VERIFY_WITH_TIMEOUT(picker.isVisible(), 400);

    picker.setOpen(true);
    QTRY_VERIFY_WITH_TIMEOUT(picker.open(), 400);
    picker.setValue(QStringLiteral("rgba(255, 0, 0, 0.5)"));
    QCoreApplication::sendPostedEvents(nullptr, 0);
    QCoreApplication::processEvents();

    auto* triggerSwatch = picker.findChild<QWidget*>(QStringLiteral("ad-color-picker-trigger-swatch"),
                                                     Qt::FindChildrenRecursively);
    auto* previewSwatch = picker.findChild<QWidget*>(QStringLiteral("ad-color-picker-preview-swatch"),
                                                     Qt::FindChildrenRecursively);
    QVERIFY(triggerSwatch != nullptr);
    QVERIFY(previewSwatch != nullptr);

    auto checkerContrast = [](const QImage& image) {
      if (image.isNull()) {
        return -1;
      }
      const QRect interior = image.rect().adjusted(3, 3, -4, -4);
      if (interior.width() <= 1 || interior.height() <= 1) {
        return -1;
      }

      int minLuma = 255;
      int maxLuma = 0;
      for (int y = interior.top(); y <= interior.bottom(); ++y) {
        for (int x = interior.left(); x <= interior.right(); ++x) {
          const QColor pixel = QColor::fromRgba(image.pixel(x, y));
          const int luma = qGray(pixel.rgb());
          minLuma = std::min(minLuma, luma);
          maxLuma = std::max(maxLuma, luma);
        }
      }
      return maxLuma - minLuma;
    };

    const QImage triggerImage =
        triggerSwatch->grab().toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
    const QImage previewImage =
        previewSwatch->grab().toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
    QVERIFY(!triggerImage.isNull());
    QVERIFY(!previewImage.isNull());

    const int triggerDiff = checkerContrast(triggerImage);
    const int previewDiff = checkerContrast(previewImage);
    QVERIFY2(triggerDiff > 0,
             qPrintable(QStringLiteral("trigger swatch checkerboard contrast missing, diff=%1")
                            .arg(triggerDiff)));
    QVERIFY2(previewDiff > 0,
             qPrintable(QStringLiteral("preview swatch checkerboard contrast missing, diff=%1")
                            .arg(previewDiff)));
  }

  void colorPicker_sliderActiveHandleRing_notClippedVertically() {
    auto rowContainsHandleColor = [](const QImage& image, int y) {
      if (image.isNull() || y < 0 || y >= image.height()) {
        return false;
      }

      auto isHandlePixel = [](const QColor& pixel) {
        return pixel.alpha() > 32 && pixel.red() > 170 && pixel.green() < 120 && pixel.blue() < 120;
      };

      for (int x = 0; x < image.width(); ++x) {
        if (isHandlePixel(QColor::fromRgba(image.pixel(x, y)))) {
          return true;
        }
      }
      return false;
    };
    auto columnContainsHandleColor = [](const QImage& image, int x) {
      if (image.isNull() || x < 0 || x >= image.width()) {
        return false;
      }

      auto isHandlePixel = [](const QColor& pixel) {
        return pixel.alpha() > 32 && pixel.red() > 170 && pixel.green() < 120 && pixel.blue() < 120;
      };

      for (int y = 0; y < image.height(); ++y) {
        if (isHandlePixel(QColor::fromRgba(image.pixel(x, y)))) {
          return true;
        }
      }
      return false;
    };

    AdColorPicker picker;
    picker.resize(300, 44);
    picker.show();
    QTRY_VERIFY_WITH_TIMEOUT(picker.isVisible(), 400);

    picker.setOpen(true);
    QTRY_VERIFY_WITH_TIMEOUT(picker.open(), 400);
    QCoreApplication::sendPostedEvents(nullptr, 0);
    QCoreApplication::processEvents();

    AdSlider* hueSlider = nullptr;
    const QList<AdSlider*> sliders =
        picker.findChildren<AdSlider*>(QString(), Qt::FindChildrenRecursively);
    for (AdSlider* slider : sliders) {
      if (!slider) {
        continue;
      }
      if (qRound(slider->minimum()) == 0 && qRound(slider->maximum()) == 359) {
        hueSlider = slider;
        break;
      }
    }
    QVERIFY(hueSlider != nullptr);

    AdSlider::ComponentTokens tokens = hueSlider->componentTokens();
    tokens.handleColor = QStringLiteral("#ff4d4f");
    tokens.handleActiveColor = QStringLiteral("#ff4d4f");
    tokens.handleActiveOutlineColor = QStringLiteral("rgba(0,0,0,0)");
    tokens.handleShadowColor = QStringLiteral("rgba(0,0,0,0)");
    tokens.handleActiveShadowColor = QStringLiteral("rgba(0,0,0,0)");
    tokens.handleColorDisabled = QStringLiteral("#ff4d4f");
    tokens.railBg = QStringLiteral("rgba(0,0,0,0)");
    tokens.railHoverBg = QStringLiteral("rgba(0,0,0,0)");
    tokens.trackBg = QStringLiteral("rgba(0,0,0,0)");
    tokens.trackHoverBg = QStringLiteral("rgba(0,0,0,0)");
    tokens.trackBgDisabled = QStringLiteral("rgba(0,0,0,0)");
    hueSlider->setComponentTokens(tokens);

    hueSlider->setValue((hueSlider->minimum() + hueSlider->maximum()) / 2.0);
    hueSlider->dragging_ = true;
    hueSlider->dragHandleIndex_ = 0;
    hueSlider->update();
    QCoreApplication::sendPostedEvents(nullptr, 0);
    QCoreApplication::processEvents();

    const QImage image = hueSlider->grab().toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
    QVERIFY(!image.isNull());

    const bool topEdgeHasHandle = rowContainsHandleColor(image, 0);
    const bool bottomEdgeHasHandle = rowContainsHandleColor(image, image.height() - 1);
    QVERIFY2(!topEdgeHasHandle, "hue slider active handle ring clipped at top edge");
    QVERIFY2(!bottomEdgeHasHandle, "hue slider active handle ring clipped at bottom edge");

    hueSlider->setValue(hueSlider->minimum());
    hueSlider->update();
    QCoreApplication::sendPostedEvents(nullptr, 0);
    QCoreApplication::processEvents();
    const QImage minImage =
        hueSlider->grab().toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
    QVERIFY(!minImage.isNull());
    const bool leftEdgeHasHandle = columnContainsHandleColor(minImage, 0);
    QVERIFY2(!leftEdgeHasHandle, "hue slider active handle ring clipped at left edge");

    hueSlider->setValue(hueSlider->maximum());
    hueSlider->update();
    QCoreApplication::sendPostedEvents(nullptr, 0);
    QCoreApplication::processEvents();
    const QImage maxImage =
        hueSlider->grab().toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
    QVERIFY(!maxImage.isNull());
    const bool rightEdgeHasHandle = columnContainsHandleColor(maxImage, maxImage.width() - 1);
    QVERIFY2(!rightEdgeHasHandle, "hue slider active handle ring clipped at right edge");

    hueSlider->dragging_ = false;
    hueSlider->dragHandleIndex_ = -1;
  }

  void colorPicker_triggerMetricsAndHoverBorderMatchAntdTokenModel() {
    AdColorPicker picker;
    picker.resize(300, 44);
    picker.show();
    QTRY_VERIFY_WITH_TIMEOUT(picker.isVisible(), 400);

    auto* triggerFrame =
        picker.findChild<QFrame*>(QStringLiteral("ad-color-picker-trigger-frame"),
                                  Qt::FindChildrenRecursively);
    auto* triggerSwatch =
        picker.findChild<QWidget*>(QStringLiteral("ad-color-picker-trigger-swatch"),
                                   Qt::FindChildrenRecursively);
    auto* triggerText =
        picker.findChild<QLabel*>(QStringLiteral("ad-color-picker-trigger-text"),
                                  Qt::FindChildrenRecursively);
    QVERIFY(triggerFrame != nullptr);
    QVERIFY(triggerSwatch != nullptr);
    QVERIFY(triggerText != nullptr);

    const auto map = ThemeManager::instance().currentMapToken();
    const int expectedPadding = std::max(0, qRound(map.sizeXXS - map.lineWidth));
    const int expectedGap = std::max(0, qRound(map.sizeXS));
    const int expectedTextEnd = std::max(0, qRound(map.sizeXS) - expectedPadding);
    const int expectedTrailingWithText = expectedPadding + expectedTextEnd;

    auto verifyForSize = [&](AdColorPicker::Size size, int expectedHeight, int expectedSwatch,
                             int expectedSwatchRadius) {
      picker.setSize(size);
      QCoreApplication::sendPostedEvents(nullptr, 0);
      QCoreApplication::processEvents();

      QCOMPARE(triggerFrame->minimumHeight(), expectedHeight);
      QCOMPARE(triggerFrame->maximumHeight(), expectedHeight);
      QCOMPARE(triggerFrame->minimumWidth(), expectedHeight);
      QCOMPARE(triggerSwatch->size(), QSize(expectedSwatch, expectedSwatch));
      const QVariant swatchRadiusProperty =
          triggerSwatch->property("ad-color-picker-swatch-radius");
      if (swatchRadiusProperty.isValid()) {
        QCOMPARE(swatchRadiusProperty.toInt(), expectedSwatchRadius);
      } else {
        QCOMPARE(extractBorderRadiusPx(triggerSwatch->styleSheet()), expectedSwatchRadius);
      }
    };

    const int middleHeight = std::max(24, qRound(map.controlHeight));
    const int middleSwatch = std::max(10, qRound(map.controlHeightSM));
    const int expectedLeadingWithText =
        std::max(expectedPadding, std::max(0, (middleHeight - middleSwatch) / 2));
    const int middleSwatchRadius = std::max(0, qRound(map.borderRadiusSM));
    const int smallHeight = std::max(20, qRound(map.controlHeightSM));
    const int smallSwatch = std::max(8, qRound(map.controlHeightXS));
    const int smallSwatchRadius = std::max(0, qRound(map.borderRadiusXS));
    const int largeHeight = std::max(28, qRound(map.controlHeightLG));
    const int largeSwatch = std::max(12, qRound(map.controlHeight));
    const int largeSwatchRadius = std::max(0, qRound(map.borderRadius));

    verifyForSize(AdColorPicker::Size::Middle, middleHeight, middleSwatch, middleSwatchRadius);

    auto* triggerLayout = qobject_cast<QHBoxLayout*>(triggerFrame->layout());
    QVERIFY(triggerLayout != nullptr);
    QCOMPARE(triggerLayout->alignment(), Qt::AlignHCenter | Qt::AlignVCenter);
    QVERIFY(triggerLayout->count() >= 2);
    QVERIFY(triggerLayout->itemAt(0) != nullptr);
    QVERIFY(triggerLayout->itemAt(1) != nullptr);
    QCOMPARE(triggerLayout->itemAt(0)->alignment(), Qt::AlignVCenter);
    QCOMPARE(triggerLayout->itemAt(1)->alignment(), Qt::AlignVCenter);
    QCOMPARE(triggerLayout->spacing(), 0);
    QCOMPARE(triggerLayout->contentsMargins().left(), expectedPadding);
    QCOMPARE(triggerLayout->contentsMargins().top(), expectedPadding);
    QCOMPARE(triggerLayout->contentsMargins().right(), expectedPadding);
    QCOMPARE(triggerLayout->contentsMargins().bottom(), expectedPadding);
    const int swatchLeftInsetWithoutText = triggerSwatch->x();

    picker.setShowText(true);
    QCoreApplication::sendPostedEvents(nullptr, 0);
    QCoreApplication::processEvents();
    QCOMPARE(triggerLayout->alignment(), Qt::AlignLeft | Qt::AlignVCenter);
    QCOMPARE(triggerLayout->spacing(), expectedGap);
    QCOMPARE(triggerLayout->contentsMargins().left(), expectedLeadingWithText);
    QCOMPARE(triggerLayout->contentsMargins().top(), expectedPadding);
    QCOMPARE(triggerLayout->contentsMargins().right(), expectedTrailingWithText);
    QCOMPARE(triggerLayout->contentsMargins().bottom(), expectedPadding);
    QCOMPARE(triggerText->alignment(), Qt::AlignLeft | Qt::AlignVCenter);
    QCOMPARE(triggerSwatch->x(), swatchLeftInsetWithoutText);

    picker.setFormat(AdColorPicker::Format::Rgb);
    picker.setValue(QStringLiteral("#010203"));
    QCoreApplication::sendPostedEvents(nullptr, 0);
    QCoreApplication::processEvents();
    const int swatchLeftInsetReference = triggerSwatch->x();
    picker.setValue(QStringLiteral("#ffffff"));
    QCoreApplication::sendPostedEvents(nullptr, 0);
    QCoreApplication::processEvents();
    QCOMPARE(triggerSwatch->x(), swatchLeftInsetReference);

    verifyForSize(AdColorPicker::Size::Small, smallHeight, smallSwatch, smallSwatchRadius);
    const int expectedSmallLineHeight = std::max(0, qRound(map.controlHeightXS));
    QCOMPARE(triggerText->minimumHeight(), expectedSmallLineHeight);
    QCOMPARE(triggerText->maximumHeight(), expectedSmallLineHeight);

    verifyForSize(AdColorPicker::Size::Large, largeHeight, largeSwatch, largeSwatchRadius);
    const int expectedLargeFontSize = std::max(12, qRound(map.fontSizeLG));
    QCOMPARE(triggerText->font().pixelSize(), expectedLargeFontSize);

    picker.setSize(AdColorPicker::Size::Middle);
    picker.setShowText(false);
    QCoreApplication::sendPostedEvents(nullptr, 0);
    QCoreApplication::processEvents();

    AdColorPicker::ComponentTokens customTokens;
    customTokens.triggerRadius = 10;
    picker.setComponentTokens(customTokens);
    const RadiusValues customRadii = deriveRadiusValuesForBase(10);
    verifyForSize(AdColorPicker::Size::Middle, middleHeight, middleSwatch, customRadii.borderRadiusSM);
    verifyForSize(AdColorPicker::Size::Small, smallHeight, smallSwatch, customRadii.borderRadiusXS);
    verifyForSize(AdColorPicker::Size::Large, largeHeight, largeSwatch, customRadii.borderRadius);

    picker.resetComponentTokens();
    verifyForSize(AdColorPicker::Size::Middle, middleHeight, middleSwatch, middleSwatchRadius);

    const QString borderColor = parseThemeColor(map.colorBorder, QColor("#d9d9d9")).name(QColor::HexArgb);
    const QString hoverColor =
        parseThemeColor(map.colorPrimaryHover, QColor("#4096ff")).name(QColor::HexArgb);
    auto triggerBorderMatches = [triggerFrame](const QString& expectedColor) {
      const QString borderProperty =
          triggerFrame->property("ad-color-picker-border-color").toString();
      if (!borderProperty.isEmpty()) {
        return borderProperty.compare(expectedColor, Qt::CaseInsensitive) == 0;
      }
      return triggerFrame->styleSheet().contains(expectedColor, Qt::CaseInsensitive);
    };

    QEvent leaveEvent(QEvent::Leave);
    QCoreApplication::sendEvent(triggerFrame, &leaveEvent);
    QCoreApplication::sendPostedEvents(nullptr, 0);
    QCoreApplication::processEvents();
    QVERIFY(triggerBorderMatches(borderColor));

    QEvent enterEvent(QEvent::Enter);
    QCoreApplication::sendEvent(triggerFrame, &enterEvent);
    QCoreApplication::sendPostedEvents(nullptr, 0);
    QCoreApplication::processEvents();
    QVERIFY(triggerBorderMatches(hoverColor));

    QEvent leaveEventAgain(QEvent::Leave);
    QCoreApplication::sendEvent(triggerFrame, &leaveEventAgain);
    QCoreApplication::sendPostedEvents(nullptr, 0);
    QCoreApplication::processEvents();
    QVERIFY(triggerBorderMatches(borderColor));
  }

  void colorPicker_showTextPopupDragging_relayoutCostMatchesNoTextBaseline() {
    AdColorPicker picker;
    picker.resize(320, 44);
    picker.setFormat(AdColorPicker::Format::Rgb);
    picker.setValue(QStringLiteral("#1677ff"));
    picker.show();

    QTRY_VERIFY_WITH_TIMEOUT(picker.isVisible(), 400);

    picker.setOpen(true);
    QTRY_VERIFY_WITH_TIMEOUT(picker.open(), 400);
    QCoreApplication::sendPostedEvents(nullptr, 0);
    QCoreApplication::processEvents();

    struct DragProfile {
      QVector<qint64> saturationMoveSamplesUs;
      QVector<qint64> hueMoveSamplesUs;
      qint64 syncCalls = 0;
      qint64 shortCircuits = 0;
      QString textBefore;
      QString textAfter;
    };

    const auto buildHorizontalSweep = [](QWidget* widget, int steps, int inset) {
      QVector<QPoint> points;
      if (!widget || steps <= 0) {
        return points;
      }

      const int left = std::clamp(inset, 0, std::max(0, widget->width() - 1));
      const int right = std::clamp(widget->width() - inset - 1, left, std::max(0, widget->width() - 1));
      const int y = std::clamp(widget->height() / 2, 1, std::max(1, widget->height() - 2));

      points.reserve(steps);
      for (int i = 0; i < steps; ++i) {
        const double t = steps == 1 ? 0.0 : static_cast<double>(i) / static_cast<double>(steps - 1);
        const int x = left + static_cast<int>(std::round(t * static_cast<double>(right - left)));
        points.append(QPoint(x, y));
      }
      return points;
    };

    const auto buildSaturationZigzag = [](QWidget* widget, int stepsPerRow, int inset) {
      QVector<QPoint> points;
      if (!widget || stepsPerRow <= 1) {
        return points;
      }

      const int left = std::clamp(inset, 0, std::max(0, widget->width() - 1));
      const int right = std::clamp(widget->width() - inset - 1, left, std::max(0, widget->width() - 1));
      const int top = std::clamp(inset, 0, std::max(0, widget->height() - 1));
      const int bottom = std::clamp(widget->height() - inset - 1, top, std::max(0, widget->height() - 1));
      const int verticalSpan = std::max(1, bottom - top);

      const QVector<double> rowFactors = {0.15, 0.85, 0.35, 0.65};
      points.reserve(rowFactors.size() * stepsPerRow + 1);
      for (int row = 0; row < rowFactors.size(); ++row) {
        const int y = top + static_cast<int>(std::round(rowFactors.at(row) * static_cast<double>(verticalSpan)));
        const bool forward = (row % 2) == 0;
        for (int i = 0; i < stepsPerRow; ++i) {
          const double t = stepsPerRow == 1 ? 0.0 : static_cast<double>(i) / static_cast<double>(stepsPerRow - 1);
          const int sweep = left + static_cast<int>(std::round(t * static_cast<double>(right - left)));
          points.append(QPoint(forward ? sweep : (left + right - sweep), y));
        }
      }
      points.append(QPoint(right, bottom));
      return points;
    };

    constexpr qint64 kRapidMoveBudgetUs = 4500;

    const auto collectDragProfile = [&](bool showText, DragProfile* profileOut) {
      QVERIFY(profileOut != nullptr);
      DragProfile& profile = *profileOut;
      profile = DragProfile{};

      picker.setOpen(false);
      QTRY_VERIFY_WITH_TIMEOUT(!picker.open(), 400);

      picker.setShowText(showText);
      picker.setValue(QStringLiteral("#1677ff"));
      picker.setOpen(true);
      QTRY_VERIFY_WITH_TIMEOUT(picker.open(), 400);
      QTest::qWait(200);
      QCoreApplication::sendPostedEvents(nullptr, 0);
      QCoreApplication::processEvents();

      AdSlider* hueSlider = nullptr;
      const QList<AdSlider*> sliders =
          picker.findChildren<AdSlider*>(QString(), Qt::FindChildrenRecursively);
      for (AdSlider* slider : sliders) {
        if (!slider) {
          continue;
        }
        if (qRound(slider->minimum()) == 0 && qRound(slider->maximum()) >= 300) {
          hueSlider = slider;
          break;
        }
      }
      QVERIFY(hueSlider != nullptr);

      QWidget* saturationPanel =
          picker.findChild<QWidget*>(QStringLiteral("ad-color-picker-saturation-panel"),
                                     Qt::FindChildrenRecursively);
      QVERIFY(saturationPanel != nullptr);

      QTRY_VERIFY_WITH_TIMEOUT(hueSlider->isVisible(), 500);
      QTRY_VERIFY_WITH_TIMEOUT(hueSlider->width() > 24, 500);
      QTRY_VERIFY_WITH_TIMEOUT(hueSlider->height() > 8, 500);
      QTRY_VERIFY_WITH_TIMEOUT(saturationPanel->isVisible(), 500);
      QTRY_VERIFY_WITH_TIMEOUT(saturationPanel->width() > 24, 500);
      QTRY_VERIFY_WITH_TIMEOUT(saturationPanel->height() > 24, 500);

      QLabel* triggerText = nullptr;
      if (showText) {
        triggerText = picker.findChild<QLabel*>(
            QStringLiteral("ad-color-picker-trigger-text"), Qt::FindChildrenRecursively);
        QVERIFY(triggerText != nullptr);
        profile.textBefore = triggerText->text();
      }

      const QVector<QPoint> hueSweep = buildHorizontalSweep(hueSlider, 40, 4);
      const QVector<QPoint> saturationSweep = buildSaturationZigzag(saturationPanel, 22, 5);
      QVERIFY(!hueSweep.isEmpty());
      QVERIFY(!saturationSweep.isEmpty());

      auto runDragBurst = [&](QWidget* target,
                              const QVector<QPoint>& path,
                              int rounds,
                              bool collectSamples) {
        QVector<qint64> samplesUs;
        if (!target || path.isEmpty() || rounds <= 0) {
          return samplesUs;
        }
        if (collectSamples) {
          samplesUs.reserve(rounds * path.size());
        }
        for (int round = 0; round < rounds; ++round) {
          sendMousePress(target, Qt::LeftButton, Qt::NoModifier, path.constFirst());
          for (const QPoint& point : path) {
            QElapsedTimer timer;
            timer.start();
            sendMouseMoveWithButtons(target, point, Qt::LeftButton, Qt::NoModifier);
            QCoreApplication::sendPostedEvents(nullptr, 0);
            QCoreApplication::processEvents();
            const qint64 elapsedUs = timer.nsecsElapsed() / 1000;
            if (collectSamples) {
              samplesUs.append(elapsedUs);
            }
          }
          sendMouseRelease(target, Qt::LeftButton, Qt::NoModifier, path.constLast());
        }
        QCoreApplication::sendPostedEvents(nullptr, 0);
        QCoreApplication::processEvents();
        return samplesUs;
      };

      (void)runDragBurst(saturationPanel, saturationSweep, 2, false);
      (void)runDragBurst(hueSlider, hueSweep, 2, false);
      AdPopover::resetSyncPopupGeometryCountersForTesting();
      profile.saturationMoveSamplesUs = runDragBurst(saturationPanel, saturationSweep, 6, true);
      profile.hueMoveSamplesUs = runDragBurst(hueSlider, hueSweep, 8, true);
      profile.syncCalls = AdPopover::syncPopupGeometryCallCountForTesting();
      profile.shortCircuits = AdPopover::syncPopupGeometryShortCircuitCountForTesting();
      if (triggerText) {
        profile.textAfter = triggerText->text();
      }
    };

    DragProfile noTextProfile;
    collectDragProfile(false, &noTextProfile);
    DragProfile withTextProfile;
    collectDragProfile(true, &withTextProfile);

    const auto mergeSamples = [](const DragProfile& profile) {
      QVector<qint64> merged;
      merged.reserve(profile.saturationMoveSamplesUs.size() + profile.hueMoveSamplesUs.size());
      merged += profile.saturationMoveSamplesUs;
      merged += profile.hueMoveSamplesUs;
      return merged;
    };
    const auto averageUs = [](const QVector<qint64>& samples) -> double {
      if (samples.isEmpty()) {
        return 0.0;
      }
      const qint64 sum = std::accumulate(samples.cbegin(), samples.cend(), static_cast<qint64>(0));
      return static_cast<double>(sum) / static_cast<double>(samples.size());
    };
    const auto p95Us = [](QVector<qint64> samples) -> qint64 {
      if (samples.isEmpty()) {
        return 0;
      }
      std::sort(samples.begin(), samples.end());
      const int index = std::clamp(static_cast<int>(std::ceil(samples.size() * 0.95)) - 1,
                                   0,
                                   static_cast<int>(samples.size()) - 1);
      return samples.at(index);
    };
    const auto overBudgetRateUs = [](const QVector<qint64>& samples, qint64 budgetUs) -> double {
      if (samples.isEmpty()) {
        return 0.0;
      }
      const qint64 overBudgetMoves =
          static_cast<qint64>(std::count_if(samples.cbegin(), samples.cend(),
                                            [budgetUs](qint64 sample) { return sample > budgetUs; }));
      return static_cast<double>(overBudgetMoves) / static_cast<double>(samples.size());
    };

    const QVector<qint64> noTextSamples = mergeSamples(noTextProfile);
    const QVector<qint64> withTextSamples = mergeSamples(withTextProfile);
    QVERIFY(!noTextSamples.isEmpty());
    QVERIFY(!withTextSamples.isEmpty());

    const double noTextAvgUs = averageUs(noTextSamples);
    const double withTextAvgUs = averageUs(withTextSamples);
    const qint64 noTextP95Us = p95Us(noTextSamples);
    const qint64 withTextP95Us = p95Us(withTextSamples);

    QVERIFY2(!withTextProfile.textBefore.trimmed().isEmpty(),
             qPrintable(QStringLiteral("showText should provide non-empty trigger text before drag, text='%1'")
                            .arg(withTextProfile.textBefore)));
    QVERIFY2(withTextProfile.textAfter != withTextProfile.textBefore,
             qPrintable(QStringLiteral("showText trigger text should update during drag, before='%1' after='%2'")
                            .arg(withTextProfile.textBefore)
                            .arg(withTextProfile.textAfter)));

    const double avgRatio = noTextAvgUs <= 0.0 ? withTextAvgUs : withTextAvgUs / noTextAvgUs;
    const double p95Ratio = noTextP95Us <= 0
                                ? static_cast<double>(withTextP95Us)
                                : static_cast<double>(withTextP95Us) / static_cast<double>(noTextP95Us);
    const qint64 adaptiveMoveBudgetUs =
        std::max<qint64>(kRapidMoveBudgetUs, qRound(static_cast<double>(noTextP95Us) * 1.15));
    const double noTextOverBudgetRate = overBudgetRateUs(noTextSamples, adaptiveMoveBudgetUs);
    const double withTextOverBudgetRate = overBudgetRateUs(withTextSamples, adaptiveMoveBudgetUs);
    const qint64 syncBudget = noTextProfile.syncCalls + std::max<qint64>(12, noTextProfile.syncCalls / 3);
    const double overBudgetDelta = withTextOverBudgetRate - noTextOverBudgetRate;

    qInfo().noquote()
        << QStringLiteral("color picker rapid drag latency us: "
                          "noText avg=%1 p95=%2 overBudgetRate=%3 sync=%4 short=%5, "
                          "showText avg=%6 p95=%7 overBudgetRate=%8 sync=%9 short=%10, "
                          "overBudgetUs=%11")
               .arg(noTextAvgUs, 0, 'f', 1)
               .arg(noTextP95Us)
               .arg(noTextOverBudgetRate, 0, 'f', 3)
               .arg(noTextProfile.syncCalls)
               .arg(noTextProfile.shortCircuits)
               .arg(withTextAvgUs, 0, 'f', 1)
               .arg(withTextP95Us)
               .arg(withTextOverBudgetRate, 0, 'f', 3)
               .arg(withTextProfile.syncCalls)
               .arg(withTextProfile.shortCircuits)
               .arg(adaptiveMoveBudgetUs);

    QVERIFY2(avgRatio <= 1.45,
             qPrintable(QStringLiteral("showText drag avg latency too high, ratio=%1 "
                                       "noTextAvgUs=%2 withTextAvgUs=%3")
                            .arg(avgRatio, 0, 'f', 3)
                            .arg(noTextAvgUs, 0, 'f', 1)
                            .arg(withTextAvgUs, 0, 'f', 1)));
    QVERIFY2(p95Ratio <= 1.60,
             qPrintable(QStringLiteral("showText drag p95 latency too high, ratio=%1 "
                                       "noTextP95Us=%2 withTextP95Us=%3")
                            .arg(p95Ratio, 0, 'f', 3)
                            .arg(noTextP95Us)
                            .arg(withTextP95Us)));
    QVERIFY2(overBudgetDelta <= 0.25,
             qPrintable(QStringLiteral("showText drag over-budget rate too high for budget %1us, "
                                       "delta=%2 noTextRate=%3 withTextRate=%4")
                            .arg(adaptiveMoveBudgetUs)
                            .arg(overBudgetDelta, 0, 'f', 3)
                            .arg(noTextOverBudgetRate, 0, 'f', 3)
                            .arg(withTextOverBudgetRate, 0, 'f', 3)));
    QVERIFY2(withTextProfile.syncCalls <= syncBudget,
             qPrintable(QStringLiteral("showText drag relayout calls too high, "
                                       "noTextCalls=%1 noTextShort=%2 withTextCalls=%3 withTextShort=%4 budget=%5")
                            .arg(noTextProfile.syncCalls)
                            .arg(noTextProfile.shortCircuits)
                            .arg(withTextProfile.syncCalls)
                            .arg(withTextProfile.shortCircuits)
                            .arg(syncBudget)));
  }

  void colorPicker_popupBorderColorUsesCssOrderAndMatchesThemeToken() {
    AdColorPicker picker;
    picker.resize(300, 44);
    picker.show();
    QTRY_VERIFY_WITH_TIMEOUT(picker.isVisible(), 400);

    picker.setOpen(true);
    QTRY_VERIFY_WITH_TIMEOUT(picker.open(), 400);

    auto* popover = picker.findChild<AdPopover*>(QString(), Qt::FindChildrenRecursively);
    QVERIFY(popover != nullptr);

    const auto map = ThemeManager::instance().currentMapToken();
    const QColor expectedPopupBg = parseThemeColor(map.colorBgElevated, QColor("#ffffff"));
    const QColor expectedPopupBorder = parseThemeColor(map.colorBorderSecondary, QColor("#f0f0f0"));

    auto verifyPopupTokens = [&]() {
      const AdPopover::ComponentTokens tokens = popover->componentTokens();
      QVERIFY(tokens.popupBg.has_value());
      QVERIFY(tokens.popupBorderColor.has_value());

      const QColor popupBg = parseThemeColor(tokens.popupBg.value(), QColor());
      const QColor popupBorder = parseThemeColor(tokens.popupBorderColor.value(), QColor());
      QVERIFY(popupBg.isValid());
      QVERIFY(popupBorder.isValid());
      QCOMPARE(popupBg, expectedPopupBg);
      QCOMPARE(popupBorder, expectedPopupBorder);
    };

    verifyPopupTokens();

    picker.setValue(QStringLiteral("#ff4d4f"));
    QCoreApplication::sendPostedEvents(nullptr, 0);
    QCoreApplication::processEvents();
    verifyPopupTokens();

    picker.setValue(QStringLiteral("rgba(82, 196, 26, 0.6)"));
    QCoreApplication::sendPostedEvents(nullptr, 0);
    QCoreApplication::processEvents();
    verifyPopupTokens();
  }

  void colorPicker_panelWidthMatchesAntdPopupOverlayInnerModel() {
    AdColorPicker picker;
    picker.resize(320, 44);
    AdColorPicker::ComponentTokens tokens;
    tokens.panelWidth = 480;
    picker.setComponentTokens(tokens);
    picker.show();
    QTRY_VERIFY_WITH_TIMEOUT(picker.isVisible(), 400);

    picker.setOpen(true);
    QTRY_VERIFY_WITH_TIMEOUT(picker.open(), 400);

    QWidget* scopeWindow = picker.window();
    QVERIFY(scopeWindow != nullptr);

    QWidget* popup =
        scopeWindow->findChild<QWidget*>(QStringLiteral("adpopover-popup"),
                                         Qt::FindChildrenRecursively);
    QTRY_VERIFY_WITH_TIMEOUT(popup != nullptr && popup->isVisible(), 400);
    QCOMPARE(popup->width(), 480);

    auto* popover = picker.findChild<AdPopover*>(QString(), Qt::FindChildrenRecursively);
    QVERIFY(popover != nullptr);
    QWidget* panelHost =
        picker.findChild<QWidget*>(QStringLiteral("ad-color-picker-panel-host"),
                                   Qt::FindChildrenRecursively);
    QVERIFY(panelHost != nullptr);

    const int popupPadding = std::max(0, popover->componentTokens().popupPadding.value_or(0));
    QCOMPARE(panelHost->width(), std::max(1, 480 - popupPadding * 2));
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

  void input_clickingVisualShellRegionsFocusesLineEdit() {
    QWidget host;
    host.resize(380, 120);
    auto* layout = new QVBoxLayout(&host);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto* input = new AdInput(&host);
    input->setFixedWidth(320);
    input->setFixedHeight(64);
    input->setPrefixText(QStringLiteral("https://"));
    input->setSuffixText(QStringLiteral(".com"));
    input->setValue(QStringLiteral("example"));

    auto* blurTarget = new QPushButton(QStringLiteral("blur"), &host);
    blurTarget->setFixedWidth(120);

    layout->addWidget(input);
    layout->addWidget(blurTarget);
    layout->addStretch();

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(input->isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(blurTarget->isVisible(), 400);

    QLineEdit* lineEdit = input->lineEdit();
    QVERIFY(lineEdit != nullptr);
    QWidget* shell = lineEdit->parentWidget();
    QVERIFY(shell != nullptr);
    QCOMPARE(input->cursor().shape(), Qt::ArrowCursor);
    QCOMPARE(shell->cursor().shape(), Qt::ArrowCursor);
    QCOMPARE(lineEdit->cursor().shape(), Qt::IBeamCursor);

    blurTarget->setFocus(Qt::OtherFocusReason);
    QTRY_VERIFY_WITH_TIMEOUT(!lineEdit->hasFocus(), 400);

    sendMouseClick(input, Qt::LeftButton, Qt::NoModifier, QPoint(input->width() / 2, 1));
    QTRY_VERIFY_WITH_TIMEOUT(lineEdit->hasFocus(), 400);

    blurTarget->setFocus(Qt::OtherFocusReason);
    QTRY_VERIFY_WITH_TIMEOUT(!lineEdit->hasFocus(), 400);
    sendMouseClick(input, Qt::LeftButton, Qt::NoModifier,
                   QPoint(input->width() / 2, std::max(1, input->height() - 2)));
    QTRY_VERIFY_WITH_TIMEOUT(lineEdit->hasFocus(), 400);

    sendMouseClick(shell, Qt::LeftButton, Qt::NoModifier, QPoint(1, shell->height() / 2));
    QTRY_VERIFY_WITH_TIMEOUT(lineEdit->hasFocus(), 400);
    QCOMPARE(lineEdit->cursorPosition(), 0);

    blurTarget->setFocus(Qt::OtherFocusReason);
    QTRY_VERIFY_WITH_TIMEOUT(!lineEdit->hasFocus(), 400);
    sendMouseClick(shell, Qt::LeftButton, Qt::NoModifier, QPoint(shell->width() / 2, 1));
    QTRY_VERIFY_WITH_TIMEOUT(lineEdit->hasFocus(), 400);

    blurTarget->setFocus(Qt::OtherFocusReason);
    QTRY_VERIFY_WITH_TIMEOUT(!lineEdit->hasFocus(), 400);
    sendMouseClick(shell, Qt::LeftButton, Qt::NoModifier,
                   QPoint(shell->width() / 2, std::max(1, shell->height() - 2)));
    QTRY_VERIFY_WITH_TIMEOUT(lineEdit->hasFocus(), 400);

    QLabel* prefix = nullptr;
    QLabel* suffix = nullptr;
    const QList<QLabel*> labels = input->findChildren<QLabel*>();
    for (QLabel* label : labels) {
      if (!label || !label->isVisible()) {
        continue;
      }
      if (label->text() == QStringLiteral("https://")) {
        prefix = label;
      } else if (label->text() == QStringLiteral(".com")) {
        suffix = label;
      }
    }
    QVERIFY(prefix != nullptr);
    QVERIFY(suffix != nullptr);

    blurTarget->setFocus(Qt::OtherFocusReason);
    QTRY_VERIFY_WITH_TIMEOUT(!lineEdit->hasFocus(), 400);
    sendMouseClick(prefix);
    QTRY_VERIFY_WITH_TIMEOUT(lineEdit->hasFocus(), 400);
    QCOMPARE(lineEdit->cursorPosition(), 0);

    blurTarget->setFocus(Qt::OtherFocusReason);
    QTRY_VERIFY_WITH_TIMEOUT(!lineEdit->hasFocus(), 400);
    sendMouseClick(suffix);
    QTRY_VERIFY_WITH_TIMEOUT(lineEdit->hasFocus(), 400);
    QCOMPARE(lineEdit->cursorPosition(), lineEdit->text().size());
  }

  void input_cursorStylesFollowAntdRoles() {
    QWidget host;
    host.resize(360, 120);
    auto* layout = new QVBoxLayout(&host);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto* input = new AdInput(&host);
    input->setFixedWidth(300);
    input->setAllowClear(true);
    input->setValue(QStringLiteral("abc"));
    layout->addWidget(input);

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(input->isVisible(), 400);

    QLineEdit* lineEdit = input->lineEdit();
    QVERIFY(lineEdit != nullptr);
    QWidget* shell = lineEdit->parentWidget();
    QVERIFY(shell != nullptr);

    QToolButton* clear = nullptr;
    const QList<QToolButton*> toolButtons = input->findChildren<QToolButton*>();
    for (QToolButton* button : toolButtons) {
      if (button && button->isVisible()) {
        clear = button;
        break;
      }
    }
    QVERIFY(clear != nullptr);

    QCOMPARE(input->cursor().shape(), Qt::ArrowCursor);
    QCOMPARE(shell->cursor().shape(), Qt::ArrowCursor);
    QCOMPARE(lineEdit->cursor().shape(), Qt::IBeamCursor);
    QCOMPARE(clear->cursor().shape(), Qt::PointingHandCursor);

    input->setDisabled(true);
    QCoreApplication::processEvents();

    QCOMPARE(input->cursor().shape(), Qt::ForbiddenCursor);
    QCOMPARE(shell->cursor().shape(), Qt::ForbiddenCursor);
    QCOMPARE(lineEdit->cursor().shape(), Qt::ForbiddenCursor);
  }

  void input_maxLengthCountFollowsShowCount() {
    AdInput input;
    input.resize(320, 40);
    input.setMaxLength(6);
    input.show();
    QTRY_VERIFY_WITH_TIMEOUT(input.isVisible(), 400);

    const QList<QLabel*> labels = input.findChildren<QLabel*>();
    bool hasVisibleCount = false;
    QStringList visibleLabelTexts;
    for (QLabel* label : labels) {
      if (!label || !label->isVisible()) {
        continue;
      }
      visibleLabelTexts.append(label->text());
      if (label->text() == QStringLiteral("0 / 6")) {
        hasVisibleCount = true;
      }
    }
    QVERIFY2(!hasVisibleCount,
             qPrintable(QStringLiteral("unexpected visible count before showCount=true, visible labels: [%1]")
                            .arg(visibleLabelTexts.join(QStringLiteral(", ")))));

    input.setShowCount(true);
    QCoreApplication::processEvents();

    QLabel* count = nullptr;
    for (QLabel* label : labels) {
      if (label && label->isVisible() && label->text() == QStringLiteral("0 / 6")) {
        count = label;
        break;
      }
    }
    QVERIFY(count != nullptr);
    QVERIFY(input.lineEdit() != nullptr);
    QVERIFY(input.lineEdit()->parentWidget() != nullptr);

    const QRect shellRectInGlobal(input.lineEdit()->parentWidget()->mapToGlobal(QPoint(0, 0)),
                                  input.lineEdit()->parentWidget()->rect().size());
    const QRect countRectInGlobal(count->mapToGlobal(QPoint(0, 0)), count->size());
    QVERIFY2(shellRectInGlobal.contains(countRectInGlobal.center()),
             qPrintable(QStringLiteral("count label center is outside input shell: shell=%1,%2 %3x%4 count=%5,%6 %7x%8")
                            .arg(shellRectInGlobal.x())
                            .arg(shellRectInGlobal.y())
                            .arg(shellRectInGlobal.width())
                            .arg(shellRectInGlobal.height())
                            .arg(countRectInGlobal.x())
                            .arg(countRectInGlobal.y())
                            .arg(countRectInGlobal.width())
                            .arg(countRectInGlobal.height())));
  }

  void inputNumber_stepSignalIncludesEmitterAndDirection() {
    qRegisterMetaType<AdInputNumber::StepType>();
    qRegisterMetaType<AdInputNumber::StepEmitter>();

    AdInputNumber input;
    input.resize(220, 40);
    input.setMin(0);
    input.setMax(10);
    input.setValue(3);
    input.show();
    QTRY_VERIFY_WITH_TIMEOUT(input.isVisible(), 400);

    QSignalSpy stepSpy(&input, &AdInputNumber::stepped);
    const QList<QToolButton*> buttons = input.findChildren<QToolButton*>();
    QVERIFY(buttons.size() >= 2);

    QToolButton* topButton = nullptr;
    QToolButton* bottomButton = nullptr;
    for (QToolButton* button : buttons) {
      if (!button || !button->isVisible()) {
        continue;
      }
      if (!topButton || button->mapToGlobal(QPoint(0, 0)).y() < topButton->mapToGlobal(QPoint(0, 0)).y()) {
        topButton = button;
      }
      if (!bottomButton ||
          button->mapToGlobal(QPoint(0, 0)).y() > bottomButton->mapToGlobal(QPoint(0, 0)).y()) {
        bottomButton = button;
      }
    }
    QVERIFY(topButton != nullptr);
    QVERIFY(bottomButton != nullptr);

    sendMouseClick(topButton);
    QTRY_COMPARE_WITH_TIMEOUT(stepSpy.count(), 1, 400);
    QCOMPARE(stepSpy.last().at(1).toInt(), 1);
    QCOMPARE(stepSpy.last().at(2).value<AdInputNumber::StepType>(), AdInputNumber::StepType::Up);
    QCOMPARE(stepSpy.last().at(3).value<AdInputNumber::StepEmitter>(),
             AdInputNumber::StepEmitter::Handler);

    sendMouseClick(bottomButton);
    QTRY_COMPARE_WITH_TIMEOUT(stepSpy.count(), 2, 400);
    QCOMPARE(stepSpy.last().at(1).toInt(), -1);
    QCOMPARE(stepSpy.last().at(2).value<AdInputNumber::StepType>(), AdInputNumber::StepType::Down);
    QCOMPARE(stepSpy.last().at(3).value<AdInputNumber::StepEmitter>(),
             AdInputNumber::StepEmitter::Handler);
  }

  void inputNumber_handlersStayInsideShellBorder() {
    AdInputNumber input;
    input.resize(220, 40);
    input.setMin(0);
    input.setMax(10);
    input.setValue(3);
    input.show();
    QTRY_VERIFY_WITH_TIMEOUT(input.isVisible(), 400);
    QVERIFY(input.lineEdit() != nullptr);

    input.focusInput(AdInputNumber::FocusCursor::End, false);
    QTRY_VERIFY_WITH_TIMEOUT(input.lineEdit()->hasFocus(), 400);
    QCoreApplication::processEvents();

    const QList<QToolButton*> buttons = input.findChildren<QToolButton*>();
    int visibleCount = 0;
    for (QToolButton* button : buttons) {
      if (!button || !button->isVisible()) {
        continue;
      }
      ++visibleCount;
      const QPoint topLeft = button->mapTo(&input, QPoint(0, 0));
      const QRect rect(topLeft, button->size());
      QVERIFY2(rect.top() > 0,
               qPrintable(QStringLiteral("button touches top edge: top=%1 inputHeight=%2")
                              .arg(rect.top())
                              .arg(input.height())));
      QVERIFY2(rect.bottom() < input.height() - 1,
               qPrintable(QStringLiteral("button touches bottom edge: bottom=%1 inputHeight=%2")
                              .arg(rect.bottom())
                              .arg(input.height())));
    }

    QVERIFY2(visibleCount >= 2, qPrintable(QStringLiteral("expected at least 2 visible handlers, got %1").arg(visibleCount)));
  }

  void inputNumber_handlersUseRightCornerRadius() {
    AdInputNumber input;
    input.resize(220, 40);
    input.setMin(0);
    input.setMax(10);
    input.setValue(3);
    input.show();
    QTRY_VERIFY_WITH_TIMEOUT(input.isVisible(), 400);
    QVERIFY(input.lineEdit() != nullptr);

    auto findTopBottomVisibleButtons = [&input]() {
      QToolButton* topButton = nullptr;
      QToolButton* bottomButton = nullptr;
      const QList<QToolButton*> buttons = input.findChildren<QToolButton*>();
      for (QToolButton* button : buttons) {
        if (!button || !button->isVisible()) {
          continue;
        }
        if (!topButton || button->mapToGlobal(QPoint(0, 0)).y() < topButton->mapToGlobal(QPoint(0, 0)).y()) {
          topButton = button;
        }
        if (!bottomButton ||
            button->mapToGlobal(QPoint(0, 0)).y() > bottomButton->mapToGlobal(QPoint(0, 0)).y()) {
          bottomButton = button;
        }
      }
      return qMakePair(topButton, bottomButton);
    };

    auto extractRadius = [](const QString& css, const QString& property) {
      const QRegularExpression re(
          QStringLiteral("%1\\s*:\\s*([0-9]+)px").arg(QRegularExpression::escape(property)),
          QRegularExpression::CaseInsensitiveOption);
      const QRegularExpressionMatch match = re.match(css);
      if (!match.hasMatch()) {
        return -1;
      }
      bool ok = false;
      const int value = match.captured(1).toInt(&ok);
      return ok ? value : -1;
    };

    input.focusInput(AdInputNumber::FocusCursor::End, false);
    QTRY_VERIFY_WITH_TIMEOUT(input.lineEdit()->hasFocus(), 400);
    QCoreApplication::processEvents();

    auto [topButton, bottomButton] = findTopBottomVisibleButtons();
    QVERIFY(topButton != nullptr);
    QVERIFY(bottomButton != nullptr);

    const int topRadius =
        extractRadius(topButton->styleSheet(), QStringLiteral("border-top-right-radius"));
    const int bottomRadius =
        extractRadius(bottomButton->styleSheet(), QStringLiteral("border-bottom-right-radius"));
    QVERIFY2(topRadius > 0,
             qPrintable(QStringLiteral("top handler right corner radius invalid: %1 css=%2")
                            .arg(topRadius)
                            .arg(topButton->styleSheet())));
    QVERIFY2(bottomRadius > 0,
             qPrintable(QStringLiteral("bottom handler right corner radius invalid: %1 css=%2")
                            .arg(bottomRadius)
                            .arg(bottomButton->styleSheet())));

  }

  void inputNumber_keyboardToggleControlsArrowStep() {
    AdInputNumber input;
    input.resize(220, 40);
    input.setMin(1);
    input.setMax(10);
    input.setValue(3);
    input.show();
    QTRY_VERIFY_WITH_TIMEOUT(input.isVisible(), 400);

    QVERIFY(input.lineEdit() != nullptr);
    input.focusInput();
    QTRY_VERIFY_WITH_TIMEOUT(input.lineEdit()->hasFocus(), 400);

    QSignalSpy valueSpy(&input, &AdInputNumber::valueChanged);
    QTest::keyClick(input.lineEdit(), Qt::Key_Up);
    QTRY_VERIFY_WITH_TIMEOUT(valueSpy.count() >= 1, 400);
    QCOMPARE(input.value().toInt(), 4);

    input.setKeyboardEnabled(false);
    const int countBefore = valueSpy.count();
    QTest::keyClick(input.lineEdit(), Qt::Key_Up);
    QCoreApplication::processEvents();
    QCOMPARE(valueSpy.count(), countBefore);
    QCOMPARE(input.value().toInt(), 4);
  }

  void inputNumber_changeOnWheelHonored() {
    AdInputNumber input;
    input.resize(220, 40);
    input.setMin(1);
    input.setMax(10);
    input.setValue(3);
    input.show();
    QTRY_VERIFY_WITH_TIMEOUT(input.isVisible(), 400);
    input.focusInput();

    QSignalSpy stepSpy(&input, &AdInputNumber::stepped);
    sendMouseWheel(&input, QPoint(), 120);
    QCoreApplication::processEvents();
    QCOMPARE(stepSpy.count(), 0);
    QCOMPARE(input.value().toInt(), 3);

    input.setChangeOnWheel(true);
    sendMouseWheel(&input, QPoint(), 120);
    QTRY_COMPARE_WITH_TIMEOUT(stepSpy.count(), 1, 400);
    QCOMPARE(stepSpy.last().at(2).value<AdInputNumber::StepType>(), AdInputNumber::StepType::Up);
    QCOMPARE(stepSpy.last().at(3).value<AdInputNumber::StepEmitter>(),
             AdInputNumber::StepEmitter::Wheel);
    QCOMPARE(input.value().toInt(), 4);
  }

  void inputNumber_blurClampsWhenChangeOnBlurEnabled() {
    AdInputNumber input;
    input.resize(220, 40);
    input.setMin(1);
    input.setMax(10);
    input.setValue(5);
    input.show();
    QTRY_VERIFY_WITH_TIMEOUT(input.isVisible(), 400);
    QVERIFY(input.lineEdit() != nullptr);

    input.focusInput(AdInputNumber::FocusCursor::All);
    QTRY_VERIFY_WITH_TIMEOUT(input.lineEdit()->hasFocus(), 400);
    QTest::keyClicks(input.lineEdit(), QStringLiteral("99"));
    input.blurInput();
    QTRY_COMPARE_WITH_TIMEOUT(input.value().toInt(), 10, 400);

    input.setChangeOnBlur(false);
    input.focusInput(AdInputNumber::FocusCursor::All);
    QTRY_VERIFY_WITH_TIMEOUT(input.lineEdit()->hasFocus(), 400);
    QTest::keyClicks(input.lineEdit(), QStringLiteral("99"));
    input.blurInput();
    QTRY_COMPARE_WITH_TIMEOUT(input.value().toInt(), 99, 400);
  }

  void inputNumber_stringModeEmitsStringValue() {
    AdInputNumber input;
    input.setStringMode(true);

    QSignalSpy valueSpy(&input, &AdInputNumber::valueChanged);
    input.setValue(QStringLiteral("1.2345"));
    QTRY_VERIFY_WITH_TIMEOUT(valueSpy.count() >= 1, 400);
    QCOMPARE(input.value().userType(), QMetaType::QString);
    QCOMPARE(valueSpy.last().at(0).userType(), QMetaType::QString);

    input.setValue(2.5);
    QTRY_VERIFY_WITH_TIMEOUT(valueSpy.count() >= 2, 400);
    QCOMPARE(input.value().userType(), QMetaType::QString);
    QCOMPARE(valueSpy.last().at(0).userType(), QMetaType::QString);
  }

  void inputNumber_formatterAndParserRoundTrip() {
    AdInputNumber input;
    input.resize(220, 40);
    input.show();
    QTRY_VERIFY_WITH_TIMEOUT(input.isVisible(), 400);
    QVERIFY(input.lineEdit() != nullptr);

    input.setFormatter([](const QVariant& value, bool, const QString&) {
      QString text = value.toString();
      if (text.trimmed().isEmpty()) {
        return QString();
      }
      text.replace(QRegularExpression(QStringLiteral("\\B(?=(\\d{3})+(?!\\d))")),
                   QStringLiteral(","));
      return QStringLiteral("$ %1").arg(text);
    });
    input.setParser([](const QString& text) -> QVariant {
      QString clean = text;
      clean.remove(QRegularExpression(QStringLiteral("[\\$,\\s]")));
      return clean;
    });
    input.setValue(1000);
    QTRY_COMPARE_WITH_TIMEOUT(input.lineEdit()->text(), QStringLiteral("$ 1,000"), 400);

    input.focusInput(AdInputNumber::FocusCursor::All);
    QTRY_VERIFY_WITH_TIMEOUT(input.lineEdit()->hasFocus(), 400);
    QTest::keyClicks(input.lineEdit(), QStringLiteral("$ 2,500"));
    input.blurInput();

    QTRY_COMPARE_WITH_TIMEOUT(input.value().toInt(), 2500, 400);
    QTRY_COMPARE_WITH_TIMEOUT(input.lineEdit()->text(), QStringLiteral("$ 2,500"), 400);
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

  void inputSearch_compactJoin_inputAndButtonOverlapByLineWidth() {
    AdInputSearch search;
    search.resize(360, 40);
    search.show();
    QTRY_VERIFY_WITH_TIMEOUT(search.isVisible(), 400);
    QCoreApplication::processEvents();

    AdInput* innerInput = search.input();
    QVERIFY(innerInput != nullptr);

    const QList<AdButton*> innerButtons = search.findChildren<AdButton*>();
    QVERIFY(!innerButtons.isEmpty());
    AdButton* innerButton = innerButtons.constFirst();
    QVERIFY(innerButton != nullptr);

    const int overlap = std::max(1, qRound(ThemeManager::instance().currentMapToken().lineWidth));
    const int joinDelta = innerButton->x() - (innerInput->x() + innerInput->width());
    QCOMPARE(joinDelta, -overlap);
  }

  void inputSearch_compactJoin_hoveredInputRaisedAtSharedSeam() {
    const int overlap = std::max(1, qRound(ThemeManager::instance().currentMapToken().lineWidth));

    AdInputSearch search;
    search.resize(360, 40);
    search.show();
    QTRY_VERIFY_WITH_TIMEOUT(search.isVisible(), 400);
    QCoreApplication::processEvents();

    AdInput* innerInput = search.input();
    QVERIFY(innerInput != nullptr);
    // Match the third Search box demo where Input is joined on both sides.
    innerInput->setJoinedLeft(true);

    const QList<AdButton*> innerButtons = search.findChildren<AdButton*>();
    QVERIFY(!innerButtons.isEmpty());
    AdButton* innerButton = nullptr;
    for (AdButton* candidate : innerButtons) {
      if (candidate && candidate->isVisible()) {
        innerButton = candidate;
        break;
      }
    }
    if (!innerButton) {
      innerButton = innerButtons.constFirst();
    }
    QVERIFY(innerButton != nullptr);

    QCoreApplication::processEvents();
    QCOMPARE(innerButton->x() - (innerInput->x() + innerInput->width()), -overlap);

    QEvent enterEvent(QEvent::Enter);
    QCoreApplication::sendEvent(innerInput, &enterEvent);
    QCoreApplication::processEvents();

    const int seamX = innerButton->x() + std::max(0, overlap / 2);
    const int seamY = search.height() / 2;
    QWidget* top = search.childAt(QPoint(seamX, seamY));
    QVERIFY2(top != nullptr, "failed to resolve top widget at search compact seam");

    const bool topIsInput = top == innerInput || innerInput->isAncestorOf(top);
    const bool topIsButton = top == innerButton || innerButton->isAncestorOf(top);
    const QString owner =
        topIsInput ? QStringLiteral("input")
                   : (topIsButton ? QStringLiteral("button") : QStringLiteral("other"));
    QVERIFY2(topIsInput,
             qPrintable(QStringLiteral("hovered input did not win seam z-order, owner=%1 top=%2 seam=(%3,%4) inputGeom=%5,%6 %7x%8 buttonGeom=%9,%10 %11x%12")
                            .arg(owner, QString::fromLatin1(top->metaObject()->className()))
                            .arg(seamX)
                            .arg(seamY)
                            .arg(innerInput->x())
                            .arg(innerInput->y())
                            .arg(innerInput->width())
                            .arg(innerInput->height())
                            .arg(innerButton->x())
                            .arg(innerButton->y())
                            .arg(innerButton->width())
                            .arg(innerButton->height())));
  }

  void input_compactJoin_noBackgroundSeamBetweenInputAndButton() {
    const int overlap = std::max(1, qRound(ThemeManager::instance().currentMapToken().lineWidth));
    const QColor hostBg(QStringLiteral("#ff00ff"));

    struct SeamStats {
      int bgLeakPixels = 0;
      int brightEdgeRows = 0;
      int joinDelta = 0;
      int topDelta = 0;
      int bottomDelta = 0;
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
      stats.topDelta = button->y() - input->y();
      stats.bottomDelta = (button->y() + button->height()) - (input->y() + input->height());

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
    QVERIFY2(compact.topDelta == 0 && compact.bottomDelta == 0,
             qPrintable(QStringLiteral("compact join top/bottom delta top=%1 bottom=%2")
                            .arg(compact.topDelta)
                            .arg(compact.bottomDelta)));
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

  void input_compactJoin_focusedInputRaisedAtSharedSeam() {
    const int overlap = std::max(1, qRound(ThemeManager::instance().currentMapToken().lineWidth));

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
    rowLayout->addSpacing(-overlap);
    rowLayout->addWidget(right);
    rowLayout->addStretch();

    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(host.isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(left->isVisible(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(right->isVisible(), 400);
    QCOMPARE(right->x() - (left->x() + left->width()), -overlap);

    const int seamX = right->x() + std::max(0, overlap / 2);
    const int seamY = row->height() / 2;

    left->focusInput(AdInput::FocusCursor::End);
    QTRY_VERIFY_WITH_TIMEOUT(left->lineEdit() && left->lineEdit()->hasFocus(), 400);
    QCoreApplication::processEvents();

    QWidget* top = row->childAt(QPoint(seamX, seamY));
    QVERIFY2(top != nullptr, "failed to resolve top widget at compact join seam");
    const bool topIsLeft = top == left || left->isAncestorOf(top);
    const bool topIsRight = top == right || right->isAncestorOf(top);
    const QString owner =
        topIsLeft ? QStringLiteral("left")
                  : (topIsRight ? QStringLiteral("right") : QStringLiteral("other"));
    QVERIFY2(topIsLeft,
             qPrintable(QStringLiteral("focused input did not win seam z-order, owner=%1 top=%2 seam=(%3,%4) leftGeom=%5,%6 %7x%8 rightGeom=%9,%10 %11x%12")
                            .arg(owner, QString::fromLatin1(top->metaObject()->className()))
                            .arg(seamX)
                            .arg(seamY)
                            .arg(left->x())
                            .arg(left->y())
                            .arg(left->width())
                            .arg(left->height())
                            .arg(right->x())
                            .arg(right->y())
                            .arg(right->width())
                            .arg(right->height())));
  }

  void inputPassword_toggleVisibility() {
    AdInputPassword password;
    password.resize(320, 40);
    password.input()->setSuffixIconToken(adqt::icons::outlined::Lock());
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
    QVERIFY2(password.input()->isAncestorOf(toggle), "password toggle icon should be rendered inside AdInput");
    const QRect inputRectInGlobal(password.input()->mapToGlobal(QPoint(0, 0)), password.input()->rect().size());
    const QRect toggleRectInGlobal(toggle->mapToGlobal(QPoint(0, 0)), toggle->size());
    QVERIFY2(inputRectInGlobal.contains(toggleRectInGlobal.center()),
             qPrintable(QStringLiteral("toggle icon center is outside input bounds: input=%1,%2 %3x%4 toggle=%5,%6 %7x%8")
                            .arg(inputRectInGlobal.x())
                            .arg(inputRectInGlobal.y())
                            .arg(inputRectInGlobal.width())
                            .arg(inputRectInGlobal.height())
                            .arg(toggleRectInGlobal.x())
                            .arg(toggleRectInGlobal.y())
                            .arg(toggleRectInGlobal.width())
                            .arg(toggleRectInGlobal.height())));
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

  void inputTextArea_verticalScrollOverlayDoesNotRelayoutViewport() {
    AdInputTextArea textArea;
    textArea.resize(360, 120);
    textArea.setAutoSizeEnabled(true);
    textArea.setAutoSizeMinRows(2);
    textArea.setAutoSizeMaxRows(2);
    textArea.show();
    QTRY_VERIFY_WITH_TIMEOUT(textArea.isVisible(), 400);

    QVERIFY(textArea.textEdit() != nullptr);
    QTextEdit* edit = textArea.textEdit();
    QVERIFY(edit != nullptr);
    QWidget* viewport = edit->viewport();
    QVERIFY(viewport != nullptr);
    QScrollBar* sourceBar = edit->verticalScrollBar();
    QVERIFY(sourceBar != nullptr);
    QCOMPARE(edit->verticalScrollBarPolicy(), Qt::ScrollBarAlwaysOff);

    const int viewportWidthBefore = viewport->width();
    textArea.setValue(QStringLiteral("line1\nline2\nline3\nline4\nline5\nline6\nline7\nline8"));
    QTRY_VERIFY_WITH_TIMEOUT(sourceBar->maximum() > sourceBar->minimum(), 400);
    const int viewportWidthAfter = viewport->width();
    QCOMPARE(viewportWidthAfter, viewportWidthBefore);

    QScrollBar* overlayBar = viewport->findChild<QScrollBar*>(
        QStringLiteral("adinput-textarea-overlay-vbar"), Qt::FindDirectChildrenOnly);
    QVERIFY(overlayBar != nullptr);
    QTRY_VERIFY_WITH_TIMEOUT(overlayBar->isVisible(), 400);

    sourceBar->setValue(sourceBar->maximum());
    QCoreApplication::processEvents();
    QCOMPARE(overlayBar->value(), sourceBar->value());
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

  void alert_defaultTypeAndBannerDefaults() {
    AdAlert alert;
    QCOMPARE(alert.type(), AdAlert::Type::Info);
    QVERIFY(!alert.showIcon());

    alert.setBanner(true);
    QCOMPARE(alert.type(), AdAlert::Type::Warning);
    QVERIFY(alert.showIcon());

    alert.setType(AdAlert::Type::Error);
    QCOMPARE(alert.type(), AdAlert::Type::Error);
  }

  void alert_closeFlow_emitsSignalsAndAfterClose() {
    AdAlert alert;
    alert.setTitleText(QStringLiteral("Closable alert"));
    alert.setClosable(true);
    alert.resize(360, 56);
    alert.show();
    QTRY_VERIFY_WITH_TIMEOUT(alert.isVisible(), 400);

    QSignalSpy closeSpy(&alert, &AdAlert::closeRequested);
    QSignalSpy openSpy(&alert, &AdAlert::openChanged);
    QSignalSpy afterCloseSpy(&alert, &AdAlert::afterClose);

    QToolButton* closeButton = alert.findChild<QToolButton*>();
    QVERIFY(closeButton != nullptr);
    QVERIFY(closeButton->isVisible());
    sendMouseClick(closeButton);

    QTRY_COMPARE_WITH_TIMEOUT(closeSpy.count(), 1, 400);
    QTRY_COMPARE_WITH_TIMEOUT(openSpy.count(), 1, 400);
    QCOMPARE(openSpy.first().at(0).toBool(), false);
    QTRY_COMPARE_WITH_TIMEOUT(afterCloseSpy.count(), 1, 2000);
    QVERIFY(!alert.open());
    QVERIFY(!alert.isVisible());
  }

  void alert_reopenAfterClose_restoresLayout() {
    AdAlert alert;
    alert.setTitleText(QStringLiteral("Reopen alert"));
    alert.setDescriptionText(QStringLiteral("description"));
    alert.setClosable(true);
    alert.setShowIcon(true);
    alert.resize(360, 72);
    alert.show();
    QTRY_VERIFY_WITH_TIMEOUT(alert.isVisible(), 400);

    QToolButton* closeButton = alert.findChild<QToolButton*>();
    QVERIFY(closeButton != nullptr);
    sendMouseClick(closeButton);
    QTRY_VERIFY_WITH_TIMEOUT(!alert.open(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(!alert.isVisible(), 2000);

    alert.setOpen(true);
    QTRY_VERIFY_WITH_TIMEOUT(alert.open(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(alert.isVisible(), 400);
    QVERIFY(alert.maximumHeight() == QWIDGETSIZE_MAX);
  }

  void alert_customIconAndActionWidget() {
    AdAlert alert;
    alert.setTitleText(QStringLiteral("Action alert"));
    alert.setShowIcon(true);
    alert.setIconToken(adqt::icons::outlined::Smile());
    auto* action = new AdButton(QStringLiteral("Action"));
    action->setSize(AdButton::Size::Small);
    alert.setActionWidget(action);
    alert.resize(420, 64);
    alert.show();
    QTRY_VERIFY_WITH_TIMEOUT(alert.isVisible(), 400);

    const QList<QLabel*> labels = alert.findChildren<QLabel*>();
    bool hasIconPixmap = false;
    for (QLabel* label : labels) {
      if (!label || !label->isVisible()) {
        continue;
      }
      if (labelHasPixmap(label)) {
        hasIconPixmap = true;
        break;
      }
    }
    QVERIFY(hasIconPixmap);
    QVERIFY(action->isVisible());
    QVERIFY(alert.findChildren<AdButton*>().contains(action));
  }

  void alert_semanticStyleResolver_appliesByType() {
    AdAlert alert;
    alert.setTitleText(QStringLiteral("Semantic alert"));
    alert.setShowIcon(true);
    alert.setSemanticStyleResolver([](const AdAlert::StyleContext& ctx) {
      AdAlert::SemanticStyles styles;
      if (ctx.type == AdAlert::Type::Success) {
        styles.root.backgroundColor = QColor(246, 255, 237);
        styles.root.borderColor = QColor("#b7eb8f");
      } else if (ctx.type == AdAlert::Type::Warning) {
        styles.root.backgroundColor = QColor(255, 251, 230);
        styles.root.borderColor = QColor("#ffe58f");
      }
      return styles;
    });
    alert.setType(AdAlert::Type::Success);
    alert.show();
    QTRY_VERIFY_WITH_TIMEOUT(alert.isVisible(), 400);

    QVERIFY(alert.styleSheet().contains(QStringLiteral("246, 255, 237")));
    alert.setType(AdAlert::Type::Warning);
    QVERIFY(alert.styleSheet().contains(QStringLiteral("255, 251, 230")));
  }

  void alert_componentTokens_affectWithDescriptionMetrics() {
    AdAlert alert;
    alert.setTitleText(QStringLiteral("Token alert"));
    alert.setDescriptionText(QStringLiteral("with description"));
    alert.setShowIcon(true);

    AdAlert::ComponentTokens tokens;
    tokens.withDescriptionIconSize = 30;
    tokens.withDescriptionPadding = 18;
    alert.setComponentTokens(tokens);

    alert.resize(420, 96);
    alert.show();
    QTRY_VERIFY_WITH_TIMEOUT(alert.isVisible(), 400);

    const QMargins margins = alert.layout()->contentsMargins();
    QCOMPARE(margins.left(), 18);
    QCOMPARE(margins.top(), 18);
    QCOMPARE(margins.right(), 18);
    QCOMPARE(margins.bottom(), 18);

    const QList<QLabel*> labels = alert.findChildren<QLabel*>();
    QLabel* iconLabel = nullptr;
    for (QLabel* label : labels) {
      if (!label || !label->isVisible()) {
        continue;
      }
      if (labelHasPixmap(label)) {
        iconLabel = label;
        break;
      }
    }
    QVERIFY(iconLabel != nullptr);
    QCOMPARE(iconLabel->width(), 30);
    QCOMPARE(iconLabel->height(), 30);
  }

  void alert_successType_rendersBackgroundAndBorderPixels() {
    AdAlert alert;
    alert.setType(AdAlert::Type::Success);
    alert.setTitleText(QStringLiteral("Success Text"));
    alert.resize(420, 56);
    alert.show();
    QTRY_VERIFY_WITH_TIMEOUT(alert.isVisible(), 400);
    QCoreApplication::sendPostedEvents(nullptr, 0);
    QCoreApplication::processEvents();

    const auto& map = ThemeManager::instance().currentMapToken();
    const QColor expectedBackground = parseThemeColor(map.colorSuccessBg, QColor("#f6ffed"));
    const QColor expectedBorder = parseThemeColor(map.colorSuccessBorder, QColor("#b7eb8f"));

    const QColor backgroundPixel = sampleWidgetPixel(&alert, QPoint(6, alert.height() / 2));
    QVector<QColor> borderPixels;
    borderPixels.reserve(6);
    for (int x = 0; x <= 5; ++x) {
      borderPixels.push_back(sampleWidgetPixel(&alert, QPoint(x, alert.height() / 2)));
    }

    QVERIFY2(backgroundPixel.alpha() > 0,
             qPrintable(QStringLiteral("Alert background pixel is transparent: %1")
                            .arg(colorToString(backgroundPixel))));
    QVERIFY2(
        colorClose(backgroundPixel, expectedBackground, 20),
        qPrintable(QStringLiteral("Alert background pixel mismatch. expected=%1 actual=%2")
                       .arg(colorToString(expectedBackground))
                       .arg(colorToString(backgroundPixel))));

    bool hasOpaqueBorderPixel = false;
    bool borderMatches = false;
    QStringList borderSamples;
    for (const QColor& color : borderPixels) {
      hasOpaqueBorderPixel = hasOpaqueBorderPixel || color.alpha() > 0;
      borderMatches = borderMatches || colorClose(color, expectedBorder, 24);
      borderSamples << colorToString(color);
    }

    QVERIFY2(hasOpaqueBorderPixel,
             qPrintable(QStringLiteral("Alert border pixels are transparent. samples=%1")
                            .arg(borderSamples.join(QStringLiteral(", ")))));
    QVERIFY2(borderMatches,
             qPrintable(QStringLiteral("Alert border pixel mismatch. expected=%1 samples=%2")
                            .arg(colorToString(expectedBorder))
                            .arg(borderSamples.join(QStringLiteral(", ")))));
  }

 private:
  ThemeConfig originalConfig_;
};

}  // namespace

QTEST_MAIN(TimingRefactorTests)
#include "timing_refactor_tests.moc"
