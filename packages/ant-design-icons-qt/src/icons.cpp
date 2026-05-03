#include "icons.h"

#include "icon_provider.h"

#include <utility>

namespace adqt::icons {

const char* version() { return ADQT_ICONS_VERSION_STR; }

bool isValid(const IconToken& token) { return token.isValid(); }

IconMetadata iconMetadata(const IconToken& token) { return detail::iconMetadata(token); }

bool isTwoTone(const IconToken& token) { return detail::isTwoTone(token); }

bool isSingleTone(const IconToken& token) { return detail::isSingleTone(token); }

QIcon makeIcon(const IconToken& token) { return detail::makeIcon(token); }

QPixmap renderIconPixmap(const IconToken& token,
                         const QSize& logicalSize,
                         qreal devicePixelRatio,
                         QIcon::Mode mode,
                         QIcon::State state) {
  return detail::renderIconPixmap(token, logicalSize, devicePixelRatio, mode, state);
}

IconToken registerCustomIcon(const CustomIconSource& source, const IconStyle& style) {
  return detail::registerCustomIcon(source, style);
}

IconToken registerSvgIcon(IconTheme theme,
                          const QString& name,
                          const QByteArray& svg,
                          const IconStyle& style) {
  CustomIconSource source;
  source.theme = theme;
  source.name = name;
  source.sourceType = IconSourceType::SvgBytes;
  source.svg = svg;
  return registerCustomIcon(source, style);
}

IconToken registerSvgIconFile(IconTheme theme,
                              const QString& name,
                              const QString& filePath,
                              const IconStyle& style) {
  CustomIconSource source;
  source.theme = theme;
  source.name = name;
  source.sourceType = IconSourceType::SvgFile;
  source.path = filePath;
  return registerCustomIcon(source, style);
}

IconToken registerSvgIconResource(IconTheme theme,
                                  const QString& name,
                                  const QString& qrcPath,
                                  const IconStyle& style) {
  CustomIconSource source;
  source.theme = theme;
  source.name = name;
  source.sourceType = IconSourceType::SvgResource;
  source.path = qrcPath;
  return registerCustomIcon(source, style);
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
