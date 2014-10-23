#pragma once

class QAudioOutput;
class SidDevice;

class SidPlayer
{
public:
    SidPlayer();
    ~SidPlayer();

    void start();

private:
    QAudioOutput* audioOutput;
    SidDevice* sidDevice;
};
