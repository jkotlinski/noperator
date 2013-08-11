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

#define DISPLAY_BASE ((char*)0x400)

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
#define KEYS_START (&_RAM_LAST__ + 1)
static char* key_out = KEYS_START;

static void store_char(char ch) {
    *key_out++ = ch;
    /* running out of RAM warning */
    if (key_out == (char*)0xcf00) *(char*)0xd020 = COLOR_YELLOW;
}

static void run();

char gets(char* buf) {
    unsigned char i = 0;
    while (1) {
        const char ch = cgetc();
        switch (ch) {
            case CH_ENTER:
                buf[i] = '\0';
                return i;
            case CH_DEL:
                if (i) {
                    --i;
                    gotox(wherex() - 1);
                    cputc(' ');
                    gotox(wherex() - 1);
                }
                break;
            default:
                buf[i++] = ch;
                cputc(ch);
        }
    }
}

static void save() {
    char buf[20];
    unsigned int size = key_out - KEYS_START;
    clrscr();
    textcolor(COLOR_WHITE);
    cputs("save> ");
    if (!gets(buf)) return;
    cputs(cbm_save(buf, 8, KEYS_START, size) ? " err" : " ok");
    cgetc();
}

static void ls() {
    struct cbm_dirent direntry;
    cbm_opendir(1, 8);
    while (!cbm_readdir(1, &direntry)) {
        if (direntry.size) {
            cputs(direntry.name);
            gotoxy(0, wherey() + 1);
        }
    }
    cbm_close(1);
}

static void load() {
    char buf[20];
    unsigned int read;
    clrscr();
    textcolor(COLOR_WHITE);
    ls();
    cputs("load> ");
    if (!gets(buf)) return;
    read = cbm_load(buf, 8, KEYS_START);
    if (read) {
        key_out = KEYS_START + read;
    } else {
        cputs(" err");
        cgetc();
    }
}

/* (CLIP_X1, CLIP_Y1) = top left.
 * (CLIP_X2, CLIP_Y2) = bottom right.
 */
static char CLIP_X1 = 0xff;
static char CLIP_X2;
static char CLIP_Y1;
static char CLIP_Y2;

char playback_mode;  // if set, some UI operations will be disabled
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

char copy_mode;

char clipboard[40 * 25];
char clipboard_color[40 * 25];

void handle_copy(char ch) {
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
                char tmp = *(char*)0xd020;
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
                *(char*)0xd020 = tmp;
            }
            break;
    }
}

static void start_copy() {
    CLIP_X1 = curx;
    CLIP_X2 = curx;
    CLIP_Y1 = cury;
    CLIP_Y2 = cury;

    invert_copy_mark();
    copy_mode = 1;
}

static void paste() {
    char y;
    if (CLIP_X1 == 0xff) return;

    // Pastes region.
    for (y = CLIP_Y1; y <= CLIP_Y2; ++y) {
        const char dst_y = y + cury - CLIP_Y1;
        char x;
        if (dst_y >= 25) break;
        for (x = CLIP_X1; x <= CLIP_X2; ++x) {
            const char dst_x = x + curx - CLIP_X1;
            if (dst_x >= 40) break;
            DISPLAY_BASE[dst_y * 40 + dst_x] = clipboard[y * 40 + x];
            ((char*)0xd800)[dst_y * 40 + dst_x] = clipboard_color[y * 40 + x];
        }
    }
}

/* returns 1 if ch should be stored in stream */
unsigned char handle(unsigned char ch, char first_keypress) {
    if (copy_mode) {
        handle_copy(ch);
        return 1;
    }

    switch (ch) {
        case CH_F1: load(); run(); return 0;
        case CH_F2: save(); run(); return 0;
        case CH_F3: ++*(char*)0xd020; break;
        case CH_F4: ++*(char*)0xd021; break;
        case CH_F5: start_copy(); break;
        case CH_F6: paste(); break;
        case CH_F7: break;
        case CH_F8: break;
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
    char* ptr = KEYS_START;
    playback_mode = 1;
    reset_screen();
    while (ptr < key_out) {
        handle(*ptr++, 1);
    }
    playback_mode = 0;
}

static void editloop(void) {
    char first_keypress = 1;
    unsigned char ticks_since_last_key;
    while (key_out < (char*)0xd000) {
        clock_t now = clock();
        while (now == clock());
        if (kbhit()) {
            unsigned char ch = cgetc();
            hide_cursor();
            if (handle(ch, first_keypress))
                store_char(ch);
            show_cursor();
            first_keypress = 0;
            ticks_since_last_key = 0;
        } else if (!first_keypress && ++ticks_since_last_key == 19) {
            first_keypress = 1;
        }
    }
}

void main(void) {
    init();
    editloop();
}
