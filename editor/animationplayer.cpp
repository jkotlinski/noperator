#include "animationplayer.h"

#include "animation.h"
#include "screen.h"

AnimationPlayer::AnimationPlayer(Animation *animation, Screen *screen)
    : animation(animation),
      screen(screen) {
    connect(&timer, SIGNAL(timeout()), this, SLOT(tick()));
}

void AnimationPlayer::start() {
    timer.start(1000);
}

void AnimationPlayer::tick() {
    animation->step(screen);
    screen->update();  // repaint hopefully not needed
}
