#ifndef FILEVIDEOPROVIDER_H
#define FILEVIDEOPROVIDER_H

#include "VideoProvider.h"

#include <opencv2/opencv.hpp>
#include <string>

class FileVideoProvider : public VideoProvider
{
private:
    cv::VideoCapture cam;
    std::string url;

public:
    FileVideoProvider(const char* url = nullptr);
    bool init();
    void stop();
    void run();

};




#endif // FILEVIDEOPROVIDER_H
