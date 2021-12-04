#ifndef THREADPROVIDER_H
#define THREADPROVIDER_H

#include <QThread>
#include <QMutex>
#include <QQueue>
#include "FramePtrWrapper.h"

class ThreadProvider: public QThread
{
public:
    ThreadProvider() = default;
    int getMaxQueueLength() const;
    void setMaxQueueLength(int value);
    virtual ~ThreadProvider();

    //在队列结尾插入
    virtual void push(const FramePtrWrapper& d);
    virtual void push(FramePtrWrapper&& d);
    // 读取队列中最早的数据, 返回数据无需清理 交给FramePtrWrapper维护
    virtual FramePtrWrapper pop();
    // 读取队列中最早的数据, 返回数据无需清理
    virtual FramePtrWrapper top();
    //启动线程
    virtual void start();
    //退出线程，并等待线程退出（阻塞）
    virtual void stop();

protected:
    //存放交互数据 插入策略 先进先出
    std::list<FramePtrWrapper> data_queue;
    //互斥访问 datas;
    QMutex mutex;
    int curQueueSize = 0;
    int max_queue_len = 100;
    //处理线程退出
    bool is_exit = true;
};

#endif // THREADPROVIDER_H
