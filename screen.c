#include <string.h>

void init_screen() {
    *(char*)0xd020 = 0;
    *(char*)0xd021 = 0;
    *(char*)0xd018 = 0x1a;  // screen = $400, font = $2800
    memset((char*)0x400, ' ', 40 * 25);
}
