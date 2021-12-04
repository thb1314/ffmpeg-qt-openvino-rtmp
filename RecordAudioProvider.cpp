#include <QAudioFormat>
#include <QAudioDeviceInfo>
#include <QAudioInput>
#include "RecordAudioProvider.h"
#include "Utils.h"

RecordAudioProvider::RecordAudioProvider()
{

}

bool RecordAudioProvider::init()
{
    stop();
    ///1 qt音频开始录制
    QAudioFormat fmt;
    fmt.setSampleRate(sampleRate);
    fmt.setChannelCount(channels);
    fmt.setSampleSize(sampleByte * 8);
    fmt.setCodec("audio/pcm");
    fmt.setByteOrder(QAudioFormat::LittleEndian);
    fmt.setSampleType(QAudioFormat::UnSignedInt);
    QAudioDeviceInfo info = QAudioDeviceInfo::defaultInputDevice();
    if(!info.isFormatSupported(fmt))
        fmt = info.nearestFormat(fmt);

    input = new QAudioInput(fmt, this);
    //开始录制音频
    io = input->start();
    if(!io)
        return false;
    return true;
}

void RecordAudioProvider::stop()
{
    AudioProvider::stop();
    if(input)
        input->stop();
    if(io)
        io->close();
    if(input)
        delete input;
    input = nullptr;
    io = nullptr;
}

void RecordAudioProvider::run()
{
    //一次读取一帧音频的字节数
    int readSize = nbSamples * channels * sampleByte;
    char* buf = new char[readSize];

    while(!is_exit)
    {
        //读取已录制音频
        //一次读取一帧音频
        if(input->bytesReady() < readSize)
        {
            QThread::msleep(1);
            continue;
        }

        int size = 0;
        while(size != readSize)
        {
            int len = io->read(buf + size, readSize - size);
            if(len < 0)
                break;
            size += len;
        }
        if(size != readSize)
            continue;

        //已经读取一帧音频
        push(FramePtrWrapper(buf, readSize, Utils::get_curtime()));
    }

    delete[] buf;
}
