#pragma once

#include "icons_types.h"

#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QSize>

namespace adqt::widgets::detail {

bool iconShouldInheritCurrentColor(const adqt::icons::IconToken& token);
adqt::icons::IconToken iconWithInheritedColor(const adqt::icons::IconToken& token,
                                              const QColor& color);
QPixmap renderIconPixmap(const adqt::icons::IconToken& token,
                         const QSize& logicalSize,
                         qreal devicePixelRatio,
                         const QColor& inheritedColor,
                         QIcon::Mode mode = QIcon::Normal,
                         QIcon::State state = QIcon::Off);

}  // namespace adqt::widgets::detail
