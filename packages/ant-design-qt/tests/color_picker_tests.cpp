#include <QAbstractButton>
#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QMetaProperty>
#include <QPainter>
#include <QSignalSpy>
#include <QVBoxLayout>
#include <QtTest>

#include "widgets/color_picker.h"
#include "widgets/detail/color_picker_value_model.h"

namespace {

class ColorPreviewTrigger final : public QWidget {
 public:
  explicit ColorPreviewTrigger(QWidget* parent = nullptr) : QWidget(parent) {
    setFixedSize(86, 32);
  }

  void setColor(const QColor& color) {
    color_ = color;
    update();
  }

  QColor color() const { return color_; }

 protected:
  void paintEvent(QPaintEvent* event) override {
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(QColor(QStringLiteral("#d9d9d9")), 1.0));
    painter.setBrush(color_);
    painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 6, 6);
  }

 private:
  QColor color_ = QColor(QStringLiteral("#1677ff"));
};

QWidget* findWidgetByObjectName(const QString& objectName) {
  const auto widgets = QApplication::allWidgets();
  for (QWidget* widget : widgets) {
    if (widget && widget->objectName() == objectName) {
      return widget;
    }
  }
  return nullptr;
}

class ColorPickerTests final : public QObject {
  Q_OBJECT

 private slots:
  void exposesQtFirstProperties();
  void selectionModelExposesQtStateObjectContract();
  void programmaticSettersSkipEditingFinished();
  void cssValueRoundTripsSolidAndGradient();
  void valueModelNormalizesGradientState();
  void selectionModelSynchronizesPickerAndPanel();
  void keyboardInteractionFollowsQtContract();
  void defaultTriggerKeepsUpdatesEnabledDuringFirstPopupEdit();
  void gradientPopupHeaderStaysPaintableDuringFirstPopupEdit();
  void customTriggerRetainsOpaqueColorAfterFirstPopupEdit();
  void hexInputReservesPrefixSpaceBeforeHover();
};

void ColorPickerTests::exposesQtFirstProperties() {
  adqt::widgets::AdColorPicker picker;
  const QMetaObject* meta = picker.metaObject();
  adqt::widgets::AdColorPickerPanel panel;
  const QMetaObject* panelMeta = panel.metaObject();

  const int triggerTextVisibleIndex = meta->indexOfProperty("triggerTextVisible");
  QVERIFY(triggerTextVisibleIndex >= 0);
  QVERIFY(meta->property(triggerTextVisibleIndex).isReadable());
  QVERIFY(meta->property(triggerTextVisibleIndex).isWritable());

  const int popupVisibleIndex = meta->indexOfProperty("popupVisible");
  QVERIFY(popupVisibleIndex >= 0);
  QVERIFY(meta->property(popupVisibleIndex).isReadable());
  QVERIFY(meta->property(popupVisibleIndex).isWritable());

  const int alphaChannelEnabledIndex = meta->indexOfProperty("alphaChannelEnabled");
  QVERIFY(alphaChannelEnabledIndex >= 0);
  QVERIFY(meta->property(alphaChannelEnabledIndex).isReadable());
  QVERIFY(meta->property(alphaChannelEnabledIndex).isWritable());

  const int formatSelectorEnabledIndex = meta->indexOfProperty("formatSelectorEnabled");
  QVERIFY(formatSelectorEnabledIndex >= 0);
  QVERIFY(meta->property(formatSelectorEnabledIndex).isReadable());
  QVERIFY(meta->property(formatSelectorEnabledIndex).isWritable());

  const int triggerContentIndex = meta->indexOfProperty("triggerContent");
  QVERIFY(triggerContentIndex >= 0);
  QVERIFY(meta->property(triggerContentIndex).isReadable());
  QVERIFY(meta->property(triggerContentIndex).isWritable());

  const int modeOptionsIndex = meta->indexOfProperty("modeOptions");
  QVERIFY(modeOptionsIndex >= 0);
  QVERIFY(meta->property(modeOptionsIndex).isReadable());
  QVERIFY(meta->property(modeOptionsIndex).isWritable());

  const int presetsIndex = meta->indexOfProperty("presets");
  QVERIFY(presetsIndex >= 0);
  QVERIFY(meta->property(presetsIndex).isReadable());
  QVERIFY(meta->property(presetsIndex).isWritable());

  const int valueIndex = meta->indexOfProperty("value");
  QVERIFY(valueIndex >= 0);
  QVERIFY(meta->property(valueIndex).isReadable());
  QVERIFY(meta->property(valueIndex).isWritable());

  const int stateIndex = meta->indexOfProperty("state");
  QVERIFY(stateIndex >= 0);
  QVERIFY(meta->property(stateIndex).isReadable());
  QVERIFY(meta->property(stateIndex).isWritable());

  const int panelModeOptionsIndex = panelMeta->indexOfProperty("modeOptions");
  QVERIFY(panelModeOptionsIndex >= 0);
  QVERIFY(panelMeta->property(panelModeOptionsIndex).isReadable());
  QVERIFY(panelMeta->property(panelModeOptionsIndex).isWritable());

  const int panelPresetsIndex = panelMeta->indexOfProperty("presets");
  QVERIFY(panelPresetsIndex >= 0);
  QVERIFY(panelMeta->property(panelPresetsIndex).isReadable());
  QVERIFY(panelMeta->property(panelPresetsIndex).isWritable());

  const int panelValueIndex = panelMeta->indexOfProperty("value");
  QVERIFY(panelValueIndex >= 0);
  QVERIFY(panelMeta->property(panelValueIndex).isReadable());
  QVERIFY(panelMeta->property(panelValueIndex).isWritable());

  const int panelStateIndex = panelMeta->indexOfProperty("state");
  QVERIFY(panelStateIndex >= 0);
  QVERIFY(panelMeta->property(panelStateIndex).isReadable());
  QVERIFY(panelMeta->property(panelStateIndex).isWritable());

  QCOMPARE(meta->indexOfProperty("cssValue"), -1);
  QCOMPARE(meta->indexOfProperty("formattedValue"), -1);
  QCOMPARE(meta->indexOfProperty("colorValue"), -1);
  QCOMPARE(meta->indexOfProperty("showText"), -1);
  QCOMPARE(meta->indexOfProperty("open"), -1);
  QCOMPARE(meta->indexOfProperty("disabledAlpha"), -1);
  QCOMPARE(meta->indexOfProperty("disabledFormat"), -1);
  QCOMPARE(meta->indexOfProperty("model"), -1);
  QCOMPARE(meta->indexOfProperty("customTrigger"), -1);

  QVERIFY(picker.setProperty("triggerTextVisible", true));
  QVERIFY(picker.triggerTextVisible());

  QVERIFY(picker.setProperty("alphaChannelEnabled", false));
  QVERIFY(!picker.alphaChannelEnabled());

  const QVector<adqt::widgets::AdColorPicker::Mode> modeOptions = {
      adqt::widgets::AdColorPicker::Mode::Solid,
      adqt::widgets::AdColorPicker::Mode::Gradient,
  };
  QVERIFY(picker.setProperty("modeOptions", QVariant::fromValue(modeOptions)));
  QCOMPARE(picker.modeOptions(), modeOptions);

  const adqt::widgets::AdColorValue value =
      adqt::widgets::AdColorValue::solid(QColor(QStringLiteral("#52c41a")));
  QVERIFY(picker.setProperty("value", QVariant::fromValue(value)));
  QCOMPARE(picker.value(), value);

  adqt::widgets::AdColorPicker::PresetItem preset;
  preset.label = QStringLiteral("brand");
  preset.colors = {value};
  const QVector<adqt::widgets::AdColorPicker::PresetItem> presets = {preset};
  QVERIFY(picker.setProperty("presets", QVariant::fromValue(presets)));
  QCOMPARE(picker.presets().size(), 1);
  QCOMPARE(picker.presets().constFirst().label, QStringLiteral("brand"));
}

void ColorPickerTests::selectionModelExposesQtStateObjectContract() {
  using adqt::widgets::AdColorPicker;
  using adqt::widgets::AdColorPickerState;
  using adqt::widgets::AdColorValue;

  AdColorPickerState state;
  const QMetaObject* meta = state.metaObject();

  const int modeOptionsIndex = meta->indexOfProperty("modeOptions");
  QVERIFY(modeOptionsIndex >= 0);
  QVERIFY(meta->property(modeOptionsIndex).isReadable());
  QVERIFY(meta->property(modeOptionsIndex).isWritable());

  const int presetsIndex = meta->indexOfProperty("presets");
  QVERIFY(presetsIndex >= 0);
  QVERIFY(meta->property(presetsIndex).isReadable());
  QVERIFY(meta->property(presetsIndex).isWritable());

  const int cssTextIndex = meta->indexOfProperty("cssText");
  QVERIFY(cssTextIndex >= 0);
  QVERIFY(meta->property(cssTextIndex).isReadable());
  QVERIFY(meta->property(cssTextIndex).isWritable());

  const int displayTextIndex = meta->indexOfProperty("displayText");
  QVERIFY(displayTextIndex >= 0);
  QVERIFY(meta->property(displayTextIndex).isReadable());
  QVERIFY(!meta->property(displayTextIndex).isWritable());

  const int valueIndex = meta->indexOfProperty("value");
  QVERIFY(valueIndex >= 0);
  QVERIFY(meta->property(valueIndex).isReadable());
  QVERIFY(meta->property(valueIndex).isWritable());

  QSignalSpy stateSpy(&state, &AdColorPickerState::stateChanged);

  state.setCssText(QStringLiteral("#1677ff"));
  QCOMPARE(stateSpy.count(), 1);

  state.setCssText(QStringLiteral("#1677ff"));
  QCOMPARE(stateSpy.count(), 1);

  const QVector<AdColorPicker::Mode> modeOptions = {
      AdColorPicker::Mode::Solid,
      AdColorPicker::Mode::Gradient,
  };
  QVERIFY(state.setProperty("modeOptions", QVariant::fromValue(modeOptions)));
  QCOMPARE(state.modeOptions(), modeOptions);
  QCOMPARE(stateSpy.count(), 2);

  const AdColorValue value = AdColorValue::solid(QColor(QStringLiteral("#52c41a")));
  AdColorPicker::PresetItem preset;
  preset.label = QStringLiteral("semantic");
  preset.colors = {value};
  const QVector<AdColorPicker::PresetItem> presets = {preset};
  QVERIFY(state.setProperty("presets", QVariant::fromValue(presets)));
  QCOMPARE(state.presets().size(), 1);
  QCOMPARE(state.presets().constFirst().label, QStringLiteral("semantic"));
  QCOMPARE(stateSpy.count(), 3);
}

void ColorPickerTests::programmaticSettersSkipEditingFinished() {
  using Mode = adqt::widgets::AdColorPicker::Mode;

  adqt::widgets::AdColorPicker picker;
  picker.setModeOptions({Mode::Solid, Mode::Gradient});

  QSignalSpy editingSpy(&picker, &adqt::widgets::AdColorPicker::editingFinished);

  picker.setCssText(QStringLiteral("#1677ff"));
  picker.setValue(adqt::widgets::AdColorValue::solid(QColor(QStringLiteral("#52c41a"))));
  picker.setCssText(
      QStringLiteral("linear-gradient(90deg, rgb(22,119,255) 0%, rgb(82,196,26) 100%)"));

  QCOMPARE(editingSpy.count(), 0);
}

void ColorPickerTests::cssValueRoundTripsSolidAndGradient() {
  using Mode = adqt::widgets::AdColorPicker::Mode;

  adqt::widgets::AdColorPicker picker;
  picker.setCssText(QStringLiteral("#1677ff"));

  const adqt::widgets::AdColorValue solidValue = picker.value();
  QVERIFY(solidValue.isSolid());
  QCOMPARE(solidValue.solidColor, QColor(QStringLiteral("#1677ff")));
  QCOMPARE(picker.cssText(), QStringLiteral("rgb(22,119,255)"));

  picker.setModeOptions({Mode::Solid, Mode::Gradient});
  picker.setCssText(
      QStringLiteral("linear-gradient(90deg, rgb(82,196,26) 100%, rgb(22,119,255) 0%)"));

  const adqt::widgets::AdColorValue gradientValue = picker.value();
  QVERIFY(gradientValue.isGradient());
  QCOMPARE(gradientValue.gradientStops.size(), 2);
  QCOMPARE(gradientValue.gradientStops.at(0).first, 0.0);
  QCOMPARE(gradientValue.gradientStops.at(0).second, QColor(QStringLiteral("#1677ff")));
  QCOMPARE(gradientValue.gradientStops.at(1).first, 1.0);
  QCOMPARE(gradientValue.gradientStops.at(1).second, QColor(QStringLiteral("#52c41a")));
  QCOMPARE(picker.cssText(),
           QStringLiteral("linear-gradient(90deg, rgb(22,119,255) 0%, rgb(82,196,26) 100%)"));
}

void ColorPickerTests::valueModelNormalizesGradientState() {
  using Model = adqt::widgets::detail::ColorPickerValueModel;
  using Mode = adqt::widgets::AdColorPicker::Mode;

  const adqt::widgets::AdColorSelection initial = adqt::widgets::AdColorSelection::gradient(
      {adqt::widgets::AdColorGradientStop{QColor(QStringLiteral("#52c41a")), 100},
       adqt::widgets::AdColorGradientStop{QColor(QStringLiteral("#1677ff")), 0}});

  const Model::State gradientState =
      Model::stateFromSelection(initial, {Mode::Solid, Mode::Gradient}, Mode::Gradient);
  QVERIFY(gradientState.selection.isGradient());
  QCOMPARE(gradientState.selection.gradientStops.size(), 2);
  QCOMPARE(gradientState.selection.gradientStops.at(0).percent, 0);
  QCOMPARE(gradientState.selection.gradientStops.at(0).color, QColor(QStringLiteral("#1677ff")));
  QCOMPARE(gradientState.selection.gradientStops.at(1).percent, 100);
  QCOMPARE(gradientState.selection.gradientStops.at(1).color, QColor(QStringLiteral("#52c41a")));
  QCOMPARE(gradientState.mode, Mode::Gradient);

  const Model::State singleOnly = Model::withModeOptions(gradientState, {Mode::Solid});
  QVERIFY(singleOnly.selection.isSolid());
  QCOMPARE(singleOnly.selection.solidColor, QColor(QStringLiteral("#1677ff")));
  QCOMPARE(singleOnly.mode, Mode::Solid);
}

void ColorPickerTests::selectionModelSynchronizesPickerAndPanel() {
  using adqt::widgets::AdColorGradientStop;
  using adqt::widgets::AdColorPicker;
  using adqt::widgets::AdColorPickerState;
  using adqt::widgets::AdColorPickerPanel;
  using adqt::widgets::AdColorValue;

  AdColorPickerState state;
  state.setModeOptions({AdColorPicker::Mode::Solid, AdColorPicker::Mode::Gradient});
  state.setCssText(QStringLiteral("#1677ff"));

  AdColorPicker picker;
  AdColorPickerPanel panel;
  picker.setState(&state);
  panel.setState(&state);

  QCOMPARE(picker.state(), &state);
  QCOMPARE(panel.state(), &state);
  QCOMPARE(picker.cssText(), QStringLiteral("rgb(22,119,255)"));
  QCOMPARE(panel.cssText(), QStringLiteral("rgb(22,119,255)"));

  const AdColorValue gradientValue = AdColorValue::gradient(
      QGradientStops{{0.0, QColor(QStringLiteral("#1677ff"))},
                     {1.0, QColor(QStringLiteral("#52c41a"))}},
      QColor(QStringLiteral("#1677ff")));
  state.setValue(gradientValue);
  QCoreApplication::processEvents();

  QVERIFY(picker.value().isGradient());
  QVERIFY(panel.value().isGradient());
  QCOMPARE(picker.cssText(),
           QStringLiteral("linear-gradient(90deg, rgb(22,119,255) 0%, rgb(82,196,26) 100%)"));
  QCOMPARE(panel.cssText(),
           QStringLiteral("linear-gradient(90deg, rgb(22,119,255) 0%, rgb(82,196,26) 100%)"));

  panel.setFormat(AdColorPicker::Format::Hsb);
  QCoreApplication::processEvents();
  QCOMPARE(state.format(), AdColorPicker::Format::Hsb);
  QCOMPARE(picker.format(), AdColorPicker::Format::Hsb);

  picker.setValue(AdColorValue::solid(QColor(QStringLiteral("#52c41a"))));
  QCoreApplication::processEvents();
  QCOMPARE(state.cssText(), QStringLiteral("rgb(82,196,26)"));
  QCOMPARE(panel.cssText(), QStringLiteral("rgb(82,196,26)"));
}

void ColorPickerTests::keyboardInteractionFollowsQtContract() {
  using Mode = adqt::widgets::AdColorPicker::Mode;

  adqt::widgets::AdColorPicker picker;
  picker.setModeOptions({Mode::Solid, Mode::Gradient});
  picker.setCssText(QStringLiteral("#1677ff"));
  picker.show();
  QTest::qWait(1);

  auto* trigger =
      picker.findChild<QAbstractButton*>(QStringLiteral("ad-color-picker-trigger-frame"));
  QVERIFY(trigger);
  QVERIFY(trigger->focusPolicy() != Qt::NoFocus);
  QVERIFY(!trigger->accessibleName().isEmpty());
  QVERIFY(!trigger->accessibleDescription().isEmpty());

  trigger->setFocus();
  QVERIFY(trigger->hasFocus());
  QTest::keyClick(trigger, Qt::Key_Down, Qt::AltModifier);
  QTRY_VERIFY(picker.popupVisible());

  auto* saturation =
      picker.findChild<QWidget*>(QStringLiteral("ad-color-picker-saturation-panel"));
  QVERIFY(saturation);
  QVERIFY(saturation->focusPolicy() != Qt::NoFocus);
  QVERIFY(!saturation->accessibleName().isEmpty());
  QVERIFY(!saturation->accessibleDescription().isEmpty());

  const adqt::widgets::AdColorValue before = picker.value();
  saturation->setFocus();
  QVERIFY(saturation->hasFocus());
  QTest::keyClick(saturation, Qt::Key_Left);
  QCoreApplication::processEvents();
  QVERIFY(picker.value() != before);
}

void ColorPickerTests::customTriggerRetainsOpaqueColorAfterFirstPopupEdit() {
  using adqt::widgets::AdColorPicker;
  using adqt::widgets::AdColorValue;

  QWidget host;
  auto* layout = new QVBoxLayout(&host);
  layout->setContentsMargins(0, 0, 0, 0);

  AdColorPicker picker;
  picker.setValue(AdColorValue::solid(QColor(QStringLiteral("#1677ff"))));

  auto* trigger = new ColorPreviewTrigger();
  auto syncTriggerColor = [trigger](const AdColorValue& value) {
    if (value.isGradient() && !value.gradientStops.isEmpty()) {
      trigger->setColor(value.gradientStops.constFirst().second);
      return;
    }
    trigger->setColor(value.solidColor);
  };
  syncTriggerColor(picker.value());
  connect(&picker, &AdColorPicker::valueChanged, &host, syncTriggerColor);
  picker.setTriggerContent(trigger);
  layout->addWidget(&picker);

  host.show();
  QVERIFY(QTest::qWaitForWindowExposed(&host));
  QVERIFY(trigger->isVisible());
  QCOMPARE(trigger->color().alpha(), 255);

  picker.setPopupVisible(true);
  QTRY_VERIFY(picker.popupVisible());

  QWidget* saturationPanel = nullptr;
  QTRY_VERIFY((saturationPanel = findWidgetByObjectName(QStringLiteral("ad-color-picker-saturation-panel"))) != nullptr);
  QVERIFY(saturationPanel->isVisible());

  const QPoint clickPoint(4, saturationPanel->height() - 4);
  QTest::mousePress(saturationPanel, Qt::LeftButton, Qt::NoModifier, clickPoint);
  QTest::mouseRelease(saturationPanel, Qt::LeftButton, Qt::NoModifier, clickPoint);
  QCoreApplication::processEvents();

  picker.setPopupVisible(false);
  QTRY_VERIFY(!picker.popupVisible());
  QCoreApplication::processEvents();

  QVERIFY(trigger->isVisible());
  QVERIFY2(trigger->color().isValid(), "custom trigger lost its color after first popup edit");
  QVERIFY2(trigger->color().alpha() > 0, "custom trigger became transparent after first popup edit");
}

void ColorPickerTests::defaultTriggerKeepsUpdatesEnabledDuringFirstPopupEdit() {
  using adqt::widgets::AdColorPicker;
  using adqt::widgets::AdColorValue;

  QWidget host;
  auto* layout = new QVBoxLayout(&host);
  layout->setContentsMargins(0, 0, 0, 0);

  AdColorPicker picker;
  picker.setTriggerTextVisible(true);
  picker.setValue(AdColorValue::solid(QColor(QStringLiteral("#1677ff"))));
  layout->addWidget(&picker);

  host.show();
  QVERIFY(QTest::qWaitForWindowExposed(&host));

  auto* triggerFrame =
      picker.findChild<QAbstractButton*>(QStringLiteral("ad-color-picker-trigger-frame"));
  auto* triggerSwatch =
      picker.findChild<QWidget*>(QStringLiteral("ad-color-picker-trigger-swatch"));
  auto* triggerText = picker.findChild<QLabel*>(QStringLiteral("ad-color-picker-trigger-text"));
  QVERIFY(triggerFrame);
  QVERIFY(triggerSwatch);
  QVERIFY(triggerText);
  QVERIFY(triggerFrame->isVisible());
  QVERIFY(triggerSwatch->isVisible());
  QVERIFY(triggerText->isVisible());
  QVERIFY2(!triggerText->text().trimmed().isEmpty(),
           "default trigger text should be populated before popup interaction");

  picker.setPopupVisible(true);
  QTRY_VERIFY(picker.popupVisible());

  QWidget* saturationPanel = nullptr;
  QTRY_VERIFY((saturationPanel =
                   findWidgetByObjectName(QStringLiteral("ad-color-picker-saturation-panel"))) !=
               nullptr);
  QVERIFY(saturationPanel->isVisible());

  const QPoint clickPoint(4, saturationPanel->height() - 4);
  const AdColorValue before = picker.value();

  QTest::mousePress(saturationPanel, Qt::LeftButton, Qt::NoModifier, clickPoint);
  QCoreApplication::processEvents();

  QVERIFY(picker.value() != before);
  QVERIFY(picker.updatesEnabled());
  QVERIFY2(triggerFrame->updatesEnabled(),
           "default trigger frame should stay paintable during live popup edits");
  QVERIFY2(triggerSwatch->updatesEnabled(),
           "default trigger swatch should stay paintable during live popup edits");
  QVERIFY2(triggerText->updatesEnabled(),
           "default trigger text should stay paintable during live popup edits");
  QVERIFY(triggerFrame->isVisible());
  QVERIFY(triggerSwatch->isVisible());
  QVERIFY(triggerText->isVisible());
  QVERIFY2(!triggerText->text().trimmed().isEmpty(),
           "default trigger text became blank after the first popup edit");

  QTest::mouseRelease(saturationPanel, Qt::LeftButton, Qt::NoModifier, clickPoint);
  QCoreApplication::processEvents();
}

void ColorPickerTests::gradientPopupHeaderStaysPaintableDuringFirstPopupEdit() {
  using adqt::widgets::AdColorPicker;
  using adqt::widgets::AdColorValue;

  QWidget host;
  auto* layout = new QVBoxLayout(&host);
  layout->setContentsMargins(0, 0, 0, 0);

  AdColorPicker picker;
  picker.setModeOptions({AdColorPicker::Mode::Solid, AdColorPicker::Mode::Gradient});
  picker.setMode(AdColorPicker::Mode::Gradient);
  picker.setAllowClear(true);
  picker.setTriggerTextVisible(true);
  picker.setValue(AdColorValue::gradient(
      QGradientStops{{0.0, QColor(QStringLiteral("#108ee9"))},
                     {1.0, QColor(QStringLiteral("#87d068"))}},
      QColor(QStringLiteral("#108ee9"))));
  layout->addWidget(&picker);

  host.show();
  QVERIFY(QTest::qWaitForWindowExposed(&host));

  picker.setPopupVisible(true);
  QTRY_VERIFY(picker.popupVisible());

  QWidget* operationRow = nullptr;
  QWidget* modeSegmented = nullptr;
  QWidget* saturationPanel = nullptr;
  QAbstractButton* clearButton = nullptr;
  QTRY_VERIFY((operationRow =
                   findWidgetByObjectName(QStringLiteral("ad-color-picker-operation-row"))) !=
               nullptr);
  QTRY_VERIFY((modeSegmented =
                   findWidgetByObjectName(QStringLiteral("ad-color-picker-mode-segmented"))) !=
               nullptr);
  QTRY_VERIFY((clearButton = qobject_cast<QAbstractButton*>(
                   findWidgetByObjectName(QStringLiteral("ad-color-picker-clear")))) != nullptr);
  QTRY_VERIFY((saturationPanel =
                   findWidgetByObjectName(QStringLiteral("ad-color-picker-saturation-panel"))) !=
               nullptr);

  QVERIFY(operationRow->isVisible());
  QVERIFY(modeSegmented->isVisible());
  QVERIFY(clearButton->isVisible());
  QVERIFY(saturationPanel->isVisible());

  const QPoint clickPoint(4, saturationPanel->height() - 4);
  const AdColorValue before = picker.value();

  QTest::mousePress(saturationPanel, Qt::LeftButton, Qt::NoModifier, clickPoint);
  QCoreApplication::processEvents();

  QVERIFY(picker.value() != before);
  QVERIFY(picker.updatesEnabled());
  QVERIFY2(operationRow->updatesEnabled(),
           "gradient popup header row should stay paintable during the first live edit");
  QVERIFY2(modeSegmented->updatesEnabled(),
           "gradient mode switch should stay paintable during the first live edit");
  QVERIFY(operationRow->isVisible());
  QVERIFY(modeSegmented->isVisible());
  QVERIFY(clearButton->isVisible());

  QTest::mouseRelease(saturationPanel, Qt::LeftButton, Qt::NoModifier, clickPoint);
  QCoreApplication::processEvents();
}

void ColorPickerTests::hexInputReservesPrefixSpaceBeforeHover() {
  using adqt::widgets::AdColorPickerPanel;

  AdColorPickerPanel panel;
  panel.setCssText(QStringLiteral("#1677ff"));
  panel.show();
  QVERIFY(QTest::qWaitForWindowExposed(&panel));

  auto* hexInput = panel.findChild<QLineEdit*>(QStringLiteral("ad-color-picker-hex-input"));
  QVERIFY(hexInput);
  QVERIFY(hexInput->isVisible());

  QLabel* prefixLabel = nullptr;
  const auto labels = hexInput->findChildren<QLabel*>();
  for (QLabel* label : labels) {
    if (label && label->isVisible() && label->text() == QStringLiteral("#")) {
      prefixLabel = label;
      break;
    }
  }
  QVERIFY(prefixLabel);

  QCoreApplication::processEvents();
  const int initialLeftMargin = hexInput->textMargins().left();
  const int prefixRightEdge = prefixLabel->geometry().right();
  QVERIFY2(initialLeftMargin > prefixRightEdge,
           qPrintable(QStringLiteral("initial left margin %1 should clear prefix right edge %2")
                          .arg(initialLeftMargin)
                          .arg(prefixRightEdge)));

  QTest::mouseMove(hexInput, hexInput->rect().center());
  QCoreApplication::processEvents();
  QCOMPARE(hexInput->textMargins().left(), initialLeftMargin);
}

}  // namespace

QObject* createColorPickerTests() {
  return new ColorPickerTests();
}

#include "color_picker_tests.moc"
