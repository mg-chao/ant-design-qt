#include <QApplication>
#include <QDebug>
#include <QFont>
#include <QFontMetrics>

static void printMetrics(const char* label, const QFont& font) {
  const QFontMetrics fm(font);
  qDebug().noquote() << label << "family=" << font.family() << "pixelSize=" << font.pixelSize()
                     << "height=" << fm.height() << "lineSpacing=" << fm.lineSpacing()
                     << "ascent=" << fm.ascent() << "descent=" << fm.descent()
                     << "leading=" << fm.leading();
}

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);

  QFont medium = app.font();
  medium.setPixelSize(14);
  printMetrics("medium", medium);

  QFont large = app.font();
  large.setPixelSize(16);
  printMetrics("large", large);

  QFont small = app.font();
  small.setPixelSize(14);
  printMetrics("small", small);

  return 0;
}
