#ifndef XMEDIAENCODE_H
#define XMEDIAENCODE_H

struct AVFrame;
struct AVPacket;
struct AVCodecContext;

#include <string>

struct AVFrame;
struct AVPacket;
class AVCodecContext;
enum XSampleFMT
{
    X_S16 = 1,
    X_FLATP = 8
};

class XMediaEncode
{
public:

    ///输入参数
    int inWidth = 1280;
    int inHeight = 720;
    int inPixSize = 3;
    int channels = 2;
    int sampleRate = 44100;
    XSampleFMT inSampleFmt = X_S16;

    ///输出参数
    int outWidth = inWidth;
    int outHeight = inHeight;
    int bitrate = 4000000;//压缩后每秒视频的bit位大小 50kB
    int fps = 25;
    int nbSample = 1024;
    XSampleFMT outSampleFmt = X_FLATP;

    // 工厂生产方法
    static XMediaEncode* getInstance(unsigned char index = 0);

    // 初始化像素格式转换的上下文初始化
    virtual bool initScale() = 0;

    // 音频重采样上下文初始化
    virtual bool initResample() = 0;

    //返回值无需调用者清理
    virtual AVFrame* aduio_resample(char* data) = 0;
    virtual AVFrame* bgr2yuv(char* rgb) = 0;

    //视频编码器初始化
    virtual bool initVideoCodec() = 0;
    //音频编码初始化
    virtual bool initAudioCodec() = 0;

    // 视频编码
    virtual AVPacket* encodeVideo(AVFrame* frame, int64_t pts) = 0;
    // 音频编码
    virtual AVPacket* encodeAudio(AVFrame* frame, int64_t pts) = 0;

    virtual void setLastError(const std::string&) = 0;
    virtual const std::string& getLastError(void) = 0;
    virtual ~XMediaEncode() = default;
    virtual void close() = 0;

    AVCodecContext* vc = 0; // 编码器上下文
    AVCodecContext* ac = 0; // 音频编码器上下文
protected:
    XMediaEncode() = default;
    XMediaEncode(const XMediaEncode& other) = default;
};

#endif // XMEDIAENCODE_H
