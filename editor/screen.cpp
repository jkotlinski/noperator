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

void Screen::draw(QImage *image, int column, int row) {
    static QByteArray charrom;
    if (charrom.isEmpty()) {
        QFile f(":/charrom.bin");
        f.open(QFile::ReadOnly);
        charrom = f.readAll();
        Q_ASSERT(charrom.size() == 2 * 256 * 8);
    }
    const QRgb bg(vicPalette[bgColor & 15]);
    const QRgb fg(vicPalette[fgColor[column][row] & 15]);
    for (int y = row * 8; y < row * 8 + 8; ++y) {
        const char romchar = charrom.at(chars[column][row] * 8 + y % 8);
        for (int x = column * 8; x < column * 8 + 8; ++x) {
            const bool set = (0x80 >> (x % 8)) & romchar;
            image->setPixel(x, y, set ? fg : bg);
        }
    }
}

void Screen::paintEvent(QPaintEvent *event) {
    (void)event;

    QImage image(320, 200, QImage::Format_RGB32);
    for (int y = 0; y < 25; ++y) {
        for (int x = 0; x < 40; ++x) {
            draw(&image, x, y);
        }
    }

    QPainter painter(this);
    painter.fillRect(0, 0, width(), height(), vicPalette[borderColor]);
    painter.translate(borderMargin, borderMargin);
    painter.scale((width() - borderMargin * 2) / 320.f, (height() - borderMargin * 2) / 200.f);
    painter.drawImage(0, 0, image);
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
    for (int x = 0; x < 40; ++x) {
        for (int y = 0; y < 24; ++y) {
            chars[x][y] = chars[x][y + 1];
            fgColor[x][y] = fgColor[x][y + 1];
        }
    }
    for (int x = 0; x < 40; ++x) {
        chars[x][24] = ' ';
    }
}

void Screen::moveLeft() {
    for (int x = 0; x < 39; ++x) {
        for (int y = 0; y < 25; ++y) {
            chars[x][y] = chars[x + 1][y];
            fgColor[x][y] = fgColor[x + 1][y];
        }
    }
    for (int y = 0; y < 25; ++y) {
        chars[39][y] = ' ';
    }
}

void Screen::moveDown() {
    for (int x = 0; x < 40; ++x) {
        for (int y = 24; y > 0; --y) {
            chars[x][y] = chars[x][y - 1];
            fgColor[x][y] = fgColor[x][y - 1];
        }
    }
    for (int x = 0; x < 40; ++x) {
        chars[x][0] = ' ';
    }
}

void Screen::moveRight() {
    for (int x = 39; x > 0; --x) {
        for (int y = 0; y < 25; ++y) {
            chars[x][y] = chars[x - 1][y];
            fgColor[x][y] = fgColor[x - 1][y];
        }
    }
    for (int y = 0; y < 25; ++y) {
        chars[0][y] = ' ';
    }
}
