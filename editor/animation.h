#pragma once

#include <QByteArray>

class PutChar;
class Screen;

class Animation
{
public:
    Animation();

    void step(Screen *screen);

private:
    QByteArray data;
    size_t index = 2;  // Skips adress.
    PutChar *putChar;
    int speed = 0;
    int getc();

    int rleChar = 0;
    int rleLeft = 0;
};
