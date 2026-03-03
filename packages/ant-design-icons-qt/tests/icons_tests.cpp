#include <QtTest/QtTest>

#include <QBuffer>
#include <QCryptographicHash>
#include <QFile>
#include <QImage>
#include <QSvgRenderer>

#include "icons.h"

namespace {

QByteArray pngDigest(const QImage& image) {
  QByteArray bytes;
  QBuffer buffer(&bytes);
  buffer.open(QIODevice::WriteOnly);
  image.save(&buffer, "PNG");
  return QCryptographicHash::hash(bytes, QCryptographicHash::Sha1);
}

QColor averageVisibleColor(const QImage& image) {
  qint64 red = 0;
  qint64 green = 0;
  qint64 blue = 0;
  qint64 alpha = 0;
  qint64 count = 0;

  for (int y = 0; y < image.height(); ++y) {
    for (int x = 0; x < image.width(); ++x) {
      const QColor pixel = image.pixelColor(x, y);
      if (pixel.alpha() == 0) {
        continue;
      }
      red += pixel.red();
      green += pixel.green();
      blue += pixel.blue();
      alpha += pixel.alpha();
      ++count;
    }
  }

  if (count == 0) {
    return QColor();
  }
  return QColor(static_cast<int>(red / count),
                static_cast<int>(green / count),
                static_cast<int>(blue / count),
                static_cast<int>(alpha / count));
}

class IconsTests final : public QObject {
  Q_OBJECT

 private slots:
  void init() {
    adqt::icons::clearThemeResolver();
    adqt::icons::clearIconCache();
    adqt::icons::setPixmapCacheLimitKB(32 * 1024);
  }

  void outlinedApiReturnsIcon() {
    const adqt::icons::IconToken token = adqt::icons::outlined::Search();
    QVERIFY(adqt::icons::isValid(token));
    const QIcon icon = adqt::icons::makeIcon(token);
    QVERIFY(!icon.isNull());
    QVERIFY(QFile::exists(QStringLiteral(":/adqt/icons/outlined/search.svg")));
    QFile raw(QStringLiteral(":/adqt/icons/outlined/search.svg"));
    QVERIFY(raw.open(QIODevice::ReadOnly));
    QSvgRenderer renderer(raw.readAll());
    QVERIFY(renderer.isValid());
  }

  void twotoneApiReturnsIcon() {
    const adqt::icons::IconToken token = adqt::icons::twotone::Alert();
    const QIcon icon = adqt::icons::makeIcon(token);
    QVERIFY(!icon.isNull());
  }

  void renderedPixmapHasExpectedSize() {
    const QPixmap pm =
        adqt::icons::renderIconPixmap(adqt::icons::outlined::Download(), QSize(20, 20), 1.0,
                                      QIcon::Normal, QIcon::Off);
    QVERIFY(!pm.isNull());
    QCOMPARE(pm.deviceIndependentSize().toSize(), QSize(20, 20));
    QVERIFY(pm.devicePixelRatioF() >= 1.0);
  }

  void explicitDevicePixelRatioProducesHiDpiPixmap() {
    const QPixmap pm = adqt::icons::renderIconPixmap(adqt::icons::outlined::Download(),
                                                     QSize(18, 18), 2.0, QIcon::Normal, QIcon::Off);
    QVERIFY(!pm.isNull());
    QCOMPARE(pm.devicePixelRatioF(), 2.0);
    QCOMPARE(pm.deviceIndependentSize().toSize(), QSize(18, 18));
    QCOMPARE(pm.size(), QSize(36, 36));
  }

  void disabledModeDiffersFromNormal() {
    const QIcon icon = adqt::icons::makeIcon(adqt::icons::outlined::Search());
    const QImage normal = icon.pixmap(QSize(24, 24), QIcon::Normal, QIcon::Off).toImage();
    const QImage disabled = icon.pixmap(QSize(24, 24), QIcon::Disabled, QIcon::Off).toImage();

    QVERIFY(!normal.isNull());
    QVERIFY(!disabled.isNull());
    QCOMPARE(normal.size(), disabled.size());
  }

  void outlinedIconRespectsExplicitPrimaryColor() {
    adqt::icons::IconToken red = adqt::icons::outlined::Search();
    red.style.primary = QColor(QStringLiteral("#ff4d4f"));
    red.style.hasPrimary = true;

    adqt::icons::IconToken blue = adqt::icons::outlined::Search();
    blue.style.primary = QColor(QStringLiteral("#1677ff"));
    blue.style.hasPrimary = true;

    const QImage redImage =
        adqt::icons::renderIconPixmap(red, QSize(24, 24), 1.0, QIcon::Normal, QIcon::Off).toImage();
    const QImage blueImage =
        adqt::icons::renderIconPixmap(blue, QSize(24, 24), 1.0, QIcon::Normal, QIcon::Off).toImage();

    QVERIFY(!redImage.isNull());
    QVERIFY(!blueImage.isNull());
    QVERIFY(pngDigest(redImage) != pngDigest(blueImage));
  }

  void outlinedIconRespectsPrimaryAlpha() {
    adqt::icons::IconToken opaque = adqt::icons::outlined::Mail();
    opaque.style.primary = QColor(255, 255, 255, 255);
    opaque.style.hasPrimary = true;

    adqt::icons::IconToken translucent = adqt::icons::outlined::Mail();
    translucent.style.primary = QColor(255, 255, 255, 166);
    translucent.style.hasPrimary = true;

    const QImage opaqueImage =
        adqt::icons::renderIconPixmap(opaque, QSize(32, 32), 1.0, QIcon::Normal, QIcon::Off).toImage();
    const QImage translucentImage =
        adqt::icons::renderIconPixmap(translucent, QSize(32, 32), 1.0, QIcon::Normal, QIcon::Off).toImage();

    QVERIFY(!opaqueImage.isNull());
    QVERIFY(!translucentImage.isNull());

    const QColor opaqueAvg = averageVisibleColor(opaqueImage);
    const QColor translucentAvg = averageVisibleColor(translucentImage);
    QVERIFY(opaqueAvg.isValid());
    QVERIFY(translucentAvg.isValid());

    // Semi-transparent white should remain white-ish, not turn black.
    QVERIFY(translucentAvg.red() > 200);
    QVERIFY(translucentAvg.green() > 200);
    QVERIFY(translucentAvg.blue() > 200);
    QVERIFY(translucentAvg.alpha() < opaqueAvg.alpha());
  }

  void twotoneEmptySimpleRespectsExplicitTertiaryColor() {
    adqt::icons::IconToken base = adqt::icons::twotone::EmptySimple();
    base.style.primary = QColor(QStringLiteral("#d9d9d9"));
    base.style.hasPrimary = true;
    base.style.secondary = QColor(QStringLiteral("#fafafa"));
    base.style.hasSecondary = true;
    base.style.tertiary = QColor(QStringLiteral("#f5f5f5"));
    base.style.hasTertiary = true;

    adqt::icons::IconToken mutated = base;
    mutated.style.tertiary = QColor(QStringLiteral("#d9d9d9"));
    mutated.style.hasTertiary = true;

    const QImage first =
        adqt::icons::renderIconPixmap(base, QSize(64, 41), 1.0, QIcon::Normal, QIcon::Off).toImage();
    const QImage second =
        adqt::icons::renderIconPixmap(mutated, QSize(64, 41), 1.0, QIcon::Normal, QIcon::Off).toImage();

    QVERIFY(!first.isNull());
    QVERIFY(!second.isNull());
    QVERIFY(pngDigest(first) != pngDigest(second));
  }

  void themeResolverAffectsTwotone() {
    adqt::icons::setThemeResolver([] {
      adqt::icons::IconThemeSnapshot snapshot;
      snapshot.text = QColor(QStringLiteral("#1F1F1F"));
      snapshot.textDisabled = QColor(QStringLiteral("#BFBFBF"));
      snapshot.primary = QColor(QStringLiteral("#1677FF"));
      snapshot.twoToneSecondary = QColor(QStringLiteral("#E6F4FF"));
      snapshot.revision = 101;
      return snapshot;
    });

    const QImage first = adqt::icons::makeIcon(adqt::icons::twotone::Alert()).pixmap(QSize(24, 24)).toImage();

    adqt::icons::setThemeResolver([] {
      adqt::icons::IconThemeSnapshot snapshot;
      snapshot.text = QColor(QStringLiteral("#1F1F1F"));
      snapshot.textDisabled = QColor(QStringLiteral("#BFBFBF"));
      snapshot.primary = QColor(QStringLiteral("#EB2F96"));
      snapshot.twoToneSecondary = QColor(QStringLiteral("#FFF0F6"));
      snapshot.revision = 102;
      return snapshot;
    });

    const QImage second =
        adqt::icons::makeIcon(adqt::icons::twotone::Alert()).pixmap(QSize(24, 24)).toImage();
    QVERIFY(!first.isNull());
    QVERIFY(!second.isNull());
    QVERIFY(pngDigest(first) != pngDigest(second));
  }
};

}  // namespace

QTEST_MAIN(IconsTests)
#include "icons_tests.moc"
