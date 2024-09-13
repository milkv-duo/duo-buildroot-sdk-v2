#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <cvi_buffer.h>
#include <cvi_ae_comm.h>
#include <cvi_awb_comm.h>
#include <cvi_comm_isp.h>
#include <cvi_sys.h>
#include <cvi_vb.h>
#include <cvi_vi.h>
#include <cvi_isp.h>
#include <cvi_ae.h>
#include <cvi_venc.h>
#include <cvi_rtsp/rtsp.h>
#include <sample_comm.h>

#include <cvi_comm_vpss.h>

bool gRun = true;

int InitAudio(unsigned int sampleRate)
{
    CVI_S32 s32Ret;
    if (CVI_SUCCESS != CVI_AUDIO_INIT()) {
        std::cout << "audio init fail" << std::endl;
        return -1;
    }
    AIO_ATTR_S  AudinAttr;

    AudinAttr.enSamplerate = (AUDIO_SAMPLE_RATE_E)sampleRate;
    AudinAttr.u32ChnCnt = 1;
    AudinAttr.enSoundmode = AUDIO_SOUND_MODE_MONO;
    AudinAttr.enBitwidth = AUDIO_BIT_WIDTH_16;
    AudinAttr.enWorkmode     = AIO_MODE_I2S_MASTER;
    AudinAttr.u32EXFlag      = 0;
    AudinAttr.u32FrmNum      = 10; /* only use in bind mode */
    AudinAttr.u32PtNumPerFrm = 640; //sample rate / fps
    AudinAttr.u32ClkSel      = 0;
    AudinAttr.enI2sType = AIO_I2STYPE_INNERCODEC;

    s32Ret = CVI_AI_SetPubAttr(0, &AudinAttr);
    s32Ret = CVI_AI_Enable(0);
    s32Ret = CVI_AI_EnableChn(0, 0);

    return s32Ret;
}

int init()
{
    return 0;
}

static void handleSig(int signo)
{
    gRun = false;
}


void audioInput(CVI_RTSP_CTX *ctx, CVI_RTSP_SESSION *session)
{
    CVI_S32 s32Ret;

    while (gRun) {
        AUDIO_FRAME_S stFrame;
        AEC_FRAME_S stAecFrm;

        memset(&stFrame, 0, sizeof(AUDIO_FRAME_S));

        s32Ret = CVI_AI_GetFrame(0, 0, &stFrame, &stAecFrm, CVI_FALSE);
        if (s32Ret != CVI_SUCCESS) {
            printf("CVI_AI_GetFrame --> none!!\n");
            continue;
        }

        CVI_RTSP_DATA data = {0};
        data.dataPtr[0] = stFrame.u64VirAddr[0];
        data.dataLen[0] = stFrame.u32Len * 2;
        data.blockCnt = 1;

        CVI_RTSP_WriteFrame(ctx, session->audio, &data);
    }
}

void connect(const char *ip, void *arg)
{
    std::cout << "connect: " << ip << std::endl;
}

void disconnect(const char *ip, void *arg)
{
    std::cout << "disconnect: " << ip << std::endl;
}

int main(int argc, const char *argv[])
{
    signal(SIGPIPE,SIG_IGN);
    signal(SIGINT, handleSig);
    signal(SIGTERM, handleSig);

    const unsigned int sampleRate = 16000;

    if (CVI_SUCCESS != InitAudio(sampleRate)) {
        std::cout << "init audio fail" << std::endl;
        return -1;
    }

    CVI_RTSP_CTX *ctx = NULL;
    CVI_RTSP_CONFIG config = {0};
    config.port = 554;

    if (0 > CVI_RTSP_Create(&ctx, &config)) {
        std::cout << "fail to create rtsp contex" << std::endl;
        return -1;
    }

    CVI_RTSP_SESSION *session = NULL;
    CVI_RTSP_SESSION_ATTR attr = {0};
    attr.audio.codec = RTSP_AUDIO_PCM_L16;
    attr.audio.sampleRate = sampleRate;

    snprintf(attr.name, sizeof(attr.name), "%s", "audio");

    CVI_RTSP_CreateSession(ctx, &attr, &session);

    // set listener
    CVI_RTSP_STATE_LISTENER listener = {0};
    listener.onConnect = connect;
    listener.onDisconnect = disconnect;

    CVI_RTSP_SetListener(ctx, &listener);

    if (0 > CVI_RTSP_Start(ctx)) {
        std::cout << "fail to start" << std::endl;
        return -1;
    }

    audioInput(ctx, session);

    CVI_RTSP_Stop(ctx);

    CVI_RTSP_DestroySession(ctx, session);

    CVI_RTSP_Destroy(&ctx);

    return 0;
}
