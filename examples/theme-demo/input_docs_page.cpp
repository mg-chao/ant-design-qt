#include "input_docs_page.h"

#include <QCheckBox>
#include <QFrame>
#include <QFontMetrics>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "icons.h"
#include "theme/theme_manager.h"

#include <algorithm>

using adqt::widgets::AdButton;
using adqt::widgets::AdInput;
using adqt::widgets::AdInputOtp;
using adqt::widgets::AdInputPassword;
using adqt::widgets::AdInputSearch;
using adqt::widgets::AdInputTextArea;
using adqt::widgets::AdSelect;
using adqt::widgets::AdTooltip;
namespace outlined_icons = adqt::icons::outlined;

namespace {

QColor parseThemeColor(const QString& value, const QColor& fallback) {
  const QColor parsed(value);
  return parsed.isValid() ? parsed : fallback;
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

QRectF joinedBorderRect(const QRect& bounds, qreal borderWidth, bool joinedLeft, bool joinedRight) {
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

qreal snapToDevicePixelCoord(qreal value, qreal dpr) {
  if (dpr <= 0.0) {
    return value;
  }
  return qRound(value * dpr) / dpr;
}

QRectF snapRectToDevicePixels(const QRectF& rect, qreal dpr) {
  if (dpr <= 0.0) {
    return rect;
  }
  const qreal left = snapToDevicePixelCoord(rect.left(), dpr);
  const qreal top = snapToDevicePixelCoord(rect.top(), dpr);
  const qreal right = snapToDevicePixelCoord(rect.left() + rect.width(), dpr);
  const qreal bottom = snapToDevicePixelCoord(rect.top() + rect.height(), dpr);
  const qreal minSize = 1.0 / dpr;
  return QRectF(left, top, std::max(minSize, right - left), std::max(minSize, bottom - top));
}

class CompactAddon final : public QWidget {
 public:
  explicit CompactAddon(const QString& text, QWidget* parent = nullptr) : QWidget(parent), text_(text) {
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(&adqt::theme::ThemeManager::instance(), &adqt::theme::ThemeManager::themeChanged, this,
            [this]() {
              updateGeometry();
              update();
            });
  }

  void setIconToken(const adqt::icons::IconToken& token) {
    if (iconToken_.index == token.index && iconToken_.style.hasPrimary == token.style.hasPrimary &&
        iconToken_.style.hasSecondary == token.style.hasSecondary &&
        iconToken_.style.hasTertiary == token.style.hasTertiary &&
        iconToken_.style.primary == token.style.primary &&
        iconToken_.style.secondary == token.style.secondary &&
        iconToken_.style.tertiary == token.style.tertiary) {
      return;
    }
    iconToken_ = token;
    updateGeometry();
    update();
  }

  void setSize(AdInput::Size value) {
    if (size_ == value) {
      return;
    }
    size_ = value;
    updateGeometry();
    update();
  }

  void setJoinedLeft(bool value) {
    if (joinedLeft_ == value) {
      return;
    }
    joinedLeft_ = value;
    update();
  }

  void setJoinedRight(bool value) {
    if (joinedRight_ == value) {
      return;
    }
    joinedRight_ = value;
    update();
  }

  QSize sizeHint() const override { return minimumSizeHint(); }

  QSize minimumSizeHint() const override {
    const adqt::theme::ThemeMapToken& map = adqt::theme::ThemeManager::instance().currentMapToken();
    const int height = controlHeight(map);
    const int borderWidth = std::max(1, qRound(map.lineWidth));
    const int iconSide = iconSize(map);
    const int padding = horizontalPadding();

    QFont textFont = addonFont(map);
    QFontMetrics fm(textFont);
    int width = borderWidth * 2 + padding * 2;

    const bool hasIcon = adqt::icons::isValid(iconToken_);
    const bool hasText = !text_.trimmed().isEmpty();
    if (hasIcon) {
      width += iconSide;
    }
    if (hasText) {
      if (hasIcon) {
        width += std::max(4, qRound(map.sizeXS));
      }
      width += fm.horizontalAdvance(text_);
    }
    if (!hasIcon && !hasText) {
      width += std::max(16, iconSide);
    }
    return QSize(std::max(width, height / 2), height);
  }

 protected:
  void paintEvent(QPaintEvent* event) override {
    Q_UNUSED(event)

    const adqt::theme::ThemeMapToken& map = adqt::theme::ThemeManager::instance().currentMapToken();
    const QColor background = parseThemeColor(adqt::theme::ThemeManager::instance().currentToken().colorBgContainerDisabled,
                                             QColor("#f5f5f5"));
    const QColor borderColor = parseThemeColor(map.colorBorder, QColor("#d9d9d9"));
    const QColor textColor = parseThemeColor(map.colorText, QColor(0, 0, 0, 223));
    const int borderWidth = std::max(1, qRound(map.lineWidth));
    const qreal radius = borderRadius(map);

    const QRectF rawBorderRect =
        joinedBorderRect(rect(), static_cast<qreal>(borderWidth), joinedLeft_, joinedRight_);
    const qreal dpr = devicePixelRatioF();
    const QRectF borderRect = snapRectToDevicePixels(rawBorderRect, dpr);
    if (!borderRect.isValid() || borderRect.width() <= 0.0 || borderRect.height() <= 0.0) {
      return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const qreal topLeft = joinedLeft_ ? 0.0 : radius;
    const qreal topRight = joinedRight_ ? 0.0 : radius;
    const qreal bottomRight = joinedRight_ ? 0.0 : radius;
    const qreal bottomLeft = joinedLeft_ ? 0.0 : radius;
    const QPainterPath shellPath =
        roundedRectPath(borderRect, topLeft, topRight, bottomRight, bottomLeft);

    painter.fillPath(shellPath, background);
    QPen borderPen(borderColor, borderWidth, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
    painter.setPen(borderPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(shellPath);

    const bool hasIcon = adqt::icons::isValid(iconToken_);
    const bool hasText = !text_.trimmed().isEmpty();
    if (!hasIcon && !hasText) {
      return;
    }

    const int iconSide = iconSize(map);
    QFont textFont = addonFont(map);
    painter.setFont(textFont);
    painter.setPen(textColor);
    QFontMetrics fm(textFont);
    const int gap = hasIcon && hasText ? std::max(4, qRound(map.sizeXS)) : 0;
    const int textWidth = hasText ? fm.horizontalAdvance(text_) : 0;
    const int contentWidth = (hasIcon ? iconSide : 0) + gap + textWidth;
    const int startX = qRound(borderRect.left() + (borderRect.width() - contentWidth) / 2.0);
    const int centerY = qRound(borderRect.center().y());

    int cursorX = startX;
    if (hasIcon) {
      adqt::icons::IconToken icon = iconToken_;
      icon.style.primary = textColor;
      icon.style.hasPrimary = true;
      const QPixmap iconPixmap =
          adqt::icons::renderIconPixmap(icon, QSize(iconSide, iconSide), devicePixelRatioF());
      if (!iconPixmap.isNull()) {
        const int iconY = centerY - iconSide / 2;
        painter.drawPixmap(cursorX, iconY, iconPixmap);
      }
      cursorX += iconSide + gap;
    }

    if (hasText) {
      const QRect textRect(cursorX, qRound(borderRect.top()), textWidth, qRound(borderRect.height()));
      painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, text_);
    }
  }

 private:
  int controlHeight(const adqt::theme::ThemeMapToken& map) const {
    switch (size_) {
      case AdInput::Size::Large:
        return std::max(24, qRound(map.controlHeightLG));
      case AdInput::Size::Small:
        return std::max(18, qRound(map.controlHeightSM));
      case AdInput::Size::Middle:
      default:
        return std::max(20, qRound(map.controlHeight));
    }
  }

  qreal borderRadius(const adqt::theme::ThemeMapToken& map) const {
    switch (size_) {
      case AdInput::Size::Large:
        return std::max<qreal>(0.0, map.borderRadiusLG);
      case AdInput::Size::Small:
        return std::max<qreal>(0.0, map.borderRadiusSM);
      case AdInput::Size::Middle:
      default:
        return std::max<qreal>(0.0, map.borderRadius);
    }
  }

  int iconSize(const adqt::theme::ThemeMapToken& map) const {
    switch (size_) {
      case AdInput::Size::Large:
        return std::max(14, qRound(map.fontSizeLG));
      case AdInput::Size::Small:
        return std::max(12, qRound(map.fontSizeSM));
      case AdInput::Size::Middle:
      default:
        return std::max(12, qRound(map.fontSize));
    }
  }

  int horizontalPadding() const {
    switch (size_) {
      case AdInput::Size::Small:
        return 8;
      case AdInput::Size::Large:
      case AdInput::Size::Middle:
      default:
        return 12;
    }
  }

  QFont addonFont(const adqt::theme::ThemeMapToken& map) const {
    QFont result = font();
    int pixelSize = qRound(map.fontSize);
    switch (size_) {
      case AdInput::Size::Large:
        pixelSize = qRound(map.fontSizeLG);
        break;
      case AdInput::Size::Small:
        pixelSize = qRound(map.fontSizeSM);
        break;
      case AdInput::Size::Middle:
      default:
        break;
    }
    result.setPixelSize(std::max(10, pixelSize));
    return result;
  }

  QString text_;
  adqt::icons::IconToken iconToken_;
  AdInput::Size size_ = AdInput::Size::Middle;
  bool joinedLeft_ = false;
  bool joinedRight_ = false;
};

QLabel* makeHintLabel(const QString& text, QWidget* parent = nullptr) {
  auto* label = new QLabel(text, parent);
  label->setWordWrap(true);
  QPalette palette = label->palette();
  palette.setColor(QPalette::WindowText, QColor("#8c8c8c"));
  label->setPalette(palette);
  return label;
}

CompactAddon* makeAddonLabel(const QString& text,
                             AdInput::Size size = AdInput::Size::Middle,
                             bool joinedLeft = false,
                             bool joinedRight = false,
                             QWidget* parent = nullptr) {
  auto* addon = new CompactAddon(text, parent);
  addon->setSize(size);
  addon->setJoinedLeft(joinedLeft);
  addon->setJoinedRight(joinedRight);
  return addon;
}

CompactAddon* makeAddonIcon(const adqt::icons::IconToken& token,
                            AdInput::Size size = AdInput::Size::Middle,
                            bool joinedLeft = false,
                            bool joinedRight = false,
                            QWidget* parent = nullptr) {
  auto* addon = new CompactAddon(QString(), parent);
  addon->setSize(size);
  addon->setIconToken(token);
  addon->setJoinedLeft(joinedLeft);
  addon->setJoinedRight(joinedRight);
  return addon;
}

}  // namespace

InputDocsPage::InputDocsPage(QWidget* parent) : QWidget(parent) {
  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(16, 16, 16, 24);
  root->setSpacing(16);

  auto* title = new QLabel("Input");
  QFont titleFont = title->font();
  titleFont.setPointSize(titleFont.pointSize() + 8);
  titleFont.setBold(true);
  title->setFont(titleFont);
  root->addWidget(title);

  auto* subtitle = new QLabel(
      "Through mouse or keyboard input content, it is the most basic form field wrapper. "
      "This page mirrors Ant Design Input public demos.");
  subtitle->setWordWrap(true);
  root->addWidget(subtitle);

  addSection(root, "Basic usage", "Demo: basic.tsx", buildBasicDemo());
  addSection(root, "Three sizes of Input", "Demo: size.tsx", buildSizeDemo());
  addSection(root, "Variants", "Demo: variant.tsx", buildVariantDemo());
  addSection(root, "Compact Style", "Demo: compact-style.tsx", buildCompactStyleDemo());
  addSection(root, "Search box", "Demo: search-input.tsx", buildSearchInputDemo());
  addSection(root, "Search box with loading", "Demo: search-input-loading.tsx", buildSearchLoadingDemo());
  addSection(root, "TextArea", "Demo: textarea.tsx", buildTextAreaDemo());
  addSection(root, "Autosizing the height to fit the content", "Demo: autosize-textarea.tsx",
             buildAutoSizeTextAreaDemo());
  addSection(root, "OTP", "Demo: otp.tsx", buildOtpDemo());
  addSection(root, "Format Tooltip Input", "Demo: tooltip.tsx", buildTooltipDemo());
  addSection(root, "prefix and suffix", "Demo: presuffix.tsx", buildPreSuffixDemo());
  addSection(root, "Password box", "Demo: password-input.tsx", buildPasswordDemo());
  addSection(root, "With clear icon", "Demo: allowClear.tsx", buildAllowClearDemo());
  addSection(root, "With character counting", "Demo: show-count.tsx", buildShowCountDemo());
  addSection(root, "Custom count logic", "Demo: advance-count.tsx", buildAdvanceCountDemo());
  addSection(root, "Status", "Demo: status.tsx", buildStatusDemo());
  addSection(root, "Focus", "Demo: focus.tsx", buildFocusDemo());
  addSection(root, "Custom semantic dom styling", "Demo: style-class.tsx", buildStyleClassDemo());

  root->addStretch();
}

const QVector<QWidget*>& InputDocsPage::sectionAnchors() const { return anchors_; }

const QStringList& InputDocsPage::sectionTitles() const { return titles_; }

void InputDocsPage::addSection(QVBoxLayout* root,
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

QWidget* InputDocsPage::buildBasicDemo() {
  auto* box = new QWidget();
  auto* row = new QHBoxLayout(box);
  row->setContentsMargins(0, 0, 0, 0);

  auto* input = new AdInput();
  input->setPlaceholder("Basic usage");
  input->setFixedWidth(280);

  row->addWidget(input);
  row->addStretch();
  return box;
}

QWidget* InputDocsPage::buildSizeDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(10);

  auto* large = new AdInput();
  large->setSize(AdInput::Size::Large);
  large->setPlaceholder("large size");
  large->setPrefixIconToken(outlined_icons::User());
  large->setFixedWidth(320);

  auto* middle = new AdInput();
  middle->setPlaceholder("default size");
  middle->setPrefixIconToken(outlined_icons::User());
  middle->setFixedWidth(320);

  auto* small = new AdInput();
  small->setSize(AdInput::Size::Small);
  small->setPlaceholder("small size");
  small->setPrefixIconToken(outlined_icons::User());
  small->setFixedWidth(320);

  layout->addWidget(large, 0, Qt::AlignLeft);
  layout->addWidget(middle, 0, Qt::AlignLeft);
  layout->addWidget(small, 0, Qt::AlignLeft);
  return box;
}

QWidget* InputDocsPage::buildVariantDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(10);

  auto* outlined = new AdInput();
  outlined->setPlaceholder("Outlined");

  auto* filled = new AdInput();
  filled->setPlaceholder("Filled");
  filled->setVariant(AdInput::Variant::Filled);

  auto* borderless = new AdInput();
  borderless->setPlaceholder("Borderless");
  borderless->setVariant(AdInput::Variant::Borderless);

  auto* underlined = new AdInput();
  underlined->setPlaceholder("Underlined");
  underlined->setVariant(AdInput::Variant::Underlined);

  auto* searchFilled = new AdInputSearch();
  searchFilled->setPlaceholder("Filled");
  searchFilled->setVariant(AdInput::Variant::Filled);
  searchFilled->setFixedWidth(320);

  layout->addWidget(outlined, 0, Qt::AlignLeft);
  layout->addWidget(filled, 0, Qt::AlignLeft);
  layout->addWidget(borderless, 0, Qt::AlignLeft);
  layout->addWidget(underlined, 0, Qt::AlignLeft);
  layout->addWidget(searchFilled, 0, Qt::AlignLeft);
  return box;
}

QWidget* InputDocsPage::buildCompactStyleDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(16);
  const int compactJoinOverlap =
      std::max(1, qRound(adqt::theme::ThemeManager::instance().currentMapToken().lineWidth));
  // Input joined edges already shift side insets by `lineWidth / 2`, so compact rows should
  // overlap by exactly `lineWidth` to keep the shared seam on one stable stroke.

  {
    auto* row = new QHBoxLayout();
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(0);
    auto* input = new AdInput();
    input->setValue("26888888");
    input->setFixedWidth(300);
    row->addWidget(input);
    row->addStretch();
    layout->addLayout(row);
  }

  {
    auto* row = new QHBoxLayout();
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(0);
    auto* area = new AdInput();
    area->setValue("0571");
    area->setJoinedRight(true);
    area->setFixedWidth(115);

    auto* phone = new AdInput();
    phone->setValue("26888888");
    phone->setJoinedLeft(true);
    phone->setFixedWidth(460);

    row->addWidget(area);
    row->addSpacing(-compactJoinOverlap);
    row->addWidget(phone);
    row->addStretch();
    layout->addLayout(row);
  }

  {
    auto* row = new QHBoxLayout();
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(0);

    auto* addon = makeAddonLabel("https://", AdInput::Size::Middle, false, true);
    auto* search = new AdInputSearch();
    search->setPlaceholder("input search text");
    search->setAllowClear(true);
    search->input()->setJoinedLeft(true);
    search->setFixedWidth(400);

    row->addWidget(addon);
    row->addSpacing(-compactJoinOverlap);
    row->addWidget(search);
    row->addStretch();
    layout->addLayout(row);
  }

  {
    auto* row = new QHBoxLayout();
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(0);

    auto* input = new AdInput();
    input->setValue("Combine input and button");
    input->setJoinedRight(true);
    input->setFixedWidth(470);

    auto* submit = new AdButton("Submit");
    submit->setType(AdButton::Type::Primary);
    submit->setJoinedLeft(true);

    row->addWidget(input);
    row->addSpacing(-compactJoinOverlap);
    row->addWidget(submit);
    row->addStretch();
    layout->addLayout(row);
  }

  {
    auto* row = new QHBoxLayout();
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(0);

    auto* select = new AdSelect();
    select->setFixedWidth(160);
    select->setOptions({
        {"zhejiang", "Zhejiang", false, QString(), {}},
        {"jiangsu", "Jiangsu", false, QString(), {}},
    });
    select->setValue("zhejiang");
    select->setJoinedRight(true);

    auto* input = new AdInput();
    input->setValue("Xihu District, Hangzhou");
    input->setJoinedLeft(true);
    input->setFixedWidth(290);

    row->addWidget(select);
    row->addSpacing(-compactJoinOverlap);
    row->addWidget(input);
    row->addStretch();
    layout->addLayout(row);
  }

  {
    auto* row = new QHBoxLayout();
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(0);

    auto* addon = makeAddonIcon(outlined_icons::Search(), AdInput::Size::Large, false, true);

    auto* left = new AdInput();
    left->setSize(AdInput::Size::Large);
    left->setPlaceholder("large size");
    left->setJoinedLeft(true);
    left->setJoinedRight(true);
    left->setFixedWidth(255);

    auto* right = new AdInput();
    right->setSize(AdInput::Size::Large);
    right->setPlaceholder("another input");
    right->setJoinedLeft(true);
    right->setFixedWidth(255);

    row->addWidget(addon);
    row->addSpacing(-compactJoinOverlap);
    row->addWidget(left);
    row->addSpacing(-compactJoinOverlap);
    row->addWidget(right);
    row->addStretch();
    layout->addLayout(row);
  }

  return box;
}

QWidget* InputDocsPage::buildSearchInputDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(10);
  const int compactJoinOverlap =
      std::max(1, qRound(adqt::theme::ThemeManager::instance().currentMapToken().lineWidth));

  auto* info = makeHintLabel("onSearch source/value will appear here.");

  auto makeSearch = [info](bool allowClear, bool enterButton, const QString& buttonText,
                           AdInput::Size size = AdInput::Size::Middle) {
    auto* search = new AdInputSearch();
    search->setPlaceholder("input search text");
    search->setAllowClear(allowClear);
    search->setEnterButton(enterButton);
    search->setEnterButtonText(buttonText);
    search->setSize(size);
    search->setFixedWidth(320);
    QObject::connect(search, &AdInputSearch::searchTriggered, info,
                     [info](const QString& value, AdInputSearch::SearchSource source) {
                       const QString src =
                           source == AdInputSearch::SearchSource::Input ? QStringLiteral("input") : QStringLiteral("clear");
                       info->setText(QStringLiteral("onSearch source=%1 value=%2").arg(src, value));
                     });
    return search;
  };

  layout->addWidget(makeSearch(false, false, QString()), 0, Qt::AlignLeft);
  layout->addWidget(makeSearch(true, false, QString()), 0, Qt::AlignLeft);

  {
    auto* row = new QHBoxLayout();
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(0);
    auto* addon = makeAddonLabel("https://", AdInput::Size::Middle, false, true);
    auto* search = makeSearch(true, false, QString());
    search->input()->setJoinedLeft(true);
    row->addWidget(addon);
    row->addSpacing(-compactJoinOverlap);
    row->addWidget(search);
    row->addStretch();
    layout->addLayout(row);
  }

  layout->addWidget(makeSearch(false, true, QString()), 0, Qt::AlignLeft);
  layout->addWidget(makeSearch(true, true, QStringLiteral("Search"), AdInput::Size::Large), 0, Qt::AlignLeft);

  auto* suffixSearch = makeSearch(false, true, QStringLiteral("Search"), AdInput::Size::Large);
  suffixSearch->input()->setSuffixIconToken(outlined_icons::Audio());
  layout->addWidget(suffixSearch, 0, Qt::AlignLeft);

  layout->addWidget(info);
  return box;
}

QWidget* InputDocsPage::buildSearchLoadingDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(12);

  auto* s1 = new AdInputSearch();
  s1->setPlaceholder("input search loading default");
  s1->setLoading(true);
  s1->setFixedWidth(320);

  auto* s2 = new AdInputSearch();
  s2->setPlaceholder("input search loading with enterButton");
  s2->setEnterButton(true);
  s2->setLoading(true);
  s2->setFixedWidth(320);

  auto* s3 = new AdInputSearch();
  s3->setPlaceholder("input search text");
  s3->setEnterButton(true);
  s3->setEnterButtonText("Search");
  s3->setSize(AdInput::Size::Large);
  s3->setLoading(true);
  s3->setFixedWidth(320);

  layout->addWidget(s1, 0, Qt::AlignLeft);
  layout->addWidget(s2, 0, Qt::AlignLeft);
  layout->addWidget(s3, 0, Qt::AlignLeft);
  return box;
}

QWidget* InputDocsPage::buildTextAreaDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(12);

  auto* first = new AdInputTextArea();
  first->setAutoSizeEnabled(true);
  first->setAutoSizeMinRows(4);
  first->setAutoSizeMaxRows(4);
  first->setFixedWidth(420);

  auto* second = new AdInputTextArea();
  second->setPlaceholder("maxLength is 6");
  second->setMaxLength(6);
  second->setAutoSizeEnabled(true);
  second->setAutoSizeMinRows(4);
  second->setAutoSizeMaxRows(4);
  second->setFixedWidth(420);

  layout->addWidget(first, 0, Qt::AlignLeft);
  layout->addWidget(second, 0, Qt::AlignLeft);
  return box;
}

QWidget* InputDocsPage::buildAutoSizeTextAreaDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(12);

  auto* first = new AdInputTextArea();
  first->setPlaceholder("Autosize height based on content lines");
  first->setAutoSizeEnabled(true);
  first->setAutoSizeMinRows(2);
  first->setAutoSizeMaxRows(6);
  first->setFixedWidth(460);

  auto* second = new AdInputTextArea();
  second->setPlaceholder("Autosize height with minimum and maximum number of lines");
  second->setAutoSizeEnabled(true);
  second->setAutoSizeMinRows(2);
  second->setAutoSizeMaxRows(6);
  second->setFixedWidth(460);

  auto* third = new AdInputTextArea();
  third->setPlaceholder("Controlled autosize");
  third->setValue("Type to grow");
  third->setAutoSizeEnabled(true);
  third->setAutoSizeMinRows(3);
  third->setAutoSizeMaxRows(5);
  third->setFixedWidth(460);

  layout->addWidget(first, 0, Qt::AlignLeft);
  layout->addWidget(second, 0, Qt::AlignLeft);
  layout->addWidget(third, 0, Qt::AlignLeft);
  return box;
}

QWidget* InputDocsPage::buildOtpDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(10);

  auto* title1 = new QLabel("With formatter (Upcase)");
  QFont titleFont = title1->font();
  titleFont.setBold(true);
  title1->setFont(titleFont);
  layout->addWidget(title1, 0, Qt::AlignLeft);

  auto* upcase = new AdInputOtp();
  upcase->setFormatter([](const QString& text) { return text.toUpper(); });
  layout->addWidget(upcase, 0, Qt::AlignLeft);

  layout->addWidget(new QLabel("With Disabled"), 0, Qt::AlignLeft);
  auto* disabled = new AdInputOtp();
  disabled->setDisabled(true);
  layout->addWidget(disabled, 0, Qt::AlignLeft);

  layout->addWidget(new QLabel("With Length (8)"), 0, Qt::AlignLeft);
  auto* length8 = new AdInputOtp();
  length8->setLength(8);
  layout->addWidget(length8, 0, Qt::AlignLeft);

  layout->addWidget(new QLabel("With variant"), 0, Qt::AlignLeft);
  auto* filled = new AdInputOtp();
  filled->setVariant(AdInput::Variant::Filled);
  layout->addWidget(filled, 0, Qt::AlignLeft);

  layout->addWidget(new QLabel("With custom display character"), 0, Qt::AlignLeft);
  auto* masked = new AdInputOtp();
  masked->setMaskEnabled(true);
  masked->setMaskCharacter("*");
  layout->addWidget(masked, 0, Qt::AlignLeft);

  layout->addWidget(new QLabel("With custom separator"), 0, Qt::AlignLeft);
  auto* separatorText = new AdInputOtp();
  separatorText->setSeparatorText("/");
  layout->addWidget(separatorText, 0, Qt::AlignLeft);

  layout->addWidget(new QLabel("With custom function separator"), 0, Qt::AlignLeft);
  auto* separatorFn = new AdInputOtp();
  separatorFn->setSeparatorFactory([](int index, QWidget* parent) {
    auto* label = new QLabel(index % 2 == 0 ? QStringLiteral("-") : QStringLiteral("?"), parent);
    QPalette palette = label->palette();
    palette.setColor(QPalette::WindowText, index % 2 == 0 ? QColor("#1677ff") : QColor("#ff4d4f"));
    label->setPalette(palette);
    return label;
  });
  layout->addWidget(separatorFn, 0, Qt::AlignLeft);

  auto* output = makeHintLabel("onInput / onChange output will appear here.");
  const auto bindOutput = [output](AdInputOtp* otp) {
    QObject::connect(otp, &AdInputOtp::inputChanged, output, [output](const QStringList& parts) {
      output->setText(QStringLiteral("onInput: [%1]").arg(parts.join(",")));
    });
    QObject::connect(otp, &AdInputOtp::completed, output, [output](const QString& text) {
      output->setText(QStringLiteral("onChange: %1").arg(text));
    });
  };

  bindOutput(upcase);
  bindOutput(disabled);
  bindOutput(length8);
  bindOutput(filled);
  bindOutput(masked);
  bindOutput(separatorText);
  bindOutput(separatorFn);

  layout->addWidget(output);
  return box;
}

QWidget* InputDocsPage::buildTooltipDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto* tooltip = new AdTooltip();
  tooltip->setPlacement(AdTooltip::Placement::TopLeft);
  tooltip->setTriggerModes(AdTooltip::Trigger::Focus);
  tooltip->setTitleText("Input a number");

  auto* input = new AdInput(tooltip);
  input->setPlaceholder("Input a number");
  input->setFixedWidth(180);
  tooltip->setTriggerWidget(input);

  auto* hint = makeHintLabel("Focus the input to see formatted value tooltip.");
  connect(input, &AdInput::valueChanged, tooltip, [tooltip](const QString& text) {
    if (text.trimmed().isEmpty()) {
      tooltip->setTitleText("Input a number");
      return;
    }
    bool ok = false;
    const double value = text.toDouble(&ok);
    tooltip->setTitleText(ok ? QString::number(value, 'f', 2) : text);
  });

  layout->addWidget(tooltip, 0, Qt::AlignLeft);
  layout->addWidget(hint);
  return box;
}

QWidget* InputDocsPage::buildPreSuffixDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(12);

  auto* first = new AdInput();
  first->setPlaceholder("Enter your username");
  first->setPrefixIconToken(outlined_icons::User());
  first->setSuffixIconToken(outlined_icons::InfoCircle());
  AdInput::SemanticStyles firstStyles;
  firstStyles.prefix.textColor = QColor(0, 0, 0, 64);   // Match Ant Design demo: rgba(0,0,0,.25)
  firstStyles.suffix.textColor = QColor(0, 0, 0, 115);  // Match Ant Design demo: rgba(0,0,0,.45)
  first->setSemanticStyles(firstStyles);
  first->setFixedWidth(360);

  auto* second = new AdInput();
  second->setPrefixText("￥");
  second->setSuffixText("RMB");
  second->setFixedWidth(240);

  auto* third = new AdInput();
  third->setPrefixText("￥");
  third->setSuffixText("RMB");
  third->setDisabled(true);
  third->setFixedWidth(240);

  auto* pwd = new AdInputPassword();
  pwd->setPlaceholder("input password support suffix");
  pwd->input()->setSuffixIconToken(outlined_icons::Lock());
  pwd->setFixedWidth(360);

  layout->addWidget(first, 0, Qt::AlignLeft);
  layout->addWidget(second, 0, Qt::AlignLeft);
  layout->addWidget(third, 0, Qt::AlignLeft);
  layout->addWidget(pwd, 0, Qt::AlignLeft);
  return box;
}

QWidget* InputDocsPage::buildPasswordDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(10);

  auto* p1 = new AdInputPassword();
  p1->setPlaceholder("input password");
  p1->setFixedWidth(300);

  auto* p2 = new AdInputPassword();
  p2->setPlaceholder("input password");
  p2->setVisibleIconToken(outlined_icons::Eye());
  p2->setHiddenIconToken(outlined_icons::EyeInvisible());
  p2->setFixedWidth(300);

  auto* row = new QHBoxLayout();
  auto* p3 = new AdInputPassword();
  p3->setPlaceholder("input password");
  p3->setFixedWidth(300);

  auto* toggle = new AdButton("Show");
  toggle->setFixedWidth(90);
  QObject::connect(toggle, &QPushButton::clicked, p3, [p3, toggle]() {
    p3->setPasswordVisible(!p3->passwordVisible());
    toggle->setText(p3->passwordVisible() ? QStringLiteral("Hide") : QStringLiteral("Show"));
  });

  row->addWidget(p3);
  row->addWidget(toggle);
  row->addStretch();

  auto* disabled = new AdInputPassword();
  disabled->setPlaceholder("disabled input password");
  disabled->setDisabled(true);
  disabled->setFixedWidth(300);

  layout->addWidget(p1, 0, Qt::AlignLeft);
  layout->addWidget(p2, 0, Qt::AlignLeft);
  layout->addLayout(row);
  layout->addWidget(disabled, 0, Qt::AlignLeft);
  return box;
}

QWidget* InputDocsPage::buildAllowClearDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(12);

  auto* input = new AdInput();
  input->setPlaceholder("input with clear icon");
  input->setAllowClear(true);
  input->setFixedWidth(320);

  auto* textArea = new AdInputTextArea();
  textArea->setPlaceholder("textarea with clear icon");
  textArea->setAllowClear(true);
  textArea->setAutoSizeEnabled(true);
  textArea->setAutoSizeMinRows(3);
  textArea->setAutoSizeMaxRows(4);
  textArea->setFixedWidth(420);

  auto* output = makeHintLabel("Change events will appear here.");
  connect(input, &AdInput::valueChanged, output,
          [output](const QString& text) { output->setText(QStringLiteral("Input changed: %1").arg(text)); });
  connect(textArea, &AdInputTextArea::valueChanged, output,
          [output](const QString& text) { output->setText(QStringLiteral("TextArea changed: %1").arg(text)); });

  layout->addWidget(input, 0, Qt::AlignLeft);
  layout->addWidget(textArea, 0, Qt::AlignLeft);
  layout->addWidget(output);
  return box;
}

QWidget* InputDocsPage::buildShowCountDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(32);

  auto* input = new AdInput();
  input->setShowCount(true);
  input->setMaxLength(20);
  input->setFixedWidth(420);

  auto* textArea1 = new AdInputTextArea();
  textArea1->setShowCount(true);
  textArea1->setMaxLength(100);
  textArea1->setPlaceholder("can resize");
  textArea1->setAutoSizeEnabled(false);
  textArea1->setFixedWidth(420);

  auto* textArea2 = new AdInputTextArea();
  textArea2->setShowCount(true);
  textArea2->setMaxLength(100);
  textArea2->setPlaceholder("disable resize");
  textArea2->setAutoSizeEnabled(false);
  textArea2->setAutoSizeMinRows(4);
  textArea2->setAutoSizeMaxRows(4);
  textArea2->setFixedWidth(420);

  layout->addWidget(input, 0, Qt::AlignLeft);
  layout->addWidget(textArea1, 0, Qt::AlignLeft);
  layout->addWidget(textArea2, 0, Qt::AlignLeft);
  return box;
}

QWidget* InputDocsPage::buildAdvanceCountDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(12);

  layout->addWidget(new QLabel("Exceed Max"), 0, Qt::AlignLeft);
  auto* exceed = new AdInput();
  exceed->setShowCount(true);
  exceed->setCountMax(10);
  exceed->setValue("Hello, antd!");
  exceed->setFixedWidth(320);
  layout->addWidget(exceed, 0, Qt::AlignLeft);

  layout->addWidget(new QLabel("Emoji count as length 1"), 0, Qt::AlignLeft);
  auto* emoji = new AdInput();
  emoji->setShowCount(true);
  emoji->setCountStrategy([](const QString& text) {
    int count = 0;
    for (int i = 0; i < text.size(); ++i) {
      if (text.at(i).isLowSurrogate()) {
        continue;
      }
      ++count;
    }
    return count;
  });
  emoji->setValue("??????");
  emoji->setFixedWidth(320);
  layout->addWidget(emoji, 0, Qt::AlignLeft);

  layout->addWidget(new QLabel("Not exceed max"), 0, Qt::AlignLeft);
  auto* notExceed = new AdInput();
  notExceed->setShowCount(true);
  notExceed->setCountMax(6);
  notExceed->setCountStrategy([](const QString& text) {
    int count = 0;
    for (int i = 0; i < text.size(); ++i) {
      if (text.at(i).isLowSurrogate()) {
        continue;
      }
      ++count;
    }
    return count;
  });
  notExceed->setExceedFormatter([](const QString& text, int max) {
    QString out;
    out.reserve(text.size());
    int count = 0;
    for (int i = 0; i < text.size() && count < max; ++i) {
      const QChar ch = text.at(i);
      out.append(ch);
      if (!ch.isLowSurrogate()) {
        ++count;
      }
    }
    return out;
  });
  notExceed->setValue("?? antd");
  notExceed->setFixedWidth(320);
  layout->addWidget(notExceed, 0, Qt::AlignLeft);

  return box;
}

QWidget* InputDocsPage::buildStatusDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(10);

  auto* e1 = new AdInput();
  e1->setStatus(AdInput::Status::Error);
  e1->setPlaceholder("Error");

  auto* w1 = new AdInput();
  w1->setStatus(AdInput::Status::Warning);
  w1->setPlaceholder("Warning");

  auto* e2 = new AdInput();
  e2->setStatus(AdInput::Status::Error);
  e2->setPrefixIconToken(outlined_icons::ClockCircle());
  e2->setPlaceholder("Error with prefix");

  auto* w2 = new AdInput();
  w2->setStatus(AdInput::Status::Warning);
  w2->setPrefixIconToken(outlined_icons::ClockCircle());
  w2->setPlaceholder("Warning with prefix");

  layout->addWidget(e1, 0, Qt::AlignLeft);
  layout->addWidget(w1, 0, Qt::AlignLeft);
  layout->addWidget(e2, 0, Qt::AlignLeft);
  layout->addWidget(w2, 0, Qt::AlignLeft);
  return box;
}

QWidget* InputDocsPage::buildFocusDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(10);

  auto* controls = new QHBoxLayout();
  controls->setSpacing(8);

  auto* stack = new QStackedWidget();
  auto* input = new AdInput();
  input->setValue("Ant Design love you!");
  input->setFixedWidth(420);

  auto* textArea = new AdInputTextArea();
  textArea->setValue("Ant Design love you!");
  textArea->setAutoSizeEnabled(true);
  textArea->setAutoSizeMinRows(3);
  textArea->setAutoSizeMaxRows(3);
  textArea->setFixedWidth(420);

  stack->addWidget(input);
  stack->addWidget(textArea);

  auto* focusStart = new AdButton("Focus at first");
  auto* focusEnd = new AdButton("Focus at last");
  auto* focusAll = new AdButton("Focus to select all");
  auto* focusPrevent = new AdButton("Focus prevent scroll");
  auto* toggle = new QCheckBox("TextArea");

  controls->addWidget(focusStart);
  controls->addWidget(focusEnd);
  controls->addWidget(focusAll);
  controls->addWidget(focusPrevent);
  controls->addWidget(toggle);
  controls->addStretch();

  auto focusCurrent = [stack, input, textArea](AdInput::FocusCursor cursor, bool preventScroll) {
    if (stack->currentIndex() == 0) {
      input->focusInput(cursor, preventScroll);
    } else {
      textArea->focusInput(cursor, preventScroll);
    }
  };

  connect(focusStart, &QPushButton::clicked, box,
          [focusCurrent]() { focusCurrent(AdInput::FocusCursor::Start, false); });
  connect(focusEnd, &QPushButton::clicked, box,
          [focusCurrent]() { focusCurrent(AdInput::FocusCursor::End, false); });
  connect(focusAll, &QPushButton::clicked, box,
          [focusCurrent]() { focusCurrent(AdInput::FocusCursor::All, false); });
  connect(focusPrevent, &QPushButton::clicked, box,
          [focusCurrent]() { focusCurrent(AdInput::FocusCursor::Keep, true); });
  connect(toggle, &QCheckBox::toggled, stack, [stack](bool checked) { stack->setCurrentIndex(checked ? 1 : 0); });

  layout->addLayout(controls);
  layout->addWidget(stack, 0, Qt::AlignLeft);
  return box;
}

QWidget* InputDocsPage::buildStyleClassDemo() {
  auto* box = new QWidget();
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(12);

  auto* inputObject = new AdInput();
  AdInput::SemanticStyles inputStyles;
  inputStyles.root.borderColor = QColor("#696FC7");
  inputStyles.input.textColor = QColor("#696FC7");
  inputObject->setSemanticStyles(inputStyles);
  inputObject->setPlaceholder("Object");
  inputObject->setFixedWidth(320);

  auto* inputFn = new AdInput();
  inputFn->setPlaceholder("Function");
  inputFn->setSemanticStyleResolver([](const AdInput::StyleContext& ctx) {
    AdInput::SemanticStyles styles;
    if (ctx.size == AdInput::Size::Middle) {
      styles.root.borderColor = QColor("#696FC7");
    }
    return styles;
  });
  inputFn->setFixedWidth(320);

  auto* textArea = new AdInputTextArea();
  textArea->setValue("TextArea");
  textArea->setShowCount(true);
  textArea->setSemanticStyleResolver([](const AdInput::StyleContext& ctx) {
    AdInput::SemanticStyles styles;
    if (ctx.showCount) {
      styles.root.borderColor = QColor("#BDE3C3");
      styles.count.textColor = QColor("#BDE3C3");
    }
    return styles;
  });
  textArea->setAutoSizeEnabled(true);
  textArea->setAutoSizeMinRows(2);
  textArea->setAutoSizeMaxRows(2);
  textArea->setFixedWidth(420);

  auto* password = new AdInputPassword();
  password->setValue("Password");
  AdInput::SemanticStyles passwordStyles;
  passwordStyles.root.borderColor = QColor("#F5D3C4");
  password->input()->setSemanticStyles(passwordStyles);
  password->setFixedWidth(320);

  auto* otp = new AdInputOtp();
  otp->setLength(6);
  otp->setSeparatorText("*");

  auto* search = new AdInputSearch();
  search->setPlaceholder("Search");
  search->setSize(AdInput::Size::Large);
  search->setEnterButton(true);
  search->setEnterButtonText("Search");
  AdInput::SemanticStyles searchStyles;
  searchStyles.root.borderColor = QColor("#4DA8DA");
  searchStyles.input.textColor = QColor("#4DA8DA");
  searchStyles.prefix.textColor = QColor("#4DA8DA");
  searchStyles.suffix.textColor = QColor("#4DA8DA");
  search->input()->setSemanticStyles(searchStyles);
  search->setFixedWidth(360);

  layout->addWidget(inputObject, 0, Qt::AlignLeft);
  layout->addWidget(inputFn, 0, Qt::AlignLeft);
  layout->addWidget(textArea, 0, Qt::AlignLeft);
  layout->addWidget(password, 0, Qt::AlignLeft);
  layout->addWidget(otp, 0, Qt::AlignLeft);
  layout->addWidget(search, 0, Qt::AlignLeft);
  return box;
}
