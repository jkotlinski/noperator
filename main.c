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

#include <conio.h>
#include <string.h>

#include "anim.h"
#include "disk.h"
#include "handle.h"
#include "keybuf.h"
#include "keyframe.h"
#include "keys.h"
#include "music.h"
#include "myload.h"

void loader_test() {
    anim_reset();
    loader_init();
    loader_open("smile2");
    loader_getc();  /* skip address */
    while (1) {
        int ch = loader_getc();
        if (ch == -1) break;
        if (ch == CH_HOME) {
            loader_getc();
            loader_getc();
        } else {
            handle_rle(ch);
        }
    }
}

static void load_music()
{
    prompt_load_anim();
    memcpy((char*)0x1000, KEYS_START, 0x2000);
    cputs(" ticks per step? (1-9)");
    ticks_per_step = cgetc() - '0';
}

void movie_editor(void) {}

void main_menu(void) {
    anim_reset();
    textcolor(COLOR_YELLOW);
    cputsxy(0, 0, "movie noperator");
    cputsxy(0, 2, "choose editor:");
    cputsxy(0, 4, "[a]nimation");
    cputsxy(0, 5, "[m]usic select");
    if (ticks_per_step) {
        /* Keyframe editor only makes sense if music is loaded. */
        cputsxy(0, 6, "[k]eyframe");
    }
    cputsxy(0, 8, "[t]est");
    for (;;) {
        switch (cgetc()) {
            case 'a': anim_editor(); break;
            case 'k': keyframe_editor(); break;
            case 'm': load_music(); main_menu();
            case 't': loader_test(); main_menu(); break;
        }
    }
}

void main(void) {
    /* asm("sei");
    *(unsigned int**)0x318 = (unsigned int*)0x80d;
    asm("cli"); */
    main_menu();
}
