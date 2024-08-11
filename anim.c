#include <string.h>
#include <c64.h>
#include <conio.h>
#include <time.h>

#include "disk.h"
#include "keyhandler.h"
#include "keybuf.h"
#include "keyframe.h"
#include "keys.h"
#include "fastload.h"
#include "screen.h"
#include "rotchars.h"

#define DISPLAY_BASE ((char*)0x400)

unsigned char run_length;
char prev_ch;

void anim_reset() {
    cursor_home();
    prev_ch = 0;
    run_length = 0;
    color = COLOR_WHITE;
    init_screen();
    stop_char_rotations();
}

static void init(void) {
    anim_reset();
    show_cursor();
}

// -----

static void do_store(char ch) {
    *last_char = ch;
    if (last_char >= (char*)0xcf00) {
        /* running out of RAM warning */
        ++*(char*)0xd020;
    }
    if (last_char != (char*)0xcfff) {
        ++last_char;
    }
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
    run_length = 0;
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

static void save() {
    prompt_save_anim();
}

static void load()
{
    unsigned int read = prompt_load_anim();
    if (read) {
        last_char = KEYS_START + read;
        anim_reset();
    }
}

static void pause_one_clock()
{
    clock_t now = clock();
    while (now == clock());
}

static char* run_ptr;

static void insert_keyframe()
{
    if (playback_mode) {
        run_ptr += 2;  // Skips speed.
        return;
    }
    ++*(char*)0xd020;
    store_char(CH_HOME);
    store_char(KEYFRAME_SPEED_NONE);
    store_char(KEYFRAME_SPEED_NONE >> 8);
    pause_one_clock();
    pause_one_clock();
    --*(char*)0xd020;
}

static void run() {
    run_ptr = KEYS_START;
    flush_rle();
    playback_mode = 1;
    anim_reset();
    while (run_ptr < last_char) {
        handle_rle(*run_ptr);
        ++run_ptr;
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
                case CH_STOP:
                    break;
                case CH_HOME:
                    insert_keyframe();
                    break;
                case CH_CLR:
                    break;
                case CH_RUN:
                    run();
                    break;
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
    store_char(MOVIE_START_MARKER);
    editloop();
}
