#ifndef LOCKED_QUEUE_H
#define LOCKED_QUEUE_H

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/xtime.hpp>
#include <iostream>
#include <queue>
#include "Logger.h"

// This class is a queue for communication between two threads. It
// provides thread safe push and pop. Popping an empty queue results
// in a wait until the queue is filled.
//
// Parameters:
// maxSize: If the queue, after push, exceeds this size the process sleeps for
// 2^(size-maxSize) mMaxSizeSleepSecs seconds. 

template<class T>
class LockedQueue{

public:
    LockedQueue()
        : mMaxSize(2), mMaxSizeSleepSecs(0)
        {}
    LockedQueue(size_t maxSize, size_t maxSizeSleepSecs)
        : mMaxSize(maxSize), mMaxSizeSleepSecs(maxSizeSleepSecs)
        {}
    void push(T* t);
    T* pop();
    size_t size() const;

private:
    const size_t            mMaxSize;
    const size_t            mMaxSizeSleepSecs;
    std::queue<T*>          mQueue;
    boost::mutex            mMutex;
    boost::condition        mCondition;
};


template<class T>
size_t 
LockedQueue<T>::size() const
{
    return mQueue.size();
}

template<class T>
void 
LockedQueue<T>::push(T* t)
{
    {
        boost::mutex::scoped_lock lock(mMutex);
        mQueue.push(t);
        mCondition.notify_all();
    }
    if (mQueue.size() >= mMaxSize){
        unsigned int sleepSecs = (unsigned int)
            round((exp2(mQueue.size() - mMaxSize) * mMaxSizeSleepSecs));
        LOG(LL_INFO, "Queue size " + toString(mQueue.size()) + 
            " (max " + toString(mMaxSize) +
            + ") sleeping for " + toString(sleepSecs) + " seconds");
        
        boost::xtime xt;
        boost::xtime_get(&xt, boost::TIME_UTC);
        xt.sec += sleepSecs;
        boost::thread::sleep(xt);
    }
}

template<class T>
T*
LockedQueue<T>::pop()
{
    boost::mutex::scoped_lock lock(mMutex);
    while (mQueue.empty())
        mCondition.wait(lock);
    T* t = mQueue.front();
    mQueue.pop();
    return t;
}

#endif //LOCKED_QUEUE_H
