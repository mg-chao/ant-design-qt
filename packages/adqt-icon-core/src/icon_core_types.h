#ifndef ADQT_ICON_CORE_TYPES_H
#define ADQT_ICON_CORE_TYPES_H

#include <QColor>
#include <QByteArray>
#include <QMetaType>
#include <QString>
#include <QtGlobal>

#include <functional>

#include "adqt_icon_core_global.h"

namespace adqt::icons {

inline uint iconHashCombine(uint seed, uint value) {
  return seed ^ (value + 0x9e3779b9u + (seed << 6) + (seed >> 2));
}

enum class IconTheme {
  Outlined,
  Filled,
  TwoTone,
};

enum class IconRenderModel {
  Monochrome,
  TwoTone,
  ThreeTone,
};

struct IconColorOverrides {
  QColor primary;
  QColor secondary;
  QColor tertiary;
  bool hasPrimary = false;
  bool hasSecondary = false;
  bool hasTertiary = false;

  bool isEmpty() const {
    return !hasPrimary && !hasSecondary && !hasTertiary;
  }
};

inline bool operator==(const IconColorOverrides& lhs, const IconColorOverrides& rhs) {
  return lhs.hasPrimary == rhs.hasPrimary && lhs.hasSecondary == rhs.hasSecondary &&
         lhs.hasTertiary == rhs.hasTertiary && lhs.primary == rhs.primary &&
         lhs.secondary == rhs.secondary && lhs.tertiary == rhs.tertiary;
}

inline bool operator!=(const IconColorOverrides& lhs, const IconColorOverrides& rhs) {
  return !(lhs == rhs);
}

inline uint qHash(const IconColorOverrides& value, uint seed = 0) {
  seed = iconHashCombine(seed, ::qHash(value.primary.rgba(), 0));
  seed = iconHashCombine(seed, ::qHash(value.secondary.rgba(), 0));
  seed = iconHashCombine(seed, ::qHash(value.tertiary.rgba(), 0));
  seed = iconHashCombine(seed, ::qHash(value.hasPrimary, 0));
  seed = iconHashCombine(seed, ::qHash(value.hasSecondary, 0));
  seed = iconHashCombine(seed, ::qHash(value.hasTertiary, 0));
  return seed;
}

struct IconId {
  QString pack;
  IconTheme theme = IconTheme::Outlined;
  QString name;

  bool isValid() const {
    return !pack.isEmpty() && !name.isEmpty();
  }
};

inline bool operator==(const IconId& lhs, const IconId& rhs) {
  return lhs.pack == rhs.pack && lhs.theme == rhs.theme && lhs.name == rhs.name;
}

inline bool operator!=(const IconId& lhs, const IconId& rhs) {
  return !(lhs == rhs);
}

inline bool isValid(const IconId& id) {
  return id.isValid();
}

inline uint qHash(const IconId& value, uint seed = 0) {
  seed = iconHashCombine(seed, ::qHash(value.pack, 0));
  seed = iconHashCombine(seed, ::qHash(static_cast<int>(value.theme), 0));
  seed = iconHashCombine(seed, ::qHash(value.name, 0));
  return seed;
}

struct IconRef {
  IconId id;
  IconColorOverrides colors;

  bool isValid() const {
    return id.isValid();
  }
};

inline bool operator==(const IconRef& lhs, const IconRef& rhs) {
  return lhs.id == rhs.id && lhs.colors == rhs.colors;
}

inline bool operator!=(const IconRef& lhs, const IconRef& rhs) {
  return !(lhs == rhs);
}

inline bool isValid(const IconRef& ref) {
  return ref.isValid();
}

inline uint qHash(const IconRef& value, uint seed = 0) {
  seed = iconHashCombine(seed, qHash(value.id, 0));
  seed = iconHashCombine(seed, qHash(value.colors, 0));
  return seed;
}

struct IconMetadata {
  IconId id;
  IconRenderModel model = IconRenderModel::Monochrome;

  bool isValid() const {
    return id.isValid();
  }
};

inline bool operator==(const IconMetadata& lhs, const IconMetadata& rhs) {
  return lhs.id == rhs.id && lhs.model == rhs.model;
}

inline bool operator!=(const IconMetadata& lhs, const IconMetadata& rhs) {
  return !(lhs == rhs);
}

inline bool isValid(const IconMetadata& metadata) {
  return metadata.isValid();
}

struct IconDefinition {
  IconId id;
  IconRenderModel model = IconRenderModel::Monochrome;
  QByteArray svgTemplate;

  bool isValid() const {
    return id.isValid() && !svgTemplate.isEmpty();
  }
};

struct IconPalette {
  QColor text = QColor(QStringLiteral("#1F1F1F"));
  QColor textDisabled = QColor(QStringLiteral("#BFBFBF"));
  QColor primary = QColor(QStringLiteral("#1677FF"));
  QColor twoToneSecondary = QColor(QStringLiteral("#E6F4FF"));
  quint64 revision = 1;
};

using IconPaletteResolver = std::function<IconPalette()>;

ADQT_ICON_CORE_EXPORT const char* iconThemeKey(IconTheme theme);

}  // namespace adqt::icons

Q_DECLARE_METATYPE(adqt::icons::IconTheme)
Q_DECLARE_METATYPE(adqt::icons::IconRenderModel)
Q_DECLARE_METATYPE(adqt::icons::IconColorOverrides)
Q_DECLARE_METATYPE(adqt::icons::IconId)
Q_DECLARE_METATYPE(adqt::icons::IconRef)
Q_DECLARE_METATYPE(adqt::icons::IconMetadata)

#endif  // ADQT_ICON_CORE_TYPES_H
