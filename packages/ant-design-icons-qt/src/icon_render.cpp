#include "icon_render.h"

#include <QRegularExpression>
#include <QtGlobal>
#include <cstring>

namespace adqt::icons::detail {

namespace {

QString toSvgColor(const QColor& color) {
  QColor safe = color.isValid() ? color : QColor(QStringLiteral("#000000"));
  // QtSvg does not reliably support `fill="rgba(...)"` or 8-digit hex colors.
  // Always emit opaque rgb hex and let the caller apply alpha post-render.
  return safe.name(QColor::HexRgb);
}

void replaceRegex(QString& text, const QString& pattern, const QString& replacement) {
  const QRegularExpression expr(pattern, QRegularExpression::CaseInsensitiveOption);
  text.replace(expr, replacement);
}

void ensureRootFillInheritsCurrentColor(QString& svg) {
  const QRegularExpression svgTagExpr(QStringLiteral("<svg\\b([^>]*)>"),
                                      QRegularExpression::CaseInsensitiveOption);
  const QRegularExpression fillAttrExpr(QStringLiteral("\\bfill\\s*="),
                                        QRegularExpression::CaseInsensitiveOption);

  const QRegularExpressionMatch match = svgTagExpr.match(svg);
  if (!match.hasMatch()) {
    return;
  }

  const QString attrs = match.captured(1);
  if (fillAttrExpr.match(attrs).hasMatch()) {
    return;
  }

  const QString replacement = QStringLiteral("<svg%1 fill=\"currentColor\">").arg(attrs);
  svg.replace(match.capturedStart(0), match.capturedLength(0), replacement);
}

}  // namespace

QColor deriveSecondaryColor(const QColor& primary) {
  QColor safe = primary.isValid() ? primary : QColor(QStringLiteral("#1677FF"));
  QColor hsl = safe.toHsl();
  int sat = hsl.hslSaturation();
  int light = hsl.lightness();

  sat = qMax(8, qRound(sat * 0.22));
  light = qMin(245, qRound(light + (255 - light) * 0.82));
  hsl.setHsl(hsl.hslHue(), sat, light, safe.alpha());
  return hsl.toRgb();
}

QByteArray applyColorsToSvg(const QByteArray& source,
                            IconTheme theme,
                            const char* iconName,
                            const QColor& primary,
                            const QColor& secondary,
                            const QColor& tertiary) {
  QString svg = QString::fromUtf8(source);
  const QString primaryToken = toSvgColor(primary);
  const QString secondaryToken = toSvgColor(secondary);
  const QString tertiaryToken = toSvgColor(tertiary);
  constexpr auto kPrimaryPlaceholder = "__ADQT_ICON_PRIMARY__";
  constexpr auto kSecondaryPlaceholder = "__ADQT_ICON_SECONDARY__";
  constexpr auto kTertiaryPlaceholder = "__ADQT_ICON_TERTIARY__";

  if (theme == IconTheme::TwoTone) {
    // Use placeholders first to avoid replacement collisions between final color values.
    replaceRegex(svg, QStringLiteral("#333333|#333"), QString::fromLatin1(kPrimaryPlaceholder));
    replaceRegex(svg,
                 QStringLiteral("#E6E6E6|#D9D9D9|#D8D8D8"),
                 QString::fromLatin1(kSecondaryPlaceholder));
    replaceRegex(svg, QStringLiteral("currentColor"), QString::fromLatin1(kPrimaryPlaceholder));
    if (iconName && std::strcmp(iconName, "empty-simple") == 0) {
      replaceRegex(svg,
                   QStringLiteral("#F5F5F5|#F5F5F7"),
                   QString::fromLatin1(kTertiaryPlaceholder));
    }
  } else {
    // Upstream SVGs are not fully normalized: many paths omit `fill` and would stay black.
    // Align with Ant Design behavior by making non-twotone icons inherit currentColor.
    ensureRootFillInheritsCurrentColor(svg);
    replaceRegex(svg, QStringLiteral("currentColor"), QString::fromLatin1(kPrimaryPlaceholder));
    replaceRegex(svg, QStringLiteral("#333333|#333"), QString::fromLatin1(kPrimaryPlaceholder));
    replaceRegex(svg, QStringLiteral("#000000|#000"), QString::fromLatin1(kPrimaryPlaceholder));
  }

  svg.replace(QString::fromLatin1(kPrimaryPlaceholder), primaryToken, Qt::CaseSensitive);
  svg.replace(QString::fromLatin1(kSecondaryPlaceholder), secondaryToken, Qt::CaseSensitive);
  svg.replace(QString::fromLatin1(kTertiaryPlaceholder), tertiaryToken, Qt::CaseSensitive);

  return svg.toUtf8();
}

}  // namespace adqt::icons::detail
