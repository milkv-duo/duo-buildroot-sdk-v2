#pragma once

#include <OnDemandServerMediaSubsession.hh>
#include "cvi_smss.hpp"

class CVI_AudioSubsession: public CVI_ServerMediaSubsession {
    public:
        static CVI_AudioSubsession *createNew(UsageEnvironment& env, const CVI_RTSP_SESSION_ATTR &attr);
        virtual ~CVI_AudioSubsession();
    protected:
        CVI_AudioSubsession(UsageEnvironment& env, const RTSP_AUDIO_ATTR &attr, float duration=0, bool reuseFirstSource=false);
        virtual FramedSource* createNewStreamSource(unsigned clientSessionId, unsigned& estBitrate);
        virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource* inputSource);
    protected:
        unsigned _packetLen;
        CVI_RTSP_AUDIO_CODEC _codec;
        unsigned int _sampleRate;
        unsigned int _bitrate;
};

CVI_AudioSubsession *CVI_AudioSubsession::createNew(UsageEnvironment& env, const CVI_RTSP_SESSION_ATTR &attr)
{
    return new CVI_AudioSubsession(env, attr.audio, attr.duration, attr.maxConnNum > 0? true: attr.reuseFirstSource);
}

CVI_AudioSubsession::CVI_AudioSubsession(UsageEnvironment& env, const RTSP_AUDIO_ATTR &attr, float duration, bool reuseFirstSource):
    CVI_ServerMediaSubsession(env, duration, reuseFirstSource)
{
    _codec = attr.codec;
    _sampleRate = attr.sampleRate;
    // default 192 kbps
    _bitrate = attr.bitrate == 0? 192: attr.bitrate;

    _play = attr.play;
    _playArg = attr.playArg;
    _seek = attr.seek;
    _seekArg = attr.seekArg;
    _pause = attr.pause;
    _pauseArg = attr.pauseArg;
    _teardown = attr.teardown;
    _teardownArg = attr.teardownArg;
}

CVI_AudioSubsession::~CVI_AudioSubsession()
{

}

FramedSource *CVI_AudioSubsession::createNewStreamSource(unsigned clientSessionId, unsigned& estBitrate)
{
    _mediaSource = CVI_Source::createNew(envir());
    estBitrate = _bitrate;

    switch (_codec) {
    case RTSP_AUDIO_PCM_L16:
        return EndianSwap16::createNew(envir(), _mediaSource);
    case RTSP_AUDIO_PCM_L24:
        return EndianSwap24::createNew(envir(), _mediaSource);
    case RTSP_AUDIO_AAC:
        return AC3AudioStreamFramer::createNew(envir(), _mediaSource);
    default:
        return NULL;
    }
}

RTPSink *CVI_AudioSubsession::createNewRTPSink(Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource* inputSource)
{
    MultiFramedRTPSink *sink = NULL;
    switch (_codec) {
    case RTSP_AUDIO_PCM_L16:
        sink = SimpleRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic, _sampleRate, "audio", "L16", 1);
        break;
    case RTSP_AUDIO_PCM_L24:
        sink = SimpleRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic, _sampleRate, "audio", "L24", 1);
        break;
    case RTSP_AUDIO_AAC:
        sink = AC3AudioRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic, _sampleRate);
        break;
    default:
        return NULL;
    }

    if (CVI_ServerMediaSubsession::packetLen != 0) {
        sink->setPacketSizes(CVI_ServerMediaSubsession::packetLen, CVI_ServerMediaSubsession::packetLen);
    }

    return sink;
}
