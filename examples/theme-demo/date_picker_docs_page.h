#pragma once

#include <QStringList>
#include <QVector>
#include <QWidget>

#include "widgets/widgets.h"

class QVBoxLayout;

class DatePickerDocsPage final : public QWidget {
 public:
  explicit DatePickerDocsPage(QWidget* parent = nullptr);

  const QVector<QWidget*>& sectionAnchors() const;
  const QStringList& sectionTitles() const;

 private:
  void addSection(QVBoxLayout* root,
                  const QString& title,
                  const QString& description,
                  QWidget* content);

  QWidget* buildBasicDemo();
  QWidget* buildRangeDemo();
  QWidget* buildMultipleDemo();
  QWidget* buildModeDemo();
  QWidget* buildFormatDemo();
  QWidget* buildTimeDemo();
  QWidget* buildConstraintsDemo();
  QWidget* buildPresetsDemo();
  QWidget* buildFooterDemo();
  QWidget* buildSizeStatusVariantDemo();

  QVector<QWidget*> anchors_;
  QStringList titles_;
};
