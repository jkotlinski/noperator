#include "screen.h"

#include <QBrush>
#include <QDebug>
#include <QPainter>
#include <QPaintEvent>
#include <QRgb>

#include "vicpalette.h"

Screen::Screen(QWidget *parent) :
    QWidget(parent)
{
}

static void draw(QPainter *painter, int column, int row, int bgColorIndex, int fgColorIndex, int ch) {
    Q_ASSERT(bgColorIndex >= 0 && bgColorIndex < 16);
    Q_ASSERT(fgColorIndex >= 0 && fgColorIndex < 16);
    static QByteArray charrom;
    if (charrom.isEmpty()) {
        QFile f(":/charrom.bin");
        f.open(QFile::ReadOnly);
        charrom = f.readAll();
        Q_ASSERT(charrom.size() == 2 * 256 * 8);
    }
    QColor bg(vicPalette[bgColorIndex]);
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
    painter.scale(width() / 320.f, height() / 200.f);
    for (size_t i = 0; i < 16; ++i) {
        const QColor color(vicPalette[i]);
        painter.setPen(color);
        painter.setBrush(QBrush(color));
        painter.drawRect(i * 16, 4, 16, 16);
    }

    int ch = 0;
    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 40; ++x) {
            draw(&painter, x, y, 2, 3, ch++);
        }
    }
}
