#include <QApplication>
#include <QFile>
#include <QImage>
#include <QPainter>
#include <QStandardItemModel>
#include <QTextStream>
#include <QVBoxLayout>
#include <QWidget>

#include "widgets/navigation_menu.h"

using adqt::widgets::AdNavigationMenu;

namespace {

QStandardItem* makeLeaf(const QString& stableId, const QString& label) {
  auto* item = new QStandardItem(label);
  item->setData(stableId, AdNavigationMenu::StableIdRole);
  item->setData(static_cast<int>(AdNavigationMenu::NodeKind::Action),
                AdNavigationMenu::NodeKindRole);
  item->setEditable(false);
  return item;
}

QStandardItem* makeSubMenu(const QString& stableId,
                           const QString& label,
                           std::initializer_list<QStandardItem*> children) {
  auto* item = new QStandardItem(label);
  item->setData(stableId, AdNavigationMenu::StableIdRole);
  item->setData(static_cast<int>(AdNavigationMenu::NodeKind::Action),
                AdNavigationMenu::NodeKindRole);
  item->setEditable(false);
  for (QStandardItem* child : children) {
    item->appendRow(child);
  }
  return item;
}

}  // namespace

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);

  QWidget root;
  root.setAttribute(Qt::WA_DontShowOnScreen, true);
  root.setStyleSheet("background:#ffffff;");

  auto* layout = new QVBoxLayout(&root);
  layout->setContentsMargins(24, 24, 24, 24);
  layout->setSpacing(0);

  auto* menu = new AdNavigationMenu(&root);
  auto* model = new QStandardItemModel(menu);
  menu->setMode(AdNavigationMenu::Mode::Inline);
  menu->setFixedWidth(256);
  model->appendRow(makeSubMenu("sub1", "Navigation One",
                               {makeLeaf("1", "Option 1"), makeLeaf("2", "Option 2")}));
  model->appendRow(makeSubMenu("sub2", "Navigation Two",
                               {makeLeaf("3", "Option 3"), makeLeaf("4", "Option 4")}));
  menu->setModel(model);
  menu->setExpanded(model->index(0, 0), true);

  layout->addWidget(menu);

  root.resize(root.sizeHint());
  root.show();
  app.processEvents();

  QFile metricsFile("menu-spacing-probe.txt");
  if (metricsFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
    QTextStream stream(&metricsFile);
    stream << "rootSize=" << root.size().width() << "x" << root.size().height() << "\n";
    stream << "menuSize=" << menu->size().width() << "x" << menu->size().height() << "\n";
  }

  QImage image(root.size() * root.devicePixelRatioF(), QImage::Format_ARGB32_Premultiplied);
  image.setDevicePixelRatio(root.devicePixelRatioF());
  image.fill(Qt::white);
  QPainter painter(&image);
  root.render(&painter);
  image.save("menu-spacing-probe.png");

  return 0;
}
