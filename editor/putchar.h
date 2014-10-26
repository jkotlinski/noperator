#pragma once

class Screen;

class PutChar
{
public:
    void put(Screen *screen, unsigned char ch);

private:
    int x = 0;
    int y = 0;
    int fgColor = 1;
    unsigned char reverse = 0;

    void print(Screen *screen, unsigned char ch);
};
