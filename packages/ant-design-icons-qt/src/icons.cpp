#include "icons.h"

#include "icon_provider.h"

#include <utility>

namespace adqt::icons {

const char* version() { return ADQT_ICONS_VERSION_STR; }

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
