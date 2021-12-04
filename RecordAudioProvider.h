#ifndef RECORDAUDIOPROVIDER_H
#define RECORDAUDIOPROVIDER_H

#include "AudioProvider.h"

class QAudioInput;
class QIODevice;
class RecordAudioProvider: public AudioProvider
{
private:
    QAudioInput* input = nullptr;
    QIODevice* io = nullptr;
public:
    RecordAudioProvider();
    bool init();
    void stop();
    void run();
};

#endif // RECORDAUDIOPROVIDER_H
