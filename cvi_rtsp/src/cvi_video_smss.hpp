#pragma once

#include <OnDemandServerMediaSubsession.hh>
#include "cvi_smss.hpp"
#include "cvi_jpeg_source.hpp"

class CVI_VideoSubsession: public CVI_ServerMediaSubsession {
    public:
        static CVI_VideoSubsession *createNew(UsageEnvironment& env, const CVI_RTSP_SESSION_ATTR &attr);
        virtual ~CVI_VideoSubsession();
    protected:
        CVI_VideoSubsession(UsageEnvironment& env, const RTSP_VIDEO_ATTR &attr, float duration=0, bool reuseFirstSource=false);
        virtual FramedSource* createNewStreamSource(unsigned clientSessionId, unsigned& estBitrate);
        virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource* inputSource);
    protected:
        CVI_RTSP_VIDEO_CODEC _codec;
        unsigned int _bitrate;
};

CVI_VideoSubsession *CVI_VideoSubsession::createNew(UsageEnvironment& env, const CVI_RTSP_SESSION_ATTR &attr)
{
    return new CVI_VideoSubsession(env, attr.video, attr.duration, attr.maxConnNum > 0? true: attr.reuseFirstSource);
}

CVI_VideoSubsession::CVI_VideoSubsession(UsageEnvironment& env, const RTSP_VIDEO_ATTR &attr, float duration, bool reuseFirstSource):
    CVI_ServerMediaSubsession(env, duration, reuseFirstSource)
{
    _codec = attr.codec;
    // default 512 kbps
    _bitrate = attr.bitrate == 0? 512: attr.bitrate;

    _play = attr.play;
    _playArg = attr.playArg;
    _seek = attr.seek;
    _seekArg = attr.seekArg;
    _pause = attr.pause;
    _pauseArg = attr.pauseArg;
    _teardown = attr.teardown;
    _teardownArg = attr.teardownArg;
}

CVI_VideoSubsession::~CVI_VideoSubsession()
{
}

FramedSource *CVI_VideoSubsession::createNewStreamSource(unsigned clientSessionId, unsigned& estBitrate)
{
    estBitrate = _bitrate;
    _mediaSource = CVI_Source::createNew(envir());

    switch (_codec) {
    case RTSP_VIDEO_H264:
        return H264VideoStreamDiscreteFramer::createNew(envir(), _mediaSource);
    case RTSP_VIDEO_H265:
        return H265VideoStreamDiscreteFramer::createNew(envir(), _mediaSource);
    case RTSP_VIDEO_JPEG:
        return CVI_JPEGVideoSource::createNew(envir(), _mediaSource);
    default:
        return nullptr;
    }
}

RTPSink *CVI_VideoSubsession::createNewRTPSink(Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource* inputSource)
{
    MultiFramedRTPSink *sink = nullptr;

    switch (_codec) {
    case RTSP_VIDEO_H264:
        return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
    case RTSP_VIDEO_H265:
        return H265VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
    case RTSP_VIDEO_JPEG:
        return JPEGVideoRTPSink::createNew(envir(), rtpGroupsock);
    default:
        return nullptr;
    }

    if (CVI_ServerMediaSubsession::packetLen != 0) {
        sink->setPacketSizes(CVI_ServerMediaSubsession::packetLen, CVI_ServerMediaSubsession::packetLen);
    }

    return sink;
}
