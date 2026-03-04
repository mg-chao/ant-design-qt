#pragma once

#include <QColor>
#include <QFlags>
#include <QPointer>
#include <QString>
#include <QWidget>

#include <functional>
#include <optional>

#include "popover.h"

class QEvent;
class QLabel;
class QVBoxLayout;

namespace adqt::widgets {

class AdTooltip final : public QWidget {
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
  Q_PROPERTY(QString color READ color WRITE setColor NOTIFY colorChanged)

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
    std::optional<int> maxWidth;
    std::optional<int> borderRadius;
    std::optional<int> arrowSize;
    std::optional<int> popupOffset;
    std::optional<int> paddingHorizontal;
    std::optional<int> paddingVertical;
    std::optional<QString> popupBg;
    std::optional<QString> textColor;
  };

  struct SemanticSlotStyle {
    std::optional<QColor> textColor;
    std::optional<QColor> backgroundColor;
    std::optional<QColor> borderColor;
  };

  struct SemanticStyles {
    SemanticSlotStyle root;
    SemanticSlotStyle container;
    SemanticSlotStyle body;
    SemanticSlotStyle arrow;
  };

  struct StyleContext {
    Placement placement = Placement::Top;
    Triggers triggerModes = Trigger::Hover;
    bool open = false;
    bool disabled = false;
    bool arrowVisible = true;
    QString color;
  };

  using SemanticStyleResolver = std::function<SemanticStyles(const StyleContext&)>;

  explicit AdTooltip(QWidget* parent = nullptr);
  ~AdTooltip() override;

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

  QString color() const;
  void setColor(const QString& value);

  QWidget* triggerWidget() const;
  void setTriggerWidget(QWidget* widget);

  QWidget* titleWidget() const;
  void setTitleWidget(QWidget* widget);

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
  void colorChanged(const QString& value);
  void triggerWidgetChanged(QWidget* value);
  void titleWidgetChanged(QWidget* value);
  void componentTokensChanged();
  void semanticStylesChanged();

 protected:
  void changeEvent(QEvent* event) override;

 private:
  struct DerivedVisualStyle {
    int maxWidth = 250;
    int borderRadius = 8;
    int arrowSize = 8;
    int popupOffset = 8;
    int paddingHorizontal = 8;
    int paddingVertical = 4;
    QColor popupBg = QColor("#141414");
    QColor textColor = QColor("#ffffff");
  };

  static AdPopover::Placement toPopoverPlacement(Placement value);
  static Placement fromPopoverPlacement(AdPopover::Placement value);
  static AdPopover::Triggers toPopoverTriggers(Triggers value);
  static Triggers fromPopoverTriggers(AdPopover::Triggers value);
  static AdPopover::SemanticSlotStyle toPopoverSemanticSlot(const SemanticSlotStyle& slot);

  void ensureContentHost();
  void clearContentHostLayout();
  void syncContentWidget();
  void refreshVisualStyle();
  DerivedVisualStyle deriveVisualStyle() const;
  std::optional<QColor> resolveColorValue(const QString& value) const;
  static QColor textColorForBackground(const QColor& background);
  static QString colorToTokenString(const QColor& color);

  QPointer<AdPopover> popover_;
  QPointer<QWidget> triggerWidget_;
  QPointer<QWidget> titleWidget_;
  QPointer<QWidget> contentHost_;
  QPointer<QVBoxLayout> contentHostLayout_;
  QPointer<QLabel> contentLabel_;

  QString titleText_;
  QString color_;
  ComponentTokens componentTokens_;
  SemanticStyles semanticStyles_;
  SemanticStyleResolver semanticStyleResolver_;
};

}  // namespace adqt::widgets

Q_DECLARE_OPERATORS_FOR_FLAGS(adqt::widgets::AdTooltip::Triggers)
