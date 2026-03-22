#include <algorithm>

#include <QAccessible>
#include <QItemSelectionModel>
#include <QListView>
#include <QMetaProperty>
#include <QSignalSpy>
#include <QStandardItemModel>
#include <QtTest>

#include "select_tests.h"
#include "widgets/combo_box.h"
#include "widgets/multi_select.h"
#include "widgets/tag_select.h"

namespace {

QStandardItem* makeSelectItem(const QVariant& value, const QString& label) {
  auto* item = new QStandardItem(label);
  item->setData(label, Qt::DisplayRole);
  item->setData(value, adqt::widgets::AdComboBox::DefaultValueRole);
  item->setData(label, adqt::widgets::AdComboBox::DefaultLabelRole);
  return item;
}

void populateSingleColumnModel(QStandardItemModel* model,
                               const QList<QPair<QVariant, QString>>& rows) {
  if (!model) {
    return;
  }
  for (const auto& row : rows) {
    model->appendRow(makeSelectItem(row.first, row.second));
  }
}

int popupRowCount(adqt::widgets::AdAbstractSelectWidget* widget) {
  if (!widget) {
    return 0;
  }
  QListView* view = widget->view();
  return (view && view->model()) ? view->model()->rowCount() : 0;
}

}  // namespace

void SelectTests::exposesQtModelProperties() {
  adqt::widgets::AdComboBox comboBox;
  const QMetaObject* meta = comboBox.metaObject();

  const int modelColumnIndex = meta->indexOfProperty("modelColumn");
  QVERIFY(modelColumnIndex >= 0);
  const QMetaProperty modelColumnProperty = meta->property(modelColumnIndex);
  QVERIFY(modelColumnProperty.isReadable());
  QVERIFY(modelColumnProperty.isWritable());

  QVERIFY(comboBox.setProperty("modelColumn", 1));
  QCOMPARE(comboBox.modelColumn(), 1);

  const adqt::widgets::AdComboBox::RoleConfig defaultRoles = comboBox.roleConfig();
  QCOMPARE(defaultRoles.valueRole, adqt::widgets::AdComboBox::DefaultValueRole);
  QCOMPARE(defaultRoles.labelRole, adqt::widgets::AdComboBox::DefaultLabelRole);

  adqt::widgets::AdComboBox::RoleConfig roles = defaultRoles;
  roles.valueRole = Qt::UserRole + 51;
  roles.labelRole = Qt::UserRole + 52;
  roles.tagTextRole = Qt::UserRole + 53;
  roles.selectedTextRole = Qt::UserRole + 54;
  roles.groupRole = Qt::UserRole + 55;
  roles.searchRoles = {roles.labelRole};
  comboBox.setRoleConfig(roles);

  const adqt::widgets::AdComboBox::RoleConfig appliedRoles = comboBox.roleConfig();
  QCOMPARE(appliedRoles.valueRole, roles.valueRole);
  QCOMPARE(appliedRoles.labelRole, roles.labelRole);
  QCOMPARE(appliedRoles.tagTextRole, roles.tagTextRole);
  QCOMPARE(appliedRoles.selectedTextRole, roles.selectedTextRole);
  QCOMPARE(appliedRoles.groupRole, roles.groupRole);
  QCOMPARE(appliedRoles.searchRoles, roles.searchRoles);
}

void SelectTests::popupWidthModeRoundTrips() {
  adqt::widgets::AdComboBox comboBox;

  QCOMPARE(comboBox.popupWidthMode(), adqt::widgets::AdComboBox::PopupWidthMode::MatchControlWidth);
  QCOMPARE(comboBox.popupWidth(), 0);

  comboBox.setPopupWidthMode(adqt::widgets::AdComboBox::PopupWidthMode::ContentWidth);
  QCOMPARE(comboBox.popupWidthMode(), adqt::widgets::AdComboBox::PopupWidthMode::ContentWidth);

  comboBox.setPopupWidth(220);
  QCOMPARE(comboBox.popupWidth(), 220);

  comboBox.setPopupWidthMode(adqt::widgets::AdComboBox::PopupWidthMode::FixedWidth);
  QCOMPARE(comboBox.popupWidthMode(), adqt::widgets::AdComboBox::PopupWidthMode::FixedWidth);
  QCOMPARE(comboBox.popupWidth(), 220);
}

void SelectTests::setCurrentValueTracksCurrentModelIndex() {
  adqt::widgets::AdComboBox comboBox;
  QStandardItemModel model;
  populateSingleColumnModel(
      &model, {{QVariant(1), QStringLiteral("One")}, {QVariant(2), QStringLiteral("Two")}});
  comboBox.setModel(&model);

  QSignalSpy indexSpy(&comboBox, &adqt::widgets::AdComboBox::currentModelIndexChanged);

  comboBox.setCurrentValue(2);

  QCOMPARE(comboBox.currentValue(), QVariant(2));
  QCOMPARE(comboBox.currentModelIndex(), model.index(1, 0));
  QCOMPARE(comboBox.currentIndex(), 1);
  QCOMPARE(indexSpy.count(), 1);
}

void SelectTests::setCurrentValuesSyncSelectionModel() {
  adqt::widgets::AdMultiSelect multiSelect;

  QStandardItemModel model;
  populateSingleColumnModel(&model, {{QVariant(1), QStringLiteral("One")},
                                     {QVariant(2), QStringLiteral("Two")},
                                     {QVariant(3), QStringLiteral("Three")}});
  multiSelect.setModel(&model);
  multiSelect.setSelectedValues(QVariantList({QVariant(1), QVariant(3)}));

  QCOMPARE(multiSelect.selectedValues(), QVariantList({QVariant(1), QVariant(3)}));
  QCOMPARE(multiSelect.selectedIndexes(), QModelIndexList({model.index(0, 0), model.index(2, 0)}));

  QVERIFY(multiSelect.selectionModel());
  const QModelIndexList selectedRows = multiSelect.selectionModel()->selectedRows(0);
  QCOMPARE(selectedRows.size(), 2);
  QCOMPARE(selectedRows.at(0), model.index(0, 0));
  QCOMPARE(selectedRows.at(1), model.index(2, 0));
}

void SelectTests::tagsPreserveCustomValues() {
  adqt::widgets::AdTagSelect tagSelect;

  QStandardItemModel model;
  populateSingleColumnModel(
      &model, {{QVariant(1), QStringLiteral("One")}, {QVariant(2), QStringLiteral("Two")}});
  tagSelect.setModel(&model);
  tagSelect.setSelectedValues(QVariantList({QVariant(1), QVariant(QStringLiteral("custom"))}));

  QCOMPARE(tagSelect.selectedValues(),
           QVariantList({QVariant(1), QVariant(QStringLiteral("custom"))}));
  QCOMPARE(tagSelect.selectedIndexes(), QModelIndexList({model.index(0, 0)}));

  const QVector<adqt::widgets::AdTagSelect::SelectionItem> items = tagSelect.selectedItems();
  QCOMPARE(items.size(), 2);
  QCOMPARE(items.at(0).value, QVariant(1));
  QCOMPARE(items.at(0).label, QStringLiteral("One"));
  QCOMPARE(items.at(1).value, QVariant(QStringLiteral("custom")));
  QCOMPARE(items.at(1).label, QStringLiteral("custom"));
}

void SelectTests::modelColumnUsesAlternateColumn() {
  adqt::widgets::AdComboBox comboBox;
  comboBox.setModelColumn(1);

  QStandardItemModel model(0, 2);

  QList<QStandardItem*> firstRow;
  firstRow << new QStandardItem(QStringLiteral("Row 1"))
           << makeSelectItem(QStringLiteral("alpha"), QStringLiteral("Alpha"));
  model.appendRow(firstRow);

  QList<QStandardItem*> secondRow;
  secondRow << new QStandardItem(QStringLiteral("Row 2"))
            << makeSelectItem(QStringLiteral("beta"), QStringLiteral("Beta"));
  model.appendRow(secondRow);

  comboBox.setModel(&model);
  comboBox.setCurrentValue(QStringLiteral("beta"));

  QCOMPARE(comboBox.currentValue(), QVariant(QStringLiteral("beta")));
  QCOMPARE(comboBox.currentModelIndex(), model.index(1, 1));
  QCOMPARE(comboBox.currentItem().label, QStringLiteral("Beta"));
}

void SelectTests::externalSelectionModelUpdatesWidget() {
  adqt::widgets::AdMultiSelect multiSelect;

  QStandardItemModel model;
  populateSingleColumnModel(&model, {{QVariant(1), QStringLiteral("One")},
                                     {QVariant(2), QStringLiteral("Two")},
                                     {QVariant(3), QStringLiteral("Three")}});
  QItemSelectionModel selectionModel(&model);
  multiSelect.setModel(&model);
  multiSelect.setSelectionModel(&selectionModel);

  QSignalSpy valuesSpy(&multiSelect, &adqt::widgets::AdMultiSelect::selectedValuesChanged);

  selectionModel.select(model.index(0, 0),
                        QItemSelectionModel::Select | QItemSelectionModel::Rows);
  selectionModel.select(model.index(2, 0),
                        QItemSelectionModel::Select | QItemSelectionModel::Rows);
  selectionModel.setCurrentIndex(model.index(2, 0), QItemSelectionModel::NoUpdate);
  QCoreApplication::processEvents();

  QCOMPARE(multiSelect.selectedValues(), QVariantList({QVariant(1), QVariant(3)}));
  QCOMPARE(multiSelect.selectedIndexes(),
           QModelIndexList({model.index(0, 0), model.index(2, 0)}));
  QVERIFY(valuesSpy.count() >= 1);
}

void SelectTests::accessibleRoleMatchesMode() {
  adqt::widgets::AdComboBox single;
  single.setPlaceholder(QStringLiteral("Pick one"));
  QAccessibleInterface* singleInterface = QAccessible::queryAccessibleInterface(&single);
  QVERIFY(singleInterface);
  QCOMPARE(singleInterface->role(), QAccessible::ComboBox);
  QVERIFY(singleInterface->state().collapsed);

  adqt::widgets::AdMultiSelect multi;
  QAccessibleInterface* multiInterface = QAccessible::queryAccessibleInterface(&multi);
  QVERIFY(multiInterface);
  QCOMPARE(multiInterface->role(), QAccessible::List);
  QVERIFY(multiInterface->state().multiSelectable);
}

void SelectTests::externalSearchPolicyBypassesLocalFiltering() {
  adqt::widgets::AdComboBox comboBox;
  QStandardItemModel model;
  populateSingleColumnModel(
      &model, {{QVariant(1), QStringLiteral("One")}, {QVariant(2), QStringLiteral("Two")}});
  comboBox.setModel(&model);
  comboBox.setSearchEnabled(true);

  comboBox.show();
  QTest::qWait(1);
  comboBox.showPopup();
  QTRY_VERIFY(comboBox.popupVisible());

  comboBox.setSearchText(QStringLiteral("zzz"));
  QTRY_COMPARE(popupRowCount(&comboBox), 1);

  comboBox.setSearchPolicy(adqt::widgets::AdComboBox::SearchPolicy::External);
  QTRY_COMPARE(popupRowCount(&comboBox), 2);
}

void SelectTests::keyboardNavigationActivatesCurrentRow() {
  adqt::widgets::AdComboBox comboBox;
  QStandardItemModel model;
  populateSingleColumnModel(
      &model, {{QVariant(1), QStringLiteral("One")}, {QVariant(2), QStringLiteral("Two")},
               {QVariant(3), QStringLiteral("Three")}});
  comboBox.setModel(&model);

  comboBox.show();
  QTest::qWait(1);
  comboBox.showPopup();
  QTRY_VERIFY(comboBox.popupVisible());

  QListView* view = comboBox.view();
  QVERIFY(view);
  QVERIFY(view->currentIndex().isValid());
  QCOMPARE(view->currentIndex().row(), 0);

  QTest::keyClick(view, Qt::Key_Down);
  QTRY_VERIFY(view->currentIndex().row() > 0);
  const QModelIndex activatedIndex = view->currentIndex();
  const QVariant expectedValue = activatedIndex.data(adqt::widgets::AdComboBox::DefaultValueRole);
  const QString expectedText = activatedIndex.data(adqt::widgets::AdComboBox::DefaultLabelRole).toString();

  QSignalSpy valueSpy(&comboBox, &adqt::widgets::AdComboBox::currentValueChanged);
  QTest::keyClick(view, Qt::Key_Return);

  QTRY_COMPARE(comboBox.currentValue(), expectedValue);
  QCOMPARE(comboBox.currentText(), expectedText);
  QVERIFY(valueSpy.count() >= 1);
}

void SelectTests::multiPopupSelectionReflectsSelectionState() {
  adqt::widgets::AdMultiSelect multiSelect;

  QStandardItemModel model;
  populateSingleColumnModel(&model, {{QVariant(1), QStringLiteral("One")},
                                     {QVariant(2), QStringLiteral("Two")},
                                     {QVariant(3), QStringLiteral("Three")}});
  multiSelect.setModel(&model);
  multiSelect.setSelectedValues(QVariantList({QVariant(1), QVariant(3)}));

  multiSelect.show();
  QTest::qWait(1);
  multiSelect.showPopup();
  QTRY_VERIFY(multiSelect.popupVisible());

  QListView* view = multiSelect.view();
  QVERIFY(view);
  QCOMPARE(view->selectionMode(), QAbstractItemView::MultiSelection);
  QVERIFY(view->selectionModel());

  QModelIndexList selectedRows = view->selectionModel()->selectedRows();
  std::sort(selectedRows.begin(), selectedRows.end(),
            [](const QModelIndex& lhs, const QModelIndex& rhs) { return lhs.row() < rhs.row(); });

  QCOMPARE(selectedRows.size(), 2);
  QCOMPARE(selectedRows.at(0).data(adqt::widgets::AdComboBox::DefaultValueRole), QVariant(1));
  QCOMPARE(selectedRows.at(1).data(adqt::widgets::AdComboBox::DefaultValueRole), QVariant(3));
}

void SelectTests::comboBoxForwardsQtCurrentApi() {
  adqt::widgets::AdComboBox comboBox;
  QStandardItemModel model;
  populateSingleColumnModel(
      &model, {{QVariant(1), QStringLiteral("One")}, {QVariant(2), QStringLiteral("Two")}});
  model.item(1)->setData(QStringLiteral("native-user-role"), Qt::UserRole);
  comboBox.setModel(&model);

  const QMetaObject* meta = comboBox.metaObject();
  QVERIFY(meta->indexOfProperty("searchable") >= 0);

  comboBox.setCurrentIndex(1);

  QCOMPARE(comboBox.currentIndex(), 1);
  QCOMPARE(comboBox.currentText(), QStringLiteral("Two"));
  QCOMPARE(comboBox.currentValue(), QVariant(2));
  QCOMPARE(comboBox.currentData(), QVariant(2));
  QCOMPARE(comboBox.currentData(Qt::UserRole), QVariant(QStringLiteral("native-user-role")));

  comboBox.setCurrentData(1);
  QCOMPARE(comboBox.currentIndex(), 0);
  QCOMPARE(comboBox.currentText(), QStringLiteral("One"));

  QSignalSpy searchableSpy(&comboBox, &adqt::widgets::AdComboBox::searchableChanged);
  QSignalSpy editableSpy(&comboBox, &adqt::widgets::AdComboBox::editableChanged);
  comboBox.setSearchable(true);
  QVERIFY(comboBox.searchable());
  QVERIFY(comboBox.editable());
  QCOMPARE(searchableSpy.count(), 1);
  QCOMPARE(editableSpy.count(), 1);
  QVERIFY(comboBox.lineEdit());
  QVERIFY(comboBox.view());
}

void SelectTests::multiSelectSupportsQtStyleSelectionApi() {
  adqt::widgets::AdMultiSelect multiSelect;
  QStandardItemModel model;
  populateSingleColumnModel(&model, {{QVariant(1), QStringLiteral("One")},
                                     {QVariant(2), QStringLiteral("Two")},
                                     {QVariant(3), QStringLiteral("Three")}});
  multiSelect.setModel(&model);
  multiSelect.setSelectedValues(QVariantList({QVariant(1), QVariant(3)}));

  QCOMPARE(multiSelect.selectedValues(), QVariantList({QVariant(1), QVariant(3)}));
  QCOMPARE(multiSelect.selectedItems().size(), 2);
  QCOMPARE(multiSelect.selectedIndexes(),
           QModelIndexList({model.index(0, 0), model.index(2, 0)}));
  multiSelect.setMaxSelectionCount(2);
  multiSelect.setSelectedValues(QVariantList({QVariant(1), QVariant(2), QVariant(3)}));
  QCOMPARE(multiSelect.selectedValues(), QVariantList({QVariant(1), QVariant(2)}));
  multiSelect.setResponsiveMaxTagCount(true);
  QVERIFY(multiSelect.responsiveMaxTagCount());
}

void SelectTests::tagSelectSupportsQtStyleSelectionApi() {
  adqt::widgets::AdTagSelect tagSelect;
  QStandardItemModel model;
  populateSingleColumnModel(&model, {{QVariant(1), QStringLiteral("One")},
                                     {QVariant(2), QStringLiteral("Two")},
                                     {QVariant(3), QStringLiteral("Three")}});
  tagSelect.setModel(&model);
  tagSelect.setTokenSeparators({QStringLiteral(","), QStringLiteral(";")});
  tagSelect.setSelectedValues(QVariantList({QVariant(1), QVariant(QStringLiteral("custom"))}));

  QCOMPARE(tagSelect.selectedValues(),
           QVariantList({QVariant(1), QVariant(QStringLiteral("custom"))}));
  QCOMPARE(tagSelect.selectedIndexes(), QModelIndexList({model.index(0, 0)}));
  QCOMPARE(tagSelect.selectedItems().size(), 2);
  QCOMPARE(tagSelect.tokenSeparators(), QStringList({QStringLiteral(","), QStringLiteral(";")}));
  tagSelect.setAutoClearSearchValue(false);
  QVERIFY(!tagSelect.autoClearSearchValue());
}

int runSelectTests(int argc, char** argv) {
  SelectTests tests;
  return QTest::qExec(&tests, argc, argv);
}
