#pragma once

#include "radio.h"

#include <QVariant>
#include <QVector>
#include <QWidget>

#include <functional>

class QBoxLayout;

namespace adqt::widgets {

class AdRadioGroup final : public QWidget {
  Q_OBJECT

  Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged)
  Q_PROPERTY(bool disabled READ disabled WRITE setDisabled NOTIFY disabledChanged)
  Q_PROPERTY(adqt::widgets::AdRadio::Size size READ size WRITE setSize NOTIFY sizeChanged)
  Q_PROPERTY(adqt::widgets::AdRadio::OptionType optionType READ optionType WRITE setOptionType NOTIFY optionTypeChanged)
  Q_PROPERTY(adqt::widgets::AdRadio::ButtonStyle buttonStyle READ buttonStyle WRITE setButtonStyle NOTIFY buttonStyleChanged)
  Q_PROPERTY(bool block READ block WRITE setBlock NOTIFY blockChanged)
  Q_PROPERTY(Orientation orientation READ orientation WRITE setOrientation NOTIFY orientationChanged)
  Q_PROPERTY(bool vertical READ vertical WRITE setVertical NOTIFY verticalChanged)
  Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)

 public:
  enum class Orientation {
    Horizontal,
    Vertical,
  };
  Q_ENUM(Orientation)

  struct Option {
    QVariant value;
    QString label;
    bool disabled = false;
    QString title;
    QString id;
    QString className;
    bool required = false;
  };

  struct StyleContext {
    AdRadio::Size size = AdRadio::Size::Middle;
    AdRadio::OptionType optionType = AdRadio::OptionType::Default;
    AdRadio::ButtonStyle buttonStyle = AdRadio::ButtonStyle::Outline;
    bool disabled = false;
    bool block = false;
    Orientation orientation = Orientation::Horizontal;
    bool vertical = false;
    QVariant value;
    QVector<Option> options;
  };

  using SemanticStyleResolver = std::function<AdRadio::SemanticStyles(const StyleContext&)>;

  explicit AdRadioGroup(QWidget* parent = nullptr);
  ~AdRadioGroup() override;

  QVariant value() const;
  void setValue(const QVariant& value);

  bool disabled() const;
  void setDisabled(bool value);

  AdRadio::Size size() const;
  void setSize(AdRadio::Size value);

  AdRadio::OptionType optionType() const;
  void setOptionType(AdRadio::OptionType value);

  AdRadio::ButtonStyle buttonStyle() const;
  void setButtonStyle(AdRadio::ButtonStyle value);

  bool block() const;
  void setBlock(bool value);

  Orientation orientation() const;
  void setOrientation(Orientation value);

  bool vertical() const;
  void setVertical(bool value);

  QString name() const;
  void setName(const QString& value);

  QVector<Option> options() const;
  void setOptions(const QVector<Option>& options);

  AdRadio::ComponentTokens componentTokens() const;
  void setComponentTokens(const AdRadio::ComponentTokens& tokens);
  void resetComponentTokens();

  AdRadio::SemanticStyles semanticStyles() const;
  void setSemanticStyles(const AdRadio::SemanticStyles& styles);
  void setSemanticStyleResolver(SemanticStyleResolver resolver);

 signals:
  void valueChanged(const QVariant& value);
  void changed(const QVariant& value);
  void disabledChanged(bool value);
  void sizeChanged(adqt::widgets::AdRadio::Size value);
  void optionTypeChanged(adqt::widgets::AdRadio::OptionType value);
  void buttonStyleChanged(adqt::widgets::AdRadio::ButtonStyle value);
  void blockChanged(bool value);
  void orientationChanged(Orientation value);
  void verticalChanged(bool value);
  void nameChanged(const QString& value);
  void optionsChanged();
  void componentTokensChanged();
  void semanticStylesChanged();

 private:
  Orientation effectiveOrientation() const;
  StyleContext currentStyleContext() const;
  AdRadio::SemanticStyles resolvedSemanticStyles() const;
  void ensureLayoutForOrientation();
  void updateLayoutSpacing();
  void rebuildRadios();
  void applyGroupStateToRadios();
  void syncCheckedFromValue();
  void updateGroupPositions();
  void onRadioChanged(AdRadio* radio, const QVariant& value, bool checked);

  QVariant value_;
  bool disabled_ = false;
  AdRadio::Size size_ = AdRadio::Size::Middle;
  AdRadio::OptionType optionType_ = AdRadio::OptionType::Default;
  AdRadio::ButtonStyle buttonStyle_ = AdRadio::ButtonStyle::Outline;
  bool block_ = false;
  Orientation orientation_ = Orientation::Horizontal;
  bool vertical_ = false;
  bool orientationExplicit_ = false;
  QString name_;
  QVector<Option> options_;
  AdRadio::ComponentTokens componentTokens_;
  AdRadio::SemanticStyles semanticStyles_;
  SemanticStyleResolver semanticStyleResolver_;

  QBoxLayout* layout_ = nullptr;
  QVector<AdRadio*> radios_;
  bool syncing_ = false;
};

}  // namespace adqt::widgets

