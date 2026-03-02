#include "icon_engine.h"

#include "icon_provider.h"

#include <QPainter>

namespace adqt::icons::detail {

SvgIconEngine::SvgIconEngine(int entryIndex, const IconStyle& style)
    : entryIndex_(entryIndex), style_(style) {}

QIconEngine* SvgIconEngine::clone() const {
  return new SvgIconEngine(entryIndex_, style_);
}

QString SvgIconEngine::key() const {
  return QStringLiteral("adqt.svg.icon.engine");
}

QPixmap SvgIconEngine::pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) {
  // QIcon::pixmap(size, mode, state) has no target DPR input. Render 1x here and
  // rely on scaledPixmap/paint for DPR-aware callsites.
  return renderIconPixmapByIndex(entryIndex_, style_, size, 1.0, mode, state);
}

QPixmap SvgIconEngine::scaledPixmap(const QSize& size,
                                    QIcon::Mode mode,
                                    QIcon::State state,
                                    qreal scale) {
  const qreal dpr = scale > 0.0 ? scale : 1.0;
  return renderIconPixmapByIndex(entryIndex_, style_, size, dpr, mode, state);
}

void SvgIconEngine::paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state) {
  if (!painter || rect.isEmpty()) {
    return;
  }

  const qreal dpr = painter->device() ? painter->device()->devicePixelRatioF() : 1.0;
  const QPixmap pm = renderIconPixmapByIndex(entryIndex_, style_, rect.size(), dpr, mode, state);
  if (pm.isNull()) {
    return;
  }

  painter->drawPixmap(rect, pm);
}

}  // namespace adqt::icons::detail
