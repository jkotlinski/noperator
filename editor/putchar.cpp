#include "putchar.h"

#include "screen.h"

void PutChar::put(Screen *screen, unsigned char ch) {
    Q_ASSERT(screen);

    if ((ch & 0x70) > 0x10) {
        // Normal char.
        if (ch != (0x80 | ' ')) {
            print(screen, ch);
        } else {
            reverse ^= 0x80;
            print(screen, ' ');
            reverse ^= 0x80;
        }
    } else switch (ch) {
    case 1:
        screen->init();
        // Fall through.
    case 2:
        x = 0;
        y = 0;
        fgColor = 1;
        break;
    default:
        Q_ASSERT(!"Unhandled character");
        break;
    }
}

void PutChar::print(Screen *screen, unsigned char ch) {
    Q_ASSERT(!"print not implemented");
}
