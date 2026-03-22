#include "overlay_popup_controller.h"

#include "timing_hub.h"

#include <QAbstractScrollArea>
#include <QApplication>
#include <QChildEvent>
#include <QContextMenuEvent>
#include <QCursor>
#include <QEvent>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QLayout>
#include <QMouseEvent>
#include <QScrollBar>
#include <QSet>
#include <QWidget>

#include <algorithm>
#include <atomic>

namespace adqt::widgets::detail {

namespace {

constexpr char kHoverOpenTaskKey[] = "OverlayPopup.HoverOpen";
constexpr char kHoverCloseTaskKey[] = "OverlayPopup.HoverClose";
constexpr char kFocusRecheckTaskKey[] = "OverlayPopup.FocusRecheck";
constexpr char kPopupRelayoutTaskKey[] = "OverlayPopup.Relayout";
constexpr char kScopePopupRelayoutTaskKey[] = "OverlayPopup.ScopeRelayout";
constexpr char kGeometryFrameSyncTaskKey[] = "OverlayPopup.GeometryFrameSync";
constexpr qint64 kGeometryFrameSyncTailMs = 140;
constexpr qint64 kAnchorScrollWatchersRefreshIntervalMs = 280;

std::atomic<qint64> gSyncPopupGeometryCallCount{0};
std::atomic<qint64> gSyncPopupGeometryShortCircuitCount{0};
std::atomic<bool> gSyncPopupGeometryCountersEnabled{false};

inline void recordSyncPopupGeometryCallForTesting() {
  if (!gSyncPopupGeometryCountersEnabled.load(std::memory_order_relaxed)) {
    return;
  }
  gSyncPopupGeometryCallCount.fetch_add(1, std::memory_order_relaxed);
}

inline void recordSyncPopupGeometryShortCircuitForTesting() {
  if (!gSyncPopupGeometryCountersEnabled.load(std::memory_order_relaxed)) {
    return;
  }
  gSyncPopupGeometryShortCircuitCount.fetch_add(1, std::memory_order_relaxed);
}

using PendingRelayoutList = QVector<QPointer<OverlayPopupController>>;

bool pendingRelayoutListContains(const PendingRelayoutList& controllers,
                                 const OverlayPopupController* target) {
  if (!target) {
    return false;
  }
  for (const QPointer<OverlayPopupController>& controller : controllers) {
    if (controller.data() == target) {
      return true;
    }
  }
  return false;
}

void pruneDeadPendingRelayouts(PendingRelayoutList* controllers) {
  if (!controllers) {
    return;
  }
  controllers->erase(
      std::remove_if(controllers->begin(), controllers->end(),
                     [](const QPointer<OverlayPopupController>& controller) { return controller.isNull(); }),
      controllers->end());
}

QHash<QWidget*, PendingRelayoutList>& pendingScopeRelayouts() {
  static QHash<QWidget*, PendingRelayoutList> pending;
  return pending;
}

QHash<QWidget*, QMetaObject::Connection>& scopeRelayoutDestroyedConnections() {
  static QHash<QWidget*, QMetaObject::Connection> connections;
  return connections;
}

void clearScopeRelayoutDestroyedWatcherIfUnused(QWidget* scope) {
  if (!scope || pendingScopeRelayouts().contains(scope)) {
    return;
  }
  auto& connections = scopeRelayoutDestroyedConnections();
  auto it = connections.find(scope);
  if (it == connections.end()) {
    return;
  }
  QObject::disconnect(it.value());
  connections.erase(it);
}

void ensureScopeRelayoutDestroyedWatcher(QWidget* scope) {
  if (!scope) {
    return;
  }
  auto& connections = scopeRelayoutDestroyedConnections();
  if (connections.contains(scope)) {
    return;
  }
  connections.insert(scope, QObject::connect(scope, &QObject::destroyed, [](QObject* destroyed) {
                      QWidget* scopeWidget = qobject_cast<QWidget*>(destroyed);
                      if (!scopeWidget) {
                        return;
                      }
                      pendingScopeRelayouts().remove(scopeWidget);
                      auto& watchers = scopeRelayoutDestroyedConnections();
                      auto it = watchers.find(scopeWidget);
                      if (it == watchers.end()) {
                        return;
                      }
                      QObject::disconnect(it.value());
                      watchers.erase(it);
                    }));
}

void removeControllerFromPendingScopeRelayouts(OverlayPopupController* controller,
                                               QWidget* scopeHint = nullptr) {
  if (!controller) {
    return;
  }
  auto& pending = pendingScopeRelayouts();
  auto removeFromScope = [&](QWidget* scope) -> bool {
    auto it = pending.find(scope);
    if (it == pending.end()) {
      return false;
    }
    pruneDeadPendingRelayouts(&it.value());
    it.value().erase(std::remove_if(it.value().begin(), it.value().end(),
                                    [controller](const QPointer<OverlayPopupController>& queued) {
                                      return queued.data() == controller;
                                    }),
                     it.value().end());
    if (it.value().isEmpty()) {
      pending.erase(it);
      clearScopeRelayoutDestroyedWatcherIfUnused(scope);
    }
    return true;
  };
  if (scopeHint && removeFromScope(scopeHint)) {
    return;
  }
  for (auto it = pending.begin(); it != pending.end();) {
    pruneDeadPendingRelayouts(&it.value());
    it.value().erase(std::remove_if(it.value().begin(), it.value().end(),
                                    [controller](const QPointer<OverlayPopupController>& queued) {
                                      return queued.data() == controller;
                                    }),
                     it.value().end());
    if (it.value().isEmpty()) {
      QWidget* scope = it.key();
      it = pending.erase(it);
      clearScopeRelayoutDestroyedWatcherIfUnused(scope);
      continue;
    }
    ++it;
  }
}

QRect widgetGlobalRect(const QWidget* widget) {
  if (!widget) {
    return QRect();
  }
  return QRect(widget->mapToGlobal(QPoint(0, 0)), widget->size());
}

QRect widgetGlobalRectIfEffectivelyVisible(const QWidget* widget, const QWidget* scopeWindow) {
  if (!widget || !widget->isVisible()) {
    return QRect();
  }

  QRect clippedRect = widgetGlobalRect(widget);
  if (!clippedRect.isValid()) {
    return QRect();
  }

  bool reachedScope = false;
  const QWidget* cursor = widget;
  while (cursor) {
    if (!cursor->isVisible()) {
      return QRect();
    }
    const QRect cursorRect = widgetGlobalRect(cursor);
    if (!cursorRect.isValid()) {
      return QRect();
    }
    clippedRect = clippedRect.intersected(cursorRect);
    if (!clippedRect.isValid()) {
      return QRect();
    }
    if (scopeWindow && cursor == scopeWindow) {
      reachedScope = true;
      break;
    }
    cursor = cursor->parentWidget();
  }

  if (scopeWindow && !reachedScope) {
    const QRect scopeRect = widgetGlobalRect(scopeWindow);
    if (!scopeRect.isValid()) {
      return QRect();
    }
    clippedRect = clippedRect.intersected(scopeRect);
    if (!clippedRect.isValid()) {
      return QRect();
    }
  }

  return clippedRect;
}

bool widgetContainsGlobalPos(const QWidget* widget, const QPoint& globalPos) {
  const QRect rect = widgetGlobalRect(widget);
  return rect.isValid() && rect.contains(globalPos);
}

bool widgetInTree(const QWidget* candidate, const QWidget* root) {
  if (!candidate || !root) {
    return false;
  }
  return candidate == root || root->isAncestorOf(const_cast<QWidget*>(candidate));
}

void applyPopupVisibility(QWidget* popup, bool shouldShow, bool raiseWhenShowing) {
  if (!popup) {
    return;
  }

  if (!shouldShow) {
    if (popup->isVisible()) {
      popup->hide();
    }
    return;
  }

  if (!popup->isVisible()) {
    popup->show();
  }
  if (raiseWhenShowing) {
    popup->raise();
  }
}

bool watchedObjectListContains(const OverlayPopupController::WatchedObjectList& objects,
                               const QWidget* target) {
  if (!target) {
    return false;
  }
  for (const QPointer<QWidget>& object : objects) {
    if (object.data() == target) {
      return true;
    }
  }
  return false;
}

void pruneDeadWatchedObjects(OverlayPopupController::WatchedObjectList* objects) {
  if (!objects) {
    return;
  }

  objects->erase(std::remove_if(objects->begin(), objects->end(),
                                [](const QPointer<QWidget>& object) { return object.isNull(); }),
                 objects->end());
}

void installPopupWatcher(OverlayPopupController::WatchedObjectList* objects, QObject* filter,
                         QObject* object) {
  auto* widget = qobject_cast<QWidget*>(object);
  if (!objects || !filter || !widget) {
    return;
  }

  pruneDeadWatchedObjects(objects);
  if (watchedObjectListContains(*objects, widget)) {
    return;
  }

  widget->installEventFilter(filter);
  objects->append(widget);
}

template <typename Callback>
void traverseObjectTree(QObject* root, Callback&& callback) {
  if (!root) {
    return;
  }

  callback(root);
  const QObjectList children = root->children();
  for (QObject* child : children) {
    traverseObjectTree(child, callback);
  }
}

void preparePopupForGeometrySync(QWidget* popup) {
  if (!popup) {
    return;
  }

  QCoreApplication::sendPostedEvents(nullptr, QEvent::PolishRequest);
  QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);

  traverseObjectTree(popup, [](QObject* object) {
    auto* widget = qobject_cast<QWidget*>(object);
    if (!widget) {
      return;
    }
    widget->ensurePolished();
    QCoreApplication::sendPostedEvents(widget, QEvent::PolishRequest);
    QCoreApplication::sendPostedEvents(widget, QEvent::LayoutRequest);
    if (QLayout* layout = widget->layout()) {
      layout->activate();
    }
    widget->updateGeometry();
  });

  QCoreApplication::sendPostedEvents(nullptr, QEvent::PolishRequest);
  QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
  popup->adjustSize();
}

}  // namespace

OverlayPopupController::OverlayPopupController(OverlayPopupControllerDelegate* delegate, QObject* parent)
    : QObject(parent), delegate_(delegate) {}

OverlayPopupController::~OverlayPopupController() {
  clearHoverTasks();
  cancelPopupRelayout();
  clearFrameSubscription(this, QString::fromLatin1(kGeometryFrameSyncTaskKey));
  clearAnchorScrollBarWatchers();
  delegate_ = nullptr;
  setInWindowPopupHostOpen(this, false);
  clearTriggerWatchers();
  clearPopupWatchers();
}

void OverlayPopupController::resetSyncPopupGeometryCountersForTesting() {
  gSyncPopupGeometryCountersEnabled.store(true, std::memory_order_relaxed);
  gSyncPopupGeometryCallCount.store(0);
  gSyncPopupGeometryShortCircuitCount.store(0);
}

qint64 OverlayPopupController::syncPopupGeometryCallCountForTesting() {
  return gSyncPopupGeometryCallCount.load();
}

qint64 OverlayPopupController::syncPopupGeometryShortCircuitCountForTesting() {
  return gSyncPopupGeometryShortCircuitCount.load();
}

void OverlayPopupController::setTriggerModes(Triggers value) {
  if (triggerModes_ == value) {
    return;
  }
  triggerModes_ = value;

  if (!hasTrigger(Trigger::Hover)) {
    setReasonOpen(InternalOpenReason::Hover, false);
    hoverTriggerActive_ = false;
    hoverPopupActive_ = false;
    clearHoverTasks();
  }
  if (!hasTrigger(Trigger::Focus)) {
    setReasonOpen(InternalOpenReason::Focus, false);
    focusTriggerActive_ = false;
    focusPopupActive_ = false;
  }
  if (!hasTrigger(Trigger::Click)) {
    setReasonOpen(InternalOpenReason::Click, false);
    triggerPressActive_ = false;
    triggerKeyPressActive_ = false;
  }
  if (!hasTrigger(Trigger::ContextMenu)) {
    setReasonOpen(InternalOpenReason::ContextMenu, false);
    contextMenuGlobalPos_.reset();
  }

  updatePopupVisibility(true, VisibilityUpdateSource::InternalState);
}

void OverlayPopupController::setVisibilityMode(VisibilityMode value) {
  if (visibilityMode_ == value) {
    return;
  }

  visibilityMode_ = value;
  if (visibilityMode_ == VisibilityMode::External) {
    clearAllOpenReasons();
    return;
  }

  clearAllOpenReasons();
  if (popupVisible_) {
    setReasonOpen(InternalOpenReason::Programmatic, true);
  }
}

void OverlayPopupController::setPopupVisible(bool value) {
  if (visibilityMode_ == VisibilityMode::External) {
    clearAllOpenReasons();
    setPopupVisibleInternal(value, true);
    return;
  }

  if (value) {
    setReasonOpen(InternalOpenReason::Programmatic, true);
  } else {
    clearAllOpenReasons();
  }
  updatePopupVisibility(true, VisibilityUpdateSource::InternalState);
}

void OverlayPopupController::setDisabled(bool value) {
  if (disabled_ == value) {
    return;
  }
  disabled_ = value;
  if (disabled_) {
    clearAllOpenReasons();
  }
  updatePopupVisibility(true, VisibilityUpdateSource::InternalState);
}

void OverlayPopupController::setMouseEnterDelayMs(int value) { mouseEnterDelayMs_ = std::max(0, value); }

void OverlayPopupController::setMouseLeaveDelayMs(int value) { mouseLeaveDelayMs_ = std::max(0, value); }

void OverlayPopupController::anchorWidgetChanged() {
  clearTriggerWatchers();
  markAnchorScrollWatchersDirty();
  refreshTriggerWatchers();
  if (popupVisible_) {
    schedulePopupRelayout(true);
  }
}

void OverlayPopupController::popupSurfaceChanged() {
  refreshPopupWatchers();
  invalidatePopupGeometry();
  if (!delegate_ || !delegate_->popupSurfaceWidget()) {
    hoverPopupActive_ = false;
    focusPopupActive_ = false;
  }
  if (popupVisible_) {
    schedulePopupRelayout(true);
  }
}

void OverlayPopupController::popupContentChanged(bool emitSignal) {
  invalidatePopupGeometry();
  updatePopupVisibility(emitSignal, VisibilityUpdateSource::InternalState);
}

void OverlayPopupController::refreshVisiblePopup() {
  invalidatePopupGeometry();
  if (!popupVisible_ || !delegate_) {
    return;
  }
  noteGeometryActivity();
  delegate_->popupEnsureSurface();
  delegate_->popupPrepareToShow();
  syncPreparedPopupVisibility();
}

void OverlayPopupController::invalidatePopupGeometry() { resetGeometrySyncSnapshot(); }
bool OverlayPopupController::eventFilter(QObject* watched, QEvent* event) {
  if (!watched || !event) {
    return QObject::eventFilter(watched, event);
  }

  const QEvent::Type eventType = event->type();
  if (watchedByTrigger(watched)) {
    const bool watchedIsAnchor = watched == popupAnchorWidget();
    switch (eventType) {
      case QEvent::Enter:
      case QEvent::HoverEnter:
        handleTriggerHoverEnter();
        break;
      case QEvent::Leave:
      case QEvent::HoverLeave:
        handleTriggerHoverLeave();
        break;
      case QEvent::MouseMove:
      case QEvent::HoverMove:
        handleTriggerHoverEnter();
        break;
      case QEvent::FocusIn:
        focusTriggerActive_ = true;
        if (hasTrigger(Trigger::Focus)) {
          setReasonOpen(InternalOpenReason::Focus, true);
          updatePopupVisibility(true, VisibilityUpdateSource::UserInteraction);
        }
        break;
      case QEvent::FocusOut:
        handleTriggerFocusOutDeferred();
        break;
      case QEvent::MouseButtonPress:
        handleTriggerPress(watched, event);
        break;
      case QEvent::MouseButtonRelease:
        handleTriggerRelease(watched, event);
        break;
      case QEvent::KeyPress:
        handleTriggerKeyPress(event);
        break;
      case QEvent::KeyRelease:
        handleTriggerKeyRelease(event);
        break;
      case QEvent::ContextMenu:
        handleTriggerContextMenu(event);
        break;
      case QEvent::Move:
      case QEvent::Resize:
      case QEvent::Show:
        if (popupVisible_ && watchedIsAnchor) {
          if (QApplication::mouseButtons() == Qt::NoButton) {
            schedulePopupRelayout(true);
          }
        }
        break;
      case QEvent::ParentChange:
      case QEvent::ParentAboutToChange:
        if (watchedIsAnchor) {
          markAnchorScrollWatchersDirty();
        }
        if (popupVisible_ && watchedIsAnchor) {
          if (QApplication::mouseButtons() == Qt::NoButton) {
            schedulePopupRelayout(true);
          }
        }
        break;
      case QEvent::Hide:
        if (watchedIsAnchor) {
          triggerPressActive_ = false;
          triggerKeyPressActive_ = false;
          if (popupVisible_) {
            clearAllOpenReasons();
            updatePopupVisibility(true, VisibilityUpdateSource::InternalState);
          }
        }
        break;
      case QEvent::ChildAdded: {
        auto* childEvent = static_cast<QChildEvent*>(event);
        if (childEvent->child()) {
          traverseObjectTree(childEvent->child(), [this](QObject* object) {
            installPopupWatcher(&watchedTriggerObjects_, this, object);
          });
        }
        break;
      }
      default:
        break;
    }
  } else if (watchedByPopup(watched)) {
    switch (eventType) {
      case QEvent::LayoutRequest:
        if (popupVisible_ && watched == (delegate_ ? delegate_->popupSurfaceWidget() : nullptr)) {
          auto* popupWidget = qobject_cast<QWidget*>(watched);
          if (popupWidget && geometrySyncSnapshotValid_ && popupWidget->parentWidget() == geometrySyncParent_) {
            QSize requestedSize = popupWidget->sizeHint();
            requestedSize.setWidth(std::max(1, requestedSize.width()));
            requestedSize.setHeight(std::max(1, requestedSize.height()));
            if (requestedSize == geometrySyncPopupSize_ && popupWidget->size() == geometrySyncPopupSize_) {
              break;
            }
          }
          if (!shouldSkipQueuedRelayoutSync()) {
            schedulePopupRelayout(false);
          }
        }
        break;
      case QEvent::Enter:
      case QEvent::HoverEnter:
        handlePopupHoverEnter();
        break;
      case QEvent::Leave:
      case QEvent::HoverLeave:
        handlePopupHoverLeave();
        break;
      case QEvent::MouseMove:
      case QEvent::HoverMove:
        handlePopupHoverEnter();
        break;
      case QEvent::FocusIn:
        focusPopupActive_ = true;
        if (hasTrigger(Trigger::Focus)) {
          setReasonOpen(InternalOpenReason::Focus, true);
          updatePopupVisibility(true, VisibilityUpdateSource::UserInteraction);
        }
        break;
      case QEvent::FocusOut:
        handleTriggerFocusOutDeferred();
        break;
      case QEvent::KeyPress: {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Escape && popupVisible_) {
          clearAllOpenReasons();
          updatePopupVisibility(true, VisibilityUpdateSource::UserInteraction);
          keyEvent->accept();
        }
        break;
      }
      case QEvent::ChildAdded: {
        auto* childEvent = static_cast<QChildEvent*>(event);
        if (childEvent->child()) {
          traverseObjectTree(childEvent->child(), [this](QObject* object) {
            installPopupWatcher(&watchedPopupObjects_, this, object);
          });
        }
        break;
      }
      default:
        break;
    }
  }

  return QObject::eventFilter(watched, event);
}

void OverlayPopupController::setReasonOpen(InternalOpenReason reason, bool enabled) {
  switch (reason) {
    case InternalOpenReason::Hover:
      openByHover_ = enabled;
      break;
    case InternalOpenReason::Focus:
      openByFocus_ = enabled;
      break;
    case InternalOpenReason::Click:
      openByClick_ = enabled;
      break;
    case InternalOpenReason::ContextMenu:
      openByContextMenu_ = enabled;
      if (!enabled) {
        contextMenuGlobalPos_.reset();
      }
      break;
    case InternalOpenReason::Programmatic:
      openByProgrammatic_ = enabled;
      break;
  }
}

bool OverlayPopupController::reasonOpen(InternalOpenReason reason) const {
  switch (reason) {
    case InternalOpenReason::Hover:
      return openByHover_;
    case InternalOpenReason::Focus:
      return openByFocus_;
    case InternalOpenReason::Click:
      return openByClick_;
    case InternalOpenReason::ContextMenu:
      return openByContextMenu_;
    case InternalOpenReason::Programmatic:
      return openByProgrammatic_;
  }
  return false;
}

void OverlayPopupController::clearAllOpenReasons() {
  openByHover_ = false;
  openByFocus_ = false;
  openByClick_ = false;
  openByContextMenu_ = false;
  openByProgrammatic_ = false;
  triggerPressActive_ = false;
  triggerKeyPressActive_ = false;
  contextMenuGlobalPos_.reset();
}

bool OverlayPopupController::hasTrigger(Trigger trigger) const { return triggerModes_.testFlag(trigger); }

bool OverlayPopupController::shouldBeOpen() const {
  if (!delegate_ || disabled_ || !delegate_->popupHasContent()) {
    return false;
  }
  return openByHover_ || openByFocus_ || openByClick_ || openByContextMenu_ || openByProgrammatic_;
}

bool OverlayPopupController::triggerContainsGlobalPos(const QPoint& globalPos) const {
  if (!delegate_) {
    return false;
  }

  if (delegate_->popupTriggerGlobalRect().has_value()) {
    return delegate_->popupTriggerGlobalRect().value().contains(globalPos);
  }

  QWidget* trigger = popupTriggerWidget();
  if (!trigger) {
    return false;
  }
  QWidget* hovered = QApplication::widgetAt(globalPos);
  return widgetInTree(hovered, trigger);
}

bool OverlayPopupController::isHoveringTriggerTree() const {
  QWidget* trigger = popupTriggerWidget();
  if (!trigger) {
    return false;
  }
  return triggerContainsGlobalPos(QCursor::pos());
}

bool OverlayPopupController::isHoveringPopupTree() const {
  QWidget* popup = delegate_ ? delegate_->popupSurfaceWidget() : nullptr;
  if (!popup) {
    return false;
  }
  QWidget* hovered = QApplication::widgetAt(QCursor::pos());
  return widgetInTree(hovered, popup);
}

void OverlayPopupController::scheduleHoverOpen() {
  cancelTimingTask(this, QString::fromLatin1(kHoverCloseTaskKey));
  const int delay = std::max(0, mouseEnterDelayMs_);
  auto isHoveringNow = [this]() {
    return hoverTriggerActive_ || hoverPopupActive_ || isHoveringTriggerTree() || isHoveringPopupTree();
  };
  if (delay == 0) {
    if (isHoveringNow() && hasTrigger(Trigger::Hover)) {
      setReasonOpen(InternalOpenReason::Hover, true);
      updatePopupVisibility(true, VisibilityUpdateSource::UserInteraction);
    }
    return;
  }

  scheduleTimingTask(this, QString::fromLatin1(kHoverOpenTaskKey), delay, [this]() {
    const bool hovering = hoverTriggerActive_ || hoverPopupActive_ || isHoveringTriggerTree() || isHoveringPopupTree();
    if (hovering && hasTrigger(Trigger::Hover)) {
      setReasonOpen(InternalOpenReason::Hover, true);
      updatePopupVisibility(true, VisibilityUpdateSource::UserInteraction);
    }
  });
}

void OverlayPopupController::scheduleHoverClose() {
  cancelTimingTask(this, QString::fromLatin1(kHoverOpenTaskKey));
  const int delay = std::max(0, mouseLeaveDelayMs_);
  auto reconcileHoverStateFromCursor = [this]() {
    if (!isHoveringTriggerTree()) {
      hoverTriggerActive_ = false;
    }
    if (!isHoveringPopupTree()) {
      hoverPopupActive_ = false;
    }
  };
  if (delay == 0) {
    reconcileHoverStateFromCursor();
    if (!hoverTriggerActive_ && !hoverPopupActive_) {
      setReasonOpen(InternalOpenReason::Hover, false);
      updatePopupVisibility(true, VisibilityUpdateSource::UserInteraction);
    }
    return;
  }

  scheduleTimingTask(this, QString::fromLatin1(kHoverCloseTaskKey), delay, [this]() {
    if (!isHoveringTriggerTree()) {
      hoverTriggerActive_ = false;
    }
    if (!isHoveringPopupTree()) {
      hoverPopupActive_ = false;
    }
    if (!hoverTriggerActive_ && !hoverPopupActive_) {
      setReasonOpen(InternalOpenReason::Hover, false);
      updatePopupVisibility(true, VisibilityUpdateSource::UserInteraction);
    }
  });
}

void OverlayPopupController::clearHoverTasks() {
  cancelTimingTask(this, QString::fromLatin1(kHoverOpenTaskKey));
  cancelTimingTask(this, QString::fromLatin1(kHoverCloseTaskKey));
}

void OverlayPopupController::noteGeometryActivity() {
  if (!popupVisible_) {
    return;
  }
  const qint64 now = timingNowMs();
  const qint64 nextDeadline = now + kGeometryFrameSyncTailMs;
  if (nextDeadline <= geometryFrameSyncDeadlineMs_) {
    return;
  }
  geometryFrameSyncDeadlineMs_ = nextDeadline;
  if (!geometryFrameSyncSubscribed_) {
    refreshGeometryFrameSync();
  }
}

void OverlayPopupController::schedulePopupRelayout(bool extendFrameTail) {
  if (!popupVisible_) {
    return;
  }
  if (popupRelayoutQueued_) {
    return;
  }
  if (!extendFrameTail && shouldSkipQueuedRelayoutSync()) {
    return;
  }
  if (extendFrameTail) {
    noteGeometryActivity();
  }
  popupRelayoutQueued_ = true;
  QWidget* scope = popupScopeWindow();
  if (!scope) {
    popupRelayoutQueuedScope_.clear();
    deferTimingTask(this, QString::fromLatin1(kPopupRelayoutTaskKey), [this]() {
      popupRelayoutQueued_ = false;
      popupRelayoutFromHost();
    });
    return;
  }

  auto& pending = pendingScopeRelayouts();
  ensureScopeRelayoutDestroyedWatcher(scope);
  popupRelayoutQueuedScope_ = scope;
  PendingRelayoutList& queuedSet = pending[scope];
  pruneDeadPendingRelayouts(&queuedSet);
  const bool shouldScheduleScopeTask = queuedSet.isEmpty();
  if (!pendingRelayoutListContains(queuedSet, this)) {
    queuedSet.append(this);
  }
  if (!shouldScheduleScopeTask) {
    return;
  }
  deferTimingTask(scope, QString::fromLatin1(kScopePopupRelayoutTaskKey), [scope]() {
    auto& pendingMap = pendingScopeRelayouts();
    PendingRelayoutList queued = pendingMap.take(scope);
    pruneDeadPendingRelayouts(&queued);
    clearScopeRelayoutDestroyedWatcherIfUnused(scope);
    if (queued.isEmpty()) {
      return;
    }
    for (const QPointer<OverlayPopupController>& controller : queued) {
      if (!controller) {
        continue;
      }
      controller->popupRelayoutQueued_ = false;
      controller->popupRelayoutQueuedScope_.clear();
      if (!controller->popupVisible_) {
        continue;
      }
      controller->popupRelayoutFromHost();
    }
  });
}

void OverlayPopupController::cancelPopupRelayout() {
  popupRelayoutQueued_ = false;
  QWidget* queuedScope = popupRelayoutQueuedScope_.data();
  popupRelayoutQueuedScope_.clear();
  if (queuedScope) {
    removeControllerFromPendingScopeRelayouts(this, queuedScope);
  }
  cancelTimingTask(this, QString::fromLatin1(kPopupRelayoutTaskKey));
}

void OverlayPopupController::refreshGeometryFrameSync() {
  if (!popupVisible_ || timingNowMs() >= geometryFrameSyncDeadlineMs_) {
    geometryFrameSyncDeadlineMs_ = -1;
    geometryFrameSyncSubscribed_ = false;
    clearFrameSubscription(this, QString::fromLatin1(kGeometryFrameSyncTaskKey));
    return;
  }
  if (geometryFrameSyncSubscribed_) {
    return;
  }

  geometryFrameSyncSubscribed_ = true;
  setFrameSubscription(this, QString::fromLatin1(kGeometryFrameSyncTaskKey), true,
                       [this](qint64 nowMs, qint64) {
                         if (!popupVisible_) {
                           geometryFrameSyncDeadlineMs_ = -1;
                           refreshGeometryFrameSync();
                           return;
                         }
                         schedulePopupRelayout(false);
                         if (nowMs >= geometryFrameSyncDeadlineMs_) {
                           geometryFrameSyncDeadlineMs_ = -1;
                           refreshGeometryFrameSync();
                         }
                       });
}
bool OverlayPopupController::shouldSkipQueuedRelayoutSync() const {
  QWidget* popup = delegate_ ? delegate_->popupSurfaceWidget() : nullptr;
  if (!popupVisible_ || !popup || !geometrySyncSnapshotValid_ || !delegate_) {
    return false;
  }

  QWidget* popupParent = popup->parentWidget();
  if (!popupParent || geometrySyncParent_ != popupParent) {
    return false;
  }

  QWidget* anchor = popupAnchorWidget();
  QWidget* scope = popupScopeWindow();
  const qint64 now = timingNowMs();
  if (anchorScrollWatchersDirty_ || watchedScrollAnchor_ != anchor || watchedScrollScope_ != scope ||
      now >= nextAnchorScrollWatchersRefreshMs_) {
    return false;
  }

  QRect anchorRect;
  if (contextMenuGlobalPos_.has_value() && reasonOpen(InternalOpenReason::ContextMenu)) {
    anchorRect = QRect(contextMenuGlobalPos_.value(), QSize(1, 1));
  } else {
    anchorRect = widgetGlobalRectIfEffectivelyVisible(anchor, popupParent);
  }
  if (!anchorRect.isValid()) {
    return false;
  }

  QSize popupSize = popup->sizeHint();
  popupSize.setWidth(std::max(1, popupSize.width()));
  popupSize.setHeight(std::max(1, popupSize.height()));

  OverlayPopupPlacementInput currentInput;
  currentInput.anchorRect = anchorRect;
  currentInput.popupSize = popupSize;
  currentInput.bounds = popupBoundsInGlobal(popupParent);
  currentInput.preferredPlacement = delegate_->popupPlacement();
  currentInput.popupOffset = std::max(0, delegate_->popupOffset());
  currentInput.allowFallback = delegate_->popupAutoAdjustOverflow();
  currentInput.pointAtCenter = delegate_->popupArrowPointAtCenter();
  currentInput.arrowOffsetHorizontal = delegate_->popupArrowOffsetHorizontal();
  currentInput.arrowOffsetVertical = delegate_->popupArrowOffsetVertical();

  OverlayPopupPlacementInput previousInput;
  previousInput.anchorRect = geometrySyncAnchorRect_;
  previousInput.popupSize = geometrySyncPopupSize_;
  previousInput.bounds = geometrySyncBounds_;
  previousInput.preferredPlacement = geometrySyncPlacement_;
  previousInput.popupOffset = geometrySyncPopupOffset_;
  previousInput.allowFallback = geometrySyncAutoAdjustOverflow_;
  previousInput.pointAtCenter = geometrySyncArrowPointAtCenter_;
  previousInput.arrowOffsetHorizontal = geometrySyncArrowOffsetHorizontal_;
  previousInput.arrowOffsetVertical = geometrySyncArrowOffsetVertical_;

  const OverlayPopupPlacementOutput currentPlacement = resolveOverlayPopupPlacement(currentInput);
  const OverlayPopupPlacementOutput previousPlacement = resolveOverlayPopupPlacement(previousInput);

  const bool placementUnchanged = currentPlacement.placement == previousPlacement.placement;
  const bool topLeftUnchanged = currentPlacement.topLeft == previousPlacement.topLeft;
  const bool arrowUnchanged =
      qFuzzyCompare(currentPlacement.arrowCenterCoord + 1.0, previousPlacement.arrowCenterCoord + 1.0);
  if (!placementUnchanged || !topLeftUnchanged || !arrowUnchanged) {
    return false;
  }

  const QPoint expectedPopupPos = popupParent->mapFromGlobal(currentPlacement.topLeft);
  if (popup->pos() != expectedPopupPos) {
    return false;
  }
  if (popup->size() != popupSize) {
    return false;
  }
  return true;
}

void OverlayPopupController::resetGeometrySyncSnapshot() {
  geometrySyncParent_.clear();
  geometrySyncAnchorRect_ = QRect();
  geometrySyncBounds_ = QRect();
  geometrySyncPopupSize_ = QSize();
  geometrySyncPlacement_ = OverlayPopupPlacement::Top;
  geometrySyncAutoAdjustOverflow_ = true;
  geometrySyncPopupOffset_ = 0;
  geometrySyncArrowPointAtCenter_ = false;
  geometrySyncArrowOffsetHorizontal_ = 0;
  geometrySyncArrowOffsetVertical_ = 0;
  geometrySyncSnapshotValid_ = false;
}

void OverlayPopupController::markAnchorScrollWatchersDirty() {
  anchorScrollWatchersDirty_ = true;
  nextAnchorScrollWatchersRefreshMs_ = 0;
}

void OverlayPopupController::refreshAnchorScrollBarWatchers() {
  if (!popupVisible_) {
    clearAnchorScrollBarWatchers();
    return;
  }

  QWidget* anchor = popupAnchorWidget();
  if (!anchor) {
    clearAnchorScrollBarWatchers();
    return;
  }
  QWidget* scope = popupScopeWindow();

  const qint64 now = timingNowMs();
  if (!anchorScrollWatchersDirty_ && watchedScrollAnchor_ == anchor && watchedScrollScope_ == scope &&
      now < nextAnchorScrollWatchersRefreshMs_) {
    return;
  }
  watchedScrollAnchor_ = anchor;
  watchedScrollScope_ = scope;

  QSet<QScrollBar*> nextScrollBars;
  QWidget* cursor = anchor;
  while (cursor) {
    if (auto* scrollArea = qobject_cast<QAbstractScrollArea*>(cursor)) {
      if (QScrollBar* verticalBar = scrollArea->verticalScrollBar()) {
        nextScrollBars.insert(verticalBar);
      }
      if (QScrollBar* horizontalBar = scrollArea->horizontalScrollBar()) {
        nextScrollBars.insert(horizontalBar);
      }
    }
    if (scope && cursor == scope) {
      break;
    }
    cursor = cursor->parentWidget();
  }

  for (auto it = watchedAnchorScrollBars_.begin(); it != watchedAnchorScrollBars_.end();) {
    QScrollBar* bar = it.key();
    if (!bar || !nextScrollBars.contains(bar)) {
      QObject::disconnect(it.value().valueChanged);
      QObject::disconnect(it.value().destroyed);
      it = watchedAnchorScrollBars_.erase(it);
      continue;
    }
    ++it;
  }

  for (QScrollBar* bar : nextScrollBars) {
    if (!bar || watchedAnchorScrollBars_.contains(bar)) {
      continue;
    }

    ScrollBarWatch watch;
    watch.valueChanged = QObject::connect(bar, &QScrollBar::valueChanged, this, [this](int) {
      schedulePopupRelayout(true);
    });
    watch.destroyed = QObject::connect(bar, &QObject::destroyed, this, [this, bar]() {
      auto it = watchedAnchorScrollBars_.find(bar);
      if (it == watchedAnchorScrollBars_.end()) {
        return;
      }
      QObject::disconnect(it.value().valueChanged);
      QObject::disconnect(it.value().destroyed);
      watchedAnchorScrollBars_.erase(it);
    });
    watchedAnchorScrollBars_.insert(bar, watch);
  }

  anchorScrollWatchersDirty_ = false;
  nextAnchorScrollWatchersRefreshMs_ = now + kAnchorScrollWatchersRefreshIntervalMs;
}

void OverlayPopupController::clearAnchorScrollBarWatchers() {
  for (auto it = watchedAnchorScrollBars_.begin(); it != watchedAnchorScrollBars_.end(); ++it) {
    QObject::disconnect(it.value().valueChanged);
    QObject::disconnect(it.value().destroyed);
  }
  watchedAnchorScrollBars_.clear();
  watchedScrollAnchor_.clear();
  watchedScrollScope_.clear();
  markAnchorScrollWatchersDirty();
}

void OverlayPopupController::emitVisibilityRequest(bool requestedVisible) {
  if (visibilityMode_ != VisibilityMode::External || !delegate_) {
    return;
  }
  if (requestedVisible && (disabled_ || !delegate_->popupHasContent())) {
    return;
  }
  if (requestedVisible == popupVisible_) {
    return;
  }
  emit popupVisibilityRequested(requestedVisible);
}

void OverlayPopupController::updatePopupVisibility(bool emitSignal, VisibilityUpdateSource source) {
  if (updatingPopupVisible_) {
    return;
  }
  const bool shouldOpen = shouldBeOpen();
  if (visibilityMode_ == VisibilityMode::External) {
    if (!emitSignal || source != VisibilityUpdateSource::UserInteraction) {
      return;
    }
    emitVisibilityRequest(shouldOpen);
    return;
  }
  setPopupVisibleInternal(shouldOpen, emitSignal);
}

void OverlayPopupController::setPopupVisibleInternal(bool visible, bool emitSignal) {
  if (updatingPopupVisible_ || !delegate_) {
    return;
  }
  updatingPopupVisible_ = true;

  if (popupVisible_ == visible) {
    if (popupVisible_) {
      setInWindowPopupHostOpen(this, true);
      refreshAnchorScrollBarWatchers();
      noteGeometryActivity();
      delegate_->popupEnsureSurface();
      delegate_->popupPrepareToShow();
      syncPreparedPopupVisibility();
    }
    updatingPopupVisible_ = false;
    return;
  }

  popupVisible_ = visible;
  setInWindowPopupHostOpen(this, popupVisible_);
  refreshAnchorScrollBarWatchers();
  noteGeometryActivity();
  if (popupVisible_) {
    delegate_->popupEnsureSurface();
    delegate_->popupPrepareToShow();
    syncPreparedPopupVisibility();
  } else {
    cancelPopupRelayout();
    geometryFrameSyncDeadlineMs_ = -1;
    refreshGeometryFrameSync();
    resetGeometrySyncSnapshot();
    clearAnchorScrollBarWatchers();
    clearHoverTasks();
    applyPopupVisibility(delegate_->popupSurfaceWidget(), false, false);
    if (delegate_->popupReleaseOnHide()) {
      delegate_->popupReleaseSurface();
    }
  }

  if (emitSignal) {
    emit popupVisibleChanged(popupVisible_);
  }
  updatingPopupVisible_ = false;
}

void OverlayPopupController::syncPreparedPopupVisibility() {
  QWidget* popup = delegate_ ? delegate_->popupSurfaceWidget() : nullptr;
  if (!delegate_ || !popupVisible_ || !popup) {
    return;
  }

  const bool canShowPopup = syncPopupGeometry();
  applyPopupVisibility(popup, canShowPopup, true);
  if (!canShowPopup || !popup->isVisible()) {
    return;
  }

  const bool updatedCanShowPopup = syncPopupGeometry();
  applyPopupVisibility(popup, updatedCanShowPopup, true);
}

bool OverlayPopupController::syncPopupGeometry() {
  recordSyncPopupGeometryCallForTesting();
  QWidget* popup = delegate_ ? delegate_->popupSurfaceWidget() : nullptr;
  if (!popupVisible_ || !popup || !delegate_) {
    return false;
  }

  preparePopupForGeometrySync(popup);

  refreshAnchorScrollBarWatchers();

  QWidget* popupParent = popup->parentWidget();
  QWidget* expectedPopupParent = popupScopeWindow();
  if (expectedPopupParent && popupParent != expectedPopupParent) {
    const bool wasVisible = popup->isVisible();
    popup->setParent(expectedPopupParent);
    popupParent = expectedPopupParent;
    refreshPopupWatchers();
    setInWindowPopupHostOpen(this, true);
    applyPopupVisibility(popup, wasVisible, true);
  }
  if (!popupParent) {
    applyPopupVisibility(popup, false, false);
    resetGeometrySyncSnapshot();
    return false;
  }

  QRect anchorRect;
  if (contextMenuGlobalPos_.has_value() && reasonOpen(InternalOpenReason::ContextMenu)) {
    anchorRect = QRect(contextMenuGlobalPos_.value(), QSize(1, 1));
  } else if (delegate_->popupAnchorGlobalRect().has_value()) {
    anchorRect = delegate_->popupAnchorGlobalRect().value();
    const QRect visibleAnchorRect = widgetGlobalRectIfEffectivelyVisible(popupAnchorWidget(), popupParent);
    if (!visibleAnchorRect.isValid()) {
      anchorRect = QRect();
    } else {
      anchorRect = anchorRect.intersected(visibleAnchorRect);
    }
  } else {
    anchorRect = widgetGlobalRectIfEffectivelyVisible(popupAnchorWidget(), popupParent);
  }
  if (!anchorRect.isValid()) {
    applyPopupVisibility(popup, false, false);
    resetGeometrySyncSnapshot();
    return false;
  }

  QSize popupSize = popup->sizeHint();
  popupSize.setWidth(std::max(1, popupSize.width()));
  popupSize.setHeight(std::max(1, popupSize.height()));

  const QRect bounds = popupBoundsInGlobal(popupParent);
  const int popupOffset = std::max(0, delegate_->popupOffset());
  const int arrowOffsetHorizontal = delegate_->popupArrowOffsetHorizontal();
  const int arrowOffsetVertical = delegate_->popupArrowOffsetVertical();

  const bool inputsUnchanged = geometrySyncSnapshotValid_ && geometrySyncParent_ == popupParent &&
                               geometrySyncAnchorRect_ == anchorRect && geometrySyncBounds_ == bounds &&
                               geometrySyncPopupSize_ == popupSize &&
                               geometrySyncPlacement_ == delegate_->popupPlacement() &&
                               geometrySyncAutoAdjustOverflow_ == delegate_->popupAutoAdjustOverflow() &&
                               geometrySyncPopupOffset_ == popupOffset &&
                               geometrySyncArrowPointAtCenter_ == delegate_->popupArrowPointAtCenter() &&
                               geometrySyncArrowOffsetHorizontal_ == arrowOffsetHorizontal &&
                               geometrySyncArrowOffsetVertical_ == arrowOffsetVertical;
  if (inputsUnchanged) {
    recordSyncPopupGeometryShortCircuitForTesting();
    return true;
  }

  if (popup->size() != popupSize) {
    popup->resize(popupSize);
  }

  OverlayPopupPlacementInput placementInput;
  placementInput.anchorRect = anchorRect;
  placementInput.popupSize = popupSize;
  placementInput.bounds = bounds;
  placementInput.preferredPlacement = delegate_->popupPlacement();
  placementInput.popupOffset = popupOffset;
  placementInput.allowFallback = delegate_->popupAutoAdjustOverflow();
  placementInput.pointAtCenter = delegate_->popupArrowPointAtCenter();
  placementInput.arrowOffsetHorizontal = arrowOffsetHorizontal;
  placementInput.arrowOffsetVertical = arrowOffsetVertical;
  const OverlayPopupPlacementOutput placementResult = resolveOverlayPopupPlacement(placementInput);

  const QPoint popupTopLeft = popupParent->mapFromGlobal(placementResult.topLeft);
  if (popup->pos() != popupTopLeft) {
    popup->move(popupTopLeft);
  }
  delegate_->popupApplyResolvedPlacement(placementResult.placement, placementResult.arrowCenterCoord);

  geometrySyncParent_ = popupParent;
  geometrySyncAnchorRect_ = anchorRect;
  geometrySyncBounds_ = bounds;
  geometrySyncPopupSize_ = popupSize;
  geometrySyncPlacement_ = delegate_->popupPlacement();
  geometrySyncAutoAdjustOverflow_ = delegate_->popupAutoAdjustOverflow();
  geometrySyncPopupOffset_ = popupOffset;
  geometrySyncArrowPointAtCenter_ = delegate_->popupArrowPointAtCenter();
  geometrySyncArrowOffsetHorizontal_ = arrowOffsetHorizontal;
  geometrySyncArrowOffsetVertical_ = arrowOffsetVertical;
  geometrySyncSnapshotValid_ = true;
  return true;
}
void OverlayPopupController::refreshTriggerWatchers() {
  clearTriggerWatchers();
  QWidget* trigger = popupTriggerWidget();
  if (!trigger) {
    return;
  }

  watchedTriggerRoot_ = trigger;
  traverseObjectTree(trigger, [this](QObject* object) { installPopupWatcher(&watchedTriggerObjects_, this, object); });
}

void OverlayPopupController::clearTriggerWatchers() {
  const WatchedObjectList watchedObjects = watchedTriggerObjects_;
  watchedTriggerObjects_.clear();
  for (const QPointer<QWidget>& object : watchedObjects) {
    if (object) {
      object->removeEventFilter(this);
    }
  }
  watchedTriggerRoot_.clear();
}

void OverlayPopupController::refreshPopupWatchers() {
  clearPopupWatchers();
  QWidget* popup = delegate_ ? delegate_->popupSurfaceWidget() : nullptr;
  if (!popup) {
    return;
  }
  traverseObjectTree(popup, [this](QObject* object) { installPopupWatcher(&watchedPopupObjects_, this, object); });
}

void OverlayPopupController::clearPopupWatchers() {
  const WatchedObjectList watchedObjects = watchedPopupObjects_;
  watchedPopupObjects_.clear();
  for (const QPointer<QWidget>& object : watchedObjects) {
    if (object) {
      object->removeEventFilter(this);
    }
  }
}

bool OverlayPopupController::watchedByTrigger(QObject* watched) const {
  auto* widget = qobject_cast<QWidget*>(watched);
  return widget && watchedObjectListContains(watchedTriggerObjects_, widget);
}

bool OverlayPopupController::watchedByPopup(QObject* watched) const {
  auto* widget = qobject_cast<QWidget*>(watched);
  return widget && watchedObjectListContains(watchedPopupObjects_, widget);
}

void OverlayPopupController::handleTriggerPress(QObject* watched, QEvent* event) {
  Q_UNUSED(watched)
  if (!event || disabled_ || !hasTrigger(Trigger::Click)) {
    return;
  }
  if (event->type() != QEvent::MouseButtonPress) {
    return;
  }

  auto* mouseEvent = static_cast<QMouseEvent*>(event);
  if (mouseEvent->button() != Qt::LeftButton || triggerPressActive_) {
    return;
  }
  if (!triggerContainsGlobalPos(mouseEvent->globalPosition().toPoint())) {
    return;
  }
  triggerPressActive_ = true;
}

void OverlayPopupController::handleTriggerRelease(QObject* watched, QEvent* event) {
  if (!event || event->type() != QEvent::MouseButtonRelease) {
    return;
  }
  auto* mouseEvent = static_cast<QMouseEvent*>(event);
  if (mouseEvent->button() != Qt::LeftButton || !triggerPressActive_) {
    return;
  }

  triggerPressActive_ = false;
  QWidget* releaseTarget = qobject_cast<QWidget*>(watched);
  const bool releaseOnTriggerTree = widgetInTree(releaseTarget, popupTriggerWidget());
  if (!releaseOnTriggerTree) {
    if (!triggerContainsGlobalPos(mouseEvent->globalPosition().toPoint())) {
      return;
    }
  } else if (!triggerContainsGlobalPos(mouseEvent->globalPosition().toPoint())) {
    return;
  }

  if (disabled_ || !hasTrigger(Trigger::Click)) {
    return;
  }
  if (visibilityMode_ == VisibilityMode::External) {
    emitVisibilityRequest(!popupVisible_);
    return;
  }

  setReasonOpen(InternalOpenReason::Click, !reasonOpen(InternalOpenReason::Click));
  if (!reasonOpen(InternalOpenReason::Click)) {
    contextMenuGlobalPos_.reset();
  }
  updatePopupVisibility(true, VisibilityUpdateSource::UserInteraction);
}

void OverlayPopupController::handleTriggerKeyPress(QEvent* event) {
  if (!event || event->type() != QEvent::KeyPress) {
    return;
  }

  auto* keyEvent = static_cast<QKeyEvent*>(event);
  if (keyEvent->key() == Qt::Key_Escape && popupVisible_) {
    clearAllOpenReasons();
    updatePopupVisibility(true, VisibilityUpdateSource::UserInteraction);
    keyEvent->accept();
    return;
  }
  if (disabled_ || !hasTrigger(Trigger::Click) || keyEvent->isAutoRepeat()) {
    return;
  }
  if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter ||
      keyEvent->key() == Qt::Key_Space) {
    triggerKeyPressActive_ = true;
  }
}

void OverlayPopupController::handleTriggerKeyRelease(QEvent* event) {
  if (!event || event->type() != QEvent::KeyRelease) {
    return;
  }

  auto* keyEvent = static_cast<QKeyEvent*>(event);
  if (keyEvent->isAutoRepeat()) {
    return;
  }
  const bool activationKey =
      keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter ||
      keyEvent->key() == Qt::Key_Space;
  if (!activationKey || !triggerKeyPressActive_) {
    return;
  }
  triggerKeyPressActive_ = false;

  if (disabled_ || !hasTrigger(Trigger::Click) || !widgetInTree(QApplication::focusWidget(), popupTriggerWidget())) {
    return;
  }
  if (visibilityMode_ == VisibilityMode::External) {
    emitVisibilityRequest(!popupVisible_);
    keyEvent->accept();
    return;
  }

  setReasonOpen(InternalOpenReason::Click, !reasonOpen(InternalOpenReason::Click));
  if (!reasonOpen(InternalOpenReason::Click)) {
    contextMenuGlobalPos_.reset();
  }
  updatePopupVisibility(true, VisibilityUpdateSource::UserInteraction);
  keyEvent->accept();
}

void OverlayPopupController::handleTriggerContextMenu(QEvent* event) {
  if (!event || disabled_ || !hasTrigger(Trigger::ContextMenu) || event->type() != QEvent::ContextMenu) {
    return;
  }

  auto* contextEvent = static_cast<QContextMenuEvent*>(event);
  if (!triggerContainsGlobalPos(contextEvent->globalPos())) {
    return;
  }
  contextMenuGlobalPos_ = contextEvent->globalPos();
  if (visibilityMode_ == VisibilityMode::External) {
    emitVisibilityRequest(true);
    contextEvent->accept();
    return;
  }

  setReasonOpen(InternalOpenReason::ContextMenu, true);
  updatePopupVisibility(true, VisibilityUpdateSource::UserInteraction);
  contextEvent->accept();
}

void OverlayPopupController::handleTriggerFocusOutDeferred() {
  deferTimingTask(this, QString::fromLatin1(kFocusRecheckTaskKey), [this]() {
    QWidget* focused = QApplication::focusWidget();
    focusTriggerActive_ = widgetInTree(focused, popupTriggerWidget());
    focusPopupActive_ = widgetInTree(focused, delegate_ ? delegate_->popupSurfaceWidget() : nullptr);
    if (!hasTrigger(Trigger::Focus)) {
      return;
    }
    if (visibilityMode_ == VisibilityMode::External) {
      emitVisibilityRequest(focusTriggerActive_ || focusPopupActive_);
      return;
    }
    setReasonOpen(InternalOpenReason::Focus, focusTriggerActive_ || focusPopupActive_);
    updatePopupVisibility(true, VisibilityUpdateSource::UserInteraction);
  });
}

void OverlayPopupController::handleTriggerHoverEnter() {
  const bool wasActive = hoverTriggerActive_;
  hoverTriggerActive_ = true;
  if (!hasTrigger(Trigger::Hover)) {
    return;
  }
  if (wasActive == hoverTriggerActive_ && reasonOpen(InternalOpenReason::Hover)) {
    return;
  }
  scheduleHoverOpen();
}

void OverlayPopupController::handleTriggerHoverLeave() {
  const bool wasActive = hoverTriggerActive_;
  hoverTriggerActive_ = isHoveringTriggerTree();
  if (!hasTrigger(Trigger::Hover)) {
    return;
  }
  if (wasActive == hoverTriggerActive_ && !reasonOpen(InternalOpenReason::Hover)) {
    return;
  }
  scheduleHoverClose();
}

void OverlayPopupController::handlePopupHoverEnter() {
  const bool wasActive = hoverPopupActive_;
  hoverPopupActive_ = true;
  if (!hasTrigger(Trigger::Hover)) {
    return;
  }
  if (wasActive == hoverPopupActive_ && reasonOpen(InternalOpenReason::Hover)) {
    return;
  }
  scheduleHoverOpen();
}

void OverlayPopupController::handlePopupHoverLeave() {
  const bool wasActive = hoverPopupActive_;
  hoverPopupActive_ = isHoveringPopupTree();
  if (!hasTrigger(Trigger::Hover)) {
    return;
  }
  if (wasActive == hoverPopupActive_ && !reasonOpen(InternalOpenReason::Hover)) {
    return;
  }
  scheduleHoverClose();
}

QObject* OverlayPopupController::popupOwnerObject() const {
  return const_cast<OverlayPopupController*>(this);
}

QWidget* OverlayPopupController::popupTriggerWidget() const {
  return delegate_ ? delegate_->popupTriggerWidget() : nullptr;
}

QWidget* OverlayPopupController::popupAnchorWidget() const {
  return delegate_ ? delegate_->popupAnchorWidget() : nullptr;
}

QWidget* OverlayPopupController::popupScopeWindow() const {
  return delegate_ ? delegate_->popupScopeWindow() : nullptr;
}

bool OverlayPopupController::popupIsVisible() const {
  QWidget* popup = delegate_ ? delegate_->popupSurfaceWidget() : nullptr;
  return popupVisible_ && popup && popup->isVisible();
}

bool OverlayPopupController::popupWantsHostFrameRelayout() const { return false; }

bool OverlayPopupController::popupContainsGlobalPos(const QPoint& globalPos) const {
  return widgetContainsGlobalPos(popupTriggerWidget(), globalPos) ||
         widgetContainsGlobalPos(popupAnchorWidget(), globalPos) ||
         widgetContainsGlobalPos(delegate_ ? delegate_->popupSurfaceWidget() : nullptr, globalPos);
}

void OverlayPopupController::popupCloseFromHost(PopupCloseReason reason) {
  Q_UNUSED(reason)
  if (closingFromHost_) {
    return;
  }
  closingFromHost_ = true;
  clearAllOpenReasons();
  updatePopupVisibility(true, VisibilityUpdateSource::UserInteraction);
  closingFromHost_ = false;
}

void OverlayPopupController::popupRelayoutFromHost() {
  if (!popupVisible_ || !delegate_) {
    return;
  }
  if (shouldSkipQueuedRelayoutSync()) {
    return;
  }
  const bool canShowPopup = syncPopupGeometry();
  applyPopupVisibility(delegate_->popupSurfaceWidget(), canShowPopup, true);
}

}  // namespace adqt::widgets::detail


