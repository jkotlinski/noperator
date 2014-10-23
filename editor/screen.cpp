#include "screen.h"

#include <QBrush>
#include <QDebug>
#include <QPainter>
#include <QPaintEvent>
#include <QRgb>

#include "vicpalette.h"

const int borderMargin = 64;
static int bgColor = 1;
static unsigned char fgColor[40][25];
static int borderColor = 2;
static unsigned char chars[40][25];

Screen::Screen(QWidget *parent) :
    QWidget(parent)
{
}

static void draw(QPainter *painter, int column, int row) {
    static QByteArray charrom;
    if (charrom.isEmpty()) {
        QFile f(":/charrom.bin");
        f.open(QFile::ReadOnly);
        charrom = f.readAll();
        Q_ASSERT(charrom.size() == 2 * 256 * 8);
    }
    QColor bg(vicPalette[bgColor & 15]);
    QColor fg(vicPalette[fgColor[column][row] & 15]);
    for (int y = row * 8; y < row * 8 + 8; ++y) {
        const char romchar = charrom.at(chars[column][row] * 8 + y % 8);
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
    for (int y = 0; y < 25; ++y) {
        for (int x = 0; x < 40; ++x) {
            chars[x][y] = ch;
            fgColor[x][y] = ch;
            draw(&painter, x, y);
            ++ch;
        }
    }
}

QSize Screen::minimumSizeHint() const {
    return QSize(640 + borderMargin * 2, 400 + borderMargin * 2);
}
