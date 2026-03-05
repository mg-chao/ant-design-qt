#include "color_picker_docs_page.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPalette>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include <algorithm>

using adqt::widgets::AdButton;
using adqt::widgets::AdColorPicker;

namespace {

QLabel* makeHintLabel(const QString& text, QWidget* parent = nullptr) {
  auto* label = new QLabel(text, parent);
  label->setWordWrap(true);
  QPalette palette = label->palette();
  palette.setColor(QPalette::WindowText, QColor("#8c8c8c"));
  label->setPalette(palette);
  return label;
}

QColor parseCssColor(const QString& css, const QColor& fallback) {
  QColor color(css.trimmed());
  if (!color.isValid()) {
    return fallback;
  }
  return color;
}

}  // namespace

ColorPickerDocsPage::ColorPickerDocsPage(QWidget* parent) : QWidget(parent) {
  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(16, 16, 16, 24);
  root->setSpacing(16);

  auto* title = new QLabel("ColorPicker");
  QFont titleFont = title->font();
  titleFont.setPointSize(titleFont.pointSize() + 8);
  titleFont.setBold(true);
  title->setFont(titleFont);
  root->addWidget(title);

  auto* subtitle = new QLabel("Used for color selection.");
  subtitle->setWordWrap(true);
  root->addWidget(subtitle);

  addSection(root, "Basic Usage", "Demo: base.tsx", buildBaseDemo());
  addSection(root, "Trigger size", "Demo: size.tsx", buildSizeDemo());
  addSection(root, "controlled mode", "Demo: controlled.tsx", buildControlledDemo());
  addSection(root, "Line Gradient", "Demo: line-gradient.tsx", buildLineGradientDemo());
  addSection(root, "Rendering Trigger Text", "Demo: text-render.tsx", buildTextRenderDemo());
  addSection(root, "Disable", "Demo: disabled.tsx", buildDisabledDemo());
  addSection(root, "Disabled Alpha", "Demo: disabled-alpha.tsx", buildDisabledAlphaDemo());
  addSection(root, "Clear Color", "Demo: allowClear.tsx", buildAllowClearDemo());
  addSection(root, "Custom Trigger", "Demo: trigger.tsx", buildTriggerDemo());
  addSection(root, "Custom Trigger Event", "Demo: trigger-event.tsx", buildTriggerEventDemo());
  addSection(root, "Color Format", "Demo: format.tsx", buildFormatDemo());
  addSection(root, "Preset Colors", "Demo: presets.tsx", buildPresetsDemo());
  addSection(root, "Custom Render Panel", "Demo: panel-render.tsx", buildPanelRenderDemo());
  addSection(root, "Custom semantic dom styling", "Demo: style-class.tsx", buildStyleClassDemo());

  root->addStretch();
}

const QVector<QWidget*>& ColorPickerDocsPage::sectionAnchors() const { return anchors_; }

const QStringList& ColorPickerDocsPage::sectionTitles() const { return titles_; }

void ColorPickerDocsPage::addSection(QVBoxLayout* root,
                                     const QString& title,
                                     const QString& description,
                                     QWidget* content) {
  auto* panel = new QFrame();
  panel->setFrameShape(QFrame::StyledPanel);
  auto* layout = new QVBoxLayout(panel);
  layout->setContentsMargins(14, 14, 14, 14);
  layout->setSpacing(10);

  auto* titleLabel = new QLabel(title);
  QFont titleFont = titleLabel->font();
  titleFont.setBold(true);
  titleFont.setPointSize(titleFont.pointSize() + 1);
  titleLabel->setFont(titleFont);

  auto* descLabel = new QLabel(description);
  descLabel->setWordWrap(true);

  layout->addWidget(titleLabel);
  layout->addWidget(descLabel);
  layout->addWidget(content);

  root->addWidget(panel);
  anchors_.append(panel);
  titles_.append(title);
}

ColorPickerDocsPage::ColorValue ColorPickerDocsPage::solid(const QString& value) {
  ColorValue color;
  color.solidColor = value;
  return color;
}

ColorPickerDocsPage::ColorValue ColorPickerDocsPage::gradient(const QVector<GradientStop>& stops) {
  ColorValue color;
  color.gradientStops = stops;
  return color;
}

QWidget* ColorPickerDocsPage::buildBaseDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(8);

  auto* picker = new AdColorPicker();
  picker->setValue(QStringLiteral("#1677ff"));
  row->addWidget(picker);
  row->addStretch();
  return box;
}

QWidget* ColorPickerDocsPage::buildSizeDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(24);

  auto* left = new QVBoxLayout();
  left->setSpacing(8);
  auto* right = new QVBoxLayout();
  right->setSpacing(8);

  const QList<AdColorPicker::Size> sizes = {
      AdColorPicker::Size::Small,
      AdColorPicker::Size::Middle,
      AdColorPicker::Size::Large,
  };

  for (AdColorPicker::Size size : sizes) {
    auto* picker = new AdColorPicker();
    picker->setValue(QStringLiteral("#1677ff"));
    picker->setSize(size);
    left->addWidget(picker, 0, Qt::AlignLeft);

    auto* pickerText = new AdColorPicker();
    pickerText->setValue(QStringLiteral("#1677ff"));
    pickerText->setSize(size);
    pickerText->setShowText(true);
    right->addWidget(pickerText, 0, Qt::AlignLeft);
  }

  row->addLayout(left);
  row->addLayout(right);
  row->addStretch();
  return box;
}

QWidget* ColorPickerDocsPage::buildControlledDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* row = new QHBoxLayout();
  row->setSpacing(12);

  auto* onChangePicker = new AdColorPicker();
  onChangePicker->setValue(QStringLiteral("#1677ff"));

  auto* onCompletePicker = new AdColorPicker();
  onCompletePicker->setValue(QStringLiteral("#1677ff"));

  auto syncBoth = [onChangePicker, onCompletePicker](const ColorValue& value) {
    {
      QSignalBlocker blocker(onChangePicker);
      onChangePicker->setColorValue(value);
    }
    {
      QSignalBlocker blocker(onCompletePicker);
      onCompletePicker->setColorValue(value);
    }
  };

  connect(onChangePicker, &AdColorPicker::colorValueChanged, this,
          [syncBoth](const ColorValue& value) { syncBoth(value); });
  connect(onCompletePicker, &AdColorPicker::changeCompleted, this,
          [syncBoth](const ColorValue& value) { syncBoth(value); });

  auto* liveLabel = makeHintLabel("onChange: #1677ff");
  auto* completeLabel = makeHintLabel("onChangeComplete: #1677ff");

  connect(onChangePicker, &AdColorPicker::valueChanged, this,
          [liveLabel](const QString& css) { liveLabel->setText(QStringLiteral("onChange: %1").arg(css)); });
  connect(onCompletePicker, &AdColorPicker::changeCompleted, this, [completeLabel, onCompletePicker](const ColorValue&) {
    completeLabel->setText(QStringLiteral("onChangeComplete: %1").arg(onCompletePicker->value()));
  });

  row->addWidget(onChangePicker);
  row->addWidget(onCompletePicker);
  row->addStretch();

  layout->addLayout(row);
  layout->addWidget(liveLabel);
  layout->addWidget(completeLabel);
  return box;
}

QWidget* ColorPickerDocsPage::buildLineGradientDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  const ColorValue defaultGradient = gradient({
      GradientStop{QStringLiteral("rgb(16, 142, 233)"), 0},
      GradientStop{QStringLiteral("rgb(135, 208, 104)"), 100},
  });

  auto* mixed = new AdColorPicker();
  mixed->setModeOptions({AdColorPicker::Mode::Single, AdColorPicker::Mode::Gradient});
  mixed->setMode(AdColorPicker::Mode::Gradient);
  mixed->setAllowClear(true);
  mixed->setShowText(true);
  mixed->setColorValue(defaultGradient);

  auto* gradientOnly = new AdColorPicker();
  gradientOnly->setModeOptions({AdColorPicker::Mode::Gradient});
  gradientOnly->setMode(AdColorPicker::Mode::Gradient);
  gradientOnly->setAllowClear(true);
  gradientOnly->setShowText(true);
  gradientOnly->setColorValue(defaultGradient);

  auto* output = makeHintLabel("onChangeComplete: linear-gradient(...)");
  connect(mixed, &AdColorPicker::changeCompleted, this,
          [output, mixed](const ColorValue&) { output->setText(QStringLiteral("onChangeComplete: %1").arg(mixed->value())); });
  connect(gradientOnly, &AdColorPicker::changeCompleted, this,
          [output, gradientOnly](const ColorValue&) {
            output->setText(QStringLiteral("onChangeComplete: %1").arg(gradientOnly->value()));
          });

  layout->addWidget(mixed, 0, Qt::AlignLeft);
  layout->addWidget(gradientOnly, 0, Qt::AlignLeft);
  layout->addWidget(output);
  return box;
}

QWidget* ColorPickerDocsPage::buildTextRenderDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* defaultText = new AdColorPicker();
  defaultText->setValue(QStringLiteral("#1677ff"));
  defaultText->setShowText(true);
  defaultText->setAllowClear(true);

  auto* customText = new AdColorPicker();
  customText->setValue(QStringLiteral("#1677ff"));
  customText->setShowText(true);
  customText->setShowTextFormatter([](const ColorValue& value, AdColorPicker::Format, int) {
    if (value.cleared) {
      return QStringLiteral("Custom Text (none)");
    }
    if (!value.solidColor.trimmed().isEmpty()) {
      return QStringLiteral("Custom Text (%1)").arg(value.solidColor);
    }
    return QStringLiteral("Custom Text (gradient)");
  });

  auto* arrowText = new AdColorPicker();
  arrowText->setValue(QStringLiteral("#1677ff"));
  arrowText->setShowText(true);
  arrowText->setShowTextFormatter([arrowText](const ColorValue&, AdColorPicker::Format, int) {
    return arrowText && arrowText->open() ? QStringLiteral("^") : QStringLiteral("v");
  });

  layout->addWidget(defaultText, 0, Qt::AlignLeft);
  layout->addWidget(customText, 0, Qt::AlignLeft);
  layout->addWidget(arrowText, 0, Qt::AlignLeft);
  return box;
}

QWidget* ColorPickerDocsPage::buildDisabledDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);

  auto* picker = new AdColorPicker();
  picker->setValue(QStringLiteral("#1677ff"));
  picker->setShowText(true);
  picker->setDisabled(true);

  row->addWidget(picker);
  row->addStretch();
  return box;
}

QWidget* ColorPickerDocsPage::buildDisabledAlphaDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);

  auto* picker = new AdColorPicker();
  picker->setValue(QStringLiteral("#1677ff"));
  picker->setDisabledAlpha(true);

  row->addWidget(picker);
  row->addStretch();
  return box;
}

QWidget* ColorPickerDocsPage::buildAllowClearDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* picker = new AdColorPicker();
  picker->setAllowClear(true);
  picker->setColorValue(solid(QStringLiteral("#1677ff")));

  auto* output = makeHintLabel("value: #1677ff");
  connect(picker, &AdColorPicker::valueChanged, this,
          [output](const QString& css) { output->setText(QStringLiteral("value: %1").arg(css)); });
  connect(picker, &AdColorPicker::onClear, this,
          [output]() { output->setText(QStringLiteral("value: <cleared>")); });

  layout->addWidget(picker, 0, Qt::AlignLeft);
  layout->addWidget(output);
  return box;
}

QWidget* ColorPickerDocsPage::buildTriggerDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(8);

  auto* picker = new AdColorPicker();
  picker->setColorValue(solid(QStringLiteral("#1677ff")));

  auto* triggerButton = new AdButton(QStringLiteral("open"));
  triggerButton->setType(AdButton::Type::Primary);
  triggerButton->setMinimumWidth(86);

  auto applyColor = [triggerButton](const QString& css) {
    const QColor color = parseCssColor(css, QColor("#1677ff"));
    triggerButton->setStyleSheet(QStringLiteral("background:%1;").arg(color.name(QColor::HexRgb)));
  };
  applyColor(QStringLiteral("#1677ff"));

  connect(picker, &AdColorPicker::valueChanged, this, [applyColor](const QString& css) { applyColor(css); });

  picker->setTriggerWidget(triggerButton);

  row->addWidget(picker, 0, Qt::AlignLeft);
  row->addStretch();
  return box;
}

QWidget* ColorPickerDocsPage::buildTriggerEventDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);

  auto* picker = new AdColorPicker();
  picker->setValue(QStringLiteral("#1677ff"));
  picker->setTrigger(AdColorPicker::Trigger::Hover);

  row->addWidget(picker);
  row->addWidget(makeHintLabel("Trigger mode: hover"));
  row->addStretch();
  return box;
}

QWidget* ColorPickerDocsPage::buildFormatDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(10);

  auto buildRow = [this](AdColorPicker::Format format,
                         const QString& initial,
                         const QString& prefix) -> QWidget* {
    auto* rowHost = new QWidget();
    auto* row = new QHBoxLayout(rowHost);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(8);

    auto* picker = new AdColorPicker();
    picker->setFormat(format);
    picker->setValue(initial);

    auto* text = new QLabel(QStringLiteral("%1: %2").arg(prefix).arg(picker->value()));
    connect(picker, &AdColorPicker::valueChanged, rowHost,
            [text, prefix](const QString& css) {
              text->setText(QStringLiteral("%1: %2").arg(prefix).arg(css));
            });
    connect(picker, &AdColorPicker::formatChanged, rowHost, [text, picker, prefix](AdColorPicker::Format) {
      text->setText(QStringLiteral("%1: %2").arg(prefix).arg(picker->value()));
    });

    row->addWidget(picker);
    row->addWidget(text);
    row->addStretch();
    return rowHost;
  };

  layout->addWidget(buildRow(AdColorPicker::Format::Hex, QStringLiteral("#1677ff"), QStringLiteral("HEX")));
  layout->addWidget(buildRow(AdColorPicker::Format::Hsb, QStringLiteral("hsb(215, 91%, 100%)"),
                             QStringLiteral("HSB")));
  layout->addWidget(buildRow(AdColorPicker::Format::Rgb, QStringLiteral("rgb(22, 119, 255)"),
                             QStringLiteral("RGB")));
  return box;
}

QWidget* ColorPickerDocsPage::buildPresetsDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(8);

  AdColorPicker::PresetItem primary;
  primary.label = QStringLiteral("primary");
  primary.colors = {
      solid(QStringLiteral("#1677ff")),
      solid(QStringLiteral("#4096ff")),
      solid(QStringLiteral("#69b1ff")),
      solid(QStringLiteral("#91caff")),
  };

  AdColorPicker::PresetItem red;
  red.label = QStringLiteral("red");
  red.colors = {
      solid(QStringLiteral("#ff4d4f")),
      solid(QStringLiteral("#ff7875")),
      solid(QStringLiteral("#ffa39e")),
      solid(QStringLiteral("#ffd8bf")),
  };

  AdColorPicker::PresetItem green;
  green.label = QStringLiteral("green");
  green.colors = {
      solid(QStringLiteral("#52c41a")),
      solid(QStringLiteral("#73d13d")),
      solid(QStringLiteral("#95de64")),
      solid(QStringLiteral("#b7eb8f")),
  };

  auto* picker = new AdColorPicker();
  picker->setValue(QStringLiteral("#1677ff"));
  picker->setPresets({primary, red, green});

  row->addWidget(picker, 0, Qt::AlignLeft);
  row->addStretch();
  return box;
}

QWidget* ColorPickerDocsPage::buildPanelRenderDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* basic = new AdColorPicker();
  basic->setValue(QStringLiteral("#1677ff"));
  basic->setPanelRenderFactory([](QWidget* parent, QWidget* pickerPanel, QWidget* presetsPanel) {
    auto* wrapper = new QWidget(parent);
    auto* wrapperLayout = new QVBoxLayout(wrapper);
    wrapperLayout->setContentsMargins(0, 0, 0, 0);
    wrapperLayout->setSpacing(8);
    auto* heading = new QLabel(QStringLiteral("Color Picker"), wrapper);
    QFont font = heading->font();
    font.setPointSize(std::max(10, font.pointSize()));
    heading->setFont(font);
    wrapperLayout->addWidget(heading);
    wrapperLayout->addWidget(pickerPanel);
    if (presetsPanel) {
      wrapperLayout->addWidget(presetsPanel);
    }
    return wrapper;
  });

  AdColorPicker::PresetItem gradients;
  gradients.label = QStringLiteral("presets");
  gradients.colors = {
      solid(QStringLiteral("#1677ff")),
      solid(QStringLiteral("#52c41a")),
      solid(QStringLiteral("#f5222d")),
  };

  auto* horizontal = new AdColorPicker();
  horizontal->setValue(QStringLiteral("#1677ff"));
  horizontal->setPresets({gradients});

  AdColorPicker::ComponentTokens tokens;
  tokens.panelWidth = 480;
  horizontal->setComponentTokens(tokens);

  horizontal->setPanelRenderFactory([](QWidget* parent, QWidget* pickerPanel, QWidget* presetsPanel) {
    auto* wrapper = new QWidget(parent);
    auto* row = new QHBoxLayout(wrapper);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(10);
    if (presetsPanel) {
      row->addWidget(presetsPanel, 0);
    }
    row->addWidget(pickerPanel, 1);
    return wrapper;
  });

  auto* row1 = new QHBoxLayout();
  row1->addWidget(new QLabel(QStringLiteral("Add title:")));
  row1->addWidget(basic);
  row1->addStretch();

  auto* row2 = new QHBoxLayout();
  row2->addWidget(new QLabel(QStringLiteral("Horizontal layout:")));
  row2->addWidget(horizontal);
  row2->addStretch();

  layout->addLayout(row1);
  layout->addLayout(row2);
  return box;
}

QWidget* ColorPickerDocsPage::buildStyleClassDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(12);

  auto* objectStyle = new AdColorPicker();
  objectStyle->setValue(QStringLiteral("#1677ff"));
  AdColorPicker::SemanticStyles fixedStyles;
  fixedStyles.root.backgroundColor = QColor("#ffffff");
  fixedStyles.root.borderColor = QColor("#34477d");
  fixedStyles.description.textColor = QColor("#34477d");
  fixedStyles.popup.backgroundColor = QColor("#f5f8ff");
  fixedStyles.popup.borderColor = QColor("#34477d");
  objectStyle->setSemanticStyles(fixedStyles);

  auto* resolverStyle = new AdColorPicker();
  resolverStyle->setValue(QStringLiteral("#722ed1"));
  resolverStyle->setSize(AdColorPicker::Size::Large);
  resolverStyle->setSemanticStyleResolver([](const AdColorPicker::StyleContext& ctx) {
    AdColorPicker::SemanticStyles styles;
    if (ctx.open) {
      styles.root.borderColor = QColor("#722ed1");
      styles.description.textColor = QColor("#531dab");
      styles.popup.borderColor = QColor("#722ed1");
      styles.popup.backgroundColor = QColor("#f9f0ff");
    } else {
      styles.root.borderColor = QColor("#d9d9d9");
      styles.description.textColor = QColor("#595959");
    }
    return styles;
  });

  row->addWidget(objectStyle);
  row->addWidget(resolverStyle);
  row->addStretch();
  return box;
}
