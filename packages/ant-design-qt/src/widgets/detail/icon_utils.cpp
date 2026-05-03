#include "icon_utils.h"

#include "icons.h"

namespace adqt::widgets::detail {

bool iconShouldInheritCurrentColor(const adqt::icons::IconToken& token) {
  if (!adqt::icons::isValid(token)) {
    return false;
  }
  if (token.style.hasPrimary || token.style.hasSecondary || token.style.hasTertiary) {
    return false;
  }
  return adqt::icons::isSingleTone(token);
}

adqt::icons::IconToken iconWithInheritedColor(const adqt::icons::IconToken& token,
                                              const QColor& color) {
  adqt::icons::IconToken tinted = token;
  if (iconShouldInheritCurrentColor(tinted)) {
    tinted.style.primary = color;
    tinted.style.hasPrimary = true;
  }
  return tinted;
}

QPixmap renderIconPixmap(const adqt::icons::IconToken& token,
                         const QSize& logicalSize,
                         qreal devicePixelRatio,
                         const QColor& inheritedColor,
                         QIcon::Mode mode,
                         QIcon::State state) {
  const adqt::icons::IconToken tinted = iconWithInheritedColor(token, inheritedColor);
  return adqt::icons::renderIconPixmap(tinted, logicalSize, devicePixelRatio, mode, state);
}

}  // namespace adqt::widgets::detail
