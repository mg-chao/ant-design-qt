#ifndef ADQT_ICONS_TYPES_H
#define ADQT_ICONS_TYPES_H

#include <QColor>
#include <QMetaType>
#include <QtGlobal>

#include <functional>

#include "ant_design_icons_qt_global.h"

namespace adqt::icons {

enum class IconTheme {
  Outlined,
  Filled,
  TwoTone,
};

struct IconStyle {
  QColor primary;
  QColor secondary;
  bool hasPrimary = false;
  bool hasSecondary = false;
};

struct IconToken {
  int index = -1;
  IconStyle style;

  bool isValid() const { return index >= 0; }
};

struct IconThemeSnapshot {
  QColor text = QColor(QStringLiteral("#1F1F1F"));
  QColor textDisabled = QColor(QStringLiteral("#BFBFBF"));
  QColor primary = QColor(QStringLiteral("#1677FF"));
  QColor twoToneSecondary = QColor(QStringLiteral("#E6F4FF"));
  quint64 revision = 1;
};

using IconThemeResolver = std::function<IconThemeSnapshot()>;

}  // namespace adqt::icons

Q_DECLARE_METATYPE(adqt::icons::IconToken)

#endif  // ADQT_ICONS_TYPES_H
