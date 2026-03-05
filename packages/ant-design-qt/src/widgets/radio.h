#pragma once

#include <QAbstractButton>
#include <QColor>
#include <QVariant>

#include <functional>
#include <optional>

namespace adqt::widgets {

class AdRadioGroup;

class AdRadio : public QAbstractButton {
  Q_OBJECT

  Q_PROPERTY(bool checked READ isChecked WRITE setChecked NOTIFY checkedChanged)
  Q_PROPERTY(bool disabled READ disabled WRITE setDisabled NOTIFY disabledChanged)
  Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged)
  Q_PROPERTY(Size size READ size WRITE setSize NOTIFY sizeChanged)
  Q_PROPERTY(OptionType optionType READ optionType WRITE setOptionType NOTIFY optionTypeChanged)
  Q_PROPERTY(ButtonStyle buttonStyle READ buttonStyle WRITE setButtonStyle NOTIFY buttonStyleChanged)
  Q_PROPERTY(bool block READ block WRITE setBlock NOTIFY blockChanged)

 public:
  enum class Size {
    Large,
    Middle,
    Small,
  };
  Q_ENUM(Size)

  enum class OptionType {
    Default,
    Button,
  };
  Q_ENUM(OptionType)

  enum class ButtonStyle {
    Outline,
    Solid,
  };
  Q_ENUM(ButtonStyle)

  struct ComponentTokens {
    std::optional<int> radioSize;
    std::optional<int> dotSize;
    std::optional<QString> dotColorDisabled;

    std::optional<QString> buttonBg;
    std::optional<QString> buttonCheckedBg;
    std::optional<QString> buttonColor;
    std::optional<int> buttonPaddingInline;
    std::optional<QString> buttonCheckedBgDisabled;
    std::optional<QString> buttonCheckedColorDisabled;
    std::optional<QString> buttonSolidCheckedColor;
    std::optional<QString> buttonSolidCheckedBg;
    std::optional<QString> buttonSolidCheckedHoverBg;
    std::optional<QString> buttonSolidCheckedActiveBg;
    std::optional<int> wrapperMarginInlineEnd;

    std::optional<QString> radioColor;
    std::optional<QString> radioBgColor;
  };

  struct SemanticSlotStyle {
    std::optional<QColor> textColor;
    std::optional<QColor> backgroundColor;
    std::optional<QColor> borderColor;
  };

  struct SemanticStyles {
    SemanticSlotStyle root;
    SemanticSlotStyle icon;
    SemanticSlotStyle label;
  };

  struct StyleContext {
    Size size = Size::Middle;
    OptionType optionType = OptionType::Default;
    ButtonStyle buttonStyle = ButtonStyle::Outline;
    bool checked = false;
    bool disabled = false;
    bool hovered = false;
    bool focused = false;
    bool block = false;
  };

  using SemanticStyleResolver = std::function<SemanticStyles(const StyleContext&)>;

  explicit AdRadio(QWidget* parent = nullptr);
  explicit AdRadio(const QString& text, QWidget* parent = nullptr);
  ~AdRadio() override;

  bool disabled() const;
  void setDisabled(bool value);

  QVariant value() const;
  void setValue(const QVariant& value);

  Size size() const;
  void setSize(Size value);

  OptionType optionType() const;
  void setOptionType(OptionType value);

  ButtonStyle buttonStyle() const;
  void setButtonStyle(ButtonStyle value);

  bool block() const;
  void setBlock(bool value);

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
  void disabledChanged(bool value);
  void valueChanged(const QVariant& value);
  void sizeChanged(Size value);
  void optionTypeChanged(OptionType value);
  void buttonStyleChanged(ButtonStyle value);
  void blockChanged(bool value);
  void componentTokensChanged();
  void semanticStylesChanged();
  void changed(const QVariant& value, bool checked);

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
  enum class GroupPosition {
    None,
    Only,
    First,
    Middle,
    Last,
  };

  bool effectiveDisabled() const;
  SemanticStyles resolvedSemanticStyles() const;
  void refreshAfterPropertyChange(bool updateGeometry = true);
  void updateInteractionFocusOverlay();
  void triggerInteractionWaveOverlay();
  void bumpButtonGroupZOrder();
  int textWidth(const QFontMetrics& metrics) const;
  qreal cornerRadius() const;
  void resolveButtonCornerRadii(qreal* topLeft,
                                qreal* topRight,
                                qreal* bottomRight,
                                qreal* bottomLeft) const;

  void setGroupPosition(GroupPosition position);
  void setGroupVertical(bool vertical);
  void setGroupName(const QString& name);

  friend class AdRadioGroup;

  Size size_ = Size::Middle;
  OptionType optionType_ = OptionType::Default;
  ButtonStyle buttonStyle_ = ButtonStyle::Outline;
  bool block_ = false;
  QVariant value_;
  ComponentTokens componentTokens_;
  SemanticStyles semanticStyles_;
  SemanticStyleResolver semanticStyleResolver_;

  bool hovered_ = false;
  bool pressed_ = false;
  bool focusVisible_ = false;
  GroupPosition groupPosition_ = GroupPosition::None;
  bool groupVertical_ = false;
  QString groupName_;
};

class AdRadioButton final : public AdRadio {
  Q_OBJECT

 public:
  explicit AdRadioButton(QWidget* parent = nullptr);
  explicit AdRadioButton(const QString& text, QWidget* parent = nullptr);
  ~AdRadioButton() override = default;
};

}  // namespace adqt::widgets
