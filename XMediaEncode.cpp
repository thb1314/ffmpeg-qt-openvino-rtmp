#include "XMediaEncode.h"
extern "C"
{
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
}
#include <string>

#if !defined (_WIN32) && !defined (_WIN64)
#define LINUX
#include <sysconf.h>
#else
#define WINDOWS
#include <windows.h>
#endif
int core_count()
{
    int count = 1; // 至少一个
#if defined (LINUX)
    count = (int)sysconf(_SC_NPROCESSORS_CONF);
#elif defined (WINDOWS)
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    count = (int)si.dwNumberOfProcessors;
#endif
    return count;
}

#include <iostream>


class CXMediaEncode : public XMediaEncode
{
private:
    std::string err_msg;
public:
    void setLastError(const std::string& buf)
    {
        err_msg = buf;
    }
    const std::string& getLastError()
    {
        return err_msg;
    }
    void close()
    {
        if(vsc)
        {
            sws_freeContext(vsc);
            vsc = NULL;
        }
        if(asc)
        {
            swr_free(&asc);
        }
        if(yuv)
        {
            av_frame_free(&yuv);
        }
        if(vc)
        {
            avcodec_free_context(&vc);
        }
        if(ac)
        {
            avcodec_free_context(&ac);
        }
        if(audio_frame)
        {
            av_frame_free(&audio_frame);
        }

        last_video_pts = 0;
        av_packet_unref(&vpack);
        last_audio_pts = 0;
        av_packet_unref(&apack);
    }

    bool initAudioCodec()
    {
        if(!createCodec(AV_CODEC_ID_AAC))
        {
            return false;
        }
        ac->bit_rate = 40000;
        ac->sample_rate = sampleRate;

        ac->sample_fmt = AV_SAMPLE_FMT_FLTP;
        ac->channels = channels;
        ac->time_base = {1, 1000000};
        ac->channel_layout = av_get_default_channel_layout(channels);
        return openCodec(&ac);
    }

    bool initVideoCodec()
    {
        // 初始化编码上下文
        //   a. 找到编码器
        AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
        if(!codec)
        {
            this->setLastError("Can`t find h264 encoder!");
            return false;
        }
        //   b. 创建编码器上下文
        vc = avcodec_alloc_context3(codec);
        if(!vc)
        {
            this->setLastError("avcodec_alloc_context3 failed!");
            return false;
        }
        //   c. 配置编码器参数
        vc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER; //全局参数
        vc->codec_id = codec->id;
        vc->thread_count = core_count();

        // 压缩后每秒视频的最大bit位大小 50kB
        vc->bit_rate = 100 * 1024 * 8;
        vc->width = outWidth;
        vc->height = outHeight;
        vc->time_base = { 1, 1000000 };
        vc->framerate = { fps, 1 };

        // 画面组的大小，多少帧一个关键帧
        vc->gop_size = 50;
        vc->max_b_frames = 5;
        vc->pix_fmt = AV_PIX_FMT_YUV420P;
        //   d. 打开编码器上下文
        int ret = avcodec_open2(vc, 0, 0);
        if(ret != 0)
        {
            char buf[1024] = { 0 };
            av_strerror(ret, buf, sizeof(buf) - 1);
            this->setLastError(buf);
            return false;
        }
        return true;
    }

    virtual AVPacket* encodeVideo(AVFrame* frame, int64_t pts)
    {
        av_packet_unref(&vpack);
        // h264编码
        while(pts == last_video_pts)
            pts += 1000;
        frame->pts = pts;
        last_video_pts = pts;

        // Supply a raw video or audio frame to the encoder.
        // Use avcodec_receive_packet() to retrieve buffered output packets.
        int ret = avcodec_send_frame(vc, frame);
        if(ret != 0)
            return NULL;
        // Read encoded data from the encoder.
        ret = avcodec_receive_packet(vc, &vpack);
        if(ret != 0 || vpack.size <= 0)
            return NULL;
//        vpack.stream_index =
        return &vpack;
    }

    bool initScale()
    {
        // 2. 初始化格式转换上下文
        vsc = sws_getCachedContext(vsc,
                                   inWidth, inHeight, AV_PIX_FMT_BGR24,     //源宽、高、像素格式
                                   outWidth, outHeight, AV_PIX_FMT_YUV420P,//目标宽、高、像素格式
                                   SWS_BICUBIC,  // 尺寸变化使用算法
                                   0, 0, 0
                                  );
        if(!vsc)
        {
            this->setLastError("sws_getCachedContext failed!");
            return false;
        }

        // 3. 初始化输出的数据结构
        yuv = av_frame_alloc();
        yuv->format = AV_PIX_FMT_YUV420P;
        yuv->width = inWidth;
        yuv->height = inHeight;
        yuv->pts = 0;
        // 分配yuv空间
        int ret = av_frame_get_buffer(yuv, 32);
        if(ret != 0)
        {
            char buf[1024] = { 0 };
            av_strerror(ret, buf, sizeof(buf) - 1);
            this->setLastError(buf);
            return false;
        }
        return true;
    }

    bool initResample()
    {

        asc = swr_alloc_set_opts(asc,
                                 av_get_default_channel_layout(channels), (AVSampleFormat)outSampleFmt, sampleRate,//输出格式
                                 av_get_default_channel_layout(channels), (AVSampleFormat)inSampleFmt, sampleRate, 0, 0);//输入格式
        if(!asc)
        {
            this->setLastError("swr_alloc_set_opts failed!");
            return false;
        }
        int ret = swr_init(asc);
        if(ret != 0)
        {
            char err[1024] = { 0 };
            av_strerror(ret, err, sizeof(err) - 1);
            this->setLastError(err);
            return false;
        }

        // 3 音频重采样输出空间分配
        audio_frame = av_frame_alloc();
        audio_frame->format = outSampleFmt;
        audio_frame->channels = channels;
        audio_frame->channel_layout = av_get_default_channel_layout(channels);
        audio_frame->nb_samples = nbSample; // 一帧音频一通道的采用数量
        ret = av_frame_get_buffer(audio_frame, 0); // 给 audio_frame 分配存储空间
        if(ret != 0)
        {
            char err[1024] = { 0 };
            av_strerror(ret, err, sizeof(err) - 1);
            this->setLastError(err);
            return false;
        }
        return true;
    }

    AVFrame* aduio_resample(char* data)
    {
        if(!audio_frame)
            return NULL;
        const uint8_t* indata[AV_NUM_DATA_POINTERS] = { 0 };
        indata[0] = (uint8_t*)data;
        int len = swr_convert(asc, audio_frame->data, audio_frame->nb_samples, //输出参数，输出存储地址和样本数量
                              indata, audio_frame->nb_samples
                             );
        if(len <= 0)
        {
            return NULL;
        }
        return audio_frame;
    }

    AVFrame* bgr2yuv(char* rgb)
    {
        // 输入的数据结构
        uint8_t* indata[AV_NUM_DATA_POINTERS] = { 0 };
        // indata[0] bgr bgr bgr
        indata[0] = (uint8_t*)rgb;
        int insize[AV_NUM_DATA_POINTERS] = { 0 };
        //一行（宽）数据的字节数
        insize[0] = inWidth * inPixSize;

        int h = sws_scale(vsc, indata, insize, 0, inHeight, //源数据
                          yuv->data, yuv->linesize);
        if(h <= 0)
        {
            return NULL;
        }
        return yuv;
    }

    AVPacket* encodeAudio(AVFrame* frame, int64_t pts)
    {
        av_packet_unref(&apack);
        while(pts <= last_audio_pts)
            pts += 1000;
        frame->pts = pts;
        last_audio_pts = pts;
        int ret = avcodec_send_frame(ac, frame);
        if(ret != 0)
            return NULL;

        ret = avcodec_receive_packet(ac, &apack);
        if(ret != 0)
            return NULL;
        return &apack;
    }

private:
    int64_t last_video_pts = 0;
    int64_t last_audio_pts = 0;
    SwsContext* vsc = NULL;     // 像素格式转换上下文
    AVFrame* yuv = NULL;        // 输出的YUV
    AVFrame* audio_frame = NULL;        // 输出的 audio_frame
    AVPacket vpack = {0};
    AVPacket apack = {0};

    SwrContext* asc = NULL;

    bool openCodec(AVCodecContext** c)
    {
        //打开音频编码器
        int ret = avcodec_open2(*c, 0, 0);
        if(ret != 0)
        {
            char err[1024] = { 0 };
            av_strerror(ret, err, sizeof(err) - 1);
            this->setLastError(err);
            avcodec_free_context(c);
            return false;
        }
        return true;
    }

    bool createCodec(AVCodecID cid)
    {
        // 4 初始化编码器 AV_CODEC_ID_AAC
        AVCodec* codec = avcodec_find_encoder(cid);
        if(!codec)
        {
            this->setLastError("avcodec_find_encoder  failed!");
            return false;
        }
        //音频编码器上下文
        ac = avcodec_alloc_context3(codec);
        if(!ac)
        {
            this->setLastError("avcodec_alloc_context3  failed!");
            return false;
        }

        ac->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        ac->thread_count = core_count();
        return true;
    }
};



XMediaEncode* XMediaEncode::getInstance(unsigned char index)
{
    static bool is_first = true;
    if(is_first)
    {
        // 注册所有的编解码器
        avcodec_register_all();
        is_first = false;
    }

    static CXMediaEncode cxm[255];
    return &cxm[index];
}
