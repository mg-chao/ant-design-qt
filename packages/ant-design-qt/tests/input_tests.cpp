#include <QHBoxLayout>
#include <QLabel>
#include <QSignalSpy>
#include <QToolButton>
#include <QtTest>

#define private public
#define protected public
#include "widgets/button.h"
#include "widgets/detail/button_grouping.h"
#include "widgets/field_group.h"
#include "widgets/input_line_edit.h"
#include "widgets/input_number.h"
#include "widgets/input_otp_edit.h"
#include "widgets/input_password_edit.h"
#include "widgets/input_text_edit.h"
#include "widgets/combo_box.h"
#undef protected
#undef private

namespace {

class DigitCountPolicy final : public adqt::widgets::AdInputTextPolicy {
 public:
  using adqt::widgets::AdInputTextPolicy::AdInputTextPolicy;

  int characterCount(const QString& text) const override {
    int count = 0;
    for (const QChar ch : text) {
      if (ch.isDigit()) {
        ++count;
      }
    }
    return count;
  }

  QString normalizeText(const QString& text, int maximumCharacterCount) const override {
    QString normalized;
    normalized.reserve(text.size());
    for (const QChar ch : text) {
      if (!ch.isDigit()) {
        continue;
      }
      normalized.append(ch);
      if (maximumCharacterCount > 0 && normalized.size() >= maximumCharacterCount) {
        break;
      }
    }
    return normalized;
  }

  QString formatCountLabel(const QString& text,
                           int currentCount,
                           int maximumCharacterCount) const override {
    Q_UNUSED(text)
    if (maximumCharacterCount > 0) {
      return QStringLiteral("digits:%1/%2").arg(currentCount).arg(maximumCharacterCount);
    }
    return QStringLiteral("digits:%1").arg(currentCount);
  }
};

class UppercaseTextPolicy final : public adqt::widgets::AdInputTextPolicy {
 public:
  using adqt::widgets::AdInputTextPolicy::AdInputTextPolicy;

  QString normalizeText(const QString& text, int maximumCharacterCount) const override {
    QString normalized = text.toUpper();
    if (maximumCharacterCount > 0) {
      normalized = normalized.left(maximumCharacterCount);
    }
    return normalized;
  }

  QString formatCountLabel(const QString& text,
                           int currentCount,
                           int maximumCharacterCount) const override {
    Q_UNUSED(text)
    if (maximumCharacterCount > 0) {
      return QStringLiteral("upper:%1/%2").arg(currentCount).arg(maximumCharacterCount);
    }
    return QStringLiteral("upper:%1").arg(currentCount);
  }
};

class UppercaseOtpFormatter final : public adqt::widgets::AdOtpCodeFormatter {
 public:
  using adqt::widgets::AdOtpCodeFormatter::AdOtpCodeFormatter;

  QString formatCode(const QString& value) const override { return value.toUpper(); }
};

class PipeSeparatorFactory final : public adqt::widgets::AdOtpSeparatorFactory {
 public:
  using adqt::widgets::AdOtpSeparatorFactory::AdOtpSeparatorFactory;

  QWidget* createSeparator(int index, QWidget* parent) const override {
    auto* label = new QLabel(QStringLiteral("|"), parent);
    label->setObjectName(QStringLiteral("otp-separator-%1").arg(index));
    return label;
  }
};

}  // namespace

class InputTests final : public QObject {
  Q_OBJECT

 private slots:
  void lineEditTextPolicyNormalizesAndFormatsCount();
  void textEditProgrammaticAndUserEditsFollowQtContract();
  void textEditHeightModeAndPolicyUpdateTogether();
  void passwordRevealActionIsKeyboardAccessible();
  void otpFormatterAndSeparatorFactoryUseQObjectPolicies();
  void fieldGroupRebuildsJoinedLayoutForVisibleControls();
  void fieldGroupWritesJoinStateToDynamicProperties();
  void fieldGroupAppliesJoinStateToSelectAndInputNumber();
};

void InputTests::lineEditTextPolicyNormalizesAndFormatsCount() {
  adqt::widgets::AdLineEdit edit;
  DigitCountPolicy policy(&edit);

  edit.setCountVisible(true);
  edit.setMaximumCharacterCount(3);
  edit.setTextPolicy(&policy);
  edit.setText(QStringLiteral("a1b2c3d4"));

  QCOMPARE(edit.text(), QStringLiteral("123"));
  QVERIFY(edit.countLabel_ != nullptr);
  QCOMPARE(edit.countLabel_->text(), QStringLiteral("digits:3/3"));
}

void InputTests::textEditProgrammaticAndUserEditsFollowQtContract() {
  adqt::widgets::AdTextEdit edit;
  edit.resize(320, 120);
  edit.show();
  QTest::qWait(1);

  QSignalSpy plainTextChangedSpy(&edit, &adqt::widgets::AdTextEdit::plainTextChanged);
  QSignalSpy textEditedSpy(&edit, &adqt::widgets::AdTextEdit::textEdited);

  edit.setPlainText(QStringLiteral("alpha"));
  QCOMPARE(edit.plainText(), QStringLiteral("alpha"));
  QCOMPARE(plainTextChangedSpy.count(), 1);
  QCOMPARE(textEditedSpy.count(), 0);

  edit.focusEditor(adqt::widgets::AdLineEdit::FocusSelection::End, false);
  QVERIFY(edit.hasFocus());
  QTest::keyClicks(&edit, "Z");

  QTRY_VERIFY(textEditedSpy.count() >= 1);
  QCOMPARE(edit.plainText(), QStringLiteral("alphaZ"));
  QCOMPARE(textEditedSpy.last().at(0).toString(), QStringLiteral("alphaZ"));
}

void InputTests::textEditHeightModeAndPolicyUpdateTogether() {
  adqt::widgets::AdTextEdit edit;
  UppercaseTextPolicy policy(&edit);
  edit.resize(360, 240);
  edit.show();
  QTest::qWait(1);

  edit.setCountVisible(true);
  edit.setMaxLength(16);
  edit.setMinimumVisibleRows(2);
  edit.setMaximumVisibleRows(5);
  edit.setTextPolicy(&policy);
  edit.setPlainText(QStringLiteral("hello\nworld"));

  QCOMPARE(edit.plainText(), QStringLiteral("HELLO\nWORLD"));
  QVERIFY(edit.countLabel_ != nullptr);
  QCOMPARE(edit.countLabel_->text(), QStringLiteral("upper:11/16"));

  edit.setHeightMode(adqt::widgets::AdTextEdit::HeightMode::FixedRows);
  const int fixedHeight = edit.minimumHeight();

  edit.setPlainText(QStringLiteral("one\ntwo\nthree\nfour"));
  edit.setHeightMode(adqt::widgets::AdTextEdit::HeightMode::AutoGrow);
  QCoreApplication::processEvents();
  QTest::qWait(1);

  QVERIFY(edit.minimumHeight() > fixedHeight);
}

void InputTests::passwordRevealActionIsKeyboardAccessible() {
  adqt::widgets::AdPasswordEdit edit;
  edit.resize(280, edit.sizeHint().height());
  edit.show();
  QTest::qWait(1);

  QToolButton* actionButton = edit.trailingActionButton();
  QVERIFY(actionButton != nullptr);
  QVERIFY(actionButton->isVisible());
  QCOMPARE(actionButton->focusPolicy(), Qt::StrongFocus);
  QCOMPARE(actionButton->accessibleName(), QStringLiteral("Show password"));
  QCOMPARE(edit.echoMode(), QLineEdit::Password);

  QTest::mouseClick(actionButton, Qt::LeftButton);

  QTRY_VERIFY(edit.textVisible());
  QCOMPARE(edit.echoMode(), QLineEdit::Normal);
  QCOMPARE(actionButton->accessibleName(), QStringLiteral("Hide password"));

  edit.setRevealActionVisible(false);
  QVERIFY(!actionButton->isVisible());
  QCOMPARE(actionButton->focusPolicy(), Qt::NoFocus);
}

void InputTests::otpFormatterAndSeparatorFactoryUseQObjectPolicies() {
  adqt::widgets::AdOtpEdit otp;
  UppercaseOtpFormatter formatter(&otp);
  PipeSeparatorFactory separatorFactory(&otp);

  QSignalSpy formatterSpy(&otp, &adqt::widgets::AdOtpEdit::codeFormatterChanged);
  QSignalSpy separatorSpy(&otp, &adqt::widgets::AdOtpEdit::separatorFactoryChanged);

  otp.setCodeFormatter(&formatter);
  otp.setSeparatorFactory(&separatorFactory);
  otp.setCode(QStringLiteral("ab12"));

  QCOMPARE(formatterSpy.count(), 1);
  QCOMPARE(separatorSpy.count(), 1);
  QCOMPARE(otp.codeFormatter(), &formatter);
  QCOMPARE(otp.separatorFactory(), &separatorFactory);
  QCOMPARE(otp.code(), QStringLiteral("AB12"));
  QVERIFY(otp.cellAt(0) != nullptr);
  QCOMPARE(otp.cellAt(0)->text(), QStringLiteral("A"));
  QVERIFY(!otp.cellAt(0)->accessibleName().isEmpty());
  QVERIFY(!otp.cellAt(0)->accessibleDescription().isEmpty());
  QVERIFY(otp.findChild<QLabel*>(QStringLiteral("otp-separator-0")) != nullptr);
}

void InputTests::fieldGroupRebuildsJoinedLayoutForVisibleControls() {
  adqt::widgets::AdFieldGroup group;
  auto* left = new adqt::widgets::AdLineEdit();
  auto* button = new adqt::widgets::AdButton(QStringLiteral("Go"));
  auto* right = new adqt::widgets::AdLineEdit();

  group.addControl(left);
  group.addControl(button);
  group.addControl(right);
  group.resize(480, 48);
  group.show();
  QTest::qWait(1);

  QCOMPARE(group.controlCount(), 3);
  QCOMPARE(group.controlAt(0), left);
  QCOMPARE(group.controlAt(1), button);
  QCOMPARE(group.controlAt(2), right);
  QVERIFY(!left->joinedLeft());
  QVERIFY(left->joinedRight());
  QVERIFY(right->joinedLeft());
  QVERIFY(!right->joinedRight());
  QCOMPARE(adqt::widgets::detail::buttonSegmentPosition(button),
           adqt::widgets::detail::SegmentPosition::Middle);

  auto* layout = qobject_cast<QHBoxLayout*>(group.layout());
  QVERIFY(layout != nullptr);
  QCOMPARE(layout->count(), 5);
  QVERIFY(layout->itemAt(1) != nullptr);
  QVERIFY(layout->itemAt(1)->spacerItem() != nullptr);

  button->hide();
  QCoreApplication::processEvents();

  QVERIFY(left->joinedRight());
  QVERIFY(right->joinedLeft());
  QCOMPARE(adqt::widgets::detail::buttonSegmentPosition(button),
           adqt::widgets::detail::SegmentPosition::Standalone);
  QTRY_COMPARE(layout->count(), 3);

  button->show();
  QCoreApplication::processEvents();

  QCOMPARE(adqt::widgets::detail::buttonSegmentPosition(button),
           adqt::widgets::detail::SegmentPosition::Middle);
  QTRY_COMPARE(layout->count(), 5);
}

void InputTests::fieldGroupWritesJoinStateToDynamicProperties() {
  adqt::widgets::AdFieldGroup group;
  auto* left = new QWidget();
  auto* middle = new QWidget();
  auto* right = new QWidget();

  group.addControl(left);
  group.addControl(middle);
  group.addControl(right);
  group.resize(360, 48);
  group.show();
  QTest::qWait(1);

  QVERIFY(left->property("joinedLeft").isValid());
  QVERIFY(left->property("joinedRight").isValid());
  QVERIFY(middle->property("joinedLeft").isValid());
  QVERIFY(middle->property("joinedRight").isValid());
  QVERIFY(right->property("joinedLeft").isValid());
  QVERIFY(right->property("joinedRight").isValid());

  QCOMPARE(left->property("joinedLeft").toBool(), false);
  QCOMPARE(left->property("joinedRight").toBool(), true);
  QCOMPARE(middle->property("joinedLeft").toBool(), true);
  QCOMPARE(middle->property("joinedRight").toBool(), true);
  QCOMPARE(right->property("joinedLeft").toBool(), true);
  QCOMPARE(right->property("joinedRight").toBool(), false);

  middle->hide();
  QCoreApplication::processEvents();

  QCOMPARE(left->property("joinedLeft").toBool(), false);
  QCOMPARE(left->property("joinedRight").toBool(), true);
  QCOMPARE(middle->property("joinedLeft").toBool(), false);
  QCOMPARE(middle->property("joinedRight").toBool(), false);
  QCOMPARE(right->property("joinedLeft").toBool(), true);
  QCOMPARE(right->property("joinedRight").toBool(), false);
}

void InputTests::fieldGroupAppliesJoinStateToSelectAndInputNumber() {
  adqt::widgets::AdFieldGroup group;
  auto* left = new adqt::widgets::AdComboBox();
  auto* middle = new adqt::widgets::AdLineEdit();
  auto* right = new adqt::widgets::AdInputNumber();

  left->setFixedWidth(140);
  middle->setFixedWidth(180);
  right->setFixedWidth(120);

  group.addControl(left);
  group.addControl(middle);
  group.addControl(right);
  group.resize(520, 48);
  group.show();
  QTest::qWait(1);

  QVERIFY(!left->joinedLeft());
  QVERIFY(left->joinedRight());
  QVERIFY(middle->joinedLeft());
  QVERIFY(middle->joinedRight());
  QVERIFY(right->joinedLeft());
  QVERIFY(!right->joinedRight());

  middle->hide();
  QCoreApplication::processEvents();

  QVERIFY(!left->joinedLeft());
  QVERIFY(left->joinedRight());
  QVERIFY(right->joinedLeft());
  QVERIFY(!right->joinedRight());
}

QObject* createInputTests() { return new InputTests(); }

#include "input_tests.moc"
