#pragma once

#include <QWidget>

class Screen : public QWidget
{
    Q_OBJECT
public:
    explicit Screen(QWidget *parent = 0);

    void paintEvent(QPaintEvent *event);

    QSize minimumSizeHint() const;

    void init();

signals:

public slots:

private:
    void draw(QPainter *painter, int column, int row);
    int bgColor = 0;
    unsigned char fgColor[40][25] = {{ 0 }};
    int borderColor = 0;
    unsigned char chars[40][25] = {{ ' ' }};
};
