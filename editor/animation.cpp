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
    if (rleLeft) {
        putChar->put(screen, rleChar);
        --rleLeft;
    } else while (1) {
        const unsigned char ch = getc();
        switch (ch) {
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
            putChar->put(screen, rleChar);
            --rleLeft;
            break;
        }
    }
}

int Animation::getc() {
    return static_cast<unsigned char>(data.at(index++));
}
