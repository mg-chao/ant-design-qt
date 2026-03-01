#include "theme_manager.h"

namespace adqt::theme {

ThemeManager& ThemeManager::instance() {
  static ThemeManager manager;
  return manager;
}

ThemeManager::ThemeManager(QObject* parent) : QObject(parent) {
  config_ = defaultThemeConfig();
  key_ = themeConfigKey(config_);
  mapToken_ = computeMapToken(config_);
  token_ = toGlobalPaletteToken(mapToken_);
  palette_ = buildPalette(token_);
}

void ThemeManager::setTheme(const ThemeConfig& config) {
  const ThemeConfig normalized = normalizeThemeConfig(config);
  const QByteArray nextKey = themeConfigKey(normalized);

  if (nextKey == key_) {
    return;
  }

  config_ = normalized;
  key_ = nextKey;
  mapToken_ = computeMapToken(config_);
  token_ = toGlobalPaletteToken(mapToken_);
  palette_ = buildPalette(token_);

  if (app_) {
    app_->setPalette(palette_);
  }

  emit themeChanged();
}

const ThemeConfig& ThemeManager::currentConfig() const { return config_; }

const ThemeMapToken& ThemeManager::currentMapToken() const { return mapToken_; }

const GlobalPaletteToken& ThemeManager::currentToken() const { return token_; }

const QPalette& ThemeManager::currentPalette() const { return palette_; }

void ThemeManager::applyTo(QApplication& app) {
  app_ = &app;
  app.setPalette(palette_);
}

}  // namespace adqt::theme
