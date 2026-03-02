#pragma once

#include "theme_types.h"

#include <QByteArray>

namespace adqt::theme {

ThemeConfig normalizeThemeConfig(const ThemeConfig& config);
bool hasAlgorithm(const ThemeConfig& config, ThemeAlgorithm algorithm);

ThemeMapToken computeMapToken(const ThemeConfig& config);
GlobalPaletteToken toGlobalPaletteToken(const ThemeMapToken& mapToken);

QByteArray themeConfigKey(const ThemeConfig& config);

}  // namespace adqt::theme
