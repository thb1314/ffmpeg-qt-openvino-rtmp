#ifndef FILEAUDIOPROVIDER_H
#define FILEAUDIOPROVIDER_H

#include "AudioProvider.h"

#include <string>

struct AVFormatContext;
struct AVCodecContext;
struct SwrContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;

class FileAudioProvider : public AudioProvider
{
private:

public:
    AVCodecContext* ac = nullptr;
    AVFormatContext* ic = nullptr;

    AVCodecContext* vc = nullptr;
    int audio_index = -1;
    SwrContext* actx = nullptr;
    SwsContext* vctx = nullptr;
    std::string url;
    AVFrame* frame;
    AVPacket* pkt;
    static bool is_init_ffmpeg;
    FileAudioProvider(const std::string& url);
    bool init();
    void stop();
    void run();
    ~FileAudioProvider();
};

#endif // FILEAUDIOPROVIDER_H
