#include "VideoProvider.h"
#include <exception>


VideoProvider::VideoProvider(VideoProvider::VideoType type) : type(type)
{

}

VideoProvider::~VideoProvider()
{
    stop();
}





int VideoProvider::getFps() const
{
    return fps;
}

int VideoProvider::getHeight() const
{
    return height;
}

int VideoProvider::getWidth() const
{
    return width;
}

void VideoProvider::setFilter(ImageFilter* filter)
{
    this->filter = filter;
}

