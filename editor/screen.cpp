#include "screen.h"

#include <QBrush>
#include <QDebug>
#include <QPainter>
#include <QPaintEvent>
#include <QRgb>

#include "vicpalette.h"

const int borderMargin = 64;
static int bgColor = 1;
static int borderColor = 2;

Screen::Screen(QWidget *parent) :
    QWidget(parent)
{
}

static void draw(QPainter *painter, int column, int row, int fgColorIndex, int ch) {
    Q_ASSERT(bgColor >= 0 && bgColor < 16);
    Q_ASSERT(fgColorIndex >= 0 && fgColorIndex < 16);
    static QByteArray charrom;
    if (charrom.isEmpty()) {
        QFile f(":/charrom.bin");
        f.open(QFile::ReadOnly);
        charrom = f.readAll();
        Q_ASSERT(charrom.size() == 2 * 256 * 8);
    }
    QColor bg(vicPalette[bgColor]);
    QColor fg(vicPalette[fgColorIndex]);
    for (int y = row * 8; y < row * 8 + 8; ++y) {
        const char romchar = charrom.at(ch * 8 + y % 8);
        for (int x = column * 8; x < column * 8 + 8; ++x) {
            const bool set = (0x80 >> (x % 8)) & romchar;
            painter->setPen(set ? bg : fg);
            painter->drawPoint(x, y);
        }
    }
}

void Screen::paintEvent(QPaintEvent *event) {
    (void)event;
    QPainter painter(this);
    painter.fillRect(0, 0, width(), height(), vicPalette[borderColor]);
    painter.translate(borderMargin, borderMargin);
    painter.scale((width() - borderMargin * 2) / 320.f, (height() - borderMargin * 2) / 200.f);

    int ch = 0;
    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 40; ++x) {
            draw(&painter, x, y, 3, ch++);
        }
    }
}

QSize Screen::minimumSizeHint() const {
    return QSize(640 + borderMargin * 2, 400 + borderMargin * 2);
}
