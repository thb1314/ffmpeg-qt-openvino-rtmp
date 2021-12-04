#ifndef VIDEOPROVIDER_H
#define VIDEOPROVIDER_H

#include "ThreadProvider.h"

class ImageFilter;

class VideoProvider: public ThreadProvider
{
public:
    enum VideoType
    {
        Camera = 0,
        File
    };
protected:
    int width = 0;
    int height = 0;
    int fps = 0;
    VideoType type = Camera;
    ImageFilter* filter = nullptr;
public:
    VideoProvider(VideoType type = Camera);
    virtual ~VideoProvider();
    virtual bool init() = 0;
    int getFps() const;
    int getHeight() const;
    int getWidth() const;
    virtual void setFilter(ImageFilter* filter);
};

#endif // VIDEOPROVIDER_H
