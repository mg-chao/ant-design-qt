#include "modal.h"

#include "icons.h"
#include "theme/fast_color_lite.h"
#include "theme/theme.h"

#include <QApplication>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLayoutItem>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QShortcut>
#include <QToolButton>
#include <QVector>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

namespace adqt::widgets {

namespace {

namespace filled_icons = adqt::icons::filled;
namespace outlined_icons = adqt::icons::outlined;

class ModalPanelWidget final : public QFrame {
 public:
  struct PaintStyle {
    QColor containerBg = QColor("#ffffff");
    QColor headerBg = QColor("#ffffff");
    QColor bodyBg = QColor("#ffffff");
    QColor footerBg = QColor("#ffffff");
    QColor borderColor = QColor("#f0f0f0");
    int borderRadius = 8;
    int borderWidth = 0;
    int footerBorderTopWidth = 0;
  };

  explicit ModalPanelWidget(QWidget* parent = nullptr) : QFrame(parent) {
    setAttribute(Qt::WA_StyledBackground, false);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAutoFillBackground(false);
  }

  void setSectionWidgets(QWidget* header, QWidget* body, QWidget* footer) {
    header_ = header;
    body_ = body;
    footer_ = footer;
    update();
  }

  void setPaintStyle(const PaintStyle& style) {
    paintStyle_ = style;
    update();
  }

 protected:
  void paintEvent(QPaintEvent* event) override {
    Q_UNUSED(event)

    const QRect widgetRect = rect();
    if (widgetRect.width() <= 0 || widgetRect.height() <= 0) {
      return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const qreal borderWidth = std::max(0.0, static_cast<qreal>(paintStyle_.borderWidth));
    const qreal borderRadius = std::max(0.0, static_cast<qreal>(paintStyle_.borderRadius));
    QRectF fillRect(widgetRect);
    if (borderWidth > 0.0) {
      const qreal inset = borderWidth / 2.0;
      fillRect.adjust(inset, inset, -inset, -inset);
    }
    if (fillRect.width() <= 0.0 || fillRect.height() <= 0.0) {
      return;
    }

    const qreal maxRadius = std::min(fillRect.width(), fillRect.height()) / 2.0;
    const qreal effectiveRadius = std::min(borderRadius, maxRadius);

    QPainterPath panelPath;
    panelPath.addRoundedRect(fillRect, effectiveRadius, effectiveRadius);

    painter.fillPath(panelPath, paintStyle_.containerBg);

    painter.save();
    painter.setClipPath(panelPath);
    fillSection(painter, header_, paintStyle_.headerBg);
    fillSection(painter, body_, paintStyle_.bodyBg);
    fillSection(painter, footer_, paintStyle_.footerBg);

    if (footer_ && footer_->isVisible() && paintStyle_.footerBorderTopWidth > 0 &&
        paintStyle_.borderColor.alpha() > 0) {
      const QRect footerRect = footer_->geometry();
      const qreal separatorWidth = static_cast<qreal>(paintStyle_.footerBorderTopWidth);
      QPen separatorPen(paintStyle_.borderColor);
      separatorPen.setWidthF(separatorWidth);
      separatorPen.setCapStyle(Qt::FlatCap);
      painter.setPen(separatorPen);

      const qreal y = std::clamp(static_cast<qreal>(footerRect.top()) + separatorWidth / 2.0,
                                 fillRect.top(), fillRect.bottom());
      painter.drawLine(QPointF(fillRect.left(), y), QPointF(fillRect.right(), y));
    }
    painter.restore();

    if (borderWidth > 0.0 && paintStyle_.borderColor.alpha() > 0) {
      QPen borderPen(paintStyle_.borderColor);
      borderPen.setWidthF(borderWidth);
      borderPen.setJoinStyle(Qt::RoundJoin);
      painter.setPen(borderPen);
      painter.setBrush(Qt::NoBrush);
      painter.drawPath(panelPath);
    }
  }

 private:
  static void fillSection(QPainter& painter, QWidget* section, const QColor& color) {
    if (!section || !section->isVisible() || color.alpha() <= 0) {
      return;
    }
    painter.fillRect(section->geometry(), color);
  }

  QWidget* header_ = nullptr;
  QWidget* body_ = nullptr;
  QWidget* footer_ = nullptr;
  PaintStyle paintStyle_;
};

class ModalOverlayWidget final : public QWidget {
 public:
  explicit ModalOverlayWidget(QWidget* parent = nullptr) : QWidget(parent) {
    setAttribute(Qt::WA_StyledBackground, false);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAutoFillBackground(false);
    setFocusPolicy(Qt::StrongFocus);
  }

  void setRootColor(const QColor& color) {
    if (rootColor_ == color) {
      return;
    }
    rootColor_ = color;
    update();
  }

  void setMaskEnabled(bool enabled) {
    if (maskEnabled_ == enabled) {
      return;
    }
    maskEnabled_ = enabled;
    update();
  }

  void setMaskColor(const QColor& color) {
    if (maskColor_ == color) {
      return;
    }
    maskColor_ = color;
    update();
  }

 protected:
  void paintEvent(QPaintEvent* event) override {
    Q_UNUSED(event)

    QPainter painter(this);
    if (rootColor_.alpha() > 0) {
      painter.fillRect(rect(), rootColor_);
    }
    if (maskEnabled_ && maskColor_.alpha() > 0) {
      painter.fillRect(rect(), maskColor_);
    }
  }

 private:
  QColor rootColor_ = QColor(0, 0, 0, 0);
  QColor maskColor_ = QColor(0, 0, 0, 115);
  bool maskEnabled_ = true;
};

QVector<QPointer<AdModal>>& staticModals() {
  static QVector<QPointer<AdModal>> modals;
  return modals;
}

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

AdModal::AdModal(QWidget* parent) : QWidget(parent) {
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  setFixedSize(0, 0);
  hide();

  connect(&adqt::theme::ThemeManager::instance(), &adqt::theme::ThemeManager::themeChanged, this,
          [this]() { applyVisualStyle(); });
}

AdModal::~AdModal() {
  installGlobalWatcher(false);
  detachScopeWindowWatcher();
  releaseOverlay();
  unregisterStaticModal(this);
}

bool AdModal::open() const { return open_; }

void AdModal::setOpen(bool value) { setOpenInternal(value, true); }

bool AdModal::centered() const { return centered_; }

void AdModal::setCentered(bool value) {
  if (centered_ == value) {
    return;
  }
  centered_ = value;
  emit centeredChanged(centered_);
  refreshLayout();
}

int AdModal::width() const { return width_; }

void AdModal::setWidth(int value) {
  const int clamped = std::max(240, value);
  if (width_ == clamped) {
    return;
  }
  width_ = clamped;
  emit widthChanged(width_);
  refreshLayout();
}

int AdModal::top() const { return top_; }

void AdModal::setTop(int value) {
  const int clamped = std::max(0, value);
  if (top_ == clamped) {
    return;
  }
  top_ = clamped;
  emit topChanged(top_);
  refreshLayout();
}

bool AdModal::mask() const { return mask_; }

void AdModal::setMask(bool value) {
  if (mask_ == value) {
    return;
  }
  mask_ = value;
  emit maskChanged(mask_);
  applyVisualStyle();
}

bool AdModal::maskClosable() const { return maskClosable_; }

void AdModal::setMaskClosable(bool value) {
  if (maskClosable_ == value) {
    return;
  }
  maskClosable_ = value;
  emit maskClosableChanged(maskClosable_);
}

bool AdModal::keyboard() const { return keyboard_; }

void AdModal::setKeyboard(bool value) {
  if (keyboard_ == value) {
    return;
  }
  keyboard_ = value;
  emit keyboardChanged(keyboard_);
}

bool AdModal::closable() const { return closable_; }

void AdModal::setClosable(bool value) {
  if (closable_ == value) {
    return;
  }
  closable_ = value;
  emit closableChanged(closable_);
  refreshVisibility();
}

bool AdModal::destroyOnHidden() const { return destroyOnHidden_; }

void AdModal::setDestroyOnHidden(bool value) {
  if (destroyOnHidden_ == value) {
    return;
  }
  destroyOnHidden_ = value;
  emit destroyOnHiddenChanged(destroyOnHidden_);
}

bool AdModal::footerVisible() const { return footerVisible_; }

void AdModal::setFooterVisible(bool value) {
  if (footerVisible_ == value) {
    return;
  }
  footerVisible_ = value;
  emit footerVisibleChanged(footerVisible_);
  refreshVisibility();
}

bool AdModal::showCancel() const { return showCancel_; }

void AdModal::setShowCancel(bool value) {
  if (showCancel_ == value) {
    return;
  }
  showCancel_ = value;
  emit showCancelChanged(showCancel_);
  refreshVisibility();
}

bool AdModal::okAutoClose() const { return okAutoClose_; }

void AdModal::setOkAutoClose(bool value) {
  if (okAutoClose_ == value) {
    return;
  }
  okAutoClose_ = value;
  emit okAutoCloseChanged(okAutoClose_);
}

bool AdModal::cancelAutoClose() const { return cancelAutoClose_; }

void AdModal::setCancelAutoClose(bool value) {
  if (cancelAutoClose_ == value) {
    return;
  }
  cancelAutoClose_ = value;
  emit cancelAutoCloseChanged(cancelAutoClose_);
}

bool AdModal::confirmLoading() const { return confirmLoading_; }

void AdModal::setConfirmLoading(bool value) {
  if (confirmLoading_ == value) {
    return;
  }
  confirmLoading_ = value;
  emit confirmLoadingChanged(confirmLoading_);
  refreshTexts();
}

bool AdModal::loading() const { return loading_; }

void AdModal::setLoading(bool value) {
  if (loading_ == value) {
    return;
  }
  loading_ = value;
  emit loadingChanged(loading_);
  refreshTexts();
  refreshVisibility();
}

QString AdModal::titleText() const { return titleText_; }

void AdModal::setTitleText(const QString& value) {
  if (titleText_ == value) {
    return;
  }
  titleText_ = value;
  emit titleTextChanged(titleText_);
  refreshTexts();
  refreshVisibility();
}

QString AdModal::contentText() const { return contentText_; }

void AdModal::setContentText(const QString& value) {
  if (contentText_ == value) {
    return;
  }
  contentText_ = value;
  emit contentTextChanged(contentText_);
  refreshTexts();
}

QString AdModal::okText() const { return okText_; }

void AdModal::setOkText(const QString& value) {
  if (okText_ == value) {
    return;
  }
  okText_ = value;
  emit okTextChanged(okText_);
  refreshTexts();
}

QString AdModal::cancelText() const { return cancelText_; }

void AdModal::setCancelText(const QString& value) {
  if (cancelText_ == value) {
    return;
  }
  cancelText_ = value;
  emit cancelTextChanged(cancelText_);
  refreshTexts();
}

AdButton::Type AdModal::okType() const { return okType_; }

void AdModal::setOkType(AdButton::Type value) {
  if (okType_ == value) {
    return;
  }
  okType_ = value;
  emit okTypeChanged(okType_);
  refreshTexts();
}

AdModal::Type AdModal::type() const { return type_; }

void AdModal::setType(Type value) {
  if (type_ == value) {
    return;
  }
  type_ = value;
  emit typeChanged(type_);
  refreshVisibility();
  applyVisualStyle();
}

QWidget* AdModal::scopeWindow() const { return scopeWindow_; }

void AdModal::setScopeWindow(QWidget* value) {
  if (scopeWindow_ == value) {
    return;
  }

  detachScopeWindowWatcher();
  scopeWindow_ = value;
  attachScopeWindowWatcher(scopeWindow_);
  emit scopeWindowChanged(scopeWindow_);

  if (open_) {
    releaseOverlay();
    ensureOverlay();
    setOpenInternal(true, false, false);
  }
}

QWidget* AdModal::bodyWidget() const { return bodyWidget_; }

void AdModal::setBodyWidget(QWidget* widget) {
  if (bodyWidget_ == widget) {
    return;
  }

  if (bodyWidget_ && bodyLayout_) {
    bodyLayout_->removeWidget(bodyWidget_);
    if (bodyWidget_->parent() == body_) {
      bodyWidget_->setParent(this);
    }
    bodyWidget_->hide();
  }

  bodyWidget_ = widget;
  if (bodyWidget_) {
    if (body_) {
      bodyWidget_->setParent(body_);
      bodyLayout_->insertWidget(0, bodyWidget_);
    } else {
      // Keep custom body widget out of top-level window list before overlay exists.
      bodyWidget_->setParent(this);
      bodyWidget_->hide();
    }
  }

  emit bodyWidgetChanged(bodyWidget_);
  refreshVisibility();
}

QWidget* AdModal::footerWidget() const { return footerWidget_; }

void AdModal::setFooterWidget(QWidget* widget) {
  if (footerWidget_ == widget) {
    return;
  }

  if (footerWidget_ && footerLayout_) {
    footerLayout_->removeWidget(footerWidget_);
    if (footerWidget_->parent() == footer_) {
      footerWidget_->setParent(this);
    }
    footerWidget_->hide();
  }

  footerWidget_ = widget;
  if (footerWidget_) {
    if (footer_) {
      footerWidget_->setParent(footer_);
      footerLayout_->insertWidget(0, footerWidget_);
    } else {
      // Keep custom footer widget out of top-level window list before overlay exists.
      footerWidget_->setParent(this);
      footerWidget_->hide();
    }
  }

  emit footerWidgetChanged(footerWidget_);
  refreshVisibility();
}

AdModal::ActionHandler AdModal::onOkHandler() const { return onOkHandler_; }

void AdModal::setOnOkHandler(ActionHandler handler) { onOkHandler_ = std::move(handler); }

AdModal::ActionHandler AdModal::onCancelHandler() const { return onCancelHandler_; }

void AdModal::setOnCancelHandler(ActionHandler handler) { onCancelHandler_ = std::move(handler); }

AdModal::ComponentTokens AdModal::componentTokens() const { return componentTokens_; }

void AdModal::setComponentTokens(const ComponentTokens& tokens) {
  componentTokens_ = tokens;
  emit componentTokensChanged();
  applyVisualStyle();
}

void AdModal::resetComponentTokens() {
  componentTokens_ = ComponentTokens{};
  emit componentTokensChanged();
  applyVisualStyle();
}

AdModal::SemanticStyles AdModal::semanticStyles() const { return semanticStyles_; }

void AdModal::setSemanticStyles(const SemanticStyles& styles) {
  semanticStyles_ = styles;
  emit semanticStylesChanged();
  applyVisualStyle();
}

void AdModal::setSemanticStyleResolver(SemanticStyleResolver resolver) {
  semanticStyleResolver_ = std::move(resolver);
  emit semanticStylesChanged();
  applyVisualStyle();
}

AdButton* AdModal::okButton() const { return okButton_; }

AdButton* AdModal::cancelButton() const { return cancelButton_; }

QToolButton* AdModal::closeButton() const { return closeButton_; }

void AdModal::destroy() {
  if (staticInstance_) {
    setOpen(false);
    return;
  }
  setOpen(false);
}

AdModal* AdModal::info(const StaticConfig& config, QWidget* scopeWindow) {
  return showStatic(config, Type::Info, scopeWindow);
}

AdModal* AdModal::success(const StaticConfig& config, QWidget* scopeWindow) {
  return showStatic(config, Type::Success, scopeWindow);
}

AdModal* AdModal::error(const StaticConfig& config, QWidget* scopeWindow) {
  return showStatic(config, Type::Error, scopeWindow);
}

AdModal* AdModal::warning(const StaticConfig& config, QWidget* scopeWindow) {
  return showStatic(config, Type::Warning, scopeWindow);
}

AdModal* AdModal::confirm(const StaticConfig& config, QWidget* scopeWindow) {
  return showStatic(config, Type::Confirm, scopeWindow);
}

void AdModal::destroyAll() {
  const QVector<QPointer<AdModal>> modals = staticModals();
  for (const QPointer<AdModal>& modal : modals) {
    if (modal) {
      modal->destroy();
    }
  }
}

bool AdModal::eventFilter(QObject* watched, QEvent* event) {
  if (!watched || !event) {
    return QWidget::eventFilter(watched, event);
  }

  if (watched == overlay_ && event->type() == QEvent::MouseButtonPress && open_) {
    auto* mouseEvent = static_cast<QMouseEvent*>(event);
    if (mouseEvent->button() == Qt::LeftButton && panel_) {
      const QPoint localPos = mouseEvent->position().toPoint();
      if (!panel_->geometry().contains(localPos) && mask_ && maskClosable_) {
        requestCancel();
        return true;
      }
    }
  }

  if (scopeWindow_ && watched == scopeWindow_) {
    switch (event->type()) {
      case QEvent::Resize:
      case QEvent::Move:
      case QEvent::Show:
      case QEvent::WindowStateChange:
      case QEvent::LayoutRequest:
        syncOverlayGeometry();
        if (open_ && overlay_) {
          overlay_->raise();
        }
        break;
      case QEvent::Hide:
        if (open_) {
          setOpen(false);
        }
        break;
      default:
        break;
    }
  }

  if (globalWatcherInstalled_ && open_ && keyboard_ && event->type() == QEvent::KeyPress) {
    auto* keyEvent = static_cast<QKeyEvent*>(event);
    if (keyEvent && keyEvent->key() == Qt::Key_Escape) {
      QWidget* target = qobject_cast<QWidget*>(watched);
      if (scopeWindow_ && target &&
          !(target == scopeWindow_.data() || scopeWindow_->isAncestorOf(target))) {
        return QWidget::eventFilter(watched, event);
      }
      requestCancel();
      keyEvent->accept();
      return true;
    }
  }

  return QWidget::eventFilter(watched, event);
}

void AdModal::changeEvent(QEvent* event) {
  QWidget::changeEvent(event);
  if (!event) {
    return;
  }

  if (event->type() == QEvent::EnabledChange || event->type() == QEvent::FontChange) {
    applyVisualStyle();
  }
}

AdModal* AdModal::showStatic(const StaticConfig& config, Type defaultType, QWidget* scopeWindow) {
  QWidget* resolvedScope = scopeWindow ? scopeWindow : QApplication::activeWindow();
  auto* modal = new AdModal(resolvedScope);
  modal->staticInstance_ = true;
  modal->setDestroyOnHidden(true);
  modal->setCentered(true);
  modal->setMask(true);
  modal->setMaskClosable(false);
  modal->setKeyboard(true);
  modal->setClosable(false);
  modal->setFooterVisible(true);
  modal->setType(defaultType);
  modal->setShowCancel(defaultType == Type::Confirm);
  modal->setWidth(416);
  modal->setOkAutoClose(true);
  modal->setCancelAutoClose(true);
  modal->setScopeWindow(resolvedScope);

  switch (defaultType) {
    case Type::Info:
      modal->setTitleText(QStringLiteral("Information"));
      break;
    case Type::Success:
      modal->setTitleText(QStringLiteral("Success"));
      break;
    case Type::Error:
      modal->setTitleText(QStringLiteral("Error"));
      break;
    case Type::Warning:
      modal->setTitleText(QStringLiteral("Warning"));
      break;
    case Type::Confirm:
      modal->setTitleText(QStringLiteral("Confirm"));
      break;
    case Type::Normal:
      break;
  }

  if (config.type.has_value()) {
    modal->setType(config.type.value());
  }
  if (config.titleText.has_value()) {
    modal->setTitleText(config.titleText.value());
  }
  if (config.contentText.has_value()) {
    modal->setContentText(config.contentText.value());
  }
  if (config.okText.has_value()) {
    modal->setOkText(config.okText.value());
  }
  if (config.cancelText.has_value()) {
    modal->setCancelText(config.cancelText.value());
  }
  if (config.showCancel.has_value()) {
    modal->setShowCancel(config.showCancel.value());
  } else if (modal->type() != Type::Confirm) {
    modal->setShowCancel(false);
  }
  if (config.centered.has_value()) {
    modal->setCentered(config.centered.value());
  }
  if (config.closable.has_value()) {
    modal->setClosable(config.closable.value());
  }
  if (config.mask.has_value()) {
    modal->setMask(config.mask.value());
  }
  if (config.maskClosable.has_value()) {
    modal->setMaskClosable(config.maskClosable.value());
  }
  if (config.keyboard.has_value()) {
    modal->setKeyboard(config.keyboard.value());
  }
  if (config.footerVisible.has_value()) {
    modal->setFooterVisible(config.footerVisible.value());
  }
  if (config.confirmLoading.has_value()) {
    modal->setConfirmLoading(config.confirmLoading.value());
  }
  if (config.loading.has_value()) {
    modal->setLoading(config.loading.value());
  }
  if (config.width.has_value()) {
    modal->setWidth(config.width.value());
  }
  if (config.top.has_value()) {
    modal->setTop(config.top.value());
  }
  if (config.okType.has_value()) {
    modal->setOkType(config.okType.value());
  }

  modal->setOnOkHandler(config.onOk);
  modal->setOnCancelHandler(config.onCancel);

  registerStaticModal(modal);
  modal->setOpen(true);
  return modal;
}

void AdModal::registerStaticModal(AdModal* modal) {
  if (!modal) {
    return;
  }
  auto& modals = staticModals();
  for (int i = modals.size() - 1; i >= 0; --i) {
    if (!modals.at(i) || modals.at(i).data() == modal) {
      modals.removeAt(i);
    }
  }
  modals.append(modal);
}

void AdModal::unregisterStaticModal(AdModal* modal) {
  if (!modal) {
    return;
  }
  auto& modals = staticModals();
  for (int i = modals.size() - 1; i >= 0; --i) {
    if (!modals.at(i) || modals.at(i).data() == modal) {
      modals.removeAt(i);
    }
  }
}

void AdModal::attachScopeWindowWatcher(QWidget* scope) {
  if (!scope) {
    return;
  }
  scope->removeEventFilter(this);
  scope->installEventFilter(this);
}

void AdModal::detachScopeWindowWatcher() {
  if (!scopeWindow_) {
    return;
  }
  scopeWindow_->removeEventFilter(this);
}

void AdModal::installGlobalWatcher(bool enabled) {
  if (!qApp) {
    return;
  }
  if (enabled == globalWatcherInstalled_) {
    return;
  }

  if (enabled) {
    qApp->installEventFilter(this);
  } else {
    qApp->removeEventFilter(this);
  }
  globalWatcherInstalled_ = enabled;
}

QWidget* AdModal::resolveScopeWindow() const {
  if (scopeWindow_) {
    return scopeWindow_;
  }
  if (parentWidget()) {
    if (QWidget* parentWindow = parentWidget()->window()) {
      return parentWindow;
    }
  }
  if (QWidget* active = QApplication::activeWindow()) {
    return active;
  }
  const QWidgetList topLevels = QApplication::topLevelWidgets();
  for (QWidget* widget : topLevels) {
    if (widget && widget->isVisible()) {
      return widget;
    }
  }
  return nullptr;
}

void AdModal::ensureOverlay() {
  if (overlay_) {
    syncOverlayGeometry();
    return;
  }

  QWidget* scope = resolveScopeWindow();
  if (!scope) {
    return;
  }

  if (scopeWindow_ != scope) {
    detachScopeWindowWatcher();
    scopeWindow_ = scope;
    attachScopeWindowWatcher(scopeWindow_);
    emit scopeWindowChanged(scopeWindow_);
  }

  auto* overlay = new ModalOverlayWidget(scopeWindow_);
  overlay->setObjectName(QStringLiteral("ad-modal-overlay"));
  overlay->setProperty("adqt.interaction.surface", true);
  overlay->setGeometry(scopeWindow_->rect());
  overlay->hide();
  overlay->installEventFilter(this);

  auto* overlayLayout = new QVBoxLayout(overlay);
  overlayLayout->setContentsMargins(16, top_, 16, 16);
  overlayLayout->setSpacing(0);

  auto* panel = new ModalPanelWidget(overlay);
  panel->setObjectName(QStringLiteral("ad-modal-panel"));
  panel->setFrameShape(QFrame::NoFrame);
  panel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Maximum);

  auto* panelLayout = new QVBoxLayout(panel);
  panelLayout->setContentsMargins(24, 20, 24, 20);
  panelLayout->setSpacing(0);

  auto* header = new QWidget(panel);
  header->setObjectName(QStringLiteral("ad-modal-header"));
  header->setAttribute(Qt::WA_StyledBackground, false);
  header->setAutoFillBackground(false);
  auto* headerLayout = new QHBoxLayout(header);
  headerLayout->setContentsMargins(0, 0, 0, 8);
  headerLayout->setSpacing(0);

  auto* titleLabel = new QLabel(header);
  titleLabel->setObjectName(QStringLiteral("ad-modal-title"));
  titleLabel->setWordWrap(true);
  titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

  auto* closeButton = new QToolButton(header);
  closeButton->setObjectName(QStringLiteral("ad-modal-close"));
  closeButton->setAutoRaise(true);
  closeButton->setCursor(Qt::PointingHandCursor);
  closeButton->setFocusPolicy(Qt::NoFocus);
  connect(closeButton, &QToolButton::clicked, this, [this]() { requestCancel(); });

  headerLayout->addWidget(titleLabel, 1, Qt::AlignVCenter);
  headerLayout->addWidget(closeButton, 0, Qt::AlignTop);

  auto* body = new QWidget(panel);
  body->setObjectName(QStringLiteral("ad-modal-body"));
  body->setAttribute(Qt::WA_StyledBackground, false);
  body->setAutoFillBackground(false);
  auto* bodyLayout = new QVBoxLayout(body);
  bodyLayout->setContentsMargins(0, 0, 0, 0);
  bodyLayout->setSpacing(0);

  auto* confirmBodyHost = new QWidget(body);
  confirmBodyHost->setObjectName(QStringLiteral("ad-modal-confirm-body"));
  confirmBodyHost->setAttribute(Qt::WA_StyledBackground, false);
  confirmBodyHost->setAutoFillBackground(false);
  auto* confirmBodyLayout = new QHBoxLayout(confirmBodyHost);
  confirmBodyLayout->setContentsMargins(0, 0, 0, 0);
  confirmBodyLayout->setSpacing(0);

  auto* titleIconLabel = new QLabel(confirmBodyHost);
  titleIconLabel->setObjectName(QStringLiteral("ad-modal-title-icon"));
  titleIconLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
  titleIconLabel->hide();

  auto* confirmParagraph = new QWidget(confirmBodyHost);
  confirmParagraph->setObjectName(QStringLiteral("ad-modal-confirm-paragraph"));
  auto* confirmParagraphLayout = new QVBoxLayout(confirmParagraph);
  confirmParagraphLayout->setContentsMargins(0, 0, 0, 0);
  confirmParagraphLayout->setSpacing(0);

  auto* confirmTitleLabel = new QLabel(confirmParagraph);
  confirmTitleLabel->setObjectName(QStringLiteral("ad-modal-confirm-title"));
  confirmTitleLabel->setWordWrap(true);
  confirmTitleLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);

  auto* confirmContentLabel = new QLabel(confirmParagraph);
  confirmContentLabel->setObjectName(QStringLiteral("ad-modal-confirm-content"));
  confirmContentLabel->setWordWrap(true);
  confirmContentLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);

  confirmParagraphLayout->addWidget(confirmTitleLabel);
  confirmParagraphLayout->addWidget(confirmContentLabel);

  confirmBodyLayout->addWidget(titleIconLabel, 0, Qt::AlignTop);
  confirmBodyLayout->addWidget(confirmParagraph, 1, Qt::AlignTop);

  auto* contentLabel = new QLabel(body);
  contentLabel->setObjectName(QStringLiteral("ad-modal-content"));
  contentLabel->setWordWrap(true);
  contentLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
  bodyLayout->addWidget(confirmBodyHost);
  bodyLayout->addWidget(contentLabel);

  auto* footer = new QWidget(panel);
  footer->setObjectName(QStringLiteral("ad-modal-footer"));
  footer->setAttribute(Qt::WA_StyledBackground, false);
  footer->setAutoFillBackground(false);
  auto* footerLayout = new QHBoxLayout(footer);
  footerLayout->setContentsMargins(0, 12, 0, 0);
  footerLayout->setSpacing(0);

  auto* footerButtonsHost = new QWidget(footer);
  auto* footerButtonsLayout = new QHBoxLayout(footerButtonsHost);
  footerButtonsLayout->setContentsMargins(0, 0, 0, 0);
  footerButtonsLayout->setSpacing(8);

  auto* cancelButton = new AdButton(QStringLiteral("Cancel"), footerButtonsHost);
  cancelButton->setType(AdButton::Type::Default);
  auto* okButton = new AdButton(QStringLiteral("OK"), footerButtonsHost);
  okButton->setType(okType_);

  connect(cancelButton, &QPushButton::clicked, this, [this]() { requestCancel(); });
  connect(okButton, &QPushButton::clicked, this, [this]() { requestOk(); });

  footerButtonsLayout->addStretch();
  footerButtonsLayout->addWidget(cancelButton);
  footerButtonsLayout->addWidget(okButton);
  footerLayout->addWidget(footerButtonsHost);

  panelLayout->addWidget(header);
  panelLayout->addWidget(body);
  panelLayout->addWidget(footer);

  overlayLayout->addWidget(panel, 0, Qt::AlignTop | Qt::AlignHCenter);

  auto* escShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), overlay);
  escShortcut->setContext(Qt::WidgetWithChildrenShortcut);
  connect(escShortcut, &QShortcut::activated, this, [this]() {
    if (open_ && keyboard_) {
      requestCancel();
    }
  });

  overlay_ = overlay;
  overlayLayout_ = overlayLayout;
  panel_ = panel;
  panelLayout_ = panelLayout;
  header_ = header;
  headerLayout_ = headerLayout;
  titleIconLabel_ = titleIconLabel;
  titleLabel_ = titleLabel;
  closeButton_ = closeButton;
  body_ = body;
  bodyLayout_ = bodyLayout;
  confirmBodyHost_ = confirmBodyHost;
  confirmBodyLayout_ = confirmBodyLayout;
  confirmParagraphLayout_ = confirmParagraphLayout;
  contentLabel_ = contentLabel;
  confirmTitleLabel_ = confirmTitleLabel;
  confirmContentLabel_ = confirmContentLabel;
  footer_ = footer;
  footerLayout_ = footerLayout;
  footerButtonsHost_ = footerButtonsHost;
  footerButtonsLayout_ = footerButtonsLayout;
  cancelButton_ = cancelButton;
  okButton_ = okButton;
  escShortcut_ = escShortcut;

  if (bodyWidget_) {
    bodyWidget_->setParent(body_);
    bodyLayout_->insertWidget(0, bodyWidget_);
  }
  if (footerWidget_) {
    footerWidget_->setParent(footer_);
    footerLayout_->insertWidget(0, footerWidget_);
  }

  refreshTexts();
  refreshVisibility();
  refreshLayout();
  applyVisualStyle();
  syncOverlayGeometry();
}

void AdModal::releaseOverlay() {
  if (!overlay_) {
    return;
  }

  if (bodyWidget_ && bodyWidget_->parent() == body_) {
    if (bodyLayout_) {
      bodyLayout_->removeWidget(bodyWidget_);
    }
    bodyWidget_->setParent(this);
    bodyWidget_->hide();
  }

  if (footerWidget_ && footerWidget_->parent() == footer_) {
    if (footerLayout_) {
      footerLayout_->removeWidget(footerWidget_);
    }
    footerWidget_->setParent(this);
    footerWidget_->hide();
  }

  overlay_->removeEventFilter(this);
  overlay_->deleteLater();

  overlay_.clear();
  overlayLayout_.clear();
  panel_.clear();
  panelLayout_.clear();
  header_.clear();
  headerLayout_.clear();
  titleIconLabel_.clear();
  titleLabel_.clear();
  closeButton_.clear();
  body_.clear();
  bodyLayout_.clear();
  confirmBodyHost_.clear();
  confirmBodyLayout_.clear();
  confirmParagraphLayout_.clear();
  contentLabel_.clear();
  confirmTitleLabel_.clear();
  confirmContentLabel_.clear();
  footer_.clear();
  footerLayout_.clear();
  footerButtonsHost_.clear();
  footerButtonsLayout_.clear();
  cancelButton_.clear();
  okButton_.clear();
  escShortcut_.clear();
}

void AdModal::syncOverlayGeometry() {
  if (!overlay_ || !scopeWindow_) {
    return;
  }
  const QRect rect = scopeWindow_->rect();
  if (overlay_->geometry() != rect) {
    overlay_->setGeometry(rect);
  }
}

void AdModal::refreshLayout() {
  if (!overlayLayout_ || !panel_) {
    return;
  }

  if (centered_) {
    overlayLayout_->setContentsMargins(16, 16, 16, 16);
    overlayLayout_->setAlignment(panel_, Qt::AlignCenter);
  } else {
    overlayLayout_->setContentsMargins(16, std::max(0, top_), 16, 16);
    overlayLayout_->setAlignment(panel_, Qt::AlignTop | Qt::AlignHCenter);
  }
}

void AdModal::refreshTexts() {
  const QString resolvedContent = loading_ ? QStringLiteral("Loading...") : contentText_;

  if (titleLabel_) {
    titleLabel_->setText(titleText_);
  }
  if (confirmTitleLabel_) {
    confirmTitleLabel_->setText(titleText_);
  }

  if (contentLabel_) {
    contentLabel_->setText(resolvedContent);
  }
  if (confirmContentLabel_) {
    confirmContentLabel_->setText(resolvedContent);
  }

  if (okButton_) {
    okButton_->setText(okText_.trimmed().isEmpty() ? QStringLiteral("OK") : okText_);
    okButton_->setType(okType_);
    okButton_->setLoading(confirmLoading_);
  }

  if (cancelButton_) {
    cancelButton_->setText(cancelText_.trimmed().isEmpty() ? QStringLiteral("Cancel") : cancelText_);
  }
}

void AdModal::refreshVisibility() {
  const bool hasTitle = !titleText_.trimmed().isEmpty();
  const bool confirmMode = type_ != Type::Normal;
  const bool showTextContent = loading_ || !bodyWidget_;

  if (header_) {
    const bool showHeader = confirmMode ? closable_ : (hasTitle || closable_);
    header_->setVisible(showHeader);
  }

  if (titleLabel_) {
    titleLabel_->setVisible(!confirmMode && hasTitle);
  }

  if (closeButton_) {
    closeButton_->setVisible(closable_);
  }

  if (confirmBodyHost_) {
    confirmBodyHost_->setVisible(confirmMode && showTextContent);
  }
  if (confirmTitleLabel_) {
    confirmTitleLabel_->setVisible(confirmMode && hasTitle && showTextContent);
  }
  if (titleIconLabel_) {
    titleIconLabel_->setVisible(confirmMode && showTextContent);
  }

  if (bodyWidget_) {
    bodyWidget_->setVisible(!loading_);
  }

  if (contentLabel_) {
    contentLabel_->setVisible(!confirmMode && showTextContent);
  }
  if (confirmContentLabel_) {
    confirmContentLabel_->setVisible(confirmMode && showTextContent);
  }

  if (footer_) {
    footer_->setVisible(footerVisible_);
  }

  if (footerButtonsHost_) {
    footerButtonsHost_->setVisible(!footerWidget_);
  }

  if (footerWidget_) {
    footerWidget_->setVisible(true);
  }

  if (cancelButton_) {
    cancelButton_->setVisible(showCancel_);
  }

  refreshTitleIcon();
}

void AdModal::refreshTitleIcon() {
  if (!titleIconLabel_) {
    return;
  }

  if (type_ == Type::Normal) {
    titleIconLabel_->clear();
    titleIconLabel_->hide();
    return;
  }

  const VisualStyle style = resolveVisualStyle();
  adqt::icons::IconStyle iconStyle;
  iconStyle.primary = style.iconColor;
  iconStyle.hasPrimary = true;

  adqt::icons::IconToken token;
  switch (type_) {
    case Type::Info:
      token = filled_icons::InfoCircle(iconStyle);
      break;
    case Type::Success:
      token = filled_icons::CheckCircle(iconStyle);
      break;
    case Type::Error:
      token = filled_icons::CloseCircle(iconStyle);
      break;
    case Type::Warning:
    case Type::Confirm:
      token = filled_icons::ExclamationCircle(iconStyle);
      break;
    case Type::Normal:
      return;
  }

  const int iconSize = std::max(12, style.iconSize);
  const bool hasTitle = !titleText_.trimmed().isEmpty();
  const int anchorHeight = hasTitle ? style.titleLineHeight : style.textLineHeight;
  const int iconOffsetTop = std::max(0, static_cast<int>(std::round((anchorHeight - iconSize) / 2.0)));
  titleIconLabel_->setContentsMargins(0, iconOffsetTop, 0, 0);
  titleIconLabel_->setFixedSize(iconSize, iconSize + iconOffsetTop);
  titleIconLabel_->setPixmap(
      adqt::icons::renderIconPixmap(token, QSize(iconSize, iconSize), std::max(1.0, devicePixelRatioF())));
  titleIconLabel_->show();
}

void AdModal::applyVisualStyle() {
  if (!overlay_) {
    return;
  }

  const VisualStyle style = resolveVisualStyle();

  auto* overlayWidget = static_cast<ModalOverlayWidget*>(overlay_.data());
  if (overlayWidget) {
    overlayWidget->setRootColor(style.rootBg);
    overlayWidget->setMaskEnabled(mask_);
    overlayWidget->setMaskColor(style.maskBg);
  }

  if (panel_) {
    panel_->setFixedWidth(std::max(240, style.width));
    panel_->setStyleSheet(QStringLiteral(
                              "#ad-modal-panel {"
                              "  background: transparent;"
                              "  border: none;"
                              "}"
                              "#ad-modal-header {"
                              "  background: transparent;"
                              "}"
                              "#ad-modal-body {"
                              "  background: transparent;"
                              "}"
                              "#ad-modal-footer {"
                              "  background: transparent;"
                              "  border-top: none;"
                              "}"));

    auto* panelWidget = static_cast<ModalPanelWidget*>(panel_.data());
    if (panelWidget) {
      ModalPanelWidget::PaintStyle paintStyle;
      paintStyle.containerBg = style.containerBg;
      paintStyle.headerBg = style.headerBg;
      paintStyle.bodyBg = style.bodyBg;
      paintStyle.footerBg = style.footerBg;
      paintStyle.borderColor = style.borderColor;
      paintStyle.borderRadius = style.borderRadius;
      paintStyle.borderWidth = style.borderWidth;
      paintStyle.footerBorderTopWidth = style.footerBorderTopWidth;
      panelWidget->setSectionWidgets(header_, body_, footer_);
      panelWidget->setPaintStyle(paintStyle);
    }
  }

  if (panelLayout_) {
    panelLayout_->setContentsMargins(style.contentPaddingHorizontal, style.contentPaddingVertical,
                                     style.contentPaddingHorizontal, style.contentPaddingVertical);
  }
  if (headerLayout_) {
    headerLayout_->setContentsMargins(style.headerPaddingHorizontal, style.headerPaddingVertical,
                                      style.headerPaddingHorizontal,
                                      style.headerPaddingVertical + style.headerMarginBottom);
  }
  if (bodyLayout_) {
    bodyLayout_->setContentsMargins(style.bodyPaddingHorizontal, style.bodyPaddingVertical,
                                    style.bodyPaddingHorizontal, style.bodyPaddingVertical);
  }
  if (confirmBodyLayout_) {
    confirmBodyLayout_->setSpacing(style.confirmIconGap);
  }
  if (confirmParagraphLayout_) {
    confirmParagraphLayout_->setSpacing(style.confirmParagraphGap);
  }
  if (footerLayout_) {
    footerLayout_->setContentsMargins(style.footerPaddingHorizontal,
                                      style.footerPaddingVertical + style.footerMarginTop,
                                      style.footerPaddingHorizontal, style.footerPaddingVertical);
  }
  if (footerButtonsLayout_) {
    footerButtonsLayout_->setSpacing(style.footerButtonGap);
  }

  if (titleLabel_) {
    titleLabel_->setFont(style.titleFont);
    QPalette palette = titleLabel_->palette();
    palette.setColor(QPalette::WindowText, style.titleColor);
    titleLabel_->setPalette(palette);
  }
  if (contentLabel_) {
    contentLabel_->setFont(style.bodyFont);
    QPalette palette = contentLabel_->palette();
    palette.setColor(QPalette::WindowText, style.bodyColor);
    contentLabel_->setPalette(palette);
  }
  if (confirmTitleLabel_) {
    confirmTitleLabel_->setFont(style.titleFont);
    QPalette palette = confirmTitleLabel_->palette();
    palette.setColor(QPalette::WindowText, style.titleColor);
    confirmTitleLabel_->setPalette(palette);
  }
  if (confirmContentLabel_) {
    confirmContentLabel_->setFont(style.bodyFont);
    QPalette palette = confirmContentLabel_->palette();
    palette.setColor(QPalette::WindowText, style.bodyColor);
    confirmContentLabel_->setPalette(palette);
  }

  if (closeButton_) {
    adqt::icons::IconStyle iconStyle;
    iconStyle.primary = style.closeIconColor;
    iconStyle.hasPrimary = true;
    const int closeIconSize = std::max(10, style.closeIconSize);
    closeButton_->setFixedSize(std::max(closeIconSize, style.closeButtonSize),
                               std::max(closeIconSize, style.closeButtonSize));
    closeButton_->setIcon(QIcon(adqt::icons::renderIconPixmap(outlined_icons::Close(iconStyle),
                                                              QSize(closeIconSize, closeIconSize),
                                                              std::max(1.0, devicePixelRatioF()))));
    closeButton_->setIconSize(QSize(closeIconSize, closeIconSize));
    closeButton_->setEnabled(isEnabled() && !confirmLoading_);
    closeButton_->setStyleSheet(QStringLiteral(
                                    "QToolButton#ad-modal-close {"
                                    "  border: none;"
                                    "  background: transparent;"
                                    "  padding: 0px;"
                                    "}"
                                    "QToolButton#ad-modal-close:hover {"
                                    "  background: rgba(0, 0, 0, 20);"
                                    "  border-radius: 4px;"
                                    "}"));
  }

  if (cancelButton_) {
    cancelButton_->setEnabled(isEnabled() && !confirmLoading_);
  }
  if (okButton_) {
    okButton_->setEnabled(isEnabled());
  }

  refreshTitleIcon();
  refreshLayout();
}

AdModal::VisualStyle AdModal::resolveVisualStyle() const {
  const adqt::theme::ThemeManager& themeManager = adqt::theme::ThemeManager::instance();
  const adqt::theme::ThemeMapToken& map = themeManager.currentMapToken();
  const adqt::theme::ThemeSeedToken& seed = themeManager.currentConfig().seed;

  VisualStyle style;
  style.rootBg = QColor(0, 0, 0, 0);
  style.maskBg = toColor(map.colorBgMask, QColor(0, 0, 0, 115));
  style.containerBg = toColor(map.colorBgElevated, QColor("#ffffff"));
  const bool wireframe = seed.wireframe;
  style.headerBg = wireframe ? style.containerBg : QColor(0, 0, 0, 0);
  style.bodyBg = wireframe ? style.containerBg : QColor(0, 0, 0, 0);
  style.footerBg = wireframe ? style.containerBg : QColor(0, 0, 0, 0);
  style.borderColor = toColor(map.colorBorderSecondary, QColor("#f0f0f0"));
  style.titleColor = toColor(map.colorText, QColor("#141414"));
  style.bodyColor = toColor(map.colorText, QColor("#141414"));
  style.closeIconColor = toColor(map.colorTextTertiary, QColor("#8c8c8c"));
  style.width = width_;
  style.zIndex = static_cast<int>(std::round(seed.zIndexPopupBase));
  style.borderRadius = std::max(0, qRound(map.borderRadiusLG));
  style.borderWidth = 0;
  style.contentPaddingHorizontal = wireframe ? 0 : std::max(0, qRound(map.sizeLG));
  style.contentPaddingVertical = wireframe ? 0 : std::max(0, qRound(map.sizeMD));
  style.headerPaddingHorizontal = wireframe ? std::max(0, qRound(map.sizeLG)) : 0;
  style.headerPaddingVertical = wireframe ? std::max(0, qRound(map.size)) : 0;
  style.headerMarginBottom = wireframe ? 0 : std::max(0, qRound(map.sizeXS));
  style.bodyPaddingHorizontal = wireframe ? std::max(0, qRound(map.sizeLG)) : 0;
  style.bodyPaddingVertical = wireframe ? std::max(0, qRound(map.sizeLG)) : 0;
  style.footerPaddingHorizontal = wireframe ? std::max(0, qRound(map.size)) : 0;
  style.footerPaddingVertical = wireframe ? std::max(0, qRound(map.sizeXS)) : 0;
  style.footerMarginTop = wireframe ? 0 : std::max(0, qRound(map.sizeSM));
  style.footerBorderTopWidth = wireframe ? std::max(0, qRound(map.lineWidth)) : 0;
  style.footerButtonGap = std::max(4, qRound(map.sizeXS));
  style.confirmIconGap = wireframe ? std::max(0, qRound(map.size)) : std::max(0, qRound(map.sizeSM));
  style.confirmParagraphGap = std::max(0, qRound(map.sizeXS));
  style.textLineHeight = std::max(0, qRound(map.fontHeight));
  style.titleLineHeight = std::max(0, qRound(map.fontSizeHeading5 * map.lineHeightHeading5));
  style.iconSize = std::max(12, qRound(map.fontHeight));
  style.closeButtonSize = std::max(24, qRound(map.controlHeight));
  style.closeIconSize = std::max(12, qRound(map.fontSizeLG));
  style.titleFont = font();
  style.titleFont.setPixelSize(std::max(12, qRound(map.fontSizeHeading5)));
  style.titleFont.setWeight(QFont::DemiBold);
  style.bodyFont = font();
  style.bodyFont.setPixelSize(std::max(12, qRound(map.fontSize)));
  style.bodyFont.setWeight(QFont::Normal);

  switch (type_) {
    case Type::Info:
      style.iconColor = toColor(map.colorInfo, QColor("#1677ff"));
      break;
    case Type::Success:
      style.iconColor = toColor(map.colorSuccess, QColor("#52c41a"));
      break;
    case Type::Error:
      style.iconColor = toColor(map.colorError, QColor("#ff4d4f"));
      break;
    case Type::Warning:
    case Type::Confirm:
      style.iconColor = toColor(map.colorWarning, QColor("#faad14"));
      break;
    case Type::Normal:
      style.iconColor = toColor(map.colorInfo, QColor("#1677ff"));
      break;
  }

  if (componentTokens_.width.has_value()) {
    style.width = std::max(240, componentTokens_.width.value());
  }
  if (componentTokens_.zIndexPopup.has_value()) {
    style.zIndex = std::max(0, componentTokens_.zIndexPopup.value());
  }
  if (componentTokens_.borderRadius.has_value()) {
    style.borderRadius = std::max(0, componentTokens_.borderRadius.value());
  }
  if (componentTokens_.borderWidth.has_value()) {
    style.borderWidth = std::max(0, componentTokens_.borderWidth.value());
  }
  if (componentTokens_.headerPaddingHorizontal.has_value()) {
    style.headerPaddingHorizontal = std::max(0, componentTokens_.headerPaddingHorizontal.value());
  }
  if (componentTokens_.headerPaddingVertical.has_value()) {
    style.headerPaddingVertical = std::max(0, componentTokens_.headerPaddingVertical.value());
  }
  if (componentTokens_.bodyPaddingHorizontal.has_value()) {
    style.bodyPaddingHorizontal = std::max(0, componentTokens_.bodyPaddingHorizontal.value());
  }
  if (componentTokens_.bodyPaddingVertical.has_value()) {
    style.bodyPaddingVertical = std::max(0, componentTokens_.bodyPaddingVertical.value());
  }
  if (componentTokens_.footerPaddingHorizontal.has_value()) {
    style.footerPaddingHorizontal = std::max(0, componentTokens_.footerPaddingHorizontal.value());
  }
  if (componentTokens_.footerPaddingVertical.has_value()) {
    style.footerPaddingVertical = std::max(0, componentTokens_.footerPaddingVertical.value());
  }
  if (componentTokens_.footerButtonGap.has_value()) {
    style.footerButtonGap = std::max(0, componentTokens_.footerButtonGap.value());
  }
  if (componentTokens_.iconSize.has_value()) {
    style.iconSize = std::max(10, componentTokens_.iconSize.value());
  }

  style.maskBg = parseColorToken(componentTokens_.maskBg, style.maskBg);
  style.containerBg = parseColorToken(componentTokens_.contentBg, style.containerBg);
  style.headerBg = parseColorToken(componentTokens_.headerBg, style.headerBg);
  style.bodyBg = parseColorToken(componentTokens_.bodyBg, style.bodyBg);
  style.footerBg = parseColorToken(componentTokens_.footerBg, style.footerBg);
  style.borderColor = parseColorToken(componentTokens_.borderColor, style.borderColor);
  style.titleColor = parseColorToken(componentTokens_.titleColor, style.titleColor);
  style.bodyColor = parseColorToken(componentTokens_.bodyColor, style.bodyColor);
  style.iconColor = parseColorToken(componentTokens_.iconColor, style.iconColor);
  style.closeIconColor = parseColorToken(componentTokens_.closeIconColor, style.closeIconColor);

  StyleContext context;
  context.open = open_;
  context.centered = centered_;
  context.loading = loading_;
  context.confirmLoading = confirmLoading_;
  context.mask = mask_;
  context.closable = closable_;
  context.showCancel = showCancel_;
  context.type = type_;

  const SemanticStyles effectiveStyles =
      semanticStyleResolver_ ? semanticStyleResolver_(context) : semanticStyles_;
  applySemanticSlot(effectiveStyles.root, nullptr, &style.rootBg, &style.borderColor);
  applySemanticSlot(effectiveStyles.mask, nullptr, &style.maskBg, nullptr);
  applySemanticSlot(effectiveStyles.container, nullptr, &style.containerBg, &style.borderColor);
  applySemanticSlot(effectiveStyles.header, nullptr, &style.headerBg, &style.borderColor);
  applySemanticSlot(effectiveStyles.title, &style.titleColor, nullptr, nullptr);
  applySemanticSlot(effectiveStyles.body, &style.bodyColor, &style.bodyBg, nullptr);
  applySemanticSlot(effectiveStyles.footer, nullptr, &style.footerBg, &style.borderColor);
  applySemanticSlot(effectiveStyles.icon, &style.iconColor, nullptr, nullptr);
  applySemanticSlot(effectiveStyles.close, &style.closeIconColor, nullptr, nullptr);

  return style;
}

void AdModal::setOpenInternal(bool value, bool emitSignal, bool emitOnOpenSignal) {
  if (internalOpenUpdate_) {
    return;
  }
  internalOpenUpdate_ = true;

  if (open_ == value) {
    if (open_) {
      ensureOverlay();
      refreshTexts();
      refreshVisibility();
      refreshLayout();
      applyVisualStyle();
      syncOverlayGeometry();
      if (overlay_) {
        overlay_->show();
        overlay_->raise();
        overlay_->setFocus(Qt::PopupFocusReason);
      }
    }
    internalOpenUpdate_ = false;
    return;
  }

  open_ = value;
  if (open_) {
    ensureOverlay();
    installGlobalWatcher(true);
    refreshTexts();
    refreshVisibility();
    refreshLayout();
    applyVisualStyle();
    syncOverlayGeometry();
    if (overlay_) {
      overlay_->show();
      overlay_->raise();
      overlay_->setFocus(Qt::PopupFocusReason);
    }
  } else {
    installGlobalWatcher(false);
    if (overlay_) {
      overlay_->hide();
    }
    if (destroyOnHidden_) {
      releaseOverlay();
    }
  }

  if (emitSignal) {
    emit openChanged(open_);
    if (emitOnOpenSignal) {
      emit onOpenChange(open_);
    }
  }
  emit afterOpenChange(open_);
  if (!open_) {
    emit afterClose();
  }

  internalOpenUpdate_ = false;

  if (!open_ && staticInstance_) {
    unregisterStaticModal(this);
    deleteLater();
  }
}

void AdModal::requestOk() {
  if (confirmLoading_) {
    return;
  }

  bool shouldClose = okAutoClose_;
  if (onOkHandler_) {
    shouldClose = onOkHandler_(this);
  }
  emit okTriggered();
  if (shouldClose) {
    setOpen(false);
  }
}

void AdModal::requestCancel() {
  if (confirmLoading_) {
    return;
  }

  bool shouldClose = cancelAutoClose_;
  if (onCancelHandler_) {
    shouldClose = onCancelHandler_(this);
  }
  emit cancelTriggered();
  if (shouldClose) {
    setOpen(false);
  }
}

QColor AdModal::parseColorToken(const std::optional<QString>& token, const QColor& fallback) {
  if (!token.has_value()) {
    return fallback;
  }
  return toColor(token.value(), fallback);
}

QString AdModal::toRgba(const QColor& color) {
  return QStringLiteral("rgba(%1, %2, %3, %4)")
      .arg(color.red())
      .arg(color.green())
      .arg(color.blue())
      .arg(color.alpha());
}

void AdModal::applySemanticSlot(const SemanticSlotStyle& slot,
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

}  // namespace adqt::widgets
