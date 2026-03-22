#include <QItemSelectionModel>
#include <QLabel>
#include <QMetaProperty>
#include <QSignalSpy>
#include <QStandardItemModel>
#include <QTreeView>
#include <QVBoxLayout>
#include <QtTest>

#include "widgets/navigation_menu.h"
#include "widgets/tooltip.h"

namespace {

using adqt::widgets::AdNavigationMenu;
using adqt::widgets::AdNavigationMenuItemDelegate;
using adqt::widgets::AdNavigationMenuPopupContext;
using adqt::widgets::AdNavigationMenuPopupFactory;
using adqt::widgets::AdTooltip;

QStandardItem* makeActionItem(const QString& id, const QString& label) {
  auto* item = new QStandardItem(label);
  item->setData(id, AdNavigationMenu::StableIdRole);
  item->setData(static_cast<int>(AdNavigationMenu::NodeKind::Action),
                AdNavigationMenu::NodeKindRole);
  item->setEditable(false);
  return item;
}

QStandardItem* makeSubmenuItem(const QString& id, const QString& label) {
  return makeActionItem(id, label);
}

QStandardItem* makeGroupItem(const QString& id, const QString& label) {
  auto* item = new QStandardItem(label);
  item->setData(id, AdNavigationMenu::StableIdRole);
  item->setData(static_cast<int>(AdNavigationMenu::NodeKind::Group),
                AdNavigationMenu::NodeKindRole);
  item->setEditable(false);
  item->setSelectable(false);
  return item;
}

QStandardItemModel* createBasicMenuModel(QObject* parent) {
  auto* model = new QStandardItemModel(parent);

  auto* group = makeSubmenuItem(QStringLiteral("group"), QStringLiteral("Group"));
  group->appendRow(makeActionItem(QStringLiteral("child-1"), QStringLiteral("Child 1")));
  group->appendRow(makeActionItem(QStringLiteral("child-2"), QStringLiteral("Child 2")));
  model->appendRow(group);

  auto* tools = makeGroupItem(QStringLiteral("tools"), QStringLiteral("Tools"));
  tools->appendRow(makeActionItem(QStringLiteral("leaf-a"), QStringLiteral("Leaf A")));
  model->appendRow(tools);

  model->appendRow(makeActionItem(QStringLiteral("top"), QStringLiteral("Top")));
  return model;
}

QModelIndex findIndexById(const QAbstractItemModel* model,
                          const QString& id,
                          const QModelIndex& parent = QModelIndex()) {
  if (!model) {
    return QModelIndex();
  }
  const int rowCount = model->rowCount(parent);
  for (int row = 0; row < rowCount; ++row) {
    const QModelIndex index = model->index(row, 0, parent);
    if (!index.isValid()) {
      continue;
    }
    if (index.data(AdNavigationMenu::StableIdRole).toString() == id) {
      return index;
    }
    if (const QModelIndex child = findIndexById(model, id, index); child.isValid()) {
      return child;
    }
  }
  return QModelIndex();
}

class TallMenuDelegate final : public AdNavigationMenuItemDelegate {
 public:
  explicit TallMenuDelegate(AdNavigationMenu* owner, QObject* parent = nullptr)
      : AdNavigationMenuItemDelegate(owner, parent) {}

  QSize sizeHint(const QStyleOptionViewItem& option,
                 const QModelIndex& index) const override {
    const QSize base = AdNavigationMenuItemDelegate::sizeHint(option, index);
    return QSize(base.width(), std::max(base.height(), 72));
  }
};

class CountingPopupFactory final : public AdNavigationMenuPopupFactory {
 public:
  explicit CountingPopupFactory(QObject* parent = nullptr)
      : AdNavigationMenuPopupFactory(parent) {}

  int createCount = 0;

  QWidget* createPopup(const AdNavigationMenuPopupContext& context,
                       QWidget* defaultPopup,
                       QWidget* parent) override {
    ++createCount;
    auto* wrapper = new QWidget(parent);
    wrapper->setObjectName(QStringLiteral("menu-test-popup"));
    auto* layout = new QVBoxLayout(wrapper);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->addWidget(new QLabel(context.submenuIndex.data(Qt::DisplayRole).toString(), wrapper));
    if (defaultPopup->parentWidget() != wrapper) {
      defaultPopup->setParent(wrapper);
    }
    layout->addWidget(defaultPopup);
    return wrapper;
  }
};

QWidget* findWidgetByObjectName(const QString& objectName) {
  const auto widgets = QApplication::allWidgets();
  for (QWidget* widget : widgets) {
    if (widget && widget->objectName() == objectName) {
      return widget;
    }
  }
  return nullptr;
}

class MenuTests final : public QObject {
  Q_OBJECT

 private slots:
  void exposesQtFirstProperties();
  void indentationDefaultsToTokensUntilOverridden();
  void externalSelectionModelDrivesCurrentIndex();
  void expandCollapseUsesQtIndexes();
  void keyboardActivationEmitsActivated();
  void rtlKeyboardNavigationMirrorsInlineArrows();
  void customDelegateAffectsGeometry();
  void customPopupFactoryWrapsPopup();
  void collapsedTooltipTracksCurrentLeaf();
};

void MenuTests::exposesQtFirstProperties() {
  AdNavigationMenu menu;
  const QMetaObject* meta = menu.metaObject();

  QVERIFY(meta->indexOfProperty("itemDelegate") >= 0);
  QVERIFY(meta->indexOfProperty("popupFactory") >= 0);
  QVERIFY(meta->indexOfProperty("model") >= 0);
  QVERIFY(meta->indexOfProperty("selectionModel") >= 0);
  QVERIFY(meta->indexOfProperty("currentIndex") >= 0);
  QVERIFY(meta->indexOfProperty("collapsed") >= 0);
  QVERIFY(meta->indexOfProperty("indentation") >= 0);
  QVERIFY(meta->indexOfProperty("submenuTrigger") >= 0);
  QVERIFY(meta->indexOfProperty("submenuOpenDelayMs") >= 0);
  QVERIFY(meta->indexOfProperty("submenuCloseDelayMs") >= 0);

  QCOMPARE(meta->indexOfProperty("items"), -1);
  QCOMPARE(meta->indexOfProperty("selectedKeys"), -1);
  QCOMPARE(meta->indexOfProperty("defaultSelectedKeys"), -1);
  QCOMPARE(meta->indexOfProperty("openKeys"), -1);
  QCOMPARE(meta->indexOfProperty("defaultOpenKeys"), -1);
  QCOMPARE(meta->indexOfProperty("indent"), -1);
  QCOMPARE(meta->indexOfProperty("inlineCollapsed"), -1);
  QCOMPARE(meta->indexOfProperty("inlineIndent"), -1);
  QCOMPARE(meta->indexOfProperty("triggerSubMenuAction"), -1);
  QCOMPARE(meta->indexOfProperty("popupRender"), -1);
  QCOMPARE(meta->indexOfProperty("subMenuOpenDelayMs"), -1);
  QCOMPARE(meta->indexOfProperty("subMenuCloseDelayMs"), -1);

  QVERIFY(menu.setProperty(
      "submenuTrigger",
      QVariant::fromValue(AdNavigationMenu::TriggerSubMenuAction::Click)));
  QCOMPARE(menu.submenuTrigger(), AdNavigationMenu::TriggerSubMenuAction::Click);
  QVERIFY(menu.setProperty("indentation", 18));
  QCOMPARE(menu.indentation(), 18);
  menu.setIndent(24);
  QCOMPARE(menu.indentation(), 24);
  menu.setSubMenuOpenDelayMs(320);
  QCOMPARE(menu.submenuOpenDelayMs(), 320);
  menu.setSubMenuCloseDelayMs(160);
  QCOMPARE(menu.submenuCloseDelayMs(), 160);
}

void MenuTests::indentationDefaultsToTokensUntilOverridden() {
  AdNavigationMenu menu;
  QCOMPARE(menu.indentation(), 24);

  AdNavigationMenu::ComponentTokens tokens;
  tokens.metrics.indentation = 36;
  menu.setComponentTokens(tokens);
  QCOMPARE(menu.indentation(), 36);

  menu.setIndentation(20);
  QCOMPARE(menu.indentation(), 20);

  AdNavigationMenu::ComponentTokens updatedTokens = tokens;
  updatedTokens.metrics.indentation = 44;
  menu.setComponentTokens(updatedTokens);
  QCOMPARE(menu.indentation(), 20);
}

void MenuTests::externalSelectionModelDrivesCurrentIndex() {
  AdNavigationMenu menu;
  QStandardItemModel model;
  model.appendRow(makeActionItem(QStringLiteral("one"), QStringLiteral("One")));
  model.appendRow(makeActionItem(QStringLiteral("two"), QStringLiteral("Two")));

  QItemSelectionModel selectionModel(&model);
  menu.setModel(&model);
  menu.setSelectionModel(&selectionModel);

  QSignalSpy currentSpy(&menu, &AdNavigationMenu::currentIndexChanged);

  const QModelIndex index = findIndexById(&model, QStringLiteral("two"));
  selectionModel.select(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
  selectionModel.setCurrentIndex(index, QItemSelectionModel::NoUpdate);
  QCoreApplication::processEvents();

  QCOMPARE(menu.currentIndex(), index);
  QVERIFY(currentSpy.count() >= 1);
}

void MenuTests::expandCollapseUsesQtIndexes() {
  AdNavigationMenu menu;
  menu.setMode(AdNavigationMenu::Mode::Inline);
  auto* model = createBasicMenuModel(&menu);
  menu.setModel(model);

  const QModelIndex submenuIndex = findIndexById(model, QStringLiteral("group"));
  QSignalSpy expandedSpy(&menu, &AdNavigationMenu::expanded);
  QSignalSpy collapsedSpy(
      &menu,
      static_cast<void (AdNavigationMenu::*)(const QModelIndex&)>(&AdNavigationMenu::collapsed));

  menu.setExpanded(submenuIndex, true);
  QVERIFY(menu.isExpanded(submenuIndex));
  QCOMPARE(expandedSpy.count(), 1);

  menu.collapse(submenuIndex);
  QVERIFY(!menu.isExpanded(submenuIndex));
  QCOMPARE(collapsedSpy.count(), 1);

  menu.expand(submenuIndex);
  QVERIFY(menu.isExpanded(submenuIndex));
  menu.collapseAll();
  QVERIFY(!menu.isExpanded(submenuIndex));
}

void MenuTests::keyboardActivationEmitsActivated() {
  QWidget host;
  auto* layout = new QVBoxLayout(&host);

  AdNavigationMenu menu;
  menu.setMode(AdNavigationMenu::Mode::Inline);
  auto* model = createBasicMenuModel(&menu);
  menu.setModel(model);
  layout->addWidget(&menu);

  host.show();
  QVERIFY(QTest::qWaitForWindowExposed(&host));

  const QModelIndex index = findIndexById(model, QStringLiteral("top"));
  menu.setCurrentIndex(index);

  auto* view = host.findChild<QTreeView*>(QStringLiteral("AdNavigationMenu-inline-view"));
  QVERIFY(view);
  view->setFocus();
  QVERIFY(view->hasFocus());

  QSignalSpy activatedSpy(&menu, &AdNavigationMenu::activated);
  QTest::keyClick(view, Qt::Key_Return);

  QTRY_COMPARE(activatedSpy.count(), 1);
  const QModelIndex activatedIndex =
      activatedSpy.at(0).at(0).value<QModelIndex>();
  QCOMPARE(activatedIndex, index);
}

void MenuTests::rtlKeyboardNavigationMirrorsInlineArrows() {
  QWidget host;
  auto* layout = new QVBoxLayout(&host);

  AdNavigationMenu menu;
  menu.setLayoutDirection(Qt::RightToLeft);
  menu.setMode(AdNavigationMenu::Mode::Inline);
  auto* model = createBasicMenuModel(&menu);
  menu.setModel(model);
  layout->addWidget(&menu);

  host.show();
  QVERIFY(QTest::qWaitForWindowExposed(&host));

  const QModelIndex submenuIndex = findIndexById(model, QStringLiteral("group"));
  const QModelIndex childIndex = findIndexById(model, QStringLiteral("child-1"));
  QVERIFY(submenuIndex.isValid());
  QVERIFY(childIndex.isValid());

  auto* view = host.findChild<QTreeView*>(QStringLiteral("AdNavigationMenu-inline-view"));
  QVERIFY(view);

  menu.setCurrentIndex(submenuIndex);
  view->setFocus();
  QVERIFY(view->hasFocus());

  QTest::keyClick(view, Qt::Key_Left);
  QVERIFY(menu.isExpanded(submenuIndex));

  menu.setCurrentIndex(childIndex);
  QTest::keyClick(view, Qt::Key_Right);
  QVERIFY(!menu.isExpanded(submenuIndex));
  QCOMPARE(menu.currentIndex(), submenuIndex);
}

void MenuTests::customDelegateAffectsGeometry() {
  AdNavigationMenu menu;
  menu.setMode(AdNavigationMenu::Mode::Inline);
  auto* model = new QStandardItemModel(&menu);
  model->appendRow(makeActionItem(QStringLiteral("one"), QStringLiteral("One")));
  menu.setModel(model);

  QSignalSpy delegateSpy(&menu, &AdNavigationMenu::itemDelegateChanged);

  auto* delegate = new TallMenuDelegate(&menu, &menu);
  menu.setItemDelegate(delegate);

  QCOMPARE(menu.itemDelegate(), static_cast<QAbstractItemDelegate*>(delegate));
  QVERIFY(menu.sizeHint().height() >= 72);
  QCOMPARE(delegateSpy.count(), 1);
}

void MenuTests::customPopupFactoryWrapsPopup() {
  QWidget host;
  auto* layout = new QVBoxLayout(&host);

  AdNavigationMenu menu;
  menu.setMode(AdNavigationMenu::Mode::Vertical);
  auto* model = new QStandardItemModel(&menu);
  auto* submenu = makeSubmenuItem(QStringLiteral("root"), QStringLiteral("Root"));
  submenu->appendRow(makeActionItem(QStringLiteral("child"), QStringLiteral("Child")));
  model->appendRow(submenu);
  menu.setModel(model);

  auto* factory = new CountingPopupFactory(&menu);
  menu.setPopupFactory(factory);
  layout->addWidget(&menu);

  host.show();
  QVERIFY(QTest::qWaitForWindowExposed(&host));

  const QModelIndex submenuIndex = findIndexById(model, QStringLiteral("root"));
  menu.setExpanded(submenuIndex, true);

  QTRY_VERIFY(factory->createCount > 0);
  QWidget* popup = nullptr;
  QTRY_VERIFY((popup = findWidgetByObjectName(QStringLiteral("menu-test-popup"))) != nullptr);
  QVERIFY(popup->isVisible());
}

void MenuTests::collapsedTooltipTracksCurrentLeaf() {
  AdNavigationMenu menu;
  menu.setMode(AdNavigationMenu::Mode::Inline);
  menu.setCollapsed(true);
  auto* model = new QStandardItemModel(&menu);
  auto* item = makeActionItem(QStringLiteral("leaf"), QStringLiteral("Leaf Item"));
  item->setData(QStringLiteral("Leaf Tooltip"), Qt::ToolTipRole);
  model->appendRow(item);
  menu.setModel(model);

  menu.show();
  QVERIFY(QTest::qWaitForWindowExposed(&menu));

  const QModelIndex leafIndex = findIndexById(model, QStringLiteral("leaf"));
  auto* view = menu.findChild<QTreeView*>(QStringLiteral("AdNavigationMenu-vertical-view"));
  QVERIFY(view);
  const QRect itemRect = view->visualRect(leafIndex);
  QVERIFY(itemRect.isValid());
  QTest::mouseMove(view->viewport(), itemRect.center());
  QVERIFY(QMetaObject::invokeMethod(view,
                                    "entered",
                                    Qt::DirectConnection,
                                    Q_ARG(QModelIndex, leafIndex)));

  AdTooltip* tooltip = nullptr;
  QTRY_VERIFY_WITH_TIMEOUT((tooltip = menu.findChild<AdTooltip*>()) != nullptr, 5000);
  QTRY_VERIFY_WITH_TIMEOUT(tooltip->isVisible(), 5000);
  QCOMPARE(tooltip->text(), QStringLiteral("Leaf Tooltip"));

  menu.setTooltipEnabled(false);
  QTRY_VERIFY(!tooltip->isVisible());
}

}  // namespace

int runMenuTests(int argc, char** argv) {
  MenuTests tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "menu_tests.moc"
