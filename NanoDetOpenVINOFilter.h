#ifndef NANODETOPENVINOFILTER_H
#define NANODETOPENVINOFILTER_H

#include <string>
#include <vector>
#include <fstream>
#include <inference_engine.hpp>
#include <opencv2/opencv.hpp>

#include "ImageFilter.h"



class OpenVINOFilter: public ImageFilter
{
protected:
    std::string network_xml_filepath;
    std::vector<std::string> input_names;
    InferenceEngine::InferRequest infer_request;
    InferenceEngine::ExecutableNetwork executable_network;
    virtual void predict(cv::Mat& image) = 0;
public:
    OpenVINOFilter(const std::string& network_xml_filepath);

    virtual void operator()(cv::Mat& image)
    {
        this->predict(image);
    }
};

class NanoDetOpenVINOFilter: public OpenVINOFilter
{
private:
    struct HeadInfo
    {
        std::string cls_layer;
        std::string dis_layer;
        int stride;
    };
    struct BoxInfo
    {
        float x1;
        float y1;
        float x2;
        float y2;
        float score;
        int label;
    };
    bool file2strlist(const std::string& filename, std::vector<std::string>& ret)
    {
        std::ifstream file(filename);
        if(!file.is_open())
        {
            std::cerr << "can not open file" << std::endl;
            return false;
        }
        {
            std::string line;
            while(std::getline(file, line))
                ret.push_back(line);
        }
        file.close();
        return true;
    }
    void preprocess(cv::Mat& image, InferenceEngine::Blob::Ptr& blob);

    inline void decode_infer(const float*& cls_pred, const float*& dis_pred, int stride, float threshold, std::vector<std::vector<BoxInfo>>& results);

    const int color_list[80][3] =
    {
        {216, 82, 24},
        {236, 176, 31},
        {125, 46, 141},
        {118, 171, 47},
        { 76, 189, 237},
        {238, 19, 46},
        { 76, 76, 76},
        {153, 153, 153},
        {255,  0,  0},
        {255, 127,  0},
        {190, 190,  0},
        {  0, 255,  0},
        {  0,  0, 255},
        {170,  0, 255},
        { 84, 84,  0},
        { 84, 170,  0},
        { 84, 255,  0},
        {170, 84,  0},
        {170, 170,  0},
        {170, 255,  0},
        {255, 84,  0},
        {255, 170,  0},
        {255, 255,  0},
        {  0, 84, 127},
        {  0, 170, 127},
        {  0, 255, 127},
        { 84,  0, 127},
        { 84, 84, 127},
        { 84, 170, 127},
        { 84, 255, 127},
        {170,  0, 127},
        {170, 84, 127},
        {170, 170, 127},
        {170, 255, 127},
        {255,  0, 127},
        {255, 84, 127},
        {255, 170, 127},
        {255, 255, 127},
        {  0, 84, 255},
        {  0, 170, 255},
        {  0, 255, 255},
        { 84,  0, 255},
        { 84, 84, 255},
        { 84, 170, 255},
        { 84, 255, 255},
        {170,  0, 255},
        {170, 84, 255},
        {170, 170, 255},
        {170, 255, 255},
        {255,  0, 255},
        {255, 84, 255},
        {255, 170, 255},
        { 42,  0,  0},
        { 84,  0,  0},
        {127,  0,  0},
        {170,  0,  0},
        {212,  0,  0},
        {255,  0,  0},
        {  0, 42,  0},
        {  0, 84,  0},
        {  0, 127,  0},
        {  0, 170,  0},
        {  0, 212,  0},
        {  0, 255,  0},
        {  0,  0, 42},
        {  0,  0, 84},
        {  0,  0, 127},
        {  0,  0, 170},
        {  0,  0, 212},
        {  0,  0, 255},
        {  0,  0,  0},
        { 36, 36, 36},
        { 72, 72, 72},
        {109, 109, 109},
        {145, 145, 145},
        {182, 182, 182},
        {218, 218, 218},
        {  0, 113, 188},
        { 80, 182, 188},
        {127, 127,  0},
    };
    std::vector<HeadInfo> heads_infos
    {
        {"cls_pred_stride_8", "dis_pred_stride_8", 8},
        {"cls_pred_stride_16", "dis_pred_stride_16", 16},
        {"cls_pred_stride_32", "dis_pred_stride_32", 32},
    };
    int class_number;
    std::string index2class_filepath;
    std::vector<std::string> index2class;
    float confidence_threshold;
    float nms_threshold;
    int input_height;
    int input_width;
public:
    NanoDetOpenVINOFilter(const std::string& network_xml_filepath,
                          int class_number,
                          const std::string& index2class_filepath,
                          float confidence_threshold = 0.4,
                          float nms_threshold = 0.5
                         );

    void predict(cv::Mat& image);

};

#endif // NANODETOPENVINOFILTER_H
