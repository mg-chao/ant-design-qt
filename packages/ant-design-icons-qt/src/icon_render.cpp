#include "icon_render.h"

#include <QRegularExpression>
#include <QtGlobal>

namespace adqt::icons::detail {

namespace {

QString toSvgColor(const QColor& color) {
  QColor safe = color.isValid() ? color : QColor(QStringLiteral("#000000"));
  if (safe.alpha() >= 255) {
    return safe.name(QColor::HexRgb);
  }

  return QStringLiteral("rgba(%1,%2,%3,%4)")
      .arg(safe.red())
      .arg(safe.green())
      .arg(safe.blue())
      .arg(QString::number(safe.alphaF(), 'f', 3));
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
                            const QColor& primary,
                            const QColor& secondary) {
  QString svg = QString::fromUtf8(source);
  const QString primaryToken = toSvgColor(primary);
  const QString secondaryToken = toSvgColor(secondary);

  if (theme == IconTheme::TwoTone) {
    replaceRegex(svg, QStringLiteral("#333333|#333"), primaryToken);
    replaceRegex(svg, QStringLiteral("#E6E6E6|#D9D9D9|#D8D8D8"), secondaryToken);
    replaceRegex(svg, QStringLiteral("currentColor"), primaryToken);
  } else {
    // Upstream SVGs are not fully normalized: many paths omit `fill` and would stay black.
    // Align with Ant Design behavior by making non-twotone icons inherit currentColor.
    ensureRootFillInheritsCurrentColor(svg);
    replaceRegex(svg, QStringLiteral("currentColor"), primaryToken);
    replaceRegex(svg, QStringLiteral("#333333|#333"), primaryToken);
    replaceRegex(svg, QStringLiteral("#000000|#000"), primaryToken);
  }

  return svg.toUtf8();
}

}  // namespace adqt::icons::detail
