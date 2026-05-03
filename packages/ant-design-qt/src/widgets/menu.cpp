#include "menu.h"

#include "detail/icon_utils.h"
#include "detail/timing_hub.h"
#include "icons.h"
#include "menu_style.h"
#include "popup_placement.h"
#include "theme/theme.h"
#include "tooltip.h"

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
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

namespace adqt::widgets {

namespace {

using detail::MenuStyleInput;
using detail::MenuVisualStyle;
namespace outlined_icons = adqt::icons::outlined;

constexpr int kAntdDropdownMinWidth = 160;
constexpr int kSubMenuArrowBoxWidth = 12;
constexpr int kSubMenuArrowBoxHeight = 14;
constexpr int kSubMenuArrowTextGap = 6;
constexpr char kHoverOpenTaskKey[] = "AdMenu.HoverOpen";
constexpr char kHoverCloseTaskKey[] = "AdMenu.HoverClose";
constexpr int kMenuIconPixmapCacheMaxEntries = 512;

struct IconPixmapCacheKey {
  int index = -1;
  QSize size;
  int dprMilli = 1000;
  int mode = 0;
  bool hasPrimary = false;
  bool hasSecondary = false;
  bool hasTertiary = false;
  QRgb primary = 0;
  QRgb secondary = 0;
  QRgb tertiary = 0;

  bool operator==(const IconPixmapCacheKey& other) const {
    return index == other.index && size == other.size && dprMilli == other.dprMilli &&
           mode == other.mode && hasPrimary == other.hasPrimary &&
           hasSecondary == other.hasSecondary && hasTertiary == other.hasTertiary &&
           primary == other.primary && secondary == other.secondary &&
           tertiary == other.tertiary;
  }
};

size_t qHash(const IconPixmapCacheKey& key, size_t seed) {
  return qHashMulti(seed,
                    key.index,
                    key.size.width(),
                    key.size.height(),
                    key.dprMilli,
                    key.mode,
                    key.hasPrimary,
                    key.hasSecondary,
                    key.hasTertiary,
                    key.primary,
                    key.secondary,
                    key.tertiary);
}

QHash<IconPixmapCacheKey, QPixmap>& menuIconPixmapCache() {
  static QHash<IconPixmapCacheKey, QPixmap> cache;
  return cache;
}

QPoint mouseEventPos(const QMouseEvent* event) {
  if (!event) {
    return QPoint();
  }
  return event->position().toPoint();
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

bool shouldShowSubMenuArrow(AdMenu::Mode mode,
                            bool inlineCollapsed,
                            AdMenu::ItemType type,
                            bool hasChildren) {
  if (type != AdMenu::ItemType::SubMenu || !hasChildren) {
    return false;
  }
  if (mode == AdMenu::Mode::Horizontal) {
    return false;
  }
  if (mode == AdMenu::Mode::Inline && inlineCollapsed) {
    return false;
  }
  return true;
}

AdTooltip::Placement collapsedMenuTooltipPlacement(const QWidget* widget) {
  if (widget && widget->layoutDirection() == Qt::RightToLeft) {
    return AdTooltip::Placement::Left;
  }
  return AdTooltip::Placement::Right;
}

QSize pixmapDeviceIndependentSize(const QPixmap& pixmap) {
  if (pixmap.isNull()) {
    return QSize();
  }
  const qreal dpr = pixmap.devicePixelRatio();
  if (dpr <= 0.0) {
    return pixmap.size();
  }
  return QSize(qRound(pixmap.width() / dpr), qRound(pixmap.height() / dpr));
}

void paintMenuIcon(QPainter& painter,
                   adqt::icons::IconToken icon,
                   const QRect& targetRect,
                   const QColor& color,
                   bool disabled) {
  if (!adqt::icons::isValid(icon) || !targetRect.isValid()) {
    return;
  }

  icon = detail::iconWithInheritedColor(icon, color);
  const qreal dpr = painter.device() ? painter.device()->devicePixelRatioF() : 1.0;
  const QIcon::Mode mode = disabled ? QIcon::Disabled : QIcon::Normal;
  IconPixmapCacheKey cacheKey;
  cacheKey.index = icon.index;
  cacheKey.size = targetRect.size();
  cacheKey.dprMilli = std::max(1, qRound(dpr * 1000.0));
  cacheKey.mode = static_cast<int>(mode);
  cacheKey.hasPrimary = icon.style.hasPrimary;
  cacheKey.hasSecondary = icon.style.hasSecondary;
  cacheKey.hasTertiary = icon.style.hasTertiary;
  cacheKey.primary = icon.style.primary.rgba();
  cacheKey.secondary = icon.style.secondary.rgba();
  cacheKey.tertiary = icon.style.tertiary.rgba();

  QPixmap pixmap;
  auto& cache = menuIconPixmapCache();
  const auto cachedIt = cache.constFind(cacheKey);
  if (cachedIt != cache.cend()) {
    pixmap = cachedIt.value();
  } else {
    pixmap = adqt::icons::renderIconPixmap(icon, targetRect.size(), dpr, mode, QIcon::Off);
    if (!pixmap.isNull()) {
      if (cache.size() >= kMenuIconPixmapCacheMaxEntries) {
        cache.clear();
      }
      cache.insert(cacheKey, pixmap);
    }
  }
  if (pixmap.isNull()) {
    return;
  }

  const QSize drawSize = pixmapDeviceIndependentSize(pixmap);
  const QPoint drawTopLeft(targetRect.x() + (targetRect.width() - drawSize.width()) / 2,
                           targetRect.y() + (targetRect.height() - drawSize.height()) / 2);
  painter.drawPixmap(drawTopLeft, pixmap);
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

int rootBorderWidthForStyle(const AdMenu* menu, const MenuVisualStyle& style) {
  if (!menu) {
    return std::max(0, style.metrics.borderWidth);
  }
  if (menu->mode() == AdMenu::Mode::Horizontal && menu->theme() == AdMenu::MenuTheme::Dark) {
    // Match antd dark horizontal mode: no root bottom border.
    return 0;
  }
  return std::max(0, style.metrics.borderWidth);
}

struct PopupLayer {
  QPointer<QWidget> popupWindow;
  QPointer<AdMenu> popupMenu;
  QPointer<QWidget> renderedRoot;
  QString activeKey;
};

struct SharedPopupHost {
  QPointer<QWidget> scopeWindow;
  QPointer<AdMenu> ownerMenu;
  QVector<PopupLayer*> layers;
};

QHash<QWidget*, SharedPopupHost*>& sharedPopupHosts() {
  static QHash<QWidget*, SharedPopupHost*> hosts;
  return hosts;
}

void detachSharedPopupOwner(SharedPopupHost* host) {
  if (!host || !host->ownerMenu) {
    return;
  }

  AdMenu* owner = host->ownerMenu.data();
  for (PopupLayer* layer : host->layers) {
    if (!layer) {
      continue;
    }
    if (layer->popupWindow) {
      layer->popupWindow->removeEventFilter(owner);
    }
    if (layer->popupMenu) {
      layer->popupMenu->removeEventFilter(owner);
    }
    if (layer->renderedRoot) {
      layer->renderedRoot->removeEventFilter(owner);
    }
  }
  host->ownerMenu.clear();
}

void hidePopupLayersFrom(SharedPopupHost* host, int fromLayerIndex) {
  if (!host) {
    return;
  }
  const int start = std::max(0, fromLayerIndex);
  for (int i = start; i < host->layers.size(); ++i) {
    PopupLayer* layer = host->layers.at(i);
    if (!layer) {
      continue;
    }
    if (layer->popupWindow) {
      layer->popupWindow->hide();
    }
    layer->activeKey.clear();
  }
}

bool anyPopupLayerVisible(const SharedPopupHost* host) {
  if (!host) {
    return false;
  }
  for (const PopupLayer* layer : host->layers) {
    if (layer && layer->popupWindow && layer->popupWindow->isVisible()) {
      return true;
    }
  }
  return false;
}

bool popupLayerContainsGlobalPos(const PopupLayer* layer, const QPoint& globalPos) {
  if (!layer || !layer->popupWindow || !layer->popupWindow->isVisible()) {
    return false;
  }
  return widgetContainsGlobalPos(layer->popupWindow, globalPos);
}

bool anyPopupLayerContainsGlobalPos(const SharedPopupHost* host, const QPoint& globalPos) {
  if (!host) {
    return false;
  }
  for (const PopupLayer* layer : host->layers) {
    if (popupLayerContainsGlobalPos(layer, globalPos)) {
      return true;
    }
  }
  return false;
}

void destroyPopupLayer(PopupLayer* layer) {
  if (!layer) {
    return;
  }
  if (layer->popupWindow) {
    layer->popupWindow->hide();
    layer->popupWindow->deleteLater();
  }
  delete layer;
}

PopupLayer* ensurePopupLayer(SharedPopupHost* host, int layerIndex) {
  if (!host || !host->scopeWindow || layerIndex < 0) {
    return nullptr;
  }

  while (host->layers.size() <= layerIndex) {
    host->layers.append(new PopupLayer());
  }

  PopupLayer* layer = host->layers.at(layerIndex);
  if (!layer) {
    layer = new PopupLayer();
    host->layers[layerIndex] = layer;
  }

  if (!layer->popupWindow || !layer->popupMenu) {
    if (layer->popupWindow) {
      layer->popupWindow->hide();
      layer->popupWindow->deleteLater();
    }
    layer->popupWindow.clear();
    layer->popupMenu.clear();
    layer->renderedRoot.clear();
    layer->activeKey.clear();

    QWidget* popupWindow = new QWidget(host->scopeWindow);
    popupWindow->setAttribute(Qt::WA_DeleteOnClose, false);
    popupWindow->setProperty("adqt.interaction.surface", true);
    popupWindow->setObjectName(layerIndex == 0
                                   ? QStringLiteral("admenu-shared-popup-window")
                                   : QStringLiteral("admenu-shared-popup-window-%1").arg(layerIndex));
    auto* layout = new QVBoxLayout(popupWindow);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto* popupMenu = new AdMenu(popupWindow);
    popupMenu->setMode(AdMenu::Mode::Vertical);
    popupMenu->setInlineCollapsed(false);
    popupMenu->setTooltipEnabled(false);
    popupMenu->setAttribute(Qt::WA_Hover, false);

    layer->popupWindow = popupWindow;
    layer->popupMenu = popupMenu;
  }

  return layer;
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
  for (PopupLayer* layer : host->layers) {
    destroyPopupLayer(layer);
  }
  host->layers.clear();
  delete host;
}

SharedPopupHost* sharedPopupHostFor(const AdMenu* menu) {
  QWidget* scopeWindow = detail::resolvePopupScopeWindow(menu);
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
  for (PopupLayer* layer : host->layers) {
    if (!layer) {
      continue;
    }
    if (layer->popupWindow) {
      layer->popupWindow->removeEventFilter(owner);
      layer->popupWindow->installEventFilter(owner);
    }
    if (layer->popupMenu) {
      layer->popupMenu->removeEventFilter(owner);
      layer->popupMenu->installEventFilter(owner);
    }
    if (layer->renderedRoot) {
      layer->renderedRoot->removeEventFilter(owner);
      layer->renderedRoot->installEventFilter(owner);
    }
  }
}

SharedPopupHost* ensureSharedPopupHost(AdMenu* ownerMenu) {
  QWidget* scopeWindow = detail::resolvePopupScopeWindow(ownerMenu);
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

  connect(&theme::ThemeManager::instance(), &theme::ThemeManager::themeChanged, this, [this]() {
    update();
  });
}

AdMenu::~AdMenu() {
  detail::cancelTimingTask(this, QString::fromLatin1(kHoverOpenTaskKey));
  detail::cancelTimingTask(this, QString::fromLatin1(kHoverCloseTaskKey));
  pendingHoverOpenKey_.clear();
  hideTooltip();

  SharedPopupHost* host = sharedPopupHostFor(this);
  if (host && host->ownerMenu == this) {
    bindSharedPopupOwner(host, nullptr);
    hidePopupLayersFrom(host, 0);
  }
  detail::setInWindowPopupHostOpen(this, false);
}

AdMenu::Mode AdMenu::mode() const { return mode_; }

void AdMenu::setMode(Mode value) {
  if (mode_ == value) {
    return;
  }
  const Mode previousMode = mode_;
  const bool previousInlineCollapsed = inlineCollapsed_;
  mode_ = value;
  hoveredEntry_ = -1;
  pressedEntry_ = -1;
  hideTooltip();
  syncOpenKeysForModeTransition(previousMode, previousInlineCollapsed);
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
    hidePopupLayersFrom(host, 0);
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
  const Mode previousMode = mode_;
  const bool previousInlineCollapsed = inlineCollapsed_;
  inlineCollapsed_ = value;
  hoveredEntry_ = -1;
  pressedEntry_ = -1;
  hideTooltip();
  syncOpenKeysForModeTransition(previousMode, previousInlineCollapsed);
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
  value = std::max(-1, value);
  if (subMenuOpenDelayMs_ == value) {
    return;
  }
  subMenuOpenDelayMs_ = value;
  syncPopupVisibility();
  emit subMenuOpenDelayMsChanged(subMenuOpenDelayMs_);
}

int AdMenu::subMenuCloseDelayMs() const { return subMenuCloseDelayMs_; }

void AdMenu::setSubMenuCloseDelayMs(int value) {
  value = std::max(-1, value);
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
    hideTooltip();
  } else {
    syncTooltipForHoveredEntry();
  }
  syncPopupVisibility();
  emit tooltipEnabledChanged(tooltipEnabled_);
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
    hidePopupLayersFrom(host, 0);
  }
  emit popupRenderChanged();
  syncPopupVisibility();
}

adqt::icons::IconToken AdMenu::expandIcon() const { return expandIcon_; }

void AdMenu::setExpandIcon(const adqt::icons::IconToken& icon) {
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
    const int h = style.metrics.horizontalLineHeight;
    const int contentWidth = horizontalContentWidthHint();
    const int preferredWidth = std::max(160, contentWidth);
    const int rootBorderWidth = rootBorderWidthForStyle(this, style);
    return QSize(std::max(width(), preferredWidth), h + rootBorderWidth);
  }
  const bool popupLayer = (mode_ == Mode::Vertical && eventSink_ && eventSink_.data() != this);
  if (popupLayer) {
    // Ant Design popup menu uses dropdown min-width behavior (default 160),
    // instead of content-driven width expansion.
    const int popupWidth = kAntdDropdownMinWidth;
    const int popupHeight = std::max(contentHeight_, style.metrics.itemHeight * 2);
    return QSize(popupWidth, popupHeight);
  }
  const int w = inlineCollapsed_ && mode_ == Mode::Inline ? 56 : 256;
  // Match Ant Design root menu behavior: shrink to content height instead of
  // forcing a 4-row minimum, which can leave extra blank space at the bottom
  // when top-level inline submenus are all collapsed.
  const int minRootHeight = style.metrics.itemHeight + style.metrics.itemMarginBlock * 2;
  const int h = std::max(contentHeight_, minRootHeight);
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
    const int h = style.metrics.horizontalLineHeight;
    const int rootBorderWidth = rootBorderWidthForStyle(this, style);
    return QSize(std::max(160, horizontalContentWidthHint()), h + rootBorderWidth);
  }
  const bool popupLayer = (mode_ == Mode::Vertical && eventSink_ && eventSink_.data() != this);
  if (popupLayer) {
    const int minHeight = style.metrics.itemHeight + style.metrics.itemMarginBlock * 2;
    return QSize(kAntdDropdownMinWidth, minHeight);
  }
  const int w = inlineCollapsed_ && mode_ == Mode::Inline ? 48 : 120;
  return QSize(w, style.metrics.itemHeight + style.metrics.itemMarginBlock * 2);
}

void AdMenu::paintEvent(QPaintEvent* event) {
  StyleContext ctx;
  ctx.mode = mode_;
  ctx.theme = theme_;
  ctx.inlineCollapsed = inlineCollapsed_;
  ctx.items = items_;
  const SemanticStyles effectiveSemantic =
      semanticStyleResolver_ ? semanticStyleResolver_(ctx) : semanticStyles_;
  MenuVisualStyle style = resolveVisualStyle(this, effectiveSemantic);
  QPainter painter(this);
  // Keep antialiasing off for frequent row repaints; enable it only where
  // rounded popup chrome actually needs it.
  painter.setRenderHint(QPainter::Antialiasing, false);
  painter.setFont(style.metrics.font);
  const QRect dirtyRect = (event && event->rect().isValid()) ? event->rect() : rect();

  const bool popupLayer = (mode_ == Mode::Vertical && eventSink_ && eventSink_.data() != this);
  QPainterPath popupPath;
  bool clipRowsToPopupPath = false;
  qreal popupRadius = 0.0;
  qreal popupBorderWidth = 0.0;
  if (popupLayer) {
    popupRadius = std::max<qreal>(0.0, static_cast<qreal>(style.metrics.popupBorderRadius));
    popupBorderWidth = std::max<qreal>(0.0, static_cast<qreal>(style.metrics.borderWidth));

    const QRect repaintRect = dirtyRect.intersected(rect());
    const int cornerExtent =
        std::max(0, static_cast<int>(std::ceil(std::max<qreal>(popupRadius, popupBorderWidth))));
    const bool touchesRoundedRows =
        cornerExtent > 0 && repaintRect.isValid() &&
        (repaintRect.top() < cornerExtent ||
         repaintRect.bottom() > rect().height() - cornerExtent - 1);

    if (repaintRect.isValid()) {
      if (touchesRoundedRows) {
        const qreal inset = popupBorderWidth > 0.0 ? popupBorderWidth * 0.5 : 0.5;
        const QRectF popupRect = QRectF(rect()).adjusted(inset, inset, -inset, -inset);
        popupPath.addRoundedRect(popupRect, popupRadius, popupRadius);

        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setClipRect(repaintRect);
        painter.fillPath(popupPath, style.popupBackground);
        painter.restore();
        clipRowsToPopupPath = true;
      } else {
        painter.fillRect(repaintRect, style.popupBackground);
      }
    }

    if (clipRowsToPopupPath) {
      painter.save();
      painter.setClipPath(popupPath);
    }
  } else {
    painter.fillRect(rect(), style.menuBackground);
  }

  if (!popupLayer && mode_ == Mode::Inline && !inlineCollapsed_ && style.subMenuBackground.alpha() > 0) {
    painter.save();
    painter.setPen(Qt::NoPen);
    painter.setBrush(style.subMenuBackground);
    for (const QRect& subMenuRect : inlineSubMenuBackgroundRects_) {
      const QRect clippedRect = subMenuRect.intersected(rect()).intersected(dirtyRect);
      if (clippedRect.width() <= 0 || clippedRect.height() <= 0) {
        continue;
      }
      painter.drawRect(clippedRect);
    }
    painter.restore();
  }

  const int rootBorderWidth = rootBorderWidthForStyle(this, style);
  if (!popupLayer && rootBorderWidth > 0) {
    painter.setPen(QPen(style.borderColor, rootBorderWidth));
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
    if (!rowRect.intersects(dirtyRect)) {
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
      QFont groupFont = style.metrics.font;
      groupFont.setBold(false);
      groupFont.setPixelSize(std::max(10, style.metrics.groupTitleFontSize));
      painter.setFont(groupFont);
      QRect textRect = rowRect.adjusted(style.metrics.groupTitleHorizontalPadding,
                                        style.metrics.groupTitleVerticalPadding,
                                        -style.metrics.groupTitleHorizontalPadding,
                                        -style.metrics.groupTitleVerticalPadding);
      painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft,
                       trimmedOrFallback(entry.item->label, entry.item->key));
      continue;
    }

    const bool hovered = (hoveredEntry_ == i);
    const bool pressed = (pressedEntry_ == i);
    const bool itemSelected = selectedKeys_.contains(entry.key);
    const bool subMenuSelected =
        entry.type == ItemType::SubMenu && selectedSubMenuKeys_.contains(entry.key);
    const bool selected = itemSelected || subMenuSelected;
    const bool opened = openKeys_.contains(entry.key);

    detail::MenuStateStyle state = style.normal;
    if (entry.disabled) {
      state = style.disabled;
    } else if (horizontal) {
      state = style.horizontalNormal;
      if (entry.type == ItemType::SubMenu && subMenuSelected) {
        // Match antd: submenu-selected keeps submenu title selected color.
        // Horizontal selected state still controls active bar/background.
        state = style.horizontalSelected;
        if (style.subMenuItemSelectedColor.isValid()) {
          state.text = style.subMenuItemSelectedColor;
        } else if (style.selected.text.isValid()) {
          state.text = style.selected.text;
        }
      } else if (selected) {
        state = style.horizontalSelected;
      } else if (entry.danger) {
        if (pressed) {
          state = style.dangerActive;
        } else if (hovered) {
          state = style.dangerHover;
        } else {
          state = style.danger;
        }
      } else if (pressed) {
        state = style.horizontalActive;
      } else if (hovered) {
        state = style.horizontalHover;
      }
    } else if (entry.type == ItemType::SubMenu && subMenuSelected) {
      // Match Ant Design's `submenu-selected`: keep selected text color on title,
      // but do not apply item-level selected background in inline/vertical modes.
      if (entry.danger) {
        if (pressed) {
          state = style.dangerActive;
        } else if (hovered) {
          state = style.dangerHover;
        } else {
          state = style.danger;
        }
        if (style.dangerSelected.text.isValid()) {
          state.text = style.dangerSelected.text;
        }
      } else {
        if (pressed) {
          state = style.active;
        } else if (hovered) {
          state = style.hover;
        } else {
          state = style.normal;
        }
        if (style.subMenuItemSelectedColor.isValid()) {
          state.text = style.subMenuItemSelectedColor;
        } else if (style.selected.text.isValid()) {
          state.text = style.selected.text;
        }
      }
    } else if (entry.danger) {
      if (itemSelected) {
        state = style.dangerSelected;
      } else if (pressed) {
        state = style.dangerActive;
      } else if (hovered) {
        state = style.dangerHover;
      } else {
        state = style.danger;
      }
    } else if (itemSelected) {
      state = style.selected;
    } else if (pressed) {
      state = style.active;
    } else if (hovered) {
      state = style.hover;
    }

    QRect fillRect = rowRect.adjusted(style.metrics.itemMarginInline, 0, -style.metrics.itemMarginInline, 0);
    if (horizontal) {
      fillRect = rowRect;
    }

    int radius = entry.type == ItemType::SubMenu ? style.metrics.subMenuItemBorderRadius
                                                  : style.metrics.itemBorderRadius;
    if (popupLayer) {
      // Match antd submenu popup: both items and submenu titles use subMenuItemBorderRadius.
      radius = style.metrics.subMenuItemBorderRadius;
    }
    if (horizontal) {
      radius = style.metrics.horizontalItemBorderRadius;

      // Ant Design horizontal mode keeps active/hover/open background transparent by default.
      // A custom `horizontalItemHoverBg` token can still opt into a filled background.
      if (!entry.disabled && !selected && (hovered || pressed || opened)) {
        state.background = style.horizontalHover.background;
      }
    }
    if (state.background.alpha() > 0) {
      painter.setPen(Qt::NoPen);
      painter.setBrush(state.background);
      painter.drawRoundedRect(fillRect, radius, radius);
    }

    if (style.metrics.activeBarWidth > 0 && itemSelected && !horizontal && !collapsedInline) {
      QRect activeBar(fillRect.left(), fillRect.top(), style.metrics.activeBarWidth, fillRect.height());
      painter.setPen(Qt::NoPen);
      painter.setBrush(state.text);
      painter.drawRoundedRect(activeBar, style.metrics.activeBarWidth / 2.0,
                              style.metrics.activeBarWidth / 2.0);
    }
    const int horizontalActiveBarHeight = style.metrics.activeBarHeight;
    const bool horizontalActiveBar =
        horizontal && horizontalActiveBarHeight > 0 && !entry.disabled &&
        (selected || pressed || hovered || opened);
    if (horizontalActiveBar) {
      const QColor activeBarColor = style.horizontalSelected.text.isValid()
                                        ? style.horizontalSelected.text
                                        : state.text;
      const int activeBarLeft = fillRect.left() + style.metrics.itemPaddingInline;
      const int activeBarRight = fillRect.right() - style.metrics.itemPaddingInline;
      const int activeBarWidth = activeBarRight - activeBarLeft + 1;
      const int activeBarHeight = horizontalActiveBarHeight;
      const int activeBarTop = rowRect.bottom() - activeBarHeight + 1;
      if (activeBarWidth > 0) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(activeBarColor);
        painter.drawRect(QRect(activeBarLeft, activeBarTop, activeBarWidth, activeBarHeight));
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
    const bool hasIcon = adqt::icons::isValid(entry.item->icon);
    QRect iconRect(contentRect.left(),
                   contentRect.center().y() - iconSide / 2,
                   iconSide,
                   iconSide);

    int textLeft = contentRect.left();
    if (hasIcon) {
      paintMenuIcon(painter, entry.item->icon, iconRect, state.text, entry.disabled);
      textLeft = iconRect.right() + 1 + style.metrics.iconMarginInlineEnd;
    } else if (collapsedInline) {
      textLeft = contentRect.left();
    }

    QRect arrowRect(contentRect.right() - kSubMenuArrowBoxWidth,
                    contentRect.center().y() - kSubMenuArrowBoxHeight / 2,
                    kSubMenuArrowBoxWidth,
                    kSubMenuArrowBoxHeight);

    const bool showArrow = shouldShowSubMenuArrow(mode_, inlineCollapsed_, entry.type, entry.hasChildren);

    int textRight = contentRect.right();
    if (showArrow) {
      textRight = arrowRect.left() - kSubMenuArrowTextGap;
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
    painter.setFont(style.metrics.font);
    QRect textRect(textLeft, contentRect.top(), std::max(0, textRight - textLeft), contentRect.height());
    painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, label);

    if (showArrow) {
      if (adqt::icons::isValid(expandIcon_)) {
        paintMenuIcon(painter, expandIcon_, arrowRect, state.text, entry.disabled);
      } else {
        const bool inlineMode = (mode_ == Mode::Inline && !inlineCollapsed_);
        const bool open = openKeys_.contains(entry.key);
        const bool useUpOutlined = inlineMode && open;
        const adqt::icons::IconToken defaultArrow =
            inlineMode ? (useUpOutlined ? outlined_icons::Up() : outlined_icons::Down())
                       : outlined_icons::Right();
        paintMenuIcon(painter,
                      defaultArrow,
                      arrowRect,
                      state.text,
                      entry.disabled);
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
    if (clipRowsToPopupPath) {
      painter.restore();
    }
    if (style.metrics.borderWidth > 0 && style.popupBorderColor.alpha() > 0) {
      const QRect repaintRect = dirtyRect.intersected(rect());
      const int borderInset =
          std::max(0, static_cast<int>(std::ceil(std::max<qreal>(popupRadius, popupBorderWidth))));
      const bool touchesBorder =
          !repaintRect.isValid() || borderInset <= 0 ||
          repaintRect.top() < borderInset ||
          repaintRect.bottom() > rect().height() - borderInset - 1 ||
          repaintRect.left() < borderInset ||
          repaintRect.right() > rect().width() - borderInset - 1;
      if (touchesBorder) {
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(QPen(style.popupBorderColor, style.metrics.borderWidth));
        painter.setBrush(Qt::NoBrush);
        const qreal inset = popupBorderWidth * 0.5;
        const QRectF borderRect = QRectF(rect()).adjusted(inset, inset, -inset, -inset);
        painter.drawRoundedRect(borderRect, popupRadius, popupRadius);
        painter.restore();
      }
    }
  }
}

void AdMenu::enterEvent(QEnterEvent* event) {
  QWidget::enterEvent(event);
  const QPoint localCursor = mapFromGlobal(QCursor::pos());
  if (rect().contains(localCursor)) {
    detail::cancelTimingTask(this, QString::fromLatin1(kHoverCloseTaskKey));
  }
  syncTooltipForHoveredEntry();
}

void AdMenu::leaveEvent(QEvent* event) {
  QWidget::leaveEvent(event);
  setHoveredEntry(-1);
  unsetCursor();

  const bool popupLikeMode = mode_ == Mode::Vertical || mode_ == Mode::Horizontal ||
                             (mode_ == Mode::Inline && inlineCollapsed_);
  if (triggerSubMenuAction_ == TriggerSubMenuAction::Hover && popupLikeMode) {
    requestHoverOpen(QString());
    requestHoverClose();
  }
}

void AdMenu::mouseMoveEvent(QMouseEvent* event) {
  const int previousHoveredEntry = hoveredEntry_;
  bool previousHoveredWasOpenable = false;
  if (previousHoveredEntry >= 0 && previousHoveredEntry < entries_.size()) {
    previousHoveredWasOpenable = rowIsOpenable(entries_.at(previousHoveredEntry));
  }

  const int index = entryIndexAt(mouseEventPos(event));
  const bool hoveredEntryChanged = (hoveredEntry_ != index);
  setHoveredEntry(index);
  const bool popupLayer = (mode_ == Mode::Vertical && eventSink_ && eventSink_.data() != this);

  auto applyCursorShape = [this](Qt::CursorShape shape) {
    if (!testAttribute(Qt::WA_SetCursor) || cursor().shape() != shape) {
      setCursor(shape);
    }
  };

  if (index >= 0 && index < entries_.size()) {
    const VisibleEntry& entry = entries_.at(index);
    const bool cursorEligible = entry.type == ItemType::Item || entry.type == ItemType::SubMenu;
    if (cursorEligible) {
      applyCursorShape(entry.disabled ? Qt::ForbiddenCursor : Qt::PointingHandCursor);
    } else if (testAttribute(Qt::WA_SetCursor)) {
      unsetCursor();
    }
  } else if (testAttribute(Qt::WA_SetCursor)) {
    unsetCursor();
  }

  const bool popupLikeMode = mode_ == Mode::Vertical || mode_ == Mode::Horizontal ||
                             (mode_ == Mode::Inline && inlineCollapsed_);
  const bool hoverPopupMode =
      triggerSubMenuAction_ == TriggerSubMenuAction::Hover && popupLikeMode;
  if (hoverPopupMode && popupLayer) {
    AdMenu* sink = (eventSink_ && eventSink_.data() != this) ? eventSink_.data() : this;
    if (sink) {
      detail::cancelTimingTask(sink, QString::fromLatin1(kHoverCloseTaskKey));
    }
  }

  bool hitOpenable = false;
  if (index >= 0 && index < entries_.size()) {
    const VisibleEntry& entry = entries_.at(index);
    hitOpenable = rowIsOpenable(entry);
    if (hoverPopupMode && hitOpenable) {
      detail::cancelTimingTask(this, QString::fromLatin1(kHoverCloseTaskKey));
      if (hoveredEntryChanged) {
        requestHoverOpen(entry.key);
      }
    }
  }

  if (hoverPopupMode && !hitOpenable) {
    if (hoveredEntryChanged) {
      if (!popupLayer || previousHoveredWasOpenable) {
        requestHoverOpen(QString());
      }
      if (!openKeys_.isEmpty() && !popupLayer) {
        requestHoverClose();
      }
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
  setHoveredEntry(pressedEntry_);
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
  if (!event) {
    return;
  }

  switch (event->type()) {
    case QEvent::EnabledChange:
      syncTooltipForHoveredEntry();
      update();
      break;
    case QEvent::FontChange:
      rebuildEntries();
      syncPopupVisibility();
      syncTooltipForHoveredEntry();
      updateGeometry();
      update();
      break;
    case QEvent::LayoutDirectionChange:
      syncTooltipForHoveredEntry();
      break;
    default:
      break;
  }
}

void AdMenu::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  rebuildEntries();
  syncPopupVisibility();
  syncTooltipForHoveredEntry();
}

bool AdMenu::eventFilter(QObject* watched, QEvent* event) {
  if (!watched || !event) {
    return QWidget::eventFilter(watched, event);
  }

  SharedPopupHost* host = sharedPopupHostFor(this);
  if (host && host->ownerMenu == this) {
    PopupLayer* watchedLayer = nullptr;
    const bool fromSharedPopup = [&]() {
      for (PopupLayer* layer : host->layers) {
        if (!layer) {
          continue;
        }
        if (watched == layer->popupWindow.data() || watched == layer->popupMenu.data() ||
            watched == layer->renderedRoot.data()) {
          watchedLayer = layer;
          return true;
        }
      }
      return false;
    }();

    if (fromSharedPopup && event->type() == QEvent::Destroy) {
      if (watchedLayer) {
        if (watched == watchedLayer->renderedRoot.data()) {
          watchedLayer->renderedRoot.clear();
        }
        if (watched == watchedLayer->popupMenu.data()) {
          watchedLayer->popupMenu.clear();
        }
        if (watched == watchedLayer->popupWindow.data()) {
          watchedLayer->popupWindow.clear();
          watchedLayer->popupMenu.clear();
          watchedLayer->renderedRoot.clear();
          watchedLayer->activeKey.clear();
        }
      }
    } else if (fromSharedPopup && event->type() == QEvent::Hide && watchedLayer &&
               watched == watchedLayer->popupWindow.data()) {
      watchedLayer->activeKey.clear();
    } else if (fromSharedPopup && event->type() == QEvent::Enter) {
      detail::cancelTimingTask(this, QString::fromLatin1(kHoverCloseTaskKey));
    } else if (fromSharedPopup && event->type() == QEvent::Leave &&
               triggerSubMenuAction_ == TriggerSubMenuAction::Hover &&
               (mode_ == Mode::Vertical || mode_ == Mode::Horizontal ||
                (mode_ == Mode::Inline && inlineCollapsed_))) {
      // Do not gate close scheduling on QCursor::pos() during Leave handling.
      // Cursor position can be transiently stale in event dispatch order and
      // skip scheduling, which leaves a dangling popup until another event.
      requestHoverClose();
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
  rebuildSelectedSubMenuKeys();

  entries_.clear();
  inlineSubMenuBackgroundRects_.clear();
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
  int trailingBlockMargin = 0;

  if (mode_ == Mode::Horizontal) {
    appendHorizontalEntries(items_, cursorX);
    const int rootBorderWidth = rootBorderWidthForStyle(this, style);
    contentHeight_ = style.metrics.horizontalLineHeight + rootBorderWidth;
  } else if (mode_ == Mode::Inline && !inlineCollapsed_) {
    appendInlineEntries(items_, 0, {}, cursorY, trailingBlockMargin);
    contentHeight_ = cursorY + trailingBlockMargin + style.metrics.borderWidth;
  } else {
    appendVerticalEntries(items_, 0, {}, cursorY, true, trailingBlockMargin);
    contentHeight_ = cursorY + trailingBlockMargin + style.metrics.borderWidth;
  }

  syncTooltipForHoveredEntry();
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

void AdMenu::rebuildSelectedSubMenuKeys() {
  selectedSubMenuKeys_.clear();
  if (items_.isEmpty() || selectedKeys_.isEmpty()) {
    return;
  }

  QSet<QString> selectedItemKeys;
  selectedItemKeys.reserve(selectedKeys_.size());
  for (const QString& key : selectedKeys_) {
    if (!key.isEmpty()) {
      selectedItemKeys.insert(key);
    }
  }
  if (selectedItemKeys.isEmpty()) {
    return;
  }

  collectSelectedSubMenuKeysRecursive(items_, selectedItemKeys, selectedSubMenuKeys_);
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
  inlineCacheOpenKeys_ = openKeys_;
}

bool AdMenu::shouldShowPopupForEntry(const VisibleEntry& entry) const {
  if (!entry.rect.isValid()) {
    return false;
  }
  if (!isVisible()) {
    return false;
  }
  if (!visibleRegion().intersects(entry.rect)) {
    return false;
  }

  QWidget* scopeWindow = detail::resolvePopupScopeWindow(this);
  if (!scopeWindow) {
    return false;
  }

  const QRect triggerRectInScope(mapTo(scopeWindow, entry.rect.topLeft()), entry.rect.size());
  if (!triggerRectInScope.intersects(scopeWindow->rect())) {
    return false;
  }

  const QRect triggerRectGlobal(mapToGlobal(entry.rect.topLeft()), entry.rect.size());
  const QRect scopeGlobalRect = widgetGlobalRect(scopeWindow);
  if (!scopeGlobalRect.isValid() || !triggerRectGlobal.intersects(scopeGlobalRect)) {
    return false;
  }

  return true;
}

void AdMenu::syncPopupVisibility() {
  if (eventSink_ && eventSink_.data() != this) {
    return;
  }

  const auto finalizeHostState = [this]() { syncInWindowPopupHostState(); };
  SharedPopupHost* host = sharedPopupHostFor(this);
  if (mode_ == Mode::Inline && !inlineCollapsed_) {
    if (host && host->ownerMenu == this) {
      hidePopupLayersFrom(host, 0);
    }
    finalizeHostState();
    return;
  }

  if (mode_ == Mode::Inline && inlineCollapsed_) {
    const bool hasVisiblePopupLayer =
        host && host->ownerMenu == this && anyPopupLayerVisible(host);
    bool hasUserInteractionIntent = !pendingHoverOpenKey_.isEmpty();
    if (!hasUserInteractionIntent && hoveredEntry_ >= 0 && hoveredEntry_ < entries_.size()) {
      hasUserInteractionIntent = rowIsOpenable(entries_.at(hoveredEntry_));
    }
    if (!hasUserInteractionIntent && !hasVisiblePopupLayer) {
      if (host && host->ownerMenu == this) {
        hidePopupLayersFrom(host, 0);
      }
      finalizeHostState();
      return;
    }
  }

  if (openKeys_.isEmpty()) {
    if (host && host->ownerMenu == this) {
      hidePopupLayersFrom(host, 0);
    }
    finalizeHostState();
    return;
  }

  QStringList activeChain;
  for (int i = openKeys_.size() - 1; i >= 0; --i) {
    const QString key = openKeys_.at(i);
    if (key.isEmpty() || !validSubMenuKeys_.contains(key)) {
      continue;
    }

    QStringList path;
    QString current = key;
    while (!current.isEmpty()) {
      path.prepend(current);
      current = subMenuParents_.value(current);
    }
    if (path.isEmpty()) {
      continue;
    }

    bool allOpened = true;
    for (const QString& pathKey : path) {
      if (!openKeys_.contains(pathKey)) {
        allOpened = false;
        break;
      }
    }
    if (!allOpened) {
      continue;
    }

    activeChain = path;
    break;
  }

  if (activeChain.isEmpty()) {
    if (host && host->ownerMenu == this) {
      hidePopupLayersFrom(host, 0);
    }
    finalizeHostState();
    return;
  }

  host = ensureSharedPopupHost(this);
  if (!host || host->ownerMenu != this) {
    finalizeHostState();
    return;
  }

  int shownLayers = 0;
  AdMenu* anchorMenu = this;
  for (const QString& key : activeChain) {
    if (!anchorMenu) {
      break;
    }

    const int index = anchorMenu->entryIndexByKey(key);
    if (index < 0 || index >= anchorMenu->entries_.size()) {
      break;
    }

    const VisibleEntry& entry = anchorMenu->entries_.at(index);
    if (!anchorMenu->rowIsOpenable(entry) || !anchorMenu->shouldShowPopupForEntry(entry)) {
      break;
    }

    PopupRecord* record = anchorMenu->ensurePopupForEntry(entry);
    if (!record || !record->popup || !record->popupMenu) {
      break;
    }

    anchorMenu->positionPopup(entry, *record);
    record->popup->show();
    record->popup->raise();

    anchorMenu = record->popupMenu.data();
    ++shownLayers;
  }

  hidePopupLayersFrom(host, shownLayers);
  finalizeHostState();
}

void AdMenu::syncTooltipForHoveredEntry() {
  if (!tooltipEnabled_ || !(mode_ == Mode::Inline && inlineCollapsed_)) {
    hideTooltip();
    return;
  }
  if (hoveredEntry_ < 0 || hoveredEntry_ >= entries_.size()) {
    hideTooltip();
    return;
  }

  const VisibleEntry& entry = entries_.at(hoveredEntry_);
  const QString text = tooltipTextForEntry(entry).trimmed();
  if (text.isEmpty()) {
    hideTooltip();
    return;
  }

  ensureTooltipHost();
  if (!tooltipHost_ || !tooltipTrigger_) {
    return;
  }

  QRect anchorRect = entry.rect.intersected(rect());
  if (anchorRect.isEmpty()) {
    anchorRect = QRect(0, 0, 1, 1);
  }

  tooltipHost_->setPlacement(collapsedMenuTooltipPlacement(this));
  tooltipHost_->setDisabled(!isEnabled());
  tooltipHost_->setGeometry(anchorRect);

  const QSize anchorSize = anchorRect.size();
  tooltipTrigger_->setMinimumSize(anchorSize);
  tooltipTrigger_->setMaximumSize(anchorSize);
  tooltipTrigger_->updateGeometry();

  tooltipHost_->setTitleText(text);
  tooltipHost_->setOpen(true);
  if (!tooltipHost_->isVisible()) {
    tooltipHost_->show();
  }
}

void AdMenu::appendInlineEntries(const QVector<Item>& items,
                                 int depth,
                                 const QStringList& submenuAncestors,
                                 int& cursorY,
                                 int& trailingBlockMargin) {
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

    const bool menuRowType = (type == ItemType::Item || type == ItemType::SubMenu);
    const int topBlockMargin = menuRowType ? style.metrics.itemMarginBlock : 0;
    const int bottomBlockMargin = menuRowType ? style.metrics.itemMarginBlock : 0;
    cursorY += std::max(trailingBlockMargin, topBlockMargin);

    const int height = rowHeightForType(type);
    entry.rect = QRect(0, cursorY, rowWidth, height);
    cursorY += height;
    trailingBlockMargin = bottomBlockMargin;
    entries_.append(entry);

    if (type == ItemType::Group) {
      appendInlineEntries(item.children, depth + 1, submenuAncestors, cursorY, trailingBlockMargin);
      continue;
    }

    if (type == ItemType::SubMenu && openKeys_.contains(item.key)) {
      const int subMenuTop = cursorY;
      appendInlineEntries(item.children, depth + 1, entry.keyPath, cursorY, trailingBlockMargin);
      const int subMenuBottom = cursorY + std::max(0, trailingBlockMargin);
      if (subMenuBottom > subMenuTop) {
        inlineSubMenuBackgroundRects_.append(
            QRect(0, subMenuTop, rowWidth, subMenuBottom - subMenuTop));
      }
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
                                   bool rootOnlySubmenus,
                                   int& trailingBlockMargin) {
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

    const bool menuRowType = (type == ItemType::Item || type == ItemType::SubMenu);
    const int topBlockMargin = menuRowType ? style.metrics.itemMarginBlock : 0;
    const int bottomBlockMargin = menuRowType ? style.metrics.itemMarginBlock : 0;
    cursorY += std::max(trailingBlockMargin, topBlockMargin);

    const int height = rowHeightForType(type);
    entry.rect = QRect(0, cursorY, rowWidth, height);
    cursorY += height;
    trailingBlockMargin = bottomBlockMargin;
    entries_.append(entry);

    if (type == ItemType::Group) {
      appendVerticalEntries(item.children,
                            depth + 1,
                            submenuAncestors,
                            cursorY,
                            rootOnlySubmenus,
                            trailingBlockMargin);
      continue;
    }

    if (!rootOnlySubmenus && type == ItemType::SubMenu && openKeys_.contains(item.key)) {
      appendVerticalEntries(
          item.children, depth + 1, entry.keyPath, cursorY, false, trailingBlockMargin);
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
  const int itemHeight = std::max(1, style.metrics.horizontalLineHeight);

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
  const QFontMetricsF metrics(style.metrics.font);
  const int iconSide = std::max(10, style.metrics.iconSize);

  int widthHint = 0;
  widthHint += style.metrics.itemPaddingInline * 2;
  widthHint += static_cast<int>(std::ceil(metrics.horizontalAdvance(label)));

  if (adqt::icons::isValid(item.icon)) {
    // Keep this in sync with paint geometry: icon area + explicit icon/text separation.
    widthHint += iconSide + style.metrics.iconMarginInlineEnd + 1;
  }

  const bool showArrow = shouldShowSubMenuArrow(mode_, inlineCollapsed_, type, !item.children.isEmpty());
  if (showArrow) {
    // Keep in sync with paint geometry: arrow box + text/arrow gap.
    widthHint += kSubMenuArrowBoxWidth + kSubMenuArrowTextGap;
  }

  return widthHint;
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

int AdMenu::verticalContentWidthHint(const detail::MenuVisualStyle& style) const {
  if (entries_.isEmpty()) {
    return 0;
  }

  const QFontMetricsF itemMetrics(style.metrics.font);
  QFont groupFont = style.metrics.font;
  groupFont.setBold(false);
  groupFont.setPixelSize(std::max(10, style.metrics.groupTitleFontSize));
  const QFontMetricsF groupMetrics(groupFont);

  const int iconSide = std::max(10, style.metrics.iconSize);
  int maxWidth = 0;

  for (const VisibleEntry& entry : entries_) {
    if (!entry.item) {
      continue;
    }
    if (entry.type == ItemType::Divider) {
      continue;
    }

    if (entry.type == ItemType::Group) {
      const QString label = trimmedOrFallback(entry.item->label, entry.item->key);
      const int textWidth = static_cast<int>(std::ceil(groupMetrics.horizontalAdvance(label)));
      const int groupWidth = style.metrics.groupTitleHorizontalPadding * 2 + textWidth;
      maxWidth = std::max(maxWidth, groupWidth);
      continue;
    }

    int indent = 0;
    if (mode_ == Mode::Vertical && entry.depth > 0) {
      const int groupChildIndent =
          std::max(0, style.metrics.itemPaddingInline - style.metrics.itemMarginInline);
      indent = entry.depth * groupChildIndent;
    }

    int widthHint = 0;
    widthHint += style.metrics.itemMarginInline * 2;
    widthHint += style.metrics.itemPaddingInline * 2;
    widthHint += indent;

    if (adqt::icons::isValid(entry.item->icon)) {
      widthHint += iconSide + style.metrics.iconMarginInlineEnd + 1;
    }

    const QString label = trimmedOrFallback(entry.item->label, entry.item->key);
    widthHint += static_cast<int>(std::ceil(itemMetrics.horizontalAdvance(label)));

    const bool showArrow =
        shouldShowSubMenuArrow(mode_, inlineCollapsed_, entry.type, entry.hasChildren);
    if (showArrow) {
      widthHint += kSubMenuArrowBoxWidth + kSubMenuArrowTextGap;
    }

    if (!entry.item->extra.isEmpty()) {
      widthHint += static_cast<int>(std::ceil(itemMetrics.horizontalAdvance(entry.item->extra))) + 12;
    }

    maxWidth = std::max(maxWidth, widthHint);
  }

  return maxWidth;
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
    const int groupLineHeight =
        std::max(style.metrics.groupTitleFontSize, style.metrics.groupTitleLineHeight);
    return groupLineHeight + style.metrics.groupTitleVerticalPadding * 2;
  }
  if (mode_ == Mode::Horizontal) {
    return std::max(1, style.metrics.horizontalLineHeight);
  }
  return style.metrics.itemHeight;
}

int AdMenu::entryIndexAt(const QPoint& pos) const {
  const int count = entries_.size();
  if (count <= 0) {
    return -1;
  }

  const auto containsIndex = [this, &pos, count](int idx) {
    return idx >= 0 && idx < count && entries_.at(idx).rect.contains(pos);
  };

  // Fast path for continuous mouse move: most events stay in the same row
  // or move to an adjacent row.
  if (containsIndex(hoveredEntry_)) {
    return hoveredEntry_;
  }
  if (containsIndex(hoveredEntry_ - 1)) {
    return hoveredEntry_ - 1;
  }
  if (containsIndex(hoveredEntry_ + 1)) {
    return hoveredEntry_ + 1;
  }

  // Entries are ordered on the primary axis, so we can binary-search instead
  // of scanning every row on each mouse move.
  if (mode_ == Mode::Horizontal) {
    int lo = 0;
    int hi = count - 1;
    while (lo <= hi) {
      const int mid = lo + (hi - lo) / 2;
      const QRect& rect = entries_.at(mid).rect;
      if (pos.x() < rect.left()) {
        hi = mid - 1;
      } else if (pos.x() > rect.right()) {
        lo = mid + 1;
      } else {
        return rect.contains(pos) ? mid : -1;
      }
    }
    return -1;
  }

  int lo = 0;
  int hi = count - 1;
  while (lo <= hi) {
    const int mid = lo + (hi - lo) / 2;
    const QRect& rect = entries_.at(mid).rect;
    if (pos.y() < rect.top()) {
      hi = mid - 1;
    } else if (pos.y() > rect.bottom()) {
      lo = mid + 1;
    } else {
      return rect.contains(pos) ? mid : -1;
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
  const int previous = hoveredEntry_;
  hoveredEntry_ = index;
  syncTooltipForHoveredEntry();

  bool partialUpdated = false;
  if (previous >= 0 && previous < entries_.size()) {
    update(entries_.at(previous).rect);
    partialUpdated = true;
  }
  if (hoveredEntry_ >= 0 && hoveredEntry_ < entries_.size()) {
    update(entries_.at(hoveredEntry_).rect);
    partialUpdated = true;
  }
  if (!partialUpdated) {
    update();
  }
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
      // Match antd popup behavior: selecting an item closes popup chains and
      // clears opened submenu state so highlight is computed from selection only.
      sink->applyOpenInternal({}, true);
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
  rebuildSelectedSubMenuKeys();
  if (emitSignals) {
    emit selectedKeysChanged(selectedKeys_);
  }

  SharedPopupHost* host = sharedPopupHostFor(this);
  if (host && host->ownerMenu == this) {
    for (PopupLayer* layer : host->layers) {
      if (layer && layer->popupMenu && layer->popupMenu.data() != this) {
        layer->popupMenu->setSelectedKeys(selectedKeys_);
      }
    }
  }
  update();
}

void AdMenu::applyOpenInternal(const QStringList& keys, bool emitSignals) {
  const QStringList normalized = normalizeOpenKeys(keys);
  if (openKeys_ == normalized) {
    return;
  }
  openKeys_ = normalized;
  if (mode_ == Mode::Inline && !inlineCollapsed_) {
    inlineCacheOpenKeys_ = openKeys_;
  }
  const bool openStateAffectsEntries = (mode_ == Mode::Inline && !inlineCollapsed_);
  if (openStateAffectsEntries) {
    rebuildEntries();
  } else {
    update();
  }
  syncPopupVisibility();
  if (emitSignals) {
    emit openKeysChanged(openKeys_);
    emitOpenChanged();
  }

  SharedPopupHost* host = sharedPopupHostFor(this);
  if (host && host->ownerMenu == this) {
    for (PopupLayer* layer : host->layers) {
      if (layer && layer->popupMenu && layer->popupMenu.data() != this) {
        layer->popupMenu->setOpenKeys(openKeys_);
      }
    }
  }
}

void AdMenu::syncOpenKeysForModeTransition(Mode previousMode, bool previousInlineCollapsed) {
  if (openKeysExplicit_) {
    return;
  }

  // Align with antd/rc-menu behavior:
  // - leaving inline mode clears current popup open state
  // - returning to inline mode restores the cached inline open keys
  const bool wasInlineMode = previousMode == Mode::Inline && !previousInlineCollapsed;
  const bool isInlineMode = mode_ == Mode::Inline && !inlineCollapsed_;

  if (wasInlineMode) {
    inlineCacheOpenKeys_ = normalizeOpenKeys(openKeys_);
  }

  if (isInlineMode) {
    applyOpenInternal(inlineCacheOpenKeys_, false);
    return;
  }

  applyOpenInternal({}, true);
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

bool AdMenu::collectSelectedSubMenuKeysRecursive(const QVector<Item>& items,
                                                 const QSet<QString>& selectedItemKeys,
                                                 QSet<QString>& selectedSubMenuKeys) const {
  bool hasSelectedInSubtree = false;

  for (const Item& item : items) {
    const ItemType type = effectiveType(item);
    const bool selfSelected = (type == ItemType::Item) && selectedItemKeys.contains(item.key);
    bool childSelected = false;
    if (!item.children.isEmpty()) {
      childSelected =
          collectSelectedSubMenuKeysRecursive(item.children, selectedItemKeys, selectedSubMenuKeys);
    }

    if (type == ItemType::SubMenu && childSelected) {
      selectedSubMenuKeys.insert(item.key);
    }
    if (selfSelected || childSelected) {
      hasSelectedInSubtree = true;
    }
  }

  return hasSelectedInSubtree;
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
  AdMenu* sink = (eventSink_ && eventSink_.data() != this) ? eventSink_.data() : this;
  if (sink && sink != this) {
    sink->openSubMenuByKey(key);
    return;
  }

  if (key.isEmpty() || !validSubMenuKeys_.contains(key)) {
    return;
  }
  if (openKeys_.contains(key)) {
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
  AdMenu* sink = (eventSink_ && eventSink_.data() != this) ? eventSink_.data() : this;
  if (sink && sink != this) {
    sink->closeSubMenuByKey(key);
    return;
  }

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
  AdMenu* sink = (eventSink_ && eventSink_.data() != this) ? eventSink_.data() : this;
  if (sink && sink != this) {
    sink->toggleSubMenuByKey(key);
    return;
  }

  if (openKeys_.contains(key)) {
    closeSubMenuByKey(key);
  } else {
    openSubMenuByKey(key);
  }
}

void AdMenu::requestHoverOpen(const QString& key) {
  AdMenu* sink = (eventSink_ && eventSink_.data() != this) ? eventSink_.data() : this;
  if (sink && sink != this) {
    sink->requestHoverOpen(key);
    return;
  }

  if (key.isEmpty()) {
    if (pendingHoverOpenKey_.isEmpty()) {
      return;
    }
    pendingHoverOpenKey_.clear();
    detail::cancelTimingTask(this, QString::fromLatin1(kHoverOpenTaskKey));
    return;
  }

  pendingHoverOpenKey_ = key;
  const int delay = detail::resolveMenuOpenDelayMs(subMenuOpenDelayMs_);
  if (delay == 0) {
    detail::cancelTimingTask(this, QString::fromLatin1(kHoverOpenTaskKey));
    openSubMenuByKey(key);
    return;
  }

  detail::scheduleTimingTask(this, QString::fromLatin1(kHoverOpenTaskKey), delay, [this, key]() {
    if (pendingHoverOpenKey_ != key || key.isEmpty()) {
      return;
    }
    AdMenu* sink = (eventSink_ && eventSink_.data() != this) ? eventSink_.data() : this;
    if (sink) {
      sink->openSubMenuByKey(key);
    }
  });
}

void AdMenu::requestHoverClose() {
  AdMenu* sink = (eventSink_ && eventSink_.data() != this) ? eventSink_.data() : this;
  if (sink && sink != this) {
    sink->requestHoverClose();
    return;
  }

  detail::cancelTimingTask(this, QString::fromLatin1(kHoverCloseTaskKey));

  const int delay = detail::resolveMenuCloseDelayMs(subMenuCloseDelayMs_);
  if (delay == 0) {
    clearDanglingPopups();
    return;
  }
  detail::scheduleTimingTask(this, QString::fromLatin1(kHoverCloseTaskKey), delay, [this]() {
    clearDanglingPopups();
  });
}

void AdMenu::ensureTooltipHost() {
  if (tooltipHost_) {
    return;
  }

  auto* tooltip = new AdTooltip(this);
  tooltip->setObjectName(QStringLiteral("ad-menu-inline-collapsed-tooltip"));
  tooltip->setAttribute(Qt::WA_TransparentForMouseEvents, true);
  tooltip->setFocusPolicy(Qt::NoFocus);
  tooltip->setOpenControlled(true);
  tooltip->setArrowPointAtCenter(true);
  tooltip->setMouseEnterDelayMs(0);
  tooltip->setMouseLeaveDelayMs(0);
  tooltip->setPlacement(collapsedMenuTooltipPlacement(this));

  auto* trigger = new QWidget(tooltip);
  trigger->setAttribute(Qt::WA_TransparentForMouseEvents, true);
  trigger->setFocusPolicy(Qt::NoFocus);
  tooltip->setTriggerWidget(trigger);
  tooltip->show();

  tooltipHost_ = tooltip;
  tooltipTrigger_ = trigger;
}

void AdMenu::hideTooltip() {
  if (!tooltipHost_) {
    return;
  }
  tooltipHost_->setOpen(false);
  tooltipHost_->setTitleText(QString());
}

QString AdMenu::tooltipTextForEntry(const VisibleEntry& entry) const {
  if (!entry.item || entry.type != ItemType::Item) {
    return QString();
  }

  if (entry.item->title.has_value()) {
    return entry.item->title.value().trimmed();
  }
  if (entry.depth > 0) {
    return QString();
  }
  return trimmedOrFallback(entry.item->label, entry.item->key);
}

AdMenu::PopupRecord* AdMenu::ensurePopupForEntry(const VisibleEntry& entry) {
  if (!entry.item || !rowIsOpenable(entry)) {
    return nullptr;
  }
  const Item itemSnapshot = *entry.item;
  const QString entryKey = entry.key;
  const QStringList entryKeyPath = entry.keyPath;
  const MenuTheme entryPopupTheme = entry.popupTheme;
  const QRect entryRect = entry.rect;

  AdMenu* sink = eventSink_ ? eventSink_.data() : this;
  if (!sink) {
    sink = this;
  }

  SharedPopupHost* existingHost = sharedPopupHostFor(sink);
  const bool ownerChanged = !existingHost || existingHost->ownerMenu != sink;

  SharedPopupHost* host = ensureSharedPopupHost(sink);
  if (!host || host->ownerMenu != sink) {
    return nullptr;
  }

  const int layerIndex = std::max(0, static_cast<int>(keyPathPrefix_.size()));
  PopupLayer* layer = ensurePopupLayer(host, layerIndex);
  if (!layer || !layer->popupWindow || !layer->popupMenu) {
    return nullptr;
  }
  bindSharedPopupOwner(host, sink);

  AdMenu* popupMenu = layer->popupMenu.data();
  popupMenu->eventSink_ = QPointer<AdMenu>(sink);
  popupMenu->keyPathPrefix_ = mergeKeyPathWithPrefix(entryKeyPath);
  const bool layerTargetChanged = ownerChanged || layer->activeKey != entryKey;

  popupMenu->setMode(Mode::Vertical);
  popupMenu->setTheme(entryPopupTheme);
  popupMenu->setSelectable(selectable_);
  popupMenu->setMultiple(multiple_);
  popupMenu->setInlineCollapsed(false);
  popupMenu->setInlineIndent(inlineIndent_);
  popupMenu->setTriggerSubMenuAction(triggerSubMenuAction_);
  popupMenu->setSubMenuOpenDelayMs(subMenuOpenDelayMs_);
  popupMenu->setSubMenuCloseDelayMs(subMenuCloseDelayMs_);
  popupMenu->setTooltipEnabled(false);
  popupMenu->setOverflowedIndicatorText(overflowedIndicatorText_);
  popupMenu->setExpandIcon(expandIcon_);
  popupMenu->setPopupOffset(popupOffset_);

  if (layerTargetChanged) {
    // Expensive setters (function hooks / token-driven rebuild and items flattening)
    // are only needed when this popup layer switches to a different submenu key.
    popupMenu->setComponentTokens(componentTokens_);
    popupMenu->setSemanticStyles(semanticStyles_);
    popupMenu->setSemanticStyleResolver(semanticStyleResolver_);
    popupMenu->setPopupRender(popupRender_);
    popupMenu->setItemPaintHook(itemPaintHook_);
    popupMenu->setSubMenuPaintHook(subMenuPaintHook_);
    popupMenu->setItems(itemSnapshot.children);
  }
  if (popupMenu->selectedKeys() != selectedKeys_) {
    popupMenu->setSelectedKeys(selectedKeys_);
  }
  if (popupMenu->openKeys() != openKeys_) {
    popupMenu->setOpenKeys(openKeys_);
  }

  const bool customPopupRender = static_cast<bool>(popupRender_);
  const bool renderedRootIsPopupMenu = layer->renderedRoot == popupMenu;
  bool needRebuildRenderedRoot = ownerChanged || !layer->renderedRoot;
  if (!needRebuildRenderedRoot) {
    if (customPopupRender) {
      needRebuildRenderedRoot = layer->activeKey != entryKey;
    } else {
      needRebuildRenderedRoot = !renderedRootIsPopupMenu;
    }
  }

  if (needRebuildRenderedRoot) {
    if (layer->popupMenu) {
      layer->popupMenu->setParent(layer->popupWindow);
    }
    if (layer->renderedRoot && layer->renderedRoot != popupMenu) {
      if (sink) {
        layer->renderedRoot->removeEventFilter(sink);
      }
      layer->renderedRoot->deleteLater();
    }
    layer->renderedRoot.clear();

    QWidget* renderedPopup = popupMenu;
    if (customPopupRender) {
      QWidget* defaultPopup = new QWidget(layer->popupWindow);
      auto* defaultLayout = new QVBoxLayout(defaultPopup);
      defaultLayout->setContentsMargins(0, 0, 0, 0);
      defaultLayout->setSpacing(0);
      defaultLayout->addWidget(popupMenu);

      renderedPopup = defaultPopup;
      PopupRenderContext ctx;
      ctx.item = itemSnapshot;
      ctx.keyPath = mergeKeyPathWithPrefix(entryKeyPath);
      QWidget* custom = popupRender_(ctx, defaultPopup);
      if (custom) {
        renderedPopup = custom;
      }
      if (renderedPopup != defaultPopup &&
          defaultPopup->parentWidget() == layer->popupWindow) {
        defaultPopup->deleteLater();
      }
    }

    if (renderedPopup->parentWidget() != layer->popupWindow) {
      renderedPopup->setParent(layer->popupWindow);
    }
    clearLayout(layer->popupWindow->layout());
    layer->popupWindow->layout()->addWidget(renderedPopup);
    layer->renderedRoot = renderedPopup;
  }

  bindSharedPopupOwner(host, sink);
  layer->activeKey = entryKey;

  popupRecordCache_.popup = layer->popupWindow;
  popupRecordCache_.popupMenu = layer->popupMenu;
  popupRecordCache_.key = layer->activeKey;
  popupRecordCache_.triggerRect = entryRect;
  popupRecordCache_.popupOffset = itemSnapshot.popupOffset;
  return &popupRecordCache_;
}

void AdMenu::positionPopup(const VisibleEntry& entry, PopupRecord& popupRecord) {
  if (!popupRecord.popup || !popupRecord.popup->parentWidget()) {
    return;
  }

  const QRect triggerRect = popupRecord.triggerRect.isValid() ? popupRecord.triggerRect : entry.rect;
  QWidget* scopeWindow = popupRecord.popup->parentWidget();

  QPoint totalOffset = popupOffset_ + popupRecord.popupOffset;

  int horizontalPopupAlignOffset = 0;
  int horizontalPopupGap = 0;
  int sidePopupGap = 0;
  int horizontalStretchWidth = 0;
  const bool sidePlacementPopup = (mode_ == Mode::Vertical) || (mode_ == Mode::Inline && inlineCollapsed_);
  if (mode_ == Mode::Horizontal || sidePlacementPopup) {
    // Match antd popup spacing behavior:
    // - horizontal mode uses top/bottom gap
    // - side-placement popup (vertical / inline-collapsed) uses left/right gap
    //   via submenu placement padding.
    StyleContext ctx;
    ctx.mode = mode_;
    ctx.theme = theme_;
    ctx.inlineCollapsed = inlineCollapsed_;
    ctx.items = items_;
    const SemanticStyles effectiveSemantic =
        semanticStyleResolver_ ? semanticStyleResolver_(ctx) : semanticStyles_;
    const MenuVisualStyle style = resolveVisualStyle(this, effectiveSemantic);
    if (mode_ == Mode::Horizontal) {
      horizontalPopupAlignOffset = std::max(0, style.metrics.itemPaddingInline);
      horizontalPopupGap = std::max(0, style.metrics.popupPlacementGap);
      horizontalStretchWidth =
          std::max(0, triggerRect.width() - style.metrics.itemPaddingInline * 2);
    } else {
      // Align with antd side popup spacing (paddingXS) in visual result.
      // We anchor side popups to the full row rect, while visible row content
      // is inset by itemMarginInline. Subtract it to avoid double-counting gap.
      sidePopupGap =
          std::max(0, style.metrics.popupPlacementGap - std::max(0, style.metrics.itemMarginInline));
    }
  }

  QLayout* popupLayout = popupRecord.popup->layout();
  if (popupLayout) {
    popupLayout->setContentsMargins(0, 0, 0, 0);
    if (mode_ == Mode::Horizontal && horizontalPopupGap > 0) {
      // Default to bottom placement gap (top padding). If we later flip to top placement,
      // we swap this to bottom padding.
      popupLayout->setContentsMargins(0, horizontalPopupGap, 0, 0);
    }
  }

  const auto measurePopupSize = [this, &popupRecord, horizontalStretchWidth]() {
    popupRecord.popup->adjustSize();
    QSize measured = popupRecord.popup->sizeHint();
    if (!measured.isValid() || measured.isEmpty()) {
      measured = popupRecord.popup->size();
    }
    if (!measured.isValid() || measured.isEmpty()) {
      measured = QSize(kAntdDropdownMinWidth, 120);
    }
    if (mode_ == Mode::Horizontal) {
      measured.setWidth(std::max(measured.width(), horizontalStretchWidth));
    }
    popupRecord.popup->resize(measured);
    return measured;
  };

  QSize popupSize = measurePopupSize();

  QPoint preferredPos;
  if (mode_ == Mode::Horizontal) {
    detail::PopupPlacementInput placementInput;
    placementInput.anchorTopLeft =
        mapTo(scopeWindow, triggerRect.topLeft()) + QPoint(horizontalPopupAlignOffset, 0);
    placementInput.anchorSize = triggerRect.size();
    placementInput.popupSize = popupSize;
    placementInput.bounds = scopeWindow->rect();
    placementInput.preferredPlacement = detail::PopupPlacement::BottomLeft;
    placementInput.offset = totalOffset;

    auto placementResult = detail::resolvePopupPlacement(placementInput);
    bool useUpPlacement = placementResult.placement == detail::PopupPlacement::TopLeft;

    if (popupLayout && horizontalPopupGap > 0) {
      const int topGap = useUpPlacement ? 0 : horizontalPopupGap;
      const int bottomGap = useUpPlacement ? horizontalPopupGap : 0;
      popupLayout->setContentsMargins(0, topGap, 0, bottomGap);
      popupSize = measurePopupSize();
      placementInput.popupSize = popupSize;
      placementResult = detail::resolvePopupPlacement(placementInput);
    }
    preferredPos = placementResult.topLeft;
  } else {
    detail::PopupPlacementInput placementInput;
    placementInput.anchorTopLeft = mapTo(scopeWindow, triggerRect.topLeft());
    placementInput.anchorSize = triggerRect.size();
    placementInput.popupSize = popupSize;
    placementInput.bounds = scopeWindow->rect();
    placementInput.preferredPlacement = detail::PopupPlacement::RightTop;
    placementInput.offset = totalOffset;

    auto placementResult = detail::resolvePopupPlacement(placementInput);
    bool useLeftPlacement = placementResult.placement == detail::PopupPlacement::LeftTop;

    if (popupLayout && sidePopupGap > 0) {
      // Match antd submenu placement classes:
      // rightTop/rightBottom => padding-inline-start
      // leftTop/leftBottom => padding-inline-end
      const int leftGap = useLeftPlacement ? 0 : sidePopupGap;
      const int rightGap = useLeftPlacement ? sidePopupGap : 0;
      popupLayout->setContentsMargins(leftGap, 0, rightGap, 0);
      popupSize = measurePopupSize();
      placementInput.popupSize = popupSize;
      placementResult = detail::resolvePopupPlacement(placementInput);
    }

    preferredPos = placementResult.topLeft;
  }

  popupRecord.popup->move(preferredPos);
}

void AdMenu::hidePopupAndDescendants(const QString& key) {
  AdMenu* sink = (eventSink_ && eventSink_.data() != this) ? eventSink_.data() : this;
  if (sink && sink != this) {
    sink->hidePopupAndDescendants(key);
    return;
  }

  SharedPopupHost* host = sharedPopupHostFor(this);
  if (!host || host->ownerMenu != this) {
    syncInWindowPopupHostState();
    return;
  }

  if (key.isEmpty()) {
    hidePopupLayersFrom(host, 0);
    syncInWindowPopupHostState();
    return;
  }

  int hideFromIndex = -1;
  for (int i = 0; i < host->layers.size(); ++i) {
    const PopupLayer* layer = host->layers.at(i);
    if (!layer || layer->activeKey.isEmpty()) {
      continue;
    }
    if (layer->activeKey == key || isDescendantSubMenuKey(layer->activeKey, key)) {
      hideFromIndex = i;
      break;
    }
  }
  if (hideFromIndex >= 0) {
    hidePopupLayersFrom(host, hideFromIndex);
  }
  syncInWindowPopupHostState();
}

void AdMenu::clearDanglingPopups() {
  AdMenu* sink = (eventSink_ && eventSink_.data() != this) ? eventSink_.data() : this;
  if (sink && sink != this) {
    sink->clearDanglingPopups();
    return;
  }

  SharedPopupHost* host = sharedPopupHostFor(this);
  const bool popupLikeMode = mode_ != Mode::Inline || inlineCollapsed_;

  if (host && host->ownerMenu == this) {
    if (openKeys_.isEmpty()) {
      hidePopupLayersFrom(host, 0);
    } else {
      int hideFromIndex = -1;
      for (int i = 0; i < host->layers.size(); ++i) {
        const PopupLayer* layer = host->layers.at(i);
        if (!layer) {
          hideFromIndex = i;
          break;
        }
        if (layer->activeKey.isEmpty() || !openKeys_.contains(layer->activeKey)) {
          hideFromIndex = i;
          break;
        }
      }
      if (hideFromIndex >= 0) {
        hidePopupLayersFrom(host, hideFromIndex);
      }
    }
  }

  if (triggerSubMenuAction_ != TriggerSubMenuAction::Hover || !popupLikeMode) {
    syncInWindowPopupHostState();
    return;
  }

  const QPoint cursorPos = QCursor::pos();
  const QRect menuGlobalRect(mapToGlobal(QPoint(0, 0)), size());
  const bool cursorInMenu = menuGlobalRect.contains(cursorPos);
  const QPoint cursorLocalPos = cursorInMenu ? mapFromGlobal(cursorPos) : QPoint();
  const int cursorEntryIndex = cursorInMenu ? entryIndexAt(cursorLocalPos) : -1;
  const bool cursorInPopup =
      host && host->ownerMenu == this && anyPopupLayerContainsGlobalPos(host, cursorPos);

  if (cursorInPopup) {
    syncInWindowPopupHostState();
    return;
  }

  if (cursorInMenu && cursorEntryIndex >= 0 && cursorEntryIndex < entries_.size()) {
    const VisibleEntry& hoveredEntry = entries_.at(cursorEntryIndex);
    if (!rowIsOpenable(hoveredEntry)) {
      if (!openKeys_.isEmpty()) {
        applyOpenInternal({}, true);
      }
      syncInWindowPopupHostState();
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
    syncInWindowPopupHostState();
    return;
  }

  if (!openKeys_.isEmpty()) {
    applyOpenInternal({}, true);
  }
  syncInWindowPopupHostState();
}

void AdMenu::syncInWindowPopupHostState() {
  SharedPopupHost* host = sharedPopupHostFor(this);
  const bool hasVisiblePopup =
      host && host->ownerMenu == this && anyPopupLayerVisible(host);
  detail::setInWindowPopupHostOpen(this, hasVisiblePopup);
}

QObject* AdMenu::popupOwnerObject() const { return const_cast<AdMenu*>(this); }

QWidget* AdMenu::popupAnchorWidget() const { return const_cast<AdMenu*>(this); }

QWidget* AdMenu::popupScopeWindow() const { return detail::resolvePopupScopeWindow(this); }

bool AdMenu::popupIsVisible() const {
  SharedPopupHost* host = sharedPopupHostFor(this);
  return host && host->ownerMenu == this && anyPopupLayerVisible(host);
}

bool AdMenu::popupContainsGlobalPos(const QPoint& globalPos) const {
  if (widgetContainsGlobalPos(this, globalPos)) {
    return true;
  }
  SharedPopupHost* host = sharedPopupHostFor(this);
  return host && host->ownerMenu == this &&
         anyPopupLayerContainsGlobalPos(host, globalPos);
}

void AdMenu::popupCloseFromHost(detail::PopupCloseReason reason) {
  Q_UNUSED(reason)

  pendingHoverOpenKey_.clear();
  detail::cancelTimingTask(this, QString::fromLatin1(kHoverOpenTaskKey));
  detail::cancelTimingTask(this, QString::fromLatin1(kHoverCloseTaskKey));

  if (!openKeys_.isEmpty()) {
    applyOpenInternal({}, true);
  } else {
    hidePopupAndDescendants(QString());
  }
}

void AdMenu::popupRelayoutFromHost() { syncPopupVisibility(); }

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
