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
