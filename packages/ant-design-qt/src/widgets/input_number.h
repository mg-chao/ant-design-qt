#pragma once

#include <QColor>
#include <QLineEdit>
#include <QVariant>
#include <QWidget>

#include <functional>
#include <optional>

#include "icons_types.h"

class QEnterEvent;
class QHideEvent;
class QHBoxLayout;
class QLabel;
class QMouseEvent;
class QMoveEvent;
class QPaintEvent;
class QResizeEvent;
class QShowEvent;
class QToolButton;
class QVBoxLayout;

namespace adqt::widgets {

class AdInputNumber final : public QWidget {
  Q_OBJECT

  Q_PROPERTY(Size size READ size WRITE setSize NOTIFY sizeChanged)
  Q_PROPERTY(Variant variant READ variant WRITE setVariant NOTIFY variantChanged)
  Q_PROPERTY(Status status READ status WRITE setStatus NOTIFY statusChanged)
  Q_PROPERTY(Mode mode READ mode WRITE setMode NOTIFY modeChanged)
  Q_PROPERTY(bool disabled READ disabled WRITE setDisabled NOTIFY disabledChanged)
  Q_PROPERTY(bool readOnly READ readOnly WRITE setReadOnly NOTIFY readOnlyChanged)
  Q_PROPERTY(bool controls READ controls WRITE setControls NOTIFY controlsChanged)
  Q_PROPERTY(bool keyboardEnabled READ keyboardEnabled WRITE setKeyboardEnabled
                 NOTIFY keyboardEnabledChanged)
  Q_PROPERTY(bool changeOnBlur READ changeOnBlur WRITE setChangeOnBlur NOTIFY changeOnBlurChanged)
  Q_PROPERTY(bool changeOnWheel READ changeOnWheel WRITE setChangeOnWheel NOTIFY changeOnWheelChanged)
  Q_PROPERTY(bool stringMode READ stringMode WRITE setStringMode NOTIFY stringModeChanged)
  Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged)
  Q_PROPERTY(QVariant min READ min WRITE setMin NOTIFY minChanged)
  Q_PROPERTY(QVariant max READ max WRITE setMax NOTIFY maxChanged)
  Q_PROPERTY(QVariant step READ step WRITE setStep NOTIFY stepChanged)
  Q_PROPERTY(int precision READ precision WRITE setPrecision NOTIFY precisionChanged)
  Q_PROPERTY(QString placeholder READ placeholder WRITE setPlaceholder NOTIFY placeholderChanged)
  Q_PROPERTY(QString prefixText READ prefixText WRITE setPrefixText NOTIFY prefixTextChanged)
  Q_PROPERTY(QString suffixText READ suffixText WRITE setSuffixText NOTIFY suffixTextChanged)
  Q_PROPERTY(bool joinedLeft READ joinedLeft WRITE setJoinedLeft)
  Q_PROPERTY(bool joinedRight READ joinedRight WRITE setJoinedRight)

 public:
  enum class Size {
    Large,
    Middle,
    Small,
  };
  Q_ENUM(Size)

  enum class Variant {
    Outlined,
    Filled,
    Borderless,
    Underlined,
  };
  Q_ENUM(Variant)

  enum class Status {
    None,
    Error,
    Warning,
  };
  Q_ENUM(Status)

  enum class Mode {
    Input,
    Spinner,
  };
  Q_ENUM(Mode)

  enum class FocusCursor {
    Keep,
    Start,
    End,
    All,
  };
  Q_ENUM(FocusCursor)

  enum class StepType {
    Up,
    Down,
  };
  Q_ENUM(StepType)

  enum class StepEmitter {
    Handler,
    KeyDown,
    Wheel,
  };
  Q_ENUM(StepEmitter)

  struct ComponentTokens {
    std::optional<int> controlWidth;
    std::optional<int> controlHeight;
    std::optional<int> borderRadius;
    std::optional<int> borderWidth;
    std::optional<int> horizontalPadding;
    std::optional<int> iconSize;
    std::optional<int> inputFontSize;
    std::optional<int> inputFontSizeSM;
    std::optional<int> inputFontSizeLG;
    std::optional<int> paddingInlineLG;
    std::optional<int> paddingBlockLG;
    std::optional<int> handleWidth;
    std::optional<int> handleVisibleWidth;
    std::optional<QString> selectorBg;
    std::optional<QString> selectorBorderColor;
    std::optional<QString> selectorHoverBorderColor;
    std::optional<QString> selectorActiveBorderColor;
    std::optional<QString> selectorTextColor;
    std::optional<QString> placeholderColor;
    std::optional<QString> prefixColor;
    std::optional<QString> suffixColor;
    std::optional<QString> handleBg;
    std::optional<QString> handleActiveBg;
    std::optional<QString> handleBorderColor;
    std::optional<QString> handleHoverColor;
    std::optional<QString> handleIconColor;
  };

  struct SemanticSlotStyle {
    std::optional<QColor> textColor;
    std::optional<QColor> backgroundColor;
    std::optional<QColor> borderColor;
  };

  struct SemanticStyles {
    SemanticSlotStyle root;
    SemanticSlotStyle input;
    SemanticSlotStyle prefix;
    SemanticSlotStyle suffix;
    SemanticSlotStyle actions;
    SemanticSlotStyle action;
  };

  struct StyleContext {
    Size size = Size::Middle;
    Variant variant = Variant::Outlined;
    Status status = Status::None;
    Mode mode = Mode::Input;
    bool disabled = false;
    bool readOnly = false;
    bool focused = false;
    bool hovered = false;
    bool controls = true;
    bool outOfRange = false;
  };

  using Formatter =
      std::function<QString(const QVariant& value, bool userTyping, const QString& input)>;
  using Parser = std::function<QVariant(const QString& text)>;
  using SemanticStyleResolver = std::function<SemanticStyles(const StyleContext&)>;

  explicit AdInputNumber(QWidget* parent = nullptr);
  ~AdInputNumber() override;

  Size size() const;
  void setSize(Size value);

  Variant variant() const;
  void setVariant(Variant value);

  Status status() const;
  void setStatus(Status value);

  Mode mode() const;
  void setMode(Mode value);

  bool disabled() const;
  void setDisabled(bool value);

  bool readOnly() const;
  void setReadOnly(bool value);

  bool controls() const;
  void setControls(bool value);

  bool keyboardEnabled() const;
  void setKeyboardEnabled(bool value);

  bool changeOnBlur() const;
  void setChangeOnBlur(bool value);

  bool changeOnWheel() const;
  void setChangeOnWheel(bool value);

  bool stringMode() const;
  void setStringMode(bool value);

  QVariant value() const;
  void setValue(const QVariant& value);

  QVariant min() const;
  void setMin(const QVariant& value);

  QVariant max() const;
  void setMax(const QVariant& value);

  QVariant step() const;
  void setStep(const QVariant& value);

  int precision() const;
  void setPrecision(int value);

  QString placeholder() const;
  void setPlaceholder(const QString& value);

  QString prefixText() const;
  void setPrefixText(const QString& value);

  QString suffixText() const;
  void setSuffixText(const QString& value);

  bool joinedLeft() const;
  void setJoinedLeft(bool value);

  bool joinedRight() const;
  void setJoinedRight(bool value);

  adqt::icons::IconToken prefixIconToken() const;
  void setPrefixIconToken(const adqt::icons::IconToken& token);

  adqt::icons::IconToken suffixIconToken() const;
  void setSuffixIconToken(const adqt::icons::IconToken& token);

  adqt::icons::IconToken upIconToken() const;
  void setUpIconToken(const adqt::icons::IconToken& token);

  adqt::icons::IconToken downIconToken() const;
  void setDownIconToken(const adqt::icons::IconToken& token);

  Formatter formatter() const;
  void setFormatter(Formatter value);

  Parser parser() const;
  void setParser(Parser value);

  ComponentTokens componentTokens() const;
  void setComponentTokens(const ComponentTokens& tokens);
  void resetComponentTokens();

  SemanticStyles semanticStyles() const;
  void setSemanticStyles(const SemanticStyles& styles);
  void setSemanticStyleResolver(SemanticStyleResolver resolver);

  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;

  void focusInput(FocusCursor cursor = FocusCursor::Keep, bool preventScroll = false);
  void blurInput();

  QLineEdit* lineEdit() const;

 signals:
  void sizeChanged(Size value);
  void variantChanged(Variant value);
  void statusChanged(Status value);
  void modeChanged(Mode value);
  void disabledChanged(bool value);
  void readOnlyChanged(bool value);
  void controlsChanged(bool value);
  void keyboardEnabledChanged(bool value);
  void changeOnBlurChanged(bool value);
  void changeOnWheelChanged(bool value);
  void stringModeChanged(bool value);
  void valueChanged(const QVariant& value);
  void minChanged(const QVariant& value);
  void maxChanged(const QVariant& value);
  void stepChanged(const QVariant& value);
  void precisionChanged(int value);
  void placeholderChanged(const QString& value);
  void prefixTextChanged(const QString& value);
  void suffixTextChanged(const QString& value);
  void prefixIconTokenChanged(const adqt::icons::IconToken& token);
  void suffixIconTokenChanged(const adqt::icons::IconToken& token);
  void upIconTokenChanged(const adqt::icons::IconToken& token);
  void downIconTokenChanged(const adqt::icons::IconToken& token);
  void componentTokensChanged();
  void semanticStylesChanged();
  void stepped(const QVariant& value, int offset, StepType type, StepEmitter emitter);
  void returnPressed();

 protected:
  bool eventFilter(QObject* watched, QEvent* event) override;
  void enterEvent(QEnterEvent* event) override;
  void leaveEvent(QEvent* event) override;
  void paintEvent(QPaintEvent* event) override;
  void changeEvent(QEvent* event) override;
  void moveEvent(QMoveEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void showEvent(QShowEvent* event) override;
  void hideEvent(QHideEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;

 private:
  void updateInteractionFocusOverlay();
  void syncLayoutForMode();
  void updateVisualState();
  void updateAffixVisual();
  void updateActionIcons();
  void updateActionVisibility();
  void updateReadOnlyState();
  void updateInteractiveCursor();
  void updateLineEditTextFromValue(bool userTyping);
  void commitFromText(bool triggerBlurAdjust);
  bool stepBy(int direction, StepEmitter emitter);
  void setValueInternal(const QVariant& value, bool emitSignal, bool userTyping);

  bool valueEquals(const QVariant& lhs, const QVariant& rhs) const;
  bool tryVariantToNumber(const QVariant& value, long double* out) const;
  QVariant normalizeParsedVariant(const QVariant& parsed) const;
  QVariant parsedFromText(const QString& text, bool userTyping) const;
  QVariant clampedVariant(const QVariant& value) const;
  QVariant applyPrecisionVariant(const QVariant& value) const;
  QString defaultFormatForValue(const QVariant& value) const;
  QString formattedForDisplay(const QVariant& value, bool userTyping, const QString& input) const;
  bool hasMinNumber(long double* out) const;
  bool hasMaxNumber(long double* out) const;
  bool hasOutOfRangeValue() const;
  int cursorPositionForClickX(int x) const;
  void focusFromMouseGlobalPos(const QPoint& globalPos, Qt::FocusReason reason);
  void bumpJoinedZOrder();

  Size size_ = Size::Middle;
  Variant variant_ = Variant::Outlined;
  Status status_ = Status::None;
  Mode mode_ = Mode::Input;
  bool controls_ = true;
  bool readOnly_ = false;
  bool keyboardEnabled_ = true;
  bool changeOnBlur_ = true;
  bool changeOnWheel_ = false;
  bool stringMode_ = false;
  QVariant value_;
  QVariant min_;
  QVariant max_;
  QVariant step_ = QVariant(1.0);
  int precision_ = -1;
  QString prefixText_;
  QString suffixText_;
  bool joinedLeft_ = false;
  bool joinedRight_ = false;
  adqt::icons::IconToken prefixIconToken_;
  adqt::icons::IconToken suffixIconToken_;
  adqt::icons::IconToken upIconToken_;
  adqt::icons::IconToken downIconToken_;
  Formatter formatter_;
  Parser parser_;
  ComponentTokens componentTokens_;
  SemanticStyles semanticStyles_;
  SemanticStyleResolver semanticStyleResolver_;

  QHBoxLayout* rootLayout_ = nullptr;
  QLabel* prefixIconLabel_ = nullptr;
  QLabel* prefixLabel_ = nullptr;
  QLineEdit* lineEdit_ = nullptr;
  QLabel* suffixLabel_ = nullptr;
  QLabel* suffixIconLabel_ = nullptr;
  QWidget* inputActionsWidget_ = nullptr;
  QVBoxLayout* inputActionsLayout_ = nullptr;
  QToolButton* inputUpButton_ = nullptr;
  QToolButton* inputDownButton_ = nullptr;
  QToolButton* spinnerDownButton_ = nullptr;
  QToolButton* spinnerUpButton_ = nullptr;

  bool hovered_ = false;
  bool focused_ = false;
  bool userTyping_ = false;
  bool internalTextUpdate_ = false;
};

}  // namespace adqt::widgets

Q_DECLARE_METATYPE(adqt::widgets::AdInputNumber::StepType)
Q_DECLARE_METATYPE(adqt::widgets::AdInputNumber::StepEmitter)
