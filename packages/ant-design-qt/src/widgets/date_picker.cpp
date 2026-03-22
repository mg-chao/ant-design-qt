#include "date_picker.h"

#include "antd_icons.h"
#include "button.h"
#include "detail/flow_layout.h"
#include "field_group.h"
#include "input_internal.h"
#include "input_line_edit.h"
#include "popover.h"
#include "theme/theme.h"
#include "theme/theme_manager.h"

#include <QAbstractButton>
#include <QDate>
#include <QDateTime>
#include <QComboBox>
#include <QEnterEvent>
#include <QEvent>
#include <QFontMetrics>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QListWidget>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QStandardItemModel>
#include <QStyleOption>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <optional>

namespace adqt::widgets {

namespace {

namespace outlined_icons = adqt::icons::antd::outlined;
using detail::input_internal::renderTintedIcon;

constexpr int kDayRows = 6;
constexpr int kDayColumns = 7;

struct DatePickerMetrics {
  int controlHeight = 32;
  int borderRadius = 6;
  int borderWidth = 1;
  int horizontalPadding = 11;
  int verticalPadding = 4;
  qreal focusOutlineWidth = 3.0;
  int popupWidth = 320;
  int popupPadding = 12;
  int popupRadius = 8;
  int cellSize = 36;
  int cellRadius = 6;
  int timeColumnWidth = 68;
  int timeCellHeight = 28;
  int presetRailWidth = 112;
  int chipHeight = 22;
  int chipRadius = 4;
  int chipPaddingStart = 8;
  int chipPaddingEnd = 6;
  int chipContentGap = 4;
  int iconSize = 14;
  int footerGap = 8;
};

struct DatePickerVisualStyle {
  QColor selectorBg;
  QColor selectorHoverBg;
  QColor selectorActiveBg;
  QColor selectorBorder;
  QColor selectorHoverBorder;
  QColor selectorActiveBorder;
  QColor selectorFocusOutline;
  QColor selectorText;
  QColor placeholderText;
  QColor disabledText;
  QColor disabledBg;
  QColor disabledBorder;
  QColor popupBg;
  QColor popupBorder;
  QColor titleText;
  QColor mutedText;
  QColor cellText;
  QColor cellMutedText;
  QColor cellDisabledText;
  QColor cellHoverBg;
  QColor cellRangeBg;
  QColor cellSelectedBg;
  QColor cellSelectedText;
  QColor cellTodayBorder;
  QColor divider;
  QColor chipBg;
  QColor chipBorder;
  QColor chipText;
  QColor clearColor;
  QColor clearHoverColor;
  QColor iconColor;
  QColor presetHoverBg;
  QColor presetText;
  QColor footerText;
  bool underlined = false;
  DatePickerMetrics metrics;
};

QColor fallbackColor(const QColor& value, const QColor& fallback) {
  return value.isValid() ? value : fallback;
}

QColor optionalColor(const std::optional<QColor>& value, const QColor& fallback) {
  return value.has_value() ? fallbackColor(value.value(), fallback) : fallback;
}

int optionalInt(const std::optional<int>& value, int fallback) {
  return value.has_value() ? std::max(0, value.value()) : fallback;
}

QColor withAlpha(const QColor& color, qreal alpha) {
  QColor result = color;
  result.setAlphaF(std::clamp(alpha, 0.0, 1.0));
  return result;
}

DatePickerVisualStyle resolveDatePickerVisualStyle(
    const QWidget* widget,
    AdDatePicker::ControlSize controlSize,
    AdDatePicker::Variant variant,
    AdDatePicker::Status status,
    bool disabled,
    bool active,
    const AdDatePicker::ComponentTokens& tokens) {
  const adqt::theme::ThemeMapToken& map = adqt::theme::ThemeManager::instance().resolveTheme(widget);

  DatePickerVisualStyle style;
  style.selectorBg = fallbackColor(map.colorBgContainer, QColor("#ffffff"));
  style.selectorHoverBg = style.selectorBg;
  style.selectorActiveBg = style.selectorBg;
  style.selectorBorder = fallbackColor(map.colorBorder, QColor("#d9d9d9"));
  style.selectorHoverBorder = fallbackColor(map.colorPrimaryHover, QColor("#4096ff"));
  style.selectorActiveBorder = fallbackColor(map.colorPrimary, QColor("#1677ff"));
  style.selectorFocusOutline = withAlpha(fallbackColor(map.colorPrimaryBg, QColor("#e6f4ff")), 1.0);
  style.selectorText = fallbackColor(map.colorText, QColor("#141414"));
  style.placeholderText = fallbackColor(map.colorTextQuaternary, QColor("#bfbfbf"));
  style.disabledText = fallbackColor(map.colorTextDisabled, QColor("#bfbfbf"));
  style.disabledBg = fallbackColor(map.colorBgContainerDisabled, QColor("#f5f5f5"));
  style.disabledBorder = fallbackColor(map.colorBorderDisabled, QColor("#d9d9d9"));
  style.popupBg = fallbackColor(map.colorBgElevated, QColor("#ffffff"));
  style.popupBorder = fallbackColor(map.colorBorderSecondary, QColor("#f0f0f0"));
  style.titleText = fallbackColor(map.colorTextBase, style.selectorText);
  style.mutedText = fallbackColor(map.colorTextSecondary, QColor("#8c8c8c"));
  style.cellText = style.selectorText;
  style.cellMutedText = fallbackColor(map.colorTextTertiary, QColor("#8c8c8c"));
  style.cellDisabledText = style.disabledText;
  style.cellHoverBg = fallbackColor(map.colorFillTertiary, QColor("#f5f5f5"));
  style.cellRangeBg = fallbackColor(map.colorPrimaryBg, QColor("#e6f4ff"));
  style.cellSelectedBg = fallbackColor(map.colorPrimary, QColor("#1677ff"));
  style.cellSelectedText = fallbackColor(map.colorTextLightSolid, QColor("#ffffff"));
  style.cellTodayBorder = fallbackColor(map.colorPrimary, QColor("#1677ff"));
  style.divider = fallbackColor(map.colorBorderSecondary, QColor("#f0f0f0"));
  style.chipBg = fallbackColor(map.colorFillSecondary, QColor("#f5f5f5"));
  style.chipBorder = QColor(0, 0, 0, 0);
  style.chipText = style.selectorText;
  style.clearColor = fallbackColor(map.colorTextQuaternary, QColor("#bfbfbf"));
  style.clearHoverColor = fallbackColor(map.colorTextTertiary, QColor("#8c8c8c"));
  style.iconColor = fallbackColor(map.colorTextQuaternary, QColor("#bfbfbf"));
  style.presetHoverBg = fallbackColor(map.colorFillTertiary, QColor("#f5f5f5"));
  style.presetText = style.selectorText;
  style.footerText = style.mutedText;

  style.metrics.controlHeight = std::max(24, qRound(map.controlHeight));
  style.metrics.borderRadius = std::max(0, qRound(map.borderRadius));
  style.metrics.borderWidth = std::max(1, qRound(map.lineWidth));
  style.metrics.horizontalPadding = std::max(8, qRound(map.sizeSM - map.lineWidth));
  style.metrics.verticalPadding = std::max(2, qRound(map.sizeXXS));
  style.metrics.focusOutlineWidth = std::max<qreal>(1.0, map.lineWidth * 3.0);
  style.metrics.popupWidth = 320;
  style.metrics.popupPadding = std::max(8, qRound(map.sizeSM));
  style.metrics.popupRadius = std::max(6, qRound(map.borderRadiusLG));
  style.metrics.cellSize = 36;
  style.metrics.cellRadius = std::max(4, qRound(map.borderRadiusSM));
  style.metrics.timeColumnWidth = 68;
  style.metrics.timeCellHeight = 28;
  style.metrics.presetRailWidth = 112;
  style.metrics.chipHeight = std::max(18, qRound(map.controlHeightSM - map.lineWidth * 2.0));
  style.metrics.chipRadius = std::max(4, qRound(map.borderRadiusSM));
  style.metrics.chipPaddingStart = 8;
  style.metrics.chipPaddingEnd = 6;
  style.metrics.chipContentGap = std::max(2, qRound(map.sizeXXS));
  style.metrics.iconSize = std::max(12, qRound(map.fontSizeSM));
  style.metrics.footerGap = std::max(6, qRound(map.sizeXS));

  if (controlSize == AdDatePicker::ControlSize::Large) {
    style.metrics.controlHeight = std::max(style.metrics.controlHeight, qRound(map.controlHeightLG));
    style.metrics.cellSize = 40;
  } else if (controlSize == AdDatePicker::ControlSize::Small) {
    style.metrics.controlHeight = std::max(24, qRound(map.controlHeightSM));
    style.metrics.cellSize = 32;
    style.metrics.horizontalPadding = std::max(6, qRound(map.sizeXS - map.lineWidth));
  }

  if (variant == AdDatePicker::Variant::Filled) {
    style.selectorBg = fallbackColor(map.colorFillTertiary, QColor("#f5f5f5"));
    style.selectorHoverBg = fallbackColor(map.colorFillSecondary, QColor("#f0f0f0"));
    style.selectorBorder = QColor(0, 0, 0, 0);
    style.selectorHoverBorder = QColor(0, 0, 0, 0);
  } else if (variant == AdDatePicker::Variant::Borderless) {
    style.selectorBg = QColor(0, 0, 0, 0);
    style.selectorHoverBg = QColor(0, 0, 0, 0);
    style.selectorActiveBg = QColor(0, 0, 0, 0);
    style.selectorBorder = QColor(0, 0, 0, 0);
    style.selectorHoverBorder = QColor(0, 0, 0, 0);
    style.selectorFocusOutline = QColor(0, 0, 0, 0);
  } else if (variant == AdDatePicker::Variant::Underlined) {
    style.selectorBg = QColor(0, 0, 0, 0);
    style.selectorHoverBg = QColor(0, 0, 0, 0);
    style.selectorActiveBg = QColor(0, 0, 0, 0);
    style.underlined = true;
  }

  if (status == AdDatePicker::Status::Error) {
    style.selectorBorder = fallbackColor(map.colorError, QColor("#ff4d4f"));
    style.selectorHoverBorder = fallbackColor(map.colorErrorHover, QColor("#ff7875"));
    style.selectorActiveBorder = fallbackColor(map.colorError, QColor("#ff4d4f"));
    style.selectorFocusOutline = withAlpha(fallbackColor(map.colorErrorBg, QColor("#fff2f0")), 1.0);
  } else if (status == AdDatePicker::Status::Warning) {
    style.selectorBorder = fallbackColor(map.colorWarning, QColor("#faad14"));
    style.selectorHoverBorder = fallbackColor(map.colorWarningHover, QColor("#ffd666"));
    style.selectorActiveBorder = fallbackColor(map.colorWarning, QColor("#faad14"));
    style.selectorFocusOutline = withAlpha(fallbackColor(map.colorWarningBg, QColor("#fffbe6")), 1.0);
  }

  if (disabled) {
    style.selectorBg = style.disabledBg;
    style.selectorHoverBg = style.disabledBg;
    style.selectorActiveBg = style.disabledBg;
    style.selectorBorder = style.disabledBorder;
    style.selectorHoverBorder = style.disabledBorder;
    style.selectorActiveBorder = style.disabledBorder;
    style.selectorText = style.disabledText;
    style.iconColor = style.disabledText;
    style.clearColor = style.disabledText;
  } else if (active && !style.underlined) {
    style.selectorBorder = style.selectorActiveBorder;
  }

  style.selectorBg = optionalColor(tokens.selectorBackground, style.selectorBg);
  style.selectorBorder = optionalColor(tokens.selectorBorderColor, style.selectorBorder);
  style.selectorHoverBorder = optionalColor(tokens.selectorHoverBorderColor, style.selectorHoverBorder);
  style.selectorActiveBorder = optionalColor(tokens.selectorActiveBorderColor, style.selectorActiveBorder);
  style.popupBg = optionalColor(tokens.popupBackground, style.popupBg);
  style.popupBorder = optionalColor(tokens.popupBorderColor, style.popupBorder);
  style.cellHoverBg = optionalColor(tokens.cellHoverBackground, style.cellHoverBg);
  style.cellRangeBg = optionalColor(tokens.cellRangeBackground, style.cellRangeBg);
  style.cellSelectedBg = optionalColor(tokens.cellSelectedBackground, style.cellSelectedBg);
  style.chipBg = optionalColor(tokens.chipBackground, style.chipBg);
  style.chipBorder = optionalColor(tokens.chipBorderColor, style.chipBorder);

  style.metrics.controlHeight = std::max(24, optionalInt(tokens.controlHeight, style.metrics.controlHeight));
  style.metrics.borderRadius = optionalInt(tokens.borderRadius, style.metrics.borderRadius);
  style.metrics.popupWidth = std::max(260, optionalInt(tokens.popupWidth, style.metrics.popupWidth));
  style.metrics.popupPadding = std::max(4, optionalInt(tokens.popupPadding, style.metrics.popupPadding));
  style.metrics.cellSize = std::max(28, optionalInt(tokens.cellSize, style.metrics.cellSize));
  style.metrics.timeColumnWidth = std::max(48, optionalInt(tokens.timeColumnWidth, style.metrics.timeColumnWidth));
  style.metrics.timeCellHeight = std::max(20, optionalInt(tokens.timeCellHeight, style.metrics.timeCellHeight));
  style.metrics.presetRailWidth = std::max(72, optionalInt(tokens.presetRailWidth, style.metrics.presetRailWidth));
  style.metrics.chipHeight = std::max(18, optionalInt(tokens.chipHeight, style.metrics.chipHeight));

  return style;
}

AdPopover::Placement toPopoverPlacement(AdDatePicker::Placement placement) {
  switch (placement) {
    case AdDatePicker::Placement::BottomRight:
      return AdPopover::Placement::BottomRight;
    case AdDatePicker::Placement::TopLeft:
      return AdPopover::Placement::TopLeft;
    case AdDatePicker::Placement::TopRight:
      return AdPopover::Placement::TopRight;
    case AdDatePicker::Placement::BottomLeft:
    default:
      return AdPopover::Placement::BottomLeft;
  }
}

AdLineEdit::ControlSize toLineEditSize(AdDatePicker::ControlSize size) {
  switch (size) {
    case AdDatePicker::ControlSize::Large:
      return AdLineEdit::ControlSize::Large;
    case AdDatePicker::ControlSize::Small:
      return AdLineEdit::ControlSize::Small;
    case AdDatePicker::ControlSize::Middle:
    default:
      return AdLineEdit::ControlSize::Medium;
  }
}

AdLineEdit::Variant toLineEditVariant(AdDatePicker::Variant variant) {
  switch (variant) {
    case AdDatePicker::Variant::Filled:
      return AdLineEdit::Variant::Filled;
    case AdDatePicker::Variant::Borderless:
      return AdLineEdit::Variant::Borderless;
    case AdDatePicker::Variant::Underlined:
      return AdLineEdit::Variant::Underlined;
    case AdDatePicker::Variant::Outlined:
    default:
      return AdLineEdit::Variant::Outlined;
  }
}

AdLineEdit::Status toLineEditStatus(AdDatePicker::Status status) {
  switch (status) {
    case AdDatePicker::Status::Error:
      return AdLineEdit::Status::Error;
    case AdDatePicker::Status::Warning:
      return AdLineEdit::Status::Warning;
    case AdDatePicker::Status::None:
    default:
      return AdLineEdit::Status::None;
  }
}

Qt::DayOfWeek localeFirstDay(const QLocale& locale) {
  return locale.firstDayOfWeek();
}

QDate startOfWeek(const QDate& date, const QLocale& locale) {
  if (!date.isValid()) {
    return date;
  }
  const int current = static_cast<int>(date.dayOfWeek());
  const int first = static_cast<int>(localeFirstDay(locale));
  const int delta = (current - first + 7) % 7;
  return date.addDays(-delta);
}

QDate startOfQuarter(const QDate& date) {
  if (!date.isValid()) {
    return date;
  }
  const int month = ((date.month() - 1) / 3) * 3 + 1;
  return QDate(date.year(), month, 1);
}

QDate canonicalDate(const QDate& date, AdDatePicker::PickerMode mode, const QLocale& locale) {
  if (!date.isValid()) {
    return date;
  }
  switch (mode) {
    case AdDatePicker::PickerMode::Week:
      return startOfWeek(date, locale);
    case AdDatePicker::PickerMode::Month:
      return QDate(date.year(), date.month(), 1);
    case AdDatePicker::PickerMode::Quarter:
      return startOfQuarter(date);
    case AdDatePicker::PickerMode::Year:
      return QDate(date.year(), 1, 1);
    case AdDatePicker::PickerMode::Date:
    default:
      return date;
  }
}

QDateTime canonicalDateTime(const QDateTime& value,
                            AdDatePicker::PickerMode mode,
                            bool keepTime,
                            const QLocale& locale) {
  if (!value.isValid()) {
    return value;
  }
  QDateTime result = value;
  result.setDate(canonicalDate(value.date(), mode, locale));
  if (!keepTime) {
    result.setTime(QTime(0, 0, 0));
  }
  return result;
}

QString defaultDisplayFormat(AdDatePicker::PickerMode mode,
                             bool showTime,
                             const AdDateTimePanelOptions& options) {
  if (mode == AdDatePicker::PickerMode::Week) {
    return QStringLiteral("yyyy-'W'ww");
  }
  if (mode == AdDatePicker::PickerMode::Month) {
    return QStringLiteral("yyyy-MM");
  }
  if (mode == AdDatePicker::PickerMode::Quarter) {
    return QStringLiteral("yyyy-'Q'1");
  }
  if (mode == AdDatePicker::PickerMode::Year) {
    return QStringLiteral("yyyy");
  }
  if (!showTime) {
    return QStringLiteral("yyyy-MM-dd");
  }
  if (options.showSecond) {
    return QStringLiteral("yyyy-MM-dd HH:mm:ss");
  }
  if (options.showMinute) {
    return QStringLiteral("yyyy-MM-dd HH:mm");
  }
  return QStringLiteral("yyyy-MM-dd HH");
}

QString formatQuarterText(const QDate& date) {
  if (!date.isValid()) {
    return QString();
  }
  const int quarter = ((date.month() - 1) / 3) + 1;
  return QStringLiteral("%1-Q%2").arg(date.year()).arg(quarter);
}

QString formatWeekText(const QDate& date) {
  if (!date.isValid()) {
    return QString();
  }
  int weekYear = date.year();
  const int week = date.weekNumber(&weekYear);
  return QStringLiteral("%1-W%2").arg(weekYear).arg(week, 2, 10, QLatin1Char('0'));
}

QString formatDateTimeDisplay(const QDateTime& value,
                              AdDatePicker::PickerMode mode,
                              const QString& displayFormat,
                              bool showTime,
                              const AdDateTimePanelOptions& options) {
  if (!value.isValid()) {
    return QString();
  }
  if (mode == AdDatePicker::PickerMode::Quarter) {
    return formatQuarterText(value.date());
  }
  if (mode == AdDatePicker::PickerMode::Week) {
    return formatWeekText(value.date());
  }
  const QString format = displayFormat.isEmpty() ? defaultDisplayFormat(mode, showTime, options) : displayFormat;
  return value.toString(format);
}

QDate isoWeekDate(int isoYear, int isoWeek) {
  if (isoWeek < 1 || isoWeek > 53) {
    return QDate();
  }
  const QDate januaryFourth(isoYear, 1, 4);
  if (!januaryFourth.isValid()) {
    return QDate();
  }
  const QDate monday = januaryFourth.addDays(1 - januaryFourth.dayOfWeek());
  return monday.addDays((isoWeek - 1) * 7);
}

std::optional<QDateTime> parseWeekText(const QString& text) {
  static const QRegularExpression regex(QStringLiteral("^(\\d{4})\\D*W?(\\d{1,2})$"),
                                        QRegularExpression::CaseInsensitiveOption);
  const QRegularExpressionMatch match = regex.match(text.trimmed());
  if (!match.hasMatch()) {
    return std::nullopt;
  }
  bool yearOk = false;
  bool weekOk = false;
  const int year = match.captured(1).toInt(&yearOk);
  const int week = match.captured(2).toInt(&weekOk);
  if (!yearOk || !weekOk) {
    return std::nullopt;
  }
  const QDate parsed = isoWeekDate(year, week);
  return parsed.isValid() ? std::optional<QDateTime>(QDateTime(parsed, QTime(0, 0, 0)))
                          : std::nullopt;
}

std::optional<QDateTime> parseQuarterText(const QString& text) {
  static const QRegularExpression regex(QStringLiteral("^(\\d{4})\\D*Q(\\d)$"),
                                        QRegularExpression::CaseInsensitiveOption);
  const QRegularExpressionMatch match = regex.match(text.trimmed());
  if (!match.hasMatch()) {
    return std::nullopt;
  }
  bool yearOk = false;
  bool quarterOk = false;
  const int year = match.captured(1).toInt(&yearOk);
  const int quarter = match.captured(2).toInt(&quarterOk);
  if (!yearOk || !quarterOk || quarter < 1 || quarter > 4) {
    return std::nullopt;
  }
  return QDateTime(QDate(year, (quarter - 1) * 3 + 1, 1), QTime(0, 0, 0));
}

std::optional<QDateTime> parseDateTimeDisplay(const QString& text,
                                              AdDatePicker::PickerMode mode,
                                              const QStringList& acceptedFormats,
                                              bool showTime,
                                              const AdDateTimePanelOptions& options) {
  const QString trimmed = text.trimmed();
  if (trimmed.isEmpty()) {
    return std::nullopt;
  }

  if (mode == AdDatePicker::PickerMode::Week) {
    if (const auto parsed = parseWeekText(trimmed); parsed.has_value()) {
      return parsed;
    }
  }
  if (mode == AdDatePicker::PickerMode::Quarter) {
    if (const auto parsed = parseQuarterText(trimmed); parsed.has_value()) {
      return parsed;
    }
  }

  QStringList formats = acceptedFormats;
  if (formats.isEmpty()) {
    formats.append(defaultDisplayFormat(mode, showTime, options));
  }

  for (const QString& format : formats) {
    if (showTime) {
      const QDateTime parsed = QDateTime::fromString(trimmed, format);
      if (parsed.isValid()) {
        return parsed;
      }
    }
    const QDate date = QDate::fromString(trimmed, format);
    if (date.isValid()) {
      return QDateTime(date, QTime(0, 0, 0));
    }
    const QDateTime dateTime = QDateTime::fromString(trimmed, format);
    if (dateTime.isValid()) {
      return dateTime;
    }
  }

  return std::nullopt;
}

bool vectorContains(const QVector<int>& values, int candidate) {
  return std::find(values.cbegin(), values.cend(), candidate) != values.cend();
}

class DateCellButton final : public QAbstractButton {
 public:
  explicit DateCellButton(QWidget* parent = nullptr) : QAbstractButton(parent) {
    setCursor(Qt::PointingHandCursor);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  }

  void setCellDate(const QDate& date) { date_ = date; }
  QDate cellDate() const { return date_; }

  void setTextValue(const QString& text) {
    text_ = text;
    update();
  }

  void setStyleData(const DatePickerVisualStyle& style,
                    bool disabled,
                    bool muted,
                    bool today,
                    bool selected,
                    bool inRange,
                    bool rangeStart,
                    bool rangeEnd) {
    style_ = style;
    disabled_ = disabled;
    muted_ = muted;
    today_ = today;
    selected_ = selected;
    inRange_ = inRange;
    rangeStart_ = rangeStart;
    rangeEnd_ = rangeEnd;
    setEnabled(!disabled);
    setCursor(disabled ? Qt::ArrowCursor : Qt::PointingHandCursor);
    update();
  }

  QSize sizeHint() const override {
    return QSize(style_.metrics.cellSize, style_.metrics.cellSize);
  }

 protected:
  void paintEvent(QPaintEvent* event) override {
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF frameRect = rect().adjusted(0.5, 0.5, -0.5, -0.5);
    if (!frameRect.isValid()) {
      return;
    }

    if (inRange_) {
      painter.setPen(Qt::NoPen);
      painter.setBrush(style_.cellRangeBg);
      painter.drawRoundedRect(frameRect.adjusted(2.0, 4.0, -2.0, -4.0),
                              rangeStart_ || rangeEnd_ ? style_.metrics.cellRadius : 0,
                              rangeStart_ || rangeEnd_ ? style_.metrics.cellRadius : 0);
    }

    QColor fill(0, 0, 0, 0);
    if (selected_) {
      fill = style_.cellSelectedBg;
    } else if (!disabled_ && underMouse()) {
      fill = style_.cellHoverBg;
    }

    if (fill.alpha() > 0) {
      painter.setPen(Qt::NoPen);
      painter.setBrush(fill);
      painter.drawRoundedRect(frameRect.adjusted(3.0, 3.0, -3.0, -3.0), style_.metrics.cellRadius,
                              style_.metrics.cellRadius);
    }

    if (today_ && !selected_) {
      painter.setPen(QPen(style_.cellTodayBorder, 1.0));
      painter.setBrush(Qt::NoBrush);
      painter.drawRoundedRect(frameRect.adjusted(3.0, 3.0, -3.0, -3.0), style_.metrics.cellRadius,
                              style_.metrics.cellRadius);
    }

    QColor textColor = style_.cellText;
    if (selected_) {
      textColor = style_.cellSelectedText;
    } else if (disabled_) {
      textColor = style_.cellDisabledText;
    } else if (muted_) {
      textColor = style_.cellMutedText;
    }

    painter.setPen(textColor);
    painter.drawText(rect(), Qt::AlignCenter, text_);
  }

 private:
  QDate date_;
  QString text_;
  DatePickerVisualStyle style_;
  bool disabled_ = false;
  bool muted_ = false;
  bool today_ = false;
  bool selected_ = false;
  bool inRange_ = false;
  bool rangeStart_ = false;
  bool rangeEnd_ = false;
};

class DateChipWidget final : public QWidget {
  Q_OBJECT

 public:
  explicit DateChipWidget(QWidget* parent = nullptr) : QWidget(parent) {
    layout_ = new QHBoxLayout(this);
    layout_->setContentsMargins(8, 0, 6, 0);
    layout_->setSpacing(4);

    label_ = new QLabel(this);
    layout_->addWidget(label_);

    closeButton_ = new QToolButton(this);
    closeButton_->setAutoRaise(true);
    closeButton_->setFocusPolicy(Qt::NoFocus);
    closeButton_->setCursor(Qt::PointingHandCursor);
    closeButton_->setStyleSheet(QStringLiteral("QToolButton { border: 0; padding: 0; background: transparent; }"));
    layout_->addWidget(closeButton_);

    connect(closeButton_, &QToolButton::clicked, this, &DateChipWidget::removeRequested);
  }

  void setLabelText(const QString& text) { label_->setText(text); }

  void setRemovable(bool removable) { closeButton_->setVisible(removable); }

  void setVisualStyle(const DatePickerVisualStyle& style) {
    style_ = style;
    layout_->setContentsMargins(style.metrics.chipPaddingStart, 0, style.metrics.chipPaddingEnd, 0);
    layout_->setSpacing(style.metrics.chipContentGap);
    QPalette palette = label_->palette();
    palette.setColor(QPalette::WindowText, style.chipText);
    label_->setPalette(palette);

    const QPixmap pixmap =
        renderTintedIcon(outlined_icons::Close(), style.chipText, std::max(10, style.metrics.iconSize - 2),
                         devicePixelRatioF());
    closeButton_->setIcon(QIcon(pixmap));
    closeButton_->setIconSize(QSize(std::max(10, style.metrics.iconSize - 2),
                                    std::max(10, style.metrics.iconSize - 2)));
    setFixedHeight(style.metrics.chipHeight);
    update();
  }

 signals:
  void removeRequested();

 protected:
  void paintEvent(QPaintEvent* event) override {
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(style_.chipBorder.alpha() > 0 ? QPen(style_.chipBorder, 1.0) : Qt::NoPen);
    painter.setBrush(style_.chipBg);
    painter.drawRoundedRect(rect().adjusted(0.5, 0.5, -0.5, -0.5), style_.metrics.chipRadius,
                            style_.metrics.chipRadius);
  }

 private:
  DatePickerVisualStyle style_;
  QHBoxLayout* layout_ = nullptr;
  QLabel* label_ = nullptr;
  QToolButton* closeButton_ = nullptr;
};


QString defaultPlaceholderText(AdDatePicker::PickerMode mode) {
  switch (mode) {
    case AdDatePicker::PickerMode::Week:
      return QStringLiteral("Select week");
    case AdDatePicker::PickerMode::Month:
      return QStringLiteral("Select month");
    case AdDatePicker::PickerMode::Quarter:
      return QStringLiteral("Select quarter");
    case AdDatePicker::PickerMode::Year:
      return QStringLiteral("Select year");
    case AdDatePicker::PickerMode::Date:
    default:
      return QStringLiteral("Select date");
  }
}

QString defaultRangeStartPlaceholderText(AdDatePicker::PickerMode mode) {
  switch (mode) {
    case AdDatePicker::PickerMode::Week:
      return QStringLiteral("Start week");
    case AdDatePicker::PickerMode::Month:
      return QStringLiteral("Start month");
    case AdDatePicker::PickerMode::Quarter:
      return QStringLiteral("Start quarter");
    case AdDatePicker::PickerMode::Year:
      return QStringLiteral("Start year");
    case AdDatePicker::PickerMode::Date:
    default:
      return QStringLiteral("Start date");
  }
}

QString defaultRangeEndPlaceholderText(AdDatePicker::PickerMode mode) {
  switch (mode) {
    case AdDatePicker::PickerMode::Week:
      return QStringLiteral("End week");
    case AdDatePicker::PickerMode::Month:
      return QStringLiteral("End month");
    case AdDatePicker::PickerMode::Quarter:
      return QStringLiteral("End quarter");
    case AdDatePicker::PickerMode::Year:
      return QStringLiteral("End year");
    case AdDatePicker::PickerMode::Date:
    default:
      return QStringLiteral("End date");
  }
}

adqt::icons::IconRef defaultPickerSuffixIcon() {
  return outlined_icons::Calendar();
}

AdButton::SizeClass toButtonSize(AdDatePicker::ControlSize size) {
  switch (size) {
    case AdDatePicker::ControlSize::Large:
      return AdButton::SizeClass::Large;
    case AdDatePicker::ControlSize::Small:
      return AdButton::SizeClass::Small;
    case AdDatePicker::ControlSize::Middle:
    default:
      return AdButton::SizeClass::Medium;
  }
}

QDate stepDisplayDate(const QDate& date, AdDatePicker::PickerMode mode, int step) {
  if (!date.isValid()) {
    return QDate::currentDate();
  }
  switch (mode) {
    case AdDatePicker::PickerMode::Date:
    case AdDatePicker::PickerMode::Week:
      return date.addMonths(step);
    case AdDatePicker::PickerMode::Month:
    case AdDatePicker::PickerMode::Quarter:
      return date.addYears(step);
    case AdDatePicker::PickerMode::Year:
      return date.addYears(step * 10);
  }
  return date;
}

QDate intervalStartForDate(const QDate& date,
                           AdDatePicker::PickerMode mode,
                           const QLocale& locale) {
  return canonicalDate(date, mode, locale);
}

QDate intervalEndForDate(const QDate& date,
                         AdDatePicker::PickerMode mode,
                         const QLocale& locale) {
  const QDate start = intervalStartForDate(date, mode, locale);
  if (!start.isValid()) {
    return start;
  }
  switch (mode) {
    case AdDatePicker::PickerMode::Week:
      return start.addDays(6);
    case AdDatePicker::PickerMode::Month:
      return start.addMonths(1).addDays(-1);
    case AdDatePicker::PickerMode::Quarter:
      return start.addMonths(3).addDays(-1);
    case AdDatePicker::PickerMode::Year:
      return start.addYears(1).addDays(-1);
    case AdDatePicker::PickerMode::Date:
    default:
      return start;
  }
}

bool dateIntervalsOverlap(const QDate& candidateStart,
                          const QDate& candidateEnd,
                          const QDate& rangeStart,
                          const QDate& rangeEnd) {
  if (!candidateStart.isValid() || !candidateEnd.isValid() || !rangeStart.isValid() ||
      !rangeEnd.isValid()) {
    return false;
  }
  return candidateStart <= rangeEnd && candidateEnd >= rangeStart;
}

bool sameCanonicalValue(const QDate& lhs,
                        const QDate& rhs,
                        AdDatePicker::PickerMode mode,
                        const QLocale& locale) {
  return intervalStartForDate(lhs, mode, locale) == intervalStartForDate(rhs, mode, locale);
}

bool containsDateTime(const QVector<QDateTime>& values, const QDateTime& candidate) {
  return std::find(values.cbegin(), values.cend(), candidate) != values.cend();
}

QTime canonicalTime(const QTime& time, const AdDateTimePanelOptions& options) {
  if (!time.isValid()) {
    return QTime(options.defaultOpenTime.hour(),
                 options.showMinute ? options.defaultOpenTime.minute() : 0,
                 options.showSecond ? options.defaultOpenTime.second() : 0);
  }
  return QTime(time.hour(), options.showMinute ? time.minute() : 0,
               options.showSecond ? time.second() : 0);
}

bool isTimeAllowed(const QTime& time, const AdDisabledTimeSpec& spec) {
  if (!time.isValid()) {
    return true;
  }
  return !vectorContains(spec.disabledHours, time.hour()) &&
         !vectorContains(spec.disabledMinutes, time.minute()) &&
         !vectorContains(spec.disabledSeconds, time.second());
}

QTime firstAllowedTime(const AdDateTimePanelOptions& options, const AdDisabledTimeSpec& spec) {
  for (int hour = 0; hour < 24; ++hour) {
    if (vectorContains(spec.disabledHours, hour)) {
      continue;
    }
    const int minuteLimit = options.showMinute ? 60 : 1;
    for (int minute = 0; minute < minuteLimit; ++minute) {
      if (options.showMinute && vectorContains(spec.disabledMinutes, minute)) {
        continue;
      }
      const int secondLimit = options.showSecond ? 60 : 1;
      for (int second = 0; second < secondLimit; ++second) {
        if (options.showSecond && vectorContains(spec.disabledSeconds, second)) {
          continue;
        }
        return QTime(hour, options.showMinute ? minute : 0, options.showSecond ? second : 0);
      }
    }
  }
  return canonicalTime(options.defaultOpenTime, options);
}

QTime sanitizeTime(const QTime& candidate,
                   const AdDateTimePanelOptions& options,
                   const AdDisabledTimeSpec& spec) {
  const QTime normalized = canonicalTime(candidate, options);
  return isTimeAllowed(normalized, spec) ? normalized : firstAllowedTime(options, spec);
}

AdDisabledTimeSpec evaluateDisabledTime(
    const AdDatePicker::DisabledTimeEvaluator& evaluator,
    const QDate& date,
    AdDatePicker::RangePart part,
    const std::optional<QDate>& counterpartDate) {
  return evaluator ? evaluator(date, part, counterpartDate) : AdDisabledTimeSpec{};
}

QVector<QDateTime> normalizeDateTimes(const QVector<QDateTime>& values,
                                      AdDatePicker::PickerMode mode,
                                      bool keepTime,
                                      const QLocale& locale,
                                      bool autoSort) {
  QVector<QDateTime> result;
  result.reserve(values.size());
  for (const QDateTime& value : values) {
    if (!value.isValid()) {
      continue;
    }
    const QDateTime canonical = canonicalDateTime(value, mode, keepTime, locale);
    if (!containsDateTime(result, canonical)) {
      result.append(canonical);
    }
  }
  if (autoSort) {
    std::sort(result.begin(), result.end());
  }
  return result;
}

bool isDateBlocked(const QDate& date,
                   const QDate& minDate,
                   const QDate& maxDate,
                   const AdDatePicker::DisabledDateEvaluator& evaluator) {
  if (!date.isValid()) {
    return true;
  }
  if (minDate.isValid() && date < minDate) {
    return true;
  }
  if (maxDate.isValid() && date > maxDate) {
    return true;
  }
  return evaluator ? evaluator(date) : false;
}

QString monthShortName(const QLocale& locale, int month) {
  QString text = locale.monthName(month, QLocale::ShortFormat);
  if (text.isEmpty()) {
    text = QDate(2000, month, 1).toString(QStringLiteral("MMM"));
  }
  return text;
}

QString panelTitleText(const QDate& displayDate,
                       AdDatePicker::PickerMode mode,
                       const QLocale& locale) {
  if (!displayDate.isValid()) {
    return QString();
  }
  switch (mode) {
    case AdDatePicker::PickerMode::Date:
    case AdDatePicker::PickerMode::Week:
      return locale.toString(displayDate, QStringLiteral("MMMM yyyy"));
    case AdDatePicker::PickerMode::Month:
    case AdDatePicker::PickerMode::Quarter:
      return QString::number(displayDate.year());
    case AdDatePicker::PickerMode::Year: {
      const int startYear = (displayDate.year() / 10) * 10;
      return QStringLiteral("%1-%2").arg(startYear).arg(startYear + 9);
    }
  }
  return QString();
}

bool rangeCanCommit(const AdDateTimeRangeValue& value, bool allowEmptyStart, bool allowEmptyEnd) {
  const bool hasStart = value.start.isValid();
  const bool hasEnd = value.end.isValid();
  if (!hasStart && !hasEnd) {
    return false;
  }
  if (!hasStart && !allowEmptyStart) {
    return false;
  }
  if (!hasEnd && !allowEmptyEnd) {
    return false;
  }
  return true;
}

void applyToolButtonIcon(QToolButton* button,
                         const adqt::icons::IconRef& icon,
                         const QColor& color,
                         int size,
                         qreal dpr) {
  if (!button) {
    return;
  }
  const QSize iconSize(std::max(10, size), std::max(10, size));
  button->setIcon(QIcon(renderTintedIcon(icon, color, iconSize.width(), dpr)));
  button->setIconSize(iconSize);
}

class SimpleTimeEditor final : public QWidget {
  Q_OBJECT

 public:
  explicit SimpleTimeEditor(QWidget* parent = nullptr) : QWidget(parent) {
    layout_ = new QHBoxLayout(this);
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(6);

    hourCombo_ = new QComboBox(this);
    minuteCombo_ = new QComboBox(this);
    secondCombo_ = new QComboBox(this);
    meridiemCombo_ = new QComboBox(this);

    for (QComboBox* combo : {hourCombo_, minuteCombo_, secondCombo_, meridiemCombo_}) {
      combo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
      combo->setMinimumWidth(68);
      layout_->addWidget(combo);
      connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
              &SimpleTimeEditor::handleComboChanged);
    }

    setOptions(options_);
  }

  void setOptions(const AdDateTimePanelOptions& options) {
    options_ = options;
    rebuild();
  }

  void setDisabledSpec(const AdDisabledTimeSpec& spec) {
    disabledSpec_ = spec;
    rebuild();
  }

  void setVisualStyle(const DatePickerVisualStyle& style) {
    style_ = style;
    const QString borderColor = style.popupBorder.name(QColor::HexArgb);
    const QString background = style.selectorBg.name(QColor::HexArgb);
    const QString textColor = style.selectorText.name(QColor::HexArgb);
    const QString selection = style.cellSelectedBg.name(QColor::HexArgb);
    const QString css = QStringLiteral(
                            "QComboBox { min-height: %1px; padding: 2px 8px; border: 1px solid %2; "
                            "border-radius: %3px; background: %4; color: %5; } "
                            "QComboBox::drop-down { border: 0; width: 18px; } "
                            "QComboBox QAbstractItemView { border: 1px solid %2; selection-background-color: %6; }")
                            .arg(std::max(24, style.metrics.timeCellHeight))
                            .arg(borderColor)
                            .arg(std::max(4, style.metrics.cellRadius))
                            .arg(background)
                            .arg(textColor)
                            .arg(selection);
    for (QComboBox* combo : {hourCombo_, minuteCombo_, secondCombo_, meridiemCombo_}) {
      combo->setStyleSheet(css);
      combo->setMinimumWidth(std::max(56, style.metrics.timeColumnWidth));
    }
  }

  void setTime(const QTime& time) {
    time_ = canonicalTime(time, options_);
    rebuild();
  }

  QTime time() const { return canonicalTime(time_, options_); }

 signals:
  void timeChanged(const QTime& value);

 private:
  int findIndexByValue(QComboBox* combo, int value) const {
    if (!combo) {
      return -1;
    }
    for (int i = 0; i < combo->count(); ++i) {
      if (combo->itemData(i).toInt() == value) {
        return i;
      }
    }
    return -1;
  }

  void populateNumericCombo(QComboBox* combo,
                            int start,
                            int end,
                            const QVector<int>& disabledValues,
                            bool hideDisabled,
                            int currentValue,
                            bool twoDigits = true) {
    if (!combo) {
      return;
    }
    combo->clear();
    for (int value = start; value <= end; ++value) {
      if (hideDisabled && vectorContains(disabledValues, value)) {
        continue;
      }
      combo->addItem(twoDigits ? QStringLiteral("%1").arg(value, 2, 10, QLatin1Char('0'))
                               : QString::number(value),
                     value);
    }
    if (auto* model = qobject_cast<QStandardItemModel*>(combo->model())) {
      for (int row = 0; row < combo->count(); ++row) {
        if (auto* item = model->item(row)) {
          item->setEnabled(!vectorContains(disabledValues, combo->itemData(row).toInt()));
        }
      }
    }
    const int index = findIndexByValue(combo, currentValue);
    combo->setCurrentIndex(index >= 0 ? index : (combo->count() > 0 ? 0 : -1));
    combo->setEnabled(combo->count() > 0);
  }

  void rebuild() {
    const QSignalBlocker blocker1(hourCombo_);
    const QSignalBlocker blocker2(minuteCombo_);
    const QSignalBlocker blocker3(secondCombo_);
    const QSignalBlocker blocker4(meridiemCombo_);

    const QTime desired = sanitizeTime(time_, options_, disabledSpec_);
    time_ = desired;

    if (options_.use12Hour) {
      meridiemCombo_->clear();
      meridiemCombo_->addItem(QStringLiteral("AM"), 0);
      meridiemCombo_->addItem(QStringLiteral("PM"), 1);
      meridiemCombo_->setVisible(options_.showHour);
      meridiemCombo_->setCurrentIndex(desired.hour() >= 12 ? 1 : 0);

      hourCombo_->clear();
      const bool pm = meridiemCombo_->currentIndex() == 1;
      for (int hour24 = pm ? 12 : 0; hour24 <= (pm ? 23 : 11); ++hour24) {
        if (options_.hideDisabledOptions && vectorContains(disabledSpec_.disabledHours, hour24)) {
          continue;
        }
        const int displayHour = ((hour24 + 11) % 12) + 1;
        hourCombo_->addItem(QStringLiteral("%1").arg(displayHour, 2, 10, QLatin1Char('0')), hour24);
      }
      if (auto* model = qobject_cast<QStandardItemModel*>(hourCombo_->model())) {
        for (int row = 0; row < hourCombo_->count(); ++row) {
          if (auto* item = model->item(row)) {
            item->setEnabled(
                !vectorContains(disabledSpec_.disabledHours, hourCombo_->itemData(row).toInt()));
          }
        }
      }
      const int hourIndex = findIndexByValue(hourCombo_, desired.hour());
      hourCombo_->setCurrentIndex(hourIndex >= 0 ? hourIndex : (hourCombo_->count() > 0 ? 0 : -1));
    } else {
      meridiemCombo_->setVisible(false);
      populateNumericCombo(hourCombo_, 0, 23, disabledSpec_.disabledHours,
                           options_.hideDisabledOptions, desired.hour());
    }

    populateNumericCombo(minuteCombo_, 0, 59, disabledSpec_.disabledMinutes,
                         options_.hideDisabledOptions, desired.minute());
    populateNumericCombo(secondCombo_, 0, 59, disabledSpec_.disabledSeconds,
                         options_.hideDisabledOptions, desired.second());

    hourCombo_->setVisible(options_.showHour);
    minuteCombo_->setVisible(options_.showMinute);
    secondCombo_->setVisible(options_.showSecond);
  }

 private slots:
  void handleComboChanged() {
    int hour = options_.showHour ? hourCombo_->currentData().toInt() : time_.hour();
    const int minute = options_.showMinute ? minuteCombo_->currentData().toInt() : 0;
    const int second = options_.showSecond ? secondCombo_->currentData().toInt() : 0;

    if (options_.use12Hour && options_.showHour) {
      const bool pm = meridiemCombo_->currentData().toInt() == 1;
      const int baseHour = hourCombo_->currentData().toInt();
      if (baseHour >= 0) {
        hour = baseHour;
      } else {
        hour = pm ? 12 : 0;
      }
      const QTime normalized = sanitizeTime(QTime(hour, minute, second), options_, disabledSpec_);
      if ((normalized.hour() >= 12) != pm) {
        rebuild();
      }
      time_ = normalized;
    } else {
      time_ = sanitizeTime(QTime(hour, minute, second), options_, disabledSpec_);
    }
    emit timeChanged(time_);
  }

 private:
  QHBoxLayout* layout_ = nullptr;
  QComboBox* hourCombo_ = nullptr;
  QComboBox* minuteCombo_ = nullptr;
  QComboBox* secondCombo_ = nullptr;
  QComboBox* meridiemCombo_ = nullptr;
  AdDateTimePanelOptions options_;
  AdDisabledTimeSpec disabledSpec_;
  DatePickerVisualStyle style_;
  QTime time_ = QTime(0, 0, 0);
};

class DateTagInputSurface final : public QWidget {
  Q_OBJECT

 public:
  explicit DateTagInputSurface(QWidget* parent = nullptr) : QWidget(parent) {
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::PointingHandCursor);
    setMouseTracking(true);

    rootLayout_ = new QHBoxLayout(this);
    rootLayout_->setContentsMargins(11, 4, 11, 4);
    rootLayout_->setSpacing(6);

    prefixIconLabel_ = new QLabel(this);
    prefixTextLabel_ = new QLabel(this);
    chipsHost_ = new QWidget(this);
    chipsLayout_ = new detail::FlowLayout(chipsHost_, 0, 4, 4);
    suffixTextLabel_ = new QLabel(this);
    suffixIconLabel_ = new QLabel(this);

    clearButton_ = new QToolButton(this);
    clearButton_->setAutoRaise(true);
    clearButton_->setFocusPolicy(Qt::NoFocus);
    clearButton_->setCursor(Qt::PointingHandCursor);
    clearButton_->setStyleSheet(
        QStringLiteral("QToolButton { border: 0; padding: 0; background: transparent; }"));

    rootLayout_->addWidget(prefixIconLabel_);
    rootLayout_->addWidget(prefixTextLabel_);
    rootLayout_->addWidget(chipsHost_, 1);
    rootLayout_->addWidget(suffixTextLabel_);
    rootLayout_->addWidget(clearButton_);
    rootLayout_->addWidget(suffixIconLabel_);

    connect(clearButton_, &QToolButton::clicked, this, &DateTagInputSurface::clearRequested);
  }

  void setVisualStyle(const DatePickerVisualStyle& style,
                      bool active,
                      bool disabled,
                      bool allowClear,
                      bool popupVisible) {
    style_ = style;
    active_ = active;
    disabled_ = disabled;
    allowClear_ = allowClear;
    popupVisible_ = popupVisible;

    rootLayout_->setContentsMargins(style.metrics.horizontalPadding, style.metrics.verticalPadding,
                                    style.metrics.horizontalPadding, style.metrics.verticalPadding);
    rootLayout_->setSpacing(std::max(4, style.metrics.chipContentGap + 2));
    chipsLayout_->setHorizontalSpacing(style.metrics.chipContentGap);
    chipsLayout_->setVerticalSpacing(style.metrics.chipContentGap);

    QPalette labelPalette = palette();
    labelPalette.setColor(QPalette::WindowText, disabled ? style.disabledText : style.selectorText);
    prefixTextLabel_->setPalette(labelPalette);
    suffixTextLabel_->setPalette(labelPalette);

    if (adqt::icons::isValid(prefixIconRef_)) {
      prefixIconLabel_->setPixmap(
          renderTintedIcon(prefixIconRef_, disabled ? style.disabledText : style.iconColor,
                           style.metrics.iconSize, devicePixelRatioF()));
    }
    if (adqt::icons::isValid(suffixIconRef_)) {
      suffixIconLabel_->setPixmap(
          renderTintedIcon(suffixIconRef_, disabled ? style.disabledText : style.iconColor,
                           style.metrics.iconSize, devicePixelRatioF()));
    }
    applyToolButtonIcon(clearButton_, outlined_icons::CloseCircle(),
                        disabled ? style.disabledText : style.clearColor, style.metrics.iconSize,
                        devicePixelRatioF());
    rebuildChips();
    update();
  }

  void setPlaceholder(const QString& value) {
    placeholder_ = value;
    rebuildChips();
  }

  void setPrefixText(const QString& value) {
    prefixText_ = value;
    prefixTextLabel_->setText(value);
    prefixTextLabel_->setVisible(!value.isEmpty());
  }

  void setSuffixText(const QString& value) {
    suffixText_ = value;
    suffixTextLabel_->setText(value);
    suffixTextLabel_->setVisible(!value.isEmpty());
  }

  void setPrefixIconRef(const adqt::icons::IconRef& value) {
    prefixIconRef_ = value;
    prefixIconLabel_->setVisible(adqt::icons::isValid(value));
  }

  void setSuffixIconRef(const adqt::icons::IconRef& value) {
    suffixIconRef_ = value;
    suffixIconLabel_->setVisible(adqt::icons::isValid(value));
  }

  void setChipData(const QStringList& labels, const QVector<int>& sourceIndexes) {
    labels_ = labels;
    sourceIndexes_ = sourceIndexes;
    rebuildChips();
  }

  void setMaxVisibleTags(int value) {
    maxVisibleTags_ = value;
    rebuildChips();
  }

  void setResponsiveMaxTagCount(bool value) {
    responsiveMaxTagCount_ = value;
    rebuildChips();
  }

 signals:
  void clearRequested();
  void removeChipRequested(int sourceIndex);
  void activated();

 protected:
  void mousePressEvent(QMouseEvent* event) override {
    if (event && !disabled_ && event->button() == Qt::LeftButton) {
      setFocus(Qt::MouseFocusReason);
      emit activated();
    }
    QWidget::mousePressEvent(event);
  }

  void keyPressEvent(QKeyEvent* event) override {
    if (!event) {
      return;
    }
    if (!disabled_ &&
        (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return ||
         event->key() == Qt::Key_Enter || event->key() == Qt::Key_Down)) {
      emit activated();
      event->accept();
      return;
    }
    if (!disabled_ && event->key() == Qt::Key_Backspace && !sourceIndexes_.isEmpty()) {
      emit removeChipRequested(sourceIndexes_.last());
      event->accept();
      return;
    }
    QWidget::keyPressEvent(event);
  }

  void enterEvent(QEnterEvent* event) override {
    hovered_ = true;
    update();
    QWidget::enterEvent(event);
  }

  void leaveEvent(QEvent* event) override {
    hovered_ = false;
    update();
    QWidget::leaveEvent(event);
  }

  void focusInEvent(QFocusEvent* event) override {
    focused_ = true;
    update();
    QWidget::focusInEvent(event);
  }

  void focusOutEvent(QFocusEvent* event) override {
    focused_ = false;
    update();
    QWidget::focusOutEvent(event);
  }

  void resizeEvent(QResizeEvent* event) override {
    QWidget::resizeEvent(event);
    if (responsiveMaxTagCount_) {
      rebuildChips();
    }
  }

  void paintEvent(QPaintEvent* event) override {
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QColor border = style_.selectorBorder;
    QColor fill = style_.selectorBg;
    if (disabled_) {
      border = style_.disabledBorder;
      fill = style_.disabledBg;
    } else if (popupVisible_ || focused_ || active_) {
      border = style_.selectorActiveBorder;
      fill = style_.selectorActiveBg;
    } else if (hovered_) {
      border = style_.selectorHoverBorder;
      fill = style_.selectorHoverBg;
    }

    const QRectF frame = rect().adjusted(0.5, 0.5, -0.5, -0.5);
    painter.setPen(Qt::NoPen);
    painter.setBrush(fill);
    painter.drawRoundedRect(frame, style_.metrics.borderRadius, style_.metrics.borderRadius);

    if (style_.underlined) {
      painter.setPen(QPen(border, std::max(1, style_.metrics.borderWidth)));
      painter.drawLine(frame.bottomLeft(), frame.bottomRight());
    } else {
      painter.setPen(QPen(border, std::max(1, style_.metrics.borderWidth)));
      painter.setBrush(Qt::NoBrush);
      painter.drawRoundedRect(frame, style_.metrics.borderRadius, style_.metrics.borderRadius);
    }

    if (popupVisible_ || focused_ || active_) {
      const QRectF outline = frame.adjusted(-style_.metrics.focusOutlineWidth / 3.0,
                                            -style_.metrics.focusOutlineWidth / 3.0,
                                            style_.metrics.focusOutlineWidth / 3.0,
                                            style_.metrics.focusOutlineWidth / 3.0);
      painter.setPen(QPen(style_.selectorFocusOutline, style_.metrics.focusOutlineWidth));
      painter.setBrush(Qt::NoBrush);
      painter.drawRoundedRect(outline, style_.metrics.borderRadius + 1,
                              style_.metrics.borderRadius + 1);
    }
  }

 private:
  int responsiveVisibleCount() const {
    if (!responsiveMaxTagCount_ || labels_.isEmpty()) {
      return labels_.size();
    }
    const int availableWidth = std::max(80, chipsHost_->width() - 8);
    const QFontMetrics fm(font());
    int used = 0;
    int count = 0;
    for (int i = 0; i < labels_.size(); ++i) {
      const int chipWidth = fm.horizontalAdvance(labels_.at(i)) + style_.metrics.chipPaddingStart +
                            style_.metrics.chipPaddingEnd + style_.metrics.iconSize + 24;
      if (count > 0 && used + chipWidth > availableWidth) {
        break;
      }
      used += chipWidth + style_.metrics.chipContentGap;
      ++count;
    }
    return std::max(1, count);
  }

  void clearChipWidgets() {
    while (QLayoutItem* item = chipsLayout_->takeAt(0)) {
      if (QWidget* widget = item->widget()) {
        delete widget;
      }
      delete item;
    }
  }

  void rebuildChips() {
    clearChipWidgets();

    const bool hasValues = !labels_.isEmpty();
    int visibleCount = labels_.size();
    if (maxVisibleTags_ >= 0) {
      visibleCount = std::min(visibleCount, maxVisibleTags_);
    }
    visibleCount = std::min(visibleCount, responsiveVisibleCount());
    const int hiddenCount = std::max(0, static_cast<int>(labels_.size()) - visibleCount);

    if (!hasValues) {
      auto* placeholderLabel = new QLabel(placeholder_, chipsHost_);
      QPalette palette = placeholderLabel->palette();
      palette.setColor(QPalette::WindowText, style_.placeholderText);
      placeholderLabel->setPalette(palette);
      chipsLayout_->addWidget(placeholderLabel);
    } else {
      for (int i = 0; i < visibleCount; ++i) {
        auto* chip = new DateChipWidget(chipsHost_);
        chip->setLabelText(labels_.at(i));
        chip->setRemovable(!disabled_ && i < sourceIndexes_.size());
        chip->setVisualStyle(style_);
        if (!disabled_ && i < sourceIndexes_.size()) {
          connect(chip, &DateChipWidget::removeRequested, this, [this, i]() {
            if (i >= 0 && i < sourceIndexes_.size()) {
              emit removeChipRequested(sourceIndexes_.at(i));
            }
          });
        }
        chipsLayout_->addWidget(chip);
      }
      if (hiddenCount > 0) {
        auto* summary = new DateChipWidget(chipsHost_);
        summary->setLabelText(QStringLiteral("+ %1 ...").arg(hiddenCount));
        summary->setRemovable(false);
        summary->setVisualStyle(style_);
        chipsLayout_->addWidget(summary);
      }
    }

    clearButton_->setVisible(!disabled_ && allowClear_ && hasValues && (hovered_ || focused_));
    suffixIconLabel_->setVisible(adqt::icons::isValid(suffixIconRef_) && !clearButton_->isVisible());
    updateGeometry();
  }

  QHBoxLayout* rootLayout_ = nullptr;
  QLabel* prefixIconLabel_ = nullptr;
  QLabel* prefixTextLabel_ = nullptr;
  QWidget* chipsHost_ = nullptr;
  detail::FlowLayout* chipsLayout_ = nullptr;
  QLabel* suffixTextLabel_ = nullptr;
  QToolButton* clearButton_ = nullptr;
  QLabel* suffixIconLabel_ = nullptr;
  DatePickerVisualStyle style_;
  QString placeholder_;
  QString prefixText_;
  QString suffixText_;
  adqt::icons::IconRef prefixIconRef_;
  adqt::icons::IconRef suffixIconRef_;
  QStringList labels_;
  QVector<int> sourceIndexes_;
  int maxVisibleTags_ = -1;
  bool responsiveMaxTagCount_ = false;
  bool allowClear_ = true;
  bool disabled_ = false;
  bool hovered_ = false;
  bool focused_ = false;
  bool active_ = false;
  bool popupVisible_ = false;
};



class CalendarPaneWidget final : public QWidget {
  Q_OBJECT

 public:
  explicit CalendarPaneWidget(QWidget* parent = nullptr) : QWidget(parent) {
    rootLayout_ = new QVBoxLayout(this);
    rootLayout_->setContentsMargins(0, 0, 0, 0);
    rootLayout_->setSpacing(8);

    headerLayout_ = new QHBoxLayout();
    headerLayout_->setContentsMargins(0, 0, 0, 0);
    headerLayout_->setSpacing(4);

    prevButton_ = new QToolButton(this);
    prevButton_->setAutoRaise(true);
    prevButton_->setFocusPolicy(Qt::NoFocus);
    nextButton_ = new QToolButton(this);
    nextButton_->setAutoRaise(true);
    nextButton_->setFocusPolicy(Qt::NoFocus);
    titleLabel_ = new QLabel(this);
    titleLabel_->setAlignment(Qt::AlignCenter);

    headerLayout_->addWidget(prevButton_);
    headerLayout_->addStretch();
    headerLayout_->addWidget(titleLabel_);
    headerLayout_->addStretch();
    headerLayout_->addWidget(nextButton_);
    rootLayout_->addLayout(headerLayout_);

    weekdayHost_ = new QWidget(this);
    weekdayLayout_ = new QGridLayout(weekdayHost_);
    weekdayLayout_->setContentsMargins(0, 0, 0, 0);
    weekdayLayout_->setHorizontalSpacing(0);
    weekdayLayout_->setVerticalSpacing(0);
    for (int i = 0; i < kDayColumns; ++i) {
      auto* label = new QLabel(weekdayHost_);
      label->setAlignment(Qt::AlignCenter);
      weekdayLabels_.append(label);
      weekdayLayout_->addWidget(label, 0, i);
    }
    rootLayout_->addWidget(weekdayHost_);

    gridHost_ = new QWidget(this);
    gridLayout_ = new QGridLayout(gridHost_);
    gridLayout_->setContentsMargins(0, 0, 0, 0);
    gridLayout_->setHorizontalSpacing(0);
    gridLayout_->setVerticalSpacing(0);
    rootLayout_->addWidget(gridHost_);

    timeEditor_ = new SimpleTimeEditor(this);
    timeEditor_->hide();
    rootLayout_->addWidget(timeEditor_);

    connect(prevButton_, &QToolButton::clicked, this, [this]() { emit navigateRequested(-1); });
    connect(nextButton_, &QToolButton::clicked, this, [this]() { emit navigateRequested(1); });
    connect(timeEditor_, &SimpleTimeEditor::timeChanged, this, &CalendarPaneWidget::timeChanged);
  }

  void setVisualStyle(const DatePickerVisualStyle& style) {
    style_ = style;
    QPalette titlePalette = titleLabel_->palette();
    titlePalette.setColor(QPalette::WindowText, style.titleText);
    titleLabel_->setPalette(titlePalette);
    for (QLabel* label : weekdayLabels_) {
      if (!label) {
        continue;
      }
      QPalette palette = label->palette();
      palette.setColor(QPalette::WindowText, style.mutedText);
      label->setPalette(palette);
    }
    applyToolButtonIcon(prevButton_, outlined_icons::Left(), style.iconColor, style.metrics.iconSize,
                        devicePixelRatioF());
    applyToolButtonIcon(nextButton_, outlined_icons::Right(), style.iconColor, style.metrics.iconSize,
                        devicePixelRatioF());
    timeEditor_->setVisualStyle(style);
    refresh();
  }

  void setPickerMode(AdDatePicker::PickerMode value) {
    pickerMode_ = value;
    refresh();
  }

  void setDisplayDate(const QDate& value) {
    displayDate_ = value.isValid() ? value : QDate::currentDate();
    refresh();
  }

  void setSelectionMode(AdDatePicker::SelectionMode value) {
    selectionMode_ = value;
    refresh();
  }

  void setSelectedValues(const QVector<QDateTime>& value) {
    selectedValues_ = value;
    refresh();
  }

  void setRangeValue(const AdDateTimeRangeValue& value) {
    rangeValue_ = value;
    refresh();
  }

  void setRangeMode(bool value) {
    rangeMode_ = value;
    refresh();
  }

  void setMinDate(const QDate& value) {
    minDate_ = value;
    refresh();
  }

  void setMaxDate(const QDate& value) {
    maxDate_ = value;
    refresh();
  }

  void setLocale(const QLocale& value) {
    locale_ = value;
    refresh();
  }

  void setDisabledDateEvaluator(AdDatePicker::DisabledDateEvaluator evaluator) {
    disabledDateEvaluator_ = std::move(evaluator);
    refresh();
  }

  void setNavigationVisible(bool prevVisible, bool nextVisible) {
    prevButton_->setVisible(prevVisible);
    nextButton_->setVisible(nextVisible);
  }

  void setTimeVisible(bool value) {
    timeEditor_->setVisible(value);
  }

  void setTimePanelOptions(const AdDateTimePanelOptions& options) {
    timeEditor_->setOptions(options);
  }

  void setTime(const QTime& time) {
    timeEditor_->setTime(time);
  }

  void setDisabledTimeSpec(const AdDisabledTimeSpec& spec) {
    timeEditor_->setDisabledSpec(spec);
  }

  QTime time() const { return timeEditor_->time(); }

 signals:
  void navigateRequested(int step);
  void dateActivated(const QDate& value);
  void timeChanged(const QTime& value);

 private:
  void ensureGrid(int rows, int columns) {
    if (rows == gridRows_ && columns == gridColumns_) {
      return;
    }
    while (QLayoutItem* item = gridLayout_->takeAt(0)) {
      if (QWidget* widget = item->widget()) {
        delete widget;
      }
      delete item;
    }
    cellButtons_.clear();
    gridRows_ = rows;
    gridColumns_ = columns;
    const int count = rows * columns;
    cellButtons_.reserve(count);
    for (int index = 0; index < count; ++index) {
      auto* button = new DateCellButton(gridHost_);
      cellButtons_.append(button);
      connect(button, &QAbstractButton::clicked, this, [this, button]() {
        emit dateActivated(button->cellDate());
      });
      gridLayout_->addWidget(button, index / columns, index % columns);
    }
  }

  bool cellDisabled(const QDate& date) const {
    return isDateBlocked(date, minDate_, maxDate_, disabledDateEvaluator_);
  }

  bool cellSelected(const QDate& date) const {
    if (selectionMode_ == AdDatePicker::SelectionMode::Multiple) {
      for (const QDateTime& value : selectedValues_) {
        if (sameCanonicalValue(date, value.date(), pickerMode_, locale_)) {
          return true;
        }
      }
      return false;
    }
    return !selectedValues_.isEmpty() &&
           sameCanonicalValue(date, selectedValues_.constFirst().date(), pickerMode_, locale_);
  }

  bool cellInRange(const QDate& date) const {
    if (!rangeMode_ || !rangeValue_.start.isValid() || !rangeValue_.end.isValid()) {
      return false;
    }
    const QDate candidateStart = pickerMode_ == AdDatePicker::PickerMode::Date ||
                                         pickerMode_ == AdDatePicker::PickerMode::Week
                                     ? date
                                     : intervalStartForDate(date, pickerMode_, locale_);
    const QDate candidateEnd = pickerMode_ == AdDatePicker::PickerMode::Date ||
                                       pickerMode_ == AdDatePicker::PickerMode::Week
                                   ? date
                                   : intervalEndForDate(date, pickerMode_, locale_);
    const QDate rangeStart = intervalStartForDate(rangeValue_.start.date(), pickerMode_, locale_);
    const QDate rangeEnd = intervalEndForDate(rangeValue_.end.date(), pickerMode_, locale_);
    return dateIntervalsOverlap(candidateStart, candidateEnd, rangeStart, rangeEnd);
  }

  bool cellIsRangeStart(const QDate& date) const {
    return rangeMode_ && rangeValue_.start.isValid() &&
           sameCanonicalValue(date, rangeValue_.start.date(), pickerMode_, locale_);
  }

  bool cellIsRangeEnd(const QDate& date) const {
    return rangeMode_ && rangeValue_.end.isValid() &&
           sameCanonicalValue(date, rangeValue_.end.date(), pickerMode_, locale_);
  }

  void refreshWeekdayLabels() {
    const int firstDay = static_cast<int>(localeFirstDay(locale_));
    for (int column = 0; column < weekdayLabels_.size(); ++column) {
      const int day = ((firstDay - 1 + column) % 7) + 1;
      weekdayLabels_.at(column)->setText(locale_.dayName(day, QLocale::ShortFormat));
    }
  }

  void refresh() {
    titleLabel_->setText(panelTitleText(displayDate_, pickerMode_, locale_));
    refreshWeekdayLabels();

    if (pickerMode_ == AdDatePicker::PickerMode::Date ||
        pickerMode_ == AdDatePicker::PickerMode::Week) {
      weekdayHost_->setVisible(true);
      ensureGrid(kDayRows, kDayColumns);
      const QDate monthStart(displayDate_.year(), displayDate_.month(), 1);
      const QDate gridStart = startOfWeek(monthStart, locale_);
      for (int index = 0; index < cellButtons_.size(); ++index) {
        const QDate date = gridStart.addDays(index);
        DateCellButton* button = cellButtons_.at(index);
        button->setCellDate(date);
        button->setTextValue(QString::number(date.day()));
        button->setStyleData(style_, cellDisabled(date), date.month() != displayDate_.month(),
                             date == QDate::currentDate(), cellSelected(date), cellInRange(date),
                             cellIsRangeStart(date), cellIsRangeEnd(date));
      }
      return;
    }

    weekdayHost_->setVisible(false);
    if (pickerMode_ == AdDatePicker::PickerMode::Month) {
      ensureGrid(3, 4);
      for (int month = 1; month <= 12; ++month) {
        const QDate date(displayDate_.year(), month, 1);
        DateCellButton* button = cellButtons_.at(month - 1);
        button->setCellDate(date);
        button->setTextValue(monthShortName(locale_, month));
        button->setStyleData(style_, cellDisabled(date), false,
                             sameCanonicalValue(date, QDate::currentDate(), pickerMode_, locale_),
                             cellSelected(date), cellInRange(date), cellIsRangeStart(date),
                             cellIsRangeEnd(date));
      }
      return;
    }

    if (pickerMode_ == AdDatePicker::PickerMode::Quarter) {
      ensureGrid(2, 2);
      for (int quarter = 0; quarter < 4; ++quarter) {
        const QDate date(displayDate_.year(), quarter * 3 + 1, 1);
        DateCellButton* button = cellButtons_.at(quarter);
        button->setCellDate(date);
        button->setTextValue(QStringLiteral("Q%1").arg(quarter + 1));
        button->setStyleData(style_, cellDisabled(date), false,
                             sameCanonicalValue(date, QDate::currentDate(), pickerMode_, locale_),
                             cellSelected(date), cellInRange(date), cellIsRangeStart(date),
                             cellIsRangeEnd(date));
      }
      return;
    }

    ensureGrid(3, 4);
    const int decadeStart = (displayDate_.year() / 10) * 10;
    const int firstYear = decadeStart - 1;
    for (int offset = 0; offset < 12; ++offset) {
      const int year = firstYear + offset;
      const QDate date(year, 1, 1);
      DateCellButton* button = cellButtons_.at(offset);
      button->setCellDate(date);
      button->setTextValue(QString::number(year));
      button->setStyleData(style_, cellDisabled(date), year < decadeStart || year > decadeStart + 9,
                           sameCanonicalValue(date, QDate::currentDate(), pickerMode_, locale_),
                           cellSelected(date), cellInRange(date), cellIsRangeStart(date),
                           cellIsRangeEnd(date));
    }
  }

  QVBoxLayout* rootLayout_ = nullptr;
  QHBoxLayout* headerLayout_ = nullptr;
  QToolButton* prevButton_ = nullptr;
  QToolButton* nextButton_ = nullptr;
  QLabel* titleLabel_ = nullptr;
  QWidget* weekdayHost_ = nullptr;
  QGridLayout* weekdayLayout_ = nullptr;
  QVector<QLabel*> weekdayLabels_;
  QWidget* gridHost_ = nullptr;
  QGridLayout* gridLayout_ = nullptr;
  QVector<DateCellButton*> cellButtons_;
  SimpleTimeEditor* timeEditor_ = nullptr;
  DatePickerVisualStyle style_;
  AdDatePicker::PickerMode pickerMode_ = AdDatePicker::PickerMode::Date;
  AdDatePicker::SelectionMode selectionMode_ = AdDatePicker::SelectionMode::Single;
  bool rangeMode_ = false;
  QDate displayDate_ = QDate::currentDate();
  QDate minDate_;
  QDate maxDate_;
  QLocale locale_;
  AdDatePicker::DisabledDateEvaluator disabledDateEvaluator_;
  QVector<QDateTime> selectedValues_;
  AdDateTimeRangeValue rangeValue_;
  int gridRows_ = 0;
  int gridColumns_ = 0;
};

class DatePickerPanelWidget final : public QWidget {
  Q_OBJECT

 public:
  explicit DatePickerPanelWidget(bool rangeMode, QWidget* parent = nullptr)
      : QWidget(parent), rangeMode_(rangeMode) {
    outerLayout_ = new QVBoxLayout(this);
    outerLayout_->setContentsMargins(12, 12, 12, 12);
    outerLayout_->setSpacing(8);

    bodyLayout_ = new QHBoxLayout();
    bodyLayout_->setContentsMargins(0, 0, 0, 0);
    bodyLayout_->setSpacing(12);

    presetList_ = new QListWidget(this);
    presetList_->setFrameShape(QFrame::NoFrame);
    presetList_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    presetList_->setSelectionMode(QAbstractItemView::SingleSelection);
    presetList_->hide();

    railDivider_ = new QFrame(this);
    railDivider_->setFrameShape(QFrame::VLine);
    railDivider_->hide();

    panesHost_ = new QWidget(this);
    panesLayout_ = new QHBoxLayout(panesHost_);
    panesLayout_->setContentsMargins(0, 0, 0, 0);
    panesLayout_->setSpacing(12);

    firstPane_ = new CalendarPaneWidget(panesHost_);
    panesLayout_->addWidget(firstPane_, 1);
    if (rangeMode_) {
      secondPane_ = new CalendarPaneWidget(panesHost_);
      panesLayout_->addWidget(secondPane_, 1);
    }

    bodyLayout_->addWidget(presetList_);
    bodyLayout_->addWidget(railDivider_);
    bodyLayout_->addWidget(panesHost_, 1);
    outerLayout_->addLayout(bodyLayout_);

    footerDivider_ = new QFrame(this);
    footerDivider_->setFrameShape(QFrame::HLine);
    footerDivider_->hide();
    outerLayout_->addWidget(footerDivider_);

    footerHost_ = new QWidget(this);
    footerLayout_ = new QVBoxLayout(footerHost_);
    footerLayout_->setContentsMargins(0, 0, 0, 0);
    footerLayout_->setSpacing(8);

    rangeTimeRow_ = new QWidget(footerHost_);
    rangeTimeLayout_ = new QHBoxLayout(rangeTimeRow_);
    rangeTimeLayout_->setContentsMargins(0, 0, 0, 0);
    rangeTimeLayout_->setSpacing(8);
    startTimeCaption_ = new QLabel(QStringLiteral("Start time"), rangeTimeRow_);
    endTimeCaption_ = new QLabel(QStringLiteral("End time"), rangeTimeRow_);
    startTimeEditor_ = new SimpleTimeEditor(rangeTimeRow_);
    endTimeEditor_ = new SimpleTimeEditor(rangeTimeRow_);
    rangeTimeLayout_->addWidget(startTimeCaption_);
    rangeTimeLayout_->addWidget(startTimeEditor_);
    rangeTimeLayout_->addSpacing(8);
    rangeTimeLayout_->addWidget(endTimeCaption_);
    rangeTimeLayout_->addWidget(endTimeEditor_);
    rangeTimeLayout_->addStretch();
    footerLayout_->addWidget(rangeTimeRow_);

    footerBottomRow_ = new QHBoxLayout();
    footerBottomRow_->setContentsMargins(0, 0, 0, 0);
    footerBottomRow_->setSpacing(8);
    extraFooterLabel_ = new QLabel(footerHost_);
    footerBottomRow_->addWidget(extraFooterLabel_);
    footerBottomRow_->addStretch();
    confirmButton_ = new AdButton(QStringLiteral("OK"), footerHost_);
    confirmButton_->setButtonStyle(AdButton::ButtonStyle::Solid);
    confirmButton_->setAccentRole(AdButton::AccentRole::Primary);
    footerBottomRow_->addWidget(confirmButton_);
    footerLayout_->addLayout(footerBottomRow_);

    outerLayout_->addWidget(footerHost_);

    connect(firstPane_, &CalendarPaneWidget::navigateRequested, this,
            &DatePickerPanelWidget::handleFirstNavigate);
    connect(firstPane_, &CalendarPaneWidget::dateActivated, this,
            &DatePickerPanelWidget::handleDateActivated);
    connect(firstPane_, &CalendarPaneWidget::timeChanged, this,
            &DatePickerPanelWidget::handleSingleTimeChanged);

    if (secondPane_) {
      connect(secondPane_, &CalendarPaneWidget::navigateRequested, this,
              &DatePickerPanelWidget::handleSecondNavigate);
      connect(secondPane_, &CalendarPaneWidget::dateActivated, this,
              &DatePickerPanelWidget::handleDateActivated);
    }

    connect(startTimeEditor_, &SimpleTimeEditor::timeChanged, this,
            &DatePickerPanelWidget::handleStartTimeChanged);
    connect(endTimeEditor_, &SimpleTimeEditor::timeChanged, this,
            &DatePickerPanelWidget::handleEndTimeChanged);
    connect(confirmButton_, &QAbstractButton::clicked, this, &DatePickerPanelWidget::handleConfirm);
    connect(presetList_, &QListWidget::itemClicked, this, &DatePickerPanelWidget::handlePresetClicked);

    refreshUi();
  }

  void setControlSize(AdDatePicker::ControlSize value) {
    controlSize_ = value;
    confirmButton_->setSizeClass(toButtonSize(value));
  }

  void setVisualStyle(const DatePickerVisualStyle& style) {
    style_ = style;
    outerLayout_->setContentsMargins(style.metrics.popupPadding, style.metrics.popupPadding,
                                     style.metrics.popupPadding, style.metrics.popupPadding);
    outerLayout_->setSpacing(style.metrics.footerGap);
    firstPane_->setVisualStyle(style);
    if (secondPane_) {
      secondPane_->setVisualStyle(style);
    }
    startTimeEditor_->setVisualStyle(style);
    endTimeEditor_->setVisualStyle(style);

    const QString dividerCss =
        QStringLiteral("QFrame { color: %1; background: %1; }").arg(style.divider.name());
    railDivider_->setStyleSheet(dividerCss);
    footerDivider_->setStyleSheet(dividerCss);

    QPalette palette = extraFooterLabel_->palette();
    palette.setColor(QPalette::WindowText, style.footerText);
    extraFooterLabel_->setPalette(palette);
    palette = startTimeCaption_->palette();
    palette.setColor(QPalette::WindowText, style.mutedText);
    startTimeCaption_->setPalette(palette);
    endTimeCaption_->setPalette(palette);

    const QString listCss = QStringLiteral(
                                "QListWidget { border: 0; background: transparent; color: %1; } "
                                "QListWidget::item { padding: 6px 8px; border-radius: %2px; } "
                                "QListWidget::item:selected { background: %3; color: %4; } "
                                "QListWidget::item:hover { background: %3; }")
                                .arg(style.presetText.name())
                                .arg(style.metrics.cellRadius)
                                .arg(style.presetHoverBg.name(QColor::HexArgb))
                                .arg(style.selectorText.name());
    presetList_->setStyleSheet(listCss);
    refreshUi();
  }

  void setPickerMode(AdDatePicker::PickerMode value) {
    pickerMode_ = value;
    refreshUi();
  }

  void setSelectionMode(AdDatePicker::SelectionMode value) {
    selectionMode_ = value;
    refreshUi();
  }

  void setShowTime(bool value) {
    showTime_ = value;
    refreshUi();
  }

  void setNeedConfirm(bool value) {
    needConfirm_ = value;
    refreshUi();
  }

  void setAutoSortSelections(bool value) { autoSortSelections_ = value; }

  void setTimePanelOptions(const AdDateTimePanelOptions& value) {
    timePanelOptions_ = value;
    firstPane_->setTimePanelOptions(value);
    if (secondPane_) {
      secondPane_->setTimePanelOptions(value);
    }
    startTimeEditor_->setOptions(value);
    endTimeEditor_->setOptions(value);
    refreshUi();
  }

  void setSingleValue(const QDateTime& value) {
    committedValue_ = value;
    draftValue_ = value;
    pendingSingleTime_ = value.isValid() ? canonicalTime(value.time(), timePanelOptions_)
                                         : canonicalTime(timePanelOptions_.defaultOpenTime,
                                                         timePanelOptions_);
    if (value.isValid()) {
      displayDate_ = value.date();
    }
    refreshUi();
  }

  void setMultipleValues(const QVector<QDateTime>& value) {
    committedValues_ = value;
    draftValues_ = value;
    if (!value.isEmpty()) {
      displayDate_ = value.constFirst().date();
    }
    refreshUi();
  }

  void setRangeValue(const AdDateTimeRangeValue& value) {
    committedRange_ = value;
    draftRange_ = value;
    if (value.start.isValid()) {
      displayDate_ = value.start.date();
      rangeDisplayDate_ = stepDisplayDate(displayDate_, pickerMode_, 1);
      pendingStartTime_ = canonicalTime(value.start.time(), timePanelOptions_);
    }
    if (value.end.isValid()) {
      if (!value.start.isValid()) {
        rangeDisplayDate_ = value.end.date();
        displayDate_ = stepDisplayDate(rangeDisplayDate_, pickerMode_, -1);
      }
      pendingEndTime_ = canonicalTime(value.end.time(), timePanelOptions_);
    }
    refreshUi();
  }

  void setPresets(const QVector<AdDatePresetItem>& value) {
    presets_ = value;
    refreshPresets();
  }

  void setRangePresets(const QVector<AdDateRangePresetItem>& value) {
    rangePresets_ = value;
    refreshPresets();
  }

  void setMinDate(const QDate& value) {
    minDate_ = value;
    refreshUi();
  }

  void setMaxDate(const QDate& value) {
    maxDate_ = value;
    refreshUi();
  }

  void setDisabledDateEvaluator(AdDatePicker::DisabledDateEvaluator evaluator) {
    disabledDateEvaluator_ = std::move(evaluator);
    refreshUi();
  }

  void setDisabledTimeEvaluator(AdDatePicker::DisabledTimeEvaluator evaluator) {
    disabledTimeEvaluator_ = std::move(evaluator);
    refreshUi();
  }

  void setExtraFooterText(const QString& value) {
    extraFooterText_ = value;
    extraFooterLabel_->setText(value);
    refreshFooter();
  }

  QWidget* extraFooterWidget() const { return extraFooterWidget_; }

  void setExtraFooterWidget(QWidget* widget) {
    if (extraFooterWidget_ == widget) {
      return;
    }
    if (extraFooterWidget_) {
      footerBottomRow_->removeWidget(extraFooterWidget_);
      extraFooterWidget_->setParent(nullptr);
    }
    extraFooterWidget_ = widget;
    if (extraFooterWidget_) {
      extraFooterWidget_->setParent(footerHost_);
      footerBottomRow_->insertWidget(1, extraFooterWidget_);
    }
    refreshFooter();
  }

  QWidget* takeExtraFooterWidget() {
    QWidget* widget = extraFooterWidget_;
    if (widget) {
      footerBottomRow_->removeWidget(widget);
      widget->setParent(nullptr);
      extraFooterWidget_ = nullptr;
    }
    refreshFooter();
    return widget;
  }

  void setAllowEmpty(bool startAllowed, bool endAllowed) {
    allowEmptyStart_ = startAllowed;
    allowEmptyEnd_ = endAllowed;
  }

  void setActiveRangePart(AdDatePicker::RangePart value) { activeRangePart_ = value; }

  int preferredWidth() const {
    int width = style_.metrics.popupWidth;
    if (rangeMode_) {
      width = style_.metrics.popupWidth * 2 + style_.metrics.popupPadding;
    }
    if ((rangeMode_ && !rangePresets_.isEmpty()) || (!rangeMode_ && !presets_.isEmpty())) {
      width += style_.metrics.presetRailWidth + style_.metrics.popupPadding;
    }
    return width;
  }

 public slots:
  void resetDraftFromCommitted() {
    draftValue_ = committedValue_;
    draftValues_ = committedValues_;
    draftRange_ = committedRange_;
    pendingSingleTime_ = draftValue_.isValid()
                             ? canonicalTime(draftValue_.time(), timePanelOptions_)
                             : canonicalTime(timePanelOptions_.defaultOpenTime, timePanelOptions_);
    pendingStartTime_ = draftRange_.start.isValid()
                            ? canonicalTime(draftRange_.start.time(), timePanelOptions_)
                            : canonicalTime(timePanelOptions_.defaultOpenTime, timePanelOptions_);
    pendingEndTime_ = draftRange_.end.isValid()
                          ? canonicalTime(draftRange_.end.time(), timePanelOptions_)
                          : canonicalTime(timePanelOptions_.defaultOpenTime, timePanelOptions_);
    if (rangeMode_) {
      if (committedRange_.start.isValid()) {
        displayDate_ = committedRange_.start.date();
      } else if (committedRange_.end.isValid()) {
        rangeDisplayDate_ = committedRange_.end.date();
        displayDate_ = stepDisplayDate(rangeDisplayDate_, pickerMode_, -1);
      }
      rangeDisplayDate_ = stepDisplayDate(displayDate_, pickerMode_, 1);
    } else {
      if (selectionMode_ == AdDatePicker::SelectionMode::Multiple && !draftValues_.isEmpty()) {
        displayDate_ = draftValues_.constFirst().date();
      } else if (draftValue_.isValid()) {
        displayDate_ = draftValue_.date();
      }
    }
    refreshUi();
  }

 signals:
  void singleValueCommitted(const QDateTime& value, bool closePopup);
  void multipleValuesCommitted(const QVector<QDateTime>& value);
  void rangeValueCommitted(const AdDateTimeRangeValue& value, bool closePopup);
  void closeRequested();

 private slots:
  void handleFirstNavigate(int step) {
    displayDate_ = stepDisplayDate(displayDate_, pickerMode_, step);
    if (rangeMode_) {
      rangeDisplayDate_ = stepDisplayDate(displayDate_, pickerMode_, 1);
    }
    refreshUi();
  }

  void handleSecondNavigate(int step) {
    if (!rangeMode_) {
      displayDate_ = stepDisplayDate(displayDate_, pickerMode_, step);
      refreshUi();
      return;
    }
    rangeDisplayDate_ = stepDisplayDate(rangeDisplayDate_, pickerMode_, step);
    displayDate_ = stepDisplayDate(rangeDisplayDate_, pickerMode_, -1);
    refreshUi();
  }

  void handleDateActivated(const QDate& date) {
    if (!date.isValid()) {
      return;
    }

    if (!rangeMode_) {
      if (selectionMode_ == AdDatePicker::SelectionMode::Multiple) {
        QDateTime value(date, showTime_ ? pendingSingleTime_ : QTime(0, 0, 0));
        value = canonicalDateTime(value, pickerMode_, showTime_, locale());
        if (containsDateTime(draftValues_, value)) {
          draftValues_.erase(std::remove(draftValues_.begin(), draftValues_.end(), value),
                             draftValues_.end());
        } else {
          draftValues_.append(value);
          draftValues_ =
              normalizeDateTimes(draftValues_, pickerMode_, showTime_, locale(), autoSortSelections_);
        }
        firstPane_->setSelectedValues(draftValues_);
        if (needConfirm_) {
          refreshUi();
        } else {
          emit multipleValuesCommitted(draftValues_);
          refreshUi();
        }
        return;
      }

      draftValue_ = canonicalDateTime(QDateTime(date, showTime_ ? pendingSingleTime_ : QTime(0, 0, 0)),
                                      pickerMode_, showTime_, locale());
      if (showTime_) {
        firstPane_->setTime(pendingSingleTime_);
      }
      if (needConfirm_ || showTime_) {
        refreshUi();
      } else {
        emit singleValueCommitted(draftValue_, true);
      }
      return;
    }

    const bool allowOpenStart = allowEmptyStart_;
    const QDateTime selected(date,
                             activeRangePart_ == AdDatePicker::RangePart::End ? pendingEndTime_
                                                                              : pendingStartTime_);
    if (!draftRange_.start.isValid() && !draftRange_.end.isValid()) {
      if (activeRangePart_ == AdDatePicker::RangePart::End && allowOpenStart) {
        draftRange_.end = canonicalDateTime(selected, pickerMode_, showTime_, locale());
      } else {
        draftRange_.start = canonicalDateTime(selected, pickerMode_, showTime_, locale());
        activeRangePart_ = AdDatePicker::RangePart::End;
      }
    } else if (activeRangePart_ == AdDatePicker::RangePart::End) {
      draftRange_.end = canonicalDateTime(selected, pickerMode_, showTime_, locale());
      if (!draftRange_.start.isValid() && !allowOpenStart) {
        draftRange_.start = draftRange_.end;
        draftRange_.end = QDateTime();
      }
    } else {
      draftRange_.start = canonicalDateTime(selected, pickerMode_, showTime_, locale());
      if (!draftRange_.end.isValid() && !allowEmptyEnd_) {
        activeRangePart_ = AdDatePicker::RangePart::End;
      }
    }

    if (draftRange_.start.isValid() && draftRange_.end.isValid() &&
        draftRange_.start > draftRange_.end) {
      std::swap(draftRange_.start, draftRange_.end);
    }

    if (showTime_ || needConfirm_) {
      refreshUi();
    } else if (rangeCanCommit(draftRange_, allowEmptyStart_, allowEmptyEnd_)) {
      emit rangeValueCommitted(draftRange_, true);
    } else {
      refreshUi();
    }
  }

  void handleSingleTimeChanged(const QTime& value) {
    pendingSingleTime_ = value;
    if (draftValue_.isValid()) {
      draftValue_.setTime(value);
    }
  }

  void handleStartTimeChanged(const QTime& value) {
    pendingStartTime_ = value;
    if (draftRange_.start.isValid()) {
      draftRange_.start.setTime(value);
    }
  }

  void handleEndTimeChanged(const QTime& value) {
    pendingEndTime_ = value;
    if (draftRange_.end.isValid()) {
      draftRange_.end.setTime(value);
    }
  }

  void handleConfirm() {
    if (!rangeMode_) {
      if (selectionMode_ == AdDatePicker::SelectionMode::Multiple) {
        emit multipleValuesCommitted(draftValues_);
      } else if (draftValue_.isValid()) {
        emit singleValueCommitted(draftValue_, true);
      }
      return;
    }
    if (rangeCanCommit(draftRange_, allowEmptyStart_, allowEmptyEnd_)) {
      emit rangeValueCommitted(draftRange_, true);
    }
  }

  void handlePresetClicked(QListWidgetItem* item) {
    if (!item) {
      return;
    }
    const int index = item->data(Qt::UserRole).toInt();
    if (!rangeMode_) {
      if (index < 0 || index >= presets_.size()) {
        return;
      }
      draftValue_ = canonicalDateTime(presets_.at(index).value, pickerMode_, showTime_, locale());
      pendingSingleTime_ = draftValue_.isValid() ? draftValue_.time() : pendingSingleTime_;
      if (needConfirm_ || showTime_) {
        refreshUi();
      } else {
        emit singleValueCommitted(draftValue_, true);
      }
      return;
    }

    if (index < 0 || index >= rangePresets_.size()) {
      return;
    }
    draftRange_ = rangePresets_.at(index).value;
    if (draftRange_.start.isValid()) {
      draftRange_.start =
          canonicalDateTime(draftRange_.start, pickerMode_, showTime_, locale());
      pendingStartTime_ = draftRange_.start.time();
    }
    if (draftRange_.end.isValid()) {
      draftRange_.end = canonicalDateTime(draftRange_.end, pickerMode_, showTime_, locale());
      pendingEndTime_ = draftRange_.end.time();
    }
    if (needConfirm_ || showTime_) {
      refreshUi();
    } else {
      emit rangeValueCommitted(draftRange_, true);
    }
  }

 private:
  void refreshPresets() {
    presetList_->clear();
    if (!rangeMode_) {
      for (int i = 0; i < presets_.size(); ++i) {
        auto* item = new QListWidgetItem(presets_.at(i).label, presetList_);
        item->setData(Qt::UserRole, i);
      }
    } else {
      for (int i = 0; i < rangePresets_.size(); ++i) {
        auto* item = new QListWidgetItem(rangePresets_.at(i).label, presetList_);
        item->setData(Qt::UserRole, i);
      }
    }
    const bool showRail =
        (!rangeMode_ && !presets_.isEmpty()) || (rangeMode_ && !rangePresets_.isEmpty());
    presetList_->setVisible(showRail);
    presetList_->setFixedWidth(style_.metrics.presetRailWidth);
    railDivider_->setVisible(showRail);
  }

  void refreshFooter() {
    const bool showRangeTime = rangeMode_ && showTime_;
    const bool showConfirm = needConfirm_ || showTime_;
    const bool showFooterBottom =
        showConfirm || !extraFooterText_.isEmpty() || extraFooterWidget_ != nullptr;
    rangeTimeRow_->setVisible(showRangeTime);
    extraFooterLabel_->setVisible(!extraFooterText_.isEmpty());
    confirmButton_->setVisible(showConfirm);
    footerDivider_->setVisible(showRangeTime || showFooterBottom);
    footerHost_->setVisible(showRangeTime || showFooterBottom);
  }

  void refreshUi() {
    refreshPresets();
    refreshFooter();

    firstPane_->setPickerMode(pickerMode_);
    firstPane_->setSelectionMode(selectionMode_);
    firstPane_->setRangeMode(rangeMode_);
    firstPane_->setDisplayDate(displayDate_.isValid() ? displayDate_ : QDate::currentDate());
    firstPane_->setMinDate(minDate_);
    firstPane_->setMaxDate(maxDate_);
    firstPane_->setDisabledDateEvaluator(disabledDateEvaluator_);
    firstPane_->setNavigationVisible(true, !rangeMode_);
    firstPane_->setTimeVisible(!rangeMode_ && showTime_ &&
                               selectionMode_ == AdDatePicker::SelectionMode::Single);

    if (!rangeMode_) {
      firstPane_->setSelectedValues(selectionMode_ == AdDatePicker::SelectionMode::Multiple
                                        ? draftValues_
                                        : QVector<QDateTime>{draftValue_});
      firstPane_->setRangeValue({});
      firstPane_->setTimePanelOptions(timePanelOptions_);
      firstPane_->setTime(pendingSingleTime_);
      const QDate disableDate = draftValue_.isValid() ? draftValue_.date()
                                                      : (displayDate_.isValid() ? displayDate_
                                                                                : QDate::currentDate());
      firstPane_->setDisabledTimeSpec(
          evaluateDisabledTime(disabledTimeEvaluator_, disableDate, AdDatePicker::RangePart::Single,
                               std::nullopt));
    } else {
      firstPane_->setSelectedValues({});
      firstPane_->setRangeValue(draftRange_);
      firstPane_->setTimeVisible(false);
      if (secondPane_) {
        secondPane_->setPickerMode(pickerMode_);
        secondPane_->setSelectionMode(AdDatePicker::SelectionMode::Single);
        secondPane_->setRangeMode(true);
        secondPane_->setDisplayDate(rangeDisplayDate_.isValid() ? rangeDisplayDate_
                                                                : stepDisplayDate(displayDate_, pickerMode_, 1));
        secondPane_->setMinDate(minDate_);
        secondPane_->setMaxDate(maxDate_);
        secondPane_->setDisabledDateEvaluator(disabledDateEvaluator_);
        secondPane_->setSelectedValues({});
        secondPane_->setRangeValue(draftRange_);
        secondPane_->setNavigationVisible(false, true);
        secondPane_->setTimeVisible(false);
      }

      startTimeEditor_->setTime(pendingStartTime_);
      endTimeEditor_->setTime(pendingEndTime_);
      startTimeEditor_->setDisabledSpec(
          evaluateDisabledTime(disabledTimeEvaluator_,
                               draftRange_.start.isValid() ? draftRange_.start.date() : displayDate_,
                               AdDatePicker::RangePart::Start,
                               draftRange_.end.isValid() ? std::optional<QDate>(draftRange_.end.date())
                                                         : std::nullopt));
      endTimeEditor_->setDisabledSpec(
          evaluateDisabledTime(disabledTimeEvaluator_,
                               draftRange_.end.isValid() ? draftRange_.end.date() : rangeDisplayDate_,
                               AdDatePicker::RangePart::End,
                               draftRange_.start.isValid() ? std::optional<QDate>(draftRange_.start.date())
                                                           : std::nullopt));
    }

    updateGeometry();
  }

  QVBoxLayout* outerLayout_ = nullptr;
  QHBoxLayout* bodyLayout_ = nullptr;
  QListWidget* presetList_ = nullptr;
  QFrame* railDivider_ = nullptr;
  QWidget* panesHost_ = nullptr;
  QHBoxLayout* panesLayout_ = nullptr;
  CalendarPaneWidget* firstPane_ = nullptr;
  CalendarPaneWidget* secondPane_ = nullptr;
  QFrame* footerDivider_ = nullptr;
  QWidget* footerHost_ = nullptr;
  QVBoxLayout* footerLayout_ = nullptr;
  QWidget* rangeTimeRow_ = nullptr;
  QHBoxLayout* rangeTimeLayout_ = nullptr;
  QLabel* startTimeCaption_ = nullptr;
  QLabel* endTimeCaption_ = nullptr;
  SimpleTimeEditor* startTimeEditor_ = nullptr;
  SimpleTimeEditor* endTimeEditor_ = nullptr;
  QHBoxLayout* footerBottomRow_ = nullptr;
  QLabel* extraFooterLabel_ = nullptr;
  AdButton* confirmButton_ = nullptr;
  QWidget* extraFooterWidget_ = nullptr;

  bool rangeMode_ = false;
  AdDatePicker::PickerMode pickerMode_ = AdDatePicker::PickerMode::Date;
  AdDatePicker::SelectionMode selectionMode_ = AdDatePicker::SelectionMode::Single;
  AdDatePicker::ControlSize controlSize_ = AdDatePicker::ControlSize::Middle;
  bool showTime_ = false;
  bool needConfirm_ = false;
  bool autoSortSelections_ = true;
  bool allowEmptyStart_ = false;
  bool allowEmptyEnd_ = false;
  AdDatePicker::RangePart activeRangePart_ = AdDatePicker::RangePart::Start;
  AdDateTimePanelOptions timePanelOptions_;
  DatePickerVisualStyle style_;
  QDate displayDate_ = QDate::currentDate();
  QDate rangeDisplayDate_ =
      stepDisplayDate(QDate::currentDate(), AdDatePicker::PickerMode::Date, 1);
  QDate minDate_;
  QDate maxDate_;
  AdDatePicker::DisabledDateEvaluator disabledDateEvaluator_;
  AdDatePicker::DisabledTimeEvaluator disabledTimeEvaluator_;
  QVector<AdDatePresetItem> presets_;
  QVector<AdDateRangePresetItem> rangePresets_;
  QString extraFooterText_;

  QDateTime committedValue_;
  QVector<QDateTime> committedValues_;
  AdDateTimeRangeValue committedRange_;
  QDateTime draftValue_;
  QVector<QDateTime> draftValues_;
  AdDateTimeRangeValue draftRange_;
  QTime pendingSingleTime_ = QTime(0, 0, 0);
  QTime pendingStartTime_ = QTime(0, 0, 0);
  QTime pendingEndTime_ = QTime(0, 0, 0);
};



}  // namespace

class AdDatePicker::Private {
 public:
  explicit Private(AdDatePicker* owner) : q(owner) {
    placeholder = defaultPlaceholderText(pickerMode);
  }

  bool popupVisible() const { return popover && popover->isVisible(); }

  DatePickerVisualStyle visualStyle() const {
    return resolveDatePickerVisualStyle(q, controlSize, variant, status, disabled, popupVisible(),
                                        componentTokens);
  }

  QString effectivePlaceholder() const {
    return placeholder.isEmpty() ? defaultPlaceholderText(pickerMode) : placeholder;
  }

  adqt::icons::IconRef effectiveSuffixIcon() const {
    return adqt::icons::isValid(suffixIconRef) ? suffixIconRef : defaultPickerSuffixIcon();
  }

  void bindExtraFooterDestroyed() {
    if (extraFooterDestroyedConnection) {
      QObject::disconnect(extraFooterDestroyedConnection);
    }
    if (extraFooterWidget) {
      extraFooterDestroyedConnection =
          QObject::connect(extraFooterWidget, &QObject::destroyed, q, [this]() {
            extraFooterWidget = nullptr;
            if (panel) {
              panel->setExtraFooterWidget(nullptr);
            }
            emit q->extraFooterWidgetChanged(nullptr);
          });
    }
  }

  void ensureUi() {
    if (!rootLayout) {
      rootLayout = new QHBoxLayout(q);
      rootLayout->setContentsMargins(0, 0, 0, 0);
      rootLayout->setSpacing(0);
    }

    if (!lineEdit) {
      lineEdit = new AdLineEdit(q);
      lineEdit->setObjectName(QStringLiteral("ad-date-picker-line-edit"));
      rootLayout->addWidget(lineEdit);
      lineEdit->installEventFilter(q);
      QObject::connect(lineEdit, &QLineEdit::editingFinished, q, [this]() { handleManualCommit(); });
      QObject::connect(lineEdit, &AdLineEdit::cleared, q, [this]() {
        if (allowClear) {
          q->clearSelection();
        }
      });
    }

    if (!tagSurface) {
      tagSurface = new DateTagInputSurface(q);
      tagSurface->hide();
      rootLayout->addWidget(tagSurface);
      tagSurface->installEventFilter(q);
      QObject::connect(tagSurface, &DateTagInputSurface::clearRequested, q, [this]() {
        if (allowClear) {
          q->clearSelection();
        }
      });
      QObject::connect(tagSurface, &DateTagInputSurface::removeChipRequested, q, [this](int sourceIndex) {
        if (sourceIndex < 0 || sourceIndex >= values.size()) {
          return;
        }
        QVector<QDateTime> next = values;
        next.removeAt(sourceIndex);
        setValuesInternal(next, true);
      });
      QObject::connect(tagSurface, &DateTagInputSurface::activated, q,
                       [this]() { q->setPopupVisible(true); });
    }

    syncModeVisibility();
  }

  void syncModeVisibility() {
    const bool multiple = selectionMode == AdDatePicker::SelectionMode::Multiple;
    if (lineEdit) {
      lineEdit->setVisible(!multiple);
    }
    if (tagSurface) {
      tagSurface->setVisible(multiple);
    }
  }

  void ensurePanel() {
    if (panel) {
      return;
    }
    panel = new DatePickerPanelWidget(false, q);
    QObject::connect(panel, &DatePickerPanelWidget::singleValueCommitted, q,
                     [this](const QDateTime& value, bool closePopup) {
                       setValueInternal(value, true);
                       emit q->editingFinished(value);
                       if (closePopup) {
                         q->setPopupVisible(false);
                       }
                     });
    QObject::connect(panel, &DatePickerPanelWidget::multipleValuesCommitted, q,
                     [this](const QVector<QDateTime>& nextValues) {
                       setValuesInternal(nextValues, true);
                     });
    QObject::connect(panel, &DatePickerPanelWidget::closeRequested, q,
                     [this]() { q->setPopupVisible(false); });
  }

  void ensurePopover() {
    ensurePanel();
    if (popover) {
      return;
    }

    popover = new AdPopover(q);
    popover->setVisibilityPolicy(AdPopover::VisibilityPolicy::Manual);
    popover->setArrowVisible(false);
    popover->setSourceWidget(q);
    popover->setContentWidget(panel);
    popover->setContentMargins(QMargins(0, 0, 0, 0));
    popover->setPlacement(toPopoverPlacement(placement));
    popover->setEnabled(!disabled);

    QObject::connect(popover, &AdPopover::visibilityRequested, q, [this](bool open) {
      if (open) {
        syncPanelState();
        if (panel) {
          panel->resetDraftFromCommitted();
        }
      }
      if (popover && popover->isVisible() != open) {
        popover->setVisible(open);
      }
    });

    QObject::connect(popover, &AdPopover::visibleChanged, q, [this](bool open) {
      syncEditorState();
      emit q->popupVisibleChanged(open);
    });
  }

  void syncPanelState() {
    ensurePanel();
    panel->setControlSize(controlSize);
    panel->setVisualStyle(visualStyle());
    panel->setPickerMode(pickerMode);
    panel->setSelectionMode(selectionMode);
    panel->setShowTime(showTime);
    panel->setNeedConfirm(needConfirm);
    panel->setAutoSortSelections(autoSortSelections);
    panel->setTimePanelOptions(timePanelOptions);
    panel->setMinDate(minDate);
    panel->setMaxDate(maxDate);
    panel->setDisabledDateEvaluator(disabledDateEvaluator);
    panel->setDisabledTimeEvaluator(disabledTimeEvaluator);
    panel->setExtraFooterText(extraFooterText);
    panel->setExtraFooterWidget(extraFooterWidget);
    panel->setSingleValue(value);
    panel->setMultipleValues(values);
    panel->setPresets(presets);
    if (popover) {
      popover->setBackgroundColor(visualStyle().popupBg);
      popover->setBorderColor(visualStyle().popupBorder);
      popover->setCornerRadius(visualStyle().metrics.popupRadius);
      popover->setBorderWidth(visualStyle().metrics.borderWidth);
      popover->setMaximumWidth(panel->preferredWidth());
    }
  }

  void syncPopoverState() {
    if (!popover) {
      return;
    }
    popover->setPlacement(toPopoverPlacement(placement));
    popover->setEnabled(!disabled);
    popover->setSourceWidget(selectionMode == AdDatePicker::SelectionMode::Multiple
                                 ? static_cast<QWidget*>(tagSurface)
                                 : static_cast<QWidget*>(lineEdit));
    syncPanelState();
    popover->refreshPopupLayout();
  }

  void syncEditorState() {
    ensureUi();
    const DatePickerVisualStyle style = visualStyle();
    if (lineEdit) {
      lineEdit->setControlSize(toLineEditSize(controlSize));
      lineEdit->setVariant(toLineEditVariant(variant));
      lineEdit->setStatus(toLineEditStatus(status));
      lineEdit->setAllowClear(allowClear);
      lineEdit->setPlaceholderText(effectivePlaceholder());
      lineEdit->setPrefixText(prefixText);
      lineEdit->setSuffixText(suffixText);
      lineEdit->setPrefixIconRef(prefixIconRef);
      lineEdit->setSuffixIconRef(effectiveSuffixIcon());
      lineEdit->setEnabled(!disabled);
      const QSignalBlocker blocker(lineEdit);
      lineEdit->setText(value.isValid()
                            ? formatDateTimeDisplay(value, pickerMode, displayFormat, showTime,
                                                    timePanelOptions)
                            : QString());
    }

    if (tagSurface) {
      QStringList labels;
      QVector<int> indexes;
      for (int i = 0; i < values.size(); ++i) {
        labels.append(
            formatDateTimeDisplay(values.at(i), pickerMode, displayFormat, showTime, timePanelOptions));
        indexes.append(i);
      }
      tagSurface->setPlaceholder(effectivePlaceholder());
      tagSurface->setPrefixText(prefixText);
      tagSurface->setSuffixText(suffixText);
      tagSurface->setPrefixIconRef(prefixIconRef);
      tagSurface->setSuffixIconRef(effectiveSuffixIcon());
      tagSurface->setChipData(labels, indexes);
      tagSurface->setMaxVisibleTags(maxVisibleTags);
      tagSurface->setResponsiveMaxTagCount(responsiveMaxTagCount);
      tagSurface->setVisualStyle(style, popupVisible(), disabled, allowClear, popupVisible());
      tagSurface->setEnabled(!disabled);
    }

    syncPopoverState();
  }

  void handleManualCommit() {
    if (!lineEdit || selectionMode != AdDatePicker::SelectionMode::Single) {
      return;
    }
    const QString text = lineEdit->text().trimmed();
    if (text.isEmpty()) {
      if (allowClear) {
        q->clearSelection();
      } else {
        syncEditorState();
      }
      return;
    }

    const auto parsed =
        parseDateTimeDisplay(text, pickerMode, acceptedInputFormats, showTime, timePanelOptions);
    if (!parsed.has_value()) {
      syncEditorState();
      return;
    }

    QDateTime next = canonicalDateTime(parsed.value(), pickerMode, showTime, q->locale());
    if (isDateBlocked(next.date(), minDate, maxDate, disabledDateEvaluator)) {
      syncEditorState();
      return;
    }
    next.setTime(sanitizeTime(next.time(),
                              timePanelOptions,
                              evaluateDisabledTime(disabledTimeEvaluator, next.date(),
                                                   AdDatePicker::RangePart::Single, std::nullopt)));
    setValueInternal(next, true);
    emit q->editingFinished(next);
  }

  void setValueInternal(const QDateTime& nextValue, bool fromUser) {
    const QDateTime canonical =
        nextValue.isValid() ? canonicalDateTime(nextValue, pickerMode, showTime, q->locale())
                            : QDateTime();
    if (value == canonical) {
      syncEditorState();
      return;
    }
    value = canonical;
    syncEditorState();
    emit q->valueChanged(value);
    if (fromUser && allowClear && !value.isValid()) {
      emit q->cleared();
    }
  }

  void setValuesInternal(const QVector<QDateTime>& nextValues, bool fromUser) {
    const QVector<QDateTime> normalized =
        normalizeDateTimes(nextValues, pickerMode, showTime, q->locale(), autoSortSelections);
    if (values == normalized) {
      syncEditorState();
      return;
    }
    values = normalized;
    syncEditorState();
    emit q->valuesChanged(values);
    if (fromUser && values.isEmpty()) {
      emit q->cleared();
    }
  }

  AdDatePicker* q = nullptr;
  AdDatePicker::PickerMode pickerMode = AdDatePicker::PickerMode::Date;
  AdDatePicker::SelectionMode selectionMode = AdDatePicker::SelectionMode::Single;
  AdDatePicker::ControlSize controlSize = AdDatePicker::ControlSize::Middle;
  AdDatePicker::Variant variant = AdDatePicker::Variant::Outlined;
  AdDatePicker::Status status = AdDatePicker::Status::None;
  bool allowClear = true;
  bool showTime = false;
  bool needConfirm = false;
  bool autoSortSelections = true;
  bool disabled = false;
  AdDatePicker::Placement placement = AdDatePicker::Placement::BottomLeft;
  QString placeholder;
  QString displayFormat;
  QStringList acceptedInputFormats;
  int maxVisibleTags = -1;
  bool responsiveMaxTagCount = false;
  QDate minDate;
  QDate maxDate;
  QString prefixText;
  QString suffixText;
  adqt::icons::IconRef prefixIconRef;
  adqt::icons::IconRef suffixIconRef;
  QString extraFooterText;
  AdDateTimePanelOptions timePanelOptions;
  QDateTime value;
  QVector<QDateTime> values;
  QVector<AdDatePresetItem> presets;
  AdDatePicker::DisabledDateEvaluator disabledDateEvaluator;
  AdDatePicker::DisabledTimeEvaluator disabledTimeEvaluator;
  AdDatePicker::ComponentTokens componentTokens;
  QPointer<QWidget> extraFooterWidget;
  QMetaObject::Connection extraFooterDestroyedConnection;

  QHBoxLayout* rootLayout = nullptr;
  AdLineEdit* lineEdit = nullptr;
  DateTagInputSurface* tagSurface = nullptr;
  AdPopover* popover = nullptr;
  DatePickerPanelWidget* panel = nullptr;
};

class AdDateRangePicker::Private {
 public:
  explicit Private(AdDateRangePicker* owner) : q(owner) {
    startPlaceholder = defaultRangeStartPlaceholderText(pickerMode);
    endPlaceholder = defaultRangeEndPlaceholderText(pickerMode);
    separatorText = QStringLiteral("~");
  }

  bool popupVisible() const { return popover && popover->isVisible(); }

  DatePickerVisualStyle visualStyle() const {
    return resolveDatePickerVisualStyle(q, controlSize, variant, status, disabled, popupVisible(),
                                        componentTokens);
  }

  QString effectiveStartPlaceholder() const {
    return startPlaceholder.isEmpty() ? defaultRangeStartPlaceholderText(pickerMode) : startPlaceholder;
  }

  QString effectiveEndPlaceholder() const {
    return endPlaceholder.isEmpty() ? defaultRangeEndPlaceholderText(pickerMode) : endPlaceholder;
  }

  adqt::icons::IconRef effectiveSuffixIcon() const {
    return adqt::icons::isValid(suffixIconRef) ? suffixIconRef : defaultPickerSuffixIcon();
  }

  void bindExtraFooterDestroyed() {
    if (extraFooterDestroyedConnection) {
      QObject::disconnect(extraFooterDestroyedConnection);
    }
    if (extraFooterWidget) {
      extraFooterDestroyedConnection =
          QObject::connect(extraFooterWidget, &QObject::destroyed, q, [this]() {
            extraFooterWidget = nullptr;
            if (panel) {
              panel->setExtraFooterWidget(nullptr);
            }
            emit q->extraFooterWidgetChanged(nullptr);
          });
    }
  }

  void ensureUi() {
    if (!rootLayout) {
      rootLayout = new QHBoxLayout(q);
      rootLayout->setContentsMargins(0, 0, 0, 0);
      rootLayout->setSpacing(0);
    }

    if (!fieldGroup) {
      fieldGroup = new AdFieldGroup(q);
      rootLayout->addWidget(fieldGroup);
    }

    if (!startEdit) {
      startEdit = new AdLineEdit(fieldGroup);
      startEdit->installEventFilter(q);
      fieldGroup->addControl(startEdit, 1);
      QObject::connect(startEdit, &QLineEdit::editingFinished, q,
                       [this]() { handleManualCommit(AdDatePicker::RangePart::Start); });
      QObject::connect(startEdit, &AdLineEdit::cleared, q, [this]() {
        if (allowEmptyStart || startDisabled) {
          AdDateTimeRangeValue next = rangeValue;
          next.start = QDateTime();
          setRangeInternal(next, true);
        } else if (allowClear) {
          q->clearSelection();
        }
      });
    }

    if (!separatorHost) {
      separatorHost = new QWidget(fieldGroup);
      auto* layout = new QHBoxLayout(separatorHost);
      layout->setContentsMargins(8, 0, 8, 0);
      layout->setSpacing(0);
      separatorLabel = new QLabel(separatorHost);
      separatorLabel->setAlignment(Qt::AlignCenter);
      layout->addWidget(separatorLabel);
      fieldGroup->addControl(separatorHost, 0);
    }

    if (!endEdit) {
      endEdit = new AdLineEdit(fieldGroup);
      endEdit->installEventFilter(q);
      fieldGroup->addControl(endEdit, 1);
      QObject::connect(endEdit, &QLineEdit::editingFinished, q,
                       [this]() { handleManualCommit(AdDatePicker::RangePart::End); });
      QObject::connect(endEdit, &AdLineEdit::cleared, q, [this]() {
        if (allowEmptyEnd || endDisabled) {
          AdDateTimeRangeValue next = rangeValue;
          next.end = QDateTime();
          setRangeInternal(next, true);
        } else if (allowClear) {
          q->clearSelection();
        }
      });
    }

  }

  void ensurePanel() {
    if (panel) {
      return;
    }
    panel = new DatePickerPanelWidget(true, q);
    QObject::connect(panel, &DatePickerPanelWidget::rangeValueCommitted, q,
                     [this](const AdDateTimeRangeValue& value, bool closePopup) {
                       setRangeInternal(value, true);
                       emit q->editingFinished(rangeValue);
                       if (closePopup) {
                         q->setPopupVisible(false);
                       }
                     });
    QObject::connect(panel, &DatePickerPanelWidget::closeRequested, q,
                     [this]() { q->setPopupVisible(false); });
  }

  void ensurePopover() {
    ensurePanel();
    if (popover) {
      return;
    }
    popover = new AdPopover(q);
    popover->setVisibilityPolicy(AdPopover::VisibilityPolicy::Manual);
    popover->setArrowVisible(false);
    popover->setSourceWidget(q);
    popover->setContentWidget(panel);
    popover->setContentMargins(QMargins(0, 0, 0, 0));
    popover->setPlacement(toPopoverPlacement(placement));
    popover->setEnabled(!disabled);

    QObject::connect(popover, &AdPopover::visibilityRequested, q, [this](bool open) {
      if (open) {
        syncPanelState();
        if (panel) {
          panel->resetDraftFromCommitted();
        }
      }
      if (popover && popover->isVisible() != open) {
        popover->setVisible(open);
      }
    });

    QObject::connect(popover, &AdPopover::visibleChanged, q, [this](bool open) {
      syncEditors();
      emit q->popupVisibleChanged(open);
    });
  }

  void syncPanelState() {
    ensurePanel();
    panel->setControlSize(controlSize);
    panel->setVisualStyle(visualStyle());
    panel->setPickerMode(pickerMode);
    panel->setShowTime(showTime);
    panel->setNeedConfirm(needConfirm);
    panel->setTimePanelOptions(timePanelOptions);
    panel->setMinDate(minDate);
    panel->setMaxDate(maxDate);
    panel->setDisabledDateEvaluator(disabledDateEvaluator);
    panel->setDisabledTimeEvaluator(disabledTimeEvaluator);
    panel->setRangeValue(rangeValue);
    panel->setRangePresets(presets);
    panel->setAllowEmpty(allowEmptyStart || startDisabled, allowEmptyEnd || endDisabled);
    panel->setActiveRangePart(activePart);
    panel->setExtraFooterText(extraFooterText);
    panel->setExtraFooterWidget(extraFooterWidget);
    if (popover) {
      popover->setBackgroundColor(visualStyle().popupBg);
      popover->setBorderColor(visualStyle().popupBorder);
      popover->setCornerRadius(visualStyle().metrics.popupRadius);
      popover->setBorderWidth(visualStyle().metrics.borderWidth);
      popover->setMaximumWidth(panel->preferredWidth());
    }
  }

  void syncPopoverState() {
    if (!popover) {
      return;
    }
    popover->setPlacement(toPopoverPlacement(placement));
    popover->setEnabled(!disabled);
    popover->setSourceWidget(fieldGroup);
    syncPanelState();
    popover->refreshPopupLayout();
  }

  void syncEditors() {
    ensureUi();
    const DatePickerVisualStyle style = visualStyle();
    QPalette separatorPalette = separatorLabel->palette();
    separatorPalette.setColor(QPalette::WindowText, disabled ? style.disabledText : style.mutedText);
    separatorLabel->setPalette(separatorPalette);
    separatorLabel->setText(separatorText.isEmpty() ? QStringLiteral("~") : separatorText);

    const QString startText =
        rangeValue.start.isValid()
            ? formatDateTimeDisplay(rangeValue.start, pickerMode, displayFormat, showTime,
                                    timePanelOptions)
            : QString();
    const QString endText =
        rangeValue.end.isValid()
            ? formatDateTimeDisplay(rangeValue.end, pickerMode, displayFormat, showTime,
                                    timePanelOptions)
            : QString();

    startEdit->setControlSize(toLineEditSize(controlSize));
    startEdit->setVariant(toLineEditVariant(variant));
    startEdit->setStatus(toLineEditStatus(status));
    startEdit->setAllowClear(allowClear);
    startEdit->setPlaceholderText(effectiveStartPlaceholder());
    startEdit->setPrefixText(prefixText);
    startEdit->setPrefixIconRef(prefixIconRef);
    startEdit->setSuffixText(QString());
    startEdit->setSuffixIconRef({});
    startEdit->setEnabled(!disabled && !startDisabled);
    {
      const QSignalBlocker blocker(startEdit);
      startEdit->setText(startText);
    }

    endEdit->setControlSize(toLineEditSize(controlSize));
    endEdit->setVariant(toLineEditVariant(variant));
    endEdit->setStatus(toLineEditStatus(status));
    endEdit->setAllowClear(allowClear);
    endEdit->setPlaceholderText(effectiveEndPlaceholder());
    endEdit->setPrefixText(QString());
    endEdit->setPrefixIconRef({});
    endEdit->setSuffixText(suffixText);
    endEdit->setSuffixIconRef(effectiveSuffixIcon());
    endEdit->setEnabled(!disabled && !endDisabled);
    {
      const QSignalBlocker blocker(endEdit);
      endEdit->setText(endText);
    }

    syncPopoverState();
  }

  void handleManualCommit(AdDatePicker::RangePart part) {
    AdLineEdit* edit = part == AdDatePicker::RangePart::Start ? startEdit : endEdit;
    if (!edit) {
      return;
    }
    const QString text = edit->text().trimmed();
    if (text.isEmpty()) {
      if ((part == AdDatePicker::RangePart::Start && (allowEmptyStart || startDisabled)) ||
          (part == AdDatePicker::RangePart::End && (allowEmptyEnd || endDisabled))) {
        AdDateTimeRangeValue next = rangeValue;
        if (part == AdDatePicker::RangePart::Start) {
          next.start = QDateTime();
        } else {
          next.end = QDateTime();
        }
        setRangeInternal(next, true);
      } else {
        syncEditors();
      }
      return;
    }

    const auto parsed =
        parseDateTimeDisplay(text, pickerMode, acceptedInputFormats, showTime, timePanelOptions);
    if (!parsed.has_value()) {
      syncEditors();
      return;
    }

    QDateTime nextValue =
        canonicalDateTime(parsed.value(), pickerMode, showTime, q->locale());
    if (isDateBlocked(nextValue.date(), minDate, maxDate, disabledDateEvaluator)) {
      syncEditors();
      return;
    }
    AdDateTimeRangeValue next = rangeValue;
    if (part == AdDatePicker::RangePart::Start) {
      nextValue.setTime(sanitizeTime(nextValue.time(), timePanelOptions,
                                     evaluateDisabledTime(disabledTimeEvaluator, nextValue.date(),
                                                          AdDatePicker::RangePart::Start,
                                                          next.end.isValid()
                                                              ? std::optional<QDate>(next.end.date())
                                                              : std::nullopt)));
      next.start = nextValue;
    } else {
      nextValue.setTime(sanitizeTime(nextValue.time(), timePanelOptions,
                                     evaluateDisabledTime(disabledTimeEvaluator, nextValue.date(),
                                                          AdDatePicker::RangePart::End,
                                                          next.start.isValid()
                                                              ? std::optional<QDate>(next.start.date())
                                                              : std::nullopt)));
      next.end = nextValue;
    }
    setRangeInternal(next, true);
    emit q->editingFinished(rangeValue);
  }

  void setRangeInternal(const AdDateTimeRangeValue& nextRange, bool fromUser) {
    AdDateTimeRangeValue canonical = nextRange;
    if (canonical.start.isValid()) {
      canonical.start = canonicalDateTime(canonical.start, pickerMode, showTime, q->locale());
    }
    if (canonical.end.isValid()) {
      canonical.end = canonicalDateTime(canonical.end, pickerMode, showTime, q->locale());
    }
    if (canonical.start.isValid() && canonical.end.isValid() && canonical.start > canonical.end) {
      std::swap(canonical.start, canonical.end);
    }
    if (rangeValue == canonical) {
      syncEditors();
      return;
    }
    rangeValue = canonical;
    syncEditors();
    emit q->rangeValueChanged(rangeValue);
    if (fromUser && rangeValue.isEmpty()) {
      emit q->cleared();
    }
  }

  AdDateRangePicker* q = nullptr;
  AdDatePicker::PickerMode pickerMode = AdDatePicker::PickerMode::Date;
  AdDatePicker::ControlSize controlSize = AdDatePicker::ControlSize::Middle;
  AdDatePicker::Variant variant = AdDatePicker::Variant::Outlined;
  AdDatePicker::Status status = AdDatePicker::Status::None;
  bool allowClear = true;
  bool showTime = false;
  bool needConfirm = false;
  bool disabled = false;
  bool startDisabled = false;
  bool endDisabled = false;
  bool allowEmptyStart = false;
  bool allowEmptyEnd = false;
  AdDatePicker::Placement placement = AdDatePicker::Placement::BottomLeft;
  QString startPlaceholder;
  QString endPlaceholder;
  QString separatorText;
  QString displayFormat;
  QStringList acceptedInputFormats;
  QDate minDate;
  QDate maxDate;
  QString prefixText;
  QString suffixText;
  adqt::icons::IconRef prefixIconRef;
  adqt::icons::IconRef suffixIconRef;
  QString extraFooterText;
  AdDateTimePanelOptions timePanelOptions;
  AdDateTimeRangeValue rangeValue;
  QVector<AdDateRangePresetItem> presets;
  AdDatePicker::DisabledDateEvaluator disabledDateEvaluator;
  AdDatePicker::DisabledTimeEvaluator disabledTimeEvaluator;
  AdDatePicker::ComponentTokens componentTokens;
  AdDatePicker::RangePart activePart = AdDatePicker::RangePart::Start;
  QPointer<QWidget> extraFooterWidget;
  QMetaObject::Connection extraFooterDestroyedConnection;

  QHBoxLayout* rootLayout = nullptr;
  AdFieldGroup* fieldGroup = nullptr;
  AdLineEdit* startEdit = nullptr;
  AdLineEdit* endEdit = nullptr;
  QWidget* separatorHost = nullptr;
  QLabel* separatorLabel = nullptr;
  AdPopover* popover = nullptr;
  DatePickerPanelWidget* panel = nullptr;
};

AdDatePicker::AdDatePicker(QWidget* parent) : QWidget(parent), d_(std::make_unique<Private>(this)) {
  d_->ensureUi();
}

AdDatePicker::~AdDatePicker() = default;

AdDatePicker::PickerMode AdDatePicker::pickerMode() const { return d_->pickerMode; }
void AdDatePicker::setPickerMode(PickerMode value) {
  if (d_->pickerMode == value) {
    return;
  }
  d_->pickerMode = value;
  if (d_->placeholder.isEmpty()) {
    d_->placeholder = defaultPlaceholderText(value);
  }
  d_->syncEditorState();
  emit pickerModeChanged(value);
}

AdDatePicker::SelectionMode AdDatePicker::selectionMode() const { return d_->selectionMode; }
void AdDatePicker::setSelectionMode(SelectionMode value) {
  if (d_->selectionMode == value) {
    return;
  }
  d_->selectionMode = value;
  if (value == SelectionMode::Multiple) {
    if (d_->values.isEmpty() && d_->value.isValid()) {
      d_->values = {d_->value};
    }
  } else if (value == SelectionMode::Single) {
    if (d_->value.isNull() && !d_->values.isEmpty()) {
      d_->value = d_->values.constFirst();
    }
  }
  d_->syncModeVisibility();
  d_->syncEditorState();
  emit selectionModeChanged(value);
}

AdDatePicker::ControlSize AdDatePicker::controlSize() const { return d_->controlSize; }
void AdDatePicker::setControlSize(ControlSize value) {
  if (d_->controlSize == value) {
    return;
  }
  d_->controlSize = value;
  d_->syncEditorState();
  emit controlSizeChanged(value);
}

AdDatePicker::Variant AdDatePicker::variant() const { return d_->variant; }
void AdDatePicker::setVariant(Variant value) {
  if (d_->variant == value) {
    return;
  }
  d_->variant = value;
  d_->syncEditorState();
  emit variantChanged(value);
}

AdDatePicker::Status AdDatePicker::status() const { return d_->status; }
void AdDatePicker::setStatus(Status value) {
  if (d_->status == value) {
    return;
  }
  d_->status = value;
  d_->syncEditorState();
  emit statusChanged(value);
}

bool AdDatePicker::allowClear() const { return d_->allowClear; }
void AdDatePicker::setAllowClear(bool value) {
  if (d_->allowClear == value) {
    return;
  }
  d_->allowClear = value;
  d_->syncEditorState();
  emit allowClearChanged(value);
}

bool AdDatePicker::popupVisible() const { return d_->popupVisible(); }
void AdDatePicker::setPopupVisible(bool value) {
  if (value && d_->disabled) {
    return;
  }
  d_->ensurePopover();
  d_->syncPanelState();
  if (value && d_->panel) {
    d_->panel->resetDraftFromCommitted();
  }
  d_->popover->setVisible(value);
}

bool AdDatePicker::showTime() const { return d_->showTime; }
void AdDatePicker::setShowTime(bool value) {
  if (d_->showTime == value) {
    return;
  }
  d_->showTime = value;
  d_->syncEditorState();
  emit showTimeChanged(value);
}

bool AdDatePicker::needConfirm() const { return d_->needConfirm; }
void AdDatePicker::setNeedConfirm(bool value) {
  if (d_->needConfirm == value) {
    return;
  }
  d_->needConfirm = value;
  d_->syncEditorState();
  emit needConfirmChanged(value);
}

bool AdDatePicker::autoSortSelections() const { return d_->autoSortSelections; }
void AdDatePicker::setAutoSortSelections(bool value) {
  if (d_->autoSortSelections == value) {
    return;
  }
  d_->autoSortSelections = value;
  d_->syncEditorState();
  emit autoSortSelectionsChanged(value);
}

bool AdDatePicker::disabled() const { return d_->disabled; }
void AdDatePicker::setDisabled(bool value) {
  if (d_->disabled == value) {
    return;
  }
  d_->disabled = value;
  QWidget::setDisabled(value);
  if (value && d_->popover) {
    d_->popover->hide();
  }
  d_->syncEditorState();
  emit disabledChanged(value);
}

AdDatePicker::Placement AdDatePicker::placement() const { return d_->placement; }
void AdDatePicker::setPlacement(Placement value) {
  if (d_->placement == value) {
    return;
  }
  d_->placement = value;
  d_->syncPopoverState();
  emit placementChanged(value);
}

QString AdDatePicker::placeholder() const { return d_->placeholder; }
void AdDatePicker::setPlaceholder(const QString& value) {
  if (d_->placeholder == value) {
    return;
  }
  d_->placeholder = value;
  d_->syncEditorState();
  emit placeholderChanged(value);
}

QString AdDatePicker::displayFormat() const { return d_->displayFormat; }
void AdDatePicker::setDisplayFormat(const QString& value) {
  if (d_->displayFormat == value) {
    return;
  }
  d_->displayFormat = value;
  d_->syncEditorState();
  emit displayFormatChanged(value);
}

QStringList AdDatePicker::acceptedInputFormats() const { return d_->acceptedInputFormats; }
void AdDatePicker::setAcceptedInputFormats(const QStringList& value) {
  if (d_->acceptedInputFormats == value) {
    return;
  }
  d_->acceptedInputFormats = value;
  emit acceptedInputFormatsChanged(value);
}

int AdDatePicker::maxVisibleTags() const { return d_->maxVisibleTags; }
void AdDatePicker::setMaxVisibleTags(int value) {
  if (d_->maxVisibleTags == value) {
    return;
  }
  d_->maxVisibleTags = value;
  d_->syncEditorState();
  emit maxVisibleTagsChanged(value);
}

bool AdDatePicker::responsiveMaxTagCount() const { return d_->responsiveMaxTagCount; }
void AdDatePicker::setResponsiveMaxTagCount(bool value) {
  if (d_->responsiveMaxTagCount == value) {
    return;
  }
  d_->responsiveMaxTagCount = value;
  d_->syncEditorState();
  emit responsiveMaxTagCountChanged(value);
}

QDate AdDatePicker::minDate() const { return d_->minDate; }
void AdDatePicker::setMinDate(const QDate& value) {
  if (d_->minDate == value) {
    return;
  }
  d_->minDate = value;
  d_->syncEditorState();
  emit minDateChanged(value);
}

QDate AdDatePicker::maxDate() const { return d_->maxDate; }
void AdDatePicker::setMaxDate(const QDate& value) {
  if (d_->maxDate == value) {
    return;
  }
  d_->maxDate = value;
  d_->syncEditorState();
  emit maxDateChanged(value);
}

QString AdDatePicker::prefixText() const { return d_->prefixText; }
void AdDatePicker::setPrefixText(const QString& value) {
  if (d_->prefixText == value) {
    return;
  }
  d_->prefixText = value;
  d_->syncEditorState();
  emit prefixTextChanged(value);
}

QString AdDatePicker::suffixText() const { return d_->suffixText; }
void AdDatePicker::setSuffixText(const QString& value) {
  if (d_->suffixText == value) {
    return;
  }
  d_->suffixText = value;
  d_->syncEditorState();
  emit suffixTextChanged(value);
}

adqt::icons::IconRef AdDatePicker::prefixIconRef() const { return d_->prefixIconRef; }
void AdDatePicker::setPrefixIconRef(const adqt::icons::IconRef& value) {
  if (d_->prefixIconRef == value) {
    return;
  }
  d_->prefixIconRef = value;
  d_->syncEditorState();
  emit prefixIconRefChanged(value);
}

adqt::icons::IconRef AdDatePicker::suffixIconRef() const { return d_->suffixIconRef; }
void AdDatePicker::setSuffixIconRef(const adqt::icons::IconRef& value) {
  if (d_->suffixIconRef == value) {
    return;
  }
  d_->suffixIconRef = value;
  d_->syncEditorState();
  emit suffixIconRefChanged(value);
}

QString AdDatePicker::extraFooterText() const { return d_->extraFooterText; }
void AdDatePicker::setExtraFooterText(const QString& value) {
  if (d_->extraFooterText == value) {
    return;
  }
  d_->extraFooterText = value;
  d_->syncPanelState();
  emit extraFooterTextChanged(value);
}

QWidget* AdDatePicker::extraFooterWidget() const { return d_->extraFooterWidget; }
void AdDatePicker::setExtraFooterWidget(QWidget* widget) {
  if (d_->extraFooterWidget == widget) {
    return;
  }
  d_->extraFooterWidget = widget;
  d_->bindExtraFooterDestroyed();
  d_->syncPanelState();
  emit extraFooterWidgetChanged(widget);
}

QWidget* AdDatePicker::takeExtraFooterWidget() {
  QWidget* widget = d_->extraFooterWidget;
  d_->extraFooterWidget = nullptr;
  if (d_->panel) {
    d_->panel->setExtraFooterWidget(nullptr);
  }
  emit extraFooterWidgetChanged(nullptr);
  return widget;
}

AdDateTimePanelOptions AdDatePicker::timePanelOptions() const { return d_->timePanelOptions; }
void AdDatePicker::setTimePanelOptions(const AdDateTimePanelOptions& value) {
  if (d_->timePanelOptions == value) {
    return;
  }
  d_->timePanelOptions = value;
  d_->syncEditorState();
  emit timePanelOptionsChanged(value);
}

QDateTime AdDatePicker::value() const {
  if (d_->selectionMode == SelectionMode::Multiple) {
    return d_->values.isEmpty() ? QDateTime() : d_->values.constFirst();
  }
  return d_->value;
}

void AdDatePicker::setValue(const QDateTime& value) {
  if (d_->selectionMode == SelectionMode::Multiple) {
    setValues(value.isValid() ? QVector<QDateTime>{value} : QVector<QDateTime>{});
    return;
  }
  d_->setValueInternal(value, false);
}

QVector<QDateTime> AdDatePicker::values() const {
  if (d_->selectionMode == SelectionMode::Multiple) {
    return d_->values;
  }
  return d_->value.isValid() ? QVector<QDateTime>{d_->value} : QVector<QDateTime>{};
}

void AdDatePicker::setValues(const QVector<QDateTime>& value) {
  if (d_->selectionMode != SelectionMode::Multiple) {
    d_->setValueInternal(value.isEmpty() ? QDateTime() : value.constFirst(), false);
    return;
  }
  d_->setValuesInternal(value, false);
}

QVector<AdDatePresetItem> AdDatePicker::presets() const { return d_->presets; }
void AdDatePicker::setPresets(const QVector<AdDatePresetItem>& value) {
  d_->presets = value;
  d_->syncPanelState();
  emit presetsChanged();
}

AdDatePicker::DisabledDateEvaluator AdDatePicker::disabledDateEvaluator() const {
  return d_->disabledDateEvaluator;
}
void AdDatePicker::setDisabledDateEvaluator(DisabledDateEvaluator evaluator) {
  d_->disabledDateEvaluator = std::move(evaluator);
  d_->syncEditorState();
}

AdDatePicker::DisabledTimeEvaluator AdDatePicker::disabledTimeEvaluator() const {
  return d_->disabledTimeEvaluator;
}
void AdDatePicker::setDisabledTimeEvaluator(DisabledTimeEvaluator evaluator) {
  d_->disabledTimeEvaluator = std::move(evaluator);
  d_->syncEditorState();
}

AdDatePicker::ComponentTokens AdDatePicker::componentTokens() const { return d_->componentTokens; }
void AdDatePicker::setComponentTokens(const ComponentTokens& tokens) {
  d_->componentTokens = tokens;
  d_->syncEditorState();
  emit componentTokensChanged();
}
void AdDatePicker::resetComponentTokens() {
  d_->componentTokens = {};
  d_->syncEditorState();
  emit componentTokensChanged();
}

void AdDatePicker::clearSelection() {
  if (d_->selectionMode == SelectionMode::Multiple) {
    d_->setValuesInternal({}, true);
  } else {
    d_->setValueInternal(QDateTime(), true);
  }
}

void AdDatePicker::focus() {
  d_->ensureUi();
  if (d_->selectionMode == SelectionMode::Multiple && d_->tagSurface) {
    d_->tagSurface->setFocus();
  } else if (d_->lineEdit) {
    d_->lineEdit->setFocus();
  } else {
    QWidget::setFocus();
  }
}

bool AdDatePicker::eventFilter(QObject* watched, QEvent* event) {
  if (!event || d_->disabled) {
    return QWidget::eventFilter(watched, event);
  }

  if (watched == d_->lineEdit) {
    if (event->type() == QEvent::MouseButtonPress) {
      setPopupVisible(true);
    } else if (event->type() == QEvent::KeyPress) {
      auto* keyEvent = static_cast<QKeyEvent*>(event);
      if (keyEvent->key() == Qt::Key_Down || keyEvent->key() == Qt::Key_F4) {
        setPopupVisible(true);
        return true;
      }
      if (keyEvent->key() == Qt::Key_Escape && popupVisible()) {
        setPopupVisible(false);
        return true;
      }
    }
  }
  return QWidget::eventFilter(watched, event);
}

void AdDatePicker::paintEvent(QPaintEvent* event) { QWidget::paintEvent(event); }
void AdDatePicker::mousePressEvent(QMouseEvent* event) { QWidget::mousePressEvent(event); }
void AdDatePicker::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  d_->syncEditorState();
}
void AdDatePicker::changeEvent(QEvent* event) {
  QWidget::changeEvent(event);
  if (!event) {
    return;
  }
  switch (event->type()) {
    case QEvent::PaletteChange:
    case QEvent::ApplicationPaletteChange:
    case QEvent::StyleChange:
    case QEvent::FontChange:
    case QEvent::ApplicationFontChange:
      d_->syncEditorState();
      break;
    default:
      break;
  }
}

AdDateRangePicker::AdDateRangePicker(QWidget* parent)
    : QWidget(parent), d_(std::make_unique<Private>(this)) {
  d_->ensureUi();
}

AdDateRangePicker::~AdDateRangePicker() = default;

AdDateRangePicker::PickerMode AdDateRangePicker::pickerMode() const { return d_->pickerMode; }
void AdDateRangePicker::setPickerMode(PickerMode value) {
  if (d_->pickerMode == value) {
    return;
  }
  d_->pickerMode = value;
  d_->syncEditors();
  emit pickerModeChanged(value);
}

AdDateRangePicker::ControlSize AdDateRangePicker::controlSize() const { return d_->controlSize; }
void AdDateRangePicker::setControlSize(ControlSize value) {
  if (d_->controlSize == value) {
    return;
  }
  d_->controlSize = value;
  d_->syncEditors();
  emit controlSizeChanged(value);
}

AdDateRangePicker::Variant AdDateRangePicker::variant() const { return d_->variant; }
void AdDateRangePicker::setVariant(Variant value) {
  if (d_->variant == value) {
    return;
  }
  d_->variant = value;
  d_->syncEditors();
  emit variantChanged(value);
}

AdDateRangePicker::Status AdDateRangePicker::status() const { return d_->status; }
void AdDateRangePicker::setStatus(Status value) {
  if (d_->status == value) {
    return;
  }
  d_->status = value;
  d_->syncEditors();
  emit statusChanged(value);
}

bool AdDateRangePicker::allowClear() const { return d_->allowClear; }
void AdDateRangePicker::setAllowClear(bool value) {
  if (d_->allowClear == value) {
    return;
  }
  d_->allowClear = value;
  d_->syncEditors();
  emit allowClearChanged(value);
}

bool AdDateRangePicker::popupVisible() const { return d_->popupVisible(); }
void AdDateRangePicker::setPopupVisible(bool value) {
  if (value && d_->disabled) {
    return;
  }
  d_->ensurePopover();
  d_->syncPanelState();
  if (value && d_->panel) {
    d_->panel->resetDraftFromCommitted();
  }
  d_->popover->setVisible(value);
}

bool AdDateRangePicker::showTime() const { return d_->showTime; }
void AdDateRangePicker::setShowTime(bool value) {
  if (d_->showTime == value) {
    return;
  }
  d_->showTime = value;
  d_->syncEditors();
  emit showTimeChanged(value);
}

bool AdDateRangePicker::needConfirm() const { return d_->needConfirm; }
void AdDateRangePicker::setNeedConfirm(bool value) {
  if (d_->needConfirm == value) {
    return;
  }
  d_->needConfirm = value;
  d_->syncEditors();
  emit needConfirmChanged(value);
}

bool AdDateRangePicker::disabled() const { return d_->disabled; }
void AdDateRangePicker::setDisabled(bool value) {
  if (d_->disabled == value) {
    return;
  }
  d_->disabled = value;
  QWidget::setDisabled(value);
  if (value && d_->popover) {
    d_->popover->hide();
  }
  d_->syncEditors();
  emit disabledChanged(value);
}

bool AdDateRangePicker::startDisabled() const { return d_->startDisabled; }
void AdDateRangePicker::setStartDisabled(bool value) {
  if (d_->startDisabled == value) {
    return;
  }
  d_->startDisabled = value;
  d_->syncEditors();
  emit startDisabledChanged(value);
}

bool AdDateRangePicker::endDisabled() const { return d_->endDisabled; }
void AdDateRangePicker::setEndDisabled(bool value) {
  if (d_->endDisabled == value) {
    return;
  }
  d_->endDisabled = value;
  d_->syncEditors();
  emit endDisabledChanged(value);
}

bool AdDateRangePicker::allowEmptyStart() const { return d_->allowEmptyStart; }
void AdDateRangePicker::setAllowEmptyStart(bool value) {
  if (d_->allowEmptyStart == value) {
    return;
  }
  d_->allowEmptyStart = value;
  d_->syncEditors();
  emit allowEmptyStartChanged(value);
}

bool AdDateRangePicker::allowEmptyEnd() const { return d_->allowEmptyEnd; }
void AdDateRangePicker::setAllowEmptyEnd(bool value) {
  if (d_->allowEmptyEnd == value) {
    return;
  }
  d_->allowEmptyEnd = value;
  d_->syncEditors();
  emit allowEmptyEndChanged(value);
}

AdDateRangePicker::Placement AdDateRangePicker::placement() const { return d_->placement; }
void AdDateRangePicker::setPlacement(Placement value) {
  if (d_->placement == value) {
    return;
  }
  d_->placement = value;
  d_->syncPopoverState();
  emit placementChanged(value);
}

QString AdDateRangePicker::startPlaceholder() const { return d_->startPlaceholder; }
void AdDateRangePicker::setStartPlaceholder(const QString& value) {
  if (d_->startPlaceholder == value) {
    return;
  }
  d_->startPlaceholder = value;
  d_->syncEditors();
  emit startPlaceholderChanged(value);
}

QString AdDateRangePicker::endPlaceholder() const { return d_->endPlaceholder; }
void AdDateRangePicker::setEndPlaceholder(const QString& value) {
  if (d_->endPlaceholder == value) {
    return;
  }
  d_->endPlaceholder = value;
  d_->syncEditors();
  emit endPlaceholderChanged(value);
}

QString AdDateRangePicker::separatorText() const { return d_->separatorText; }
void AdDateRangePicker::setSeparatorText(const QString& value) {
  if (d_->separatorText == value) {
    return;
  }
  d_->separatorText = value;
  d_->syncEditors();
  emit separatorTextChanged(value);
}

QString AdDateRangePicker::displayFormat() const { return d_->displayFormat; }
void AdDateRangePicker::setDisplayFormat(const QString& value) {
  if (d_->displayFormat == value) {
    return;
  }
  d_->displayFormat = value;
  d_->syncEditors();
  emit displayFormatChanged(value);
}

QStringList AdDateRangePicker::acceptedInputFormats() const { return d_->acceptedInputFormats; }
void AdDateRangePicker::setAcceptedInputFormats(const QStringList& value) {
  if (d_->acceptedInputFormats == value) {
    return;
  }
  d_->acceptedInputFormats = value;
  emit acceptedInputFormatsChanged(value);
}

QDate AdDateRangePicker::minDate() const { return d_->minDate; }
void AdDateRangePicker::setMinDate(const QDate& value) {
  if (d_->minDate == value) {
    return;
  }
  d_->minDate = value;
  d_->syncEditors();
  emit minDateChanged(value);
}

QDate AdDateRangePicker::maxDate() const { return d_->maxDate; }
void AdDateRangePicker::setMaxDate(const QDate& value) {
  if (d_->maxDate == value) {
    return;
  }
  d_->maxDate = value;
  d_->syncEditors();
  emit maxDateChanged(value);
}

QString AdDateRangePicker::prefixText() const { return d_->prefixText; }
void AdDateRangePicker::setPrefixText(const QString& value) {
  if (d_->prefixText == value) {
    return;
  }
  d_->prefixText = value;
  d_->syncEditors();
  emit prefixTextChanged(value);
}

QString AdDateRangePicker::suffixText() const { return d_->suffixText; }
void AdDateRangePicker::setSuffixText(const QString& value) {
  if (d_->suffixText == value) {
    return;
  }
  d_->suffixText = value;
  d_->syncEditors();
  emit suffixTextChanged(value);
}

adqt::icons::IconRef AdDateRangePicker::prefixIconRef() const { return d_->prefixIconRef; }
void AdDateRangePicker::setPrefixIconRef(const adqt::icons::IconRef& value) {
  if (d_->prefixIconRef == value) {
    return;
  }
  d_->prefixIconRef = value;
  d_->syncEditors();
  emit prefixIconRefChanged(value);
}

adqt::icons::IconRef AdDateRangePicker::suffixIconRef() const { return d_->suffixIconRef; }
void AdDateRangePicker::setSuffixIconRef(const adqt::icons::IconRef& value) {
  if (d_->suffixIconRef == value) {
    return;
  }
  d_->suffixIconRef = value;
  d_->syncEditors();
  emit suffixIconRefChanged(value);
}

QString AdDateRangePicker::extraFooterText() const { return d_->extraFooterText; }
void AdDateRangePicker::setExtraFooterText(const QString& value) {
  if (d_->extraFooterText == value) {
    return;
  }
  d_->extraFooterText = value;
  d_->syncPanelState();
  emit extraFooterTextChanged(value);
}

QWidget* AdDateRangePicker::extraFooterWidget() const { return d_->extraFooterWidget; }
void AdDateRangePicker::setExtraFooterWidget(QWidget* widget) {
  if (d_->extraFooterWidget == widget) {
    return;
  }
  d_->extraFooterWidget = widget;
  d_->bindExtraFooterDestroyed();
  d_->syncPanelState();
  emit extraFooterWidgetChanged(widget);
}

QWidget* AdDateRangePicker::takeExtraFooterWidget() {
  QWidget* widget = d_->extraFooterWidget;
  d_->extraFooterWidget = nullptr;
  if (d_->panel) {
    d_->panel->setExtraFooterWidget(nullptr);
  }
  emit extraFooterWidgetChanged(nullptr);
  return widget;
}

AdDateTimePanelOptions AdDateRangePicker::timePanelOptions() const { return d_->timePanelOptions; }
void AdDateRangePicker::setTimePanelOptions(const AdDateTimePanelOptions& value) {
  if (d_->timePanelOptions == value) {
    return;
  }
  d_->timePanelOptions = value;
  d_->syncEditors();
  emit timePanelOptionsChanged(value);
}

AdDateTimeRangeValue AdDateRangePicker::rangeValue() const { return d_->rangeValue; }
void AdDateRangePicker::setRangeValue(const AdDateTimeRangeValue& value) {
  d_->setRangeInternal(value, false);
}

QVector<AdDateRangePresetItem> AdDateRangePicker::presets() const { return d_->presets; }
void AdDateRangePicker::setPresets(const QVector<AdDateRangePresetItem>& value) {
  d_->presets = value;
  d_->syncPanelState();
  emit presetsChanged();
}

AdDateRangePicker::DisabledDateEvaluator AdDateRangePicker::disabledDateEvaluator() const {
  return d_->disabledDateEvaluator;
}
void AdDateRangePicker::setDisabledDateEvaluator(DisabledDateEvaluator evaluator) {
  d_->disabledDateEvaluator = std::move(evaluator);
  d_->syncEditors();
}

AdDateRangePicker::DisabledTimeEvaluator AdDateRangePicker::disabledTimeEvaluator() const {
  return d_->disabledTimeEvaluator;
}
void AdDateRangePicker::setDisabledTimeEvaluator(DisabledTimeEvaluator evaluator) {
  d_->disabledTimeEvaluator = std::move(evaluator);
  d_->syncEditors();
}

AdDateRangePicker::ComponentTokens AdDateRangePicker::componentTokens() const {
  return d_->componentTokens;
}
void AdDateRangePicker::setComponentTokens(const ComponentTokens& tokens) {
  d_->componentTokens = tokens;
  d_->syncEditors();
  emit componentTokensChanged();
}
void AdDateRangePicker::resetComponentTokens() {
  d_->componentTokens = {};
  d_->syncEditors();
  emit componentTokensChanged();
}

void AdDateRangePicker::clearSelection() {
  d_->setRangeInternal({}, true);
}

void AdDateRangePicker::focus() {
  d_->ensureUi();
  if (!d_->startDisabled && d_->startEdit) {
    d_->startEdit->setFocus();
  } else if (!d_->endDisabled && d_->endEdit) {
    d_->endEdit->setFocus();
  } else {
    QWidget::setFocus();
  }
}

bool AdDateRangePicker::eventFilter(QObject* watched, QEvent* event) {
  if (!event || d_->disabled) {
    return QWidget::eventFilter(watched, event);
  }
  if (watched == d_->startEdit) {
    d_->activePart = RangePart::Start;
    if (event->type() == QEvent::MouseButtonPress) {
      setPopupVisible(true);
    } else if (event->type() == QEvent::KeyPress) {
      auto* keyEvent = static_cast<QKeyEvent*>(event);
      if (keyEvent->key() == Qt::Key_Down || keyEvent->key() == Qt::Key_F4) {
        setPopupVisible(true);
        return true;
      }
    }
  } else if (watched == d_->endEdit) {
    d_->activePart = RangePart::End;
    if (event->type() == QEvent::MouseButtonPress) {
      setPopupVisible(true);
    } else if (event->type() == QEvent::KeyPress) {
      auto* keyEvent = static_cast<QKeyEvent*>(event);
      if (keyEvent->key() == Qt::Key_Down || keyEvent->key() == Qt::Key_F4) {
        setPopupVisible(true);
        return true;
      }
    }
  }
  return QWidget::eventFilter(watched, event);
}

void AdDateRangePicker::paintEvent(QPaintEvent* event) { QWidget::paintEvent(event); }
void AdDateRangePicker::mousePressEvent(QMouseEvent* event) { QWidget::mousePressEvent(event); }
void AdDateRangePicker::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  d_->syncEditors();
}
void AdDateRangePicker::changeEvent(QEvent* event) {
  QWidget::changeEvent(event);
  if (!event) {
    return;
  }
  switch (event->type()) {
    case QEvent::PaletteChange:
    case QEvent::ApplicationPaletteChange:
    case QEvent::StyleChange:
    case QEvent::FontChange:
    case QEvent::ApplicationFontChange:
      d_->syncEditors();
      break;
    default:
      break;
  }
}

}  // namespace adqt::widgets

#include "date_picker.moc"
