#pragma once

class Screen;

class PutChar
{
public:
    void put(unsigned char ch);

    void hideCursor();
    void showCursor();

    void setScreen(Screen *screen) {
        this->screen = screen;
    }

private:
    int x = 0;
    int y = 0;
    int fgColor = 1;
    unsigned char reverse = 0;
    Screen *screen;

    void print(unsigned char ch);
    void cursorDown();

    int hiddenCursorColor = 0;
    int hiddenCursorChar = 0;
};
