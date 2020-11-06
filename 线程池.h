#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include <queue>

#include "lock.h"

template <typename T>
class Threadpool
{
private:
    static void* worker(void* arg);
    void run();

private:
    //线程数量
    int thread_number;
    //最大任务数量
    int max_task;
    //描述线程的数组
    pthread_t* m_thread;
    //请求队列
    std::queue<T*>work_queue;
    //保护请求队列的互斥锁
    locker queue_mutex;
    //是否有新任务需要处理
    sem queuestat_;
    //是否结束线程
    bool stop_;

public:
    //定义包括线程池内线程的数量和请求队列中最多允许的请求数量
    threadpool(int thread_number = 8, int max_requests = 10000);
    ~threadpool();
    
    void Append(T* work) {
        work_queue.push(work);
        queuestat_.Post();
    }
};

#endif
