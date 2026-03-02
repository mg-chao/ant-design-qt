#pragma once

#include "button.h"

#include <QList>
#include <QWidget>

class QHBoxLayout;

namespace adqt::widgets {

class AdButtonGroup final : public QWidget {
  Q_OBJECT

  Q_PROPERTY(adqt::widgets::AdButton::Size size READ size WRITE setSize NOTIFY sizeChanged)

 public:
  explicit AdButtonGroup(QWidget* parent = nullptr);
  ~AdButtonGroup() override = default;

  AdButton::Size size() const;
  void setSize(AdButton::Size value);

  void addButton(AdButton* button);
  void removeButton(AdButton* button);
  QList<AdButton*> buttons() const;

 signals:
  void sizeChanged(AdButton::Size value);

 private:
  void refreshButtons();
  void refreshLayoutSpacing();

  QHBoxLayout* layout_ = nullptr;
  QList<AdButton*> buttons_;
  AdButton::Size size_ = AdButton::Size::Middle;
};

}  // namespace adqt::widgets

