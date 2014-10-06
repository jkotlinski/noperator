#ifndef SIDPLAYER_H
#define SIDPLAYER_H

class QAudioOutput;
class SidDevice;

class SidPlayer
{
public:
    SidPlayer();

private:
    QAudioOutput* audioOutput;
    SidDevice* sidDevice;
};

#endif // SIDPLAYER_H
