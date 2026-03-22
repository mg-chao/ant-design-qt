#pragma once

#include <QPointer>
#include <QScrollArea>

class QScrollBar;

namespace adqt::widgets {

class AdScrollArea final : public QScrollArea {
  Q_OBJECT

  Q_PROPERTY(bool fitToWidth READ fitToWidth WRITE setFitToWidth NOTIFY fitToWidthChanged)
  Q_PROPERTY(int scrollBarThickness READ scrollBarThickness WRITE setScrollBarThickness NOTIFY scrollBarThicknessChanged)
  Q_PROPERTY(int scrollBarRadius READ scrollBarRadius WRITE setScrollBarRadius NOTIFY scrollBarRadiusChanged)

 public:
  explicit AdScrollArea(QWidget* parent = nullptr);
  ~AdScrollArea() override = default;

  static void applyThemedScrollBar(QScrollBar* bar,
                                   int extent = 8,
                                   int radius = 4,
                                   int inset = 0,
                                   int marginStart = 0,
                                   int marginEnd = 0);

  void setContentWidget(QWidget* widget);
  QWidget* contentWidget() const;

  bool fitToWidth() const;
  void setFitToWidth(bool value);

  int scrollBarThickness() const;
  void setScrollBarThickness(int value);

  int scrollBarRadius() const;
  void setScrollBarRadius(int value);

 signals:
  void fitToWidthChanged(bool value);
  void scrollBarThicknessChanged(int value);
  void scrollBarRadiusChanged(int value);

 protected:
  bool eventFilter(QObject* watched, QEvent* event) override;
  void changeEvent(QEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;

 private:
  void applyScrollBarStyle();
  void syncContentSize();
  void syncOverlayScrollBar();
  void updateOverlayGeometry();

  QPointer<QWidget> contentWidget_;
  QPointer<QScrollBar> overlayVerticalScrollBar_;
  bool overlayHovered_ = false;
  bool fitToWidth_ = true;
  int scrollBarThickness_ = 8;
  int scrollBarRadius_ = 4;
  bool syncingContentSize_ = false;
};

}  // namespace adqt::widgets
