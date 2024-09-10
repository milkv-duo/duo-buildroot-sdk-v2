#pragma once

#include <semaphore.h>
#include <GroupsockHelper.hh>
#include <FramedSource.hh>

class CVI_Source: public FramedSource {
    public:
        static CVI_Source* createNew(UsageEnvironment& env);
        virtual ~CVI_Source();
        void setBuf(u_int8_t *buf, size_t size);
        void bufferRelease();
        size_t getBufLeft();
        void closeStream();
        bool isCloseed() const;
        EventTriggerId eventTriggerId() const;

    protected:
        CVI_Source(UsageEnvironment& env);
    private:
        virtual void doGetNextFrame();

    private:
        static void deliverFrame0(void* clientData);
        void deliverFrame();

    private:
        sem_t _sem;
        u_int8_t *_buf;
        size_t _bufSize;
        bool _closed;
        EventTriggerId _eventTriggerId;
};


CVI_Source* CVI_Source::createNew(UsageEnvironment& env)
{
    return new CVI_Source(env);
}

CVI_Source::CVI_Source(UsageEnvironment& env) : FramedSource(env)
{

    sem_init(&_sem, 0, 0);
    _eventTriggerId = envir().taskScheduler().createEventTrigger(deliverFrame0);
    _buf = NULL;
    _bufSize = 0;
    _closed = false;
}

CVI_Source::~CVI_Source()
{
    envir().taskScheduler().deleteEventTrigger(_eventTriggerId);
}

void CVI_Source::setBuf(u_int8_t *buf, size_t size)
{
    if (size > 4 && buf[0] == 0 && buf[1] == 0 && buf[2] == 0 && buf[3] == 1) {
        _bufSize = size - 4;
        _buf = buf + 4;
    } else {
        _bufSize = size;
        _buf = buf;
    }
}

size_t CVI_Source::getBufLeft()
{
    return _bufSize;
}

void CVI_Source::doGetNextFrame()
{
}

void CVI_Source::deliverFrame0(void* clientData)
{
    ((CVI_Source *)clientData)->deliverFrame();
}

void CVI_Source::deliverFrame()
{
    if (!isCurrentlyAwaitingData() || _buf == NULL || _bufSize == 0) {
        sem_post(&_sem);
        return;
    }

    u_int8_t *newFrameDataStart = _buf;
    unsigned newFrameSize = _bufSize;

    if (newFrameSize > fMaxSize) {
        fFrameSize = fMaxSize;
        fNumTruncatedBytes = newFrameSize - fMaxSize;
    } else {
        fFrameSize = newFrameSize;
    }
    gettimeofday(&fPresentationTime, NULL);

    memmove(fTo, newFrameDataStart, fFrameSize);

    if (fNumTruncatedBytes) {
        _buf = _buf + fFrameSize;
        _bufSize -= fFrameSize;
        fNumTruncatedBytes = 0;
    } else {
        _bufSize = 0;
    }

    sem_post(&_sem);
    FramedSource::afterGetting(this);
}

void CVI_Source::bufferRelease()
{
    if (_closed) {
        return;
    }

    struct timespec ts;

    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 1;
    sem_timedwait(&_sem, &ts);
}

void CVI_Source::closeStream()
{
    _closed = true;
    sem_post(&_sem);
}

bool CVI_Source::isCloseed() const
{
    return _closed;
}

EventTriggerId CVI_Source::eventTriggerId() const
{
    return _eventTriggerId;
}

void SignalNewFrameData(TaskScheduler *task, CVI_Source *src)
{
    if (task != NULL && src != NULL) {
        task->triggerEvent(src->eventTriggerId(), src);
    }
}
