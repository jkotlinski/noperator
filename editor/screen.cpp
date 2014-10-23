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

void Screen::paintEvent(QPaintEvent *event) {
    (void)event;
    QPainter painter(this);
    for (size_t i = 0; i < 16; ++i) {
        const QColor color(vicPalette[i]);
        painter.setPen(color);
        painter.setBrush(QBrush(color));
        painter.drawRect(i * 16, 4, 16, 16);
    }
}
