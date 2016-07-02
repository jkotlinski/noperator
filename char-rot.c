#include "char-rot.h"

static unsigned char dirs[16];
static unsigned char chars[16];

void rotate_char(unsigned char ch, unsigned char dir) {
    unsigned char i;

    // If the char is already in the table, update the direction.
    for (i = 0; i < sizeof(dirs); ++i) {
        if (chars[i] == ch) {
            dirs[i] = dir;
            return;
        }
    }
    // Find an empty spot in table and take it.
    for (i = 0; i < sizeof(dirs); ++i) {
        if (dirs[i] == 0) {
            chars[i] = ch;
            dirs[i] = dir;
            return;
        }
    }
}
