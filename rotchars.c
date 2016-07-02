#include "rotchars.h"

#include <string.h>

static unsigned char dirs[16];
static unsigned char screencodes[16];

void rotate_char(unsigned char screencode, unsigned char dir) {
    unsigned char i = 0;

    // If ch is already in the table, update the direction.
    for (; i < sizeof(dirs); ++i) {
        if (screencodes[i] == screencode) {
            dirs[i] = dir;
            return;
        }
    }
    // Find an empty spot in table and take it.
    for (i = 0; i < sizeof(dirs); ++i) {
        if (dirs[i] == 0) {
            dirs[i] = dir;
            screencodes[i] = screencode;
            return;
        }
    }
}

void stop_char_rotations() {
    memset(dirs, 0, sizeof(dirs));
}
