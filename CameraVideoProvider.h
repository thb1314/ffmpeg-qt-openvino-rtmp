#ifndef CAMERAVIDEOPROVIDER_H
#define CAMERAVIDEOPROVIDER_H

#include "VideoProvider.h"
#include <opencv2/opencv.hpp>

class CameraVideoProvider : public VideoProvider
{
private:
    int cam_index = 0;
    cv::VideoCapture cam;
public:
    CameraVideoProvider(int cam_index = 0);
    bool init();
    void stop();
    void run();
};

#endif // CAMERAVIDEOPROVIDER_H
