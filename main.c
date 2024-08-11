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
