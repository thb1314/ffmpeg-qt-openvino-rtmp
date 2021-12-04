#include <exception>
#include <iostream>
#include <cassert>
#include "FileAudioProvider.h"
#include "FramePtrWrapper.h"
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

#if !defined (_WIN32) && !defined (_WIN64)
#define LINUX
#include <sysconf.h>
#else
#define WINDOWS
#include <windows.h>
#endif
static int core_count()
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

#define CHECK_RETCODE(ret_code) if(ret_code != 0) \
{ \
char buffer[1024]; \
av_strerror(ret_code, buffer, sizeof(buffer) - 1); \
throw std::runtime_error(buffer); \
}

bool FileAudioProvider::is_init_ffmpeg = false;

FileAudioProvider::FileAudioProvider(const std::string& url):
    url(url), AudioProvider(AudioType::File)
{

    if(!is_init_ffmpeg)
    {
        is_init_ffmpeg = true;
        // 注册解码器
        avcodec_register_all();
        // 初始化网络库 可以打开rtsp rtmp http协议的流媒体视频
        avformat_network_init();
        // 初始化封装库
        av_register_all();
        // std::cout << "init ok" << std::endl;
    }

}

bool FileAudioProvider::init()
{

    AVDictionary* opts = NULL;
    av_dict_set(&opts, "rtsp_transport", "tcp", 0);
    // 网络延时时间
    av_dict_set(&opts, "max_delay", "500", 0);

    int ret = avformat_open_input(
                  &ic,
                  this->url.c_str(),
                  NULL, // NULL 表示自动选择解封装器
                  &opts  // 参数设置，比如rtsp的延时时间
              );
    CHECK_RETCODE(ret);
    avformat_find_stream_info(ic, 0);
    // 打印视频流详细信息
    av_dump_format(ic, 0, ic->filename, 0);

    // 音频
    audio_index = av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    auto& audio_codepar = ic->streams[audio_index]->codecpar;
    AVCodec* acodec = avcodec_find_decoder(audio_codepar->codec_id);
    if(!acodec)
    {
        throw std::runtime_error(std::string("can not find the codec id ") + std::to_string(audio_codepar->codec_id));
        return false;
    }
    // 根据解码器 创建解码器上下文
    ac = avcodec_alloc_context3(acodec);
    // 配置解码器上下文参数
    avcodec_parameters_to_context(ac, audio_codepar);
    ac->thread_count = core_count();
    // 打开解码器上下文
    ret = avcodec_open2(ac, NULL, NULL);
    CHECK_RETCODE(ret);

    if(2 != this->sampleByte)
        throw std::runtime_error("sample byte must be 2");
    // 音频重采样 上下文初始化
    actx = swr_alloc();

    this->sampleRate = ac->sample_rate;
    actx = swr_alloc_set_opts(actx,
                              av_get_default_channel_layout(this->channels),  // 输出通道数
                              AV_SAMPLE_FMT_S16, // 输出格式
                              this->sampleRate,  // 输出样本率
                              av_get_default_channel_layout(ac->channels), // 输入信息
                              ac->sample_fmt,
                              ac->sample_rate,
                              0, 0
                             );

    ret = swr_init(actx);
    CHECK_RETCODE(ret);

    return true;



}

void FileAudioProvider::stop()
{
    AudioProvider::stop();
    mutex.lock();
    if(actx)
    {
        swr_free(&actx);
        actx = nullptr;
    }
    if(ac)
    {
        avcodec_close(ac);
        ac = nullptr;
    }
    if(ic)
    {
        avformat_close_input(&ic);
        ic = nullptr;
    }
    mutex.unlock();

}

void FileAudioProvider::run()
{
    int ret = -1;
    if(!ac || !actx || !ic)
        return;
    int pcm_size = nbSamples * channels * sampleByte;
    uint8_t* pcm = new uint8_t[pcm_size];
    int try_time = 0;
    AVFrame*  frame = av_frame_alloc();
    AVPacket* pkt = av_packet_alloc();

    while(!is_exit)
    {
        av_packet_unref(pkt);
        ret = av_read_frame(ic, pkt);
        if(AVERROR_EOF == ret)
        {
            avcodec_send_packet(ac, NULL);
            if(0 != ret)
                break;
        }
        else if(0 != ret)
        {
            try_time++;
            if(try_time > 100)
                break;
            continue;
        }

        if(pkt->stream_index != audio_index)
            continue;

        ret = avcodec_send_packet(ac, pkt);
        if(0 != ret)
        {
            char buf[1024] = {0};
            av_strerror(ret, buf, sizeof(buf) - 1);
            std::cout << "avcodec_send_packet error! errmsg:" << buf << std::endl;
            break;
        }

        for(;;)
        {
            ret = avcodec_receive_frame(ac, frame);
            if(0 != ret)
                break;
            uint8_t* data[2] = {NULL, NULL};
            data[0] = pcm;
            int len = swr_convert(actx,
                                  data,
                                  nbSamples, // 输出样本数
                                  (const uint8_t**)frame->data,  // 输入
                                  frame->nb_samples
                                 );
            if(len <= 0)
                continue;
            // 写入时间戳，us
            while(this->data_queue.size() >= max_queue_len / 3 * 2)
            {
                QThread::usleep(10);
            }
            push(FramePtrWrapper(pcm, len * 2 * 2, av_rescale_q(frame->pts, ac->time_base, {1, 1000000})));
        }
    }
    is_exit = true;
    std::cout << "==============audio quit!=================" << std::endl;
    delete[] pcm;
    pcm = nullptr;
    av_packet_unref(pkt);
    av_packet_free(&pkt);
    av_frame_free(&frame);
}

FileAudioProvider::~FileAudioProvider()
{
    stop();
}
