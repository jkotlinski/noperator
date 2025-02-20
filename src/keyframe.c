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

static unsigned char* edit_pos;

#define RLE_MARKER 0

#define STEPS_PER_BEAT 4
#define TICKS_PER_BEAT (STEPS_PER_BEAT * ticks_per_step)

static unsigned char* next_keyframe()
{
    unsigned char *pos = edit_pos + 3;
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

static void clear_bg() {
    gotoxy(0, 24);
    while (wherex() < 8) {
        cputc(' ');
    }
    gotoxy(10, 24);
    while (wherex() < 32) {
        cputc(' ');
    }
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
    const unsigned char* pos = edit_pos + 3;
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
    keys %= TICKS_PER_BEAT;
    keys *= 10;
    print_dec(keys / TICKS_PER_BEAT);
}

static void print_speed() {
    const unsigned int speed = *(int*)(edit_pos + 1);
    gotoxy(10, 24);
    if (speed == KEYFRAME_SPEED_NONE) {
        cputs("1-9: set beats");
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
    unsigned char* now = edit_pos;
    unsigned char* last_keyframe_pos = KEYS_START;
    clear_bg();
    gotoxy(0, 24);
    // find keyframe number
    edit_pos = KEYS_START;
    while (edit_pos < now) {
        last_keyframe_pos = edit_pos;
        edit_pos = next_keyframe();
        ++keyframe;
    }
    if (edit_pos == now) {
        cputc('#');
        print_dec(keyframe);
        print_speed();
    } else {
        cputc('#');
        print_dec(keyframe - 1);
        cputc('+');
        print_dec(now - last_keyframe_pos);
    }
    edit_pos = now;
}

static void goto_next_keyframe()
{
    unsigned char* const end = next_keyframe();
    if (end >= last_char)
        return;
    if (*edit_pos == CH_HOME)
        edit_pos += 3;  /* Skips keyframe. */
    for (;;) {
        if (edit_pos == end) {
            print_position();
            return;  /* Done! */
        }
        handle_rle(*edit_pos);
        ++edit_pos;
    }
}

static void goto_prev_keyframe()
{
    unsigned char* pos = KEYS_START;
    unsigned char* new_edit_pos = KEYS_START;
    if (edit_pos == KEYS_START) return;

    /* Finds new_edit_pos. */
    while (1) {
        unsigned char ch = *pos;
        if (ch == CH_HOME) {
            if (pos < edit_pos) {
                new_edit_pos = pos;
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

    /* Replays up to new_edit_pos. */
    edit_pos = KEYS_START;
    while (edit_pos < new_edit_pos) {
        const char ch = *edit_pos;
        if (ch == CH_HOME) {
            edit_pos += 3;
        } else {
            handle_rle(ch);
            ++edit_pos;
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
    *(unsigned int*)(edit_pos + 1) = speed;
}

static void enter_beats(int beats)
{
    char c;
    clear_bg();
    gotoxy(10, 24);
    cputs("beats:");
    cputc('0' + beats);
    revers(1);
    cputc(' ');
    c = cgetc();
    if (c >= '0' && c <= '9') {
        gotox(17);
        revers(0);
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
    print_position();
}

static void goto_next_key()
{
    unsigned char* pos = edit_pos;
    switch (*edit_pos) {
        case CH_HOME:  /* Keyframe */
            edit_pos += 3;
            break;
        case RLE_MARKER:
            handle_rle(*edit_pos++);
            handle_rle(*edit_pos++);
            /* Fall through! */
        default:
            handle_rle(*edit_pos++);
    }
    if (edit_pos >= last_char) {
        // don't go out of bounds
        edit_pos = pos;
        return;
    }
    print_position();
}

static void delete_keyframe()
{
    if (*edit_pos != CH_HOME) return;
    last_char -= 3;
    memmove(edit_pos, edit_pos + 3, last_char - edit_pos);
}

static void insert_keyframe()
{
    if (*edit_pos == CH_HOME) return;
    memmove(edit_pos + 3, edit_pos, last_char - edit_pos);
    edit_pos[0] = CH_HOME;
    edit_pos[1] = KEYFRAME_SPEED_NONE;
    edit_pos[2] = KEYFRAME_SPEED_NONE;
    last_char += 3;
    print_position();
}

static void play_current_segment()
{
    unsigned int acc = 1 << 12;
    unsigned int speed;
    unsigned char* end;
    unsigned char rle_left = 0;

    if (*edit_pos != CH_HOME) {
        goto_prev_keyframe();
    }

    speed = *(int*)(edit_pos + 1);
    end = next_keyframe();

    init_music();
    start_playing();

    ticks = 0;

    edit_pos += 3;

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
                if (edit_pos == end) {
                    stop_playing();
                    goto_prev_keyframe();
                    return;
                }
                rle_left = rle_dec(*edit_pos);
                ++edit_pos;
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
                if (*edit_pos == CH_HOME && ch >= '1' && ch <= '9') {
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

    edit_pos = KEYS_START;
    last_char = KEYS_START + read;

    init_screen();
    editloop();
}
