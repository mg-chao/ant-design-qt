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
#include <QVBoxLayout>
#include <QWidget>

#include <functional>

#include "icon_theme_adapter.h"
#include "icons.h"
#include "alert_docs_page.h"
#include "color_picker_docs_page.h"
#include "image_docs_page.h"
#include "input_docs_page.h"
#include "input_number_docs_page.h"
#include "menu_docs_page.h"
#include "modal_docs_page.h"
#include "popconfirm_docs_page.h"
#include "popover_docs_page.h"
#include "radio_docs_page.h"
#include "select_docs_page.h"
#include "slider_docs_page.h"
#include "switch_docs_page.h"
#include "tooltip_docs_page.h"
#include "theme/theme.h"
#include "widgets/detail/timing_hub.h"
#include "widgets/widgets.h"

using adqt::theme::ThemeAlgorithm;
using adqt::theme::ThemeConfig;
using adqt::theme::ThemeManager;
using adqt::widgets::AdButton;
using adqt::widgets::AdButtonGroup;
using adqt::widgets::AdMenu;
using adqt::widgets::AdScrollArea;
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
      adqt::widgets::detail::scheduleTimingTask(
          button, QStringLiteral("ThemeDemo.ButtonLoading"), ms, [button]() {
            if (button) {
              button->setLoading(false);
            }
          });
    };

    auto startLoadingPairFor = [](AdButton* first, AdButton* second, int ms) {
      if (first) {
        first->setLoading(true);
      }
      if (second) {
        second->setLoading(true);
      }
      QObject* owner = first ? static_cast<QObject*>(first) : static_cast<QObject*>(second);
      if (!owner) {
        return;
      }
      adqt::widgets::detail::scheduleTimingTask(
          owner, QStringLiteral("ThemeDemo.ButtonPairLoading"), ms, [first, second]() {
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
    navMenu_->setInlineIndent(24);
    navMenu_->setMode(AdMenu::Mode::Inline);
    navScroll_ = new AdScrollArea();
    navScroll_->setFixedWidth(256);
    navScroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    navScroll_->setContentWidget(navMenu_);
    body->addWidget(navScroll_);

    scroll_ = new QScrollArea();
    scroll_->setWidgetResizable(true);
    docsStack_ = new QStackedWidget();
    buttonPage_ = new ButtonDocsPage();
    alertPage_ = new AlertDocsPage();
    inputPage_ = new InputDocsPage();
    inputNumberPage_ = new InputNumberDocsPage();
    switchPage_ = new SwitchDocsPage();
    menuPage_ = new MenuDocsPage();
    modalPage_ = new ModalDocsPage();
    selectPage_ = new SelectDocsPage();
    sliderPage_ = new SliderDocsPage();
    colorPickerPage_ = new ColorPickerDocsPage();
    imagePage_ = new ImageDocsPage();
    popoverPage_ = new PopoverDocsPage();
    popconfirmPage_ = new PopconfirmDocsPage();
    tooltipPage_ = new TooltipDocsPage();
    radioPage_ = new RadioDocsPage();
    docsStack_->addWidget(buttonPage_);
    docsStack_->addWidget(alertPage_);
    docsStack_->addWidget(inputPage_);
    docsStack_->addWidget(inputNumberPage_);
    docsStack_->addWidget(switchPage_);
    docsStack_->addWidget(menuPage_);
    docsStack_->addWidget(modalPage_);
    docsStack_->addWidget(selectPage_);
    docsStack_->addWidget(sliderPage_);
    docsStack_->addWidget(colorPickerPage_);
    docsStack_->addWidget(imagePage_);
    docsStack_->addWidget(popoverPage_);
    docsStack_->addWidget(popconfirmPage_);
    docsStack_->addWidget(tooltipPage_);
    docsStack_->addWidget(radioPage_);
    scroll_->setWidget(docsStack_);
    body->addWidget(scroll_, 1);

    root->addLayout(body, 1);

    rebuildNavMenuItems();

    connect(navMenu_, &AdMenu::clicked, this,
            [this](const QString& key, const QStringList&) { handleNavClick(key); });
    connect(navMenu_, &AdMenu::titleClicked, this, [this](const QString& key) {
      if (key == QStringLiteral("button-docs")) {
        switchDocs(DocsKind::Button, false);
      } else if (key == QStringLiteral("alert-docs")) {
        switchDocs(DocsKind::Alert, false);
      } else if (key == QStringLiteral("input-docs")) {
        switchDocs(DocsKind::Input, false);
      } else if (key == QStringLiteral("inputnumber-docs")) {
        switchDocs(DocsKind::InputNumber, false);
      } else if (key == QStringLiteral("switch-docs")) {
        switchDocs(DocsKind::Switch, false);
      } else if (key == QStringLiteral("menu-docs")) {
        switchDocs(DocsKind::Menu, false);
      } else if (key == QStringLiteral("modal-docs")) {
        switchDocs(DocsKind::Modal, false);
      } else if (key == QStringLiteral("select-docs")) {
        switchDocs(DocsKind::Select, false);
      } else if (key == QStringLiteral("slider-docs")) {
        switchDocs(DocsKind::Slider, false);
      } else if (key == QStringLiteral("colorpicker-docs")) {
        switchDocs(DocsKind::ColorPicker, false);
      } else if (key == QStringLiteral("image-docs")) {
        switchDocs(DocsKind::Image, false);
      } else if (key == QStringLiteral("popover-docs")) {
        switchDocs(DocsKind::Popover, false);
      } else if (key == QStringLiteral("popconfirm-docs")) {
        switchDocs(DocsKind::Popconfirm, false);
      } else if (key == QStringLiteral("tooltip-docs")) {
        switchDocs(DocsKind::Tooltip, false);
      } else if (key == QStringLiteral("radio-docs")) {
        switchDocs(DocsKind::Radio, false);
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
    Alert,
    Input,
    InputNumber,
    Switch,
    Menu,
    Modal,
    Select,
    Slider,
    ColorPicker,
    Image,
    Popover,
    Popconfirm,
    Tooltip,
    Radio,
  };

  QString docsRootKey(DocsKind kind) const {
    if (kind == DocsKind::Button) {
      return QStringLiteral("button-docs");
    }
    if (kind == DocsKind::Alert) {
      return QStringLiteral("alert-docs");
    }
    if (kind == DocsKind::Input) {
      return QStringLiteral("input-docs");
    }
    if (kind == DocsKind::InputNumber) {
      return QStringLiteral("inputnumber-docs");
    }
    if (kind == DocsKind::Switch) {
      return QStringLiteral("switch-docs");
    }
    if (kind == DocsKind::Menu) {
      return QStringLiteral("menu-docs");
    }
    if (kind == DocsKind::Modal) {
      return QStringLiteral("modal-docs");
    }
    if (kind == DocsKind::Select) {
      return QStringLiteral("select-docs");
    }
    if (kind == DocsKind::Slider) {
      return QStringLiteral("slider-docs");
    }
    if (kind == DocsKind::ColorPicker) {
      return QStringLiteral("colorpicker-docs");
    }
    if (kind == DocsKind::Image) {
      return QStringLiteral("image-docs");
    }
    if (kind == DocsKind::Popover) {
      return QStringLiteral("popover-docs");
    }
    if (kind == DocsKind::Popconfirm) {
      return QStringLiteral("popconfirm-docs");
    }
    if (kind == DocsKind::Tooltip) {
      return QStringLiteral("tooltip-docs");
    }
    return QStringLiteral("radio-docs");
  }

  QString sectionKey(DocsKind kind, int row) const {
    QString prefix;
    if (kind == DocsKind::Button) {
      prefix = QStringLiteral("button");
    } else if (kind == DocsKind::Alert) {
      prefix = QStringLiteral("alert");
    } else if (kind == DocsKind::Input) {
      prefix = QStringLiteral("input");
    } else if (kind == DocsKind::InputNumber) {
      prefix = QStringLiteral("inputnumber");
    } else if (kind == DocsKind::Switch) {
      prefix = QStringLiteral("switch");
    } else if (kind == DocsKind::Menu) {
      prefix = QStringLiteral("menu");
    } else if (kind == DocsKind::Modal) {
      prefix = QStringLiteral("modal");
    } else if (kind == DocsKind::Select) {
      prefix = QStringLiteral("select");
    } else if (kind == DocsKind::Slider) {
      prefix = QStringLiteral("slider");
    } else if (kind == DocsKind::ColorPicker) {
      prefix = QStringLiteral("colorpicker");
    } else if (kind == DocsKind::Image) {
      prefix = QStringLiteral("image");
    } else if (kind == DocsKind::Popover) {
      prefix = QStringLiteral("popover");
    } else if (kind == DocsKind::Popconfirm) {
      prefix = QStringLiteral("popconfirm");
    } else if (kind == DocsKind::Tooltip) {
      prefix = QStringLiteral("tooltip");
    } else {
      prefix = QStringLiteral("radio");
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
    if (key.startsWith(QStringLiteral("alert-section-"))) {
      bool ok = false;
      const int value = key.mid(QStringLiteral("alert-section-").size()).toInt(&ok);
      if (ok) {
        if (kind) {
          *kind = DocsKind::Alert;
        }
        if (row) {
          *row = value;
        }
        return true;
      }
      return false;
    }
    if (key.startsWith(QStringLiteral("input-section-"))) {
      bool ok = false;
      const int value = key.mid(QStringLiteral("input-section-").size()).toInt(&ok);
      if (ok) {
        if (kind) {
          *kind = DocsKind::Input;
        }
        if (row) {
          *row = value;
        }
        return true;
      }
      return false;
    }
    if (key.startsWith(QStringLiteral("inputnumber-section-"))) {
      bool ok = false;
      const int value = key.mid(QStringLiteral("inputnumber-section-").size()).toInt(&ok);
      if (ok) {
        if (kind) {
          *kind = DocsKind::InputNumber;
        }
        if (row) {
          *row = value;
        }
        return true;
      }
      return false;
    }
    if (key.startsWith(QStringLiteral("switch-section-"))) {
      bool ok = false;
      const int value = key.mid(QStringLiteral("switch-section-").size()).toInt(&ok);
      if (ok) {
        if (kind) {
          *kind = DocsKind::Switch;
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
    if (key.startsWith(QStringLiteral("modal-section-"))) {
      bool ok = false;
      const int value = key.mid(QStringLiteral("modal-section-").size()).toInt(&ok);
      if (ok) {
        if (kind) {
          *kind = DocsKind::Modal;
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
    if (key.startsWith(QStringLiteral("slider-section-"))) {
      bool ok = false;
      const int value = key.mid(QStringLiteral("slider-section-").size()).toInt(&ok);
      if (ok) {
        if (kind) {
          *kind = DocsKind::Slider;
        }
        if (row) {
          *row = value;
        }
        return true;
      }
      return false;
    }
    if (key.startsWith(QStringLiteral("colorpicker-section-"))) {
      bool ok = false;
      const int value = key.mid(QStringLiteral("colorpicker-section-").size()).toInt(&ok);
      if (ok) {
        if (kind) {
          *kind = DocsKind::ColorPicker;
        }
        if (row) {
          *row = value;
        }
        return true;
      }
      return false;
    }
    if (key.startsWith(QStringLiteral("image-section-"))) {
      bool ok = false;
      const int value = key.mid(QStringLiteral("image-section-").size()).toInt(&ok);
      if (ok) {
        if (kind) {
          *kind = DocsKind::Image;
        }
        if (row) {
          *row = value;
        }
        return true;
      }
      return false;
    }
    if (key.startsWith(QStringLiteral("popover-section-"))) {
      bool ok = false;
      const int value = key.mid(QStringLiteral("popover-section-").size()).toInt(&ok);
      if (ok) {
        if (kind) {
          *kind = DocsKind::Popover;
        }
        if (row) {
          *row = value;
        }
        return true;
      }
      return false;
    }
    if (key.startsWith(QStringLiteral("popconfirm-section-"))) {
      bool ok = false;
      const int value = key.mid(QStringLiteral("popconfirm-section-").size()).toInt(&ok);
      if (ok) {
        if (kind) {
          *kind = DocsKind::Popconfirm;
        }
        if (row) {
          *row = value;
        }
        return true;
      }
      return false;
    }
    if (key.startsWith(QStringLiteral("tooltip-section-"))) {
      bool ok = false;
      const int value = key.mid(QStringLiteral("tooltip-section-").size()).toInt(&ok);
      if (ok) {
        if (kind) {
          *kind = DocsKind::Tooltip;
        }
        if (row) {
          *row = value;
        }
        return true;
      }
      return false;
    }
    if (key.startsWith(QStringLiteral("radio-section-"))) {
      bool ok = false;
      const int value = key.mid(QStringLiteral("radio-section-").size()).toInt(&ok);
      if (ok) {
        if (kind) {
          *kind = DocsKind::Radio;
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
    if (kind == DocsKind::Alert && alertPage_) {
      return alertPage_->sectionAnchors();
    }
    if (kind == DocsKind::Input && inputPage_) {
      return inputPage_->sectionAnchors();
    }
    if (kind == DocsKind::InputNumber && inputNumberPage_) {
      return inputNumberPage_->sectionAnchors();
    }
    if (kind == DocsKind::Switch && switchPage_) {
      return switchPage_->sectionAnchors();
    }
    if (kind == DocsKind::Menu && menuPage_) {
      return menuPage_->sectionAnchors();
    }
    if (kind == DocsKind::Modal && modalPage_) {
      return modalPage_->sectionAnchors();
    }
    if (kind == DocsKind::Select && selectPage_) {
      return selectPage_->sectionAnchors();
    }
    if (kind == DocsKind::Slider && sliderPage_) {
      return sliderPage_->sectionAnchors();
    }
    if (kind == DocsKind::ColorPicker && colorPickerPage_) {
      return colorPickerPage_->sectionAnchors();
    }
    if (kind == DocsKind::Image && imagePage_) {
      return imagePage_->sectionAnchors();
    }
    if (kind == DocsKind::Popover && popoverPage_) {
      return popoverPage_->sectionAnchors();
    }
    if (kind == DocsKind::Popconfirm && popconfirmPage_) {
      return popconfirmPage_->sectionAnchors();
    }
    if (kind == DocsKind::Tooltip && tooltipPage_) {
      return tooltipPage_->sectionAnchors();
    }
    if (kind == DocsKind::Radio && radioPage_) {
      return radioPage_->sectionAnchors();
    }
    return kEmpty;
  }

  const QStringList& titlesFor(DocsKind kind) const {
    static const QStringList kEmpty;
    if (kind == DocsKind::Button && buttonPage_) {
      return buttonPage_->sectionTitles();
    }
    if (kind == DocsKind::Alert && alertPage_) {
      return alertPage_->sectionTitles();
    }
    if (kind == DocsKind::Input && inputPage_) {
      return inputPage_->sectionTitles();
    }
    if (kind == DocsKind::InputNumber && inputNumberPage_) {
      return inputNumberPage_->sectionTitles();
    }
    if (kind == DocsKind::Switch && switchPage_) {
      return switchPage_->sectionTitles();
    }
    if (kind == DocsKind::Menu && menuPage_) {
      return menuPage_->sectionTitles();
    }
    if (kind == DocsKind::Modal && modalPage_) {
      return modalPage_->sectionTitles();
    }
    if (kind == DocsKind::Select && selectPage_) {
      return selectPage_->sectionTitles();
    }
    if (kind == DocsKind::Slider && sliderPage_) {
      return sliderPage_->sectionTitles();
    }
    if (kind == DocsKind::ColorPicker && colorPickerPage_) {
      return colorPickerPage_->sectionTitles();
    }
    if (kind == DocsKind::Image && imagePage_) {
      return imagePage_->sectionTitles();
    }
    if (kind == DocsKind::Popover && popoverPage_) {
      return popoverPage_->sectionTitles();
    }
    if (kind == DocsKind::Popconfirm && popconfirmPage_) {
      return popconfirmPage_->sectionTitles();
    }
    if (kind == DocsKind::Tooltip && tooltipPage_) {
      return tooltipPage_->sectionTitles();
    }
    if (kind == DocsKind::Radio && radioPage_) {
      return radioPage_->sectionTitles();
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

    AdMenu::Item alertRoot;
    alertRoot.key = docsRootKey(DocsKind::Alert);
    alertRoot.label = "Alert";
    alertRoot.type = AdMenu::ItemType::SubMenu;
    alertRoot.icon = outlined_icons::Alert();
    alertRoot.children = buildSectionItems(DocsKind::Alert);

    AdMenu::Item inputRoot;
    inputRoot.key = docsRootKey(DocsKind::Input);
    inputRoot.label = "Input";
    inputRoot.type = AdMenu::ItemType::SubMenu;
    inputRoot.icon = outlined_icons::Edit();
    inputRoot.children = buildSectionItems(DocsKind::Input);

    AdMenu::Item inputNumberRoot;
    inputNumberRoot.key = docsRootKey(DocsKind::InputNumber);
    inputNumberRoot.label = "InputNumber";
    inputNumberRoot.type = AdMenu::ItemType::SubMenu;
    inputNumberRoot.icon = outlined_icons::Number();
    inputNumberRoot.children = buildSectionItems(DocsKind::InputNumber);

    AdMenu::Item switchRoot;
    switchRoot.key = docsRootKey(DocsKind::Switch);
    switchRoot.label = "Switch";
    switchRoot.type = AdMenu::ItemType::SubMenu;
    switchRoot.icon = outlined_icons::Switcher();
    switchRoot.children = buildSectionItems(DocsKind::Switch);

    AdMenu::Item menuRoot;
    menuRoot.key = docsRootKey(DocsKind::Menu);
    menuRoot.label = "Menu";
    menuRoot.type = AdMenu::ItemType::SubMenu;
    menuRoot.icon = outlined_icons::Menu();
    menuRoot.children = buildSectionItems(DocsKind::Menu);

    AdMenu::Item modalRoot;
    modalRoot.key = docsRootKey(DocsKind::Modal);
    modalRoot.label = "Modal";
    modalRoot.type = AdMenu::ItemType::SubMenu;
    modalRoot.icon = outlined_icons::Appstore();
    modalRoot.children = buildSectionItems(DocsKind::Modal);

    AdMenu::Item selectRoot;
    selectRoot.key = docsRootKey(DocsKind::Select);
    selectRoot.label = "Select";
    selectRoot.type = AdMenu::ItemType::SubMenu;
    selectRoot.icon = outlined_icons::Select();
    selectRoot.children = buildSectionItems(DocsKind::Select);

    AdMenu::Item sliderRoot;
    sliderRoot.key = docsRootKey(DocsKind::Slider);
    sliderRoot.label = "Slider";
    sliderRoot.type = AdMenu::ItemType::SubMenu;
    sliderRoot.icon = outlined_icons::Sliders();
    sliderRoot.children = buildSectionItems(DocsKind::Slider);

    AdMenu::Item colorPickerRoot;
    colorPickerRoot.key = docsRootKey(DocsKind::ColorPicker);
    colorPickerRoot.label = "ColorPicker";
    colorPickerRoot.type = AdMenu::ItemType::SubMenu;
    colorPickerRoot.icon = outlined_icons::BgColors();
    colorPickerRoot.children = buildSectionItems(DocsKind::ColorPicker);

    AdMenu::Item imageRoot;
    imageRoot.key = docsRootKey(DocsKind::Image);
    imageRoot.label = "Image";
    imageRoot.type = AdMenu::ItemType::SubMenu;
    imageRoot.icon = outlined_icons::Picture();
    imageRoot.children = buildSectionItems(DocsKind::Image);

    AdMenu::Item popoverRoot;
    popoverRoot.key = docsRootKey(DocsKind::Popover);
    popoverRoot.label = "Popover";
    popoverRoot.type = AdMenu::ItemType::SubMenu;
    popoverRoot.icon = outlined_icons::Message();
    popoverRoot.children = buildSectionItems(DocsKind::Popover);

    AdMenu::Item popconfirmRoot;
    popconfirmRoot.key = docsRootKey(DocsKind::Popconfirm);
    popconfirmRoot.label = "Popconfirm";
    popconfirmRoot.type = AdMenu::ItemType::SubMenu;
    popconfirmRoot.icon = outlined_icons::ExclamationCircle();
    popconfirmRoot.children = buildSectionItems(DocsKind::Popconfirm);

    AdMenu::Item tooltipRoot;
    tooltipRoot.key = docsRootKey(DocsKind::Tooltip);
    tooltipRoot.label = "Tooltip";
    tooltipRoot.type = AdMenu::ItemType::SubMenu;
    tooltipRoot.icon = outlined_icons::QuestionCircle();
    tooltipRoot.children = buildSectionItems(DocsKind::Tooltip);

    AdMenu::Item radioRoot;
    radioRoot.key = docsRootKey(DocsKind::Radio);
    radioRoot.label = "Radio";
    radioRoot.type = AdMenu::ItemType::SubMenu;
    radioRoot.icon = outlined_icons::Appstore();
    radioRoot.children = buildSectionItems(DocsKind::Radio);

    navMenu_->setItems({buttonRoot, alertRoot, inputRoot, inputNumberRoot, switchRoot, menuRoot, modalRoot,
                        selectRoot, sliderRoot, colorPickerRoot, imageRoot, popoverRoot, popconfirmRoot,
                        tooltipRoot, radioRoot});
    navMenu_->setOpenKeys({});
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
      } else if (kind == DocsKind::Alert) {
        target = alertPage_;
      } else if (kind == DocsKind::Input) {
        target = inputPage_;
      } else if (kind == DocsKind::InputNumber) {
        target = inputNumberPage_;
      } else if (kind == DocsKind::Switch) {
        target = switchPage_;
      } else if (kind == DocsKind::Menu) {
        target = menuPage_;
      } else if (kind == DocsKind::Modal) {
        target = modalPage_;
      } else if (kind == DocsKind::Select) {
        target = selectPage_;
      } else if (kind == DocsKind::Slider) {
        target = sliderPage_;
      } else if (kind == DocsKind::ColorPicker) {
        target = colorPickerPage_;
      } else if (kind == DocsKind::Image) {
        target = imagePage_;
      } else if (kind == DocsKind::Popover) {
        target = popoverPage_;
      } else if (kind == DocsKind::Popconfirm) {
        target = popconfirmPage_;
      } else if (kind == DocsKind::Tooltip) {
        target = tooltipPage_;
      } else {
        target = radioPage_;
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
  AdScrollArea* navScroll_ = nullptr;
  QScrollArea* scroll_ = nullptr;
  QStackedWidget* docsStack_ = nullptr;
  ButtonDocsPage* buttonPage_ = nullptr;
  AlertDocsPage* alertPage_ = nullptr;
  InputDocsPage* inputPage_ = nullptr;
  InputNumberDocsPage* inputNumberPage_ = nullptr;
  SwitchDocsPage* switchPage_ = nullptr;
  MenuDocsPage* menuPage_ = nullptr;
  ModalDocsPage* modalPage_ = nullptr;
  SelectDocsPage* selectPage_ = nullptr;
  SliderDocsPage* sliderPage_ = nullptr;
  ColorPickerDocsPage* colorPickerPage_ = nullptr;
  ImageDocsPage* imagePage_ = nullptr;
  PopoverDocsPage* popoverPage_ = nullptr;
  PopconfirmDocsPage* popconfirmPage_ = nullptr;
  TooltipDocsPage* tooltipPage_ = nullptr;
  RadioDocsPage* radioPage_ = nullptr;
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
