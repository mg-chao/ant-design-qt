#include "in_window_popup_host.h"
#include "detail/timing_hub.h"

#include <QApplication>
#include <QEvent>
#include <QHash>
#include <QMouseEvent>
#include <QPointer>
#include <QSet>
#include <QString>
#include <QTouchEvent>
#include <QWidget>

#include <optional>

namespace adqt::widgets::detail {

namespace {

constexpr char kRelayoutTaskKey[] = "InWindowPopupHost.Relayout";
constexpr char kRefreshAnchorWatchersTaskKey[] = "InWindowPopupHost.RefreshAnchorWatchers";

QPoint mouseEventGlobalPos(const QMouseEvent* event) {
  if (!event) {
    return QPoint();
  }
  return event->globalPosition().toPoint();
}

std::optional<QPoint> touchEventGlobalPos(const QTouchEvent* event) {
  if (!event) {
    return std::nullopt;
  }
  const QList<QEventPoint> points = event->points();
  if (points.isEmpty()) {
    return std::nullopt;
  }
  return points.constFirst().globalPosition().toPoint();
}

std::optional<QPoint> popupInteractionGlobalPos(const QEvent* event) {
  if (!event) {
    return std::nullopt;
  }
  switch (event->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonDblClick:
      return mouseEventGlobalPos(static_cast<const QMouseEvent*>(event));
    case QEvent::TouchBegin:
      return touchEventGlobalPos(static_cast<const QTouchEvent*>(event));
    default:
      return std::nullopt;
  }
}

QRect widgetGlobalRect(const QWidget* widget) {
  if (!widget) {
    return QRect();
  }
  return QRect(widget->mapToGlobal(QPoint(0, 0)), widget->size());
}

bool isAnchorGeometryEvent(QEvent::Type type) {
  switch (type) {
    case QEvent::Move:
    case QEvent::Resize:
    case QEvent::Show:
    case QEvent::LayoutRequest:
    case QEvent::Wheel:
    case QEvent::ContentsRectChange:
    case QEvent::StyleChange:
    case QEvent::PolishRequest:
      return true;
    default:
      return false;
  }
}

class InWindowPopupHost final : public QObject {
 public:
  explicit InWindowPopupHost(QWidget* scopeWindow)
      : QObject(scopeWindow), scopeWindow_(scopeWindow) {}

  bool owns(const InWindowPopupOwner* owner) const { return activeOwner_ == owner; }

  void activateOwner(InWindowPopupOwner* owner) {
    if (!owner || !scopeWindow_) {
      return;
    }

    QObject* ownerObject = owner->popupOwnerObject();
    QWidget* anchorWidget = owner->popupAnchorWidget();
    if (!ownerObject || !anchorWidget) {
      return;
    }

    if (activeOwner_ == owner) {
      refreshAnchorChainWatchers();
      if (owner->popupIsVisible()) {
        scheduleRelayout();
      }
      return;
    }

    if (activeOwner_ && activeOwner_ != owner) {
      requestCloseActive(PopupCloseReason::SupersededByAnotherOwner);
      if (activeOwner_ && activeOwner_ != owner) {
        clearActiveOwner();
      }
    }

    activeOwner_ = owner;
    activeOwnerObject_ = ownerObject;
    activeAnchorWidget_ = anchorWidget;

    ownerDestroyedConnection_ = QObject::connect(ownerObject, &QObject::destroyed, this, [this]() {
      clearActiveOwner();
      removeAllWatchers();
    });

    if (qApp) {
      qApp->removeEventFilter(this);
      qApp->installEventFilter(this);
    }
    if (scopeWindow_) {
      scopeWindow_->removeEventFilter(this);
      scopeWindow_->installEventFilter(this);
    }

    refreshAnchorChainWatchers();
    if (owner->popupIsVisible()) {
      scheduleRelayout();
    }
  }

  void deactivateOwner(InWindowPopupOwner* owner) {
    if (!owner || activeOwner_ != owner) {
      return;
    }
    removeAllWatchers();
    clearActiveOwner();
  }

 protected:
  bool eventFilter(QObject* watched, QEvent* event) override {
    if (!watched || !event || !activeOwner_) {
      return QObject::eventFilter(watched, event);
    }

    const bool watchedScope = (scopeWindow_ && watched == scopeWindow_.data());
    const bool watchedAnchorChain = watchedInAnchorChain(watched);

    // Application-level event filters receive the concrete target QObject (not qApp itself),
    // so outside-click close must not depend on watched == qApp.
    const bool outsideCloseInteractionEvent = event->type() == QEvent::MouseButtonPress ||
                                              event->type() == QEvent::MouseButtonDblClick ||
                                              event->type() == QEvent::TouchBegin;
    if (outsideCloseInteractionEvent && activeOwner_->popupIsVisible()) {
      const std::optional<QPoint> interactionGlobalPos = popupInteractionGlobalPos(event);
      if (!interactionGlobalPos.has_value()) {
        return QObject::eventFilter(watched, event);
      }
      const QRect scopeGlobalRect = widgetGlobalRect(scopeWindow_);
      if (scopeGlobalRect.isValid() && scopeGlobalRect.contains(interactionGlobalPos.value()) &&
          !activeOwner_->popupContainsGlobalPos(interactionGlobalPos.value())) {
        requestCloseActive(PopupCloseReason::OutsidePressInScope);
      }
      return QObject::eventFilter(watched, event);
    }

    if (watchedScope) {
      switch (event->type()) {
        case QEvent::Hide:
          requestCloseActive(PopupCloseReason::ScopeHidden);
          break;
        case QEvent::WindowDeactivate:
          requestCloseActive(PopupCloseReason::ScopeDeactivated);
          break;
        case QEvent::Move:
        case QEvent::Resize:
        case QEvent::Show:
        case QEvent::LayoutRequest:
          scheduleRelayout();
          break;
        default:
          break;
      }
      return QObject::eventFilter(watched, event);
    }

    if (watchedAnchorChain) {
      if (event->type() == QEvent::Hide) {
        requestCloseActive(PopupCloseReason::OwnerHidden);
      } else if (event->type() == QEvent::Destroy) {
        requestCloseActive(PopupCloseReason::OwnerDestroyed);
      } else if (event->type() == QEvent::ParentChange ||
                 event->type() == QEvent::ParentAboutToChange) {
        scheduleRelayout();
        detail::deferTimingTask(this, QString::fromLatin1(kRefreshAnchorWatchersTaskKey), [this]() {
          refreshAnchorChainWatchers();
          scheduleRelayout();
        });
      } else if (isAnchorGeometryEvent(event->type())) {
        scheduleRelayout();
      }
      return QObject::eventFilter(watched, event);
    }

    return QObject::eventFilter(watched, event);
  }

 private:
  bool watchedInAnchorChain(QObject* watched) const {
    for (const QPointer<QWidget>& widget : watchedAnchorChain_) {
      if (widget && widget.data() == watched) {
        return true;
      }
    }
    return false;
  }

  void scheduleRelayout() {
    if (relayoutQueued_) {
      return;
    }
    relayoutQueued_ = true;
    detail::deferTimingTask(this, QString::fromLatin1(kRelayoutTaskKey), [this]() {
      relayoutQueued_ = false;
      if (!activeOwner_) {
        return;
      }
      refreshAnchorChainWatchers();
      QWidget* anchor = activeOwner_->popupAnchorWidget();
      QWidget* scope = activeOwner_->popupScopeWindow();
      if (!anchor || !scope || !anchor->isVisible() || !scope->isVisible()) {
        requestCloseActive(PopupCloseReason::OwnerHidden);
        return;
      }
      if (activeOwner_->popupIsVisible()) {
        activeOwner_->popupRelayoutFromHost();
      }
    });
  }

  void requestCloseActive(PopupCloseReason reason) {
    if (!activeOwner_ || closingActive_) {
      return;
    }
    closingActive_ = true;
    InWindowPopupOwner* owner = activeOwner_;
    owner->popupCloseFromHost(reason);
    closingActive_ = false;
    if (activeOwner_ == owner) {
      deactivateOwner(owner);
    }
  }

  void removeAllWatchers() {
    for (const QPointer<QWidget>& widget : watchedAnchorChain_) {
      if (widget) {
        widget->removeEventFilter(this);
      }
    }
    watchedAnchorChain_.clear();

    if (scopeWindow_) {
      scopeWindow_->removeEventFilter(this);
    }
    if (qApp) {
      qApp->removeEventFilter(this);
    }
  }

  void refreshAnchorChainWatchers() {
    if (!activeOwner_) {
      return;
    }

    QWidget* anchor = activeOwner_->popupAnchorWidget();
    QWidget* scope = scopeWindow_;
    if (!anchor || !scope) {
      return;
    }

    QVector<QWidget*> nextChain;
    QWidget* cursor = anchor;
    while (cursor && cursor != scope) {
      nextChain.append(cursor);
      cursor = cursor->parentWidget();
    }

    QSet<QWidget*> nextSet;
    nextSet.reserve(nextChain.size());
    for (QWidget* widget : nextChain) {
      nextSet.insert(widget);
    }

    for (const QPointer<QWidget>& widget : watchedAnchorChain_) {
      if (widget && !nextSet.contains(widget.data())) {
        widget->removeEventFilter(this);
      }
    }
    watchedAnchorChain_.clear();
    for (QWidget* widget : nextChain) {
      if (!widget) {
        continue;
      }
      widget->removeEventFilter(this);
      widget->installEventFilter(this);
      watchedAnchorChain_.append(widget);
    }
  }

  void clearActiveOwner() {
    relayoutQueued_ = false;
    if (ownerDestroyedConnection_) {
      QObject::disconnect(ownerDestroyedConnection_);
      ownerDestroyedConnection_ = QMetaObject::Connection();
    }
    activeOwner_ = nullptr;
    activeOwnerObject_.clear();
    activeAnchorWidget_.clear();
  }

  QPointer<QWidget> scopeWindow_;
  InWindowPopupOwner* activeOwner_ = nullptr;
  QPointer<QObject> activeOwnerObject_;
  QPointer<QWidget> activeAnchorWidget_;
  QVector<QPointer<QWidget>> watchedAnchorChain_;
  QMetaObject::Connection ownerDestroyedConnection_;
  bool relayoutQueued_ = false;
  bool closingActive_ = false;
};

QHash<QWidget*, InWindowPopupHost*>& popupHostMap() {
  static QHash<QWidget*, InWindowPopupHost*> hosts;
  return hosts;
}

InWindowPopupHost* hostForScope(QWidget* scopeWindow) {
  if (!scopeWindow) {
    return nullptr;
  }
  return popupHostMap().value(scopeWindow, nullptr);
}

InWindowPopupHost* ensureHostForScope(QWidget* scopeWindow) {
  if (!scopeWindow) {
    return nullptr;
  }
  auto& hosts = popupHostMap();
  if (auto it = hosts.find(scopeWindow); it != hosts.end() && it.value()) {
    return it.value();
  }

  auto* host = new InWindowPopupHost(scopeWindow);
  hosts.insert(scopeWindow, host);
  QObject::connect(scopeWindow, &QObject::destroyed, scopeWindow,
                   [scopeWindow]() { popupHostMap().remove(scopeWindow); });
  return host;
}

InWindowPopupHost* hostForOwner(const InWindowPopupOwner* owner) {
  if (!owner) {
    return nullptr;
  }
  const auto& hosts = popupHostMap();
  for (auto it = hosts.constBegin(); it != hosts.constEnd(); ++it) {
    InWindowPopupHost* host = it.value();
    if (host && host->owns(owner)) {
      return host;
    }
  }
  return nullptr;
}

}  // namespace

void setInWindowPopupHostOpen(InWindowPopupOwner* owner, bool open) {
  if (!owner) {
    return;
  }

  InWindowPopupHost* currentOwnerHost = hostForOwner(owner);
  if (open) {
    QWidget* scopeWindow = owner->popupScopeWindow();
    if (!scopeWindow) {
      return;
    }
    InWindowPopupHost* targetHost = ensureHostForScope(scopeWindow);
    if (!targetHost) {
      return;
    }
    if (currentOwnerHost && currentOwnerHost != targetHost) {
      currentOwnerHost->deactivateOwner(owner);
    }
    targetHost->activateOwner(owner);
    return;
  }

  if (currentOwnerHost) {
    currentOwnerHost->deactivateOwner(owner);
    return;
  }

  QWidget* scopeWindow = owner->popupScopeWindow();
  if (InWindowPopupHost* scopeHost = hostForScope(scopeWindow)) {
    scopeHost->deactivateOwner(owner);
  }
}

}  // namespace adqt::widgets::detail
