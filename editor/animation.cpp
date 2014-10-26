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
    const unsigned char ch = data.at(index++);
    putChar->put(screen, ch);
}
