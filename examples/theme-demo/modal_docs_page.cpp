#include "modal_docs_page.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QRandomGenerator>
#include <QTimer>
#include <QVBoxLayout>

using adqt::widgets::AdButton;
using adqt::widgets::AdModal;

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

ModalDocsPage::ModalDocsPage(QWidget* parent) : QWidget(parent) {
  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(16, 16, 16, 24);
  root->setSpacing(16);

  auto* title = new QLabel("Modal");
  QFont titleFont = title->font();
  titleFont.setPointSize(titleFont.pointSize() + 8);
  titleFont.setBold(true);
  title->setFont(titleFont);
  root->addWidget(title);

  auto* subtitle = new QLabel(
      "A floating layer for focused tasks and confirmations. This page ports the official antd Modal demos.");
  subtitle->setWordWrap(true);
  root->addWidget(subtitle);

  addSection(root, "Basic", "Demo: basic.tsx", buildBasicDemo());
  addSection(root, "Asynchronously close", "Demo: async.tsx", buildAsyncDemo());
  addSection(root, "Customized Footer", "Demo: footer.tsx", buildFooterDemo());
  addSection(root, "Customized Footer render function", "Demo: footer-render.tsx", buildFooterRenderDemo());
  addSection(root, "Internationalization", "Demo: locale.tsx", buildLocaleDemo());
  addSection(root, "Loading", "Demo: loading.tsx", buildLoadingDemo());
  addSection(root, "Mask", "Demo: mask.tsx", buildMaskDemo());
  addSection(root, "Position", "Demo: position.tsx", buildPositionDemo());
  addSection(root, "Width", "Demo: width.tsx", buildWidthDemo());
  addSection(root, "Customize footer buttons props", "Demo: button-props.tsx", buildButtonPropsDemo());
  addSection(root, "Static Method", "Demo: static-info.tsx", buildStaticInfoDemo());
  addSection(root, "Static confirmation", "Demo: confirm.tsx", buildStaticConfirmDemo());
  addSection(root, "Manual to update destroy", "Demo: manual.tsx", buildManualDemo());
  addSection(root, "Destroy all static modals", "Demo: confirm-router.tsx", buildDestroyAllDemo());
  addSection(root, "Custom semantic dom styling", "Demo: style-class.tsx", buildStyleClassDemo());
  addSection(root, "Component Token / Wireframe", "Demo: component-token.tsx + wireframe.tsx",
             buildComponentTokenDemo());

  root->addStretch();
}

const QVector<QWidget*>& ModalDocsPage::sectionAnchors() const { return anchors_; }

const QStringList& ModalDocsPage::sectionTitles() const { return titles_; }

void ModalDocsPage::addSection(QVBoxLayout* root,
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

QWidget* ModalDocsPage::buildBasicDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(10);

  auto* openButton = new AdButton("Open Modal");
  openButton->setType(AdButton::Type::Primary);
  auto* status = new QLabel("Status: waiting");

  auto* modal = new AdModal(box);
  modal->setTitleText("Basic Modal");
  modal->setContentText("Some contents...\nSome contents...\nSome contents...");
  modal->setOpen(false);

  connect(openButton, &QPushButton::clicked, modal, [modal]() { modal->setOpen(true); });
  connect(modal, &AdModal::okTriggered, status, [status]() { status->setText("Status: OK clicked"); });
  connect(modal, &AdModal::cancelTriggered, status, [status]() {
    status->setText("Status: Cancel/Close clicked");
  });

  row->addWidget(openButton);
  row->addWidget(status);
  row->addStretch();
  return box;
}

QWidget* ModalDocsPage::buildAsyncDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* openButton = new AdButton("Open Modal with async logic");
  openButton->setType(AdButton::Type::Primary);
  auto* status = new QLabel("Status: waiting");

  auto* modal = new AdModal(box);
  modal->setTitleText("Title");
  modal->setContentText("Content of the modal");
  modal->setOkAutoClose(false);

  connect(openButton, &QPushButton::clicked, modal, [modal, status]() {
    modal->setContentText("Content of the modal");
    status->setText("Status: opened");
    modal->setOpen(true);
  });
  connect(modal, &AdModal::okTriggered, modal, [modal, status]() {
    modal->setContentText("The modal will be closed after two seconds");
    modal->setConfirmLoading(true);
    status->setText("Status: confirming...");
    QTimer::singleShot(2000, modal, [modal, status]() {
      modal->setConfirmLoading(false);
      modal->setOpen(false);
      status->setText("Status: closed");
    });
  });
  connect(modal, &AdModal::cancelTriggered, status, [status]() { status->setText("Status: canceled"); });

  layout->addWidget(openButton, 0, Qt::AlignLeft);
  layout->addWidget(status);
  return box;
}

QWidget* ModalDocsPage::buildFooterDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* openButton = new AdButton("Open Modal with customized footer");
  openButton->setType(AdButton::Type::Primary);

  auto* status = new QLabel("Status: waiting");
  auto* modal = new AdModal(box);
  modal->setTitleText("Title");
  modal->setContentText("Some contents...\nSome contents...\nSome contents...");

  auto* footerHost = new QWidget();
  auto* footerRow = new QHBoxLayout(footerHost);
  footerRow->setContentsMargins(0, 0, 0, 0);
  footerRow->setSpacing(8);
  auto* returnButton = new AdButton("Return");
  auto* submitButton = new AdButton("Submit");
  submitButton->setType(AdButton::Type::Primary);
  auto* searchButton = new AdButton("Search");
  searchButton->setType(AdButton::Type::Primary);
  footerRow->addStretch();
  footerRow->addWidget(returnButton);
  footerRow->addWidget(submitButton);
  footerRow->addWidget(searchButton);
  modal->setFooterWidget(footerHost);

  connect(openButton, &QPushButton::clicked, modal, [modal, status]() {
    status->setText("Status: opened");
    modal->setOpen(true);
  });
  connect(returnButton, &QPushButton::clicked, modal, [modal, status]() {
    modal->setOpen(false);
    status->setText("Status: return");
  });
  connect(submitButton, &QPushButton::clicked, modal, [modal, status, submitButton]() {
    submitButton->setLoading(true);
    status->setText("Status: submit loading");
    QTimer::singleShot(1200, submitButton, [modal, status, submitButton]() {
      submitButton->setLoading(false);
      modal->setOpen(false);
      status->setText("Status: submit complete");
    });
  });
  connect(searchButton, &QPushButton::clicked, status, [status]() {
    status->setText("Status: custom action from footer");
  });

  layout->addWidget(openButton, 0, Qt::AlignLeft);
  layout->addWidget(status);
  return box;
}

QWidget* ModalDocsPage::buildFooterRenderDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(10);

  auto* openModalButton = new AdButton("Open Modal");
  openModalButton->setType(AdButton::Type::Primary);
  auto* openConfirmButton = new AdButton("Open Modal Confirm");
  openConfirmButton->setType(AdButton::Type::Primary);

  auto* modal = new AdModal(box);
  modal->setTitleText("Title");
  modal->setContentText("Some contents...\nSome contents...\nSome contents...");

  auto* footerHost = new QWidget();
  auto* footerRow = new QHBoxLayout(footerHost);
  footerRow->setContentsMargins(0, 0, 0, 0);
  footerRow->setSpacing(8);
  auto* customButton = new AdButton("Custom Button");
  auto* cancelButton = new AdButton("Cancel");
  auto* okButton = new AdButton("OK");
  okButton->setType(AdButton::Type::Primary);
  footerRow->addStretch();
  footerRow->addWidget(customButton);
  footerRow->addWidget(cancelButton);
  footerRow->addWidget(okButton);
  modal->setFooterWidget(footerHost);

  connect(openModalButton, &QPushButton::clicked, modal, [modal]() { modal->setOpen(true); });
  connect(customButton, &QPushButton::clicked, modal, [modal]() {
    modal->setContentText("Custom footer button clicked.");
  });
  connect(cancelButton, &QPushButton::clicked, modal, [modal]() { modal->setOpen(false); });
  connect(okButton, &QPushButton::clicked, modal, [modal]() { modal->setOpen(false); });

  connect(openConfirmButton, &QPushButton::clicked, this, [this]() {
    AdModal::StaticConfig config;
    config.titleText = QStringLiteral("Confirm");
    config.contentText = QStringLiteral("Bla bla ...");
    config.onOk = [](AdModal*) { return true; };
    config.onCancel = [](AdModal*) { return true; };
    AdModal* staticModal = AdModal::confirm(config, window());
    if (!staticModal) {
      return;
    }

    auto* footer = new QWidget(staticModal);
    auto* row = new QHBoxLayout(footer);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(8);
    auto* custom = new AdButton("Custom Button");
    auto* cancel = new AdButton("Cancel");
    auto* ok = new AdButton("OK");
    ok->setType(AdButton::Type::Primary);
    row->addStretch();
    row->addWidget(custom);
    row->addWidget(cancel);
    row->addWidget(ok);
    staticModal->setFooterWidget(footer);

    connect(custom, &QPushButton::clicked, staticModal,
            [staticModal]() { staticModal->setContentText("Custom static footer clicked."); });
    connect(cancel, &QPushButton::clicked, staticModal, [staticModal]() { staticModal->setOpen(false); });
    connect(ok, &QPushButton::clicked, staticModal, [staticModal]() { staticModal->setOpen(false); });
  });

  row->addWidget(openModalButton);
  row->addWidget(openConfirmButton);
  row->addStretch();
  return box;
}

QWidget* ModalDocsPage::buildLocaleDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(10);

  auto* modalButton = new AdButton("Modal");
  modalButton->setType(AdButton::Type::Primary);
  auto* confirmButton = new AdButton("Confirm");

  auto* modal = new AdModal(box);
  modal->setTitleText("Modal");
  modal->setContentText("Bla bla ...");
  modal->setOkText(QStringLiteral("确认"));
  modal->setCancelText(QStringLiteral("取消"));

  connect(modalButton, &QPushButton::clicked, modal, [modal]() { modal->setOpen(true); });
  connect(confirmButton, &QPushButton::clicked, this, [this]() {
    AdModal::StaticConfig config;
    config.titleText = QStringLiteral("Confirm");
    config.contentText = QStringLiteral("Bla bla ...");
    config.okText = QStringLiteral("确认");
    config.cancelText = QStringLiteral("取消");
    config.showCancel = true;
    AdModal::confirm(config, window());
  });

  row->addWidget(modalButton);
  row->addWidget(confirmButton);
  row->addStretch();
  return box;
}

QWidget* ModalDocsPage::buildLoadingDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* openButton = new AdButton("Open Modal");
  openButton->setType(AdButton::Type::Primary);

  auto* modal = new AdModal(box);
  modal->setTitleText("Loading Modal");
  modal->setContentText("Some contents...\nSome contents...\nSome contents...");
  modal->setShowCancel(false);

  auto* footerHost = new QWidget();
  auto* footerRow = new QHBoxLayout(footerHost);
  footerRow->setContentsMargins(0, 0, 0, 0);
  auto* reloadButton = new AdButton("Reload");
  reloadButton->setType(AdButton::Type::Primary);
  footerRow->addStretch();
  footerRow->addWidget(reloadButton);
  modal->setFooterWidget(footerHost);

  connect(openButton, &QPushButton::clicked, modal, [modal]() {
    modal->setOpen(true);
    modal->setLoading(true);
    QTimer::singleShot(2000, modal, [modal]() { modal->setLoading(false); });
  });
  connect(reloadButton, &QPushButton::clicked, modal, [modal]() {
    modal->setLoading(true);
    QTimer::singleShot(2000, modal, [modal]() { modal->setLoading(false); });
  });
  connect(modal, &AdModal::cancelTriggered, modal, [modal]() { modal->setOpen(false); });

  layout->addWidget(openButton, 0, Qt::AlignLeft);
  layout->addWidget(makeHintLabel("Loading=true replaces content with a skeleton-like placeholder text."));
  return box;
}

QWidget* ModalDocsPage::buildMaskDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(8);

  auto* dimmed = new AdButton("Dimmed mask");
  auto* noMask = new AdButton("No mask");
  auto* notClosable = new AdButton("Mask not closable");

  const auto openConfirm = [this](bool maskEnabled, bool closable) {
    AdModal::StaticConfig config;
    config.titleText = QStringLiteral("Title");
    config.contentText = QStringLiteral("Some contents...");
    config.mask = maskEnabled;
    config.maskClosable = closable;
    config.showCancel = true;
    AdModal::confirm(config, window());
  };

  connect(dimmed, &QPushButton::clicked, this, [openConfirm]() { openConfirm(true, true); });
  connect(noMask, &QPushButton::clicked, this, [openConfirm]() { openConfirm(false, false); });
  connect(notClosable, &QPushButton::clicked, this, [openConfirm]() { openConfirm(true, false); });

  row->addWidget(dimmed);
  row->addWidget(noMask);
  row->addWidget(notClosable);
  row->addStretch();
  return box;
}

QWidget* ModalDocsPage::buildPositionDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(10);

  auto* topButton = new AdButton("Display a modal dialog at 20px to Top");
  topButton->setType(AdButton::Type::Primary);
  auto* centeredButton = new AdButton("Vertically centered modal dialog");
  centeredButton->setType(AdButton::Type::Primary);

  auto* topModal = new AdModal(box);
  topModal->setTitleText("20px to Top");
  topModal->setContentText("some contents...\nsome contents...\nsome contents...");
  topModal->setTop(20);
  topModal->setCentered(false);

  auto* centeredModal = new AdModal(box);
  centeredModal->setTitleText("Vertically centered modal dialog");
  centeredModal->setContentText("some contents...\nsome contents...\nsome contents...");
  centeredModal->setCentered(true);

  connect(topButton, &QPushButton::clicked, topModal, [topModal]() { topModal->setOpen(true); });
  connect(centeredButton, &QPushButton::clicked, centeredModal,
          [centeredModal]() { centeredModal->setOpen(true); });

  layout->addWidget(topButton, 0, Qt::AlignLeft);
  layout->addWidget(centeredButton, 0, Qt::AlignLeft);
  return box;
}

QWidget* ModalDocsPage::buildWidthDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* fixedButton = new AdButton("Open Modal of 1000px width");
  fixedButton->setType(AdButton::Type::Primary);
  auto* responsiveButton = new AdButton("Open Modal of responsive width");
  responsiveButton->setType(AdButton::Type::Primary);

  auto* fixedModal = new AdModal(box);
  fixedModal->setTitleText("Modal 1000px width");
  fixedModal->setContentText("some contents...\nsome contents...\nsome contents...");
  fixedModal->setCentered(true);
  fixedModal->setWidth(1000);

  auto* responsiveModal = new AdModal(box);
  responsiveModal->setTitleText("Modal responsive width");
  responsiveModal->setContentText("some contents...\nsome contents...\nsome contents...");
  responsiveModal->setCentered(true);

  connect(fixedButton, &QPushButton::clicked, fixedModal, [fixedModal]() { fixedModal->setOpen(true); });
  connect(responsiveButton, &QPushButton::clicked, responsiveModal, [responsiveModal, this]() {
    const int viewportWidth = window() ? window()->width() : 1280;
    int width = static_cast<int>(viewportWidth * 0.9);
    if (viewportWidth >= 1600) {
      width = static_cast<int>(viewportWidth * 0.4);
    } else if (viewportWidth >= 1200) {
      width = static_cast<int>(viewportWidth * 0.5);
    } else if (viewportWidth >= 992) {
      width = static_cast<int>(viewportWidth * 0.6);
    } else if (viewportWidth >= 768) {
      width = static_cast<int>(viewportWidth * 0.7);
    } else if (viewportWidth >= 576) {
      width = static_cast<int>(viewportWidth * 0.8);
    }
    responsiveModal->setWidth(width);
    responsiveModal->setOpen(true);
  });

  layout->addWidget(fixedButton, 0, Qt::AlignLeft);
  layout->addWidget(responsiveButton, 0, Qt::AlignLeft);
  return box;
}

QWidget* ModalDocsPage::buildButtonPropsDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* openButton = new AdButton("Open Modal with customized button props");
  openButton->setType(AdButton::Type::Primary);

  auto* modal = new AdModal(box);
  modal->setTitleText("Basic Modal");
  modal->setContentText("Some contents...\nSome contents...\nSome contents...");
  modal->setOpen(false);

  connect(openButton, &QPushButton::clicked, modal, [modal]() {
    modal->setOpen(true);
    if (modal->okButton()) {
      modal->okButton()->setEnabled(false);
    }
    if (modal->cancelButton()) {
      modal->cancelButton()->setEnabled(false);
    }
  });
  connect(modal, &AdModal::afterOpenChange, modal, [modal](bool open) {
    if (!open) {
      if (modal->okButton()) {
        modal->okButton()->setEnabled(true);
      }
      if (modal->cancelButton()) {
        modal->cancelButton()->setEnabled(true);
      }
    }
  });

  layout->addWidget(openButton, 0, Qt::AlignLeft);
  layout->addWidget(makeHintLabel("OK / Cancel are disabled while this demo modal is open."));
  return box;
}

QWidget* ModalDocsPage::buildStaticInfoDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(8);

  auto* info = new AdButton("Info");
  auto* success = new AdButton("Success");
  auto* error = new AdButton("Error");
  auto* warning = new AdButton("Warning");

  connect(info, &QPushButton::clicked, this, [this]() {
    AdModal::StaticConfig config;
    config.titleText = QStringLiteral("This is a notification message");
    config.contentText = QStringLiteral("some messages...some messages...");
    AdModal::info(config, window());
  });
  connect(success, &QPushButton::clicked, this, [this]() {
    AdModal::StaticConfig config;
    config.contentText = QStringLiteral("some messages...some messages...");
    AdModal::success(config, window());
  });
  connect(error, &QPushButton::clicked, this, [this]() {
    AdModal::StaticConfig config;
    config.titleText = QStringLiteral("This is an error message");
    config.contentText = QStringLiteral("some messages...some messages...");
    AdModal::error(config, window());
  });
  connect(warning, &QPushButton::clicked, this, [this]() {
    AdModal::StaticConfig config;
    config.titleText = QStringLiteral("This is a warning message");
    config.contentText = QStringLiteral("some messages...some messages...");
    AdModal::warning(config, window());
  });

  row->addWidget(info);
  row->addWidget(success);
  row->addWidget(error);
  row->addWidget(warning);
  row->addStretch();
  return box;
}

QWidget* ModalDocsPage::buildStaticConfirmDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(8);

  auto* confirmButton = new AdButton("Confirm");
  auto* promiseButton = new AdButton("With promise");
  auto* deleteButton = new AdButton("Delete");
  deleteButton->setType(AdButton::Type::Dashed);
  auto* propsButton = new AdButton("With extra props");
  propsButton->setType(AdButton::Type::Dashed);

  connect(confirmButton, &QPushButton::clicked, this, [this]() {
    AdModal::StaticConfig config;
    config.titleText = QStringLiteral("Do you want to delete these items?");
    config.contentText = QStringLiteral("Some descriptions");
    config.showCancel = true;
    AdModal::confirm(config, window());
  });

  connect(promiseButton, &QPushButton::clicked, this, [this]() {
    AdModal::StaticConfig config;
    config.titleText = QStringLiteral("Do you want to delete these items?");
    config.contentText =
        QStringLiteral("When clicked the OK button, this dialog will be closed after 1 second");
    config.showCancel = true;
    config.onOk = [](AdModal* modal) {
      if (!modal) {
        return false;
      }
      modal->setConfirmLoading(true);
      const bool shouldResolve = QRandomGenerator::global()->bounded(100) >= 50;
      QTimer::singleShot(1000, modal, [modal, shouldResolve]() {
        modal->setConfirmLoading(false);
        if (shouldResolve) {
          modal->setOpen(false);
        } else {
          modal->setContentText("Oops errors! Click OK again.");
        }
      });
      return false;
    };
    AdModal::confirm(config, window());
  });

  connect(deleteButton, &QPushButton::clicked, this, [this]() {
    AdModal::StaticConfig config;
    config.titleText = QStringLiteral("Are you sure delete this task?");
    config.contentText = QStringLiteral("Some descriptions");
    config.okText = QStringLiteral("Yes");
    config.cancelText = QStringLiteral("No");
    config.okType = AdButton::Type::Primary;
    config.showCancel = true;
    AdModal::confirm(config, window());
  });

  connect(propsButton, &QPushButton::clicked, this, [this]() {
    AdModal::StaticConfig config;
    config.titleText = QStringLiteral("Are you sure delete this task?");
    config.contentText = QStringLiteral("Some descriptions");
    config.okText = QStringLiteral("Yes");
    config.cancelText = QStringLiteral("No");
    config.showCancel = true;
    AdModal* modal = AdModal::confirm(config, window());
    if (modal && modal->okButton()) {
      modal->okButton()->setEnabled(false);
    }
  });

  row->addWidget(confirmButton);
  row->addWidget(promiseButton);
  row->addWidget(deleteButton);
  row->addWidget(propsButton);
  row->addStretch();
  return box;
}

QWidget* ModalDocsPage::buildManualDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(8);

  auto* openButton = new AdButton("Open modal to close in 5s");
  auto* status = new QLabel("Status: waiting");

  connect(openButton, &QPushButton::clicked, this, [this, status]() {
    int* secondsToGo = new int(5);
    AdModal::StaticConfig config;
    config.titleText = QStringLiteral("This is a notification message");
    config.contentText = QStringLiteral("This modal will be destroyed after 5 seconds.");
    config.showCancel = false;
    AdModal* modal = AdModal::success(config, window());
    if (!modal) {
      delete secondsToGo;
      return;
    }

    status->setText("Status: countdown started");
    auto* timer = new QTimer(modal);
    connect(timer, &QTimer::timeout, modal, [modal, timer, secondsToGo, status]() {
      *secondsToGo -= 1;
      if (*secondsToGo <= 0) {
        timer->stop();
        modal->destroy();
        status->setText("Status: destroyed");
        delete secondsToGo;
        return;
      }
      modal->setContentText(QStringLiteral("This modal will be destroyed after %1 seconds.")
                                .arg(*secondsToGo));
    });
    timer->start(1000);
  });

  row->addWidget(openButton);
  row->addWidget(status);
  row->addStretch();
  return box;
}

QWidget* ModalDocsPage::buildDestroyAllDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* openButton = new AdButton("Open 3 confirms");
  auto* destroyAllButton = new AdButton("Destroy all");
  auto* row = new QHBoxLayout();
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(8);
  row->addWidget(openButton);
  row->addWidget(destroyAllButton);
  row->addStretch();

  connect(openButton, &QPushButton::clicked, this, [this]() {
    for (int i = 0; i < 3; ++i) {
      QTimer::singleShot(i * 400, this, [this, i]() {
        AdModal::StaticConfig config;
        config.titleText = QStringLiteral("Confirmation #%1").arg(i + 1);
        config.contentText = QStringLiteral("Click destroy-all to close every static modal.");
        config.showCancel = true;
        AdModal::confirm(config, window());
      });
    }
  });
  connect(destroyAllButton, &QPushButton::clicked, this, []() { AdModal::destroyAll(); });

  layout->addLayout(row);
  layout->addWidget(makeHintLabel(
      "Equivalent of Modal.destroyAll() on router change. Here it closes all static modals."));
  return box;
}

QWidget* ModalDocsPage::buildStyleClassDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(8);

  auto* objectButton = new AdButton("Open Style Modal");
  auto* resolverButton = new AdButton("Open Function Modal");
  objectButton->setType(AdButton::Type::Primary);
  resolverButton->setType(AdButton::Type::Primary);

  connect(objectButton, &QPushButton::clicked, this, [this]() {
    auto* modal = new AdModal(this);
    connect(modal, &AdModal::afterClose, modal, &QObject::deleteLater);
    modal->setTitleText("Custom Style Modal");
    modal->setContentText("Following the Ant Design specification, this demo customizes semantic slots.");
    modal->setDestroyOnHidden(true);
    AdModal::SemanticStyles styles;
    styles.mask.backgroundColor = QColor(24, 24, 27, 230);
    styles.container.backgroundColor = QColor("#f7f8fa");
    styles.container.borderColor = QColor("#d9d9d9");
    styles.title.textColor = QColor("#171717");
    styles.body.textColor = QColor("#171717");
    styles.footer.backgroundColor = QColor("#fafafa");
    modal->setSemanticStyles(styles);
    modal->setOpen(true);
  });

  connect(resolverButton, &QPushButton::clicked, this, [this]() {
    auto* modal = new AdModal(this);
    connect(modal, &AdModal::afterClose, modal, &QObject::deleteLater);
    modal->setTitleText("Custom Function Modal");
    modal->setContentText("Semantic style resolver changes color theme based on open state.");
    modal->setDestroyOnHidden(true);
    modal->setSemanticStyleResolver([](const AdModal::StyleContext& ctx) {
      AdModal::SemanticStyles styles;
      if (ctx.open) {
        styles.container.backgroundColor = QColor("#fffbe6");
        styles.container.borderColor = QColor("#ffe58f");
        styles.title.textColor = QColor("#ad6800");
        styles.body.textColor = QColor("#ad6800");
      } else {
        styles.container.backgroundColor = QColor(53, 71, 125, 204);
        styles.title.textColor = QColor("#ffffff");
        styles.body.textColor = QColor("#ffffff");
      }
      return styles;
    });
    modal->setOpen(true);
  });

  row->addWidget(objectButton);
  row->addWidget(resolverButton);
  row->addStretch();
  return box;
}

QWidget* ModalDocsPage::buildComponentTokenDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(8);

  auto* tokenButton = new AdButton("Open token modal");
  auto* wireframeButton = new AdButton("Open wireframe modal");
  tokenButton->setType(AdButton::Type::Primary);
  wireframeButton->setType(AdButton::Type::Default);

  connect(tokenButton, &QPushButton::clicked, this, [this]() {
    auto* modal = new AdModal(this);
    connect(modal, &AdModal::afterClose, modal, &QObject::deleteLater);
    modal->setTitleText("Component Token Modal");
    modal->setContentText("Footer/header/content colors and metrics are overridden via component tokens.");
    modal->setDestroyOnHidden(true);
    AdModal::ComponentTokens tokens;
    tokens.headerBg = QStringLiteral("#f9f0ff");
    tokens.bodyBg = QStringLiteral("#e6fffb");
    tokens.footerBg = QStringLiteral("#f6ffed");
    tokens.titleColor = QStringLiteral("#1d39c4");
    tokens.borderRadius = 12;
    tokens.width = 560;
    tokens.headerPaddingHorizontal = 20;
    tokens.bodyPaddingHorizontal = 20;
    tokens.footerPaddingHorizontal = 20;
    modal->setComponentTokens(tokens);
    modal->setOpen(true);
  });

  connect(wireframeButton, &QPushButton::clicked, this, [this]() {
    auto* modal = new AdModal(this);
    connect(modal, &AdModal::afterClose, modal, &QObject::deleteLater);
    modal->setTitleText("Wireframe-like Modal");
    modal->setContentText("A wireframe style look using semantic slots and token overrides.");
    modal->setDestroyOnHidden(true);
    AdModal::ComponentTokens tokens;
    tokens.contentBg = QStringLiteral("#ffffff");
    tokens.headerBg = QStringLiteral("#ffffff");
    tokens.footerBg = QStringLiteral("#ffffff");
    tokens.borderColor = QStringLiteral("#d9d9d9");
    tokens.borderRadius = 8;
    modal->setComponentTokens(tokens);

    AdModal::SemanticStyles styles;
    styles.title.textColor = QColor("#262626");
    styles.body.textColor = QColor("#595959");
    styles.mask.backgroundColor = QColor(0, 0, 0, 70);
    modal->setSemanticStyles(styles);
    modal->setOpen(true);
  });

  row->addWidget(tokenButton);
  row->addWidget(wireframeButton);
  row->addStretch();
  return box;
}
