#ifndef SIDDEVICE_H
#define SIDDEVICE_H

#include <QIODevice>
#include <QDebug>

class sidplayfp;
class SidTune;

class SidDevice : public QIODevice
{
    Q_OBJECT

public:
    SidDevice();

    virtual void start() {
        open(ReadOnly);
    }

    virtual qint64 readData(char* buf, qint64 size);
    virtual qint64 writeData(const char*, qint64);

private:
    sidplayfp* player;
    SidTune* tune;
};

#endif // SIDDEVICE_H
