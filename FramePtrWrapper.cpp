#include "FramePtrWrapper.h"
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cstdio>
#include <iostream>


int64_t FramePtrWrapper::getTimestamp() const
{
    return timestamp;
}

void FramePtrWrapper::setTimestamp(int64_t value)
{
    timestamp = value;
}

int FramePtrWrapper::getByteSize() const
{
    return byte_size;
}

FramePtrWrapper::FramePtrWrapper(void* data_ptr, int byte_size, int64_t timestamp)
{
    if(byte_size <= 0)
    {
        this->data_ptr = nullptr;
        this->byte_size = 0;
        this->timestamp = -1;
        return;
    }
    this->data_ptr = malloc(byte_size);
    assert(NULL != this->data_ptr);
    assert(NULL != data_ptr);
    memcpy(this->data_ptr, data_ptr, byte_size);
    this->byte_size = byte_size;
    this->timestamp = timestamp;
}

FramePtrWrapper::FramePtrWrapper(int byte_size)
{
    if(byte_size <= 0)
    {
        this->data_ptr = nullptr;
        this->byte_size = 0;
        this->timestamp = -1;
        return;
    }
    this->data_ptr = malloc(byte_size);
    this->byte_size = byte_size;
    this->timestamp = -1;
}

FramePtrWrapper::FramePtrWrapper(const FramePtrWrapper& other)
{
    this->byte_size = other.byte_size;
    if(0 != this->byte_size)
    {
        this->data_ptr = malloc(this->byte_size);
        assert(NULL != this->data_ptr);
        memcpy(this->data_ptr, other.data_ptr, this->byte_size);
    }
    this->timestamp = other.timestamp;
}

FramePtrWrapper& FramePtrWrapper::operator=(const FramePtrWrapper& other)
{
    if(this->data_ptr)
        free(this->data_ptr);

    this->byte_size = other.byte_size;
    if(0 != this->byte_size)
    {
        this->data_ptr = malloc(this->byte_size);
        assert(NULL != this->data_ptr);
        memcpy(this->data_ptr, other.data_ptr, this->byte_size);
    }
    this->timestamp = other.timestamp;
    return *this;
}

FramePtrWrapper::FramePtrWrapper(FramePtrWrapper&& other)
{
    this->byte_size = other.byte_size;
    other.byte_size = 0;
    this->data_ptr = other.data_ptr;
    other.data_ptr = nullptr;
    this->timestamp = other.timestamp;
    other.timestamp = -1;
}

FramePtrWrapper& FramePtrWrapper::operator=(FramePtrWrapper&& other)
{
    if(this->data_ptr)
        free(this->data_ptr);
    this->byte_size = other.byte_size;
    other.byte_size = 0;
    this->data_ptr = other.data_ptr;
    other.data_ptr = nullptr;
    this->timestamp = other.timestamp;
    other.timestamp = -1;
    return *this;
}

void FramePtrWrapper::swap(FramePtrWrapper& other)
{
    std::swap(this->byte_size, other.byte_size);
    std::swap(this->data_ptr, other.data_ptr);
    std::swap(this->timestamp, other.timestamp);
}

FramePtrWrapper::~FramePtrWrapper()
{
    if(this->data_ptr)
    {
        free(this->data_ptr);
        this->data_ptr = nullptr;
    }
    this->byte_size = 0;
    this->timestamp = -1;
}

void FramePtrWrapper::setDataPtr(void* data_ptr, int byte_size)
{
    if(byte_size <= 0)
        return;
    if(this->data_ptr)
        free(this->data_ptr);

    this->data_ptr = malloc(byte_size);
    assert(NULL != this->data_ptr);
    assert(NULL != data_ptr);
    memcpy(this->data_ptr, data_ptr, byte_size);
    this->byte_size = byte_size;
}

void FramePtrWrapper::bindDataPtr(void* data_ptr, int byte_size)
{
    if(byte_size <= 0)
        return;
    this->data_ptr = data_ptr;
    assert(NULL != this->data_ptr);
    this->byte_size = byte_size;
}

void* FramePtrWrapper::getDataPtr() const
{
    return data_ptr;
}

void FramePtrWrapper::resize(int byte_size)
{
    if(this->data_ptr)
        free(this->data_ptr);
    this->byte_size = byte_size;
    if(0 != this->byte_size)
    {
        this->data_ptr = malloc(this->byte_size);
        assert(NULL != this->data_ptr);
    }
}
