#ifndef IMAGEFILTER_H
#define IMAGEFILTER_H


class ImageFilter
{
public:
    virtual void operator()(cv::Mat& image) = 0;
    virtual ~ImageFilter() = default;
};

#endif // IMAGEFILTER_H
