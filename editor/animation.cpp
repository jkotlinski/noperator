#include "animation.h"

#include <QFile>

#include "putchar.h"
#include "screen.h"

Animation::Animation()
    : putChar(new PutChar)
{
    QFile f(":/12");
    f.open(QFile::ReadOnly);
    data = f.readAll();
}

void Animation::step(Screen *screen) {
    putChar->setScreen(screen);

    putChar->hideCursor();

    if (rleLeft) {
        putChar->put(rleChar);
        --rleLeft;
    } else while (1) {
        const int ch = getc();
        switch (ch) {
        case -1:  // Done!
            return;
        case 0:  // RLE
            rleChar = getc();
            rleLeft = getc();
            break;
        case 0x13:  // HOME
            speed = getc();
            speed |= getc() << 8;
            Q_ASSERT(speed > 0);
            break;
        default:
            rleChar = ch;
            rleLeft = 1;
            break;
        }
        if (rleLeft) {
            putChar->put(rleChar);
            --rleLeft;
            break;
        }
    }

    putChar->showCursor();
}

int Animation::getc() {
    if (index == data.size())
        return -1;
    return static_cast<unsigned char>(data.at(index++));
}
