#include "icons.h"

#include "icon_provider.h"

#include <utility>

namespace adqt::icons {

const char* version() { return ADQT_ICONS_VERSION_STR; }

bool isValid(const IconToken& token) { return token.isValid(); }

QIcon makeIcon(const IconToken& token) { return detail::makeIcon(token); }

QPixmap renderIconPixmap(const IconToken& token,
                         const QSize& logicalSize,
                         qreal devicePixelRatio,
                         QIcon::Mode mode,
                         QIcon::State state) {
  return detail::renderIconPixmap(token, logicalSize, devicePixelRatio, mode, state);
}

void setThemeResolver(IconThemeResolver resolver) {
  detail::setThemeResolver(std::move(resolver));
}

void clearThemeResolver() {
  detail::clearThemeResolver();
}

void setPixmapCacheLimitKB(int kb) {
  detail::setPixmapCacheLimitKB(kb);
}

void clearIconCache() {
  detail::clearIconCache();
}

}  // namespace adqt::icons
