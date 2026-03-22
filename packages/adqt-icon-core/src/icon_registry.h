#ifndef ADQT_ICON_REGISTRY_H
#define ADQT_ICON_REGISTRY_H

#include "adqt_icon_core_global.h"
#include "icon_core_types.h"

#include <QIcon>
#include <QList>
#include <QPainter>
#include <QPixmap>
#include <QRectF>
#include <QSize>

#include <memory>

namespace adqt::icons::detail {
struct IconRegistryImpl;
}

namespace adqt::icons {

class ADQT_ICON_CORE_EXPORT IconRegistry final {
 public:
  IconRegistry();
  ~IconRegistry();

  IconRegistry(const IconRegistry&) = delete;
  IconRegistry& operator=(const IconRegistry&) = delete;

  bool registerIcon(const IconDefinition& definition);
  bool containsIcon(const IconId& id) const;
  IconMetadata describeIcon(const IconId& id) const;
  QList<IconMetadata> listIcons(const QString& pack = QString()) const;
  QList<IconMetadata> listIcons(const QString& pack, IconTheme theme) const;

  QIcon makeIcon(const IconRef& ref) const;
  QPixmap renderIconPixmap(const IconRef& ref,
                           const QSize& logicalSize,
                           qreal devicePixelRatio,
                           QIcon::Mode mode = QIcon::Normal,
                           QIcon::State state = QIcon::Off) const;
  void paintIcon(QPainter* painter,
                 const IconRef& ref,
                 const QRectF& rect,
                 QIcon::Mode mode = QIcon::Normal,
                 QIcon::State state = QIcon::Off) const;

  void setPaletteResolver(IconPaletteResolver resolver);
  void clearPaletteResolver();

  void setCacheLimitKB(int kb);
  void clearCache();

 private:
  std::shared_ptr<detail::IconRegistryImpl> impl_;
};

ADQT_ICON_CORE_EXPORT IconRegistry& defaultRegistry();

ADQT_ICON_CORE_EXPORT bool registerIcon(const IconDefinition& definition);
ADQT_ICON_CORE_EXPORT bool containsIcon(const IconId& id);
ADQT_ICON_CORE_EXPORT IconMetadata describeIcon(const IconId& id);
ADQT_ICON_CORE_EXPORT QList<IconMetadata> listIcons(const QString& pack = QString());
ADQT_ICON_CORE_EXPORT QList<IconMetadata> listIcons(const QString& pack, IconTheme theme);

ADQT_ICON_CORE_EXPORT QIcon makeIcon(const IconRef& ref);
ADQT_ICON_CORE_EXPORT QPixmap renderIconPixmap(const IconRef& ref,
                                               const QSize& logicalSize,
                                               qreal devicePixelRatio,
                                               QIcon::Mode mode = QIcon::Normal,
                                               QIcon::State state = QIcon::Off);
ADQT_ICON_CORE_EXPORT void paintIcon(QPainter* painter,
                                     const IconRef& ref,
                                     const QRectF& rect,
                                     QIcon::Mode mode = QIcon::Normal,
                                     QIcon::State state = QIcon::Off);

ADQT_ICON_CORE_EXPORT void setPaletteResolver(IconPaletteResolver resolver);
ADQT_ICON_CORE_EXPORT void clearPaletteResolver();

ADQT_ICON_CORE_EXPORT void setCacheLimitKB(int kb);
ADQT_ICON_CORE_EXPORT void clearCache();

}  // namespace adqt::icons

#endif  // ADQT_ICON_REGISTRY_H
