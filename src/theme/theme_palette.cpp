#include "theme_palette.h"

#include "fast_color_lite.h"

#include <QColor>

namespace adqt::theme {

namespace {

QColor parseColor(const QString& value, const QColor& fallback) {
  const FastColorLite parsed(value);
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

QPalette buildPalette(const GlobalPaletteToken& token) {
  QPalette palette;

  const QColor bgLayout = parseColor(token.colorBgLayout, QColor("#f5f5f5"));
  const QColor bgContainer = parseColor(token.colorBgContainer, QColor("#ffffff"));
  const QColor bgContainerDisabled = parseColor(token.colorBgContainerDisabled, QColor("#f0f0f0"));
  const QColor fillAlter = parseColor(token.colorFillAlter, QColor("#fafafa"));
  const QColor bgElevated = parseColor(token.colorBgElevated, QColor("#ffffff"));

  const QColor text = parseColor(token.colorText, QColor("#141414"));
  const QColor textDisabled = parseColor(token.colorTextDisabled, QColor("#8c8c8c"));
  const QColor textPlaceholder = parseColor(token.colorTextPlaceholder, QColor("#bfbfbf"));
  const QColor textLightSolid = parseColor(token.colorTextLightSolid, QColor("#ffffff"));

  const QColor primary = parseColor(token.colorPrimary, QColor("#1677ff"));
  const QColor primaryHover = parseColor(token.colorPrimaryHover, QColor("#4096ff"));
  const QColor link = parseColor(token.colorLink, primary);
  const QColor linkActive = parseColor(token.colorLinkActive, primaryHover);

  palette.setColor(QPalette::Active, QPalette::Window, bgLayout);
  palette.setColor(QPalette::Active, QPalette::Base, bgContainer);
  palette.setColor(QPalette::Active, QPalette::AlternateBase, fillAlter);
  palette.setColor(QPalette::Active, QPalette::ToolTipBase, bgElevated);
  palette.setColor(QPalette::Active, QPalette::Button, bgContainer);

  palette.setColor(QPalette::Active, QPalette::Text, text);
  palette.setColor(QPalette::Active, QPalette::WindowText, text);
  palette.setColor(QPalette::Active, QPalette::ButtonText, text);
  palette.setColor(QPalette::Active, QPalette::PlaceholderText, textPlaceholder);

  palette.setColor(QPalette::Active, QPalette::Highlight, primary);
  palette.setColor(QPalette::Active, QPalette::HighlightedText, textLightSolid);
  palette.setColor(QPalette::Active, QPalette::Link, link);
  palette.setColor(QPalette::Active, QPalette::LinkVisited, linkActive);

  palette.setColor(QPalette::Inactive, QPalette::Window, bgLayout);
  palette.setColor(QPalette::Inactive, QPalette::Base, bgContainer);
  palette.setColor(QPalette::Inactive, QPalette::AlternateBase, fillAlter);
  palette.setColor(QPalette::Inactive, QPalette::ToolTipBase, bgElevated);
  palette.setColor(QPalette::Inactive, QPalette::Button, bgContainer);

  palette.setColor(QPalette::Inactive, QPalette::Text, text);
  palette.setColor(QPalette::Inactive, QPalette::WindowText, text);
  palette.setColor(QPalette::Inactive, QPalette::ButtonText, text);
  palette.setColor(QPalette::Inactive, QPalette::PlaceholderText, textPlaceholder);

  palette.setColor(QPalette::Inactive, QPalette::Highlight, primary);
  palette.setColor(QPalette::Inactive, QPalette::HighlightedText, textLightSolid);
  palette.setColor(QPalette::Inactive, QPalette::Link, link);
  palette.setColor(QPalette::Inactive, QPalette::LinkVisited, linkActive);

  palette.setColor(QPalette::Disabled, QPalette::Window, bgLayout);
  palette.setColor(QPalette::Disabled, QPalette::Base, bgContainerDisabled);
  palette.setColor(QPalette::Disabled, QPalette::AlternateBase, fillAlter);
  palette.setColor(QPalette::Disabled, QPalette::ToolTipBase, bgElevated);
  palette.setColor(QPalette::Disabled, QPalette::Button, bgContainerDisabled);

  palette.setColor(QPalette::Disabled, QPalette::Text, textDisabled);
  palette.setColor(QPalette::Disabled, QPalette::WindowText, textDisabled);
  palette.setColor(QPalette::Disabled, QPalette::ButtonText, textDisabled);
  palette.setColor(QPalette::Disabled, QPalette::PlaceholderText, textDisabled);

  palette.setColor(QPalette::Disabled, QPalette::Highlight, primaryHover);
  palette.setColor(QPalette::Disabled, QPalette::HighlightedText, textLightSolid);
  palette.setColor(QPalette::Disabled, QPalette::Link, link);
  palette.setColor(QPalette::Disabled, QPalette::LinkVisited, linkActive);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  palette.setColor(QPalette::Active, QPalette::Accent, primary);
  palette.setColor(QPalette::Inactive, QPalette::Accent, primary);
  palette.setColor(QPalette::Disabled, QPalette::Accent, primaryHover);
#endif

  return palette;
}

}  // namespace adqt::theme
