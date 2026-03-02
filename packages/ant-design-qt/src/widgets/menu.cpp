#include "menu.h"

#include "menu_style.h"
#include "theme/theme.h"

#include <QApplication>
#include <QCursor>
#include <QEvent>
#include <QFocusEvent>
#include <QFontMetrics>
#include <QHash>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QPointer>
#include <QStyle>
#include <QToolTip>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

namespace adqt::widgets {

namespace {

using detail::MenuStyleInput;
using detail::MenuVisualStyle;

QPoint mouseEventPos(const QMouseEvent* event) {
  if (!event) {
    return QPoint();
  }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  return event->position().toPoint();
#else
  return event->pos();
#endif
}

QPoint mouseEventGlobalPos(const QMouseEvent* event) {
  if (!event) {
    return QPoint();
  }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  return event->globalPosition().toPoint();
#else
  return event->globalPos();
#endif
}

QRect widgetGlobalRect(const QWidget* widget) {
  if (!widget) {
    return QRect();
  }
  return QRect(widget->mapToGlobal(QPoint(0, 0)), widget->size());
}

bool widgetContainsGlobalPos(const QWidget* widget, const QPoint& globalPos) {
  const QRect globalRect = widgetGlobalRect(widget);
  return globalRect.isValid() && globalRect.contains(globalPos);
}

QString trimmedOrFallback(const QString& value, const QString& fallback) {
  const QString trimmed = value.trimmed();
  return trimmed.isEmpty() ? fallback : trimmed;
}

QString normalizedKeyPart(const QString& key) {
  return key.trimmed();
}

QString autoItemKey(const QString& prefix, int index) {
  return QStringLiteral("%1%2").arg(prefix).arg(index);
}

QStringList uniqueStringList(const QStringList& values) {
  QStringList out;
  out.reserve(values.size());
  QSet<QString> seen;
  for (const QString& value : values) {
    if (value.isEmpty() || seen.contains(value)) {
      continue;
    }
    seen.insert(value);
    out.append(value);
  }
  return out;
}

void drawDefaultExpandIndicator(QPainter& painter,
                                const QRect& area,
                                bool pointDown,
                                const QColor& color) {
  if (!area.isValid()) {
    return;
  }

  const int side = std::max(6, std::min(area.width(), area.height()) - 2);
  const QRect iconRect(area.center().x() - side / 2, area.center().y() - side / 2, side, side);

  QPainterPath path;
  if (pointDown) {
    path.moveTo(iconRect.left(), iconRect.top() + side / 3.0);
    path.lineTo(iconRect.center().x(), iconRect.bottom() - side / 4.0);
    path.lineTo(iconRect.right(), iconRect.top() + side / 3.0);
  } else {
    path.moveTo(iconRect.left() + side / 3.0, iconRect.top());
    path.lineTo(iconRect.right() - side / 4.0, iconRect.center().y());
    path.lineTo(iconRect.left() + side / 3.0, iconRect.bottom());
  }

  QPen pen(color, 1.4);
  pen.setCapStyle(Qt::RoundCap);
  pen.setJoinStyle(Qt::RoundJoin);
  painter.save();
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.setBrush(Qt::NoBrush);
  painter.setPen(pen);
  painter.drawPath(path);
  painter.restore();
}

MenuVisualStyle resolveVisualStyle(const AdMenu* menu, const AdMenu::SemanticStyles& semantic) {
  MenuStyleInput input;
  input.mode = menu->mode();
  input.theme = menu->theme();
  input.inlineCollapsed = menu->inlineCollapsed();
  input.baseFont = menu->font();
  input.componentTokens = menu->componentTokens();
  input.semanticStyles = semantic;
  return detail::resolveMenuVisualStyle(input);
}

struct SharedPopupHost {
  QPointer<QWidget> scopeWindow;
  QPointer<QWidget> popupWindow;
  QPointer<AdMenu> popupMenu;
  QPointer<QWidget> renderedRoot;
  QPointer<AdMenu> ownerMenu;
  QString activeKey;
};

QHash<QWidget*, SharedPopupHost*>& sharedPopupHosts() {
  static QHash<QWidget*, SharedPopupHost*> hosts;
  return hosts;
}

QWidget* popupScopeWindowFor(const AdMenu* menu) {
  if (!menu) {
    return nullptr;
  }
  QWidget* scopeWindow = menu->window();
  return scopeWindow ? scopeWindow : const_cast<AdMenu*>(menu);
}

void detachSharedPopupOwner(SharedPopupHost* host) {
  if (!host || !host->ownerMenu) {
    return;
  }

  AdMenu* owner = host->ownerMenu.data();
  if (host->scopeWindow) {
    host->scopeWindow->removeEventFilter(owner);
  }
  if (qApp) {
    qApp->removeEventFilter(owner);
  }
  if (host->popupWindow) {
    host->popupWindow->removeEventFilter(owner);
  }
  if (host->popupMenu) {
    host->popupMenu->removeEventFilter(owner);
  }
  if (host->renderedRoot) {
    host->renderedRoot->removeEventFilter(owner);
  }
  host->ownerMenu.clear();
}

void destroySharedPopupHost(QWidget* scopeWindow) {
  if (!scopeWindow) {
    return;
  }

  auto& hosts = sharedPopupHosts();
  auto it = hosts.find(scopeWindow);
  if (it == hosts.end()) {
    return;
  }

  SharedPopupHost* host = it.value();
  hosts.erase(it);
  if (!host) {
    return;
  }

  detachSharedPopupOwner(host);
  if (host->popupWindow) {
    host->popupWindow->hide();
    host->popupWindow->deleteLater();
  }
  delete host;
}

SharedPopupHost* sharedPopupHostFor(const AdMenu* menu) {
  QWidget* scopeWindow = popupScopeWindowFor(menu);
  if (!scopeWindow) {
    return nullptr;
  }

  auto& hosts = sharedPopupHosts();
  auto it = hosts.find(scopeWindow);
  if (it == hosts.end()) {
    return nullptr;
  }
  return it.value();
}

void bindSharedPopupOwner(SharedPopupHost* host, AdMenu* owner) {
  if (!host) {
    return;
  }

  if (host->ownerMenu && host->ownerMenu != owner) {
    detachSharedPopupOwner(host);
    host->ownerMenu.clear();
  }

  host->ownerMenu = owner;
  if (!owner) {
    return;
  }
  if (host->scopeWindow) {
    host->scopeWindow->removeEventFilter(owner);
    host->scopeWindow->installEventFilter(owner);
  }
  if (qApp) {
    qApp->removeEventFilter(owner);
    qApp->installEventFilter(owner);
  }
  if (host->popupWindow) {
    host->popupWindow->removeEventFilter(owner);
    host->popupWindow->installEventFilter(owner);
  }
  if (host->popupMenu) {
    host->popupMenu->removeEventFilter(owner);
    host->popupMenu->installEventFilter(owner);
  }
  if (host->renderedRoot) {
    host->renderedRoot->removeEventFilter(owner);
    host->renderedRoot->installEventFilter(owner);
  }
}

SharedPopupHost* ensureSharedPopupHost(AdMenu* ownerMenu) {
  QWidget* scopeWindow = popupScopeWindowFor(ownerMenu);
  if (!scopeWindow) {
    return nullptr;
  }

  auto& hosts = sharedPopupHosts();
  SharedPopupHost* host = hosts.value(scopeWindow, nullptr);
  if (!host) {
    host = new SharedPopupHost;
    host->scopeWindow = scopeWindow;
    hosts.insert(scopeWindow, host);
    QObject::connect(scopeWindow, &QObject::destroyed, scopeWindow, [scopeWindow]() {
      destroySharedPopupHost(scopeWindow);
    });
  }

  if (!host->popupWindow || !host->popupMenu) {
    if (host->popupWindow) {
      host->popupWindow->hide();
      host->popupWindow->deleteLater();
    }
    host->popupWindow.clear();
    host->popupMenu.clear();
    host->renderedRoot.clear();
    host->activeKey.clear();

    QWidget* popupWindow = new QWidget(scopeWindow);
    popupWindow->setAttribute(Qt::WA_DeleteOnClose, false);
    popupWindow->setObjectName(QStringLiteral("admenu-shared-popup-window"));
    auto* layout = new QVBoxLayout(popupWindow);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto* popupMenu = new AdMenu(popupWindow);
    popupMenu->setMode(AdMenu::Mode::Vertical);
    popupMenu->setInlineCollapsed(false);
    popupMenu->setTooltipEnabled(false);

    host->popupWindow = popupWindow;
    host->popupMenu = popupMenu;
    host->renderedRoot.clear();
    host->activeKey.clear();
  }

  bindSharedPopupOwner(host, ownerMenu);
  return host;
}

void clearLayout(QLayout* layout) {
  if (!layout) {
    return;
  }
  while (QLayoutItem* item = layout->takeAt(0)) {
    delete item;
  }
}

}  // namespace

AdMenu::AdMenu(QWidget* parent) : QWidget(parent) {
  setMouseTracking(true);
  setFocusPolicy(Qt::StrongFocus);
  setAttribute(Qt::WA_Hover, true);

  hoverOpenTimer_.setSingleShot(true);
  connect(&hoverOpenTimer_, &QTimer::timeout, this, [this]() {
    if (pendingHoverOpenKey_.isEmpty()) {
      return;
    }
    AdMenu* sink = (eventSink_ && eventSink_.data() != this) ? eventSink_.data() : this;
    if (sink) {
      sink->openSubMenuByKey(pendingHoverOpenKey_);
    }
  });
  hoverCloseTimer_.setSingleShot(true);
  connect(&hoverCloseTimer_, &QTimer::timeout, this, [this]() {
    clearDanglingPopups();
  });

  connect(&theme::ThemeManager::instance(), &theme::ThemeManager::themeChanged, this, [this]() {
    update();
  });
}

AdMenu::~AdMenu() {
  hoverOpenTimer_.stop();
  hoverCloseTimer_.stop();
  pendingHoverOpenKey_.clear();
  QToolTip::hideText();

  SharedPopupHost* host = sharedPopupHostFor(this);
  if (host && host->ownerMenu == this) {
    bindSharedPopupOwner(host, nullptr);
    host->activeKey.clear();
    if (host->popupWindow) {
      host->popupWindow->hide();
    }
  }
}

AdMenu::Mode AdMenu::mode() const { return mode_; }

void AdMenu::setMode(Mode value) {
  if (mode_ == value) {
    return;
  }
  mode_ = value;
  hoveredEntry_ = -1;
  pressedEntry_ = -1;
  rebuildEntries();
  syncPopupVisibility();
  emit modeChanged(mode_);
  updateGeometry();
  update();
}

AdMenu::MenuTheme AdMenu::theme() const { return theme_; }

void AdMenu::setTheme(MenuTheme value) {
  if (theme_ == value) {
    return;
  }
  theme_ = value;
  syncPopupVisibility();
  emit themeChanged(theme_);
  update();
}

bool AdMenu::selectable() const { return selectable_; }

void AdMenu::setSelectable(bool value) {
  if (selectable_ == value) {
    return;
  }
  selectable_ = value;
  syncPopupVisibility();
  emit selectableChanged(selectable_);
  update();
}

bool AdMenu::multiple() const { return multiple_; }

void AdMenu::setMultiple(bool value) {
  if (multiple_ == value) {
    return;
  }
  multiple_ = value;
  if (!multiple_ && selectedKeys_.size() > 1) {
    applySelectedInternal(QStringList{selectedKeys_.constFirst()}, true);
  }
  syncPopupVisibility();
  emit multipleChanged(multiple_);
  update();
}

QVector<AdMenu::Item> AdMenu::items() const { return items_; }

void AdMenu::setItems(const QVector<Item>& value) {
  QVector<Item> normalized = value;
  normalizeItems(normalized);
  items_ = normalized;

  defaultsApplied_ = false;
  rebuildDepthMaps();
  ensureDefaultStatesApplied();
  rebuildEntries();
  SharedPopupHost* host = sharedPopupHostFor(this);
  if (host && host->ownerMenu == this) {
    host->activeKey.clear();
  }
  syncPopupVisibility();

  emit itemsChanged();
  updateGeometry();
  update();
}

QStringList AdMenu::selectedKeys() const { return selectedKeys_; }

void AdMenu::setSelectedKeys(const QStringList& keys) {
  selectedKeysExplicit_ = true;
  applySelectedInternal(keys, true);
}

QStringList AdMenu::defaultSelectedKeys() const { return defaultSelectedKeys_; }

void AdMenu::setDefaultSelectedKeys(const QStringList& keys) {
  const QStringList normalized = uniqueStringList(keys);
  if (defaultSelectedKeys_ == normalized) {
    return;
  }
  defaultSelectedKeys_ = normalized;
  if (!selectedKeysExplicit_) {
    applySelectedInternal(defaultSelectedKeys_, false);
  }
  emit defaultSelectedKeysChanged(defaultSelectedKeys_);
}

QStringList AdMenu::openKeys() const { return openKeys_; }

void AdMenu::setOpenKeys(const QStringList& keys) {
  openKeysExplicit_ = true;
  applyOpenInternal(keys, true);
}

QStringList AdMenu::defaultOpenKeys() const { return defaultOpenKeys_; }

void AdMenu::setDefaultOpenKeys(const QStringList& keys) {
  const QStringList normalized = uniqueStringList(keys);
  if (defaultOpenKeys_ == normalized) {
    return;
  }
  defaultOpenKeys_ = normalized;
  if (!openKeysExplicit_) {
    applyOpenInternal(defaultOpenKeys_, false);
  }
  emit defaultOpenKeysChanged(defaultOpenKeys_);
}

bool AdMenu::inlineCollapsed() const { return inlineCollapsed_; }

void AdMenu::setInlineCollapsed(bool value) {
  if (inlineCollapsed_ == value) {
    return;
  }
  inlineCollapsed_ = value;
  hoveredEntry_ = -1;
  pressedEntry_ = -1;
  QToolTip::hideText();
  rebuildEntries();
  syncPopupVisibility();
  emit inlineCollapsedChanged(inlineCollapsed_);
  updateGeometry();
  update();
}

int AdMenu::inlineIndent() const { return inlineIndent_; }

void AdMenu::setInlineIndent(int value) {
  value = std::max(0, value);
  if (inlineIndent_ == value) {
    return;
  }
  inlineIndent_ = value;
  rebuildEntries();
  syncPopupVisibility();
  emit inlineIndentChanged(inlineIndent_);
  updateGeometry();
  update();
}

AdMenu::TriggerSubMenuAction AdMenu::triggerSubMenuAction() const { return triggerSubMenuAction_; }

void AdMenu::setTriggerSubMenuAction(TriggerSubMenuAction value) {
  if (triggerSubMenuAction_ == value) {
    return;
  }
  triggerSubMenuAction_ = value;
  syncPopupVisibility();
  emit triggerSubMenuActionChanged(triggerSubMenuAction_);
}

int AdMenu::subMenuOpenDelayMs() const { return subMenuOpenDelayMs_; }

void AdMenu::setSubMenuOpenDelayMs(int value) {
  value = std::max(0, value);
  if (subMenuOpenDelayMs_ == value) {
    return;
  }
  subMenuOpenDelayMs_ = value;
  syncPopupVisibility();
  emit subMenuOpenDelayMsChanged(subMenuOpenDelayMs_);
}

int AdMenu::subMenuCloseDelayMs() const { return subMenuCloseDelayMs_; }

void AdMenu::setSubMenuCloseDelayMs(int value) {
  value = std::max(0, value);
  if (subMenuCloseDelayMs_ == value) {
    return;
  }
  subMenuCloseDelayMs_ = value;
  syncPopupVisibility();
  emit subMenuCloseDelayMsChanged(subMenuCloseDelayMs_);
}

bool AdMenu::tooltipEnabled() const { return tooltipEnabled_; }

void AdMenu::setTooltipEnabled(bool value) {
  if (tooltipEnabled_ == value) {
    return;
  }
  tooltipEnabled_ = value;
  if (!tooltipEnabled_) {
    QToolTip::hideText();
  } else {
    syncTooltipForHoveredEntry();
  }
  syncPopupVisibility();
  emit tooltipEnabledChanged(tooltipEnabled_);
}

AdMenu::TooltipPlacement AdMenu::tooltipPlacement() const { return tooltipPlacement_; }

void AdMenu::setTooltipPlacement(TooltipPlacement value) {
  if (tooltipPlacement_ == value) {
    return;
  }
  tooltipPlacement_ = value;
  emit tooltipPlacementChanged(tooltipPlacement_);
  syncPopupVisibility();
  syncTooltipForHoveredEntry();
}

QString AdMenu::overflowedIndicatorText() const { return overflowedIndicatorText_; }

void AdMenu::setOverflowedIndicatorText(const QString& value) {
  const QString normalized = value.isEmpty() ? QStringLiteral("...") : value;
  if (overflowedIndicatorText_ == normalized) {
    return;
  }
  overflowedIndicatorText_ = normalized;
  syncPopupVisibility();
  emit overflowedIndicatorTextChanged(overflowedIndicatorText_);
  update();
}

AdMenu::ComponentTokens AdMenu::componentTokens() const { return componentTokens_; }

void AdMenu::setComponentTokens(const ComponentTokens& tokens) {
  componentTokens_ = tokens;
  rebuildEntries();
  syncPopupVisibility();
  emit componentTokensChanged();
  update();
}

void AdMenu::resetComponentTokens() {
  componentTokens_ = {};
  rebuildEntries();
  syncPopupVisibility();
  emit componentTokensChanged();
  update();
}

AdMenu::SemanticStyles AdMenu::semanticStyles() const { return semanticStyles_; }

void AdMenu::setSemanticStyles(const SemanticStyles& styles) {
  semanticStyles_ = styles;
  syncPopupVisibility();
  emit semanticStylesChanged();
  update();
}

void AdMenu::setSemanticStyleResolver(SemanticStyleResolver resolver) {
  semanticStyleResolver_ = std::move(resolver);
  syncPopupVisibility();
  emit semanticStylesChanged();
  update();
}

AdMenu::PopupRender AdMenu::popupRender() const { return popupRender_; }

void AdMenu::setPopupRender(PopupRender render) {
  popupRender_ = std::move(render);
  SharedPopupHost* host = sharedPopupHostFor(this);
  if (host && host->ownerMenu == this) {
    host->activeKey.clear();
  }
  emit popupRenderChanged();
  syncPopupVisibility();
}

QIcon AdMenu::expandIcon() const { return expandIcon_; }

void AdMenu::setExpandIcon(const QIcon& icon) {
  expandIcon_ = icon;
  syncPopupVisibility();
  emit expandIconChanged(expandIcon_);
  update();
}

QPoint AdMenu::popupOffset() const { return popupOffset_; }

void AdMenu::setPopupOffset(const QPoint& value) {
  if (popupOffset_ == value) {
    return;
  }
  popupOffset_ = value;
  emit popupOffsetChanged(popupOffset_);
  syncPopupVisibility();
}

AdMenu::ItemPaintHook AdMenu::itemPaintHook() const { return itemPaintHook_; }

void AdMenu::setItemPaintHook(ItemPaintHook hook) {
  itemPaintHook_ = std::move(hook);
  syncPopupVisibility();
  update();
}

AdMenu::ItemPaintHook AdMenu::subMenuPaintHook() const { return subMenuPaintHook_; }

void AdMenu::setSubMenuPaintHook(ItemPaintHook hook) {
  subMenuPaintHook_ = std::move(hook);
  syncPopupVisibility();
  update();
}

QString AdMenu::selectedKey() const { return selectedKeys_.isEmpty() ? QString() : selectedKeys_.first(); }

void AdMenu::setSelectedKey(const QString& key) {
  if (key.isEmpty()) {
    setSelectedKeys({});
    return;
  }
  setSelectedKeys({key});
}

QSize AdMenu::sizeHint() const {
  StyleContext ctx;
  ctx.mode = mode_;
  ctx.theme = theme_;
  ctx.inlineCollapsed = inlineCollapsed_;
  ctx.items = items_;
  const SemanticStyles effectiveSemantic =
      semanticStyleResolver_ ? semanticStyleResolver_(ctx) : semanticStyles_;
  MenuVisualStyle style = resolveVisualStyle(this, effectiveSemantic);
  if (mode_ == Mode::Horizontal) {
    const int h = style.metrics.itemHeight + style.metrics.itemMarginBlock * 2;
    const int contentWidth = horizontalContentWidthHint();
    const int preferredWidth = std::max(160, contentWidth);
    return QSize(std::max(width(), preferredWidth), h + style.metrics.borderWidth);
  }
  const int w = inlineCollapsed_ && mode_ == Mode::Inline ? 56 : 256;
  const int h = std::max(contentHeight_, style.metrics.itemHeight * 4);
  return QSize(w, h);
}

QSize AdMenu::minimumSizeHint() const {
  StyleContext ctx;
  ctx.mode = mode_;
  ctx.theme = theme_;
  ctx.inlineCollapsed = inlineCollapsed_;
  ctx.items = items_;
  const SemanticStyles effectiveSemantic =
      semanticStyleResolver_ ? semanticStyleResolver_(ctx) : semanticStyles_;
  MenuVisualStyle style = resolveVisualStyle(this, effectiveSemantic);
  if (mode_ == Mode::Horizontal) {
    const int h = style.metrics.itemHeight + style.metrics.itemMarginBlock * 2;
    return QSize(std::max(160, horizontalContentWidthHint()), h);
  }
  const int w = inlineCollapsed_ && mode_ == Mode::Inline ? 48 : 120;
  return QSize(w, style.metrics.itemHeight + style.metrics.itemMarginBlock * 2);
}

void AdMenu::paintEvent(QPaintEvent* event) {
  Q_UNUSED(event)

  StyleContext ctx;
  ctx.mode = mode_;
  ctx.theme = theme_;
  ctx.inlineCollapsed = inlineCollapsed_;
  ctx.items = items_;
  const SemanticStyles effectiveSemantic =
      semanticStyleResolver_ ? semanticStyleResolver_(ctx) : semanticStyles_;
  MenuVisualStyle style = resolveVisualStyle(this, effectiveSemantic);
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, true);

  const bool popupLayer = (mode_ == Mode::Vertical && eventSink_ && eventSink_.data() != this);
  if (popupLayer) {
    const qreal popupRadius = std::max<qreal>(0.0, static_cast<qreal>(style.metrics.itemBorderRadius));
    const QRectF popupRect = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    QPainterPath popupPath;
    popupPath.addRoundedRect(popupRect, popupRadius, popupRadius);
    painter.fillPath(popupPath, style.popupBackground);
    painter.save();
    painter.setClipPath(popupPath);
  } else {
    painter.fillRect(rect(), style.menuBackground);
  }

  if (!popupLayer && style.metrics.borderWidth > 0) {
    painter.setPen(QPen(style.borderColor, style.metrics.borderWidth));
    if (mode_ == Mode::Horizontal) {
      painter.drawLine(rect().bottomLeft(), rect().bottomRight());
    } else {
      painter.drawLine(rect().topRight(), rect().bottomRight());
    }
  }

  const bool collapsedInline = (mode_ == Mode::Inline && inlineCollapsed_);
  const bool horizontal = mode_ == Mode::Horizontal;

  for (int i = 0; i < entries_.size(); ++i) {
    const VisibleEntry& entry = entries_.at(i);
    if (!entry.item) {
      continue;
    }

    const QRect rowRect = entry.rect;
    if (!rowRect.intersects(rect())) {
      continue;
    }

    if (entry.type == ItemType::Divider) {
      const int midY = rowRect.center().y();
      QPen pen(style.dividerColor, std::max(1, style.metrics.borderWidth));
      if (entry.dashed) {
        pen.setStyle(Qt::DashLine);
      }
      painter.setPen(pen);
      painter.drawLine(rowRect.left() + style.metrics.itemPaddingInline, midY,
                       rowRect.right() - style.metrics.itemPaddingInline, midY);
      continue;
    }

    if (entry.type == ItemType::Group) {
      painter.setPen(style.groupTitleColor);
      QFont groupFont = font();
      groupFont.setBold(false);
      groupFont.setPointSize(std::max(10, style.metrics.groupTitleFontSize));
      painter.setFont(groupFont);
      QRect textRect = rowRect.adjusted(style.metrics.groupTitleHorizontalPadding, 0,
                                        -style.metrics.groupTitleHorizontalPadding, 0);
      painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft,
                       trimmedOrFallback(entry.item->label, entry.item->key));
      continue;
    }

    const bool hovered = (hoveredEntry_ == i);
    const bool pressed = (pressedEntry_ == i);
    const bool selected = selectedKeys_.contains(entry.key);
    const bool opened = openKeys_.contains(entry.key);

    detail::MenuStateStyle state = style.normal;
    if (horizontal) {
      state = style.horizontalNormal;
      if (selected) {
        state = style.horizontalSelected;
      } else if (pressed) {
        state = style.horizontalActive;
      } else if (hovered || opened) {
        state = style.horizontalHover;
      }
    } else if (entry.disabled) {
      state = style.disabled;
    } else if (entry.danger) {
      if (selected) {
        state = style.dangerSelected;
      } else if (pressed) {
        state = style.dangerActive;
      } else if (hovered) {
        state = style.dangerHover;
      } else {
        state = style.danger;
      }
    } else if (selected) {
      state = style.selected;
    } else if (pressed) {
      state = style.active;
    } else if (hovered) {
      state = style.hover;
    }

    QRect fillRect = rowRect.adjusted(style.metrics.itemMarginInline, style.metrics.itemMarginBlock,
                                      -style.metrics.itemMarginInline, -style.metrics.itemMarginBlock);

    int radius = entry.type == ItemType::SubMenu ? style.metrics.subMenuItemBorderRadius
                                                  : style.metrics.itemBorderRadius;
    if (horizontal) {
      radius = style.metrics.horizontalItemBorderRadius;
    }
    if (state.background.alpha() > 0) {
      painter.setPen(Qt::NoPen);
      painter.setBrush(state.background);
      painter.drawRoundedRect(fillRect, radius, radius);
    }

    if (style.metrics.activeBarWidth > 0 && selected && !horizontal && !collapsedInline) {
      QRect activeBar(fillRect.left(), fillRect.top(), style.metrics.activeBarWidth, fillRect.height());
      painter.setPen(Qt::NoPen);
      painter.setBrush(state.text);
      painter.drawRoundedRect(activeBar, style.metrics.activeBarWidth / 2.0,
                              style.metrics.activeBarWidth / 2.0);
    }
    const bool horizontalActiveBar = horizontal && (selected || pressed || hovered || opened);
    if (horizontalActiveBar) {
      const QColor activeBarColor = style.horizontalSelected.text.isValid()
                                        ? style.horizontalSelected.text
                                        : state.text;
      painter.setPen(QPen(activeBarColor, 2));
      const int activeBarLeft = fillRect.left() + style.metrics.itemPaddingInline;
      const int activeBarRight = fillRect.right() - style.metrics.itemPaddingInline;
      if (activeBarRight > activeBarLeft) {
        painter.drawLine(activeBarLeft, fillRect.bottom(), activeBarRight, fillRect.bottom());
      }
    }

    int indent = 0;
    if (mode_ == Mode::Inline && !collapsedInline) {
      indent = entry.depth * std::max(0, inlineIndent_);
    } else if (mode_ == Mode::Vertical && entry.depth > 0) {
      const int groupChildIndent = std::max(0, style.metrics.itemPaddingInline - style.metrics.itemMarginInline);
      indent = entry.depth * groupChildIndent;
    }

    QRect contentRect = fillRect.adjusted(style.metrics.itemPaddingInline + indent, 0,
                                          -style.metrics.itemPaddingInline, 0);
    if (horizontal) {
      contentRect = fillRect.adjusted(style.metrics.itemPaddingInline, 0, -style.metrics.itemPaddingInline, 0);
    }

    const int iconSide = std::max(10, style.metrics.iconSize);
    const bool hasIcon = !entry.item->icon.isNull();
    QRect iconRect(contentRect.left(),
                   contentRect.center().y() - iconSide / 2,
                   iconSide,
                   iconSide);

    int textLeft = contentRect.left();
    if (hasIcon) {
      const QIcon::Mode iconMode = entry.disabled ? QIcon::Disabled : QIcon::Normal;
      entry.item->icon.paint(&painter, iconRect, Qt::AlignCenter, iconMode);
      textLeft = iconRect.right() + 1 + style.metrics.iconMarginInlineEnd;
    } else if (collapsedInline) {
      textLeft = contentRect.left();
    }

    QRect arrowRect(contentRect.right() - 12,
                    contentRect.center().y() - 7,
                    12,
                    14);

    bool showArrow = (entry.type == ItemType::SubMenu && entry.hasChildren);
    if (collapsedInline && mode_ == Mode::Inline) {
      showArrow = false;
    }

    int textRight = contentRect.right();
    if (showArrow) {
      textRight = arrowRect.left() - 6;
    }
    if (!entry.item->extra.isEmpty() && !collapsedInline && !horizontal) {
      painter.setPen(state.text);
      const QString extraText = entry.item->extra;
      const int extraWidth = painter.fontMetrics().horizontalAdvance(extraText) + 8;
      QRect extraRect(std::max(textLeft, textRight - extraWidth),
                     contentRect.top(),
                     extraWidth,
                     contentRect.height());
      painter.drawText(extraRect, Qt::AlignVCenter | Qt::AlignRight, extraText);
      textRight = extraRect.left() - 4;
    }

    QString label = trimmedOrFallback(entry.item->label, entry.item->key);
    if (collapsedInline && mode_ == Mode::Inline) {
      if (hasIcon) {
        label.clear();
      } else if (!label.isEmpty()) {
        label = label.left(1);
      }
    }

    painter.setPen(state.text);
    painter.setFont(font());
    QRect textRect(textLeft, contentRect.top(), std::max(0, textRight - textLeft), contentRect.height());
    painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, label);

    if (showArrow) {
      if (!expandIcon_.isNull()) {
        const QIcon::Mode iconMode = entry.disabled ? QIcon::Disabled : QIcon::Normal;
        expandIcon_.paint(&painter, arrowRect, Qt::AlignCenter, iconMode);
      } else {
        const bool open = openKeys_.contains(entry.key);
        const bool pointDown = (mode_ == Mode::Horizontal) ||
                               (mode_ == Mode::Inline && !inlineCollapsed_ && open);
        drawDefaultExpandIndicator(painter, arrowRect, pointDown, state.text);
      }
    }

    ItemPaintContext ctx;
    ctx.item = *entry.item;
    ctx.itemRect = fillRect;
    ctx.hovered = hovered;
    ctx.pressed = pressed;
    ctx.selected = selected;
    ctx.mode = mode_;
    if (entry.type == ItemType::SubMenu && subMenuPaintHook_) {
      subMenuPaintHook_(painter, ctx);
    } else if (itemPaintHook_) {
      itemPaintHook_(painter, ctx);
    }
  }

  if (popupLayer) {
    painter.restore();
  }
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void AdMenu::enterEvent(QEnterEvent* event) {
#else
void AdMenu::enterEvent(QEvent* event) {
#endif
  QWidget::enterEvent(event);
  const QPoint localCursor = mapFromGlobal(QCursor::pos());
  if (rect().contains(localCursor)) {
    hoverCloseTimer_.stop();
  }
  syncTooltipForHoveredEntry();
}

void AdMenu::leaveEvent(QEvent* event) {
  QWidget::leaveEvent(event);
  setHoveredEntry(-1);

  const bool popupLikeMode = mode_ == Mode::Vertical || mode_ == Mode::Horizontal ||
                             (mode_ == Mode::Inline && inlineCollapsed_);
  if (triggerSubMenuAction_ == TriggerSubMenuAction::Hover && popupLikeMode) {
    requestHoverOpen(QString());
    const int delay = std::max(0, subMenuCloseDelayMs_);
    hoverCloseTimer_.start(delay);
  }
}

void AdMenu::mouseMoveEvent(QMouseEvent* event) {
  const int index = entryIndexAt(mouseEventPos(event));
  setHoveredEntry(index);

  const bool popupLikeMode = mode_ == Mode::Vertical || mode_ == Mode::Horizontal ||
                             (mode_ == Mode::Inline && inlineCollapsed_);
  const bool hoverPopupMode =
      triggerSubMenuAction_ == TriggerSubMenuAction::Hover && popupLikeMode;

  bool hitOpenable = false;
  if (index >= 0 && index < entries_.size()) {
    const VisibleEntry& entry = entries_.at(index);
    hitOpenable = rowIsOpenable(entry);
    if (hoverPopupMode && hitOpenable) {
      hoverCloseTimer_.stop();
      requestHoverOpen(entry.key);
    }
  }

  if (hoverPopupMode && !hitOpenable) {
    requestHoverOpen(QString());
    if (!openKeys_.isEmpty()) {
      const int delay = std::max(0, subMenuCloseDelayMs_);
      hoverCloseTimer_.start(delay);
    }
  }

  QWidget::mouseMoveEvent(event);
}

void AdMenu::mousePressEvent(QMouseEvent* event) {
  if (!event || event->button() != Qt::LeftButton) {
    QWidget::mousePressEvent(event);
    return;
  }
  pressedEntry_ = entryIndexAt(mouseEventPos(event));
  update();
  QWidget::mousePressEvent(event);
}

void AdMenu::mouseReleaseEvent(QMouseEvent* event) {
  if (!event || event->button() != Qt::LeftButton) {
    QWidget::mouseReleaseEvent(event);
    return;
  }

  const int releaseEntry = entryIndexAt(mouseEventPos(event));
  const int pressed = pressedEntry_;
  pressedEntry_ = -1;

  if (releaseEntry >= 0 && releaseEntry == pressed) {
    activateEntry(releaseEntry, false);
  }

  update();
  QWidget::mouseReleaseEvent(event);
}

void AdMenu::keyPressEvent(QKeyEvent* event) {
  if (!event) {
    QWidget::keyPressEvent(event);
    return;
  }

  if (entries_.isEmpty()) {
    QWidget::keyPressEvent(event);
    return;
  }

  const int dir = (mode_ == Mode::Horizontal) ? 1 : 0;

  if (event->key() == Qt::Key_Down && dir == 0) {
    int index = hoveredEntry_;
    const int last = static_cast<int>(entries_.size()) - 1;
    if (index < 0) {
      index = 0;
    } else {
      index = std::min(last, index + 1);
    }
    setHoveredEntry(index);
    event->accept();
    return;
  }

  if (event->key() == Qt::Key_Up && dir == 0) {
    int index = hoveredEntry_;
    if (index < 0) {
      index = 0;
    } else {
      index = std::max(0, index - 1);
    }
    setHoveredEntry(index);
    event->accept();
    return;
  }

  if (event->key() == Qt::Key_Right && dir == 1) {
    int index = hoveredEntry_;
    if (index < 0) {
      index = 0;
    } else {
      index = (index + 1) % static_cast<int>(entries_.size());
    }
    setHoveredEntry(index);
    event->accept();
    return;
  }

  if (event->key() == Qt::Key_Left && dir == 1) {
    int index = hoveredEntry_;
    if (index < 0) {
      index = 0;
    } else {
      index = (index - 1 + static_cast<int>(entries_.size())) % static_cast<int>(entries_.size());
    }
    setHoveredEntry(index);
    event->accept();
    return;
  }

  if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter || event->key() == Qt::Key_Space) {
    if (hoveredEntry_ >= 0 && hoveredEntry_ < entries_.size()) {
      activateEntry(hoveredEntry_, true);
      event->accept();
      return;
    }
  }

  QWidget::keyPressEvent(event);
}

void AdMenu::focusOutEvent(QFocusEvent* event) {
  pressedEntry_ = -1;
  QWidget::focusOutEvent(event);
  update();
}

void AdMenu::changeEvent(QEvent* event) {
  QWidget::changeEvent(event);
  if (event && event->type() == QEvent::EnabledChange) {
    update();
  }
}

void AdMenu::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  rebuildEntries();
  syncPopupVisibility();
}

bool AdMenu::eventFilter(QObject* watched, QEvent* event) {
  if (!watched || !event) {
    return QWidget::eventFilter(watched, event);
  }

  SharedPopupHost* host = sharedPopupHostFor(this);
  if (host && host->ownerMenu == this) {
    // Recreate Qt::Popup outside-click dismissal by observing mouse presses at app scope.
    if (event->type() == QEvent::MouseButtonPress && host->popupWindow && host->popupWindow->isVisible()) {
      const auto* mouseEvent = static_cast<QMouseEvent*>(event);
      const QPoint clickGlobalPos = mouseEventGlobalPos(mouseEvent);
      const bool clickInScope = widgetContainsGlobalPos(host->scopeWindow, clickGlobalPos);
      const bool clickInMenu = widgetContainsGlobalPos(this, clickGlobalPos);
      const bool clickInPopup = widgetContainsGlobalPos(host->popupWindow, clickGlobalPos);
      if (clickInScope && !clickInMenu && !clickInPopup) {
        applyOpenInternal({}, true);
      }
    }

    if (watched == host->scopeWindow.data()) {
      if (event->type() == QEvent::WindowDeactivate || event->type() == QEvent::Hide) {
        if (!openKeys_.isEmpty()) {
          applyOpenInternal({}, true);
        } else {
          hidePopupAndDescendants(QString());
        }
      } else if (event->type() == QEvent::Resize && host->popupWindow && host->popupWindow->isVisible()) {
        syncPopupVisibility();
      }
    }

    const bool fromSharedPopup = watched == host->popupWindow.data() || watched == host->popupMenu.data() ||
                                 watched == host->renderedRoot.data();
    if (fromSharedPopup && event->type() == QEvent::Destroy) {
      if (watched == host->renderedRoot.data()) {
        host->renderedRoot.clear();
      }
      if (watched == host->popupMenu.data()) {
        host->popupMenu.clear();
      }
      if (watched == host->popupWindow.data()) {
        detachSharedPopupOwner(host);
        host->popupWindow.clear();
        host->popupMenu.clear();
        host->renderedRoot.clear();
        host->activeKey.clear();
      }
    } else if (fromSharedPopup && event->type() == QEvent::Hide && watched == host->popupWindow.data()) {
      host->activeKey.clear();
    } else if (fromSharedPopup && event->type() == QEvent::Enter) {
      if (widgetContainsGlobalPos(host->popupWindow, QCursor::pos())) {
        hoverCloseTimer_.stop();
      }
    } else if (fromSharedPopup && event->type() == QEvent::Leave &&
               triggerSubMenuAction_ == TriggerSubMenuAction::Hover &&
               (mode_ == Mode::Vertical || mode_ == Mode::Horizontal ||
                (mode_ == Mode::Inline && inlineCollapsed_))) {
      const bool cursorStillInPopup = host->popupWindow && host->popupWindow->isVisible() &&
                                      widgetContainsGlobalPos(host->popupWindow, QCursor::pos());
      if (!cursorStillInPopup) {
        const int delay = std::max(0, subMenuCloseDelayMs_);
        hoverCloseTimer_.start(delay);
      }
    }
  }

  return QWidget::eventFilter(watched, event);
}

bool AdMenu::rowIsInteractive(const VisibleEntry& row) const {
  if (row.disabled) {
    return false;
  }
  return row.type == ItemType::Item || row.type == ItemType::SubMenu;
}

bool AdMenu::rowIsSelectable(const VisibleEntry& row) const {
  return rowIsInteractive(row) && row.type == ItemType::Item;
}

bool AdMenu::rowIsOpenable(const VisibleEntry& row) const {
  return rowIsInteractive(row) && row.type == ItemType::SubMenu && row.hasChildren;
}

void AdMenu::normalizeItems(QVector<Item>& items) {
  std::function<void(QVector<Item>&, const QString&)> normalizeRecursive;
  normalizeRecursive = [this, &normalizeRecursive](QVector<Item>& level, const QString& prefix) {
    for (int i = 0; i < level.size(); ++i) {
      Item& item = level[i];
      if (item.key.trimmed().isEmpty()) {
        item.key = autoItemKey(prefix, i);
      } else {
        item.key = normalizedKeyPart(item.key.trimmed());
      }
      if (item.label.isEmpty() && item.type != ItemType::Divider) {
        item.label = item.key;
      }

      if (item.type == ItemType::Divider) {
        item.children.clear();
        continue;
      }

      if (!item.children.isEmpty()) {
        normalizeRecursive(item.children, item.key + QStringLiteral("_"));
      }
    }
  };

  normalizeRecursive(items, QStringLiteral("item_"));
}

AdMenu::ItemType AdMenu::effectiveType(const Item& item) const {
  if (item.type == ItemType::Divider || item.type == ItemType::Group || item.type == ItemType::SubMenu) {
    return item.type;
  }
  if (!item.children.isEmpty()) {
    return ItemType::SubMenu;
  }
  return ItemType::Item;
}

bool AdMenu::isSubMenuItem(const Item& item) const { return effectiveType(item) == ItemType::SubMenu; }

void AdMenu::rebuildEntries() {
  ensureDefaultStatesApplied();
  rebuildDepthMaps();

  validItemKeys_.clear();
  validSubMenuKeys_.clear();
  collectItemKeysRecursive(items_, validItemKeys_);
  collectSubMenuKeysRecursive(items_, validSubMenuKeys_);

  selectedKeys_ = normalizeSelectedKeys(selectedKeys_);
  openKeys_ = normalizeOpenKeys(openKeys_);

  entries_.clear();
  contentHeight_ = 0;

  StyleContext ctx;
  ctx.mode = mode_;
  ctx.theme = theme_;
  ctx.inlineCollapsed = inlineCollapsed_;
  ctx.items = items_;
  const SemanticStyles effectiveSemantic =
      semanticStyleResolver_ ? semanticStyleResolver_(ctx) : semanticStyles_;
  MenuVisualStyle style = resolveVisualStyle(this, effectiveSemantic);
  int cursorY = 0;
  int cursorX = 0;

  if (mode_ == Mode::Horizontal) {
    appendHorizontalEntries(items_, cursorX);
    contentHeight_ = style.metrics.itemHeight + style.metrics.itemMarginBlock * 2 + style.metrics.borderWidth;
  } else if (mode_ == Mode::Inline && !inlineCollapsed_) {
    appendInlineEntries(items_, 0, {}, cursorY);
    contentHeight_ = cursorY + style.metrics.borderWidth;
  } else {
    appendVerticalEntries(items_, 0, {}, cursorY, true);
    contentHeight_ = cursorY + style.metrics.borderWidth;
  }

  updateGeometry();
  update();
}

void AdMenu::rebuildDepthMaps() {
  subMenuDepths_.clear();
  subMenuParents_.clear();

  std::function<void(const QVector<Item>&, int, const QString&)> walk;
  walk = [this, &walk](const QVector<Item>& items, int depth, const QString& parentKey) {
    for (const Item& item : items) {
      if (effectiveType(item) == ItemType::SubMenu) {
        subMenuDepths_.insert(item.key, depth);
        if (!parentKey.isEmpty()) {
          subMenuParents_.insert(item.key, parentKey);
        }
        walk(item.children, depth + 1, item.key);
      } else if (effectiveType(item) == ItemType::Group) {
        walk(item.children, depth, parentKey);
      }
    }
  };

  walk(items_, 1, QString());
}

void AdMenu::ensureDefaultStatesApplied() {
  if (defaultsApplied_) {
    return;
  }

  defaultsApplied_ = true;
  if (!selectedKeysExplicit_ && !defaultSelectedKeys_.isEmpty()) {
    selectedKeys_ = normalizeSelectedKeys(defaultSelectedKeys_);
  } else {
    selectedKeys_ = normalizeSelectedKeys(selectedKeys_);
  }

  if (!openKeysExplicit_ && !defaultOpenKeys_.isEmpty()) {
    openKeys_ = normalizeOpenKeys(defaultOpenKeys_);
  } else {
    openKeys_ = normalizeOpenKeys(openKeys_);
  }
}

void AdMenu::syncPopupVisibility() {
  if (eventSink_ && eventSink_.data() != this) {
    return;
  }

  if (mode_ == Mode::Inline && !inlineCollapsed_) {
    hidePopupAndDescendants(QString());
    return;
  }

  const VisibleEntry* targetEntry = nullptr;
  for (int i = openKeys_.size() - 1; i >= 0; --i) {
    const int index = entryIndexByKey(openKeys_.at(i));
    if (index < 0 || index >= entries_.size()) {
      continue;
    }
    const VisibleEntry& entry = entries_.at(index);
    if (!rowIsOpenable(entry)) {
      continue;
    }
    targetEntry = &entry;
    break;
  }

  if (!targetEntry) {
    hidePopupAndDescendants(QString());
    return;
  }

  PopupRecord* record = ensurePopupForEntry(*targetEntry);
  if (!record || !record->popup) {
    return;
  }
  positionPopup(*targetEntry, *record);
  record->popup->show();
  record->popup->raise();
}

void AdMenu::syncTooltipForHoveredEntry() {
  if (!tooltipEnabled_ || !(mode_ == Mode::Inline && inlineCollapsed_)) {
    QToolTip::hideText();
    return;
  }
  if (hoveredEntry_ < 0 || hoveredEntry_ >= entries_.size()) {
    QToolTip::hideText();
    return;
  }
  const VisibleEntry& entry = entries_.at(hoveredEntry_);
  showTooltipForEntry(entry, QCursor::pos());
}

void AdMenu::appendInlineEntries(const QVector<Item>& items,
                                 int depth,
                                 const QStringList& submenuAncestors,
                                 int& cursorY) {
  StyleContext ctx;
  ctx.mode = mode_;
  ctx.theme = theme_;
  ctx.inlineCollapsed = inlineCollapsed_;
  ctx.items = items_;
  const SemanticStyles effectiveSemantic =
      semanticStyleResolver_ ? semanticStyleResolver_(ctx) : semanticStyles_;
  MenuVisualStyle style = resolveVisualStyle(this, effectiveSemantic);
  const int rowWidth = std::max(width(), minimumSizeHint().width());

  for (const Item& item : items) {
    const ItemType type = effectiveType(item);
    VisibleEntry entry;
    entry.item = &item;
    entry.type = type;
    entry.key = item.key;
    entry.keyPath = QStringList{item.key} + submenuAncestors;
    entry.depth = depth;
    entry.disabled = item.disabled;
    entry.danger = item.danger;
    entry.dashed = item.dashed;
    entry.hasChildren = !item.children.isEmpty();
    entry.popupTheme = item.hasSubMenuTheme ? item.subMenuTheme : theme_;

    const int height = rowHeightForType(type);
    entry.rect = QRect(0, cursorY, rowWidth, height);
    cursorY += height;
    entries_.append(entry);

    if (type == ItemType::Group) {
      appendInlineEntries(item.children, depth + 1, submenuAncestors, cursorY);
      continue;
    }

    if (type == ItemType::SubMenu && openKeys_.contains(item.key)) {
      appendInlineEntries(item.children, depth + 1, entry.keyPath, cursorY);
    }
  }

  if (entries_.isEmpty()) {
    cursorY += style.metrics.itemHeight;
  }
}

void AdMenu::appendVerticalEntries(const QVector<Item>& items,
                                   int depth,
                                   const QStringList& submenuAncestors,
                                   int& cursorY,
                                   bool rootOnlySubmenus) {
  StyleContext ctx;
  ctx.mode = mode_;
  ctx.theme = theme_;
  ctx.inlineCollapsed = inlineCollapsed_;
  ctx.items = items_;
  const SemanticStyles effectiveSemantic =
      semanticStyleResolver_ ? semanticStyleResolver_(ctx) : semanticStyles_;
  MenuVisualStyle style = resolveVisualStyle(this, effectiveSemantic);
  const int rowWidth = std::max(width(), minimumSizeHint().width());

  for (const Item& item : items) {
    const ItemType type = effectiveType(item);
    VisibleEntry entry;
    entry.item = &item;
    entry.type = type;
    entry.key = item.key;
    entry.keyPath = QStringList{item.key} + submenuAncestors;
    entry.depth = depth;
    entry.disabled = item.disabled;
    entry.danger = item.danger;
    entry.dashed = item.dashed;
    entry.hasChildren = !item.children.isEmpty();
    entry.popupTheme = item.hasSubMenuTheme ? item.subMenuTheme : theme_;

    const int height = rowHeightForType(type);
    entry.rect = QRect(0, cursorY, rowWidth, height);
    cursorY += height;
    entries_.append(entry);

    if (type == ItemType::Group) {
      appendVerticalEntries(item.children, depth + 1, submenuAncestors, cursorY, rootOnlySubmenus);
      continue;
    }

    if (!rootOnlySubmenus && type == ItemType::SubMenu && openKeys_.contains(item.key)) {
      appendVerticalEntries(item.children, depth + 1, entry.keyPath, cursorY, false);
    }
  }

  if (entries_.isEmpty()) {
    cursorY += style.metrics.itemHeight;
  }
}

void AdMenu::appendHorizontalEntries(const QVector<Item>& items, int& cursorX) {
  StyleContext ctx;
  ctx.mode = mode_;
  ctx.theme = theme_;
  ctx.inlineCollapsed = inlineCollapsed_;
  ctx.items = items_;
  const SemanticStyles effectiveSemantic =
      semanticStyleResolver_ ? semanticStyleResolver_(ctx) : semanticStyles_;
  MenuVisualStyle style = resolveVisualStyle(this, effectiveSemantic);
  const int itemHeight = style.metrics.itemHeight + style.metrics.itemMarginBlock * 2;

  for (const Item& item : items) {
    const ItemType type = effectiveType(item);
    if (type == ItemType::Divider) {
      continue;
    }

    VisibleEntry entry;
    entry.item = &item;
    entry.type = type;
    entry.key = item.key;
    entry.keyPath = QStringList{item.key};
    entry.depth = 0;
    entry.disabled = item.disabled;
    entry.danger = item.danger;
    entry.dashed = item.dashed;
    entry.hasChildren = !item.children.isEmpty();
    entry.popupTheme = item.hasSubMenuTheme ? item.subMenuTheme : theme_;

    const int widthHint = horizontalEntryWidthHint(item, type, style);

    entry.rect = QRect(cursorX, 0, widthHint, itemHeight);
    cursorX += widthHint + style.metrics.horizontalSpacing;
    entries_.append(entry);
  }
}

int AdMenu::horizontalEntryWidthHint(const Item& item,
                                     ItemType type,
                                     const detail::MenuVisualStyle& style) const {
  const QString label = trimmedOrFallback(item.label, item.key);
  const QFontMetricsF metrics(font());
  const int iconSide = std::max(10, style.metrics.iconSize);

  int widthHint = 0;
  widthHint += style.metrics.itemMarginInline * 2;
  widthHint += style.metrics.itemPaddingInline * 2;
  widthHint += static_cast<int>(std::ceil(metrics.horizontalAdvance(label)));

  if (!item.icon.isNull()) {
    // Keep this in sync with paint geometry: icon area + explicit icon/text separation.
    widthHint += iconSide + style.metrics.iconMarginInlineEnd + 1;
  }

  if (type == ItemType::SubMenu && !item.children.isEmpty()) {
    // Match paint geometry: 12px arrow box + 6px text/arrow gap.
    widthHint += 12 + 6;
  }

  const int minWidth = 72 + style.metrics.itemMarginInline * 2;
  return std::max(widthHint, minWidth);
}

int AdMenu::horizontalContentWidthHint() const {
  if (mode_ != Mode::Horizontal || items_.isEmpty()) {
    return 0;
  }

  StyleContext ctx;
  ctx.mode = mode_;
  ctx.theme = theme_;
  ctx.inlineCollapsed = inlineCollapsed_;
  ctx.items = items_;
  const SemanticStyles effectiveSemantic =
      semanticStyleResolver_ ? semanticStyleResolver_(ctx) : semanticStyles_;
  const MenuVisualStyle style = resolveVisualStyle(this, effectiveSemantic);

  int totalWidth = 0;
  int visibleCount = 0;
  for (const Item& item : items_) {
    const ItemType type = effectiveType(item);
    if (type == ItemType::Divider) {
      continue;
    }
    totalWidth += horizontalEntryWidthHint(item, type, style);
    ++visibleCount;
  }

  if (visibleCount > 1) {
    totalWidth += (visibleCount - 1) * style.metrics.horizontalSpacing;
  }

  return totalWidth;
}

int AdMenu::rowHeightForType(ItemType type) const {
  StyleContext ctx;
  ctx.mode = mode_;
  ctx.theme = theme_;
  ctx.inlineCollapsed = inlineCollapsed_;
  ctx.items = items_;
  const SemanticStyles effectiveSemantic =
      semanticStyleResolver_ ? semanticStyleResolver_(ctx) : semanticStyles_;
  MenuVisualStyle style = resolveVisualStyle(this, effectiveSemantic);
  if (type == ItemType::Divider) {
    return std::max(4, style.metrics.borderWidth + style.metrics.dividerMarginBlock * 2 + 2);
  }
  if (type == ItemType::Group) {
    return std::max(style.metrics.groupTitleLineHeight,
                    style.metrics.groupTitleVerticalPadding * 2 + style.metrics.groupTitleFontSize);
  }
  return style.metrics.itemHeight + style.metrics.itemMarginBlock * 2;
}

int AdMenu::entryIndexAt(const QPoint& pos) const {
  for (int i = 0; i < entries_.size(); ++i) {
    if (entries_.at(i).rect.contains(pos)) {
      return i;
    }
  }
  return -1;
}

int AdMenu::entryIndexByKey(const QString& key) const {
  if (key.isEmpty()) {
    return -1;
  }
  for (int i = 0; i < entries_.size(); ++i) {
    if (entries_.at(i).key == key) {
      return i;
    }
  }
  return -1;
}

void AdMenu::setHoveredEntry(int index) {
  if (hoveredEntry_ == index) {
    return;
  }
  hoveredEntry_ = index;
  if (hoveredEntry_ >= 0 && hoveredEntry_ < entries_.size()) {
    const VisibleEntry& entry = entries_.at(hoveredEntry_);
    if (mode_ == Mode::Inline && inlineCollapsed_ && tooltipEnabled_) {
      showTooltipForEntry(entry, mapToGlobal(entry.rect.center()));
    } else if (!(mode_ == Mode::Inline && inlineCollapsed_)) {
      QToolTip::hideText();
    }
  } else {
    QToolTip::hideText();
  }
  update();
}

void AdMenu::activateEntry(int index, bool fromKeyboard) {
  if (index < 0 || index >= entries_.size()) {
    return;
  }

  const VisibleEntry& entry = entries_.at(index);
  if (!rowIsInteractive(entry)) {
    return;
  }

  const QStringList mergedKeyPath = mergeKeyPathWithPrefix(entry.keyPath);
  AdMenu* sink = eventSink_ ? eventSink_.data() : this;
  if (!sink) {
    sink = this;
  }

  if (rowIsSelectable(entry)) {
    QStringList nextSelected = sink->selectedKeys_;
    bool deselected = false;
    if (sink->multiple_) {
      if (nextSelected.contains(entry.key)) {
        nextSelected.removeAll(entry.key);
        deselected = true;
      } else {
        nextSelected.append(entry.key);
      }
      nextSelected = uniqueStringList(nextSelected);
    } else {
      nextSelected = QStringList{entry.key};
      deselected = false;
    }

    if (sink->selectable_) {
      sink->applySelectedInternal(nextSelected, true);
      if (sink != this) {
        applySelectedInternal(nextSelected, false);
      }
      if (deselected) {
        emit sink->deselected(entry.key, mergedKeyPath, sink->selectedKeys_);
      } else {
        emit sink->selected(entry.key, mergedKeyPath, sink->selectedKeys_);
      }
    }

    emit sink->clicked(entry.key, mergedKeyPath);

    if (sink->mode_ != Mode::Inline || sink->inlineCollapsed_) {
      sink->hidePopupAndDescendants(QString());
    }
  } else if (rowIsOpenable(entry)) {
    emit sink->titleClicked(entry.key);

    const bool popupMode = sink->mode_ == Mode::Vertical || sink->mode_ == Mode::Horizontal ||
                           (sink->mode_ == Mode::Inline && sink->inlineCollapsed_);
    const bool shouldToggle = sink->triggerSubMenuAction_ == TriggerSubMenuAction::Click ||
                              fromKeyboard || !popupMode;
    if (shouldToggle) {
      sink->toggleSubMenuByKey(entry.key);
    }
  }
}

QStringList AdMenu::normalizeSelectedKeys(const QStringList& keys) const {
  QStringList unique = uniqueStringList(keys);
  if (validItemKeys_.isEmpty()) {
    return multiple_ ? unique : (unique.isEmpty() ? QStringList() : QStringList{unique.first()});
  }

  QStringList out;
  out.reserve(unique.size());
  for (const QString& key : unique) {
    if (!validItemKeys_.contains(key)) {
      continue;
    }
    const Item* found = nullptr;
    ItemType resolved = ItemType::Item;
    if (!findItemByKeyRecursive(items_, key, &found, &resolved)) {
      continue;
    }
    if (resolved == ItemType::Item) {
      out.append(key);
    }
  }
  if (!multiple_ && out.size() > 1) {
    out = QStringList{out.first()};
  }
  return out;
}

QStringList AdMenu::normalizeOpenKeys(const QStringList& keys) const {
  QStringList unique = uniqueStringList(keys);
  if (validSubMenuKeys_.isEmpty()) {
    return unique;
  }

  QStringList out;
  out.reserve(unique.size());
  for (const QString& key : unique) {
    if (validSubMenuKeys_.contains(key)) {
      out.append(key);
    }
  }
  return out;
}

void AdMenu::applySelectedInternal(const QStringList& keys, bool emitSignals) {
  const QStringList normalized = normalizeSelectedKeys(keys);
  if (selectedKeys_ == normalized) {
    return;
  }
  selectedKeys_ = normalized;
  if (emitSignals) {
    emit selectedKeysChanged(selectedKeys_);
  }

  SharedPopupHost* host = sharedPopupHostFor(this);
  if (host && host->ownerMenu == this && host->popupMenu && host->popupMenu.data() != this) {
    host->popupMenu->setSelectedKeys(selectedKeys_);
  }
  update();
}

void AdMenu::applyOpenInternal(const QStringList& keys, bool emitSignals) {
  const QStringList normalized = normalizeOpenKeys(keys);
  if (openKeys_ == normalized) {
    return;
  }
  openKeys_ = normalized;
  rebuildEntries();
  syncPopupVisibility();
  if (emitSignals) {
    emit openKeysChanged(openKeys_);
    emitOpenChanged();
  }

  SharedPopupHost* host = sharedPopupHostFor(this);
  if (host && host->ownerMenu == this && host->popupMenu && host->popupMenu.data() != this) {
    host->popupMenu->setOpenKeys(openKeys_);
  }
}

void AdMenu::emitOpenChanged() { emit openChanged(openKeys_); }

bool AdMenu::findItemByKeyRecursive(const QVector<Item>& items,
                                    const QString& key,
                                    const Item** result,
                                    ItemType* resolvedType) const {
  if (result) {
    *result = nullptr;
  }
  for (const Item& item : items) {
    if (item.key == key) {
      if (result) {
        *result = &item;
      }
      if (resolvedType) {
        *resolvedType = effectiveType(item);
      }
      return true;
    }
    if (!item.children.isEmpty()) {
      if (findItemByKeyRecursive(item.children, key, result, resolvedType)) {
        return true;
      }
    }
  }
  return false;
}

void AdMenu::collectItemKeysRecursive(const QVector<Item>& items, QSet<QString>& keys) const {
  for (const Item& item : items) {
    keys.insert(item.key);
    if (!item.children.isEmpty()) {
      collectItemKeysRecursive(item.children, keys);
    }
  }
}

void AdMenu::collectSubMenuKeysRecursive(const QVector<Item>& items,
                                         QSet<QString>& keys,
                                         int depth,
                                         const QString& parentKey) {
  Q_UNUSED(depth)
  Q_UNUSED(parentKey)
  for (const Item& item : items) {
    if (effectiveType(item) == ItemType::SubMenu) {
      keys.insert(item.key);
      collectSubMenuKeysRecursive(item.children, keys, depth + 1, item.key);
    } else if (effectiveType(item) == ItemType::Group) {
      collectSubMenuKeysRecursive(item.children, keys, depth, parentKey);
    }
  }
}

bool AdMenu::isDescendantSubMenuKey(const QString& candidateKey, const QString& parentKey) const {
  if (candidateKey.isEmpty() || parentKey.isEmpty() || candidateKey == parentKey) {
    return false;
  }

  QString current = candidateKey;
  while (!current.isEmpty()) {
    const QString parent = subMenuParents_.value(current);
    if (parent.isEmpty()) {
      return false;
    }
    if (parent == parentKey) {
      return true;
    }
    current = parent;
  }
  return false;
}

void AdMenu::openSubMenuByKey(const QString& key) {
  if (key.isEmpty() || !validSubMenuKeys_.contains(key)) {
    return;
  }
  if (openKeys_.contains(key)) {
    SharedPopupHost* host = sharedPopupHostFor(this);
    if (host && host->ownerMenu == this && host->popupWindow && host->popupWindow->isVisible() &&
        host->activeKey == key) {
      return;
    }
    syncPopupVisibility();
    return;
  }

  QStringList next = openKeys_;
  const bool popupLike = mode_ != Mode::Inline || inlineCollapsed_;
  if (popupLike) {
    QStringList reduced;
    reduced.reserve(next.size() + 1);
    for (const QString& existing : next) {
      if (existing == key) {
        continue;
      }
      const bool isAncestor = isDescendantSubMenuKey(key, existing);
      const bool isDescendant = isDescendantSubMenuKey(existing, key);
      if (isAncestor || isDescendant) {
        reduced.append(existing);
      }
    }
    next = reduced;
  }
  if (!next.contains(key)) {
    next.append(key);
  }
  applyOpenInternal(next, true);
}

void AdMenu::closeSubMenuByKey(const QString& key) {
  if (key.isEmpty()) {
    return;
  }
  QStringList next = openKeys_;
  bool changed = false;
  for (int i = next.size() - 1; i >= 0; --i) {
    if (next.at(i) == key || isDescendantSubMenuKey(next.at(i), key)) {
      next.removeAt(i);
      changed = true;
    }
  }
  if (changed) {
    applyOpenInternal(next, true);
  }
  hidePopupAndDescendants(key);
}

void AdMenu::toggleSubMenuByKey(const QString& key) {
  if (openKeys_.contains(key)) {
    closeSubMenuByKey(key);
  } else {
    openSubMenuByKey(key);
  }
}

void AdMenu::requestHoverOpen(const QString& key) {
  if (key.isEmpty()) {
    pendingHoverOpenKey_.clear();
    hoverOpenTimer_.stop();
    return;
  }

  pendingHoverOpenKey_ = key;
  const int delay = std::max(0, subMenuOpenDelayMs_);
  if (delay == 0) {
    hoverOpenTimer_.stop();
    openSubMenuByKey(key);
  } else {
    hoverOpenTimer_.start(delay);
  }
}

void AdMenu::showTooltipForEntry(const VisibleEntry& entry, const QPoint& globalPos) {
  if (!tooltipEnabled_ || !(mode_ == Mode::Inline && inlineCollapsed_)) {
    return;
  }
  if (!entry.item || entry.type == ItemType::Divider || entry.type == ItemType::Group) {
    return;
  }

  const QString text = !entry.item->title.trimmed().isEmpty()
                           ? entry.item->title.trimmed()
                           : trimmedOrFallback(entry.item->label, entry.item->key);
  if (text.isEmpty()) {
    return;
  }

  QPoint pos = globalPos;
  const QRect globalRect(mapToGlobal(entry.rect.topLeft()), entry.rect.size());
  switch (tooltipPlacement_) {
    case TooltipPlacement::Left:
      pos = QPoint(globalRect.left() - 8, globalRect.center().y());
      break;
    case TooltipPlacement::Top:
      pos = QPoint(globalRect.center().x(), globalRect.top() - 8);
      break;
    case TooltipPlacement::Bottom:
      pos = QPoint(globalRect.center().x(), globalRect.bottom() + 8);
      break;
    case TooltipPlacement::Right:
    default:
      pos = QPoint(globalRect.right() + 8, globalRect.center().y());
      break;
  }

  QToolTip::showText(pos, text, this, entry.rect);
}

AdMenu::PopupRecord* AdMenu::ensurePopupForEntry(const VisibleEntry& entry) {
  if (!entry.item || !rowIsOpenable(entry)) {
    return nullptr;
  }

  SharedPopupHost* host = ensureSharedPopupHost(this);
  if (!host || !host->popupWindow || !host->popupMenu) {
    return nullptr;
  }
  const bool ownerChanged = host->ownerMenu != this;
  bindSharedPopupOwner(host, this);

  AdMenu* sink = eventSink_ ? eventSink_.data() : this;
  if (!sink) {
    sink = this;
  }

  AdMenu* popupMenu = host->popupMenu.data();
  popupMenu->eventSink_ = QPointer<AdMenu>(sink);
  popupMenu->keyPathPrefix_ = mergeKeyPathWithPrefix(entry.keyPath);
  popupMenu->setMode(Mode::Vertical);
  popupMenu->setTheme(entry.popupTheme);
  popupMenu->setSelectable(selectable_);
  popupMenu->setMultiple(multiple_);
  popupMenu->setInlineCollapsed(false);
  popupMenu->setInlineIndent(inlineIndent_);
  popupMenu->setTriggerSubMenuAction(triggerSubMenuAction_);
  popupMenu->setSubMenuOpenDelayMs(subMenuOpenDelayMs_);
  popupMenu->setSubMenuCloseDelayMs(subMenuCloseDelayMs_);
  popupMenu->setTooltipEnabled(false);
  popupMenu->setTooltipPlacement(tooltipPlacement_);
  popupMenu->setOverflowedIndicatorText(overflowedIndicatorText_);
  popupMenu->setComponentTokens(componentTokens_);
  popupMenu->setSemanticStyles(semanticStyles_);
  popupMenu->setSemanticStyleResolver(semanticStyleResolver_);
  popupMenu->setPopupRender(popupRender_);
  popupMenu->setExpandIcon(expandIcon_);
  popupMenu->setPopupOffset(popupOffset_);
  popupMenu->setItemPaintHook(itemPaintHook_);
  popupMenu->setSubMenuPaintHook(subMenuPaintHook_);
  popupMenu->setItems(entry.item->children);
  popupMenu->setSelectedKeys(selectedKeys_);
  popupMenu->setOpenKeys(openKeys_);

  const bool customPopupRender = static_cast<bool>(popupRender_);
  const bool renderedRootIsPopupMenu = host->renderedRoot == popupMenu;
  bool needRebuildRenderedRoot = ownerChanged || !host->renderedRoot;
  if (!needRebuildRenderedRoot) {
    if (customPopupRender) {
      needRebuildRenderedRoot = host->activeKey != entry.key;
    } else {
      needRebuildRenderedRoot = !renderedRootIsPopupMenu;
    }
  }

  if (needRebuildRenderedRoot) {
    if (host->popupMenu) {
      host->popupMenu->setParent(host->popupWindow);
    }
    if (host->renderedRoot && host->renderedRoot != popupMenu) {
      host->renderedRoot->removeEventFilter(this);
      host->renderedRoot->deleteLater();
    }
    host->renderedRoot.clear();

    QWidget* renderedPopup = popupMenu;
    if (customPopupRender) {
      QWidget* defaultPopup = new QWidget(host->popupWindow);
      auto* defaultLayout = new QVBoxLayout(defaultPopup);
      defaultLayout->setContentsMargins(0, 0, 0, 0);
      defaultLayout->setSpacing(0);
      defaultLayout->addWidget(popupMenu);

      renderedPopup = defaultPopup;
      PopupRenderContext ctx;
      ctx.item = *entry.item;
      ctx.keyPath = mergeKeyPathWithPrefix(entry.keyPath);
      QWidget* custom = popupRender_(ctx, defaultPopup);
      if (custom) {
        renderedPopup = custom;
      }
      if (renderedPopup != defaultPopup &&
          defaultPopup->parentWidget() == host->popupWindow) {
        defaultPopup->deleteLater();
      }
    }

    if (renderedPopup->parentWidget() != host->popupWindow) {
      renderedPopup->setParent(host->popupWindow);
    }
    clearLayout(host->popupWindow->layout());
    host->popupWindow->layout()->addWidget(renderedPopup);
    host->renderedRoot = renderedPopup;
  }

  bindSharedPopupOwner(host, this);
  host->activeKey = entry.key;

  popupRecordCache_.popup = host->popupWindow;
  popupRecordCache_.popupMenu = host->popupMenu;
  popupRecordCache_.key = host->activeKey;
  popupRecordCache_.triggerRect = entry.rect;
  return &popupRecordCache_;
}

void AdMenu::positionPopup(const VisibleEntry& entry, PopupRecord& popupRecord) {
  if (!popupRecord.popup || !popupRecord.popup->parentWidget()) {
    return;
  }

  const QRect triggerRect = entry.rect;
  QWidget* scopeWindow = popupRecord.popup->parentWidget();
  const QPoint triggerTopLeft = mapTo(scopeWindow, triggerRect.topLeft());

  QPoint totalOffset = popupOffset_;
  if (entry.item) {
    totalOffset += entry.item->popupOffset;
  }

  popupRecord.popup->adjustSize();
  QSize popupSize = popupRecord.popup->sizeHint();
  if (!popupSize.isValid() || popupSize.isEmpty()) {
    popupSize = popupRecord.popup->size();
  }
  if (!popupSize.isValid() || popupSize.isEmpty()) {
    popupSize = QSize(220, 120);
  }

  QPoint preferredPos;
  if (mode_ == Mode::Horizontal) {
    QPoint downPos(triggerTopLeft.x(), triggerTopLeft.y() + triggerRect.height());
    QPoint upPos(triggerTopLeft.x(), triggerTopLeft.y() - popupSize.height());
    downPos += totalOffset;
    upPos += totalOffset;

    preferredPos = downPos;
    if (downPos.y() + popupSize.height() > scopeWindow->rect().bottom() + 1 &&
        upPos.y() >= scopeWindow->rect().top()) {
      preferredPos = upPos;
    }
  } else {
    QPoint rightPos(triggerTopLeft.x() + triggerRect.width(), triggerTopLeft.y());
    QPoint leftPos(triggerTopLeft.x() - popupSize.width(), triggerTopLeft.y());
    rightPos += totalOffset;
    leftPos += totalOffset;

    preferredPos = rightPos;
    if (rightPos.x() + popupSize.width() > scopeWindow->rect().right() + 1 &&
        leftPos.x() >= scopeWindow->rect().left()) {
      preferredPos = leftPos;
    }
  }

  // Final clamp keeps popup fully inside the host window content rect.
  const QRect bounds = scopeWindow->rect();
  const int minX = bounds.left();
  const int minY = bounds.top();
  const int maxX = std::max(minX, bounds.right() - popupSize.width() + 1);
  const int maxY = std::max(minY, bounds.bottom() - popupSize.height() + 1);

  preferredPos.setX(std::clamp(preferredPos.x(), minX, maxX));
  preferredPos.setY(std::clamp(preferredPos.y(), minY, maxY));

  popupRecord.popup->move(preferredPos);
}

void AdMenu::hidePopupAndDescendants(const QString& key) {
  SharedPopupHost* host = sharedPopupHostFor(this);
  if (!host || host->ownerMenu != this || !host->popupWindow) {
    return;
  }

  if (key.isEmpty()) {
    host->popupWindow->hide();
    host->activeKey.clear();
    return;
  }

  const QString activeKey = host->activeKey;
  if (activeKey.isEmpty()) {
    return;
  }
  if (activeKey == key || isDescendantSubMenuKey(activeKey, key)) {
    host->popupWindow->hide();
    host->activeKey.clear();
  }
}

void AdMenu::clearDanglingPopups() {
  SharedPopupHost* host = sharedPopupHostFor(this);
  const bool popupLikeMode = mode_ != Mode::Inline || inlineCollapsed_;

  if (host && host->ownerMenu == this && host->popupWindow) {
    if (openKeys_.isEmpty()) {
      host->popupWindow->hide();
      host->activeKey.clear();
    } else if (!host->activeKey.isEmpty() && !openKeys_.contains(host->activeKey)) {
      // Keep popup visible while switching hover target to avoid hide/show flicker.
      host->activeKey.clear();
    }
  }

  if (triggerSubMenuAction_ != TriggerSubMenuAction::Hover || !popupLikeMode) {
    return;
  }

  const QPoint cursorPos = QCursor::pos();
  const QRect menuGlobalRect(mapToGlobal(QPoint(0, 0)), size());
  const bool cursorInMenu = menuGlobalRect.contains(cursorPos);
  const QPoint cursorLocalPos = cursorInMenu ? mapFromGlobal(cursorPos) : QPoint();
  const int cursorEntryIndex = cursorInMenu ? entryIndexAt(cursorLocalPos) : -1;
  const bool cursorInPopup = host && host->ownerMenu == this && host->popupWindow &&
                             host->popupWindow->isVisible() &&
                             widgetContainsGlobalPos(host->popupWindow, cursorPos);

  if (cursorInPopup) {
    return;
  }

  if (cursorInMenu && cursorEntryIndex >= 0 && cursorEntryIndex < entries_.size()) {
    const VisibleEntry& hoveredEntry = entries_.at(cursorEntryIndex);
    if (!rowIsOpenable(hoveredEntry)) {
      if (!openKeys_.isEmpty()) {
        applyOpenInternal({}, true);
      }
      return;
    }

    const QString hoveredKey = hoveredEntry.key;
    QStringList next = openKeys_;
    for (int i = next.size() - 1; i >= 0; --i) {
      const QString key = next.at(i);
      if (key == hoveredKey || isDescendantSubMenuKey(hoveredKey, key)) {
        continue;
      }
      next.removeAt(i);
    }
    if (next != openKeys_) {
      applyOpenInternal(next, true);
    }
    return;
  }

  if (!openKeys_.isEmpty()) {
    applyOpenInternal({}, true);
  }
}

QStringList AdMenu::mergeKeyPathWithPrefix(const QStringList& localPath) const {
  QStringList merged = localPath;
  for (const QString& key : keyPathPrefix_) {
    if (!merged.contains(key)) {
      merged.append(key);
    }
  }
  return merged;
}

}  // namespace adqt::widgets
