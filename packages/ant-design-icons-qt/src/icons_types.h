#ifndef ADQT_ICONS_TYPES_H
#define ADQT_ICONS_TYPES_H

#include <QColor>
#include <QByteArray>
#include <QMetaType>
#include <QString>
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
  QColor tertiary;
  bool hasPrimary = false;
  bool hasSecondary = false;
  bool hasTertiary = false;
};

inline bool operator==(const IconStyle& lhs, const IconStyle& rhs) {
  return lhs.hasPrimary == rhs.hasPrimary && lhs.hasSecondary == rhs.hasSecondary &&
         lhs.hasTertiary == rhs.hasTertiary && lhs.primary == rhs.primary &&
         lhs.secondary == rhs.secondary && lhs.tertiary == rhs.tertiary;
}

inline bool operator!=(const IconStyle& lhs, const IconStyle& rhs) {
  return !(lhs == rhs);
}

enum class IconSourceType {
  SvgBytes,
  SvgFile,
  SvgResource,
};

struct CustomIconSource {
  IconTheme theme = IconTheme::Outlined;
  QString name;
  IconSourceType sourceType = IconSourceType::SvgBytes;
  QByteArray svg;
  QString path;

  bool isValid() const {
    if (name.trimmed().isEmpty()) {
      return false;
    }
    if (sourceType == IconSourceType::SvgBytes) {
      return !svg.trimmed().isEmpty();
    }
    return !path.trimmed().isEmpty();
  }
};

struct IconToken {
  int index = -1;
  IconStyle style;

  bool isValid() const { return index >= 0; }
};

inline bool operator==(const IconToken& lhs, const IconToken& rhs) {
  return lhs.index == rhs.index && lhs.style == rhs.style;
}

inline bool operator!=(const IconToken& lhs, const IconToken& rhs) {
  return !(lhs == rhs);
}

struct IconMetadata {
  bool valid = false;
  bool custom = false;
  IconTheme theme = IconTheme::Outlined;
  QString name;
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
