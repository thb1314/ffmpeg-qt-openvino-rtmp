#include "CameraVideoProvider.h"
#include "Utils.h"

CameraVideoProvider::CameraVideoProvider(int cam_index):
    VideoProvider(VideoProvider::VideoType::Camera), cam_index(cam_index)
{

}

bool CameraVideoProvider::init()
{
    cam.open(cam_index);
    if(!cam.isOpened())
    {
        return false;
    }
    width = cam.get(cv::CAP_PROP_FRAME_WIDTH);
    height = cam.get(cv::CAP_PROP_FRAME_HEIGHT);
    fps = cam.get(cv::CAP_PROP_FPS);
    if(fps == 0)
        fps = 25;
    return true;
}

void CameraVideoProvider::stop()
{
    VideoProvider::stop();
    if(cam.isOpened())
        cam.release();
}

void CameraVideoProvider::run()
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
        //这里要确保数据是连续的
        push(FramePtrWrapper(frame.data, frame.cols * frame.rows * frame.elemSize(), Utils::get_curtime()));
    }
}
