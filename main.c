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
#include "irq.h"
#include "keyframe.h"
#include "movie.h"
#include "music.h"

void main_menu(void) {
    anim_reset();
    textcolor(COLOR_YELLOW);
    cputsxy(0, 0, "movie noperator");
    cputsxy(0, 2, "[a]nim edit");
    cputsxy(0, 3, "[m]usic select");
    cputsxy(0, 7, "[r]eset autostart");
    if (!ticks_per_step) {
        /* Keyframe editor only makes sense if music is loaded. */
        textcolor(COLOR_GRAY1);
    }
    cputsxy(0, 4, "[k]eyframe edit");
    cputsxy(0, 6, "[s]ave demo");
    for (;;) {
        switch (cgetc()) {
            case 'a': anim_editor(); break;
            case 'm': load_music(); main_menu(); break;
            case 'r': scratch_movie(); break;
            case 'k':
                      if (ticks_per_step) {
                          keyframe_editor();
                          main_menu();
                      }
                      break;
            case 's':
                      if (ticks_per_step) {
                          write_movie();
                          main_menu();
                      }
                      break;
        }
    }
}

void main(void) {
    setup_irq();
    play_movie();
    main_menu();
}
