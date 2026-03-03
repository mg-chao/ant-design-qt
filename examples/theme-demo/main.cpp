#include <QAbstractSlider>
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPaintEvent>
#include <QPainter>
#include <QRadioButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QStackedWidget>
#include <QStyleFactory>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <functional>

#include "icon_theme_adapter.h"
#include "icons.h"
#include "menu_docs_page.h"
#include "select_docs_page.h"
#include "theme/theme.h"
#include "widgets/widgets.h"

using adqt::theme::ThemeAlgorithm;
using adqt::theme::ThemeConfig;
using adqt::theme::ThemeManager;
using adqt::widgets::AdButton;
using adqt::widgets::AdButtonGroup;
using adqt::widgets::AdMenu;
namespace outlined_icons = adqt::icons::outlined;

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

class GhostDemoPanel final : public QFrame {
 public:
  explicit GhostDemoPanel(QWidget* parent = nullptr) : QFrame(parent) {
    setFrameStyle(QFrame::NoFrame);
  }

 protected:
  void paintEvent(QPaintEvent* event) override {
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(190, 200, 200));
    painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 6.0, 6.0);
  }
};

class ButtonDocsPage final : public QWidget {
 public:
  explicit ButtonDocsPage(QWidget* parent = nullptr) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 24);
    root->setSpacing(16);

    auto* title = new QLabel("Button");
    QFont titleFont = title->font();
    titleFont.setPointSize(titleFont.pointSize() + 8);
    titleFont.setBold(true);
    title->setFont(titleFont);
    root->addWidget(title);

    auto* subtitle = new QLabel("Buttons are used to trigger an immediate action.");
    subtitle->setWordWrap(true);
    root->addWidget(subtitle);

    addSection(root, "Button Types",
               "Use `type` to switch among primary/default/dashed/text/link button styles.",
               buildBasicDemo());
    addSection(root, "Color & Variant",
               "Set `color` and `variant` together to compose additional button appearances.",
               buildColorVariantDemo());
    addSection(root, "Button Icon", "Set `iconToken` to show an icon in the button.", buildIconDemo());
    addSection(root, "Icon Placement",
               "Set `iconPlacement` to `start` or `end` to control icon position.",
               buildIconPlacementDemo());
    addSection(root, "Button Size",
               "Buttons support large/middle/small sizes via the `size` property.",
               buildSizeDemo());
    addSection(root, "Disabled",
               "Set `disabled` to make a button non-interactive and visually disabled.",
               buildDisabledDemo());
    addSection(root, "Loading",
               "Set `loading` to show a spinner, or use `loadingIconToken` for a custom loading icon.",
               buildLoadingDemo());
    addSection(root, "Button Group",
               "Group multiple related actions together using button group patterns.",
               buildMultipleDemo());
    addSection(root, "Ghost Button",
               "Ghost buttons invert foreground and keep transparent background.",
               buildGhostDemo());
    addSection(root, "Danger Button",
               "Use danger styling for destructive actions.", buildDangerDemo());
    addSection(root, "Block Button",
               "Set `block` so the button spans the parent width.", buildBlockDemo());
    root->addStretch();
  }

  const QVector<QWidget*>& sectionAnchors() const { return anchors_; }

  const QStringList& sectionTitles() const { return titles_; }


  void addSection(QVBoxLayout* root,
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

  QWidget* buildBasicDemo() {
    auto* box = new QWidget();
    auto* row = new QHBoxLayout(box);
    row->setContentsMargins(0, 0, 0, 0);

    auto* primary = new AdButton("Primary Button");
    primary->setType(AdButton::Type::Primary);
    row->addWidget(primary);

    row->addWidget(new AdButton("Default Button"));

    auto* dashed = new AdButton("Dashed Button");
    dashed->setType(AdButton::Type::Dashed);
    row->addWidget(dashed);

    auto* text = new AdButton("Text Button");
    text->setType(AdButton::Type::Text);
    row->addWidget(text);

    auto* link = new AdButton("Link Button");
    link->setType(AdButton::Type::Link);
    row->addWidget(link);

    row->addStretch();
    return box;
  }

  QWidget* buildColorVariantDemo() {
    auto* box = new QWidget();
    auto* layout = new QVBoxLayout(box);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    const QList<AdButton::Color> colors = {
        AdButton::Color::Default, AdButton::Color::Primary, AdButton::Color::Danger,
        AdButton::Color::Pink,    AdButton::Color::Purple,  AdButton::Color::Cyan,
    };
    const QList<QPair<QString, AdButton::Variant>> variants = {
        {"Solid", AdButton::Variant::Solid},       {"Outlined", AdButton::Variant::Outlined},
        {"Dashed", AdButton::Variant::Dashed},     {"Filled", AdButton::Variant::Filled},
        {"Text", AdButton::Variant::Text},         {"Link", AdButton::Variant::Link},
    };

    for (AdButton::Color color : colors) {
      auto* row = new QHBoxLayout();
      for (const auto& variant : variants) {
        auto* button = new AdButton(variant.first);
        button->setColor(color);
        button->setVariant(variant.second);
        row->addWidget(button);
      }

      row->addStretch();
      layout->addLayout(row);
    }

    return box;
  }

  QWidget* buildIconDemo() {
    auto* box = new QWidget();
    auto* layout = new QVBoxLayout(box);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto* row1 = new QHBoxLayout();
    row1->setSpacing(8);
    const auto search = outlined_icons::Search();

    auto* pCircle = new AdButton();
    pCircle->setType(AdButton::Type::Primary);
    pCircle->setShape(AdButton::Shape::Circle);
    pCircle->setIconToken(search);
    row1->addWidget(pCircle);

    auto* pCircleA = new AdButton("A");
    pCircleA->setType(AdButton::Type::Primary);
    pCircleA->setShape(AdButton::Shape::Circle);
    row1->addWidget(pCircleA);

    auto* pIcon = new AdButton("Search");
    pIcon->setType(AdButton::Type::Primary);
    pIcon->setIconToken(search);
    row1->addWidget(pIcon);

    auto* defaultCircle = new AdButton();
    defaultCircle->setShape(AdButton::Shape::Circle);
    defaultCircle->setIconToken(search);
    row1->addWidget(defaultCircle);

    auto* defaultIcon = new AdButton("Search");
    defaultIcon->setIconToken(search);
    row1->addWidget(defaultIcon);
    row1->addStretch();

    auto* row2 = new QHBoxLayout();
    row2->setSpacing(8);
    auto* defaultCircle2 = new AdButton();
    defaultCircle2->setShape(AdButton::Shape::Circle);
    defaultCircle2->setIconToken(search);
    row2->addWidget(defaultCircle2);

    auto* defaultIcon2 = new AdButton("Search");
    defaultIcon2->setIconToken(search);
    row2->addWidget(defaultIcon2);

    auto* dashedCircle = new AdButton();
    dashedCircle->setType(AdButton::Type::Dashed);
    dashedCircle->setShape(AdButton::Shape::Circle);
    dashedCircle->setIconToken(search);
    row2->addWidget(dashedCircle);

    auto* dashedIcon = new AdButton("Search");
    dashedIcon->setType(AdButton::Type::Dashed);
    dashedIcon->setIconToken(search);
    row2->addWidget(dashedIcon);

    auto* defaultIconOnly = new AdButton();
    defaultIconOnly->setIconToken(search);
    row2->addWidget(defaultIconOnly);
    row2->addStretch();

    layout->addLayout(row1);
    layout->addLayout(row2);
    return box;
  }

  QWidget* buildIconPlacementDemo() {
    auto* box = new QWidget();
    auto* layout = new QVBoxLayout(box);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto* switchRow = new QHBoxLayout();
    switchRow->addWidget(new QLabel("iconPlacement:"));
    iconPlacementBox_ = new QComboBox();
    iconPlacementBox_->addItem("start", static_cast<int>(AdButton::IconPlacement::Start));
    iconPlacementBox_->addItem("end", static_cast<int>(AdButton::IconPlacement::End));
    iconPlacementBox_->setCurrentIndex(1);
    switchRow->addWidget(iconPlacementBox_);
    switchRow->addStretch();
    layout->addLayout(switchRow);

    iconPlacementButtons_.clear();
    const auto icon = outlined_icons::Search();

    auto* row1 = new QHBoxLayout();
    row1->setSpacing(8);

    auto* primaryCircle = new AdButton();
    primaryCircle->setType(AdButton::Type::Primary);
    primaryCircle->setShape(AdButton::Shape::Circle);
    primaryCircle->setIconToken(icon);
    row1->addWidget(primaryCircle);

    auto* primaryCircleText = new AdButton("A");
    primaryCircleText->setType(AdButton::Type::Primary);
    primaryCircleText->setShape(AdButton::Shape::Circle);
    row1->addWidget(primaryCircleText);

    auto* primarySearch = new AdButton("Search");
    primarySearch->setType(AdButton::Type::Primary);
    primarySearch->setIconToken(icon);
    row1->addWidget(primarySearch);
    iconPlacementButtons_.append(primarySearch);

    auto* defaultCircle = new AdButton();
    defaultCircle->setShape(AdButton::Shape::Circle);
    defaultCircle->setIconToken(icon);
    row1->addWidget(defaultCircle);

    auto* defaultSearch = new AdButton("Search");
    defaultSearch->setIconToken(icon);
    row1->addWidget(defaultSearch);
    iconPlacementButtons_.append(defaultSearch);

    row1->addStretch();
    layout->addLayout(row1);

    auto* row2 = new QHBoxLayout();
    row2->setSpacing(8);

    auto* defaultCircle2 = new AdButton();
    defaultCircle2->setShape(AdButton::Shape::Circle);
    defaultCircle2->setIconToken(icon);
    row2->addWidget(defaultCircle2);

    auto* textSearch = new AdButton("Search");
    textSearch->setType(AdButton::Type::Text);
    textSearch->setIconToken(icon);
    row2->addWidget(textSearch);
    iconPlacementButtons_.append(textSearch);

    auto* dashedCircle = new AdButton();
    dashedCircle->setType(AdButton::Type::Dashed);
    dashedCircle->setShape(AdButton::Shape::Circle);
    dashedCircle->setIconToken(icon);
    row2->addWidget(dashedCircle);

    auto* dashedSearch = new AdButton("Search");
    dashedSearch->setType(AdButton::Type::Dashed);
    dashedSearch->setIconToken(icon);
    row2->addWidget(dashedSearch);
    iconPlacementButtons_.append(dashedSearch);

    auto* iconOnly = new AdButton();
    iconOnly->setIconToken(icon);
    row2->addWidget(iconOnly);
    iconPlacementButtons_.append(iconOnly);

    auto* loadingButton = new AdButton("Loading");
    loadingButton->setType(AdButton::Type::Primary);
    loadingButton->setLoading(true);
    row2->addWidget(loadingButton);
    iconPlacementButtons_.append(loadingButton);

    row2->addStretch();
    layout->addLayout(row2);

    const auto applyPlacement = [this](AdButton::IconPlacement placement) {
      for (AdButton* button : iconPlacementButtons_) {
        if (button) {
          button->setIconPlacement(placement);
        }
      }
    };

    applyPlacement(static_cast<AdButton::IconPlacement>(iconPlacementBox_->currentData().toInt()));

    connect(iconPlacementBox_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index) {
              const auto placement = static_cast<AdButton::IconPlacement>(
                  iconPlacementBox_->itemData(index).toInt());
              for (AdButton* button : iconPlacementButtons_) {
                if (button) {
                  button->setIconPlacement(placement);
                }
              }
            });

    return box;
  }

  QWidget* buildSizeDemo() {
    auto* box = new QWidget();
    auto* layout = new QVBoxLayout(box);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* sizeRow = new QHBoxLayout();
    sizeRow->addWidget(new QLabel("size:"));

    auto* group = new QButtonGroup(this);
    auto* large = new QRadioButton("large");
    auto* middle = new QRadioButton("default");
    auto* small = new QRadioButton("small");
    large->setChecked(true);
    group->addButton(large, static_cast<int>(AdButton::Size::Large));
    group->addButton(middle, static_cast<int>(AdButton::Size::Middle));
    group->addButton(small, static_cast<int>(AdButton::Size::Small));
    sizeRow->addWidget(large);
    sizeRow->addWidget(middle);
    sizeRow->addWidget(small);
    sizeRow->addStretch();
    layout->addLayout(sizeRow);

    auto* line1 = new QHBoxLayout();
    sizePrimary_ = new AdButton("Primary");
    sizePrimary_->setType(AdButton::Type::Primary);
    sizeDefault_ = new AdButton("Default");
    sizeDashed_ = new AdButton("Dashed");
    sizeDashed_->setType(AdButton::Type::Dashed);
    line1->addWidget(sizePrimary_);
    line1->addWidget(sizeDefault_);
    line1->addWidget(sizeDashed_);
    line1->addStretch();
    layout->addLayout(line1);

    auto* line2 = new QHBoxLayout();
    sizeLink_ = new AdButton("Link");
    sizeLink_->setType(AdButton::Type::Link);
    line2->addWidget(sizeLink_);
    line2->addStretch();
    layout->addLayout(line2);

    auto* line3 = new QHBoxLayout();
    const auto dl = outlined_icons::Download();
    sizeIconOnly_ = new AdButton();
    sizeIconOnly_->setType(AdButton::Type::Primary);
    sizeIconOnly_->setIconToken(dl);
    sizeCircle_ = new AdButton();
    sizeCircle_->setType(AdButton::Type::Primary);
    sizeCircle_->setShape(AdButton::Shape::Circle);
    sizeCircle_->setIconToken(dl);
    sizeRoundIcon_ = new AdButton();
    sizeRoundIcon_->setType(AdButton::Type::Primary);
    sizeRoundIcon_->setShape(AdButton::Shape::Round);
    sizeRoundIcon_->setIconToken(dl);
    sizeRoundText_ = new AdButton("Download");
    sizeRoundText_->setType(AdButton::Type::Primary);
    sizeRoundText_->setShape(AdButton::Shape::Round);
    sizeRoundText_->setIconToken(dl);
    sizePlainText_ = new AdButton("Download");
    sizePlainText_->setType(AdButton::Type::Primary);
    sizePlainText_->setIconToken(dl);
    line3->addWidget(sizeIconOnly_);
    line3->addWidget(sizeCircle_);
    line3->addWidget(sizeRoundIcon_);
    line3->addWidget(sizeRoundText_);
    line3->addWidget(sizePlainText_);
    line3->addStretch();
    layout->addLayout(line3);

    connect(group, QOverload<int>::of(&QButtonGroup::idClicked), this,
            [this](int id) { applySizeToDemo(static_cast<AdButton::Size>(id)); });
    applySizeToDemo(AdButton::Size::Large);

    return box;
  }

  void applySizeToDemo(AdButton::Size size) {
    const QList<AdButton*> buttons = {
        sizePrimary_,   sizeDefault_,  sizeDashed_,   sizeLink_,      sizeIconOnly_,
        sizeCircle_,    sizeRoundIcon_, sizeRoundText_, sizePlainText_,
    };
    for (AdButton* button : buttons) {
      if (button) {
        button->setSize(size);
      }
    }
  }

  QWidget* buildDisabledDemo() {
    auto* box = new QWidget();
    auto* layout = new QVBoxLayout(box);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    auto addRow = [layout](const std::function<void(QHBoxLayout*)>& fill) {
      auto* row = new QHBoxLayout();
      fill(row);
      row->addStretch();
      layout->addLayout(row);
    };

    addRow([](QHBoxLayout* row) {
      auto* a = new AdButton("Primary");
      a->setType(AdButton::Type::Primary);
      auto* b = new AdButton("Primary(disabled)");
      b->setType(AdButton::Type::Primary);
      b->setEnabled(false);
      row->addWidget(a);
      row->addWidget(b);
    });
    addRow([](QHBoxLayout* row) {
      auto* a = new AdButton("Default");
      auto* b = new AdButton("Default(disabled)");
      b->setEnabled(false);
      row->addWidget(a);
      row->addWidget(b);
    });
    addRow([](QHBoxLayout* row) {
      auto* a = new AdButton("Dashed");
      a->setType(AdButton::Type::Dashed);
      auto* b = new AdButton("Dashed(disabled)");
      b->setType(AdButton::Type::Dashed);
      b->setEnabled(false);
      row->addWidget(a);
      row->addWidget(b);
    });
    addRow([](QHBoxLayout* row) {
      auto* a = new AdButton("Text");
      a->setType(AdButton::Type::Text);
      auto* b = new AdButton("Text(disabled)");
      b->setType(AdButton::Type::Text);
      b->setEnabled(false);
      row->addWidget(a);
      row->addWidget(b);
    });
    addRow([](QHBoxLayout* row) {
      auto* a = new AdButton("Link");
      a->setType(AdButton::Type::Link);
      auto* b = new AdButton("Link(disabled)");
      b->setType(AdButton::Type::Link);
      b->setEnabled(false);
      row->addWidget(a);
      row->addWidget(b);
    });
    addRow([](QHBoxLayout* row) {
      auto* a = new AdButton("Href Primary");
      a->setType(AdButton::Type::Primary);
      auto* b = new AdButton("Href Primary(disabled)");
      b->setType(AdButton::Type::Primary);
      b->setEnabled(false);
      row->addWidget(a);
      row->addWidget(b);
    });
    addRow([](QHBoxLayout* row) {
      auto* a = new AdButton("Danger Default");
      a->setDanger(true);
      auto* b = new AdButton("Danger Default(disabled)");
      b->setDanger(true);
      b->setEnabled(false);
      row->addWidget(a);
      row->addWidget(b);
    });
    addRow([](QHBoxLayout* row) {
      auto* a = new AdButton("Danger Text");
      a->setType(AdButton::Type::Text);
      a->setDanger(true);
      auto* b = new AdButton("Danger Text(disabled)");
      b->setType(AdButton::Type::Text);
      b->setDanger(true);
      b->setEnabled(false);
      row->addWidget(a);
      row->addWidget(b);
    });
    addRow([](QHBoxLayout* row) {
      auto* a = new AdButton("Danger Link");
      a->setType(AdButton::Type::Link);
      a->setDanger(true);
      auto* b = new AdButton("Danger Link(disabled)");
      b->setType(AdButton::Type::Link);
      b->setDanger(true);
      b->setEnabled(false);
      row->addWidget(a);
      row->addWidget(b);
    });
    addRow([](QHBoxLayout* row) {
      auto* wrap = new GhostDemoPanel();
      auto* ghostRow = new QHBoxLayout(wrap);
      ghostRow->setContentsMargins(12, 8, 12, 8);
      ghostRow->setSpacing(8);

      auto* a = new AdButton("Ghost");
      a->setGhost(true);
      auto* b = new AdButton("Ghost(disabled)");
      b->setGhost(true);
      b->setEnabled(false);

      ghostRow->addWidget(a);
      ghostRow->addWidget(b);
      ghostRow->addStretch();
      row->addWidget(wrap);
    });

    return box;
  }

  QWidget* buildLoadingDemo() {
    auto* box = new QWidget();
    auto* layout = new QVBoxLayout(box);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* row1 = new QHBoxLayout();
    auto* loading = new AdButton("Loading");
    loading->setType(AdButton::Type::Primary);
    loading->setLoading(true);
    row1->addWidget(loading);

    auto* loadingSmall = new AdButton("Loading");
    loadingSmall->setType(AdButton::Type::Primary);
    loadingSmall->setSize(AdButton::Size::Small);
    loadingSmall->setLoading(true);
    row1->addWidget(loadingSmall);

    auto* loadingIcon = new AdButton();
    loadingIcon->setType(AdButton::Type::Primary);
    loadingIcon->setIconToken(outlined_icons::Poweroff());
    loadingIcon->setLoading(true);
    row1->addWidget(loadingIcon);

    auto* loadingCustom = new AdButton("Loading Icon");
    loadingCustom->setType(AdButton::Type::Primary);
    loadingCustom->setLoadingIconToken(outlined_icons::Sync());
    loadingCustom->setLoading(true);
    row1->addWidget(loadingCustom);
    row1->addStretch();
    layout->addLayout(row1);

    auto* row2 = new QHBoxLayout();
    row2->setSpacing(8);
    auto* iconStart = new AdButton("Icon Start");
    iconStart->setType(AdButton::Type::Primary);

    auto* iconEnd = new AdButton("Icon End");
    iconEnd->setType(AdButton::Type::Primary);
    iconEnd->setIconPlacement(AdButton::IconPlacement::End);

    auto* iconReplace = new AdButton("Icon Replace");
    iconReplace->setType(AdButton::Type::Primary);
    const auto powerIcon = outlined_icons::Poweroff();
    const auto syncIcon = outlined_icons::Sync();
    iconReplace->setIconToken(powerIcon);

    auto* iconOnly = new AdButton();
    iconOnly->setType(AdButton::Type::Primary);
    iconOnly->setIconToken(powerIcon);

    auto* loadingIconButton = new AdButton("Loading Icon");
    loadingIconButton->setType(AdButton::Type::Primary);
    loadingIconButton->setIconToken(powerIcon);
    loadingIconButton->setLoadingIconToken(syncIcon);

    auto startLoadingFor = [](AdButton* button, int ms) {
      if (!button) {
        return;
      }
      button->setLoading(true);
      QTimer::singleShot(ms, button, [button]() { button->setLoading(false); });
    };

    auto startLoadingPairFor = [](AdButton* first, AdButton* second, int ms) {
      if (first) {
        first->setLoading(true);
      }
      if (second) {
        second->setLoading(true);
      }
      QTimer::singleShot(ms, first ? first : second, [first, second]() {
        if (first) {
          first->setLoading(false);
        }
        if (second) {
          second->setLoading(false);
        }
      });
    };

    connect(iconStart, &QPushButton::clicked, this, [iconStart, startLoadingFor]() {
      startLoadingFor(iconStart, 3000);
    });
    connect(iconEnd, &QPushButton::clicked, this, [iconEnd, startLoadingFor]() {
      startLoadingFor(iconEnd, 3000);
    });
    connect(iconReplace, &QPushButton::clicked, this, [iconReplace, startLoadingFor]() {
      startLoadingFor(iconReplace, 3000);
    });
    connect(iconOnly, &QPushButton::clicked, this, [iconOnly, loadingIconButton, startLoadingPairFor]() {
      startLoadingPairFor(iconOnly, loadingIconButton, 3000);
    });
    connect(loadingIconButton, &QPushButton::clicked, this,
            [iconOnly, loadingIconButton, startLoadingPairFor]() {
              startLoadingPairFor(iconOnly, loadingIconButton, 3000);
            });

    row2->addWidget(iconStart);
    row2->addWidget(iconEnd);
    row2->addWidget(iconReplace);
    row2->addWidget(iconOnly);
    row2->addWidget(loadingIconButton);
    row2->addStretch();
    layout->addLayout(row2);

    return box;
  }

  QWidget* buildGhostDemo() {
    auto* wrap = new GhostDemoPanel();
    auto* layout = new QHBoxLayout(wrap);
    layout->setContentsMargins(12, 12, 12, 12);

    auto* primary = new AdButton("Primary");
    primary->setType(AdButton::Type::Primary);
    primary->setGhost(true);
    layout->addWidget(primary);

    auto* normal = new AdButton("Default");
    normal->setGhost(true);
    layout->addWidget(normal);

    auto* dashed = new AdButton("Dashed");
    dashed->setType(AdButton::Type::Dashed);
    dashed->setGhost(true);
    layout->addWidget(dashed);

    auto* danger = new AdButton("Danger");
    danger->setType(AdButton::Type::Primary);
    danger->setDanger(true);
    danger->setGhost(true);
    layout->addWidget(danger);
    layout->addStretch();

    return wrap;
  }

  QWidget* buildDangerDemo() {
    auto* box = new QWidget();
    auto* row = new QHBoxLayout(box);
    row->setContentsMargins(0, 0, 0, 0);

    auto* primary = new AdButton("Primary");
    primary->setType(AdButton::Type::Primary);
    primary->setDanger(true);
    row->addWidget(primary);

    auto* normal = new AdButton("Default");
    normal->setDanger(true);
    row->addWidget(normal);

    auto* dashed = new AdButton("Dashed");
    dashed->setType(AdButton::Type::Dashed);
    dashed->setDanger(true);
    row->addWidget(dashed);

    auto* text = new AdButton("Text");
    text->setType(AdButton::Type::Text);
    text->setDanger(true);
    row->addWidget(text);

    auto* link = new AdButton("Link");
    link->setType(AdButton::Type::Link);
    link->setDanger(true);
    row->addWidget(link);
    row->addStretch();

    return box;
  }

  QWidget* buildBlockDemo() {
    auto* box = new QWidget();
    auto* layout = new QVBoxLayout(box);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto addButton = [layout](const QString& text, AdButton::Type type, bool disabled, bool block) {
      auto* button = new AdButton(text);
      button->setType(type);
      button->setEnabled(!disabled);
      button->setBlock(block);
      layout->addWidget(button);
    };

    addButton("Primary", AdButton::Type::Primary, false, true);
    addButton("Default", AdButton::Type::Default, false, true);
    addButton("Dashed", AdButton::Type::Dashed, false, true);
    addButton("disabled", AdButton::Type::Default, true, true);
    addButton("text", AdButton::Type::Text, false, true);
    addButton("Link", AdButton::Type::Link, false, true);

    return box;
  }

  QWidget* buildMultipleDemo() {
    auto* box = new QWidget();
    auto* layout = new QVBoxLayout(box);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto* primary = new AdButton("primary");
    primary->setType(AdButton::Type::Primary);
    layout->addWidget(primary, 0, Qt::AlignLeft);

    auto* secondary = new AdButton("secondary");
    layout->addWidget(secondary, 0, Qt::AlignLeft);

    auto* compactGroup = new AdButtonGroup();
    compactGroup->setSize(AdButton::Size::Middle);
    auto* actions = new AdButton("Actions");
    auto* more = new AdButton("...");
    compactGroup->addButton(actions);
    compactGroup->addButton(more);
    layout->addWidget(compactGroup, 0, Qt::AlignLeft);

    return box;
  }

  QVector<QWidget*> anchors_;
  QStringList titles_;

  QComboBox* iconPlacementBox_ = nullptr;
  QList<AdButton*> iconPlacementButtons_;

  AdButton* sizePrimary_ = nullptr;
  AdButton* sizeDefault_ = nullptr;
  AdButton* sizeDashed_ = nullptr;
  AdButton* sizeLink_ = nullptr;
  AdButton* sizeIconOnly_ = nullptr;
  AdButton* sizeCircle_ = nullptr;
  AdButton* sizeRoundIcon_ = nullptr;
  AdButton* sizeRoundText_ = nullptr;
  AdButton* sizePlainText_ = nullptr;
};

class DemoWindow final : public QWidget {
 public:
  DemoWindow() {
    setWindowTitle("ant-design-qt docs demo");
    resize(1320, 820);

    auto* root = new QVBoxLayout(this);

    auto* header = new QHBoxLayout();
    auto* title = new QLabel("Theme:");
    modeBox_ = new QComboBox();
    modeBox_->addItem("Default");
    modeBox_->addItem("Dark");
    modeBox_->addItem("Compact");
    modeBox_->addItem("Dark + Compact");
    loadAntdFontCheck_ = new QCheckBox("Load antd font");
    loadAntdFontCheck_->setChecked(false);
    header->addWidget(title);
    header->addWidget(modeBox_);
    header->addWidget(loadAntdFontCheck_);
    header->addStretch();
    root->addLayout(header);

    auto* body = new QHBoxLayout();
    navMenu_ = new AdMenu();
    navMenu_->setFixedWidth(256);
    navMenu_->setInlineIndent(24);
    navMenu_->setMode(AdMenu::Mode::Inline);
    body->addWidget(navMenu_);

    scroll_ = new QScrollArea();
    scroll_->setWidgetResizable(true);
    docsStack_ = new QStackedWidget();
    buttonPage_ = new ButtonDocsPage();
    menuPage_ = new MenuDocsPage();
    selectPage_ = new SelectDocsPage();
    docsStack_->addWidget(buttonPage_);
    docsStack_->addWidget(menuPage_);
    docsStack_->addWidget(selectPage_);
    scroll_->setWidget(docsStack_);
    body->addWidget(scroll_, 1);

    root->addLayout(body, 1);

    rebuildNavMenuItems();

    connect(navMenu_, &AdMenu::clicked, this,
            [this](const QString& key, const QStringList&) { handleNavClick(key); });
    connect(navMenu_, &AdMenu::titleClicked, this, [this](const QString& key) {
      if (key == QStringLiteral("button-docs")) {
        switchDocs(DocsKind::Button, false);
      } else if (key == QStringLiteral("menu-docs")) {
        switchDocs(DocsKind::Menu, false);
      } else if (key == QStringLiteral("select-docs")) {
        switchDocs(DocsKind::Select, false);
      }
    });
    connect(scroll_->verticalScrollBar(), &QAbstractSlider::valueChanged, this,
            [this](int value) { syncNavToScroll(value); });

    connect(modeBox_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int) { applyThemeFromControls(); });
    connect(loadAntdFontCheck_, &QCheckBox::toggled, this, [this](bool) { applyThemeFromControls(); });

    switchDocs(DocsKind::Button, true);
    navMenu_->setSelectedKey(sectionKey(DocsKind::Button, 0));
    applyThemeFromControls();
  }

 private:
  enum class DocsKind {
    Button,
    Menu,
    Select,
  };

  QString docsRootKey(DocsKind kind) const {
    if (kind == DocsKind::Button) {
      return QStringLiteral("button-docs");
    }
    if (kind == DocsKind::Menu) {
      return QStringLiteral("menu-docs");
    }
    return QStringLiteral("select-docs");
  }

  QString sectionKey(DocsKind kind, int row) const {
    QString prefix;
    if (kind == DocsKind::Button) {
      prefix = QStringLiteral("button");
    } else if (kind == DocsKind::Menu) {
      prefix = QStringLiteral("menu");
    } else {
      prefix = QStringLiteral("select");
    }
    return QStringLiteral("%1-section-%2")
        .arg(prefix)
        .arg(row);
  }

  bool parseSectionKey(const QString& key, DocsKind* kind, int* row) const {
    if (key.startsWith(QStringLiteral("button-section-"))) {
      bool ok = false;
      const int value = key.mid(QStringLiteral("button-section-").size()).toInt(&ok);
      if (ok) {
        if (kind) {
          *kind = DocsKind::Button;
        }
        if (row) {
          *row = value;
        }
        return true;
      }
      return false;
    }
    if (key.startsWith(QStringLiteral("menu-section-"))) {
      bool ok = false;
      const int value = key.mid(QStringLiteral("menu-section-").size()).toInt(&ok);
      if (ok) {
        if (kind) {
          *kind = DocsKind::Menu;
        }
        if (row) {
          *row = value;
        }
        return true;
      }
      return false;
    }
    if (key.startsWith(QStringLiteral("select-section-"))) {
      bool ok = false;
      const int value = key.mid(QStringLiteral("select-section-").size()).toInt(&ok);
      if (ok) {
        if (kind) {
          *kind = DocsKind::Select;
        }
        if (row) {
          *row = value;
        }
        return true;
      }
      return false;
    }
    return false;
  }

  const QVector<QWidget*>& anchorsFor(DocsKind kind) const {
    static const QVector<QWidget*> kEmpty;
    if (kind == DocsKind::Button && buttonPage_) {
      return buttonPage_->sectionAnchors();
    }
    if (kind == DocsKind::Menu && menuPage_) {
      return menuPage_->sectionAnchors();
    }
    if (kind == DocsKind::Select && selectPage_) {
      return selectPage_->sectionAnchors();
    }
    return kEmpty;
  }

  const QStringList& titlesFor(DocsKind kind) const {
    static const QStringList kEmpty;
    if (kind == DocsKind::Button && buttonPage_) {
      return buttonPage_->sectionTitles();
    }
    if (kind == DocsKind::Menu && menuPage_) {
      return menuPage_->sectionTitles();
    }
    if (kind == DocsKind::Select && selectPage_) {
      return selectPage_->sectionTitles();
    }
    return kEmpty;
  }

  void rebuildNavMenuItems() {
    if (!navMenu_) {
      return;
    }

    auto buildSectionItems = [this](DocsKind kind) {
      QVector<AdMenu::Item> sectionItems;
      const QStringList& titles = titlesFor(kind);
      sectionItems.reserve(titles.size());
      const auto sectionIcon = outlined_icons::Right();
      for (int i = 0; i < titles.size(); ++i) {
        AdMenu::Item item;
        item.key = sectionKey(kind, i);
        item.label = titles.at(i);
        item.icon = sectionIcon;
        sectionItems.append(item);
      }
      return sectionItems;
    };

    AdMenu::Item buttonRoot;
    buttonRoot.key = docsRootKey(DocsKind::Button);
    buttonRoot.label = "Button";
    buttonRoot.type = AdMenu::ItemType::SubMenu;
    buttonRoot.icon = outlined_icons::Appstore();
    buttonRoot.children = buildSectionItems(DocsKind::Button);

    AdMenu::Item menuRoot;
    menuRoot.key = docsRootKey(DocsKind::Menu);
    menuRoot.label = "Menu";
    menuRoot.type = AdMenu::ItemType::SubMenu;
    menuRoot.icon = outlined_icons::Menu();
    menuRoot.children = buildSectionItems(DocsKind::Menu);

    AdMenu::Item selectRoot;
    selectRoot.key = docsRootKey(DocsKind::Select);
    selectRoot.label = "Select";
    selectRoot.type = AdMenu::ItemType::SubMenu;
    selectRoot.icon = outlined_icons::Select();
    selectRoot.children = buildSectionItems(DocsKind::Select);

    navMenu_->setItems({buttonRoot, menuRoot, selectRoot});
    navMenu_->setOpenKeys(
        {docsRootKey(DocsKind::Button), docsRootKey(DocsKind::Menu), docsRootKey(DocsKind::Select)});
  }

  void applyThemeFromControls() {
    ThemeConfig config = configForIndex(modeBox_ ? modeBox_->currentIndex() : 0);
    if (loadAntdFontCheck_) {
      config.loadAntdFont = loadAntdFontCheck_->isChecked();
    }
    ThemeManager::instance().setTheme(config);

    if (navMenu_) {
      const bool dark = config.algorithms.contains(ThemeAlgorithm::Dark);
      navMenu_->setTheme(dark ? AdMenu::MenuTheme::Dark : AdMenu::MenuTheme::Light);
    }
  }

  void switchDocs(DocsKind kind, bool preserveScroll) {
    currentKind_ = kind;
    if (docsStack_) {
      QWidget* target = nullptr;
      if (kind == DocsKind::Button) {
        target = buttonPage_;
      } else if (kind == DocsKind::Menu) {
        target = menuPage_;
      } else {
        target = selectPage_;
      }
      docsStack_->setCurrentWidget(target);
    }
    if (!preserveScroll && scroll_) {
      scroll_->verticalScrollBar()->setValue(0);
    }
    if (scroll_) {
      syncNavToScroll(scroll_->verticalScrollBar()->value());
    }
  }

  void handleNavClick(const QString& key) {
    DocsKind kind = DocsKind::Button;
    int row = -1;
    if (!parseSectionKey(key, &kind, &row)) {
      return;
    }
    switchDocs(kind, true);
    scrollToSection(kind, row);
  }

  void scrollToSection(DocsKind kind, int row) {
    const QVector<QWidget*>& anchors = anchorsFor(kind);
    if (!scroll_ || row < 0 || row >= anchors.size()) {
      return;
    }

    QWidget* target = anchors.at(row);
    if (!target) {
      return;
    }

    scroll_->ensureWidgetVisible(target, 24, 24);
    if (navMenu_) {
      navMenu_->setSelectedKey(sectionKey(kind, row));
    }
  }

  void syncNavToScroll(int scrollValue) {
    const QVector<QWidget*>& anchors = anchorsFor(currentKind_);
    if (anchors.isEmpty() || !navMenu_) {
      return;
    }

    const int probeY = scrollValue + 28;
    int best = 0;
    for (int i = 0; i < anchors.size(); ++i) {
      QWidget* anchor = anchors.at(i);
      if (!anchor) {
        continue;
      }
      if (anchor->y() <= probeY) {
        best = i;
      } else {
        break;
      }
    }

    const QString key = sectionKey(currentKind_, best);
    if (navMenu_->selectedKey() != key) {
      navMenu_->setSelectedKey(key);
    }
  }

  QComboBox* modeBox_ = nullptr;
  QCheckBox* loadAntdFontCheck_ = nullptr;
  AdMenu* navMenu_ = nullptr;
  QScrollArea* scroll_ = nullptr;
  QStackedWidget* docsStack_ = nullptr;
  ButtonDocsPage* buttonPage_ = nullptr;
  MenuDocsPage* menuPage_ = nullptr;
  SelectDocsPage* selectPage_ = nullptr;
  DocsKind currentKind_ = DocsKind::Button;
};

}  // namespace

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);

  if (QStyleFactory::keys().contains("Fusion", Qt::CaseInsensitive)) {
    app.setStyle(QStyleFactory::create("Fusion"));
  }

  ThemeManager::instance().applyTo(app);
  demo::installIconThemeResolver();

  DemoWindow window;
  window.show();

  return app.exec();
}
