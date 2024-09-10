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

bool run = true;
// cvitek
static chnInputCfg pIc; //for venc
SAMPLE_VI_CONFIG_S stViConfig;

static void _initInputCfg(chnInputCfg *ipIc)
{
    ipIc->rcMode = -1;
    ipIc->iqp = -1;
    ipIc->pqp = -1;
    ipIc->gop = -1;
    ipIc->bitrate = -1;
    ipIc->firstFrmstartQp = -1;
    ipIc->num_frames = -1;
    ipIc->framerate = 30;
    ipIc->maxQp = -1;
    ipIc->minQp = -1;
    ipIc->maxIqp = -1;
    ipIc->minIqp = -1;
}

static CVI_S32 InitEnc(void)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    VENC_CHN VencChn[] = {0};
    PAYLOAD_TYPE_E enPayLoad = PT_H264;
    PIC_SIZE_E enSize = PIC_1080P; // TODO, according to settings
    VENC_GOP_MODE_E enGopMode = VENC_GOPMODE_NORMALP;
    VENC_GOP_ATTR_S stGopAttr;
    SAMPLE_RC_E enRcMode;
    CVI_U32 u32Profile = 0;

    _initInputCfg(&pIc);
    strcpy(pIc.codec, "264");
    pIc.rcMode = 0; // cbr
    pIc.iqp = 38;
    pIc.pqp = 38;
    pIc.gop = 60;
    pIc.bitrate = 1800; // if fps = 20
    pIc.firstFrmstartQp = 63;
    pIc.num_frames = -1;
    pIc.framerate = 25;
    pIc.maxQp = 42;
    pIc.minQp = 26;
    pIc.maxIqp = 42;
    pIc.minIqp = 26;

    enRcMode = (SAMPLE_RC_E) pIc.rcMode;

    s32Ret = SAMPLE_COMM_VENC_GetGopAttr(enGopMode, &stGopAttr);
    if (s32Ret != CVI_SUCCESS) {
        printf("[Err]Venc Get GopAttr for %#x!\n", s32Ret);
        return CVI_FAILURE;
    }

    s32Ret = SAMPLE_COMM_VENC_Start(
            &pIc,
            VencChn[0],
            enPayLoad,
            enSize,
            enRcMode,
            u32Profile,
            CVI_FALSE,
            &stGopAttr);
    if (s32Ret != CVI_SUCCESS) {
        printf("[Err]Venc Start failed for %#x!\n", s32Ret);
        return CVI_FAILURE;
    }

    return s32Ret;
}

static CVI_S32 InitVpss(VPSS_GRP VpssGrp, VPSS_GRP_ATTR_S *stVpssGrpAttr)
{
    VPSS_CHN VpssChn = VPSS_CHN0;
    CVI_BOOL abChnEnable[VPSS_MAX_PHY_CHN_NUM] = { 0 };
    VPSS_CHN_ATTR_S astVpssChnAttr[VPSS_MAX_PHY_CHN_NUM];
    CVI_S32 s32Ret = CVI_SUCCESS;

    abChnEnable[0] = CVI_TRUE;
    VpssChn        = VPSS_CHN0;
    astVpssChnAttr[VpssChn].u32Width                    = 1920;
    astVpssChnAttr[VpssChn].u32Height                   = 1080;
    astVpssChnAttr[VpssChn].enVideoFormat               = VIDEO_FORMAT_LINEAR;
    astVpssChnAttr[VpssChn].enPixelFormat               = PIXEL_FORMAT_YUV_PLANAR_420;
    astVpssChnAttr[VpssChn].stFrameRate.s32SrcFrameRate = 30;
    astVpssChnAttr[VpssChn].stFrameRate.s32DstFrameRate = 30;
    astVpssChnAttr[VpssChn].u32Depth                    = 1;
    astVpssChnAttr[VpssChn].bMirror                     = CVI_FALSE;
    astVpssChnAttr[VpssChn].bFlip                       = CVI_FALSE;
    astVpssChnAttr[VpssChn].stAspectRatio.enMode        = ASPECT_RATIO_AUTO;
    astVpssChnAttr[VpssChn].stAspectRatio.bEnableBgColor = CVI_TRUE;
    astVpssChnAttr[VpssChn].stNormalize.bEnable         = CVI_FALSE;

    /*start vpss*/
    s32Ret = SAMPLE_COMM_VPSS_Init(VpssGrp, abChnEnable, stVpssGrpAttr, astVpssChnAttr);
    if (s32Ret != CVI_SUCCESS) {
        SAMPLE_PRT("init vpss group failed. s32Ret: 0x%x !\n", s32Ret);
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp, abChnEnable, stVpssGrpAttr, astVpssChnAttr);
    if (s32Ret != CVI_SUCCESS) {
        SAMPLE_PRT("start vpss group failed. s32Ret: 0x%x !\n", s32Ret);
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_VI_Bind_VPSS(0, VpssGrp);
    if (s32Ret != CVI_SUCCESS) {
        SAMPLE_PRT("vi bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        return s32Ret;
    }

    /* check if bind sucessfully */
    {
        MMF_CHN_S stSrcChn, stDestChn;
        MMF_BIND_DEST_S stDests;
        stDestChn.enModId = CVI_ID_VPSS;
        stDestChn.s32DevId = 0;
        stDestChn.s32ChnId = 0;
        s32Ret = CVI_SYS_GetBindbyDest(&stDestChn, &stSrcChn);
        if (s32Ret != CVI_SUCCESS) {
            CVI_TRACE_LOG(CVI_DBG_ERR, "CVI_SYS_GetBindbyDest failed. s32Ret: 0x%x !\n", s32Ret);
            return s32Ret;
        }
        if (stSrcChn.enModId != CVI_ID_VI || stSrcChn.s32DevId != 0 || stSrcChn.s32ChnId != 0) {
            CVI_TRACE_LOG(CVI_DBG_ERR, "src chn info incorrect. !\n");
            return s32Ret;
        }

        stSrcChn.enModId = CVI_ID_VI;
        stSrcChn.s32DevId = 0;
        stSrcChn.s32ChnId = 0;
        s32Ret = CVI_SYS_GetBindbySrc(&stSrcChn, &stDests);
        if (s32Ret != CVI_SUCCESS) {
            CVI_TRACE_LOG(CVI_DBG_ERR, "CVI_SYS_GetBindbySrc failed. s32Ret: 0x%x !\n", s32Ret);
            return s32Ret;
        }
        if (stDests.u32Num != 1 || stDests.astMmfChn[0].enModId != CVI_ID_VPSS) {
            CVI_TRACE_LOG(CVI_DBG_ERR, "dst chn info incorrect. !\n");
            return s32Ret;
        }

        printf("sys TEST-PASS\n");
    }

    return s32Ret;
}

CVI_S32 InitVI()
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    SAMPLE_INI_CFG_S stIniCfg = {};
    memset(&stIniCfg, 0, sizeof(SAMPLE_INI_CFG_S));
    stIniCfg.enSource  = VI_PIPE_FRAME_SOURCE_DEV;
    stIniCfg.devNum    = 1;
    stIniCfg.enSnsType = SONY_IMX327_MIPI_2M_30FPS_12BIT;
    stIniCfg.enWDRMode = WDR_MODE_NONE;
    stIniCfg.s32BusId  = 3;
    stIniCfg.MipiDev   = 0xFF;
    stIniCfg.u8UseDualSns = 0;

    DYNAMIC_RANGE_E    enDynamicRange   = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E	   enPixFormat	    = PIXEL_FORMAT_YUV_PLANAR_420;
    VIDEO_FORMAT_E	   enVideoFormat    = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode   = COMPRESS_MODE_NONE;
    VI_VPSS_MODE_E	   enMastPipeMode   = VI_OFFLINE_VPSS_OFFLINE;
    CVI_S32 s32WorkSnsId = 0;

    VI_CHN ViChn = 0;
    SIZE_S stSize;
    PIC_SIZE_E enPicSize;

    // Get config from ini if found.
    if (0 > SAMPLE_COMM_VI_ParseIni(&stIniCfg)) {
        std::cout << "parse ini fail" << std::endl;
        return -1;
    }

    SAMPLE_COMM_VI_GetSensorInfo(&stViConfig);

    for (; s32WorkSnsId < stIniCfg.devNum; s32WorkSnsId++) {
        stViConfig.s32WorkingViNum				     = 1 + s32WorkSnsId;
        stViConfig.as32WorkingViId[s32WorkSnsId]		     = s32WorkSnsId;
        stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType	     =
            (s32WorkSnsId == 0) ? stIniCfg.enSnsType : stIniCfg.enSns2Type;
        stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.MipiDev	     =
            (s32WorkSnsId == 0) ? stIniCfg.MipiDev : stIniCfg.Sns2MipiDev;
        stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.s32BusId	     =
            (s32WorkSnsId == 0) ? stIniCfg.s32BusId : stIniCfg.s32Sns2BusId;
        stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.as16LaneId[0]   =
            (s32WorkSnsId == 0) ? stIniCfg.as16LaneId[0] : stIniCfg.as16Sns2LaneId[0];
        stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.as16LaneId[1]   =
            (s32WorkSnsId == 0) ? stIniCfg.as16LaneId[1] : stIniCfg.as16Sns2LaneId[1];
        stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.as16LaneId[2]   =
            (s32WorkSnsId == 0) ? stIniCfg.as16LaneId[2] : stIniCfg.as16Sns2LaneId[2];
        stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.as16LaneId[3]   =
            (s32WorkSnsId == 0) ? stIniCfg.as16LaneId[3] : stIniCfg.as16Sns2LaneId[3];
        stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.as16LaneId[4]   =
            (s32WorkSnsId == 0) ? stIniCfg.as16LaneId[4] : stIniCfg.as16Sns2LaneId[4];

        stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.as8PNSwap[0]   =
            (s32WorkSnsId == 0) ? stIniCfg.as8PNSwap[0] : stIniCfg.as8Sns2PNSwap[0];
        stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.as8PNSwap[1]   =
            (s32WorkSnsId == 0) ? stIniCfg.as8PNSwap[1] : stIniCfg.as8Sns2PNSwap[1];
        stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.as8PNSwap[2]   =
            (s32WorkSnsId == 0) ? stIniCfg.as8PNSwap[2] : stIniCfg.as8Sns2PNSwap[2];
        stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.as8PNSwap[3]   =
            (s32WorkSnsId == 0) ? stIniCfg.as8PNSwap[3] : stIniCfg.as8Sns2PNSwap[3];
        stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.as8PNSwap[4]   =
            (s32WorkSnsId == 0) ? stIniCfg.as8PNSwap[4] : stIniCfg.as8Sns2PNSwap[4];

        stViConfig.astViInfo[s32WorkSnsId].stDevInfo.ViDev	     = 0;
        stViConfig.astViInfo[s32WorkSnsId].stDevInfo.enWDRMode	     =
            (s32WorkSnsId == 0) ? stIniCfg.enWDRMode : stIniCfg.enSns2WDRMode;

        stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.enMastPipeMode = enMastPipeMode;
        stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[0]	     = s32WorkSnsId;
        stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[1]	     = -1;
        stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[2]	     = -1;
        stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[3]	     = -1;

        stViConfig.astViInfo[s32WorkSnsId].stChnInfo.ViChn	     = ViChn;
        stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enPixFormat     = enPixFormat;
        stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enDynamicRange  = enDynamicRange;
        stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enVideoFormat   = enVideoFormat;
        stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enCompressMode  = enCompressMode;
    }

    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stIniCfg.enSnsType, &enPicSize);
    if (s32Ret != CVI_SUCCESS) {
        std::cout << "SAMPLE_COMM_VI_GetSizeBySensor failed, ret=" << s32Ret << std::endl;;
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (s32Ret != CVI_SUCCESS) {
        std::cout << "SAMPLE_COMM_SYS_GetPicSize fail, ret=" << s32Ret << std::endl;
        return s32Ret;
    }

    /************************************************
     * step3:  Init modules
     ************************************************/
    s32Ret = SAMPLE_PLAT_SYS_INIT(stSize);
    if (s32Ret != CVI_SUCCESS) {
        std::cout << "sys init failed. s32Ret: " << s32Ret << std::endl;;
        return s32Ret;
    }

    return SAMPLE_PLAT_VI_INIT(&stViConfig);
}

int init()
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    //****************************************************************
    // Init VI, Vpss
    s32Ret = InitVI();
    if (s32Ret != CVI_SUCCESS) {
        printf("Init video input failed with %d\n", s32Ret);
        return s32Ret;
    }

    // start Vpss
    VPSS_GRP_ATTR_S stVpssGrpAttr;
    stVpssGrpAttr.stFrameRate.s32SrcFrameRate = -1;
    stVpssGrpAttr.stFrameRate.s32DstFrameRate = -1;
    stVpssGrpAttr.enPixelFormat = PIXEL_FORMAT_YUV_PLANAR_420;
    stVpssGrpAttr.u32MaxW = 1920;
    stVpssGrpAttr.u32MaxH = 1080;
    /// only for test here. u8VpssDev should be decided by VPSS_MODE and usage.
    stVpssGrpAttr.u8VpssDev = 0;

    s32Ret = InitVpss(0, &stVpssGrpAttr);
    if (s32Ret != CVI_SUCCESS) {
        SAMPLE_PRT("CVI_Init_Video_Process Grp0 failed with %d\n", s32Ret);
        return -1;
    }

    InitEnc();

    return 0;
}

void finalize()
{
    SAMPLE_COMM_VI_UnBind_VPSS(0, 0);
    CVI_BOOL abChnEnable[VPSS_MAX_PHY_CHN_NUM] = {0};
    for (int i = 0; i < 1; i++) {
        abChnEnable[i] = true;
    }
    SAMPLE_COMM_VPSS_Stop(0, abChnEnable);
    SAMPLE_COMM_VENC_Stop(0);
    SAMPLE_COMM_ISP_Stop(0);
    SAMPLE_COMM_VI_DestroyVi(&stViConfig);
    SAMPLE_COMM_SYS_Exit();
}

static void handleSig(int signo)
{
    run = false;
}

void ispInput(CVI_RTSP_CTX *ctx, CVI_RTSP_SESSION *session_1, CVI_RTSP_SESSION *session_2) {

    CVI_S32 s32Ret = CVI_SUCCESS;

    VIDEO_FRAME_INFO_S stVencFrame;
    s32Ret = CVI_VPSS_GetChnFrame(0, 0, &stVencFrame, 1000);
    if (s32Ret != CVI_SUCCESS) {
        SAMPLE_PRT("CVI_VPSS_GetChnFrame grp0 chn0 failed with %#x\n", s32Ret);
        return;
    }

    CVI_S32 s32SetFrameMilliSec = 20000;
    VENC_STREAM_S stStream;
    VENC_CHN_ATTR_S stVencChnAttr;
    VENC_CHN_STATUS_S stStat;
    s32Ret = CVI_VENC_SendFrame(0, &stVencFrame, s32SetFrameMilliSec);
    if (s32Ret != CVI_SUCCESS) {
        printf("CVI_VENC_SendFrame failed! %d\n", s32Ret);
        return;
    }

    VENC_CHN VencChn = 0;
    s32Ret = CVI_VENC_GetChnAttr(VencChn, &stVencChnAttr);
    if (s32Ret != CVI_SUCCESS) {
        printf("CVI_VENC_GetChnAttr, VencChn[%d], s32Ret = %d\n", VencChn, s32Ret);
        return;
    }

    s32Ret = CVI_VENC_QueryStatus(VencChn, &stStat);
    if (s32Ret != CVI_SUCCESS) {
        printf("CVI_VENC_QueryStatus failed with %#x!\n", s32Ret);
        return;
    }
    if (!stStat.u32CurPacks) {
        printf("NOTE: Current frame is NULL!\n");
        return;
    }

    stStream.pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
    if (stStream.pstPack == NULL) {
        printf("malloc memory failed!\n");
        return;
    }

    s32Ret = CVI_VENC_GetStream(VencChn, &stStream, -1);
    if (s32Ret != CVI_SUCCESS) {
        printf("CVI_VENC_GetStream failed with %#x!\n", s32Ret);
        free(stStream.pstPack);
        stStream.pstPack = NULL;
        return;
    }

    //
    VENC_PACK_S *ppack;
    CVI_RTSP_DATA data = {};
    CVI_RTSP_DATA data2 = {};
    memset(&data, 0, sizeof(CVI_RTSP_DATA));
    memset(&data2, 0, sizeof(CVI_RTSP_DATA));

    int len = 0;
    for (unsigned int i = 0; i < stStream.u32PackCount; i++) {
        ppack = &stStream.pstPack[i];
        len += ppack->u32Len - ppack->u32Offset;
    }

    {
        unsigned char *p = (unsigned char *)malloc(len);
        if(p) {
            unsigned char *pTmp = p;

            data2.blockCnt = stStream.u32PackCount;
            for (unsigned int i = 0; i < stStream.u32PackCount; i++) {
                ppack = &stStream.pstPack[i];
                memcpy(pTmp,ppack->pu8Addr + ppack->u32Offset, ppack->u32Len - ppack->u32Offset);
                pTmp += ppack->u32Len - ppack->u32Offset;

                data2.dataPtr[i] = ppack->pu8Addr;
                data2.dataLen[i] = ppack->u32Len;
            }

            data.dataPtr[0] = p;
            data.dataLen[0] = len;
            data.blockCnt = 1;
            CVI_RTSP_WriteFrame(ctx, session_1->video, &data);
            CVI_RTSP_WriteFrame(ctx, session_2->video, &data2);

            free(p);
        } else {
            printf("[%s,%d] malloc failed,drop current packet count(%d).\n",__FUNCTION__,__LINE__,stStream.u32PackCount);
        }
    }

    s32Ret = CVI_VENC_ReleaseStream(VencChn, &stStream);
    if (s32Ret != CVI_SUCCESS) {
        printf("CVI_VENC_ReleaseStream, s32Ret = %d\n", s32Ret);
        free(stStream.pstPack);
        stStream.pstPack = NULL;
        return;
    }

    free(stStream.pstPack);
    stStream.pstPack = NULL;

    if (CVI_VPSS_ReleaseChnFrame(0, 0, &stVencFrame) != 0) {
        SAMPLE_PRT("CVI_VPSS_ReleaseChnFrame chn1 NG\n");
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

    if (0 > init()) {
        printf("init fail");
        return -1;
    }

    CVI_RTSP_CTX *ctx = NULL;
    CVI_RTSP_CONFIG config = {0};
    config.port = 554;

    if (0 > CVI_RTSP_Create(&ctx, &config)) {
        std::cout << "fail to create rtsp contex" << std::endl;
        return -1;
    }
    // set listener
    CVI_RTSP_STATE_LISTENER listener = {0};
    listener.onConnect = connect;
    listener.argConn = ctx;
    listener.onDisconnect = disconnect;

    CVI_RTSP_SetListener(ctx, &listener);

    if (0 > CVI_RTSP_Start(ctx)) {
        std::cout << "fail to start" << std::endl;
        return -1;
    }

    // add media stream
    CVI_RTSP_SESSION *session_1 = NULL;
    CVI_RTSP_SESSION_ATTR attr = {0};
    attr.video.codec = RTSP_VIDEO_H264;
    snprintf(attr.name, sizeof(attr.name), "%s", "rtsp");

    CVI_RTSP_CreateSession(ctx, &attr, &session_1);

    // add media stream2
    CVI_RTSP_SESSION *session_2;
    snprintf(attr.name, sizeof(attr.name), "%s", "rtsp2");
    CVI_RTSP_CreateSession(ctx, &attr, &session_2);
    //

    while (run) {
        ispInput(ctx, session_1, session_2);
    }

    CVI_RTSP_Stop(ctx);

    CVI_RTSP_DestroySession(ctx, session_1);
    CVI_RTSP_DestroySession(ctx, session_2);

    CVI_RTSP_Destroy(&ctx);

    finalize();

    return 0;
}
