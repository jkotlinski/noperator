#pragma once

#include <QWidget>

class Screen : public QWidget
{
    Q_OBJECT
public:
    explicit Screen(QWidget *parent = 0);

    void paintEvent(QPaintEvent *event);

    QSize minimumSizeHint() const {
        return QSize(640, 400);
    }

signals:

public slots:

};
