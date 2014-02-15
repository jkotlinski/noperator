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
#include "fastload.h"
#include "handle.h"
#include "irq.h"
#include "keys.h"
#include "music.h"
#include "rledec.h"

static struct Movie
{
    char music_path[16];
    char ticks_per_step;
    char anim_path[16];
};
static struct Movie movie;

#define MOVIE_CONFIG "movie-rc"

void scratch_movie()
{
    cbm_open(1, 8, 15, "s:" MOVIE_CONFIG);
    cbm_close(1);
}

void write_movie()
{
    clrscr();
    textcolor(COLOR_WHITE);
    ls();
    cputs("anim> ");
    while (!mygets(movie.anim_path));
    movie.ticks_per_step = ticks_per_step;
    scratch_movie();
    cbm_save(MOVIE_CONFIG, 8, &movie, sizeof(movie));
    play_movie();
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
    if (read > 0x1800) {
        cputs("too big:(");
        while (1) ++*(char*)0xd020;
    }
    cputs(" ticks per step? (1-9)");
    ticks_per_step = cgetc() - '0';
}

void play_movie()
{
    unsigned int acc = 1 << 12;
    unsigned int speed = 0;
    unsigned char rle_left = 0;

    if (cbm_load(MOVIE_CONFIG, 8, &movie) != sizeof(movie))
        return;
    anim_reset();
    loader_init();
    loader_load(movie.music_path);
    init_music();
    ticks_per_step = movie.ticks_per_step;
    loader_open(movie.anim_path);
    loader_getc();  /* skip address */

    ticks = 0;
    startirq();
    while (1) {
        /* Wait for tick. */
        if (!ticks) {
            show_cursor();
            while (ticks == 0);
            hide_cursor();
        }
        --ticks;

        acc += speed;
        while (acc >= (1 << 12)) {
            if (rle_left) {
                handle(rle_char, 1);
                --rle_left;
            } else while (1) {
                int ch = loader_getc();
                switch (ch) {
                    case -1:  /* Done. */
                        stopirq();
                        cgetc();
                        return;
                    case CH_HOME:
                        speed = loader_getc();
                        speed |= loader_getc() << 8;
                        break;
                    case 0:  /* RLE */
                        rle_dec(0);
                        rle_dec(loader_getc());
                        rle_left = rle_dec(loader_getc());
                        break;
                    default:
                        rle_left = rle_dec(ch);
                }
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

