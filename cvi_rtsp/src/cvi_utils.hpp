#pragma once

#include <BasicUsageEnvironment.hh>
#include <iostream>

class CVI_TaskScheduler: public BasicTaskScheduler {
    public:
        static CVI_TaskScheduler* createNew(unsigned maxSchedulerGranularity = 10000/*microseconds*/);
        virtual ~CVI_TaskScheduler() {};
        virtual void internalError() override;
    protected:
        CVI_TaskScheduler(unsigned maxSchedulerGranularity);
};

CVI_TaskScheduler* CVI_TaskScheduler::createNew(unsigned maxSchedulerGranularity)
{
    return new CVI_TaskScheduler(maxSchedulerGranularity);
}

CVI_TaskScheduler::CVI_TaskScheduler(unsigned maxSchedulerGranularity):
    BasicTaskScheduler(maxSchedulerGranularity)
{

}

void CVI_TaskScheduler::internalError()
{
    std::cout << "TaskScheduler internal error" << std::endl;
}
