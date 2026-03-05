#pragma once

#include <QColor>
#include <QLineEdit>
#include <QString>
#include <QStringList>
#include <QWidget>

#include <functional>
#include <optional>

#include "icons_types.h"

class QEnterEvent;
class QHBoxLayout;
class QLabel;
class QMouseEvent;
class QMoveEvent;
class QResizeEvent;
class QShowEvent;
class QHideEvent;
class QScrollBar;
class QSpacerItem;
class QToolButton;
class QTextEdit;
class QVBoxLayout;

namespace adqt::widgets {

class AdButton;

class AdInput final : public QWidget {
  Q_OBJECT

  Q_PROPERTY(Size size READ size WRITE setSize NOTIFY sizeChanged)
  Q_PROPERTY(Variant variant READ variant WRITE setVariant NOTIFY variantChanged)
  Q_PROPERTY(Status status READ status WRITE setStatus NOTIFY statusChanged)
  Q_PROPERTY(bool disabled READ disabled WRITE setDisabled NOTIFY disabledChanged)
  Q_PROPERTY(bool allowClear READ allowClear WRITE setAllowClear NOTIFY allowClearChanged)
  Q_PROPERTY(QString placeholder READ placeholder WRITE setPlaceholder NOTIFY placeholderChanged)
  Q_PROPERTY(QString value READ value WRITE setValue NOTIFY valueChanged)
  Q_PROPERTY(int maxLength READ maxLength WRITE setMaxLength NOTIFY maxLengthChanged)
  Q_PROPERTY(QString prefixText READ prefixText WRITE setPrefixText NOTIFY prefixTextChanged)
  Q_PROPERTY(QString suffixText READ suffixText WRITE setSuffixText NOTIFY suffixTextChanged)
  Q_PROPERTY(bool showCount READ showCount WRITE setShowCount NOTIFY showCountChanged)
  Q_PROPERTY(int countMax READ countMax WRITE setCountMax NOTIFY countMaxChanged)
  Q_PROPERTY(Qt::Alignment textAlignment READ textAlignment WRITE setTextAlignment
                 NOTIFY textAlignmentChanged)

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

  enum class FocusCursor {
    Keep,
    Start,
    End,
    All,
  };
  Q_ENUM(FocusCursor)

  struct ComponentTokens {
    std::optional<int> controlHeight;
    std::optional<int> borderRadius;
    std::optional<int> borderWidth;
    std::optional<int> horizontalPadding;
    std::optional<int> iconSize;
    std::optional<int> inputFontSize;
    std::optional<QString> selectorBg;
    std::optional<QString> selectorBorderColor;
    std::optional<QString> selectorHoverBorderColor;
    std::optional<QString> selectorActiveBorderColor;
    std::optional<QString> selectorTextColor;
    std::optional<QString> placeholderColor;
    std::optional<QString> clearColor;
    std::optional<QString> prefixColor;
    std::optional<QString> suffixColor;
    std::optional<QString> countColor;
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
    SemanticSlotStyle count;
    SemanticSlotStyle clear;
  };

  struct StyleContext {
    Size size = Size::Middle;
    Variant variant = Variant::Outlined;
    Status status = Status::None;
    bool disabled = false;
    bool focused = false;
    bool hovered = false;
    bool showCount = false;
    int valueLength = 0;
    int count = 0;
    int countMax = -1;
  };

  using CountStrategy = std::function<int(const QString&)>;
  using CountFormatter = std::function<QString(const QString&, int, int)>;
  using ExceedFormatter = std::function<QString(const QString&, int)>;
  using SemanticStyleResolver = std::function<SemanticStyles(const StyleContext&)>;

  explicit AdInput(QWidget* parent = nullptr);
  ~AdInput() override;

  Size size() const;
  void setSize(Size value);

  Variant variant() const;
  void setVariant(Variant value);

  Status status() const;
  void setStatus(Status value);

  bool disabled() const;
  void setDisabled(bool value);

  bool allowClear() const;
  void setAllowClear(bool value);

  QString placeholder() const;
  void setPlaceholder(const QString& value);

  QString value() const;
  void setValue(const QString& value);

  int maxLength() const;
  void setMaxLength(int value);

  QString prefixText() const;
  void setPrefixText(const QString& value);

  QString suffixText() const;
  void setSuffixText(const QString& value);

  bool showCount() const;
  void setShowCount(bool value);

  int countMax() const;
  void setCountMax(int value);

  Qt::Alignment textAlignment() const;
  void setTextAlignment(Qt::Alignment value);

  bool joinedLeft() const;
  void setJoinedLeft(bool value);

  bool joinedRight() const;
  void setJoinedRight(bool value);

  QLineEdit::EchoMode echoMode() const;
  void setEchoMode(QLineEdit::EchoMode value);

  adqt::icons::IconToken prefixIconToken() const;
  void setPrefixIconToken(const adqt::icons::IconToken& token);

  adqt::icons::IconToken suffixIconToken() const;
  void setSuffixIconToken(const adqt::icons::IconToken& token);

  bool suffixActionVisible() const;
  void setSuffixActionVisible(bool value);

  adqt::icons::IconToken suffixActionIconToken() const;
  void setSuffixActionIconToken(const adqt::icons::IconToken& token);

  CountStrategy countStrategy() const;
  void setCountStrategy(CountStrategy value);

  CountFormatter countFormatter() const;
  void setCountFormatter(CountFormatter value);

  ExceedFormatter exceedFormatter() const;
  void setExceedFormatter(ExceedFormatter value);

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
  void disabledChanged(bool value);
  void allowClearChanged(bool value);
  void placeholderChanged(const QString& value);
  void valueChanged(const QString& value);
  void maxLengthChanged(int value);
  void prefixTextChanged(const QString& value);
  void suffixTextChanged(const QString& value);
  void showCountChanged(bool value);
  void countMaxChanged(int value);
  void textAlignmentChanged(Qt::Alignment value);
  void echoModeChanged(QLineEdit::EchoMode value);
  void prefixIconTokenChanged(const adqt::icons::IconToken& token);
  void suffixIconTokenChanged(const adqt::icons::IconToken& token);
  void suffixActionVisibleChanged(bool value);
  void suffixActionIconTokenChanged(const adqt::icons::IconToken& token);
  void componentTokensChanged();
  void semanticStylesChanged();
  void textEdited(const QString& value);
  void returnPressed();
  void cleared();
  void suffixActionTriggered();

 protected:
  bool eventFilter(QObject* watched, QEvent* event) override;
  void enterEvent(QEnterEvent* event) override;
  void leaveEvent(QEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseDoubleClickEvent(QMouseEvent* event) override;
  void changeEvent(QEvent* event) override;
  void moveEvent(QMoveEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void showEvent(QShowEvent* event) override;
  void hideEvent(QHideEvent* event) override;

 private:
  void updateCountLabel();
  void updateClearButton();
  void updateAccessoryVisibility();
  void updateInteractiveCursor();
  void focusFromMouseGlobalPos(const QPoint& globalPos, Qt::FocusReason reason);
  void bumpJoinedZOrder();
  void updatePrefixVisual();
  void updateSuffixVisual();
  void applyVisualStyle();
  void updateInteractionFocusOverlay();
  int effectiveCount(const QString& text) const;
  int effectiveCountMax() const;

  Size size_ = Size::Middle;
  Variant variant_ = Variant::Outlined;
  Status status_ = Status::None;
  bool allowClear_ = false;
  bool joinedLeft_ = false;
  bool joinedRight_ = false;
  int maxLength_ = -1;
  QString prefixText_;
  QString suffixText_;
  bool showCount_ = false;
  int countMax_ = -1;
  Qt::Alignment textAlignment_ = Qt::AlignLeft;
  adqt::icons::IconToken prefixIconToken_;
  adqt::icons::IconToken suffixIconToken_;
  bool suffixActionVisible_ = false;
  adqt::icons::IconToken suffixActionIconToken_;
  CountStrategy countStrategy_;
  CountFormatter countFormatter_;
  ExceedFormatter exceedFormatter_;
  ComponentTokens componentTokens_;
  SemanticStyles semanticStyles_;
  SemanticStyleResolver semanticStyleResolver_;

  QVBoxLayout* rootLayout_ = nullptr;
  QWidget* shell_ = nullptr;
  QHBoxLayout* shellLayout_ = nullptr;
  QLabel* prefixIconLabel_ = nullptr;
  QLabel* prefixLabel_ = nullptr;
  QLineEdit* lineEdit_ = nullptr;
  QToolButton* clearButton_ = nullptr;
  QToolButton* suffixActionButton_ = nullptr;
  QLabel* suffixLabel_ = nullptr;
  QLabel* suffixIconLabel_ = nullptr;
  QLabel* countLabel_ = nullptr;

  bool hovered_ = false;
  bool focused_ = false;
  bool internalTextUpdate_ = false;
  QString lastValue_;
};

class AdInputTextArea final : public QWidget {
  Q_OBJECT

  Q_PROPERTY(AdInput::Size size READ size WRITE setSize NOTIFY sizeChanged)
  Q_PROPERTY(AdInput::Variant variant READ variant WRITE setVariant NOTIFY variantChanged)
  Q_PROPERTY(AdInput::Status status READ status WRITE setStatus NOTIFY statusChanged)
  Q_PROPERTY(bool disabled READ disabled WRITE setDisabled NOTIFY disabledChanged)
  Q_PROPERTY(bool allowClear READ allowClear WRITE setAllowClear NOTIFY allowClearChanged)
  Q_PROPERTY(QString placeholder READ placeholder WRITE setPlaceholder NOTIFY placeholderChanged)
  Q_PROPERTY(QString value READ value WRITE setValue NOTIFY valueChanged)
  Q_PROPERTY(int maxLength READ maxLength WRITE setMaxLength NOTIFY maxLengthChanged)
  Q_PROPERTY(bool showCount READ showCount WRITE setShowCount NOTIFY showCountChanged)
  Q_PROPERTY(int countMax READ countMax WRITE setCountMax NOTIFY countMaxChanged)
  Q_PROPERTY(bool autoSizeEnabled READ autoSizeEnabled WRITE setAutoSizeEnabled
                 NOTIFY autoSizeEnabledChanged)
  Q_PROPERTY(int autoSizeMinRows READ autoSizeMinRows WRITE setAutoSizeMinRows
                 NOTIFY autoSizeMinRowsChanged)
  Q_PROPERTY(int autoSizeMaxRows READ autoSizeMaxRows WRITE setAutoSizeMaxRows
                 NOTIFY autoSizeMaxRowsChanged)

 public:
  using Size = AdInput::Size;
  using Variant = AdInput::Variant;
  using Status = AdInput::Status;
  using FocusCursor = AdInput::FocusCursor;
  using ComponentTokens = AdInput::ComponentTokens;
  using SemanticSlotStyle = AdInput::SemanticSlotStyle;
  using SemanticStyles = AdInput::SemanticStyles;
  using StyleContext = AdInput::StyleContext;
  using CountStrategy = AdInput::CountStrategy;
  using CountFormatter = AdInput::CountFormatter;
  using ExceedFormatter = AdInput::ExceedFormatter;
  using SemanticStyleResolver = AdInput::SemanticStyleResolver;

  explicit AdInputTextArea(QWidget* parent = nullptr);
  ~AdInputTextArea() override;

  Size size() const;
  void setSize(Size value);

  Variant variant() const;
  void setVariant(Variant value);

  Status status() const;
  void setStatus(Status value);

  bool disabled() const;
  void setDisabled(bool value);

  bool allowClear() const;
  void setAllowClear(bool value);

  QString placeholder() const;
  void setPlaceholder(const QString& value);

  QString value() const;
  void setValue(const QString& value);

  int maxLength() const;
  void setMaxLength(int value);

  bool showCount() const;
  void setShowCount(bool value);

  int countMax() const;
  void setCountMax(int value);

  bool autoSizeEnabled() const;
  void setAutoSizeEnabled(bool value);

  int autoSizeMinRows() const;
  void setAutoSizeMinRows(int value);

  int autoSizeMaxRows() const;
  void setAutoSizeMaxRows(int value);

  CountStrategy countStrategy() const;
  void setCountStrategy(CountStrategy value);

  CountFormatter countFormatter() const;
  void setCountFormatter(CountFormatter value);

  ExceedFormatter exceedFormatter() const;
  void setExceedFormatter(ExceedFormatter value);

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

  QTextEdit* textEdit() const;

 signals:
  void sizeChanged(Size value);
  void variantChanged(Variant value);
  void statusChanged(Status value);
  void disabledChanged(bool value);
  void allowClearChanged(bool value);
  void placeholderChanged(const QString& value);
  void valueChanged(const QString& value);
  void maxLengthChanged(int value);
  void showCountChanged(bool value);
  void countMaxChanged(int value);
  void autoSizeEnabledChanged(bool value);
  void autoSizeMinRowsChanged(int value);
  void autoSizeMaxRowsChanged(int value);
  void componentTokensChanged();
  void semanticStylesChanged();
  void textEdited(const QString& value);
  void cleared();

 protected:
  bool eventFilter(QObject* watched, QEvent* event) override;
  void enterEvent(QEnterEvent* event) override;
  void leaveEvent(QEvent* event) override;
  void changeEvent(QEvent* event) override;
  void moveEvent(QMoveEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void showEvent(QShowEvent* event) override;
  void hideEvent(QHideEvent* event) override;

 private:
  void updateCountLabel();
  void updateClearButton();
  void updateTextEditScrollBars();
  void syncOverlayTextEditScrollBar();
  void updateOverlayTextEditScrollBarGeometry();
  void updateAutoSize();
  void applyVisualStyle();
  void updateInteractionFocusOverlay();
  int effectiveCount(const QString& text) const;
  int effectiveCountMax() const;

  Size size_ = Size::Middle;
  Variant variant_ = Variant::Outlined;
  Status status_ = Status::None;
  bool allowClear_ = false;
  int maxLength_ = -1;
  bool showCount_ = false;
  int countMax_ = -1;
  bool autoSizeEnabled_ = false;
  int autoSizeMinRows_ = 2;
  int autoSizeMaxRows_ = 6;
  CountStrategy countStrategy_;
  CountFormatter countFormatter_;
  ExceedFormatter exceedFormatter_;
  ComponentTokens componentTokens_;
  SemanticStyles semanticStyles_;
  SemanticStyleResolver semanticStyleResolver_;

  QVBoxLayout* rootLayout_ = nullptr;
  QWidget* shell_ = nullptr;
  QHBoxLayout* shellLayout_ = nullptr;
  QTextEdit* textEdit_ = nullptr;
  QScrollBar* overlayVerticalScrollBar_ = nullptr;
  QToolButton* clearButton_ = nullptr;
  QLabel* countLabel_ = nullptr;

  bool hovered_ = false;
  bool focused_ = false;
  bool verticalScrollBarHovered_ = false;
  int textEditScrollBarVerticalMargin_ = 0;
  bool internalTextUpdate_ = false;
  QString lastValue_;
};

class AdInputSearch final : public QWidget {
  Q_OBJECT

  Q_PROPERTY(AdInput::Size size READ size WRITE setSize NOTIFY sizeChanged)
  Q_PROPERTY(AdInput::Variant variant READ variant WRITE setVariant NOTIFY variantChanged)
  Q_PROPERTY(AdInput::Status status READ status WRITE setStatus NOTIFY statusChanged)
  Q_PROPERTY(bool disabled READ disabled WRITE setDisabled NOTIFY disabledChanged)
  Q_PROPERTY(bool allowClear READ allowClear WRITE setAllowClear NOTIFY allowClearChanged)
  Q_PROPERTY(QString placeholder READ placeholder WRITE setPlaceholder NOTIFY placeholderChanged)
  Q_PROPERTY(QString value READ value WRITE setValue NOTIFY valueChanged)
  Q_PROPERTY(bool loading READ loading WRITE setLoading NOTIFY loadingChanged)
  Q_PROPERTY(bool enterButton READ enterButton WRITE setEnterButton NOTIFY enterButtonChanged)
  Q_PROPERTY(QString enterButtonText READ enterButtonText WRITE setEnterButtonText
                 NOTIFY enterButtonTextChanged)

 public:
  using Size = AdInput::Size;
  using Variant = AdInput::Variant;
  using Status = AdInput::Status;

  enum class SearchSource {
    Input,
    Clear,
  };
  Q_ENUM(SearchSource)

  explicit AdInputSearch(QWidget* parent = nullptr);
  ~AdInputSearch() override;

  Size size() const;
  void setSize(Size value);

  Variant variant() const;
  void setVariant(Variant value);

  Status status() const;
  void setStatus(Status value);

  bool disabled() const;
  void setDisabled(bool value);

  bool allowClear() const;
  void setAllowClear(bool value);

  QString placeholder() const;
  void setPlaceholder(const QString& value);

  QString value() const;
  void setValue(const QString& value);

  bool loading() const;
  void setLoading(bool value);

  bool enterButton() const;
  void setEnterButton(bool value);

  QString enterButtonText() const;
  void setEnterButtonText(const QString& value);

  AdInput* input() const;

 signals:
  void sizeChanged(Size value);
  void variantChanged(Variant value);
  void statusChanged(Status value);
  void disabledChanged(bool value);
  void allowClearChanged(bool value);
  void placeholderChanged(const QString& value);
  void valueChanged(const QString& value);
  void loadingChanged(bool value);
  void enterButtonChanged(bool value);
  void enterButtonTextChanged(const QString& value);
  void searchTriggered(const QString& value, SearchSource source);

 private:
  void updateButtonVisual();

  QHBoxLayout* rootLayout_ = nullptr;
  AdInput* input_ = nullptr;
  QSpacerItem* joinOverlapSpacer_ = nullptr;
  AdButton* button_ = nullptr;
  bool loading_ = false;
  bool enterButton_ = false;
  QString enterButtonText_;
};

class AdInputPassword final : public QWidget {
  Q_OBJECT

  Q_PROPERTY(AdInput::Size size READ size WRITE setSize NOTIFY sizeChanged)
  Q_PROPERTY(AdInput::Variant variant READ variant WRITE setVariant NOTIFY variantChanged)
  Q_PROPERTY(AdInput::Status status READ status WRITE setStatus NOTIFY statusChanged)
  Q_PROPERTY(bool disabled READ disabled WRITE setDisabled NOTIFY disabledChanged)
  Q_PROPERTY(QString placeholder READ placeholder WRITE setPlaceholder NOTIFY placeholderChanged)
  Q_PROPERTY(QString value READ value WRITE setValue NOTIFY valueChanged)
  Q_PROPERTY(bool visibilityToggle READ visibilityToggle WRITE setVisibilityToggle
                 NOTIFY visibilityToggleChanged)
  Q_PROPERTY(bool passwordVisible READ passwordVisible WRITE setPasswordVisible
                 NOTIFY passwordVisibleChanged)

 public:
  using Size = AdInput::Size;
  using Variant = AdInput::Variant;
  using Status = AdInput::Status;

  explicit AdInputPassword(QWidget* parent = nullptr);
  ~AdInputPassword() override;

  Size size() const;
  void setSize(Size value);

  Variant variant() const;
  void setVariant(Variant value);

  Status status() const;
  void setStatus(Status value);

  bool disabled() const;
  void setDisabled(bool value);

  QString placeholder() const;
  void setPlaceholder(const QString& value);

  QString value() const;
  void setValue(const QString& value);

  bool visibilityToggle() const;
  void setVisibilityToggle(bool value);

  bool passwordVisible() const;
  void setPasswordVisible(bool value);

  adqt::icons::IconToken visibleIconToken() const;
  void setVisibleIconToken(const adqt::icons::IconToken& value);

  adqt::icons::IconToken hiddenIconToken() const;
  void setHiddenIconToken(const adqt::icons::IconToken& value);

  AdInput* input() const;

 signals:
  void sizeChanged(Size value);
  void variantChanged(Variant value);
  void statusChanged(Status value);
  void disabledChanged(bool value);
  void placeholderChanged(const QString& value);
  void valueChanged(const QString& value);
  void visibilityToggleChanged(bool value);
  void passwordVisibleChanged(bool value);

 private:
  void updateToggleVisual();

  QHBoxLayout* rootLayout_ = nullptr;
  AdInput* input_ = nullptr;
  bool visibilityToggle_ = true;
  bool passwordVisible_ = false;
  adqt::icons::IconToken visibleIconToken_;
  adqt::icons::IconToken hiddenIconToken_;
};

class AdInputOtp final : public QWidget {
  Q_OBJECT

  Q_PROPERTY(int length READ length WRITE setLength NOTIFY lengthChanged)
  Q_PROPERTY(QString value READ value WRITE setValue NOTIFY valueChanged)
  Q_PROPERTY(AdInput::Size size READ size WRITE setSize NOTIFY sizeChanged)
  Q_PROPERTY(AdInput::Variant variant READ variant WRITE setVariant NOTIFY variantChanged)
  Q_PROPERTY(AdInput::Status status READ status WRITE setStatus NOTIFY statusChanged)
  Q_PROPERTY(bool disabled READ disabled WRITE setDisabled NOTIFY disabledChanged)
  Q_PROPERTY(bool maskEnabled READ maskEnabled WRITE setMaskEnabled NOTIFY maskEnabledChanged)
  Q_PROPERTY(QString maskCharacter READ maskCharacter WRITE setMaskCharacter
                 NOTIFY maskCharacterChanged)

 public:
  using Size = AdInput::Size;
  using Variant = AdInput::Variant;
  using Status = AdInput::Status;

  using Formatter = std::function<QString(const QString&)>;
  using SeparatorFactory = std::function<QWidget*(int index, QWidget* parent)>;

  explicit AdInputOtp(QWidget* parent = nullptr);
  ~AdInputOtp() override;

  int length() const;
  void setLength(int value);

  QString value() const;
  void setValue(const QString& value);

  Size size() const;
  void setSize(Size value);

  Variant variant() const;
  void setVariant(Variant value);

  Status status() const;
  void setStatus(Status value);

  bool disabled() const;
  void setDisabled(bool value);

  bool maskEnabled() const;
  void setMaskEnabled(bool value);

  QString maskCharacter() const;
  void setMaskCharacter(const QString& value);

  Formatter formatter() const;
  void setFormatter(Formatter value);

  QString separatorText() const;
  void setSeparatorText(const QString& value);

  SeparatorFactory separatorFactory() const;
  void setSeparatorFactory(SeparatorFactory value);

 signals:
  void lengthChanged(int value);
  void valueChanged(const QString& value);
  void sizeChanged(Size value);
  void variantChanged(Variant value);
  void statusChanged(Status value);
  void disabledChanged(bool value);
  void maskEnabledChanged(bool value);
  void maskCharacterChanged(const QString& value);
  void inputChanged(const QStringList& values);
  void completed(const QString& value);

 protected:
  bool eventFilter(QObject* watched, QEvent* event) override;

 private:
  void rebuildCells();
  void applyVisualStyle();
  void applyValueToCells(const QString& value);
  QString combinedValueFromCells() const;
  void emitInputState();
  void handleCellEdited(int index, const QString& text);
  void focusCell(int index);
  void updateEchoModes();

  int length_ = 6;
  QString value_;
  Size size_ = Size::Middle;
  Variant variant_ = Variant::Outlined;
  Status status_ = Status::None;
  bool maskEnabled_ = false;
  QString maskCharacter_;
  Formatter formatter_;
  QString separatorText_;
  SeparatorFactory separatorFactory_;

  QHBoxLayout* rootLayout_ = nullptr;
  QVector<QLineEdit*> cells_;
  QVector<QWidget*> separators_;
  bool internalUpdate_ = false;
};

}  // namespace adqt::widgets
