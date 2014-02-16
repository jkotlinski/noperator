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
#include "screen.h"

#define RLE_MARKER 0

static char copy_mode;

static unsigned char color = COLOR_WHITE;
static char clipboard[40 * 25];
static char clipboard_color[40 * 25];

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

static char x_;
static char y_;
static char* charptr = DISPLAY_BASE;
static char* colptr = (char*)0xd800;

static void start_copy() {
    CLIP_X1 = x_;
    CLIP_X2 = x_;
    CLIP_Y1 = y_;
    CLIP_Y2 = y_;

    invert_copy_mark();
    copy_mode = 1;
}

static void paste() {
    char width;
    char height;
    char* src_char = clipboard + CLIP_Y1 * 40 + CLIP_X1;
    /* Assumes clipboard_color is directly after clipboard. */
    char* src_color = src_char + sizeof(clipboard);
    char* dst_color = colptr;
    char* dst_char = charptr;

    if (CLIP_X1 == 0xff) {
        return;
    }

    height = CLIP_Y2 - CLIP_Y1 + 1;
    if (y_ + height >= 25) {
        height = 24 - y_;
    }
    width = CLIP_X2 - CLIP_X1 + 1;
    if (x_ + width >= 40) {
        width = 39 - x_;
    }

    do {
        memcpy(dst_char, src_char, width);
        memcpy(dst_color, src_color, width);

        src_char += 40;
        src_color += 40;
        dst_char += 40;
        dst_color += 40;
    } while (--height);
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
                if (!playback_mode) {
                    *(char*)0xd020 = 5;
                    invert_copy_mark();
                }
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
                if (!playback_mode)
                    *(char*)0xd020 = bgcol;
            }
            break;
    }
}

void screen_left_opt();
static void screen_left() {
    char *ch = (char*)0x400 + 40;
    do {
        *ch = ' ';
        ch += 40;
    } while (ch < (char*)0x400 + 40 * 25);
    screen_left_opt();
    *(char*)(0x400 + 40 * 25 - 1) = ' ';
}

void screen_right_opt();
static void screen_right() {
    char *ch = (char*)0x400 + 39;
    do {
        *ch = ' ';
        ch += 40;
    } while (ch < (char*)0x400 + 40 * 25 - 1);
    screen_right_opt();
    *(char*)0x400 = ' ';
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
    if (y_) {
        --y_;
        charptr -= 40;
        colptr -= 40;
    } else if (may_move_screen)
        screen_down();
    else
        return 0;
    return 1;
}

static void fast_cur_down() {
    if (y_ != 24) {
        ++y_;
        charptr += 40;
        colptr += 40;
    }
}

static char cur_down(char may_move_screen) {
    if (y_ != 24) {
        ++y_;
        charptr += 40;
        colptr += 40;
    } else if (may_move_screen)
        screen_up();
    else
        return 0;
    return 1;
}

static void fast_cur_left() {
    if (x_) {
        --x_;
        --charptr;
        --colptr;
    }
}

static char cur_left(char may_move_screen) {
    if (x_) {
        --x_;
        --charptr;
        --colptr;
    } else if (may_move_screen)
        screen_right();
    else
        return 0;
    return 1;
}

static char cur_right(char may_move_screen) {
    if (x_ != 39) {
        ++x_;
        ++charptr;
        ++colptr;
    } else if (may_move_screen)
        screen_left();
    else
        return 0;
    return 1;
}

static unsigned char reverse;

static void emit(unsigned char ch) {
    static const unsigned char screencode[256] = {
        0x80u, 0x81u, 0x82u, 0x83u, 0x84u, 0x85u, 0x86u, 0x87u,
        0x88u, 0x89u, 0x8au, 0x8bu, 0x8cu, 0x8du, 0x8eu, 0x8fu,
        0x90u, 0x91u, 0x92u, 0x93u, 0x94u, 0x95u, 0x96u, 0x97u,
        0x98u, 0x99u, 0x9au, 0x9bu, 0x9cu, 0x9du, 0x9eu, 0x9fu,
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
        0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
        0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
        0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7,
        0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
        0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
        0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
        0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
        0xc0u, 0xc1u, 0xc2u, 0xc3u, 0xc4u, 0xc5u, 0xc6u, 0xc7u,
        0xc8u, 0xc9u, 0xcau, 0xcbu, 0xccu, 0xcdu, 0xceu, 0xcfu,
        0xd0u, 0xd1u, 0xd2u, 0xd3u, 0xd4u, 0xd5u, 0xd6u, 0xd7u,
        0xd8u, 0xd9u, 0xdau, 0xdbu, 0xdcu, 0xddu, 0xdeu, 0xdfu,
        0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
        0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
        0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
        0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
        0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
        0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
        0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
        0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
        0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
        0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
        0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0xffu
    };
    *charptr = screencode[ch] ^ reverse;
    *colptr = color;

    /* Inlined cur_right. */
    if (x_ != 39) {
        ++x_;
        ++charptr;
        ++colptr;
    }
}

#define switch_color(col) color = col;

/* returns 1 if ch should be stored in stream */
unsigned char handle(unsigned char ch, char first_keypress) {
    if (copy_mode) {
        handle_copy(ch);
        return 1;
    }

    if ((ch & 0x70) > 0x10) {
        /* Normal char */
        if (ch != (0x80 | ' ')) {
            emit(ch);
        } else {
            reverse ^= 0x80;
            emit(' ');
            reverse ^= 0x80;
        }
    } else switch (ch) {
        case MOVIE_START_MARKER:
            init_screen();
            // Fall through.
        case 2:
            cursor_home();
            color = COLOR_WHITE;
            break;
        case CH_F3: ++*(char*)0xd020; break;
        case CH_F4: ++*(char*)0xd021; break;
        case CH_F5: start_copy(); break;
        case CH_F6: paste(); break;
        case CH_DEL:
                fast_cur_left();
                emit(' ');
                fast_cur_left();
                break;
        case CH_ENTER:
                   charptr -= x_;
                   colptr -= x_;
                   x_ = 0;
                   fast_cur_down();
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
    }
    return 1;
}

void handle_rle(unsigned char ch)
{
    unsigned char i = rle_dec(ch);
    while (i--)
        handle(rle_char, 1);
}

unsigned char curx()
{
    return x_;
}
unsigned char cury()
{
    return y_;
}
void cursor_home()
{
    x_ = 0;
    y_ = 0;
    charptr = DISPLAY_BASE;
    colptr = (char*)0xd800;
}

// -----

static unsigned char hidden_color;
static unsigned char hidden_char;

void hide_cursor(void) {
    *charptr = hidden_char;
    *colptr = hidden_color;
}

void show_cursor(void) {
    hidden_color = *colptr;
    *colptr = color;
    hidden_char = *charptr;
    *charptr ^= 0x80u;
}
