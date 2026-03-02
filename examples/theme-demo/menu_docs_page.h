#pragma once

#include <QStringList>
#include <QVector>
#include <QWidget>

#include "widgets/widgets.h"

class QVBoxLayout;

class MenuDocsPage final : public QWidget {
 public:
  explicit MenuDocsPage(QWidget* parent = nullptr);

  const QVector<QWidget*>& sectionAnchors() const;
  const QStringList& sectionTitles() const;

 private:
  void addSection(QVBoxLayout* root,
                  const QString& title,
                  const QString& description,
                  QWidget* content);

  QWidget* buildHorizontalDemo(bool dark);
  QWidget* buildInlineDemo();
  QWidget* buildInlineCollapsedDemo();
  QWidget* buildTooltipDemo();
  QWidget* buildSiderCurrentDemo();
  QWidget* buildVerticalDemo();
  QWidget* buildThemeDemo();
  QWidget* buildSubMenuThemeDemo();
  QWidget* buildSwitchModeDemo();
  QWidget* buildStyleClassDemo();
  QWidget* buildStyleDebugDemo();
  QWidget* buildMenuV4Demo();
  QWidget* buildComponentTokenDemo();
  QWidget* buildExtraStyleDemo();
  QWidget* buildCustomPopupRenderDemo();
  QWidget* buildSemanticDemo();
  QWidget* buildApiOverview();

  QVector<adqt::widgets::AdMenu::Item> itemsHorizontal() const;
  QVector<adqt::widgets::AdMenu::Item> itemsInline() const;
  QVector<adqt::widgets::AdMenu::Item> itemsCollapsedInline() const;
  QVector<adqt::widgets::AdMenu::Item> itemsSiderCurrent() const;
  QVector<adqt::widgets::AdMenu::Item> itemsSwitchMode() const;

  QVector<QWidget*> anchors_;
  QStringList titles_;
};
