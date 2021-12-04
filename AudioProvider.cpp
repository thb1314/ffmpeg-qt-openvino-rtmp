#include "AudioProvider.h"

int AudioProvider::getChannels() const
{
    return channels;
}

void AudioProvider::setChannels(int value)
{
    channels = value;
}

int AudioProvider::getSampleRate() const
{
    return sampleRate;
}

void AudioProvider::setSampleRate(int value)
{
    sampleRate = value;
}

int AudioProvider::getSampleByte() const
{
    return sampleByte;
}

void AudioProvider::setSampleByte(int value)
{
    sampleByte = value;
}

int AudioProvider::getNbSamples() const
{
    return nbSamples;
}

void AudioProvider::setNbSamples(int value)
{
    nbSamples = value;
}

AudioProvider::AudioProvider(AudioType type): type(type)
{

}

AudioProvider::~AudioProvider()
{
    stop();
}
