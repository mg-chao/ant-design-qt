#include "radio_docs_page.h"

#include <QButtonGroup>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPalette>
#include <QPushButton>
#include <QVBoxLayout>

using adqt::widgets::AdRadio;
using adqt::widgets::AdRadioGroup;

namespace {

QLabel* makeHintLabel(const QString& text, QWidget* parent = nullptr) {
  auto* label = new QLabel(text, parent);
  label->setWordWrap(true);
  QPalette palette = label->palette();
  palette.setColor(QPalette::WindowText, QColor("#8c8c8c"));
  label->setPalette(palette);
  return label;
}

AdRadioGroup::Option makeOption(const QVariant& value,
                                const QString& label,
                                bool disabled = false,
                                const QString& title = QString(),
                                const QString& id = QString(),
                                const QString& className = QString(),
                                bool required = false) {
  AdRadioGroup::Option option;
  option.value = value;
  option.label = label;
  option.disabled = disabled;
  option.title = title;
  option.id = id;
  option.className = className;
  option.required = required;
  return option;
}

}  // namespace

RadioDocsPage::RadioDocsPage(QWidget* parent) : QWidget(parent) {
  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(16, 16, 16, 24);
  root->setSpacing(16);

  auto* title = new QLabel("Radio");
  QFont titleFont = title->font();
  titleFont.setPointSize(titleFont.pointSize() + 8);
  titleFont.setBold(true);
  title->setFont(titleFont);
  root->addWidget(title);

  auto* subtitle = new QLabel(
      "Used to select a single state from multiple options. This page mirrors Ant Design "
      "Radio public demos.");
  subtitle->setWordWrap(true);
  root->addWidget(subtitle);

  addSection(root, "Basic", "Demo: basic.tsx", buildBasicDemo());
  addSection(root, "disabled", "Demo: disabled.tsx", buildDisabledDemo());
  addSection(root, "Radio Group", "Demo: radiogroup.tsx", buildRadioGroupDemo());
  addSection(root, "Vertical Radio.Group", "Demo: radiogroup-more.tsx", buildVerticalGroupDemo());
  addSection(root, "Block Radio.Group", "Demo: radiogroup-block.tsx", buildBlockGroupDemo());
  addSection(root, "Radio.Group group - optional", "Demo: radiogroup-options.tsx",
             buildGroupOptionsDemo());
  addSection(root, "radio style", "Demo: radiobutton.tsx", buildRadioButtonDemo());
  addSection(root, "Radio.Group with name", "Demo: radiogroup-with-name.tsx",
             buildGroupWithNameDemo());
  addSection(root, "Size", "Demo: size.tsx", buildSizeDemo());
  addSection(root, "Solid radio button", "Demo: radiobutton-solid.tsx", buildSolidButtonDemo());
  addSection(root, "Custom semantic dom styling", "Demo: style-class.tsx", buildStyleClassDemo());

  root->addStretch();
}

const QVector<QWidget*>& RadioDocsPage::sectionAnchors() const { return anchors_; }

const QStringList& RadioDocsPage::sectionTitles() const { return titles_; }

void RadioDocsPage::addSection(QVBoxLayout* root,
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

QVector<RadioDocsPage::RadioOption> RadioDocsPage::cityOptions() const {
  return {
      makeOption("a", "Hangzhou"),
      makeOption("b", "Shanghai"),
      makeOption("c", "Beijing"),
      makeOption("d", "Chengdu"),
  };
}

QVector<RadioDocsPage::RadioOption> RadioDocsPage::fruitOptions(bool disableOrange) const {
  return {
      makeOption("Apple", "Apple", false, QString(), QString(), "label-1"),
      makeOption("Pear", "Pear", false, QString(), QString(), "label-2"),
      makeOption("Orange", "Orange", disableOrange, "Orange", QString(), "label-3"),
  };
}

QWidget* RadioDocsPage::buildBasicDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);

  auto* radio = new AdRadio("Radio");
  row->addWidget(radio);
  row->addStretch();
  return box;
}

QWidget* RadioDocsPage::buildDisabledDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(10);

  auto* row = new QHBoxLayout();
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(12);

  auto* first = new AdRadio("Disabled");
  first->setChecked(false);
  first->setDisabled(true);
  auto* second = new AdRadio("Disabled");
  second->setChecked(true);
  second->setDisabled(true);

  row->addWidget(first);
  row->addWidget(second);
  row->addStretch();

  auto* toggle = new QPushButton("Toggle disabled");
  connect(toggle, &QPushButton::clicked, this, [first, second]() {
    const bool next = !first->disabled();
    first->setDisabled(next);
    second->setDisabled(next);
  });

  layout->addLayout(row);
  layout->addWidget(toggle, 0, Qt::AlignLeft);
  return box;
}

QWidget* RadioDocsPage::buildRadioGroupDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* group = new AdRadioGroup();
  group->setOptions({
      makeOption(1, "LineChart"),
      makeOption(2, "DotChart"),
      makeOption(3, "BarChart"),
      makeOption(4, "PieChart"),
  });
  group->setValue(1);

  auto* output = makeHintLabel("selected: 1");
  connect(group, &AdRadioGroup::valueChanged, output, [output](const QVariant& value) {
    output->setText(QStringLiteral("selected: %1").arg(value.toString()));
  });

  layout->addWidget(group, 0, Qt::AlignLeft);
  layout->addWidget(output);
  return box;
}

QWidget* RadioDocsPage::buildVerticalGroupDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* group = new AdRadioGroup();
  group->setVertical(true);
  group->setOptions({
      makeOption(1, "Option A"),
      makeOption(2, "Option B"),
      makeOption(3, "Option C"),
      makeOption(4, "More..."),
  });
  group->setValue(1);

  // Match antd `radiogroup-more.tsx` labelStyle: { height: 32, lineHeight: '32px' }.
  const auto radios = group->findChildren<AdRadio*>(QString(), Qt::FindDirectChildrenOnly);
  for (AdRadio* radio : radios) {
    if (!radio) {
      continue;
    }
    radio->setFixedHeight(32);
  }

  auto* moreInput = new QLineEdit();
  moreInput->setPlaceholderText("please input");
  moreInput->setVisible(false);
  moreInput->setFixedWidth(120);

  connect(group, &AdRadioGroup::valueChanged, moreInput, [moreInput](const QVariant& value) {
    moreInput->setVisible(value.toInt() == 4);
  });

  layout->addWidget(group, 0, Qt::AlignLeft);
  layout->addWidget(moreInput, 0, Qt::AlignLeft);
  return box;
}

QWidget* RadioDocsPage::buildBlockGroupDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(10);

  auto* group1 = new AdRadioGroup();
  group1->setBlock(true);
  group1->setOptions(fruitOptions(false));
  group1->setValue("Apple");
  group1->setFixedWidth(520);

  auto* group2 = new AdRadioGroup();
  group2->setBlock(true);
  group2->setOptionType(AdRadio::OptionType::Button);
  group2->setButtonStyle(AdRadio::ButtonStyle::Solid);
  group2->setOptions(fruitOptions(false));
  group2->setValue("Apple");
  group2->setFixedWidth(520);

  auto* group3 = new AdRadioGroup();
  group3->setBlock(true);
  group3->setOptionType(AdRadio::OptionType::Button);
  group3->setOptions(fruitOptions(false));
  group3->setValue("Pear");
  group3->setFixedWidth(520);

  layout->addWidget(group1);
  layout->addWidget(group2);
  layout->addWidget(group3);
  return box;
}

QWidget* RadioDocsPage::buildGroupOptionsDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(10);

  auto* group1 = new AdRadioGroup();
  group1->setOptions(fruitOptions(false));
  group1->setValue("Apple");

  auto* group2 = new AdRadioGroup();
  group2->setOptions(fruitOptions(true));
  group2->setValue("Apple");

  auto* group3 = new AdRadioGroup();
  group3->setOptionType(AdRadio::OptionType::Button);
  group3->setOptions(fruitOptions(false));
  group3->setValue("Apple");

  auto* group4 = new AdRadioGroup();
  group4->setOptionType(AdRadio::OptionType::Button);
  group4->setButtonStyle(AdRadio::ButtonStyle::Solid);
  group4->setOptions(fruitOptions(true));
  group4->setValue("Apple");

  layout->addWidget(group1, 0, Qt::AlignLeft);
  layout->addWidget(group2, 0, Qt::AlignLeft);
  layout->addWidget(group3, 0, Qt::AlignLeft);
  layout->addWidget(group4, 0, Qt::AlignLeft);
  return box;
}

QWidget* RadioDocsPage::buildRadioButtonDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(10);

  auto* first = new AdRadioGroup();
  first->setOptionType(AdRadio::OptionType::Button);
  first->setOptions(cityOptions());
  first->setValue("a");

  auto* second = new AdRadioGroup();
  second->setOptionType(AdRadio::OptionType::Button);
  second->setOptions({
      makeOption("a", "Hangzhou"),
      makeOption("b", "Shanghai", true),
      makeOption("c", "Beijing"),
      makeOption("d", "Chengdu"),
  });
  second->setValue("a");

  auto* third = new AdRadioGroup();
  third->setOptionType(AdRadio::OptionType::Button);
  third->setOptions(cityOptions());
  third->setValue("a");
  third->setDisabled(true);

  layout->addWidget(first, 0, Qt::AlignLeft);
  layout->addWidget(second, 0, Qt::AlignLeft);
  layout->addWidget(third, 0, Qt::AlignLeft);
  return box;
}

QWidget* RadioDocsPage::buildGroupWithNameDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);

  auto* group = new AdRadioGroup();
  group->setName("radiogroup");
  group->setOptions({
      makeOption(1, "A"),
      makeOption(2, "B"),
      makeOption(3, "C"),
      makeOption(4, "D"),
  });
  group->setValue(1);

  layout->addWidget(group, 0, Qt::AlignLeft);
  layout->addWidget(makeHintLabel("All options receive the same `name` property."));
  return box;
}

QWidget* RadioDocsPage::buildSizeDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(10);

  auto* large = new AdRadioGroup();
  large->setSize(AdRadio::Size::Large);
  large->setOptionType(AdRadio::OptionType::Button);
  large->setOptions(cityOptions());
  large->setValue("a");

  auto* middle = new AdRadioGroup();
  middle->setSize(AdRadio::Size::Middle);
  middle->setOptionType(AdRadio::OptionType::Button);
  middle->setOptions(cityOptions());
  middle->setValue("a");

  auto* small = new AdRadioGroup();
  small->setSize(AdRadio::Size::Small);
  small->setOptionType(AdRadio::OptionType::Button);
  small->setOptions(cityOptions());
  small->setValue("a");

  layout->addWidget(large, 0, Qt::AlignLeft);
  layout->addWidget(middle, 0, Qt::AlignLeft);
  layout->addWidget(small, 0, Qt::AlignLeft);
  return box;
}

QWidget* RadioDocsPage::buildSolidButtonDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(10);

  auto* first = new AdRadioGroup();
  first->setOptionType(AdRadio::OptionType::Button);
  first->setButtonStyle(AdRadio::ButtonStyle::Solid);
  first->setOptions(cityOptions());
  first->setValue("a");

  auto* second = new AdRadioGroup();
  second->setOptionType(AdRadio::OptionType::Button);
  second->setButtonStyle(AdRadio::ButtonStyle::Solid);
  second->setOptions({
      makeOption("a", "Hangzhou"),
      makeOption("b", "Shanghai", true),
      makeOption("c", "Beijing"),
      makeOption("d", "Chengdu"),
  });
  second->setValue("c");

  layout->addWidget(first, 0, Qt::AlignLeft);
  layout->addWidget(second, 0, Qt::AlignLeft);
  return box;
}

QWidget* RadioDocsPage::buildStyleClassDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(10);

  auto* objectStyle = new AdRadio("Object styles");
  objectStyle->setValue("styles");
  AdRadio::SemanticStyles objectSemantic;
  objectSemantic.icon.borderColor = QColor("#faad14");
  objectSemantic.icon.backgroundColor = QColor("#fff7e6");
  objectSemantic.label.textColor = QColor("#1677ff");
  objectStyle->setSemanticStyles(objectSemantic);
  objectStyle->setChecked(true);

  auto* functionStyle = new AdRadio("Function semantic resolver");
  functionStyle->setValue("classNames");
  functionStyle->setSemanticStyleResolver([](const AdRadio::StyleContext& ctx) {
    AdRadio::SemanticStyles styles;
    if (ctx.checked) {
      styles.icon.borderColor = QColor("#faad14");
      styles.icon.backgroundColor = QColor("#faad14");
      styles.label.textColor = QColor("#d48806");
    } else {
      styles.icon.borderColor = QColor("#d9d9d9");
      styles.label.textColor = QColor("#8c8c8c");
    }
    return styles;
  });

  auto* group = new QButtonGroup(box);
  group->setExclusive(true);
  group->addButton(objectStyle);
  group->addButton(functionStyle);

  layout->addWidget(objectStyle, 0, Qt::AlignLeft);
  layout->addWidget(functionStyle, 0, Qt::AlignLeft);
  return box;
}
