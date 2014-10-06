#-------------------------------------------------
#
# Project created by QtCreator 2014-10-05T21:09:19
#
#-------------------------------------------------

QT       += core gui multimedia
CONFIG += c++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = editor
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    sidplayfp/c64/CIA/mos6526.cpp \
    sidplayfp/c64/CIA/timer.cpp \
    sidplayfp/c64/CPU/mos6510.cpp \
    sidplayfp/c64/CPU/mos6510debug.cpp \
    sidplayfp/c64/VIC_II/mos656x.cpp \
    sidplayfp/c64/c64.cpp \
    sidplayfp/c64/mmu.cpp \
    sidplayfp/sidtune/MUS.cpp \
    sidplayfp/sidtune/p00.cpp \
    sidplayfp/sidtune/prg.cpp \
    sidplayfp/sidtune/PSID.cpp \
    sidplayfp/sidtune/SidTuneBase.cpp \
    sidplayfp/sidtune/SidTuneTools.cpp \
    sidplayfp/config.cpp \
    sidplayfp/EventScheduler.cpp \
    sidplayfp/mixer.cpp \
    sidplayfp/player.cpp \
    sidplayfp/psiddrv.cpp \
    sidplayfp/reloc65.cpp \
    sidplayfp/sidbuilder.cpp \
    sidplayfp/SidConfig.cpp \
    sidplayfp/sidemu.cpp \
    sidplayfp/sidplayfp.cpp \
    sidplayfp/SidTune.cpp \
    sidplayfp/utils/MD5/MD5.cpp \
    sidplayfp/utils/STILview/stil.cpp \
    sidplayfp/utils/iniParser.cpp \
    sidplayfp/utils/SidDatabase.cpp \
    sidplayer.cpp \
    siddevice.cpp \
    residfp-builder/residfp/resample/SincResampler.cpp \
    residfp-builder/residfp/Dac.cpp \
    residfp-builder/residfp/EnvelopeGenerator.cpp \
    residfp-builder/residfp/ExternalFilter.cpp \
    residfp-builder/residfp/Filter.cpp \
    residfp-builder/residfp/Filter6581.cpp \
    residfp-builder/residfp/Filter8580.cpp \
    residfp-builder/residfp/FilterModelConfig.cpp \
    residfp-builder/residfp/Integrator.cpp \
    residfp-builder/residfp/OpAmp.cpp \
    residfp-builder/residfp/SID.cpp \
    residfp-builder/residfp/Spline.cpp \
    residfp-builder/residfp/version.cc \
    residfp-builder/residfp/WaveformCalculator.cpp \
    residfp-builder/residfp/WaveformGenerator.cpp \
    residfp-builder/residfp-builder.cpp \
    residfp-builder/residfp-emu.cpp

HEADERS  += mainwindow.h \
    sidplayfp/c64/Banks/Bank.h \
    sidplayfp/c64/Banks/ColorRAMBank.h \
    sidplayfp/c64/Banks/DisconnectedBusBank.h \
    sidplayfp/c64/Banks/ExtraSidBank.h \
    sidplayfp/c64/Banks/IOBank.h \
    sidplayfp/c64/Banks/NullSid.h \
    sidplayfp/c64/Banks/SidBank.h \
    sidplayfp/c64/Banks/SystemRAMBank.h \
    sidplayfp/c64/Banks/SystemROMBanks.h \
    sidplayfp/c64/Banks/ZeroRAMBank.h \
    sidplayfp/c64/CIA/mos6526.h \
    sidplayfp/c64/CIA/timer.h \
    sidplayfp/c64/CPU/mos6510.h \
    sidplayfp/c64/CPU/mos6510debug.h \
    sidplayfp/c64/CPU/opcodes.h \
    sidplayfp/c64/VIC_II/mos656x.h \
    sidplayfp/c64/c64.h \
    sidplayfp/c64/c64cia.h \
    sidplayfp/c64/c64cpu.h \
    sidplayfp/c64/c64env.h \
    sidplayfp/c64/c64sid.h \
    sidplayfp/c64/c64vic.h \
    sidplayfp/c64/mmu.h \
    sidplayfp/sidtune/MUS.h \
    sidplayfp/sidtune/p00.h \
    sidplayfp/sidtune/prg.h \
    sidplayfp/sidtune/PSID.h \
    sidplayfp/sidtune/SidTuneBase.h \
    sidplayfp/sidtune/SidTuneCfg.h \
    sidplayfp/sidtune/SidTuneInfoImpl.h \
    sidplayfp/sidtune/SidTuneTools.h \
    sidplayfp/sidtune/SmartPtr.h \
    sidplayfp/component.h \
    sidplayfp/event.h \
    sidplayfp/EventScheduler.h \
    sidplayfp/mixer.h \
    sidplayfp/player.h \
    sidplayfp/psiddrv.h \
    sidplayfp/reloc65.h \
    sidplayfp/romCheck.h \
    sidplayfp/sidbuilder.h \
    sidplayfp/SidConfig.h \
    sidplayfp/siddefs.h \
    sidplayfp/sidemu.h \
    sidplayfp/sidendian.h \
    sidplayfp/SidInfo.h \
    sidplayfp/SidInfoImpl.h \
    sidplayfp/sidmd5.h \
    sidplayfp/sidmemory.h \
    sidplayfp/sidplayfp.h \
    sidplayfp/sidrandom.h \
    sidplayfp/SidTune.h \
    sidplayfp/SidTuneInfo.h \
    sidplayfp/stringutils.h \
    sidplayfp/utils/MD5/MD5.h \
    sidplayfp/utils/MD5/MD5_Defs.h \
    sidplayfp/utils/STILview/stil.h \
    sidplayfp/utils/STILview/stildefs.h \
    sidplayfp/utils/iniParser.h \
    sidplayfp/utils/SidDatabase.h \
    sidplayer.h \
    siddevice.h \
    residfp-builder/residfp/resample/Resampler.h \
    residfp-builder/residfp/resample/SincResampler.h \
    residfp-builder/residfp/resample/TwoPassSincResampler.h \
    residfp-builder/residfp/resample/ZeroOrderResampler.h \
    residfp-builder/residfp/array.h \
    residfp-builder/residfp/Dac.h \
    residfp-builder/residfp/EnvelopeGenerator.h \
    residfp-builder/residfp/ExternalFilter.h \
    residfp-builder/residfp/Filter.h \
    residfp-builder/residfp/Filter6581.h \
    residfp-builder/residfp/Filter8580.h \
    residfp-builder/residfp/FilterModelConfig.h \
    residfp-builder/residfp/Integrator.h \
    residfp-builder/residfp/OpAmp.h \
    residfp-builder/residfp/Potentiometer.h \
    residfp-builder/residfp/SID.h \
    residfp-builder/residfp/Spline.h \
    residfp-builder/residfp/Voice.h \
    residfp-builder/residfp/WaveformCalculator.h \
    residfp-builder/residfp/WaveformGenerator.h \
    residfp-builder/residfp-emu.h \
    residfp-builder/residfp.h \
    residfp-builder/residfp/siddefs-fp.h

FORMS    += mainwindow.ui

OTHER_FILES +=

RESOURCES += \
    resources.qrc
