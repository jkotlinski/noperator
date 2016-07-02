#include "rotchars.h"

#include <string.h>

unsigned char rotchar_dirs[16];
unsigned char rotchar_screencodes[16];

void rotate_char(unsigned char screencode, unsigned char dir) {
    unsigned char i = 0;

    // If ch is already in the table, update the direction.
    for (; i < sizeof(rotchar_dirs); ++i) {
        if (rotchar_screencodes[i] == screencode) {
            rotchar_dirs[i] = dir;
            return;
        }
    }
    // Find an empty spot in table and take it.
    for (i = 0; i < sizeof(rotchar_dirs); ++i) {
        if (rotchar_dirs[i] == 0) {
            rotchar_dirs[i] = dir;
            rotchar_screencodes[i] = screencode;
            return;
        }
    }
}

void stop_char_rotations() {
    memset(rotchar_dirs, 0, sizeof(rotchar_dirs));
}
