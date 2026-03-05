#include "image.h"

#include "image_style.h"
#include "icons.h"
#include "theme/theme.h"

#include <QApplication>
#include <QCloseEvent>
#include <QEnterEvent>
#include <QEvent>
#include <QHash>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QShowEvent>
#include <QStyle>
#include <QStackedLayout>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

namespace adqt::widgets {

namespace {

namespace outlined_icons = adqt::icons::outlined;

struct PreviewItemData {
  QString src;
  QString alt;
  int width = -1;
  int height = -1;
};

using PixmapCallback = std::function<void(bool ok, const QPixmap& pixmap)>;

QHash<QString, QPixmap>& imageCache() {
  static QHash<QString, QPixmap> cache;
  return cache;
}

QNetworkAccessManager* sharedNetworkManager() {
  static QNetworkAccessManager* manager = new QNetworkAccessManager(qApp);
  return manager;
}

QString normalizeSourceKey(const QString& source) {
  const QString trimmed = source.trimmed();
  if (trimmed.isEmpty()) {
    return QString();
  }
  const QUrl url(trimmed);
  if (url.isLocalFile()) {
    return QUrl::fromLocalFile(url.toLocalFile()).toString();
  }
  return trimmed;
}

bool isRemoteSource(const QString& source) {
  const QUrl url(source);
  if (!url.isValid()) {
    return false;
  }
  const QString scheme = url.scheme().toLower();
  return scheme == QStringLiteral("http") || scheme == QStringLiteral("https");
}

bool decodeDataUrl(const QString& source, QByteArray* bytes) {
  if (!bytes) {
    return false;
  }

  const QString trimmed = source.trimmed();
  if (!trimmed.startsWith(QStringLiteral("data:"), Qt::CaseInsensitive)) {
    return false;
  }

  const int commaIndex = trimmed.indexOf(',');
  if (commaIndex <= 0) {
    return false;
  }

  const QString meta = trimmed.mid(5, commaIndex - 5);
  const QString payload = trimmed.mid(commaIndex + 1);
  if (meta.contains(QStringLiteral(";base64"), Qt::CaseInsensitive)) {
    *bytes = QByteArray::fromBase64(payload.toUtf8());
  } else {
    *bytes = QUrl::fromPercentEncoding(payload.toUtf8()).toUtf8();
  }
  return !bytes->isEmpty();
}

QPixmap loadPixmapSynchronously(const QString& source) {
  const QString trimmed = source.trimmed();
  if (trimmed.isEmpty()) {
    return QPixmap();
  }

  QByteArray dataBytes;
  if (decodeDataUrl(trimmed, &dataBytes)) {
    QPixmap pixmap;
    if (pixmap.loadFromData(dataBytes)) {
      return pixmap;
    }
    return QPixmap();
  }

  if (isRemoteSource(trimmed)) {
    return QPixmap();
  }

  QPixmap pixmap;
  if (pixmap.load(trimmed)) {
    return pixmap;
  }

  const QUrl url(trimmed);
  if (url.isLocalFile() && pixmap.load(url.toLocalFile())) {
    return pixmap;
  }

  return QPixmap();
}

void requestPixmap(const QString& source, QObject* context, PixmapCallback callback) {
  if (!callback) {
    return;
  }

  const QString key = normalizeSourceKey(source);
  if (key.isEmpty()) {
    callback(false, QPixmap());
    return;
  }

  const auto cacheIt = imageCache().constFind(key);
  if (cacheIt != imageCache().constEnd() && !cacheIt.value().isNull()) {
    callback(true, cacheIt.value());
    return;
  }

  const QPixmap localPixmap = loadPixmapSynchronously(key);
  if (!localPixmap.isNull()) {
    imageCache().insert(key, localPixmap);
    callback(true, localPixmap);
    return;
  }

  if (!isRemoteSource(key)) {
    callback(false, QPixmap());
    return;
  }

  QNetworkRequest request{QUrl(key)};
  QNetworkReply* reply = sharedNetworkManager()->get(request);
  QObject::connect(reply, &QNetworkReply::finished, context ? context : reply, [reply, key, callback]() {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
      callback(false, QPixmap());
      return;
    }

    const QByteArray bytes = reply->readAll();
    QPixmap pixmap;
    if (!pixmap.loadFromData(bytes)) {
      callback(false, QPixmap());
      return;
    }

    imageCache().insert(key, pixmap);
    callback(true, pixmap);
  });
}

QString colorToRgba(const QColor& color) {
  return QStringLiteral("rgba(%1,%2,%3,%4)")
      .arg(color.red())
      .arg(color.green())
      .arg(color.blue())
      .arg(color.alphaF(), 0, 'f', 3);
}

template <typename SlotStyle>
void mergeSlotStyle(const SlotStyle& source, SlotStyle* target) {
  if (!target) {
    return;
  }
  if (source.textColor.has_value()) {
    target->textColor = source.textColor;
  }
  if (source.backgroundColor.has_value()) {
    target->backgroundColor = source.backgroundColor;
  }
  if (source.borderColor.has_value()) {
    target->borderColor = source.borderColor;
  }
}

AdImage::SemanticStyles mergedImageSemanticStyles(const AdImage::SemanticStyles& base,
                                                  const AdImage::SemanticStyles& extra) {
  AdImage::SemanticStyles output = base;
  mergeSlotStyle(extra.root, &output.root);
  mergeSlotStyle(extra.image, &output.image);
  mergeSlotStyle(extra.cover, &output.cover);
  mergeSlotStyle(extra.popupRoot, &output.popupRoot);
  mergeSlotStyle(extra.popupMask, &output.popupMask);
  mergeSlotStyle(extra.popupBody, &output.popupBody);
  mergeSlotStyle(extra.popupFooter, &output.popupFooter);
  mergeSlotStyle(extra.popupActions, &output.popupActions);
  return output;
}

AdImagePreviewGroup::SemanticStyles mergedGroupSemanticStyles(
    const AdImagePreviewGroup::SemanticStyles& base,
    const AdImagePreviewGroup::SemanticStyles& extra) {
  AdImagePreviewGroup::SemanticStyles output = base;
  mergeSlotStyle(extra.popupRoot, &output.popupRoot);
  mergeSlotStyle(extra.popupMask, &output.popupMask);
  mergeSlotStyle(extra.popupBody, &output.popupBody);
  mergeSlotStyle(extra.popupFooter, &output.popupFooter);
  mergeSlotStyle(extra.popupActions, &output.popupActions);
  return output;
}

class ImagePreviewOverlay final : public QWidget {
 public:
  class RoundedBackgroundWidget final : public QWidget {
   public:
    explicit RoundedBackgroundWidget(QWidget* parent = nullptr) : QWidget(parent) {
      setAttribute(Qt::WA_TranslucentBackground, true);
      setAttribute(Qt::WA_NoSystemBackground, true);
      setAutoFillBackground(false);
    }

    void setBackgroundColor(const QColor& color) {
      if (backgroundColor_ == color) {
        return;
      }
      backgroundColor_ = color;
      update();
    }

    void setCornerRadius(qreal radius) {
      const qreal clamped = std::max(0.0, radius);
      if (qFuzzyCompare(cornerRadius_ + 1.0, clamped + 1.0)) {
        return;
      }
      cornerRadius_ = clamped;
      update();
    }

   protected:
    void paintEvent(QPaintEvent* event) override {
      Q_UNUSED(event);
      if (!backgroundColor_.isValid() || backgroundColor_.alpha() <= 0) {
        return;
      }

      QPainter painter(this);
      painter.setRenderHint(QPainter::Antialiasing, true);
      painter.setPen(Qt::NoPen);
      painter.setBrush(backgroundColor_);

      QRectF bounds = rect();
      bounds.adjust(0.5, 0.5, -0.5, -0.5);
      const qreal radius = std::min(cornerRadius_, bounds.height() * 0.5);
      painter.drawRoundedRect(bounds, radius, radius);
    }

   private:
    QColor backgroundColor_ = QColor(Qt::transparent);
    qreal cornerRadius_ = 0.0;
  };

  struct ButtonIconSpec {
    QToolButton* button = nullptr;
    adqt::icons::IconToken token;
    qreal rotationDegrees = 0.0;
  };

  explicit ImagePreviewOverlay(QWidget* parent = nullptr) : QWidget(parent) {
    setAttribute(Qt::WA_DeleteOnClose, false);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_StyledBackground, true);
    setAutoFillBackground(false);
    setFocusPolicy(Qt::StrongFocus);
    setStyleSheet(QStringLiteral("background:transparent;"));

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    container_ = new QWidget(this);
    container_->setAttribute(Qt::WA_StyledBackground, true);
    container_->setStyleSheet(QStringLiteral("background:transparent;"));
    rootLayout->addWidget(container_);

    maskLayer_ = new QWidget(container_);
    maskLayer_->setAttribute(Qt::WA_StyledBackground, true);
    maskLayer_->installEventFilter(this);

    closeButton_ = new QToolButton(container_);
    closeButton_->setAutoRaise(false);
    closeButton_->setCursor(Qt::PointingHandCursor);
    closeButton_->setToolTip(QStringLiteral("Close"));
    closeButtonSpec_.button = closeButton_;
    closeButtonSpec_.token = outlined_icons::Close();
    connect(closeButton_, &QToolButton::clicked, this, [this]() { close(); });

    prevButton_ = new QToolButton(container_);
    prevButton_->setAutoRaise(false);
    prevButton_->setCursor(Qt::PointingHandCursor);
    prevButton_->setToolTip(QStringLiteral("Previous"));
    prevButtonSpec_.button = prevButton_;
    prevButtonSpec_.token = outlined_icons::Left();
    connect(prevButton_, &QToolButton::clicked, this, [this]() { activate(-1); });

    nextButton_ = new QToolButton(container_);
    nextButton_->setAutoRaise(false);
    nextButton_->setCursor(Qt::PointingHandCursor);
    nextButton_->setToolTip(QStringLiteral("Next"));
    nextButtonSpec_.button = nextButton_;
    nextButtonSpec_.token = outlined_icons::Right();
    connect(nextButton_, &QToolButton::clicked, this, [this]() { activate(1); });

    bodyHost_ = new QWidget(container_);
    bodyHost_->setAttribute(Qt::WA_StyledBackground, true);
    bodyHost_->setStyleSheet(QStringLiteral("background:transparent;"));

    auto* bodyHostLayout = new QVBoxLayout(bodyHost_);
    bodyHostLayout->setContentsMargins(0, 0, 0, 0);
    bodyHostLayout->setSpacing(0);

    imageStack_ = new QStackedLayout();
    imageStack_->setContentsMargins(0, 0, 0, 0);
    imageStack_->setStackingMode(QStackedLayout::StackOne);

    loadingLabel_ = new QLabel(QStringLiteral("Loading image..."), bodyHost_);
    loadingLabel_->setAlignment(Qt::AlignCenter);
    imageStack_->addWidget(loadingLabel_);

    imageScrollArea_ = new QScrollArea(bodyHost_);
    imageScrollArea_->setFrameShape(QFrame::NoFrame);
    imageScrollArea_->setWidgetResizable(false);
    imageScrollArea_->setAlignment(Qt::AlignCenter);
    imageScrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    imageScrollArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    imageScrollArea_->setStyleSheet(QStringLiteral("background:transparent;"));

    imageLabel_ = new QLabel(imageScrollArea_);
    imageLabel_->setAlignment(Qt::AlignCenter);
    imageLabel_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    imageLabel_->setStyleSheet(QStringLiteral("background:transparent;"));
    imageScrollArea_->setWidget(imageLabel_);
    imageScrollArea_->viewport()->setStyleSheet(QStringLiteral("background:transparent;"));

    imageStack_->addWidget(imageScrollArea_);
    bodyHostLayout->addLayout(imageStack_);

    footer_ = new QWidget(container_);
    footer_->setAttribute(Qt::WA_StyledBackground, true);
    footer_->setStyleSheet(QStringLiteral("background:transparent;"));

    footerLayout_ = new QVBoxLayout(footer_);
    footerLayout_->setContentsMargins(0, 0, 0, 0);
    footerLayout_->setSpacing(16);

    countLabel_ = new QLabel(footer_);
    countLabel_->setAlignment(Qt::AlignCenter);
    footerLayout_->addWidget(countLabel_, 0, Qt::AlignHCenter);

    actions_ = new RoundedBackgroundWidget(footer_);
    actionsLayout_ = new QHBoxLayout(actions_);
    actionsLayout_->setContentsMargins(24, 0, 24, 0);
    actionsLayout_->setSpacing(12);

    auto createAction = [this](const adqt::icons::IconToken& token,
                               qreal rotationDegrees,
                               const QString& tooltip,
                               const std::function<void()>& fn) {
      auto* button = new QToolButton(actions_);
      button->setAutoRaise(false);
      button->setCursor(Qt::PointingHandCursor);
      button->setToolTip(tooltip);
      actionIconSpecs_.append(ButtonIconSpec{button, token, rotationDegrees});
      connect(button, &QToolButton::clicked, this, [fn]() {
        if (fn) {
          fn();
        }
      });
      actionsLayout_->addWidget(button);
      actionButtons_.append(button);
      return button;
    };

    flipYButton_ = createAction(outlined_icons::Swap(),
                                90.0,
                                QStringLiteral("Flip vertical"),
                                [this]() { flipY(); });
    flipXButton_ = createAction(outlined_icons::Swap(),
                                0.0,
                                QStringLiteral("Flip horizontal"),
                                [this]() { flipX(); });
    rotateLeftButton_ = createAction(outlined_icons::RotateLeft(),
                                     0.0,
                                     QStringLiteral("Rotate left"),
                                     [this]() { rotateLeft(); });
    rotateRightButton_ = createAction(outlined_icons::RotateRight(),
                                      0.0,
                                      QStringLiteral("Rotate right"),
                                      [this]() { rotateRight(); });
    zoomOutButton_ = createAction(outlined_icons::ZoomOut(),
                                  0.0,
                                  QStringLiteral("Zoom out"),
                                  [this]() { zoomOut(); });
    zoomInButton_ = createAction(outlined_icons::ZoomIn(),
                                 0.0,
                                 QStringLiteral("Zoom in"),
                                 [this]() { zoomIn(); });

    footerLayout_->addWidget(actions_, 0, Qt::AlignHCenter);

    applyVisualStyle();
    updateOverlayLayout();
  }

  ~ImagePreviewOverlay() override {
    if (hostWindow_) {
      hostWindow_->removeEventFilter(this);
    }
  }

  std::function<void(bool open)> visibilityChanged;
  std::function<void(int current, int previous)> currentChanged;
  std::function<void(int current, int total, const QString& src, const QString& alt, int width, int height)>
      imageInfoChanged;

  void setItems(const QVector<PreviewItemData>& items) {
    items_ = items;
    if (items_.isEmpty()) {
      currentIndex_ = 0;
      basePixmap_ = QPixmap();
      imageLabel_->clear();
      imageStack_->setCurrentWidget(loadingLabel_);
      updateControls();
      return;
    }

    const int clamped = std::clamp(currentIndex_, 0, static_cast<int>(items_.size() - 1));
    setCurrentIndex(clamped, false);
  }

  void setCurrentIndex(int index, bool emitChange) {
    if (items_.isEmpty()) {
      return;
    }

    const int next = std::clamp(index, 0, static_cast<int>(items_.size() - 1));
    const int previous = currentIndex_;
    if (next == currentIndex_ && !basePixmap_.isNull()) {
      updateControls();
      return;
    }

    currentIndex_ = next;
    resetTransform();
    loadCurrentItem();
    updateControls();

    if (emitChange && previous != currentIndex_ && currentChanged) {
      currentChanged(currentIndex_, previous);
    }
  }

  int currentIndex() const { return currentIndex_; }

  int totalCount() const { return items_.size(); }

  QString currentSource() const {
    if (items_.isEmpty() || currentIndex_ < 0 || currentIndex_ >= items_.size()) {
      return QString();
    }
    return items_.at(currentIndex_).src;
  }

  void setCountRenderFormat(const QString& value) {
    countRenderFormat_ = value;
    updateControls();
  }

  void setScaleStep(double value) { scaleStep_ = std::clamp(value, 0.05, 10.0); }

  void setMaskVisible(bool value) {
    maskVisible_ = value;
    applyVisualStyle();
  }

  void setMaskBlur(bool value) {
    maskBlur_ = value;
    applyVisualStyle();
  }

  void setVisualStyle(const detail::ImageVisualStyle& style) {
    visualStyle_ = style;
    applyVisualStyle();
  }

  void openFor(QWidget* owner) {
    QWidget* ownerWindow = owner ? owner->window() : nullptr;
    if (!ownerWindow) {
      ownerWindow = qobject_cast<QWidget*>(qApp->activeWindow());
    }

    if (!ownerWindow) {
      return;
    }

    attachToHostWindow(ownerWindow);
    syncToHostGeometry();
    show();
    raise();
    updateOverlayLayout();
    applyCurrentTransform();
    setFocus(Qt::OtherFocusReason);
  }

  void activate(int delta) {
    if (items_.size() <= 1 || delta == 0) {
      return;
    }
    const int next = currentIndex_ + delta;
    setCurrentIndex(next, true);
  }

  void zoomIn() {
    scale_ = std::min(50.0, scale_ + scaleStep_);
    applyCurrentTransform();
    updateControls();
  }

  void zoomOut() {
    scale_ = std::max(1.0, scale_ - scaleStep_);
    applyCurrentTransform();
    updateControls();
  }

  void rotateLeft() {
    rotation_ -= 90;
    applyCurrentTransform();
  }

  void rotateRight() {
    rotation_ += 90;
    applyCurrentTransform();
  }

  void flipX() {
    flipX_ = !flipX_;
    applyCurrentTransform();
  }

  void flipY() {
    flipY_ = !flipY_;
    applyCurrentTransform();
  }

  void resetTransform() {
    scale_ = 1.0;
    rotation_ = 0;
    flipX_ = false;
    flipY_ = false;
    applyCurrentTransform();
    updateControls();
  }

 protected:
  bool eventFilter(QObject* watched, QEvent* event) override {
    if (watched == maskLayer_ && event && event->type() == QEvent::MouseButtonPress) {
      close();
      return true;
    }

    if (watched == hostWindow_ && event) {
      switch (event->type()) {
        case QEvent::Resize:
        case QEvent::Move:
        case QEvent::Show:
          syncToHostGeometry();
          updateOverlayLayout();
          applyCurrentTransform();
          break;
        case QEvent::Hide:
          close();
          break;
        default:
          break;
      }
    }

    return QWidget::eventFilter(watched, event);
  }

  void resizeEvent(QResizeEvent* event) override {
    QWidget::resizeEvent(event);
    updateOverlayLayout();
    applyCurrentTransform();
  }

  void wheelEvent(QWheelEvent* event) override {
    if (event->angleDelta().y() > 0) {
      zoomIn();
    } else if (event->angleDelta().y() < 0) {
      zoomOut();
    }
    event->accept();
  }

  void keyPressEvent(QKeyEvent* event) override {
    if (!event) {
      return;
    }
    switch (event->key()) {
      case Qt::Key_Escape:
        close();
        return;
      case Qt::Key_Left:
        activate(-1);
        return;
      case Qt::Key_Right:
        activate(1);
        return;
      case Qt::Key_Plus:
      case Qt::Key_Equal:
        zoomIn();
        return;
      case Qt::Key_Minus:
      case Qt::Key_Underscore:
        zoomOut();
        return;
      default:
        break;
    }
    QWidget::keyPressEvent(event);
  }

  void closeEvent(QCloseEvent* event) override {
    QWidget::closeEvent(event);
    if (visibilityChanged) {
      visibilityChanged(false);
    }
  }

  void showEvent(QShowEvent* event) override {
    QWidget::showEvent(event);
    syncToHostGeometry();
    updateOverlayLayout();
    applyCurrentTransform();
    if (visibilityChanged) {
      visibilityChanged(true);
    }
  }

 private:
  static adqt::icons::IconToken withPrimaryColor(adqt::icons::IconToken token, const QColor& color) {
    if (!token.isValid() || !color.isValid()) {
      return token;
    }
    token.style.primary = color;
    token.style.hasPrimary = true;
    return token;
  }

  static QPixmap rotatedIconPixmap(const QPixmap& source, qreal degrees) {
    if (source.isNull() || qFuzzyIsNull(std::fmod(std::abs(degrees), 360.0))) {
      return source;
    }

    QTransform transform;
    transform.rotate(degrees);
    QPixmap rotated = source.transformed(transform, Qt::SmoothTransformation);
    if (!rotated.isNull()) {
      rotated.setDevicePixelRatio(source.devicePixelRatio());
    }
    return rotated;
  }

  QPixmap renderButtonIconPixmap(const ButtonIconSpec& spec,
                                 const QColor& color,
                                 const QSize& logicalSize,
                                 QIcon::Mode mode) const {
    if (!spec.button || !spec.token.isValid()) {
      return QPixmap();
    }

    const adqt::icons::IconToken coloredToken = withPrimaryColor(spec.token, color);
    const qreal dpr = std::max(1.0, devicePixelRatioF());
    QPixmap pixmap = adqt::icons::renderIconPixmap(coloredToken, logicalSize, dpr, mode, QIcon::Off);
    if (pixmap.isNull()) {
      return pixmap;
    }
    return rotatedIconPixmap(pixmap, spec.rotationDegrees);
  }

  void applyButtonIconStyle(const ButtonIconSpec& spec,
                            const QSize& logicalSize,
                            const QColor& normalColor,
                            const QColor& activeColor,
                            const QColor& disabledColor) {
    if (!spec.button) {
      return;
    }
    if (!spec.token.isValid()) {
      spec.button->setIcon(QIcon());
      return;
    }

    QIcon icon;

    const QPixmap normal = renderButtonIconPixmap(spec, normalColor, logicalSize, QIcon::Normal);
    if (!normal.isNull()) {
      icon.addPixmap(normal, QIcon::Normal, QIcon::Off);
    }

    const QColor resolvedActiveColor = activeColor.isValid() ? activeColor : normalColor;
    const QPixmap active = renderButtonIconPixmap(spec, resolvedActiveColor, logicalSize, QIcon::Active);
    if (!active.isNull()) {
      icon.addPixmap(active, QIcon::Active, QIcon::Off);
    }

    const QColor resolvedDisabledColor = disabledColor.isValid() ? disabledColor : normalColor;
    const QPixmap disabled = renderButtonIconPixmap(spec, resolvedDisabledColor, logicalSize, QIcon::Disabled);
    if (!disabled.isNull()) {
      icon.addPixmap(disabled, QIcon::Disabled, QIcon::Off);
    }

    if (!icon.isNull()) {
      spec.button->setIcon(icon);
      return;
    }

    spec.button->setIcon(adqt::icons::makeIcon(spec.token));
  }

  void attachToHostWindow(QWidget* hostWindow) {
    if (hostWindow_ == hostWindow) {
      return;
    }

    if (hostWindow_) {
      hostWindow_->removeEventFilter(this);
    }

    hostWindow_ = hostWindow;
    if (!hostWindow_) {
      return;
    }

    hostWindow_->installEventFilter(this);
    if (parentWidget() != hostWindow_) {
      setParent(hostWindow_);
    }
  }

  void syncToHostGeometry() {
    if (!hostWindow_) {
      return;
    }
    setGeometry(hostWindow_->rect());
  }

  void loadCurrentItem() {
    if (items_.isEmpty() || currentIndex_ < 0 || currentIndex_ >= items_.size()) {
      return;
    }

    const int token = ++loadToken_;
    imageStack_->setCurrentWidget(loadingLabel_);
    loadingLabel_->setText(QStringLiteral("Loading image..."));

    const PreviewItemData item = items_.at(currentIndex_);
    requestPixmap(item.src, this, [this, token, item](bool ok, const QPixmap& pixmap) {
      if (token != loadToken_) {
        return;
      }

      if (!ok || pixmap.isNull()) {
        basePixmap_ = QPixmap();
        imageLabel_->clear();
        loadingLabel_->setText(QStringLiteral("Failed to load image"));
        imageStack_->setCurrentWidget(loadingLabel_);
        if (imageInfoChanged) {
          imageInfoChanged(currentIndex_, items_.size(), item.src, item.alt, item.width, item.height);
        }
        updateControls();
        return;
      }

      basePixmap_ = pixmap;
      imageStack_->setCurrentWidget(imageScrollArea_);
      applyCurrentTransform();
      if (imageInfoChanged) {
        imageInfoChanged(currentIndex_,
                         items_.size(),
                         item.src,
                         item.alt,
                         pixmap.width(),
                         pixmap.height());
      }
      updateControls();
    });
  }

  void applyCurrentTransform() {
    if (basePixmap_.isNull()) {
      return;
    }

    QImage image = basePixmap_.toImage();
    QTransform transform;
    transform.scale(flipX_ ? -1.0 : 1.0, flipY_ ? -1.0 : 1.0);
    transform.rotate(rotation_);
    image = image.transformed(transform, Qt::SmoothTransformation);
    if (image.isNull()) {
      return;
    }

    QSize available = imageScrollArea_->viewport()->size();
    available -= QSize(16, 16);
    available.setWidth(std::max(available.width(), 1));
    available.setHeight(std::max(available.height(), 1));

    const qreal fitScale = std::min(static_cast<qreal>(available.width()) / image.width(),
                                    static_cast<qreal>(available.height()) / image.height());
    const qreal finalScale = std::max(0.01, fitScale * scale_);

    const QSize finalSize(std::max(1, static_cast<int>(std::round(image.width() * finalScale))),
                          std::max(1, static_cast<int>(std::round(image.height() * finalScale))));
    const QPixmap display = QPixmap::fromImage(
        image.scaled(finalSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));

    imageLabel_->setPixmap(display);
    imageLabel_->setFixedSize(display.size());
  }

  void updateControls() {
    const int total = items_.size();
    const int current = currentIndex_ + 1;

    QString format = countRenderFormat_.trimmed();
    if (format.isEmpty()) {
      format = QStringLiteral("%1 / %2");
    }

    if (total > 1) {
      countLabel_->setText(format.arg(current).arg(total));
      countLabel_->show();
    } else {
      countLabel_->hide();
    }

    const bool canNavigate = total > 1;
    prevButton_->setEnabled(canNavigate && currentIndex_ > 0);
    nextButton_->setEnabled(canNavigate && currentIndex_ < total - 1);
    prevButton_->setVisible(canNavigate);
    nextButton_->setVisible(canNavigate);
    zoomOutButton_->setEnabled(scale_ > 1.001);
    zoomInButton_->setEnabled(scale_ < 49.999);

    updateOverlayLayout();
  }

  void applyVisualStyle() {
    QColor maskColor = visualStyle_.popupMask;
    if (!maskVisible_) {
      maskColor = QColor(Qt::transparent);
      maskLayer_->setVisible(false);
    } else {
      maskLayer_->setVisible(true);
      if (maskBlur_) {
        maskColor.setAlpha(std::max(maskColor.alpha(), 176));
      }
    }

    maskLayer_->setStyleSheet(QStringLiteral("background-color:%1;").arg(colorToRgba(maskColor)));
    bodyHost_->setStyleSheet(QStringLiteral("background:transparent;"));
    loadingLabel_->setStyleSheet(QStringLiteral("color:%1; font-size:14px;")
                                     .arg(colorToRgba(visualStyle_.popupFooterText)));
    countLabel_->setStyleSheet(
        QStringLiteral("color:%1; font-size:14px;").arg(colorToRgba(visualStyle_.operationColor)));

    const int iconSize = std::max(10, visualStyle_.metrics.operationIconSize);
    const int basePadding = std::max(4, visualStyle_.metrics.footerPadding);
    const int closeSize = std::max(iconSize + basePadding * 2, visualStyle_.metrics.closeButtonSize);
    const int switchSize = std::max(iconSize + basePadding * 2, visualStyle_.metrics.switchButtonSize);
    const int actionSize = iconSize + basePadding * 2;
    const int actionsHorizontalPadding = std::max(0, visualStyle_.metrics.actionsHorizontalPadding);
    const int actionsGap = std::max(0, visualStyle_.metrics.actionsGap);
    const int footerGap = std::max(0, visualStyle_.metrics.footerGap);
    const int controlOffset = std::max(4, visualStyle_.metrics.controlOffset);
    const int footerBottomOffset = std::max(controlOffset, visualStyle_.metrics.footerBottomOffset);

    actionPaddingPx_ = basePadding;
    closeButtonSizePx_ = closeSize;
    switchButtonSizePx_ = switchSize;
    controlOffsetPx_ = controlOffset;
    footerBottomOffsetPx_ = footerBottomOffset;
    footerGapPx_ = footerGap;

    QColor operationBg = visualStyle_.popupActionsBackground;
    if (operationBg.alpha() <= 0) {
      operationBg = QColor(0, 0, 0, 28);
    }
    QColor operationBgHover = operationBg;
    operationBgHover.setAlpha(std::min(255, std::max(operationBg.alpha() + 24, operationBg.alpha() * 2)));
    QColor floatingColor = visualStyle_.popupFooterText;
    if (floatingColor.alpha() <= 0) {
      floatingColor = QColor(255, 255, 255);
    }

    auto applyFloatingButtonStyle = [&](QToolButton* button, int sizePx) {
      if (!button) {
        return;
      }
      button->setIconSize(QSize(iconSize, iconSize));
      button->setFixedSize(sizePx, sizePx);
      const QString style = QStringLiteral(
                                "QToolButton {"
                                "color:%1;"
                                "background-color:%2;"
                                "border:none;"
                                "border-radius:%3px;"
                                "padding:0px;"
                                "}"
                                "QToolButton:hover {"
                                "background-color:%4;"
                                "}"
                                "QToolButton:pressed {"
                                "background-color:%5;"
                                "}"
                                "QToolButton:disabled {"
                                "color:%6;"
                                "background-color:transparent;"
                                "}")
                                .arg(colorToRgba(floatingColor))
                                .arg(colorToRgba(operationBg))
                                .arg(sizePx / 2)
                                .arg(colorToRgba(operationBgHover))
                                .arg(colorToRgba(operationBg))
                                .arg(colorToRgba(visualStyle_.operationDisabledColor));
      button->setStyleSheet(style);
    };

    auto applyActionStyle = [&](QToolButton* button) {
      if (!button) {
        return;
      }
      button->setIconSize(QSize(iconSize, iconSize));
      button->setFixedSize(actionSize, actionSize);
      const QString style = QStringLiteral(
                                "QToolButton {"
                                "color:%1;"
                                "background-color:transparent;"
                                "border:none;"
                                "padding:%2px;"
                                "}"
                                "QToolButton:hover {"
                                "color:%3;"
                                "}"
                                "QToolButton:pressed {"
                                "color:%1;"
                                "}"
                                "QToolButton:disabled {"
                                "color:%4;"
                                "}")
                                .arg(colorToRgba(visualStyle_.operationColor))
                                .arg(basePadding)
                                .arg(colorToRgba(visualStyle_.operationHoverColor))
                                .arg(colorToRgba(visualStyle_.operationDisabledColor));
      button->setStyleSheet(style);
    };

    applyFloatingButtonStyle(closeButton_, closeSize);
    applyFloatingButtonStyle(prevButton_, switchSize);
    applyFloatingButtonStyle(nextButton_, switchSize);

    const QSize iconLogicalSize(iconSize, iconSize);
    applyButtonIconStyle(closeButtonSpec_,
                         iconLogicalSize,
                         floatingColor,
                         floatingColor,
                         visualStyle_.operationDisabledColor);
    applyButtonIconStyle(prevButtonSpec_,
                         iconLogicalSize,
                         floatingColor,
                         floatingColor,
                         visualStyle_.operationDisabledColor);
    applyButtonIconStyle(nextButtonSpec_,
                         iconLogicalSize,
                         floatingColor,
                         floatingColor,
                         visualStyle_.operationDisabledColor);

    for (QToolButton* button : actionButtons_) {
      applyActionStyle(button);
    }
    for (const ButtonIconSpec& spec : actionIconSpecs_) {
      applyButtonIconStyle(spec,
                           iconLogicalSize,
                           visualStyle_.operationColor,
                           visualStyle_.operationHoverColor,
                           visualStyle_.operationDisabledColor);
    }

    if (actionsLayout_) {
      actionsLayout_->setContentsMargins(actionsHorizontalPadding, 0, actionsHorizontalPadding, 0);
      actionsLayout_->setSpacing(actionsGap);
    }
    if (footerLayout_) {
      footerLayout_->setSpacing(footerGap);
    }

    actions_->setBackgroundColor(operationBg);
    actions_->setCornerRadius(100.0);

    updateControls();
    updateOverlayLayout();
  }

  void updateOverlayLayout() {
    if (!container_) {
      return;
    }

    const QRect bounds = container_->rect();
    if (bounds.isEmpty()) {
      return;
    }

    if (maskLayer_) {
      maskLayer_->setGeometry(bounds);
      maskLayer_->lower();
    }

    const int controlOffset = std::max(4, controlOffsetPx_);
    const int spacing = std::max(8, actionPaddingPx_);

    QSize footerSize;
    if (footer_) {
      footer_->adjustSize();
      footerSize = footer_->sizeHint();
      footer_->setFixedSize(footerSize);
      const int footerX = (bounds.width() - footerSize.width()) / 2;
      const int footerY = bounds.height() - footerBottomOffsetPx_ - footerSize.height();
      footer_->move(std::max(0, footerX), std::max(0, footerY));
    }

    if (closeButton_) {
      const int closeX = bounds.width() - controlOffset - closeButtonSizePx_;
      closeButton_->move(std::max(0, closeX), controlOffset);
    }

    const int switchY = (bounds.height() - switchButtonSizePx_) / 2;
    if (prevButton_) {
      prevButton_->move(controlOffset, std::max(0, switchY));
    }
    if (nextButton_) {
      const int nextX = bounds.width() - controlOffset - switchButtonSizePx_;
      nextButton_->move(std::max(0, nextX), std::max(0, switchY));
    }

    int sideInset = controlOffset;
    if (prevButton_ && prevButton_->isVisible()) {
      sideInset = std::max(sideInset, controlOffset + switchButtonSizePx_ + spacing);
    }
    if (nextButton_ && nextButton_->isVisible()) {
      sideInset = std::max(sideInset, controlOffset + switchButtonSizePx_ + spacing);
    }

    const int bottomInset =
        footerBottomOffsetPx_ + (footer_ && footer_->isVisible() ? footer_->height() + footerGapPx_
                                                                  : footerGapPx_);

    QRect bodyRect = bounds.adjusted(sideInset, controlOffset, -sideInset, -bottomInset);
    if (bodyRect.width() <= 0 || bodyRect.height() <= 0) {
      bodyRect = bounds.adjusted(controlOffset, controlOffset, -controlOffset, -controlOffset);
    }
    if (bodyHost_) {
      bodyHost_->setGeometry(bodyRect);
      bodyHost_->raise();
    }

    // Match Ant Design layering: controls stay above preview body.
    if (footer_) {
      footer_->raise();
    }
    if (closeButton_) {
      closeButton_->raise();
    }
    if (prevButton_) {
      prevButton_->raise();
    }
    if (nextButton_) {
      nextButton_->raise();
    }
  }

  QVector<PreviewItemData> items_;
  int currentIndex_ = 0;
  int loadToken_ = 0;
  QString countRenderFormat_ = QStringLiteral("%1 / %2");
  qreal scale_ = 1.0;
  qreal scaleStep_ = 0.5;
  int rotation_ = 0;
  bool flipX_ = false;
  bool flipY_ = false;
  bool maskVisible_ = true;
  bool maskBlur_ = false;
  int actionPaddingPx_ = 12;
  int closeButtonSizePx_ = 42;
  int switchButtonSizePx_ = 42;
  int controlOffsetPx_ = 12;
  int footerBottomOffsetPx_ = 32;
  int footerGapPx_ = 16;
  QPixmap basePixmap_;
  detail::ImageVisualStyle visualStyle_;
  QPointer<QWidget> hostWindow_;

  QWidget* container_ = nullptr;
  QWidget* maskLayer_ = nullptr;
  QWidget* bodyHost_ = nullptr;
  QLabel* countLabel_ = nullptr;
  QLabel* loadingLabel_ = nullptr;
  QLabel* imageLabel_ = nullptr;
  QStackedLayout* imageStack_ = nullptr;
  QScrollArea* imageScrollArea_ = nullptr;
  QWidget* footer_ = nullptr;
  QVBoxLayout* footerLayout_ = nullptr;
  RoundedBackgroundWidget* actions_ = nullptr;
  QHBoxLayout* actionsLayout_ = nullptr;
  QToolButton* closeButton_ = nullptr;
  QToolButton* prevButton_ = nullptr;
  QToolButton* nextButton_ = nullptr;
  QToolButton* zoomOutButton_ = nullptr;
  QToolButton* zoomInButton_ = nullptr;
  QToolButton* rotateLeftButton_ = nullptr;
  QToolButton* rotateRightButton_ = nullptr;
  QToolButton* flipXButton_ = nullptr;
  QToolButton* flipYButton_ = nullptr;
  ButtonIconSpec closeButtonSpec_;
  ButtonIconSpec prevButtonSpec_;
  ButtonIconSpec nextButtonSpec_;
  QVector<ButtonIconSpec> actionIconSpecs_;
  QList<QToolButton*> actionButtons_;
};
ImagePreviewOverlay* asPreviewOverlay(const QPointer<QWidget>& dialog) {
  return dynamic_cast<ImagePreviewOverlay*>(dialog.data());
}

QRect coverRectForPlacement(const QRect& bounds, AdImage::CoverPlacement placement) {
  if (placement == AdImage::CoverPlacement::Center) {
    return bounds;
  }

  const int stripHeight = std::max(28, std::min(44, bounds.height() / 3));
  if (placement == AdImage::CoverPlacement::Top) {
    return QRect(bounds.left(), bounds.top(), bounds.width(), stripHeight);
  }
  return QRect(bounds.left(), bounds.bottom() - stripHeight + 1, bounds.width(), stripHeight);
}

}  // namespace

AdImagePreviewGroup::AdImagePreviewGroup(QObject* parent) : QObject(parent) {
  connect(&adqt::theme::ThemeManager::instance(), &adqt::theme::ThemeManager::themeChanged, this,
          [this]() { refreshDialogVisualState(); });
}

AdImagePreviewGroup::~AdImagePreviewGroup() {
  while (!attachedImages_.isEmpty()) {
    AdImage* image = attachedImages_.takeLast();
    if (image && image->previewGroup_ == this) {
      image->previewGroup_ = nullptr;
      emit image->previewGroupChanged(nullptr);
      image->update();
    }
  }

  if (previewDialog_) {
    previewDialog_->close();
    previewDialog_->deleteLater();
  }
}

QStringList AdImagePreviewGroup::items() const { return items_; }

void AdImagePreviewGroup::setItems(const QStringList& value) {
  if (items_ == value) {
    return;
  }
  items_ = value;
  emit itemsChanged(items_);
  if (current_ >= items_.size() && !items_.isEmpty()) {
    current_ = items_.size() - 1;
    emit currentChanged(current_);
  }
  refreshDialogVisualState();
}

bool AdImagePreviewGroup::open() const { return open_; }

void AdImagePreviewGroup::setOpen(bool value) {
  if (open_ == value && value == false) {
    return;
  }
  if (!value) {
    closePreview();
    return;
  }

  if (!previewEnabled_) {
    return;
  }

  AdImage* preferred = nullptr;
  int visibleIndex = 0;
  if (items_.isEmpty()) {
    int imageIndex = 0;
    for (const QPointer<AdImage>& imagePtr : attachedImages_) {
      if (!imagePtr) {
        continue;
      }
      const QString source = imagePtr->effectivePreviewSource();
      if (source.trimmed().isEmpty()) {
        continue;
      }
      if (imageIndex == current_) {
        preferred = imagePtr;
        break;
      }
      ++imageIndex;
    }
  } else if (!items_.isEmpty()) {
    visibleIndex = std::clamp(current_, 0, static_cast<int>(items_.size() - 1));
  }

  handleImageActivated(preferred);
  if (ImagePreviewOverlay* dialog = asPreviewOverlay(previewDialog_)) {
    dialog->setCurrentIndex(items_.isEmpty() ? current_ : visibleIndex, false);
  }
}

int AdImagePreviewGroup::current() const { return current_; }

void AdImagePreviewGroup::setCurrent(int value) {
  if (value < 0) {
    value = 0;
  }
  if (current_ == value) {
    return;
  }
  const int previous = current_;
  current_ = value;
  emit currentChanged(current_);
  emit onChange(current_, previous);

  if (ImagePreviewOverlay* dialog = asPreviewOverlay(previewDialog_)) {
    dialog->setCurrentIndex(current_, false);
  }
}

QString AdImagePreviewGroup::countRenderFormat() const { return countRenderFormat_; }

void AdImagePreviewGroup::setCountRenderFormat(const QString& value) {
  if (countRenderFormat_ == value) {
    return;
  }
  countRenderFormat_ = value;
  emit countRenderFormatChanged(countRenderFormat_);
  refreshDialogVisualState();
}

bool AdImagePreviewGroup::previewEnabled() const { return previewEnabled_; }

void AdImagePreviewGroup::setPreviewEnabled(bool value) {
  if (previewEnabled_ == value) {
    return;
  }
  previewEnabled_ = value;
  emit previewEnabledChanged(previewEnabled_);
  if (!previewEnabled_) {
    closePreview();
  }
}

bool AdImagePreviewGroup::previewMaskVisible() const { return previewMaskVisible_; }

void AdImagePreviewGroup::setPreviewMaskVisible(bool value) {
  if (previewMaskVisible_ == value) {
    return;
  }
  previewMaskVisible_ = value;
  emit previewMaskVisibleChanged(previewMaskVisible_);
  refreshDialogVisualState();
}

bool AdImagePreviewGroup::previewMaskBlur() const { return previewMaskBlur_; }

void AdImagePreviewGroup::setPreviewMaskBlur(bool value) {
  if (previewMaskBlur_ == value) {
    return;
  }
  previewMaskBlur_ = value;
  emit previewMaskBlurChanged(previewMaskBlur_);
  refreshDialogVisualState();
}

double AdImagePreviewGroup::scaleStep() const { return scaleStep_; }

void AdImagePreviewGroup::setScaleStep(double value) {
  const double clamped = std::clamp(value, 0.05, 10.0);
  if (qFuzzyCompare(scaleStep_ + 1.0, clamped + 1.0)) {
    return;
  }
  scaleStep_ = clamped;
  emit scaleStepChanged(scaleStep_);
  if (ImagePreviewOverlay* dialog = asPreviewOverlay(previewDialog_)) {
    dialog->setScaleStep(scaleStep_);
  }
}

AdImagePreviewGroup::ComponentTokens AdImagePreviewGroup::componentTokens() const { return componentTokens_; }

void AdImagePreviewGroup::setComponentTokens(const ComponentTokens& tokens) {
  componentTokens_ = tokens;
  emit componentTokensChanged();
  refreshDialogVisualState();
}

void AdImagePreviewGroup::resetComponentTokens() {
  componentTokens_ = ComponentTokens{};
  emit componentTokensChanged();
  refreshDialogVisualState();
}

AdImagePreviewGroup::SemanticStyles AdImagePreviewGroup::semanticStyles() const { return semanticStyles_; }

void AdImagePreviewGroup::setSemanticStyles(const SemanticStyles& styles) {
  semanticStyles_ = styles;
  emit semanticStylesChanged();
  refreshDialogVisualState();
}

void AdImagePreviewGroup::setSemanticStyleResolver(SemanticStyleResolver resolver) {
  semanticStyleResolver_ = std::move(resolver);
  emit semanticStylesChanged();
  refreshDialogVisualState();
}

QString AdImagePreviewGroup::currentSource() const { return currentSource_; }

int AdImagePreviewGroup::totalCount() const {
  if (!items_.isEmpty()) {
    return items_.size();
  }
  int total = 0;
  for (const QPointer<AdImage>& imagePtr : attachedImages_) {
    if (imagePtr && !imagePtr->effectivePreviewSource().trimmed().isEmpty()) {
      ++total;
    }
  }
  return total;
}

void AdImagePreviewGroup::activate(int delta) {
  if (ImagePreviewOverlay* dialog = asPreviewOverlay(previewDialog_)) {
    dialog->activate(delta);
  }
}

void AdImagePreviewGroup::zoomIn() {
  if (ImagePreviewOverlay* dialog = asPreviewOverlay(previewDialog_)) {
    dialog->zoomIn();
  }
}

void AdImagePreviewGroup::zoomOut() {
  if (ImagePreviewOverlay* dialog = asPreviewOverlay(previewDialog_)) {
    dialog->zoomOut();
  }
}

void AdImagePreviewGroup::rotateLeft() {
  if (ImagePreviewOverlay* dialog = asPreviewOverlay(previewDialog_)) {
    dialog->rotateLeft();
  }
}

void AdImagePreviewGroup::rotateRight() {
  if (ImagePreviewOverlay* dialog = asPreviewOverlay(previewDialog_)) {
    dialog->rotateRight();
  }
}

void AdImagePreviewGroup::flipX() {
  if (ImagePreviewOverlay* dialog = asPreviewOverlay(previewDialog_)) {
    dialog->flipX();
  }
}

void AdImagePreviewGroup::flipY() {
  if (ImagePreviewOverlay* dialog = asPreviewOverlay(previewDialog_)) {
    dialog->flipY();
  }
}

void AdImagePreviewGroup::resetTransform() {
  if (ImagePreviewOverlay* dialog = asPreviewOverlay(previewDialog_)) {
    dialog->resetTransform();
  }
}

void AdImagePreviewGroup::closePreview() {
  if (ImagePreviewOverlay* dialog = asPreviewOverlay(previewDialog_)) {
    dialog->close();
  } else if (open_) {
    open_ = false;
    emit openChanged(false);
    emit onOpenChange(false, current_);
  }
}

void AdImagePreviewGroup::attachImage(AdImage* image) {
  if (!image) {
    return;
  }

  for (int i = attachedImages_.size() - 1; i >= 0; --i) {
    if (!attachedImages_.at(i) || attachedImages_.at(i) == image) {
      attachedImages_.removeAt(i);
    }
  }
  attachedImages_.append(image);
}

void AdImagePreviewGroup::detachImage(AdImage* image) {
  for (int i = attachedImages_.size() - 1; i >= 0; --i) {
    if (!attachedImages_.at(i) || attachedImages_.at(i) == image) {
      attachedImages_.removeAt(i);
    }
  }
}

void AdImagePreviewGroup::handleImageActivated(AdImage* image) {
  if (!previewEnabled_) {
    return;
  }

  QVector<PreviewItemData> previewItems;
  previewItems.reserve(!items_.isEmpty() ? items_.size() : attachedImages_.size());

  int preferredIndex = 0;

  if (!items_.isEmpty()) {
    const QString preferredSource = image ? image->effectivePreviewSource() : QString();
    for (int i = 0; i < items_.size(); ++i) {
      PreviewItemData item;
      item.src = items_.at(i);
      previewItems.append(item);
      if (!preferredSource.isEmpty() && item.src == preferredSource) {
        preferredIndex = i;
      }
    }
  } else {
    int visibleIndex = 0;
    for (const QPointer<AdImage>& imagePtr : attachedImages_) {
      if (!imagePtr) {
        continue;
      }
      const QString source = imagePtr->effectivePreviewSource();
      if (source.trimmed().isEmpty()) {
        continue;
      }
      PreviewItemData item;
      item.src = source;
      item.alt = imagePtr->alt();
      item.width = imagePtr->imageWidth();
      item.height = imagePtr->imageHeight();
      previewItems.append(item);

      if (image && imagePtr == image) {
        preferredIndex = visibleIndex;
      }
      ++visibleIndex;
    }
  }

  if (previewItems.isEmpty()) {
    return;
  }

  if (!previewDialog_) {
    auto* dialog = new ImagePreviewOverlay();
    previewDialog_ = dialog;

    dialog->visibilityChanged = [this](bool value) {
      if (open_ == value) {
        return;
      }
      open_ = value;
      emit openChanged(open_);
      emit onOpenChange(open_, current_);
    };

    dialog->currentChanged = [this](int current, int previous) {
      if (current_ != current) {
        current_ = current;
        emit currentChanged(current_);
      }
      emit onChange(current, previous);
    };

    dialog->imageInfoChanged = [this](int current,
                                      int total,
                                      const QString& src,
                                      const QString& alt,
                                      int width,
                                      int height) {
      currentSource_ = src;
      previewImageInfoChanged(current, total, src, alt, width, height);
    };
  }

  ImagePreviewOverlay* dialog = asPreviewOverlay(previewDialog_);
  if (!dialog) {
    return;
  }

  dialog->setItems(previewItems);
  dialog->setCurrentIndex(std::clamp(preferredIndex, 0, static_cast<int>(previewItems.size() - 1)),
                          false);
  current_ = dialog->currentIndex();
  emit currentChanged(current_);

  refreshDialogVisualState();

  QWidget* owner = image ? image : qApp->activeWindow();
  dialog->openFor(owner);
  if (!open_) {
    open_ = true;
    emit openChanged(true);
    emit onOpenChange(true, current_);
  }
}

void AdImagePreviewGroup::refreshDialogVisualState() {
  ImagePreviewOverlay* dialog = asPreviewOverlay(previewDialog_);
  if (!dialog) {
    return;
  }

  SemanticStyles mergedSemantic = semanticStyles_;
  if (semanticStyleResolver_) {
    StyleContext context;
    context.open = open_;
    context.previewMaskVisible = previewMaskVisible_;
    context.previewMaskBlur = previewMaskBlur_;
    mergedSemantic = mergedGroupSemanticStyles(mergedSemantic, semanticStyleResolver_(context));
  }

  detail::ImageGroupStyleInput styleInput;
  styleInput.componentTokens = componentTokens_;
  styleInput.semanticStyles = mergedSemantic;
  styleInput.previewMaskVisible = previewMaskVisible_;
  styleInput.previewMaskBlur = previewMaskBlur_;

  dialog->setVisualStyle(detail::resolveImageGroupVisualStyle(styleInput));
  dialog->setCountRenderFormat(countRenderFormat_);
  dialog->setMaskVisible(previewMaskVisible_);
  dialog->setMaskBlur(previewMaskBlur_);
  dialog->setScaleStep(scaleStep_);
}

void AdImagePreviewGroup::syncDialogOpenState() { setOpen(open_); }

AdImage::AdImage(QWidget* parent) : QWidget(parent) {
  setAttribute(Qt::WA_Hover, true);
  setMouseTracking(true);
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

  connect(&adqt::theme::ThemeManager::instance(), &adqt::theme::ThemeManager::themeChanged, this,
          [this]() {
            update();
            syncStandaloneDialogOpenState();
            if (previewGroup_) {
              previewGroup_->refreshDialogVisualState();
            }
          });
}

AdImage::~AdImage() {
  if (previewGroup_) {
    previewGroup_->detachImage(this);
  }

  if (standalonePreviewDialog_) {
    standalonePreviewDialog_->close();
    standalonePreviewDialog_->deleteLater();
  }
}

QString AdImage::src() const { return src_; }

void AdImage::setSrc(const QString& value) {
  if (src_ == value) {
    return;
  }
  src_ = value;
  emit srcChanged(src_);
  reloadMainImage();
}

QString AdImage::alt() const { return alt_; }

void AdImage::setAlt(const QString& value) {
  if (alt_ == value) {
    return;
  }
  alt_ = value;
  emit altChanged(alt_);
}

QString AdImage::fallbackSrc() const { return fallbackSrc_; }

void AdImage::setFallbackSrc(const QString& value) {
  if (fallbackSrc_ == value) {
    return;
  }
  fallbackSrc_ = value;
  emit fallbackSrcChanged(fallbackSrc_);
  if (loadFailed_ || imagePixmap_.isNull()) {
    reloadMainImage();
  }
}

QString AdImage::placeholderSrc() const { return placeholderSrc_; }

void AdImage::setPlaceholderSrc(const QString& value) {
  if (placeholderSrc_ == value) {
    return;
  }
  placeholderSrc_ = value;
  emit placeholderSrcChanged(placeholderSrc_);
  reloadPlaceholderImage();
}

QString AdImage::previewSrc() const { return previewSrc_; }

void AdImage::setPreviewSrc(const QString& value) {
  if (previewSrc_ == value) {
    return;
  }
  previewSrc_ = value;
  emit previewSrcChanged(previewSrc_);
}

bool AdImage::previewEnabled() const { return previewEnabled_; }

void AdImage::setPreviewEnabled(bool value) {
  if (previewEnabled_ == value) {
    return;
  }
  previewEnabled_ = value;
  emit previewEnabledChanged(previewEnabled_);
  if (!previewEnabled_) {
    setPreviewOpen(false);
  }
  update();
}

bool AdImage::previewMaskVisible() const { return previewMaskVisible_; }

void AdImage::setPreviewMaskVisible(bool value) {
  if (previewMaskVisible_ == value) {
    return;
  }
  previewMaskVisible_ = value;
  emit previewMaskVisibleChanged(previewMaskVisible_);
  syncStandaloneDialogOpenState();
}

bool AdImage::previewMaskBlur() const { return previewMaskBlur_; }

void AdImage::setPreviewMaskBlur(bool value) {
  if (previewMaskBlur_ == value) {
    return;
  }
  previewMaskBlur_ = value;
  emit previewMaskBlurChanged(previewMaskBlur_);
  syncStandaloneDialogOpenState();
}

QString AdImage::previewCoverText() const { return previewCoverText_; }

void AdImage::setPreviewCoverText(const QString& value) {
  if (previewCoverText_ == value) {
    return;
  }
  previewCoverText_ = value;
  emit previewCoverTextChanged(previewCoverText_);
  update();
}

AdImage::CoverPlacement AdImage::previewCoverPlacement() const { return previewCoverPlacement_; }

void AdImage::setPreviewCoverPlacement(CoverPlacement value) {
  if (previewCoverPlacement_ == value) {
    return;
  }
  previewCoverPlacement_ = value;
  emit previewCoverPlacementChanged(previewCoverPlacement_);
  update();
}

bool AdImage::previewOpen() const { return previewOpen_; }

void AdImage::setPreviewOpen(bool value) {
  if (previewOpen_ == value) {
    return;
  }
  previewOpen_ = value;
  emit previewOpenChanged(previewOpen_);
  emit onPreviewOpenChange(previewOpen_);
  syncStandaloneDialogOpenState();
}

bool AdImage::previewOpenControlled() const { return previewOpenControlled_; }

void AdImage::setPreviewOpenControlled(bool value) {
  if (previewOpenControlled_ == value) {
    return;
  }
  previewOpenControlled_ = value;
  emit previewOpenControlledChanged(previewOpenControlled_);
}

double AdImage::previewScaleStep() const { return previewScaleStep_; }

void AdImage::setPreviewScaleStep(double value) {
  const double clamped = std::clamp(value, 0.05, 10.0);
  if (qFuzzyCompare(previewScaleStep_ + 1.0, clamped + 1.0)) {
    return;
  }
  previewScaleStep_ = clamped;
  emit previewScaleStepChanged(previewScaleStep_);
  syncStandaloneDialogOpenState();
}

int AdImage::imageWidth() const { return imageWidth_; }

void AdImage::setImageWidth(int value) {
  const int clamped = value <= 0 ? -1 : value;
  if (imageWidth_ == clamped) {
    return;
  }
  imageWidth_ = clamped;
  emit imageWidthChanged(imageWidth_);
  updateGeometry();
  update();
}

int AdImage::imageHeight() const { return imageHeight_; }

void AdImage::setImageHeight(int value) {
  const int clamped = value <= 0 ? -1 : value;
  if (imageHeight_ == clamped) {
    return;
  }
  imageHeight_ = clamped;
  emit imageHeightChanged(imageHeight_);
  updateGeometry();
  update();
}

bool AdImage::loading() const { return loading_; }

bool AdImage::loadFailed() const { return loadFailed_; }

AdImagePreviewGroup* AdImage::previewGroup() const { return previewGroup_; }

void AdImage::setPreviewGroup(AdImagePreviewGroup* value) {
  if (previewGroup_ == value) {
    return;
  }

  if (previewGroup_) {
    previewGroup_->detachImage(this);
  }

  previewGroup_ = value;

  if (previewGroup_) {
    previewGroup_->attachImage(this);
  }

  emit previewGroupChanged(previewGroup_);
  update();
}

AdImage::ComponentTokens AdImage::componentTokens() const { return componentTokens_; }

void AdImage::setComponentTokens(const ComponentTokens& tokens) {
  componentTokens_ = tokens;
  emit componentTokensChanged();
  update();
  syncStandaloneDialogOpenState();
}

void AdImage::resetComponentTokens() {
  componentTokens_ = ComponentTokens{};
  emit componentTokensChanged();
  update();
  syncStandaloneDialogOpenState();
}

AdImage::SemanticStyles AdImage::semanticStyles() const { return semanticStyles_; }

void AdImage::setSemanticStyles(const SemanticStyles& styles) {
  semanticStyles_ = styles;
  emit semanticStylesChanged();
  update();
  syncStandaloneDialogOpenState();
}

void AdImage::setSemanticStyleResolver(SemanticStyleResolver resolver) {
  semanticStyleResolver_ = std::move(resolver);
  emit semanticStylesChanged();
  update();
  syncStandaloneDialogOpenState();
}

QSize AdImage::sizeHint() const {
  int targetWidth = imageWidth_;
  int targetHeight = imageHeight_;

  if (targetWidth > 0 && targetHeight > 0) {
    return QSize(targetWidth, targetHeight);
  }

  const QPixmap sourcePixmap = imagePixmap_.isNull() ? placeholderPixmap_ : imagePixmap_;
  if (!sourcePixmap.isNull()) {
    const QSize natural = sourcePixmap.size() / std::max(1.0, sourcePixmap.devicePixelRatioF());
    if (targetWidth > 0) {
      const int h = std::max(1, static_cast<int>(std::round(targetWidth * natural.height() /
                                                            std::max(1, natural.width()))));
      return QSize(targetWidth, h);
    }
    if (targetHeight > 0) {
      const int w = std::max(1, static_cast<int>(std::round(targetHeight * natural.width() /
                                                            std::max(1, natural.height()))));
      return QSize(w, targetHeight);
    }
    return natural;
  }

  if (targetWidth > 0) {
    return QSize(targetWidth, std::max(80, targetWidth * 3 / 4));
  }
  if (targetHeight > 0) {
    return QSize(std::max(100, targetHeight * 4 / 3), targetHeight);
  }

  return QSize(160, 120);
}

QSize AdImage::minimumSizeHint() const { return QSize(40, 30); }

void AdImage::paintEvent(QPaintEvent* event) {
  Q_UNUSED(event)

  StyleContext context;
  context.hovered = hovered_;
  context.loading = loading_;
  context.failed = loadFailed_;
  context.previewEnabled = previewEnabled_;
  context.previewMaskVisible = previewMaskVisible_;
  context.previewMaskBlur = previewMaskBlur_;

  SemanticStyles mergedSemantic = semanticStyles_;
  if (semanticStyleResolver_) {
    mergedSemantic = mergedImageSemanticStyles(mergedSemantic, semanticStyleResolver_(context));
  }

  detail::ImageStyleInput styleInput;
  styleInput.componentTokens = componentTokens_;
  styleInput.semanticStyles = mergedSemantic;
  styleInput.previewMaskVisible = previewMaskVisible_;
  styleInput.previewMaskBlur = previewMaskBlur_;
  const detail::ImageVisualStyle visual = detail::resolveImageVisualStyle(styleInput);

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, true);

  const QRect drawRect = rect().adjusted(0, 0, -1, -1);
  const qreal radius = std::max(0, visual.metrics.borderRadius);
  QPainterPath clipPath;
  clipPath.addRoundedRect(drawRect, radius, radius);
  painter.setClipPath(clipPath);

  if (visual.rootBackground.alpha() > 0) {
    painter.fillRect(drawRect, visual.rootBackground);
  }

  QPixmap displayPixmap = imagePixmap_;
  if (displayPixmap.isNull() && loading_ && !placeholderPixmap_.isNull()) {
    displayPixmap = placeholderPixmap_;
  }

  if (!displayPixmap.isNull()) {
    const QSize scaled = displayPixmap.size().scaled(drawRect.size(), Qt::KeepAspectRatio);
    const QPoint topLeft(drawRect.center().x() - scaled.width() / 2,
                         drawRect.center().y() - scaled.height() / 2);
    painter.drawPixmap(QRect(topLeft, scaled), displayPixmap);
  } else {
    painter.fillRect(drawRect, visual.placeholderBackground);

    const int iconSize = std::max(14, std::min(drawRect.width(), drawRect.height()) / 4);
    adqt::icons::IconStyle iconStyle;
    iconStyle.hasPrimary = true;
    iconStyle.primary = visual.placeholderIcon;
    const QPixmap iconPixmap = adqt::icons::renderIconPixmap(outlined_icons::Picture(iconStyle),
                                                             QSize(iconSize, iconSize),
                                                             devicePixelRatioF());
    const QPoint iconPos(drawRect.center().x() - iconSize / 2, drawRect.center().y() - iconSize / 2 - 8);
    painter.drawPixmap(iconPos, iconPixmap);

    painter.setPen(visual.placeholderIcon);
    const QString text = loadFailed_ ? QStringLiteral("Failed to load image")
                                     : (loading_ ? QStringLiteral("Loading...") : QStringLiteral("No image"));
    const QRect textRect(drawRect.left() + 8, drawRect.center().y() + iconSize / 2, drawRect.width() - 16, 30);
    painter.drawText(textRect, Qt::AlignHCenter | Qt::AlignTop, text);
  }

  if (previewEnabled_ && (hovered_ || previewOpen_)) {
    const QRect overlayRect = coverRectForPlacement(drawRect, previewCoverPlacement_);
    painter.fillRect(overlayRect, visual.coverBackground);
    painter.setPen(visual.coverText);

    const int iconSize = std::max(12, std::min(18, overlayRect.height() - 8));
    adqt::icons::IconStyle iconStyle;
    iconStyle.hasPrimary = true;
    iconStyle.primary = visual.coverText;
    const QPixmap iconPixmap = adqt::icons::renderIconPixmap(outlined_icons::ZoomIn(iconStyle),
                                                             QSize(iconSize, iconSize),
                                                             devicePixelRatioF());

    QString coverText = previewCoverText_.trimmed();
    if (coverText.isEmpty()) {
      coverText = QStringLiteral("Preview");
    }

    const QFontMetrics metrics(font());
    const int textWidth = metrics.horizontalAdvance(coverText);
    const int contentWidth = iconSize + 6 + textWidth;
    const int startX = overlayRect.center().x() - contentWidth / 2;
    const int iconY = overlayRect.center().y() - iconSize / 2;
    painter.drawPixmap(QPoint(startX, iconY), iconPixmap);
    const QRect textRect(startX + iconSize + 6, overlayRect.top(), textWidth + 4, overlayRect.height());
    painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, coverText);
  }

  painter.setClipping(false);
  if (visual.rootBorder.alpha() > 0) {
    QPen pen(visual.rootBorder);
    pen.setWidthF(1.0);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(drawRect, radius, radius);
  }
}

void AdImage::enterEvent(QEnterEvent* event) {
  QWidget::enterEvent(event);
  hovered_ = true;
  update();
}

void AdImage::leaveEvent(QEvent* event) {
  QWidget::leaveEvent(event);
  hovered_ = false;
  update();
}

void AdImage::mouseReleaseEvent(QMouseEvent* event) {
  QWidget::mouseReleaseEvent(event);
  if (!event || event->button() != Qt::LeftButton) {
    return;
  }
  if (!rect().contains(event->position().toPoint())) {
    return;
  }
  activatePreviewFromUser();
}

void AdImage::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  update();
}

QString AdImage::effectivePreviewSource() const {
  if (!previewSrc_.trimmed().isEmpty()) {
    return previewSrc_.trimmed();
  }
  return src_.trimmed();
}

void AdImage::reloadMainImage() {
  const QString source = src_.trimmed();
  const int token = ++mainLoadToken_;

  if (source.isEmpty()) {
    imagePixmap_ = QPixmap();
    setLoadingState(false);
    setLoadFailedState(false);
    update();
    updateGeometry();
    return;
  }

  setLoadingState(true);
  setLoadFailedState(false);

  const QString fallback = fallbackSrc_.trimmed();
  requestPixmap(source, this, [this, token, fallback](bool ok, const QPixmap& pixmap) {
    if (token != mainLoadToken_) {
      return;
    }

    if (ok && !pixmap.isNull()) {
      imagePixmap_ = pixmap;
      setLoadingState(false);
      setLoadFailedState(false);
      update();
      updateGeometry();
      return;
    }

    if (!fallback.isEmpty() && fallback != src_.trimmed()) {
      requestPixmap(fallback, this, [this, token](bool fallbackOk, const QPixmap& fallbackPixmap) {
        if (token != mainLoadToken_) {
          return;
        }
        if (fallbackOk && !fallbackPixmap.isNull()) {
          imagePixmap_ = fallbackPixmap;
          setLoadingState(false);
          setLoadFailedState(false);
          update();
          updateGeometry();
          return;
        }

        imagePixmap_ = QPixmap();
        setLoadingState(false);
        setLoadFailedState(true);
        update();
        updateGeometry();
      });
      return;
    }

    imagePixmap_ = QPixmap();
    setLoadingState(false);
    setLoadFailedState(true);
    update();
    updateGeometry();
  });
}

void AdImage::reloadPlaceholderImage() {
  const QString source = placeholderSrc_.trimmed();
  const int token = ++placeholderLoadToken_;

  if (source.isEmpty()) {
    placeholderPixmap_ = QPixmap();
    update();
    return;
  }

  requestPixmap(source, this, [this, token](bool ok, const QPixmap& pixmap) {
    if (token != placeholderLoadToken_) {
      return;
    }
    if (ok && !pixmap.isNull()) {
      placeholderPixmap_ = pixmap;
    } else {
      placeholderPixmap_ = QPixmap();
    }
    update();
  });
}

void AdImage::setLoadingState(bool value) {
  if (loading_ == value) {
    return;
  }
  loading_ = value;
  emit loadingChanged(loading_);
}

void AdImage::setLoadFailedState(bool value) {
  if (loadFailed_ == value) {
    return;
  }
  loadFailed_ = value;
  emit loadFailedChanged(loadFailed_);
}

void AdImage::activatePreviewFromUser() {
  if (previewGroup_) {
    previewGroup_->handleImageActivated(this);
    return;
  }

  if (!previewEnabled_) {
    return;
  }

  if (previewOpenControlled_) {
    emit onPreviewOpenChange(true);
    return;
  }

  setPreviewOpen(true);
}

void AdImage::openStandalonePreview() {
  if (!previewEnabled_ || previewGroup_) {
    return;
  }

  const QString source = effectivePreviewSource();
  if (source.isEmpty()) {
    return;
  }

  if (!standalonePreviewDialog_) {
    auto* dialog = new ImagePreviewOverlay();
    standalonePreviewDialog_ = dialog;

    dialog->visibilityChanged = [this](bool open) {
      if (!open && previewOpen_) {
        previewOpen_ = false;
        emit previewOpenChanged(false);
        emit onPreviewOpenChange(false);
      }
    };

    dialog->imageInfoChanged = [this](int,
                                      int,
                                      const QString& src,
                                      const QString& alt,
                                      int width,
                                      int height) {
      emit previewImageInfoChanged(src, alt, width, height);
    };
  }

  ImagePreviewOverlay* dialog = asPreviewOverlay(standalonePreviewDialog_);
  if (!dialog) {
    return;
  }

  PreviewItemData item;
  item.src = source;
  item.alt = alt_;
  item.width = imageWidth_;
  item.height = imageHeight_;
  dialog->setItems({item});
  dialog->setCurrentIndex(0, false);
  dialog->setScaleStep(previewScaleStep_);
  dialog->setCountRenderFormat(QString());
  dialog->setMaskVisible(previewMaskVisible_);
  dialog->setMaskBlur(previewMaskBlur_);

  StyleContext context;
  context.hovered = hovered_;
  context.loading = loading_;
  context.failed = loadFailed_;
  context.previewEnabled = previewEnabled_;
  context.previewMaskVisible = previewMaskVisible_;
  context.previewMaskBlur = previewMaskBlur_;

  SemanticStyles mergedSemantic = semanticStyles_;
  if (semanticStyleResolver_) {
    mergedSemantic = mergedImageSemanticStyles(mergedSemantic, semanticStyleResolver_(context));
  }

  detail::ImageStyleInput styleInput;
  styleInput.componentTokens = componentTokens_;
  styleInput.semanticStyles = mergedSemantic;
  styleInput.previewMaskVisible = previewMaskVisible_;
  styleInput.previewMaskBlur = previewMaskBlur_;
  dialog->setVisualStyle(detail::resolveImageVisualStyle(styleInput));

  dialog->openFor(this);
}

void AdImage::syncStandaloneDialogOpenState() {
  if (previewGroup_) {
    return;
  }

  if (!previewOpen_ || !previewEnabled_) {
    if (ImagePreviewOverlay* dialog = asPreviewOverlay(standalonePreviewDialog_)) {
      dialog->close();
    }
    return;
  }

  openStandalonePreview();
}

}  // namespace adqt::widgets
