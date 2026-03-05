#include "select.h"

#include "detail/timing_hub.h"
#include "icons.h"
#include "interaction_overlay_manager.h"
#include "popup_placement.h"
#include "scroll_area.h"
#include "select_style.h"
#include "theme/theme.h"

#include <QAbstractItemView>
#include <QCursor>
#include <QEvent>
#include <QFontMetrics>
#include <QFrame>
#include <QHBoxLayout>
#include <QInputMethodEvent>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMouseEvent>
#include <QMoveEvent>
#include <QPalette>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QScrollBar>
#include <QScopedValueRollback>
#include <QSet>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <utility>

namespace adqt::widgets {

namespace {

namespace outlined_icons = adqt::icons::outlined;
namespace filled_icons = adqt::icons::filled;
namespace twotone_icons = adqt::icons::twotone;
constexpr char kSuffixSpinnerFrameKey[] = "AdSelect.SuffixSpinnerFrame";
constexpr char kShowLayoutRefreshKey[] = "AdSelect.ShowLayoutRefresh";

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

QStringList uniqueStringList(const QStringList& values) {
  QStringList out;
  out.reserve(values.size());
  QSet<QString> seen;
  for (const QString& value : values) {
    const QString trimmed = value.trimmed();
    if (trimmed.isEmpty() || seen.contains(trimmed)) {
      continue;
    }
    seen.insert(trimmed);
    out.append(trimmed);
  }
  return out;
}

QString escapedForRegex(const QString& text) {
  return QRegularExpression::escape(text);
}

bool iconStylesEqual(const adqt::icons::IconStyle& lhs, const adqt::icons::IconStyle& rhs) {
  return lhs.hasPrimary == rhs.hasPrimary && lhs.hasSecondary == rhs.hasSecondary &&
         lhs.hasTertiary == rhs.hasTertiary && lhs.primary == rhs.primary &&
         lhs.secondary == rhs.secondary && lhs.tertiary == rhs.tertiary;
}

bool iconTokensEqual(const adqt::icons::IconToken& lhs, const adqt::icons::IconToken& rhs) {
  return lhs.index == rhs.index && iconStylesEqual(lhs.style, rhs.style);
}

QPixmap makeSpinnerPixmap(const QSize& logicalSize, qreal devicePixelRatio, const QColor& color, int angleDegrees) {
  if (!logicalSize.isValid() || logicalSize.isEmpty()) {
    return QPixmap();
  }

  const qreal dpr = devicePixelRatio > 0.0 ? devicePixelRatio : 1.0;
  const int logicalW = qMax(1, logicalSize.width());
  const int logicalH = qMax(1, logicalSize.height());
  const int pixelW = qMax(1, qRound(static_cast<qreal>(logicalW) * dpr));
  const int pixelH = qMax(1, qRound(static_cast<qreal>(logicalH) * dpr));

  QPixmap spinner(pixelW, pixelH);
  spinner.setDevicePixelRatio(dpr);
  spinner.fill(Qt::transparent);

  const int normalizedAngle = ((angleDegrees % 360) + 360) % 360;
  const qreal side = std::max<qreal>(8.0, std::min(logicalW, logicalH) - 2.0);
  const QPointF center(static_cast<qreal>(logicalW) / 2.0, static_cast<qreal>(logicalH) / 2.0);
  const QRectF spinnerRect(center.x() - side / 2.0, center.y() - side / 2.0, side, side);
  const qreal strokeWidth =
      std::clamp(static_cast<qreal>(std::min(logicalW, logicalH)) * 0.08, 1.0, 2.0);

  QPainter painter(&spinner);
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.translate(center);
  painter.rotate(static_cast<qreal>(normalizedAngle));
  painter.translate(-center);
  QPen pen(color, strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
  painter.setPen(pen);
  painter.setBrush(Qt::NoBrush);
  painter.drawArc(spinnerRect, 90 * 16, -270 * 16);
  return spinner;
}

bool setWidgetFontIfChanged(QWidget* widget, const QFont& font) {
  if (!widget || widget->font() == font) {
    return false;
  }
  widget->setFont(font);
  return true;
}

bool setWidgetPaletteIfChanged(QWidget* widget, const QPalette& palette) {
  if (!widget || widget->palette() == palette) {
    return false;
  }
  widget->setPalette(palette);
  return true;
}

bool setWidgetAutoFillBackgroundIfChanged(QWidget* widget, bool enabled) {
  if (!widget || widget->autoFillBackground() == enabled) {
    return false;
  }
  widget->setAutoFillBackground(enabled);
  return true;
}

bool setWidgetContentsMarginsIfChanged(QWidget* widget, const QMargins& margins) {
  if (!widget || widget->contentsMargins() == margins) {
    return false;
  }
  widget->setContentsMargins(margins);
  return true;
}

bool setWidgetFixedHeightIfChanged(QWidget* widget, int height) {
  if (!widget) {
    return false;
  }
  const int normalized = std::max(0, height);
  bool changed = false;
  if (widget->minimumHeight() != normalized) {
    widget->setMinimumHeight(normalized);
    changed = true;
  }
  if (widget->maximumHeight() != normalized) {
    widget->setMaximumHeight(normalized);
    changed = true;
  }
  if (widget->height() != normalized) {
    widget->resize(widget->width(), normalized);
    changed = true;
  }
  return changed;
}

bool resetWidgetHeightConstraintsIfChanged(QWidget* widget) {
  if (!widget) {
    return false;
  }

  bool changed = false;
  if (widget->minimumHeight() != 0) {
    widget->setMinimumHeight(0);
    changed = true;
  }
  if (widget->maximumHeight() != QWIDGETSIZE_MAX) {
    widget->setMaximumHeight(QWIDGETSIZE_MAX);
    changed = true;
  }
  return changed;
}

bool setWidgetFixedWidthIfChanged(QWidget* widget, int width) {
  if (!widget) {
    return false;
  }
  const int normalized = std::max(0, width);
  bool changed = false;
  if (widget->minimumWidth() != normalized) {
    widget->setMinimumWidth(normalized);
    changed = true;
  }
  if (widget->maximumWidth() != normalized) {
    widget->setMaximumWidth(normalized);
    changed = true;
  }
  return changed;
}

bool setWidgetCursorIfChanged(QWidget* widget, Qt::CursorShape cursorShape) {
  if (!widget) {
    return false;
  }
  if (widget->cursor().shape() == cursorShape) {
    return false;
  }
  widget->setCursor(cursorShape);
  return true;
}

QRectF joinedSelectorRect(const QRect& bounds, qreal borderWidth, bool joinedLeft, bool joinedRight) {
  const qreal half = std::max<qreal>(0.0, borderWidth / 2.0);
  qreal leftInset = half + 0.5;
  qreal rightInset = half + 0.5;
  if (joinedLeft) {
    leftInset = half;
  }
  if (joinedRight) {
    rightInset = half;
  }
  return QRectF(bounds).adjusted(leftInset, half + 0.5, -rightInset, -half - 0.5);
}

bool setLayoutContentsMarginsIfChanged(QLayout* layout, const QMargins& margins) {
  if (!layout || layout->contentsMargins() == margins) {
    return false;
  }
  layout->setContentsMargins(margins);
  return true;
}

bool setLayoutSpacingIfChanged(QLayout* layout, int spacing) {
  if (!layout || layout->spacing() == spacing) {
    return false;
  }
  layout->setSpacing(spacing);
  return true;
}

int boundedWidgetHeightHint(const QWidget* widget, int availableWidth) {
  if (!widget) {
    return 0;
  }

  int hintHeight = widget->sizeHint().height();
  if (availableWidth > 0 && widget->sizePolicy().hasHeightForWidth()) {
    hintHeight = widget->heightForWidth(availableWidth);
  }
  hintHeight = std::max(hintHeight, widget->minimumSizeHint().height());

  const int minHeight = std::max(0, widget->minimumHeight());
  const int maxHeight = std::max(minHeight, widget->maximumHeight());
  return std::clamp(hintHeight, minHeight, maxHeight);
}

detail::PopupPlacement toPopupPlacement(AdSelect::Placement placement) {
  switch (placement) {
    case AdSelect::Placement::BottomLeft:
      return detail::PopupPlacement::BottomLeft;
    case AdSelect::Placement::BottomRight:
      return detail::PopupPlacement::BottomRight;
    case AdSelect::Placement::TopLeft:
      return detail::PopupPlacement::TopLeft;
    case AdSelect::Placement::TopRight:
      return detail::PopupPlacement::TopRight;
  }
  return detail::PopupPlacement::BottomLeft;
}

QPainterPath roundedRectPath(const QRectF& rect, qreal radius) {
  const qreal clampedRadius = std::clamp(radius, 0.0, std::min(rect.width(), rect.height()) / 2.0);
  QPainterPath path;
  path.addRoundedRect(rect, clampedRadius, clampedRadius);
  return path;
}

QPainterPath roundedRectPath(const QRectF& rect,
                             qreal topLeft,
                             qreal topRight,
                             qreal bottomRight,
                             qreal bottomLeft) {
  const qreal w = std::max(rect.width(), 0.0);
  const qreal h = std::max(rect.height(), 0.0);
  const qreal maxRadius = std::min(w, h) / 2.0;

  topLeft = std::clamp(topLeft, 0.0, maxRadius);
  topRight = std::clamp(topRight, 0.0, maxRadius);
  bottomRight = std::clamp(bottomRight, 0.0, maxRadius);
  bottomLeft = std::clamp(bottomLeft, 0.0, maxRadius);

  const qreal left = rect.left();
  const qreal top = rect.top();
  const qreal right = left + rect.width();
  const qreal bottom = top + rect.height();

  QPainterPath path;
  path.moveTo(left + topLeft, top);
  path.lineTo(right - topRight, top);
  if (topRight > 0.0) {
    path.arcTo(QRectF(right - 2.0 * topRight, top, 2.0 * topRight, 2.0 * topRight), 90.0, -90.0);
  }
  path.lineTo(right, bottom - bottomRight);
  if (bottomRight > 0.0) {
    path.arcTo(QRectF(right - 2.0 * bottomRight,
                      bottom - 2.0 * bottomRight,
                      2.0 * bottomRight,
                      2.0 * bottomRight),
               0.0,
               -90.0);
  }
  path.lineTo(left + bottomLeft, bottom);
  if (bottomLeft > 0.0) {
    path.arcTo(QRectF(left, bottom - 2.0 * bottomLeft, 2.0 * bottomLeft, 2.0 * bottomLeft), 270.0,
               -90.0);
  }
  path.lineTo(left, top + topLeft);
  if (topLeft > 0.0) {
    path.arcTo(QRectF(left, top, 2.0 * topLeft, 2.0 * topLeft), 180.0, -90.0);
  }
  path.closeSubpath();
  return path;
}

class WrappingTagsLayout final : public QLayout {
 public:
  explicit WrappingTagsLayout(QWidget* parent = nullptr) : QLayout(parent) {
    setContentsMargins(0, 0, 0, 0);
    setSpacing(0);
  }

  ~WrappingTagsLayout() override {
    while (QLayoutItem* item = takeAt(0)) {
      delete item;
    }
  }

  void addItem(QLayoutItem* item) override {
    if (!item) {
      return;
    }
    items_.append(item);
  }

  int count() const override { return items_.size(); }

  QLayoutItem* itemAt(int index) const override {
    if (index < 0 || index >= items_.size()) {
      return nullptr;
    }
    return items_.at(index);
  }

  QLayoutItem* takeAt(int index) override {
    if (index < 0 || index >= items_.size()) {
      return nullptr;
    }
    return items_.takeAt(index);
  }

  Qt::Orientations expandingDirections() const override { return {}; }

  bool hasHeightForWidth() const override { return true; }

  int heightForWidth(int width) const override {
    return doLayout(QRect(0, 0, std::max(0, width), 0), true);
  }

  int layoutHeightForWidth(int width) const {
    return heightForWidth(width);
  }

  QSize sizeHint() const override { return minimumSize(); }

  QSize minimumSize() const override {
    QSize size;
    for (QLayoutItem* item : items_) {
      if (!item) {
        continue;
      }
      size = size.expandedTo(item->minimumSize());
    }
    const QMargins margins = contentsMargins();
    size += QSize(margins.left() + margins.right(), margins.top() + margins.bottom());
    return size;
  }

  void setGeometry(const QRect& rect) override {
    QLayout::setGeometry(rect);
    doLayout(rect, false);
  }

 private:
  int doLayout(const QRect& rect, bool testOnly) const {
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;
    getContentsMargins(&left, &top, &right, &bottom);
    QRect effectiveRect = rect.adjusted(left, top, -right, -bottom);
    if (effectiveRect.width() < 0) {
      effectiveRect.setWidth(0);
    }

    const int horizontalSpacing = std::max(0, spacing());
    // Match Ant Design multiple selector layout:
    // item vertical rhythm is driven by each tag's marginBlock, so wrapped
    // lines should not add extra row spacing on top.
    const int verticalSpacing = 0;
    int x = effectiveRect.x();
    int y = effectiveRect.y();
    int lineHeight = 0;

    auto remainingWidth = [&effectiveRect, &x]() {
      return std::max(0, effectiveRect.right() - x + 1);
    };

    for (QLayoutItem* item : items_) {
      if (!item) {
        continue;
      }

      QSize itemSize = item->sizeHint();
      if (!itemSize.isValid()) {
        itemSize = item->minimumSize();
      }
      const QSize minimum = item->minimumSize();
      int minWidth = std::max(0, minimum.width());
      int itemHeight = std::max(itemSize.height(), minimum.height());

      QWidget* widget = item->widget();
      bool expanding = false;
      if (widget) {
        minWidth = std::max(minWidth, std::max(0, widget->minimumWidth()));
        itemHeight = std::max(itemHeight, std::max(0, widget->minimumHeight()));
        if (itemSize.width() <= 0) {
          const QSize hint = widget->sizeHint().isValid() ? widget->sizeHint() : widget->minimumSizeHint();
          minWidth = std::max(minWidth, std::max(0, hint.width()));
        }
        if (itemHeight <= 0) {
          const QSize hint = widget->sizeHint().isValid() ? widget->sizeHint() : widget->minimumSizeHint();
          itemHeight = std::max(itemHeight, std::max(0, hint.height()));
        }
        const QSizePolicy::Policy horizontalPolicy = widget->sizePolicy().horizontalPolicy();
        expanding = horizontalPolicy == QSizePolicy::Expanding ||
                    horizontalPolicy == QSizePolicy::MinimumExpanding;
      }

      int itemWidth = std::max(itemSize.width(), minWidth);
      if (expanding) {
        itemWidth = std::max(minWidth, remainingWidth());
      }
      const int maxItemWidth = std::max(0, effectiveRect.width());
      if (maxItemWidth > 0) {
        itemWidth = std::min(itemWidth, maxItemWidth);
      }

      if (x > effectiveRect.x() && x + itemWidth > effectiveRect.right() + 1) {
        x = effectiveRect.x();
        y += lineHeight + verticalSpacing;
        lineHeight = 0;
        if (expanding) {
          itemWidth = std::max(minWidth, remainingWidth());
          if (maxItemWidth > 0) {
            itemWidth = std::min(itemWidth, maxItemWidth);
          }
        }
      }

      if (!testOnly) {
        item->setGeometry(QRect(QPoint(x, y), QSize(itemWidth, itemHeight)));
      }

      x += itemWidth + horizontalSpacing;
      lineHeight = std::max(lineHeight, itemHeight);
    }

    const int contentBottom = lineHeight > 0 ? (y + lineHeight) : effectiveRect.y();
    return contentBottom - rect.y() + bottom;
  }

  QVector<QLayoutItem*> items_;
};

constexpr int kSelectRowHeaderRole = Qt::UserRole + 97;
constexpr int kSelectRowEmptyRole = Qt::UserRole + 98;

}  // namespace

class FlatIconToolButton final : public QToolButton {
 public:
  explicit FlatIconToolButton(QWidget* parent = nullptr) : QToolButton(parent) {
    setAutoRaise(true);
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_Hover, true);
    setAutoFillBackground(false);
  }

  void setBackgroundDecoration(const QColor& background, int radius) {
    const int normalizedRadius = std::max(0, radius);
    if (background_ == background && radius_ == normalizedRadius) {
      return;
    }

    background_ = background;
    radius_ = normalizedRadius;
    update();
  }

 protected:
  void paintEvent(QPaintEvent* event) override {
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    if (background_.isValid() && background_.alpha() > 0) {
      const QRectF fillRect = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
      if (fillRect.isValid()) {
        painter.fillPath(roundedRectPath(fillRect, radius_), background_);
      }
    }

    const QSize targetIconSize = iconSize().isValid() ? iconSize() : QSize(width(), height());
    const QRect iconRect((width() - targetIconSize.width()) / 2,
                         (height() - targetIconSize.height()) / 2, targetIconSize.width(),
                         targetIconSize.height());

    const QIcon::Mode iconMode =
        !isEnabled() ? QIcon::Disabled
                     : (isDown() ? QIcon::Selected
                                 : (underMouse() ? QIcon::Active : QIcon::Normal));
    const QIcon::State iconState = isChecked() ? QIcon::On : QIcon::Off;
    if (!icon().isNull() && iconRect.isValid()) {
      const QPixmap iconPixmap = icon().pixmap(targetIconSize, iconMode, iconState);
      if (!iconPixmap.isNull()) {
        painter.drawPixmap(iconRect.topLeft(), iconPixmap);
        return;
      }
    }

    if (!text().isEmpty()) {
      painter.setPen(palette().color(QPalette::ButtonText));
      painter.setFont(font());
      painter.drawText(rect(), Qt::AlignCenter, text());
    }
  }

 private:
  QColor background_;
  int radius_ = 0;
};

class TagChipWidget final : public QWidget {
 public:
  explicit TagChipWidget(QWidget* parent = nullptr) : QWidget(parent) {
    setAutoFillBackground(false);
  }

  void setVisualStyle(const QColor& background,
                      const QColor& borderColor,
                      int borderRadius,
                      int borderWidth,
                      int blockInset) {
    const int normalizedRadius = std::max(0, borderRadius);
    const int normalizedBorderWidth = std::max(0, borderWidth);
    const int normalizedBlockInset = std::max(0, blockInset);
    if (background_ == background && borderColor_ == borderColor && borderRadius_ == normalizedRadius &&
        borderWidth_ == normalizedBorderWidth && blockInset_ == normalizedBlockInset) {
      return;
    }

    background_ = background;
    borderColor_ = borderColor;
    borderRadius_ = normalizedRadius;
    borderWidth_ = normalizedBorderWidth;
    blockInset_ = normalizedBlockInset;
    update();
  }

  protected:
  void paintEvent(QPaintEvent* event) override {
    Q_UNUSED(event)

    const int safeBlockInset = std::clamp(blockInset_, 0, std::max(0, height() / 2));
    const qreal borderWidth =
        (borderColor_.isValid() && borderColor_.alpha() > 0 && borderWidth_ > 0)
            ? static_cast<qreal>(borderWidth_)
            : 0.0;
    const qreal inset = borderWidth > 0.0 ? borderWidth * 0.5 : 0.0;
    const QRectF chipRect =
        QRectF(rect()).adjusted(inset,
                                inset + static_cast<qreal>(safeBlockInset),
                                -inset,
                                -inset - static_cast<qreal>(safeBlockInset));
    if (!chipRect.isValid()) {
      return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const QPainterPath chipPath = roundedRectPath(chipRect, borderRadius_);
    if (background_.isValid() && background_.alpha() > 0) {
      painter.fillPath(chipPath, background_);
    }

    if (borderWidth > 0.0) {
      QPen borderPen(borderColor_, borderWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
      painter.setPen(borderPen);
      painter.setBrush(Qt::NoBrush);
      painter.drawPath(chipPath);
    }
  }

 private:
  QColor background_;
  QColor borderColor_;
  int borderRadius_ = 0;
  int borderWidth_ = 0;
  int blockInset_ = 0;
};

class AdSelect::OptionListModel final : public QAbstractListModel {
 public:
  explicit OptionListModel(AdSelect* owner) : QAbstractListModel(owner), owner_(owner) {}

  void setRows(const QVector<ModelRow>& rows) {
    beginResetModel();
    rows_ = rows;
    endResetModel();
  }

  int rowCount(const QModelIndex& parent) const override {
    if (parent.isValid()) {
      return 0;
    }
    return rows_.size();
  }

  QVariant data(const QModelIndex& index, int role) const override {
    if (!owner_ || !index.isValid() || index.row() < 0 || index.row() >= rows_.size()) {
      return QVariant();
    }

    const ModelRow& row = rows_.at(index.row());
    const detail::SelectVisualStyle& style = *owner_->visualStyle_;

    if (role == kSelectRowHeaderRole) {
      return row.header;
    }

    if (role == kSelectRowEmptyRole) {
      return row.empty;
    }

    if (role == Qt::SizeHintRole) {
      if (row.empty) {
        return QSize(0, style.metrics.emptyStateHeight);
      }
      return QSize(0, style.metrics.optionHeight);
    }

    if (row.empty) {
      if (role == Qt::DisplayRole) {
        return row.headerText;
      }
      if (role == Qt::FontRole) {
        QFont font = style.metrics.optionFont;
        font.setBold(false);
        return font;
      }
      if (role == Qt::ForegroundRole) {
        return style.emptyTextColor;
      }
      if (role == Qt::TextAlignmentRole) {
        return static_cast<int>(Qt::AlignHCenter | Qt::AlignTop);
      }
      return QVariant();
    }

    if (row.header) {
      if (role == Qt::DisplayRole) {
        return row.headerText;
      }
      if (role == Qt::FontRole) {
        QFont font = style.metrics.optionFont;
        font.setBold(true);
        return font;
      }
      if (role == Qt::ForegroundRole) {
        return style.prefixColor;
      }
      if (role == Qt::TextAlignmentRole) {
        return static_cast<int>(Qt::AlignVCenter | Qt::AlignLeft);
      }
      return QVariant();
    }

    if (row.optionIndex < 0 || row.optionIndex >= owner_->options_.size()) {
      return QVariant();
    }

    const Option& option = owner_->options_.at(row.optionIndex);
    if (role == Qt::DisplayRole) {
      return owner_->formattedOptionText(option);
    }
    if (role == Qt::ForegroundRole) {
      if (option.disabled) {
        return style.disabledTextColor;
      }
      if (owner_->isValueSelected(option.value)) {
        return style.optionSelectedColor;
      }
      return style.optionTextColor;
    }
    if (role == Qt::FontRole) {
      QFont font = style.metrics.optionFont;
      if (owner_->isValueSelected(option.value)) {
        font.setWeight(QFont::DemiBold);
      }
      return font;
    }
    if (role == Qt::TextAlignmentRole) {
      return static_cast<int>(Qt::AlignVCenter | Qt::AlignLeft);
    }
    if (role == Qt::UserRole) {
      return option.value;
    }
    return QVariant();
  }

  Qt::ItemFlags flags(const QModelIndex& index) const override {
    if (!owner_ || !index.isValid() || index.row() < 0 || index.row() >= rows_.size()) {
      return Qt::NoItemFlags;
    }

    const ModelRow& row = rows_.at(index.row());
    if (row.empty) {
      return Qt::NoItemFlags;
    }
    if (row.header || row.optionIndex < 0 || row.optionIndex >= owner_->options_.size()) {
      return Qt::ItemIsEnabled;
    }

    const Option& option = owner_->options_.at(row.optionIndex);
    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (option.disabled) {
      flags &= ~Qt::ItemIsEnabled;
      flags &= ~Qt::ItemIsSelectable;
    }
    return flags;
  }

 private:
  QPointer<AdSelect> owner_;
  QVector<ModelRow> rows_;
};

class AdSelect::PopupFrame final : public QFrame {
 public:
  explicit PopupFrame(QWidget* parent = nullptr) : QFrame(parent) {
    setFrameShape(QFrame::NoFrame);
    setAttribute(Qt::WA_Hover, true);
    setAutoFillBackground(false);
  }

  void setVisualStyle(const QColor& background, const QColor& borderColor, int borderRadius) {
    const int normalizedRadius = std::max(0, borderRadius);
    if (background_ == background && borderColor_ == borderColor &&
        borderRadius_ == normalizedRadius) {
      return;
    }

    background_ = background;
    borderColor_ = borderColor;
    borderRadius_ = normalizedRadius;
    update();
  }

 protected:
  void paintEvent(QPaintEvent* event) override {
    Q_UNUSED(event)

    const qreal popupRadius = std::max<qreal>(0.0, static_cast<qreal>(borderRadius_));
    const qreal popupBorderWidth =
        (borderColor_.isValid() && borderColor_.alpha() > 0) ? 1.0 : 0.0;
    const qreal inset = popupBorderWidth > 0.0 ? popupBorderWidth * 0.5 : 0.5;
    const QRectF frameRect = QRectF(rect()).adjusted(inset, inset, -inset, -inset);
    if (!frameRect.isValid()) {
      return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    QPainterPath framePath;
    framePath.addRoundedRect(frameRect, popupRadius, popupRadius);
    if (background_.isValid() && background_.alpha() > 0) {
      painter.fillPath(framePath, background_);
    }

    if (popupBorderWidth > 0.0) {
      QPen borderPen(borderColor_, popupBorderWidth);
      painter.setPen(borderPen);
      painter.setBrush(Qt::NoBrush);
      painter.drawRoundedRect(frameRect, popupRadius, popupRadius);
    }
  }

 private:
  QColor background_;
  QColor borderColor_;
  int borderRadius_ = 0;
};

class AdSelect::OptionListDelegate final : public QStyledItemDelegate {
 public:
  explicit OptionListDelegate(AdSelect* owner) : QStyledItemDelegate(owner), owner_(owner) {}

  QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override {
    const QVariant candidate = index.data(Qt::SizeHintRole);
    if (candidate.isValid()) {
      const QSize size = candidate.toSize();
      if (size.isValid()) {
        return size;
      }
    }
    return QStyledItemDelegate::sizeHint(option, index);
  }

  void paint(QPainter* painter,
             const QStyleOptionViewItem& option,
             const QModelIndex& index) const override {
    if (!painter || !index.isValid() || !owner_ || !owner_->visualStyle_) {
      QStyledItemDelegate::paint(painter, option, index);
      return;
    }

    QStyleOptionViewItem itemOption(option);
    initStyleOption(&itemOption, index);

    const detail::SelectVisualStyle& style = *owner_->visualStyle_;
    const bool isHeader = index.data(kSelectRowHeaderRole).toBool();
    const bool isEmpty = index.data(kSelectRowEmptyRole).toBool();
    const bool isSelected = (itemOption.state & QStyle::State_Selected) != 0;
    const bool isHovered = (itemOption.state & QStyle::State_MouseOver) != 0;
    const bool isEnabled = (itemOption.state & QStyle::State_Enabled) != 0;
    bool optionSelected = false;
    bool optionDisabled = false;
    if (!isHeader && !isEmpty && index.row() >= 0 && index.row() < owner_->rows_.size()) {
      const AdSelect::ModelRow& modelRow = owner_->rows_.at(index.row());
      if (modelRow.optionIndex >= 0 && modelRow.optionIndex < owner_->options_.size()) {
        const Option& modelOption = owner_->options_.at(modelRow.optionIndex);
        optionDisabled = modelOption.disabled;
        optionSelected = owner_->isValueSelected(modelOption.value);
      }
    }

    QColor background(Qt::transparent);
    if (!isHeader && !isEmpty) {
      if (optionSelected) {
        background = optionDisabled ? style.disabledBg : style.optionSelectedBg;
      } else if (!optionDisabled && (isHovered || isSelected)) {
        background = style.optionHoverBg;
      }
    }
    const bool hasBackground = background.isValid() && background.alpha() > 0;
    const int optionRadius = std::max(0, style.metrics.optionBorderRadius);
    const bool drawRoundedBackground = hasBackground && optionRadius > 0;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, drawRoundedBackground);

    painter->fillRect(itemOption.rect, style.popupBg);

    if (isEmpty) {
      const int optionHInset = std::max(0, style.metrics.optionPaddingHorizontal);
      const int optionVInset = std::max(0, style.metrics.optionPaddingVertical);
      QRect contentRect = itemOption.rect.adjusted(optionHInset, optionVInset, -optionHInset, -optionVInset);
      const int marginInline = std::max(0, style.metrics.emptyStateMarginInline);
      contentRect.adjust(marginInline, 0, -marginInline, 0);

      const int marginBlock = std::max(0, style.metrics.emptyStateMarginBlock);
      const int imageBottomMargin = std::max(0, style.metrics.emptyStateImageMarginBottom);
      const int iconWidth = std::max(30, style.metrics.emptyStateIconWidth);
      const int iconHeight = std::max(20, style.metrics.emptyStateIconHeight);
      const int textHeight = std::max(12, style.metrics.emptyDescriptionLineHeight);
      const int top = contentRect.top() + marginBlock;

      const QRectF iconRect(contentRect.left() + (contentRect.width() - iconWidth) / 2.0,
                            top,
                            iconWidth,
                            iconHeight);
      adqt::icons::IconStyle emptyIconStyle;
      emptyIconStyle.primary = style.emptyBorderColor;
      emptyIconStyle.hasPrimary = true;
      emptyIconStyle.secondary = style.emptyContentColor;
      emptyIconStyle.hasSecondary = true;
      emptyIconStyle.tertiary = style.emptyShadowColor;
      emptyIconStyle.hasTertiary = true;
      const adqt::icons::IconToken emptyIcon = twotone_icons::EmptySimple(emptyIconStyle);
      const qreal dpr = owner_->devicePixelRatioF();
      const QPixmap iconPixmap =
          adqt::icons::renderIconPixmap(emptyIcon, QSize(iconWidth, iconHeight), dpr, QIcon::Normal, QIcon::Off);
      if (!iconPixmap.isNull()) {
        painter->drawPixmap(QPointF(iconRect.left(), iconRect.top()), iconPixmap);
      }

      QFont textFont = style.metrics.optionFont;
      textFont.setWeight(QFont::Normal);
      textFont.setPixelSize(std::max(12, style.metrics.emptyDescriptionFontSize));
      painter->setFont(textFont);
      painter->setPen(style.emptyTextColor);
      const QFontMetrics metrics(textFont);
      const QString text = metrics.elidedText(itemOption.text, Qt::ElideRight, std::max(0, contentRect.width()));
      const QRect textRect(
          contentRect.left(), top + iconHeight + imageBottomMargin, contentRect.width(), textHeight);
      painter->drawText(textRect, Qt::AlignHCenter | Qt::AlignTop, text);

      painter->restore();
      return;
    }

    if (hasBackground) {
      QRectF backgroundRect(itemOption.rect);
      if (backgroundRect.isValid()) {
        if (drawRoundedBackground) {
          qreal topLeftRadius = static_cast<qreal>(optionRadius);
          qreal topRightRadius = static_cast<qreal>(optionRadius);
          qreal bottomRightRadius = static_cast<qreal>(optionRadius);
          qreal bottomLeftRadius = static_cast<qreal>(optionRadius);
          if (optionSelected) {
            const bool hasSelectedPrev = rowHasSelectedOption(index.row() - 1);
            const bool hasSelectedNext = rowHasSelectedOption(index.row() + 1);
            if (hasSelectedPrev) {
              topLeftRadius = 0.0;
              topRightRadius = 0.0;
              // Overlap by half pixel to avoid anti-aliased seams between joined rows.
              backgroundRect.adjust(0.0, -0.5, 0.0, 0.0);
            }
            if (hasSelectedNext) {
              bottomRightRadius = 0.0;
              bottomLeftRadius = 0.0;
              // Overlap by half pixel to avoid anti-aliased seams between joined rows.
              backgroundRect.adjust(0.0, 0.0, 0.0, 0.5);
            }
          }
          painter->fillPath(roundedRectPath(backgroundRect,
                                            topLeftRadius,
                                            topRightRadius,
                                            bottomRightRadius,
                                            bottomLeftRadius),
                            background);
        } else {
          painter->fillRect(backgroundRect.toAlignedRect(), background);
        }
      }
    }

    QFont textFont = itemOption.font;
    const QVariant fontRole = index.data(Qt::FontRole);
    if (fontRole.canConvert<QFont>()) {
      textFont = qvariant_cast<QFont>(fontRole);
    }
    painter->setFont(textFont);

    QColor textColor = style.optionTextColor;
    const QVariant foregroundRole = index.data(Qt::ForegroundRole);
    if (foregroundRole.canConvert<QColor>()) {
      textColor = qvariant_cast<QColor>(foregroundRole);
    } else if (!isEnabled) {
      textColor = style.disabledTextColor;
    }
    painter->setPen(textColor);

    Qt::Alignment textAlignment = Qt::AlignVCenter | Qt::AlignLeft;
    const QVariant alignmentRole = index.data(Qt::TextAlignmentRole);
    if (alignmentRole.isValid()) {
      textAlignment = Qt::Alignment(alignmentRole.toInt());
    }

    const int horizontalPadding = std::max(0, style.metrics.optionPaddingHorizontal);
    const int verticalPadding = std::max(0, style.metrics.optionPaddingVertical);
    const bool showSelectedIcon = optionSelected && owner_->mode_ != AdSelect::Mode::Single;
    const int selectedIconSize = std::max(10, style.metrics.iconSize);
    const int selectedStateGap = std::max(2, style.metrics.optionStateGap);

    QRect textRect = itemOption.rect.adjusted(
        horizontalPadding, verticalPadding, -horizontalPadding, -verticalPadding);
    if (showSelectedIcon) {
      textRect.adjust(0, 0, -(selectedIconSize + selectedStateGap), 0);
    }
    const QFontMetrics metrics(textFont);
    const QString text =
        metrics.elidedText(itemOption.text, Qt::ElideRight, std::max(0, textRect.width()));
    painter->drawText(textRect, textAlignment, text);

    if (showSelectedIcon) {
      const QRect stateRect(itemOption.rect.right() - horizontalPadding - selectedIconSize + 1,
                            itemOption.rect.top() + (itemOption.rect.height() - selectedIconSize) / 2,
                            selectedIconSize,
                            selectedIconSize);
      adqt::icons::IconToken checkIcon = outlined_icons::Check();
      if (adqt::icons::isValid(checkIcon)) {
        checkIcon.style.primary = isEnabled ? style.selectorActiveBorderColor : style.disabledTextColor;
        checkIcon.style.hasPrimary = true;
        const qreal dpr = owner_->devicePixelRatioF();
        const QPixmap iconPixmap = adqt::icons::renderIconPixmap(
            checkIcon, QSize(selectedIconSize, selectedIconSize), dpr, QIcon::Normal, QIcon::Off);
        if (!iconPixmap.isNull()) {
          painter->drawPixmap(stateRect.topLeft(), iconPixmap);
        }
      }
    }

    painter->restore();
  }

 private:
  bool rowHasSelectedOption(int row) const {
    if (!owner_ || row < 0 || row >= owner_->rows_.size()) {
      return false;
    }
    const AdSelect::ModelRow& modelRow = owner_->rows_.at(row);
    if (modelRow.header || modelRow.empty || modelRow.optionIndex < 0 ||
        modelRow.optionIndex >= owner_->options_.size()) {
      return false;
    }
    const Option& modelOption = owner_->options_.at(modelRow.optionIndex);
    return owner_->isValueSelected(modelOption.value);
  }

  QPointer<AdSelect> owner_;
};

AdSelect::AdSelect(QWidget* parent) : QWidget(parent) {
  setObjectName(QStringLiteral("adselect-root"));
  setAttribute(Qt::WA_Hover, true);
  setAttribute(Qt::WA_StyledBackground, true);
  setFocusPolicy(Qt::StrongFocus);

  visualStyle_ = new detail::SelectVisualStyle();
  searchFilterFields_ = {QStringLiteral("label"), QStringLiteral("value")};
  tokenSeparators_ = {QStringLiteral(",")};

  rootLayout_ = new QHBoxLayout(this);
  rootLayout_->setContentsMargins(10, 4, 10, 4);
  rootLayout_->setSpacing(6);

  prefixLabel_ = new QLabel(this);
  prefixLabel_->setObjectName(QStringLiteral("adselect-prefix"));
  prefixLabel_->setVisible(false);
  rootLayout_->addWidget(prefixLabel_);

  contentHost_ = new QWidget(this);
  contentHost_->setAutoFillBackground(false);
  contentLayout_ = new QHBoxLayout(contentHost_);
  contentLayout_->setContentsMargins(0, 0, 0, 0);
  contentLayout_->setSpacing(6);

  tagsContainer_ = new QWidget(contentHost_);
  tagsContainer_->setObjectName(QStringLiteral("adselect-tags"));
  tagsContainer_->setAutoFillBackground(false);
  tagsContainer_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  tagsContainer_->setVisible(false);
  tagsLayout_ = new WrappingTagsLayout(tagsContainer_);
  tagsLayout_->setContentsMargins(0, 0, 0, 0);
  tagsLayout_->setSpacing(4);
  contentLayout_->addWidget(tagsContainer_);

  placeholderLabel_ = new QLabel(tagsContainer_);
  placeholderLabel_->setObjectName(QStringLiteral("adselect-placeholder"));
  placeholderLabel_->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
  placeholderLabel_->setAttribute(Qt::WA_TransparentForMouseEvents, true);
  placeholderLabel_->setVisible(false);

  lineEdit_ = new QLineEdit(contentHost_);
  lineEdit_->setObjectName(QStringLiteral("adselect-input"));
  lineEdit_->setFrame(false);
  lineEdit_->setAutoFillBackground(false);
  lineEdit_->setMinimumWidth(4);
  lineEdit_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  QPalette inputBasePalette = lineEdit_->palette();
  inputBasePalette.setColor(QPalette::Base, QColor(0, 0, 0, 0));
  inputBasePalette.setColor(QPalette::Disabled, QPalette::Base, QColor(0, 0, 0, 0));
  lineEdit_->setPalette(inputBasePalette);
  lineEdit_->setPlaceholderText(placeholder_);
  lineEdit_->installEventFilter(this);
  contentLayout_->addWidget(lineEdit_, 1);
  rootLayout_->addWidget(contentHost_, 1);

  clearButton_ = new FlatIconToolButton(this);
  clearButton_->setObjectName(QStringLiteral("adselect-clear"));
  clearButton_->setCursor(Qt::PointingHandCursor);
  clearButton_->installEventFilter(this);
  clearButton_->setVisible(false);

  suffixButton_ = new FlatIconToolButton(this);
  suffixButton_->setObjectName(QStringLiteral("adselect-suffix"));
  suffixButton_->setCursor(Qt::PointingHandCursor);
  rootLayout_->addWidget(suffixButton_);

  listModel_ = new OptionListModel(this);

  connect(lineEdit_, &QLineEdit::textEdited, this, [this](const QString& text) {
    if (suppressLineEditChange_) {
      return;
    }

    if (mode_ == Mode::Tags && !tokenSeparators_.isEmpty()) {
      consumeTokenizedInput(text);
    }

    if (!isSearchEnabledForCurrentMode()) {
      return;
    }
    if (searchText_ == text) {
      return;
    }
    searchText_ = text;
    emit searchTextChanged(searchText_);
    if (mode_ != Mode::Single) {
      updateDisplay();
    }
    if (!open_) {
      openPopup();
    } else {
      refreshRows();
    }
  });

  connect(clearButton_, &QToolButton::clicked, this, [this]() { clearSelectionInternal(true); });
  connect(suffixButton_, &QToolButton::clicked, this, [this]() {
    if (!suffixButtonTriggersPopup()) {
      return;
    }
    if (open_) {
      closePopup();
    } else {
      openPopup();
    }
  });

  connect(&adqt::theme::ThemeManager::instance(), &adqt::theme::ThemeManager::themeChanged, this,
          [this]() { applyVisualStyle(); });

  applyVisualStyle();
  updateInputMode();
  updateDisplay();
  updateClearButton();
  updatePrefixVisual();
  updateSuffixVisual();
}

AdSelect::~AdSelect() {
  stopInteractionFocusForOwner(this);
  detail::clearFrameSubscription(this, QString::fromLatin1(kSuffixSpinnerFrameKey));
  suffixSpinnerSubscribed_ = false;
  detail::setInWindowPopupHostOpen(this, false);
  if (popup_) {
    popup_->hide();
    popup_->deleteLater();
    popup_ = nullptr;
  }
  delete visualStyle_;
  visualStyle_ = nullptr;
}

AdSelect::Mode AdSelect::mode() const { return mode_; }

void AdSelect::setMode(Mode value) {
  if (mode_ == value) {
    return;
  }
  mode_ = value;

  if (mode_ == Mode::Single) {
    if (!values_.isEmpty()) {
      value_ = values_.constFirst();
      values_ = {value_};
    }
    if (!searchEnabled_) {
      searchText_.clear();
    }
  } else {
    if (!value_.isEmpty() && values_.isEmpty()) {
      values_.append(value_);
    }
    value_.clear();
  }

  enforceMaxCount();
  updateInputMode();
  applyVisualStyle();
  refreshRows();
  updateDisplay();
  updateClearButton();
  updateSuffixVisual();
  emit modeChanged(mode_);
  emit valueChanged(value_);
  emit valuesChanged(values_);
  emitSelectionChangedSignals();
}

AdSelect::Size AdSelect::size() const { return size_; }

void AdSelect::setSize(Size value) {
  if (size_ == value) {
    return;
  }
  size_ = value;
  applyVisualStyle();
  emit sizeChanged(size_);
}

AdSelect::Variant AdSelect::variant() const { return variant_; }

void AdSelect::setVariant(Variant value) {
  if (variant_ == value) {
    return;
  }
  variant_ = value;
  applyVisualStyle();
  emit variantChanged(variant_);
}

AdSelect::Status AdSelect::status() const { return status_; }

void AdSelect::setStatus(Status value) {
  if (status_ == value) {
    return;
  }
  status_ = value;
  applyVisualStyle();
  emit statusChanged(status_);
}

bool AdSelect::allowClear() const { return allowClear_; }

void AdSelect::setAllowClear(bool value) {
  if (allowClear_ == value) {
    return;
  }
  allowClear_ = value;
  updateClearButton();
  emit allowClearChanged(allowClear_);
}

bool AdSelect::loading() const { return loading_; }

void AdSelect::setLoading(bool value) {
  if (loading_ == value) {
    return;
  }
  loading_ = value;
  updateLoadingSpinnerState();
  updateSuffixVisual();
  emit loadingChanged(loading_);
}

bool AdSelect::open() const { return open_; }

void AdSelect::setOpen(bool value) {
  if (value) {
    openPopup();
  } else {
    closePopup();
  }
}

bool AdSelect::disabled() const { return !isEnabled(); }

void AdSelect::setDisabled(bool value) {
  if (disabled() == value) {
    return;
  }
  QWidget::setDisabled(value);
  if (value) {
    clearHovered_ = false;
    closePopup();
  }
  updateInputMode();
  applyVisualStyle();
  updateDisplay();
  updateClearButton();
  updateSuffixVisual();
  emit disabledChanged(value);
}

bool AdSelect::searchEnabled() const { return searchEnabled_; }

void AdSelect::setSearchEnabled(bool value) {
  if (searchEnabled_ == value) {
    return;
  }
  searchEnabled_ = value;
  updateInputMode();
  refreshRows();
  updateDisplay();
  updateSuffixVisual();
  emit searchEnabledChanged(searchEnabled_);
}

QString AdSelect::searchText() const { return searchText_; }

void AdSelect::setSearchText(const QString& value) {
  if (searchText_ == value) {
    return;
  }
  if (!inputMethodPreeditText_.isEmpty()) {
    inputMethodPreeditText_.clear();
  }
  searchText_ = value;
  emit searchTextChanged(searchText_);
  if (isSearchEnabledForCurrentMode()) {
    if (lineEdit_ && (open_ || mode_ != Mode::Single)) {
      suppressLineEditChange_ = true;
      lineEdit_->setText(searchText_);
      suppressLineEditChange_ = false;
    }
    refreshRows();
  }
  if (mode_ != Mode::Single) {
    updateDisplay();
  }
}

int AdSelect::maxCount() const { return maxCount_; }

void AdSelect::setMaxCount(int value) {
  if (maxCount_ == value) {
    return;
  }
  maxCount_ = value;
  enforceMaxCount();
  updateDisplay();
  updateClearButton();
  emit maxCountChanged(maxCount_);
}

int AdSelect::maxTagCount() const { return maxTagCount_; }

void AdSelect::setMaxTagCount(int value) {
  if (maxTagCount_ == value) {
    return;
  }
  maxTagCount_ = value;
  updateDisplay();
  emit maxTagCountChanged(maxTagCount_);
}

bool AdSelect::responsiveMaxTagCount() const { return responsiveMaxTagCount_; }

void AdSelect::setResponsiveMaxTagCount(bool value) {
  if (responsiveMaxTagCount_ == value) {
    return;
  }
  responsiveMaxTagCount_ = value;
  updateDisplay();
  emit responsiveMaxTagCountChanged(responsiveMaxTagCount_);
}

bool AdSelect::autoClearSearchValue() const { return autoClearSearchValue_; }

void AdSelect::setAutoClearSearchValue(bool value) {
  if (autoClearSearchValue_ == value) {
    return;
  }
  autoClearSearchValue_ = value;
  emit autoClearSearchValueChanged(autoClearSearchValue_);
}

AdSelect::Placement AdSelect::placement() const { return placement_; }

void AdSelect::setPlacement(Placement value) {
  if (placement_ == value) {
    return;
  }
  placement_ = value;
  if (open_) {
    syncPopupGeometry();
  }
  emit placementChanged(placement_);
}

bool AdSelect::popupMatchSelectWidth() const { return popupMatchSelectWidth_; }

void AdSelect::setPopupMatchSelectWidth(bool value) {
  if (popupMatchSelectWidth_ == value) {
    return;
  }
  popupMatchSelectWidth_ = value;
  if (open_) {
    syncPopupGeometry();
  }
  emit popupMatchSelectWidthChanged(popupMatchSelectWidth_);
}

int AdSelect::popupWidth() const { return popupWidth_; }

void AdSelect::setPopupWidth(int value) {
  if (popupWidth_ == value) {
    return;
  }
  popupWidth_ = std::max(0, value);
  if (open_) {
    syncPopupGeometry();
  }
  emit popupWidthChanged(popupWidth_);
}

QString AdSelect::placeholder() const { return placeholder_; }

void AdSelect::setPlaceholder(const QString& value) {
  if (placeholder_ == value) {
    return;
  }
  placeholder_ = value;
  updateDisplay();
  emit placeholderChanged(placeholder_);
}

bool AdSelect::joinedLeft() const { return joinedLeft_; }

void AdSelect::setJoinedLeft(bool value) {
  if (joinedLeft_ == value) {
    return;
  }
  joinedLeft_ = value;
  bumpJoinedZOrder();
  updateInteractionFocusOverlay();
  update();
}

bool AdSelect::joinedRight() const { return joinedRight_; }

void AdSelect::setJoinedRight(bool value) {
  if (joinedRight_ == value) {
    return;
  }
  joinedRight_ = value;
  bumpJoinedZOrder();
  updateInteractionFocusOverlay();
  update();
}

QString AdSelect::prefixText() const { return prefixText_; }

void AdSelect::setPrefixText(const QString& value) {
  if (prefixText_ == value) {
    return;
  }
  prefixText_ = value;
  updatePrefixVisual();
  emit prefixTextChanged(prefixText_);
}

adqt::icons::IconToken AdSelect::prefixIconToken() const { return prefixIconToken_; }

void AdSelect::setPrefixIconToken(const adqt::icons::IconToken& token) {
  if (iconTokensEqual(prefixIconToken_, token)) {
    return;
  }
  prefixIconToken_ = token;
  updatePrefixVisual();
  emit prefixIconTokenChanged(prefixIconToken_);
}

adqt::icons::IconToken AdSelect::suffixIconToken() const { return suffixIconToken_; }

void AdSelect::setSuffixIconToken(const adqt::icons::IconToken& token) {
  if (iconTokensEqual(suffixIconToken_, token)) {
    return;
  }
  suffixIconToken_ = token;
  updateInputMode();
  updateSuffixVisual();
  emit suffixIconTokenChanged(suffixIconToken_);
}

QString AdSelect::value() const { return value_; }

void AdSelect::setValue(const QString& value) {
  if (mode_ != Mode::Single) {
    setMode(Mode::Single);
  }
  selectSingleValue(value.trimmed(), true);
  refreshRows();
}

QStringList AdSelect::values() const { return values_; }

void AdSelect::setValues(const QStringList& values) {
  QStringList normalized = normalizedValues(values);
  if (mode_ == Mode::Single) {
    const QString next = normalized.isEmpty() ? QString() : normalized.constFirst();
    selectSingleValue(next, true);
    return;
  }

  if (mode_ == Mode::Tags) {
    for (const QString& value : normalized) {
      ensureTagOptionExists(value);
    }
  }

  if (maxCount_ > 0 && normalized.size() > maxCount_) {
    normalized = normalized.mid(0, maxCount_);
  }
  if (values_ == normalized) {
    return;
  }

  values_ = normalized;
  refreshRows();
  updateDisplay();
  updateClearButton();
  emit valuesChanged(values_);
  emitSelectionChangedSignals();
}

QVector<AdSelect::SelectionItem> AdSelect::selectedItems() const {
  QVector<SelectionItem> items;
  items.reserve(values_.size());
  for (const QString& current : values_) {
    SelectionItem item;
    item.value = current;
    item.label = fallbackSelectedLabel(current);
    items.append(item);
  }
  return items;
}

QVector<AdSelect::Option> AdSelect::options() const { return options_; }

void AdSelect::setOptions(const QVector<Option>& options) {
  options_ = options;
  if (mode_ == Mode::Tags) {
    for (const QString& current : values_) {
      ensureTagOptionExists(current);
    }
  }
  refreshRows();
  updateDisplay();
  updateClearButton();
  emit optionsChanged();
}

void AdSelect::appendOption(const Option& option) {
  options_.append(option);
  refreshRows();
  updateDisplay();
  emit optionsChanged();
}

void AdSelect::clearOptions() {
  if (options_.isEmpty()) {
    return;
  }
  options_.clear();
  refreshRows();
  updateDisplay();
  emit optionsChanged();
}

void AdSelect::setSearchFilterFields(const QStringList& fields) {
  const QStringList normalized = uniqueStringList(fields);
  if (searchFilterFields_ == normalized) {
    return;
  }
  searchFilterFields_ = normalized;
  refreshRows();
}

QStringList AdSelect::searchFilterFields() const { return searchFilterFields_; }

void AdSelect::setFilterPredicate(FilterPredicate predicate) {
  filterPredicate_ = std::move(predicate);
  refreshRows();
}

AdSelect::FilterPredicate AdSelect::filterPredicate() const { return filterPredicate_; }

void AdSelect::setSortComparator(SortComparator comparator) {
  sortComparator_ = std::move(comparator);
  refreshRows();
}

AdSelect::SortComparator AdSelect::sortComparator() const { return sortComparator_; }

void AdSelect::setTokenSeparators(const QStringList& separators) {
  tokenSeparators_ = uniqueStringList(separators);
}

QStringList AdSelect::tokenSeparators() const { return tokenSeparators_; }

void AdSelect::setOptionTextFormatter(OptionTextFormatter formatter) {
  optionTextFormatter_ = std::move(formatter);
  refreshRows();
  updateDisplay();
}

AdSelect::OptionTextFormatter AdSelect::optionTextFormatter() const { return optionTextFormatter_; }

void AdSelect::setTagTextFormatter(TagTextFormatter formatter) {
  tagTextFormatter_ = std::move(formatter);
  updateDisplay();
}

AdSelect::TagTextFormatter AdSelect::tagTextFormatter() const { return tagTextFormatter_; }

void AdSelect::setLabelFormatter(LabelFormatter formatter) {
  labelFormatter_ = std::move(formatter);
  updateDisplay();
}

AdSelect::LabelFormatter AdSelect::labelFormatter() const { return labelFormatter_; }

void AdSelect::setPopupExtraContentFactory(PopupExtraContentFactory factory) {
  popupExtraContentFactory_ = std::move(factory);
  if (open_) {
    rebuildPopupExtraContent();
    syncPopupGeometry();
  }
}

AdSelect::PopupExtraContentFactory AdSelect::popupExtraContentFactory() const {
  return popupExtraContentFactory_;
}

AdSelect::ComponentTokens AdSelect::componentTokens() const { return componentTokens_; }

void AdSelect::setComponentTokens(const ComponentTokens& tokens) {
  componentTokens_ = tokens;
  applyVisualStyle();
}

void AdSelect::resetComponentTokens() {
  componentTokens_ = ComponentTokens();
  applyVisualStyle();
}

AdSelect::SemanticStyles AdSelect::semanticStyles() const { return semanticStyles_; }

void AdSelect::setSemanticStyles(const SemanticStyles& styles) {
  semanticStyles_ = styles;
  applyVisualStyle();
}

void AdSelect::setSemanticStyleResolver(SemanticStyleResolver resolver) {
  semanticStyleResolver_ = std::move(resolver);
  applyVisualStyle();
}

QSize AdSelect::sizeHint() const {
  int height = visualStyle_ ? visualStyle_->metrics.height : 32;
  if (mode_ != Mode::Single) {
    height = std::max(height, this->height());
  }
  return QSize(240, height);
}

QSize AdSelect::minimumSizeHint() const {
  int height = visualStyle_ ? visualStyle_->metrics.height : 32;
  if (mode_ != Mode::Single) {
    height = std::max(height, this->height());
  }
  return QSize(120, height);
}

bool AdSelect::eventFilter(QObject* watched, QEvent* event) {
  if (!watched || !event) {
    return QWidget::eventFilter(watched, event);
  }

  if (watched == lineEdit_) {
    if (event->type() == QEvent::MouseButtonPress) {
      if (lineEdit_->isReadOnly()) {
        if (!disabled()) {
          if (open_) {
            closePopup();
          } else {
            openPopup();
          }
        }
        lineEdit_->deselect();
        return true;
      }
      if (!disabled()) {
        // Single-select should behave like a pure toggle target even when
        // search is enabled: repeated clicks on the selector close popup.
        if (mode_ == Mode::Single && open_) {
          closePopup();
          return true;
        }
        if (!open_) {
          openPopup();
        }
      }
    } else if (event->type() == QEvent::MouseMove) {
      if (lineEdit_->isReadOnly()) {
        return true;
      }
    } else if (event->type() == QEvent::MouseButtonRelease) {
      if (lineEdit_->isReadOnly()) {
        return true;
      }
    } else if (event->type() == QEvent::MouseButtonDblClick) {
      if (lineEdit_->isReadOnly()) {
        lineEdit_->deselect();
        return true;
      }
    } else if (event->type() == QEvent::ContextMenu) {
      if (lineEdit_->isReadOnly()) {
        return true;
      }
    } else if (event->type() == QEvent::FocusIn) {
      updateFocusState();
    } else if (event->type() == QEvent::FocusOut) {
      if (!open_) {
        updateFocusState();
      }
      if (mode_ != Mode::Single && !inputMethodPreeditText_.isEmpty()) {
        inputMethodPreeditText_.clear();
        updateDisplay();
      }
    } else if (event->type() == QEvent::InputMethod) {
      if (mode_ != Mode::Single) {
        const auto* inputMethodEvent = static_cast<const QInputMethodEvent*>(event);
        const QString preeditText =
            inputMethodEvent ? inputMethodEvent->preeditString() : QString();
        if (inputMethodPreeditText_ != preeditText) {
          inputMethodPreeditText_ = preeditText;
          updateDisplay();
        }
      }
    } else if (event->type() == QEvent::KeyPress) {
      auto* keyEvent = static_cast<QKeyEvent*>(event);
      if (lineEdit_->isReadOnly() && keyEvent->matches(QKeySequence::SelectAll)) {
        return true;
      }
      if (keyEvent->key() == Qt::Key_Down) {
        if (!open_) {
          openPopup();
          return true;
        }
      } else if (keyEvent->key() == Qt::Key_Escape) {
        if (open_) {
          closePopup();
          return true;
        }
      } else if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
        if (mode_ == Mode::Tags) {
          const QString token = lineEdit_->text().trimmed();
          if (!token.isEmpty() && addTagValue(token)) {
            if (autoClearSearchValue_) {
              setSearchText(QString());
            }
            suppressLineEditChange_ = true;
            lineEdit_->clear();
            suppressLineEditChange_ = false;
            refreshRows();
            updateDisplay();
            return true;
          }
        }
        if (open_ && listView_ && listView_->currentIndex().isValid()) {
          const int row = listView_->currentIndex().row();
          if (row >= 0 && row < rows_.size()) {
            const ModelRow& modelRow = rows_.at(row);
            if (modelRow.optionIndex >= 0 && modelRow.optionIndex < options_.size()) {
              toggleSelectionForOption(options_.at(modelRow.optionIndex));
              return true;
            }
          }
        }
      } else if (keyEvent->key() == Qt::Key_Backspace &&
                 (mode_ == Mode::Multiple || mode_ == Mode::Tags)) {
        if (lineEdit_->text().isEmpty() && !values_.isEmpty()) {
          const QString removed = values_.takeLast();
          emit deselected(removed, fallbackSelectedLabel(removed));
          emit valuesChanged(values_);
          emitSelectionChangedSignals();
          refreshRows();
          updateDisplay();
          updateClearButton();
          return true;
        }
      }
    }
  } else if (watched == clearButton_) {
    if (event->type() == QEvent::Enter) {
      clearHovered_ = true;
      updateClearButton();
    } else if (event->type() == QEvent::Leave) {
      clearHovered_ = false;
      updateClearButton();
    }
  } else if (watched == popup_) {
    if (event->type() == QEvent::Hide) {
      if (open_) {
        setOpenInternal(false, true);
      }
    }
  } else if (watched == listView_) {
    if (event->type() == QEvent::KeyPress) {
      auto* keyEvent = static_cast<QKeyEvent*>(event);
      if (keyEvent->key() == Qt::Key_Escape) {
        closePopup();
        return true;
      }
      if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
        if (listView_->currentIndex().isValid()) {
          const int row = listView_->currentIndex().row();
          if (row >= 0 && row < rows_.size()) {
            const ModelRow& modelRow = rows_.at(row);
            if (modelRow.optionIndex >= 0 && modelRow.optionIndex < options_.size()) {
              toggleSelectionForOption(options_.at(modelRow.optionIndex));
              return true;
            }
          }
        }
      }
    }
  } else if (listView_ && watched == listView_->viewport()) {
    if (event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonPress ||
        event->type() == QEvent::MouseButtonRelease) {
      const auto* mouseEvent = static_cast<const QMouseEvent*>(event);
      syncPopupOptionCursor(mouseEvent->pos());
    } else if (event->type() == QEvent::Enter) {
      syncPopupOptionCursor(listView_->viewport()->mapFromGlobal(QCursor::pos()));
    } else if (event->type() == QEvent::Leave) {
      syncPopupOptionCursor(QPoint(-1, -1));
    }
  }
  return QWidget::eventFilter(watched, event);
}

void AdSelect::paintEvent(QPaintEvent* event) {
  QWidget::paintEvent(event);
  QPainter painter(this);
  paintSelectorShell(painter);
}

QRectF AdSelect::selectorPaintRect() const {
  if (!visualStyle_) {
    return rect();
  }
  return joinedSelectorRect(rect(), visualStyle_->metrics.borderWidth, joinedLeft_, joinedRight_);
}

QColor AdSelect::resolveSelectorBgColor() const {
  if (!visualStyle_) {
    return QColor();
  }

  if (disabled()) {
    return visualStyle_->selectorBg;
  }
  if (hasFocusWithin_ || open_) {
    return visualStyle_->selectorActiveBg;
  }
  if (hovered_) {
    return visualStyle_->selectorHoverBg;
  }
  return visualStyle_->selectorBg;
}

QColor AdSelect::resolveSelectorBorderColor() const {
  if (!visualStyle_) {
    return QColor();
  }

  if (disabled()) {
    return visualStyle_->selectorBorderColor;
  }
  if (hasFocusWithin_ || open_) {
    return visualStyle_->selectorActiveBorderColor;
  }
  if (hovered_) {
    return visualStyle_->selectorHoverBorderColor;
  }
  return visualStyle_->selectorBorderColor;
}

qreal AdSelect::resolveSelectorRadius() const {
  if (!visualStyle_) {
    return 0.0;
  }
  if (variant_ == Variant::Underlined) {
    return 0.0;
  }
  return std::max<qreal>(0.0, visualStyle_->metrics.borderRadius);
}

void AdSelect::paintSelectorShell(QPainter& painter) const {
  if (!visualStyle_) {
    return;
  }

  const QRectF shellRect = selectorPaintRect();
  if (!shellRect.isValid() || shellRect.width() <= 0.0 || shellRect.height() <= 0.0) {
    return;
  }

  const QColor background = resolveSelectorBgColor();
  const QColor border = resolveSelectorBorderColor();
  const qreal borderWidth = std::max<qreal>(0.0, visualStyle_->metrics.borderWidth);
  const qreal radius = resolveSelectorRadius();
  const qreal topLeftRadius = joinedLeft_ ? 0.0 : radius;
  const qreal topRightRadius = joinedRight_ ? 0.0 : radius;
  const qreal bottomRightRadius = joinedRight_ ? 0.0 : radius;
  const qreal bottomLeftRadius = joinedLeft_ ? 0.0 : radius;

  painter.save();
  painter.setRenderHint(QPainter::Antialiasing, true);

  const QPainterPath shellPath =
      roundedRectPath(shellRect, topLeftRadius, topRightRadius, bottomRightRadius, bottomLeftRadius);
  if (background.alpha() > 0) {
    painter.fillPath(shellPath, background);
  }

  if (variant_ == Variant::Underlined) {
    if (borderWidth > 0.0 && border.alpha() > 0) {
      QPen underlinePen(border, borderWidth, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
      painter.setPen(underlinePen);
      painter.setBrush(Qt::NoBrush);
      const qreal y = shellRect.bottom();
      painter.drawLine(QPointF(shellRect.left(), y), QPointF(shellRect.right(), y));
    }
  } else if (borderWidth > 0.0 && border.alpha() > 0) {
    QPen borderPen(border, borderWidth, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
    painter.setPen(borderPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(shellPath);
  }

  painter.restore();
}

void AdSelect::enterEvent(QEnterEvent* event) {
  QWidget::enterEvent(event);
  hovered_ = true;
  bumpJoinedZOrder();
  updateClearButton();
  update();
}

void AdSelect::leaveEvent(QEvent* event) {
  QWidget::leaveEvent(event);
  hovered_ = false;
  if (!clearButton_ || !clearButton_->underMouse()) {
    clearHovered_ = false;
  }
  updateClearButton();
  update();
}

void AdSelect::mousePressEvent(QMouseEvent* event) {
  if (!disabled() && event && event->button() == Qt::LeftButton) {
    if (clearButton_ && clearButton_->geometry().contains(event->pos())) {
      QWidget::mousePressEvent(event);
      return;
    }
    const bool readOnlyDisplay = lineEdit_ && lineEdit_->isReadOnly();
    if (readOnlyDisplay) {
      if (open_) {
        closePopup();
      } else {
        openPopup();
      }
    } else {
      if (lineEdit_ && !lineEdit_->hasFocus()) {
        lineEdit_->setFocus(Qt::MouseFocusReason);
      }
      // Keep multiple/tags editable interaction unchanged, but in single mode
      // clicking the selector while open should close the popup.
      if (mode_ == Mode::Single && open_) {
        closePopup();
      } else if (!open_) {
        openPopup();
      }
    }
  }
  QWidget::mousePressEvent(event);
}

void AdSelect::keyPressEvent(QKeyEvent* event) {
  if (!event || disabled()) {
    QWidget::keyPressEvent(event);
    return;
  }

  if (event->key() == Qt::Key_Down) {
    if (!open_) {
      openPopup();
      return;
    }
  } else if (event->key() == Qt::Key_Escape) {
    if (open_) {
      closePopup();
      return;
    }
  } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
    if (!open_) {
      openPopup();
      return;
    }
  }

  QWidget::keyPressEvent(event);
}

void AdSelect::moveEvent(QMoveEvent* event) {
  QWidget::moveEvent(event);
  updateInteractionFocusOverlay();
}

void AdSelect::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  if (responsiveMaxTagCount_ || mode_ != Mode::Single) {
    updateDisplay();
  }
  if (open_) {
    syncPopupGeometry();
  }
  updateAccessoryGeometry();
  updateInteractionFocusOverlay();
}

void AdSelect::changeEvent(QEvent* event) {
  QWidget::changeEvent(event);
  if (!event) {
    return;
  }
  if (event->type() == QEvent::Hide) {
    detail::cancelTimingTask(this, QString::fromLatin1(kShowLayoutRefreshKey));
    hovered_ = false;
    clearHovered_ = false;
    updateClearButton();
    stopInteractionFocusForOwner(this);
    return;
  }
  if (event->type() == QEvent::Show) {
    updateInteractionFocusOverlay();
    if (mode_ != Mode::Single || responsiveMaxTagCount_) {
      detail::deferTimingTask(this, QString::fromLatin1(kShowLayoutRefreshKey), [this]() {
        if (!isVisible()) {
          return;
        }
        updateDisplay();
        updateClearButton();
        updateAccessoryGeometry();
      });
    }
    return;
  }
  if (event->type() == QEvent::EnabledChange || event->type() == QEvent::PaletteChange ||
      event->type() == QEvent::FontChange) {
    if (event->type() == QEvent::EnabledChange && disabled()) {
      hovered_ = false;
    }
    updateInputMode();
    applyVisualStyle();
    updateDisplay();
    updateClearButton();
    updateSuffixVisual();
    update();
  }
}

bool AdSelect::isSearchEnabledForCurrentMode() const {
  if (mode_ == Mode::Single) {
    return searchEnabled_;
  }
  return true;
}

bool AdSelect::isValueSelected(const QString& value) const {
  if (mode_ == Mode::Single) {
    return !value_.isEmpty() && value_ == value;
  }
  return values_.contains(value);
}

int AdSelect::indexOfOptionValue(const QString& value) const {
  for (int i = 0; i < options_.size(); ++i) {
    if (options_.at(i).value == value) {
      return i;
    }
  }
  return -1;
}

const AdSelect::Option* AdSelect::findOption(const QString& value) const {
  const int index = indexOfOptionValue(value);
  if (index < 0 || index >= options_.size()) {
    return nullptr;
  }
  return &options_.at(index);
}

AdSelect::Option* AdSelect::findOption(const QString& value) {
  const int index = indexOfOptionValue(value);
  if (index < 0 || index >= options_.size()) {
    return nullptr;
  }
  return &options_[index];
}

QString AdSelect::optionLabelOrFallback(const Option& option) const {
  const QString trimmed = option.label.trimmed();
  return trimmed.isEmpty() ? option.value : trimmed;
}

QString AdSelect::optionSearchFieldValue(const Option& option, const QString& field) const {
  if (field == QStringLiteral("label")) {
    return optionLabelOrFallback(option);
  }
  if (field == QStringLiteral("value")) {
    return option.value;
  }
  return option.metadata.value(field).toString();
}

QString AdSelect::formattedOptionText(const Option& option) const {
  if (optionTextFormatter_) {
    return optionTextFormatter_(option);
  }
  return optionLabelOrFallback(option);
}

QString AdSelect::formattedTagText(const Option& option) const {
  if (tagTextFormatter_) {
    return tagTextFormatter_(option);
  }
  return optionLabelOrFallback(option);
}

QString AdSelect::formattedSelectedLabel(const Option& option) const {
  if (labelFormatter_) {
    return labelFormatter_(option);
  }
  return optionLabelOrFallback(option);
}

QString AdSelect::fallbackSelectedLabel(const QString& value) const {
  const Option* option = findOption(value);
  if (!option) {
    return value;
  }
  if (mode_ == Mode::Multiple || mode_ == Mode::Tags) {
    return formattedTagText(*option);
  }
  return formattedSelectedLabel(*option);
}

QStringList AdSelect::normalizedValues(const QStringList& values) const {
  return uniqueStringList(values);
}

int AdSelect::responsiveVisibleTagCount(const QStringList& labels, int availableWidth) const {
  if (labels.isEmpty() || availableWidth <= 0) {
    return 0;
  }

  const QFontMetrics fm(tagsContainer_ ? tagsContainer_->font() : font());
  const int tagPaddingStart =
      std::max(4, visualStyle_ ? visualStyle_->metrics.tagPaddingInlineStart : 8);
  const int tagPaddingEnd =
      std::max(2, visualStyle_ ? visualStyle_->metrics.tagPaddingInlineEnd : 4);
  const int tagInnerGap =
      std::max(2, visualStyle_ ? visualStyle_->metrics.tagContentGap : 4);
  const int removeIconWidth =
      std::max(8, visualStyle_ ? (visualStyle_->metrics.iconSize - 2) : 10);
  const int interTagGap =
      std::max(2, visualStyle_ ? visualStyle_->metrics.tagItemGap : 4);

  const auto selectedTagWidth = [&](const QString& text) {
    return fm.horizontalAdvance(text) + tagPaddingStart + tagPaddingEnd + removeIconWidth + tagInnerGap;
  };
  const auto restTagWidth = [&](int hiddenCount) {
    if (hiddenCount <= 0) {
      return 0;
    }
    const QString restText = QStringLiteral("+%1...").arg(hiddenCount);
    return fm.horizontalAdvance(restText) + tagPaddingStart + tagPaddingEnd;
  };

  QVector<int> prefixWidths(labels.size() + 1, 0);
  for (int i = 0; i < labels.size(); ++i) {
    const int gap = i > 0 ? interTagGap : 0;
    prefixWidths[i + 1] = prefixWidths[i] + gap + selectedTagWidth(labels.at(i));
  }

  int bestVisibleCount = 0;
  for (int candidate = 0; candidate <= labels.size(); ++candidate) {
    const int hiddenCount = labels.size() - candidate;
    int totalWidth = prefixWidths[candidate];
    if (hiddenCount > 0) {
      if (candidate > 0) {
        totalWidth += interTagGap;
      }
      totalWidth += restTagWidth(hiddenCount);
    }
    if (totalWidth <= availableWidth) {
      bestVisibleCount = candidate;
    }
  }

  return bestVisibleCount;
}

void AdSelect::clearTagWidgets() {
  if (!tagsLayout_) {
    return;
  }

  while (QLayoutItem* item = tagsLayout_->takeAt(0)) {
    if (QWidget* widget = item->widget()) {
      if (widget == lineEdit_) {
        delete item;
        continue;
      }
      delete widget;
    }
    delete item;
  }
}

void AdSelect::rebuildTagWidgets() {
  if (!tagsContainer_ || !tagsLayout_) {
    return;
  }

  clearTagWidgets();

  if (mode_ == Mode::Single) {
    tagsContainer_->setVisible(false);
    tagsContainer_->setToolTip(QString());
    return;
  }

  QStringList labels;
  labels.reserve(values_.size());
  for (const QString& current : values_) {
    labels.append(fallbackSelectedLabel(current));
  }

  int visibleCount = labels.size();
  if (maxTagCount_ >= 0) {
    visibleCount = std::min(visibleCount, maxTagCount_);
  }
  if (responsiveMaxTagCount_) {
    int availableWidth = contentHost_ ? contentHost_->width() : width();
    const int reservedInputWidth =
        std::max(24, (visualStyle_ ? visualStyle_->metrics.tagHeight : 20) + 6);
    availableWidth = std::max(0, availableWidth - reservedInputWidth);
    visibleCount = std::min(visibleCount, responsiveVisibleTagCount(labels, availableWidth));
  }
  visibleCount = std::clamp(visibleCount, 0, static_cast<int>(labels.size()));
  const int hiddenCount = labels.size() - visibleCount;
  const bool hasPrefix = prefixLabel_ && prefixLabel_->isVisible();
  int contentWidth = tagsContainer_->contentsRect().width();
  if (contentHost_) {
    contentWidth = std::max(contentWidth, contentHost_->contentsRect().width());
  }
  if (contentWidth <= 0) {
    contentWidth = contentsRect().width();
  }
  if (contentWidth <= 0) {
    contentWidth = std::max(1, width());
  }
  constexpr int kFixedInputMinWidth = 4;
  const int maxTagItemWidth = std::max(kFixedInputMinWidth, contentWidth - kFixedInputMinWidth);

  const int tagHeight = std::max(16, visualStyle_ ? visualStyle_->metrics.tagHeight : 20);
  const int tagRadius = std::max(0, visualStyle_ ? visualStyle_->metrics.tagBorderRadius : 4);
  const int tagBorderWidth = std::max(0, visualStyle_ ? visualStyle_->metrics.borderWidth : 1);
  const int tagItemMargin =
      std::max(0, visualStyle_ ? visualStyle_->metrics.tagItemMargin : 2);
  const int tagOuterHeight = std::max(1, tagHeight + tagItemMargin * 2);
  const int tagPaddingStart =
      std::max(4, visualStyle_ ? visualStyle_->metrics.tagPaddingInlineStart : 8);
  const int tagPaddingEnd =
      std::max(2, visualStyle_ ? visualStyle_->metrics.tagPaddingInlineEnd : 4);
  const int tagGap =
      std::max(2, visualStyle_ ? visualStyle_->metrics.tagContentGap : 4);
  const int removeIconSize =
      std::max(8, visualStyle_ ? (visualStyle_->metrics.iconSize - 2) : 10);
  const QColor tagBg = visualStyle_ ? visualStyle_->tagBg : QColor("#f5f5f5");
  const QColor tagBorderColor = visualStyle_ ? visualStyle_->tagBorderColor : QColor(0, 0, 0, 0);
  const QColor tagTextColor = visualStyle_ ? visualStyle_->tagTextColor : QColor("#141414");
  const QColor disabledTextColor = visualStyle_ ? visualStyle_->disabledTextColor : QColor("#bfbfbf");
  const QColor removeColor = visualStyle_ ? visualStyle_->clearColor : QColor("#8c8c8c");
  const QColor removeHoverColor = visualStyle_ ? visualStyle_->clearHoverColor : QColor("#595959");

  const auto buildTag = [this,
                         tagHeight,
                         tagOuterHeight,
                         tagRadius,
                         tagBorderWidth,
                         tagItemMargin,
                         tagPaddingStart,
                         tagPaddingEnd,
                         tagGap,
                         removeIconSize,
                         maxTagItemWidth,
                         tagBg,
                         tagBorderColor,
                         tagTextColor,
                         disabledTextColor,
                          removeColor,
                          removeHoverColor](const QString& text,
                                            const QString& value,
                                            bool removable) {
    auto* chip = new TagChipWidget(tagsContainer_);
    chip->setObjectName(QStringLiteral("adselect-tag-item"));
    chip->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    setWidgetFixedHeightIfChanged(chip, tagOuterHeight);
    chip->setMaximumWidth(maxTagItemWidth);

    auto* chipLayout = new QHBoxLayout(chip);
    chipLayout->setContentsMargins(tagPaddingStart, tagItemMargin, tagPaddingEnd, tagItemMargin);
    chipLayout->setSpacing(tagGap);

    const bool showRemoveButton = removable && !disabled() && !value.isEmpty();
    const int removeSlotWidth = showRemoveButton ? (removeIconSize + tagGap) : 0;
    const int textAvailableWidth =
        std::max(8, maxTagItemWidth - tagPaddingStart - tagPaddingEnd - removeSlotWidth);
    auto* label = new QLabel(chip);
    label->setObjectName(QStringLiteral("adselect-tag-text"));
    label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    label->setMinimumWidth(1);
    label->setMaximumWidth(textAvailableWidth);
    label->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    const QFontMetrics tagLabelMetrics(label->font());
    const QString displayText = tagLabelMetrics.elidedText(text, Qt::ElideRight, textAvailableWidth);
    label->setText(displayText);
    label->setToolTip(displayText != text ? text : QString());
    setWidgetFixedHeightIfChanged(label, std::max(1, tagHeight - tagBorderWidth * 2));
    chipLayout->addWidget(label);

    QPalette chipPalette = chip->palette();
    chipPalette.setColor(QPalette::Window, tagBg);
    chipPalette.setColor(QPalette::WindowText, tagTextColor);
    chipPalette.setColor(QPalette::Disabled, QPalette::WindowText, disabledTextColor);
    chip->setPalette(chipPalette);

    QPalette labelPalette = label->palette();
    labelPalette.setColor(QPalette::WindowText, tagTextColor);
    labelPalette.setColor(QPalette::Disabled, QPalette::WindowText, disabledTextColor);
    label->setPalette(labelPalette);

    chip->setVisualStyle(tagBg, tagBorderColor, tagRadius, tagBorderWidth, tagItemMargin);

    setWidgetCursorIfChanged(chip, selectorCursorShape());
    setWidgetCursorIfChanged(label, selectorCursorShape());

    if (showRemoveButton) {
      auto* removeButton = new FlatIconToolButton(chip);
      removeButton->setObjectName(QStringLiteral("adselect-tag-remove"));
      removeButton->setText(QString());
      removeButton->setFixedSize(removeIconSize, removeIconSize);
      removeButton->setIconSize(QSize(removeIconSize, removeIconSize));
      removeButton->setCursor(Qt::PointingHandCursor);

      adqt::icons::IconToken closeIcon = outlined_icons::Close();
      if (adqt::icons::isValid(closeIcon)) {
        closeIcon.style.primary = removeColor;
        closeIcon.style.hasPrimary = true;
        const qreal dpr = devicePixelRatioF();
        QPixmap closePixmap = adqt::icons::renderIconPixmap(
            closeIcon, QSize(removeIconSize, removeIconSize), dpr, QIcon::Normal, QIcon::Off);
        QIcon closeButtonIcon;
        if (!closePixmap.isNull()) {
          closeButtonIcon.addPixmap(closePixmap, QIcon::Normal, QIcon::Off);
        }
        adqt::icons::IconToken hoverIcon = closeIcon;
        hoverIcon.style.primary = removeHoverColor;
        QPixmap closeHoverPixmap = adqt::icons::renderIconPixmap(
            hoverIcon, QSize(removeIconSize, removeIconSize), dpr, QIcon::Active, QIcon::Off);
        if (!closeHoverPixmap.isNull()) {
          closeButtonIcon.addPixmap(closeHoverPixmap, QIcon::Active, QIcon::Off);
        }
        if (!closeButtonIcon.isNull()) {
          removeButton->setIcon(closeButtonIcon);
        }
      } else {
        removeButton->setText(QStringLiteral("x"));
      }

      connect(removeButton, &QToolButton::clicked, this, [this, value]() {
        if (value.isEmpty() || disabled()) {
          return;
        }
        const bool shouldRestoreInputFocus = open_ && lineEdit_ && !lineEdit_->isReadOnly();
        const int index = values_.indexOf(value);
        if (index < 0) {
          return;
        }
        values_.removeAt(index);
        emit deselected(value, fallbackSelectedLabel(value));
        emit valuesChanged(values_);
        emitSelectionChangedSignals();
        refreshRows();
        updateDisplay();
        updateClearButton();
        if (shouldRestoreInputFocus && lineEdit_ && !lineEdit_->hasFocus()) {
          lineEdit_->setFocus(Qt::MouseFocusReason);
        }
      });
      chipLayout->addWidget(removeButton, 0, Qt::AlignVCenter);
    }

    tagsLayout_->addWidget(chip);
  };

  bool hasRenderedTag = false;
  for (int i = 0; i < visibleCount && i < labels.size() && i < values_.size(); ++i) {
    buildTag(labels.at(i), values_.at(i), true);
    hasRenderedTag = true;
  }
  if (hiddenCount > 0) {
    buildTag(QStringLiteral("+%1...").arg(hiddenCount), QString(), false);
    hasRenderedTag = true;
  }

  if (lineEdit_ && lineEdit_->parentWidget() == tagsContainer_) {
    if (!hasPrefix && !hasRenderedTag && visualStyle_) {
      auto* spacer = new QWidget(tagsContainer_);
      spacer->setObjectName(QStringLiteral("adselect-tag-leading-spacer"));
      spacer->setAttribute(Qt::WA_TransparentForMouseEvents, true);
      spacer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
      const int leadingInset = std::max(0, visualStyle_->metrics.multipleItemPaddingHorizontal);
      const int itemGap = std::max(0, tagsLayout_->spacing());
      spacer->setFixedWidth(std::max(0, leadingInset - itemGap));
      spacer->setFixedHeight(std::max(1, tagOuterHeight));
      tagsLayout_->addWidget(spacer);
    }
    tagsLayout_->addWidget(lineEdit_);
  }

  tagsContainer_->setVisible(true);
  tagsContainer_->setToolTip(labels.join(QStringLiteral(", ")));
}

void AdSelect::enforceMaxCount() {
  if (maxCount_ <= 0) {
    return;
  }
  if (mode_ == Mode::Single) {
    return;
  }
  if (values_.size() > maxCount_) {
    values_ = values_.mid(0, maxCount_);
    emit valuesChanged(values_);
    emitSelectionChangedSignals();
  }
}

bool AdSelect::suffixButtonTriggersPopup() const {
  if (disabled()) {
    return false;
  }
  // Keep Ant Design behavior: custom suffix icons are decorative by default and
  // should not change popup visibility.
  return !adqt::icons::isValid(suffixIconToken_);
}

Qt::CursorShape AdSelect::selectorCursorShape() const {
  if (disabled()) {
    return Qt::ForbiddenCursor;
  }
  return isSearchEnabledForCurrentMode() ? Qt::IBeamCursor : Qt::PointingHandCursor;
}

Qt::CursorShape AdSelect::optionCursorShapeAtRow(int row) const {
  if (disabled()) {
    return Qt::ForbiddenCursor;
  }
  if (row < 0 || row >= rows_.size()) {
    return Qt::ArrowCursor;
  }

  const ModelRow& modelRow = rows_.at(row);
  if (modelRow.optionIndex < 0 || modelRow.optionIndex >= options_.size()) {
    return Qt::ArrowCursor;
  }
  return options_.at(modelRow.optionIndex).disabled ? Qt::ForbiddenCursor : Qt::PointingHandCursor;
}

void AdSelect::syncPopupOptionCursor(const QPoint& viewportPos) {
  if (!listView_) {
    return;
  }
  QWidget* viewport = listView_->viewport();
  if (!viewport) {
    return;
  }

  Qt::CursorShape shape = disabled() ? Qt::ForbiddenCursor : Qt::ArrowCursor;
  if (!disabled()) {
    const QModelIndex hoveredIndex = listView_->indexAt(viewportPos);
    shape = optionCursorShapeAtRow(hoveredIndex.isValid() ? hoveredIndex.row() : -1);
  }
  setWidgetCursorIfChanged(viewport, shape);
}

void AdSelect::syncContentLayoutForMode() {
  if (!contentLayout_ || !contentHost_ || !tagsContainer_ || !tagsLayout_ || !lineEdit_) {
    return;
  }

  const bool multipleMode = mode_ != Mode::Single;
  if (multipleMode) {
    if (contentLayout_->indexOf(tagsContainer_) < 0) {
      contentLayout_->insertWidget(0, tagsContainer_, 1);
    }
    if (contentLayout_->indexOf(lineEdit_) >= 0) {
      contentLayout_->removeWidget(lineEdit_);
    }
    if (lineEdit_->parentWidget() != tagsContainer_) {
      lineEdit_->setParent(tagsContainer_);
      lineEdit_->show();
    }
    lineEdit_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    lineEdit_->setMinimumWidth(4);
    if (tagsLayout_->indexOf(lineEdit_) < 0) {
      tagsLayout_->addWidget(lineEdit_);
    }
    if (placeholderLabel_) {
      placeholderLabel_->setParent(tagsContainer_);
      placeholderLabel_->show();
    }
    tagsContainer_->setVisible(true);
    return;
  }

  if (tagsLayout_->indexOf(lineEdit_) >= 0) {
    tagsLayout_->removeWidget(lineEdit_);
  }
  if (lineEdit_->parentWidget() != contentHost_) {
    lineEdit_->setParent(contentHost_);
    lineEdit_->show();
  }
  lineEdit_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  lineEdit_->setMinimumWidth(4);
  lineEdit_->setMaximumWidth(QWIDGETSIZE_MAX);
  if (contentLayout_->indexOf(tagsContainer_) < 0) {
    contentLayout_->insertWidget(0, tagsContainer_);
  }
  if (contentLayout_->indexOf(lineEdit_) < 0) {
    contentLayout_->addWidget(lineEdit_, 1);
  }
  if (placeholderLabel_) {
    placeholderLabel_->hide();
  }
  tagsContainer_->setVisible(false);
}

void AdSelect::updateInputMode() {
  syncContentLayoutForMode();

  const Qt::CursorShape selectorCursor = selectorCursorShape();
  setWidgetCursorIfChanged(this, selectorCursor);
  setWidgetCursorIfChanged(contentHost_, selectorCursor);
  setWidgetCursorIfChanged(prefixLabel_, selectorCursor);
  setWidgetCursorIfChanged(tagsContainer_, selectorCursor);
  setWidgetCursorIfChanged(placeholderLabel_, selectorCursor);
  if (tagsContainer_) {
    const auto tagWidgets =
        tagsContainer_->findChildren<QWidget*>(QStringLiteral("adselect-tag-item"), Qt::FindChildrenRecursively);
    for (QWidget* tagWidget : tagWidgets) {
      setWidgetCursorIfChanged(tagWidget, selectorCursor);
    }
    const auto removeButtons =
        tagsContainer_->findChildren<QToolButton*>(QStringLiteral("adselect-tag-remove"),
                                                   Qt::FindChildrenRecursively);
    const Qt::CursorShape removeCursor = disabled() ? Qt::ForbiddenCursor : Qt::PointingHandCursor;
    for (QToolButton* removeButton : removeButtons) {
      setWidgetCursorIfChanged(removeButton, removeCursor);
    }
  }
  if (suffixButton_) {
    const Qt::CursorShape suffixCursor =
        suffixButtonTriggersPopup() ? Qt::PointingHandCursor : selectorCursor;
    setWidgetCursorIfChanged(suffixButton_, suffixCursor);
  }

  const bool searchable = isSearchEnabledForCurrentMode();
  if (lineEdit_) {
    const bool readOnly = !searchable;
    lineEdit_->setReadOnly(readOnly);
    setWidgetCursorIfChanged(lineEdit_, selectorCursor);
  }

  if (listView_) {
    setWidgetCursorIfChanged(listView_, disabled() ? Qt::ForbiddenCursor : Qt::ArrowCursor);
    if (QWidget* viewport = listView_->viewport()) {
      if (disabled()) {
        setWidgetCursorIfChanged(viewport, Qt::ForbiddenCursor);
      } else {
        syncPopupOptionCursor(viewport->mapFromGlobal(QCursor::pos()));
      }
    }
  }
}

void AdSelect::updateDisplay() {
  if (!lineEdit_ || !tagsContainer_) {
    return;
  }

  suppressLineEditChange_ = true;

  if (mode_ == Mode::Single) {
    if (!inputMethodPreeditText_.isEmpty()) {
      inputMethodPreeditText_.clear();
    }
    clearTagWidgets();
    tagsContainer_->setVisible(false);
    tagsContainer_->setToolTip(QString());
    const QString label = value_.isEmpty() ? QString() : fallbackSelectedLabel(value_);
    const QString targetText = (open_ && isSearchEnabledForCurrentMode()) ? searchText_ : label;
    if (lineEdit_->text() != targetText) {
      lineEdit_->setText(targetText);
    }
    lineEdit_->setPlaceholderText(placeholder_);
    lineEdit_->setToolTip(label);
    lineEdit_->setTextMargins(0, 0, 0, 0);
    resetWidgetHeightConstraintsIfChanged(lineEdit_);
    if (placeholderLabel_) {
      placeholderLabel_->setVisible(false);
    }
  } else {
    rebuildTagWidgets();

    if (open_) {
      if (lineEdit_->text() != searchText_) {
        lineEdit_->setText(searchText_);
      }
    } else {
      if (!inputMethodPreeditText_.isEmpty()) {
        inputMethodPreeditText_.clear();
      }
      if (!lineEdit_->text().isEmpty()) {
        lineEdit_->clear();
      }
    }
    lineEdit_->setPlaceholderText(QString());
    lineEdit_->setToolTip(QString());
    lineEdit_->setTextMargins(0, 0, 0, 0);
    const QFontMetrics inputMetrics(lineEdit_->font());
    const QString inputText = lineEdit_->text();
    QString effectiveInputText = inputText;
    if (!inputMethodPreeditText_.isEmpty()) {
      const qsizetype cursorPosition =
          std::clamp(static_cast<qsizetype>(lineEdit_->cursorPosition()), qsizetype(0),
                     effectiveInputText.size());
      effectiveInputText.insert(cursorPosition, inputMethodPreeditText_);
    }
    const int inputWidth =
        effectiveInputText.isEmpty()
            ? 4
            : std::max(4, inputMetrics.horizontalAdvance(effectiveInputText + QStringLiteral(" ")) + 2);
    int availableInputWidth = tagsContainer_->contentsRect().width();
    if (contentHost_) {
      availableInputWidth = std::max(availableInputWidth, contentHost_->contentsRect().width());
    }
    if (availableInputWidth <= 0) {
      availableInputWidth = contentsRect().width();
    }
    if (availableInputWidth <= 0) {
      availableInputWidth = std::max(4, width());
    }
    const int cappedInputWidth = std::min(inputWidth, std::max(4, availableInputWidth));
    setWidgetFixedWidthIfChanged(lineEdit_, cappedInputWidth);
    const int tagHeight = std::max(16, visualStyle_ ? visualStyle_->metrics.tagHeight : 20);
    const int tagItemMargin =
        std::max(0, visualStyle_ ? visualStyle_->metrics.tagItemMargin : 2);
    setWidgetFixedHeightIfChanged(lineEdit_, std::max(1, tagHeight + tagItemMargin * 2));

    if (placeholderLabel_) {
      const bool showPlaceholder =
          values_.isEmpty() && effectiveInputText.isEmpty() && !placeholder_.trimmed().isEmpty();
      placeholderLabel_->setText(placeholder_);
      placeholderLabel_->setVisible(showPlaceholder);
    }
  }

  suppressLineEditChange_ = false;
  updateMultipleSelectorHeight();
}

void AdSelect::updateMultipleSelectorHeight() {
  if (!visualStyle_ || mode_ == Mode::Single || !rootLayout_ || !tagsContainer_ || !tagsLayout_) {
    return;
  }

  int availableWidth = tagsContainer_->contentsRect().width();
  if (contentHost_) {
    availableWidth = std::max(availableWidth, contentHost_->contentsRect().width());
  }
  if (availableWidth <= 0) {
    availableWidth = contentsRect().width();
  }
  if (availableWidth <= 0) {
    availableWidth = std::max(1, width());
  }

  int tagsHeight = 0;
  if (auto* wrappingLayout = dynamic_cast<WrappingTagsLayout*>(tagsLayout_)) {
    tagsHeight = wrappingLayout->layoutHeightForWidth(availableWidth);
  } else {
    tagsHeight = tagsLayout_->sizeHint().height();
  }

  const int minTagsHeight = std::max(
      16, visualStyle_->metrics.tagHeight + std::max(0, visualStyle_->metrics.tagItemMargin) * 2);
  tagsHeight = std::max(minTagsHeight, tagsHeight);
  setWidgetFixedHeightIfChanged(tagsContainer_, tagsHeight);

  if (placeholderLabel_) {
    const bool hasPrefix = prefixLabel_ && prefixLabel_->isVisible();
    const int inset =
        hasPrefix ? 0 : std::max(0, visualStyle_->metrics.multipleItemPaddingHorizontal);
    const int labelWidth = std::max(0, availableWidth - inset);
    placeholderLabel_->setGeometry(inset, 0, labelWidth, tagsHeight);
    placeholderLabel_->raise();
  }

  const QMargins rootMargins = rootLayout_->contentsMargins();
  const bool wrappedToMultipleLines = tagsHeight > minTagsHeight;
  const int expandedHeight = tagsHeight + rootMargins.top() + rootMargins.bottom();
  const int targetHeight = wrappedToMultipleLines ? std::max(visualStyle_->metrics.height, expandedHeight)
                                                  : visualStyle_->metrics.height;
  setWidgetFixedHeightIfChanged(this, targetHeight);
  updateGeometry();
}

void AdSelect::updateClearButton() {
  if (!clearButton_ || !visualStyle_) {
    return;
  }
  const bool hasValue = mode_ == Mode::Single ? !value_.isEmpty() : !values_.isEmpty();
  const bool canShow = allowClear_ && hasValue && !disabled();
  if (!canShow) {
    clearHovered_ = false;
  }
  const bool hovered = hovered_ || clearHovered_;
  const bool shouldShow = canShow && hovered;
  if (clearButton_->isVisible() != shouldShow) {
    clearButton_->setVisible(shouldShow);
  }
  updateClearVisual();
  updateAccessoryGeometry();
}

void AdSelect::updateClearVisual() {
  if (!clearButton_ || !visualStyle_) {
    return;
  }

  const int iconSize = std::max(10, visualStyle_->metrics.iconSize);
  clearButton_->setText(QString());
  QColor iconColor = visualStyle_->clearColor;
  if (clearButton_->isVisible() && clearHovered_ && !disabled()) {
    iconColor = visualStyle_->clearHoverColor;
  } else if (disabled()) {
    iconColor = visualStyle_->disabledTextColor;
  }

  adqt::icons::IconToken icon = filled_icons::CloseCircle();
  if (adqt::icons::isValid(icon)) {
    icon.style.primary = iconColor;
    icon.style.hasPrimary = true;
    const qreal dpr = devicePixelRatioF();
    const QPixmap pixmap =
        adqt::icons::renderIconPixmap(icon, QSize(iconSize, iconSize), dpr, QIcon::Normal, QIcon::Off);
    clearButton_->setIcon(QIcon(pixmap));
    clearButton_->setIconSize(QSize(iconSize, iconSize));
  } else {
    clearButton_->setIcon(QIcon());
    clearButton_->setText(QStringLiteral("x"));
  }

  const QColor clearBg =
      clearButton_->isVisible() ? visualStyle_->clearBg : QColor(0, 0, 0, 0);
  const int radius = std::max(0, iconSize / 2);
  if (auto* flatButton = dynamic_cast<FlatIconToolButton*>(clearButton_)) {
    flatButton->setBackgroundDecoration(clearBg, radius);
  }
}

void AdSelect::updateAccessoryGeometry() {
  if (!visualStyle_) {
    return;
  }

  const int iconSize = std::max(10, visualStyle_->metrics.iconSize);
  if (suffixButton_) {
    // Keep suffix footprint aligned with clear overlay, matching Ant Design:
    // clear is absolutely positioned over the suffix slot when it appears.
    suffixButton_->setFixedSize(iconSize, iconSize);
  }

  if (!clearButton_) {
    return;
  }
  clearButton_->setFixedSize(iconSize, iconSize);

  const int borderInset = std::max(0, visualStyle_->metrics.borderWidth);
  const int endInset = std::max(0, visualStyle_->metrics.horizontalPadding) + borderInset;
  const int x = std::max(0, width() - endInset - iconSize);
  const int y = std::max(0, (height() - iconSize) / 2);
  clearButton_->move(x, y);
  clearButton_->raise();
}

void AdSelect::updatePrefixVisual() {
  if (!prefixLabel_ || !visualStyle_) {
    return;
  }

  const bool hasPrefixText = !prefixText_.trimmed().isEmpty();
  const bool hasPrefixIcon = adqt::icons::isValid(prefixIconToken_);
  if (!hasPrefixText && !hasPrefixIcon) {
    prefixLabel_->clear();
    prefixLabel_->setVisible(false);
    return;
  }

  if (hasPrefixText) {
    prefixLabel_->setPixmap(QPixmap());
    prefixLabel_->setText(prefixText_);
    prefixLabel_->setVisible(true);
    return;
  }

  adqt::icons::IconToken icon = prefixIconToken_;
  icon.style.primary = visualStyle_->prefixColor;
  icon.style.hasPrimary = true;
  const qreal dpr = devicePixelRatioF();
  const int iconSize = std::max(10, visualStyle_->metrics.iconSize);
  const QPixmap pixmap =
      adqt::icons::renderIconPixmap(icon, QSize(iconSize, iconSize), dpr, QIcon::Normal, QIcon::Off);
  prefixLabel_->setText(QString());
  prefixLabel_->setPixmap(pixmap);
  prefixLabel_->setVisible(!pixmap.isNull());
}

void AdSelect::updateLoadingSpinnerState() {
  if (loading_) {
    if (!suffixSpinnerSubscribed_) {
      detail::setFrameSubscription(this, QString::fromLatin1(kSuffixSpinnerFrameKey), true,
                                   [this](qint64, qint64) {
                                     if (!loading_) {
                                       return;
                                     }
                                     updateSuffixVisual();
                                   });
      suffixSpinnerSubscribed_ = true;
    }
    return;
  }

  if (suffixSpinnerSubscribed_) {
    detail::clearFrameSubscription(this, QString::fromLatin1(kSuffixSpinnerFrameKey));
    suffixSpinnerSubscribed_ = false;
  }
}

void AdSelect::updateSuffixVisual() {
  if (!suffixButton_ || !visualStyle_) {
    return;
  }

  suffixButton_->setText(QString());
  suffixButton_->setIcon(QIcon());

  if (loading_) {
    const qreal dpr = devicePixelRatioF();
    const int iconSize = std::max(10, visualStyle_->metrics.iconSize);
    const int cycleMs = detail::spinnerCycleDurationMs();
    int angle = 0;
    if (cycleMs > 0) {
      qint64 phaseMs = detail::timingNowMs() % cycleMs;
      if (phaseMs < 0) {
        phaseMs += cycleMs;
      }
      angle = static_cast<int>((phaseMs * 360) / cycleMs);
    }
    QPixmap pixmap =
        makeSpinnerPixmap(QSize(iconSize, iconSize), dpr, visualStyle_->suffixColor, angle);
    if (!pixmap.isNull()) {
      suffixButton_->setIcon(QIcon(pixmap));
      suffixButton_->setIconSize(QSize(iconSize, iconSize));
      return;
    }
    suffixButton_->setText(QStringLiteral("..."));
    return;
  }

  adqt::icons::IconToken icon = suffixIconToken_;
  if (!adqt::icons::isValid(icon)) {
    const bool showSearchIcon = open_ && isSearchEnabledForCurrentMode();
    icon = showSearchIcon ? outlined_icons::Search() : outlined_icons::Down();
  }
  if (adqt::icons::isValid(icon)) {
    icon.style.primary = visualStyle_->suffixColor;
    icon.style.hasPrimary = true;
    const qreal dpr = devicePixelRatioF();
    const int iconSize = std::max(10, visualStyle_->metrics.iconSize);
    const QPixmap pixmap =
        adqt::icons::renderIconPixmap(icon, QSize(iconSize, iconSize), dpr, QIcon::Normal, QIcon::Off);
    if (!pixmap.isNull()) {
      suffixButton_->setIcon(QIcon(pixmap));
      suffixButton_->setIconSize(QSize(iconSize, iconSize));
      return;
    }
  }

  suffixButton_->setText(QStringLiteral("v"));
}

void AdSelect::applyVisualStyle() {
  if (!visualStyle_ || applyingVisualStyle_) {
    return;
  }
  QScopedValueRollback<bool> styleGuard(applyingVisualStyle_, true);

  StyleContext context;
  context.mode = mode_;
  context.size = size_;
  context.variant = variant_;
  context.status = status_;
  context.disabled = disabled();
  context.open = open_;
  context.searchText = searchText_;
  context.values = values_;

  SemanticStyles effectiveSemantic = semanticStyles_;
  if (semanticStyleResolver_) {
    effectiveSemantic = semanticStyleResolver_(context);
  }

  detail::SelectStyleInput input;
  input.mode = mode_;
  input.size = size_;
  input.variant = variant_;
  input.status = status_;
  input.disabled = disabled();
  input.baseFont = font();
  input.componentTokens = componentTokens_;
  input.semanticStyles = effectiveSemantic;
  const detail::SelectVisualStyle previousStyle = *visualStyle_;
  *visualStyle_ = detail::resolveSelectVisualStyle(input);
  const bool prefixIconStyleChanged =
      previousStyle.prefixColor != visualStyle_->prefixColor ||
      previousStyle.metrics.iconSize != visualStyle_->metrics.iconSize;
  const bool suffixIconStyleChanged =
      previousStyle.suffixColor != visualStyle_->suffixColor ||
      previousStyle.metrics.iconSize != visualStyle_->metrics.iconSize;
  const bool listDelegateStyleChanged =
      previousStyle.popupBg != visualStyle_->popupBg ||
      previousStyle.optionTextColor != visualStyle_->optionTextColor ||
      previousStyle.optionHoverBg != visualStyle_->optionHoverBg ||
      previousStyle.optionSelectedBg != visualStyle_->optionSelectedBg ||
      previousStyle.optionSelectedColor != visualStyle_->optionSelectedColor ||
      previousStyle.selectorActiveBorderColor != visualStyle_->selectorActiveBorderColor ||
      previousStyle.disabledTextColor != visualStyle_->disabledTextColor ||
      previousStyle.disabledBg != visualStyle_->disabledBg ||
      previousStyle.metrics.optionBorderRadius != visualStyle_->metrics.optionBorderRadius ||
      previousStyle.metrics.optionPaddingHorizontal != visualStyle_->metrics.optionPaddingHorizontal ||
      previousStyle.metrics.optionPaddingVertical != visualStyle_->metrics.optionPaddingVertical ||
      previousStyle.metrics.iconSize != visualStyle_->metrics.iconSize ||
      previousStyle.metrics.optionFont != visualStyle_->metrics.optionFont;

  bool widgetStyleChanged = false;

  widgetStyleChanged |= setWidgetFontIfChanged(this, visualStyle_->metrics.selectorFont);
  if (lineEdit_) {
    widgetStyleChanged |= setWidgetFontIfChanged(lineEdit_, visualStyle_->metrics.selectorFont);
  }
  if (tagsContainer_) {
    widgetStyleChanged |= setWidgetFontIfChanged(tagsContainer_, visualStyle_->metrics.selectorFont);
  }
  if (placeholderLabel_) {
    widgetStyleChanged |= setWidgetFontIfChanged(placeholderLabel_, visualStyle_->metrics.selectorFont);
  }
  if (prefixLabel_) {
    widgetStyleChanged |= setWidgetFontIfChanged(prefixLabel_, visualStyle_->metrics.selectorFont);
    const int prefixInset = mode_ != Mode::Single
                                ? std::max(0, visualStyle_->metrics.multipleItemPaddingHorizontal)
                                : 0;
    widgetStyleChanged |= setWidgetContentsMarginsIfChanged(prefixLabel_, QMargins(prefixInset, 0, 0, 0));
  }

  const bool multipleMode = mode_ != Mode::Single;
  // QLayout margins are measured from the widget outer rect, while CSS padding
  // is measured from the inner border edge (border-box). Add border inset to
  // match Ant Design's selector content positioning.
  const int borderInset = std::max(0, visualStyle_->metrics.borderWidth);
  const int baseStartPadding = multipleMode ? std::max(0, visualStyle_->metrics.multiplePaddingInlineStart)
                                            : std::max(0, visualStyle_->metrics.horizontalPadding);
  const int baseEndPadding = std::max(0, visualStyle_->metrics.horizontalPadding);
  const int baseVerticalPadding =
      multipleMode ? std::max(0, visualStyle_->metrics.multiplePaddingVertical) : 0;
  const int startPadding = baseStartPadding + borderInset;
  const int endPadding = baseEndPadding + borderInset;
  const int verticalPadding = baseVerticalPadding + borderInset;
  widgetStyleChanged |= setLayoutContentsMarginsIfChanged(
      rootLayout_, QMargins(startPadding, verticalPadding, endPadding, verticalPadding));
  widgetStyleChanged |= setLayoutSpacingIfChanged(rootLayout_, visualStyle_->metrics.spacing);
  widgetStyleChanged |=
      setLayoutSpacingIfChanged(contentLayout_, std::max(0, visualStyle_->metrics.tagItemGap));

  widgetStyleChanged |= setWidgetFixedHeightIfChanged(this, visualStyle_->metrics.height);

  const bool openSingleDisplay =
      mode_ == Mode::Single && open_ && !isSearchEnabledForCurrentMode();
  bool lineEditStyleChanged = false;

  if (lineEdit_) {
    QPalette inputPalette = lineEdit_->palette();
    const QColor inputColor = openSingleDisplay ? visualStyle_->placeholderColor
                                                : visualStyle_->selectorTextColor;
    inputPalette.setColor(QPalette::Text, inputColor);
    inputPalette.setColor(QPalette::Disabled, QPalette::Text, visualStyle_->disabledTextColor);
    inputPalette.setColor(QPalette::Highlight, visualStyle_->optionSelectedBg);
    inputPalette.setColor(QPalette::HighlightedText, visualStyle_->optionSelectedColor);
    inputPalette.setColor(QPalette::PlaceholderText, visualStyle_->placeholderColor);
    lineEditStyleChanged = setWidgetPaletteIfChanged(lineEdit_, inputPalette);
    if (lineEditStyleChanged) {
      lineEdit_->update();
    }
  }

  bool tagsStyleChanged = false;
  if (tagsLayout_) {
    tagsStyleChanged |=
        setLayoutSpacingIfChanged(tagsLayout_, std::max(0, visualStyle_->metrics.tagItemGap));
  }

  bool prefixPaletteChanged = false;
  if (prefixLabel_) {
    QPalette prefixPalette = prefixLabel_->palette();
    prefixPalette.setColor(QPalette::WindowText, visualStyle_->prefixColor);
    prefixPalette.setColor(QPalette::Disabled, QPalette::WindowText, visualStyle_->disabledTextColor);
    prefixPaletteChanged = setWidgetPaletteIfChanged(prefixLabel_, prefixPalette);
    if (prefixPaletteChanged) {
      prefixLabel_->update();
    }
  }
  if (placeholderLabel_) {
    QPalette placeholderPalette = placeholderLabel_->palette();
    placeholderPalette.setColor(QPalette::WindowText, visualStyle_->placeholderColor);
    placeholderPalette.setColor(QPalette::Disabled, QPalette::WindowText, visualStyle_->disabledTextColor);
    const bool placeholderPaletteChanged = setWidgetPaletteIfChanged(placeholderLabel_, placeholderPalette);
    if (placeholderPaletteChanged) {
      placeholderLabel_->update();
    }
    widgetStyleChanged |= placeholderPaletteChanged;
  }

  const auto applyToolButtonPalette = [this](QToolButton* button,
                                             const QColor& textColor) -> bool {
    if (!button || !visualStyle_) {
      return false;
    }
    QPalette palette = button->palette();
    palette.setColor(QPalette::ButtonText, textColor);
    palette.setColor(QPalette::WindowText, textColor);
    palette.setColor(QPalette::Text, textColor);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, visualStyle_->disabledTextColor);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, visualStyle_->disabledTextColor);
    const bool changed = setWidgetPaletteIfChanged(button, palette);
    if (changed) {
      button->update();
    }
    return changed;
  };
  const bool suffixPaletteChanged = applyToolButtonPalette(suffixButton_, visualStyle_->suffixColor);
  const QColor clearPaletteColor = clearHovered_ ? visualStyle_->clearHoverColor : visualStyle_->clearColor;
  const bool clearPaletteChanged = applyToolButtonPalette(clearButton_, clearPaletteColor);

  bool popupStyleChanged = false;
  if (popup_) {
    if (popupLayout_) {
      const int popupPadding = visualStyle_->metrics.popupPadding;
      popupStyleChanged |= setLayoutContentsMarginsIfChanged(
          popupLayout_, QMargins(popupPadding, popupPadding, popupPadding, popupPadding));
      popupStyleChanged |= setLayoutSpacingIfChanged(popupLayout_, 0);
    }

    static_cast<PopupFrame*>(popup_)->setVisualStyle(
        visualStyle_->popupBg, visualStyle_->popupBorderColor, visualStyle_->metrics.popupBorderRadius);

    if (popupScrollArea_) {
      QPalette scrollPalette = popupScrollArea_->palette();
      scrollPalette.setColor(QPalette::Base, visualStyle_->popupBg);
      scrollPalette.setColor(QPalette::Window, visualStyle_->popupBg);
      popupStyleChanged |= setWidgetPaletteIfChanged(popupScrollArea_, scrollPalette);
      if (QWidget* viewport = popupScrollArea_->viewport()) {
        popupStyleChanged |= setWidgetPaletteIfChanged(viewport, scrollPalette);
        popupStyleChanged |= setWidgetAutoFillBackgroundIfChanged(viewport, true);
      }
    }

    if (listView_) {
      QPalette listPalette = listView_->palette();
      listPalette.setColor(QPalette::Base, visualStyle_->popupBg);
      listPalette.setColor(QPalette::Window, visualStyle_->popupBg);
      listPalette.setColor(QPalette::Text, visualStyle_->optionTextColor);
      listPalette.setColor(QPalette::Disabled, QPalette::Text, visualStyle_->disabledTextColor);
      listPalette.setColor(QPalette::Highlight, visualStyle_->optionSelectedBg);
      listPalette.setColor(QPalette::HighlightedText, visualStyle_->optionSelectedColor);
      popupStyleChanged |= setWidgetPaletteIfChanged(listView_, listPalette);
      if (QWidget* viewport = listView_->viewport()) {
        popupStyleChanged |= setWidgetAutoFillBackgroundIfChanged(viewport, true);
      }
    }
  }

  updateInteractionFocusOverlay();
  const bool hasPrefixIcon = prefixText_.trimmed().isEmpty() && adqt::icons::isValid(prefixIconToken_);
  const bool shouldRefreshPrefixIcon = hasPrefixIcon && prefixIconStyleChanged;
  const bool shouldRefreshSuffixIcon = suffixIconStyleChanged || loading_;
  if (shouldRefreshPrefixIcon) {
    updatePrefixVisual();
  }
  if (shouldRefreshSuffixIcon) {
    updateSuffixVisual();
  }
  updateDisplay();
  updateClearButton();
  updateAccessoryGeometry();
  if (widgetStyleChanged || lineEditStyleChanged || tagsStyleChanged || prefixPaletteChanged ||
      suffixPaletteChanged || clearPaletteChanged || shouldRefreshPrefixIcon ||
      shouldRefreshSuffixIcon) {
    update();
  }
  if (listView_ && popup_ && popup_->isVisible() && listView_->viewport() &&
      (popupStyleChanged || listDelegateStyleChanged)) {
    listView_->viewport()->update();
  }
}

void AdSelect::refreshRows() {
  const bool preserveScrollPosition =
      preservePopupScrollOnRefresh_ && open_ && mode_ != Mode::Single;
  preservePopupScrollOnRefresh_ = false;

  int preservedScrollValue = -1;
  QString preservedCurrentValue;
  if (preserveScrollPosition && listView_ && popupScrollArea_) {
    if (QScrollBar* scrollBar = popupScrollArea_->verticalScrollBar()) {
      preservedScrollValue = scrollBar->value();
    }
    const QModelIndex currentIndex = listView_->currentIndex();
    if (currentIndex.isValid()) {
      const int currentRow = currentIndex.row();
      if (currentRow >= 0 && currentRow < rows_.size()) {
        const ModelRow& modelRow = rows_.at(currentRow);
        if (modelRow.optionIndex >= 0 && modelRow.optionIndex < options_.size()) {
          preservedCurrentValue = options_.at(modelRow.optionIndex).value;
        }
      }
    }
  }

  QVector<ModelRow> nextRows;
  const QVector<int> filtered = filteredOptionIndexes();

  bool hasGroup = false;
  for (const Option& option : options_) {
    if (!option.group.trimmed().isEmpty()) {
      hasGroup = true;
      break;
    }
  }

  QString currentGroup;
  for (int index : filtered) {
    if (index < 0 || index >= options_.size()) {
      continue;
    }
    const Option& option = options_.at(index);
    const QString group = option.group.trimmed();
    if (hasGroup && !group.isEmpty() && group != currentGroup) {
      ModelRow groupRow;
      groupRow.header = true;
      groupRow.optionIndex = -1;
      groupRow.headerText = group;
      nextRows.append(groupRow);
      currentGroup = group;
    } else if (group.isEmpty()) {
      currentGroup.clear();
    }

    ModelRow row;
    row.optionIndex = index;
    nextRows.append(row);
  }

  if (nextRows.isEmpty()) {
    ModelRow emptyRow;
    emptyRow.empty = true;
    emptyRow.headerText = QStringLiteral("No data");
    nextRows.append(emptyRow);
  }

  rows_ = nextRows;
  if (listModel_) {
    listModel_->setRows(rows_);
  }
  syncCurrentListRow(preservedCurrentValue, preserveScrollPosition);
  if (listView_ && listView_->viewport()) {
    syncPopupOptionCursor(listView_->viewport()->mapFromGlobal(QCursor::pos()));
  }
  if (popupIsVisible()) {
    syncPopupGeometry();
  }
  if (preserveScrollPosition && preservedScrollValue >= 0 && popupScrollArea_) {
    if (QScrollBar* scrollBar = popupScrollArea_->verticalScrollBar()) {
      scrollBar->setValue(preservedScrollValue);
    }
  }
}

QVector<int> AdSelect::filteredOptionIndexes() const {
  QVector<int> indexes;
  indexes.reserve(options_.size());

  const bool useSearch = isSearchEnabledForCurrentMode() && !searchText_.trimmed().isEmpty();
  const QString term = searchText_.trimmed();
  const QStringList fields = searchFilterFields_.isEmpty()
                                 ? QStringList{QStringLiteral("label"), QStringLiteral("value")}
                                 : searchFilterFields_;

  for (int i = 0; i < options_.size(); ++i) {
    const Option& option = options_.at(i);
    bool match = true;
    if (useSearch) {
      if (filterPredicate_) {
        match = filterPredicate_(term, option);
      } else {
        match = false;
        for (const QString& field : fields) {
          const QString fieldValue = optionSearchFieldValue(option, field);
          if (fieldValue.contains(term, Qt::CaseInsensitive)) {
            match = true;
            break;
          }
        }
      }
    }
    if (match) {
      indexes.append(i);
    }
  }

  if (sortComparator_) {
    std::stable_sort(indexes.begin(), indexes.end(),
                     [this](int lhsIndex, int rhsIndex) {
                       if (lhsIndex < 0 || lhsIndex >= options_.size() || rhsIndex < 0 ||
                           rhsIndex >= options_.size()) {
                         return lhsIndex < rhsIndex;
                       }
                       return sortComparator_(options_.at(lhsIndex), options_.at(rhsIndex));
                     });
  }

  return indexes;
}

void AdSelect::syncCurrentListRow(const QString& preferredValue, bool preserveScrollPosition) {
  if (!listView_ || rows_.isEmpty()) {
    return;
  }

  int targetRow = -1;
  if (!preferredValue.isEmpty()) {
    for (int row = 0; row < rows_.size(); ++row) {
      const ModelRow& modelRow = rows_.at(row);
      if (modelRow.optionIndex < 0 || modelRow.optionIndex >= options_.size()) {
        continue;
      }
      if (options_.at(modelRow.optionIndex).value == preferredValue) {
        targetRow = row;
        break;
      }
    }
  }

  if (mode_ == Mode::Single && !value_.isEmpty()) {
    for (int row = 0; row < rows_.size(); ++row) {
      const ModelRow& modelRow = rows_.at(row);
      if (modelRow.optionIndex < 0 || modelRow.optionIndex >= options_.size()) {
        continue;
      }
      const Option& option = options_.at(modelRow.optionIndex);
      if (option.value == value_) {
        targetRow = row;
        break;
      }
    }
  }

  if (targetRow < 0) {
    for (int row = 0; row < rows_.size(); ++row) {
      const ModelRow& modelRow = rows_.at(row);
      if (modelRow.optionIndex < 0 || modelRow.optionIndex >= options_.size()) {
        continue;
      }
      if (!options_.at(modelRow.optionIndex).disabled) {
        targetRow = row;
        break;
      }
    }
  }

  if (targetRow >= 0) {
    const QModelIndex targetIndex = listModel_->index(targetRow, 0);
    listView_->setCurrentIndex(targetIndex);
    if (mode_ == Mode::Single && listView_->selectionModel()) {
      listView_->selectionModel()->select(targetIndex, QItemSelectionModel::ClearAndSelect);
    }
    if (!preserveScrollPosition && popupScrollArea_) {
      const QRect targetRect = listView_->visualRect(targetIndex);
      if (targetRect.isValid()) {
        const int margin = visualStyle_ ? std::max(2, visualStyle_->metrics.optionHeight / 2) : 8;
        popupScrollArea_->ensureVisible(targetRect.center().x(), targetRect.center().y(), 0, margin);
      }
    }
  }
}

bool AdSelect::addTagValue(const QString& value) {
  const QString normalized = value.trimmed();
  if (normalized.isEmpty()) {
    return false;
  }
  if (mode_ != Mode::Tags) {
    return false;
  }
  if (values_.contains(normalized)) {
    return false;
  }
  if (maxCount_ > 0 && values_.size() >= maxCount_) {
    return false;
  }

  ensureTagOptionExists(normalized);
  values_.append(normalized);
  emit selected(normalized, fallbackSelectedLabel(normalized));
  emit valuesChanged(values_);
  emitSelectionChangedSignals();
  return true;
}

void AdSelect::ensureTagOptionExists(const QString& value) {
  if (indexOfOptionValue(value) >= 0) {
    return;
  }
  Option option;
  option.value = value;
  option.label = value;
  options_.append(option);
  emit optionsChanged();
}

void AdSelect::consumeTokenizedInput(const QString& text) {
  if (tokenSeparators_.isEmpty()) {
    return;
  }

  QString pattern;
  for (const QString& separator : tokenSeparators_) {
    if (separator.isEmpty()) {
      continue;
    }
    if (!pattern.isEmpty()) {
      pattern.append(QStringLiteral("|"));
    }
    pattern.append(escapedForRegex(separator));
  }
  if (pattern.isEmpty()) {
    return;
  }

  QRegularExpression regex(pattern);
  if (!regex.isValid()) {
    return;
  }
  if (!text.contains(regex)) {
    return;
  }

  QStringList parts = text.split(regex, Qt::KeepEmptyParts);
  if (parts.isEmpty()) {
    return;
  }

  const QRegularExpression trailingRegex(QStringLiteral("(%1)$").arg(pattern));
  const bool trailingSeparator = trailingRegex.isValid() && trailingRegex.match(text).hasMatch();
  const int limit = trailingSeparator ? parts.size() : parts.size() - 1;
  bool changed = false;
  for (int i = 0; i < limit; ++i) {
    if (addTagValue(parts.at(i))) {
      changed = true;
    }
  }

  const QString remaining = trailingSeparator ? QString() : parts.constLast();
  if (searchText_ != remaining) {
    searchText_ = remaining;
    emit searchTextChanged(searchText_);
  }
  suppressLineEditChange_ = true;
  lineEdit_->setText(remaining);
  suppressLineEditChange_ = false;

  if (changed) {
    refreshRows();
    updateDisplay();
    updateClearButton();
  }
}

void AdSelect::clearSelectionInternal(bool emitSignals) {
  bool changed = false;
  if (mode_ == Mode::Single) {
    changed = !value_.isEmpty();
    value_.clear();
    values_.clear();
  } else {
    changed = !values_.isEmpty();
    values_.clear();
  }

  if (!changed) {
    return;
  }

  if (emitSignals) {
    emit cleared();
    emit valueChanged(value_);
    emit valuesChanged(values_);
    emitSelectionChangedSignals();
  }
  refreshRows();
  updateDisplay();
  updateClearButton();
}

void AdSelect::emitSelectionChangedSignals() { emit selectionChanged(selectedItems()); }

void AdSelect::toggleSelectionForOption(const Option& option) {
  if (option.disabled) {
    return;
  }

  if (mode_ == Mode::Single) {
    const bool changed = value_ != option.value;
    selectSingleValue(option.value, true);
    if (changed) {
      emit selected(option.value, fallbackSelectedLabel(option.value));
    }
    closePopup();
    return;
  }

  const bool shouldRestoreInputFocus = open_ && lineEdit_ && !lineEdit_->isReadOnly();

  const int index = values_.indexOf(option.value);
  if (index >= 0) {
    values_.removeAt(index);
    emit deselected(option.value, fallbackSelectedLabel(option.value));
  } else {
    if (maxCount_ > 0 && values_.size() >= maxCount_) {
      return;
    }
    values_.append(option.value);
    emit selected(option.value, fallbackSelectedLabel(option.value));
  }

  emit valuesChanged(values_);
  emitSelectionChangedSignals();
  if (autoClearSearchValue_ && isSearchEnabledForCurrentMode()) {
    preservePopupScrollOnRefresh_ = true;
    setSearchText(QString());
    suppressLineEditChange_ = true;
    lineEdit_->clear();
    suppressLineEditChange_ = false;
  }
  preservePopupScrollOnRefresh_ = true;
  refreshRows();
  updateDisplay();
  updateClearButton();
  if (shouldRestoreInputFocus && lineEdit_ && !lineEdit_->hasFocus()) {
    lineEdit_->setFocus(Qt::MouseFocusReason);
  }
}

void AdSelect::selectSingleValue(const QString& value, bool emitSignals) {
  const QString normalized = value.trimmed();
  if (value_ == normalized && values_ == QStringList{normalized}) {
    return;
  }

  value_ = normalized;
  values_.clear();
  if (!value_.isEmpty()) {
    values_.append(value_);
  }

  if (emitSignals) {
    emit valueChanged(value_);
    emit valuesChanged(values_);
    emitSelectionChangedSignals();
  }
  updateDisplay();
  updateClearButton();
}

void AdSelect::ensurePopup() {
  if (popup_) {
    return;
  }

  QWidget* scopeWindow = detail::resolvePopupScopeWindow(this);
  popup_ = new PopupFrame(scopeWindow);
  popup_->setAttribute(Qt::WA_DeleteOnClose, false);
  popup_->setObjectName(QStringLiteral("adselect-popup"));
  popup_->setProperty("adqt.interaction.surface", true);
  popup_->installEventFilter(this);

  popupLayout_ = new QVBoxLayout(popup_);
  popupLayout_->setContentsMargins(4, 4, 4, 4);
  popupLayout_->setSpacing(0);

  popupScrollArea_ = new AdScrollArea(popup_);
  popupScrollArea_->setObjectName(QStringLiteral("adselect-list-scroll"));
  popupScrollArea_->setFitToWidth(true);
  popupScrollArea_->setFocusPolicy(Qt::NoFocus);
  popupScrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  listView_ = new QListView(popupScrollArea_);
  listView_->setObjectName(QStringLiteral("adselect-list"));
  listView_->setModel(listModel_);
  listView_->setItemDelegate(new OptionListDelegate(this));
  listView_->setFrameShape(QFrame::NoFrame);
  listView_->setSelectionMode(QAbstractItemView::SingleSelection);
  listView_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  listView_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  listView_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  listView_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  listView_->setSpacing(0);
  listView_->setUniformItemSizes(true);
  listView_->setMouseTracking(true);
  listView_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  if (listView_->viewport()) {
    listView_->viewport()->setMouseTracking(true);
    listView_->viewport()->installEventFilter(this);
  }
  listView_->installEventFilter(this);
  popupScrollArea_->setContentWidget(listView_);
  popupLayout_->addWidget(popupScrollArea_);

  connect(listView_, &QListView::clicked, this, [this](const QModelIndex& index) {
    if (!index.isValid()) {
      return;
    }
    const int row = index.row();
    if (row < 0 || row >= rows_.size()) {
      return;
    }
    const ModelRow& modelRow = rows_.at(row);
    if (modelRow.optionIndex < 0 || modelRow.optionIndex >= options_.size()) {
      return;
    }
    toggleSelectionForOption(options_.at(modelRow.optionIndex));
  });

  applyVisualStyle();
  updateInputMode();
}

void AdSelect::rebuildPopupExtraContent() {
  if (!popup_ || !popupLayout_) {
    return;
  }
  if (popupExtraContent_) {
    popupExtraContent_->deleteLater();
    popupExtraContent_ = nullptr;
  }
  if (!popupExtraContentFactory_) {
    return;
  }

  popupExtraContent_ = popupExtraContentFactory_(popup_);
  if (!popupExtraContent_) {
    return;
  }
  popupExtraContent_->setParent(popup_);
  popupLayout_->addWidget(popupExtraContent_);
}

int AdSelect::popupContentWidthHint() const {
  if (!visualStyle_) {
    return 0;
  }

  const detail::SelectMetrics& metrics = visualStyle_->metrics;
  const int horizontalPadding = std::max(0, metrics.optionPaddingHorizontal);
  const int selectedIconSize = std::max(10, metrics.iconSize);
  const int selectedStateGap = std::max(2, metrics.optionStateGap);

  auto measureRowTextWidth = [horizontalPadding](const QString& text, const QFont& font) {
    const QFontMetrics fm(font);
    const int textWidth = std::max(fm.horizontalAdvance(text), fm.boundingRect(text).width());
    // Leave a tiny safety room to avoid boundary elide caused by font hinting/rounding.
    return textWidth + horizontalPadding * 2 + 2;
  };

  int maxRowWidth = 0;
  for (const ModelRow& row : rows_) {
    if (row.empty) {
      QFont emptyFont = metrics.optionFont;
      emptyFont.setWeight(QFont::Normal);
      emptyFont.setPixelSize(std::max(12, metrics.emptyDescriptionFontSize));

      const QFontMetrics emptyMetrics(emptyFont);
      const int emptyTextWidth = emptyMetrics.horizontalAdvance(row.headerText);
      const int emptyIconWidth = std::max(30, metrics.emptyStateIconWidth);
      const int emptyInlineMargin = std::max(0, metrics.emptyStateMarginInline);
      const int emptyContentWidth = std::max(emptyTextWidth, emptyIconWidth) + emptyInlineMargin * 2;
      maxRowWidth = std::max(maxRowWidth, emptyContentWidth + horizontalPadding * 2);
      continue;
    }

    if (row.header) {
      QFont headerFont = metrics.optionFont;
      headerFont.setBold(true);
      maxRowWidth = std::max(maxRowWidth, measureRowTextWidth(row.headerText, headerFont));
      continue;
    }

    if (row.optionIndex < 0 || row.optionIndex >= options_.size()) {
      continue;
    }

    const Option& option = options_.at(row.optionIndex);
    QFont optionFont = metrics.optionFont;
    const bool selected = isValueSelected(option.value);
    if (selected) {
      optionFont.setWeight(QFont::DemiBold);
    }

    int optionWidth = measureRowTextWidth(formattedOptionText(option), optionFont);
    if (selected && mode_ != Mode::Single) {
      optionWidth += selectedIconSize + selectedStateGap;
    }
    maxRowWidth = std::max(maxRowWidth, optionWidth);
  }

  if (maxRowWidth <= 0) {
    maxRowWidth = horizontalPadding * 2;
  }

  const QMargins popupMargins =
      popupLayout_ ? popupLayout_->contentsMargins()
                   : QMargins(std::max(0, metrics.popupPadding),
                              std::max(0, metrics.popupPadding),
                              std::max(0, metrics.popupPadding),
                              std::max(0, metrics.popupPadding));
  return maxRowWidth + popupMargins.left() + popupMargins.right();
}

void AdSelect::syncPopupGeometry() {
  if (!popup_ || !visualStyle_) {
    return;
  }

  int contentHeight = 0;
  if (rows_.isEmpty()) {
    contentHeight = visualStyle_->metrics.optionHeight;
  } else {
    for (const ModelRow& row : rows_) {
      contentHeight += row.empty ? visualStyle_->metrics.emptyStateHeight
                                 : visualStyle_->metrics.optionHeight;
    }
  }
  const int listHeight = std::min(visualStyle_->metrics.popupMaxHeight, contentHeight);
  int targetListHeight = std::max(visualStyle_->metrics.optionHeight, listHeight);
  const int targetListContentHeight = std::max(visualStyle_->metrics.optionHeight, contentHeight);
  if (listView_) {
    const int listViewHeight = popupScrollArea_ ? targetListContentHeight : targetListHeight;
    setWidgetFixedHeightIfChanged(listView_, listViewHeight);
    listView_->updateGeometry();
  }
  if (popupScrollArea_) {
    setWidgetFixedHeightIfChanged(popupScrollArea_, targetListHeight);
    popupScrollArea_->updateGeometry();
  } else if (!listView_) {
    targetListHeight = 0;
  }
  if (popupLayout_) {
    popupLayout_->invalidate();
    popupLayout_->activate();
  }

  popup_->adjustSize();
  int popupW = popup_->sizeHint().width();
  if (popupMatchSelectWidth_) {
    popupW = std::max(width(), popupWidth_ > 0 ? popupWidth_ : width());
  } else if (popupWidth_ > 0) {
    popupW = popupWidth_;
  } else {
    popupW = std::max(popupW, popupContentWidthHint());
  }
  const int popupMinWidth = popupWidth_ > 0 ? 1 : 120;
  popupW = std::max(popupMinWidth, popupW);

  int popupH = popup_->sizeHint().height();
  if (popupLayout_ && (popupScrollArea_ || listView_)) {
    const QMargins margins = popupLayout_->contentsMargins();
    popupH = margins.top() + targetListHeight + margins.bottom();
    if (popupExtraContent_ && popupExtraContent_->isVisible()) {
      popupH += std::max(0, popupLayout_->spacing());
      const int availableWidth = std::max(0, popupW - margins.left() - margins.right());
      popupH += boundedWidgetHeightHint(popupExtraContent_, availableWidth);
    }
  }
  popup_->resize(popupW, std::max(1, popupH));

  QWidget* popupParent = popup_->parentWidget();
  if (!popupParent) {
    popupParent = detail::resolvePopupScopeWindow(this);
  }

  detail::PopupPlacementInput placementInput;
  placementInput.anchorTopLeft = mapToGlobal(QPoint(0, 0));
  placementInput.anchorSize = QSize(width(), height());
  placementInput.popupSize = popup_->size();
  QRect popupBounds = detail::popupBoundsInGlobal(popupParent);
  bool hostedInsidePopoverPopup = false;
  for (QWidget* ancestor = this; ancestor != nullptr; ancestor = ancestor->parentWidget()) {
    if (ancestor->objectName() == QStringLiteral("adpopover-popup")) {
      hostedInsidePopoverPopup = true;
      break;
    }
  }
  if (hostedInsidePopoverPopup) {
    // ColorPicker and other popover-hosted selects should align against the
    // trigger without being clamped to the narrow popover surface.
    popupBounds = detail::popupBoundsInGlobal(nullptr);
  }
  placementInput.bounds = popupBounds;
  placementInput.preferredPlacement = toPopupPlacement(placement_);

  const detail::PopupPlacementOutput placementOutput =
      detail::resolvePopupPlacement(placementInput);
  QPoint popupTopLeft = placementOutput.topLeft;
  const int popupOffset = std::max(0, visualStyle_->metrics.popupOffset);
  if (popupOffset > 0) {
    switch (placementOutput.placement) {
      case detail::PopupPlacement::BottomLeft:
      case detail::PopupPlacement::BottomRight:
        popupTopLeft.ry() += popupOffset;
        break;
      case detail::PopupPlacement::TopLeft:
      case detail::PopupPlacement::TopRight:
        popupTopLeft.ry() -= popupOffset;
        break;
      case detail::PopupPlacement::RightTop:
        popupTopLeft.rx() += popupOffset;
        break;
      case detail::PopupPlacement::LeftTop:
        popupTopLeft.rx() -= popupOffset;
        break;
    }
    popupTopLeft = detail::clampPopupTopLeft(popupTopLeft, popup_->size(), placementInput.bounds);
  }

  if (popupParent) {
    // Placement resolves in global coordinates; child popup geometry is parent-local.
    popupTopLeft = popupParent->mapFromGlobal(popupTopLeft);
  }
  popup_->move(popupTopLeft);
}

void AdSelect::closePopup() {
  setOpenInternal(false, true);
}

void AdSelect::openPopup() {
  if (disabled()) {
    return;
  }
  ensurePopup();
  refreshRows();
  rebuildPopupExtraContent();
  syncPopupGeometry();
  setOpenInternal(true, true);
}

void AdSelect::setOpenInternal(bool value, bool emitSignal) {
  if (open_ == value) {
    return;
  }

  open_ = value;
  detail::setInWindowPopupHostOpen(this, open_);

  if (open_) {
    if (popup_) {
      popup_->show();
      popup_->raise();
    }
    hasFocusWithin_ = true;
    bumpJoinedZOrder();
    applyVisualStyle();
    if (lineEdit_) {
      lineEdit_->setFocus();
      if (isSearchEnabledForCurrentMode()) {
        lineEdit_->selectAll();
      }
    }
  } else {
    if (popup_) {
      popup_->hide();
    }
    if (autoClearSearchValue_ && isSearchEnabledForCurrentMode()) {
      setSearchText(QString());
      if (lineEdit_) {
        suppressLineEditChange_ = true;
        lineEdit_->clear();
        suppressLineEditChange_ = false;
      }
    }
    hasFocusWithin_ = false;
    applyVisualStyle();
  }

  // In filled/multiple mode most content surfaces are transparent. Make sure
  // they repaint when open state changes shell background/border colors.
  update();
  if (contentHost_) {
    contentHost_->update();
  }
  if (tagsContainer_ && tagsContainer_->isVisible()) {
    tagsContainer_->update();
  }
  if (lineEdit_) {
    lineEdit_->update();
  }
  if (placeholderLabel_ && placeholderLabel_->isVisible()) {
    placeholderLabel_->update();
  }

  updateSuffixVisual();

  if (emitSignal) {
    emit openChanged(open_);
  }
}

QObject* AdSelect::popupOwnerObject() const { return const_cast<AdSelect*>(this); }

QWidget* AdSelect::popupAnchorWidget() const { return const_cast<AdSelect*>(this); }

QWidget* AdSelect::popupScopeWindow() const { return detail::resolvePopupScopeWindow(this); }

bool AdSelect::popupIsVisible() const { return open_ && popup_ && popup_->isVisible(); }

bool AdSelect::popupContainsGlobalPos(const QPoint& globalPos) const {
  return widgetContainsGlobalPos(this, globalPos) || widgetContainsGlobalPos(popup_, globalPos);
}

void AdSelect::popupCloseFromHost(detail::PopupCloseReason reason) {
  Q_UNUSED(reason)
  closePopup();
}

void AdSelect::popupRelayoutFromHost() {
  if (open_) {
    syncPopupGeometry();
  }
}

void AdSelect::updateInteractionFocusOverlay() {
  if (!visualStyle_ || disabled() || !(hasFocusWithin_ || open_)) {
    stopInteractionFocusForOwner(this);
    return;
  }

  const QColor focusColor = visualStyle_->selectorFocusOutlineColor;
  if (focusColor.alpha() <= 0 || visualStyle_->metrics.focusOutlineWidth <= 0.0) {
    stopInteractionFocusForOwner(this);
    return;
  }

  QRectF focusBaseRectInWindow =
      joinedSelectorRect(rect(), visualStyle_->metrics.borderWidth, joinedLeft_, joinedRight_);

  QWidget* hostWindow = window();
  if (hostWindow) {
    const QPoint origin = mapTo(hostWindow, QPoint(0, 0));
    focusBaseRectInWindow.translate(origin.x(), origin.y());
  }

  const qreal radius = resolveSelectorRadius();
  InteractionFocusRequest request;
  request.owner = this;
  request.baseRectInWindow = focusBaseRectInWindow;
  request.topLeft = joinedLeft_ ? 0.0 : radius;
  request.topRight = joinedRight_ ? 0.0 : radius;
  request.bottomRight = joinedRight_ ? 0.0 : radius;
  request.bottomLeft = joinedLeft_ ? 0.0 : radius;
  request.color = focusColor;
  request.strokeWidth = std::max<qreal>(1.0, visualStyle_->metrics.focusOutlineWidth);
  request.offset = std::max<qreal>(0.0, visualStyle_->metrics.focusOutlineOffset);
  triggerInteractionFocus(request);
}

void AdSelect::bumpJoinedZOrder() {
  if (!(joinedLeft_ || joinedRight_)) {
    return;
  }
  if (!(hovered_ || hasFocusWithin_ || open_)) {
    return;
  }
  raise();
}

void AdSelect::updateFocusState() {
  const bool nextFocus = open_ || (lineEdit_ && lineEdit_->hasFocus());
  if (hasFocusWithin_ == nextFocus) {
    if (nextFocus) {
      bumpJoinedZOrder();
    }
    updateInteractionFocusOverlay();
    return;
  }
  hasFocusWithin_ = nextFocus;
  if (hasFocusWithin_) {
    bumpJoinedZOrder();
  }
  updateInteractionFocusOverlay();
  update();
}

}  // namespace adqt::widgets
