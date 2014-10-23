#include "siddevice.h"

#include <QFile>

#include "sidplayfp/sidplayfp.h"
#include "sidplayfp/SidTune.h"
#include "sidplayfp/SidConfig.h"
#include "sidplayfp/sidtune/PSID.h"
#include "residfp-builder/residfp.h"

SidDevice::SidDevice()
    : player(new sidplayfp())
{
    SidConfig config = player->config();
    config.sidEmulation = new ReSIDfpBuilder("resid");
    config.sidEmulation->create(1);
    player->config(config);

    QFile kernal(":/kernal.bin");
    kernal.open(ReadOnly);
    player->setRoms(reinterpret_cast<uint8_t*>(kernal.readAll().data()));

    const char psidHeader[] = {
        'P', 'S', 'I', 'D', // id
        0, 1,  // version
        0, 22,  // offset
        0, 0,  // load address
        0x10, 0, // init routine
        0x10, 3,  // play routine
        0, 1,  // number of songs
        0, 1,  // start song
        0, 0, 0, 0  // speed info
    };

    QFile sidFile(":/packed.prg");
    sidFile.open(ReadOnly);
    QByteArray tuneData = sidFile.readAll();
    tuneData.prepend(psidHeader, sizeof(psidHeader));
    char* data = tuneData.data();

    tune = new SidTune(reinterpret_cast<uint_least8_t*>(data), tuneData.length());

    if (!tune->getStatus()) {
        qWarning() << "error: " << tune->statusString();
        return;
    }

    player->load(tune);
}

qint64 SidDevice::readData(char* charBuf, qint64 size) {
    qint64 ret = player->play((short*)charBuf, size / sizeof(short)) * sizeof(short);
    return ret;
}

qint64 SidDevice::writeData(const char*, qint64) {
    return 0;
}
