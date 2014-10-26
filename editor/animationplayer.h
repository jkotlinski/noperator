#pragma once

#include <QTimer>

class Animation;
class Screen;

class AnimationPlayer : public QObject
{
    Q_OBJECT

public:
    AnimationPlayer(Animation *animation, Screen *screen);

    void start();

public slots:
    void tick();

private:
    Animation *animation;
    Screen *screen;
    QTimer timer;
};
