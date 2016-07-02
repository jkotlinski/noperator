#include "rotchars.h"

#include <string.h>

unsigned char rotchar_dirs[16];
unsigned char rotchar_screencodes[16];
extern unsigned char _FONTRAM_START__;

static void reset(unsigned char screencode) {
    unsigned char tmp = *(char*)1;
    unsigned char* dst = &_FONTRAM_START__ + screencode * 8;
    unsigned char* src = dst + (0xd000u - (unsigned int)&_FONTRAM_START__);
    asm("sei");
    *(char*)1 = 0xfb;
    memcpy(dst, src, 8);
    *(char*)1 = tmp;
    asm("cli");
}

void rotate_char(unsigned char screencode, unsigned char dir) {
    unsigned char i = 0;

    // If ch is already in the table, update the direction.
    for (; i < sizeof(rotchar_dirs); ++i) {
        if (rotchar_screencodes[i] == screencode) {
            rotchar_dirs[i] = dir;
            if (!dir) {
                reset(screencode);
            }
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
    unsigned char i = 0;
    memset(rotchar_dirs, 0, sizeof(rotchar_dirs));
    for (; i < sizeof(rotchar_screencodes); ++i) {
        reset(rotchar_screencodes[i]);
    }
}
