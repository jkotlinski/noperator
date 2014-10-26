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
    const unsigned char ch = getc();
    switch (ch) {
    case 0x13:  // HOME
        speed = getc();
        speed |= getc() << 8;
        break;
    default:
        putChar->put(screen, ch);
        break;
    }
}

int Animation::getc() {
    return data.at(index++);
}
