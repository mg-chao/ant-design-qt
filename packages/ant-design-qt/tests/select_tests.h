#pragma once

#include <QObject>

class SelectTests final : public QObject {
  Q_OBJECT

 private slots:
  void exposesQtModelProperties();
  void popupWidthModeRoundTrips();
  void setCurrentValueTracksCurrentModelIndex();
  void setCurrentValuesSyncSelectionModel();
  void tagsPreserveCustomValues();
  void modelColumnUsesAlternateColumn();
  void externalSelectionModelUpdatesWidget();
  void accessibleRoleMatchesMode();
  void externalSearchPolicyBypassesLocalFiltering();
  void keyboardNavigationActivatesCurrentRow();
  void multiPopupSelectionReflectsSelectionState();
  void comboBoxForwardsQtCurrentApi();
  void multiSelectSupportsQtStyleSelectionApi();
  void tagSelectSupportsQtStyleSelectionApi();
};

int runSelectTests(int argc, char** argv);
