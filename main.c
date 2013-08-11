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

static void reset_screen() {
    curx = 0;
    cury = 0;
    *(char*)0xd020 = 0;
    *(char*)0xd021 = 0;
    *(char*)0xd018 &= ~2;  // uppercase + gfx
    memset((char*)0x400, ' ', 40 * 25);
    color = COLOR_WHITE;
}

void __fastcall__ startirq(void);
static void init(void) {
    reset_screen();
    show_cursor();
    // startirq();
}

// -----

static void screen_left() {
    unsigned char* ptr;
    memmove((char*)0x400, (char*)0x401, 40 * 25 - 1);
    memmove((char*)0xd800, (char*)0xd801, 40 * 25 - 1);
    for (ptr = (char*)0x400 + 39; ptr < (char*)0x400 + 39 + 40 * 25; ptr += 40)
        *ptr = ' ';
}

static void screen_right() {
    unsigned char* ptr;
    memmove((char*)0x401, (char*)0x400, 40 * 25 - 1);
    memmove((char*)0xd801, (char*)0xd800, 40 * 25 - 1);
    for (ptr = (char*)0x400; ptr < (char*)0x400 + 40 * 25; ptr += 40)
        *ptr = ' ';
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

static char cur_up(char may_move_screen) {
    if (cury) {
        --cury;
    } else if (may_move_screen)
        screen_down();
    else
        return 0;
    return 1;
}

static char cur_down(char may_move_screen) {
    if (cury != 24) {
        ++cury;
    } else if (may_move_screen)
        screen_up();
    else
        return 0;
    return 1;
}

static char cur_left(char may_move_screen) {
    if (curx)
        --curx;
    else if (may_move_screen)
        screen_right();
    else
        return 0;
    return 1;
}

static char cur_right(char may_move_screen) {
    if (curx != 39)
        ++curx;
    else if (may_move_screen)
        screen_left();
    else
        return 0;
    return 1;
}

unsigned char reverse;

static void emit(unsigned char ch) {
    unsigned int i = cury * 40 + curx;
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
    ch ^= reverse;
    *(unsigned char*)(0xd800 + i) = color;
    *(char*)(0x400 + i) = ch;
    cur_right(0);
}

#define switch_color(col) color = col;

extern unsigned char _RAM_LAST__;  /* Defined by linker. */
static char* key_out = &_RAM_LAST__ + 1;

static void store_char(char ch) {
    *key_out++ = ch;
    /* running out of RAM warning */
    if (key_out == (char*)0xcf00) *(char*)0xd020 = COLOR_YELLOW;
}

static void run();

static unsigned char first_keypress;

/* returns 1 if ch should be stored in stream */
unsigned char handle(unsigned char ch) {
    switch (ch) {
        case 3: run(); return 0;  /* RUN */
        case 0x83: return 0;  /* STOP */
        case 0x13: break;  /* HOME */
        case 0x93: break;  /* CLR */
        case CH_DEL:
                   cur_left(0);
                   emit(' ');
                   cur_left(0);
                   break;
        case CH_ENTER:
                   curx = 0;
                   cur_down(0);
                   break;
        case CH_CURS_RIGHT: return cur_right(first_keypress);
        case CH_CURS_DOWN: return cur_down(first_keypress);
        case CH_CURS_UP: return cur_up(first_keypress);
        case CH_CURS_LEFT: return cur_left(first_keypress);
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
        case 0x80 | ' ':
                   reverse ^= 0x80;
                   emit(' ');
                   reverse ^= 0x80;
                   break;
        default: emit(ch);
    }
    return 1;
}

static void run() {
    char* ptr = &_RAM_LAST__ + 1;
    reset_screen();
    first_keypress = 1;
    while (ptr < key_out) {
        handle(*ptr++);
    }
}

static void editloop(void) {
    while (key_out < (char*)0xd000) {
        static unsigned char ticks_since_last_key;

        clock_t now = clock();
        while (now == clock());
        if (kbhit()) {
            unsigned char ch = cgetc();
            hide_cursor();
            if (handle(ch))
                store_char(ch);
            show_cursor();
            first_keypress = 0;
            ticks_since_last_key = 0;
        } else if (!first_keypress && ++ticks_since_last_key == 20) {
            first_keypress = 1;
        }
    }
}

void main(void) {
    init();
    editloop();
}
