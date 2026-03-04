#include "switch_docs_page.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "icons.h"

using adqt::widgets::AdSwitch;
namespace outlined_icons = adqt::icons::outlined;

SwitchDocsPage::SwitchDocsPage(QWidget* parent) : QWidget(parent) {
  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(16, 16, 16, 24);
  root->setSpacing(16);

  auto* title = new QLabel("Switch");
  QFont titleFont = title->font();
  titleFont.setPointSize(titleFont.pointSize() + 8);
  titleFont.setBold(true);
  title->setFont(titleFont);
  root->addWidget(title);

  auto* subtitle = new QLabel("Used to toggle between two states.");
  subtitle->setWordWrap(true);
  root->addWidget(subtitle);

  addSection(root, "Basic", "Demo: basic.tsx", buildBasicDemo());
  addSection(root, "Disabled", "Demo: disabled.tsx", buildDisabledDemo());
  addSection(root, "Text & icon", "Demo: text.tsx", buildTextDemo());
  addSection(root, "Two sizes", "Demo: size.tsx", buildSizeDemo());
  addSection(root, "Loading", "Demo: loading.tsx", buildLoadingDemo());
  addSection(root, "Custom component token", "Demo: component-token.tsx", buildComponentTokenDemo());
  addSection(root, "Custom semantic dom styling", "Demo: style-class.tsx", buildStyleClassDemo());

  root->addStretch();
}

const QVector<QWidget*>& SwitchDocsPage::sectionAnchors() const { return anchors_; }

const QStringList& SwitchDocsPage::sectionTitles() const { return titles_; }

void SwitchDocsPage::addSection(QVBoxLayout* root,
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

QWidget* SwitchDocsPage::buildBasicDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);

  auto* sw = new AdSwitch();
  sw->setChecked(true);
  row->addWidget(sw);
  row->addStretch();
  return box;
}

QWidget* SwitchDocsPage::buildDisabledDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(10);

  auto* sw = new AdSwitch();
  sw->setChecked(true);
  sw->setDisabled(true);

  auto* toggle = new QPushButton("Toggle disabled");
  connect(toggle, &QPushButton::clicked, sw, [sw]() { sw->setDisabled(!sw->disabled()); });

  auto* row = new QHBoxLayout();
  row->setContentsMargins(0, 0, 0, 0);
  row->addWidget(sw);
  row->addStretch();

  layout->addLayout(row);
  layout->addWidget(toggle, 0, Qt::AlignLeft);
  return box;
}

QWidget* SwitchDocsPage::buildTextDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* first = new AdSwitch();
  first->setCheckedChildren("ON");
  first->setUnCheckedChildren("OFF");
  first->setChecked(true);

  auto* second = new AdSwitch();
  second->setCheckedChildren("1");
  second->setUnCheckedChildren("0");

  auto* third = new AdSwitch();
  third->setCheckedChildrenIconToken(outlined_icons::Check());
  third->setUnCheckedChildrenIconToken(outlined_icons::Close());
  third->setChecked(true);

  auto* row1 = new QHBoxLayout();
  row1->setContentsMargins(0, 0, 0, 0);
  row1->setSpacing(8);
  row1->addWidget(first);
  row1->addWidget(second);
  row1->addWidget(third);
  row1->addStretch();

  layout->addLayout(row1);
  return box;
}

QWidget* SwitchDocsPage::buildSizeDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* first = new AdSwitch();
  first->setChecked(true);

  auto* second = new AdSwitch();
  second->setSize(AdSwitch::Size::Small);
  second->setChecked(true);

  auto* row1 = new QHBoxLayout();
  row1->setContentsMargins(0, 0, 0, 0);
  row1->addWidget(first);
  row1->addStretch();

  auto* row2 = new QHBoxLayout();
  row2->setContentsMargins(0, 0, 0, 0);
  row2->addWidget(second);
  row2->addStretch();

  layout->addLayout(row1);
  layout->addLayout(row2);
  return box;
}

QWidget* SwitchDocsPage::buildLoadingDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* first = new AdSwitch();
  first->setLoading(true);
  first->setChecked(true);

  auto* second = new AdSwitch();
  second->setSize(AdSwitch::Size::Small);
  second->setLoading(true);

  auto* row1 = new QHBoxLayout();
  row1->setContentsMargins(0, 0, 0, 0);
  row1->addWidget(first);
  row1->addStretch();

  auto* row2 = new QHBoxLayout();
  row2->setContentsMargins(0, 0, 0, 0);
  row2->addWidget(second);
  row2->addStretch();

  layout->addLayout(row1);
  layout->addLayout(row2);
  return box;
}

QWidget* SwitchDocsPage::buildComponentTokenDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);

  auto* sw = new AdSwitch();
  sw->setChecked(true);
  AdSwitch::ComponentTokens tokens;
  tokens.trackHeight = 14;
  tokens.trackMinWidth = 32;
  tokens.colorPrimary = QStringLiteral("rgba(25,118,210,0.5)");
  tokens.trackPadding = 0;
  tokens.handleSize = 20;
  tokens.handleBg = QStringLiteral("rgb(25,118,210)");
  tokens.handleShadow = QStringLiteral("rgba(0,0,0,0.22)");
  sw->setComponentTokens(tokens);

  row->addWidget(sw);
  row->addStretch();
  return box;
}

QWidget* SwitchDocsPage::buildStyleClassDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(10);

  auto* objectStyled = new AdSwitch();
  objectStyled->setSize(AdSwitch::Size::Small);
  objectStyled->setCheckedChildren("on");
  objectStyled->setUnCheckedChildren("off");
  objectStyled->setChecked(true);

  AdSwitch::SemanticStyles styles;
  styles.root.backgroundColor = QColor("#F5D2D2");
  styles.content.textColor = QColor("#4A4A4A");
  styles.indicator.backgroundColor = QColor("#FFFFFF");
  objectStyled->setSemanticStyles(styles);

  auto* functionStyled = new AdSwitch();
  functionStyled->setCheckedChildren("on");
  functionStyled->setUnCheckedChildren("off");
  functionStyled->setSemanticStyleResolver([](const AdSwitch::StyleContext& ctx) {
    AdSwitch::SemanticStyles out;
    if (ctx.size == AdSwitch::Size::Default) {
      out.root.backgroundColor = QColor("#BDE3C3");
      out.content.textColor = QColor("#214D28");
    }
    if (ctx.checked) {
      out.indicator.backgroundColor = QColor("#FFFFFF");
    }
    return out;
  });

  auto* row = new QHBoxLayout();
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(8);
  row->addWidget(objectStyled);
  row->addWidget(functionStyled);
  row->addStretch();

  layout->addLayout(row);
  return box;
}

