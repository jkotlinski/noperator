#include "screen.h"

#include <QBrush>
#include <QDebug>
#include <QPainter>
#include <QPaintEvent>
#include <QRgb>

#include "vicpalette.h"

const int borderMargin = 64;

Screen::Screen(QWidget *parent) :
    QWidget(parent)
{
}

void Screen::draw(QPainter *painter, int column, int row) {
    static QByteArray charrom;
    if (charrom.isEmpty()) {
        QFile f(":/charrom.bin");
        f.open(QFile::ReadOnly);
        charrom = f.readAll();
        Q_ASSERT(charrom.size() == 2 * 256 * 8);
    }
    const QPen &bg(vicPens[bgColor & 15]);
    const QPen &fg(vicPens[fgColor[column][row] & 15]);
    for (int y = row * 8; y < row * 8 + 8; ++y) {
        const char romchar = charrom.at(chars[column][row] * 8 + y % 8);
        for (int x = column * 8; x < column * 8 + 8; ++x) {
            const bool set = (0x80 >> (x % 8)) & romchar;
            painter->setPen(set ? fg : bg);
            painter->drawPoint(x, y);
        }
    }
}

void Screen::paintEvent(QPaintEvent *event) {
    (void)event;
    QPainter painter(this);
    painter.fillRect(0, 0, width(), height(), vicPens[borderColor].color());
    painter.translate(borderMargin, borderMargin);
    painter.scale((width() - borderMargin * 2) / 320.f, (height() - borderMargin * 2) / 200.f);

    for (int y = 0; y < 25; ++y) {
        for (int x = 0; x < 40; ++x) {
            draw(&painter, x, y);
        }
    }
}

QSize Screen::minimumSizeHint() const {
    return QSize(640 + borderMargin * 2, 400 + borderMargin * 2);
}

void Screen::init() {
    bgColor = 0;
    borderColor = 0;
    memset(fgColor, 0, sizeof(fgColor));
    memset(chars, ' ', sizeof(chars));
}

void Screen::moveUp() {
    Q_ASSERT(!"not implemented");
}

void Screen::moveLeft() {
    Q_ASSERT(!"not implemented");
}

void Screen::moveDown() {
    Q_ASSERT(!"not implemented");
}

void Screen::moveRight() {
    Q_ASSERT(!"not implemented");
}
