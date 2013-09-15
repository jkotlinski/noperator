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
#include "keybuf.h"
#include "screen.h"

#include <stdio.h>

static char* read_pos = KEYS_START;

#define CH_HOME 0x13

static char* next_keyframe()
{
    char *pos = read_pos + 2;
    for (;;) {
        if (pos >= last_char)
            return last_char;
        if (*pos == CH_HOME) {
            return pos;
        }
        ++pos;
    }
}

static char behind_speed_buf[20];

static void store_screen()
{
    memcpy(behind_speed_buf, (char*)0x400 + 24 * 40, sizeof(behind_speed_buf));
}
static void restore_screen()
{
    memcpy((char*)0x400 + 24 * 40, behind_speed_buf, sizeof(behind_speed_buf));
}

static void print_speed()
{
    unsigned int speed = *(int*)(read_pos + 1);

    store_screen();

    gotoxy(0, 24);
    printf("%i-%i ",
            read_pos - KEYS_START,
            next_keyframe() - KEYS_START);
    if (speed == KEYFRAME_SPEED_NONE) {
        cputs("spd? bts?");
    }
}

static void goto_next_keyframe()
{
    if (next_keyframe() >= last_char)
        return;
    restore_screen();
    read_pos += 3;
    for (;;) {
        char ch = *read_pos;
        switch (ch) {
            case CH_HOME:
                print_speed();
                return;  /* Done! */
            default:
                handle(ch, 1);
                ++read_pos;
        }
    }
}

static void editloop()
{
    print_speed();
    for (;;) {
        switch (cgetc())
        {
            case CH_CURS_RIGHT:
                goto_next_keyframe();
                break;
        }
    }
}

void keyframe_editor(void)
{
    init_screen();

    for (;;) {
        unsigned int read;
        if (read = prompt_load_anim()) {
            last_char = KEYS_START + read;
            break;
        }
    }

    init_screen();

    editloop();
}
