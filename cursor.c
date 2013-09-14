/** {{{ Copyright (c) 2013, Johan Kotlinski

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE. }}} */

#include "cursor.h"

#include <string.h>

static char x;
static char y;

static void screen_left() {
    char *ch = (char*)0x400;
    do
    {
        memmove(ch, ch + 1, 39);
        memmove(ch + 0xd400, ch + 0xd401, 39);
        ch += 40;
        *(ch - 1) = ' ';
    } while (ch < (char*)0x400 + 40 * 25);
}

static void screen_right() {
    char *ch = (char*)0x400;
    do
    {
        memmove(ch + 1, ch, 39);
        memmove(ch + 0xd401, ch + 0xd400, 39);
        *ch = ' ';
        ch += 40;
    } while (ch < (char*)0x400 + 40 * 25);
}

static void screen_down() {
    memmove((char*)0x400 + 40, (char*)0x400, 40 * 25 - 40);
    memmove((char*)0xd800 + 40, (char*)0xd800, 40 * 25 - 40);
    memset((char*)0x400, ' ', 40);
}

static void screen_up() {
    memmove((char*)0x400, (char*)0x400 + 40, 40 * 25 - 40);
    memmove((char*)0xd800, (char*)0xd800 + 40, 40 * 25 - 40);
    memset((char*)0x400 + 40 * 24, ' ', 40);
}

char cur_up(char may_move_screen) {
    if (y) {
        --y;
    } else if (may_move_screen)
        screen_down();
    else
        return 0;
    return 1;
}

char cur_down(char may_move_screen) {
    if (y != 24) {
        ++y;
    } else if (may_move_screen)
        screen_up();
    else
        return 0;
    return 1;
}

char cur_left(char may_move_screen) {
    if (x)
        --x;
    else if (may_move_screen)
        screen_right();
    else
        return 0;
    return 1;
}

char cur_right(char may_move_screen) {
    if (x != 39)
        ++x;
    else if (may_move_screen)
        screen_left();
    else
        return 0;
    return 1;
}

char curx() { return x; }
char cury() { return y; }
void resetcurx() { x = 0; }
