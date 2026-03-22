#include <QMetaProperty>
#include <QSignalSpy>
#include <QVBoxLayout>
#include <QtTest>

#include "antd_icons.h"

#define private public
#include "widgets/tag.h"
#include "widgets/tag_group.h"
#include "widgets/tag_style.h"
#undef private

namespace {

using adqt::widgets::AdTag;
using adqt::widgets::AdTagGroup;
namespace tag_detail = adqt::widgets::detail;
namespace outlined_icons = adqt::icons::antd::outlined;

void showAndWait(QWidget* widget) {
  if (!widget) {
    return;
  }
  widget->show();
  QTest::qWait(1);
  QCoreApplication::processEvents();
}

}  // namespace

class TagTests final : public QObject {
  Q_OBJECT

 private slots:
  void exposesQtProperties();
  void closableTagEmitsAndAutoHides();
  void closeCanRemainVisible();
  void defaultClosableTagUsesIconOnlyHoverHighlight();
  void closableTagSupportsExplicitHoverBackgroundOverride();
  void presetColorVariantsMatchAntDesignTokens();
  void customColorVariantsMatchAntDesignRules();
  void statusSolidUsesStatusBorderColor();
  void checkableTagTogglesAndSuppressesCloseAffordance();
  void singleGroupAllowsDeselectToNone();
  void multipleGroupTracksValues();
};

void TagTests::exposesQtProperties() {
  AdTag tag;
  const QMetaObject* tagMeta = tag.metaObject();

  const int variantIndex = tagMeta->indexOfProperty("variant");
  QVERIFY(variantIndex >= 0);
  QVERIFY(tagMeta->property(variantIndex).isWritable());

  const int borderStyleIndex = tagMeta->indexOfProperty("borderStyle");
  QVERIFY(borderStyleIndex >= 0);
  QVERIFY(tagMeta->property(borderStyleIndex).isWritable());

  const int colorSchemeIndex = tagMeta->indexOfProperty("colorScheme");
  QVERIFY(colorSchemeIndex >= 0);
  QVERIFY(tagMeta->property(colorSchemeIndex).isWritable());

  const int customColorIndex = tagMeta->indexOfProperty("customColor");
  QVERIFY(customColorIndex >= 0);
  QVERIFY(tagMeta->property(customColorIndex).isWritable());

  const int closableIndex = tagMeta->indexOfProperty("closable");
  QVERIFY(closableIndex >= 0);
  QVERIFY(tagMeta->property(closableIndex).isWritable());

  QVERIFY(tag.setProperty("variant", QVariant::fromValue(AdTag::Variant::Solid)));
  QCOMPARE(tag.variant(), AdTag::Variant::Solid);
  QVERIFY(tag.setProperty("borderStyle", QVariant::fromValue(AdTag::BorderStyle::Dashed)));
  QCOMPARE(tag.borderStyle(), AdTag::BorderStyle::Dashed);
  QVERIFY(tag.setProperty("colorScheme", QVariant::fromValue(AdTag::ColorScheme::Success)));
  QCOMPARE(tag.colorScheme(), AdTag::ColorScheme::Success);
  QVERIFY(tag.setProperty("customColor", QColor(QStringLiteral("#ff4d4f"))));
  QCOMPARE(tag.customColor(), QColor(QStringLiteral("#ff4d4f")));
  QVERIFY(tag.setProperty("closable", true));
  QVERIFY(tag.closable());

  const adqt::icons::IconRef iconRef = outlined_icons::CheckCircle();
  QVERIFY(tag.setProperty("iconRef", QVariant::fromValue(iconRef)));
  QCOMPARE(tag.iconRef(), iconRef);

  AdTagGroup group;
  const QMetaObject* groupMeta = group.metaObject();
  const int selectionModeIndex = groupMeta->indexOfProperty("selectionMode");
  QVERIFY(selectionModeIndex >= 0);
  QVERIFY(groupMeta->property(selectionModeIndex).isWritable());
  QVERIFY(group.setProperty("selectionMode", QVariant::fromValue(AdTagGroup::SelectionMode::Multiple)));
  QCOMPARE(group.selectionMode(), AdTagGroup::SelectionMode::Multiple);

  const int selectedValuesIndex = groupMeta->indexOfProperty("selectedValues");
  QVERIFY(selectedValuesIndex >= 0);
  QVERIFY(groupMeta->property(selectedValuesIndex).isWritable());
}

void TagTests::closableTagEmitsAndAutoHides() {
  AdTag tag(QStringLiteral("Closable"));
  tag.setClosable(true);
  showAndWait(&tag);
  QVERIFY(tag.isVisible());

  QSignalSpy closeRequestedSpy(&tag, &AdTag::closeRequested);
  QSignalSpy closedSpy(&tag, &AdTag::closed);

  QTest::mouseClick(&tag, Qt::LeftButton, Qt::NoModifier, tag.closeButtonRect().center());

  QTRY_COMPARE(closeRequestedSpy.count(), 1);
  QTRY_COMPARE(closedSpy.count(), 1);
  QVERIFY(!tag.isVisible());
}

void TagTests::closeCanRemainVisible() {
  AdTag tag(QStringLiteral("Stay visible"));
  tag.setClosable(true);
  tag.setAutoHideOnClose(false);
  showAndWait(&tag);
  QVERIFY(tag.isVisible());

  QSignalSpy closeRequestedSpy(&tag, &AdTag::closeRequested);
  QSignalSpy closedSpy(&tag, &AdTag::closed);

  QTest::mouseClick(&tag, Qt::LeftButton, Qt::NoModifier, tag.closeButtonRect().center());

  QTRY_COMPARE(closeRequestedSpy.count(), 1);
  QCOMPARE(closedSpy.count(), 0);
  QVERIFY(tag.isVisible());
}

void TagTests::defaultClosableTagUsesIconOnlyHoverHighlight() {
  tag_detail::TagStyleInput input;
  input.closable = true;

  const tag_detail::TagVisualStyle style = tag_detail::resolveTagVisualStyle(input);

  QCOMPARE(style.closeHoverBackground.alpha(), 0);
}

void TagTests::closableTagSupportsExplicitHoverBackgroundOverride() {
  tag_detail::TagStyleInput input;
  input.closable = true;
  input.componentTokens.colors.closeHoverBackground = QColor(QStringLiteral("#123456"));

  const tag_detail::TagVisualStyle style = tag_detail::resolveTagVisualStyle(input);

  QCOMPARE(style.closeHoverBackground, QColor(QStringLiteral("#123456")));
}

void TagTests::presetColorVariantsMatchAntDesignTokens() {
  tag_detail::TagStyleInput filledInput;
  filledInput.colorScheme = AdTag::ColorScheme::Magenta;
  filledInput.variant = AdTag::Variant::Filled;

  const tag_detail::TagVisualStyle filledStyle = tag_detail::resolveTagVisualStyle(filledInput);
  QCOMPARE(filledStyle.backgroundColor, QColor(QStringLiteral("#fff0f6")));
  QCOMPARE(filledStyle.borderColor, QColor(Qt::transparent));
  QCOMPARE(filledStyle.contentColor, QColor(QStringLiteral("#c41d7f")));

  tag_detail::TagStyleInput outlinedInput = filledInput;
  outlinedInput.variant = AdTag::Variant::Outlined;

  const tag_detail::TagVisualStyle outlinedStyle = tag_detail::resolveTagVisualStyle(outlinedInput);
  QCOMPARE(outlinedStyle.backgroundColor, QColor(QStringLiteral("#fff0f6")));
  QCOMPARE(outlinedStyle.borderColor, QColor(QStringLiteral("#ffadd2")));
  QCOMPARE(outlinedStyle.contentColor, QColor(QStringLiteral("#c41d7f")));

  tag_detail::TagStyleInput solidInput = filledInput;
  solidInput.variant = AdTag::Variant::Solid;

  const tag_detail::TagVisualStyle solidStyle = tag_detail::resolveTagVisualStyle(solidInput);
  QCOMPARE(solidStyle.backgroundColor, QColor(QStringLiteral("#eb2f96")));
  QCOMPARE(solidStyle.borderColor, QColor(QStringLiteral("#eb2f96")));
  QCOMPARE(solidStyle.contentColor, QColor(QStringLiteral("#ffffff")));
}

void TagTests::customColorVariantsMatchAntDesignRules() {
  tag_detail::TagStyleInput filledInput;
  filledInput.colorScheme = AdTag::ColorScheme::Custom;
  filledInput.customColor = QColor(QStringLiteral("#f50"));
  filledInput.variant = AdTag::Variant::Filled;

  const tag_detail::TagVisualStyle filledStyle = tag_detail::resolveTagVisualStyle(filledInput);
  QCOMPARE(filledStyle.backgroundColor.name(QColor::HexRgb), QStringLiteral("#ffeee5"));
  QCOMPARE(filledStyle.borderColor, QColor(Qt::transparent));
  QCOMPARE(filledStyle.contentColor.name(QColor::HexRgb), QStringLiteral("#ff5500"));

  tag_detail::TagStyleInput outlinedInput = filledInput;
  outlinedInput.variant = AdTag::Variant::Outlined;

  const tag_detail::TagVisualStyle outlinedStyle = tag_detail::resolveTagVisualStyle(outlinedInput);
  QCOMPARE(outlinedStyle.backgroundColor.name(QColor::HexRgb), QStringLiteral("#ffeee5"));
  QCOMPARE(outlinedStyle.borderColor.name(QColor::HexRgb), QStringLiteral("#ff5500"));
  QCOMPARE(outlinedStyle.contentColor.name(QColor::HexRgb), QStringLiteral("#ff5500"));

  tag_detail::TagStyleInput solidInput = filledInput;
  solidInput.variant = AdTag::Variant::Solid;

  const tag_detail::TagVisualStyle solidStyle = tag_detail::resolveTagVisualStyle(solidInput);
  QCOMPARE(solidStyle.backgroundColor.name(QColor::HexRgb), QStringLiteral("#ff5500"));
  QCOMPARE(solidStyle.borderColor, QColor(Qt::transparent));
  QCOMPARE(solidStyle.contentColor.name(QColor::HexRgb), QStringLiteral("#ffffff"));
}

void TagTests::statusSolidUsesStatusBorderColor() {
  tag_detail::TagStyleInput input;
  input.colorScheme = AdTag::ColorScheme::Success;
  input.variant = AdTag::Variant::Solid;

  const tag_detail::TagVisualStyle style = tag_detail::resolveTagVisualStyle(input);
  QCOMPARE(style.backgroundColor, QColor(QStringLiteral("#52c41a")));
  QCOMPARE(style.borderColor, QColor(QStringLiteral("#52c41a")));
  QCOMPARE(style.contentColor, QColor(QStringLiteral("#ffffff")));
}

void TagTests::checkableTagTogglesAndSuppressesCloseAffordance() {
  AdTag tag(QStringLiteral("Checkable"));
  tag.setClosable(true);
  tag.setCheckable(true);
  showAndWait(&tag);

  QVERIFY(!tag.closeButtonVisible());
  QVERIFY(!tag.isChecked());

  QTest::mouseClick(&tag, Qt::LeftButton, Qt::NoModifier, tag.rect().center());
  QTRY_VERIFY(tag.isChecked());

  QTest::mouseClick(&tag, Qt::LeftButton, Qt::NoModifier, tag.rect().center());
  QTRY_VERIFY(!tag.isChecked());
}

void TagTests::singleGroupAllowsDeselectToNone() {
  AdTagGroup group;
  group.setOptions({{QStringLiteral("a"), QStringLiteral("Alpha"), {}, false},
                    {QStringLiteral("b"), QStringLiteral("Beta"), {}, false},
                    {QStringLiteral("c"), QStringLiteral("Gamma"), {}, false}});
  group.setSelectedValue(QStringLiteral("b"));
  showAndWait(&group);

  QVERIFY(group.itemTags_.size() == 3);
  QVERIFY(group.itemTags_.at(1)->isChecked());

  QSignalSpy selectedValueSpy(&group, &AdTagGroup::selectedValueChanged);
  QTest::mouseClick(group.itemTags_.at(1), Qt::LeftButton, Qt::NoModifier,
                    group.itemTags_.at(1)->rect().center());

  QTRY_VERIFY(!group.selectedValue().isValid());
  QTRY_COMPARE(selectedValueSpy.count(), 1);

  QTest::mouseClick(group.itemTags_.at(0), Qt::LeftButton, Qt::NoModifier,
                    group.itemTags_.at(0)->rect().center());
  QTRY_COMPARE(group.selectedValue(), QVariant(QStringLiteral("a")));
  QVERIFY(group.itemTags_.at(0)->isChecked());
  QVERIFY(!group.itemTags_.at(1)->isChecked());
}

void TagTests::multipleGroupTracksValues() {
  AdTagGroup group;
  group.setSelectionMode(AdTagGroup::SelectionMode::Multiple);
  group.setOptions({{QStringLiteral("movies"), QStringLiteral("Movies"), {}, false},
                    {QStringLiteral("books"), QStringLiteral("Books"), {}, false},
                    {QStringLiteral("music"), QStringLiteral("Music"), {}, false}});
  showAndWait(&group);

  QSignalSpy valuesSpy(&group, &AdTagGroup::selectedValuesChanged);

  QTest::mouseClick(group.itemTags_.at(0), Qt::LeftButton, Qt::NoModifier,
                    group.itemTags_.at(0)->rect().center());
  QTest::mouseClick(group.itemTags_.at(2), Qt::LeftButton, Qt::NoModifier,
                    group.itemTags_.at(2)->rect().center());

  QTRY_COMPARE(group.selectedValues(),
               QVariantList({QVariant(QStringLiteral("movies")), QVariant(QStringLiteral("music"))}));
  QVERIFY(valuesSpy.count() >= 2);

  QTest::mouseClick(group.itemTags_.at(0), Qt::LeftButton, Qt::NoModifier,
                    group.itemTags_.at(0)->rect().center());
  QTRY_COMPARE(group.selectedValues(), QVariantList({QVariant(QStringLiteral("music"))}));
}

int runTagTests(int argc, char** argv) {
  qRegisterMetaType<AdTag::Variant>();
  qRegisterMetaType<AdTag::BorderStyle>();
  qRegisterMetaType<AdTag::ColorScheme>();
  qRegisterMetaType<AdTagGroup::SelectionMode>();

  TagTests tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "tag_tests.moc"
