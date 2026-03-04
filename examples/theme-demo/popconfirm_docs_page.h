#pragma once

#include <QStringList>
#include <QVector>
#include <QWidget>

#include "widgets/widgets.h"

class QVBoxLayout;

class PopconfirmDocsPage final : public QWidget {
 public:
  explicit PopconfirmDocsPage(QWidget* parent = nullptr);

  const QVector<QWidget*>& sectionAnchors() const;
  const QStringList& sectionTitles() const;

 private:
  using Placement = adqt::widgets::AdPopconfirm::Placement;
  using Trigger = adqt::widgets::AdPopconfirm::Trigger;
  using Triggers = adqt::widgets::AdPopconfirm::Triggers;

  void addSection(QVBoxLayout* root,
                  const QString& title,
                  const QString& description,
                  QWidget* content);

  adqt::widgets::AdPopconfirm* makePopconfirm(const QString& triggerText,
                                              const QString& title,
                                              const QString& description,
                                              Triggers triggers = Trigger::Click,
                                              QWidget* parent = nullptr);

  QWidget* buildBasicDemo();
  QWidget* buildLocaleDemo();
  QWidget* buildPlacementDemo();
  QWidget* buildAutoShiftDemo();
  QWidget* buildDynamicTriggerDemo();
  QWidget* buildIconDemo();
  QWidget* buildAsyncDemo();
  QWidget* buildPromiseDemo();
  QWidget* buildStyleClassDemo();
  QWidget* buildRenderPanelDemo();
  QWidget* buildWireframeDemo();

  QVector<QWidget*> anchors_;
  QStringList titles_;
};
