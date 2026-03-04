#pragma once

#include <QColor>
#include <QFlags>
#include <QPointer>
#include <QSet>
#include <QString>
#include <QWidget>

#include <functional>
#include <optional>

#include "in_window_popup_host.h"

class QEvent;
class QLabel;
class QVBoxLayout;

template <typename T>
size_t qHash(const QPointer<T>& key, size_t seed = 0) noexcept {
  return qHash(key.data(), seed);
}

namespace adqt::widgets {

class AdPopover final : public QWidget, private detail::InWindowPopupOwner {
  Q_OBJECT

  Q_PROPERTY(Placement placement READ placement WRITE setPlacement NOTIFY placementChanged)
  Q_PROPERTY(Triggers triggerModes READ triggerModes WRITE setTriggerModes NOTIFY triggerModesChanged)
  Q_PROPERTY(bool open READ open WRITE setOpen NOTIFY openChanged)
  Q_PROPERTY(bool openControlled READ openControlled WRITE setOpenControlled NOTIFY openControlledChanged)
  Q_PROPERTY(bool defaultOpen READ defaultOpen WRITE setDefaultOpen NOTIFY defaultOpenChanged)
  Q_PROPERTY(bool autoAdjustOverflow READ autoAdjustOverflow WRITE setAutoAdjustOverflow NOTIFY autoAdjustOverflowChanged)
  Q_PROPERTY(bool arrowVisible READ arrowVisible WRITE setArrowVisible NOTIFY arrowVisibleChanged)
  Q_PROPERTY(bool arrowPointAtCenter READ arrowPointAtCenter WRITE setArrowPointAtCenter NOTIFY arrowPointAtCenterChanged)
  Q_PROPERTY(bool destroyOnHidden READ destroyOnHidden WRITE setDestroyOnHidden NOTIFY destroyOnHiddenChanged)
  Q_PROPERTY(bool disabled READ disabled WRITE setDisabled NOTIFY disabledChanged)
  Q_PROPERTY(int mouseEnterDelayMs READ mouseEnterDelayMs WRITE setMouseEnterDelayMs NOTIFY mouseEnterDelayMsChanged)
  Q_PROPERTY(int mouseLeaveDelayMs READ mouseLeaveDelayMs WRITE setMouseLeaveDelayMs NOTIFY mouseLeaveDelayMsChanged)
  Q_PROPERTY(QString titleText READ titleText WRITE setTitleText NOTIFY titleTextChanged)
  Q_PROPERTY(QString contentText READ contentText WRITE setContentText NOTIFY contentTextChanged)

 public:
  enum class Placement {
    Top,
    TopLeft,
    TopRight,
    Bottom,
    BottomLeft,
    BottomRight,
    Left,
    LeftTop,
    LeftBottom,
    Right,
    RightTop,
    RightBottom,
  };
  Q_ENUM(Placement)

  enum class Trigger {
    Hover = 0x1,
    Focus = 0x2,
    Click = 0x4,
    ContextMenu = 0x8,
  };
  Q_ENUM(Trigger)
  Q_DECLARE_FLAGS(Triggers, Trigger)
  Q_FLAG(Triggers)

  struct ComponentTokens {
    std::optional<int> titleMinWidth;
    std::optional<int> zIndexPopup;
    std::optional<int> borderRadius;
    std::optional<int> borderWidth;
    std::optional<int> arrowSize;
    std::optional<int> popupOffset;
    std::optional<int> popupPadding;
    std::optional<int> titlePaddingHorizontal;
    std::optional<int> titlePaddingVertical;
    std::optional<int> titleMarginBottom;
    std::optional<int> contentPaddingHorizontal;
    std::optional<int> contentPaddingVertical;
    std::optional<QString> popupBg;
    std::optional<QString> popupBorderColor;
    std::optional<QString> titleColor;
    std::optional<QString> contentColor;
  };

  struct SemanticSlotStyle {
    std::optional<QColor> textColor;
    std::optional<QColor> backgroundColor;
    std::optional<QColor> borderColor;
  };

  struct SemanticStyles {
    SemanticSlotStyle root;
    SemanticSlotStyle container;
    SemanticSlotStyle title;
    SemanticSlotStyle content;
    SemanticSlotStyle arrow;
  };

  struct StyleContext {
    Placement placement = Placement::Top;
    Triggers triggerModes = Trigger::Hover;
    bool open = false;
    bool disabled = false;
    bool arrowVisible = true;
  };

  using SemanticStyleResolver = std::function<SemanticStyles(const StyleContext&)>;

  explicit AdPopover(QWidget* parent = nullptr);
  ~AdPopover() override;

  Placement placement() const;
  void setPlacement(Placement value);

  Triggers triggerModes() const;
  void setTriggerModes(Triggers value);

  bool open() const;
  void setOpen(bool value);
  bool openControlled() const;
  void setOpenControlled(bool value);

  bool defaultOpen() const;
  void setDefaultOpen(bool value);

  bool autoAdjustOverflow() const;
  void setAutoAdjustOverflow(bool value);

  bool arrowVisible() const;
  void setArrowVisible(bool value);

  bool arrowPointAtCenter() const;
  void setArrowPointAtCenter(bool value);

  bool destroyOnHidden() const;
  void setDestroyOnHidden(bool value);

  bool disabled() const;
  void setDisabled(bool value);

  int mouseEnterDelayMs() const;
  void setMouseEnterDelayMs(int value);

  int mouseLeaveDelayMs() const;
  void setMouseLeaveDelayMs(int value);

  QString titleText() const;
  void setTitleText(const QString& value);

  QString contentText() const;
  void setContentText(const QString& value);

  QWidget* triggerWidget() const;
  void setTriggerWidget(QWidget* widget);

  QWidget* titleWidget() const;
  void setTitleWidget(QWidget* widget);

  QWidget* contentWidget() const;
  void setContentWidget(QWidget* widget);

  ComponentTokens componentTokens() const;
  void setComponentTokens(const ComponentTokens& tokens);
  void resetComponentTokens();

  SemanticStyles semanticStyles() const;
  void setSemanticStyles(const SemanticStyles& styles);
  void setSemanticStyleResolver(SemanticStyleResolver resolver);

 signals:
  void placementChanged(Placement value);
  void triggerModesChanged(Triggers value);
  void openChanged(bool value);
  void onOpenChange(bool value);
  void openControlledChanged(bool value);
  void defaultOpenChanged(bool value);
  void autoAdjustOverflowChanged(bool value);
  void arrowVisibleChanged(bool value);
  void arrowPointAtCenterChanged(bool value);
  void destroyOnHiddenChanged(bool value);
  void disabledChanged(bool value);
  void mouseEnterDelayMsChanged(int value);
  void mouseLeaveDelayMsChanged(int value);
  void titleTextChanged(const QString& value);
  void contentTextChanged(const QString& value);
  void triggerWidgetChanged(QWidget* value);
  void titleWidgetChanged(QWidget* value);
  void contentWidgetChanged(QWidget* value);
  void componentTokensChanged();
  void semanticStylesChanged();

 protected:
  bool eventFilter(QObject* watched, QEvent* event) override;
  void showEvent(QShowEvent* event) override;
  void hideEvent(QHideEvent* event) override;
  void moveEvent(QMoveEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void changeEvent(QEvent* event) override;

 private:
  enum class InternalOpenReason {
    Hover,
    Focus,
    Click,
    ContextMenu,
    Programmatic,
  };

  void ensurePopup();
  void releasePopup();
  void refreshPopupContent();
  void syncPopupGeometry();

  bool hasOverlayContent() const;
  bool hasTrigger(Trigger trigger) const;
  bool shouldBeOpen() const;
  bool isHoveringTriggerTree() const;
  bool isHoveringPopupTree() const;
  void scheduleHoverOpen();
  void scheduleHoverClose();
  void clearHoverTasks();

  void setReasonOpen(InternalOpenReason reason, bool enabled);
  bool reasonOpen(InternalOpenReason reason) const;
  void clearAllOpenReasons();
  void emitControlledOpenRequest(bool requestedOpen);
  void updateOpenState(bool emitSignal);
  void setOpenInternal(bool open, bool emitSignal, bool emitOnOpenChangeSignal = true);

  void refreshTriggerWatchers();
  void clearTriggerWatchers();
  void refreshPopupWatchers();
  void clearPopupWatchers();

  bool watchedByTrigger(QObject* watched) const;
  bool watchedByPopup(QObject* watched) const;

  void handleTriggerPress(QEvent* event);
  void handleTriggerRelease(QEvent* event);
  void handleTriggerKeyPress(QEvent* event);
  void handleTriggerKeyRelease(QEvent* event);
  void handleTriggerContextMenu(QEvent* event);
  void handleTriggerFocusOutDeferred();
  void handleTriggerHoverEnter();
  void handleTriggerHoverLeave();
  void handlePopupHoverEnter();
  void handlePopupHoverLeave();

  void updateStyle();

  QObject* popupOwnerObject() const override;
  QWidget* popupAnchorWidget() const override;
  QWidget* popupScopeWindow() const override;
  bool popupIsVisible() const override;
  bool popupContainsGlobalPos(const QPoint& globalPos) const override;
  void popupCloseFromHost(detail::PopupCloseReason reason) override;
  void popupRelayoutFromHost() override;

  Placement placement_ = Placement::Top;
  Triggers triggerModes_ = Trigger::Hover;
  bool open_ = false;
  bool openControlled_ = false;
  bool defaultOpen_ = false;
  bool defaultOpenApplied_ = false;
  bool explicitOpenSet_ = false;
  bool autoAdjustOverflow_ = true;
  bool arrowVisible_ = true;
  bool arrowPointAtCenter_ = false;
  bool destroyOnHidden_ = false;
  bool disabled_ = false;
  int mouseEnterDelayMs_ = 100;
  int mouseLeaveDelayMs_ = 100;
  QString titleText_;
  QString contentText_;
  ComponentTokens componentTokens_;
  SemanticStyles semanticStyles_;
  SemanticStyleResolver semanticStyleResolver_;

  QVBoxLayout* rootLayout_ = nullptr;
  QPointer<QWidget> triggerWidget_;
  QPointer<QWidget> titleWidget_;
  QPointer<QWidget> contentWidget_;

  QPointer<QWidget> popup_;
  QPointer<QWidget> popupBody_;
  QPointer<QVBoxLayout> popupBodyLayout_;
  QPointer<QWidget> titleContainer_;
  QPointer<QWidget> contentContainer_;
  QPointer<QVBoxLayout> titleContainerLayout_;
  QPointer<QVBoxLayout> contentContainerLayout_;
  QPointer<QLabel> titleLabel_;
  QPointer<QLabel> contentLabel_;

  QPointer<QObject> watchedTriggerRoot_;
  QSet<QPointer<QObject>> watchedTriggerObjects_;
  QSet<QPointer<QObject>> watchedPopupObjects_;

  bool hoverTriggerActive_ = false;
  bool hoverPopupActive_ = false;
  bool focusTriggerActive_ = false;
  bool focusPopupActive_ = false;
  bool openByHover_ = false;
  bool openByFocus_ = false;
  bool openByClick_ = false;
  bool openByContextMenu_ = false;
  bool openByProgrammatic_ = false;
  bool triggerPressActive_ = false;
  bool triggerKeyPressActive_ = false;
  bool suppressOnOpenChangeEmission_ = false;
  bool closingFromHost_ = false;
  bool updatingOpen_ = false;
  std::optional<QPoint> contextMenuGlobalPos_;
};

}  // namespace adqt::widgets

Q_DECLARE_OPERATORS_FOR_FLAGS(adqt::widgets::AdPopover::Triggers)
