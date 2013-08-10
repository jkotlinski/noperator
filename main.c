/* {{{ Copyright (c) 2013, Johan Kotlinski

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

#include <string.h>
#include <c64.h>
#include <conio.h>
#include <time.h>

#define SPACE 32

char curx;
char cury;
unsigned char color = COLOR_WHITE;
unsigned char hidden_color;

static void hide_cursor(void) {
    unsigned int i = cury * 40 + curx;
    unsigned char* colptr = (unsigned char*)(0xd800 + i);
    *colptr = hidden_color;
    *(char*)(0x400 + i) ^= 0x80;
}

static void show_cursor(void) {
    unsigned int i = cury * 40 + curx;
    unsigned char* colptr = (unsigned char*)(0xd800 + i);
    hidden_color = *colptr;
    *colptr = color;
    *(char*)(0x400 + i) ^= 0x80;
}

void __fastcall__ startirq(void);
static void init(void) {
    *(int*)0xd020 = 0;
    memset((char*)0x400, SPACE, 40 * 25);

    // startirq();

    show_cursor();

    *(char*)0xd018 &= ~2;  // uppercase + gfx
}

static void cur_up() {
    if (cury) {
        --cury;
    }
}

static void cur_down() {
    if (cury != 24) {
        ++cury;
    }
}

static void cur_left() {
    if (curx) {
        --curx;
    }
}

static void screen_left() {
    char* ptr;
    for (ptr = (char*)0x400; ptr < (char*)(0x400 + 40 * 25); ptr += 40) {
        memmove(ptr, ptr + 1, 39);
        memmove(ptr + 0xd400, ptr + 0xd401, 39);
        ptr[39] = SPACE;
    }
}

static void cur_right() {
    if (curx != 39) {
        ++curx;
    }
}

unsigned char reverse;

static void emit_char(unsigned char ch) {
    unsigned int i = cury * 40 + curx;
    ch ^= reverse;
    /* calculate screencode */
    if (ch < 0x20) {
        ch ^= 0x80;
    } else if (ch < 0x40) {
    } else if (ch < 0x60) {
        ch += 0xc0;
    } else if (ch < 0x80) {
        ch += 0xe0;
    } else if (ch < 0xa0) {
        ch += 0x40;
    } else if (ch < 0xc0) {
        ch += 0xc0;
    } else if (ch != 0xff) {
        ch ^= 0x80;
    }
    *(unsigned char*)(0xd800 + i) = color;
    *(char*)(0x400 + i) = ch;
    cur_right();
}

static void switch_color(unsigned char col) {
    color = col;
}

static void editloop(void) {
    while(1) {
        clock_t now = clock();
        while (now == clock());
        if (kbhit()) {
            unsigned char ch = cgetc();
            hide_cursor();
            switch (ch) {
                case CH_DEL:
                    cur_left();
                    emit_char(SPACE);
                    cur_left();
                    break;
                case CH_ENTER:
                    curx = 0;
                    cur_down();
                    break;
                case CH_CURS_RIGHT:
                    cur_right();
                    break;
                case CH_CURS_DOWN:
                    cur_down();
                    break;
                case CH_CURS_UP:
                    cur_up();
                    break;
                case CH_CURS_LEFT:
                    cur_left();
                    break;
                case 0x12: reverse = 0x80u; break;
                case 0x92: reverse = 0; break;

                // Colors.
                case 0x05: switch_color(COLOR_WHITE); break;
                case 0x1c: switch_color(COLOR_RED); break;
                case 0x1e: switch_color(COLOR_GREEN); break;
                case 0x1f: switch_color(COLOR_BLUE); break;
                case 0x81: switch_color(COLOR_ORANGE); break;
                case 0x90: switch_color(COLOR_BLACK); break;
                case 0x95: switch_color(COLOR_BROWN); break;
                case 0x96: switch_color(COLOR_LIGHTRED); break;
                case 0x97: switch_color(COLOR_GRAY1); break;
                case 0x98: switch_color(COLOR_GRAY2); break;
                case 0x99: switch_color(COLOR_LIGHTGREEN); break;
                case 0x9a: switch_color(COLOR_LIGHTBLUE); break;
                case 0x9b: switch_color(COLOR_GRAY3); break;
                case 0x9c: switch_color(COLOR_PURPLE); break;
                case 0x9e: switch_color(COLOR_YELLOW); break;
                case 0x9f: switch_color(COLOR_CYAN); break;
                default:
                    emit_char(ch);
            }
            show_cursor();
        }
    }
}

void main(void) {
    init();
    editloop();
}
