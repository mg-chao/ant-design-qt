#pragma once

#include <QColor>
#include <QPointer>
#include <QString>
#include <QWidget>

#include <functional>
#include <optional>

#include "icons_types.h"

class QEvent;
class QGraphicsOpacityEffect;
class QHBoxLayout;
class QLayout;
class QLabel;
class QParallelAnimationGroup;
class QPropertyAnimation;
class QToolButton;
class QVBoxLayout;

namespace adqt::widgets {

class AdAlert final : public QWidget {
  Q_OBJECT

  Q_PROPERTY(Type type READ type WRITE setType NOTIFY typeChanged)
  Q_PROPERTY(bool banner READ banner WRITE setBanner NOTIFY bannerChanged)
  Q_PROPERTY(bool showIcon READ showIcon WRITE setShowIcon NOTIFY showIconChanged)
  Q_PROPERTY(bool closable READ closable WRITE setClosable NOTIFY closableChanged)
  Q_PROPERTY(bool open READ open WRITE setOpen NOTIFY openChanged)
  Q_PROPERTY(QString titleText READ titleText WRITE setTitleText NOTIFY titleTextChanged)
  Q_PROPERTY(QString descriptionText READ descriptionText WRITE setDescriptionText NOTIFY descriptionTextChanged)
  Q_PROPERTY(adqt::icons::IconToken iconToken READ iconToken WRITE setIconToken NOTIFY iconTokenChanged)
  Q_PROPERTY(adqt::icons::IconToken closeIconToken READ closeIconToken WRITE setCloseIconToken NOTIFY closeIconTokenChanged)

 public:
  enum class Type {
    Success,
    Info,
    Warning,
    Error,
  };
  Q_ENUM(Type)

  struct ComponentTokens {
    std::optional<int> defaultPaddingHorizontal;
    std::optional<int> defaultPaddingVertical;
    std::optional<int> withDescriptionIconSize;
    std::optional<int> withDescriptionPadding;
    std::optional<int> iconSize;
    std::optional<int> borderRadius;
    std::optional<int> actionGap;
    std::optional<int> closeGap;
    std::optional<int> closeButtonSize;
  };

  struct SemanticSlotStyle {
    std::optional<QColor> textColor;
    std::optional<QColor> backgroundColor;
    std::optional<QColor> borderColor;
  };

  struct SemanticStyles {
    SemanticSlotStyle root;
    SemanticSlotStyle icon;
    SemanticSlotStyle section;
    SemanticSlotStyle title;
    SemanticSlotStyle description;
    SemanticSlotStyle actions;
    SemanticSlotStyle close;
  };

  struct StyleContext {
    Type type = Type::Info;
    bool banner = false;
    bool showIcon = false;
    bool closable = false;
    bool hasDescription = false;
    bool open = true;
  };

  using SemanticStyleResolver = std::function<SemanticStyles(const StyleContext&)>;

  explicit AdAlert(QWidget* parent = nullptr);
  ~AdAlert() override;

  Type type() const;
  void setType(Type value);

  bool banner() const;
  void setBanner(bool value);

  bool showIcon() const;
  void setShowIcon(bool value);

  bool closable() const;
  void setClosable(bool value);

  bool open() const;
  void setOpen(bool value);

  QString titleText() const;
  void setTitleText(const QString& value);

  QString descriptionText() const;
  void setDescriptionText(const QString& value);

  adqt::icons::IconToken iconToken() const;
  void setIconToken(const adqt::icons::IconToken& value);

  adqt::icons::IconToken closeIconToken() const;
  void setCloseIconToken(const adqt::icons::IconToken& value);

  QWidget* titleWidget() const;
  void setTitleWidget(QWidget* widget);

  QWidget* descriptionWidget() const;
  void setDescriptionWidget(QWidget* widget);

  QWidget* actionWidget() const;
  void setActionWidget(QWidget* widget);

  ComponentTokens componentTokens() const;
  void setComponentTokens(const ComponentTokens& tokens);
  void resetComponentTokens();

  SemanticStyles semanticStyles() const;
  void setSemanticStyles(const SemanticStyles& styles);
  void setSemanticStyleResolver(SemanticStyleResolver resolver);

 signals:
  void typeChanged(Type value);
  void bannerChanged(bool value);
  void showIconChanged(bool value);
  void closableChanged(bool value);
  void openChanged(bool value);
  void titleTextChanged(const QString& value);
  void descriptionTextChanged(const QString& value);
  void iconTokenChanged(const adqt::icons::IconToken& value);
  void closeIconTokenChanged(const adqt::icons::IconToken& value);
  void titleWidgetChanged(QWidget* value);
  void descriptionWidgetChanged(QWidget* value);
  void actionWidgetChanged(QWidget* value);
  void componentTokensChanged();
  void semanticStylesChanged();
  void closeRequested();
  void afterClose();

 protected:
  bool eventFilter(QObject* watched, QEvent* event) override;
  void changeEvent(QEvent* event) override;

 private:
  struct DerivedState {
    Type type = Type::Info;
    bool showIcon = false;
    bool hasDescription = false;
  };

  static bool iconTokensEqual(const adqt::icons::IconToken& lhs, const adqt::icons::IconToken& rhs);

  DerivedState deriveState() const;
  SemanticStyles resolvedSemanticStyles() const;
  adqt::icons::IconToken resolvedIconToken() const;
  adqt::icons::IconToken resolvedCloseIconToken() const;

  void ensureLayout();
  void refreshContent();
  void applyVisualStyle();
  void refreshOpenState();
  void startCloseAnimation(int durationMs);
  void finishCloseAnimation(bool emitAfterCloseSignal);
  void updateCloseButtonIcon();
  static void clearLayout(QLayout* layout);

  bool typeExplicit_ = false;
  Type typeValue_ = Type::Info;
  bool banner_ = false;
  bool showIconExplicit_ = false;
  bool showIconValue_ = false;
  bool closable_ = false;
  bool open_ = true;
  QString titleText_;
  QString descriptionText_;
  adqt::icons::IconToken iconToken_;
  adqt::icons::IconToken closeIconToken_;
  ComponentTokens componentTokens_;
  SemanticStyles semanticStyles_;
  SemanticStyleResolver semanticStyleResolver_;

  QPointer<QHBoxLayout> rootLayout_;
  QPointer<QLabel> iconLabel_;
  QPointer<QWidget> sectionHost_;
  QPointer<QVBoxLayout> sectionLayout_;
  QPointer<QWidget> titleHost_;
  QPointer<QVBoxLayout> titleLayout_;
  QPointer<QLabel> titleLabel_;
  QPointer<QWidget> descriptionHost_;
  QPointer<QVBoxLayout> descriptionLayout_;
  QPointer<QLabel> descriptionLabel_;
  QPointer<QWidget> actionHost_;
  QPointer<QVBoxLayout> actionLayout_;
  QPointer<QToolButton> closeButton_;

  QPointer<QWidget> titleWidget_;
  QPointer<QWidget> descriptionWidget_;
  QPointer<QWidget> actionWidget_;

  QPointer<QGraphicsOpacityEffect> opacityEffect_;
  QPointer<QParallelAnimationGroup> closeAnimation_;
  QPointer<QPropertyAnimation> heightAnimation_;
  QPointer<QPropertyAnimation> opacityAnimation_;
  bool closeButtonHovered_ = false;
};

}  // namespace adqt::widgets
