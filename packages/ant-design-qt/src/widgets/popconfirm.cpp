#include "popconfirm.h"

#include "theme/fast_color_lite.h"
#include "theme/theme.h"

#include <QEvent>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayoutItem>
#include <QMouseEvent>
#include <QPalette>
#include <QStringList>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

#include "icons.h"

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

void applySemanticSlot(const AdPopconfirm::SemanticSlotStyle& slot,
                       QColor* textColor,
                       QColor* backgroundColor,
                       QColor* borderColor) {
  if (textColor && slot.textColor.has_value()) {
    *textColor = slot.textColor.value();
  }
  if (backgroundColor && slot.backgroundColor.has_value()) {
    *backgroundColor = slot.backgroundColor.value();
  }
  if (borderColor && slot.borderColor.has_value()) {
    *borderColor = slot.borderColor.value();
  }
}

template <typename Callback>
void traverseObjectTree(QObject* root, Callback&& callback) {
  if (!root) {
    return;
  }
  callback(root);
  const QObjectList children = root->children();
  for (QObject* child : children) {
    traverseObjectTree(child, callback);
  }
}

bool iconStylesEqual(const adqt::icons::IconStyle& lhs, const adqt::icons::IconStyle& rhs) {
  return lhs.hasPrimary == rhs.hasPrimary && lhs.hasSecondary == rhs.hasSecondary &&
         lhs.hasTertiary == rhs.hasTertiary && lhs.primary == rhs.primary &&
         lhs.secondary == rhs.secondary && lhs.tertiary == rhs.tertiary;
}

bool iconTokensEqual(const adqt::icons::IconToken& lhs, const adqt::icons::IconToken& rhs) {
  return lhs.index == rhs.index && iconStylesEqual(lhs.style, rhs.style);
}

int measureSingleLineTextWidth(const QFontMetrics& metrics, const QString& line) {
  if (line.isEmpty()) {
    return 0;
  }
  const int advance = metrics.horizontalAdvance(line);
  if (advance > 0) {
    return advance;
  }
  return metrics.boundingRect(line).width();
}

int measurePlainTextWidth(const QString& text, const QFont& font) {
  if (text.isEmpty()) {
    return 0;
  }

  const QFontMetrics metrics(font);
  const QStringList lines = text.split(QChar::LineFeed);
  int maxWidth = 0;
  for (const QString& rawLine : lines) {
    QString line = rawLine;
    line.remove(QChar::CarriageReturn);
    maxWidth = std::max(maxWidth, measureSingleLineTextWidth(metrics, line));
  }
  return maxWidth;
}

}  // namespace

AdPopconfirm::AdPopconfirm(QWidget* parent) : QWidget(parent) {
  auto* rootLayout = new QHBoxLayout(this);
  rootLayout->setContentsMargins(0, 0, 0, 0);
  rootLayout->setSpacing(0);

  popover_ = new AdPopover(this);
  popover_->setTitleText(QString());
  popover_->setContentText(QString());
  popover_->setTriggerModes(AdPopover::Trigger::Click);
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
  connect(popover_, &AdPopover::onOpenChange, this, &AdPopconfirm::onOpenChange);
  connect(popover_, &AdPopover::openControlledChanged, this, &AdPopconfirm::openControlledChanged);
  connect(popover_, &AdPopover::defaultOpenChanged, this, &AdPopconfirm::defaultOpenChanged);
  connect(popover_, &AdPopover::autoAdjustOverflowChanged, this, [this](bool value) {
    emit autoAdjustOverflowChanged(value);
    refreshVisualStyle();
  });
  connect(popover_, &AdPopover::arrowVisibleChanged, this, [this](bool value) {
    emit arrowVisibleChanged(value);
    refreshVisualStyle();
  });
  connect(popover_, &AdPopover::arrowPointAtCenterChanged, this,
          &AdPopconfirm::arrowPointAtCenterChanged);
  connect(popover_, &AdPopover::destroyOnHiddenChanged, this, &AdPopconfirm::destroyOnHiddenChanged);
  connect(popover_, &AdPopover::disabledChanged, this, [this](bool value) {
    emit disabledChanged(value);
    refreshVisualStyle();
  });
  connect(popover_, &AdPopover::mouseEnterDelayMsChanged, this, &AdPopconfirm::mouseEnterDelayMsChanged);
  connect(popover_, &AdPopover::mouseLeaveDelayMsChanged, this, &AdPopconfirm::mouseLeaveDelayMsChanged);
  connect(popover_, &AdPopover::triggerWidgetChanged, this, [this](QWidget* value) {
    triggerWidget_ = value;
    emit triggerWidgetChanged(value);
  });

  connect(&adqt::theme::ThemeManager::instance(), &adqt::theme::ThemeManager::themeChanged, this,
          [this]() { refreshVisualStyle(); });

  iconToken_ = adqt::icons::filled::ExclamationCircle();
  ensureContentHost();
  syncContentWidget();
  refreshVisualStyle();
}

AdPopconfirm::~AdPopconfirm() { clearOverlayWatchers(); }

AdPopconfirm::Placement AdPopconfirm::placement() const {
  if (!popover_) {
    return Placement::Top;
  }
  return fromPopoverPlacement(popover_->placement());
}

void AdPopconfirm::setPlacement(Placement value) {
  if (!popover_) {
    return;
  }
  popover_->setPlacement(toPopoverPlacement(value));
}

AdPopconfirm::Triggers AdPopconfirm::triggerModes() const {
  if (!popover_) {
    return Trigger::Click;
  }
  return fromPopoverTriggers(popover_->triggerModes());
}

void AdPopconfirm::setTriggerModes(Triggers value) {
  if (!popover_) {
    return;
  }
  popover_->setTriggerModes(toPopoverTriggers(value));
}

bool AdPopconfirm::open() const { return popover_ && popover_->open(); }

void AdPopconfirm::setOpen(bool value) {
  if (!popover_) {
    return;
  }
  popover_->setOpen(value);
}

bool AdPopconfirm::openControlled() const { return popover_ && popover_->openControlled(); }

void AdPopconfirm::setOpenControlled(bool value) {
  if (!popover_) {
    return;
  }
  popover_->setOpenControlled(value);
}

bool AdPopconfirm::defaultOpen() const { return popover_ && popover_->defaultOpen(); }

void AdPopconfirm::setDefaultOpen(bool value) {
  if (!popover_) {
    return;
  }
  popover_->setDefaultOpen(value);
}

bool AdPopconfirm::autoAdjustOverflow() const {
  return popover_ && popover_->autoAdjustOverflow();
}

void AdPopconfirm::setAutoAdjustOverflow(bool value) {
  if (!popover_) {
    return;
  }
  popover_->setAutoAdjustOverflow(value);
}

bool AdPopconfirm::arrowVisible() const { return popover_ && popover_->arrowVisible(); }

void AdPopconfirm::setArrowVisible(bool value) {
  if (!popover_) {
    return;
  }
  popover_->setArrowVisible(value);
}

bool AdPopconfirm::arrowPointAtCenter() const {
  return popover_ && popover_->arrowPointAtCenter();
}

void AdPopconfirm::setArrowPointAtCenter(bool value) {
  if (!popover_) {
    return;
  }
  popover_->setArrowPointAtCenter(value);
}

bool AdPopconfirm::destroyOnHidden() const { return popover_ && popover_->destroyOnHidden(); }

void AdPopconfirm::setDestroyOnHidden(bool value) {
  if (!popover_) {
    return;
  }
  popover_->setDestroyOnHidden(value);
}

bool AdPopconfirm::disabled() const { return popover_ && popover_->disabled(); }

void AdPopconfirm::setDisabled(bool value) {
  if (!popover_) {
    return;
  }
  popover_->setDisabled(value);
}

int AdPopconfirm::mouseEnterDelayMs() const {
  if (!popover_) {
    return 100;
  }
  return popover_->mouseEnterDelayMs();
}

void AdPopconfirm::setMouseEnterDelayMs(int value) {
  if (!popover_) {
    return;
  }
  popover_->setMouseEnterDelayMs(value);
}

int AdPopconfirm::mouseLeaveDelayMs() const {
  if (!popover_) {
    return 100;
  }
  return popover_->mouseLeaveDelayMs();
}

void AdPopconfirm::setMouseLeaveDelayMs(int value) {
  if (!popover_) {
    return;
  }
  popover_->setMouseLeaveDelayMs(value);
}

QString AdPopconfirm::titleText() const { return titleText_; }

void AdPopconfirm::setTitleText(const QString& value) {
  if (titleText_ == value) {
    return;
  }
  titleText_ = value;
  emit titleTextChanged(titleText_);
  syncContentWidget();
}

QString AdPopconfirm::descriptionText() const { return descriptionText_; }

void AdPopconfirm::setDescriptionText(const QString& value) {
  if (descriptionText_ == value) {
    return;
  }
  descriptionText_ = value;
  emit descriptionTextChanged(descriptionText_);
  syncContentWidget();
}

QString AdPopconfirm::okText() const { return okText_; }

void AdPopconfirm::setOkText(const QString& value) {
  if (okText_ == value) {
    return;
  }
  okText_ = value;
  emit okTextChanged(okText_);
  syncContentWidget();
}

QString AdPopconfirm::cancelText() const { return cancelText_; }

void AdPopconfirm::setCancelText(const QString& value) {
  if (cancelText_ == value) {
    return;
  }
  cancelText_ = value;
  emit cancelTextChanged(cancelText_);
  syncContentWidget();
}

bool AdPopconfirm::showCancel() const { return showCancel_; }

void AdPopconfirm::setShowCancel(bool value) {
  if (showCancel_ == value) {
    return;
  }
  showCancel_ = value;
  emit showCancelChanged(showCancel_);
  syncContentWidget();
}

bool AdPopconfirm::confirmAutoClose() const { return confirmAutoClose_; }

void AdPopconfirm::setConfirmAutoClose(bool value) {
  if (confirmAutoClose_ == value) {
    return;
  }
  confirmAutoClose_ = value;
  emit confirmAutoCloseChanged(confirmAutoClose_);
}

bool AdPopconfirm::iconVisible() const { return iconVisible_; }

void AdPopconfirm::setIconVisible(bool value) {
  if (iconVisible_ == value) {
    return;
  }
  iconVisible_ = value;
  emit iconVisibleChanged(iconVisible_);
  syncContentWidget();
}

adqt::icons::IconToken AdPopconfirm::iconToken() const { return iconToken_; }

void AdPopconfirm::setIconToken(const adqt::icons::IconToken& value) {
  if (iconTokensEqual(iconToken_, value)) {
    return;
  }
  iconToken_ = value;
  emit iconTokenChanged(iconToken_);
  syncContentWidget();
}

AdButton::Type AdPopconfirm::okType() const { return okType_; }

void AdPopconfirm::setOkType(AdButton::Type value) {
  if (okType_ == value) {
    return;
  }
  okType_ = value;
  emit okTypeChanged(okType_);
  syncContentWidget();
}

bool AdPopconfirm::okButtonLoading() const { return okButtonLoading_; }

void AdPopconfirm::setOkButtonLoading(bool value) {
  if (okButtonLoading_ == value) {
    return;
  }
  okButtonLoading_ = value;
  emit okButtonLoadingChanged(okButtonLoading_);
  syncContentWidget();
}

QWidget* AdPopconfirm::triggerWidget() const { return triggerWidget_; }

void AdPopconfirm::setTriggerWidget(QWidget* widget) {
  if (triggerWidget_ == widget) {
    return;
  }
  triggerWidget_ = widget;
  if (popover_) {
    popover_->setTriggerWidget(widget);
  }
}

AdButton* AdPopconfirm::okButton() const { return okButton_; }

AdButton* AdPopconfirm::cancelButton() const { return cancelButton_; }

AdPopconfirm::ComponentTokens AdPopconfirm::componentTokens() const { return componentTokens_; }

void AdPopconfirm::setComponentTokens(const ComponentTokens& tokens) {
  componentTokens_ = tokens;
  emit componentTokensChanged();
  refreshVisualStyle();
}

void AdPopconfirm::resetComponentTokens() {
  componentTokens_ = ComponentTokens{};
  emit componentTokensChanged();
  refreshVisualStyle();
}

AdPopconfirm::SemanticStyles AdPopconfirm::semanticStyles() const { return semanticStyles_; }

void AdPopconfirm::setSemanticStyles(const SemanticStyles& styles) {
  semanticStyles_ = styles;
  emit semanticStylesChanged();
  refreshVisualStyle();
}

void AdPopconfirm::setSemanticStyleResolver(SemanticStyleResolver resolver) {
  semanticStyleResolver_ = std::move(resolver);
  emit semanticStylesChanged();
  refreshVisualStyle();
}

bool AdPopconfirm::eventFilter(QObject* watched, QEvent* event) {
  if (watched && event && watchedOverlayObjects_.contains(watched) &&
      event->type() == QEvent::MouseButtonPress) {
    emit popupClicked();
  }
  return QWidget::eventFilter(watched, event);
}

void AdPopconfirm::changeEvent(QEvent* event) {
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

AdPopover::Placement AdPopconfirm::toPopoverPlacement(Placement value) {
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

AdPopconfirm::Placement AdPopconfirm::fromPopoverPlacement(AdPopover::Placement value) {
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

AdPopover::Triggers AdPopconfirm::toPopoverTriggers(Triggers value) {
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

AdPopconfirm::Triggers AdPopconfirm::fromPopoverTriggers(AdPopover::Triggers value) {
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

AdPopover::SemanticSlotStyle AdPopconfirm::toPopoverSemanticSlot(const SemanticSlotStyle& slot) {
  AdPopover::SemanticSlotStyle mapped;
  mapped.textColor = slot.textColor;
  mapped.backgroundColor = slot.backgroundColor;
  mapped.borderColor = slot.borderColor;
  return mapped;
}

void AdPopconfirm::ensureContentHost() {
  if (contentHost_) {
    return;
  }

  auto* host = new QWidget(this);
  host->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  auto* hostLayout = new QVBoxLayout(host);
  hostLayout->setContentsMargins(0, 0, 0, 0);
  hostLayout->setSpacing(0);

  auto* messageHost = new QWidget(host);
  messageHost->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  auto* messageLayout = new QHBoxLayout(messageHost);
  messageLayout->setContentsMargins(0, 0, 0, 0);
  messageLayout->setSpacing(8);

  auto* iconLabel = new QLabel(messageHost);
  iconLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

  auto* textHost = new QWidget(messageHost);
  // Match antd popover max-content behavior: text width should be intrinsic,
  // while still allowing wraps when an external width constraint exists.
  textHost->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  auto* textLayout = new QVBoxLayout(textHost);
  textLayout->setContentsMargins(0, 0, 0, 0);
  textLayout->setSpacing(0);

  auto* titleLabel = new QLabel(textHost);
  titleLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  titleLabel->setWordWrap(true);
  titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);

  auto* descriptionLabel = new QLabel(textHost);
  descriptionLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  descriptionLabel->setWordWrap(true);
  descriptionLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);

  textLayout->addWidget(titleLabel);
  textLayout->addWidget(descriptionLabel);

  messageLayout->addWidget(iconLabel, 0, Qt::AlignTop);
  messageLayout->addWidget(textHost, 0, Qt::AlignTop);

  auto* buttonsHost = new QWidget(host);
  auto* buttonsLayout = new QHBoxLayout(buttonsHost);
  buttonsLayout->setContentsMargins(0, 0, 0, 0);
  buttonsLayout->setSpacing(0);
  buttonsLayout->addStretch();

  auto* buttonsLeadSpacer = new QWidget(buttonsHost);
  buttonsLeadSpacer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
  buttonsLeadSpacer->setFixedWidth(0);

  auto* cancelButton = new AdButton(cancelText_, buttonsHost);
  cancelButton->setSize(AdButton::Size::Small);
  cancelButton->setType(AdButton::Type::Default);

  auto* buttonsInnerSpacer = new QWidget(buttonsHost);
  buttonsInnerSpacer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
  buttonsInnerSpacer->setFixedWidth(0);

  auto* okButton = new AdButton(okText_, buttonsHost);
  okButton->setSize(AdButton::Size::Small);
  okButton->setType(okType_);
  okButton->setLoading(okButtonLoading_);

  buttonsLayout->addWidget(buttonsLeadSpacer);
  buttonsLayout->addWidget(cancelButton);
  buttonsLayout->addWidget(buttonsInnerSpacer);
  buttonsLayout->addWidget(okButton);

  hostLayout->addWidget(messageHost);
  hostLayout->addWidget(buttonsHost);

  contentHost_ = host;
  contentLayout_ = hostLayout;
  messageHost_ = messageHost;
  messageLayout_ = messageLayout;
  iconLabel_ = iconLabel;
  textHost_ = textHost;
  textLayout_ = textLayout;
  titleLabel_ = titleLabel;
  descriptionLabel_ = descriptionLabel;
  buttonsHost_ = buttonsHost;
  buttonsLayout_ = buttonsLayout;
  buttonsLeadSpacer_ = buttonsLeadSpacer;
  buttonsInnerSpacer_ = buttonsInnerSpacer;
  cancelButton_ = cancelButton;
  okButton_ = okButton;

  connect(cancelButton_, &QPushButton::clicked, this, [this]() {
    emit canceled();
    requestCloseAfterAction();
  });
  connect(okButton_, &QPushButton::clicked, this, [this]() {
    emit confirmed();
    if (confirmAutoClose_) {
      requestCloseAfterAction();
    }
  });

  if (popover_) {
    popover_->setContentWidget(contentHost_);
  }
  refreshOverlayWatchers();
}

void AdPopconfirm::syncContentWidget() {
  if (!popover_) {
    return;
  }
  ensureContentHost();

  const bool hasTitleText = !titleText_.trimmed().isEmpty();
  const bool hasDescriptionText = !descriptionText_.trimmed().isEmpty();
  const bool hasOverlayContent = hasTitleText || hasDescriptionText;

  if (!hasOverlayContent) {
    clearOverlayWatchers();
    popover_->setContentWidget(nullptr);
    if (contentHost_ && !contentHost_->parentWidget()) {
      contentHost_->setParent(this);
      contentHost_->hide();
    }
    return;
  }

  if (titleLabel_) {
    titleLabel_->setText(titleText_);
    titleLabel_->setVisible(hasTitleText);
  }
  if (descriptionLabel_) {
    descriptionLabel_->setText(descriptionText_);
    descriptionLabel_->setVisible(hasDescriptionText);
  }
  if (cancelButton_) {
    const QString resolvedCancel = cancelText_.trimmed().isEmpty() ? QStringLiteral("Cancel") : cancelText_;
    cancelButton_->setText(resolvedCancel);
    cancelButton_->setVisible(showCancel_);
  }
  if (buttonsInnerSpacer_) {
    buttonsInnerSpacer_->setVisible(showCancel_);
  }
  if (okButton_) {
    const QString resolvedOk = okText_.trimmed().isEmpty() ? QStringLiteral("OK") : okText_;
    okButton_->setText(resolvedOk);
    okButton_->setType(okType_);
    okButton_->setLoading(okButtonLoading_);
  }

  const bool showIcon = iconVisible_ && adqt::icons::isValid(iconToken_);
  if (iconLabel_) {
    iconLabel_->setVisible(showIcon);
  }
  if (messageHost_) {
    messageHost_->setVisible(showIcon || hasTitleText || hasDescriptionText);
  }
  if (buttonsHost_) {
    buttonsHost_->setVisible(true);
  }

  popover_->setContentWidget(contentHost_);
  refreshOverlayWatchers();
  refreshVisualStyle();
}

void AdPopconfirm::refreshVisualStyle() {
  if (!popover_) {
    return;
  }

  if (!contentHost_) {
    ensureContentHost();
  }
  if (!contentHost_) {
    return;
  }

  const DerivedVisualStyle style = deriveVisualStyle();

  const bool hasDescription = !descriptionText_.trimmed().isEmpty();
  if (contentLayout_) {
    contentLayout_->setSpacing(std::max(0, style.messageBottom));
  }
  if (messageLayout_) {
    messageLayout_->setSpacing(std::max(0, style.messageGap));
  }
  if (textLayout_) {
    textLayout_->setSpacing(0);
  }
  if (buttonsLayout_) {
    buttonsLayout_->setSpacing(0);
  }
  if (buttonsLeadSpacer_) {
    buttonsLeadSpacer_->setFixedWidth(std::max(0, style.buttonGap));
  }
  if (buttonsInnerSpacer_) {
    buttonsInnerSpacer_->setFixedWidth(std::max(0, style.buttonGap));
  }

  if (titleLabel_) {
    titleLabel_->setFont(hasDescription ? style.titleFont : style.titleOnlyFont);
    QPalette titlePalette = titleLabel_->palette();
    titlePalette.setColor(QPalette::WindowText, style.titleColor);
    titleLabel_->setPalette(titlePalette);
  }
  if (descriptionLabel_) {
    const int descriptionTopGap = hasDescription ? std::max(0, style.descriptionGap) : 0;
    descriptionLabel_->setContentsMargins(0, descriptionTopGap, 0, 0);
    descriptionLabel_->setFont(style.descriptionFont);
    QPalette descriptionPalette = descriptionLabel_->palette();
    descriptionPalette.setColor(QPalette::WindowText, style.descriptionColor);
    descriptionLabel_->setPalette(descriptionPalette);
  }

  // Ant Design popover uses width: max-content for popup and lets text wrap
  // only when constrained by viewport max-width. QLabel with word-wrap can
  // report a shrinkable preferred width, so explicitly anchor the message text
  // host to intrinsic one-line width to avoid premature wrapping.
  if (textHost_) {
    const QFont titleFont = hasDescription ? style.titleFont : style.titleOnlyFont;
    const int titleWidth = measurePlainTextWidth(titleText_, titleFont);
    const int descriptionWidth = measurePlainTextWidth(descriptionText_, style.descriptionFont);
    const int intrinsicTextWidth = std::max(titleWidth, descriptionWidth);
    textHost_->setMinimumWidth(std::max(0, intrinsicTextWidth > 0 ? intrinsicTextWidth + 1 : 0));
  }

  const bool showIcon = iconVisible_ && adqt::icons::isValid(iconToken_);
  if (iconLabel_) {
    iconLabel_->setVisible(showIcon);
    if (showIcon) {
      adqt::icons::IconToken icon = iconToken_;
      if (!icon.style.hasPrimary) {
        icon.style.primary = style.iconColor;
        icon.style.hasPrimary = true;
      }
      const QSize iconSize(std::max(10, style.iconSize), std::max(10, style.iconSize));
      const qreal dpr = std::max(1.0, devicePixelRatioF());
      iconLabel_->setPixmap(adqt::icons::renderIconPixmap(icon, iconSize, dpr));
      iconLabel_->setFixedSize(iconSize);
    } else {
      iconLabel_->setPixmap(QPixmap());
    }
  }

  AdPopover::ComponentTokens popoverTokens;
  popoverTokens.titleMinWidth = style.titleMinWidth;
  popoverTokens.zIndexPopup = style.zIndexPopup;
  popover_->setComponentTokens(popoverTokens);
  popover_->setSemanticStyles(style.popoverSemantic);
}

AdPopconfirm::DerivedVisualStyle AdPopconfirm::deriveVisualStyle() const {
  DerivedVisualStyle style;
  const adqt::theme::ThemeManager& themeManager = adqt::theme::ThemeManager::instance();
  const adqt::theme::ThemeMapToken& map = themeManager.currentMapToken();
  const adqt::theme::ThemeSeedToken& seed = themeManager.currentConfig().seed;

  style.titleMinWidth = 177;
  style.zIndexPopup = static_cast<int>(std::round(seed.zIndexPopupBase + 60.0));
  style.messageGap = std::max(0, qRound(map.sizeXS));
  style.messageBottom = std::max(0, qRound(map.sizeXS));
  style.descriptionGap = std::max(0, qRound(map.sizeXXS));
  style.buttonGap = std::max(0, qRound(map.sizeXS));
  style.iconSize = std::max(12, qRound(map.fontSize));
  style.titleColor = toColor(map.colorText, QColor("#141414"));
  style.descriptionColor = toColor(map.colorText, QColor("#141414"));
  style.iconColor = toColor(map.colorWarning, QColor("#faad14"));

  style.titleFont = font();
  style.titleFont.setPixelSize(std::max(12, qRound(map.fontSize)));
  style.titleFont.setWeight(QFont::DemiBold);

  style.titleOnlyFont = style.titleFont;
  style.titleOnlyFont.setWeight(QFont::Normal);

  style.descriptionFont = font();
  style.descriptionFont.setPixelSize(std::max(12, qRound(map.fontSize)));
  style.descriptionFont.setWeight(QFont::Normal);

  if (disabled()) {
    const QColor disabledText = toColor(map.colorTextQuaternary, QColor("#bfbfbf"));
    style.titleColor = disabledText;
    style.descriptionColor = disabledText;
  }

  if (componentTokens_.titleMinWidth.has_value()) {
    style.titleMinWidth = std::max(0, componentTokens_.titleMinWidth.value());
  }
  if (componentTokens_.zIndexPopup.has_value()) {
    style.zIndexPopup = std::max(0, componentTokens_.zIndexPopup.value());
  }
  if (componentTokens_.messageGap.has_value()) {
    style.messageGap = std::max(0, componentTokens_.messageGap.value());
  }
  if (componentTokens_.messageBottom.has_value()) {
    style.messageBottom = std::max(0, componentTokens_.messageBottom.value());
  }
  if (componentTokens_.descriptionGap.has_value()) {
    style.descriptionGap = std::max(0, componentTokens_.descriptionGap.value());
  }
  if (componentTokens_.buttonGap.has_value()) {
    style.buttonGap = std::max(0, componentTokens_.buttonGap.value());
  }
  if (componentTokens_.iconSize.has_value()) {
    style.iconSize = std::max(10, componentTokens_.iconSize.value());
  }
  if (componentTokens_.iconColor.has_value()) {
    style.iconColor = toColor(componentTokens_.iconColor.value(), style.iconColor);
  }

  StyleContext context;
  context.placement = placement();
  context.triggerModes = triggerModes();
  context.open = open();
  context.disabled = disabled();
  context.arrowVisible = arrowVisible();
  const SemanticStyles effectiveSemantic =
      semanticStyleResolver_ ? semanticStyleResolver_(context) : semanticStyles_;

  applySemanticSlot(effectiveSemantic.title, &style.titleColor, nullptr, nullptr);
  applySemanticSlot(effectiveSemantic.description, &style.descriptionColor, nullptr, nullptr);
  applySemanticSlot(effectiveSemantic.icon, &style.iconColor, nullptr, nullptr);

  style.popoverSemantic.root = toPopoverSemanticSlot(effectiveSemantic.root);
  style.popoverSemantic.container = toPopoverSemanticSlot(effectiveSemantic.container);
  style.popoverSemantic.title = toPopoverSemanticSlot(effectiveSemantic.title);
  style.popoverSemantic.content = toPopoverSemanticSlot(effectiveSemantic.description);
  style.popoverSemantic.arrow = toPopoverSemanticSlot(effectiveSemantic.arrow);

  return style;
}

void AdPopconfirm::refreshOverlayWatchers() {
  clearOverlayWatchers();
  if (!contentHost_) {
    return;
  }

  traverseObjectTree(contentHost_, [this](QObject* object) {
    if (!object) {
      return;
    }
    object->installEventFilter(this);
    watchedOverlayObjects_.insert(object);
  });
}

void AdPopconfirm::clearOverlayWatchers() {
  for (auto it = watchedOverlayObjects_.begin(); it != watchedOverlayObjects_.end(); ++it) {
    if (it->data()) {
      it->data()->removeEventFilter(this);
    }
  }
  watchedOverlayObjects_.clear();
}

void AdPopconfirm::requestCloseAfterAction() {
  if (!popover_) {
    return;
  }
  if (popover_->openControlled()) {
    emit onOpenChange(false);
    return;
  }
  const bool wasOpen = popover_->open();
  popover_->setOpen(false);
  if (wasOpen) {
    emit onOpenChange(false);
  }
}

}  // namespace adqt::widgets
