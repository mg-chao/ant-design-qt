#include "popconfirm_docs_page.h"

#include <QCheckBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>

#include "icons.h"

using adqt::widgets::AdButton;
using adqt::widgets::AdPopconfirm;
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

}  // namespace

PopconfirmDocsPage::PopconfirmDocsPage(QWidget* parent) : QWidget(parent) {
  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(16, 16, 16, 24);
  root->setSpacing(16);

  auto* title = new QLabel("Popconfirm");
  QFont titleFont = title->font();
  titleFont.setPointSize(titleFont.pointSize() + 8);
  titleFont.setBold(true);
  title->setFont(titleFont);
  root->addWidget(title);

  auto* subtitle = new QLabel(
      "A compact confirmation popup for actions. This page ports the official antd Popconfirm examples.");
  subtitle->setWordWrap(true);
  root->addWidget(subtitle);

  addSection(root, "Basic", "Demo: basic.tsx", buildBasicDemo());
  addSection(root, "Locale text", "Demo: locale.tsx", buildLocaleDemo());
  addSection(root, "Placement", "Demo: placement.tsx", buildPlacementDemo());
  addSection(root, "Auto Shift", "Demo: shift.tsx", buildAutoShiftDemo());
  addSection(root, "Conditional trigger", "Demo: dynamic-trigger.tsx", buildDynamicTriggerDemo());
  addSection(root, "Customize icon", "Demo: icon.tsx", buildIconDemo());
  addSection(root, "Asynchronously close", "Demo: async.tsx", buildAsyncDemo());
  addSection(root, "Asynchronously close on Promise", "Demo: promise.tsx", buildPromiseDemo());
  addSection(root, "Custom semantic dom styling", "Demo: style-class.tsx", buildStyleClassDemo());
  addSection(root, "_InternalPanelDoNotUseOrYouWillBeFired", "Demo: render-panel.tsx", buildRenderPanelDemo());
  addSection(root, "Wireframe", "Demo: wireframe.tsx", buildWireframeDemo());

  root->addStretch();
}

const QVector<QWidget*>& PopconfirmDocsPage::sectionAnchors() const { return anchors_; }

const QStringList& PopconfirmDocsPage::sectionTitles() const { return titles_; }

void PopconfirmDocsPage::addSection(QVBoxLayout* root,
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

AdPopconfirm* PopconfirmDocsPage::makePopconfirm(const QString& triggerText,
                                                 const QString& title,
                                                 const QString& description,
                                                 Triggers triggers,
                                                 QWidget* parent) {
  auto* popconfirm = new AdPopconfirm(parent);
  auto* trigger = new AdButton(triggerText, popconfirm);
  trigger->setType(AdButton::Type::Default);
  popconfirm->setTriggerWidget(trigger);
  popconfirm->setTitleText(title);
  popconfirm->setDescriptionText(description);
  popconfirm->setTriggerModes(triggers);
  return popconfirm;
}

QWidget* PopconfirmDocsPage::buildBasicDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* status = new QLabel("Result: waiting for action.");
  auto* popconfirm = makePopconfirm("Delete", "Delete the task", "Are you sure to delete this task?");
  popconfirm->setOkText("Yes");
  popconfirm->setCancelText("No");
  if (auto* trigger = qobject_cast<AdButton*>(popconfirm->triggerWidget())) {
    trigger->setDanger(true);
  }

  connect(popconfirm, &AdPopconfirm::confirmed, status,
          [status]() { status->setText("Result: Click on Yes."); });
  connect(popconfirm, &AdPopconfirm::canceled, status,
          [status]() { status->setText("Result: Click on No."); });

  auto* row = new QHBoxLayout();
  row->setContentsMargins(0, 0, 0, 0);
  row->addWidget(popconfirm);
  row->addWidget(status);
  row->addStretch();

  layout->addLayout(row);
  return box;
}

QWidget* PopconfirmDocsPage::buildLocaleDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(10);

  auto* locale = makePopconfirm("Delete", "Delete the task", "Are you sure to delete this task?");
  locale->setOkText("Yes");
  locale->setCancelText("No");
  if (auto* trigger = qobject_cast<AdButton*>(locale->triggerWidget())) {
    trigger->setDanger(true);
  }

  row->addWidget(locale);
  row->addWidget(makeHintLabel("Use `okText` and `cancelText` to customize locale text."));
  row->addStretch();
  return box;
}

QWidget* PopconfirmDocsPage::buildPlacementDemo() {
  auto* box = new QWidget();
  auto* grid = new QGridLayout(box);
  grid->setContentsMargins(0, 0, 0, 0);
  grid->setHorizontalSpacing(8);
  grid->setVerticalSpacing(8);
  const QString title = "Are you sure to delete this task?";
  const QString description = "Delete the task";
  constexpr int buttonWidth = 80;

  auto addPlacement = [&](int row, int column, const QString& text, Placement placement) {
    auto* popconfirm = makePopconfirm(text, title, description);
    popconfirm->setPlacement(placement);
    popconfirm->setOkText("Yes");
    popconfirm->setCancelText("No");
    if (auto* trigger = qobject_cast<AdButton*>(popconfirm->triggerWidget())) {
      trigger->setFixedWidth(buttonWidth);
    }
    if (auto* cancel = popconfirm->cancelButton()) {
      cancel->setFixedWidth(buttonWidth);
    }
    if (auto* ok = popconfirm->okButton()) {
      ok->setFixedWidth(buttonWidth);
    }
    grid->addWidget(popconfirm, row, column);
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

QWidget* PopconfirmDocsPage::buildAutoShiftDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* frame = new QFrame();
  frame->setFrameShape(QFrame::StyledPanel);
  frame->setFixedSize(360, 160);
  auto* frameLayout = new QHBoxLayout(frame);
  frameLayout->setContentsMargins(8, 8, 8, 8);
  frameLayout->setSpacing(0);

  auto* popconfirm = makePopconfirm("Scroll The Window",
                                    "Thanks for using antd. Have a nice day !",
                                    "Popconfirm auto shifts near viewport edges.",
                                    Trigger::Click,
                                    frame);
  if (auto* trigger = qobject_cast<AdButton*>(popconfirm->triggerWidget())) {
    trigger->setType(AdButton::Type::Primary);
  }
  popconfirm->setPlacement(Placement::Top);
  popconfirm->setAutoAdjustOverflow(true);
  popconfirm->setOpen(true);

  frameLayout->addWidget(popconfirm, 0, Qt::AlignTop | Qt::AlignLeft);
  frameLayout->addStretch();

  layout->addWidget(frame);
  layout->addWidget(makeHintLabel("Near the edge, popup position and arrow auto-adjust to keep visible area."));
  return box;
}

QWidget* PopconfirmDocsPage::buildDynamicTriggerDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(10);

  auto* conditionCheck = new QCheckBox("Whether directly execute");
  conditionCheck->setChecked(true);
  auto* status = new QLabel("Result: waiting for action.");

  auto* popconfirm =
      makePopconfirm("Delete a task", "Delete the task", "Are you sure to delete this task?");
  if (auto* trigger = qobject_cast<AdButton*>(popconfirm->triggerWidget())) {
    trigger->setDanger(true);
  }
  popconfirm->setOpenControlled(true);
  popconfirm->setOkText("Yes");
  popconfirm->setCancelText("No");

  connect(popconfirm, &AdPopconfirm::onOpenChange, this,
          [popconfirm, conditionCheck, status](bool nextOpen) {
            if (!nextOpen) {
              popconfirm->setOpen(false);
              return;
            }
            if (conditionCheck->isChecked()) {
              status->setText("Result: Next step.");
              popconfirm->setOpen(false);
              return;
            }
            popconfirm->setOpen(true);
          });

  connect(popconfirm, &AdPopconfirm::confirmed, status,
          [status]() { status->setText("Result: Next step."); });
  connect(popconfirm, &AdPopconfirm::canceled, status,
          [status]() { status->setText("Result: Click on cancel."); });

  auto* row = new QHBoxLayout();
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(10);
  row->addWidget(popconfirm);
  row->addWidget(conditionCheck);
  row->addStretch();

  layout->addLayout(row);
  layout->addWidget(status);
  return box;
}

QWidget* PopconfirmDocsPage::buildIconDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(10);

  auto* defaultIcon =
      makePopconfirm("Default icon", "Delete the task", "Are you sure to delete this task?");

  auto* customIcon =
      makePopconfirm("Custom icon", "Delete the task", "Are you sure to delete this task?");
  adqt::icons::IconStyle iconStyle;
  iconStyle.primary = QColor("#ff4d4f");
  iconStyle.hasPrimary = true;
  customIcon->setIconToken(outlined_icons::QuestionCircle(iconStyle));

  row->addWidget(defaultIcon);
  row->addWidget(customIcon);
  row->addStretch();
  return box;
}

QWidget* PopconfirmDocsPage::buildAsyncDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* status = new QLabel("Result: waiting for action.");
  auto* popconfirm =
      makePopconfirm("Open Popconfirm with async logic",
                     "Title",
                     "Open Popconfirm with async logic");
  if (auto* trigger = qobject_cast<AdButton*>(popconfirm->triggerWidget())) {
    trigger->setType(AdButton::Type::Primary);
  }
  popconfirm->setOpenControlled(true);
  popconfirm->setConfirmAutoClose(false);

  connect(popconfirm, &AdPopconfirm::onOpenChange, this,
          [popconfirm](bool nextOpen) { popconfirm->setOpen(nextOpen); });
  connect(popconfirm, &AdPopconfirm::confirmed, this, [popconfirm, status]() {
    status->setText("Result: confirming...");
    popconfirm->setOkButtonLoading(true);
    QTimer::singleShot(2000, popconfirm, [popconfirm, status]() {
      popconfirm->setOkButtonLoading(false);
      popconfirm->setOpen(false);
      status->setText("Result: confirmed after async logic.");
    });
  });
  connect(popconfirm, &AdPopconfirm::canceled, this, [popconfirm, status]() {
    popconfirm->setOkButtonLoading(false);
    popconfirm->setOpen(false);
    status->setText("Result: canceled.");
  });

  layout->addWidget(popconfirm, 0, Qt::AlignLeft);
  layout->addWidget(status);
  return box;
}

QWidget* PopconfirmDocsPage::buildPromiseDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* status = new QLabel("Result: open change log.");
  auto* popconfirm = makePopconfirm("Open Popconfirm with Promise",
                                    "Title",
                                    "Open Popconfirm with Promise");
  if (auto* trigger = qobject_cast<AdButton*>(popconfirm->triggerWidget())) {
    trigger->setType(AdButton::Type::Primary);
  }
  popconfirm->setConfirmAutoClose(false);

  connect(popconfirm, &AdPopconfirm::onOpenChange, status, [status](bool nextOpen) {
    status->setText(QStringLiteral("Result: open request -> %1").arg(nextOpen ? "true" : "false"));
  });
  connect(popconfirm, &AdPopconfirm::confirmed, this, [popconfirm, status]() {
    status->setText("Result: promise pending...");
    popconfirm->setOkButtonLoading(true);
    QTimer::singleShot(3000, popconfirm, [popconfirm, status]() {
      popconfirm->setOkButtonLoading(false);
      popconfirm->setOpen(false);
      status->setText("Result: promise resolved, popup closed.");
    });
  });

  layout->addWidget(popconfirm, 0, Qt::AlignLeft);
  layout->addWidget(status);
  return box;
}

QWidget* PopconfirmDocsPage::buildStyleClassDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(10);

  auto* objectStyle = makePopconfirm("Object Style", "Object text", "Object description");
  objectStyle->setArrowVisible(false);
  AdPopconfirm::SemanticStyles objectSemantic;
  objectSemantic.container.backgroundColor = QColor(53, 71, 125, 204);
  objectSemantic.title.textColor = QColor("#ffffff");
  objectSemantic.description.textColor = QColor("#ffffff");
  objectSemantic.icon.textColor = QColor("#ffffff");
  objectStyle->setSemanticStyles(objectSemantic);

  auto* functionStyle = makePopconfirm("Function Style", "Function text", "Function description");
  functionStyle->setArrowVisible(false);
  functionStyle->setSemanticStyleResolver([](const AdPopconfirm::StyleContext& ctx) {
    AdPopconfirm::SemanticStyles styles;
    if (ctx.open) {
      styles.container.backgroundColor = QColor("#fffbe6");
      styles.title.textColor = QColor("#ad6800");
      styles.description.textColor = QColor("#ad6800");
      styles.icon.textColor = QColor("#d48806");
    } else {
      styles.container.backgroundColor = QColor(53, 71, 125, 204);
      styles.title.textColor = QColor("#ffffff");
      styles.description.textColor = QColor("#ffffff");
      styles.icon.textColor = QColor("#ffffff");
    }
    return styles;
  });

  row->addWidget(objectStyle);
  row->addWidget(functionStyle);
  row->addStretch();
  return box;
}

QWidget* PopconfirmDocsPage::buildRenderPanelDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* row = new QHBoxLayout();
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(10);

  auto* panelA = makePopconfirm("Panel A", "Are you OK?", "Does this look good?");
  panelA->setOpen(true);

  auto* panelB = makePopconfirm("Panel B", "Are you OK?", "Does this look good?");
  panelB->setPlacement(Placement::BottomRight);
  panelB->setOpen(true);

  auto* panelC = makePopconfirm("Panel C", "Are you OK?", QString());
  panelC->setIconVisible(false);
  panelC->setOpen(true);

  auto* panelD = makePopconfirm("Panel D", "Are you OK?", "Does this look good?");
  panelD->setIconVisible(false);
  panelD->setOpen(true);

  row->addWidget(panelA);
  row->addWidget(panelB);
  row->addWidget(panelC);
  row->addWidget(panelD);
  row->addStretch();

  layout->addLayout(row);
  layout->addWidget(makeHintLabel("Equivalent of internal panel showcase using always-open popconfirm instances."));
  return box;
}

QWidget* PopconfirmDocsPage::buildWireframeDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(10);

  AdPopconfirm::SemanticStyles wireframeStyles;
  wireframeStyles.container.backgroundColor = QColor("#ffffff");
  wireframeStyles.container.borderColor = QColor("#d9d9d9");
  wireframeStyles.title.textColor = QColor("#262626");
  wireframeStyles.description.textColor = QColor("#595959");
  wireframeStyles.arrow.backgroundColor = QColor("#ffffff");

  auto* panelA = makePopconfirm("Wireframe A", "Are you OK?", "Wireframe-like semantic style.");
  panelA->setSemanticStyles(wireframeStyles);
  panelA->setOpen(true);

  auto* panelB = makePopconfirm("Wireframe B", "Are you OK?", "Bottom-right wireframe style.");
  panelB->setSemanticStyles(wireframeStyles);
  panelB->setPlacement(Placement::BottomRight);
  panelB->setOpen(true);

  row->addWidget(panelA);
  row->addWidget(panelB);
  row->addStretch();
  return box;
}
