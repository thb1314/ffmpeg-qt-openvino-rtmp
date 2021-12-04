#ifndef XRTMP_H
#define XRTMP_H

#include <string>

class AVCodecContext;
class AVPacket;
class XRtmp
{
public:

    //工厂生产方法
    static XRtmp* getInstance(unsigned char index = 0);

    //初始化封装器上下文
    virtual bool init(const char* url) = 0;

    //添加视频或者音频流 返回对应的index -1表示失败
    virtual int addStream(const AVCodecContext* c) = 0;

    //打开rtmp网络IO，发送封装头
    virtual bool sendHead() = 0;

    //rtmp 帧推流
    virtual bool sendFrame(AVPacket* pkt, int index) = 0;

    virtual void setLastError(const std::string&) = 0;
    virtual const std::string& getLastError(void) = 0;
    virtual void close() = 0;
    virtual ~XRtmp() = default;
protected:
    XRtmp() = default;
    XRtmp(const XRtmp& other) = default;
};

#endif // XRTMP_H
