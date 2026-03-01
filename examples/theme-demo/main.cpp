#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStyleFactory>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

#include "theme/theme.h"

using adqt::theme::ThemeAlgorithm;
using adqt::theme::ThemeConfig;
using adqt::theme::ThemeManager;

namespace {

ThemeConfig configForIndex(int index) {
  ThemeConfig config = adqt::theme::defaultThemeConfig();
  config.algorithms.clear();

  switch (index) {
    case 1:
      config.algorithms = {ThemeAlgorithm::Dark};
      break;
    case 2:
      config.algorithms = {ThemeAlgorithm::Compact};
      break;
    case 3:
      config.algorithms = {ThemeAlgorithm::Dark, ThemeAlgorithm::Compact};
      break;
    case 0:
    default:
      config.algorithms = {ThemeAlgorithm::Default};
      break;
  }

  return config;
}

class DemoWindow : public QWidget {
 public:
  DemoWindow() {
    setWindowTitle("ant-design-qt theme demo");
    resize(900, 560);

    auto* root = new QVBoxLayout(this);

    auto* topRow = new QHBoxLayout();
    auto* modeLabel = new QLabel("Theme:");
    modeBox_ = new QComboBox();
    modeBox_->addItem("Default");
    modeBox_->addItem("Dark");
    modeBox_->addItem("Compact");
    modeBox_->addItem("Dark + Compact");

    topRow->addWidget(modeLabel);
    topRow->addWidget(modeBox_);
    topRow->addStretch();
    root->addLayout(topRow);

    auto* sampleFrame = new QFrame();
    sampleFrame->setFrameShape(QFrame::StyledPanel);
    auto* sampleLayout = new QHBoxLayout(sampleFrame);

    auto* controls = new QGroupBox("Widget Samples");
    auto* controlsLayout = new QFormLayout(controls);

    auto* primaryButton = new QPushButton("Primary");
    primaryButton->setObjectName("primaryButton");
    auto* disabledButton = new QPushButton("Disabled");
    disabledButton->setEnabled(false);

    auto* lineEdit = new QLineEdit();
    lineEdit->setPlaceholderText("Placeholder text");
    auto* textEdit = new QTextEdit();
    textEdit->setPlainText("The palette is applied globally via ThemeManager.");
    textEdit->setFixedHeight(90);

    auto* check = new QCheckBox("Checked option");
    check->setChecked(true);

    controlsLayout->addRow("Button", primaryButton);
    controlsLayout->addRow("Disabled", disabledButton);
    controlsLayout->addRow("Input", lineEdit);
    controlsLayout->addRow("Text area", textEdit);
    controlsLayout->addRow("Checkbox", check);

    sampleLayout->addWidget(controls, 1);

    auto* tokenBox = new QGroupBox("Current Token Snapshot");
    auto* tokenLayout = new QVBoxLayout(tokenBox);
    infoLabel_ = new QLabel();
    infoLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    infoLabel_->setWordWrap(false);
    infoLabel_->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    infoLabel_->setStyleSheet("QLabel { font-family: Consolas, monospace; }");
    tokenLayout->addWidget(infoLabel_);

    sampleLayout->addWidget(tokenBox, 1);

    root->addWidget(sampleFrame, 1);

    connect(modeBox_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index) {
              ThemeManager::instance().setTheme(configForIndex(index));
              refreshInfo();
            });

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this,
            [this]() { refreshInfo(); });

    ThemeManager::instance().setTheme(configForIndex(0));
    refreshInfo();
  }

 private:
  void refreshInfo() {
    const auto& token = ThemeManager::instance().currentToken();
    const auto& map = ThemeManager::instance().currentMapToken();

    const QString text =
        QString(
            "colorPrimary          : %1\n"
            "colorPrimaryHover     : %2\n"
            "colorPrimaryActive    : %3\n"
            "colorLink             : %4\n"
            "colorBgLayout         : %5\n"
            "colorBgContainer      : %6\n"
            "colorText             : %7\n"
            "\n"
            "sizeXXL sizeXL sizeLG : %8 %9 %10\n"
            "sizeMD sizeMS size    : %11 %12 %13\n"
            "sizeSM sizeXS sizeXXS : %14 %15 %16\n"
            "controlHeight         : %17\n"
            "controlHeightSM/XS/LG : %18 / %19 / %20")
            .arg(token.colorPrimary, token.colorPrimaryHover, token.colorPrimaryActive,
                 token.colorLink, token.colorBgLayout, token.colorBgContainer, token.colorText,
                 QString::number(map.sizeXXL), QString::number(map.sizeXL),
                 QString::number(map.sizeLG), QString::number(map.sizeMD),
                 QString::number(map.sizeMS), QString::number(map.size),
                 QString::number(map.sizeSM), QString::number(map.sizeXS),
                 QString::number(map.sizeXXS), QString::number(map.controlHeight),
                 QString::number(map.controlHeightSM), QString::number(map.controlHeightXS),
                 QString::number(map.controlHeightLG));

    infoLabel_->setText(text);
  }

  QComboBox* modeBox_ = nullptr;
  QLabel* infoLabel_ = nullptr;
};

}  // namespace

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);

  if (QStyleFactory::keys().contains("Fusion", Qt::CaseInsensitive)) {
    app.setStyle(QStyleFactory::create("Fusion"));
  }

  ThemeManager::instance().applyTo(app);

  DemoWindow window;
  window.show();

  return app.exec();
}
