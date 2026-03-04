#include "tooltip_docs_page.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

using adqt::widgets::AdButton;
using adqt::widgets::AdTooltip;

namespace {

QLabel* makeHintLabel(const QString& text, QWidget* parent = nullptr) {
  auto* label = new QLabel(text, parent);
  label->setWordWrap(true);
  QPalette palette = label->palette();
  palette.setColor(QPalette::WindowText, QColor("#8c8c8c"));
  label->setPalette(palette);
  return label;
}

QWidget* wrapDisabledWidget(QWidget* child) {
  auto* host = new QWidget();
  host->setAttribute(Qt::WA_Hover, true);
  host->setMouseTracking(true);
  auto* layout = new QVBoxLayout(host);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addWidget(child);
  return host;
}

class EventAwareWidget final : public QWidget {
 public:
  explicit EventAwareWidget(QWidget* parent = nullptr) : QWidget(parent) {
    setAttribute(Qt::WA_Hover, true);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 6, 10, 6);
    layout->setSpacing(0);
    auto* label = new QLabel("This text is inside a custom event-aware component.");
    label->setWordWrap(true);
    layout->addWidget(label);
  }
};

}  // namespace

TooltipDocsPage::TooltipDocsPage(QWidget* parent) : QWidget(parent) {
  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(16, 16, 16, 24);
  root->setSpacing(16);

  auto* title = new QLabel("Tooltip");
  QFont titleFont = title->font();
  titleFont.setPointSize(titleFont.pointSize() + 8);
  titleFont.setBold(true);
  title->setFont(titleFont);
  root->addWidget(title);

  auto* subtitle = new QLabel(
      "Simple text popup box. The tip appears on hover/focus/click/context menu and is typically used "
      "to explain an element.");
  subtitle->setWordWrap(true);
  root->addWidget(subtitle);

  addSection(root, "Basic", "Demo: basic.tsx", buildBasicDemo());
  addSection(root, "Smooth Transition", "Demo: smooth-transition.tsx (behavior only, no animation)",
             buildSmoothTransitionDemo());
  addSection(root, "Placement", "Demo: placement.tsx", buildPlacementDemo());
  addSection(root, "Arrow", "Demo: arrow.tsx + arrow-point-at-center.tsx", buildArrowDemo());
  addSection(root, "Auto Shift", "Demo: shift.tsx", buildShiftDemo());
  addSection(root, "Adjust placement automatically", "Demo: auto-adjust-overflow.tsx",
             buildAutoAdjustOverflowDemo());
  addSection(root, "Destroy tooltip when hidden", "Demo: destroy-on-close.tsx", buildDestroyOnCloseDemo());
  addSection(root, "Colorful Tooltip", "Demo: colorful.tsx", buildColorfulDemo());
  addSection(root, "Disabled", "Demo: disabled.tsx", buildDisabledDemo());
  addSection(root, "Disabled children", "Demo: disabled-children.tsx", buildDisabledChildrenDemo());
  addSection(root, "Wrap custom component", "Demo: wrap-custom-component.tsx", buildWrapCustomComponentDemo());
  addSection(root, "Custom semantic dom styling", "Demo: style-class.tsx", buildStyleClassDemo());

  root->addStretch();
}

const QVector<QWidget*>& TooltipDocsPage::sectionAnchors() const { return anchors_; }

const QStringList& TooltipDocsPage::sectionTitles() const { return titles_; }

void TooltipDocsPage::addSection(QVBoxLayout* root,
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

AdTooltip* TooltipDocsPage::makeTooltip(const QString& triggerText,
                                        const QString& title,
                                        Triggers triggers,
                                        QWidget* parent) {
  auto* tooltip = new AdTooltip(parent);
  auto* trigger = new AdButton(triggerText, tooltip);
  trigger->setType(AdButton::Type::Default);
  tooltip->setTriggerWidget(trigger);
  tooltip->setTitleText(title);
  tooltip->setTriggerModes(triggers);
  return tooltip;
}

QWidget* TooltipDocsPage::buildBasicDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(8);

  row->addWidget(makeTooltip("Hover me", "prompt text", Trigger::Hover));
  row->addWidget(new QLabel("Tooltip will show on mouse enter."));
  row->addStretch();
  return box;
}

QWidget* TooltipDocsPage::buildSmoothTransitionDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* row1 = new QHBoxLayout();
  row1->setSpacing(8);
  auto* row2 = new QHBoxLayout();
  row2->setSpacing(8);

  auto* t1 = makeTooltip("Button 1", "First tooltip");
  auto* t2 = makeTooltip("Button 2", "Second tooltip");
  auto* t3 = makeTooltip("Button 3", "Third tooltip");
  auto* t4 = makeTooltip("Button 4", "Fourth tooltip");

  row1->addWidget(t1);
  row1->addWidget(t2);
  row1->addStretch();
  row2->addWidget(t3);
  row2->addWidget(t4);
  row2->addStretch();

  layout->addLayout(row1);
  layout->addLayout(row2);
  layout->addWidget(
      makeHintLabel("No animation in this Qt demo. Behavior matches unique display: only one tooltip is visible in the same window scope."));
  return box;
}

QWidget* TooltipDocsPage::buildPlacementDemo() {
  auto* box = new QWidget();
  auto* grid = new QGridLayout(box);
  grid->setContentsMargins(0, 0, 0, 0);
  grid->setHorizontalSpacing(8);
  grid->setVerticalSpacing(8);

  auto addPlacement = [&](int row, int column, const QString& text, Placement placement) {
    auto* tooltip = makeTooltip(text, "prompt text");
    tooltip->setPlacement(placement);
    if (auto* trigger = qobject_cast<AdButton*>(tooltip->triggerWidget())) {
      trigger->setFixedWidth(78);
    }
    grid->addWidget(tooltip, row, column);
  };

  addPlacement(0, 1, "TL", Placement::TopLeft);
  addPlacement(0, 2, "Top", Placement::Top);
  addPlacement(0, 3, "TR", Placement::TopRight);

  addPlacement(1, 0, "LT", Placement::LeftTop);
  addPlacement(2, 0, "Left", Placement::Left);
  addPlacement(3, 0, "LB", Placement::LeftBottom);

  addPlacement(1, 4, "RT", Placement::RightTop);
  addPlacement(2, 4, "Right", Placement::Right);
  addPlacement(3, 4, "RB", Placement::RightBottom);

  addPlacement(4, 1, "BL", Placement::BottomLeft);
  addPlacement(4, 2, "Bottom", Placement::Bottom);
  addPlacement(4, 3, "BR", Placement::BottomRight);

  return box;
}

QWidget* TooltipDocsPage::buildArrowDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* modeBox = new QComboBox();
  modeBox->addItem("Show");
  modeBox->addItem("Hide");
  modeBox->addItem("Center");

  auto* row = new QHBoxLayout();
  row->setSpacing(8);
  auto* left = makeTooltip("TL", "prompt text");
  left->setPlacement(Placement::TopLeft);
  auto* middle = makeTooltip("Top", "prompt text");
  middle->setPlacement(Placement::Top);
  auto* right = makeTooltip("TR", "prompt text");
  right->setPlacement(Placement::TopRight);
  row->addWidget(left);
  row->addWidget(middle);
  row->addWidget(right);
  row->addStretch();

  const QList<AdTooltip*> samples = {left, middle, right};
  auto applyMode = [samples](const QString& mode) {
    const bool visible = mode != QStringLiteral("Hide");
    const bool center = mode == QStringLiteral("Center");
    for (AdTooltip* tooltip : samples) {
      if (!tooltip) {
        continue;
      }
      tooltip->setArrowVisible(visible);
      tooltip->setArrowPointAtCenter(center);
    }
  };

  connect(modeBox, &QComboBox::currentTextChanged, this, applyMode);
  applyMode(modeBox->currentText());

  layout->addWidget(modeBox, 0, Qt::AlignLeft);
  layout->addLayout(row);
  return box;
}

QWidget* TooltipDocsPage::buildShiftDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* frame = new QFrame();
  frame->setFrameShape(QFrame::StyledPanel);
  frame->setFixedSize(320, 140);
  auto* frameLayout = new QHBoxLayout(frame);
  frameLayout->setContentsMargins(8, 8, 8, 8);
  frameLayout->setSpacing(0);

  auto* tooltip = makeTooltip("Near top-left", "Thanks for using antd. Have a nice day !", Trigger::Click, frame);
  tooltip->setPlacement(Placement::Top);
  tooltip->setOpen(true);
  tooltip->setAutoAdjustOverflow(true);

  frameLayout->addWidget(tooltip, 0, Qt::AlignLeft | Qt::AlignTop);
  frameLayout->addStretch();

  layout->addWidget(frame);
  layout->addWidget(makeHintLabel("When close to viewport edge, popup and arrow will auto shift for top/bottom/left/right placements."));
  return box;
}

QWidget* TooltipDocsPage::buildAutoAdjustOverflowDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* autoAdjust = new QCheckBox("autoAdjustOverflow");
  autoAdjust->setChecked(true);

  auto* tooltip = makeTooltip("Placement: left", "Prompt Text", Trigger::Click);
  tooltip->setPlacement(Placement::Left);
  tooltip->setOpen(true);
  tooltip->setAutoAdjustOverflow(true);

  connect(autoAdjust, &QCheckBox::toggled, tooltip, &AdTooltip::setAutoAdjustOverflow);

  layout->addWidget(autoAdjust, 0, Qt::AlignLeft);
  layout->addWidget(tooltip, 0, Qt::AlignLeft);
  layout->addWidget(makeHintLabel("Toggle this option to compare automatic flip/clamp behavior with overflow-allowed behavior."));
  return box;
}

QWidget* TooltipDocsPage::buildDestroyOnCloseDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* openCheck = new QCheckBox("Open");
  auto* refreshButton = new QPushButton("Refresh popup count");
  auto* status = new QLabel();

  auto* tooltip = makeTooltip("Click trigger", "Dom will destroyed when Tooltip close", Trigger::Click);
  tooltip->setDestroyOnHidden(true);

  auto updateStatus = [tooltip, status]() {
    if (!tooltip || !status) {
      return;
    }
    QWidget* scope = tooltip->window();
    const int popupCount =
        scope ? scope->findChildren<QWidget*>(QStringLiteral("adpopover-popup"), Qt::FindChildrenRecursively).size()
              : 0;
    status->setText(QStringLiteral("Popup widgets in scope: %1").arg(popupCount));
  };

  connect(openCheck, &QCheckBox::toggled, tooltip, &AdTooltip::setOpen);
  connect(tooltip, &AdTooltip::openChanged, openCheck, &QCheckBox::setChecked);
  connect(tooltip, &AdTooltip::openChanged, this, [updateStatus](bool) { updateStatus(); });
  connect(refreshButton, &QPushButton::clicked, this, [updateStatus]() { updateStatus(); });

  updateStatus();
  layout->addWidget(openCheck, 0, Qt::AlignLeft);
  layout->addWidget(tooltip, 0, Qt::AlignLeft);
  layout->addWidget(refreshButton, 0, Qt::AlignLeft);
  layout->addWidget(status);
  return box;
}

QWidget* TooltipDocsPage::buildColorfulDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  const QStringList presetColors = {
      "pink", "red",   "yellow", "orange",  "cyan", "green", "blue",
      "purple", "geekblue", "magenta", "volcano", "gold", "lime",
  };
  const QStringList customColors = {"#f50", "#2db7f5", "#87d068", "#108ee9"};

  auto* presets = new QGridLayout();
  presets->setHorizontalSpacing(8);
  presets->setVerticalSpacing(8);
  for (int i = 0; i < presetColors.size(); ++i) {
    auto* tooltip = makeTooltip(presetColors.at(i), "prompt text");
    tooltip->setColor(presetColors.at(i));
    presets->addWidget(tooltip, i / 5, i % 5);
  }

  auto* customs = new QHBoxLayout();
  customs->setSpacing(8);
  for (const QString& color : customColors) {
    auto* tooltip = makeTooltip(color, "prompt text");
    tooltip->setColor(color);
    customs->addWidget(tooltip);
  }
  customs->addStretch();

  layout->addWidget(new QLabel("Presets"));
  layout->addLayout(presets);
  layout->addWidget(new QLabel("Custom"));
  layout->addLayout(customs);
  return box;
}

QWidget* TooltipDocsPage::buildDisabledDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(8);

  auto* tooltip = new AdTooltip();
  auto* trigger = new AdButton("Enable", tooltip);
  tooltip->setTriggerWidget(trigger);
  tooltip->setTitleText(QString());

  tooltip->setProperty("tooltipDisabledState", true);
  auto updateState = [tooltip, trigger]() {
    const bool disabled = tooltip->property("tooltipDisabledState").toBool();
    if (disabled) {
      tooltip->setTitleText(QString());
      trigger->setText("Enable");
    } else {
      tooltip->setTitleText("prompt text");
      trigger->setText("Disable");
    }
  };

  connect(trigger, &QPushButton::clicked, this, [tooltip, updateState]() {
    const bool disabled = tooltip->property("tooltipDisabledState").toBool();
    tooltip->setProperty("tooltipDisabledState", !disabled);
    updateState();
  });

  updateState();
  row->addWidget(tooltip);
  row->addStretch();
  return box;
}

QWidget* TooltipDocsPage::buildDisabledChildrenDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(10);

  auto addWrappedTooltip = [row](QWidget* disabledWidget) {
    auto* tooltip = new AdTooltip();
    tooltip->setTitleText("Thanks for using antd. Have a nice day !");
    tooltip->setTriggerWidget(wrapDisabledWidget(disabledWidget));
    row->addWidget(tooltip);
  };

  auto* disabledButton = new AdButton("Disabled");
  disabledButton->setEnabled(false);
  addWrappedTooltip(disabledButton);

  auto* disabledInput = new QLineEdit();
  disabledInput->setPlaceholderText("disabled");
  disabledInput->setEnabled(false);
  addWrappedTooltip(disabledInput);

  auto* disabledCheck = new QCheckBox("Checkbox");
  disabledCheck->setEnabled(false);
  addWrappedTooltip(disabledCheck);

  auto* disabledCombo = new QComboBox();
  disabledCombo->addItems({"One", "Two", "Three"});
  disabledCombo->setEnabled(false);
  addWrappedTooltip(disabledCombo);

  row->addStretch();
  return box;
}

QWidget* TooltipDocsPage::buildWrapCustomComponentDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(8);

  auto* tooltip = new AdTooltip();
  tooltip->setTitleText("prompt text");
  tooltip->setTriggerWidget(new EventAwareWidget(tooltip));

  row->addWidget(tooltip);
  row->addStretch();
  return box;
}

QWidget* TooltipDocsPage::buildStyleClassDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(10);

  auto* objectStyled = makeTooltip("Object Style", "Object text", Trigger::Click);
  objectStyled->setArrowVisible(false);
  AdTooltip::SemanticStyles objectStyles;
  objectStyles.container.backgroundColor = QColor(53, 71, 125, 204);
  objectStyles.body.textColor = QColor("#ffffff");
  objectStyled->setSemanticStyles(objectStyles);

  auto* resolverStyled = makeTooltip("Function Style", "Function text", Trigger::Click);
  resolverStyled->setArrowVisible(false);
  resolverStyled->setSemanticStyleResolver([](const AdTooltip::StyleContext& ctx) {
    AdTooltip::SemanticStyles styles;
    if (ctx.open) {
      styles.container.backgroundColor = QColor("#fffbe6");
      styles.body.textColor = QColor("#ad6800");
    } else {
      styles.container.backgroundColor = QColor(53, 71, 125, 204);
      styles.body.textColor = QColor("#ffffff");
    }
    return styles;
  });

  row->addWidget(objectStyled);
  row->addWidget(resolverStyled);
  row->addStretch();
  return box;
}
