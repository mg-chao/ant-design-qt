#include "alert.h"

#include "alert_style.h"
#include "icons.h"
#include "theme/theme.h"

#include <QEvent>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

namespace adqt::widgets {

namespace {

QString toRgba(const QColor& color) {
  return QStringLiteral("rgba(%1, %2, %3, %4)")
      .arg(color.red())
      .arg(color.green())
      .arg(color.blue())
      .arg(color.alpha());
}

adqt::icons::IconToken defaultTypeIcon(AdAlert::Type type) {
  adqt::icons::IconToken token;
  switch (type) {
    case AdAlert::Type::Success:
      token = adqt::icons::filled::CheckCircle();
      break;
    case AdAlert::Type::Info:
      token = adqt::icons::filled::InfoCircle();
      break;
    case AdAlert::Type::Warning:
      token = adqt::icons::filled::ExclamationCircle();
      break;
    case AdAlert::Type::Error:
      token = adqt::icons::filled::CloseCircle();
      break;
  }
  return token;
}

void mergeSemanticSlot(AdAlert::SemanticSlotStyle* target, const AdAlert::SemanticSlotStyle& source) {
  if (source.textColor.has_value()) {
    target->textColor = source.textColor;
  }
  if (source.backgroundColor.has_value()) {
    target->backgroundColor = source.backgroundColor;
  }
  if (source.borderColor.has_value()) {
    target->borderColor = source.borderColor;
  }
}

}  // namespace

AdAlert::AdAlert(QWidget* parent) : QWidget(parent) {
  setObjectName(QStringLiteral("ad-alert"));
  setAttribute(Qt::WA_StyledBackground, true);
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

  ensureLayout();
  refreshContent();
  applyVisualStyle();
  refreshOpenState();

  connect(&adqt::theme::ThemeManager::instance(), &adqt::theme::ThemeManager::themeChanged, this,
          [this]() {
            applyVisualStyle();
            refreshContent();
          });
}

AdAlert::~AdAlert() {
  if (closeAnimation_) {
    closeAnimation_->stop();
  }
}

AdAlert::Type AdAlert::type() const {
  const DerivedState state = deriveState();
  return state.type;
}

void AdAlert::setType(Type value) {
  if (typeExplicit_ && typeValue_ == value) {
    return;
  }
  const Type before = type();
  typeExplicit_ = true;
  typeValue_ = value;
  if (type() != before) {
    emit typeChanged(type());
  }
  applyVisualStyle();
  refreshContent();
}

bool AdAlert::banner() const { return banner_; }

void AdAlert::setBanner(bool value) {
  if (banner_ == value) {
    return;
  }
  const Type beforeType = type();
  const bool beforeShowIcon = showIcon();
  banner_ = value;
  emit bannerChanged(banner_);
  if (type() != beforeType) {
    emit typeChanged(type());
  }
  if (showIcon() != beforeShowIcon) {
    emit showIconChanged(showIcon());
  }
  applyVisualStyle();
  refreshContent();
}

bool AdAlert::showIcon() const { return deriveState().showIcon; }

void AdAlert::setShowIcon(bool value) {
  if (showIconExplicit_ && showIconValue_ == value) {
    return;
  }
  const bool before = showIcon();
  showIconExplicit_ = true;
  showIconValue_ = value;
  if (showIcon() != before) {
    emit showIconChanged(showIcon());
  }
  applyVisualStyle();
  refreshContent();
}

bool AdAlert::closable() const { return closable_; }

void AdAlert::setClosable(bool value) {
  if (closable_ == value) {
    return;
  }
  closable_ = value;
  emit closableChanged(closable_);
  applyVisualStyle();
  refreshContent();
}

bool AdAlert::open() const { return open_; }

void AdAlert::setOpen(bool value) {
  if (open_ == value) {
    return;
  }
  open_ = value;
  emit openChanged(open_);
  refreshOpenState();
}

QString AdAlert::titleText() const { return titleText_; }

void AdAlert::setTitleText(const QString& value) {
  if (titleText_ == value) {
    return;
  }
  titleText_ = value;
  emit titleTextChanged(titleText_);
  refreshContent();
}

QString AdAlert::descriptionText() const { return descriptionText_; }

void AdAlert::setDescriptionText(const QString& value) {
  if (descriptionText_ == value) {
    return;
  }
  descriptionText_ = value;
  emit descriptionTextChanged(descriptionText_);
  refreshContent();
}

adqt::icons::IconToken AdAlert::iconToken() const { return iconToken_; }

void AdAlert::setIconToken(const adqt::icons::IconToken& value) {
  if (iconToken_ == value) {
    return;
  }
  iconToken_ = value;
  emit iconTokenChanged(iconToken_);
  applyVisualStyle();
  refreshContent();
}

adqt::icons::IconToken AdAlert::closeIconToken() const { return closeIconToken_; }

void AdAlert::setCloseIconToken(const adqt::icons::IconToken& value) {
  if (closeIconToken_ == value) {
    return;
  }
  closeIconToken_ = value;
  emit closeIconTokenChanged(closeIconToken_);
  updateCloseButtonIcon();
}

QWidget* AdAlert::titleWidget() const { return titleWidget_; }

void AdAlert::setTitleWidget(QWidget* widget) {
  if (titleWidget_ == widget) {
    return;
  }
  if (titleWidget_) {
    titleWidget_->hide();
    titleWidget_->setParent(nullptr);
  }
  titleWidget_ = widget;
  if (titleWidget_) {
    titleWidget_->hide();
    if (titleWidget_->parentWidget()) {
      titleWidget_->setParent(nullptr);
    }
  }
  emit titleWidgetChanged(titleWidget_);
  refreshContent();
}

QWidget* AdAlert::descriptionWidget() const { return descriptionWidget_; }

void AdAlert::setDescriptionWidget(QWidget* widget) {
  if (descriptionWidget_ == widget) {
    return;
  }
  if (descriptionWidget_) {
    descriptionWidget_->hide();
    descriptionWidget_->setParent(nullptr);
  }
  descriptionWidget_ = widget;
  if (descriptionWidget_) {
    descriptionWidget_->hide();
    if (descriptionWidget_->parentWidget()) {
      descriptionWidget_->setParent(nullptr);
    }
  }
  emit descriptionWidgetChanged(descriptionWidget_);
  refreshContent();
}

QWidget* AdAlert::actionWidget() const { return actionWidget_; }

void AdAlert::setActionWidget(QWidget* widget) {
  if (actionWidget_ == widget) {
    return;
  }
  if (actionWidget_) {
    actionWidget_->hide();
    actionWidget_->setParent(nullptr);
  }
  actionWidget_ = widget;
  if (actionWidget_) {
    actionWidget_->hide();
    if (actionWidget_->parentWidget()) {
      actionWidget_->setParent(nullptr);
    }
  }
  emit actionWidgetChanged(actionWidget_);
  refreshContent();
}

AdAlert::ComponentTokens AdAlert::componentTokens() const { return componentTokens_; }

void AdAlert::setComponentTokens(const ComponentTokens& tokens) {
  componentTokens_ = tokens;
  emit componentTokensChanged();
  applyVisualStyle();
  refreshContent();
}

void AdAlert::resetComponentTokens() {
  componentTokens_ = ComponentTokens{};
  emit componentTokensChanged();
  applyVisualStyle();
  refreshContent();
}

AdAlert::SemanticStyles AdAlert::semanticStyles() const { return semanticStyles_; }

void AdAlert::setSemanticStyles(const SemanticStyles& styles) {
  semanticStyles_ = styles;
  emit semanticStylesChanged();
  applyVisualStyle();
}

void AdAlert::setSemanticStyleResolver(SemanticStyleResolver resolver) {
  semanticStyleResolver_ = std::move(resolver);
  emit semanticStylesChanged();
  applyVisualStyle();
}

bool AdAlert::eventFilter(QObject* watched, QEvent* event) {
  if (watched == closeButton_ && event) {
    if (event->type() == QEvent::Enter) {
      closeButtonHovered_ = true;
      updateCloseButtonIcon();
    } else if (event->type() == QEvent::Leave) {
      closeButtonHovered_ = false;
      updateCloseButtonIcon();
    }
  }
  return QWidget::eventFilter(watched, event);
}

void AdAlert::changeEvent(QEvent* event) {
  QWidget::changeEvent(event);
  if (!event) {
    return;
  }
  if (event->type() == QEvent::EnabledChange || event->type() == QEvent::FontChange) {
    applyVisualStyle();
  }
}

AdAlert::DerivedState AdAlert::deriveState() const {
  DerivedState state;
  state.type = typeExplicit_ ? typeValue_ : (banner_ ? Type::Warning : Type::Info);
  state.showIcon = showIconExplicit_ ? showIconValue_ : banner_;
  state.hasDescription = descriptionWidget_ || !descriptionText_.trimmed().isEmpty();
  return state;
}

AdAlert::SemanticStyles AdAlert::resolvedSemanticStyles() const {
  SemanticStyles merged = semanticStyles_;
  if (!semanticStyleResolver_) {
    return merged;
  }

  const DerivedState state = deriveState();
  StyleContext context;
  context.type = state.type;
  context.banner = banner_;
  context.showIcon = state.showIcon;
  context.closable = closable_;
  context.hasDescription = state.hasDescription;
  context.open = open_;

  const SemanticStyles resolved = semanticStyleResolver_(context);
  mergeSemanticSlot(&merged.root, resolved.root);
  mergeSemanticSlot(&merged.icon, resolved.icon);
  mergeSemanticSlot(&merged.section, resolved.section);
  mergeSemanticSlot(&merged.title, resolved.title);
  mergeSemanticSlot(&merged.description, resolved.description);
  mergeSemanticSlot(&merged.actions, resolved.actions);
  mergeSemanticSlot(&merged.close, resolved.close);
  return merged;
}

adqt::icons::IconToken AdAlert::resolvedIconToken() const {
  if (adqt::icons::isValid(iconToken_)) {
    return iconToken_;
  }
  return defaultTypeIcon(type());
}

adqt::icons::IconToken AdAlert::resolvedCloseIconToken() const {
  if (adqt::icons::isValid(closeIconToken_)) {
    return closeIconToken_;
  }
  return adqt::icons::outlined::Close();
}

void AdAlert::ensureLayout() {
  if (rootLayout_) {
    return;
  }

  auto* layout = new QHBoxLayout(this);
  layout->setContentsMargins(12, 8, 12, 8);
  layout->setSpacing(8);

  auto* iconLabel = new QLabel(this);
  iconLabel->setVisible(false);
  iconLabel->setAlignment(Qt::AlignCenter);

  auto* sectionHost = new QWidget(this);
  sectionHost->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  auto* sectionLayout = new QVBoxLayout(sectionHost);
  sectionLayout->setContentsMargins(0, 0, 0, 0);
  sectionLayout->setSpacing(4);

  auto* titleHost = new QWidget(sectionHost);
  auto* titleLayout = new QVBoxLayout(titleHost);
  titleLayout->setContentsMargins(0, 0, 0, 0);
  titleLayout->setSpacing(0);
  auto* titleLabel = new QLabel(titleHost);
  titleLabel->setWordWrap(true);
  titleLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

  auto* descriptionHost = new QWidget(sectionHost);
  auto* descriptionLayout = new QVBoxLayout(descriptionHost);
  descriptionLayout->setContentsMargins(0, 0, 0, 0);
  descriptionLayout->setSpacing(0);
  auto* descriptionLabel = new QLabel(descriptionHost);
  descriptionLabel->setWordWrap(true);
  descriptionLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

  sectionLayout->addWidget(titleHost);
  sectionLayout->addWidget(descriptionHost);

  auto* actionHost = new QWidget(this);
  actionHost->setVisible(false);
  auto* actionLayout = new QVBoxLayout(actionHost);
  actionLayout->setContentsMargins(0, 0, 0, 0);
  actionLayout->setSpacing(0);

  auto* closeButton = new QToolButton(this);
  closeButton->setAutoRaise(true);
  closeButton->setCursor(Qt::PointingHandCursor);
  closeButton->setVisible(false);
  closeButton->installEventFilter(this);

  layout->addWidget(iconLabel, 0, Qt::AlignVCenter);
  layout->addWidget(sectionHost, 1, Qt::AlignVCenter);
  layout->addWidget(actionHost, 0, Qt::AlignVCenter);
  layout->addWidget(closeButton, 0, Qt::AlignVCenter);

  opacityEffect_ = new QGraphicsOpacityEffect(this);
  opacityEffect_->setOpacity(1.0);
  setGraphicsEffect(opacityEffect_);

  closeAnimation_ = new QParallelAnimationGroup(this);
  heightAnimation_ = new QPropertyAnimation(this, "maximumHeight", closeAnimation_);
  opacityAnimation_ = new QPropertyAnimation(opacityEffect_, "opacity", closeAnimation_);
  connect(closeAnimation_, &QParallelAnimationGroup::finished, this, [this]() {
    finishCloseAnimation(true);
  });

  connect(closeButton, &QToolButton::clicked, this, [this]() {
    emit closeRequested();
    setOpen(false);
  });

  rootLayout_ = layout;
  iconLabel_ = iconLabel;
  sectionHost_ = sectionHost;
  sectionLayout_ = sectionLayout;
  titleHost_ = titleHost;
  titleLayout_ = titleLayout;
  titleLabel_ = titleLabel;
  descriptionHost_ = descriptionHost;
  descriptionLayout_ = descriptionLayout;
  descriptionLabel_ = descriptionLabel;
  actionHost_ = actionHost;
  actionLayout_ = actionLayout;
  closeButton_ = closeButton;
}

void AdAlert::refreshContent() {
  ensureLayout();

  const DerivedState state = deriveState();
  const bool hasTitleContent = titleWidget_ || !titleText_.trimmed().isEmpty();
  const bool hasDescriptionContent = descriptionWidget_ || !descriptionText_.trimmed().isEmpty();

  clearLayout(titleLayout_);
  clearLayout(descriptionLayout_);
  clearLayout(actionLayout_);

  if (titleWidget_) {
    if (titleWidget_->parentWidget() != titleHost_) {
      titleWidget_->setParent(titleHost_);
    }
    titleLayout_->addWidget(titleWidget_);
    titleWidget_->show();
  } else if (hasTitleContent && titleLabel_) {
    titleLabel_->setText(titleText_);
    titleLayout_->addWidget(titleLabel_);
    titleLabel_->show();
  }

  if (descriptionWidget_) {
    if (descriptionWidget_->parentWidget() != descriptionHost_) {
      descriptionWidget_->setParent(descriptionHost_);
    }
    descriptionLayout_->addWidget(descriptionWidget_);
    descriptionWidget_->show();
  } else if (hasDescriptionContent && descriptionLabel_) {
    descriptionLabel_->setText(descriptionText_);
    descriptionLayout_->addWidget(descriptionLabel_);
    descriptionLabel_->show();
  }

  if (actionWidget_) {
    if (actionWidget_->parentWidget() != actionHost_) {
      actionWidget_->setParent(actionHost_);
    }
    actionLayout_->addWidget(actionWidget_);
    actionWidget_->show();
    actionHost_->setVisible(true);
  } else {
    actionHost_->setVisible(false);
  }

  if (titleHost_) {
    titleHost_->setVisible(hasTitleContent);
  }
  if (descriptionHost_) {
    descriptionHost_->setVisible(hasDescriptionContent);
  }

  if (iconLabel_) {
    iconLabel_->setVisible(state.showIcon);
  }
  if (closeButton_) {
    closeButton_->setVisible(closable_);
  }

  applyVisualStyle();
}

void AdAlert::applyVisualStyle() {
  ensureLayout();

  detail::AlertStyleInput styleInput;
  const DerivedState state = deriveState();
  styleInput.type = state.type;
  styleInput.banner = banner_;
  styleInput.showIcon = state.showIcon;
  styleInput.closable = closable_;
  styleInput.hasDescription = state.hasDescription;
  styleInput.open = open_;
  styleInput.baseFont = font();
  styleInput.componentTokens = componentTokens_;
  styleInput.semanticStyles = resolvedSemanticStyles();
  const detail::AlertVisualStyle style = detail::resolveAlertVisualStyle(styleInput);

  const int horizontalPadding =
      state.hasDescription ? style.metrics.withDescriptionPadding : style.metrics.defaultPaddingHorizontal;
  const int verticalPadding =
      state.hasDescription ? style.metrics.withDescriptionPadding : style.metrics.defaultPaddingVertical;
  if (rootLayout_) {
    rootLayout_->setContentsMargins(horizontalPadding, verticalPadding, horizontalPadding, verticalPadding);
    rootLayout_->setSpacing(state.hasDescription ? style.metrics.actionGap : style.metrics.iconGap);
    rootLayout_->setAlignment(iconLabel_, state.hasDescription ? Qt::AlignTop : Qt::AlignVCenter);
    rootLayout_->setAlignment(sectionHost_, state.hasDescription ? Qt::AlignTop : Qt::AlignVCenter);
    rootLayout_->setAlignment(actionHost_, state.hasDescription ? Qt::AlignTop : Qt::AlignVCenter);
    rootLayout_->setAlignment(closeButton_, state.hasDescription ? Qt::AlignTop : Qt::AlignVCenter);
  }
  if (sectionLayout_) {
    sectionLayout_->setSpacing(style.metrics.titleDescriptionGap);
  }

  if (titleLabel_) {
    titleLabel_->setFont(state.hasDescription ? style.metrics.titleWithDescriptionFont : style.metrics.titleFont);
    QPalette palette = titleLabel_->palette();
    palette.setColor(QPalette::WindowText, style.titleColor);
    titleLabel_->setPalette(palette);
  }
  if (descriptionLabel_) {
    descriptionLabel_->setFont(style.metrics.descriptionFont);
    QPalette palette = descriptionLabel_->palette();
    palette.setColor(QPalette::WindowText, style.descriptionColor);
    descriptionLabel_->setPalette(palette);
  }

  if (actionHost_) {
    actionHost_->setStyleSheet(QStringLiteral("color: %1;").arg(toRgba(style.actionTextColor)));
  }

  setStyleSheet(QStringLiteral(
                    "QWidget#ad-alert {"
                    "  background-color: %1;"
                    "  border-width: %2px;"
                    "  border-style: solid;"
                    "  border-color: %3;"
                    "  border-radius: %4px;"
                    "} ")
                    .arg(toRgba(style.background))
                    .arg(style.metrics.borderWidth)
                    .arg(toRgba(style.border))
                    .arg(style.metrics.borderRadius));

  if (iconLabel_ && state.showIcon) {
    adqt::icons::IconToken icon = resolvedIconToken();
    if (!icon.style.hasPrimary) {
      icon.style.primary = style.iconColor;
      icon.style.hasPrimary = true;
    }
    const int iconSize = state.hasDescription ? style.metrics.withDescriptionIconSize : style.metrics.iconSize;
    iconLabel_->setFixedSize(iconSize, iconSize);
    iconLabel_->setPixmap(
        adqt::icons::renderIconPixmap(icon, QSize(iconSize, iconSize), std::max(1.0, devicePixelRatioF())));
  }

  if (closeButton_) {
    closeButton_->setFixedSize(style.metrics.closeButtonSize, style.metrics.closeButtonSize);
    closeButton_->setIconSize(QSize(style.metrics.closeIconSize, style.metrics.closeIconSize));
    closeButton_->setStyleSheet(
        QStringLiteral("QToolButton {"
                       "  border: none;"
                       "  background: transparent;"
                       "  padding: 0px;"
                       "}"));
  }

  updateCloseButtonIcon();
}

void AdAlert::refreshOpenState() {
  ensureLayout();
  if (open_) {
    if (closeAnimation_ && closeAnimation_->state() == QAbstractAnimation::Running) {
      closeAnimation_->stop();
    }
    setVisible(true);
    setMaximumHeight(QWIDGETSIZE_MAX);
    if (opacityEffect_) {
      opacityEffect_->setOpacity(1.0);
    }
    return;
  }

  detail::AlertStyleInput styleInput;
  const DerivedState state = deriveState();
  styleInput.type = state.type;
  styleInput.banner = banner_;
  styleInput.showIcon = state.showIcon;
  styleInput.closable = closable_;
  styleInput.hasDescription = state.hasDescription;
  styleInput.open = open_;
  styleInput.baseFont = font();
  styleInput.componentTokens = componentTokens_;
  styleInput.semanticStyles = resolvedSemanticStyles();
  const detail::AlertVisualStyle style = detail::resolveAlertVisualStyle(styleInput);

  if (style.metrics.animationDurationMs <= 0 || !isVisible()) {
    finishCloseAnimation(true);
    return;
  }
  startCloseAnimation(style.metrics.animationDurationMs);
}

void AdAlert::startCloseAnimation(int durationMs) {
  if (!closeAnimation_ || !heightAnimation_ || !opacityAnimation_) {
    finishCloseAnimation(true);
    return;
  }

  setVisible(true);
  int startHeight = height();
  if (startHeight <= 0) {
    startHeight = sizeHint().height();
  }
  startHeight = std::max(1, startHeight);

  setMaximumHeight(startHeight);
  closeAnimation_->stop();

  heightAnimation_->setDuration(durationMs);
  heightAnimation_->setStartValue(startHeight);
  heightAnimation_->setEndValue(0);
  heightAnimation_->setEasingCurve(QEasingCurve::InOutCubic);

  opacityAnimation_->setDuration(durationMs);
  opacityAnimation_->setStartValue(1.0);
  opacityAnimation_->setEndValue(0.0);
  opacityAnimation_->setEasingCurve(QEasingCurve::InOutCubic);

  closeAnimation_->start();
}

void AdAlert::finishCloseAnimation(bool emitAfterCloseSignal) {
  if (open_) {
    setVisible(true);
    setMaximumHeight(QWIDGETSIZE_MAX);
    if (opacityEffect_) {
      opacityEffect_->setOpacity(1.0);
    }
    return;
  }

  if (closeAnimation_ && closeAnimation_->state() == QAbstractAnimation::Running) {
    closeAnimation_->stop();
  }
  setVisible(false);
  setMaximumHeight(QWIDGETSIZE_MAX);
  if (opacityEffect_) {
    opacityEffect_->setOpacity(1.0);
  }
  if (emitAfterCloseSignal) {
    emit afterClose();
  }
}

void AdAlert::updateCloseButtonIcon() {
  if (!closeButton_) {
    return;
  }

  detail::AlertStyleInput styleInput;
  const DerivedState state = deriveState();
  styleInput.type = state.type;
  styleInput.banner = banner_;
  styleInput.showIcon = state.showIcon;
  styleInput.closable = closable_;
  styleInput.hasDescription = state.hasDescription;
  styleInput.open = open_;
  styleInput.baseFont = font();
  styleInput.componentTokens = componentTokens_;
  styleInput.semanticStyles = resolvedSemanticStyles();
  const detail::AlertVisualStyle style = detail::resolveAlertVisualStyle(styleInput);

  adqt::icons::IconToken token = resolvedCloseIconToken();
  if (!token.style.hasPrimary) {
    token.style.primary = closeButtonHovered_ ? style.closeHoverColor : style.closeColor;
    token.style.hasPrimary = true;
  }
  closeButton_->setIcon(adqt::icons::makeIcon(token));
}

void AdAlert::clearLayout(QLayout* layout) {
  if (!layout) {
    return;
  }
  while (QLayoutItem* item = layout->takeAt(0)) {
    if (QWidget* widget = item->widget()) {
      widget->hide();
    }
    delete item;
  }
}

}  // namespace adqt::widgets
