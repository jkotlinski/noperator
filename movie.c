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

#include "movie.h"

#include <conio.h>
#include <string.h>

#include "anim.h"
#include "disk.h"
#include "handle.h"
#include "irq.h"
#include "keys.h"
#include "music.h"
#include "fastload.h"

static struct Movie
{
    char music_path[16];
    char ticks_per_step;
    char anim_path[16];
};
static struct Movie movie;

void write_movie()
{
    clrscr();
    textcolor(COLOR_WHITE);
    ls();
    cputs("anim> ");
    while (!mygets(movie.anim_path));
    movie.ticks_per_step = ticks_per_step;
    cbm_open(1, 8, 15, "s:movie");  /* scratch */
    cbm_close(1);
    cbm_save("movie", 8, &movie, sizeof(movie));
    cputs(" ok");
    while (1) ++*(char*)0xd020;
}

void load_music()
{
    unsigned int read;
    clrscr();
    textcolor(COLOR_WHITE);
    ls();
    cputs("music> ");
    if (!mygets(movie.music_path)) return;
    read = cbm_load(movie.music_path, 8, (void*)0x1000);
    if (read == 0) return;
    if (read > 0x2000) {
        cputs("too big:(");
        while (1) ++*(char*)0xd020;
    }
    cputs(" ticks per step? (1-9)");
    ticks_per_step = cgetc() - '0';
}

void play_movie() {
    if (cbm_load("movie", 8, &movie) != sizeof(movie))
        return;
    anim_reset();
    cbm_load(movie.music_path, 8, (void*)0x1000);
    init_music();
    ticks_per_step = movie.ticks_per_step;
    loader_init();
    loader_open(movie.anim_path);
    loader_getc();  /* skip address */

    ticks = 0;
    startirq();
    while (1) {
        int ch;

        show_cursor();
        while (!ticks);
        hide_cursor();
        --ticks;

        ch = loader_getc();
        while (ch == -1);  /* Done! */
        if (ch == CH_HOME) {
            loader_getc();
            loader_getc();
        } else {
            handle_rle(ch);
        }
    }
}
