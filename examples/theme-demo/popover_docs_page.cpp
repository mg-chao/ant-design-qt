#include "popover_docs_page.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

#include "icons.h"

using adqt::widgets::AdButton;
using adqt::widgets::AdPopover;

namespace {

QLabel* makeHintLabel(const QString& text, QWidget* parent = nullptr) {
  auto* label = new QLabel(text, parent);
  label->setWordWrap(true);
  QPalette palette = label->palette();
  palette.setColor(QPalette::WindowText, QColor("#8c8c8c"));
  label->setPalette(palette);
  return label;
}

}  // namespace

PopoverDocsPage::PopoverDocsPage(QWidget* parent) : QWidget(parent) {
  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(16, 16, 16, 24);
  root->setSpacing(16);

  auto* title = new QLabel("Popover");
  QFont titleFont = title->font();
  titleFont.setPointSize(titleFont.pointSize() + 8);
  titleFont.setBold(true);
  title->setFont(titleFont);
  root->addWidget(title);

  auto* subtitle = new QLabel(
      "A floating card that appears on hover, focus, click or context menu, aligned to a trigger widget.");
  subtitle->setWordWrap(true);
  root->addWidget(subtitle);

  addSection(root, "Basic", "Simple title + content card.", buildBasicDemo());
  addSection(root, "Trigger Modes", "Hover / Focus / Click / ContextMenu.", buildTriggerDemo());
  addSection(root, "Placement", "12 placements with automatic fallback and clamping.", buildPlacementDemo());
  addSection(root, "Arrow", "Show/hide arrow and point-at-center behavior.", buildArrowDemo());
  addSection(root, "Auto Shift", "When near viewport edge, placement and position auto adjust.", buildAutoShiftDemo());
  addSection(root, "Controlled Open", "External state drives open visibility.", buildControlledDemo());
  addSection(root, "Hover With Click", "Mixed nested interaction pattern.", buildHoverWithClickDemo());
  addSection(root, "Semantic Styling", "Use semantic slot style and style resolver.", buildSemanticDemo());
  addSection(root, "Component Tokens", "Override popover metrics and colors via component tokens.", buildComponentTokenDemo());
  addSection(root, "API Overview", "Core API names and property meanings.", buildApiOverview());

  root->addStretch();
}

const QVector<QWidget*>& PopoverDocsPage::sectionAnchors() const { return anchors_; }

const QStringList& PopoverDocsPage::sectionTitles() const { return titles_; }

void PopoverDocsPage::addSection(QVBoxLayout* root,
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

AdPopover* PopoverDocsPage::makePopover(const QString& triggerText,
                                        const QString& title,
                                        const QString& content,
                                        Triggers triggers,
                                        QWidget* parent) {
  auto* popover = new AdPopover(parent);
  auto* trigger = new AdButton(triggerText, popover);
  trigger->setType(AdButton::Type::Default);
  popover->setTriggerWidget(trigger);
  popover->setTitleText(title);
  popover->setContentText(content);
  popover->setTriggerModes(triggers);
  return popover;
}

QWidget* PopoverDocsPage::buildBasicDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(8);

  auto* hover = makePopover("Hover me", "Title", "Content", Trigger::Hover);
  auto* click = makePopover("Click me", "Title", "Content", Trigger::Click);
  auto* focus = makePopover("Focus me", "Title", "Focusable trigger", Trigger::Focus);

  row->addWidget(hover);
  row->addWidget(click);
  row->addWidget(focus);
  row->addStretch();
  return box;
}

QWidget* PopoverDocsPage::buildTriggerDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(8);

  row->addWidget(makePopover("Hover", "Hover", "Trigger = hover", Trigger::Hover));
  row->addWidget(makePopover("Focus", "Focus", "Trigger = focus", Trigger::Focus));
  row->addWidget(makePopover("Click", "Click", "Trigger = click", Trigger::Click));
  row->addWidget(makePopover("ContextMenu", "Context Menu", "Right click me", Trigger::ContextMenu));
  row->addStretch();
  return box;
}

QWidget* PopoverDocsPage::buildPlacementDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* placementBox = new QComboBox();
  placementBox->addItem("top", static_cast<int>(Placement::Top));
  placementBox->addItem("topLeft", static_cast<int>(Placement::TopLeft));
  placementBox->addItem("topRight", static_cast<int>(Placement::TopRight));
  placementBox->addItem("bottom", static_cast<int>(Placement::Bottom));
  placementBox->addItem("bottomLeft", static_cast<int>(Placement::BottomLeft));
  placementBox->addItem("bottomRight", static_cast<int>(Placement::BottomRight));
  placementBox->addItem("left", static_cast<int>(Placement::Left));
  placementBox->addItem("leftTop", static_cast<int>(Placement::LeftTop));
  placementBox->addItem("leftBottom", static_cast<int>(Placement::LeftBottom));
  placementBox->addItem("right", static_cast<int>(Placement::Right));
  placementBox->addItem("rightTop", static_cast<int>(Placement::RightTop));
  placementBox->addItem("rightBottom", static_cast<int>(Placement::RightBottom));
  placementBox->setCurrentIndex(0);

  auto* popover = makePopover("Open popover", "Placement", "Use combo box to change placement.", Trigger::Click);
  popover->setAutoAdjustOverflow(true);
  popover->setOpen(true);

  connect(placementBox, QOverload<int>::of(&QComboBox::currentIndexChanged), popover,
          [placementBox, popover](int) {
            popover->setPlacement(static_cast<Placement>(placementBox->currentData().toInt()));
          });

  layout->addWidget(placementBox, 0, Qt::AlignLeft);
  layout->addWidget(popover, 0, Qt::AlignLeft);
  layout->addWidget(makeHintLabel("When not enough space, Popover falls back to opposite placement."));
  return box;
}

QWidget* PopoverDocsPage::buildArrowDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* showArrow = new QCheckBox("Show Arrow");
  showArrow->setChecked(true);
  auto* pointAtCenter = new QCheckBox("Point At Center");

  auto* row = new QHBoxLayout();
  row->addWidget(showArrow);
  row->addWidget(pointAtCenter);
  row->addStretch();

  auto* popover = makePopover("Arrow demo", "Arrow", "Toggle arrow visibility and center alignment.", Trigger::Click);
  popover->setOpen(true);

  connect(showArrow, &QCheckBox::toggled, popover, &AdPopover::setArrowVisible);
  connect(pointAtCenter, &QCheckBox::toggled, popover, &AdPopover::setArrowPointAtCenter);

  layout->addLayout(row);
  layout->addWidget(popover, 0, Qt::AlignLeft);
  return box;
}

QWidget* PopoverDocsPage::buildAutoShiftDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* frame = new QFrame();
  frame->setFrameShape(QFrame::StyledPanel);
  frame->setFixedSize(320, 140);
  auto* frameLayout = new QHBoxLayout(frame);
  frameLayout->setContentsMargins(8, 8, 8, 8);

  auto* popover = makePopover("Near top-left", "Auto Adjust", "Try resizing window: popover keeps visible area.", Trigger::Click, frame);
  popover->setPlacement(Placement::TopLeft);
  popover->setOpen(true);
  popover->setAutoAdjustOverflow(true);
  frameLayout->addWidget(popover, 0, Qt::AlignTop | Qt::AlignLeft);
  frameLayout->addStretch();

  layout->addWidget(frame);
  layout->addWidget(makeHintLabel("This mirrors antd auto-shift behavior on narrow/edge viewport."));
  return box;
}

QWidget* PopoverDocsPage::buildControlledDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* openCheck = new QCheckBox("Open");
  auto* popover = makePopover("Controlled popover", "Controlled", "Open state is fully controlled by checkbox.", Trigger::Click);

  connect(openCheck, &QCheckBox::toggled, popover, &AdPopover::setOpen);
  connect(popover, &AdPopover::openChanged, openCheck, &QCheckBox::setChecked);

  layout->addWidget(openCheck, 0, Qt::AlignLeft);
  layout->addWidget(popover, 0, Qt::AlignLeft);
  return box;
}

QWidget* PopoverDocsPage::buildHoverWithClickDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(10);

  auto* outer = makePopover("Hover area", "Hover title", "Hover content", Trigger::Hover);
  auto* clickInsideHost = new QWidget();
  auto* clickInsideLayout = new QHBoxLayout(clickInsideHost);
  clickInsideLayout->setContentsMargins(0, 0, 0, 0);

  auto* inner = makePopover("Click inside", "Click title", "Nested click popover content", Trigger::Click, clickInsideHost);
  clickInsideLayout->addWidget(inner);
  clickInsideLayout->addStretch();
  outer->setContentWidget(clickInsideHost);

  row->addWidget(outer);
  row->addWidget(makeHintLabel("Outer uses hover, inner uses click. Both can keep popup interactions alive."));
  row->addStretch();
  return box;
}

QWidget* PopoverDocsPage::buildSemanticDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(10);

  auto* fixed = makePopover("Fixed semantic", "Semantic", "Semantic slot colors from fixed object.", Trigger::Click);
  AdPopover::SemanticStyles fixedStyles;
  fixedStyles.container.backgroundColor = QColor("#f6ffed");
  fixedStyles.container.borderColor = QColor("#b7eb8f");
  fixedStyles.title.textColor = QColor("#389e0d");
  fixedStyles.content.textColor = QColor("#237804");
  fixed->setSemanticStyles(fixedStyles);

  auto* dynamic = makePopover("Resolver semantic", "Resolver", "Semantic style resolver based on open state.", Trigger::Click);
  dynamic->setSemanticStyleResolver([](const AdPopover::StyleContext& ctx) {
    AdPopover::SemanticStyles styles;
    if (ctx.open) {
      styles.container.backgroundColor = QColor("#fff7e6");
      styles.container.borderColor = QColor("#ffd591");
      styles.title.textColor = QColor("#d46b08");
      styles.content.textColor = QColor("#ad4e00");
    } else {
      styles.container.backgroundColor = QColor("#e6f4ff");
      styles.container.borderColor = QColor("#91caff");
      styles.title.textColor = QColor("#0958d9");
      styles.content.textColor = QColor("#003eb3");
    }
    return styles;
  });

  row->addWidget(fixed);
  row->addWidget(dynamic);
  row->addStretch();
  return box;
}

QWidget* PopoverDocsPage::buildComponentTokenDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(8);

  auto* custom = makePopover("Token custom", "Token", "Custom title width, padding, radius and colors.", Trigger::Click);
  AdPopover::ComponentTokens tokens;
  tokens.titleMinWidth = 40;
  tokens.popupPadding = 10;
  tokens.popupOffset = 6;
  tokens.borderRadius = 12;
  tokens.arrowSize = 10;
  tokens.popupBg = QStringLiteral("#fffbe6");
  tokens.popupBorderColor = QStringLiteral("#ffe58f");
  tokens.titleColor = QStringLiteral("#d48806");
  tokens.contentColor = QStringLiteral("#ad6800");
  custom->setComponentTokens(tokens);

  auto* defaultToken = makePopover("Default token", "Default", "Theme token based default style.", Trigger::Click);

  row->addWidget(custom);
  row->addWidget(defaultToken);
  row->addStretch();
  return box;
}

QWidget* PopoverDocsPage::buildApiOverview() {
  auto* box = new QWidget();
  auto* grid = new QGridLayout(box);
  grid->setContentsMargins(0, 0, 0, 0);
  grid->setHorizontalSpacing(16);
  grid->setVerticalSpacing(8);

  const QVector<QPair<QString, QString>> rows = {
      {"placement", "top/topLeft/topRight/bottom/bottomLeft/bottomRight/left/leftTop/leftBottom/right/rightTop/rightBottom"},
      {"triggerModes", "Hover | Focus | Click | ContextMenu (flags, combinable)"},
      {"open / defaultOpen", "controlled or initial uncontrolled visible state"},
      {"autoAdjustOverflow", "enable opposite placement fallback and edge clamping"},
      {"arrowVisible / arrowPointAtCenter", "arrow visibility and center alignment behavior"},
      {"mouseEnterDelayMs / mouseLeaveDelayMs", "hover open/close delay (ms)"},
      {"destroyOnHidden", "destroy popup subtree when closed"},
      {"titleText/contentText", "basic text content"},
      {"setTitleWidget/setContentWidget", "custom widget content slots"},
      {"componentTokens", "metrics + colors overrides"},
      {"semanticStyles / resolver", "semantic slot style customizations"},
      {"signals", "openChanged(bool), onOpenChange(bool)"},
  };

  for (int i = 0; i < rows.size(); ++i) {
    auto* name = new QLabel(rows.at(i).first);
    auto* desc = new QLabel(rows.at(i).second);
    name->setTextInteractionFlags(Qt::TextSelectableByMouse);
    desc->setTextInteractionFlags(Qt::TextSelectableByMouse);
    desc->setWordWrap(true);
    QFont nameFont = name->font();
    nameFont.setBold(true);
    name->setFont(nameFont);
    grid->addWidget(name, i, 0, Qt::AlignTop);
    grid->addWidget(desc, i, 1);
  }

  return box;
}

