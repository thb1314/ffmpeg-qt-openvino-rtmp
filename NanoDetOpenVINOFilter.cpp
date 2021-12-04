
#include "NanoDetOpenVINOFilter.h"


OpenVINOFilter::OpenVINOFilter(const std::string& network_xml_filepath) : network_xml_filepath(network_xml_filepath)
{
    InferenceEngine::Core ie;
    std::map<std::string, std::string> config = {{ InferenceEngine::PluginConfigParams::KEY_CPU_THREADS_NUM, "8"}};
    std::string device_name("CPU");
    InferenceEngine::CNNNetwork network_reader;

    InferenceEngine::CNNNetwork model = ie.ReadNetwork("./model/nanodet-simp.xml");
    model.setBatchSize(1);
    InferenceEngine::InputInfo::Ptr input_info = model.getInputsInfo().begin()->second;

    for(auto& item : model.getInputsInfo())
    {
        const std::string& input_name = item.first;
        input_names.push_back(input_name);
    }
    for(auto& item : model.getOutputsInfo())
    {
        auto&  output_info = item.second;
        std::string output_name = item.first;
        output_info->setPrecision(InferenceEngine::Precision::FP32);
    }

    executable_network = ie.LoadNetwork(model, device_name, config);
    infer_request = executable_network.CreateInferRequest();
}


void NanoDetOpenVINOFilter::preprocess(cv::Mat& image, InferenceEngine::Blob::Ptr& blob)
{
    int img_w = image.cols;
    int img_h = image.rows;
    int channels = 3;
    InferenceEngine::MemoryBlob::Ptr mblob = InferenceEngine::as<InferenceEngine::MemoryBlob>(blob);
    if(!mblob)
    {
        THROW_IE_EXCEPTION << "We expect blob to be inherited from MemoryBlob in matU8ToBlob, "
                           << "but by fact we were not able to cast inputBlob to MemoryBlob";
    }

    // locked memory holder should be alive all time while access to its buffer happens
    auto mblobHolder = mblob->wmap();

    float* blob_data = mblobHolder.as<float*>();


    for(size_t c = 0; c < channels; c++)
    {
        for(size_t  h = 0; h < img_h; h++)
        {
            for(size_t w = 0; w < img_w; w++)
            {
                blob_data[c * img_w * img_h + h * img_w + w] =
                    (float)image.at<cv::Vec3b>(h, w)[c];
            }
        }
    }
}

void NanoDetOpenVINOFilter::decode_infer(const float*& cls_pred, const float*& dis_pred, int stride, float threshold, std::vector<std::vector<NanoDetOpenVINOFilter::BoxInfo> >& results)
{
    int feature_h = input_height / stride;
    int feature_w = input_width / stride;
    int num_class_ = 80;
    for(int idx = 0; idx < feature_h * feature_w; idx++)
    {
        int row = idx / feature_w;
        int col = idx % feature_w;
        float score = 0;
        int cur_label = 0;

        for(int label = 0; label < num_class_; label++)
        {
            if(cls_pred[idx * num_class_ + label] > score)
            {
                score = cls_pred[idx * num_class_ + label];
                cur_label = label;
            }
        }

        if(score > threshold)
        {
            const float* bbox_pred = dis_pred + idx * 4;
            results[cur_label].push_back({bbox_pred[0], bbox_pred[1],
                                          bbox_pred[2], bbox_pred[3],
                                          score, cur_label});
        }

    }
}

NanoDetOpenVINOFilter::NanoDetOpenVINOFilter(const std::string& network_xml_filepath, int class_number, const std::string& index2class_filepath, float confidence_threshold, float nms_threshold)
    : OpenVINOFilter(network_xml_filepath),
      class_number(class_number),
      index2class_filepath(index2class_filepath),
      confidence_threshold(confidence_threshold),
      nms_threshold(nms_threshold)
{
    assert(input_names.size() == 1);
    file2strlist(this->index2class_filepath, this->index2class);
    auto inputBlob = infer_request.GetBlob(input_names[0]);
    auto& dims = inputBlob->getTensorDesc().getDims();
    assert(4 == dims.size());
    input_height = static_cast<int>(dims[2]);
    input_width  = static_cast<int>(dims[3]);
}

void NanoDetOpenVINOFilter::predict(cv::Mat& image)
{
    static auto inputBlob = infer_request.GetBlob(input_names[0]);
    cv::Mat dst;
    float ori_height = image.size[0];
    float ori_width = image.size[1];
    cv::resize(image, dst, cv::Size(input_width, input_height), 0, 0, cv::INTER_NEAREST);
    preprocess(dst, inputBlob);

    infer_request.Infer();
    std::vector<std::vector<BoxInfo>> results;
    results.resize(class_number);
    for(const auto& head_info : heads_infos)
    {
        const InferenceEngine::Blob::Ptr dis_pred_blob = infer_request.GetBlob(head_info.dis_layer);
        const InferenceEngine::Blob::Ptr cls_pred_blob = infer_request.GetBlob(head_info.cls_layer);

        auto mdis_pred = InferenceEngine::as<InferenceEngine::MemoryBlob>(dis_pred_blob);
        auto mdis_pred_holder = mdis_pred->rmap();
        const float* dis_pred = mdis_pred_holder.as<const float*>();

        auto mcls_pred = InferenceEngine::as<InferenceEngine::MemoryBlob>(cls_pred_blob);
        auto mcls_pred_holder = mcls_pred->rmap();
        const float* cls_pred = mcls_pred_holder.as<const float*>();
        decode_infer(cls_pred, dis_pred, head_info.stride, confidence_threshold, results);
    }

    std::vector<cv::Rect> boxes;
    std::vector<int> classIds;
    std::vector<float> confidences;
    boxes.clear();
    classIds.clear();
    confidences.clear();
    static std::vector<int> indices;

    std::vector<BoxInfo> pred_bboxes;
    for(int i = 0; i < class_number; ++i)
    {
        boxes.clear();
        confidences.clear();
        auto& result = results[i];
        for(auto& item : result)
        {
            classIds.push_back(i);
            confidences.push_back(item.score);
            boxes.push_back(cv::Rect(item.x1, item.y1, item.x2, item.y2));
        }
        indices.clear();
        cv::dnn::NMSBoxes(boxes, confidences, confidence_threshold, nms_threshold, indices);
        for(auto index : indices)
            pred_bboxes.push_back(result[index]);
    }


    cv::Scalar rectColor, textColor; //box 和 text 的颜色
    cv::Rect box, textBox;
    int idx = 0;
    cv::Size labelSize;
    for(size_t i = 0; i < int(pred_bboxes.size()); ++i)
    {
        auto& item = pred_bboxes[i];
        item.x1 = item.x1 / input_width * ori_width;
        item.x2 = item.x2 / input_width * ori_width;
        item.y1 = item.y1 / input_height * ori_height;
        item.y2 = item.y2 / input_height * ori_height;
        box = cv::Rect(int(item.x1), int(item.y1), int(item.x2 - item.x1), int(item.y2 - item.y1));
        idx = item.label;
        const cv::String& className = index2class[idx];
        labelSize = cv::getTextSize(className, cv::FONT_HERSHEY_COMPLEX, 0.5, 1, 0);

        textBox = cv::Rect(cv::Point(box.x - 1, box.y),
                           cv::Point(box.x + labelSize.width, box.y - labelSize.height));
        rectColor = cv::Scalar(color_list[idx][0], color_list[idx][1], color_list[idx][2]);
        textColor = cv::Scalar(255, 255, 255);

        cv::rectangle(image, box, rectColor, 2, cv::LINE_AA, 0);
        cv::rectangle(image, textBox, rectColor, -1, cv::LINE_AA, 0);
        cv::putText(image, className, cv::Point(box.x, box.y - 2), cv::FONT_HERSHEY_COMPLEX, 0.5, textColor, 1, 8);
    }

}
