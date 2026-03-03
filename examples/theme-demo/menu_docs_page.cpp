#include "menu_docs_page.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMap>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>

#include <memory>

#include "icons.h"

using adqt::widgets::AdMenu;
namespace outlined_icons = adqt::icons::outlined;

namespace {

AdMenu::Item makeLeaf(const QString& key,
                      const QString& label,
                      const adqt::icons::IconToken& icon = {},
                      bool disabled = false) {
  AdMenu::Item item;
  item.key = key;
  item.label = label;
  item.icon = icon;
  item.type = AdMenu::ItemType::Item;
  item.disabled = disabled;
  return item;
}

AdMenu::Item makeSubMenu(const QString& key,
                         const QString& label,
                         const QVector<AdMenu::Item>& children,
                         const adqt::icons::IconToken& icon = {}) {
  AdMenu::Item item;
  item.key = key;
  item.label = label;
  item.icon = icon;
  item.type = AdMenu::ItemType::SubMenu;
  item.children = children;
  return item;
}

AdMenu::Item makeGroup(const QString& key,
                       const QString& label,
                       const QVector<AdMenu::Item>& children) {
  AdMenu::Item item;
  item.key = key;
  item.label = label;
  item.type = AdMenu::ItemType::Group;
  item.children = children;
  return item;
}

AdMenu::Item makeDivider(const QString& key, bool dashed = false) {
  AdMenu::Item item;
  item.key = key;
  item.type = AdMenu::ItemType::Divider;
  item.dashed = dashed;
  return item;
}

QLabel* makeHintLabel(const QString& text, QWidget* parent = nullptr) {
  auto* label = new QLabel(text, parent);
  label->setWordWrap(true);
  label->setStyleSheet("color: #8c8c8c;");
  return label;
}

}  // namespace

MenuDocsPage::MenuDocsPage(QWidget* parent) : QWidget(parent) {
  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(16, 16, 16, 24);
  root->setSpacing(16);

  auto* title = new QLabel("Menu Navigation");
  QFont titleFont = title->font();
  titleFont.setPointSize(titleFont.pointSize() + 8);
  titleFont.setBold(true);
  title->setFont(titleFont);
  root->addWidget(title);

  auto* subtitle = new QLabel(
      "This page mirrors the full set of Ant Design Menu examples, covering modes, themes, styling, semantic features, and debug extensions.");
  subtitle->setWordWrap(true);
  root->addWidget(subtitle);

  addSection(root, "Top Navigation", "Demo: horizontal.tsx", buildHorizontalDemo(false));
  addSection(root, "Top Navigation (Dark)", "Demo: horizontal-dark.tsx", buildHorizontalDemo(true));
  addSection(root, "Inline Menu", "Demo: inline.tsx", buildInlineDemo());
  addSection(root, "Collapsed Inline Menu", "Demo: inline-collapsed.tsx", buildInlineCollapsedDemo());
  addSection(root, "Collapsed Menu Tooltip", "Demo: tooltip.tsx", buildTooltipDemo());
  addSection(root, "Open Current Submenu Only", "Demo: sider-current.tsx", buildSiderCurrentDemo());
  addSection(root, "Vertical Popup Menu", "Demo: vertical.tsx", buildVerticalDemo());
  addSection(root, "Theme Switch", "Demo: theme.tsx", buildThemeDemo());
  addSection(root, "Submenu Theme", "Demo: submenu-theme.tsx", buildSubMenuThemeDemo());
  addSection(root, "Dynamic Mode Switch", "Demo: switch-mode.tsx", buildSwitchModeDemo());
  addSection(root, "Semantic Styling (styles/classNames)", "Demo: style-class.tsx", buildStyleClassDemo());
  addSection(root, "Style Debugging", "Demo: style-debug.tsx", buildStyleDebugDemo());
  addSection(root, "Menu v4 Style", "Demo: menu-v4.tsx", buildMenuV4Demo());
  addSection(root, "Component Token", "Demo: component-token.tsx", buildComponentTokenDemo());
  addSection(root, "Extra Content / Danger Item / Divider", "Demo: extra-style.tsx", buildExtraStyleDemo());
  addSection(root, "Custom Popup Render", "Demo: custom-popup-render.tsx", buildCustomPopupRenderDemo());
  addSection(root, "Semantic DOM Comparison", "Demo: _semantic.tsx", buildSemanticDemo());
  addSection(root, "API Overview", "Core API names align with Ant Design Menu.", buildApiOverview());

  root->addStretch();
}

const QVector<QWidget*>& MenuDocsPage::sectionAnchors() const { return anchors_; }

const QStringList& MenuDocsPage::sectionTitles() const { return titles_; }

void MenuDocsPage::addSection(QVBoxLayout* root,
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

QVector<AdMenu::Item> MenuDocsPage::itemsHorizontal() const {
  const auto mail = outlined_icons::Mail();
  const auto app = outlined_icons::Appstore();
  const auto setting = outlined_icons::Setting();

  QVector<AdMenu::Item> items;
  items.append(makeLeaf("mail", "Navigation One", mail));
  items.append(makeLeaf("app", "Navigation Two", app, true));

  QVector<AdMenu::Item> submenuChildren;
  submenuChildren.append(
      makeGroup("g1", "Item 1", {makeLeaf("setting:1", "Option 1"), makeLeaf("setting:2", "Option 2")}));
  submenuChildren.append(
      makeGroup("g2", "Item 2", {makeLeaf("setting:3", "Option 3"), makeLeaf("setting:4", "Option 4")}));
  items.append(makeSubMenu("SubMenu", "Navigation Three - Submenu", submenuChildren, setting));
  items.append(makeLeaf("alipay", "Navigation Four - Link"));
  return items;
}

QVector<AdMenu::Item> MenuDocsPage::itemsInline() const {
  const auto mail = outlined_icons::Mail();
  const auto app = outlined_icons::Appstore();
  const auto setting = outlined_icons::Setting();

  QVector<AdMenu::Item> items;
  items.append(makeSubMenu(
      "sub1", "Navigation One",
      {makeGroup("g1", "Item 1", {makeLeaf("1", "Option 1"), makeLeaf("2", "Option 2")}),
       makeGroup("g2", "Item 2", {makeLeaf("3", "Option 3"), makeLeaf("4", "Option 4")})},
      mail));
  items.append(makeSubMenu(
      "sub2", "Navigation Two",
      {makeLeaf("5", "Option 5"), makeLeaf("6", "Option 6"),
       makeSubMenu("sub3", "Submenu", {makeLeaf("7", "Option 7"), makeLeaf("8", "Option 8")})},
      app));
  items.append(makeSubMenu("sub4", "Navigation Three",
                           {makeLeaf("9", "Option 9"), makeLeaf("10", "Option 10"),
                            makeLeaf("11", "Option 11"), makeLeaf("12", "Option 12")},
                           setting));
  return items;
}

QVector<AdMenu::Item> MenuDocsPage::itemsCollapsedInline() const {
  const auto pie = outlined_icons::PieChart();
  const auto desktop = outlined_icons::Desktop();
  const auto container = outlined_icons::Container();
  const auto mail = outlined_icons::Mail();
  const auto app = outlined_icons::Appstore();

  QVector<AdMenu::Item> items;
  items.append(makeLeaf("1", "Option 1", pie));
  items.append(makeLeaf("2", "Option 2", desktop));
  items.append(makeLeaf("3", "Option 3", container));
  items.append(makeSubMenu("sub1", "Navigation One",
                           {makeLeaf("5", "Option 5"), makeLeaf("6", "Option 6"), makeLeaf("7", "Option 7"),
                            makeLeaf("8", "Option 8")},
                           mail));
  items.append(makeSubMenu(
      "sub2", "Navigation Two",
      {makeLeaf("9", "Option 9"), makeLeaf("10", "Option 10"),
       makeSubMenu("sub3", "Submenu", {makeLeaf("11", "Option 11"), makeLeaf("12", "Option 12")})},
      app));
  return items;
}

QVector<AdMenu::Item> MenuDocsPage::itemsSiderCurrent() const {
  const auto mail = outlined_icons::Mail();
  const auto app = outlined_icons::Appstore();
  const auto setting = outlined_icons::Setting();

  return {
      makeSubMenu("1", "Navigation One",
                  {makeLeaf("11", "Option 1"), makeLeaf("12", "Option 2"), makeLeaf("13", "Option 3"),
                   makeLeaf("14", "Option 4")},
                  mail),
      makeSubMenu("2", "Navigation Two",
                  {makeLeaf("21", "Option 1"), makeLeaf("22", "Option 2"),
                   makeSubMenu("23", "Submenu",
                               {makeLeaf("231", "Option 1"), makeLeaf("232", "Option 2"),
                                makeLeaf("233", "Option 3")}),
                   makeSubMenu("24", "Submenu 2",
                               {makeLeaf("241", "Option 1"), makeLeaf("242", "Option 2"),
                                makeLeaf("243", "Option 3")})},
                  app),
      makeSubMenu("3", "Navigation Three",
                  {makeLeaf("31", "Option 1"), makeLeaf("32", "Option 2"), makeLeaf("33", "Option 3"),
                   makeLeaf("34", "Option 4")},
                  setting),
  };
}

QVector<AdMenu::Item> MenuDocsPage::itemsSwitchMode() const {
  const auto mail = outlined_icons::Mail();
  const auto calendar = outlined_icons::Calendar();
  const auto app = outlined_icons::Appstore();
  const auto setting = outlined_icons::Setting();
  const auto link = outlined_icons::Link();

  return {
      makeLeaf("1", "Navigation One", mail),
      makeLeaf("2", "Navigation Two", calendar),
      makeSubMenu("sub1", "Navigation Two",
                  {makeLeaf("3", "Option 3"), makeLeaf("4", "Option 4"),
                   makeSubMenu("sub1-2", "Submenu", {makeLeaf("5", "Option 5"), makeLeaf("6", "Option 6")})},
                  app),
      makeSubMenu("sub2", "Navigation Three",
                  {makeLeaf("7", "Option 7"), makeLeaf("8", "Option 8"),
                   makeLeaf("9", "Option 9"), makeLeaf("10", "Option 10")},
                  setting),
      makeLeaf("link", "Ant Design", link),
  };
}

QWidget* MenuDocsPage::buildHorizontalDemo(bool dark) {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* menu = new AdMenu();
  menu->setMode(AdMenu::Mode::Horizontal);
  menu->setTheme(dark ? AdMenu::MenuTheme::Dark : AdMenu::MenuTheme::Light);
  menu->setItems(itemsHorizontal());
  menu->setSelectedKey("mail");
  menu->setMinimumWidth(700);

  connect(menu, &AdMenu::clicked, menu, [menu](const QString& key, const QStringList&) {
    menu->setSelectedKey(key);
  });

  layout->addWidget(menu);
  return box;
}

QWidget* MenuDocsPage::buildInlineDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* menu = new AdMenu();
  menu->setMode(AdMenu::Mode::Inline);
  menu->setItems(itemsInline());
  menu->setDefaultOpenKeys({"sub1"});
  menu->setDefaultSelectedKeys({"1"});
  menu->setFixedWidth(256);

  layout->addWidget(menu);
  return box;
}

QWidget* MenuDocsPage::buildInlineCollapsedDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* toggle = new QPushButton("Toggle Collapsed");
  auto* menu = new AdMenu();
  menu->setMode(AdMenu::Mode::Inline);
  menu->setTheme(AdMenu::MenuTheme::Dark);
  menu->setItems(itemsCollapsedInline());
  menu->setDefaultOpenKeys({"sub1"});
  menu->setDefaultSelectedKeys({"1"});
  menu->setFixedWidth(256);

  connect(toggle, &QPushButton::clicked, this, [menu]() {
    const bool collapsed = !menu->inlineCollapsed();
    menu->setInlineCollapsed(collapsed);
    menu->setFixedWidth(collapsed ? 56 : 256);
  });

  layout->addWidget(toggle, 0, Qt::AlignLeft);
  layout->addWidget(menu);
  return box;
}

QWidget* MenuDocsPage::buildTooltipDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* controls = new QHBoxLayout();
  auto* collapseBtn = new QPushButton("Toggle");
  auto* tooltipCheck = new QCheckBox("Tooltip Enabled");
  tooltipCheck->setChecked(true);
  controls->addWidget(collapseBtn);
  controls->addWidget(tooltipCheck);
  controls->addStretch();

  auto* menu = new AdMenu();
  menu->setMode(AdMenu::Mode::Inline);
  menu->setTheme(AdMenu::MenuTheme::Dark);
  menu->setItems(itemsCollapsedInline());
  menu->setDefaultSelectedKeys({"1"});
  menu->setDefaultOpenKeys({"sub1"});
  menu->setFixedWidth(256);

  connect(collapseBtn, &QPushButton::clicked, this, [menu]() {
    const bool collapsed = !menu->inlineCollapsed();
    menu->setInlineCollapsed(collapsed);
    menu->setFixedWidth(collapsed ? 56 : 256);
  });
  connect(tooltipCheck, &QCheckBox::toggled, menu, &AdMenu::setTooltipEnabled);

  layout->addLayout(controls);
  layout->addWidget(menu);
  return box;
}

QWidget* MenuDocsPage::buildSiderCurrentDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  const QVector<AdMenu::Item> items = itemsSiderCurrent();
  auto* menu = new AdMenu();
  menu->setMode(AdMenu::Mode::Inline);
  menu->setItems(items);
  menu->setDefaultSelectedKeys({"231"});
  menu->setFixedWidth(256);

  auto levelMap = std::make_shared<QMap<QString, int>>();
  std::function<void(const QVector<AdMenu::Item>&, int)> collectLevels;
  collectLevels = [&collectLevels, &levelMap](const QVector<AdMenu::Item>& nodes, int level) {
    for (const AdMenu::Item& item : nodes) {
      if (!item.key.isEmpty()) {
        levelMap->insert(item.key, level);
      }
      if (!item.children.isEmpty()) {
        collectLevels(item.children, level + 1);
      }
    }
  };
  collectLevels(items, 1);

  auto stateOpenKeys = std::make_shared<QStringList>(QStringList{"2", "23"});
  menu->setOpenKeys(*stateOpenKeys);

  connect(menu, &AdMenu::openChanged, menu,
          [menu, levelMap, stateOpenKeys](const QStringList& openKeys) {
            QString currentOpenKey;
            for (const QString& key : openKeys) {
              if (!stateOpenKeys->contains(key)) {
                currentOpenKey = key;
                break;
              }
            }

            if (!currentOpenKey.isEmpty()) {
              QStringList next = openKeys;
              QString repeatedKey;
              for (const QString& key : openKeys) {
                if (key == currentOpenKey) {
                  continue;
                }
                if (levelMap->value(key) == levelMap->value(currentOpenKey)) {
                  repeatedKey = key;
                  break;
                }
              }
              if (!repeatedKey.isEmpty()) {
                next.removeAll(repeatedKey);
              }

              QStringList trimmed;
              for (const QString& key : next) {
                if (levelMap->value(key) <= levelMap->value(currentOpenKey)) {
                  trimmed.append(key);
                }
              }
              *stateOpenKeys = trimmed;
            } else {
              *stateOpenKeys = openKeys;
            }

            menu->setOpenKeys(*stateOpenKeys);
          });

  layout->addWidget(menu);
  layout->addWidget(makeHintLabel("Only one submenu stays open at the same depth."));
  return box;
}

QWidget* MenuDocsPage::buildVerticalDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* menu = new AdMenu();
  menu->setMode(AdMenu::Mode::Vertical);
  menu->setItems(itemsInline());
  menu->setFixedWidth(256);

  layout->addWidget(menu);
  return box;
}

QWidget* MenuDocsPage::buildThemeDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* darkCheck = new QCheckBox("Dark");
  darkCheck->setChecked(true);

  auto* menu = new AdMenu();
  menu->setMode(AdMenu::Mode::Inline);
  menu->setTheme(AdMenu::MenuTheme::Dark);
  menu->setItems(itemsInline());
  menu->setOpenKeys({"sub1"});
  menu->setSelectedKey("1");
  menu->setFixedWidth(256);

  connect(darkCheck, &QCheckBox::toggled, menu, [menu](bool checked) {
    menu->setTheme(checked ? AdMenu::MenuTheme::Dark : AdMenu::MenuTheme::Light);
  });
  connect(menu, &AdMenu::clicked, menu, [menu](const QString& key, const QStringList&) {
    menu->setSelectedKey(key);
  });

  layout->addWidget(darkCheck, 0, Qt::AlignLeft);
  layout->addWidget(menu);
  return box;
}

QWidget* MenuDocsPage::buildSubMenuThemeDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* subThemeDark = new QCheckBox("SubMenu Dark");
  subThemeDark->setChecked(false);

  auto* menu = new AdMenu();
  menu->setMode(AdMenu::Mode::Vertical);
  menu->setTheme(AdMenu::MenuTheme::Dark);
  menu->setOpenKeys({"sub1"});
  menu->setSelectedKey("1");
  menu->setFixedWidth(256);

  const auto rebuildItems = [this, menu, subThemeDark]() {
    const auto mail = outlined_icons::Mail();
    AdMenu::Item sub1 = makeSubMenu(
        "sub1", "Navigation One",
        {makeLeaf("1", "Option 1"), makeLeaf("2", "Option 2"), makeLeaf("3", "Option 3")},
        mail);
    sub1.hasSubMenuTheme = true;
    sub1.subMenuTheme = subThemeDark->isChecked() ? AdMenu::MenuTheme::Dark : AdMenu::MenuTheme::Light;
    menu->setItems({sub1, makeLeaf("5", "Option 5"), makeLeaf("6", "Option 6")});
    menu->setOpenKeys({"sub1"});
  };

  connect(subThemeDark, &QCheckBox::toggled, menu, [rebuildItems](bool) { rebuildItems(); });
  connect(menu, &AdMenu::clicked, menu, [menu](const QString& key, const QStringList&) {
    menu->setSelectedKey(key);
  });

  rebuildItems();

  layout->addWidget(subThemeDark, 0, Qt::AlignLeft);
  layout->addWidget(menu);
  return box;
}

QWidget* MenuDocsPage::buildSwitchModeDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* modeCheck = new QCheckBox("Vertical Mode");
  auto* themeCheck = new QCheckBox("Dark Theme");
  auto* controls = new QHBoxLayout();
  controls->addWidget(modeCheck);
  controls->addWidget(themeCheck);
  controls->addStretch();

  auto* menu = new AdMenu();
  menu->setItems(itemsSwitchMode());
  menu->setMode(AdMenu::Mode::Inline);
  menu->setTheme(AdMenu::MenuTheme::Light);
  menu->setDefaultSelectedKeys({"1"});
  menu->setDefaultOpenKeys({"sub1"});
  menu->setFixedWidth(256);

  const auto applyState = [menu, modeCheck, themeCheck]() {
    menu->setMode(modeCheck->isChecked() ? AdMenu::Mode::Vertical : AdMenu::Mode::Inline);
    menu->setTheme(themeCheck->isChecked() ? AdMenu::MenuTheme::Dark : AdMenu::MenuTheme::Light);
  };

  connect(modeCheck, &QCheckBox::toggled, menu, [applyState](bool) { applyState(); });
  connect(themeCheck, &QCheckBox::toggled, menu, [applyState](bool) { applyState(); });

  layout->addLayout(controls);
  layout->addWidget(menu);
  return box;
}

QWidget* MenuDocsPage::buildStyleClassDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  const QVector<AdMenu::Item> items = {
      makeSubMenu("SubMenu", "Navigation One",
                  {makeGroup("g1", "Item 1", {makeLeaf("1", "Option 1"), makeLeaf("2", "Option 2")})}),
      makeLeaf("mail", "Navigation Two"),
  };

  auto* menu1 = new AdMenu();
  menu1->setItems(items);
  menu1->setMode(AdMenu::Mode::Vertical);
  menu1->setFixedWidth(520);
  AdMenu::SemanticStyles semantic1;
  semantic1.root.backgroundColor = QColor(255, 255, 255);
  semantic1.root.borderColor = QColor("#d9d9d9");
  semantic1.item.textColor = QColor("#1677ff");
  semantic1.subMenuItemContent.textColor = QColor("#fa541c");
  menu1->setSemanticStyles(semantic1);

  auto* menu2 = new AdMenu();
  menu2->setItems(items);
  menu2->setMode(AdMenu::Mode::Inline);
  menu2->setOpenKeys({"SubMenu"});
  menu2->setFixedWidth(520);
  menu2->setSemanticStyleResolver([](const AdMenu::StyleContext& ctx) {
    AdMenu::SemanticStyles styles;
    const bool hasSub = !ctx.items.isEmpty() && !ctx.items.first().children.isEmpty();
    styles.root.backgroundColor = hasSub ? QColor(240, 249, 255, 160) : QColor(255, 255, 255);
    styles.root.borderColor = QColor("#d9d9d9");
    styles.item.textColor = QColor("#1677ff");
    return styles;
  });

  layout->addWidget(menu1);
  layout->addWidget(menu2);
  return box;
}

QWidget* MenuDocsPage::buildStyleDebugDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* themeCheck = new QCheckBox("Dark");
  themeCheck->setChecked(true);

  auto* menu = new AdMenu();
  menu->setMode(AdMenu::Mode::Inline);
  menu->setInlineCollapsed(true);
  menu->setTheme(AdMenu::MenuTheme::Dark);
  menu->setItems({
      makeSubMenu("sub1", "Navigation One Long Long Long Long",
                  {makeLeaf("1", "Option 1"), makeLeaf("2", "Option 2"), makeLeaf("3", "Option 3"),
                   makeLeaf("4", "Option 4")},
                  outlined_icons::Mail()),
      makeSubMenu("sub2", "Navigation Two",
                  {makeLeaf("5", "Option 5"), makeLeaf("6", "Option 6"),
                   makeSubMenu("sub3", "Submenu", {makeLeaf("7", "Option 7"), makeLeaf("8", "Option 8")})},
                  outlined_icons::Appstore()),
      makeLeaf("11", "Option 11"),
      makeLeaf("12", "Option 12"),
  });
  menu->setSelectedKey("1");
  menu->setOpenKeys({"sub1"});
  menu->setTooltipEnabled(false);
  menu->setFixedWidth(80);

  menu->setItemPaintHook([](QPainter& painter, const AdMenu::ItemPaintContext& ctx) {
    if (!ctx.itemRect.isValid()) {
      return;
    }
    painter.save();
    painter.setPen(QPen(QColor(22, 119, 255, 180), 1));
    painter.drawLine(ctx.itemRect.left() + 6, ctx.itemRect.bottom() - 2, ctx.itemRect.right() - 6,
                     ctx.itemRect.bottom() - 2);
    painter.restore();
  });
  menu->setSubMenuPaintHook([](QPainter& painter, const AdMenu::ItemPaintContext& ctx) {
    if (!ctx.itemRect.isValid()) {
      return;
    }
    painter.save();
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 255, 255, 40));
    painter.drawRoundedRect(ctx.itemRect.adjusted(1, 1, -1, -1), 6, 6);
    painter.restore();
  });

  connect(themeCheck, &QCheckBox::toggled, menu, [menu](bool checked) {
    menu->setTheme(checked ? AdMenu::MenuTheme::Dark : AdMenu::MenuTheme::Light);
  });

  layout->addWidget(themeCheck, 0, Qt::AlignLeft);
  layout->addWidget(menu);
  return box;
}

QWidget* MenuDocsPage::buildMenuV4Demo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* modeCheck = new QCheckBox("Vertical Mode");

  auto* menu = new AdMenu();
  menu->setItems(itemsSwitchMode());
  menu->setMode(AdMenu::Mode::Inline);
  menu->setDefaultSelectedKeys({"1"});
  menu->setDefaultOpenKeys({"sub1"});
  menu->setFixedWidth(256);

  AdMenu::ComponentTokens tokens;
  tokens.itemBorderRadius = 0;
  tokens.subMenuItemBorderRadius = 0;
  tokens.itemMarginInline = 0;
  tokens.activeBarWidth = 3;
  tokens.itemHoverColor = QStringLiteral("#1890ff");
  tokens.itemSelectedColor = QStringLiteral("#1890ff");
  tokens.itemSelectedBg = QStringLiteral("#e6f7ff");
  tokens.itemHoverBg = QStringLiteral("transparent");
  tokens.horizontalItemHoverBg = QStringLiteral("transparent");
  menu->setComponentTokens(tokens);

  connect(modeCheck, &QCheckBox::toggled, menu, [menu](bool checked) {
    menu->setMode(checked ? AdMenu::Mode::Vertical : AdMenu::Mode::Inline);
  });

  layout->addWidget(modeCheck, 0, Qt::AlignLeft);
  layout->addWidget(menu);
  return box;
}

QWidget* MenuDocsPage::buildComponentTokenDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(12);

  auto* group1 = new QFrame();
  auto* group1Layout = new QVBoxLayout(group1);
  group1Layout->setContentsMargins(0, 0, 0, 0);
  group1Layout->setSpacing(8);

  auto* menu1 = new AdMenu();
  menu1->setMode(AdMenu::Mode::Horizontal);
  menu1->setTheme(AdMenu::MenuTheme::Dark);
  menu1->setItems(itemsHorizontal());
  menu1->setSelectedKey("mail");
  menu1->setMinimumWidth(700);
  AdMenu::ComponentTokens t1;
  t1.popupBg = QStringLiteral("yellow");
  t1.darkPopupBg = QStringLiteral("red");
  menu1->setComponentTokens(t1);

  auto* menu1InlineCollapsed = new AdMenu();
  menu1InlineCollapsed->setMode(AdMenu::Mode::Inline);
  menu1InlineCollapsed->setTheme(AdMenu::MenuTheme::Dark);
  menu1InlineCollapsed->setItems(itemsCollapsedInline());
  menu1InlineCollapsed->setInlineCollapsed(true);
  menu1InlineCollapsed->setDefaultOpenKeys({"sub1"});
  menu1InlineCollapsed->setDefaultSelectedKeys({"1"});
  menu1InlineCollapsed->setFixedWidth(56);
  menu1InlineCollapsed->setComponentTokens(t1);

  group1Layout->addWidget(menu1);
  group1Layout->addWidget(menu1InlineCollapsed);

  auto* group2 = new QFrame();
  auto* group2Layout = new QVBoxLayout(group2);
  group2Layout->setContentsMargins(0, 0, 0, 0);
  group2Layout->setSpacing(8);

  auto* menu2 = new AdMenu();
  menu2->setMode(AdMenu::Mode::Horizontal);
  menu2->setItems(itemsHorizontal());
  menu2->setSelectedKey("mail");
  menu2->setMinimumWidth(700);
  AdMenu::ComponentTokens t2;
  t2.horizontalItemBorderRadius = 6;
  t2.popupBg = QStringLiteral("red");
  t2.horizontalItemHoverBg = QStringLiteral("#f5f5f5");
  menu2->setComponentTokens(t2);
  group2Layout->addWidget(menu2);

  auto* group3 = new QFrame();
  auto* group3Layout = new QVBoxLayout(group3);
  group3Layout->setContentsMargins(0, 0, 0, 0);
  group3Layout->setSpacing(8);

  auto* menu3 = new AdMenu();
  menu3->setMode(AdMenu::Mode::Inline);
  menu3->setTheme(AdMenu::MenuTheme::Dark);
  menu3->setItems(itemsCollapsedInline());
  menu3->setDefaultOpenKeys({"sub1"});
  menu3->setDefaultSelectedKeys({"1"});
  menu3->setFixedWidth(256);
  AdMenu::ComponentTokens t3;
  t3.darkItemColor = QStringLiteral("#91daff");
  t3.darkItemBg = QStringLiteral("#d48806");
  t3.darkSubMenuItemBg = QStringLiteral("#faad14");
  t3.darkItemSelectedColor = QStringLiteral("#ffccc7");
  t3.darkItemSelectedBg = QStringLiteral("#52c41a");
  menu3->setComponentTokens(t3);
  group3Layout->addWidget(menu3);

  layout->addWidget(group1);
  layout->addWidget(group2);
  layout->addWidget(group3);
  return box;
}

QWidget* MenuDocsPage::buildExtraStyleDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  AdMenu::Item sub1 = makeSubMenu("sub1", "Navigation One",
                                  {makeLeaf("1", "Option 1 + icon"), makeLeaf("2", "Option 2"),
                                   makeLeaf("3", "Link Option", {}, true)},
                                  outlined_icons::Mail());
  sub1.children[1].extra = QStringLiteral("Ctrl+P");
  QVector<AdMenu::Item> items1 = {sub1};

  auto* menu1 = new AdMenu();
  menu1->setMode(AdMenu::Mode::Inline);
  menu1->setItems(items1);
  menu1->setDefaultOpenKeys({"sub1"});
  menu1->setDefaultSelectedKeys({"1"});
  menu1->setFixedWidth(256);

  QVector<AdMenu::Item> items2 = {
      makeLeaf("users", "Users"),
      makeLeaf("profile", "Profile"),
      makeDivider("d1", true),
      makeLeaf("danger", "Danger Action"),
  };
  items2[0].extra = QStringLiteral("Ctrl+U");
  items2[1].extra = QStringLiteral("Ctrl+P");
  items2[3].danger = true;

  auto* menu2 = new AdMenu();
  menu2->setItems(items2);
  menu2->setTheme(AdMenu::MenuTheme::Dark);
  menu2->setFixedWidth(256);

  layout->addWidget(menu1);
  layout->addWidget(menu2);
  return box;
}

QWidget* MenuDocsPage::buildCustomPopupRenderDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  QVector<AdMenu::Item> items = {
      makeLeaf("home", "Home"),
      makeSubMenu("features", "Features",
                  {makeLeaf("getting-started", "Getting Started"), makeLeaf("components", "Components"),
                   makeLeaf("templates", "Templates")}),
      makeSubMenu("resources", "Resources", {makeLeaf("blog", "Blog"), makeLeaf("community", "Community")}),
  };

  auto* menu = new AdMenu();
  menu->setMode(AdMenu::Mode::Horizontal);
  menu->setItems(items);
  menu->setMinimumWidth(700);
  AdMenu::ComponentTokens popupTokens;
  popupTokens.horizontalItemSelectedColor = QStringLiteral("#1677ff");
  popupTokens.horizontalItemHoverColor = QStringLiteral("#1677ff");
  menu->setComponentTokens(popupTokens);
  menu->setPopupRender([](const AdMenu::PopupRenderContext& ctx, QWidget* defaultPopup) -> QWidget* {
    auto* panel = new QFrame();
    panel->setObjectName("customPopupPanel");
    panel->setStyleSheet(
        "QFrame#customPopupPanel {"
        "  background: #ffffff;"
        "  border: 1px solid #f0f0f0;"
        "  border-radius: 8px;"
        "}");

    auto* v = new QVBoxLayout(panel);
    v->setContentsMargins(12, 10, 12, 12);
    v->setSpacing(8);

    const QString popupTitle =
        (ctx.item.title.has_value() && !ctx.item.title.value().isEmpty()) ? ctx.item.title.value()
                                                                           : ctx.item.label;
    auto* title = new QLabel(popupTitle, panel);
    QFont titleFont = title->font();
    titleFont.setBold(true);
    title->setFont(titleFont);
    v->addWidget(title);

    if (defaultPopup->parentWidget() != panel) {
      defaultPopup->setParent(panel);
    }
    defaultPopup->setMinimumWidth(320);
    v->addWidget(defaultPopup);
    return panel;
  });

  layout->addWidget(menu);
  return box;
}

QWidget* MenuDocsPage::buildSemanticDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* modeBox = new QComboBox();
  modeBox->addItem("horizontal", static_cast<int>(AdMenu::Mode::Horizontal));
  modeBox->addItem("vertical", static_cast<int>(AdMenu::Mode::Vertical));
  modeBox->addItem("inline", static_cast<int>(AdMenu::Mode::Inline));
  modeBox->setCurrentIndex(0);

  auto* menu = new AdMenu();
  menu->setItems(itemsHorizontal());
  menu->setMode(AdMenu::Mode::Horizontal);
  menu->setOpenKeys({"SubMenu"});
  menu->setSelectedKey("mail");
  menu->setMinimumWidth(560);

  menu->setSemanticStyleResolver([](const AdMenu::StyleContext& ctx) {
    AdMenu::SemanticStyles styles;
    if (ctx.mode == AdMenu::Mode::Horizontal) {
      styles.root.backgroundColor = QColor("#f6ffed");
      styles.item.textColor = QColor("#389e0d");
    } else if (ctx.mode == AdMenu::Mode::Vertical) {
      styles.root.backgroundColor = QColor("#fff7e6");
      styles.item.textColor = QColor("#d46b08");
    } else {
      styles.root.backgroundColor = QColor("#e6f4ff");
      styles.item.textColor = QColor("#1677ff");
    }
    styles.root.borderColor = QColor("#d9d9d9");
    return styles;
  });

  connect(modeBox, QOverload<int>::of(&QComboBox::currentIndexChanged), menu, [this, menu, modeBox](int) {
    const auto mode = static_cast<AdMenu::Mode>(modeBox->currentData().toInt());
    menu->setMode(mode);
    if (mode == AdMenu::Mode::Horizontal) {
      menu->setItems(itemsHorizontal());
      menu->setMinimumWidth(560);
    } else {
      QVector<AdMenu::Item> items = itemsHorizontal();
      items.append(makeGroup("grp", "Group", {makeLeaf("13", "Option 13"), makeLeaf("14", "Option 14")}));
      menu->setItems(items);
      menu->setFixedWidth(260);
    }
    menu->setOpenKeys({"SubMenu"});
  });

  auto* slotsLabel = makeHintLabel(
      "Semantic slots: root / item / itemIcon / itemContent / itemTitle / list / popup / "
      "subMenu.item / subMenu.itemTitle / subMenu.list / subMenu.itemIcon / subMenu.itemContent");

  layout->addWidget(modeBox, 0, Qt::AlignLeft);
  layout->addWidget(menu);
  layout->addWidget(slotsLabel);
  return box;
}

QWidget* MenuDocsPage::buildApiOverview() {
  auto* box = new QWidget();
  auto* grid = new QGridLayout(box);
  grid->setContentsMargins(0, 0, 0, 0);
  grid->setHorizontalSpacing(16);
  grid->setVerticalSpacing(8);

  const QVector<QPair<QString, QString>> rows = {
      {"mode", "vertical | horizontal | inline"},
      {"theme", "light | dark"},
      {"items", "QVector<AdMenu::Item>"},
      {"items[].icon", "adqt::icons::IconToken"},
      {"expandIcon", "adqt::icons::IconToken"},
      {"selectedKeys / defaultSelectedKeys", "controlled / uncontrolled selected items"},
      {"openKeys / defaultOpenKeys", "controlled / uncontrolled opened sub menus"},
      {"multiple", "enable multi selection"},
      {"inlineCollapsed", "collapsed state for inline mode"},
      {"inlineIndent", "indent per nested level (px)"},
      {"triggerSubMenuAction", "hover | click"},
      {"subMenuOpenDelayMs / subMenuCloseDelayMs", "popup open/close delay in milliseconds"},
      {"tooltipEnabled", "tooltip behavior in inline-collapsed mode"},
      {"componentTokens", "component token overrides"},
      {"semanticStyles / semanticStyleResolver", "semantic style overrides"},
      {"popupRender", "custom popup renderer"},
      {"itemPaintHook / subMenuPaintHook", "public debug render hooks"},
      {"signals: clicked/selected/deselected/openChanged/titleClicked", "event callbacks"},
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
