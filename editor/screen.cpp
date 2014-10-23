#include "screen.h"

#include <QBrush>
#include <QDebug>
#include <QPainter>
#include <QPaintEvent>

Screen::Screen(QWidget *parent) :
    QWidget(parent)
{
}

void Screen::paintEvent(QPaintEvent *event) {
    (void)event;
    QPainter painter(this);
    painter.setPen(Qt::green);
    painter.setBrush(QBrush(Qt::green));
    painter.drawRect(4, 4, 4, 4);

}
