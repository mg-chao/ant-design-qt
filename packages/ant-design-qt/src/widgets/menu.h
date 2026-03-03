#pragma once

#include <QColor>
#include <QList>
#include <QMap>
#include <QObject>
#include <QPointer>
#include <QRect>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVector>
#include <QWidget>
#include <QPainter>

#include <functional>
#include <optional>

#include "in_window_popup_host.h"
#include "icons_types.h"

namespace adqt::widgets {

namespace detail {
struct MenuVisualStyle;
}

class AdMenu final : public QWidget, private detail::InWindowPopupOwner {
  Q_OBJECT

  Q_PROPERTY(Mode mode READ mode WRITE setMode NOTIFY modeChanged)
  Q_PROPERTY(MenuTheme theme READ theme WRITE setTheme NOTIFY themeChanged)
  Q_PROPERTY(bool selectable READ selectable WRITE setSelectable NOTIFY selectableChanged)
  Q_PROPERTY(bool multiple READ multiple WRITE setMultiple NOTIFY multipleChanged)
  Q_PROPERTY(QStringList selectedKeys READ selectedKeys WRITE setSelectedKeys NOTIFY selectedKeysChanged)
  Q_PROPERTY(QStringList defaultSelectedKeys READ defaultSelectedKeys WRITE setDefaultSelectedKeys NOTIFY defaultSelectedKeysChanged)
  Q_PROPERTY(QStringList openKeys READ openKeys WRITE setOpenKeys NOTIFY openKeysChanged)
  Q_PROPERTY(QStringList defaultOpenKeys READ defaultOpenKeys WRITE setDefaultOpenKeys NOTIFY defaultOpenKeysChanged)
  Q_PROPERTY(bool inlineCollapsed READ inlineCollapsed WRITE setInlineCollapsed NOTIFY inlineCollapsedChanged)
  Q_PROPERTY(int inlineIndent READ inlineIndent WRITE setInlineIndent NOTIFY inlineIndentChanged)
  Q_PROPERTY(TriggerSubMenuAction triggerSubMenuAction READ triggerSubMenuAction WRITE setTriggerSubMenuAction NOTIFY triggerSubMenuActionChanged)
  Q_PROPERTY(int subMenuOpenDelayMs READ subMenuOpenDelayMs WRITE setSubMenuOpenDelayMs NOTIFY subMenuOpenDelayMsChanged)
  Q_PROPERTY(int subMenuCloseDelayMs READ subMenuCloseDelayMs WRITE setSubMenuCloseDelayMs NOTIFY subMenuCloseDelayMsChanged)
  Q_PROPERTY(bool tooltipEnabled READ tooltipEnabled WRITE setTooltipEnabled NOTIFY tooltipEnabledChanged)
  Q_PROPERTY(QString overflowedIndicatorText READ overflowedIndicatorText WRITE setOverflowedIndicatorText NOTIFY overflowedIndicatorTextChanged)

 public:
  enum class Mode {
    Vertical,
    Horizontal,
    Inline,
  };
  Q_ENUM(Mode)

  enum class MenuTheme {
    Light,
    Dark,
  };
  Q_ENUM(MenuTheme)

  enum class ItemType {
    Item,
    SubMenu,
    Group,
    Divider,
  };
  Q_ENUM(ItemType)

  enum class TriggerSubMenuAction {
    Hover,
    Click,
  };
  Q_ENUM(TriggerSubMenuAction)

  struct Item {
    QString key;
    QString label;
    std::optional<QString> title;
    QString extra;
    adqt::icons::IconToken icon;
    ItemType type = ItemType::Item;
    bool disabled = false;
    bool danger = false;
    bool dashed = false;
    bool hasSubMenuTheme = false;
    MenuTheme subMenuTheme = MenuTheme::Light;
    QPoint popupOffset;
    QVector<Item> children;
  };

  struct ComponentTokens {
    std::optional<int> itemHeight;
    std::optional<int> itemPaddingInline;
    std::optional<int> itemMarginInline;
    std::optional<int> itemMarginBlock;
    std::optional<int> itemBorderRadius;
    std::optional<int> horizontalItemBorderRadius;
    std::optional<int> subMenuItemBorderRadius;
    std::optional<int> inlineIndent;
    std::optional<int> iconSize;
    std::optional<int> iconMarginInlineEnd;
    std::optional<int> activeBarWidth;
    std::optional<int> borderWidth;
    std::optional<int> groupTitleFontSize;
    std::optional<int> groupTitleLineHeight;
    std::optional<QString> popupBg;
    std::optional<QString> darkPopupBg;
    std::optional<QString> itemBg;
    std::optional<QString> subMenuItemBg;
    std::optional<QString> itemActiveBg;
    std::optional<QString> itemHoverBg;
    std::optional<QString> itemHoverColor;
    std::optional<QString> itemDisabledColor;
    std::optional<QString> subMenuItemSelectedColor;
    std::optional<QString> horizontalItemHoverBg;
    std::optional<QString> horizontalItemHoverColor;
    std::optional<QString> horizontalItemSelectedBg;
    std::optional<QString> horizontalItemSelectedColor;
    std::optional<QString> dangerItemColor;
    std::optional<QString> dangerItemHoverColor;
    std::optional<QString> dangerItemSelectedColor;
    std::optional<QString> dangerItemActiveBg;
    std::optional<QString> dangerItemSelectedBg;
    std::optional<QString> itemSelectedBg;
    std::optional<QString> itemSelectedColor;
    std::optional<QString> darkItemColor;
    std::optional<QString> darkItemBg;
    std::optional<QString> darkSubMenuItemBg;
    std::optional<QString> darkGroupTitleColor;
    std::optional<QString> darkItemHoverBg;
    std::optional<QString> darkItemHoverColor;
    std::optional<QString> darkItemDisabledColor;
    std::optional<QString> darkDangerItemColor;
    std::optional<QString> darkDangerItemHoverColor;
    std::optional<QString> darkDangerItemSelectedColor;
    std::optional<QString> darkDangerItemActiveBg;
    std::optional<QString> darkDangerItemSelectedBg;
    std::optional<QString> darkItemSelectedColor;
    std::optional<QString> darkItemSelectedBg;
  };

  struct SemanticSlotStyle {
    std::optional<QColor> textColor;
    std::optional<QColor> backgroundColor;
    std::optional<QColor> borderColor;
  };

  struct SemanticStyles {
    SemanticSlotStyle root;
    SemanticSlotStyle item;
    SemanticSlotStyle itemTitle;
    SemanticSlotStyle list;
    SemanticSlotStyle itemIcon;
    SemanticSlotStyle itemContent;
    SemanticSlotStyle popup;
    SemanticSlotStyle subMenuItem;
    SemanticSlotStyle subMenuItemTitle;
    SemanticSlotStyle subMenuList;
    SemanticSlotStyle subMenuItemIcon;
    SemanticSlotStyle subMenuItemContent;
  };

  struct StyleContext {
    Mode mode = Mode::Vertical;
    MenuTheme theme = MenuTheme::Light;
    bool inlineCollapsed = false;
    QVector<Item> items;
  };

  struct PopupRenderContext {
    Item item;
    QStringList keyPath;
  };

  struct ItemPaintContext {
    Item item;
    QRect itemRect;
    bool hovered = false;
    bool pressed = false;
    bool selected = false;
    Mode mode = Mode::Vertical;
  };

  using SemanticStyleResolver = std::function<SemanticStyles(const StyleContext&)>;
  using PopupRender = std::function<QWidget*(const PopupRenderContext&, QWidget* defaultPopup)>;
  using ItemPaintHook = std::function<void(QPainter&, const ItemPaintContext&)>;

  explicit AdMenu(QWidget* parent = nullptr);
  ~AdMenu() override;

  Mode mode() const;
  void setMode(Mode value);

  MenuTheme theme() const;
  void setTheme(MenuTheme value);

  bool selectable() const;
  void setSelectable(bool value);

  bool multiple() const;
  void setMultiple(bool value);

  QVector<Item> items() const;
  void setItems(const QVector<Item>& value);

  QStringList selectedKeys() const;
  void setSelectedKeys(const QStringList& keys);
  QStringList defaultSelectedKeys() const;
  void setDefaultSelectedKeys(const QStringList& keys);

  QStringList openKeys() const;
  void setOpenKeys(const QStringList& keys);
  QStringList defaultOpenKeys() const;
  void setDefaultOpenKeys(const QStringList& keys);

  bool inlineCollapsed() const;
  void setInlineCollapsed(bool value);

  int inlineIndent() const;
  void setInlineIndent(int value);

  TriggerSubMenuAction triggerSubMenuAction() const;
  void setTriggerSubMenuAction(TriggerSubMenuAction value);

  int subMenuOpenDelayMs() const;
  void setSubMenuOpenDelayMs(int value);

  int subMenuCloseDelayMs() const;
  void setSubMenuCloseDelayMs(int value);

  bool tooltipEnabled() const;
  void setTooltipEnabled(bool value);

  QString overflowedIndicatorText() const;
  void setOverflowedIndicatorText(const QString& value);

  ComponentTokens componentTokens() const;
  void setComponentTokens(const ComponentTokens& tokens);
  void resetComponentTokens();

  SemanticStyles semanticStyles() const;
  void setSemanticStyles(const SemanticStyles& styles);
  void setSemanticStyleResolver(SemanticStyleResolver resolver);

  PopupRender popupRender() const;
  void setPopupRender(PopupRender render);

  adqt::icons::IconToken expandIcon() const;
  void setExpandIcon(const adqt::icons::IconToken& icon);

  QPoint popupOffset() const;
  void setPopupOffset(const QPoint& value);

  ItemPaintHook itemPaintHook() const;
  void setItemPaintHook(ItemPaintHook hook);

  ItemPaintHook subMenuPaintHook() const;
  void setSubMenuPaintHook(ItemPaintHook hook);

  // Compatibility helpers kept for existing code while using AntD-like selectedKeys API.
  QString selectedKey() const;
  void setSelectedKey(const QString& key);

  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;

 signals:
  void modeChanged(Mode value);
  void themeChanged(MenuTheme value);
  void selectableChanged(bool value);
  void multipleChanged(bool value);
  void itemsChanged();
  void selectedKeysChanged(const QStringList& value);
  void defaultSelectedKeysChanged(const QStringList& value);
  void openKeysChanged(const QStringList& value);
  void defaultOpenKeysChanged(const QStringList& value);
  void inlineCollapsedChanged(bool value);
  void inlineIndentChanged(int value);
  void triggerSubMenuActionChanged(TriggerSubMenuAction value);
  void subMenuOpenDelayMsChanged(int value);
  void subMenuCloseDelayMsChanged(int value);
  void tooltipEnabledChanged(bool value);
  void overflowedIndicatorTextChanged(const QString& value);
  void componentTokensChanged();
  void semanticStylesChanged();
  void popupRenderChanged();
  void expandIconChanged(const adqt::icons::IconToken& value);
  void popupOffsetChanged(const QPoint& value);

  void clicked(const QString& key, const QStringList& keyPath);
  void selected(const QString& key, const QStringList& keyPath, const QStringList& selectedKeys);
  void deselected(const QString& key, const QStringList& keyPath, const QStringList& selectedKeys);
  void openChanged(const QStringList& openKeys);
  void titleClicked(const QString& key);

 protected:
  void paintEvent(QPaintEvent* event) override;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  void enterEvent(QEnterEvent* event) override;
#else
  void enterEvent(QEvent* event) override;
#endif
  void leaveEvent(QEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
  void focusOutEvent(QFocusEvent* event) override;
  void changeEvent(QEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  bool eventFilter(QObject* watched, QEvent* event) override;

 private:
  struct VisibleEntry {
    const Item* item = nullptr;
    ItemType type = ItemType::Item;
    QString key;
    QStringList keyPath;
    int depth = 0;
    QRect rect;
    bool disabled = false;
    bool danger = false;
    bool dashed = false;
    bool hasChildren = false;
    MenuTheme popupTheme = MenuTheme::Light;
  };

  struct PopupRecord {
    QPointer<QWidget> popup;
    QPointer<AdMenu> popupMenu;
    QString key;
    QRect triggerRect;
    QPoint popupOffset;
  };

  bool rowIsInteractive(const VisibleEntry& row) const;
  bool rowIsSelectable(const VisibleEntry& row) const;
  bool rowIsOpenable(const VisibleEntry& row) const;

  void normalizeItems(QVector<Item>& items);
  ItemType effectiveType(const Item& item) const;
  bool isSubMenuItem(const Item& item) const;

  void rebuildEntries();
  void rebuildDepthMaps();
  void rebuildSelectedSubMenuKeys();
  void ensureDefaultStatesApplied();
  bool shouldShowPopupForEntry(const VisibleEntry& entry) const;
  void syncPopupVisibility();
  void syncTooltipForHoveredEntry();

  void appendInlineEntries(const QVector<Item>& items,
                           int depth,
                           const QStringList& submenuAncestors,
                           int& cursorY,
                           int& trailingBlockMargin);
  void appendVerticalEntries(const QVector<Item>& items,
                             int depth,
                             const QStringList& submenuAncestors,
                             int& cursorY,
                             bool rootOnlySubmenus,
                             int& trailingBlockMargin);
  void appendHorizontalEntries(const QVector<Item>& items, int& cursorX);
  int horizontalEntryWidthHint(const Item& item, ItemType type, const detail::MenuVisualStyle& style) const;
  int horizontalContentWidthHint() const;
  int verticalContentWidthHint(const detail::MenuVisualStyle& style) const;

  int rowHeightForType(ItemType type) const;
  int entryIndexAt(const QPoint& pos) const;
  int entryIndexByKey(const QString& key) const;
  void setHoveredEntry(int index);
  void activateEntry(int index, bool fromKeyboard = false);

  QStringList normalizeSelectedKeys(const QStringList& keys) const;
  QStringList normalizeOpenKeys(const QStringList& keys) const;
  void applySelectedInternal(const QStringList& keys, bool emitSignals);
  void applyOpenInternal(const QStringList& keys, bool emitSignals);
  void syncOpenKeysForModeTransition(Mode previousMode, bool previousInlineCollapsed);
  void emitOpenChanged();

  bool findItemByKeyRecursive(const QVector<Item>& items,
                              const QString& key,
                              const Item** result,
                              ItemType* resolvedType = nullptr) const;
  void collectItemKeysRecursive(const QVector<Item>& items, QSet<QString>& keys) const;
  void collectSubMenuKeysRecursive(const QVector<Item>& items,
                                   QSet<QString>& keys,
                                   int depth = 0,
                                   const QString& parentKey = QString());
  bool collectSelectedSubMenuKeysRecursive(const QVector<Item>& items,
                                           const QSet<QString>& selectedItemKeys,
                                           QSet<QString>& selectedSubMenuKeys) const;
  bool isDescendantSubMenuKey(const QString& candidateKey, const QString& parentKey) const;

  void openSubMenuByKey(const QString& key);
  void closeSubMenuByKey(const QString& key);
  void toggleSubMenuByKey(const QString& key);

  void requestHoverOpen(const QString& key);
  void requestHoverClose();
  void hideTooltip();
  QString tooltipTextForEntry(const VisibleEntry& entry) const;

  PopupRecord* ensurePopupForEntry(const VisibleEntry& entry);
  void positionPopup(const VisibleEntry& entry, PopupRecord& popupRecord);
  void hidePopupAndDescendants(const QString& key);
  void clearDanglingPopups();
  void syncInWindowPopupHostState();

  QObject* popupOwnerObject() const override;
  QWidget* popupAnchorWidget() const override;
  QWidget* popupScopeWindow() const override;
  bool popupIsVisible() const override;
  bool popupContainsGlobalPos(const QPoint& globalPos) const override;
  void popupCloseFromHost(detail::PopupCloseReason reason) override;
  void popupRelayoutFromHost() override;

  QStringList mergeKeyPathWithPrefix(const QStringList& localPath) const;

  QVector<Item> items_;
  QVector<VisibleEntry> entries_;
  QVector<QRect> inlineSubMenuBackgroundRects_;

  QSet<QString> validItemKeys_;
  QSet<QString> validSubMenuKeys_;
  QSet<QString> selectedSubMenuKeys_;
  QMap<QString, int> subMenuDepths_;
  QMap<QString, QString> subMenuParents_;

  QStringList selectedKeys_;
  QStringList defaultSelectedKeys_;
  QStringList openKeys_;
  QStringList defaultOpenKeys_;
  QStringList inlineCacheOpenKeys_;

  bool selectedKeysExplicit_ = false;
  bool openKeysExplicit_ = false;
  bool defaultsApplied_ = false;

  int contentHeight_ = 0;
  int hoveredEntry_ = -1;
  int pressedEntry_ = -1;

  Mode mode_ = Mode::Vertical;
  MenuTheme theme_ = MenuTheme::Light;
  bool selectable_ = true;
  bool multiple_ = false;
  bool inlineCollapsed_ = false;
  int inlineIndent_ = 24;
  TriggerSubMenuAction triggerSubMenuAction_ = TriggerSubMenuAction::Hover;
  int subMenuOpenDelayMs_ = 0;
  int subMenuCloseDelayMs_ = 100;
  bool tooltipEnabled_ = true;
  QString overflowedIndicatorText_ = "...";

  ComponentTokens componentTokens_;
  SemanticStyles semanticStyles_;
  SemanticStyleResolver semanticStyleResolver_;
  PopupRender popupRender_;
  adqt::icons::IconToken expandIcon_;
  QPoint popupOffset_;

  ItemPaintHook itemPaintHook_;
  ItemPaintHook subMenuPaintHook_;

  QStringList keyPathPrefix_;
  QPointer<AdMenu> eventSink_;

  QTimer hoverOpenTimer_;
  QTimer hoverCloseTimer_;
  QString pendingHoverOpenKey_;

  PopupRecord popupRecordCache_;
};

}  // namespace adqt::widgets
