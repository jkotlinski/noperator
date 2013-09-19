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

#include "disk.h"
#include "music.h"

char music_path[20];

void write_movie()
{
    char buf[20];
    clrscr();
    textcolor(COLOR_WHITE);
    ls();
    cputs("animation> ");
    while (!mygets(buf));
}

void load_music()
{
    unsigned int read;
    clrscr();
    textcolor(COLOR_WHITE);
    ls();
    cputs("music> ");
    if (!mygets(music_path)) return;
    read = cbm_load(music_path, 8, (void*)0x1000);
    if (read == 0) return;
    if (read > 0x2000) {
        cputs("too big:(");
        while (1) ++*(char*)0xd020;
    }
    cputs(" ticks per step? (1-9)");
    ticks_per_step = cgetc() - '0';
}

