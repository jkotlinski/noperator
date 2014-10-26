#pragma once

class Screen;

class PutChar
{
public:
    void put(Screen *screen, unsigned char ch);

private:
    int x = 0;
    int y = 0;
};
