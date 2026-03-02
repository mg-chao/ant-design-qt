#include "theme_manager.h"

#include <QFontDatabase>

namespace adqt::theme {

namespace {

QString normalizeFontFamilyToken(const QString& token) {
  QString normalized = token.trimmed();
  if (normalized.size() >= 2) {
    const QChar first = normalized.front();
    const QChar last = normalized.back();
    if ((first == '\'' && last == '\'') || (first == '"' && last == '"')) {
      normalized = normalized.mid(1, normalized.size() - 2).trimmed();
    }
  }
  return normalized;
}

QStringList parseCssFontFamilies(const QString& cssFontFamily) {
  QStringList families;
  const QStringList parts = cssFontFamily.split(',', Qt::SkipEmptyParts);
  for (const QString& part : parts) {
    const QString normalized = normalizeFontFamilyToken(part);
    if (!normalized.isEmpty()) {
      families.append(normalized);
    }
  }
  return families;
}

bool isGenericCssFamily(const QString& family) {
  static const QStringList genericFamilies = {
      "serif",        "sans-serif",    "monospace", "cursive",      "fantasy",
      "system-ui",    "ui-serif",      "ui-sans-serif", "ui-monospace",
      "ui-rounded",   "emoji",         "math",      "fangsong",
  };

  for (const QString& generic : genericFamilies) {
    if (family.compare(generic, Qt::CaseInsensitive) == 0) {
      return true;
    }
  }
  return false;
}

QString resolveInstalledFamily(const QString& requested, const QStringList& installedFamilies) {
  for (const QString& installed : installedFamilies) {
    if (requested.compare(installed, Qt::CaseInsensitive) == 0) {
      return installed;
    }
  }
  return QString();
}

QString resolveFirstAvailableFontFamily(const QString& cssFontFamily) {
  const QStringList preferredFamilies = parseCssFontFamilies(cssFontFamily);
  if (preferredFamilies.isEmpty()) {
    return QString();
  }

  QFontDatabase fontDatabase;
  const QStringList installedFamilies = fontDatabase.families();
  if (installedFamilies.isEmpty()) {
    return QString();
  }

  for (const QString& family : preferredFamilies) {
    if (family.startsWith('-') || isGenericCssFamily(family)) {
      continue;
    }

    const QString installed = resolveInstalledFamily(family, installedFamilies);
    if (!installed.isEmpty()) {
      return installed;
    }
  }

  return QString();
}

}  // namespace

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
  ++revision_;

  if (app_) {
    app_->setPalette(palette_);
    applyAppFont(*app_);
  }

  emit themeChanged();
}

const ThemeConfig& ThemeManager::currentConfig() const { return config_; }

const ThemeMapToken& ThemeManager::currentMapToken() const { return mapToken_; }

const GlobalPaletteToken& ThemeManager::currentToken() const { return token_; }

const QPalette& ThemeManager::currentPalette() const { return palette_; }

quint64 ThemeManager::themeRevision() const { return revision_; }

void ThemeManager::applyAppFont(QApplication& app) const {
  if (!hasBaseFont_) {
    return;
  }

  if (!config_.loadAntdFont) {
    app.setFont(baseFont_);
    return;
  }

  QFont font = baseFont_;
  const QString family = resolveFirstAvailableFontFamily(config_.seed.fontFamily);
  if (!family.isEmpty()) {
    font.setFamily(family);
  }
  app.setFont(font);
}

void ThemeManager::applyTo(QApplication& app) {
  app_ = &app;
  baseFont_ = app.font();
  hasBaseFont_ = true;
  app.setPalette(palette_);
  applyAppFont(app);
}

}  // namespace adqt::theme
