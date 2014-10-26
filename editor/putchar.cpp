#include "putchar.h"

#include "screen.h"

void PutChar::put(unsigned char ch) {
    if ((ch & 0x70) > 0x10) {
        // Normal char.
        if (ch != (0x80 | ' ')) {
            print(ch);
        } else {
            reverse ^= 0x80;
            print(' ');
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
    case 17: cursorDown(); break;
    case 145:  // Cursor up.
        break;
    case 157:  // Cursor left.
        break;
    case 29:  // Cursor right.
        break;
    default:
        Q_ASSERT(!"Unhandled character");
        break;
    }
}

void PutChar::print(unsigned char ch) {
   Q_ASSERT(!"print not implemented");
}

void PutChar::cursorDown() {
    if (y != 24) {
        ++y;
    } else {
        screen->moveUp();
    }
}

void PutChar::hideCursor() {
    screen->setColor(x, y, hiddenCursorColor);
    screen->setChar(x, y, hiddenCursorChar);
}

void PutChar::showCursor() {
    hiddenCursorColor = screen->getColor(x, y);
    screen->setColor(x, y, fgColor);
    hiddenCursorChar = screen->getChar(x, y);
    screen->setChar(x, y, 0x80 ^ hiddenCursorChar);
}
