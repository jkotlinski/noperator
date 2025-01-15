#include "movie.h"

#include <conio.h>
#include <string.h>

#include "anim.h"
#include "disk.h"
#include "fastload.h"
#include "keyhandler.h"
#include "irq.h"
#include "keyframe.h"
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

#define MOVIE_CONFIG "movrc"

void scratch_movie()
{
    cbm_open(1, 8, 15, "s:" MOVIE_CONFIG);
    cbm_close(1);
}

void write_movie()
{
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
    unsigned int speed = KEYFRAME_SPEED_NONE;
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
    start_playing();
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
                    case -1:  /* Reached file end, restart. */
                        loader_open(movie.anim_path);
                        loader_getc();  /* skip address */
                        continue;
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
                        rle_char = ch;
                        rle_left = 1;
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

