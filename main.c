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

#include "anim.h"
#include "keyframe.h"
#include "myload.h"

void loader_test() {
    anim_reset();
    loader_init();
    loader_open("k");
    loader_getc();  /* skip address */
    while (1) {
        int ch = loader_getc();
        if (ch == -1) break;
        handle(ch, 1);
    }
}

void movie_editor(void) {}

void main_menu(void) {
    anim_reset();
    textcolor(COLOR_YELLOW);
    cputsxy(0, 0, "movie noperator");
    cputsxy(0, 2, "choose editor:");
    cputsxy(0, 4, "[a]nimation");
    cputsxy(0, 5, "[k]eyframe");
    cputsxy(0, 6, "[m]ovie");
    cputsxy(0, 8, "[t]est");
    for (;;) {
        switch (cgetc()) {
            case 'a': anim_editor(); break;
            case 'k': keyframe_editor(); break;
            case 'm': movie_editor(); break;
            case 't': loader_test(); break;
        }
    }
}

void main(void) {
    /* asm("sei");
    *(unsigned int**)0x318 = (unsigned int*)0x80d;
    asm("cli"); */
    main_menu();
}
