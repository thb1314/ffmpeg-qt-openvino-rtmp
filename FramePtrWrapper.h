#ifndef FRAMEPTRWRAPPER_H
#define FRAMEPTRWRAPPER_H

#include <cstdint>

class FramePtrWrapper
{
private:
    void* data_ptr = nullptr;
    int byte_size = 0;
    int64_t timestamp = -1;
public:
    FramePtrWrapper() = default;
    FramePtrWrapper(void* data_ptr, int byte_size, int64_t timestamp = -1);
    FramePtrWrapper(int byte_size);

    //深拷贝 拷贝构造
    FramePtrWrapper(const FramePtrWrapper& other);
    // 赋值构造
    FramePtrWrapper& operator=(const FramePtrWrapper& other);
    // 移动构造
    FramePtrWrapper(FramePtrWrapper&& other);
    // 赋值移动构造
    FramePtrWrapper& operator=(FramePtrWrapper&& other);
    // bind
    void swap(FramePtrWrapper& other);
    ~FramePtrWrapper();

    void setDataPtr(void* data_ptr, int byte_size);
    void bindDataPtr(void* data_ptr, int byte_size);
    void* getDataPtr() const;
    void resize(int byte_size);

    int64_t getTimestamp() const;
    void setTimestamp(int64_t value);
    int getByteSize() const;
};

#endif // FRAMEPTRWRAPPER_H
