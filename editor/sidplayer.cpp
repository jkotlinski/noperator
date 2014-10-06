#include "sidplayer.h"

#include <QDebug>
#include <QAudioOutput>

#include "siddevice.h"

SidPlayer::SidPlayer()
    : audioOutput(0)
{
    QAudioFormat format;
    format.setSampleRate(44100);
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(format)) {
        qWarning() << "Audio format not supported";
        return;
    }

    audioOutput = new QAudioOutput(format);
    sidDevice = new SidDevice;
    sidDevice->start();
    audioOutput->start(sidDevice);
}
