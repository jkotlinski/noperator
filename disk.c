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
    clrscr();
    textcolor(COLOR_WHITE);
    while (!cbm_readdir(1, &direntry)) {
        if (direntry.size) {
            if (wherey() == 24) {
                revers(1);
                cputs("<more>");
                revers(0);
                cgetc();
                clrscr();
            }
            cputs(direntry.name);
            if (wherex() < 20) {
                gotox(20);
            } else {
                gotoxy(0, wherey() + 1);
            }
        }
    }
    cbm_close(1);
}

unsigned int prompt_load_anim(void)
{
    unsigned int read = 0;
    char buf[16];
    ls();
    cputs("load anim> ");
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
    ls();
    cputs("save anim> ");
    if (!mygets(buf + 2)) return;
    buf[0] = 's';
    buf[1] = ':';
    cbm_open(1, 8, 15, buf);  /* scratch */
    cbm_close(1);
    cputs(cbm_save(buf + 2, 8, KEYS_START, last_char - KEYS_START)
            ? " err" : " ok");
    cgetc();
}
