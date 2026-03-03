#ifndef ADQT_ICONS_ICON_RENDER_H
#define ADQT_ICONS_ICON_RENDER_H

#include "icons_types.h"

#include <QByteArray>
#include <QColor>

namespace adqt::icons::detail {

QByteArray applyColorsToSvg(const QByteArray& source,
                            IconTheme theme,
                            const char* iconName,
                            const QColor& primary,
                            const QColor& secondary,
                            const QColor& tertiary);

QColor deriveSecondaryColor(const QColor& primary);

}  // namespace adqt::icons::detail

#endif  // ADQT_ICONS_ICON_RENDER_H
