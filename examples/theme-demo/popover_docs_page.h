#pragma once

#include <QStringList>
#include <QVector>
#include <QWidget>

#include "widgets/widgets.h"

class QVBoxLayout;

class PopoverDocsPage final : public QWidget {
 public:
  explicit PopoverDocsPage(QWidget* parent = nullptr);

  const QVector<QWidget*>& sectionAnchors() const;
  const QStringList& sectionTitles() const;

 private:
  using Placement = adqt::widgets::AdPopover::Placement;
  using Trigger = adqt::widgets::AdPopover::Trigger;
  using Triggers = adqt::widgets::AdPopover::Triggers;

  void addSection(QVBoxLayout* root,
                  const QString& title,
                  const QString& description,
                  QWidget* content);

  QWidget* buildBasicDemo();
  QWidget* buildTriggerDemo();
  QWidget* buildPlacementDemo();
  QWidget* buildArrowDemo();
  QWidget* buildAutoShiftDemo();
  QWidget* buildControlledDemo();
  QWidget* buildHoverWithClickDemo();
  QWidget* buildSemanticDemo();
  QWidget* buildComponentTokenDemo();
  QWidget* buildApiOverview();

  adqt::widgets::AdPopover* makePopover(const QString& triggerText,
                                        const QString& title,
                                        const QString& content,
                                        Triggers triggers,
                                        QWidget* parent = nullptr);

  QVector<QWidget*> anchors_;
  QStringList titles_;
};

