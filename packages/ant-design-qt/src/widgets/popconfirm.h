#pragma once

#include <QColor>
#include <QFlags>
#include <QPointer>
#include <QSet>
#include <QString>
#include <QWidget>

#include <functional>
#include <optional>

#include "button.h"
#include "popover.h"
#include "icons_types.h"

class QEvent;
class QLabel;
class QVBoxLayout;
class QHBoxLayout;

namespace adqt::widgets {

class AdPopconfirm final : public QWidget {
  Q_OBJECT

  Q_PROPERTY(Placement placement READ placement WRITE setPlacement NOTIFY placementChanged)
  Q_PROPERTY(Triggers triggerModes READ triggerModes WRITE setTriggerModes NOTIFY triggerModesChanged)
  Q_PROPERTY(bool open READ open WRITE setOpen NOTIFY openChanged)
  Q_PROPERTY(bool openControlled READ openControlled WRITE setOpenControlled NOTIFY openControlledChanged)
  Q_PROPERTY(bool defaultOpen READ defaultOpen WRITE setDefaultOpen NOTIFY defaultOpenChanged)
  Q_PROPERTY(bool autoAdjustOverflow READ autoAdjustOverflow WRITE setAutoAdjustOverflow
                 NOTIFY autoAdjustOverflowChanged)
  Q_PROPERTY(bool arrowVisible READ arrowVisible WRITE setArrowVisible NOTIFY arrowVisibleChanged)
  Q_PROPERTY(bool arrowPointAtCenter READ arrowPointAtCenter WRITE setArrowPointAtCenter
                 NOTIFY arrowPointAtCenterChanged)
  Q_PROPERTY(bool destroyOnHidden READ destroyOnHidden WRITE setDestroyOnHidden NOTIFY destroyOnHiddenChanged)
  Q_PROPERTY(bool disabled READ disabled WRITE setDisabled NOTIFY disabledChanged)
  Q_PROPERTY(int mouseEnterDelayMs READ mouseEnterDelayMs WRITE setMouseEnterDelayMs
                 NOTIFY mouseEnterDelayMsChanged)
  Q_PROPERTY(int mouseLeaveDelayMs READ mouseLeaveDelayMs WRITE setMouseLeaveDelayMs
                 NOTIFY mouseLeaveDelayMsChanged)
  Q_PROPERTY(QString titleText READ titleText WRITE setTitleText NOTIFY titleTextChanged)
  Q_PROPERTY(QString descriptionText READ descriptionText WRITE setDescriptionText
                 NOTIFY descriptionTextChanged)
  Q_PROPERTY(QString okText READ okText WRITE setOkText NOTIFY okTextChanged)
  Q_PROPERTY(QString cancelText READ cancelText WRITE setCancelText NOTIFY cancelTextChanged)
  Q_PROPERTY(bool showCancel READ showCancel WRITE setShowCancel NOTIFY showCancelChanged)
  Q_PROPERTY(bool confirmAutoClose READ confirmAutoClose WRITE setConfirmAutoClose
                 NOTIFY confirmAutoCloseChanged)
  Q_PROPERTY(bool iconVisible READ iconVisible WRITE setIconVisible NOTIFY iconVisibleChanged)
  Q_PROPERTY(adqt::icons::IconToken iconToken READ iconToken WRITE setIconToken NOTIFY iconTokenChanged)
  Q_PROPERTY(adqt::widgets::AdButton::Type okType READ okType WRITE setOkType NOTIFY okTypeChanged)
  Q_PROPERTY(bool okButtonLoading READ okButtonLoading WRITE setOkButtonLoading
                 NOTIFY okButtonLoadingChanged)

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
    std::optional<int> messageGap;
    std::optional<int> messageBottom;
    std::optional<int> descriptionGap;
    std::optional<int> buttonGap;
    std::optional<int> iconSize;
    std::optional<QString> iconColor;
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
    SemanticSlotStyle description;
    SemanticSlotStyle icon;
    SemanticSlotStyle arrow;
  };

  struct StyleContext {
    Placement placement = Placement::Top;
    Triggers triggerModes = Trigger::Click;
    bool open = false;
    bool disabled = false;
    bool arrowVisible = true;
  };

  using SemanticStyleResolver = std::function<SemanticStyles(const StyleContext&)>;

  explicit AdPopconfirm(QWidget* parent = nullptr);
  ~AdPopconfirm() override;

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

  QString descriptionText() const;
  void setDescriptionText(const QString& value);

  QString okText() const;
  void setOkText(const QString& value);

  QString cancelText() const;
  void setCancelText(const QString& value);

  bool showCancel() const;
  void setShowCancel(bool value);

  bool confirmAutoClose() const;
  void setConfirmAutoClose(bool value);

  bool iconVisible() const;
  void setIconVisible(bool value);

  adqt::icons::IconToken iconToken() const;
  void setIconToken(const adqt::icons::IconToken& value);

  AdButton::Type okType() const;
  void setOkType(AdButton::Type value);

  bool okButtonLoading() const;
  void setOkButtonLoading(bool value);

  QWidget* triggerWidget() const;
  void setTriggerWidget(QWidget* widget);

  AdButton* okButton() const;
  AdButton* cancelButton() const;

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
  void descriptionTextChanged(const QString& value);
  void okTextChanged(const QString& value);
  void cancelTextChanged(const QString& value);
  void showCancelChanged(bool value);
  void confirmAutoCloseChanged(bool value);
  void iconVisibleChanged(bool value);
  void iconTokenChanged(const adqt::icons::IconToken& value);
  void okTypeChanged(AdButton::Type value);
  void okButtonLoadingChanged(bool value);
  void triggerWidgetChanged(QWidget* value);
  void componentTokensChanged();
  void semanticStylesChanged();
  void confirmed();
  void canceled();
  void popupClicked();

 protected:
  bool eventFilter(QObject* watched, QEvent* event) override;
  void changeEvent(QEvent* event) override;

 private:
  struct DerivedVisualStyle {
    int titleMinWidth = 177;
    int zIndexPopup = 1060;
    int messageGap = 8;
    int messageBottom = 8;
    int descriptionGap = 4;
    int buttonGap = 8;
    int iconSize = 14;
    QFont titleFont;
    QFont titleOnlyFont;
    QFont descriptionFont;
    QColor titleColor = QColor("#141414");
    QColor descriptionColor = QColor("#141414");
    QColor iconColor = QColor("#faad14");
    AdPopover::SemanticStyles popoverSemantic;
  };

  static AdPopover::Placement toPopoverPlacement(Placement value);
  static Placement fromPopoverPlacement(AdPopover::Placement value);
  static AdPopover::Triggers toPopoverTriggers(Triggers value);
  static Triggers fromPopoverTriggers(AdPopover::Triggers value);
  static AdPopover::SemanticSlotStyle toPopoverSemanticSlot(const SemanticSlotStyle& slot);

  void ensureContentHost();
  void syncContentWidget();
  void refreshVisualStyle();
  DerivedVisualStyle deriveVisualStyle() const;
  void refreshOverlayWatchers();
  void clearOverlayWatchers();
  void requestCloseAfterAction();

  QPointer<AdPopover> popover_;
  QPointer<QWidget> triggerWidget_;

  QPointer<QWidget> contentHost_;
  QPointer<QVBoxLayout> contentLayout_;
  QPointer<QWidget> messageHost_;
  QPointer<QHBoxLayout> messageLayout_;
  QPointer<QLabel> iconLabel_;
  QPointer<QWidget> textHost_;
  QPointer<QVBoxLayout> textLayout_;
  QPointer<QLabel> titleLabel_;
  QPointer<QLabel> descriptionLabel_;
  QPointer<QWidget> buttonsHost_;
  QPointer<QHBoxLayout> buttonsLayout_;
  QPointer<QWidget> buttonsLeadSpacer_;
  QPointer<QWidget> buttonsInnerSpacer_;
  QPointer<AdButton> cancelButton_;
  QPointer<AdButton> okButton_;

  QSet<QPointer<QObject>> watchedOverlayObjects_;

  QString titleText_;
  QString descriptionText_;
  QString okText_ = QStringLiteral("OK");
  QString cancelText_ = QStringLiteral("Cancel");
  bool showCancel_ = true;
  bool confirmAutoClose_ = true;
  bool iconVisible_ = true;
  adqt::icons::IconToken iconToken_;
  AdButton::Type okType_ = AdButton::Type::Primary;
  bool okButtonLoading_ = false;

  ComponentTokens componentTokens_;
  SemanticStyles semanticStyles_;
  SemanticStyleResolver semanticStyleResolver_;
};

}  // namespace adqt::widgets

Q_DECLARE_OPERATORS_FOR_FLAGS(adqt::widgets::AdPopconfirm::Triggers)
