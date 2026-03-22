#include "flow_layout.h"

#include <QLayoutItem>
#include <QWidget>

namespace adqt::widgets::detail {

FlowLayout::FlowLayout(QWidget* parent, int margin, int hSpacing, int vSpacing)
    : QLayout(parent), hSpacing_(hSpacing), vSpacing_(vSpacing) {
  setContentsMargins(margin, margin, margin, margin);
}

FlowLayout::~FlowLayout() {
  while (QLayoutItem* item = takeAt(0)) {
    delete item;
  }
}

void FlowLayout::addItem(QLayoutItem* item) {
  if (!item) {
    return;
  }
  itemList_.append(item);
}

int FlowLayout::horizontalSpacing() const {
  if (hSpacing_ >= 0) {
    return hSpacing_;
  }
  return smartSpacing(QStyle::PM_LayoutHorizontalSpacing);
}

int FlowLayout::verticalSpacing() const {
  if (vSpacing_ >= 0) {
    return vSpacing_;
  }
  return smartSpacing(QStyle::PM_LayoutVerticalSpacing);
}

void FlowLayout::setHorizontalSpacing(int spacing) {
  if (hSpacing_ == spacing) {
    return;
  }
  hSpacing_ = spacing;
  invalidate();
}

void FlowLayout::setVerticalSpacing(int spacing) {
  if (vSpacing_ == spacing) {
    return;
  }
  vSpacing_ = spacing;
  invalidate();
}

Qt::Orientations FlowLayout::expandingDirections() const { return {}; }

bool FlowLayout::hasHeightForWidth() const { return true; }

int FlowLayout::heightForWidth(int width) const {
  return doLayout(QRect(0, 0, width, 0), true);
}

int FlowLayout::count() const { return itemList_.size(); }

QLayoutItem* FlowLayout::itemAt(int index) const {
  if (index < 0 || index >= itemList_.size()) {
    return nullptr;
  }
  return itemList_.at(index);
}

QLayoutItem* FlowLayout::takeAt(int index) {
  if (index < 0 || index >= itemList_.size()) {
    return nullptr;
  }
  return itemList_.takeAt(index);
}

QSize FlowLayout::minimumSize() const {
  QSize size;
  for (QLayoutItem* item : itemList_) {
    if (!item) {
      continue;
    }
    size = size.expandedTo(item->minimumSize());
  }

  int left = 0;
  int top = 0;
  int right = 0;
  int bottom = 0;
  getContentsMargins(&left, &top, &right, &bottom);
  size += QSize(left + right, top + bottom);
  return size;
}

QSize FlowLayout::sizeHint() const { return minimumSize(); }

void FlowLayout::setGeometry(const QRect& rect) {
  QLayout::setGeometry(rect);
  doLayout(rect, false);
}

int FlowLayout::doLayout(const QRect& rect, bool testOnly) const {
  int left = 0;
  int top = 0;
  int right = 0;
  int bottom = 0;
  getContentsMargins(&left, &top, &right, &bottom);

  QRect effectiveRect = rect.adjusted(+left, +top, -right, -bottom);
  int x = effectiveRect.x();
  int y = effectiveRect.y();
  int lineHeight = 0;
  const int hSpace = horizontalSpacing();
  const int vSpace = verticalSpacing();

  for (QLayoutItem* item : itemList_) {
    if (!item) {
      continue;
    }
    QWidget* widget = item->widget();
    if (widget && widget->isHidden()) {
      continue;
    }

    const QSize itemSize = item->sizeHint();
    const int nextX = x + itemSize.width() + hSpace;
    if (lineHeight > 0 && nextX - hSpace > effectiveRect.right() + 1) {
      x = effectiveRect.x();
      y += lineHeight + vSpace;
      lineHeight = 0;
    }

    if (!testOnly) {
      item->setGeometry(QRect(QPoint(x, y), itemSize));
    }

    x += itemSize.width() + hSpace;
    lineHeight = std::max(lineHeight, itemSize.height());
  }

  return y + lineHeight - rect.y() + bottom;
}

int FlowLayout::smartSpacing(QStyle::PixelMetric pm) const {
  const QObject* parentObject = parent();
  if (!parentObject) {
    return -1;
  }

  if (const QWidget* parentWidget = qobject_cast<const QWidget*>(parentObject)) {
    return parentWidget->style()->pixelMetric(pm, nullptr, parentWidget);
  }

  if (const QLayout* parentLayout = qobject_cast<const QLayout*>(parentObject)) {
    return parentLayout->spacing();
  }

  return -1;
}

}  // namespace adqt::widgets::detail
