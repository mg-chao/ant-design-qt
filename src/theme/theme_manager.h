#pragma once

#include "theme_algorithms.h"
#include "theme_palette.h"
#include "theme_types.h"

#include <QApplication>
#include <QObject>
#include <QPointer>

namespace adqt::theme {

class ThemeManager final : public QObject {
  Q_OBJECT

 public:
  static ThemeManager& instance();

  void setTheme(const ThemeConfig& config);

  const ThemeConfig& currentConfig() const;
  const ThemeMapToken& currentMapToken() const;
  const GlobalPaletteToken& currentToken() const;
  const QPalette& currentPalette() const;

  void applyTo(QApplication& app);

 signals:
  void themeChanged();

 private:
  explicit ThemeManager(QObject* parent = nullptr);

  Q_DISABLE_COPY_MOVE(ThemeManager)

  ThemeConfig config_;
  ThemeMapToken mapToken_;
  GlobalPaletteToken token_;
  QPalette palette_;
  QByteArray key_;
  QPointer<QApplication> app_;
};

}  // namespace adqt::theme
