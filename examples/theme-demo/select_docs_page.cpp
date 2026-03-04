#include "select_docs_page.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPalette>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

#include <memory>

#include "icons.h"
#include "widgets/detail/timing_hub.h"

using adqt::widgets::AdSelect;
namespace outlined_icons = adqt::icons::outlined;

namespace {

QLabel* makeHintLabel(const QString& text, QWidget* parent = nullptr) {
  auto* label = new QLabel(text, parent);
  label->setWordWrap(true);
  QPalette palette = label->palette();
  palette.setColor(QPalette::WindowText, QColor("#8c8c8c"));
  label->setPalette(palette);
  return label;
}

AdSelect::Option makeOption(const QString& value,
                            const QString& label,
                            bool disabled = false,
                            const QString& group = QString(),
                            const QVariantMap& metadata = {}) {
  AdSelect::Option option;
  option.value = value;
  option.label = label;
  option.disabled = disabled;
  option.group = group;
  option.metadata = metadata;
  return option;
}

}  // namespace

SelectDocsPage::SelectDocsPage(QWidget* parent) : QWidget(parent) {
  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(16, 16, 16, 24);
  root->setSpacing(16);

  auto* title = new QLabel("Select");
  QFont titleFont = title->font();
  titleFont.setPointSize(titleFont.pointSize() + 8);
  titleFont.setBold(true);
  title->setFont(titleFont);
  root->addWidget(title);

  auto* subtitle = new QLabel(
      "A dropdown menu for displaying choices. This page mirrors Ant Design Select public demos.");
  subtitle->setWordWrap(true);
  root->addWidget(subtitle);

  addSection(root, "Basic Usage", "Demo: basic.tsx", buildBasicDemo());
  addSection(root, "Select with search field", "Demo: search.tsx", buildSearchDemo());
  addSection(root, "Custom Search", "Demo: search-filter-option.tsx", buildSearchFilterOptionDemo());
  addSection(root, "Multi field search", "Demo: search-multi-field.tsx", buildSearchMultiFieldDemo());
  addSection(root, "multiple selection", "Demo: multiple.tsx", buildMultipleDemo());
  addSection(root, "Sizes", "Demo: size.tsx", buildSizeDemo());
  addSection(root, "Custom dropdown options", "Demo: option-render.tsx", buildOptionRenderDemo());
  addSection(root, "Search with sort", "Demo: search-sort.tsx", buildSearchSortDemo());
  addSection(root, "Tags", "Demo: tags.tsx", buildTagsDemo());
  addSection(root, "Option Group", "Demo: optgroup.tsx", buildOptGroupDemo());
  addSection(root, "coordinate", "Demo: coordinate.tsx", buildCoordinateDemo());
  addSection(root, "Search Box", "Demo: search-box.tsx", buildSearchBoxDemo());
  addSection(root, "Get value of selected item", "Demo: label-in-value.tsx", buildLabelInValueDemo());
  addSection(root, "Automatic tokenization", "Demo: automatic-tokenization.tsx", buildAutomaticTokenizationDemo());
  addSection(root, "Search and Select Users", "Demo: select-users.tsx", buildSelectUsersDemo());
  addSection(root, "Prefix and Suffix", "Demo: suffix.tsx", buildSuffixDemo());
  addSection(root, "Custom dropdown", "Demo: custom-dropdown-menu.tsx", buildCustomDropdownDemo());
  addSection(root, "Hide Already Selected", "Demo: hide-selected.tsx", buildHideSelectedDemo());
  addSection(root, "Variants", "Demo: variant.tsx", buildVariantDemo());
  addSection(root, "Custom Tag Render", "Demo: custom-tag-render.tsx", buildCustomTagRenderDemo());
  addSection(root, "Custom Selected Label Render", "Demo: custom-label-render.tsx", buildCustomLabelRenderDemo());
  addSection(root, "Responsive maxTagCount", "Demo: responsive.tsx", buildResponsiveDemo());
  addSection(root, "Status", "Demo: status.tsx", buildStatusDemo());
  addSection(root, "Placement", "Demo: placement.tsx", buildPlacementDemo());
  addSection(root, "Max Count", "Demo: maxCount.tsx", buildMaxCountDemo());
  addSection(root, "Custom semantic dom styling", "Demo: style-class.tsx", buildStyleClassDemo());

  root->addStretch();
}

const QVector<QWidget*>& SelectDocsPage::sectionAnchors() const { return anchors_; }

const QStringList& SelectDocsPage::sectionTitles() const { return titles_; }

void SelectDocsPage::addSection(QVBoxLayout* root,
                                const QString& title,
                                const QString& description,
                                QWidget* content) {
  auto* panel = new QFrame();
  panel->setFrameShape(QFrame::StyledPanel);
  auto* layout = new QVBoxLayout(panel);
  layout->setContentsMargins(14, 14, 14, 14);
  layout->setSpacing(10);

  auto* titleLabel = new QLabel(title);
  QFont titleFont = titleLabel->font();
  titleFont.setBold(true);
  titleFont.setPointSize(titleFont.pointSize() + 1);
  titleLabel->setFont(titleFont);

  auto* descLabel = new QLabel(description);
  descLabel->setWordWrap(true);

  layout->addWidget(titleLabel);
  layout->addWidget(descLabel);
  layout->addWidget(content);

  root->addWidget(panel);
  anchors_.append(panel);
  titles_.append(title);
}

QVector<SelectDocsPage::Option> SelectDocsPage::basicOptions() const {
  return {
      makeOption("jack", "Jack"),
      makeOption("lucy", "Lucy"),
      makeOption("yiminghe", "yiminghe"),
      makeOption("disabled", "Disabled", true),
  };
}

QVector<SelectDocsPage::Option> SelectDocsPage::alphaNumericOptions() const {
  QVector<Option> options;
  for (int i = 10; i < 36; ++i) {
    const QString value = QString::number(i, 36) + QString::number(i);
    options.append(makeOption(value, value));
  }
  return options;
}

QVector<SelectDocsPage::Option> SelectDocsPage::cityOptions() const {
  return {
      makeOption("HangZhou", "HangZhou #310000"),
      makeOption("NingBo", "NingBo #315000"),
      makeOption("WenZhou", "WenZhou #325000"),
  };
}

QWidget* SelectDocsPage::buildBasicDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(12);

  auto* normal = new AdSelect();
  normal->setPlaceholder("Select");
  normal->setOptions(basicOptions());
  normal->setValue("lucy");
  normal->setFixedWidth(180);

  auto* disabled = new AdSelect();
  disabled->setOptions({makeOption("lucy", "Lucy")});
  disabled->setValue("lucy");
  disabled->setDisabled(true);
  disabled->setFixedWidth(180);

  auto* loading = new AdSelect();
  loading->setOptions({makeOption("lucy", "Lucy")});
  loading->setValue("lucy");
  loading->setLoading(true);
  loading->setFixedWidth(180);

  auto* allowClear = new AdSelect();
  allowClear->setPlaceholder("select it");
  allowClear->setAllowClear(true);
  allowClear->setOptions({makeOption("lucy", "Lucy")});
  allowClear->setValue("lucy");
  allowClear->setFixedWidth(180);

  row->addWidget(normal);
  row->addWidget(disabled);
  row->addWidget(loading);
  row->addWidget(allowClear);
  row->addStretch();
  return box;
}

QWidget* SelectDocsPage::buildSearchDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* select = new AdSelect();
  select->setPlaceholder("Select a person");
  select->setSearchEnabled(true);
  select->setOptions(basicOptions());
  select->setFixedWidth(320);

  auto* output = makeHintLabel("Search: ");
  connect(select, &AdSelect::searchTextChanged, output, [output](const QString& value) {
    output->setText(QStringLiteral("Search: %1").arg(value));
  });

  layout->addWidget(select, 0, Qt::AlignLeft);
  layout->addWidget(output);
  return box;
}

QWidget* SelectDocsPage::buildSearchFilterOptionDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);

  auto* select = new AdSelect();
  select->setPlaceholder("Select a person");
  select->setSearchEnabled(true);
  select->setOptions({makeOption("1", "Jack"), makeOption("2", "Lucy"), makeOption("3", "Tom")});
  select->setFilterPredicate([](const QString& input, const Option& option) {
    return option.label.toLower().contains(input.toLower());
  });
  select->setFixedWidth(260);

  layout->addWidget(select, 0, Qt::AlignLeft);
  return box;
}

QWidget* SelectDocsPage::buildSearchMultiFieldDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);

  auto* select = new AdSelect();
  select->setPlaceholder("Select an option");
  select->setSearchEnabled(true);
  select->setSearchFilterFields({"label", "otherField"});
  select->setOptions({
      makeOption("a11", "a11", false, QString(), {{"otherField", "c11"}}),
      makeOption("b22", "b22", false, QString(), {{"otherField", "b11"}}),
      makeOption("c33", "c33", false, QString(), {{"otherField", "b33"}}),
      makeOption("d44", "d44", false, QString(), {{"otherField", "d44"}}),
  });
  select->setFixedWidth(260);

  layout->addWidget(select, 0, Qt::AlignLeft);
  layout->addWidget(makeHintLabel("Type either label or otherField value to filter."));
  return box;
}

QWidget* SelectDocsPage::buildMultipleDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* first = new AdSelect();
  first->setMode(AdSelect::Mode::Multiple);
  first->setAllowClear(true);
  first->setPlaceholder("Please select");
  first->setOptions(alphaNumericOptions());
  first->setValues({"a10", "c12"});

  auto* second = new AdSelect();
  second->setMode(AdSelect::Mode::Multiple);
  second->setPlaceholder("Please select");
  second->setOptions(alphaNumericOptions());
  second->setValues({"a10", "c12"});
  second->setDisabled(true);

  layout->addWidget(first);
  layout->addWidget(second);
  return box;
}

QWidget* SelectDocsPage::buildSizeDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* controls = new QHBoxLayout();
  controls->addWidget(new QLabel("size:"));
  auto* large = new QRadioButton("large");
  auto* middle = new QRadioButton("default");
  auto* small = new QRadioButton("small");
  large->setChecked(true);
  controls->addWidget(large);
  controls->addWidget(middle);
  controls->addWidget(small);
  controls->addStretch();

  auto* single = new AdSelect();
  single->setOptions(alphaNumericOptions());
  single->setValue("a10");
  single->setFixedWidth(220);

  auto* multiple = new AdSelect();
  multiple->setMode(AdSelect::Mode::Multiple);
  multiple->setOptions(alphaNumericOptions());
  multiple->setValues({"a10", "c12"});

  auto* tags = new AdSelect();
  tags->setMode(AdSelect::Mode::Tags);
  tags->setOptions(alphaNumericOptions());
  tags->setValues({"a10", "c12"});

  auto applySize = [single, multiple, tags](AdSelect::Size size) {
    single->setSize(size);
    multiple->setSize(size);
    tags->setSize(size);
  };

  connect(large, &QRadioButton::clicked, this, [applySize]() { applySize(AdSelect::Size::Large); });
  connect(middle, &QRadioButton::clicked, this,
          [applySize]() { applySize(AdSelect::Size::Middle); });
  connect(small, &QRadioButton::clicked, this, [applySize]() { applySize(AdSelect::Size::Small); });
  applySize(AdSelect::Size::Large);

  layout->addLayout(controls);
  layout->addWidget(single, 0, Qt::AlignLeft);
  layout->addWidget(multiple);
  layout->addWidget(tags);
  return box;
}

QWidget* SelectDocsPage::buildOptionRenderDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);

  auto* select = new AdSelect();
  select->setMode(AdSelect::Mode::Multiple);
  select->setPlaceholder("Please select your current mood.");
  select->setValues({"happy"});
  select->setOptions({
      makeOption("happy", "Happy", false, QString(), {{"emoji", ":)"}, {"desc", "Feeling Good"}}),
      makeOption("sad", "Sad", false, QString(), {{"emoji", ":("}, {"desc", "Feeling Blue"}}),
      makeOption("angry", "Angry", false, QString(), {{"emoji", ">:("}, {"desc", "Furious"}}),
      makeOption("cool", "Cool", false, QString(), {{"emoji", "B)"}, {"desc", "Chilling"}}),
      makeOption("sleepy", "Sleepy", false, QString(), {{"emoji", "-_-"}, {"desc", "Need Sleep"}}),
  });
  select->setOptionTextFormatter([](const Option& option) {
    const QString emoji = option.metadata.value("emoji").toString();
    const QString desc = option.metadata.value("desc").toString();
    return QStringLiteral("%1 %2 (%3)").arg(emoji, option.label, desc);
  });

  layout->addWidget(select);
  return box;
}

QWidget* SelectDocsPage::buildSearchSortDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);

  auto* select = new AdSelect();
  select->setSearchEnabled(true);
  select->setPlaceholder("Search to Select");
  select->setFixedWidth(220);
  select->setOptions({
      makeOption("1", "Not Identified"),
      makeOption("2", "Closed"),
      makeOption("3", "Communicated"),
      makeOption("4", "Identified"),
      makeOption("5", "Resolved"),
      makeOption("6", "Cancelled"),
  });
  select->setSortComparator([](const Option& a, const Option& b) {
    return a.label.toLower() < b.label.toLower();
  });

  layout->addWidget(select, 0, Qt::AlignLeft);
  return box;
}

QWidget* SelectDocsPage::buildTagsDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);

  auto* select = new AdSelect();
  select->setMode(AdSelect::Mode::Tags);
  select->setPlaceholder("Tags Mode");
  select->setOptions(alphaNumericOptions());

  layout->addWidget(select);
  return box;
}

QWidget* SelectDocsPage::buildOptGroupDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);

  auto* select = new AdSelect();
  select->setOptions({
      makeOption("Jack", "Jack", false, "manager"),
      makeOption("Lucy", "Lucy", false, "manager"),
      makeOption("Chloe", "Chloe", false, "engineer"),
      makeOption("Lucas", "Lucas", false, "engineer"),
  });
  select->setValue("Lucy");
  select->setFixedWidth(260);

  layout->addWidget(select, 0, Qt::AlignLeft);
  return box;
}

QWidget* SelectDocsPage::buildCoordinateDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(8);

  const QMap<QString, QStringList> cityData = {
      {"Zhejiang", {"Hangzhou", "Ningbo", "Wenzhou"}},
      {"Jiangsu", {"Nanjing", "Suzhou", "Zhenjiang"}},
  };

  auto* province = new AdSelect();
  province->setFixedWidth(140);
  province->setOptions({makeOption("Zhejiang", "Zhejiang"), makeOption("Jiangsu", "Jiangsu")});
  province->setValue("Zhejiang");

  auto* city = new AdSelect();
  city->setFixedWidth(140);
  auto resetCity = [city, cityData](const QString& provinceName) {
    QVector<Option> options;
    const QStringList list = cityData.value(provinceName);
    for (const QString& value : list) {
      options.append(makeOption(value, value));
    }
    city->setOptions(options);
    if (!list.isEmpty()) {
      city->setValue(list.constFirst());
    }
  };
  resetCity("Zhejiang");

  connect(province, &AdSelect::valueChanged, city, [resetCity](const QString& value) { resetCity(value); });

  row->addWidget(province);
  row->addWidget(city);
  row->addStretch();
  return box;
}

QWidget* SelectDocsPage::buildSearchBoxDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* select = new AdSelect();
  select->setSearchEnabled(true);
  select->setFixedWidth(260);
  select->setPlaceholder("input search text");
  select->setFilterPredicate([](const QString&, const Option&) { return true; });

  auto requestId = std::make_shared<int>(0);
  auto* hint = makeHintLabel("Type to fetch local mock results...");
  connect(select, &AdSelect::searchTextChanged, select,
          [select, requestId, hint](const QString& value) {
            *requestId += 1;
            const int current = *requestId;
            select->setLoading(true);
            hint->setText(QStringLiteral("Fetching: %1").arg(value));
            adqt::widgets::detail::scheduleTimingTask(
                select, QStringLiteral("ThemeDemo.SelectSearchBox"), 300,
                [select, requestId, current, value, hint]() {
                  if (current != *requestId) {
                    return;
                  }
                  QVector<Option> options;
                  if (!value.trimmed().isEmpty()) {
                    for (int i = 0; i < 5; ++i) {
                      const QString text = QStringLiteral("%1-result-%2").arg(value).arg(i + 1);
                      options.append(makeOption(text, text));
                    }
                  }
                  select->setOptions(options);
                  select->setLoading(false);
                  hint->setText(QStringLiteral("Loaded %1 options").arg(options.size()));
                  if (select->open()) {
                    select->setOpen(true);
                  }
                });
          });

  layout->addWidget(select, 0, Qt::AlignLeft);
  layout->addWidget(hint);
  return box;
}

QWidget* SelectDocsPage::buildLabelInValueDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* select = new AdSelect();
  select->setOptions({makeOption("jack", "Jack (100)"), makeOption("lucy", "Lucy (101)")});
  select->setValue("lucy");
  select->setFixedWidth(180);

  auto* output = makeHintLabel("Selected: { value: 'lucy', label: 'Lucy (101)' }");
  connect(select, &AdSelect::selectionChanged, output,
          [output](const QVector<AdSelect::SelectionItem>& items) {
            if (items.isEmpty()) {
              output->setText("Selected: {}");
              return;
            }
            const auto& item = items.constFirst();
            output->setText(
                QStringLiteral("Selected: { value: '%1', label: '%2' }").arg(item.value, item.label));
          });

  layout->addWidget(select, 0, Qt::AlignLeft);
  layout->addWidget(output);
  return box;
}

QWidget* SelectDocsPage::buildAutomaticTokenizationDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);

  auto* select = new AdSelect();
  select->setMode(AdSelect::Mode::Tags);
  select->setTokenSeparators({","});
  select->setPlaceholder("Type words and separate with comma");
  select->setOptions(alphaNumericOptions());

  layout->addWidget(select);
  return box;
}

QWidget* SelectDocsPage::buildSelectUsersDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* select = new AdSelect();
  select->setMode(AdSelect::Mode::Multiple);
  select->setSearchEnabled(true);
  select->setPlaceholder("Select users");
  select->setFilterPredicate([](const QString&, const Option&) { return true; });

  auto* hint = makeHintLabel("Search users (local async mock)");
  auto requestId = std::make_shared<int>(0);
  connect(select, &AdSelect::searchTextChanged, select,
          [select, hint, requestId](const QString& value) {
            *requestId += 1;
            const int current = *requestId;
            select->setLoading(true);
            adqt::widgets::detail::scheduleTimingTask(
                select, QStringLiteral("ThemeDemo.SelectUsersSearch"), 280,
                [select, hint, requestId, current, value]() {
                  if (current != *requestId) {
                    return;
                  }
                  QVector<Option> options;
                  if (!value.trimmed().isEmpty()) {
                    for (int i = 0; i < 8; ++i) {
                      const QString id = QStringLiteral("%1-%2").arg(value).arg(i + 1);
                      options.append(makeOption(id, QStringLiteral("User %1").arg(id), false, QString(),
                                                {{"avatar", QStringLiteral("[%1]").arg(i + 1)}}));
                    }
                  }
                  select->setOptions(options);
                  select->setLoading(false);
                  hint->setText(QStringLiteral("Loaded users: %1").arg(options.size()));
                });
          });
  select->setOptionTextFormatter([](const Option& option) {
    const QString avatar = option.metadata.value("avatar").toString();
    return QStringLiteral("%1 %2").arg(avatar, option.label);
  });

  layout->addWidget(select);
  layout->addWidget(hint);
  return box;
}

QWidget* SelectDocsPage::buildSuffixDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(10);

  auto* prefixed = new AdSelect();
  prefixed->setPrefixText("User");
  prefixed->setSearchEnabled(true);
  prefixed->setAllowClear(true);
  prefixed->setOptions(basicOptions());
  prefixed->setValue("lucy");
  prefixed->setFixedWidth(220);

  auto* suffixA = new AdSelect();
  suffixA->setSuffixIconToken(outlined_icons::Smile());
  suffixA->setOptions(basicOptions());
  suffixA->setValue("lucy");
  suffixA->setFixedWidth(170);

  auto* suffixB = new AdSelect();
  suffixB->setSuffixIconToken(outlined_icons::Meh());
  suffixB->setOptions({makeOption("lucy", "Lucy")});
  suffixB->setValue("lucy");
  suffixB->setDisabled(true);
  suffixB->setFixedWidth(170);

  auto* multi = new AdSelect();
  multi->setMode(AdSelect::Mode::Multiple);
  multi->setPrefixText("User");
  multi->setOptions(basicOptions());
  multi->setValues({"lucy"});
  multi->setFixedWidth(220);

  row->addWidget(prefixed);
  row->addWidget(suffixA);
  row->addWidget(suffixB);
  row->addWidget(multi);
  row->addStretch();
  return box;
}

QWidget* SelectDocsPage::buildCustomDropdownDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);

  auto* select = new AdSelect();
  select->setPlaceholder("custom dropdown render");
  select->setFixedWidth(320);
  QVector<Option> items = {makeOption("jack", "jack"), makeOption("lucy", "lucy")};
  select->setOptions(items);

  auto dynamicItems = std::make_shared<QVector<Option>>(items);
  auto index = std::make_shared<int>(0);

  select->setPopupExtraContentFactory([select, dynamicItems, index](QWidget* parent) -> QWidget* {
    auto* panel = new QWidget(parent);
    auto* row = new QHBoxLayout(panel);
    row->setContentsMargins(0, 4, 0, 0);
    row->setSpacing(6);

    auto* input = new QLineEdit(panel);
    input->setPlaceholderText("Please enter item");
    auto* add = new QPushButton("Add item", panel);

    connect(add, &QPushButton::clicked, panel, [select, dynamicItems, index, input]() {
      const QString text = input->text().trimmed().isEmpty()
                               ? QStringLiteral("New item %1").arg((*index)++)
                               : input->text().trimmed();
      dynamicItems->append(makeOption(text, text));
      select->setOptions(*dynamicItems);
      input->clear();
      select->setOpen(true);
    });

    row->addWidget(input, 1);
    row->addWidget(add);
    return panel;
  });

  layout->addWidget(select, 0, Qt::AlignLeft);
  return box;
}

QWidget* SelectDocsPage::buildHideSelectedDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);

  const QStringList source = {"Apples", "Nails", "Bananas", "Helicopters"};
  auto* select = new AdSelect();
  select->setMode(AdSelect::Mode::Multiple);
  select->setPlaceholder("Inserted are removed");

  auto rebuild = [select, source](const QStringList& selectedValues) {
    QVector<Option> options;
    for (const QString& value : source) {
      if (!selectedValues.contains(value)) {
        options.append(makeOption(value, value));
      }
    }
    select->setOptions(options);
  };

  connect(select, &AdSelect::valuesChanged, select, [rebuild](const QStringList& values) { rebuild(values); });
  rebuild({});

  layout->addWidget(select);
  return box;
}

QWidget* SelectDocsPage::buildVariantDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  const QVector<QPair<QString, AdSelect::Variant>> variants = {
      {"Outlined", AdSelect::Variant::Outlined},
      {"Filled", AdSelect::Variant::Filled},
      {"Borderless", AdSelect::Variant::Borderless},
      {"Underlined", AdSelect::Variant::Underlined},
  };

  for (const auto& item : variants) {
    auto* row = new QHBoxLayout();
    row->setSpacing(8);

    auto* single = new AdSelect();
    single->setVariant(item.second);
    single->setPlaceholder(item.first);
    single->setOptions(basicOptions());
    single->setValue("lucy");
    single->setFixedWidth(220);

    auto* multi = new AdSelect();
    multi->setMode(AdSelect::Mode::Multiple);
    multi->setVariant(item.second);
    multi->setPlaceholder(item.first);
    multi->setOptions(basicOptions());
    multi->setValues({"lucy"});
    multi->setFixedWidth(220);

    row->addWidget(single);
    row->addWidget(multi);
    row->addStretch();
    layout->addLayout(row);
  }

  return box;
}

QWidget* SelectDocsPage::buildCustomTagRenderDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);

  auto* select = new AdSelect();
  select->setMode(AdSelect::Mode::Multiple);
  select->setOptions({makeOption("gold", "gold"), makeOption("lime", "lime"),
                      makeOption("green", "green"), makeOption("cyan", "cyan")});
  select->setValues({"gold", "cyan"});
  select->setTagTextFormatter([](const Option& option) {
    return QStringLiteral("[%1]").arg(option.label.toUpper());
  });

  layout->addWidget(select);
  return box;
}

QWidget* SelectDocsPage::buildCustomLabelRenderDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);

  auto* select = new AdSelect();
  select->setOptions({makeOption("gold", "gold"), makeOption("lime", "lime"),
                      makeOption("green", "green"), makeOption("cyan", "cyan")});
  select->setValue("gold");
  select->setLabelFormatter([](const Option& option) { return QStringLiteral("value=%1").arg(option.value); });

  layout->addWidget(select, 0, Qt::AlignLeft);
  return box;
}

QWidget* SelectDocsPage::buildResponsiveDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* select = new AdSelect();
  select->setMode(AdSelect::Mode::Multiple);
  select->setOptions(alphaNumericOptions());
  select->setValues({"a10", "c12", "h17", "j19", "k20"});
  select->setResponsiveMaxTagCount(true);
  select->setPlaceholder("Select Item...");

  auto* disabled = new AdSelect();
  disabled->setMode(AdSelect::Mode::Multiple);
  disabled->setOptions(alphaNumericOptions());
  disabled->setValues({"a10", "c12", "h17", "j19", "k20"});
  disabled->setResponsiveMaxTagCount(true);
  disabled->setDisabled(true);

  layout->addWidget(select);
  layout->addWidget(disabled);
  return box;
}

QWidget* SelectDocsPage::buildStatusDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* error = new AdSelect();
  error->setStatus(AdSelect::Status::Error);
  error->setPlaceholder("Error");
  error->setOptions(basicOptions());

  auto* warning = new AdSelect();
  warning->setStatus(AdSelect::Status::Warning);
  warning->setPlaceholder("Warning");
  warning->setOptions(basicOptions());

  layout->addWidget(error);
  layout->addWidget(warning);
  return box;
}

QWidget* SelectDocsPage::buildPlacementDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* radio = new QComboBox();
  radio->addItem("topLeft", static_cast<int>(AdSelect::Placement::TopLeft));
  radio->addItem("topRight", static_cast<int>(AdSelect::Placement::TopRight));
  radio->addItem("bottomLeft", static_cast<int>(AdSelect::Placement::BottomLeft));
  radio->addItem("bottomRight", static_cast<int>(AdSelect::Placement::BottomRight));
  radio->setCurrentIndex(0);

  auto* select = new AdSelect();
  select->setOptions(cityOptions());
  select->setValue("HangZhou");
  select->setPopupMatchSelectWidth(false);
  select->setFixedWidth(120);

  connect(radio, QOverload<int>::of(&QComboBox::currentIndexChanged), select,
          [radio, select](int) {
            select->setPlacement(static_cast<AdSelect::Placement>(radio->currentData().toInt()));
          });

  layout->addWidget(radio, 0, Qt::AlignLeft);
  layout->addWidget(select, 0, Qt::AlignLeft);
  return box;
}

QWidget* SelectDocsPage::buildMaxCountDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* select = new AdSelect();
  select->setMode(AdSelect::Mode::Multiple);
  select->setMaxCount(3);
  select->setOptions({
      makeOption("Ava Swift", "Ava Swift"),
      makeOption("Cole Reed", "Cole Reed"),
      makeOption("Mia Blake", "Mia Blake"),
      makeOption("Jake Stone", "Jake Stone"),
      makeOption("Lily Lane", "Lily Lane"),
      makeOption("Ryan Chase", "Ryan Chase"),
      makeOption("Zoe Fox", "Zoe Fox"),
      makeOption("Alex Grey", "Alex Grey"),
      makeOption("Elle Blair", "Elle Blair"),
  });
  select->setValues({"Ava Swift"});

  auto* suffix = makeHintLabel("1 / 3");
  connect(select, &AdSelect::valuesChanged, suffix,
          [suffix](const QStringList& values) { suffix->setText(QStringLiteral("%1 / 3").arg(values.size())); });

  layout->addWidget(select);
  layout->addWidget(suffix);
  return box;
}

QWidget* SelectDocsPage::buildStyleClassDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* objectStyle = new AdSelect();
  objectStyle->setPrefixIconToken(outlined_icons::Meh());
  objectStyle->setOptions({makeOption("GuangZhou", "GuangZhou"), makeOption("ShenZhen", "ShenZhen")});

  AdSelect::SemanticStyles semantic;
  semantic.prefix.textColor = QColor("#1890ff");
  semantic.suffix.textColor = QColor("#1890ff");
  objectStyle->setSemanticStyles(semantic);

  auto* functionStyle = new AdSelect();
  functionStyle->setPrefixIconToken(outlined_icons::Meh());
  functionStyle->setVariant(AdSelect::Variant::Filled);
  functionStyle->setOptions({makeOption("GuangZhou", "GuangZhou"), makeOption("ShenZhen", "ShenZhen")});
  functionStyle->setSemanticStyleResolver([](const AdSelect::StyleContext& ctx) {
    AdSelect::SemanticStyles styles;
    if (ctx.variant == AdSelect::Variant::Filled) {
      styles.prefix.textColor = QColor("#722ed1");
      styles.suffix.textColor = QColor("#722ed1");
      styles.popup.borderColor = QColor("#722ed1");
    }
    return styles;
  });

  layout->addWidget(objectStyle);
  layout->addWidget(functionStyle);
  return box;
}
