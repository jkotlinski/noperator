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

// -----

#if 0
static void up(unsigned char* ptr) {
    unsigned char tmp = ptr[0];
    ptr[0] = ptr[1];
    ptr[1] = ptr[2];
    ptr[2] = ptr[3];
    ptr[3] = ptr[4];
    ptr[4] = ptr[5];
    ptr[5] = ptr[6];
    ptr[6] = ptr[7];
    ptr[7] = tmp;
}

static unsigned char rotr(unsigned char c) {
    return (c >> 1) | (c << 7);
}

static void right(unsigned char* ptr) {
    // TODO: asm
    ptr[0] = rotr(ptr[0]);
    ptr[1] = rotr(ptr[1]);
    ptr[2] = rotr(ptr[2]);
    ptr[3] = rotr(ptr[3]);
    ptr[4] = rotr(ptr[4]);
    ptr[5] = rotr(ptr[5]);
    ptr[6] = rotr(ptr[6]);
    ptr[7] = rotr(ptr[7]);
}

static void down(unsigned char* ptr) {
    unsigned char tmp = ptr[0];
    ptr[0] = ptr[7];
    ptr[7] = ptr[6];
    ptr[6] = ptr[5];
    ptr[5] = ptr[4];
    ptr[4] = ptr[3];
    ptr[3] = ptr[2];
    ptr[2] = ptr[1];
    ptr[1] = tmp;
}

static unsigned char rotl(unsigned char c) {
    return (c >> 7) | (c << 1);
}

static void left(unsigned char* ptr) {
    // TODO: asm
    ptr[0] = rotl(ptr[0]);
    ptr[1] = rotl(ptr[1]);
    ptr[2] = rotl(ptr[2]);
    ptr[3] = rotl(ptr[3]);
    ptr[4] = rotl(ptr[4]);
    ptr[5] = rotl(ptr[5]);
    ptr[6] = rotl(ptr[6]);
    ptr[7] = rotl(ptr[7]);
}

void tick_rotate_chars() {
    unsigned char i = 0;
    ++*(char*)0xd020;
    for (i = 0; i < sizeof(rotchar_dirs); ++i) {
        unsigned char* ptr;
        unsigned char dir = rotchar_dirs[i];
        if (!dir) {
            continue;
        }
        ptr = (unsigned char*)0x2800 + 8 * rotchar_screencodes[i];
        switch (dir) {
            case 1: up(ptr); break;
            case 2: right(ptr); break;
            case 3: down(ptr); break;
            case 4: left(ptr); break;
        }
    }
    --*(char*)0xd020;
}
#endif
