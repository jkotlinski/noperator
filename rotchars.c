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

// -----

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
    for (i = 0; i < sizeof(dirs); ++i) {
        unsigned char* ptr;
        unsigned char dir = dirs[i];
        if (!dir) {
            continue;
        }
        ptr = (unsigned char*)0x2800 + 8 * screencodes[i];
        switch (dir) {
            case 1: up(ptr); break;
            case 2: right(ptr); break;
            case 3: down(ptr); break;
            case 4: left(ptr); break;
        }
    }
}
