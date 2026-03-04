#pragma once

#include <QStringList>
#include <QVector>
#include <QWidget>

#include "widgets/widgets.h"

class QVBoxLayout;

class RadioDocsPage final : public QWidget {
 public:
  explicit RadioDocsPage(QWidget* parent = nullptr);

  const QVector<QWidget*>& sectionAnchors() const;
  const QStringList& sectionTitles() const;

 private:
  using RadioOption = adqt::widgets::AdRadioGroup::Option;

  void addSection(QVBoxLayout* root,
                  const QString& title,
                  const QString& description,
                  QWidget* content);

  QVector<RadioOption> cityOptions() const;
  QVector<RadioOption> fruitOptions(bool disableOrange = false) const;

  QWidget* buildBasicDemo();
  QWidget* buildDisabledDemo();
  QWidget* buildRadioGroupDemo();
  QWidget* buildVerticalGroupDemo();
  QWidget* buildBlockGroupDemo();
  QWidget* buildGroupOptionsDemo();
  QWidget* buildRadioButtonDemo();
  QWidget* buildGroupWithNameDemo();
  QWidget* buildSizeDemo();
  QWidget* buildSolidButtonDemo();
  QWidget* buildStyleClassDemo();

  QVector<QWidget*> anchors_;
  QStringList titles_;
};

