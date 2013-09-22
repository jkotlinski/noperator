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

#include "handle.h"

#include <conio.h>
#include <string.h>

#include "keys.h"
#include "rledec.h"

#define RLE_MARKER 0

char copy_mode;

unsigned char color = COLOR_WHITE;
char clipboard[40 * 25];
char clipboard_color[40 * 25];

/* (CLIP_X1, CLIP_Y1) = top left.
 * (CLIP_X2, CLIP_Y2) = bottom right.
 */
static char CLIP_X1 = 0xff;
static char CLIP_X2;
static char CLIP_Y1;
static char CLIP_Y2;

#define DISPLAY_BASE ((char*)0x400)

char playback_mode = 1;  // if set, some UI operations will be disabled
static void invert_copy_mark() {
    if (playback_mode) return;
    {
        const unsigned char x2 = ((CLIP_X1 < CLIP_X2) ? CLIP_X2 : CLIP_X1) + 1;
        const unsigned char y2 = ((CLIP_Y1 < CLIP_Y2) ? CLIP_Y2 : CLIP_Y1) + 1;
        unsigned char y1 = (CLIP_Y1 < CLIP_Y2) ? CLIP_Y1 : CLIP_Y2;
        while (y1 < y2) {
            unsigned char x1 = (CLIP_X1 < CLIP_X2) ? CLIP_X1 : CLIP_X2;
            unsigned char* ptr = DISPLAY_BASE + y1 * 40 + x1;
            x1 = x2 - x1;
            while (x1--) {
                *ptr ^= 0x80;
                ++ptr;
            }
            ++y1;
        }
    }
}

static char x;
static char y;

static void start_copy() {
    CLIP_X1 = x;
    CLIP_X2 = x;
    CLIP_Y1 = y;
    CLIP_Y2 = y;

    invert_copy_mark();
    copy_mode = 1;
}

static void paste() {
    char yi;
    char* src_color;
    char* src_char;
    char* dst_color;
    char* dst_char;
    if (CLIP_X1 == 0xff) return;

    // Pastes region.
    for (yi = CLIP_Y1; yi <= CLIP_Y2; ++yi) {
        const char dst_y = y + yi - CLIP_Y1;
        char width;
        if (dst_y >= 25) break;
        src_char = clipboard + yi * 40 + CLIP_X1;
        src_color = clipboard_color + yi * 40 + CLIP_X1;
        dst_char = DISPLAY_BASE + dst_y * 40 + x;
        dst_color = (char*)0xd800 + dst_y * 40 + x;
        
        width = CLIP_X2 - CLIP_X1 + 1;
        if (x + width >= 40) {
            width = 39 - x;
        }
        memcpy(dst_char, src_char, width);
        memcpy(dst_color, src_color, width);
    }
}

static void handle_copy(char ch)
{
    switch (ch) {
        case CH_CURS_DOWN:
            if (CLIP_Y2 < 24) {
                invert_copy_mark();
                ++CLIP_Y2;
                invert_copy_mark();
            }
            break;
        case CH_CURS_UP:
            if (CLIP_Y2) {
                invert_copy_mark();
                --CLIP_Y2;
                invert_copy_mark();
            }
            break;
        case CH_CURS_RIGHT:
            if (CLIP_X2 < 39) {
                invert_copy_mark();
                ++CLIP_X2;
                invert_copy_mark();
            }
            break;
        case CH_CURS_LEFT:
            if (CLIP_X2) {
                invert_copy_mark();
                --CLIP_X2;
                invert_copy_mark();
            }
            break;
        case CH_F5: /* copy done */
            {
                char bgcol = *(char*)0xd020;
                if (!playback_mode)
                    *(char*)0xd020 = 5;
                invert_copy_mark();
                // Copies screen to clipboard.
                memcpy(clipboard, DISPLAY_BASE, 40 * 25);
                memcpy(clipboard_color, (char*)0xd800, 40 * 25);
                // Orders coordinates.
                if (CLIP_X1 > CLIP_X2) {
                    const char tmp = CLIP_X1;
                    CLIP_X1 = CLIP_X2;
                    CLIP_X2 = tmp;
                }
                if (CLIP_Y1 > CLIP_Y2) {
                    const char tmp = CLIP_Y1;
                    CLIP_Y1 = CLIP_Y2;
                    CLIP_Y2 = tmp;
                }
                copy_mode = 0;
                *(char*)0xd020 = bgcol;
            }
            break;
    }
}

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

static char cur_up(char may_move_screen) {
    if (y) {
        --y;
    } else if (may_move_screen)
        screen_down();
    else
        return 0;
    return 1;
}

static char cur_down(char may_move_screen) {
    if (y != 24) {
        ++y;
    } else if (may_move_screen)
        screen_up();
    else
        return 0;
    return 1;
}

static char cur_left(char may_move_screen) {
    if (x)
        --x;
    else if (may_move_screen)
        screen_right();
    else
        return 0;
    return 1;
}

static char cur_right(char may_move_screen) {
    if (x != 39)
        ++x;
    else if (may_move_screen)
        screen_left();
    else
        return 0;
    return 1;
}

unsigned char reverse;

static void emit(unsigned char ch) {
    unsigned int i = y * 40 + x;
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

/* returns 1 if ch should be stored in stream */
unsigned char handle(unsigned char ch, char first_keypress) {
    if (copy_mode) {
        handle_copy(ch);
        return 1;
    }

    switch (ch) {
        case CH_F3: ++*(char*)0xd020; break;
        case CH_F4: ++*(char*)0xd021; break;
        case CH_F5: start_copy(); break;
        case CH_F6: paste(); break;
        case CH_F7: break;
        case CH_F8: break;
        case CH_DEL:
                   cur_left(0);
                   emit(' ');
                   cur_left(0);
                   break;
        case CH_ENTER:
                   x = 0;
                   cur_down(0);
                   break;
        case CH_CURS_RIGHT: return cur_right(first_keypress);
        case CH_CURS_DOWN: return cur_down(first_keypress);
        case CH_CURS_UP: return cur_up(first_keypress);
        case CH_CURS_LEFT: return cur_left(first_keypress);
        case CH_RVS_ON: reverse = 0x80u; break;
        case CH_RVS_OFF: reverse = 0; break;

        /* Colors. */
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

void handle_rle(unsigned char ch)
{
    unsigned char i = rle_dec(ch);
    while (i--)
        handle(rle_char(), 1);
}

unsigned char curx()
{
    return x;
}
unsigned char cury()
{
    return y;
}
void cursor_home()
{
    x = 0;
    y = 0;
}

// -----

static unsigned int offset() {
    return cury() * 40 + curx();
}
static char* colptr() {
    return (unsigned char*)(0xd800 + offset());
}
static char* charptr() {
    return (unsigned char*)(0x400 + offset());
}

static unsigned char hidden_color;
static unsigned char hidden_char;

void hide_cursor(void) {
    *charptr() = hidden_char;
    *colptr() = hidden_color;
}

void show_cursor(void) {
    hidden_char = *charptr();
    hidden_color = *colptr();
    *colptr() = color;
    *charptr() = *charptr() ^ 0x80u;
}
