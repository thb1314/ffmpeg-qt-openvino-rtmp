#include "FileVideoProvider.h"
#include "Utils.h"
#include "ImageFilter.h"

FileVideoProvider::FileVideoProvider(const char* url):
    url(url), VideoProvider(VideoType::File)
{
    max_queue_len = 20;
}

bool FileVideoProvider::init()
{
    cam.open(url.c_str());
    if(!cam.isOpened())
        return false;

    width = cam.get(cv::CAP_PROP_FRAME_WIDTH);
    height = cam.get(cv::CAP_PROP_FRAME_HEIGHT);
    fps = cam.get(cv::CAP_PROP_FPS);
    if(fps == 0)
        fps = 25;
    return true;
}

void FileVideoProvider::stop()
{
    VideoProvider::stop();
    if(cam.isOpened())
        cam.release();
}

void FileVideoProvider::run()
{
    cv::Mat frame;
    while(!is_exit)
    {
        if(!cam.read(frame))
        {
            QThread::msleep(1);
            continue;
        }
        if(frame.empty())
        {
            QThread::msleep(1);
            continue;
        }
        if(filter)
            (*filter)(frame);
        // 这里要确保数据是连续的
        int64_t timestamp = (int64_t)cam.get(cv::CAP_PROP_POS_MSEC) * 1000;
        while(data_queue.size() > max_queue_len / 3 * 2)
            QThread::msleep(1);
        push(FramePtrWrapper(frame.data, frame.cols * frame.rows * frame.elemSize(), timestamp));
    }
}


