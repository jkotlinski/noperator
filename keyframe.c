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

#include "keyframe.h"

#include <c64.h>
#include <conio.h>
#include <string.h>

#include "disk.h"
#include "handle.h"
#include "irq.h"
#include "keybuf.h"
#include "keys.h"
#include "music.h"
#include "rledec.h"
#include "screen.h"

static char* read_pos = KEYS_START;

#define RLE_MARKER 0

#define STEPS_PER_BEAT 4
#define TICKS_PER_BEAT (STEPS_PER_BEAT * ticks_per_step)

static char* next_keyframe()
{
    char *pos = read_pos + 3;
    for (;;) {
        if (pos >= last_char)
            return last_char;
        if (*pos == CH_HOME) {
            return pos;
        }
        ++pos;
    }
}

static char behind_speed_buf[30];

static void store_screen()
{
    if (!*behind_speed_buf)
        memcpy(behind_speed_buf, (char*)0x400 + 24 * 40, sizeof(behind_speed_buf));
}
static void restore_screen()
{
    if (*behind_speed_buf)
        memcpy((char*)0x400 + 24 * 40, behind_speed_buf, sizeof(behind_speed_buf));
    *behind_speed_buf = '\0';
}

static void print_dec(unsigned int number)
{
    char buf[5];
    char i = 0;
    do {
        buf[i++] = number % 10;
        number /= 10;
    } while (number);
    while (i) {
        cputc('0' + buf[--i]);
    }
}

static void print_fract(unsigned int number)
{
    // Prints 4.12 number.
    print_dec(number / (1 << 12));
    cputc('.');
    print_dec(number % (1 << 12));
}

static unsigned int keys_in_segment()
{
    /* Keys in segment, excluding keyframe. */
    const char* const end = next_keyframe();
    const char* pos = read_pos + 3;
    unsigned int count = 0;
    while (pos < end) {
        count += rle_dec(*pos);
        ++pos;
    }
    return count;
}

static void print_beats(unsigned int speed)
{
    unsigned long keys = keys_in_segment();
    keys <<= 12;
    keys /= speed;
    print_dec(keys / TICKS_PER_BEAT);
    cputc('.');
    print_dec(keys % TICKS_PER_BEAT);
}

static void print_speed()
{
    unsigned int speed = *(int*)(read_pos + 1);

    if (*read_pos != CH_HOME) return;

    store_screen();

    gotoxy(0, 24);
    print_dec(read_pos - KEYS_START);
    cputc('-');
    print_dec(next_keyframe() - KEYS_START);
    cputc(' ');
    if (speed == KEYFRAME_SPEED_NONE) {
        cputs("spd not set");
    } else {
        cputs("spd:");
        print_fract(speed);
        cputs(" bts:");
        print_beats(speed);
    }
}

static void goto_next_keyframe()
{
    if (next_keyframe() >= last_char)
        return;
    restore_screen();
    read_pos += 3;  /* Skips keyframe. */
    for (;;) {
        char ch = *read_pos;
        switch (ch) {
            case CH_HOME:
                print_speed();
                return;  /* Done! */
            default:
                handle_rle(ch);
                ++read_pos;
        }
    }
}

static void goto_prev_keyframe()
{
    char* pos = KEYS_START;
    char* new_read_pos = KEYS_START;
    if (read_pos == KEYS_START) return;

    /* Finds new_read_pos. */
    while (1) {
        char ch = *pos;
        if (ch == CH_HOME) {
            if (pos < read_pos) {
                new_read_pos = pos;
                pos += 3;
            } else {
                break;
            }
        } else if (ch == RLE_MARKER) {
            pos += 3;  /* Skips RLE. */
        } else {
            ++pos;
        }
    }

    init_screen();
    cursor_home();

    /* Replays up to new_read_pos. */
    read_pos = KEYS_START;
    while (read_pos < new_read_pos) {
        const char ch = *read_pos;
        if (ch == CH_HOME) {
            read_pos += 3;
        } else {
            handle_rle(ch);
            ++read_pos;
        }
    }

    print_speed();
}

static unsigned char read_digits() {
    unsigned char number = 0;
    while (1) {
        char c = cgetc();
        if (c >= '0' && c <= '9') {
            cputc(c);
            number *= 10;
            number += c - '0';
            if (number > 100) break;
        } else if (c == CH_ENTER) {
            break;
        }
    }
    return number;
}

static void calc_speed(unsigned char beats)
{
    /* speed = (keys << 12) / ticks) */
    unsigned long speed = keys_in_segment();
    speed <<= 12;
    speed /= TICKS_PER_BEAT * beats;
    if (speed >= 0x10000u) return;
    *(unsigned int*)(read_pos + 1) = speed;
}

static void enter_beats()
{
    unsigned char beats;
    gotoxy(0, 24);
    for (beats = 0; beats < sizeof(behind_speed_buf); ++beats)
        cputc(' ');
    gotoxy(0, 24);
    cputs("bts:");
    revers(1);
    cputs("   ");
    gotox(wherex() - 3);
    beats = read_digits();
    revers(0);
    calc_speed(beats);
    print_speed();
}

static void goto_next_key()
{
    if (read_pos == last_char) return;
    restore_screen();
    switch (*read_pos) {
        case CH_HOME:  /* Keyframe */
            read_pos += 3;
            break;
        case RLE_MARKER:
            handle_rle(*read_pos++);
            handle_rle(*read_pos++);
            /* Fall through! */
        default:
            handle_rle(*read_pos++);
    }
    if (*read_pos == CH_HOME) {
        print_speed();
    }
}

static void delete_keyframe()
{
    if (*read_pos != CH_HOME) return;
    last_char -= 3;
    memmove(read_pos, read_pos + 3, last_char - read_pos);
    restore_screen();
}

static void insert_keyframe()
{
    if (*read_pos == CH_HOME) return;
    memmove(read_pos + 3, read_pos, last_char - read_pos);
    read_pos[0] = CH_HOME;
    read_pos[1] = KEYFRAME_SPEED_NONE;
    read_pos[2] = KEYFRAME_SPEED_NONE;
    last_char += 3;
    print_speed();
}

static void play_current_segment()
{
    unsigned int acc = 1 << 12;
    unsigned int speed = *(int*)(read_pos + 1);
    char* const end = next_keyframe();
    unsigned char rle_left = 0;
    init_music();
    startirq();

    ticks = 0;

    read_pos += 3;

    while (1)
    {
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
                if (read_pos == end) {
                    stopirq();
                    goto_prev_keyframe();
                    return;
                }
                rle_left = rle_dec(*read_pos);
                ++read_pos;
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

static void editloop()
{
    print_speed();
    for (;;) {
        switch (cgetc())
        {
            case CH_F2:
                prompt_save_anim();
                init_screen();
                return;
            case CH_CURS_RIGHT:
                goto_next_keyframe();
                break;
            case CH_CURS_LEFT:
                goto_prev_keyframe();
                break;
            case 'b':
                if (*read_pos == CH_HOME)
                    enter_beats();
                break;
            case ' ':
                goto_next_key();
                break;
            case ' ' | 0x80:
                {
                    char i = 10;
                    while (--i)
                        goto_next_key();
                }
                break;
            case CH_DEL:
                delete_keyframe();
                break;
            case CH_INS:
                insert_keyframe();
                break;
            case CH_RUN:
                play_current_segment();
                break;
            case CH_LEFTARROW:
                return;
        }
    }
}

void keyframe_editor(void)
{
    unsigned int read;
    init_screen();

    if (read = prompt_load_anim()) {
        last_char = KEYS_START + read;
    } else {
        return;
    }

    init_screen();

    editloop();
}
