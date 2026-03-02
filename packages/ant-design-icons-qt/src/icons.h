#ifndef ADQT_ICONS_ICONS_H
#define ADQT_ICONS_ICONS_H

#include "ant_design_icons_qt_global.h"
#include "icons_types.h"
#include "version.h"

#include <QIcon>

namespace adqt::icons {

ADQT_ICONS_EXPORT const char* version();

ADQT_ICONS_EXPORT void setThemeResolver(IconThemeResolver resolver);
ADQT_ICONS_EXPORT void clearThemeResolver();

ADQT_ICONS_EXPORT void setPixmapCacheLimitKB(int kb);
ADQT_ICONS_EXPORT void clearIconCache();

}  // namespace adqt::icons

#include "generated/icon_functions.h"

#endif  // ADQT_ICONS_ICONS_H
