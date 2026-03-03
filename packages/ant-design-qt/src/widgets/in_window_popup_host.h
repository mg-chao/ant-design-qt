#pragma once

#include <QPoint>

class QObject;
class QWidget;

namespace adqt::widgets::detail {

enum class PopupCloseReason {
  OutsidePressInScope,
  ScopeHidden,
  ScopeDeactivated,
  OwnerHidden,
  OwnerDestroyed,
  SupersededByAnotherOwner,
  ExplicitClose,
};

class InWindowPopupOwner {
 public:
  virtual ~InWindowPopupOwner() = default;

  virtual QObject* popupOwnerObject() const = 0;
  virtual QWidget* popupAnchorWidget() const = 0;
  virtual QWidget* popupScopeWindow() const = 0;
  virtual bool popupIsVisible() const = 0;
  virtual bool popupContainsGlobalPos(const QPoint& globalPos) const = 0;
  virtual void popupCloseFromHost(PopupCloseReason reason) = 0;
  virtual void popupRelayoutFromHost() = 0;
};

void setInWindowPopupHostOpen(InWindowPopupOwner* owner, bool open);

}  // namespace adqt::widgets::detail

