#include "button_group.h"

#include "theme/theme.h"

#include <QHBoxLayout>

#include <algorithm>

namespace adqt::widgets {

namespace {

int groupSpacing() {
  const int lineWidth =
      qRound(adqt::theme::ThemeManager::instance().currentMapToken().lineWidth);
  return -std::max(1, lineWidth);
}

}  // namespace

AdButtonGroup::AdButtonGroup(QWidget* parent) : QWidget(parent) {
  layout_ = new QHBoxLayout(this);
  layout_->setContentsMargins(0, 0, 0, 0);
  layout_->setSpacing(groupSpacing());

  connect(&adqt::theme::ThemeManager::instance(), &adqt::theme::ThemeManager::themeChanged, this,
          [this]() { refreshLayoutSpacing(); });
}

AdButton::Size AdButtonGroup::size() const { return size_; }

void AdButtonGroup::setSize(AdButton::Size value) {
  if (size_ == value) {
    return;
  }

  size_ = value;
  refreshButtons();
  emit sizeChanged(size_);
}

void AdButtonGroup::addButton(AdButton* button) {
  if (!button || buttons_.contains(button)) {
    return;
  }

  if (button->parentWidget() != this) {
    button->setParent(this);
  }

  layout_->addWidget(button);
  buttons_.append(button);

  connect(button, &QObject::destroyed, this, [this, button]() {
    buttons_.removeAll(button);
    refreshButtons();
  });

  refreshButtons();
}

void AdButtonGroup::removeButton(AdButton* button) {
  if (!button) {
    return;
  }

  if (!buttons_.removeOne(button)) {
    return;
  }

  layout_->removeWidget(button);
  button->setGroupPosition(AdButton::GroupPosition::None);
  button->setGroupSizeContext(size_, false);
  refreshButtons();
}

QList<AdButton*> AdButtonGroup::buttons() const { return buttons_; }

void AdButtonGroup::refreshButtons() {
  for (int i = 0; i < buttons_.size(); ++i) {
    AdButton* button = buttons_.at(i);
    if (!button) {
      continue;
    }

    AdButton::GroupPosition position = AdButton::GroupPosition::None;
    if (buttons_.size() == 1) {
      position = AdButton::GroupPosition::Only;
    } else if (i == 0) {
      position = AdButton::GroupPosition::First;
    } else if (i == buttons_.size() - 1) {
      position = AdButton::GroupPosition::Last;
    } else {
      position = AdButton::GroupPosition::Middle;
    }

    button->setGroupPosition(position);
    button->setGroupSizeContext(size_, true);
  }
}

void AdButtonGroup::refreshLayoutSpacing() {
  if (!layout_) {
    return;
  }
  layout_->setSpacing(groupSpacing());
}

}  // namespace adqt::widgets
