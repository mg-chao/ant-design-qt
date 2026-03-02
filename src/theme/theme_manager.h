#pragma once

#include "theme_algorithms.h"
#include "theme_palette.h"
#include "theme_types.h"

#include <QApplication>
#include <QFont>
#include <QObject>
#include <QPointer>
#include <QtGlobal>

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
  quint64 themeRevision() const;

  void applyTo(QApplication& app);

 signals:
  void themeChanged();

 private:
  explicit ThemeManager(QObject* parent = nullptr);
  void applyAppFont(QApplication& app) const;

  Q_DISABLE_COPY_MOVE(ThemeManager)

  ThemeConfig config_;
  ThemeMapToken mapToken_;
  GlobalPaletteToken token_;
  QPalette palette_;
  QByteArray key_;
  QPointer<QApplication> app_;
  QFont baseFont_;
  bool hasBaseFont_ = false;
  quint64 revision_ = 1;
};

}  // namespace adqt::theme
