#pragma once

#include <QColor>
#include <QPointer>
#include <QString>
#include <QWidget>

#include <functional>
#include <optional>

#include "button.h"

class QEvent;
class QFrame;
class QHBoxLayout;
class QLabel;
class QShortcut;
class QToolButton;
class QVBoxLayout;

namespace adqt::widgets {

class AdModal final : public QWidget {
  Q_OBJECT

  Q_PROPERTY(bool open READ open WRITE setOpen NOTIFY openChanged)
  Q_PROPERTY(bool centered READ centered WRITE setCentered NOTIFY centeredChanged)
  Q_PROPERTY(int width READ width WRITE setWidth NOTIFY widthChanged)
  Q_PROPERTY(int top READ top WRITE setTop NOTIFY topChanged)
  Q_PROPERTY(bool mask READ mask WRITE setMask NOTIFY maskChanged)
  Q_PROPERTY(bool maskClosable READ maskClosable WRITE setMaskClosable NOTIFY maskClosableChanged)
  Q_PROPERTY(bool keyboard READ keyboard WRITE setKeyboard NOTIFY keyboardChanged)
  Q_PROPERTY(bool closable READ closable WRITE setClosable NOTIFY closableChanged)
  Q_PROPERTY(bool destroyOnHidden READ destroyOnHidden WRITE setDestroyOnHidden
                 NOTIFY destroyOnHiddenChanged)
  Q_PROPERTY(bool footerVisible READ footerVisible WRITE setFooterVisible NOTIFY footerVisibleChanged)
  Q_PROPERTY(bool showCancel READ showCancel WRITE setShowCancel NOTIFY showCancelChanged)
  Q_PROPERTY(bool okAutoClose READ okAutoClose WRITE setOkAutoClose NOTIFY okAutoCloseChanged)
  Q_PROPERTY(bool cancelAutoClose READ cancelAutoClose WRITE setCancelAutoClose
                 NOTIFY cancelAutoCloseChanged)
  Q_PROPERTY(bool confirmLoading READ confirmLoading WRITE setConfirmLoading NOTIFY confirmLoadingChanged)
  Q_PROPERTY(bool loading READ loading WRITE setLoading NOTIFY loadingChanged)
  Q_PROPERTY(QString titleText READ titleText WRITE setTitleText NOTIFY titleTextChanged)
  Q_PROPERTY(QString contentText READ contentText WRITE setContentText NOTIFY contentTextChanged)
  Q_PROPERTY(QString okText READ okText WRITE setOkText NOTIFY okTextChanged)
  Q_PROPERTY(QString cancelText READ cancelText WRITE setCancelText NOTIFY cancelTextChanged)
  Q_PROPERTY(adqt::widgets::AdButton::Type okType READ okType WRITE setOkType NOTIFY okTypeChanged)
  Q_PROPERTY(Type type READ type WRITE setType NOTIFY typeChanged)

 public:
  enum class Type {
    Normal,
    Info,
    Success,
    Error,
    Warning,
    Confirm,
  };
  Q_ENUM(Type)

  struct ComponentTokens {
    std::optional<int> width;
    std::optional<int> zIndexPopup;
    std::optional<int> borderRadius;
    std::optional<int> borderWidth;
    std::optional<int> headerPaddingHorizontal;
    std::optional<int> headerPaddingVertical;
    std::optional<int> bodyPaddingHorizontal;
    std::optional<int> bodyPaddingVertical;
    std::optional<int> footerPaddingHorizontal;
    std::optional<int> footerPaddingVertical;
    std::optional<int> footerButtonGap;
    std::optional<int> iconSize;
    std::optional<QString> maskBg;
    std::optional<QString> contentBg;
    std::optional<QString> headerBg;
    std::optional<QString> bodyBg;
    std::optional<QString> footerBg;
    std::optional<QString> borderColor;
    std::optional<QString> titleColor;
    std::optional<QString> bodyColor;
    std::optional<QString> iconColor;
    std::optional<QString> closeIconColor;
  };

  struct SemanticSlotStyle {
    std::optional<QColor> textColor;
    std::optional<QColor> backgroundColor;
    std::optional<QColor> borderColor;
  };

  struct SemanticStyles {
    SemanticSlotStyle root;
    SemanticSlotStyle mask;
    SemanticSlotStyle container;
    SemanticSlotStyle header;
    SemanticSlotStyle title;
    SemanticSlotStyle body;
    SemanticSlotStyle footer;
    SemanticSlotStyle icon;
    SemanticSlotStyle close;
  };

  struct StyleContext {
    bool open = false;
    bool centered = false;
    bool loading = false;
    bool confirmLoading = false;
    bool mask = true;
    bool closable = true;
    bool showCancel = true;
    Type type = Type::Normal;
  };

  using SemanticStyleResolver = std::function<SemanticStyles(const StyleContext&)>;
  using ActionHandler = std::function<bool(AdModal*)>;

  struct StaticConfig {
    std::optional<Type> type;
    std::optional<QString> titleText;
    std::optional<QString> contentText;
    std::optional<QString> okText;
    std::optional<QString> cancelText;
    std::optional<bool> showCancel;
    std::optional<bool> centered;
    std::optional<bool> closable;
    std::optional<bool> mask;
    std::optional<bool> maskClosable;
    std::optional<bool> keyboard;
    std::optional<bool> footerVisible;
    std::optional<bool> confirmLoading;
    std::optional<bool> loading;
    std::optional<int> width;
    std::optional<int> top;
    std::optional<AdButton::Type> okType;
    ActionHandler onOk;
    ActionHandler onCancel;
  };

  explicit AdModal(QWidget* parent = nullptr);
  ~AdModal() override;

  bool open() const;
  void setOpen(bool value);

  bool centered() const;
  void setCentered(bool value);

  int width() const;
  void setWidth(int value);

  int top() const;
  void setTop(int value);

  bool mask() const;
  void setMask(bool value);

  bool maskClosable() const;
  void setMaskClosable(bool value);

  bool keyboard() const;
  void setKeyboard(bool value);

  bool closable() const;
  void setClosable(bool value);

  bool destroyOnHidden() const;
  void setDestroyOnHidden(bool value);

  bool footerVisible() const;
  void setFooterVisible(bool value);

  bool showCancel() const;
  void setShowCancel(bool value);

  bool okAutoClose() const;
  void setOkAutoClose(bool value);

  bool cancelAutoClose() const;
  void setCancelAutoClose(bool value);

  bool confirmLoading() const;
  void setConfirmLoading(bool value);

  bool loading() const;
  void setLoading(bool value);

  QString titleText() const;
  void setTitleText(const QString& value);

  QString contentText() const;
  void setContentText(const QString& value);

  QString okText() const;
  void setOkText(const QString& value);

  QString cancelText() const;
  void setCancelText(const QString& value);

  AdButton::Type okType() const;
  void setOkType(AdButton::Type value);

  Type type() const;
  void setType(Type value);

  QWidget* scopeWindow() const;
  void setScopeWindow(QWidget* value);

  QWidget* bodyWidget() const;
  void setBodyWidget(QWidget* widget);

  QWidget* footerWidget() const;
  void setFooterWidget(QWidget* widget);

  ActionHandler onOkHandler() const;
  void setOnOkHandler(ActionHandler handler);

  ActionHandler onCancelHandler() const;
  void setOnCancelHandler(ActionHandler handler);

  ComponentTokens componentTokens() const;
  void setComponentTokens(const ComponentTokens& tokens);
  void resetComponentTokens();

  SemanticStyles semanticStyles() const;
  void setSemanticStyles(const SemanticStyles& styles);
  void setSemanticStyleResolver(SemanticStyleResolver resolver);

  AdButton* okButton() const;
  AdButton* cancelButton() const;
  QToolButton* closeButton() const;

  void destroy();

  static AdModal* info(const StaticConfig& config = {}, QWidget* scopeWindow = nullptr);
  static AdModal* success(const StaticConfig& config = {}, QWidget* scopeWindow = nullptr);
  static AdModal* error(const StaticConfig& config = {}, QWidget* scopeWindow = nullptr);
  static AdModal* warning(const StaticConfig& config = {}, QWidget* scopeWindow = nullptr);
  static AdModal* confirm(const StaticConfig& config = {}, QWidget* scopeWindow = nullptr);
  static void destroyAll();

 signals:
  void openChanged(bool value);
  void onOpenChange(bool value);
  void afterOpenChange(bool value);
  void afterClose();
  void centeredChanged(bool value);
  void widthChanged(int value);
  void topChanged(int value);
  void maskChanged(bool value);
  void maskClosableChanged(bool value);
  void keyboardChanged(bool value);
  void closableChanged(bool value);
  void destroyOnHiddenChanged(bool value);
  void footerVisibleChanged(bool value);
  void showCancelChanged(bool value);
  void okAutoCloseChanged(bool value);
  void cancelAutoCloseChanged(bool value);
  void confirmLoadingChanged(bool value);
  void loadingChanged(bool value);
  void titleTextChanged(const QString& value);
  void contentTextChanged(const QString& value);
  void okTextChanged(const QString& value);
  void cancelTextChanged(const QString& value);
  void okTypeChanged(adqt::widgets::AdButton::Type value);
  void typeChanged(Type value);
  void scopeWindowChanged(QWidget* value);
  void bodyWidgetChanged(QWidget* value);
  void footerWidgetChanged(QWidget* value);
  void componentTokensChanged();
  void semanticStylesChanged();
  void okTriggered();
  void cancelTriggered();

 protected:
  bool eventFilter(QObject* watched, QEvent* event) override;
  void changeEvent(QEvent* event) override;

 private:
  struct VisualStyle {
    QColor rootBg = QColor(0, 0, 0, 0);
    QColor maskBg = QColor(0, 0, 0, 115);
    QColor containerBg = QColor("#ffffff");
    QColor headerBg = QColor("#ffffff");
    QColor bodyBg = QColor("#ffffff");
    QColor footerBg = QColor("#ffffff");
    QColor borderColor = QColor("#f0f0f0");
    QColor titleColor = QColor("#141414");
    QColor bodyColor = QColor("#141414");
    QColor iconColor = QColor("#1677ff");
    QColor closeIconColor = QColor("#8c8c8c");
    int zIndex = 1000;
    int width = 520;
    int borderRadius = 8;
    int borderWidth = 0;
    int contentPaddingHorizontal = 24;
    int contentPaddingVertical = 20;
    int headerPaddingHorizontal = 0;
    int headerPaddingVertical = 0;
    int headerMarginBottom = 8;
    int bodyPaddingHorizontal = 0;
    int bodyPaddingVertical = 0;
    int footerPaddingHorizontal = 0;
    int footerPaddingVertical = 0;
    int footerMarginTop = 12;
    int footerBorderTopWidth = 0;
    int footerButtonGap = 8;
    int confirmIconGap = 12;
    int confirmParagraphGap = 8;
    int textLineHeight = 22;
    int titleLineHeight = 24;
    int iconSize = 16;
    int closeButtonSize = 32;
    int closeIconSize = 16;
    QFont titleFont;
    QFont bodyFont;
  };

  static AdModal* showStatic(const StaticConfig& config, Type defaultType, QWidget* scopeWindow);
  static void registerStaticModal(AdModal* modal);
  static void unregisterStaticModal(AdModal* modal);

  void attachScopeWindowWatcher(QWidget* scope);
  void detachScopeWindowWatcher();
  void installGlobalWatcher(bool enabled);

  QWidget* resolveScopeWindow() const;

  void ensureOverlay();
  void releaseOverlay();
  void syncOverlayGeometry();
  void refreshLayout();
  void refreshTexts();
  void refreshVisibility();
  void refreshTitleIcon();
  void applyVisualStyle();
  VisualStyle resolveVisualStyle() const;

  void setOpenInternal(bool value, bool emitSignal, bool emitOnOpenSignal = true);
  void requestOk();
  void requestCancel();

  static QColor parseColorToken(const std::optional<QString>& token, const QColor& fallback);
  static QString toRgba(const QColor& color);
  static void applySemanticSlot(const SemanticSlotStyle& slot,
                                QColor* textColor,
                                QColor* backgroundColor,
                                QColor* borderColor);

  bool open_ = false;
  bool centered_ = false;
  int width_ = 520;
  int top_ = 100;
  bool mask_ = true;
  bool maskClosable_ = true;
  bool keyboard_ = true;
  bool closable_ = true;
  bool destroyOnHidden_ = false;
  bool footerVisible_ = true;
  bool showCancel_ = true;
  bool okAutoClose_ = true;
  bool cancelAutoClose_ = true;
  bool confirmLoading_ = false;
  bool loading_ = false;
  QString titleText_;
  QString contentText_;
  QString okText_ = QStringLiteral("OK");
  QString cancelText_ = QStringLiteral("Cancel");
  AdButton::Type okType_ = AdButton::Type::Primary;
  Type type_ = Type::Normal;
  ComponentTokens componentTokens_;
  SemanticStyles semanticStyles_;
  SemanticStyleResolver semanticStyleResolver_;
  ActionHandler onOkHandler_;
  ActionHandler onCancelHandler_;

  QPointer<QWidget> scopeWindow_;
  QPointer<QWidget> overlay_;
  QPointer<QVBoxLayout> overlayLayout_;
  QPointer<QFrame> panel_;
  QPointer<QVBoxLayout> panelLayout_;
  QPointer<QWidget> header_;
  QPointer<QHBoxLayout> headerLayout_;
  QPointer<QLabel> titleIconLabel_;
  QPointer<QLabel> titleLabel_;
  QPointer<QToolButton> closeButton_;
  QPointer<QWidget> body_;
  QPointer<QVBoxLayout> bodyLayout_;
  QPointer<QWidget> confirmBodyHost_;
  QPointer<QHBoxLayout> confirmBodyLayout_;
  QPointer<QVBoxLayout> confirmParagraphLayout_;
  QPointer<QLabel> contentLabel_;
  QPointer<QLabel> confirmTitleLabel_;
  QPointer<QLabel> confirmContentLabel_;
  QPointer<QWidget> bodyWidget_;
  QPointer<QWidget> footer_;
  QPointer<QHBoxLayout> footerLayout_;
  QPointer<QWidget> footerButtonsHost_;
  QPointer<QHBoxLayout> footerButtonsLayout_;
  QPointer<AdButton> cancelButton_;
  QPointer<AdButton> okButton_;
  QPointer<QWidget> footerWidget_;
  QPointer<QShortcut> escShortcut_;

  bool globalWatcherInstalled_ = false;
  bool internalOpenUpdate_ = false;
  bool staticInstance_ = false;
};

}  // namespace adqt::widgets
