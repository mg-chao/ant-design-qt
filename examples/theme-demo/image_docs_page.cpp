#include "image_docs_page.h"

#include <QDateTime>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include <functional>

using adqt::widgets::AdImage;
using adqt::widgets::AdImagePreviewGroup;

namespace {

const QString kBasicPng =
    QStringLiteral("https://zos.alipayobjects.com/rmsportal/jkjgkEfvpUPVyRjUImniVslZfWPnJuuZ.png");
const QString kSvgA =
    QStringLiteral("https://gw.alipayobjects.com/zos/rmsportal/KDpgvguMpGfqaHPjicRK.svg");
const QString kSvgB =
    QStringLiteral("https://gw.alipayobjects.com/zos/antfincdn/aPkFc8Sj7n/method-draw-image.svg");
const QString kWebpA =
    QStringLiteral("https://gw.alipayobjects.com/zos/antfincdn/LlvErxo8H9/photo-1503185912284-5271ff81b9a8.webp");
const QString kWebpB =
    QStringLiteral("https://gw.alipayobjects.com/zos/antfincdn/cV16ZqzMjW/photo-1473091540282-9b846e7965e3.webp");
const QString kWebpC =
    QStringLiteral("https://gw.alipayobjects.com/zos/antfincdn/x43I27A55%26/photo-1438109491414-7198515b166b.webp");
const QString kBlurPng = QStringLiteral(
    "https://zos.alipayobjects.com/rmsportal/jkjgkEfvpUPVyRjUImniVslZfWPnJuuZ.png"
    "?x-oss-process=image/blur,r_50,s_50/quality,q_1/resize,m_mfit,h_200,w_200");

QWidget* makeRow(QWidget* parent = nullptr) {
  auto* box = new QWidget(parent);
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(8);
  return box;
}

}  // namespace

ImageDocsPage::ImageDocsPage(QWidget* parent) : QWidget(parent) {
  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(16, 16, 16, 24);
  root->setSpacing(16);

  auto* title = new QLabel(QStringLiteral("Image"));
  QFont titleFont = title->font();
  titleFont.setPointSize(titleFont.pointSize() + 8);
  titleFont.setBold(true);
  title->setFont(titleFont);
  root->addWidget(title);

  auto* subtitle = new QLabel(QStringLiteral("Preview-able image."));
  subtitle->setWordWrap(true);
  root->addWidget(subtitle);

  addSection(root, QStringLiteral("Basic Usage"), QStringLiteral("Demo: basic.tsx"), buildBasicDemo());
  addSection(root, QStringLiteral("Fault tolerant"), QStringLiteral("Demo: fallback.tsx"),
             buildFallbackDemo());
  addSection(root, QStringLiteral("Progressive Loading"), QStringLiteral("Demo: placeholder.tsx"),
             buildPlaceholderDemo());
  addSection(root, QStringLiteral("Multiple image preview"), QStringLiteral("Demo: preview-group.tsx"),
             buildPreviewGroupDemo());
  addSection(root, QStringLiteral("Preview from one image"),
             QStringLiteral("Demo: preview-group-visible.tsx"), buildPreviewGroupVisibleDemo());
  addSection(root, QStringLiteral("Custom preview image"), QStringLiteral("Demo: previewSrc.tsx"),
             buildPreviewSrcDemo());
  addSection(root, QStringLiteral("Controlled Preview"), QStringLiteral("Demo: controlled-preview.tsx"),
             buildControlledPreviewDemo());
  addSection(root, QStringLiteral("Custom toolbar render"), QStringLiteral("Demo: toolbarRender.tsx"),
             buildToolbarRenderDemo());
  addSection(root, QStringLiteral("Custom preview render"), QStringLiteral("Demo: imageRender.tsx"),
             buildImageRenderDemo());
  addSection(root, QStringLiteral("preview mask"), QStringLiteral("Demo: mask.tsx"), buildMaskDemo());
  addSection(root, QStringLiteral("Custom semantic dom styling"), QStringLiteral("Demo: style-class.tsx"),
             buildStyleClassDemo());
  addSection(root, QStringLiteral("Custom preview mask"), QStringLiteral("Demo: preview-mask.tsx"),
             buildPreviewMaskDemo());
  addSection(root, QStringLiteral("Custom preview cover placement"),
             QStringLiteral("Demo: coverPlacement.tsx"), buildCoverPlacementDemo());
  addSection(root, QStringLiteral("nested"), QStringLiteral("Demo: nested.tsx"), buildNestedDemo());
  addSection(root,
             QStringLiteral("Top progress customization when previewing multiple images"),
             QStringLiteral("Demo: preview-group-top-progress.tsx"),
             buildPreviewGroupTopProgressDemo());
  addSection(root, QStringLiteral("Custom component token"),
             QStringLiteral("Demo: component-token.tsx"), buildComponentTokenDemo());
  addSection(root, QStringLiteral("Gets image info in the render function"),
             QStringLiteral("Demo: preview-imgInfo.tsx"), buildPreviewImageInfoDemo());

  root->addStretch();
}

const QVector<QWidget*>& ImageDocsPage::sectionAnchors() const { return anchors_; }

const QStringList& ImageDocsPage::sectionTitles() const { return titles_; }

void ImageDocsPage::addSection(QVBoxLayout* root,
                               const QString& title,
                               const QString& description,
                               QWidget* content) {
  auto* panel = new QFrame();
  panel->setFrameShape(QFrame::StyledPanel);
  auto* layout = new QVBoxLayout(panel);
  layout->setContentsMargins(14, 14, 14, 14);
  layout->setSpacing(10);

  auto* titleLabel = new QLabel(title);
  QFont titleFont = titleLabel->font();
  titleFont.setBold(true);
  titleFont.setPointSize(titleFont.pointSize() + 1);
  titleLabel->setFont(titleFont);

  auto* descLabel = new QLabel(description);
  descLabel->setWordWrap(true);

  layout->addWidget(titleLabel);
  layout->addWidget(descLabel);
  layout->addWidget(content);

  root->addWidget(panel);
  anchors_.append(panel);
  titles_.append(title);
}

AdImage* ImageDocsPage::createImage(const QString& src, int width, const QString& alt, QWidget* parent) {
  auto* image = new AdImage(parent);
  image->setSrc(src);
  image->setAlt(alt);
  image->setImageWidth(width);
  return image;
}

QWidget* ImageDocsPage::buildBasicDemo() {
  auto* box = makeRow();
  auto* row = qobject_cast<QHBoxLayout*>(box->layout());
  row->addWidget(createImage(kBasicPng, 200, QStringLiteral("basic"), box));
  row->addStretch();
  return box;
}

QWidget* ImageDocsPage::buildFallbackDemo() {
  auto* box = makeRow();
  auto* row = qobject_cast<QHBoxLayout*>(box->layout());
  auto* image = createImage(QStringLiteral("https://invalid.ant.design/image.png"), 200,
                            QStringLiteral("fallback"), box);
  image->setImageHeight(200);
  image->setFallbackSrc(kBasicPng);
  row->addWidget(image);
  row->addStretch();
  return box;
}

QWidget* ImageDocsPage::buildPlaceholderDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(8);

  auto* image = createImage(kBasicPng, 200, QStringLiteral("progressive"), box);
  image->setPlaceholderSrc(kBlurPng);
  row->addWidget(image);

  auto* reload = new QPushButton(QStringLiteral("Reload"));
  connect(reload, &QPushButton::clicked, image, [image]() {
    const qint64 stamp = QDateTime::currentMSecsSinceEpoch();
    image->setSrc(QStringLiteral("%1?%2").arg(kBasicPng).arg(stamp));
  });
  row->addWidget(reload, 0, Qt::AlignBottom);
  row->addStretch();
  return box;
}

QWidget* ImageDocsPage::buildPreviewGroupDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* info = new QLabel(QStringLiteral("current index: 1, prev index: 0"));
  auto* rowBox = makeRow(box);
  auto* row = qobject_cast<QHBoxLayout*>(rowBox->layout());

  auto* group = new AdImagePreviewGroup(box);
  auto* first = createImage(kSvgA, 200, QStringLiteral("svg image"), rowBox);
  auto* second = createImage(kSvgB, 200, QStringLiteral("svg image"), rowBox);
  first->setPreviewGroup(group);
  second->setPreviewGroup(group);

  connect(group, &AdImagePreviewGroup::onChange, box, [info](int current, int previous) {
    info->setText(QStringLiteral("current index: %1, prev index: %2")
                      .arg(current)
                      .arg(previous));
  });

  row->addWidget(first);
  row->addWidget(second);
  row->addStretch();
  layout->addWidget(rowBox);
  layout->addWidget(info);
  return box;
}

QWidget* ImageDocsPage::buildPreviewGroupVisibleDemo() {
  auto* box = makeRow();
  auto* row = qobject_cast<QHBoxLayout*>(box->layout());

  auto* group = new AdImagePreviewGroup(box);
  group->setItems({kWebpA, kWebpB, kWebpC});

  auto* image = createImage(kWebpA, 200, QStringLiteral("webp image"), box);
  image->setPreviewGroup(group);
  row->addWidget(image);
  row->addStretch();
  return box;
}

QWidget* ImageDocsPage::buildPreviewSrcDemo() {
  auto* box = makeRow();
  auto* row = qobject_cast<QHBoxLayout*>(box->layout());
  auto* image = createImage(kBlurPng, 200, QStringLiteral("preview src"), box);
  image->setPreviewSrc(kBasicPng);
  row->addWidget(image);
  row->addStretch();
  return box;
}

QWidget* ImageDocsPage::buildControlledPreviewDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* controls = new QHBoxLayout();
  controls->setContentsMargins(0, 0, 0, 0);
  controls->setSpacing(6);
  controls->addWidget(new QLabel(QStringLiteral("scaleStep:")));
  auto* step = new QDoubleSpinBox();
  step->setRange(0.1, 5.0);
  step->setSingleStep(0.1);
  step->setValue(0.5);
  controls->addWidget(step);
  controls->addStretch();
  layout->addLayout(controls);

  auto* openButton = new QPushButton(QStringLiteral("show image preview"));
  layout->addWidget(openButton, 0, Qt::AlignLeft);

  auto* image = createImage(kBlurPng, 200, QStringLiteral("controlled preview"), box);
  image->setPreviewSrc(kBasicPng);
  image->setPreviewOpenControlled(true);
  image->setVisible(false);
  layout->addWidget(image);

  connect(step,
          QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          image,
          &AdImage::setPreviewScaleStep);
  connect(openButton, &QPushButton::clicked, image, [image]() { image->setPreviewOpen(true); });
  connect(image, &AdImage::onPreviewOpenChange, box, [image](bool open) {
    if (!open) {
      image->setPreviewOpen(false);
    }
  });

  return box;
}

QWidget* ImageDocsPage::buildToolbarRenderDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* group = new AdImagePreviewGroup(box);
  auto* rowBox = makeRow(box);
  auto* row = qobject_cast<QHBoxLayout*>(rowBox->layout());
  auto* first = createImage(kSvgA, 200, QStringLiteral("image-0"), rowBox);
  auto* second = createImage(kSvgB, 200, QStringLiteral("image-1"), rowBox);
  first->setPreviewGroup(group);
  second->setPreviewGroup(group);
  row->addWidget(first);
  row->addWidget(second);
  row->addStretch();
  layout->addWidget(rowBox);

  auto* toolbar = new QHBoxLayout();
  toolbar->setContentsMargins(0, 0, 0, 0);
  toolbar->setSpacing(6);
  auto* open = new QPushButton(QStringLiteral("Open"));
  auto* prev = new QPushButton(QStringLiteral("Prev"));
  auto* next = new QPushButton(QStringLiteral("Next"));
  auto* zoomIn = new QPushButton(QStringLiteral("Zoom+"));
  auto* zoomOut = new QPushButton(QStringLiteral("Zoom-"));
  auto* rotateL = new QPushButton(QStringLiteral("Rotate L"));
  auto* rotateR = new QPushButton(QStringLiteral("Rotate R"));
  auto* reset = new QPushButton(QStringLiteral("Reset"));
  toolbar->addWidget(open);
  toolbar->addWidget(prev);
  toolbar->addWidget(next);
  toolbar->addWidget(zoomIn);
  toolbar->addWidget(zoomOut);
  toolbar->addWidget(rotateL);
  toolbar->addWidget(rotateR);
  toolbar->addWidget(reset);
  toolbar->addStretch();
  layout->addLayout(toolbar);

  connect(open, &QPushButton::clicked, group, [group]() { group->setOpen(true); });
  connect(prev, &QPushButton::clicked, group, [group]() { group->activate(-1); });
  connect(next, &QPushButton::clicked, group, [group]() { group->activate(1); });
  connect(zoomIn, &QPushButton::clicked, group, &AdImagePreviewGroup::zoomIn);
  connect(zoomOut, &QPushButton::clicked, group, &AdImagePreviewGroup::zoomOut);
  connect(rotateL, &QPushButton::clicked, group, &AdImagePreviewGroup::rotateLeft);
  connect(rotateR, &QPushButton::clicked, group, &AdImagePreviewGroup::rotateRight);
  connect(reset, &QPushButton::clicked, group, &AdImagePreviewGroup::resetTransform);

  return box;
}

QWidget* ImageDocsPage::buildImageRenderDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* image = createImage(kBasicPng, 200, QStringLiteral("custom preview render"), box);
  layout->addWidget(image, 0, Qt::AlignLeft);

  auto* hint = new QLabel(
      QStringLiteral("Qt demo adaptation: use external dialog content alongside preview actions."));
  hint->setWordWrap(true);
  layout->addWidget(hint);

  auto* openCustom = new QPushButton(QStringLiteral("Open custom render dialog"));
  layout->addWidget(openCustom, 0, Qt::AlignLeft);
  connect(openCustom, &QPushButton::clicked, box, [box]() {
    auto* dialog = new QDialog(box);
    dialog->setAttribute(Qt::WA_DeleteOnClose, true);
    dialog->setWindowTitle(QStringLiteral("Custom preview content"));
    auto* root = new QVBoxLayout(dialog);
    root->addWidget(new QLabel(QStringLiteral("Custom render area (video placeholder)")));
    root->addWidget(new QLabel(QStringLiteral(
        "In Ant Design this is `preview.imageRender`; here it is represented by a custom dialog body.")));
    dialog->resize(420, 180);
    dialog->show();
  });

  return box;
}

QWidget* ImageDocsPage::buildMaskDemo() {
  auto* box = makeRow();
  auto* row = qobject_cast<QHBoxLayout*>(box->layout());

  auto* blur = createImage(kBasicPng, 100, QStringLiteral("blur"), box);
  blur->setPreviewMaskBlur(true);
  blur->setPreviewCoverText(QStringLiteral("blur"));

  auto* dimmed = createImage(kSvgA, 100, QStringLiteral("dimmed"), box);
  dimmed->setPreviewCoverText(QStringLiteral("dimmed"));

  auto* noMask = createImage(kSvgB, 100, QStringLiteral("no mask"), box);
  noMask->setPreviewMaskVisible(false);
  noMask->setPreviewCoverText(QStringLiteral("No mask"));

  row->addWidget(blur);
  row->addWidget(dimmed);
  row->addWidget(noMask);
  row->addStretch();
  return box;
}

QWidget* ImageDocsPage::buildStyleClassDemo() {
  auto* box = makeRow();
  auto* row = qobject_cast<QHBoxLayout*>(box->layout());

  auto* first = createImage(kBasicPng, 160, QStringLiteral("style object"), box);
  AdImage::SemanticStyles styles;
  styles.root.borderColor = QColor(QStringLiteral("#A594F9"));
  styles.cover.backgroundColor = QColor(70, 52, 160, 160);
  styles.cover.textColor = QColor(Qt::white);
  first->setSemanticStyles(styles);

  auto* second = createImage(kBasicPng, 160, QStringLiteral("style resolver"), box);
  second->setSemanticStyleResolver([](const AdImage::StyleContext& context) {
    AdImage::SemanticStyles out;
    if (context.previewEnabled) {
      out.root.borderColor = QColor(QStringLiteral("#4A90E2"));
      out.cover.backgroundColor = QColor(20, 20, 20, 156);
      out.cover.textColor = QColor(QStringLiteral("#E6F4FF"));
    }
    return out;
  });

  row->addWidget(first);
  row->addWidget(second);
  row->addStretch();
  return box;
}

QWidget* ImageDocsPage::buildPreviewMaskDemo() {
  auto* box = makeRow();
  auto* row = qobject_cast<QHBoxLayout*>(box->layout());

  auto* image = createImage(kBasicPng, 120, QStringLiteral("custom mask"), box);
  image->setPreviewCoverText(QStringLiteral("Example"));
  AdImage::SemanticStyles semantic;
  semantic.popupMask.backgroundColor = QColor(60, 39, 147, 200);
  image->setSemanticStyles(semantic);

  row->addWidget(image);
  row->addStretch();
  return box;
}

QWidget* ImageDocsPage::buildCoverPlacementDemo() {
  auto* box = makeRow();
  auto* row = qobject_cast<QHBoxLayout*>(box->layout());

  auto* center = createImage(kBasicPng, 96, QStringLiteral("center"), box);
  center->setPreviewCoverText(QStringLiteral("center"));
  center->setPreviewCoverPlacement(AdImage::CoverPlacement::Center);

  auto* top = createImage(kBasicPng, 96, QStringLiteral("top"), box);
  top->setPreviewCoverText(QStringLiteral("top"));
  top->setPreviewCoverPlacement(AdImage::CoverPlacement::Top);

  auto* bottom = createImage(kBasicPng, 96, QStringLiteral("bottom"), box);
  bottom->setPreviewCoverText(QStringLiteral("bottom"));
  bottom->setPreviewCoverPlacement(AdImage::CoverPlacement::Bottom);

  row->addWidget(center);
  row->addWidget(top);
  row->addWidget(bottom);
  row->addStretch();
  return box;
}

QWidget* ImageDocsPage::buildNestedDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* button = new QPushButton(QStringLiteral("showModal"));
  layout->addWidget(button, 0, Qt::AlignLeft);

  connect(button, &QPushButton::clicked, box, [box]() {
    auto createLevel = [box](const QString& title, QWidget* content, std::function<void()> openNext) {
      auto* dialog = new QDialog(box);
      dialog->setAttribute(Qt::WA_DeleteOnClose, true);
      dialog->setWindowTitle(title);
      auto* root = new QVBoxLayout(dialog);
      if (content) {
        root->addWidget(content);
      }
      if (openNext) {
        auto* next = new QPushButton(QStringLiteral("open next modal"), dialog);
        root->addWidget(next, 0, Qt::AlignLeft);
        QObject::connect(next, &QPushButton::clicked, dialog, [openNext]() { openNext(); });
      }
      dialog->resize(500, 300);
      dialog->show();
      return dialog;
    };

    auto openThird = [box, createLevel]() {
      auto* holder = new QWidget();
      auto* root = new QVBoxLayout(holder);
      root->setContentsMargins(0, 0, 0, 0);
      auto* group = new AdImagePreviewGroup(holder);
      auto* row = new QHBoxLayout();
      row->setContentsMargins(0, 0, 0, 0);
      auto* first = ImageDocsPage::createImage(kSvgA, 180, QStringLiteral("nested-1"), holder);
      auto* second = ImageDocsPage::createImage(kSvgB, 180, QStringLiteral("nested-2"), holder);
      first->setPreviewGroup(group);
      second->setPreviewGroup(group);
      row->addWidget(first);
      row->addWidget(second);
      row->addStretch();
      root->addLayout(row);
      createLevel(QStringLiteral("Modal level 3"), holder, {});
    };

    auto openSecond = [box, createLevel, openThird]() {
      createLevel(QStringLiteral("Modal level 2"), nullptr, openThird);
    };

    createLevel(QStringLiteral("Modal level 1"), nullptr, openSecond);
  });

  return box;
}

QWidget* ImageDocsPage::buildPreviewGroupTopProgressDemo() {
  auto* box = makeRow();
  auto* row = qobject_cast<QHBoxLayout*>(box->layout());

  auto* group = new AdImagePreviewGroup(box);
  group->setCountRenderFormat(QStringLiteral("Current %1 / Total %2"));

  auto* first = createImage(kSvgA, 150, QStringLiteral("a"), box);
  auto* second = createImage(kSvgB, 150, QStringLiteral("b"), box);
  auto* third = createImage(kBasicPng, 150, QStringLiteral("c"), box);
  first->setPreviewGroup(group);
  second->setPreviewGroup(group);
  third->setPreviewGroup(group);

  row->addWidget(first);
  row->addWidget(second);
  row->addWidget(third);
  row->addStretch();
  return box;
}

QWidget* ImageDocsPage::buildComponentTokenDemo() {
  auto* box = makeRow();
  auto* row = qobject_cast<QHBoxLayout*>(box->layout());

  auto* group = new AdImagePreviewGroup(box);
  group->setCountRenderFormat(QStringLiteral("Current %1 / Total %2"));
  AdImagePreviewGroup::ComponentTokens groupTokens;
  groupTokens.previewOperationSize = 20;
  groupTokens.previewOperationColor = QStringLiteral("#222222");
  groupTokens.previewOperationColorDisabled = QStringLiteral("#b20000");
  group->setComponentTokens(groupTokens);

  auto* first = createImage(kSvgB, 150, QStringLiteral("svg image"), box);
  auto* second = createImage(kBasicPng, 150, QStringLiteral("basic image"), box);
  first->setPreviewGroup(group);
  second->setPreviewGroup(group);

  row->addWidget(first);
  row->addWidget(second);
  row->addStretch();
  return box;
}

QWidget* ImageDocsPage::buildPreviewImageInfoDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* image = createImage(kBasicPng, 200, QStringLiteral("test"), box);
  layout->addWidget(image, 0, Qt::AlignLeft);

  auto* info = new QLabel(QStringLiteral("{ \"url\": \"\", \"alt\": \"\", \"width\": 0, \"height\": 0 }"));
  info->setWordWrap(true);
  layout->addWidget(info);

  connect(image, &AdImage::previewImageInfoChanged, box, [info](const QString& src,
                                                                 const QString& alt,
                                                                 int width,
                                                                 int height) {
    info->setText(QStringLiteral("{ \"url\": \"%1\", \"alt\": \"%2\", \"width\": %3, \"height\": %4 }")
                      .arg(src)
                      .arg(alt)
                      .arg(width)
                      .arg(height));
  });

  return box;
}
