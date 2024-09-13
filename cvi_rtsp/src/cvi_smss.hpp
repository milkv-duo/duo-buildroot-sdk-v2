#pragma once

#include <atomic>
#include <OnDemandServerMediaSubsession.hh>
#include "cvi_source.hpp"

typedef void (SeekCallback) (double seekNPT, void *arg);
typedef void (PauseCallback) (void *arg);
typedef void (PlayCallback) (int references, void *arg);
typedef void (TeardownCallback) (int references, void *arg);

class CVI_ServerMediaSession: public ServerMediaSession {
    public:
        static CVI_ServerMediaSession* createNew(UsageEnvironment& env,
                char const* streamName = NULL,
                char const* info = NULL,
                char const* description = NULL,
                Boolean isSSM = False,
                char const* miscSDPLines = NULL,
                int maxConn = 0);

        int maxConn() const { return fMaxConn; }
        void setMaxConn(int num) { fMaxConn= num; }
    protected:
        CVI_ServerMediaSession(UsageEnvironment& env, char const* streamName,
		     char const* info, char const* description,
		     Boolean isSSM, char const* miscSDPLines, int maxConn);

        virtual ~CVI_ServerMediaSession();

    private:
        int fMaxConn;
};

CVI_ServerMediaSession* CVI_ServerMediaSession::createNew(UsageEnvironment& env,
	    char const* streamName, char const* info,
	    char const* description, Boolean isSSM, char const* miscSDPLines, int maxConn) {
    return new CVI_ServerMediaSession(env, streamName, info, description, isSSM, miscSDPLines, maxConn);
}

CVI_ServerMediaSession::CVI_ServerMediaSession(UsageEnvironment& env,
	    char const* streamName, char const* info,
	    char const* description, Boolean isSSM, char const* miscSDPLines, int maxConn):
        ServerMediaSession(env, streamName, info, description, isSSM, miscSDPLines),
        fMaxConn(maxConn) {

}

CVI_ServerMediaSession::~CVI_ServerMediaSession() {

}


class CVI_ServerMediaSubsession: public OnDemandServerMediaSubsession {
    public:
        static unsigned packetLen;
        virtual ~CVI_ServerMediaSubsession();
        CVI_Source *getMediaSource();
        virtual void setPlayCallback(PlayCallback *play, void *arg);
        virtual void setSeekCallback(SeekCallback *seek, void *arg);
        virtual void setPauseCallback(PauseCallback *pause, void *arg);
        virtual void setTeardownCallback(TeardownCallback *teardown, void *arg);
        virtual int writeData(uint8_t *data, uint32_t size);
    protected:
        CVI_ServerMediaSubsession(UsageEnvironment& env, float duration, bool reuseFirstSource=false);
        virtual FramedSource* createNewStreamSource(unsigned clientSessionId, unsigned& estBitrate) = 0;
        virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource* inputSource) = 0;
        virtual void startStream(unsigned clientSessionId, void* streamToken,
                TaskFunc* rtcpRRHandler,
                void* rtcpRRHandlerClientData,
                unsigned short& rtpSeqNum,
                unsigned& rtpTimestamp,
                ServerRequestAlternativeByteHandler* serverRequestAlternativeByteHandler,
                void* serverRequestAlternativeByteHandlerClientData);
        virtual void seekStreamSource(FramedSource* inputSource, double& seekNPT, double streamDuration, u_int64_t& numBytes);
        virtual void pauseStream(unsigned clientSessionId, void* streamToken);
        virtual void deleteStream(unsigned clientSessionId, void*& streamToken);
        virtual float duration() const;
    protected:
        CVI_Source *_mediaSource;
        PlayCallback *_play;
        void *_playArg;
        SeekCallback *_seek;
        void *_seekArg;
        PauseCallback *_pause;
        void *_pauseArg;
        TeardownCallback *_teardown;
        void *_teardownArg;
        float _duration;
    private:
        std::mutex _mutex;
        bool _reuseFirstSource;
        std::atomic<bool> _start;
};

unsigned CVI_ServerMediaSubsession::packetLen = 0;

CVI_ServerMediaSubsession::CVI_ServerMediaSubsession(UsageEnvironment& env, float duration, bool reuseFirstSource):
    OnDemandServerMediaSubsession(env, reuseFirstSource),
    _duration(duration)
{
    _start = false;
    _mediaSource = NULL;
    // HACK
    // by live555 spec, seek & pause only support in single client mode
    _reuseFirstSource = false;

    OutPacketBuffer::maxSize = 600000;
}

CVI_ServerMediaSubsession::~CVI_ServerMediaSubsession()
{
}

CVI_Source *CVI_ServerMediaSubsession::getMediaSource()
{
    return _mediaSource;
}

void CVI_ServerMediaSubsession::seekStreamSource(FramedSource* inputSource, double& seekNPT, double streamDuration, u_int64_t& numBytes)
{
    if (_seek != NULL) {
        _seek(seekNPT, _seekArg);
    }
}

float CVI_ServerMediaSubsession::duration() const
{
    return _duration;
}

void CVI_ServerMediaSubsession::setPlayCallback(PlayCallback *play, void *arg)
{
    _play = play;
    _playArg = arg;
}

void CVI_ServerMediaSubsession::setSeekCallback(SeekCallback *seek, void *arg)
{
    _seek = seek;
    _seekArg = arg;
}

void CVI_ServerMediaSubsession::setPauseCallback(PauseCallback *pause, void *arg)
{
    _pause = pause;
    _pauseArg = arg;
}

void CVI_ServerMediaSubsession::setTeardownCallback(TeardownCallback *teardown, void *arg)
{
    _teardown = teardown;
    _teardownArg = arg;
}

void CVI_ServerMediaSubsession::pauseStream(unsigned clientSessionId, void* streamToken)
{
    if (_reuseFirstSource) {
        return;
    }

    OnDemandServerMediaSubsession::pauseStream(clientSessionId, streamToken);

    if (_pause) {
        _pause(_pauseArg);
    }
}

void CVI_ServerMediaSubsession::startStream(unsigned clientSessionId, void* streamToken,
                TaskFunc* rtcpRRHandler,
                void* rtcpRRHandlerClientData,
                unsigned short& rtpSeqNum,
                unsigned& rtpTimestamp,
                ServerRequestAlternativeByteHandler* serverRequestAlternativeByteHandler,
                void* serverRequestAlternativeByteHandlerClientData)
{
    OnDemandServerMediaSubsession::startStream(
        clientSessionId, streamToken, rtcpRRHandler,
        rtcpRRHandlerClientData, rtpSeqNum, rtpTimestamp, serverRequestAlternativeByteHandler, serverRequestAlternativeByteHandlerClientData);

    StreamState* streamState = (StreamState*)streamToken;
    int referenceCount = streamState->referenceCount();

    if (_play) {
        _play(referenceCount - 1, _playArg);
    }

    _start = true;
}

int CVI_ServerMediaSubsession::writeData(uint8_t *data, uint32_t size)
{
    if (!_start) {
        return 1;
    }

    if (_mediaSource == NULL
        || _mediaSource->isCloseed()) {
        return 1;
    }

    std::lock_guard<std::mutex> lock(_mutex);
    _mediaSource->setBuf(data, size);
    while (!_mediaSource->isCloseed() && _mediaSource->getBufLeft()) {
        SignalNewFrameData(&(envir().taskScheduler()), _mediaSource);
        _mediaSource->bufferRelease();
    }

    int referenceCount = fParentSession->referenceCount();
    if (_mediaSource->isCloseed() && referenceCount == 0) {
        // FIXME: deleting would raise an seg fault
        // delete _mediaSource;

        _mediaSource = nullptr;
    }

    return 0;
}

void CVI_ServerMediaSubsession::deleteStream(unsigned clientSessionId, void*& streamToken)
{
    int referenceCount = fParentSession->referenceCount();

    if (_mediaSource && referenceCount == 1) {
        // stopping accepting new writing task first
        _start = false;
        // closeStream to make writeData to exit while loop
        _mediaSource->closeStream();
    }

    if (_teardown) {
        _teardown(referenceCount - 1, _teardownArg);
    }

    OnDemandServerMediaSubsession::deleteStream(clientSessionId, streamToken);
}
