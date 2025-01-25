#include "keyframe.h"

#include <c64.h>
#include <conio.h>
#include <string.h>

#include "disk.h"
#include "keyhandler.h"
#include "irq.h"
#include "keybuf.h"
#include "keys.h"
#include "music.h"
#include "rledec.h"
#include "screen.h"

static unsigned char* read_pos;

#define RLE_MARKER 0

#define STEPS_PER_BEAT 4
#define TICKS_PER_BEAT (STEPS_PER_BEAT * ticks_per_step)

static unsigned char* next_keyframe()
{
    unsigned char *pos = read_pos + 3;
    for (;;) {
        if (pos >= last_char)
            return last_char;
        if (*pos == CH_HOME) {
            return pos;
        }
        if (*pos == RLE_MARKER) {
            pos += 2;
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

static unsigned int keys_in_segment()
{
    /* Keys in segment, excluding keyframe. */
    const unsigned char* const end = next_keyframe();
    const unsigned char* pos = read_pos + 3;
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

static void print_speed() {
    const unsigned int speed = *(int*)(read_pos + 1);
    if (speed == KEYFRAME_SPEED_NONE) {
        cputs("1-9: set beat count");
    } else {
        cputs("beats:");
        print_beats(speed);
        cputs(" (");
        print_dec(speed / ((1 << 12) / 60));
        cputs(" kps)");
    }
}

static void print_position() {
    int keyframe = 1;
    unsigned char* now = read_pos;
    if (*read_pos != CH_HOME) {
        return; // not a keyframe
    }
    store_screen();
    gotoxy(0, 24);
    // find keyframe number
    read_pos = KEYS_START;
    while (read_pos != now) {
        read_pos = next_keyframe();
        ++keyframe;
    }
    cputc('#');
    print_dec(keyframe);
    gotoxy(10, 24);
    print_speed();
}

static void goto_next_keyframe()
{
    unsigned char* const end = next_keyframe();
    if (end >= last_char)
        return;
    restore_screen();
    if (*read_pos == CH_HOME)
        read_pos += 3;  /* Skips keyframe. */
    for (;;) {
        if (read_pos == end) {
            print_position();
            return;  /* Done! */
        }
        handle_rle(*read_pos);
        ++read_pos;
    }
}

static void goto_prev_keyframe()
{
    unsigned char* pos = KEYS_START;
    unsigned char* new_read_pos = KEYS_START;
    if (read_pos == KEYS_START) return;

    /* Finds new_read_pos. */
    while (1) {
        unsigned char ch = *pos;
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

    print_position();
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

static void enter_beats(int beats)
{
    char c;
    store_screen();
    gotoxy(10, 24);
    cputs("beats:");
    cputc('0' + beats);
    revers(1);
    cputc(' ');
    revers(0);
    while (wherex() < 39) {
        cputc(' ');
    }
    c = cgetc();
    if (c >= '0' && c <= '9') {
        gotox(17);
        cputc(c);
        revers(1);
        cputc(' ');
        beats *= 10;
        beats += c - '0';
        c = cgetc();
        if (c >= '0' && c <= '9') {
            beats *= 10;
            beats += c - '0';
        }
    }
    revers(0);
    calc_speed(beats);
    restore_screen();
    print_position();
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
    print_position();
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
    print_position();
}

static void play_current_segment()
{
    unsigned int acc = 1 << 12;
    unsigned int speed = *(int*)(read_pos + 1);
    unsigned char* const end = next_keyframe();
    unsigned char rle_left = 0;
    init_music();
    start_playing();

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
                    stop_playing();
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
    print_position();
    for (;;) {
        char ch = cgetc();
        switch (ch)
        {
            case CH_F2:
                prompt_save_anim();
                init_screen();
                return;
            case ' ':
                goto_next_keyframe();
                break;
            case ' ' | 0x80:
                goto_prev_keyframe();
                break;
            case CH_CURS_RIGHT:
                goto_next_key();
                print_position();
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
            default:
                if (*read_pos == CH_HOME && ch >= '1' && ch <= '9') {
                    enter_beats(ch - '0');
                }
                break;
        }
    }
}

void keyframe_editor(void)
{
    unsigned int read;
    init_screen();

    read = prompt_load_anim();

    if (!read) return;

    read_pos = KEYS_START;
    last_char = KEYS_START + read;

    init_screen();
    editloop();
}
