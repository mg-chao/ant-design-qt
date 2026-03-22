#include <QApplication>
#include <QDebug>
#include <QSignalSpy>

#include "widgets/color_picker.h"
#include "widgets/select.h"

using adqt::widgets::AdColorPickerPanel;
using adqt::widgets::AdSelect;

int main(int argc, char** argv) {
  QApplication app(argc, argv);

  AdColorPickerPanel panel;
  panel.setValue(QStringLiteral("#1677ff"));

  auto* combo = panel.findChild<AdSelect*>(QStringLiteral("ad-color-picker-format-select"));
  qDebug() << "combo" << combo;
  if (!combo) {
    return 2;
  }

  QSignalSpy comboSpy(combo, &AdSelect::currentValueChanged);
  QSignalSpy formatSpy(&panel, &AdColorPickerPanel::formatChanged);

  qDebug() << "before current" << combo->currentValue() << "format" << static_cast<int>(panel.format());
  combo->setCurrentValue(QStringLiteral("rgb"));
  qDebug() << "after current" << combo->currentValue() << "format" << static_cast<int>(panel.format());
  qDebug() << "comboSpy" << comboSpy.count() << "formatSpy" << formatSpy.count();
  if (comboSpy.count() > 0) {
    qDebug() << "combo arg" << comboSpy.at(0).value(0);
  }

  return 0;
}
