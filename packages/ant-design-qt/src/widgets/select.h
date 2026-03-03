#pragma once

#include <QColor>
#include <QRectF>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QWidget>

#include <functional>
#include <optional>

#include "in_window_popup_host.h"
#include "icons_types.h"

class QAbstractListModel;
class QFrame;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QListView;
class QMoveEvent;
class QPaintEvent;
class QEnterEvent;
class QPainter;
class QToolButton;
class QVBoxLayout;

namespace adqt::widgets {

namespace detail {
struct SelectVisualStyle;
}

class AdSelect final : public QWidget, private detail::InWindowPopupOwner {
  Q_OBJECT

  Q_PROPERTY(Mode mode READ mode WRITE setMode NOTIFY modeChanged)
  Q_PROPERTY(Size size READ size WRITE setSize NOTIFY sizeChanged)
  Q_PROPERTY(Variant variant READ variant WRITE setVariant NOTIFY variantChanged)
  Q_PROPERTY(Status status READ status WRITE setStatus NOTIFY statusChanged)
  Q_PROPERTY(bool allowClear READ allowClear WRITE setAllowClear NOTIFY allowClearChanged)
  Q_PROPERTY(bool loading READ loading WRITE setLoading NOTIFY loadingChanged)
  Q_PROPERTY(bool open READ open WRITE setOpen NOTIFY openChanged)
  Q_PROPERTY(bool disabled READ disabled WRITE setDisabled NOTIFY disabledChanged)
  Q_PROPERTY(bool searchEnabled READ searchEnabled WRITE setSearchEnabled NOTIFY searchEnabledChanged)
  Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
  Q_PROPERTY(int maxCount READ maxCount WRITE setMaxCount NOTIFY maxCountChanged)
  Q_PROPERTY(int maxTagCount READ maxTagCount WRITE setMaxTagCount NOTIFY maxTagCountChanged)
  Q_PROPERTY(bool responsiveMaxTagCount READ responsiveMaxTagCount WRITE setResponsiveMaxTagCount NOTIFY responsiveMaxTagCountChanged)
  Q_PROPERTY(bool autoClearSearchValue READ autoClearSearchValue WRITE setAutoClearSearchValue NOTIFY autoClearSearchValueChanged)
  Q_PROPERTY(Placement placement READ placement WRITE setPlacement NOTIFY placementChanged)
  Q_PROPERTY(bool popupMatchSelectWidth READ popupMatchSelectWidth WRITE setPopupMatchSelectWidth NOTIFY popupMatchSelectWidthChanged)
  Q_PROPERTY(int popupWidth READ popupWidth WRITE setPopupWidth NOTIFY popupWidthChanged)
  Q_PROPERTY(QString placeholder READ placeholder WRITE setPlaceholder NOTIFY placeholderChanged)
  Q_PROPERTY(QString prefixText READ prefixText WRITE setPrefixText NOTIFY prefixTextChanged)
  Q_PROPERTY(QString value READ value WRITE setValue NOTIFY valueChanged)
  Q_PROPERTY(QStringList values READ values WRITE setValues NOTIFY valuesChanged)

 public:
  enum class Mode {
    Single,
    Multiple,
    Tags,
  };
  Q_ENUM(Mode)

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

  enum class Placement {
    BottomLeft,
    BottomRight,
    TopLeft,
    TopRight,
  };
  Q_ENUM(Placement)

  struct Option {
    QString value;
    QString label;
    bool disabled = false;
    QString group;
    QVariantMap metadata;
  };

  struct SelectionItem {
    QString value;
    QString label;
  };

  struct ComponentTokens {
    std::optional<int> controlHeight;
    std::optional<int> borderRadius;
    std::optional<int> borderWidth;
    std::optional<int> horizontalPadding;
    std::optional<int> popupMaxHeight;
    std::optional<int> optionHeight;
    std::optional<int> tagHeight;
    std::optional<int> iconSize;
    std::optional<QString> selectorBg;
    std::optional<QString> selectorBorderColor;
    std::optional<QString> selectorHoverBorderColor;
    std::optional<QString> selectorActiveBorderColor;
    std::optional<QString> selectorTextColor;
    std::optional<QString> placeholderColor;
    std::optional<QString> popupBg;
    std::optional<QString> popupBorderColor;
    std::optional<QString> optionTextColor;
    std::optional<QString> optionHoverBg;
    std::optional<QString> optionSelectedBg;
    std::optional<QString> optionSelectedColor;
    std::optional<QString> tagBg;
    std::optional<QString> tagTextColor;
    std::optional<QString> clearColor;
    std::optional<QString> prefixColor;
    std::optional<QString> suffixColor;
  };

  struct SemanticSlotStyle {
    std::optional<QColor> textColor;
    std::optional<QColor> backgroundColor;
    std::optional<QColor> borderColor;
  };

  struct SemanticStyles {
    SemanticSlotStyle root;
    SemanticSlotStyle selector;
    SemanticSlotStyle placeholder;
    SemanticSlotStyle tag;
    SemanticSlotStyle popup;
    SemanticSlotStyle option;
    SemanticSlotStyle optionHover;
    SemanticSlotStyle optionSelected;
    SemanticSlotStyle prefix;
    SemanticSlotStyle suffix;
  };

  struct StyleContext {
    Mode mode = Mode::Single;
    Size size = Size::Middle;
    Variant variant = Variant::Outlined;
    Status status = Status::None;
    bool disabled = false;
    bool open = false;
    QString searchText;
    QStringList values;
  };

  using SemanticStyleResolver = std::function<SemanticStyles(const StyleContext&)>;
  using FilterPredicate = std::function<bool(const QString& searchText, const Option& option)>;
  using SortComparator = std::function<bool(const Option& lhs, const Option& rhs)>;
  using OptionTextFormatter = std::function<QString(const Option&)>;
  using TagTextFormatter = std::function<QString(const Option&)>;
  using LabelFormatter = std::function<QString(const Option&)>;
  using PopupExtraContentFactory = std::function<QWidget*(QWidget* parent)>;

  explicit AdSelect(QWidget* parent = nullptr);
  ~AdSelect() override;

  Mode mode() const;
  void setMode(Mode value);

  Size size() const;
  void setSize(Size value);

  Variant variant() const;
  void setVariant(Variant value);

  Status status() const;
  void setStatus(Status value);

  bool allowClear() const;
  void setAllowClear(bool value);

  bool loading() const;
  void setLoading(bool value);

  bool open() const;
  void setOpen(bool value);

  bool disabled() const;
  void setDisabled(bool value);

  bool searchEnabled() const;
  void setSearchEnabled(bool value);

  QString searchText() const;
  void setSearchText(const QString& value);

  int maxCount() const;
  void setMaxCount(int value);

  int maxTagCount() const;
  void setMaxTagCount(int value);

  bool responsiveMaxTagCount() const;
  void setResponsiveMaxTagCount(bool value);

  bool autoClearSearchValue() const;
  void setAutoClearSearchValue(bool value);

  Placement placement() const;
  void setPlacement(Placement value);

  bool popupMatchSelectWidth() const;
  void setPopupMatchSelectWidth(bool value);

  int popupWidth() const;
  void setPopupWidth(int value);

  QString placeholder() const;
  void setPlaceholder(const QString& value);

  QString prefixText() const;
  void setPrefixText(const QString& value);

  adqt::icons::IconToken prefixIconToken() const;
  void setPrefixIconToken(const adqt::icons::IconToken& token);

  adqt::icons::IconToken suffixIconToken() const;
  void setSuffixIconToken(const adqt::icons::IconToken& token);

  QString value() const;
  void setValue(const QString& value);

  QStringList values() const;
  void setValues(const QStringList& values);

  QVector<SelectionItem> selectedItems() const;

  QVector<Option> options() const;
  void setOptions(const QVector<Option>& options);
  void appendOption(const Option& option);
  void clearOptions();

  void setSearchFilterFields(const QStringList& fields);
  QStringList searchFilterFields() const;

  void setFilterPredicate(FilterPredicate predicate);
  FilterPredicate filterPredicate() const;

  void setSortComparator(SortComparator comparator);
  SortComparator sortComparator() const;

  void setTokenSeparators(const QStringList& separators);
  QStringList tokenSeparators() const;

  void setOptionTextFormatter(OptionTextFormatter formatter);
  OptionTextFormatter optionTextFormatter() const;

  void setTagTextFormatter(TagTextFormatter formatter);
  TagTextFormatter tagTextFormatter() const;

  void setLabelFormatter(LabelFormatter formatter);
  LabelFormatter labelFormatter() const;

  void setPopupExtraContentFactory(PopupExtraContentFactory factory);
  PopupExtraContentFactory popupExtraContentFactory() const;

  ComponentTokens componentTokens() const;
  void setComponentTokens(const ComponentTokens& tokens);
  void resetComponentTokens();

  SemanticStyles semanticStyles() const;
  void setSemanticStyles(const SemanticStyles& styles);
  void setSemanticStyleResolver(SemanticStyleResolver resolver);

  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;

 signals:
  void modeChanged(Mode value);
  void sizeChanged(Size value);
  void variantChanged(Variant value);
  void statusChanged(Status value);
  void allowClearChanged(bool value);
  void loadingChanged(bool value);
  void openChanged(bool value);
  void disabledChanged(bool value);
  void searchEnabledChanged(bool value);
  void searchTextChanged(const QString& value);
  void maxCountChanged(int value);
  void maxTagCountChanged(int value);
  void responsiveMaxTagCountChanged(bool value);
  void autoClearSearchValueChanged(bool value);
  void placementChanged(Placement value);
  void popupMatchSelectWidthChanged(bool value);
  void popupWidthChanged(int value);
  void placeholderChanged(const QString& value);
  void prefixTextChanged(const QString& value);
  void prefixIconTokenChanged(const adqt::icons::IconToken& token);
  void suffixIconTokenChanged(const adqt::icons::IconToken& token);
  void valueChanged(const QString& value);
  void valuesChanged(const QStringList& values);
  void optionsChanged();
  void selected(const QString& value, const QString& label);
  void deselected(const QString& value, const QString& label);
  void cleared();
  void selectionChanged(const QVector<SelectionItem>& items);

 protected:
  bool eventFilter(QObject* watched, QEvent* event) override;
  void paintEvent(QPaintEvent* event) override;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  void enterEvent(QEnterEvent* event) override;
#else
  void enterEvent(QEvent* event) override;
#endif
  void leaveEvent(QEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
  void moveEvent(QMoveEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void changeEvent(QEvent* event) override;

 private:
  struct ModelRow {
    bool header = false;
    int optionIndex = -1;
    QString headerText;
  };

  class PopupFrame;
  class OptionListModel;
  class OptionListDelegate;

  bool isSearchEnabledForCurrentMode() const;
  bool isValueSelected(const QString& value) const;
  int indexOfOptionValue(const QString& value) const;
  const Option* findOption(const QString& value) const;
  Option* findOption(const QString& value);
  QString optionLabelOrFallback(const Option& option) const;
  QString optionSearchFieldValue(const Option& option, const QString& field) const;
  QString formattedOptionText(const Option& option) const;
  QString formattedTagText(const Option& option) const;
  QString formattedSelectedLabel(const Option& option) const;
  QString fallbackSelectedLabel(const QString& value) const;
  QString summaryForSelectedValues() const;
  QStringList normalizedValues(const QStringList& values) const;
  int responsiveVisibleTagCount(const QStringList& labels, int availableWidth) const;
  void enforceMaxCount();
  void updateInputMode();
  void updateDisplay();
  void updateClearButton();
  void updatePrefixVisual();
  void updateSuffixVisual();
  void applyVisualStyle();
  void refreshRows();
  QVector<int> filteredOptionIndexes() const;
  void syncCurrentListRow();
  bool addTagValue(const QString& value);
  void ensureTagOptionExists(const QString& value);
  void consumeTokenizedInput(const QString& text);
  void clearSelectionInternal(bool emitSignals);
  void emitSelectionChangedSignals();
  void toggleSelectionForOption(const Option& option);
  void selectSingleValue(const QString& value, bool emitSignals);
  void ensurePopup();
  void rebuildPopupExtraContent();
  void syncPopupGeometry();
  void closePopup();
  void openPopup();
  void setOpenInternal(bool value, bool emitSignal);
  void updateFocusState();
  QRectF selectorPaintRect() const;
  QColor resolveSelectorBgColor() const;
  QColor resolveSelectorBorderColor() const;
  qreal resolveSelectorRadius() const;
  void paintSelectorShell(QPainter& painter) const;
  void updateInteractionFocusOverlay();

  QObject* popupOwnerObject() const override;
  QWidget* popupAnchorWidget() const override;
  QWidget* popupScopeWindow() const override;
  bool popupIsVisible() const override;
  bool popupContainsGlobalPos(const QPoint& globalPos) const override;
  void popupCloseFromHost(detail::PopupCloseReason reason) override;
  void popupRelayoutFromHost() override;

  Mode mode_ = Mode::Single;
  Size size_ = Size::Middle;
  Variant variant_ = Variant::Outlined;
  Status status_ = Status::None;
  bool allowClear_ = false;
  bool loading_ = false;
  bool open_ = false;
  bool searchEnabled_ = false;
  QString searchText_;
  int maxCount_ = -1;
  int maxTagCount_ = -1;
  bool responsiveMaxTagCount_ = false;
  bool autoClearSearchValue_ = true;
  Placement placement_ = Placement::BottomLeft;
  bool popupMatchSelectWidth_ = true;
  int popupWidth_ = 0;
  QString placeholder_;
  QString prefixText_;
  adqt::icons::IconToken prefixIconToken_;
  adqt::icons::IconToken suffixIconToken_;
  QString value_;
  QStringList values_;
  QVector<Option> options_;
  QStringList searchFilterFields_;
  FilterPredicate filterPredicate_;
  SortComparator sortComparator_;
  QStringList tokenSeparators_;
  OptionTextFormatter optionTextFormatter_;
  TagTextFormatter tagTextFormatter_;
  LabelFormatter labelFormatter_;
  PopupExtraContentFactory popupExtraContentFactory_;
  ComponentTokens componentTokens_;
  SemanticStyles semanticStyles_;
  SemanticStyleResolver semanticStyleResolver_;
  detail::SelectVisualStyle* visualStyle_ = nullptr;

  QHBoxLayout* rootLayout_ = nullptr;
  QLabel* prefixLabel_ = nullptr;
  QWidget* contentHost_ = nullptr;
  QHBoxLayout* contentLayout_ = nullptr;
  QLabel* tagsSummaryLabel_ = nullptr;
  QLineEdit* lineEdit_ = nullptr;
  QToolButton* clearButton_ = nullptr;
  QToolButton* suffixButton_ = nullptr;

  QFrame* popup_ = nullptr;
  QVBoxLayout* popupLayout_ = nullptr;
  QListView* listView_ = nullptr;
  QWidget* popupExtraContent_ = nullptr;
  OptionListModel* listModel_ = nullptr;
  QVector<ModelRow> rows_;

  bool hovered_ = false;
  bool hasFocusWithin_ = false;
  bool suppressLineEditChange_ = false;
  bool applyingVisualStyle_ = false;
};

}  // namespace adqt::widgets
