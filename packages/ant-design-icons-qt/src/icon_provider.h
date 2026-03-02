#ifndef ADQT_ICONS_ICON_PROVIDER_H
#define ADQT_ICONS_ICON_PROVIDER_H

#include "generated/icon_manifest.h"
#include "icons_types.h"

#include <QIcon>

namespace adqt::icons::detail {

QIcon makeIconByIndex(int index, const IconStyle& style);

QPixmap renderIconPixmapByIndex(int index,
                                const IconStyle& style,
                                const QSize& logicalSize,
                                qreal devicePixelRatio,
                                QIcon::Mode mode,
                                QIcon::State state);

void setThemeResolver(IconThemeResolver resolver);
void clearThemeResolver();

void setPixmapCacheLimitKB(int kb);
void clearIconCache();

}  // namespace adqt::icons::detail

#endif  // ADQT_ICONS_ICON_PROVIDER_H
