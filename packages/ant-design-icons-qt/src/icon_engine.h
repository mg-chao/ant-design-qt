#ifndef ADQT_ICONS_ICON_ENGINE_H
#define ADQT_ICONS_ICON_ENGINE_H

#include "icons_types.h"

#include <QIconEngine>

namespace adqt::icons::detail {

class SvgIconEngine final : public QIconEngine {
 public:
  SvgIconEngine(int entryIndex, const IconStyle& style);
  ~SvgIconEngine() override = default;

  QIconEngine* clone() const override;
  QString key() const override;
  QPixmap pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) override;
  QPixmap scaledPixmap(const QSize& size,
                       QIcon::Mode mode,
                       QIcon::State state,
                       qreal scale) override;
  void paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state) override;

 private:
  int entryIndex_ = -1;
  IconStyle style_;
};

}  // namespace adqt::icons::detail

#endif  // ADQT_ICONS_ICON_ENGINE_H
