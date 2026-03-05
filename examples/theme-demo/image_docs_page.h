#pragma once

#include <QStringList>
#include <QVector>
#include <QWidget>

#include "widgets/widgets.h"

class QVBoxLayout;

class ImageDocsPage final : public QWidget {
 public:
  explicit ImageDocsPage(QWidget* parent = nullptr);

  const QVector<QWidget*>& sectionAnchors() const;
 const QStringList& sectionTitles() const;

 private:
  using AdImage = adqt::widgets::AdImage;
  using AdImagePreviewGroup = adqt::widgets::AdImagePreviewGroup;

  void addSection(QVBoxLayout* root,
                  const QString& title,
                  const QString& description,
                  QWidget* content);

  QWidget* buildBasicDemo();
  QWidget* buildFallbackDemo();
  QWidget* buildPlaceholderDemo();
  QWidget* buildPreviewGroupDemo();
  QWidget* buildPreviewGroupVisibleDemo();
  QWidget* buildPreviewSrcDemo();
  QWidget* buildControlledPreviewDemo();
  QWidget* buildToolbarRenderDemo();
  QWidget* buildImageRenderDemo();
  QWidget* buildMaskDemo();
  QWidget* buildStyleClassDemo();
  QWidget* buildPreviewMaskDemo();
  QWidget* buildCoverPlacementDemo();
  QWidget* buildNestedDemo();
  QWidget* buildPreviewGroupTopProgressDemo();
  QWidget* buildComponentTokenDemo();
  QWidget* buildPreviewImageInfoDemo();

  static AdImage* createImage(const QString& src,
                              int width,
                              const QString& alt = QStringLiteral("image"),
                              QWidget* parent = nullptr);

  QVector<QWidget*> anchors_;
  QStringList titles_;
};
