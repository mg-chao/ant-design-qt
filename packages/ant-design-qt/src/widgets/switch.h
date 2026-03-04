#pragma once

#include <QAbstractButton>
#include <QColor>
#include <QString>

#include <functional>
#include <optional>

#include "icons_types.h"

namespace adqt::widgets {

class AdSwitch final : public QAbstractButton {
  Q_OBJECT

  Q_PROPERTY(bool checked READ isChecked WRITE setChecked NOTIFY checkedChanged)
  Q_PROPERTY(bool value READ value WRITE setValue NOTIFY valueChanged)
  Q_PROPERTY(bool disabled READ disabled WRITE setDisabled NOTIFY disabledChanged)
  Q_PROPERTY(bool loading READ loading WRITE setLoading NOTIFY loadingChanged)
  Q_PROPERTY(Size size READ size WRITE setSize NOTIFY sizeChanged)
  Q_PROPERTY(QString checkedChildren READ checkedChildren WRITE setCheckedChildren NOTIFY checkedChildrenChanged)
  Q_PROPERTY(QString unCheckedChildren READ unCheckedChildren WRITE setUnCheckedChildren
                 NOTIFY unCheckedChildrenChanged)

 public:
  enum class Size {
    Default,
    Small,
  };
  Q_ENUM(Size)

  struct ComponentTokens {
    std::optional<int> trackHeight;
    std::optional<int> trackHeightSM;
    std::optional<int> trackMinWidth;
    std::optional<int> trackMinWidthSM;
    std::optional<int> trackPadding;
    std::optional<QString> handleBg;
    std::optional<QString> handleShadow;
    std::optional<int> handleSize;
    std::optional<int> handleSizeSM;
    std::optional<int> innerMinMargin;
    std::optional<int> innerMaxMargin;
    std::optional<int> innerMinMarginSM;
    std::optional<int> innerMaxMarginSM;
    std::optional<QString> colorPrimary;
    std::optional<int> loadingIconSize;
    std::optional<QString> loadingIconColor;
    std::optional<double> disabledOpacity;
  };

  struct SemanticSlotStyle {
    std::optional<QColor> textColor;
    std::optional<QColor> backgroundColor;
    std::optional<QColor> borderColor;
  };

  struct SemanticStyles {
    SemanticSlotStyle root;
    SemanticSlotStyle content;
    SemanticSlotStyle indicator;
  };

  struct StyleContext {
    Size size = Size::Default;
    bool checked = false;
    bool loading = false;
    bool disabled = false;
    bool hovered = false;
    bool pressed = false;
    bool focused = false;
  };

  using SemanticStyleResolver = std::function<SemanticStyles(const StyleContext&)>;

  explicit AdSwitch(QWidget* parent = nullptr);
  ~AdSwitch() override;

  bool value() const;
  void setValue(bool value);

  bool disabled() const;
  void setDisabled(bool value);

  bool loading() const;
  void setLoading(bool value);

  Size size() const;
  void setSize(Size value);

  QString checkedChildren() const;
  void setCheckedChildren(const QString& value);

  QString unCheckedChildren() const;
  void setUnCheckedChildren(const QString& value);

  adqt::icons::IconToken checkedChildrenIconToken() const;
  void setCheckedChildrenIconToken(const adqt::icons::IconToken& value);

  adqt::icons::IconToken unCheckedChildrenIconToken() const;
  void setUnCheckedChildrenIconToken(const adqt::icons::IconToken& value);

  ComponentTokens componentTokens() const;
  void setComponentTokens(const ComponentTokens& tokens);
  void resetComponentTokens();

  SemanticStyles semanticStyles() const;
  void setSemanticStyles(const SemanticStyles& styles);
  void setSemanticStyleResolver(SemanticStyleResolver resolver);

  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;

 signals:
  void checkedChanged(bool value);
  void valueChanged(bool value);
  void disabledChanged(bool value);
  void loadingChanged(bool value);
  void sizeChanged(Size value);
  void checkedChildrenChanged(const QString& value);
  void unCheckedChildrenChanged(const QString& value);
  void checkedChildrenIconTokenChanged(const adqt::icons::IconToken& value);
  void unCheckedChildrenIconTokenChanged(const adqt::icons::IconToken& value);
  void componentTokensChanged();
  void semanticStylesChanged();
  void changed(bool value);

 protected:
  void paintEvent(QPaintEvent* event) override;
  void nextCheckState() override;
  void enterEvent(QEnterEvent* event) override;
  void leaveEvent(QEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
  void keyReleaseEvent(QKeyEvent* event) override;
  void focusInEvent(QFocusEvent* event) override;
  void focusOutEvent(QFocusEvent* event) override;
  void moveEvent(QMoveEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void showEvent(QShowEvent* event) override;
  void hideEvent(QHideEvent* event) override;
  void changeEvent(QEvent* event) override;

 private:
  bool effectiveDisabled() const;
  SemanticStyles resolvedSemanticStyles() const;
  void refreshAfterPropertyChange(bool updateGeometryHint = true);
  void updateCursorForState();
  void setPressedState(bool value, bool immediate = false);
  void updateThumbAnimationTarget(bool immediate);
  void updateThumbAnimationState();
  void updatePressAnimationTarget(bool immediate);
  void updatePressAnimationState();
  void updateLoadingSpinnerState();
  void updateInteractionFocusOverlay();
  void startWaveEffect();
  void stopWaveEffect();
  void stopAnimations();
  void drawSpinner(QPainter* painter,
                   const QRectF& rect,
                   const QColor& color,
                   qreal preferredSize) const;
  int contentWidthForState(bool checkedState) const;

  Size size_ = Size::Default;
  bool loading_ = false;
  QString checkedChildren_;
  QString unCheckedChildren_;
  adqt::icons::IconToken checkedChildrenIconToken_;
  adqt::icons::IconToken unCheckedChildrenIconToken_;
  ComponentTokens componentTokens_;
  SemanticStyles semanticStyles_;
  SemanticStyleResolver semanticStyleResolver_;
  bool hovered_ = false;
  bool pressed_ = false;
  bool focusVisible_ = false;

  qreal thumbPosition_ = 0.0;
  qreal thumbStartPosition_ = 0.0;
  qreal thumbTargetPosition_ = 0.0;
  qint64 thumbAnimationStartMs_ = 0;
  bool thumbAnimationSubscribed_ = false;
  qreal pressStateProgress_ = 0.0;
  qreal pressStateStartProgress_ = 0.0;
  qreal pressStateTargetProgress_ = 0.0;
  qreal pressStateDirection_ = 1.0;
  qint64 pressStateAnimationStartMs_ = 0;
  bool pressStateAnimationSubscribed_ = false;
  bool spinnerSubscribed_ = false;
};

}  // namespace adqt::widgets
