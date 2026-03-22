#include <QApplication>
#include <QHBoxLayout>
#include <QMetaProperty>
#include <QSignalSpy>
#include <QTimer>
#include <QtTest>

#include "widgets/image.h"

namespace {

using adqt::widgets::AdImage;
using adqt::widgets::AdImageItem;
using adqt::widgets::AdImageItems;
using adqt::widgets::AdImageListModel;
using adqt::widgets::AdImageLoader;
using adqt::widgets::AdImageReply;
using adqt::widgets::AdImageViewer;

QString keyForUrl(const QUrl& url) {
  return url.toString(QUrl::FullyEncoded);
}

QImage makeImage(const QColor& color, const QSize& size = QSize(48, 32)) {
  QImage image(size, QImage::Format_ARGB32_Premultiplied);
  image.fill(color);
  return image;
}

class FakeImageReply final : public AdImageReply {
  Q_OBJECT

 public:
  explicit FakeImageReply(QObject* parent = nullptr) : AdImageReply(parent) {}

  void finishSuccess(const QImage& image) { succeed(image); }

  void finishFailure(const QString& errorString = QStringLiteral("load failed")) { fail(errorString); }

 public slots:
  void abort() override {
    if (!isFinished()) {
      fail(QStringLiteral("aborted"));
    }
  }
};

class FakeImageLoader final : public AdImageLoader {
  Q_OBJECT

 public:
  struct Result {
    bool success = false;
    QImage image;
    QString errorString = QStringLiteral("load failed");
  };

  explicit FakeImageLoader(QObject* parent = nullptr) : AdImageLoader(parent) {}

  void setResult(const QUrl& source, const Result& result) { results_.insert(keyForUrl(source), result); }

  void clearRequests() { requested_.clear(); }

  QStringList requests() const { return requested_; }

  AdImageReply* load(const QUrl& source, QObject* parent = nullptr) override {
    requested_.append(keyForUrl(source));

    auto* reply = new FakeImageReply(parent);
    const Result result = results_.value(keyForUrl(source));
    QTimer::singleShot(0, reply, [reply, result]() {
      if (result.success) {
        reply->finishSuccess(result.image);
      } else {
        reply->finishFailure(result.errorString);
      }
    });
    return reply;
  }

 private:
  QHash<QString, Result> results_;
  QStringList requested_;
};

class ImageTests final : public QObject {
  Q_OBJECT

 private slots:
  void exposesQtNativeUrlProperties();
  void listModelExposesQtRoles();
  void mainLoadUsesPlaceholderAndFallback();
  void standaloneViewerUsesImageLoaderAndSignals();
  void invalidPreviewItemsDoNotOpenViewer();
  void sharedViewerPrefersViewerLoaderAndTracksCurrentRow();
  void keyboardOpensAndClosesStandaloneViewer();
};

void ImageTests::exposesQtNativeUrlProperties() {
  AdImage image;
  const QMetaObject* meta = image.metaObject();

  const QMetaProperty sourceProperty = meta->property(meta->indexOfProperty("source"));
  QCOMPARE(sourceProperty.metaType().id(), QMetaType::QUrl);

  const QMetaProperty fallbackProperty = meta->property(meta->indexOfProperty("fallbackSource"));
  QCOMPARE(fallbackProperty.metaType().id(), QMetaType::QUrl);

  const QMetaProperty previewSourceProperty = meta->property(meta->indexOfProperty("previewSource"));
  QCOMPARE(previewSourceProperty.metaType().id(), QMetaType::QUrl);

  QVERIFY(image.setProperty("source", QVariant::fromValue(QUrl(QStringLiteral("https://example.com/a.png")))));
  QCOMPARE(image.source(), QUrl(QStringLiteral("https://example.com/a.png")));
}

void ImageTests::listModelExposesQtRoles() {
  AdImageListModel model;
  AdImageItem item;
  item.source = QUrl(QStringLiteral("https://example.com/model.png"));
  item.altText = QStringLiteral("model-alt");
  model.setItems(AdImageItems{item});

  QCOMPARE(model.rowCount(), 1);
  const QModelIndex index = model.index(0, 0);
  QCOMPARE(model.data(index, adqt::widgets::AdImageItemRoles::SourceRole).toUrl(), item.source);
  QCOMPARE(model.data(index, adqt::widgets::AdImageItemRoles::AltTextRole).toString(), item.altText);
  QCOMPARE(model.roleNames().value(adqt::widgets::AdImageItemRoles::SourceRole), QByteArray("source"));
  QCOMPARE(model.roleNames().value(adqt::widgets::AdImageItemRoles::AltTextRole), QByteArray("altText"));
}

void ImageTests::mainLoadUsesPlaceholderAndFallback() {
  FakeImageLoader loader;
  const QUrl source(QStringLiteral("https://example.com/main.png"));
  const QUrl fallback(QStringLiteral("https://example.com/fallback.png"));
  const QUrl placeholder(QStringLiteral("https://example.com/placeholder.png"));

  loader.setResult(source, FakeImageLoader::Result{false, QImage(), QStringLiteral("missing")});
  loader.setResult(fallback, FakeImageLoader::Result{true, makeImage(Qt::green, QSize(64, 40)), QString()});
  loader.setResult(placeholder,
                   FakeImageLoader::Result{true, makeImage(Qt::yellow, QSize(24, 24)), QString()});

  AdImage image;
  image.setImageLoader(&loader);
  image.setPlaceholderSource(placeholder);
  image.setFallbackSource(fallback);
  image.setSource(source);

  QTRY_VERIFY(!image.loading());
  QVERIFY(!image.loadFailed());
  QCOMPARE(image.sizeHint(), QSize(64, 40));

  const QStringList requests = loader.requests();
  QVERIFY(requests.contains(keyForUrl(placeholder)));
  QVERIFY(requests.contains(keyForUrl(source)));
  QVERIFY(requests.contains(keyForUrl(fallback)));
}

void ImageTests::standaloneViewerUsesImageLoaderAndSignals() {
  FakeImageLoader loader;
  const QUrl source(QStringLiteral("https://example.com/standalone.png"));
  loader.setResult(source, FakeImageLoader::Result{true, makeImage(Qt::red, QSize(60, 36)), QString()});

  QWidget host;
  auto* layout = new QHBoxLayout(&host);
  auto* image = new AdImage(&host);
  image->setImageLoader(&loader);
  image->setSource(source);
  layout->addWidget(image);
  host.show();
  QTest::qWait(1);

  QTRY_VERIFY(!image->loading());
  loader.clearRequests();

  AdImageViewer* viewer = image->ensureViewer();
  viewer->setOwnerWindow(&host);

  QSignalSpy visibleSpy(viewer, &AdImageViewer::visibleChanged);
  QSignalSpy currentItemSpy(viewer, &AdImageViewer::currentItemChanged);

  viewer->openAt(0);

  QTRY_VERIFY(viewer->isVisible());
  QTRY_VERIFY(currentItemSpy.count() >= 1);
  QCOMPARE(loader.requests(), QStringList{keyForUrl(source)});

  viewer->close();
  QTRY_VERIFY(!viewer->isVisible());
  QVERIFY(visibleSpy.count() >= 2);
}

void ImageTests::invalidPreviewItemsDoNotOpenViewer() {
  QWidget host;
  auto* layout = new QHBoxLayout(&host);
  auto* image = new AdImage(&host);
  layout->addWidget(image);
  host.show();
  QTest::qWait(1);

  AdImageItem invalidItem;
  image->setPreviewItems(AdImageItems{invalidItem});

  AdImageViewer* viewer = image->ensureViewer();
  viewer->setOwnerWindow(&host);
  viewer->openAt(0);
  QTest::qWait(10);
  QVERIFY(!viewer->isVisible());

  AdImageItem validItem;
  validItem.source = QUrl(QStringLiteral("https://example.com/valid.png"));
  validItem.altText = QStringLiteral("valid");
  image->setPreviewItems(AdImageItems{validItem});

  FakeImageLoader loader;
  loader.setResult(validItem.source,
                   FakeImageLoader::Result{true, makeImage(Qt::blue, QSize(32, 32)), QString()});
  image->setImageLoader(&loader);

  viewer->openAt(0);
  QTRY_VERIFY(viewer->isVisible());
  viewer->close();
  QTRY_VERIFY(!viewer->isVisible());
}

void ImageTests::sharedViewerPrefersViewerLoaderAndTracksCurrentRow() {
  const QUrl firstSource(QStringLiteral("https://example.com/group-1.png"));
  const QUrl secondSource(QStringLiteral("https://example.com/group-2.png"));

  FakeImageLoader firstImageLoader;
  FakeImageLoader secondImageLoader;
  FakeImageLoader viewerLoader;

  firstImageLoader.setResult(firstSource,
                             FakeImageLoader::Result{true, makeImage(Qt::gray, QSize(32, 32)), QString()});
  secondImageLoader.setResult(secondSource,
                              FakeImageLoader::Result{true, makeImage(Qt::darkGray, QSize(32, 32)), QString()});
  viewerLoader.setResult(firstSource,
                         FakeImageLoader::Result{true, makeImage(Qt::cyan, QSize(50, 30)), QString()});
  viewerLoader.setResult(secondSource,
                         FakeImageLoader::Result{true, makeImage(Qt::magenta, QSize(50, 30)), QString()});

  AdImageItem firstItem;
  firstItem.source = firstSource;
  firstItem.altText = QStringLiteral("first");
  AdImageItem secondItem;
  secondItem.source = secondSource;
  secondItem.altText = QStringLiteral("second");

  AdImageListModel model;
  model.setItems(AdImageItems{firstItem, secondItem});

  QWidget host;
  auto* layout = new QHBoxLayout(&host);
  auto* first = new AdImage(&host);
  auto* second = new AdImage(&host);
  AdImageViewer viewer;

  viewer.setModel(&model);
  viewer.setImageLoader(&viewerLoader);
  viewer.setOwnerWindow(&host);

  first->setImageLoader(&firstImageLoader);
  second->setImageLoader(&secondImageLoader);
  first->setSource(firstSource);
  second->setSource(secondSource);
  first->setViewer(&viewer);
  first->setPreviewRow(0);
  second->setViewer(&viewer);
  second->setPreviewRow(1);

  layout->addWidget(first);
  layout->addWidget(second);
  host.show();
  QTest::qWait(1);

  QTRY_VERIFY(!first->loading());
  QTRY_VERIFY(!second->loading());

  firstImageLoader.clearRequests();
  secondImageLoader.clearRequests();
  viewerLoader.clearRequests();

  QTest::mouseClick(first, Qt::LeftButton);
  QTRY_VERIFY(viewer.isVisible());
  QTRY_COMPARE(viewer.currentRow(), 0);
  QCOMPARE(viewerLoader.requests(), QStringList{keyForUrl(firstSource)});
  QVERIFY(firstImageLoader.requests().isEmpty());
  QVERIFY(secondImageLoader.requests().isEmpty());

  viewer.activate(1);
  QTRY_COMPARE(viewer.currentRow(), 1);
  QVERIFY(viewerLoader.requests().contains(keyForUrl(secondSource)));

  viewer.close();
  QTRY_VERIFY(!viewer.isVisible());
}

void ImageTests::keyboardOpensAndClosesStandaloneViewer() {
  FakeImageLoader loader;
  const QUrl source(QStringLiteral("https://example.com/keyboard.png"));
  loader.setResult(source, FakeImageLoader::Result{true, makeImage(Qt::green, QSize(40, 40)), QString()});

  QWidget host;
  auto* layout = new QHBoxLayout(&host);
  auto* image = new AdImage(&host);
  image->setImageLoader(&loader);
  image->setSource(source);
  layout->addWidget(image);
  host.show();
  QTest::qWait(1);
  QTRY_VERIFY(!image->loading());

  AdImageViewer* viewer = image->ensureViewer();
  viewer->setOwnerWindow(&host);

  image->setFocus();
  QVERIFY(image->hasFocus());

  QTest::keyClick(image, Qt::Key_Space);
  QTRY_VERIFY(viewer->isVisible());

  QWidget* focusWidget = QApplication::focusWidget();
  QVERIFY(focusWidget);
  QTest::keyClick(focusWidget, Qt::Key_Escape);
  QTRY_VERIFY(!viewer->isVisible());
}

}  // namespace

int runImageTests(int argc, char** argv) {
  ImageTests tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "image_tests.moc"
