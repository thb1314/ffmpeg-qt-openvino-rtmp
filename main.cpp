#include <iostream>
#include <cstdint>
#include <ctime>
#include <memory>
#include <QCoreApplication>

#include <QThread>
#include <QAudioFormat>
#include <QAudioOutput>

#include "CameraVideoProvider.h"
#include "Utils.h"
#include "RecordAudioProvider.h"
#include "FileVideoProvider.h"
#include "FileAudioProvider.h"
#include "XRtmp.h"
#include "XMediaEncode.h"
#include "NanoDetOpenVINOFilter.h"


int start_up()
{
    std::unique_ptr<VideoProvider> video_provider(new CameraVideoProvider());
    std::unique_ptr<AudioProvider> audio_provider(new RecordAudioProvider());

    char outUrl[] = "rtmp://192.168.0.101/live";

    audio_provider->init();
    video_provider->init();
    int64_t begintime = Utils::get_curtime();
    audio_provider->start();
    video_provider->start();

    // 音频重采样 上下文初始化
    XMediaEncode* xe = XMediaEncode::getInstance();
    xe->channels = audio_provider->getChannels();
    xe->nbSample = audio_provider->getNbSamples();
    xe->sampleRate = audio_provider->getSampleRate();
    assert(audio_provider->getSampleByte() == 2);
    xe->inSampleFmt = XSampleFMT::X_S16;
    xe->outSampleFmt = XSampleFMT::X_FLATP;
    // 视频缩放配置
    xe->fps = video_provider->getFps();
    xe->inHeight = video_provider->getHeight();
    xe->outHeight = video_provider->getHeight();
    xe->inWidth = video_provider->getWidth();
    xe->outWidth = video_provider->getWidth();
    // bgr
    xe->inPixSize = 3;
    // 视频编码初始化 视频编码器初始化
    if(!xe->initScale())
        return -1;
    if(!xe->initVideoCodec())
        return -1;

    // 音频重采样上下文初始化
    if(!xe->initResample())
        return -1;
    // 初始化音频编码器
    if(!xe->initAudioCodec())
        return -1;

    // 输出封装器和音频流配置
    // a 创建输出封装器上下文
    XRtmp* xr = XRtmp::getInstance(0);
    if(!xr->init(outUrl))
        return -1;
    // b 添加音频流 添加视频流
    int video_stream_index = -1;
    int audio_stream_index = -1;
    audio_stream_index = xr->addStream(xe->ac);
    video_stream_index = xr->addStream(xe->vc);
    if(-1 == audio_stream_index || -1 == video_stream_index)
        return -1;

    // 打开rtmp 的网络输出IO
    // 写入封装头
    if(!xr->sendHead())
        return -1;
    int i = 0;
    int ret = 0;
    try
    {
        while(i < 1000)
        {
            ++i;
            auto audio_data_wraper = audio_provider->pop();
            int audio_data_size = audio_data_wraper.getByteSize();
            auto video_data_wraper = video_provider->pop();
            int video_data_size = video_data_wraper.getByteSize();
            if(0 == audio_data_size && 0 == video_data_size)
                continue;
            if(0 != audio_data_size)
            {
                //重采样源数据
                char* audio_data = (char*)audio_data_wraper.getDataPtr();
                AVFrame* pcm = xe->aduio_resample(audio_data);
                if(!pcm)
                    continue;
                int64_t audio_timestamp = audio_data_wraper.getTimestamp();
                AVPacket* pkt = xe->encodeAudio(pcm, audio_timestamp - begintime);
                if(!pkt)
                    continue;
                // 推流
                xr->sendFrame(pkt, audio_stream_index);
//                if(ret == 0)
//                    std::cout << "@A@" << std::endl;

            }
            if(0 != video_data_size)
            {
                char* video_data = (char*)video_data_wraper.getDataPtr();
                int64_t video_timestamp = video_data_wraper.getTimestamp();
                AVFrame* yuv = xe->bgr2yuv(video_data);
                if(!yuv)
                    continue;
                AVPacket* pkt = xe->encodeVideo(yuv, video_timestamp - begintime);
                if(!pkt)
                    continue;
                // 推流
                xr->sendFrame(pkt, video_stream_index);
//                if(ret == 0)
//                    std::cout << "@V@" << std::endl;
            }

        }
    }
    catch(std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
    }
    audio_provider->stop();
    video_provider->stop();
    xe->close();
    xr->close();
    return 0;
}

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

void audio_play()
{
    std::unique_ptr<FileAudioProvider> audio_provider;
    try
    {
        audio_provider = std::unique_ptr<FileAudioProvider>(new FileAudioProvider("Gee.mp4"));
        audio_provider->init();
    }
    catch(std::runtime_error& e)
    {
        std::cout << e.what() << std::endl;
        return;
    }
    audio_provider->start();

    QAudioFormat fmt;
    fmt.setSampleRate(audio_provider->getSampleRate());
    fmt.setSampleSize(8 * audio_provider->getSampleByte());
    fmt.setChannelCount(audio_provider->getChannels());
    fmt.setByteOrder(QAudioFormat::LittleEndian);
    fmt.setCodec("audio/pcm");
    fmt.setSampleType(QAudioFormat::UnSignedInt);

    QAudioOutput out(fmt);
    QIODevice* io = out.start();
    int size = out.periodSize();

    while(audio_provider->isRunning())
    {
        auto audio_data_wraper = audio_provider->pop();
        int audio_data_size = audio_data_wraper.getByteSize();
        if(0 == audio_data_size)
            continue;
        while(out.bytesFree() < audio_data_size)
        {
            QThread::usleep(10);
            continue;
        }
        auto pcm = audio_data_wraper.getDataPtr();
        io->write((const char*)pcm, audio_data_size);
    }
}


int camera_with_audiofile_rtmp()
{
    std::unique_ptr<VideoProvider> video_provider(new CameraVideoProvider());
    std::unique_ptr<AudioProvider> audio_provider(new FileAudioProvider("Gee.mp4"));

    char outUrl[] = "rtmp://localhost/live";

    audio_provider->init();
    video_provider->init();
    int64_t begintime = Utils::get_curtime();
    audio_provider->start();
    video_provider->start();

    // 音频重采样 上下文初始化
    XMediaEncode* xe = XMediaEncode::getInstance();
    xe->channels = audio_provider->getChannels();
    xe->nbSample = audio_provider->getNbSamples();
    xe->sampleRate = audio_provider->getSampleRate();
    assert(audio_provider->getSampleByte() == 2);
    xe->inSampleFmt = XSampleFMT::X_S16;
    xe->outSampleFmt = XSampleFMT::X_FLATP;
    // 视频缩放配置
    xe->fps = video_provider->getFps();
    xe->inHeight = video_provider->getHeight();
    xe->outHeight = video_provider->getHeight();
    xe->inWidth = video_provider->getWidth();
    xe->outWidth = video_provider->getWidth();
    // bgr
    xe->inPixSize = 3;
    // 视频编码初始化 视频编码器初始化
    if(!xe->initScale())
        return -1;
    if(!xe->initVideoCodec())
        return -1;

    // 音频重采样上下文初始化
    if(!xe->initResample())
        return -1;
    // 初始化音频编码器
    if(!xe->initAudioCodec())
        return -1;

    // 输出封装器和音频流配置
    // a 创建输出封装器上下文
    XRtmp* xr = XRtmp::getInstance(0);
    if(!xr->init(outUrl))
        return -1;
    // b 添加音频流 添加视频流
    int video_stream_index = -1;
    int audio_stream_index = -1;
    audio_stream_index = xr->addStream(xe->ac);
    video_stream_index = xr->addStream(xe->vc);
    if(-1 == audio_stream_index || -1 == video_stream_index)
        return -1;

    // 打开rtmp 的网络输出IO
    // 写入封装头
    if(!xr->sendHead())
        return -1;

    int ret = 0;
    int64_t video_timestamp = 0;
    try
    {
        while(audio_provider->isRunning())
        {
            auto video_data_wraper = video_provider->pop();
            int video_data_size = video_data_wraper.getByteSize();

            auto audio_data_wraper = audio_provider->top();
            int audio_data_size = audio_data_wraper.getByteSize();
            if(0 == video_data_size && 0 == audio_data_size)
                continue;

            if(0 != video_data_size)
            {
                char* video_data = (char*)video_data_wraper.getDataPtr();
                video_timestamp = video_data_wraper.getTimestamp() - begintime;
                AVFrame* yuv = xe->bgr2yuv(video_data);
                if(!yuv)
                    continue;
                AVPacket* pkt = xe->encodeVideo(yuv, video_timestamp);
                if(!pkt)
                    continue;
                // 推流
                xr->sendFrame(pkt, video_stream_index);
//                if(ret == 0)
//                    std::cout << "@V@" << std::endl;
            }

            if(0 != audio_data_size)
            {
                int64_t audio_timestamp = audio_data_wraper.getTimestamp();
                if(audio_timestamp > video_timestamp)
                    continue;
                audio_provider->pop();
                //重采样源数据
                char* audio_data = (char*)audio_data_wraper.getDataPtr();
                AVFrame* pcm = xe->aduio_resample(audio_data);
                if(!pcm)
                    continue;

                AVPacket* pkt = xe->encodeAudio(pcm, audio_timestamp);
                if(!pkt)
                    continue;
                // 推流
                xr->sendFrame(pkt, audio_stream_index);

            }


        }
    }
    catch(std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
    }
    audio_provider->stop();
    video_provider->stop();
    xe->close();
    xr->close();
    return 0;
}

int filevideo_with_fileaudio()
{
    std::unique_ptr<VideoProvider> video_provider(new FileVideoProvider("D:\\JJDown\\Download\\erxianqiao.wmv"));
    std::unique_ptr<AudioProvider> audio_provider(new FileAudioProvider("D:\\JJDown\\Download\\erxianqiao.mp4"));
    std::unique_ptr<ImageFilter> filter(new NanoDetOpenVINOFilter("./model/nanodet-simp.xml", 80, "./model/labelmap_coco.txt"));

    char outUrl[] = "rtmp://10.170.16.186/live";

    audio_provider->init();
    video_provider->init();
    int64_t begintime = Utils::get_curtime();
    audio_provider->start();
    video_provider->start();
    video_provider->setFilter(filter.get());
    // 音频重采样 上下文初始化
    XMediaEncode* xe = XMediaEncode::getInstance();
    xe->channels = audio_provider->getChannels();
    xe->nbSample = audio_provider->getNbSamples();
    xe->sampleRate = audio_provider->getSampleRate();
    assert(audio_provider->getSampleByte() == 2);
    xe->inSampleFmt = XSampleFMT::X_S16;
    xe->outSampleFmt = XSampleFMT::X_FLATP;
    // 视频缩放配置
    xe->fps = video_provider->getFps();
    xe->inHeight = video_provider->getHeight();
    xe->outHeight = video_provider->getHeight();
    xe->inWidth = video_provider->getWidth();
    xe->outWidth = video_provider->getWidth();
    // bgr
    xe->inPixSize = 3;
    // 视频编码初始化 视频编码器初始化
    if(!xe->initScale())
        return -1;
    if(!xe->initVideoCodec())
        return -1;

    // 音频重采样上下文初始化
    if(!xe->initResample())
        return -1;
    // 初始化音频编码器
    if(!xe->initAudioCodec())
        return -1;

    // 输出封装器和音频流配置
    // a 创建输出封装器上下文
    XRtmp* xr = XRtmp::getInstance(0);
    if(!xr->init(outUrl))
        return -1;
    // b 添加音频流 添加视频流
    int video_stream_index = -1;
    int audio_stream_index = -1;
    audio_stream_index = xr->addStream(xe->ac);
    video_stream_index = xr->addStream(xe->vc);
    if(-1 == audio_stream_index || -1 == video_stream_index)
        return -1;

    // 打开rtmp 的网络输出IO
    // 写入封装头
    if(!xr->sendHead())
        return -1;

    int ret = 0;
    int64_t video_timestamp = 0;
    int64_t audio_timestamp = 0;
    try
    {
        while(audio_provider->isRunning())
        {
            auto video_data_wraper = video_provider->top();
            int video_data_size = video_data_wraper.getByteSize();
            int64_t tmp_video_timestemp = video_data_wraper.getTimestamp();

            int64_t curTime = Utils::get_curtime() - begintime;
            if(0 != video_data_size && tmp_video_timestemp <= curTime)
            {

                video_provider->pop();
                char* video_data = (char*)video_data_wraper.getDataPtr();
                video_timestamp = video_data_wraper.getTimestamp();

                AVFrame* yuv = xe->bgr2yuv(video_data);
                if(!yuv)
                    continue;
                AVPacket* pkt = xe->encodeVideo(yuv, video_timestamp);
                if(!pkt)
                    continue;
                // 推流
                bool rer_val = xr->sendFrame(pkt, video_stream_index);
                if(rer_val)
                    std::cout << "@V@" << std::endl;
            }

            while(true)
            {
                auto audio_data_wraper = audio_provider->top();
                int audio_data_size = audio_data_wraper.getByteSize();
                int tmp_audio_timestamp = audio_data_wraper.getTimestamp();

                if(audio_data_size == 0 || tmp_audio_timestamp > video_timestamp)
                    break;

                audio_timestamp = tmp_audio_timestamp;
                audio_provider->pop();
                //重采样源数据
                char* audio_data = (char*)audio_data_wraper.getDataPtr();
                AVFrame* pcm = xe->aduio_resample(audio_data);
                if(!pcm)
                    continue;
                AVPacket* pkt = xe->encodeAudio(pcm, audio_timestamp);
                if(!pkt)
                    continue;
                // 推流
                bool rer_val = xr->sendFrame(pkt, audio_stream_index);
                if(rer_val)
                    std::cout << "@A@" << std::endl;
            }


        }
    }
    catch(std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
    }
    audio_provider->stop();
    video_provider->stop();
    xe->close();
    xr->close();
    return 0;
}


int filevideo_with_fileaudio_to_flvfile()
{
    std::unique_ptr<VideoProvider> video_provider(new FileVideoProvider("D:\\JJDown\\Download\\erxianqiao.wmv"));
    std::unique_ptr<AudioProvider> audio_provider(new FileAudioProvider("D:\\JJDown\\Download\\erxianqiao.mp4"));
    std::unique_ptr<ImageFilter> filter(new NanoDetOpenVINOFilter("./model/nanodet-simp.xml", 80, "./model/labelmap_coco.txt"));
    char outUrl[] = "0.mp4";

    audio_provider->init();
    video_provider->init();
    int64_t begintime = Utils::get_curtime();
    video_provider->setFilter(filter.get());
    audio_provider->start();
    video_provider->start();
    // 音频重采样 上下文初始化
    XMediaEncode* xe = XMediaEncode::getInstance();
    xe->channels = audio_provider->getChannels();
    xe->nbSample = audio_provider->getNbSamples();
    xe->sampleRate = audio_provider->getSampleRate();
    assert(audio_provider->getSampleByte() == 2);
    xe->inSampleFmt = XSampleFMT::X_S16;
    xe->outSampleFmt = XSampleFMT::X_FLATP;
    // 视频缩放配置
    xe->fps = video_provider->getFps();
    xe->inHeight = video_provider->getHeight();
    xe->outHeight = video_provider->getHeight();
    xe->inWidth = video_provider->getWidth();
    xe->outWidth = video_provider->getWidth();
    // bgr
    xe->inPixSize = 3;
    // 视频编码初始化 视频编码器初始化
    if(!xe->initScale())
        return -1;
    if(!xe->initVideoCodec())
        return -1;

    // 音频重采样上下文初始化
    if(!xe->initResample())
        return -1;
    // 初始化音频编码器
    if(!xe->initAudioCodec())
        return -1;

    // 输出封装器和音频流配置
    // a 创建输出封装器上下文
    XRtmp* xr = XRtmp::getInstance(0);
    if(!xr->init(outUrl))
        return -1;
    // b 添加音频流 添加视频流
    int video_stream_index = -1;
    int audio_stream_index = -1;
    audio_stream_index = xr->addStream(xe->ac);
    video_stream_index = xr->addStream(xe->vc);
    if(-1 == audio_stream_index || -1 == video_stream_index)
        return -1;

    // 打开rtmp 的网络输出IO
    // 写入封装头
    if(!xr->sendHead())
        return -1;

    int ret = 0;
    int64_t video_timestamp = 0;
    int64_t audio_timestamp = 0;
    try
    {
        while(audio_provider->isRunning())
        {
            auto video_data_wraper = video_provider->top();
            int video_data_size = video_data_wraper.getByteSize();
            int64_t tmp_video_timestemp = video_data_wraper.getTimestamp();

            if(0 != video_data_size)
            {

                video_provider->pop();
                char* video_data = (char*)video_data_wraper.getDataPtr();
                video_timestamp = video_data_wraper.getTimestamp();

                AVFrame* yuv = xe->bgr2yuv(video_data);
                if(!yuv)
                    continue;
                AVPacket* pkt = xe->encodeVideo(yuv, video_timestamp);
                if(!pkt)
                    continue;
                // 推流
                bool rer_val = xr->sendFrame(pkt, video_stream_index);
                if(rer_val)
                    std::cout << "@V@" << std::endl;
            }

            while(true)
            {
                auto audio_data_wraper = audio_provider->top();
                int audio_data_size = audio_data_wraper.getByteSize();
                int tmp_audio_timestamp = audio_data_wraper.getTimestamp();

                if(audio_data_size == 0 || tmp_audio_timestamp > video_timestamp)
                    break;

                audio_timestamp = tmp_audio_timestamp;
                audio_provider->pop();
                //重采样源数据
                char* audio_data = (char*)audio_data_wraper.getDataPtr();
                AVFrame* pcm = xe->aduio_resample(audio_data);
                if(!pcm)
                    continue;
                AVPacket* pkt = xe->encodeAudio(pcm, audio_timestamp);
                if(!pkt)
                    continue;
                // 推流
                bool rer_val = xr->sendFrame(pkt, audio_stream_index);
                if(rer_val)
                    std::cout << "@A@" << std::endl;
            }


        }
    }
    catch(std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
    }
    audio_provider->stop();
    video_provider->stop();
    xe->close();
    xr->close();
    return 0;
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    // 观察是否有内存泄漏
//    for(int i = 0; i < 10; ++i)
//        std::cout << "start_up:" << start_up() << std::endl;
//    audio_play();

    filevideo_with_fileaudio_to_flvfile();
//    app.exit(0);
    return 0;
}
