#include "Utils.hpp"
#include "Logger.h"
#include "Thread.h"

using namespace std;

Thread::Thread(const string& name)
    : mName(name),
      mStartRun(false),
      mStopRun(false),
      mBoostThread(ThreadRunner(this))
{
    LOG(LL_INFO, "Finished initializing thread: " + mName);
}
    
void
Thread::startThreadRun()
{
    LOG(LL_INFO, "Starting thread: " + mName);
    mStartRun = true;
}

void
Thread::stopThreadRun()
{
    LOG(LL_INFO, "Stopping thread: " + mName);
    mStopRun = true;
}

void
Thread::joinThread()
{
    mBoostThread.join();
}

void
Thread::runThread()
{
    LOG(LL_INFO, "Running thread: " + mName);
    while (!mStartRun){
        LOG(LL_INFO, "Waiting for start signal: " + mName);
        sleep(1); 
    }
    threadMain();
    LOG(LL_INFO, "Exiting thread: " + mName);
}

bool
Thread::keepThreadRunning() const
{
    return !mStopRun;
}



