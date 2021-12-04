#ifndef AUDIOPROVIDER_H
#define AUDIOPROVIDER_H


#include "ThreadProvider.h"

class AudioProvider : public ThreadProvider
{
public:
    enum AudioType
    {
        Record = 0,
        File
    };
protected:
    int channels = 2;       //声道数
    int sampleRate = 44100; //样本率
    int sampleByte = 2;     //样板字节大小
    int nbSamples = 1024;   //一帧音频每个通道的样板数量
    AudioType type = Record;

public:
    AudioProvider(AudioType type = Record);
    virtual ~AudioProvider();
    virtual bool init() = 0;
    int getChannels() const;
    void setChannels(int value);
    int getSampleRate() const;
    void setSampleRate(int value);
    int getSampleByte() const;
    void setSampleByte(int value);
    int getNbSamples() const;
    void setNbSamples(int value);
};

#endif // AUDIOPROVIDER_H
