#include "alert_docs_page.h"

#include <algorithm>
#include <cmath>
#include <QElapsedTimer>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QTimer>
#include <QVBoxLayout>

#include "icons.h"

using adqt::widgets::AdAlert;
using adqt::widgets::AdButton;
using adqt::widgets::AdSwitch;
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

class LoopingTextLabel final : public QWidget {
 public:
  explicit LoopingTextLabel(const QString& text, QWidget* parent = nullptr) : QWidget(parent), source_(text) {
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setAttribute(Qt::WA_Hover, true);
    timer_.setInterval(16);
    connect(&timer_, &QTimer::timeout, this, [this]() { tick(); });
    refreshMetrics();
    elapsed_.start();
    timer_.start();
  }

  QSize sizeHint() const override {
    const QFontMetrics metrics(font());
    return QSize(std::max(metrics.horizontalAdvance(source_), 1), metrics.height() + 2);
  }

  QSize minimumSizeHint() const override {
    const QFontMetrics metrics(font());
    return QSize(0, metrics.height() + 2);
  }

 private:
  bool event(QEvent* event) override {
    if (event) {
      if (event->type() == QEvent::Enter) {
        paused_ = true;
      } else if (event->type() == QEvent::Leave) {
        paused_ = false;
        elapsed_.restart();
      } else if (event->type() == QEvent::FontChange) {
        refreshMetrics();
      }
    }
    return QWidget::event(event);
  }

  void resizeEvent(QResizeEvent* event) override {
    QWidget::resizeEvent(event);
    refreshMetrics();
  }

  void paintEvent(QPaintEvent* event) override {
    Q_UNUSED(event);
    if (source_.isEmpty()) {
      return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.setPen(palette().color(QPalette::WindowText));

    const QFontMetrics metrics(font());
    const int baseline = (height() - metrics.height()) / 2 + metrics.ascent();
    const qreal cycle = static_cast<qreal>(textWidth_ + gapPx_);
    if (cycle <= 0.0) {
      painter.drawText(0, baseline, source_);
      return;
    }

    qreal x = -offsetPx_;
    while (x < width()) {
      painter.drawText(QPointF(x, baseline), source_);
      x += cycle;
    }
  }

  void refreshMetrics() {
    const QFontMetrics metrics(font());
    textWidth_ = std::max(metrics.horizontalAdvance(source_), 1);
    gapPx_ = std::max(metrics.horizontalAdvance(QStringLiteral("    ")), 24);
    const qreal cycle = static_cast<qreal>(textWidth_ + gapPx_);
    if (cycle > 0.0) {
      offsetPx_ = std::fmod(offsetPx_, cycle);
      if (offsetPx_ < 0.0) {
        offsetPx_ += cycle;
      }
    } else {
      offsetPx_ = 0.0;
    }
    updateGeometry();
    update();
  }

  void tick() {
    if (source_.isEmpty() || paused_) {
      return;
    }

    const qint64 elapsedMs = elapsed_.restart();
    const qreal frameMs = elapsedMs > 0 ? static_cast<qreal>(elapsedMs) : 16.0;
    offsetPx_ += (speedPxPerSecond_ * frameMs) / 1000.0;

    const qreal cycle = static_cast<qreal>(textWidth_ + gapPx_);
    if (cycle > 0.0) {
      while (offsetPx_ >= cycle) {
        offsetPx_ -= cycle;
      }
    }
    update();
  }

  QString source_;
  QTimer timer_;
  QElapsedTimer elapsed_;
  qreal offsetPx_ = 0.0;
  int textWidth_ = 1;
  int gapPx_ = 24;
  qreal speedPxPerSecond_ = 50.0;
  bool paused_ = false;
};

}  // namespace

AlertDocsPage::AlertDocsPage(QWidget* parent) : QWidget(parent) {
  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(16, 16, 16, 24);
  root->setSpacing(16);

  auto* title = new QLabel("Alert");
  QFont titleFont = title->font();
  titleFont.setPointSize(titleFont.pointSize() + 8);
  titleFont.setBold(true);
  title->setFont(titleFont);
  root->addWidget(title);

  auto* subtitle = new QLabel(
      "Display warning messages that require attention. This page ports upstream antd Alert demos. "
      "React-only ErrorBoundary is intentionally omitted.");
  subtitle->setWordWrap(true);
  root->addWidget(subtitle);

  addSection(root, "Basic", "Demo: basic.tsx", buildBasicDemo());
  addSection(root, "More types", "Demo: style.tsx", buildStyleDemo());
  addSection(root, "Closable", "Demo: closable.tsx", buildClosableDemo());
  addSection(root, "Description", "Demo: description.tsx", buildDescriptionDemo());
  addSection(root, "Icon", "Demo: icon.tsx", buildIconDemo());
  addSection(root, "Banner", "Demo: banner.tsx", buildBannerDemo());
  addSection(root, "Loop Banner", "Demo: loop-banner.tsx", buildLoopBannerDemo());
  addSection(root, "Smoothly Unmount", "Demo: smooth-closed.tsx", buildSmoothClosedDemo());
  addSection(root, "Custom Icon", "Demo: custom-icon.tsx", buildCustomIconDemo());
  addSection(root, "Custom action", "Demo: action.tsx", buildActionDemo());
  addSection(root, "Component Token", "Demo: component-token.tsx", buildComponentTokenDemo());
  addSection(root, "Custom semantic dom styling", "Demo: style-class.tsx", buildStyleClassDemo());

  root->addStretch();
}

const QVector<QWidget*>& AlertDocsPage::sectionAnchors() const { return anchors_; }

const QStringList& AlertDocsPage::sectionTitles() const { return titles_; }

void AlertDocsPage::addSection(QVBoxLayout* root,
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

AdAlert* AlertDocsPage::makeAlert(const QString& title,
                                  std::optional<AdAlert::Type> type,
                                  std::optional<bool> showIcon,
                                  bool closable,
                                  const QString& description,
                                  QWidget* parent) {
  auto* alert = new AdAlert(parent);
  if (type.has_value()) {
    alert->setType(type.value());
  }
  alert->setTitleText(title);
  if (showIcon.has_value()) {
    alert->setShowIcon(showIcon.value());
  }
  alert->setClosable(closable);
  alert->setDescriptionText(description);
  return alert;
}

QWidget* AlertDocsPage::buildBasicDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);
  layout->addWidget(makeAlert("Success Text", AdAlert::Type::Success));
  return box;
}

QWidget* AlertDocsPage::buildStyleDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);
  layout->addWidget(makeAlert("Success Text", AdAlert::Type::Success));
  layout->addWidget(makeAlert("Info Text", AdAlert::Type::Info));
  layout->addWidget(makeAlert("Warning Text", AdAlert::Type::Warning));
  layout->addWidget(makeAlert("Error Text", AdAlert::Type::Error));
  return box;
}

QWidget* AlertDocsPage::buildClosableDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* status = new QLabel("Close status: waiting.");
  const QList<QPair<QString, AdAlert::Type>> items = {
      {"Warning Title", AdAlert::Type::Warning},
      {"Success Title", AdAlert::Type::Success},
      {"Info Title", AdAlert::Type::Info},
      {"Error Title", AdAlert::Type::Error},
  };
  for (const auto& item : items) {
    AdAlert* alert = makeAlert(item.first, item.second, false, true);
    connect(alert, &AdAlert::closeRequested, status,
            [status, title = item.first]() { status->setText(QStringLiteral("Close status: closed \"%1\"").arg(title)); });
    layout->addWidget(alert);
  }
  layout->addWidget(status);
  return box;
}

QWidget* AlertDocsPage::buildDescriptionDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  layout->addWidget(makeAlert(
      "Success Text", AdAlert::Type::Success, false, false,
      "Success Description Success Description Success Description"));
  layout->addWidget(makeAlert(
      "Info Text", AdAlert::Type::Info, false, false,
      "Info Description Info Description Info Description Info Description"));
  layout->addWidget(makeAlert(
      "Warning Text", AdAlert::Type::Warning, false, false,
      "Warning Description Warning Description Warning Description Warning Description"));
  layout->addWidget(makeAlert(
      "Error Text", AdAlert::Type::Error, false, false,
      "Error Description Error Description Error Description Error Description"));
  return box;
}

QWidget* AlertDocsPage::buildIconDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  layout->addWidget(makeAlert("Success Tips", AdAlert::Type::Success, true));
  layout->addWidget(makeAlert("Informational Notes", AdAlert::Type::Info, true));
  layout->addWidget(makeAlert("Warning", AdAlert::Type::Warning, true, true));
  layout->addWidget(makeAlert("Error", AdAlert::Type::Error, true));
  layout->addWidget(makeAlert("Success Tips",
                              AdAlert::Type::Success,
                              true,
                              false,
                              "Detailed description and advice about successful copywriting."));
  layout->addWidget(makeAlert("Informational Notes",
                              AdAlert::Type::Info,
                              true,
                              false,
                              "Additional description and information about copywriting."));
  layout->addWidget(makeAlert("Warning",
                              AdAlert::Type::Warning,
                              true,
                              true,
                              "This is a warning notice about copywriting."));
  layout->addWidget(makeAlert("Error",
                              AdAlert::Type::Error,
                              true,
                              false,
                              "This is an error message about copywriting."));

  return box;
}

QWidget* AlertDocsPage::buildBannerDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* first = makeAlert("Warning text");
  first->setBanner(true);
  layout->addWidget(first);

  auto* second = makeAlert("Very long warning text warning text text text text text text text");
  second->setBanner(true);
  second->setClosable(true);
  layout->addWidget(second);

  auto* third = makeAlert("Warning text without icon");
  third->setBanner(true);
  third->setShowIcon(false);
  layout->addWidget(third);

  auto* fourth = makeAlert("Error text", AdAlert::Type::Error);
  fourth->setBanner(true);
  layout->addWidget(fourth);

  return box;
}

QWidget* AlertDocsPage::buildLoopBannerDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  auto* alert = makeAlert(QString());
  alert->setBanner(true);
  alert->setTitleWidget(
      new LoopingTextLabel("I can be a React component, multiple React components, or just some text.", alert));
  layout->addWidget(alert);
  return box;
}

QWidget* AlertDocsPage::buildSmoothClosedDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* alert = makeAlert("Alert Message Text", AdAlert::Type::Success, false, true);
  auto* status = new QLabel("Status: open");
  auto* toggleRow = new QHBoxLayout();
  toggleRow->setContentsMargins(0, 0, 0, 0);
  toggleRow->setSpacing(8);
  toggleRow->addWidget(new QLabel("Reopen:"));
  auto* reopenSwitch = new AdSwitch();
  reopenSwitch->setChecked(true);
  reopenSwitch->setDisabled(true);
  toggleRow->addWidget(reopenSwitch);
  toggleRow->addStretch();

  connect(alert, &AdAlert::openChanged, this, [status, reopenSwitch](bool open) {
    status->setText(open ? "Status: open" : "Status: closing");
    reopenSwitch->setChecked(open);
    reopenSwitch->setDisabled(open);
  });
  connect(alert, &AdAlert::afterClose, this, [status]() { status->setText("Status: closed (afterClose fired)"); });
  connect(reopenSwitch, &AdSwitch::changed, alert, [alert](bool checked) {
    if (checked) {
      alert->setOpen(true);
    }
  });

  layout->addWidget(alert);
  layout->addWidget(status);
  layout->addLayout(toggleRow);
  layout->addWidget(makeHintLabel("Click close to see smooth collapse animation, then use switch to reopen."));
  return box;
}

QWidget* AlertDocsPage::buildCustomIconDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  const auto smileIcon = outlined_icons::Smile();

  auto* first = makeAlert("showIcon = false", AdAlert::Type::Success, false);
  first->setIconToken(smileIcon);
  layout->addWidget(first);

  auto* second = makeAlert("Success Tips", AdAlert::Type::Success, true);
  second->setIconToken(smileIcon);
  layout->addWidget(second);

  auto* third = makeAlert("Informational Notes", AdAlert::Type::Info, true);
  third->setIconToken(smileIcon);
  layout->addWidget(third);

  auto* fourth = makeAlert("Warning", AdAlert::Type::Warning, true);
  fourth->setIconToken(smileIcon);
  layout->addWidget(fourth);

  auto* fifth = makeAlert("Error", AdAlert::Type::Error, true);
  fifth->setIconToken(smileIcon);
  layout->addWidget(fifth);

  auto* sixth = makeAlert("Success Tips",
                          AdAlert::Type::Success,
                          true,
                          false,
                          "Detailed description and advice about successful copywriting.");
  sixth->setIconToken(smileIcon);
  layout->addWidget(sixth);

  auto* seventh = makeAlert("Informational Notes",
                            AdAlert::Type::Info,
                            true,
                            false,
                            "Additional description and information about copywriting.");
  seventh->setIconToken(smileIcon);
  layout->addWidget(seventh);

  auto* eighth = makeAlert("Warning",
                           AdAlert::Type::Warning,
                           true,
                           false,
                           "This is a warning notice about copywriting.");
  eighth->setIconToken(smileIcon);
  layout->addWidget(eighth);

  auto* ninth = makeAlert("Error",
                          AdAlert::Type::Error,
                          true,
                          false,
                          "This is an error message about copywriting.");
  ninth->setIconToken(smileIcon);
  layout->addWidget(ninth);

  return box;
}

QWidget* AlertDocsPage::buildActionDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* first = makeAlert("Success Tips", AdAlert::Type::Success, true, true);
  auto* undo = new AdButton("UNDO");
  undo->setSize(AdButton::Size::Small);
  undo->setType(AdButton::Type::Text);
  first->setActionWidget(undo);
  layout->addWidget(first);

  auto* second = makeAlert("Error Text",
                           AdAlert::Type::Error,
                           true,
                           false,
                           "Error Description Error Description Error Description Error Description");
  auto* detail = new AdButton("Detail");
  detail->setSize(AdButton::Size::Small);
  detail->setDanger(true);
  second->setActionWidget(detail);
  layout->addWidget(second);

  auto* third = makeAlert("Warning Text", AdAlert::Type::Warning, false, true);
  auto* done = new AdButton("Done");
  done->setSize(AdButton::Size::Small);
  done->setType(AdButton::Type::Text);
  third->setActionWidget(done);
  layout->addWidget(third);

  auto* fourth = makeAlert("Info Text",
                           AdAlert::Type::Info,
                           false,
                           true,
                           "Info Description Info Description Info Description Info Description");
  auto* actions = new QWidget();
  auto* actionsLayout = new QVBoxLayout(actions);
  actionsLayout->setContentsMargins(0, 0, 0, 0);
  actionsLayout->setSpacing(4);
  auto* accept = new AdButton("Accept");
  accept->setSize(AdButton::Size::Small);
  accept->setType(AdButton::Type::Primary);
  auto* decline = new AdButton("Decline");
  decline->setSize(AdButton::Size::Small);
  decline->setDanger(true);
  decline->setGhost(true);
  actionsLayout->addWidget(accept);
  actionsLayout->addWidget(decline);
  fourth->setActionWidget(actions);
  layout->addWidget(fourth);

  return box;
}

QWidget* AlertDocsPage::buildComponentTokenDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* alert = makeAlert("Success Tips",
                          AdAlert::Type::Success,
                          true,
                          false,
                          "Detailed description and advice about successful copywriting.");
  alert->setIconToken(outlined_icons::Smile());
  AdAlert::ComponentTokens tokens;
  tokens.withDescriptionIconSize = 32;
  tokens.withDescriptionPadding = 16;
  alert->setComponentTokens(tokens);

  layout->addWidget(alert);
  layout->addWidget(makeHintLabel(
      "Token demo overrides withDescriptionIconSize and withDescriptionPadding."));
  return box;
}

QWidget* AlertDocsPage::buildStyleClassDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* objectStyle = makeAlert("Object styles", AdAlert::Type::Info, true);
  auto* objectAction = new AdButton("Action");
  objectAction->setSize(AdButton::Size::Small);
  objectStyle->setActionWidget(objectAction);
  AdAlert::SemanticStyles objectSemantic;
  objectSemantic.root.backgroundColor = QColor(230, 244, 255);
  objectSemantic.root.borderColor = QColor("#91caff");
  objectSemantic.icon.textColor = QColor("#1677ff");
  objectSemantic.title.textColor = QColor("#0958d9");
  objectStyle->setSemanticStyles(objectSemantic);

  auto* functionStyle = makeAlert("Function styles", AdAlert::Type::Success, true);
  functionStyle->setSemanticStyleResolver([](const AdAlert::StyleContext& ctx) {
    AdAlert::SemanticStyles styles;
    if (ctx.type == AdAlert::Type::Success) {
      styles.root.backgroundColor = QColor(246, 255, 237);
      styles.root.borderColor = QColor("#b7eb8f");
      styles.icon.textColor = QColor("#52c41a");
      styles.title.textColor = QColor("#237804");
    } else if (ctx.type == AdAlert::Type::Warning) {
      styles.root.backgroundColor = QColor(255, 251, 230);
      styles.root.borderColor = QColor("#ffe58f");
      styles.icon.textColor = QColor("#faad14");
      styles.title.textColor = QColor("#ad6800");
    }
    return styles;
  });

  layout->addWidget(objectStyle);
  layout->addWidget(functionStyle);
  return box;
}
