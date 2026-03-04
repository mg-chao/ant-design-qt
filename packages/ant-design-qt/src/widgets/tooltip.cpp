#include "tooltip.h"

#include "theme/fast_color_lite.h"
#include "theme/theme.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPalette>
#include <QVBoxLayout>

#include <algorithm>

namespace adqt::widgets {

namespace {

QColor toColor(const QString& value, const QColor& fallback) {
  const adqt::theme::FastColorLite parsed(value);
  if (!parsed.isValid()) {
    return fallback;
  }

  QColor color;
  color.setRed(parsed.red());
  color.setGreen(parsed.green());
  color.setBlue(parsed.blue());
  color.setAlphaF(parsed.alpha());
  return color;
}

}  // namespace

AdTooltip::AdTooltip(QWidget* parent) : QWidget(parent) {
  auto* rootLayout = new QHBoxLayout(this);
  rootLayout->setContentsMargins(0, 0, 0, 0);
  rootLayout->setSpacing(0);

  popover_ = new AdPopover(this);
  popover_->setTitleText(QString());
  popover_->setTitleWidget(nullptr);
  rootLayout->addWidget(popover_);

  connect(popover_, &AdPopover::placementChanged, this, [this](AdPopover::Placement value) {
    emit placementChanged(fromPopoverPlacement(value));
    refreshVisualStyle();
  });
  connect(popover_, &AdPopover::triggerModesChanged, this, [this](AdPopover::Triggers value) {
    emit triggerModesChanged(fromPopoverTriggers(value));
    refreshVisualStyle();
  });
  connect(popover_, &AdPopover::openChanged, this, [this](bool value) {
    emit openChanged(value);
    refreshVisualStyle();
  });
  connect(popover_, &AdPopover::onOpenChange, this, &AdTooltip::onOpenChange);
  connect(popover_, &AdPopover::openControlledChanged, this, &AdTooltip::openControlledChanged);
  connect(popover_, &AdPopover::defaultOpenChanged, this, &AdTooltip::defaultOpenChanged);
  connect(popover_, &AdPopover::autoAdjustOverflowChanged, this, [this](bool value) {
    emit autoAdjustOverflowChanged(value);
    refreshVisualStyle();
  });
  connect(popover_, &AdPopover::arrowVisibleChanged, this, [this](bool value) {
    emit arrowVisibleChanged(value);
    refreshVisualStyle();
  });
  connect(popover_, &AdPopover::arrowPointAtCenterChanged, this, &AdTooltip::arrowPointAtCenterChanged);
  connect(popover_, &AdPopover::destroyOnHiddenChanged, this, &AdTooltip::destroyOnHiddenChanged);
  connect(popover_, &AdPopover::disabledChanged, this, [this](bool value) {
    emit disabledChanged(value);
    refreshVisualStyle();
  });
  connect(popover_, &AdPopover::mouseEnterDelayMsChanged, this, &AdTooltip::mouseEnterDelayMsChanged);
  connect(popover_, &AdPopover::mouseLeaveDelayMsChanged, this, &AdTooltip::mouseLeaveDelayMsChanged);
  connect(popover_, &AdPopover::triggerWidgetChanged, this, [this](QWidget* value) {
    triggerWidget_ = value;
    emit triggerWidgetChanged(value);
  });

  connect(&adqt::theme::ThemeManager::instance(), &adqt::theme::ThemeManager::themeChanged, this,
          [this]() { refreshVisualStyle(); });

  syncContentWidget();
  refreshVisualStyle();
}

AdTooltip::~AdTooltip() = default;

AdTooltip::Placement AdTooltip::placement() const {
  if (!popover_) {
    return Placement::Top;
  }
  return fromPopoverPlacement(popover_->placement());
}

void AdTooltip::setPlacement(Placement value) {
  if (!popover_) {
    return;
  }
  popover_->setPlacement(toPopoverPlacement(value));
}

AdTooltip::Triggers AdTooltip::triggerModes() const {
  if (!popover_) {
    return Trigger::Hover;
  }
  return fromPopoverTriggers(popover_->triggerModes());
}

void AdTooltip::setTriggerModes(Triggers value) {
  if (!popover_) {
    return;
  }
  popover_->setTriggerModes(toPopoverTriggers(value));
}

bool AdTooltip::open() const { return popover_ && popover_->open(); }

void AdTooltip::setOpen(bool value) {
  if (!popover_) {
    return;
  }
  popover_->setOpen(value);
}

bool AdTooltip::openControlled() const { return popover_ && popover_->openControlled(); }

void AdTooltip::setOpenControlled(bool value) {
  if (!popover_) {
    return;
  }
  popover_->setOpenControlled(value);
}

bool AdTooltip::defaultOpen() const { return popover_ && popover_->defaultOpen(); }

void AdTooltip::setDefaultOpen(bool value) {
  if (!popover_) {
    return;
  }
  popover_->setDefaultOpen(value);
}

bool AdTooltip::autoAdjustOverflow() const { return popover_ && popover_->autoAdjustOverflow(); }

void AdTooltip::setAutoAdjustOverflow(bool value) {
  if (!popover_) {
    return;
  }
  popover_->setAutoAdjustOverflow(value);
}

bool AdTooltip::arrowVisible() const { return popover_ && popover_->arrowVisible(); }

void AdTooltip::setArrowVisible(bool value) {
  if (!popover_) {
    return;
  }
  popover_->setArrowVisible(value);
}

bool AdTooltip::arrowPointAtCenter() const { return popover_ && popover_->arrowPointAtCenter(); }

void AdTooltip::setArrowPointAtCenter(bool value) {
  if (!popover_) {
    return;
  }
  popover_->setArrowPointAtCenter(value);
}

bool AdTooltip::destroyOnHidden() const { return popover_ && popover_->destroyOnHidden(); }

void AdTooltip::setDestroyOnHidden(bool value) {
  if (!popover_) {
    return;
  }
  popover_->setDestroyOnHidden(value);
}

bool AdTooltip::disabled() const { return popover_ && popover_->disabled(); }

void AdTooltip::setDisabled(bool value) {
  if (!popover_) {
    return;
  }
  popover_->setDisabled(value);
}

int AdTooltip::mouseEnterDelayMs() const {
  if (!popover_) {
    return 100;
  }
  return popover_->mouseEnterDelayMs();
}

void AdTooltip::setMouseEnterDelayMs(int value) {
  if (!popover_) {
    return;
  }
  popover_->setMouseEnterDelayMs(value);
}

int AdTooltip::mouseLeaveDelayMs() const {
  if (!popover_) {
    return 100;
  }
  return popover_->mouseLeaveDelayMs();
}

void AdTooltip::setMouseLeaveDelayMs(int value) {
  if (!popover_) {
    return;
  }
  popover_->setMouseLeaveDelayMs(value);
}

QString AdTooltip::titleText() const { return titleText_; }

void AdTooltip::setTitleText(const QString& value) {
  if (titleText_ == value) {
    return;
  }
  titleText_ = value;
  emit titleTextChanged(titleText_);
  syncContentWidget();
}

QString AdTooltip::color() const { return color_; }

void AdTooltip::setColor(const QString& value) {
  if (color_ == value) {
    return;
  }
  color_ = value;
  emit colorChanged(color_);
  refreshVisualStyle();
}

QWidget* AdTooltip::triggerWidget() const { return triggerWidget_; }

void AdTooltip::setTriggerWidget(QWidget* widget) {
  if (triggerWidget_ == widget) {
    return;
  }
  triggerWidget_ = widget;
  if (popover_) {
    popover_->setTriggerWidget(widget);
  }
}

QWidget* AdTooltip::titleWidget() const { return titleWidget_; }

void AdTooltip::setTitleWidget(QWidget* widget) {
  if (titleWidget_ == widget) {
    return;
  }

  QWidget* previous = titleWidget_;
  titleWidget_ = widget;
  if (previous && previous != titleWidget_ && contentHost_ && previous->parentWidget() == contentHost_) {
    previous->hide();
    previous->setParent(nullptr);
  }

  emit titleWidgetChanged(titleWidget_);
  syncContentWidget();
}

AdTooltip::ComponentTokens AdTooltip::componentTokens() const { return componentTokens_; }

void AdTooltip::setComponentTokens(const ComponentTokens& tokens) {
  componentTokens_ = tokens;
  emit componentTokensChanged();
  refreshVisualStyle();
}

void AdTooltip::resetComponentTokens() {
  componentTokens_ = ComponentTokens{};
  emit componentTokensChanged();
  refreshVisualStyle();
}

AdTooltip::SemanticStyles AdTooltip::semanticStyles() const { return semanticStyles_; }

void AdTooltip::setSemanticStyles(const SemanticStyles& styles) {
  semanticStyles_ = styles;
  emit semanticStylesChanged();
  refreshVisualStyle();
}

void AdTooltip::setSemanticStyleResolver(SemanticStyleResolver resolver) {
  semanticStyleResolver_ = std::move(resolver);
  emit semanticStylesChanged();
  refreshVisualStyle();
}

void AdTooltip::changeEvent(QEvent* event) {
  QWidget::changeEvent(event);
  if (!event || !popover_) {
    return;
  }

  if (event->type() == QEvent::EnabledChange) {
    popover_->setDisabled(!isEnabled());
  }
  if (event->type() == QEvent::EnabledChange || event->type() == QEvent::FontChange) {
    refreshVisualStyle();
  }
}

AdPopover::Placement AdTooltip::toPopoverPlacement(Placement value) {
  switch (value) {
    case Placement::Top:
      return AdPopover::Placement::Top;
    case Placement::TopLeft:
      return AdPopover::Placement::TopLeft;
    case Placement::TopRight:
      return AdPopover::Placement::TopRight;
    case Placement::Bottom:
      return AdPopover::Placement::Bottom;
    case Placement::BottomLeft:
      return AdPopover::Placement::BottomLeft;
    case Placement::BottomRight:
      return AdPopover::Placement::BottomRight;
    case Placement::Left:
      return AdPopover::Placement::Left;
    case Placement::LeftTop:
      return AdPopover::Placement::LeftTop;
    case Placement::LeftBottom:
      return AdPopover::Placement::LeftBottom;
    case Placement::Right:
      return AdPopover::Placement::Right;
    case Placement::RightTop:
      return AdPopover::Placement::RightTop;
    case Placement::RightBottom:
      return AdPopover::Placement::RightBottom;
  }
  return AdPopover::Placement::Top;
}

AdTooltip::Placement AdTooltip::fromPopoverPlacement(AdPopover::Placement value) {
  switch (value) {
    case AdPopover::Placement::Top:
      return Placement::Top;
    case AdPopover::Placement::TopLeft:
      return Placement::TopLeft;
    case AdPopover::Placement::TopRight:
      return Placement::TopRight;
    case AdPopover::Placement::Bottom:
      return Placement::Bottom;
    case AdPopover::Placement::BottomLeft:
      return Placement::BottomLeft;
    case AdPopover::Placement::BottomRight:
      return Placement::BottomRight;
    case AdPopover::Placement::Left:
      return Placement::Left;
    case AdPopover::Placement::LeftTop:
      return Placement::LeftTop;
    case AdPopover::Placement::LeftBottom:
      return Placement::LeftBottom;
    case AdPopover::Placement::Right:
      return Placement::Right;
    case AdPopover::Placement::RightTop:
      return Placement::RightTop;
    case AdPopover::Placement::RightBottom:
      return Placement::RightBottom;
  }
  return Placement::Top;
}

AdPopover::Triggers AdTooltip::toPopoverTriggers(Triggers value) {
  AdPopover::Triggers mapped;
  if (value.testFlag(Trigger::Hover)) {
    mapped |= AdPopover::Trigger::Hover;
  }
  if (value.testFlag(Trigger::Focus)) {
    mapped |= AdPopover::Trigger::Focus;
  }
  if (value.testFlag(Trigger::Click)) {
    mapped |= AdPopover::Trigger::Click;
  }
  if (value.testFlag(Trigger::ContextMenu)) {
    mapped |= AdPopover::Trigger::ContextMenu;
  }
  return mapped;
}

AdTooltip::Triggers AdTooltip::fromPopoverTriggers(AdPopover::Triggers value) {
  Triggers mapped;
  if (value.testFlag(AdPopover::Trigger::Hover)) {
    mapped |= Trigger::Hover;
  }
  if (value.testFlag(AdPopover::Trigger::Focus)) {
    mapped |= Trigger::Focus;
  }
  if (value.testFlag(AdPopover::Trigger::Click)) {
    mapped |= Trigger::Click;
  }
  if (value.testFlag(AdPopover::Trigger::ContextMenu)) {
    mapped |= Trigger::ContextMenu;
  }
  return mapped;
}

AdPopover::SemanticSlotStyle AdTooltip::toPopoverSemanticSlot(const SemanticSlotStyle& slot) {
  AdPopover::SemanticSlotStyle mapped;
  mapped.textColor = slot.textColor;
  mapped.backgroundColor = slot.backgroundColor;
  mapped.borderColor = slot.borderColor;
  return mapped;
}

void AdTooltip::ensureContentHost() {
  if (contentHost_) {
    return;
  }

  auto* host = new QWidget(this);
  auto* layout = new QVBoxLayout(host);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  auto* label = new QLabel(host);
  label->setWordWrap(true);
  label->setAlignment(Qt::AlignLeft | Qt::AlignTop);

  contentHost_ = host;
  contentHostLayout_ = layout;
  contentLabel_ = label;
}

void AdTooltip::clearContentHostLayout() {
  if (!contentHostLayout_) {
    return;
  }
  while (QLayoutItem* item = contentHostLayout_->takeAt(0)) {
    if (QWidget* widget = item->widget()) {
      widget->hide();
    }
    delete item;
  }
}

void AdTooltip::syncContentWidget() {
  if (!popover_) {
    return;
  }

  const bool hasCustomWidget = titleWidget_;
  const bool hasTitleText = !titleText_.trimmed().isEmpty();

  if (!hasCustomWidget && !hasTitleText) {
    popover_->setContentWidget(nullptr);
    if (contentHost_ && !contentHost_->parentWidget()) {
      contentHost_->setParent(this);
      contentHost_->hide();
    }
    return;
  }

  ensureContentHost();
  clearContentHostLayout();

  if (hasCustomWidget) {
    if (titleWidget_->parentWidget() != contentHost_) {
      titleWidget_->setParent(contentHost_);
    }
    contentHostLayout_->addWidget(titleWidget_);
    titleWidget_->show();
  } else {
    contentLabel_->setText(titleText_);
    contentHostLayout_->addWidget(contentLabel_);
    contentLabel_->show();
  }

  popover_->setContentWidget(contentHost_);
  refreshVisualStyle();
}

void AdTooltip::refreshVisualStyle() {
  if (!popover_) {
    return;
  }

  const DerivedVisualStyle style = deriveVisualStyle();
  const int horizontalPadding = std::max(0, style.paddingHorizontal);
  const int verticalPadding = std::max(0, style.paddingVertical);
  const int arrowWidth = std::max(0, style.arrowSize * 2);
  const int centerAlignedMinWidth = std::max(1, style.borderRadius * 2 + arrowWidth);
  const int edgeOffsetHorizontal = (style.borderRadius > 12) ? (style.borderRadius + 2) : 12;
  const int edgeAlignedMinWidth =
      std::max(centerAlignedMinWidth, style.borderRadius + arrowWidth + edgeOffsetHorizontal);
  const Placement currentPlacement = placement();
  const bool edgeAlignedPlacement = currentPlacement == Placement::TopLeft ||
                                    currentPlacement == Placement::TopRight ||
                                    currentPlacement == Placement::BottomLeft ||
                                    currentPlacement == Placement::BottomRight;

  if (contentHost_) {
    // antd applies min/max width and min-height to the tooltip body that already includes
    // text padding. The content host in Qt sits inside a padded container, so subtract
    // those paddings to keep the final rendered bubble size aligned with antd.
    const int horizontalPaddingSpace = horizontalPadding * 2;
    const int verticalPaddingSpace = verticalPadding * 2;
    const int bodyMinWidth = edgeAlignedPlacement ? edgeAlignedMinWidth : centerAlignedMinWidth;
    const int contentMinWidth = std::max(1, bodyMinWidth - horizontalPaddingSpace);
    const int contentMinHeight = std::max(0, style.minHeight - verticalPaddingSpace);
    const int contentMaxWidth =
        (style.maxWidth > 0) ? std::max(1, style.maxWidth - horizontalPaddingSpace) : QWIDGETSIZE_MAX;

    contentHost_->setMaximumWidth(contentMaxWidth);
    contentHost_->setMinimumWidth(contentMinWidth);
    contentHost_->setMinimumHeight(contentMinHeight);
  }
  if (contentLabel_) {
    QPalette palette = contentLabel_->palette();
    palette.setColor(QPalette::WindowText, style.textColor);
    contentLabel_->setPalette(palette);
    contentLabel_->setFont(style.textFont);
  }

  AdPopover::ComponentTokens popoverTokens;
  popoverTokens.titleMinWidth = 0;
  popoverTokens.borderWidth = 0;
  popoverTokens.borderRadius = style.borderRadius;
  popoverTokens.arrowSize = style.arrowSize;
  popoverTokens.popupOffset = style.popupOffset;
  popoverTokens.popupPadding = 0;
  popoverTokens.titlePaddingHorizontal = 0;
  popoverTokens.titlePaddingVertical = 0;
  popoverTokens.titleMarginBottom = 0;
  popoverTokens.contentPaddingHorizontal = horizontalPadding;
  popoverTokens.contentPaddingVertical = verticalPadding;
  popoverTokens.popupBg = colorToTokenString(style.popupBg);
  popoverTokens.titleColor = colorToTokenString(style.textColor);
  popoverTokens.contentColor = colorToTokenString(style.textColor);
  popover_->setComponentTokens(popoverTokens);

  StyleContext context;
  context.placement = placement();
  context.triggerModes = triggerModes();
  context.open = open();
  context.disabled = disabled();
  context.arrowVisible = arrowVisible();
  context.color = color_;

  const SemanticStyles effectiveSemantic =
      semanticStyleResolver_ ? semanticStyleResolver_(context) : semanticStyles_;

  AdPopover::SemanticStyles popoverSemantic;
  popoverSemantic.root = toPopoverSemanticSlot(effectiveSemantic.root);
  popoverSemantic.container = toPopoverSemanticSlot(effectiveSemantic.container);
  popoverSemantic.content = toPopoverSemanticSlot(effectiveSemantic.body);
  popoverSemantic.arrow = toPopoverSemanticSlot(effectiveSemantic.arrow);
  popover_->setSemanticStyles(popoverSemantic);
}

AdTooltip::DerivedVisualStyle AdTooltip::deriveVisualStyle() const {
  DerivedVisualStyle style;
  const adqt::theme::ThemeManager& themeManager = adqt::theme::ThemeManager::instance();
  const adqt::theme::ThemeMapToken& map = themeManager.currentMapToken();
  const adqt::theme::ThemeSeedToken& seed = themeManager.currentConfig().seed;

  style.minHeight = std::max(0, qRound(map.controlHeight));
  style.borderRadius = std::max(0, qRound(map.borderRadius));
  style.arrowSize = std::max(0, qRound(seed.sizePopupArrow / 2.0));
  style.popupOffset = std::max(0, qRound(map.sizeXXS));
  style.paddingHorizontal = std::max(0, qRound(map.sizeXS));
  style.paddingVertical = std::max(0, qRound(map.sizeSM / 2.0));
  style.textFont = font();
  style.textFont.setPixelSize(std::max(12, qRound(map.fontSize)));
  style.popupBg = toColor(map.colorBgSpotlight, QColor("#141414"));
  style.textColor = toColor(map.colorWhite, QColor("#ffffff"));

  if (disabled()) {
    style.textColor = toColor(map.colorTextQuaternary, QColor("#bfbfbf"));
  }

  const std::optional<QColor> resolvedColor = resolveColorValue(color_);
  if (resolvedColor.has_value()) {
    style.popupBg = resolvedColor.value();
    style.textColor = textColorForBackground(style.popupBg);
  }

  if (componentTokens_.maxWidth.has_value()) {
    style.maxWidth = std::max(1, componentTokens_.maxWidth.value());
  }
  if (componentTokens_.borderRadius.has_value()) {
    style.borderRadius = std::max(0, componentTokens_.borderRadius.value());
  }
  if (componentTokens_.arrowSize.has_value()) {
    style.arrowSize = std::max(0, componentTokens_.arrowSize.value());
  }
  if (componentTokens_.popupOffset.has_value()) {
    style.popupOffset = std::max(0, componentTokens_.popupOffset.value());
  }
  if (componentTokens_.paddingHorizontal.has_value()) {
    style.paddingHorizontal = std::max(0, componentTokens_.paddingHorizontal.value());
  }
  if (componentTokens_.paddingVertical.has_value()) {
    style.paddingVertical = std::max(0, componentTokens_.paddingVertical.value());
  }
  if (componentTokens_.popupBg.has_value()) {
    style.popupBg = toColor(componentTokens_.popupBg.value(), style.popupBg);
  }
  if (componentTokens_.textColor.has_value()) {
    style.textColor = toColor(componentTokens_.textColor.value(), style.textColor);
  }

  return style;
}

std::optional<QColor> AdTooltip::resolveColorValue(const QString& value) const {
  const QString trimmed = value.trimmed();
  if (trimmed.isEmpty()) {
    return std::nullopt;
  }

  const QString key = trimmed.toLower();
  const auto& seed = adqt::theme::ThemeManager::instance().currentConfig().seed;

  QString resolved = trimmed;
  if (key == QStringLiteral("pink")) {
    resolved = seed.pink;
  } else if (key == QStringLiteral("red")) {
    resolved = seed.red;
  } else if (key == QStringLiteral("yellow")) {
    resolved = seed.yellow;
  } else if (key == QStringLiteral("orange")) {
    resolved = seed.orange;
  } else if (key == QStringLiteral("cyan")) {
    resolved = seed.cyan;
  } else if (key == QStringLiteral("green")) {
    resolved = seed.green;
  } else if (key == QStringLiteral("blue")) {
    resolved = seed.blue;
  } else if (key == QStringLiteral("purple")) {
    resolved = seed.purple;
  } else if (key == QStringLiteral("geekblue")) {
    resolved = seed.geekblue;
  } else if (key == QStringLiteral("magenta")) {
    resolved = seed.magenta;
  } else if (key == QStringLiteral("volcano")) {
    resolved = seed.volcano;
  } else if (key == QStringLiteral("gold")) {
    resolved = seed.gold;
  } else if (key == QStringLiteral("lime")) {
    resolved = seed.lime;
  }

  const adqt::theme::FastColorLite parsed(resolved);
  if (parsed.isValid()) {
    QColor color;
    color.setRed(parsed.red());
    color.setGreen(parsed.green());
    color.setBlue(parsed.blue());
    color.setAlphaF(parsed.alpha());
    return color;
  }

  QColor fallback(resolved);
  if (fallback.isValid()) {
    return fallback;
  }
  return std::nullopt;
}

QColor AdTooltip::textColorForBackground(const QColor& background) {
  if (!background.isValid()) {
    return QColor("#ffffff");
  }
  const qreal luminance =
      (0.299 * background.redF() + 0.587 * background.greenF() + 0.114 * background.blueF());
  return luminance < 0.5 ? QColor("#ffffff") : QColor("#000000");
}

QString AdTooltip::colorToTokenString(const QColor& color) {
  if (!color.isValid()) {
    return QString();
  }
  if (color.alpha() < 255) {
    // FastColorLite parses 8-digit hex as #RRGGBBAA. Qt's HexArgb emits #AARRGGBB.
    // Use explicit RGBA ordering to preserve tooltip background opacity.
    return QStringLiteral("#%1%2%3%4")
        .arg(color.red(), 2, 16, QChar('0'))
        .arg(color.green(), 2, 16, QChar('0'))
        .arg(color.blue(), 2, 16, QChar('0'))
        .arg(color.alpha(), 2, 16, QChar('0'))
        .toLower();
  }
  return color.name(QColor::HexRgb);
}

}  // namespace adqt::widgets
