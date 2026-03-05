#include "slider_docs_page.h"

#include <QCheckBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLinearGradient>
#include <QPalette>
#include <QVariant>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

#include "icons.h"

using adqt::widgets::AdSlider;
using adqt::widgets::AdInputNumber;
namespace outlined_icons = adqt::icons::outlined;

namespace {

QLabel* makeHintLabel(const QString& text, QWidget* parent = nullptr) {
  auto* label = new QLabel(text, parent);
  label->setWordWrap(true);
  QPalette palette = label->palette();
  palette.setColor(QPalette::WindowText, QColor("#8c8c8c"));
  label->setPalette(palette);
  return label;
}

QLabel* makeIconLabel(const adqt::icons::IconToken& token, const QColor& color, QWidget* parent = nullptr) {
  auto* label = new QLabel(parent);
  adqt::icons::IconToken tinted = token;
  tinted.style.primary = color;
  tinted.style.hasPrimary = true;
  label->setPixmap(adqt::icons::renderIconPixmap(tinted, QSize(18, 18), 1.0));
  label->setFixedSize(20, 20);
  return label;
}

QString formatNumber(double value) {
  if (!std::isfinite(value)) {
    return QStringLiteral("0");
  }

  QString text = QString::number(value, 'f', 4);
  while (text.contains(QLatin1Char('.')) &&
         (text.endsWith(QLatin1Char('0')) || text.endsWith(QLatin1Char('.')))) {
    text.chop(1);
    if (text.endsWith(QLatin1Char('.'))) {
      text.chop(1);
      break;
    }
  }

  if (text.isEmpty() || text == QStringLiteral("-0")) {
    return QStringLiteral("0");
  }
  return text;
}

}  // namespace

SliderDocsPage::SliderDocsPage(QWidget* parent) : QWidget(parent) {
  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(16, 16, 16, 24);
  root->setSpacing(16);

  auto* title = new QLabel("Slider");
  QFont titleFont = title->font();
  titleFont.setPointSize(titleFont.pointSize() + 8);
  titleFont.setBold(true);
  title->setFont(titleFont);
  root->addWidget(title);

  auto* subtitle =
      new QLabel("A Slider component for displaying current value and intervals in range.");
  subtitle->setWordWrap(true);
  root->addWidget(subtitle);

  addSection(root, "Basic", "Demo: basic.tsx", buildBasicDemo());
  addSection(root, "Slider with InputNumber", "Demo: input-number.tsx", buildInputNumberDemo());
  addSection(root, "Slider with icon", "Demo: icon-slider.tsx", buildIconSliderDemo());
  addSection(root, "Customize tooltip", "Demo: tip-formatter.tsx", buildTipFormatterDemo());
  addSection(root, "Event", "Demo: event.tsx", buildEventDemo());
  addSection(root, "Graduated slider", "Demo: mark.tsx", buildMarkDemo());
  addSection(root, "Vertical", "Demo: vertical.tsx", buildVerticalDemo());
  addSection(root, "Control visibility of Tooltip", "Demo: show-tooltip.tsx", buildShowTooltipDemo());
  addSection(root, "Reverse", "Demo: reverse.tsx", buildReverseDemo());
  addSection(root, "Draggable track", "Demo: draggableTrack.tsx", buildDraggableTrackDemo());
  addSection(root, "Multiple handles", "Demo: multiple.tsx", buildMultipleDemo());
  addSection(root, "Dynamic edit nodes", "Demo: editable.tsx", buildEditableDemo());
  addSection(root, "Custom semantic dom styling", "Demo: style-class.tsx", buildStyleClassDemo());
  addSection(root, "Component Token", "Demo: component-token.tsx", buildComponentTokenDemo());

  root->addStretch();
}

const QVector<QWidget*>& SliderDocsPage::sectionAnchors() const { return anchors_; }

const QStringList& SliderDocsPage::sectionTitles() const { return titles_; }

void SliderDocsPage::addSection(QVBoxLayout* root,
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

SliderDocsPage::MarkMap SliderDocsPage::temperatureMarks() const {
  MarkMap marks;
  marks.insert(0, Mark{QStringLiteral("0\u00B0C"), std::nullopt, std::nullopt});
  marks.insert(26, Mark{QStringLiteral("26\u00B0C"), std::nullopt, std::nullopt});
  marks.insert(37, Mark{QStringLiteral("37\u00B0C"), std::nullopt, std::nullopt});
  marks.insert(100, Mark{QStringLiteral("100\u00B0C"), QColor("#f50"), std::nullopt});
  return marks;
}

QWidget* SliderDocsPage::buildBasicDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(10);

  auto* single = new AdSlider();
  single->setValue(30);

  auto* range = new AdSlider();
  range->setMode(AdSlider::Mode::Range);
  range->setValues({20, 50});

  auto* disabled = new QCheckBox("Disabled");
  connect(disabled, &QCheckBox::toggled, single, &AdSlider::setDisabled);
  connect(disabled, &QCheckBox::toggled, range, &AdSlider::setDisabled);

  layout->addWidget(single);
  layout->addWidget(range);
  layout->addWidget(disabled, 0, Qt::AlignLeft);
  return box;
}

QWidget* SliderDocsPage::buildInputNumberDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(12);

  auto* row1 = new QHBoxLayout();
  auto* integerSlider = new AdSlider();
  integerSlider->setMinimum(1);
  integerSlider->setMaximum(20);
  integerSlider->setStep(1);
  integerSlider->setValue(1);
  integerSlider->setFixedWidth(300);
  auto* integerInput = new AdInputNumber();
  integerInput->setMin(1);
  integerInput->setMax(20);
  integerInput->setStep(1);
  integerInput->setValue(1);
  integerInput->setFixedWidth(120);

  connect(integerSlider, &AdSlider::valueChanged, integerInput, [integerInput](double value) {
    integerInput->setValue(qRound(value));
  });
  connect(integerInput, &AdInputNumber::valueChanged, integerSlider, [integerSlider](const QVariant& value) {
    bool ok = false;
    const double parsed = value.toDouble(&ok);
    if (ok) {
      integerSlider->setValue(parsed);
    }
  });

  row1->addWidget(integerSlider);
  row1->addWidget(integerInput);
  row1->addStretch();

  auto* row2 = new QHBoxLayout();
  auto* decimalSlider = new AdSlider();
  decimalSlider->setMinimum(0.0);
  decimalSlider->setMaximum(1.0);
  decimalSlider->setStep(0.01);
  decimalSlider->setValue(0.0);
  decimalSlider->setFixedWidth(300);
  auto* decimalInput = new AdInputNumber();
  decimalInput->setMin(0.0);
  decimalInput->setMax(1.0);
  decimalInput->setStep(0.01);
  decimalInput->setPrecision(2);
  decimalInput->setValue(0.0);
  decimalInput->setFixedWidth(120);

  connect(decimalSlider, &AdSlider::valueChanged, decimalInput, [decimalInput](double value) {
    decimalInput->setValue(value);
  });
  connect(decimalInput, &AdInputNumber::valueChanged, decimalSlider, [decimalSlider](const QVariant& value) {
    bool ok = false;
    const double parsed = value.toDouble(&ok);
    if (ok) {
      decimalSlider->setValue(parsed);
    }
  });

  row2->addWidget(decimalSlider);
  row2->addWidget(decimalInput);
  row2->addStretch();

  layout->addLayout(row1);
  layout->addLayout(row2);
  return box;
}

QWidget* SliderDocsPage::buildIconSliderDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(8);

  auto* slider = new AdSlider();
  slider->setMinimum(0);
  slider->setMaximum(20);
  slider->setValue(0);
  slider->setFixedWidth(280);

  auto* left = makeIconLabel(outlined_icons::Frown(), QColor(0, 0, 0, 96));
  auto* right = makeIconLabel(outlined_icons::Smile(), QColor(0, 0, 0, 96));

  const auto applyState = [slider, left, right]() {
    const double mid = (slider->maximum() - slider->minimum()) / 2.0;
    const bool rightActive = slider->value() >= mid;
    adqt::icons::IconStyle leftStyle;
    leftStyle.primary = rightActive ? QColor(0, 0, 0, 96) : QColor(0, 0, 0, 150);
    leftStyle.hasPrimary = true;
    adqt::icons::IconStyle rightStyle;
    rightStyle.primary = rightActive ? QColor(0, 0, 0, 150) : QColor(0, 0, 0, 96);
    rightStyle.hasPrimary = true;
    left->setPixmap(adqt::icons::renderIconPixmap(
        outlined_icons::Frown(leftStyle),
        QSize(18, 18), 1.0));
    right->setPixmap(adqt::icons::renderIconPixmap(
        outlined_icons::Smile(rightStyle),
        QSize(18, 18), 1.0));
  };

  connect(slider, &AdSlider::valueChanged, slider, [applyState](double) { applyState(); });
  applyState();

  row->addWidget(left);
  row->addWidget(slider);
  row->addWidget(right);
  row->addStretch();
  return box;
}

QWidget* SliderDocsPage::buildTipFormatterDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* first = new AdSlider();
  first->setValue(30);
  first->setTooltipFormatter([](double value) { return QStringLiteral("%1%").arg(formatNumber(value)); });

  auto* second = new AdSlider();
  second->setValue(30);
  second->setTooltipEnabled(false);

  layout->addWidget(first);
  layout->addWidget(second);
  return box;
}

QWidget* SliderDocsPage::buildEventDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* single = new AdSlider();
  single->setValue(30);
  auto* singleHint = makeHintLabel("single onChange: 30, onChangeComplete: 30");
  connect(single, &AdSlider::valueChanged, singleHint, [singleHint](double value) {
    singleHint->setText(QStringLiteral("single onChange: %1").arg(formatNumber(value)));
  });
  connect(single, &AdSlider::valueChangeCompleted, singleHint, [singleHint](double value) {
    singleHint->setText(singleHint->text() +
                        QStringLiteral(", onChangeComplete: %1").arg(formatNumber(value)));
  });

  auto* range = new AdSlider();
  range->setMode(AdSlider::Mode::Range);
  range->setStep(10);
  range->setValues({20, 50});
  auto* rangeHint = makeHintLabel("range onChange: [20, 50], onChangeComplete: [20, 50]");
  connect(range, &AdSlider::valuesChanged, rangeHint, [rangeHint](const QList<double>& values) {
    QStringList parts;
    for (double value : values) {
      parts.append(formatNumber(value));
    }
    rangeHint->setText(QStringLiteral("range onChange: [%1]").arg(parts.join(", ")));
  });
  connect(range, &AdSlider::valuesChangeCompleted, rangeHint, [rangeHint](const QList<double>& values) {
    QStringList parts;
    for (double value : values) {
      parts.append(formatNumber(value));
    }
    rangeHint->setText(rangeHint->text() +
                       QStringLiteral(", onChangeComplete: [%1]").arg(parts.join(", ")));
  });

  layout->addWidget(single);
  layout->addWidget(singleHint);
  layout->addWidget(range);
  layout->addWidget(rangeHint);
  return box;
}

QWidget* SliderDocsPage::buildMarkDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(10);

  auto* includedTitle = new QLabel("included=true");
  auto* includedSingle = new AdSlider();
  includedSingle->setMarks(temperatureMarks());
  includedSingle->setValue(37);
  auto* includedRange = new AdSlider();
  includedRange->setMode(AdSlider::Mode::Range);
  includedRange->setMarks(temperatureMarks());
  includedRange->setValues({26, 37});

  auto* excludedTitle = new QLabel("included=false");
  auto* excludedSingle = new AdSlider();
  excludedSingle->setMarks(temperatureMarks());
  excludedSingle->setIncluded(false);
  excludedSingle->setValue(37);

  auto* stepTitle = new QLabel("marks & step");
  auto* stepSlider = new AdSlider();
  stepSlider->setMarks(temperatureMarks());
  stepSlider->setStep(10);
  stepSlider->setValue(37);

  auto* marksOnlyTitle = new QLabel("step=null (marksOnly)");
  auto* marksOnlySlider = new AdSlider();
  marksOnlySlider->setMarks(temperatureMarks());
  marksOnlySlider->setMarksOnly(true);
  marksOnlySlider->setValue(37);

  layout->addWidget(includedTitle);
  layout->addWidget(includedSingle);
  layout->addWidget(includedRange);
  layout->addWidget(excludedTitle);
  layout->addWidget(excludedSingle);
  layout->addWidget(stepTitle);
  layout->addWidget(stepSlider);
  layout->addWidget(marksOnlyTitle);
  layout->addWidget(marksOnlySlider);
  return box;
}

QWidget* SliderDocsPage::buildVerticalDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(22);
  row->setAlignment(Qt::AlignLeft | Qt::AlignTop);

  constexpr int kVerticalSliderWidth = 90;
  constexpr int kVerticalSliderHeight = 300;

  auto* sliderA = new AdSlider();
  sliderA->setOrientation(Qt::Vertical);
  sliderA->setValue(30);
  sliderA->setFixedSize(kVerticalSliderWidth, kVerticalSliderHeight);

  auto* sliderB = new AdSlider();
  sliderB->setMode(AdSlider::Mode::Range);
  sliderB->setOrientation(Qt::Vertical);
  sliderB->setStep(10);
  sliderB->setValues({20, 50});
  sliderB->setFixedSize(kVerticalSliderWidth, kVerticalSliderHeight);

  auto* sliderC = new AdSlider();
  sliderC->setMode(AdSlider::Mode::Range);
  sliderC->setOrientation(Qt::Vertical);
  sliderC->setMarks(temperatureMarks());
  sliderC->setValues({26, 37});
  sliderC->setFixedSize(kVerticalSliderWidth, kVerticalSliderHeight);

  row->addWidget(sliderA);
  row->addWidget(sliderB);
  row->addWidget(sliderC);
  row->addStretch();
  return box;
}

QWidget* SliderDocsPage::buildShowTooltipDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);

  auto* slider = new AdSlider();
  slider->setValue(30);
  slider->setTooltipVisibleMode(AdSlider::TooltipVisibleMode::Always);
  layout->addWidget(slider);
  return box;
}

QWidget* SliderDocsPage::buildReverseDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(10);

  auto* single = new AdSlider();
  single->setValue(30);
  auto* range = new AdSlider();
  range->setMode(AdSlider::Mode::Range);
  range->setValues({20, 50});
  auto* reverseCheck = new QCheckBox("Reversed");
  reverseCheck->setChecked(true);
  single->setReverse(true);
  range->setReverse(true);

  connect(reverseCheck, &QCheckBox::toggled, single, &AdSlider::setReverse);
  connect(reverseCheck, &QCheckBox::toggled, range, &AdSlider::setReverse);

  layout->addWidget(single);
  layout->addWidget(range);
  layout->addWidget(reverseCheck, 0, Qt::AlignLeft);
  return box;
}

QWidget* SliderDocsPage::buildDraggableTrackDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);

  auto* slider = new AdSlider();
  slider->setMode(AdSlider::Mode::Range);
  slider->setValues({20, 50});
  slider->setDraggableTrack(true);

  layout->addWidget(slider);
  layout->addWidget(makeHintLabel("Drag the selected range track to move the whole segment."));
  return box;
}

QWidget* SliderDocsPage::buildMultipleDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);

  auto* slider = new AdSlider();
  slider->setMode(AdSlider::Mode::Range);
  slider->setValues({0, 10, 20});
  slider->setTooltipVisibleMode(AdSlider::TooltipVisibleMode::Always);
  slider->setSemanticStyleResolver([](const AdSlider::StyleContext& ctx) {
    AdSlider::SemanticStyles styles;
    if (ctx.values.isEmpty()) {
      return styles;
    }
    const double low = ctx.values.constFirst() / 100.0;
    const double high = ctx.values.constLast() / 100.0;

    QColor start(135, 208, 104);
    QColor end(255, 204, 199);
    auto colorAt = [&](double p) {
      const int r = qRound(start.red() + (end.red() - start.red()) * p);
      const int g = qRound(start.green() + (end.green() - start.green()) * p);
      const int b = qRound(start.blue() + (end.blue() - start.blue()) * p);
      return QColor(r, g, b);
    };

    QLinearGradient gradient(0, 0, 1, 0);
    gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    gradient.setColorAt(0, colorAt(low));
    gradient.setColorAt(1, colorAt(high));
    styles.track.backgroundColor = QColor(0, 0, 0, 0);
    styles.tracks.brush = QBrush(gradient);
    return styles;
  });

  layout->addWidget(slider);
  return box;
}

QWidget* SliderDocsPage::buildEditableDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* slider = new AdSlider();
  slider->setMode(AdSlider::Mode::Range);
  slider->setValues({20, 80});
  slider->setEditableHandles(true);
  slider->setMinHandleCount(1);
  slider->setMaxHandleCount(5);

  layout->addWidget(slider);
  layout->addWidget(makeHintLabel("Click rail to add a node. Focus a handle then press Delete/Backspace to remove."));
  return box;
}

QWidget* SliderDocsPage::buildStyleClassDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(12);

  const auto makeVerticalGradientBrush = [](const QColor& top, const QColor& bottom) {
    QLinearGradient gradient(0.0, 0.0, 0.0, 1.0);
    gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    gradient.setColorAt(0.0, top);
    gradient.setColorAt(1.0, bottom);
    return QBrush(gradient);
  };

  auto* objectStyled = new AdSlider();
  objectStyled->setValue(30);
  objectStyled->setFixedWidth(300);
  AdSlider::SemanticStyles fixedStyles;
  fixedStyles.tracks.brush = makeVerticalGradientBrush(QColor("#91caff"), QColor("#1677ff"));
  fixedStyles.handle.borderColor = QColor("#1677ff");
  objectStyled->setSemanticStyles(fixedStyles);

  auto* resolverStyled = new AdSlider();
  resolverStyled->setOrientation(Qt::Vertical);
  resolverStyled->setReverse(true);
  resolverStyled->setValue(30);
  resolverStyled->setFixedSize(100, 300);
  resolverStyled->setSemanticStyleResolver([](const AdSlider::StyleContext& ctx) {
    AdSlider::SemanticStyles styles;
    if (ctx.orientation == Qt::Vertical) {
      QLinearGradient gradient(0.0, 0.0, 0.0, 1.0);
      gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
      gradient.setColorAt(0.0, QColor("#722cc0"));
      gradient.setColorAt(1.0, QColor("#722ed1"));
      styles.tracks.brush = QBrush(gradient);
      styles.handle.borderColor = QColor("#722ed1");
    }
    return styles;
  });

  auto* row = new QHBoxLayout();
  row->addWidget(objectStyled, 1);
  row->addWidget(resolverStyled, 0, Qt::AlignLeft);
  row->addStretch();

  layout->addLayout(row);
  return box;
}

QWidget* SliderDocsPage::buildComponentTokenDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(12);

  AdSlider::ComponentTokens tokens;
  tokens.controlSize = 20;
  tokens.railSize = 4;
  tokens.handleSize = 22;
  tokens.handleSizeHover = 18;
  tokens.dotSize = 8;
  tokens.handleLineWidth = 6;
  tokens.handleLineWidthHover = 2;
  tokens.railBg = QStringLiteral("#9f3434");
  tokens.railHoverBg = QStringLiteral("#8d2424");
  tokens.trackBg = QStringLiteral("#b0b0ef");
  tokens.trackHoverBg = QStringLiteral("#c77195");
  tokens.handleColor = QStringLiteral("#e6f6a2");
  tokens.handleActiveColor = QStringLiteral("#d22bc4");
  tokens.dotBorderColor = QStringLiteral("#303030");
  tokens.dotActiveBorderColor = QStringLiteral("#918542");
  tokens.trackBgDisabled = QStringLiteral("#1a1b80");

  auto* disabled = new AdSlider();
  disabled->setComponentTokens(tokens);
  disabled->setValue(30);
  disabled->setDisabled(true);

  auto* draggable = new AdSlider();
  draggable->setComponentTokens(tokens);
  draggable->setMode(AdSlider::Mode::Range);
  draggable->setDraggableTrack(true);
  draggable->setValues({20, 50});

  auto* verticalRow = new QHBoxLayout();
  auto* vSingle = new AdSlider();
  vSingle->setComponentTokens(tokens);
  vSingle->setOrientation(Qt::Vertical);
  vSingle->setValue(30);
  vSingle->setFixedSize(90, 300);

  auto* vRange = new AdSlider();
  vRange->setComponentTokens(tokens);
  vRange->setMode(AdSlider::Mode::Range);
  vRange->setOrientation(Qt::Vertical);
  vRange->setStep(10);
  vRange->setValues({20, 50});
  vRange->setFixedSize(90, 300);

  auto* vMarks = new AdSlider();
  vMarks->setComponentTokens(tokens);
  vMarks->setMode(AdSlider::Mode::Range);
  vMarks->setOrientation(Qt::Vertical);
  vMarks->setMarks(temperatureMarks());
  vMarks->setValues({26, 37});
  vMarks->setFixedSize(90, 300);

  verticalRow->addWidget(vSingle);
  verticalRow->addWidget(vRange);
  verticalRow->addWidget(vMarks);
  verticalRow->addStretch();

  layout->addWidget(disabled);
  layout->addWidget(draggable);
  layout->addLayout(verticalRow);
  return box;
}

