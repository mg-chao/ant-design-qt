#include "radio_group.h"

#include "radio_style.h"
#include "theme/theme.h"

#include <QBoxLayout>
#include <QHBoxLayout>
#include <QScopedValueRollback>
#include <QVBoxLayout>

#include <algorithm>

namespace adqt::widgets {

AdRadioGroup::AdRadioGroup(QWidget* parent) : QWidget(parent) {
  ensureLayoutForOrientation();

  connect(&adqt::theme::ThemeManager::instance(), &adqt::theme::ThemeManager::themeChanged, this,
          [this]() {
            updateLayoutSpacing();
            applyGroupStateToRadios();
          });
}

AdRadioGroup::~AdRadioGroup() = default;

QVariant AdRadioGroup::value() const { return value_; }

void AdRadioGroup::setValue(const QVariant& value) {
  if (value_ == value) {
    syncCheckedFromValue();
    return;
  }

  value_ = value;
  syncCheckedFromValue();
  emit valueChanged(value_);
}

bool AdRadioGroup::disabled() const { return disabled_; }

void AdRadioGroup::setDisabled(bool value) {
  if (disabled_ == value) {
    return;
  }
  disabled_ = value;
  setEnabled(!disabled_);
  emit disabledChanged(disabled_);
}

AdRadio::Size AdRadioGroup::size() const { return size_; }

void AdRadioGroup::setSize(AdRadio::Size value) {
  if (size_ == value) {
    return;
  }
  size_ = value;
  applyGroupStateToRadios();
  emit sizeChanged(size_);
}

AdRadio::OptionType AdRadioGroup::optionType() const { return optionType_; }

void AdRadioGroup::setOptionType(AdRadio::OptionType value) {
  if (optionType_ == value) {
    return;
  }
  optionType_ = value;
  applyGroupStateToRadios();
  emit optionTypeChanged(optionType_);
}

AdRadio::ButtonStyle AdRadioGroup::buttonStyle() const { return buttonStyle_; }

void AdRadioGroup::setButtonStyle(AdRadio::ButtonStyle value) {
  if (buttonStyle_ == value) {
    return;
  }
  buttonStyle_ = value;
  applyGroupStateToRadios();
  emit buttonStyleChanged(buttonStyle_);
}

bool AdRadioGroup::block() const { return block_; }

void AdRadioGroup::setBlock(bool value) {
  if (block_ == value) {
    return;
  }
  block_ = value;
  applyGroupStateToRadios();
  emit blockChanged(block_);
}

AdRadioGroup::Orientation AdRadioGroup::orientation() const { return orientation_; }

void AdRadioGroup::setOrientation(Orientation value) {
  const bool changed = orientation_ != value;
  orientation_ = value;
  orientationExplicit_ = true;
  if (changed) {
    ensureLayoutForOrientation();
    applyGroupStateToRadios();
    emit orientationChanged(orientation_);
  }
}

bool AdRadioGroup::vertical() const { return vertical_; }

void AdRadioGroup::setVertical(bool value) {
  if (vertical_ == value) {
    return;
  }
  vertical_ = value;
  if (!orientationExplicit_) {
    ensureLayoutForOrientation();
    applyGroupStateToRadios();
  }
  emit verticalChanged(vertical_);
}

QString AdRadioGroup::name() const { return name_; }

void AdRadioGroup::setName(const QString& value) {
  if (name_ == value) {
    return;
  }
  name_ = value;
  for (AdRadio* radio : std::as_const(radios_)) {
    if (!radio) {
      continue;
    }
    radio->setGroupName(name_);
  }
  emit nameChanged(name_);
}

QVector<AdRadioGroup::Option> AdRadioGroup::options() const { return options_; }

void AdRadioGroup::setOptions(const QVector<Option>& options) {
  options_ = options;
  rebuildRadios();
  emit optionsChanged();
}

AdRadio::ComponentTokens AdRadioGroup::componentTokens() const { return componentTokens_; }

void AdRadioGroup::setComponentTokens(const AdRadio::ComponentTokens& tokens) {
  componentTokens_ = tokens;
  applyGroupStateToRadios();
  emit componentTokensChanged();
}

void AdRadioGroup::resetComponentTokens() {
  componentTokens_ = {};
  applyGroupStateToRadios();
  emit componentTokensChanged();
}

AdRadio::SemanticStyles AdRadioGroup::semanticStyles() const { return semanticStyles_; }

void AdRadioGroup::setSemanticStyles(const AdRadio::SemanticStyles& styles) {
  semanticStyles_ = styles;
  applyGroupStateToRadios();
  emit semanticStylesChanged();
}

void AdRadioGroup::setSemanticStyleResolver(SemanticStyleResolver resolver) {
  semanticStyleResolver_ = std::move(resolver);
  applyGroupStateToRadios();
  emit semanticStylesChanged();
}

AdRadioGroup::Orientation AdRadioGroup::effectiveOrientation() const {
  if (orientationExplicit_) {
    return orientation_;
  }
  return vertical_ ? Orientation::Vertical : Orientation::Horizontal;
}

AdRadioGroup::StyleContext AdRadioGroup::currentStyleContext() const {
  StyleContext ctx;
  ctx.size = size_;
  ctx.optionType = optionType_;
  ctx.buttonStyle = buttonStyle_;
  ctx.disabled = disabled_;
  ctx.block = block_;
  ctx.orientation = effectiveOrientation();
  ctx.vertical = vertical_;
  ctx.value = value_;
  ctx.options = options_;
  return ctx;
}

AdRadio::SemanticStyles AdRadioGroup::resolvedSemanticStyles() const {
  AdRadio::SemanticStyles merged = semanticStyles_;
  if (!semanticStyleResolver_) {
    return merged;
  }

  const AdRadio::SemanticStyles resolved = semanticStyleResolver_(currentStyleContext());
  auto mergeSlot = [](AdRadio::SemanticSlotStyle* target, const AdRadio::SemanticSlotStyle& source) {
    if (source.textColor.has_value()) {
      target->textColor = source.textColor;
    }
    if (source.backgroundColor.has_value()) {
      target->backgroundColor = source.backgroundColor;
    }
    if (source.borderColor.has_value()) {
      target->borderColor = source.borderColor;
    }
  };

  mergeSlot(&merged.root, resolved.root);
  mergeSlot(&merged.icon, resolved.icon);
  mergeSlot(&merged.label, resolved.label);
  return merged;
}

void AdRadioGroup::ensureLayoutForOrientation() {
  const bool needVertical = effectiveOrientation() == Orientation::Vertical;
  const bool hasVerticalLayout = qobject_cast<QVBoxLayout*>(layout_) != nullptr;
  if (layout_ && hasVerticalLayout == needVertical) {
    updateLayoutSpacing();
    return;
  }

  if (layout_) {
    delete layout_;
    layout_ = nullptr;
  }

  if (needVertical) {
    layout_ = new QVBoxLayout(this);
  } else {
    layout_ = new QHBoxLayout(this);
  }
  layout_->setContentsMargins(0, 0, 0, 0);

  for (AdRadio* radio : std::as_const(radios_)) {
    if (!radio) {
      continue;
    }
    layout_->addWidget(radio);
  }
  updateLayoutSpacing();
}

void AdRadioGroup::updateLayoutSpacing() {
  if (!layout_) {
    return;
  }

  const adqt::theme::ThemeMapToken& map = adqt::theme::ThemeManager::instance().currentMapToken();
  int spacing = 0;
  if (effectiveOrientation() == Orientation::Horizontal && optionType_ == AdRadio::OptionType::Button) {
    // Qt treats negative spacing as "use style default spacing" instead of overlap.
    // Keep button items directly adjacent and let paint clipping decide shared borders.
    spacing = 0;
  } else if (effectiveOrientation() == Orientation::Vertical) {
    // Align with antd rowGap token: marginXS.
    spacing = std::max(0, qRound(map.sizeXS));
  } else {
    detail::RadioStyleInput input;
    input.size = size_;
    input.optionType = optionType_;
    input.buttonStyle = buttonStyle_;
    input.block = block_;
    input.baseFont = font();
    input.componentTokens = componentTokens_;
    input.semanticStyles = resolvedSemanticStyles();
    const detail::RadioVisualStyle style = detail::resolveRadioVisualStyle(input);
    spacing = std::max(0, style.metrics.wrapperMarginInlineEnd);
  }
  layout_->setSpacing(spacing);
}

void AdRadioGroup::rebuildRadios() {
  for (AdRadio* radio : std::as_const(radios_)) {
    if (radio) {
      radio->deleteLater();
    }
  }
  radios_.clear();

  ensureLayoutForOrientation();

  for (const Option& option : std::as_const(options_)) {
    auto* radio = new AdRadio(this);
    radio->setText(option.label);
    radio->setValue(option.value);
    radio->setDisabled(option.disabled);
    radio->setToolTip(option.title);
    if (!option.id.isEmpty()) {
      radio->setObjectName(option.id);
    }
    if (!option.className.isEmpty()) {
      radio->setProperty("className", option.className);
    }
    if (option.required) {
      radio->setProperty("required", true);
    }
    connect(radio, &AdRadio::changed, this, [this, radio](const QVariant& value, bool checked) {
      onRadioChanged(radio, value, checked);
    });
    radios_.append(radio);
    layout_->addWidget(radio);
  }

  applyGroupStateToRadios();
}

void AdRadioGroup::applyGroupStateToRadios() {
  ensureLayoutForOrientation();
  const AdRadio::SemanticStyles mergedSemantic = resolvedSemanticStyles();
  const bool verticalLayout = effectiveOrientation() == Orientation::Vertical;

  for (int i = 0; i < radios_.size(); ++i) {
    AdRadio* radio = radios_.at(i);
    if (!radio) {
      continue;
    }

    const bool optionDisabled = (i >= 0 && i < options_.size()) ? options_.at(i).disabled : false;
    radio->setDisabled(optionDisabled);
    radio->setSize(size_);
    radio->setOptionType(optionType_);
    radio->setButtonStyle(buttonStyle_);
    radio->setBlock(block_);
    radio->setComponentTokens(componentTokens_);
    radio->setSemanticStyles(mergedSemantic);
    radio->setGroupVertical(verticalLayout);
    radio->setGroupName(name_);
    if (block_) {
      radio->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    } else {
      radio->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    }
  }

  updateGroupPositions();
  syncCheckedFromValue();
  if (optionType_ == AdRadio::OptionType::Button) {
    for (AdRadio* radio : std::as_const(radios_)) {
      if (radio && radio->isChecked()) {
        radio->raise();
      }
    }
  }
  updateLayoutSpacing();
  updateGeometry();
  update();
}

void AdRadioGroup::syncCheckedFromValue() {
  QScopedValueRollback<bool> guard(syncing_, true);
  for (AdRadio* radio : std::as_const(radios_)) {
    if (!radio) {
      continue;
    }
    const bool shouldCheck = radio->value() == value_;
    if (radio->isChecked() != shouldCheck) {
      radio->setChecked(shouldCheck);
    }
  }
}

void AdRadioGroup::updateGroupPositions() {
  if (optionType_ != AdRadio::OptionType::Button || radios_.isEmpty()) {
    for (AdRadio* radio : std::as_const(radios_)) {
      if (radio) {
        radio->setGroupPosition(AdRadio::GroupPosition::None);
      }
    }
    return;
  }

  for (int i = 0; i < radios_.size(); ++i) {
    AdRadio* radio = radios_.at(i);
    if (!radio) {
      continue;
    }

    AdRadio::GroupPosition position = AdRadio::GroupPosition::None;
    if (radios_.size() == 1) {
      position = AdRadio::GroupPosition::Only;
    } else if (i == 0) {
      position = AdRadio::GroupPosition::First;
    } else if (i == radios_.size() - 1) {
      position = AdRadio::GroupPosition::Last;
    } else {
      position = AdRadio::GroupPosition::Middle;
    }
    radio->setGroupPosition(position);
  }
}

void AdRadioGroup::onRadioChanged(AdRadio* radio, const QVariant& value, bool checked) {
  if (!radio || syncing_ || !checked) {
    return;
  }
  if (value_ == value) {
    return;
  }

  value_ = value;
  syncCheckedFromValue();
  emit valueChanged(value_);
  emit changed(value_);
}

}  // namespace adqt::widgets
