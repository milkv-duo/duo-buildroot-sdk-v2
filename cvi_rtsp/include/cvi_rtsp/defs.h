#pragma once

#define CVI_RTSP_DATA_MAX_BLOCK 8

typedef void * CVI_RTSP_TRACK;

typedef enum {
    RTSP_VIDEO_NONE = 0,
    RTSP_VIDEO_H264, // 1
    RTSP_VIDEO_H265, // 2
    RTSP_VIDEO_JPEG, // 3
} CVI_RTSP_VIDEO_CODEC;

typedef enum {
    RTSP_AUDIO_NONE = 0,
    RTSP_AUDIO_PCM_L16, // 1
    RTSP_AUDIO_PCM_L24, // 2
    RTSP_AUDIO_AAC      // 3
} CVI_RTSP_AUDIO_CODEC;

typedef struct {
    unsigned packetLen;
    int port;
    int timeout;
    int maxConnNum;
    unsigned tcpBufSize; // for SO_SNDBUF, only affects tcp client
} CVI_RTSP_CONFIG;

typedef struct {
    void *server;
    void *env;
    void *scheduler;
    pthread_t worker;
    char volatile stop;
    CVI_RTSP_CONFIG config;
} CVI_RTSP_CTX;

typedef struct {
    unsigned int bitrate;
    unsigned int sampleRate;
    CVI_RTSP_AUDIO_CODEC codec;

    // play callback
    void (*play) (int references, void *arg);
    void *playArg;

    // seek callback
    void (*seek) (double npt, void *arg);
    void *seekArg;

    // pause callback
    void (*pause) (void *arg);
    void *pauseArg;

    // taeadown callback
    void (*teardown) (int references, void *arg);
    void *teardownArg;
} RTSP_AUDIO_ATTR;

typedef struct {
    // bitrate setting is use to create rtp buffer
    // The code below here shows how live555 create rtp buffer
    /*
    // Try to use a big send buffer for RTP -  at least 0.1 second of
    // specified bandwidth and at least 50 KB
    unsigned rtpBufSize = streamBitrate * 25 / 2; // 1 kbps * 0.1 s = 12.5 bytes
    if (rtpBufSize < 50 * 1024) rtpBufSize = 50 * 1024;
    */
    unsigned int bitrate; // only affects udp client
    CVI_RTSP_VIDEO_CODEC codec;

    // play callback
    void (*play) (int references, void *arg);
    void *playArg;

    // seek callback
    void (*seek) (double npt, void *arg);
    void *seekArg;

    // pause callback
    void (*pause) (void *arg);
    void *pauseArg;

    // taeadown callback
    void (*teardown) (int references, void *arg);
    void *teardownArg;
} RTSP_VIDEO_ATTR;

typedef struct {
    char name[128];
    float duration;
    int reuseFirstSource;
    int maxConnNum;
    RTSP_VIDEO_ATTR video;
    RTSP_AUDIO_ATTR audio;
} CVI_RTSP_SESSION_ATTR;

typedef struct {
    CVI_RTSP_TRACK video;
    CVI_RTSP_TRACK audio;
    char name[128];
} CVI_RTSP_SESSION;

typedef struct {
    uint8_t *dataPtr[CVI_RTSP_DATA_MAX_BLOCK];
    uint32_t dataLen[CVI_RTSP_DATA_MAX_BLOCK];
    uint64_t timestamp;
    uint32_t blockCnt;
} CVI_RTSP_DATA;

typedef struct {
    void (*onConnect) (const char *ip, void *arg);
    void *argConn;
    void (*onDisconnect) (const char *ip, void *arg);
    void *argDisconn;
} CVI_RTSP_STATE_LISTENER;
