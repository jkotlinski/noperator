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

#include "disk.h"

#include <conio.h>

#include "keybuf.h"

char mygets(char* buf) {
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

void ls() {
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

unsigned int prompt_load_anim(void)
{
    unsigned int read = 0;
    char buf[16];
    clrscr();
    textcolor(COLOR_WHITE);
    ls();
    cputs("load> ");
    if (mygets(buf)) {
        read = cbm_load(buf, 8, KEYS_START);
        if (!read) {
            cputs(" err");
            cgetc();
        }
    }
    return read;
}

void prompt_save_anim()
{
    char buf[18];
    clrscr();
    textcolor(COLOR_WHITE);
    ls();
    cputs("save> ");
    if (!mygets(buf + 2)) return;
    buf[0] = 's';
    buf[1] = ':';
    cbm_open(1, 8, 15, buf);  /* scratch */
    cbm_close(1);
    cputs(cbm_save(buf + 2, 8, KEYS_START, last_char - KEYS_START)
            ? " err" : " ok");
    cgetc();
}
