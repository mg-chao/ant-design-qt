#pragma once

#include <QColor>
#include <QObject>
#include <QPointer>
#include <QPixmap>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QWidget>

#include <functional>
#include <optional>

class QEnterEvent;
class QEvent;
class QMouseEvent;
class QPaintEvent;
class QResizeEvent;

namespace adqt::widgets {

class AdImage;

class AdImagePreviewGroup final : public QObject {
  Q_OBJECT

  Q_PROPERTY(QStringList items READ items WRITE setItems NOTIFY itemsChanged)
  Q_PROPERTY(bool open READ open WRITE setOpen NOTIFY openChanged)
  Q_PROPERTY(int current READ current WRITE setCurrent NOTIFY currentChanged)
  Q_PROPERTY(QString countRenderFormat READ countRenderFormat WRITE setCountRenderFormat
                 NOTIFY countRenderFormatChanged)
  Q_PROPERTY(bool previewEnabled READ previewEnabled WRITE setPreviewEnabled NOTIFY previewEnabledChanged)
  Q_PROPERTY(bool previewMaskVisible READ previewMaskVisible WRITE setPreviewMaskVisible
                 NOTIFY previewMaskVisibleChanged)
  Q_PROPERTY(bool previewMaskBlur READ previewMaskBlur WRITE setPreviewMaskBlur
                 NOTIFY previewMaskBlurChanged)
  Q_PROPERTY(double scaleStep READ scaleStep WRITE setScaleStep NOTIFY scaleStepChanged)

 public:
  struct ComponentTokens {
    std::optional<int> zIndexPopup;
    std::optional<int> previewOperationSize;
    std::optional<QString> previewOperationColor;
    std::optional<QString> previewOperationHoverColor;
    std::optional<QString> previewOperationColorDisabled;
  };

  struct SemanticSlotStyle {
    std::optional<QColor> textColor;
    std::optional<QColor> backgroundColor;
    std::optional<QColor> borderColor;
  };

  struct SemanticStyles {
    SemanticSlotStyle popupRoot;
    SemanticSlotStyle popupMask;
    SemanticSlotStyle popupBody;
    SemanticSlotStyle popupFooter;
    SemanticSlotStyle popupActions;
  };

  struct StyleContext {
    bool open = false;
    bool previewMaskVisible = true;
    bool previewMaskBlur = false;
  };

  using SemanticStyleResolver = std::function<SemanticStyles(const StyleContext&)>;

  explicit AdImagePreviewGroup(QObject* parent = nullptr);
  ~AdImagePreviewGroup() override;

  QStringList items() const;
  void setItems(const QStringList& value);

  bool open() const;
  void setOpen(bool value);

  int current() const;
  void setCurrent(int value);

  QString countRenderFormat() const;
  void setCountRenderFormat(const QString& value);

  bool previewEnabled() const;
  void setPreviewEnabled(bool value);

  bool previewMaskVisible() const;
  void setPreviewMaskVisible(bool value);

  bool previewMaskBlur() const;
  void setPreviewMaskBlur(bool value);

  double scaleStep() const;
  void setScaleStep(double value);

  ComponentTokens componentTokens() const;
  void setComponentTokens(const ComponentTokens& tokens);
  void resetComponentTokens();

  SemanticStyles semanticStyles() const;
  void setSemanticStyles(const SemanticStyles& styles);
  void setSemanticStyleResolver(SemanticStyleResolver resolver);

  QString currentSource() const;
  int totalCount() const;

 public slots:
  void activate(int delta);
  void zoomIn();
  void zoomOut();
  void rotateLeft();
  void rotateRight();
  void flipX();
  void flipY();
  void resetTransform();
  void closePreview();

 signals:
  void itemsChanged(const QStringList& value);
  void openChanged(bool value);
  void onOpenChange(bool value, int current);
  void currentChanged(int value);
  void onChange(int current, int previous);
  void countRenderFormatChanged(const QString& value);
  void previewEnabledChanged(bool value);
  void previewMaskVisibleChanged(bool value);
  void previewMaskBlurChanged(bool value);
  void scaleStepChanged(double value);
  void componentTokensChanged();
  void semanticStylesChanged();
  void previewImageInfoChanged(int current,
                               int total,
                               const QString& src,
                               const QString& alt,
                               int width,
                               int height);

 private:
  friend class AdImage;

  void attachImage(AdImage* image);
  void detachImage(AdImage* image);
  void handleImageActivated(AdImage* image);
  void refreshDialogVisualState();
  void syncDialogOpenState();

  QVector<QPointer<AdImage>> attachedImages_;
  QStringList items_;
  bool open_ = false;
  int current_ = 0;
  QString countRenderFormat_ = QStringLiteral("%1 / %2");
  bool previewEnabled_ = true;
  bool previewMaskVisible_ = true;
  bool previewMaskBlur_ = false;
  double scaleStep_ = 0.5;
  ComponentTokens componentTokens_;
  SemanticStyles semanticStyles_;
  SemanticStyleResolver semanticStyleResolver_;
  QPointer<QWidget> previewDialog_;
  QString currentSource_;
};

class AdImage final : public QWidget {
  Q_OBJECT

  Q_PROPERTY(QString src READ src WRITE setSrc NOTIFY srcChanged)
  Q_PROPERTY(QString alt READ alt WRITE setAlt NOTIFY altChanged)
  Q_PROPERTY(QString fallbackSrc READ fallbackSrc WRITE setFallbackSrc NOTIFY fallbackSrcChanged)
  Q_PROPERTY(QString placeholderSrc READ placeholderSrc WRITE setPlaceholderSrc NOTIFY placeholderSrcChanged)
  Q_PROPERTY(QString previewSrc READ previewSrc WRITE setPreviewSrc NOTIFY previewSrcChanged)
  Q_PROPERTY(bool previewEnabled READ previewEnabled WRITE setPreviewEnabled NOTIFY previewEnabledChanged)
  Q_PROPERTY(bool previewMaskVisible READ previewMaskVisible WRITE setPreviewMaskVisible
                 NOTIFY previewMaskVisibleChanged)
  Q_PROPERTY(bool previewMaskBlur READ previewMaskBlur WRITE setPreviewMaskBlur
                 NOTIFY previewMaskBlurChanged)
  Q_PROPERTY(QString previewCoverText READ previewCoverText WRITE setPreviewCoverText
                 NOTIFY previewCoverTextChanged)
  Q_PROPERTY(CoverPlacement previewCoverPlacement READ previewCoverPlacement WRITE setPreviewCoverPlacement
                 NOTIFY previewCoverPlacementChanged)
  Q_PROPERTY(bool previewOpen READ previewOpen WRITE setPreviewOpen NOTIFY previewOpenChanged)
  Q_PROPERTY(bool previewOpenControlled READ previewOpenControlled WRITE setPreviewOpenControlled
                 NOTIFY previewOpenControlledChanged)
  Q_PROPERTY(double previewScaleStep READ previewScaleStep WRITE setPreviewScaleStep
                 NOTIFY previewScaleStepChanged)
  Q_PROPERTY(int imageWidth READ imageWidth WRITE setImageWidth NOTIFY imageWidthChanged)
  Q_PROPERTY(int imageHeight READ imageHeight WRITE setImageHeight NOTIFY imageHeightChanged)
  Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
  Q_PROPERTY(bool loadFailed READ loadFailed NOTIFY loadFailedChanged)
  Q_PROPERTY(AdImagePreviewGroup* previewGroup READ previewGroup WRITE setPreviewGroup
                 NOTIFY previewGroupChanged)

 public:
  enum class CoverPlacement {
    Center,
    Top,
    Bottom,
  };
  Q_ENUM(CoverPlacement)

  struct ComponentTokens {
    std::optional<int> borderRadius;
    std::optional<QString> placeholderBg;
    std::optional<QString> placeholderIconColor;
    std::optional<QString> coverBg;
    std::optional<QString> coverColor;
    std::optional<int> zIndexPopup;
    std::optional<int> previewOperationSize;
    std::optional<QString> previewOperationColor;
    std::optional<QString> previewOperationHoverColor;
    std::optional<QString> previewOperationColorDisabled;
  };

  struct SemanticSlotStyle {
    std::optional<QColor> textColor;
    std::optional<QColor> backgroundColor;
    std::optional<QColor> borderColor;
  };

  struct SemanticStyles {
    SemanticSlotStyle root;
    SemanticSlotStyle image;
    SemanticSlotStyle cover;
    SemanticSlotStyle popupRoot;
    SemanticSlotStyle popupMask;
    SemanticSlotStyle popupBody;
    SemanticSlotStyle popupFooter;
    SemanticSlotStyle popupActions;
  };

  struct StyleContext {
    bool hovered = false;
    bool loading = false;
    bool failed = false;
    bool previewEnabled = true;
    bool previewMaskVisible = true;
    bool previewMaskBlur = false;
  };

  using SemanticStyleResolver = std::function<SemanticStyles(const StyleContext&)>;

  explicit AdImage(QWidget* parent = nullptr);
  ~AdImage() override;

  QString src() const;
  void setSrc(const QString& value);

  QString alt() const;
  void setAlt(const QString& value);

  QString fallbackSrc() const;
  void setFallbackSrc(const QString& value);

  QString placeholderSrc() const;
  void setPlaceholderSrc(const QString& value);

  QString previewSrc() const;
  void setPreviewSrc(const QString& value);

  bool previewEnabled() const;
  void setPreviewEnabled(bool value);

  bool previewMaskVisible() const;
  void setPreviewMaskVisible(bool value);

  bool previewMaskBlur() const;
  void setPreviewMaskBlur(bool value);

  QString previewCoverText() const;
  void setPreviewCoverText(const QString& value);

  CoverPlacement previewCoverPlacement() const;
  void setPreviewCoverPlacement(CoverPlacement value);

  bool previewOpen() const;
  void setPreviewOpen(bool value);

  bool previewOpenControlled() const;
  void setPreviewOpenControlled(bool value);

  double previewScaleStep() const;
  void setPreviewScaleStep(double value);

  int imageWidth() const;
  void setImageWidth(int value);

  int imageHeight() const;
  void setImageHeight(int value);

  bool loading() const;
  bool loadFailed() const;

  AdImagePreviewGroup* previewGroup() const;
  void setPreviewGroup(AdImagePreviewGroup* value);

  ComponentTokens componentTokens() const;
  void setComponentTokens(const ComponentTokens& tokens);
  void resetComponentTokens();

  SemanticStyles semanticStyles() const;
  void setSemanticStyles(const SemanticStyles& styles);
  void setSemanticStyleResolver(SemanticStyleResolver resolver);

  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;

 signals:
  void srcChanged(const QString& value);
  void altChanged(const QString& value);
  void fallbackSrcChanged(const QString& value);
  void placeholderSrcChanged(const QString& value);
  void previewSrcChanged(const QString& value);
  void previewEnabledChanged(bool value);
  void previewMaskVisibleChanged(bool value);
  void previewMaskBlurChanged(bool value);
  void previewCoverTextChanged(const QString& value);
  void previewCoverPlacementChanged(CoverPlacement value);
  void previewOpenChanged(bool value);
  void onPreviewOpenChange(bool value);
  void previewOpenControlledChanged(bool value);
  void previewScaleStepChanged(double value);
  void imageWidthChanged(int value);
  void imageHeightChanged(int value);
  void loadingChanged(bool value);
  void loadFailedChanged(bool value);
  void previewGroupChanged(AdImagePreviewGroup* value);
  void componentTokensChanged();
  void semanticStylesChanged();
  void previewImageInfoChanged(const QString& src,
                               const QString& alt,
                               int width,
                               int height);

 protected:
  void paintEvent(QPaintEvent* event) override;
  void enterEvent(QEnterEvent* event) override;
  void leaveEvent(QEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;

 private:
  friend class AdImagePreviewGroup;

  QString effectivePreviewSource() const;
  void reloadMainImage();
  void reloadPlaceholderImage();
  void setLoadingState(bool value);
  void setLoadFailedState(bool value);
  void activatePreviewFromUser();
  void openStandalonePreview();
  void syncStandaloneDialogOpenState();

  QString src_;
  QString alt_;
  QString fallbackSrc_;
  QString placeholderSrc_;
  QString previewSrc_;
  bool previewEnabled_ = true;
  bool previewMaskVisible_ = true;
  bool previewMaskBlur_ = false;
  QString previewCoverText_ = QStringLiteral("Preview");
  CoverPlacement previewCoverPlacement_ = CoverPlacement::Center;
  bool previewOpen_ = false;
  bool previewOpenControlled_ = false;
  double previewScaleStep_ = 0.5;
  int imageWidth_ = -1;
  int imageHeight_ = -1;
  bool loading_ = false;
  bool loadFailed_ = false;
  bool hovered_ = false;
  int mainLoadToken_ = 0;
  int placeholderLoadToken_ = 0;
  QPixmap imagePixmap_;
  QPixmap placeholderPixmap_;
  QPointer<AdImagePreviewGroup> previewGroup_;
  QPointer<QWidget> standalonePreviewDialog_;
  ComponentTokens componentTokens_;
  SemanticStyles semanticStyles_;
  SemanticStyleResolver semanticStyleResolver_;
};

}  // namespace adqt::widgets
