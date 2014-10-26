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

    void moveUp();

    void setColor(int x, int y, unsigned char color) {
        Q_ASSERT(x >= 0);
        Q_ASSERT(x < 40);
        Q_ASSERT(y >= 0);
        Q_ASSERT(y < 25);
        Q_ASSERT(color < 16);
        fgColor[x][y] = color;
    }

    void setChar(int x, int y, unsigned char ch) {
        Q_ASSERT(x >= 0);
        Q_ASSERT(x < 40);
        Q_ASSERT(y >= 0);
        Q_ASSERT(y < 25);
        chars[x][y] = ch;
    }

    int getColor(int x, int y) const {
        Q_ASSERT(x >= 0);
        Q_ASSERT(x < 40);
        Q_ASSERT(y >= 0);
        Q_ASSERT(y < 25);
        return fgColor[x][y];
    }

    int getChar(int x, int y) const {
        Q_ASSERT(x >= 0);
        Q_ASSERT(x < 40);
        Q_ASSERT(y >= 0);
        Q_ASSERT(y < 25);
        return chars[x][y];
    }

signals:

public slots:

private:
    void draw(QPainter *painter, int column, int row);
    int bgColor = 0;
    unsigned char fgColor[40][25] = {{ 0 }};
    int borderColor = 0;
    unsigned char chars[40][25] = {{ ' ' }};
};
