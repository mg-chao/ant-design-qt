#include "icon_theme_adapter.h"

#include "icons.h"
#include "theme/theme_manager.h"

#include <QColor>

namespace demo {

namespace {

adqt::icons::IconThemeSnapshot buildSnapshot() {
  const adqt::theme::ThemeManager& manager = adqt::theme::ThemeManager::instance();
  const adqt::theme::ThemeMapToken& map = manager.currentMapToken();

  adqt::icons::IconThemeSnapshot snapshot;
  snapshot.text = QColor(map.colorText);
  snapshot.textDisabled = QColor(map.colorTextQuaternary);
  snapshot.primary = QColor(map.colorPrimary);
  snapshot.twoToneSecondary = QColor(map.colorPrimaryBg);
  snapshot.revision = manager.themeRevision();
  return snapshot;
}

}  // namespace

void installIconThemeResolver() {
  adqt::icons::setThemeResolver([]() { return buildSnapshot(); });
}

}  // namespace demo
