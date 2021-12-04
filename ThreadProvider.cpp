#include "ThreadProvider.h"


int ThreadProvider::getMaxQueueLength() const
{
    return max_queue_len;
}

void ThreadProvider::setMaxQueueLength(int value)
{
    max_queue_len = value;
}

ThreadProvider::~ThreadProvider()
{
    stop();
}

void ThreadProvider::push(const FramePtrWrapper& d)
{
    mutex.lock();
    while(curQueueSize >= max_queue_len)
    {
        data_queue.pop_front();
        --curQueueSize;
    }
    data_queue.emplace_back(d);
    ++curQueueSize;
    mutex.unlock();
}

void ThreadProvider::push(FramePtrWrapper&& d)
{
    mutex.lock();
    while(curQueueSize >= max_queue_len)
    {
        data_queue.pop_front();
        --curQueueSize;
    }
    data_queue.emplace_back(d);
    ++curQueueSize;
    mutex.unlock();
}

FramePtrWrapper ThreadProvider::pop()
{
    mutex.lock();
    if(data_queue.empty())
    {
        mutex.unlock();
        return FramePtrWrapper();
    }
    FramePtrWrapper d;
    d.swap(data_queue.front());
    data_queue.pop_front();
    --curQueueSize;
    mutex.unlock();
    return d;
}

FramePtrWrapper ThreadProvider::top()
{
    mutex.lock();
    if(data_queue.empty())
    {
        mutex.unlock();
        return FramePtrWrapper();
    }
    FramePtrWrapper d(data_queue.front());
    mutex.unlock();
    return d;
}

void ThreadProvider::start()
{
    is_exit = false;
    QThread::start();
}

void ThreadProvider::stop()
{
    mutex.lock();
    is_exit = true;
    data_queue.clear();
    curQueueSize = 0;
    mutex.unlock();
    if(isRunning())
    {
        quit();
        wait();
    }
}
