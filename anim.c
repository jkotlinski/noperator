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

#include <string.h>
#include <c64.h>
#include <conio.h>
#include <time.h>

#include "disk.h"
#include "handle.h"
#include "keybuf.h"
#include "keyframe.h"
#include "myload.h"
#include "screen.h"

#define DISPLAY_BASE ((char*)0x400)

unsigned char hidden_color;
unsigned char hidden_char;

static unsigned int offset() {
    return cury() * 40 + curx();
}
static char* colptr() {
    return (unsigned char*)(0xd800 + offset());
}
static char* charptr() {
    return (unsigned char*)(0x400 + offset());
}

static void hide_cursor(void) {
    *charptr() = hidden_char;
    *colptr() = hidden_color;
}

static void show_cursor(void) {
    hidden_char = *charptr();
    hidden_color = *colptr();
    *colptr() = color;
    *charptr() = *charptr() ^ 0x80u;
}

unsigned char run_length;
char prev_ch;

void anim_reset() {
    cursor_home();
    prev_ch = 0;
    run_length = 0;
    color = COLOR_WHITE;
    init_screen();
}

void __fastcall__ startirq(void);
static void init(void) {
    anim_reset();
    show_cursor();
    // startirq();
}

// -----

static void do_store(char ch) {
    *last_char++ = ch;
    /* running out of RAM warning */
    if (last_char == (char*)0xcf00) *(char*)0xd020 = COLOR_YELLOW;
}

#define RLE_MARKER 0

static void flush_rle() {
    switch (run_length) {
        default:
            do_store(RLE_MARKER);
            do_store(prev_ch);
            do_store(run_length);
            break;
        case 3: do_store(prev_ch);
        case 2: do_store(prev_ch);
        case 1: do_store(prev_ch);
        case 0: break;
    }
}

static void store_char(char ch) {
    if (prev_ch == ch) {
        ++run_length;
    } else {
        flush_rle();
        run_length = 1;
        prev_ch = ch;
    }
}

static void run();

static void save() {
    prompt_save_anim(last_char - KEYS_START);
}

static void load()
{
    unsigned int read = prompt_load_anim();
    if (read)
        last_char = KEYS_START + read;
}

static void pause_one_clock()
{
    clock_t now = clock();
    while (now == clock());
}

static void insert_keyframe()
{
    ++*(char*)0xd020;
    store_char(0x13);  /* HOME */
    store_char(KEYFRAME_SPEED_NONE);
    store_char(KEYFRAME_SPEED_NONE >> 8);
    pause_one_clock();
    pause_one_clock();
    --*(char*)0xd020;
}

static void run() {
    char* ptr = KEYS_START;
    playback_mode = 1;
    anim_reset();
    while (ptr < last_char) {
        char ch = *ptr;
        switch (ch) {
            case 0x13: /* HOME */
                ptr += 3;  /* skip keyframe */
                break;
            default:
                handle(ch, 1);
                ++ptr;
        }
    }
    playback_mode = 0;
}

static unsigned char blink_on;
static void unblink() {
    if (blink_on) {
        show_cursor();
        blink_on = 0;
    }
}
static void blink() {
    if (blink_on) {
        unblink();
    } else {
        hide_cursor();
        blink_on = 1;
    }
}

static void editloop(void) {
    char first_keypress = 1;
    unsigned char ticks_since_last_key;
    while (last_char < (char*)0xd000) {
        static unsigned char blink_delay = 1;
        pause_one_clock();
        if (--blink_delay == 0) {
            blink();
            blink_delay = 10;
        }
        if (kbhit()) {
            unsigned char ch = cgetc();
            unblink();
            hide_cursor();
            switch (ch) {
                case CH_F1:
                    load();
                    run();
                    break;
                case CH_F2:
                    flush_rle();
                    save();
                    run();
                    break;
                case 0x83:
                    break;  /* STOP */
                case 0x13: /* HOME */
                    insert_keyframe();
                    break;
                case 0x93: break;  /* CLR */
                case 3:
                    run();
                    break;  /* RUN */
                default:
                    if (handle(ch, first_keypress))
                        store_char(ch);
                    break;
            }
            show_cursor();
            first_keypress = 0;
            ticks_since_last_key = 0;
        } else if (!first_keypress && ++ticks_since_last_key == 19) {
            first_keypress = 1;
        }
    }
}

void anim_editor(void) {
    init();
    playback_mode = 0;
    insert_keyframe();
    editloop();
}
