#include <conio.h>

#include "anim.h"
#include "irq.h"
#include "keyframe.h"
#include "movie.h"
#include "music.h"

static void print_ticks_per_step() {
    cputsxy(0, 4, "song speed [1-9]: ");
    cputc('0' + ticks_per_step);
}

void main_menu(void) {
    anim_reset();
    textcolor(COLOR_YELLOW);
    cputsxy(0, 0, "movie noperator");
    cputsxy(0, 2, "[a]nim edit");
    cputsxy(0, 3, "[m]usic select");
    print_ticks_per_step();
    cputsxy(0, 5, "[k]eyframe edit");
    cputsxy(0, 7, "[s]ave demo");
    cputsxy(0, 8, "[r]eset autostart");
    for (;;) {
        char ch;
        switch (ch = cgetc()) {
            case 'a':
                anim_editor();
                break;
            case 'm':
                select_music();
                main_menu();
                break;
            case 'r':
                scratch_movie();
                break;
            case 'k':
                keyframe_editor();
                main_menu();
                break;
            case 's':
                write_movie();
                main_menu();
                break;
            default:
                if (ch >= '1' && ch <= '9') {
                    ticks_per_step = ch - '0';
                    print_ticks_per_step();
                }
        }
    }
}

void main(void) {
    setup_irq();
    if (!play_movie()) {
        load_music("lightforce");
    }
    main_menu();
}
