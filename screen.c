#include <string.h>

void reset_screen_and_font_address() {
    *(char*)0xd018 = 0x1a;  // screen = $400, font = $2800
}

void init_screen() {
    *(char*)0xd020 = 0;
    *(char*)0xd021 = 0;
    reset_screen_and_font_address();
    memset((char*)0x400, ' ', 40 * 25);
}
