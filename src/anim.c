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
#include "irq.h"
#include "music.h"
#include "rledec.h"

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
    reset_keyhandler();
}

static void init(void) {
    anim_reset();
    show_cursor();
}

// -----

static void do_store(char ch) {
    *last_char = ch;
    if (last_char != (unsigned char*)0xcfff) {
        ++last_char;
    } else {
        ++*(unsigned char*)0xd020; // out of RAM
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

static unsigned char* run_ptr;

static unsigned int prompt_speed(void)
{
    char i;
    unsigned int speed = 0;
    char chars[15];
    char colors[15];
    char* screenptr = (char*)(0x400 + 40 * 24);
    char* colorptr = (char*)(0xd800 + 40 * 24);
    // save chars+colors
    memcpy(chars, screenptr, sizeof(chars));
    memcpy(colors, colorptr, sizeof(colors));
    memset(colorptr, 1, sizeof(colors));
    cputsxy(0, 24, "set speed (1-9)");
    while (speed < 1 || speed > 9) {
        speed = cgetc() - '0';
    }
    // arbitrary fixup
    for (i = 8; i > 2; --i) {
        if (speed > i) {
            speed += speed - i;
        }
    }
    speed *= 256;
    // restore chars+colors
    memcpy(screenptr, chars, sizeof(chars));
    memcpy(colorptr, colors, sizeof(colors));
    return speed;
}

static void insert_keyframe()
{
    unsigned int speed;
    if (playback_mode) {
        run_ptr += 2;  // Skips speed.
        return;
    }
    speed = prompt_speed();
    store_char(CH_HOME);
    store_char(speed);
    store_char(speed >> 8);
    pause_one_clock();
    pause_one_clock();
}

static void replay_all_instantly() {
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

static void play_with_music() {
    unsigned int acc = 1 << 12;
    unsigned int speed;
    unsigned char rle_left = 0;
    run_ptr = KEYS_START;
    flush_rle();
    playback_mode = 1;
    anim_reset();

    speed = *(int*)(run_ptr + 1);
    run_ptr += 3;
    init_music();
    start_playing();
    ticks = 0;
    while (1) {
        /* Wait for tick. */
        show_cursor();
        while (ticks == 0);
        hide_cursor();
        --ticks;

        acc += speed;
        while (acc >= (1 << 12)) {
            if (rle_left) {
                handle(rle_char, 1);
                --rle_left;
            } else while (1) {
                unsigned char ch;
                if (run_ptr >= last_char) {
                    stop_playing();
                    playback_mode = 0;
                    return;
                }
                switch (ch = *run_ptr++) {
                    case CH_HOME:
                        speed = *run_ptr++;
                        speed |= *run_ptr++ << 8;
                        break;
                    case 0:  /* RLE */
                        rle_dec(0);
                        rle_dec(*run_ptr++);
                        rle_left = rle_dec(*run_ptr++);
                        break;
                    default:
                        rle_char = ch;
                        rle_left = 1;
                        break;
                }
                if (rle_left) {
                    handle(rle_char, 1);
                    --rle_left;
                    break;
                }
            }
            acc -= (1 << 12);
        }
    }
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
    while (last_char < (unsigned char*)0xd000) {
        static unsigned char blink_delay = 1;
        const unsigned char near_end = (last_char > (unsigned char*)0xcc00);
        reset_screen_and_font_address(); // Undoes any C= upper/lower case change.
        if (near_end) {
            ++*(char*)0xd020; // Warning! RAM is about to get full.
        }
        pause_one_clock();
        if (near_end) {
            --*(char*)0xd020;
        }
        if (--blink_delay == 0) {
            blink();
            blink_delay = 10;
        }
        if (kbhit()) {
            unsigned char ch = cgetc();
            unblink();
            hide_cursor();
            if (copy_mode) {
                if (handle(ch, first_keypress)) {
                    store_char(ch);
                }
            } else {
                switch (ch) {
                    case CH_F1:
                        load();
                        replay_all_instantly();
                        break;
                    case CH_F2:
                        flush_rle();
                        save();
                        replay_all_instantly();
                        break;
                    case CH_STOP:
                        break;
                    case CH_HOME:
                        insert_keyframe();
                        break;
                    case CH_CLR:
                        break;
                    case CH_RUN:
                        play_with_music();
                        break;
                    default:
                        if (handle(ch, first_keypress)) {
                            store_char(ch);
                        }
                        break;
                }
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
