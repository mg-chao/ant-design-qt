#include "select.h"

#include "icons.h"
#include "popup_placement.h"
#include "select_style.h"
#include "theme/theme.h"

#include <QApplication>
#include <QAbstractItemView>
#include <QEvent>
#include <QFontMetrics>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMouseEvent>
#include <QItemSelectionModel>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QScopedValueRollback>
#include <QSet>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <utility>

namespace adqt::widgets {

namespace {

namespace outlined_icons = adqt::icons::outlined;

QPoint mouseEventGlobalPos(const QMouseEvent* event) {
  if (!event) {
    return QPoint();
  }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  return event->globalPosition().toPoint();
#else
  return event->globalPos();
#endif
}

QRect widgetGlobalRect(const QWidget* widget) {
  if (!widget) {
    return QRect();
  }
  return QRect(widget->mapToGlobal(QPoint(0, 0)), widget->size());
}

bool widgetContainsGlobalPos(const QWidget* widget, const QPoint& globalPos) {
  const QRect globalRect = widgetGlobalRect(widget);
  return globalRect.isValid() && globalRect.contains(globalPos);
}

QStringList uniqueStringList(const QStringList& values) {
  QStringList out;
  out.reserve(values.size());
  QSet<QString> seen;
  for (const QString& value : values) {
    const QString trimmed = value.trimmed();
    if (trimmed.isEmpty() || seen.contains(trimmed)) {
      continue;
    }
    seen.insert(trimmed);
    out.append(trimmed);
  }
  return out;
}

QString escapedForRegex(const QString& text) {
  return QRegularExpression::escape(text);
}

bool iconStylesEqual(const adqt::icons::IconStyle& lhs, const adqt::icons::IconStyle& rhs) {
  return lhs.hasPrimary == rhs.hasPrimary && lhs.hasSecondary == rhs.hasSecondary &&
         lhs.primary == rhs.primary && lhs.secondary == rhs.secondary;
}

bool iconTokensEqual(const adqt::icons::IconToken& lhs, const adqt::icons::IconToken& rhs) {
  return lhs.index == rhs.index && iconStylesEqual(lhs.style, rhs.style);
}

detail::PopupPlacement toPopupPlacement(AdSelect::Placement placement) {
  switch (placement) {
    case AdSelect::Placement::BottomLeft:
      return detail::PopupPlacement::BottomLeft;
    case AdSelect::Placement::BottomRight:
      return detail::PopupPlacement::BottomRight;
    case AdSelect::Placement::TopLeft:
      return detail::PopupPlacement::TopLeft;
    case AdSelect::Placement::TopRight:
      return detail::PopupPlacement::TopRight;
  }
  return detail::PopupPlacement::BottomLeft;
}

}  // namespace

class AdSelect::OptionListModel final : public QAbstractListModel {
 public:
  explicit OptionListModel(AdSelect* owner) : QAbstractListModel(owner), owner_(owner) {}

  void setRows(const QVector<ModelRow>& rows) {
    beginResetModel();
    rows_ = rows;
    endResetModel();
  }

  int rowCount(const QModelIndex& parent) const override {
    if (parent.isValid()) {
      return 0;
    }
    return rows_.size();
  }

  QVariant data(const QModelIndex& index, int role) const override {
    if (!owner_ || !index.isValid() || index.row() < 0 || index.row() >= rows_.size()) {
      return QVariant();
    }

    const ModelRow& row = rows_.at(index.row());
    const detail::SelectVisualStyle& style = *owner_->visualStyle_;

    if (role == Qt::SizeHintRole) {
      return QSize(0, style.metrics.optionHeight);
    }

    if (row.optionIndex < 0 || row.optionIndex >= owner_->options_.size()) {
      if (role == Qt::DisplayRole) {
        return row.headerText;
      }
      if (role == Qt::FontRole) {
        QFont font = style.metrics.font;
        font.setBold(true);
        return font;
      }
      if (role == Qt::ForegroundRole) {
        return style.prefixColor;
      }
      if (role == Qt::TextAlignmentRole) {
        return static_cast<int>(Qt::AlignVCenter | Qt::AlignLeft);
      }
      return QVariant();
    }

    const Option& option = owner_->options_.at(row.optionIndex);
    if (role == Qt::DisplayRole) {
      QString text = owner_->formattedOptionText(option);
      if (owner_->mode_ != Mode::Single && owner_->isValueSelected(option.value)) {
        text = QStringLiteral("\u2713 %1").arg(text);
      }
      return text;
    }
    if (role == Qt::ForegroundRole) {
      if (option.disabled) {
        return style.disabledTextColor;
      }
      if (owner_->isValueSelected(option.value)) {
        return style.optionSelectedColor;
      }
      return style.optionTextColor;
    }
    if (role == Qt::FontRole) {
      return style.metrics.font;
    }
    if (role == Qt::UserRole) {
      return option.value;
    }
    return QVariant();
  }

  Qt::ItemFlags flags(const QModelIndex& index) const override {
    if (!owner_ || !index.isValid() || index.row() < 0 || index.row() >= rows_.size()) {
      return Qt::NoItemFlags;
    }

    const ModelRow& row = rows_.at(index.row());
    if (row.optionIndex < 0 || row.optionIndex >= owner_->options_.size()) {
      return Qt::ItemIsEnabled;
    }

    const Option& option = owner_->options_.at(row.optionIndex);
    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (option.disabled) {
      flags &= ~Qt::ItemIsEnabled;
      flags &= ~Qt::ItemIsSelectable;
    }
    return flags;
  }

 private:
  QPointer<AdSelect> owner_;
  QVector<ModelRow> rows_;
};

AdSelect::AdSelect(QWidget* parent) : QWidget(parent) {
  setObjectName(QStringLiteral("adselect-root"));
  setAttribute(Qt::WA_Hover, true);
  setFocusPolicy(Qt::StrongFocus);

  visualStyle_ = new detail::SelectVisualStyle();
  searchFilterFields_ = {QStringLiteral("label"), QStringLiteral("value")};
  tokenSeparators_ = {QStringLiteral(",")};

  rootLayout_ = new QHBoxLayout(this);
  rootLayout_->setContentsMargins(10, 4, 10, 4);
  rootLayout_->setSpacing(6);

  prefixLabel_ = new QLabel(this);
  prefixLabel_->setObjectName(QStringLiteral("adselect-prefix"));
  prefixLabel_->setVisible(false);
  rootLayout_->addWidget(prefixLabel_);

  contentHost_ = new QWidget(this);
  contentLayout_ = new QHBoxLayout(contentHost_);
  contentLayout_->setContentsMargins(0, 0, 0, 0);
  contentLayout_->setSpacing(6);

  tagsSummaryLabel_ = new QLabel(contentHost_);
  tagsSummaryLabel_->setObjectName(QStringLiteral("adselect-tags"));
  tagsSummaryLabel_->setVisible(false);
  contentLayout_->addWidget(tagsSummaryLabel_);

  lineEdit_ = new QLineEdit(contentHost_);
  lineEdit_->setObjectName(QStringLiteral("adselect-input"));
  lineEdit_->setFrame(false);
  lineEdit_->setPlaceholderText(placeholder_);
  lineEdit_->installEventFilter(this);
  contentLayout_->addWidget(lineEdit_, 1);
  rootLayout_->addWidget(contentHost_, 1);

  clearButton_ = new QToolButton(this);
  clearButton_->setObjectName(QStringLiteral("adselect-clear"));
  clearButton_->setText(QStringLiteral("x"));
  clearButton_->setAutoRaise(true);
  clearButton_->setCursor(Qt::PointingHandCursor);
  clearButton_->setVisible(false);
  rootLayout_->addWidget(clearButton_);

  suffixButton_ = new QToolButton(this);
  suffixButton_->setObjectName(QStringLiteral("adselect-suffix"));
  suffixButton_->setAutoRaise(true);
  suffixButton_->setCursor(Qt::PointingHandCursor);
  rootLayout_->addWidget(suffixButton_);

  listModel_ = new OptionListModel(this);

  connect(lineEdit_, &QLineEdit::textEdited, this, [this](const QString& text) {
    if (suppressLineEditChange_) {
      return;
    }

    if (mode_ == Mode::Tags && !tokenSeparators_.isEmpty()) {
      consumeTokenizedInput(text);
    }

    if (!isSearchEnabledForCurrentMode()) {
      return;
    }
    if (searchText_ == text) {
      return;
    }
    searchText_ = text;
    emit searchTextChanged(searchText_);
    if (!open_) {
      openPopup();
    } else {
      refreshRows();
    }
  });

  connect(clearButton_, &QToolButton::clicked, this, [this]() { clearSelectionInternal(true); });
  connect(suffixButton_, &QToolButton::clicked, this, [this]() {
    if (open_) {
      closePopup();
    } else {
      openPopup();
    }
  });

  connect(&adqt::theme::ThemeManager::instance(), &adqt::theme::ThemeManager::themeChanged, this,
          [this]() { applyVisualStyle(); });

  applyVisualStyle();
  updateInputMode();
  updateDisplay();
  updateClearButton();
  updatePrefixVisual();
  updateSuffixVisual();
}

AdSelect::~AdSelect() {
  unbindPopupScopeEvents();
  if (popup_) {
    popup_->hide();
    popup_->deleteLater();
    popup_ = nullptr;
  }
  delete visualStyle_;
  visualStyle_ = nullptr;
}

AdSelect::Mode AdSelect::mode() const { return mode_; }

void AdSelect::setMode(Mode value) {
  if (mode_ == value) {
    return;
  }
  mode_ = value;

  if (mode_ == Mode::Single) {
    if (!values_.isEmpty()) {
      value_ = values_.constFirst();
      values_ = {value_};
    }
    if (!searchEnabled_) {
      searchText_.clear();
    }
  } else {
    if (!value_.isEmpty() && values_.isEmpty()) {
      values_.append(value_);
    }
    value_.clear();
  }

  enforceMaxCount();
  updateInputMode();
  refreshRows();
  updateDisplay();
  updateClearButton();
  emit modeChanged(mode_);
  emit valueChanged(value_);
  emit valuesChanged(values_);
  emitSelectionChangedSignals();
}

AdSelect::Size AdSelect::size() const { return size_; }

void AdSelect::setSize(Size value) {
  if (size_ == value) {
    return;
  }
  size_ = value;
  applyVisualStyle();
  emit sizeChanged(size_);
}

AdSelect::Variant AdSelect::variant() const { return variant_; }

void AdSelect::setVariant(Variant value) {
  if (variant_ == value) {
    return;
  }
  variant_ = value;
  applyVisualStyle();
  emit variantChanged(variant_);
}

AdSelect::Status AdSelect::status() const { return status_; }

void AdSelect::setStatus(Status value) {
  if (status_ == value) {
    return;
  }
  status_ = value;
  applyVisualStyle();
  emit statusChanged(status_);
}

bool AdSelect::allowClear() const { return allowClear_; }

void AdSelect::setAllowClear(bool value) {
  if (allowClear_ == value) {
    return;
  }
  allowClear_ = value;
  updateClearButton();
  emit allowClearChanged(allowClear_);
}

bool AdSelect::loading() const { return loading_; }

void AdSelect::setLoading(bool value) {
  if (loading_ == value) {
    return;
  }
  loading_ = value;
  updateSuffixVisual();
  emit loadingChanged(loading_);
}

bool AdSelect::open() const { return open_; }

void AdSelect::setOpen(bool value) {
  if (value) {
    openPopup();
  } else {
    closePopup();
  }
}

bool AdSelect::disabled() const { return !isEnabled(); }

void AdSelect::setDisabled(bool value) {
  if (disabled() == value) {
    return;
  }
  QWidget::setDisabled(value);
  if (value) {
    closePopup();
  }
  applyVisualStyle();
  updateDisplay();
  updateClearButton();
  updateSuffixVisual();
  emit disabledChanged(value);
}

bool AdSelect::searchEnabled() const { return searchEnabled_; }

void AdSelect::setSearchEnabled(bool value) {
  if (searchEnabled_ == value) {
    return;
  }
  searchEnabled_ = value;
  updateInputMode();
  refreshRows();
  updateDisplay();
  emit searchEnabledChanged(searchEnabled_);
}

QString AdSelect::searchText() const { return searchText_; }

void AdSelect::setSearchText(const QString& value) {
  if (searchText_ == value) {
    return;
  }
  searchText_ = value;
  emit searchTextChanged(searchText_);
  if (isSearchEnabledForCurrentMode()) {
    if (lineEdit_ && (open_ || mode_ != Mode::Single)) {
      suppressLineEditChange_ = true;
      lineEdit_->setText(searchText_);
      suppressLineEditChange_ = false;
    }
    refreshRows();
  }
}

int AdSelect::maxCount() const { return maxCount_; }

void AdSelect::setMaxCount(int value) {
  if (maxCount_ == value) {
    return;
  }
  maxCount_ = value;
  enforceMaxCount();
  updateDisplay();
  updateClearButton();
  emit maxCountChanged(maxCount_);
}

int AdSelect::maxTagCount() const { return maxTagCount_; }

void AdSelect::setMaxTagCount(int value) {
  if (maxTagCount_ == value) {
    return;
  }
  maxTagCount_ = value;
  updateDisplay();
  emit maxTagCountChanged(maxTagCount_);
}

bool AdSelect::responsiveMaxTagCount() const { return responsiveMaxTagCount_; }

void AdSelect::setResponsiveMaxTagCount(bool value) {
  if (responsiveMaxTagCount_ == value) {
    return;
  }
  responsiveMaxTagCount_ = value;
  updateDisplay();
  emit responsiveMaxTagCountChanged(responsiveMaxTagCount_);
}

bool AdSelect::autoClearSearchValue() const { return autoClearSearchValue_; }

void AdSelect::setAutoClearSearchValue(bool value) {
  if (autoClearSearchValue_ == value) {
    return;
  }
  autoClearSearchValue_ = value;
  emit autoClearSearchValueChanged(autoClearSearchValue_);
}

AdSelect::Placement AdSelect::placement() const { return placement_; }

void AdSelect::setPlacement(Placement value) {
  if (placement_ == value) {
    return;
  }
  placement_ = value;
  if (open_) {
    syncPopupGeometry();
  }
  emit placementChanged(placement_);
}

bool AdSelect::popupMatchSelectWidth() const { return popupMatchSelectWidth_; }

void AdSelect::setPopupMatchSelectWidth(bool value) {
  if (popupMatchSelectWidth_ == value) {
    return;
  }
  popupMatchSelectWidth_ = value;
  if (open_) {
    syncPopupGeometry();
  }
  emit popupMatchSelectWidthChanged(popupMatchSelectWidth_);
}

int AdSelect::popupWidth() const { return popupWidth_; }

void AdSelect::setPopupWidth(int value) {
  if (popupWidth_ == value) {
    return;
  }
  popupWidth_ = std::max(0, value);
  if (open_) {
    syncPopupGeometry();
  }
  emit popupWidthChanged(popupWidth_);
}

QString AdSelect::placeholder() const { return placeholder_; }

void AdSelect::setPlaceholder(const QString& value) {
  if (placeholder_ == value) {
    return;
  }
  placeholder_ = value;
  updateDisplay();
  emit placeholderChanged(placeholder_);
}

QString AdSelect::prefixText() const { return prefixText_; }

void AdSelect::setPrefixText(const QString& value) {
  if (prefixText_ == value) {
    return;
  }
  prefixText_ = value;
  updatePrefixVisual();
  emit prefixTextChanged(prefixText_);
}

adqt::icons::IconToken AdSelect::prefixIconToken() const { return prefixIconToken_; }

void AdSelect::setPrefixIconToken(const adqt::icons::IconToken& token) {
  if (iconTokensEqual(prefixIconToken_, token)) {
    return;
  }
  prefixIconToken_ = token;
  updatePrefixVisual();
  emit prefixIconTokenChanged(prefixIconToken_);
}

adqt::icons::IconToken AdSelect::suffixIconToken() const { return suffixIconToken_; }

void AdSelect::setSuffixIconToken(const adqt::icons::IconToken& token) {
  if (iconTokensEqual(suffixIconToken_, token)) {
    return;
  }
  suffixIconToken_ = token;
  updateSuffixVisual();
  emit suffixIconTokenChanged(suffixIconToken_);
}

QString AdSelect::value() const { return value_; }

void AdSelect::setValue(const QString& value) {
  if (mode_ != Mode::Single) {
    setMode(Mode::Single);
  }
  selectSingleValue(value.trimmed(), true);
  refreshRows();
}

QStringList AdSelect::values() const { return values_; }

void AdSelect::setValues(const QStringList& values) {
  QStringList normalized = normalizedValues(values);
  if (mode_ == Mode::Single) {
    const QString next = normalized.isEmpty() ? QString() : normalized.constFirst();
    selectSingleValue(next, true);
    return;
  }

  if (mode_ == Mode::Tags) {
    for (const QString& value : normalized) {
      ensureTagOptionExists(value);
    }
  }

  if (maxCount_ > 0 && normalized.size() > maxCount_) {
    normalized = normalized.mid(0, maxCount_);
  }
  if (values_ == normalized) {
    return;
  }

  values_ = normalized;
  refreshRows();
  updateDisplay();
  updateClearButton();
  emit valuesChanged(values_);
  emitSelectionChangedSignals();
}

QVector<AdSelect::SelectionItem> AdSelect::selectedItems() const {
  QVector<SelectionItem> items;
  items.reserve(values_.size());
  for (const QString& current : values_) {
    SelectionItem item;
    item.value = current;
    item.label = fallbackSelectedLabel(current);
    items.append(item);
  }
  return items;
}

QVector<AdSelect::Option> AdSelect::options() const { return options_; }

void AdSelect::setOptions(const QVector<Option>& options) {
  options_ = options;
  if (mode_ == Mode::Tags) {
    for (const QString& current : values_) {
      ensureTagOptionExists(current);
    }
  }
  refreshRows();
  updateDisplay();
  updateClearButton();
  emit optionsChanged();
}

void AdSelect::appendOption(const Option& option) {
  options_.append(option);
  refreshRows();
  updateDisplay();
  emit optionsChanged();
}

void AdSelect::clearOptions() {
  if (options_.isEmpty()) {
    return;
  }
  options_.clear();
  refreshRows();
  updateDisplay();
  emit optionsChanged();
}

void AdSelect::setSearchFilterFields(const QStringList& fields) {
  const QStringList normalized = uniqueStringList(fields);
  if (searchFilterFields_ == normalized) {
    return;
  }
  searchFilterFields_ = normalized;
  refreshRows();
}

QStringList AdSelect::searchFilterFields() const { return searchFilterFields_; }

void AdSelect::setFilterPredicate(FilterPredicate predicate) {
  filterPredicate_ = std::move(predicate);
  refreshRows();
}

AdSelect::FilterPredicate AdSelect::filterPredicate() const { return filterPredicate_; }

void AdSelect::setSortComparator(SortComparator comparator) {
  sortComparator_ = std::move(comparator);
  refreshRows();
}

AdSelect::SortComparator AdSelect::sortComparator() const { return sortComparator_; }

void AdSelect::setTokenSeparators(const QStringList& separators) {
  tokenSeparators_ = uniqueStringList(separators);
}

QStringList AdSelect::tokenSeparators() const { return tokenSeparators_; }

void AdSelect::setOptionTextFormatter(OptionTextFormatter formatter) {
  optionTextFormatter_ = std::move(formatter);
  refreshRows();
  updateDisplay();
}

AdSelect::OptionTextFormatter AdSelect::optionTextFormatter() const { return optionTextFormatter_; }

void AdSelect::setTagTextFormatter(TagTextFormatter formatter) {
  tagTextFormatter_ = std::move(formatter);
  updateDisplay();
}

AdSelect::TagTextFormatter AdSelect::tagTextFormatter() const { return tagTextFormatter_; }

void AdSelect::setLabelFormatter(LabelFormatter formatter) {
  labelFormatter_ = std::move(formatter);
  updateDisplay();
}

AdSelect::LabelFormatter AdSelect::labelFormatter() const { return labelFormatter_; }

void AdSelect::setPopupExtraContentFactory(PopupExtraContentFactory factory) {
  popupExtraContentFactory_ = std::move(factory);
  if (open_) {
    rebuildPopupExtraContent();
    syncPopupGeometry();
  }
}

AdSelect::PopupExtraContentFactory AdSelect::popupExtraContentFactory() const {
  return popupExtraContentFactory_;
}

AdSelect::ComponentTokens AdSelect::componentTokens() const { return componentTokens_; }

void AdSelect::setComponentTokens(const ComponentTokens& tokens) {
  componentTokens_ = tokens;
  applyVisualStyle();
}

void AdSelect::resetComponentTokens() {
  componentTokens_ = ComponentTokens();
  applyVisualStyle();
}

AdSelect::SemanticStyles AdSelect::semanticStyles() const { return semanticStyles_; }

void AdSelect::setSemanticStyles(const SemanticStyles& styles) {
  semanticStyles_ = styles;
  applyVisualStyle();
}

void AdSelect::setSemanticStyleResolver(SemanticStyleResolver resolver) {
  semanticStyleResolver_ = std::move(resolver);
  applyVisualStyle();
}

QSize AdSelect::sizeHint() const {
  const int height = visualStyle_ ? visualStyle_->metrics.height : 32;
  return QSize(240, height);
}

QSize AdSelect::minimumSizeHint() const {
  const int height = visualStyle_ ? visualStyle_->metrics.height : 32;
  return QSize(120, height);
}

bool AdSelect::eventFilter(QObject* watched, QEvent* event) {
  if (!watched || !event) {
    return QWidget::eventFilter(watched, event);
  }

  if (open_) {
    const bool watchedScopeWindow = popupScopeWindow_ && watched == popupScopeWindow_.data();
    const bool watchedApp = qApp && watched == qApp;

    if ((watchedScopeWindow || watchedApp) && event->type() == QEvent::MouseButtonPress) {
      const auto* mouseEvent = static_cast<QMouseEvent*>(event);
      const QPoint clickGlobalPos = mouseEventGlobalPos(mouseEvent);
      const bool clickInScope =
          popupScopeWindow_ ? widgetContainsGlobalPos(popupScopeWindow_.data(), clickGlobalPos) : true;
      const bool clickInSelect = widgetContainsGlobalPos(this, clickGlobalPos);
      const bool clickInPopup = widgetContainsGlobalPos(popup_, clickGlobalPos);
      if (clickInScope && !clickInSelect && !clickInPopup) {
        closePopup();
      }
    }

    if (watchedScopeWindow) {
      if (event->type() == QEvent::WindowDeactivate || event->type() == QEvent::Hide) {
        closePopup();
      } else if (event->type() == QEvent::Resize || event->type() == QEvent::Move) {
        syncPopupGeometry();
      }
    }
  }

  if (watched == lineEdit_) {
    if (event->type() == QEvent::MouseButtonPress) {
      if (!disabled() && !open_) {
        openPopup();
      }
    } else if (event->type() == QEvent::FocusIn) {
      hasFocusWithin_ = true;
      updateFocusState();
    } else if (event->type() == QEvent::FocusOut) {
      if (!open_) {
        hasFocusWithin_ = false;
        updateFocusState();
      }
    } else if (event->type() == QEvent::KeyPress) {
      auto* keyEvent = static_cast<QKeyEvent*>(event);
      if (keyEvent->key() == Qt::Key_Down) {
        if (!open_) {
          openPopup();
          return true;
        }
      } else if (keyEvent->key() == Qt::Key_Escape) {
        if (open_) {
          closePopup();
          return true;
        }
      } else if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
        if (mode_ == Mode::Tags) {
          const QString token = lineEdit_->text().trimmed();
          if (!token.isEmpty() && addTagValue(token)) {
            if (autoClearSearchValue_) {
              setSearchText(QString());
            }
            suppressLineEditChange_ = true;
            lineEdit_->clear();
            suppressLineEditChange_ = false;
            refreshRows();
            updateDisplay();
            return true;
          }
        }
        if (open_ && listView_ && listView_->currentIndex().isValid()) {
          const int row = listView_->currentIndex().row();
          if (row >= 0 && row < rows_.size()) {
            const ModelRow& modelRow = rows_.at(row);
            if (modelRow.optionIndex >= 0 && modelRow.optionIndex < options_.size()) {
              toggleSelectionForOption(options_.at(modelRow.optionIndex));
              return true;
            }
          }
        }
      } else if (keyEvent->key() == Qt::Key_Backspace &&
                 (mode_ == Mode::Multiple || mode_ == Mode::Tags)) {
        if (lineEdit_->text().isEmpty() && !values_.isEmpty()) {
          const QString removed = values_.takeLast();
          emit deselected(removed, fallbackSelectedLabel(removed));
          emit valuesChanged(values_);
          emitSelectionChangedSignals();
          refreshRows();
          updateDisplay();
          updateClearButton();
          return true;
        }
      }
    }
  } else if (watched == popup_) {
    if (event->type() == QEvent::Hide) {
      if (open_) {
        open_ = false;
        emit openChanged(false);
        updateDisplay();
        updateSuffixVisual();
      }
      unbindPopupScopeEvents();
      hasFocusWithin_ = false;
      updateFocusState();
    }
  } else if (watched == listView_ && event->type() == QEvent::KeyPress) {
    auto* keyEvent = static_cast<QKeyEvent*>(event);
    if (keyEvent->key() == Qt::Key_Escape) {
      closePopup();
      return true;
    }
    if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
      if (listView_->currentIndex().isValid()) {
        const int row = listView_->currentIndex().row();
        if (row >= 0 && row < rows_.size()) {
          const ModelRow& modelRow = rows_.at(row);
          if (modelRow.optionIndex >= 0 && modelRow.optionIndex < options_.size()) {
            toggleSelectionForOption(options_.at(modelRow.optionIndex));
            return true;
          }
        }
      }
    }
  }
  return QWidget::eventFilter(watched, event);
}

void AdSelect::mousePressEvent(QMouseEvent* event) {
  if (!disabled() && event && event->button() == Qt::LeftButton) {
    if (clearButton_ && clearButton_->geometry().contains(event->pos())) {
      QWidget::mousePressEvent(event);
      return;
    }
    if (!open_) {
      openPopup();
    }
  }
  QWidget::mousePressEvent(event);
}

void AdSelect::keyPressEvent(QKeyEvent* event) {
  if (!event || disabled()) {
    QWidget::keyPressEvent(event);
    return;
  }

  if (event->key() == Qt::Key_Down) {
    if (!open_) {
      openPopup();
      return;
    }
  } else if (event->key() == Qt::Key_Escape) {
    if (open_) {
      closePopup();
      return;
    }
  } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
    if (!open_) {
      openPopup();
      return;
    }
  }

  QWidget::keyPressEvent(event);
}

void AdSelect::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  if (responsiveMaxTagCount_) {
    updateDisplay();
  }
  if (open_) {
    syncPopupGeometry();
  }
}

void AdSelect::changeEvent(QEvent* event) {
  QWidget::changeEvent(event);
  if (!event) {
    return;
  }
  if (event->type() == QEvent::EnabledChange || event->type() == QEvent::PaletteChange ||
      event->type() == QEvent::FontChange) {
    applyVisualStyle();
    updateDisplay();
    updateClearButton();
    updateSuffixVisual();
  }
}

bool AdSelect::isSearchEnabledForCurrentMode() const {
  if (mode_ == Mode::Single) {
    return searchEnabled_;
  }
  return true;
}

bool AdSelect::isValueSelected(const QString& value) const {
  if (mode_ == Mode::Single) {
    return !value_.isEmpty() && value_ == value;
  }
  return values_.contains(value);
}

int AdSelect::indexOfOptionValue(const QString& value) const {
  for (int i = 0; i < options_.size(); ++i) {
    if (options_.at(i).value == value) {
      return i;
    }
  }
  return -1;
}

const AdSelect::Option* AdSelect::findOption(const QString& value) const {
  const int index = indexOfOptionValue(value);
  if (index < 0 || index >= options_.size()) {
    return nullptr;
  }
  return &options_.at(index);
}

AdSelect::Option* AdSelect::findOption(const QString& value) {
  const int index = indexOfOptionValue(value);
  if (index < 0 || index >= options_.size()) {
    return nullptr;
  }
  return &options_[index];
}

QString AdSelect::optionLabelOrFallback(const Option& option) const {
  const QString trimmed = option.label.trimmed();
  return trimmed.isEmpty() ? option.value : trimmed;
}

QString AdSelect::optionSearchFieldValue(const Option& option, const QString& field) const {
  if (field == QStringLiteral("label")) {
    return optionLabelOrFallback(option);
  }
  if (field == QStringLiteral("value")) {
    return option.value;
  }
  return option.metadata.value(field).toString();
}

QString AdSelect::formattedOptionText(const Option& option) const {
  if (optionTextFormatter_) {
    return optionTextFormatter_(option);
  }
  return optionLabelOrFallback(option);
}

QString AdSelect::formattedTagText(const Option& option) const {
  if (tagTextFormatter_) {
    return tagTextFormatter_(option);
  }
  return optionLabelOrFallback(option);
}

QString AdSelect::formattedSelectedLabel(const Option& option) const {
  if (labelFormatter_) {
    return labelFormatter_(option);
  }
  return optionLabelOrFallback(option);
}

QString AdSelect::fallbackSelectedLabel(const QString& value) const {
  const Option* option = findOption(value);
  if (!option) {
    return value;
  }
  if (mode_ == Mode::Multiple || mode_ == Mode::Tags) {
    return formattedTagText(*option);
  }
  return formattedSelectedLabel(*option);
}

QString AdSelect::summaryForSelectedValues() const {
  if (values_.isEmpty()) {
    return QString();
  }

  QStringList labels;
  labels.reserve(values_.size());
  for (const QString& current : values_) {
    labels.append(fallbackSelectedLabel(current));
  }

  int visibleCount = labels.size();
  if (maxTagCount_ >= 0) {
    visibleCount = std::min(visibleCount, maxTagCount_);
  }
  if (responsiveMaxTagCount_) {
    const int prefixWidth = prefixLabel_ && prefixLabel_->isVisible() ? prefixLabel_->width() : 0;
    const int suffixWidth = suffixButton_ ? suffixButton_->width() : 0;
    const int clearWidth = clearButton_ && clearButton_->isVisible() ? clearButton_->width() : 0;
    const int available = std::max(40, width() - prefixWidth - suffixWidth - clearWidth - 56);
    visibleCount = std::min(visibleCount, responsiveVisibleTagCount(labels, available));
  }

  QString summary = labels.mid(0, visibleCount).join(QStringLiteral(", "));
  const int hiddenCount = labels.size() - visibleCount;
  if (hiddenCount > 0) {
    if (!summary.isEmpty()) {
      summary.append(QStringLiteral(" "));
    }
    summary.append(QStringLiteral("+%1...").arg(hiddenCount));
  }
  return summary;
}

QStringList AdSelect::normalizedValues(const QStringList& values) const {
  return uniqueStringList(values);
}

int AdSelect::responsiveVisibleTagCount(const QStringList& labels, int availableWidth) const {
  if (labels.isEmpty() || availableWidth <= 0) {
    return 0;
  }

  const QFontMetrics fm(tagsSummaryLabel_ ? tagsSummaryLabel_->font() : font());
  int usedWidth = 0;
  int count = 0;
  for (const QString& label : labels) {
    const int width = fm.horizontalAdvance(label) + 18;
    if (count > 0) {
      usedWidth += fm.horizontalAdvance(QStringLiteral(", "));
    }
    if (count > 0 && usedWidth + width > availableWidth) {
      break;
    }
    usedWidth += width;
    ++count;
  }
  return std::max(1, count);
}

void AdSelect::enforceMaxCount() {
  if (maxCount_ <= 0) {
    return;
  }
  if (mode_ == Mode::Single) {
    return;
  }
  if (values_.size() > maxCount_) {
    values_ = values_.mid(0, maxCount_);
    emit valuesChanged(values_);
    emitSelectionChangedSignals();
  }
}

void AdSelect::updateInputMode() {
  const bool searchable = isSearchEnabledForCurrentMode();
  if (lineEdit_) {
    const bool readOnly = !searchable;
    lineEdit_->setReadOnly(readOnly);
    lineEdit_->setCursor(readOnly ? Qt::ArrowCursor : Qt::IBeamCursor);
  }
}

void AdSelect::updateDisplay() {
  if (!lineEdit_ || !tagsSummaryLabel_) {
    return;
  }

  suppressLineEditChange_ = true;

  if (mode_ == Mode::Single) {
    tagsSummaryLabel_->setVisible(false);
    const QString label = value_.isEmpty() ? QString() : fallbackSelectedLabel(value_);
    if (open_ && isSearchEnabledForCurrentMode()) {
      lineEdit_->setText(searchText_);
    } else {
      lineEdit_->setText(label);
    }
    lineEdit_->setPlaceholderText(placeholder_);
    lineEdit_->setToolTip(label);
  } else {
    const QString summary = summaryForSelectedValues();
    tagsSummaryLabel_->setVisible(!summary.isEmpty());
    tagsSummaryLabel_->setText(summary);

    QStringList labels;
    for (const QString& current : values_) {
      labels.append(fallbackSelectedLabel(current));
    }
    tagsSummaryLabel_->setToolTip(labels.join(QStringLiteral(", ")));

    if (open_) {
      lineEdit_->setText(searchText_);
    } else {
      lineEdit_->clear();
    }
    lineEdit_->setPlaceholderText(values_.isEmpty() ? placeholder_ : QString());
  }

  suppressLineEditChange_ = false;
}

void AdSelect::updateClearButton() {
  if (!clearButton_) {
    return;
  }
  const bool hasValue = mode_ == Mode::Single ? !value_.isEmpty() : !values_.isEmpty();
  clearButton_->setVisible(allowClear_ && hasValue && !disabled());
}

void AdSelect::updatePrefixVisual() {
  if (!prefixLabel_ || !visualStyle_) {
    return;
  }

  const bool hasPrefixText = !prefixText_.trimmed().isEmpty();
  const bool hasPrefixIcon = adqt::icons::isValid(prefixIconToken_);
  if (!hasPrefixText && !hasPrefixIcon) {
    prefixLabel_->clear();
    prefixLabel_->setVisible(false);
    return;
  }

  if (hasPrefixText) {
    prefixLabel_->setPixmap(QPixmap());
    prefixLabel_->setText(prefixText_);
    prefixLabel_->setVisible(true);
    return;
  }

  adqt::icons::IconToken icon = prefixIconToken_;
  icon.style.primary = visualStyle_->prefixColor;
  icon.style.hasPrimary = true;
  const qreal dpr = devicePixelRatioF();
  const int iconSize = std::max(10, visualStyle_->metrics.iconSize);
  const QPixmap pixmap =
      adqt::icons::renderIconPixmap(icon, QSize(iconSize, iconSize), dpr, QIcon::Normal, QIcon::Off);
  prefixLabel_->setText(QString());
  prefixLabel_->setPixmap(pixmap);
  prefixLabel_->setVisible(!pixmap.isNull());
}

void AdSelect::updateSuffixVisual() {
  if (!suffixButton_ || !visualStyle_) {
    return;
  }

  suffixButton_->setText(QString());
  suffixButton_->setIcon(QIcon());

  if (loading_) {
    suffixButton_->setText(QStringLiteral("..."));
    return;
  }

  adqt::icons::IconToken icon = suffixIconToken_;
  if (!adqt::icons::isValid(icon)) {
    icon = open_ ? outlined_icons::Up() : outlined_icons::Down();
  }
  if (adqt::icons::isValid(icon)) {
    icon.style.primary = visualStyle_->suffixColor;
    icon.style.hasPrimary = true;
    const qreal dpr = devicePixelRatioF();
    const int iconSize = std::max(10, visualStyle_->metrics.iconSize);
    const QPixmap pixmap =
        adqt::icons::renderIconPixmap(icon, QSize(iconSize, iconSize), dpr, QIcon::Normal, QIcon::Off);
    if (!pixmap.isNull()) {
      suffixButton_->setIcon(QIcon(pixmap));
      suffixButton_->setIconSize(QSize(iconSize, iconSize));
      return;
    }
  }

  suffixButton_->setText(open_ ? QStringLiteral("^") : QStringLiteral("v"));
}

void AdSelect::applyVisualStyle() {
  if (!visualStyle_ || applyingVisualStyle_) {
    return;
  }
  QScopedValueRollback<bool> styleGuard(applyingVisualStyle_, true);

  StyleContext context;
  context.mode = mode_;
  context.size = size_;
  context.variant = variant_;
  context.status = status_;
  context.disabled = disabled();
  context.open = open_;
  context.searchText = searchText_;
  context.values = values_;

  SemanticStyles effectiveSemantic = semanticStyles_;
  if (semanticStyleResolver_) {
    effectiveSemantic = semanticStyleResolver_(context);
  }

  detail::SelectStyleInput input;
  input.mode = mode_;
  input.size = size_;
  input.variant = variant_;
  input.status = status_;
  input.disabled = disabled();
  input.baseFont = font();
  input.componentTokens = componentTokens_;
  input.semanticStyles = effectiveSemantic;
  *visualStyle_ = detail::resolveSelectVisualStyle(input);

  setFont(visualStyle_->metrics.font);
  if (lineEdit_) {
    lineEdit_->setFont(visualStyle_->metrics.font);
  }
  if (tagsSummaryLabel_) {
    tagsSummaryLabel_->setFont(visualStyle_->metrics.font);
  }
  if (prefixLabel_) {
    prefixLabel_->setFont(visualStyle_->metrics.font);
  }

  rootLayout_->setContentsMargins(visualStyle_->metrics.horizontalPadding, 0,
                                  visualStyle_->metrics.horizontalPadding, 0);
  rootLayout_->setSpacing(visualStyle_->metrics.spacing);
  contentLayout_->setSpacing(visualStyle_->metrics.spacing);

  setMinimumHeight(visualStyle_->metrics.height);
  setMaximumHeight(visualStyle_->metrics.height);

  const QString borderColor = (hasFocusWithin_ || open_) ? visualStyle_->selectorActiveBorderColor.name(QColor::HexArgb)
                                                          : visualStyle_->selectorBorderColor.name(QColor::HexArgb);
  const QString hoverBorderColor = visualStyle_->selectorHoverBorderColor.name(QColor::HexArgb);
  const QString selectorBg = visualStyle_->selectorBg.name(QColor::HexArgb);
  const QString textColor = visualStyle_->selectorTextColor.name(QColor::HexArgb);
  const QString placeholderColor = visualStyle_->placeholderColor.name(QColor::HexArgb);
  const QString prefixColor = visualStyle_->prefixColor.name(QColor::HexArgb);
  const QString suffixColor = visualStyle_->suffixColor.name(QColor::HexArgb);
  const QString clearColor = visualStyle_->clearColor.name(QColor::HexArgb);
  const QString tagBg = visualStyle_->tagBg.name(QColor::HexArgb);
  const QString tagColor = visualStyle_->tagTextColor.name(QColor::HexArgb);

  setProperty("focused", hasFocusWithin_ || open_);
  setProperty("variant", static_cast<int>(variant_));

  QString rootSheet = QStringLiteral(
                          "QWidget#adselect-root {"
                          "  background: %1;"
                          "  border: %2px solid %3;"
                          "  border-radius: %4px;"
                          "}"
                          "QWidget#adselect-root:hover {"
                          "  border-color: %5;"
                          "}"
                          "QWidget#adselect-root[focused=\"true\"] {"
                          "  border-color: %6;"
                          "}"
                          "QWidget#adselect-root[variant=\"%7\"] {"
                          "  border-top: 0px;"
                          "  border-left: 0px;"
                          "  border-right: 0px;"
                          "  border-radius: 0px;"
                          "}"
                          "QLineEdit#adselect-input {"
                          "  border: none;"
                          "  background: transparent;"
                          "  color: %8;"
                          "  selection-background-color: %9;"
                          "}"
                          "QLineEdit#adselect-input:disabled {"
                          "  color: %10;"
                          "}"
                          "QLineEdit#adselect-input::placeholder {"
                          "  color: %11;"
                          "}"
                          "QLabel#adselect-tags {"
                          "  background: %12;"
                          "  color: %13;"
                          "  border-radius: 4px;"
                          "  padding: 1px 6px;"
                          "}"
                          "QLabel#adselect-prefix {"
                          "  color: %14;"
                          "}"
                          "QToolButton#adselect-suffix, QToolButton#adselect-clear {"
                          "  border: none;"
                          "  background: transparent;"
                          "  color: %15;"
                          "  padding: 0px;"
                          "}"
                          "QToolButton#adselect-clear {"
                          "  color: %16;"
                          "}")
                          .arg(selectorBg)
                          .arg(visualStyle_->metrics.borderWidth)
                          .arg(borderColor)
                          .arg(visualStyle_->metrics.borderRadius)
                          .arg(hoverBorderColor)
                          .arg(visualStyle_->selectorActiveBorderColor.name(QColor::HexArgb))
                          .arg(static_cast<int>(Variant::Underlined))
                          .arg(textColor)
                          .arg(visualStyle_->optionSelectedBg.name(QColor::HexArgb))
                          .arg(visualStyle_->disabledTextColor.name(QColor::HexArgb))
                          .arg(placeholderColor)
                          .arg(tagBg)
                          .arg(tagColor)
                          .arg(prefixColor)
                          .arg(suffixColor)
                          .arg(clearColor);

  if (variant_ == Variant::Borderless) {
    rootSheet.append(QStringLiteral("QWidget#adselect-root { border-color: transparent; }"));
  }

  setStyleSheet(rootSheet);

  if (popup_) {
    const QString popupSheet = QStringLiteral(
                                   "QFrame#adselect-popup {"
                                   "  background: %1;"
                                   "  border: 1px solid %2;"
                                   "  border-radius: %3px;"
                                   "}"
                                   "QListView#adselect-list {"
                                   "  background: transparent;"
                                   "  border: none;"
                                   "  color: %4;"
                                   "  outline: none;"
                                   "}"
                                   "QListView#adselect-list::item {"
                                   "  padding-left: 10px;"
                                   "  padding-right: 10px;"
                                   "}"
                                   "QListView#adselect-list::item:hover {"
                                   "  background: %5;"
                                   "}"
                                   "QListView#adselect-list::item:selected {"
                                   "  background: %6;"
                                   "  color: %7;"
                                   "}")
                                   .arg(visualStyle_->popupBg.name(QColor::HexArgb))
                                   .arg(visualStyle_->popupBorderColor.name(QColor::HexArgb))
                                   .arg(visualStyle_->metrics.borderRadius)
                                   .arg(visualStyle_->optionTextColor.name(QColor::HexArgb))
                                   .arg(visualStyle_->optionHoverBg.name(QColor::HexArgb))
                                   .arg(visualStyle_->optionSelectedBg.name(QColor::HexArgb))
                                   .arg(visualStyle_->optionSelectedColor.name(QColor::HexArgb));
    popup_->setStyleSheet(popupSheet);
  }

  updatePrefixVisual();
  updateSuffixVisual();
  updateDisplay();
  updateClearButton();
  update();
  if (listView_) {
    listView_->setSpacing(0);
    listView_->setUniformItemSizes(true);
    listView_->viewport()->update();
  }
}

void AdSelect::refreshRows() {
  QVector<ModelRow> nextRows;
  const QVector<int> filtered = filteredOptionIndexes();

  bool hasGroup = false;
  for (const Option& option : options_) {
    if (!option.group.trimmed().isEmpty()) {
      hasGroup = true;
      break;
    }
  }

  QString currentGroup;
  for (int index : filtered) {
    if (index < 0 || index >= options_.size()) {
      continue;
    }
    const Option& option = options_.at(index);
    const QString group = option.group.trimmed();
    if (hasGroup && !group.isEmpty() && group != currentGroup) {
      ModelRow groupRow;
      groupRow.header = true;
      groupRow.optionIndex = -1;
      groupRow.headerText = group;
      nextRows.append(groupRow);
      currentGroup = group;
    } else if (group.isEmpty()) {
      currentGroup.clear();
    }

    ModelRow row;
    row.optionIndex = index;
    nextRows.append(row);
  }

  if (nextRows.isEmpty()) {
    ModelRow emptyRow;
    emptyRow.header = true;
    emptyRow.headerText = QStringLiteral("Not Found");
    nextRows.append(emptyRow);
  }

  rows_ = nextRows;
  if (listModel_) {
    listModel_->setRows(rows_);
  }
  syncCurrentListRow();
}

QVector<int> AdSelect::filteredOptionIndexes() const {
  QVector<int> indexes;
  indexes.reserve(options_.size());

  const bool useSearch = isSearchEnabledForCurrentMode() && !searchText_.trimmed().isEmpty();
  const QString term = searchText_.trimmed();
  const QStringList fields = searchFilterFields_.isEmpty()
                                 ? QStringList{QStringLiteral("label"), QStringLiteral("value")}
                                 : searchFilterFields_;

  for (int i = 0; i < options_.size(); ++i) {
    const Option& option = options_.at(i);
    bool match = true;
    if (useSearch) {
      if (filterPredicate_) {
        match = filterPredicate_(term, option);
      } else {
        match = false;
        for (const QString& field : fields) {
          const QString fieldValue = optionSearchFieldValue(option, field);
          if (fieldValue.contains(term, Qt::CaseInsensitive)) {
            match = true;
            break;
          }
        }
      }
    }
    if (match) {
      indexes.append(i);
    }
  }

  if (sortComparator_) {
    std::stable_sort(indexes.begin(), indexes.end(),
                     [this](int lhsIndex, int rhsIndex) {
                       if (lhsIndex < 0 || lhsIndex >= options_.size() || rhsIndex < 0 ||
                           rhsIndex >= options_.size()) {
                         return lhsIndex < rhsIndex;
                       }
                       return sortComparator_(options_.at(lhsIndex), options_.at(rhsIndex));
                     });
  }

  return indexes;
}

void AdSelect::syncCurrentListRow() {
  if (!listView_ || rows_.isEmpty()) {
    return;
  }

  int targetRow = -1;
  if (mode_ == Mode::Single && !value_.isEmpty()) {
    for (int row = 0; row < rows_.size(); ++row) {
      const ModelRow& modelRow = rows_.at(row);
      if (modelRow.optionIndex < 0 || modelRow.optionIndex >= options_.size()) {
        continue;
      }
      const Option& option = options_.at(modelRow.optionIndex);
      if (option.value == value_) {
        targetRow = row;
        break;
      }
    }
  }

  if (targetRow < 0) {
    for (int row = 0; row < rows_.size(); ++row) {
      const ModelRow& modelRow = rows_.at(row);
      if (modelRow.optionIndex < 0 || modelRow.optionIndex >= options_.size()) {
        continue;
      }
      if (!options_.at(modelRow.optionIndex).disabled) {
        targetRow = row;
        break;
      }
    }
  }

  if (targetRow >= 0) {
    listView_->setCurrentIndex(listModel_->index(targetRow, 0));
    if (mode_ == Mode::Single) {
      listView_->selectionModel()->select(listModel_->index(targetRow, 0),
                                          QItemSelectionModel::ClearAndSelect);
    }
  }
}

bool AdSelect::addTagValue(const QString& value) {
  const QString normalized = value.trimmed();
  if (normalized.isEmpty()) {
    return false;
  }
  if (mode_ != Mode::Tags) {
    return false;
  }
  if (values_.contains(normalized)) {
    return false;
  }
  if (maxCount_ > 0 && values_.size() >= maxCount_) {
    return false;
  }

  ensureTagOptionExists(normalized);
  values_.append(normalized);
  emit selected(normalized, fallbackSelectedLabel(normalized));
  emit valuesChanged(values_);
  emitSelectionChangedSignals();
  return true;
}

void AdSelect::ensureTagOptionExists(const QString& value) {
  if (indexOfOptionValue(value) >= 0) {
    return;
  }
  Option option;
  option.value = value;
  option.label = value;
  options_.append(option);
  emit optionsChanged();
}

void AdSelect::consumeTokenizedInput(const QString& text) {
  if (tokenSeparators_.isEmpty()) {
    return;
  }

  QString pattern;
  for (const QString& separator : tokenSeparators_) {
    if (separator.isEmpty()) {
      continue;
    }
    if (!pattern.isEmpty()) {
      pattern.append(QStringLiteral("|"));
    }
    pattern.append(escapedForRegex(separator));
  }
  if (pattern.isEmpty()) {
    return;
  }

  QRegularExpression regex(pattern);
  if (!regex.isValid()) {
    return;
  }
  if (!text.contains(regex)) {
    return;
  }

  QStringList parts = text.split(regex, Qt::KeepEmptyParts);
  if (parts.isEmpty()) {
    return;
  }

  const QRegularExpression trailingRegex(QStringLiteral("(%1)$").arg(pattern));
  const bool trailingSeparator = trailingRegex.isValid() && trailingRegex.match(text).hasMatch();
  const int limit = trailingSeparator ? parts.size() : parts.size() - 1;
  bool changed = false;
  for (int i = 0; i < limit; ++i) {
    if (addTagValue(parts.at(i))) {
      changed = true;
    }
  }

  const QString remaining = trailingSeparator ? QString() : parts.constLast();
  if (searchText_ != remaining) {
    searchText_ = remaining;
    emit searchTextChanged(searchText_);
  }
  suppressLineEditChange_ = true;
  lineEdit_->setText(remaining);
  suppressLineEditChange_ = false;

  if (changed) {
    refreshRows();
    updateDisplay();
    updateClearButton();
  }
}

void AdSelect::clearSelectionInternal(bool emitSignals) {
  bool changed = false;
  if (mode_ == Mode::Single) {
    changed = !value_.isEmpty();
    value_.clear();
    values_.clear();
  } else {
    changed = !values_.isEmpty();
    values_.clear();
  }

  if (!changed) {
    return;
  }

  if (emitSignals) {
    emit cleared();
    emit valueChanged(value_);
    emit valuesChanged(values_);
    emitSelectionChangedSignals();
  }
  refreshRows();
  updateDisplay();
  updateClearButton();
}

void AdSelect::emitSelectionChangedSignals() { emit selectionChanged(selectedItems()); }

void AdSelect::toggleSelectionForOption(const Option& option) {
  if (option.disabled) {
    return;
  }

  if (mode_ == Mode::Single) {
    const bool changed = value_ != option.value;
    selectSingleValue(option.value, true);
    if (changed) {
      emit selected(option.value, fallbackSelectedLabel(option.value));
    }
    closePopup();
    return;
  }

  const int index = values_.indexOf(option.value);
  if (index >= 0) {
    values_.removeAt(index);
    emit deselected(option.value, fallbackSelectedLabel(option.value));
  } else {
    if (maxCount_ > 0 && values_.size() >= maxCount_) {
      return;
    }
    values_.append(option.value);
    emit selected(option.value, fallbackSelectedLabel(option.value));
  }

  emit valuesChanged(values_);
  emitSelectionChangedSignals();
  if (autoClearSearchValue_ && isSearchEnabledForCurrentMode()) {
    setSearchText(QString());
    suppressLineEditChange_ = true;
    lineEdit_->clear();
    suppressLineEditChange_ = false;
  }
  refreshRows();
  updateDisplay();
  updateClearButton();
}

void AdSelect::selectSingleValue(const QString& value, bool emitSignals) {
  const QString normalized = value.trimmed();
  if (value_ == normalized && values_ == QStringList{normalized}) {
    return;
  }

  value_ = normalized;
  values_.clear();
  if (!value_.isEmpty()) {
    values_.append(value_);
  }

  if (emitSignals) {
    emit valueChanged(value_);
    emit valuesChanged(values_);
    emitSelectionChangedSignals();
  }
  updateDisplay();
  updateClearButton();
}

void AdSelect::ensurePopup() {
  if (popup_) {
    return;
  }

  QWidget* scopeWindow = detail::resolvePopupScopeWindow(this);
  popup_ = new QFrame(scopeWindow, Qt::Popup | Qt::FramelessWindowHint);
  popup_->setObjectName(QStringLiteral("adselect-popup"));
  popup_->installEventFilter(this);

  popupLayout_ = new QVBoxLayout(popup_);
  popupLayout_->setContentsMargins(4, 4, 4, 4);
  popupLayout_->setSpacing(4);

  listView_ = new QListView(popup_);
  listView_->setObjectName(QStringLiteral("adselect-list"));
  listView_->setModel(listModel_);
  listView_->setSelectionMode(QAbstractItemView::SingleSelection);
  listView_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  listView_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  listView_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  listView_->installEventFilter(this);
  popupLayout_->addWidget(listView_);

  connect(listView_, &QListView::clicked, this, [this](const QModelIndex& index) {
    if (!index.isValid()) {
      return;
    }
    const int row = index.row();
    if (row < 0 || row >= rows_.size()) {
      return;
    }
    const ModelRow& modelRow = rows_.at(row);
    if (modelRow.optionIndex < 0 || modelRow.optionIndex >= options_.size()) {
      return;
    }
    toggleSelectionForOption(options_.at(modelRow.optionIndex));
  });

  applyVisualStyle();
}

void AdSelect::rebuildPopupExtraContent() {
  if (!popup_ || !popupLayout_) {
    return;
  }
  if (popupExtraContent_) {
    popupExtraContent_->deleteLater();
    popupExtraContent_ = nullptr;
  }
  if (!popupExtraContentFactory_) {
    return;
  }

  popupExtraContent_ = popupExtraContentFactory_(popup_);
  if (!popupExtraContent_) {
    return;
  }
  popupExtraContent_->setParent(popup_);
  popupLayout_->addWidget(popupExtraContent_);
}

void AdSelect::syncPopupGeometry() {
  if (!popup_ || !visualStyle_) {
    return;
  }

  const int rowCount = std::max(1, static_cast<int>(rows_.size()));
  const int listHeight = std::min(visualStyle_->metrics.popupMaxHeight,
                                  rowCount * visualStyle_->metrics.optionHeight + 8);
  if (listView_) {
    listView_->setMinimumHeight(std::max(visualStyle_->metrics.optionHeight * 2, listHeight));
    listView_->setMaximumHeight(visualStyle_->metrics.popupMaxHeight);
  }

  popup_->adjustSize();
  int popupW = popup_->sizeHint().width();
  if (popupMatchSelectWidth_) {
    popupW = std::max(width(), popupWidth_ > 0 ? popupWidth_ : width());
  } else if (popupWidth_ > 0) {
    popupW = popupWidth_;
  }
  popupW = std::max(120, popupW);
  popup_->resize(popupW, popup_->sizeHint().height());

  detail::PopupPlacementInput placementInput;
  placementInput.anchorTopLeft = mapToGlobal(QPoint(0, 0));
  placementInput.anchorSize = QSize(width(), height());
  placementInput.popupSize = popup_->size();
  placementInput.bounds = detail::popupBoundsInGlobal(detail::resolvePopupScopeWindow(this));
  placementInput.preferredPlacement = toPopupPlacement(placement_);

  const detail::PopupPlacementOutput placementOutput =
      detail::resolvePopupPlacement(placementInput);
  popup_->move(placementOutput.topLeft);
}

void AdSelect::bindPopupScopeEvents() {
  QWidget* scopeWindow = detail::resolvePopupScopeWindow(this);

  if (popup_ && scopeWindow && popup_->parentWidget() != scopeWindow) {
    popup_->setParent(scopeWindow, popup_->windowFlags());
  }

  if (popupScopeWindow_ && popupScopeWindow_.data() != scopeWindow) {
    popupScopeWindow_->removeEventFilter(this);
    popupScopeWindow_.clear();
  }

  if (scopeWindow) {
    scopeWindow->removeEventFilter(this);
    scopeWindow->installEventFilter(this);
    popupScopeWindow_ = scopeWindow;
  }

  if (qApp) {
    qApp->removeEventFilter(this);
    qApp->installEventFilter(this);
  }
}

void AdSelect::unbindPopupScopeEvents() {
  if (popupScopeWindow_) {
    popupScopeWindow_->removeEventFilter(this);
    popupScopeWindow_.clear();
  }
  if (qApp) {
    qApp->removeEventFilter(this);
  }
}

void AdSelect::closePopup() {
  if (!open_) {
    return;
  }

  open_ = false;
  if (popup_) {
    popup_->hide();
  }
  unbindPopupScopeEvents();
  if (autoClearSearchValue_ && isSearchEnabledForCurrentMode()) {
    setSearchText(QString());
    if (lineEdit_) {
      suppressLineEditChange_ = true;
      lineEdit_->clear();
      suppressLineEditChange_ = false;
    }
  }
  emit openChanged(false);
  hasFocusWithin_ = false;
  updateFocusState();
  updateDisplay();
  updateSuffixVisual();
}

void AdSelect::openPopup() {
  if (disabled()) {
    return;
  }
  ensurePopup();
  bindPopupScopeEvents();
  refreshRows();
  rebuildPopupExtraContent();
  syncPopupGeometry();

  if (popup_) {
    popup_->show();
    popup_->raise();
  }
  if (!open_) {
    open_ = true;
    emit openChanged(true);
  }
  hasFocusWithin_ = true;
  updateFocusState();
  updateDisplay();
  updateSuffixVisual();

  if (lineEdit_) {
    lineEdit_->setFocus();
    if (isSearchEnabledForCurrentMode()) {
      lineEdit_->selectAll();
    }
  }
}

void AdSelect::setOpenInternal(bool value, bool emitSignal) {
  Q_UNUSED(value)
  Q_UNUSED(emitSignal)
}

void AdSelect::updateFocusState() {
  hasFocusWithin_ = hasFocusWithin_ || (lineEdit_ && lineEdit_->hasFocus());
  applyVisualStyle();
}

}  // namespace adqt::widgets
