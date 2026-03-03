#pragma once

#include <QFont>
#include <QFontMetrics>
#include <QEnterEvent>
#include <QHideEvent>
#include <QIcon>
#include <QMoveEvent>
#include <QPushButton>
#include <QResizeEvent>
#include <QShowEvent>
#include <QtGlobal>

#include "icons_types.h"

namespace adqt::widgets {

class AdButtonGroup;

class AdButton final : public QPushButton {
  Q_OBJECT

  Q_PROPERTY(Type type READ type WRITE setType NOTIFY typeChanged)
  Q_PROPERTY(Color color READ color WRITE setColor NOTIFY colorChanged)
  Q_PROPERTY(Variant variant READ variant WRITE setVariant NOTIFY variantChanged)
  Q_PROPERTY(Shape shape READ shape WRITE setShape NOTIFY shapeChanged)
  Q_PROPERTY(Size size READ size WRITE setSize NOTIFY sizeChanged)
  Q_PROPERTY(bool danger READ danger WRITE setDanger NOTIFY dangerChanged)
  Q_PROPERTY(bool ghost READ ghost WRITE setGhost NOTIFY ghostChanged)
  Q_PROPERTY(bool block READ block WRITE setBlock NOTIFY blockChanged)
  Q_PROPERTY(bool loading READ loading WRITE setLoading NOTIFY loadingChanged)
  Q_PROPERTY(int loadingDelay READ loadingDelay WRITE setLoadingDelay NOTIFY loadingDelayChanged)
  Q_PROPERTY(IconPlacement iconPlacement READ iconPlacement WRITE setIconPlacement NOTIFY iconPlacementChanged)
  Q_PROPERTY(bool autoInsertSpace READ autoInsertSpace WRITE setAutoInsertSpace NOTIFY autoInsertSpaceChanged)

 public:
  enum class Type {
    Default,
    Primary,
    Dashed,
    Link,
    Text,
  };
  Q_ENUM(Type)

  enum class Color {
    Default,
    Primary,
    Danger,
    Blue,
    Purple,
    Cyan,
    Green,
    Magenta,
    Pink,
    Red,
    Orange,
    Yellow,
    Volcano,
    Geekblue,
    Lime,
    Gold,
  };
  Q_ENUM(Color)

  enum class Variant {
    Outlined,
    Dashed,
    Solid,
    Filled,
    Text,
    Link,
  };
  Q_ENUM(Variant)

  enum class Shape {
    Default,
    Circle,
    Round,
    Square,
  };
  Q_ENUM(Shape)

  enum class Size {
    Large,
    Middle,
    Small,
  };
  Q_ENUM(Size)

  enum class IconPlacement {
    Start,
    End,
  };
  Q_ENUM(IconPlacement)

  explicit AdButton(QWidget* parent = nullptr);
  explicit AdButton(const QString& text, QWidget* parent = nullptr);
  ~AdButton() override;

  Type type() const;
  void setType(Type value);

  Color color() const;
  void setColor(Color value);

  Variant variant() const;
  void setVariant(Variant value);

  Shape shape() const;
  void setShape(Shape value);

  Size size() const;
  void setSize(Size value);

  bool danger() const;
  void setDanger(bool value);

  bool ghost() const;
  void setGhost(bool value);

  bool block() const;
  void setBlock(bool value);

  bool loading() const;
  void setLoading(bool value);

  int loadingDelay() const;
  void setLoadingDelay(int value);

  IconPlacement iconPlacement() const;
  void setIconPlacement(IconPlacement value);

  bool autoInsertSpace() const;
  void setAutoInsertSpace(bool value);

  adqt::icons::IconToken iconToken() const;
  void setIconToken(const adqt::icons::IconToken& value);

  adqt::icons::IconToken loadingIconToken() const;
  void setLoadingIconToken(const adqt::icons::IconToken& value);

  void setIcon(const QIcon& value) = delete;
  QIcon icon() const = delete;

  bool isLoadingVisible() const;

  void resetSizeOverride();

 signals:
  void typeChanged(Type value);
  void colorChanged(Color value);
  void variantChanged(Variant value);
  void shapeChanged(Shape value);
  void sizeChanged(Size value);
  void dangerChanged(bool value);
  void ghostChanged(bool value);
  void blockChanged(bool value);
  void loadingChanged(bool value);
  void loadingDelayChanged(int value);
  void iconPlacementChanged(IconPlacement value);
  void autoInsertSpaceChanged(bool value);
  void iconTokenChanged(const adqt::icons::IconToken& value);
  void loadingIconTokenChanged(const adqt::icons::IconToken& value);

 protected:
  void paintEvent(QPaintEvent* event) override;
  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;

  void changeEvent(QEvent* event) override;
  void enterEvent(QEnterEvent* event) override;
  void leaveEvent(QEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
  void keyReleaseEvent(QKeyEvent* event) override;
  void focusInEvent(QFocusEvent* event) override;
  void focusOutEvent(QFocusEvent* event) override;
  void moveEvent(QMoveEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void showEvent(QShowEvent* event) override;
  void hideEvent(QHideEvent* event) override;

 private:
  friend class AdButtonGroup;

  enum class GroupPosition {
    None,
    Only,
    First,
    Middle,
    Last,
  };

  struct ContentLayout;

  bool interactionBlocked() const;
  bool hasUserIcon() const;
  bool shouldApplyTwoCnSpacing(const QString& sourceText) const;
  QString renderText() const;

  void refreshAfterPropertyChange(bool updateGeometry = true);
  void updateLoadingVisualState();
  void updateSpinnerState();
  void applyBlockSizePolicy();
  void bumpGroupZOrder();
  void updateCursorForRole();
  void startWaveEffect();
  void stopWaveEffect();

  Size effectiveSize() const;
  bool usesExplicitSize() const;

  void setGroupPosition(GroupPosition position);
  void setGroupSizeContext(Size size, bool enabled);

  ContentLayout computeContentLayout(const QRect& contentRect,
                                     const QSize& iconSize,
                                     const QString& displayText,
                                     const QFontMetrics& fm,
                                     int iconGap,
                                     const QFont& contentFont,
                                     bool twoCnAutoSpacing) const;
  void drawSpinner(QPainter& painter, const QRect& iconRect, const QColor& color) const;

  Type type_ = Type::Default;
  Color color_ = Color::Default;
  Variant variant_ = Variant::Outlined;
  Shape shape_ = Shape::Default;
  Size size_ = Size::Middle;
  IconPlacement iconPlacement_ = IconPlacement::Start;

  bool colorExplicit_ = false;
  bool variantExplicit_ = false;
  bool sizeExplicit_ = false;

  bool danger_ = false;
  bool ghost_ = false;
  bool block_ = false;
  bool loading_ = false;
  int loadingDelay_ = -1;
  bool loadingVisible_ = false;
  bool autoInsertSpace_ = true;
  bool hovered_ = false;
  bool focusVisible_ = false;

  adqt::icons::IconToken iconToken_;
  adqt::icons::IconToken loadingIconToken_;
  bool spinnerSubscribed_ = false;

  GroupPosition groupPosition_ = GroupPosition::None;
  bool hasGroupSizeContext_ = false;
  Size groupSizeContext_ = Size::Middle;

  QSizePolicy baseSizePolicy_;
};

}  // namespace adqt::widgets
