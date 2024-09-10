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
#include <cvi_tracer.h>
#include <cvi_rtsp/rtsp.h>
#include <sample_comm.h>

#include <cvi_comm_vpss.h>

#define NUM_VB_CNT 18
#define VI_FSP 25
#define NUM_OF_STREAMS 6
#define NUM_OF_CHN_PER_VPSS 3
#define NUM_OF_VPSS_GRP NUM_OF_STREAMS / NUM_OF_CHN_PER_VPSS

// #define TEST_FILE_MODE TRUE
// #define VPSS_VENC_BINDING_MODE TRUE

struct CodecInfo_S {
    std::string str;
    CVI_U32 width;
    CVI_U32 height;
    CVI_S32 framerate;
    CVI_S32 bitrate; // kbits
};

static const struct CodecInfo_S gCodecs[] = {
    {"265",    3840,  2160,   25,   5120},
    {"265",    1920,  1080,   25,   2048},
    {"265",    384,   288,    25,   256},
 
    {"265",    384,   288,    25,   256},
    {"265",    384,   288,    25,   256},
    {"265",    384,   288,    25,   256},
};

bool gRun = true;
CVI_RTSP_CTX *gCtx = NULL;
// cvitek

SAMPLE_VI_CONFIG_S stViConfig;

static void _DefaultVencInputCfg(chnInputCfg *pIc)
{
    pIc->bsMode = BS_MODE_QUERY_STAT;
    pIc->rcMode = SAMPLE_RC_CBR; // cbr
    pIc->iqp = DEF_IQP;
    pIc->pqp = DEF_PQP;
    pIc->gop = 50;
    pIc->bitrate = 8000;
    pIc->firstFrmstartQp = 63;
    pIc->num_frames = -1;
    pIc->framerate = VI_FSP;
    pIc->srcFramerate = VI_FSP;
    pIc->maxQp = 42;
    pIc->minQp = 22;
    pIc->maxIqp = 42;
    pIc->minIqp = 22;
    pIc->bind_mode = VENC_BIND_DISABLE;
}

inline PIC_SIZE_E MapSizeToPicSize(CVI_U32 width, CVI_U32 height) {
    if (width == 352 && height == 288)
        return PIC_CIF;
    else if (width == 720 && height == 576)
        return PIC_D1_PAL;
    else if (width == 720 && height == 480)
        return PIC_D1_NTSC;
    else if (width == 1280 && height == 720)
        return PIC_720P;
    else if (width == 1920 && height == 1080)
        return PIC_1080P;
    else if (width == 2592 && height == 1520)
        return PIC_2592x1520;
    else if (width == 2592 && height == 1536)
        return PIC_2592x1536;
    else if (width == 2592 && height == 1944)
        return PIC_2592x1944;
    else if (width == 2716 && height == 1524)
        return PIC_2716x1524;
    else if (width == 3840 && height == 2160)
        return PIC_3840x2160;
    else if (width == 4096 && height == 2160)
        return PIC_4096x2160;
    else if (width == 3000 && height == 3000)
        return PIC_3000x3000;
    else if (width == 4000 && height == 3000)
        return PIC_4000x3000;
    else if (width == 3840 && height == 8640)
        return PIC_3840x8640;
    else if (width == 640 && height == 480)
        return PIC_640x480;
    else
        return PIC_CUSTOMIZE;
}

static CVI_S32 InitVenc(VENC_CHN VencChn, chnInputCfg *pIc)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    PAYLOAD_TYPE_E enPayLoad = PT_H265;
    PIC_SIZE_E enSize = PIC_1080P;
    VENC_GOP_MODE_E enGopMode = VENC_GOPMODE_NORMALP;
    VENC_GOP_ATTR_S stGopAttr;
    SAMPLE_RC_E enRcMode;
    CVI_U32 u32Profile = 0;

    enRcMode = (SAMPLE_RC_E) pIc->rcMode;
    memset(&stGopAttr, 0, sizeof(VENC_GOP_ATTR_S));
    s32Ret = SAMPLE_COMM_VENC_GetGopAttr(enGopMode, &stGopAttr);
    if (s32Ret != CVI_SUCCESS) {
        printf("[Err]Venc Get GopAttr for %#x!\n", s32Ret);
        return CVI_FAILURE;
    }

    if (!strcmp(pIc->codec, "264"))
        enPayLoad = PT_H264;
    else if (!strcmp(pIc->codec, "265"))
        enPayLoad = PT_H265;
    else {
        printf("Unsupport Codec %s, only support h265/h264!\n", pIc->codec);
        return CVI_FAILURE;
    }

    enSize = MapSizeToPicSize(pIc->width, pIc->height);

    s32Ret = SAMPLE_COMM_VENC_Start(
            pIc,
            VencChn,
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
    CVI_BOOL abChnEnable[VPSS_MAX_PHY_CHN_NUM] = { 0 };
    VPSS_CHN_ATTR_S astVpssChnAttr[VPSS_MAX_PHY_CHN_NUM];
    CVI_S32 s32Ret = CVI_SUCCESS;

    for (size_t i = 0; i < NUM_OF_CHN_PER_VPSS; i++) {
        size_t idx = VpssGrp * NUM_OF_CHN_PER_VPSS + i;
        abChnEnable[i] = CVI_TRUE;
        astVpssChnAttr[i].u32Width                    = gCodecs[idx].width;
        astVpssChnAttr[i].u32Height                   = gCodecs[idx].height;
        astVpssChnAttr[i].enVideoFormat               = VIDEO_FORMAT_LINEAR;
        astVpssChnAttr[i].enPixelFormat               = PIXEL_FORMAT_YUV_PLANAR_420;
        astVpssChnAttr[i].stFrameRate.s32SrcFrameRate = VI_FSP;
        astVpssChnAttr[i].stFrameRate.s32DstFrameRate = gCodecs[idx].framerate;
        astVpssChnAttr[i].u32Depth                    = 1;
        astVpssChnAttr[i].bMirror                     = CVI_FALSE;
        astVpssChnAttr[i].bFlip                       = CVI_FALSE;
        astVpssChnAttr[i].stAspectRatio.enMode        = ASPECT_RATIO_AUTO;
        astVpssChnAttr[i].stAspectRatio.bEnableBgColor = CVI_TRUE;
	    astVpssChnAttr[i].stAspectRatio.u32BgColor    = COLOR_RGB_BLACK;
        astVpssChnAttr[i].stNormalize.bEnable         = CVI_FALSE;
    }

    /*start vpss*/
    s32Ret = SAMPLE_COMM_VPSS_Init(VpssGrp, abChnEnable, stVpssGrpAttr, astVpssChnAttr);
    if (s32Ret != CVI_SUCCESS) {
        printf("init Vpss Grp %u failed. s32Ret: 0x%x !\n", VpssGrp, s32Ret);
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp, abChnEnable, stVpssGrpAttr, astVpssChnAttr);
    if (s32Ret != CVI_SUCCESS) {
        printf("start Vpss Grp %u failed. s32Ret: 0x%x !\n", VpssGrp, s32Ret);
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_VI_Bind_VPSS(0, VpssGrp);
    if (s32Ret != CVI_SUCCESS) {
        printf("vi bind vpss Grp: %u failed. s32Ret: 0x%x !\n", VpssGrp, s32Ret);
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
    VI_PIPE ViPipe = 0;
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
    VB_CONFIG_S stVbConf;
    CVI_U32 u32BlkSize;
    enCompressMode = COMPRESS_MODE_NONE;
    memset(&stVbConf, 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt = 1;
    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT,
                                         DATA_BITWIDTH_8, enCompressMode, DEFAULT_ALIGN);
    
    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt = NUM_VB_CNT;
    stVbConf.astCommPool[0].enRemapMode = VB_REMAP_MODE_CACHED;
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (s32Ret != CVI_SUCCESS) {
        printf("sys init failed. s32Ret: 0x%x !\n", s32Ret);
        return s32Ret;
    }

    s32Ret = SAMPLE_PLAT_VI_INIT(&stViConfig);
    // 4K need to disable LSC
    ISP_MESH_SHADING_ATTR_S stLSC = {0};
    CVI_ISP_GetMeshShadingAttr(ViPipe, &stLSC);
    stLSC.Enable = CVI_FALSE;
    CVI_ISP_SetMeshShadingAttr(ViPipe, &stLSC);

    return s32Ret;
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
    stVpssGrpAttr.u32MaxW = 3840;
    stVpssGrpAttr.u32MaxH = 2160;
    stVpssGrpAttr.u8VpssDev = 0;

    for (CVI_S32 i = 0; i < NUM_OF_VPSS_GRP; i++) {
        s32Ret = InitVpss(i, &stVpssGrpAttr);
        if (s32Ret != CVI_SUCCESS) {
            printf("InitVpss Grp%d failed with %x\n", i, s32Ret);
            return -1;
        }
    }

    chnInputCfg pIc; //for venc
    _DefaultVencInputCfg(&pIc);
    for (size_t i = 0; i < NUM_OF_STREAMS; i++) {
        strcpy(pIc.codec, gCodecs[i].str.c_str());
        pIc.width = gCodecs[i].width;
        pIc.height = gCodecs[i].height;
        pIc.srcFramerate = VI_FSP;
        // pIc.dstFramerate = gCodecs[i].framerate;
        pIc.bitrate = gCodecs[i].bitrate;
#ifdef VPSS_VENC_BINDING_MODE
        pIc.bind_mode = VENC_BIND_VPSS;
        pIc.vpssGrp = i / NUM_OF_VPSS_GRP;
        pIc.vpssChn = i % NUM_OF_CHN_PER_VPSS;
#endif
        InitVenc(i, &pIc);
    }
    
    return 0;
}

void finalize()
{
    for (size_t i = 0; i < NUM_OF_STREAMS; i++) {
#ifdef VPSS_VENC_BINDING_MODE
        VPSS_GRP grp = i / NUM_OF_CHN_PER_VPSS;
        VPSS_CHN chn = i % NUM_OF_CHN_PER_VPSS;
        SAMPLE_COMM_VPSS_UnBind_VENC(grp, chn, i);
#endif
        SAMPLE_COMM_VENC_Stop(i);
    }

    for (size_t i = 0; i < NUM_OF_VPSS_GRP; i++)
        SAMPLE_COMM_VI_UnBind_VPSS(0, i);
    
    CVI_BOOL abChnEnable[VPSS_MAX_PHY_CHN_NUM] = {0};
    for (int i = 0; i < NUM_OF_CHN_PER_VPSS; i++)
        abChnEnable[i] = true;
    
    for (size_t i = 0; i < NUM_OF_VPSS_GRP; i++)
        SAMPLE_COMM_VPSS_Stop(i, abChnEnable);
    
    SAMPLE_COMM_ISP_Stop(0);
    SAMPLE_COMM_VI_DestroyVi(&stViConfig);
    SAMPLE_COMM_SYS_Exit();
}

static void handleSig(int signo)
{
    gRun = false;
}

typedef struct _rountine_data {
    VENC_CHN VencChn;
    CVI_RTSP_SESSION *session;
#ifdef TEST_FILE_MODE    
    FILE *file_bs;
#endif
} rountine_data;

void *VencRoutine(void *arg) {

    CVI_S32 s32Ret = CVI_SUCCESS;
    rountine_data *venc_data = (rountine_data *)arg;
    VENC_CHN VencChn = venc_data->VencChn;

    while (gRun) {
        CVI_CHAR trace_str[32] = {0};
        VENC_STREAM_S stStream;
        VENC_CHN_STATUS_S stStat;
#ifndef VPSS_VENC_BINDING_MODE
        VPSS_GRP VpssGrp = venc_data->VencChn / NUM_OF_CHN_PER_VPSS;
        VPSS_CHN VpssChn = venc_data->VencChn % NUM_OF_CHN_PER_VPSS;
        VIDEO_FRAME_INFO_S stVencFrame;
        s32Ret = CVI_VPSS_GetChnFrame(VpssGrp, VpssChn, &stVencFrame, 1000);
        if (s32Ret != CVI_SUCCESS) {
            printf("CVI_VPSS_GetChnFrame Grp: %u Chn: %u failed with %#x\n", VpssGrp, VpssChn, s32Ret);
            return nullptr;
        }

        sprintf(trace_str, "Send VENC %u", VencChn);
        CVI_SYS_TraceBegin(trace_str);
        CVI_S32 s32SetFrameMilliSec = 20000;
        s32Ret = CVI_VENC_SendFrame(VencChn, &stVencFrame, s32SetFrameMilliSec);
        if (s32Ret != CVI_SUCCESS) {
            printf("CVI_VENC_SendFrame failed! %d\n", s32Ret);
            return nullptr;
        }
        CVI_SYS_TraceEnd();
#endif
        sprintf(trace_str, "Query VENC %u", VencChn);
        CVI_SYS_TraceBegin(trace_str);
        s32Ret = CVI_VENC_QueryStatus(VencChn, &stStat);
        if (s32Ret != CVI_SUCCESS) {
            printf("CVI_VENC_QueryStatus %d failed with %#x!\n", VencChn, s32Ret);
            return nullptr;
        }
        if (!stStat.u32CurPacks) {
            printf("NOTE: Current frame is NULL!\n");
            return nullptr;
        }
        CVI_SYS_TraceEnd();

        stStream.pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
        if (stStream.pstPack == NULL) {
            printf("malloc memory failed!\n");
            return nullptr;
        }

        sprintf(trace_str, "GetStream %u", VencChn);
        CVI_SYS_TraceBegin(trace_str);
        s32Ret = CVI_VENC_GetStream(VencChn, &stStream, -1);
        if (s32Ret != CVI_SUCCESS) {
            printf("CVI_VENC_GetStream: %d failed with %#x!\n", VencChn, s32Ret);
            free(stStream.pstPack);
            stStream.pstPack = NULL;
            return nullptr;
        }
        CVI_SYS_TraceEnd();
        //
        VENC_PACK_S *ppack;
        CVI_RTSP_DATA data;
        memset(&data, 0, sizeof(CVI_RTSP_DATA));
        
        sprintf(trace_str, "RTSP %u", VencChn);
        CVI_SYS_TraceBegin(trace_str);
        int len = 0;
        for (unsigned int i = 0; i < stStream.u32PackCount; i++) {
            ppack = &stStream.pstPack[i];
            len += ppack->u32Len - ppack->u32Offset;
        }

        {
            unsigned char *p = (unsigned char *)malloc(len);
            if(p) {
                unsigned char *pTmp = p;

                data.blockCnt = stStream.u32PackCount;
                for (unsigned int i = 0; i < stStream.u32PackCount; i++) {
                    ppack = &stStream.pstPack[i];
                    memcpy(pTmp,ppack->pu8Addr + ppack->u32Offset, ppack->u32Len - ppack->u32Offset);
                    pTmp += ppack->u32Len - ppack->u32Offset;

                    data.dataPtr[i] = ppack->pu8Addr + ppack->u32Offset;
                    data.dataLen[i] = ppack->u32Len - ppack->u32Offset;
                }
#ifdef TEST_FILE_MODE                    
                fwrite(p, len, 1, venc_data->file_bs);
#endif
                CVI_RTSP_WriteFrame(gCtx, venc_data->session->video, &data);

                free(p);
            } else {
                printf("[%s,%d] malloc failed, drop current packet count(%d).\n",__FUNCTION__,__LINE__,stStream.u32PackCount);
            }
        }
        CVI_SYS_TraceEnd();
        s32Ret = CVI_VENC_ReleaseStream(VencChn, &stStream);
        if (s32Ret != CVI_SUCCESS) {
            printf("CVI_VENC_ReleaseStream %d, s32Ret = %d\n", VencChn, s32Ret);
            free(stStream.pstPack);
            stStream.pstPack = NULL;
            return nullptr;
        }

        free(stStream.pstPack);
        stStream.pstPack = NULL;
#ifndef VPSS_VENC_BINDING_MODE
        if (CVI_VPSS_ReleaseChnFrame(VpssGrp, VpssChn, &stVencFrame) != 0)
            printf("CVI_VPSS_ReleaseChnFrame Grp: %u Chn: %u NG\n", VpssGrp, VpssChn);
#endif
    }

    return nullptr;
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

    CVI_RTSP_CONFIG config = {0};
    config.port = 554;

    if (0 > CVI_RTSP_Create(&gCtx, &config)) {
        std::cout << "fail to create rtsp contex" << std::endl;
        return -1;
    }
    // set listener
    CVI_RTSP_STATE_LISTENER listener = {0};
    listener.onConnect = connect;
    listener.argConn = gCtx;
    listener.onDisconnect = disconnect;

    CVI_RTSP_SetListener(gCtx, &listener);

    if (0 > CVI_RTSP_Start(gCtx)) {
        std::cout << "fail to start" << std::endl;
        return -1;
    }

    // add media stream
    CVI_RTSP_SESSION *session[NUM_OF_STREAMS];
    CVI_RTSP_SESSION_ATTR attr = {0};
    for (int i = 0; i < NUM_OF_STREAMS; ++i) {
        attr.video.codec = (gCodecs[i].str == "265") ? RTSP_VIDEO_H265 : RTSP_VIDEO_H264;
        snprintf(attr.name, sizeof(attr.name), "%s%d", "rtsp", i);
        attr.video.bitrate = (unsigned int) gCodecs[i].bitrate * 10;

        CVI_RTSP_CreateSession(gCtx, &attr, &session[i]);
    }

    pthread_t VencThread[NUM_OF_STREAMS];
    rountine_data data[NUM_OF_STREAMS];
#ifdef TEST_FILE_MODE
    FILE *file_bs[NUM_OF_STREAMS];
    CVI_CHAR bs_file[32];
#endif
    for (size_t i = 0; i < NUM_OF_STREAMS; ++i) {
        data[i].VencChn = i;
        data[i].session = session[i];
#ifdef TEST_FILE_MODE
        memset(&bs_file, 0, sizeof(bs_file));
        snprintf(bs_file, sizeof(bs_file), "./stream%lu.%s", i, gCodecs[i].str.c_str());
        file_bs[i] = fopen(bs_file, "wb");
        if (file_bs[i] == NULL)
            printf("fopen: %s failed\n", bs_file);
        data[i].file_bs = file_bs[i];
#endif
        pthread_create(&VencThread[i], NULL, VencRoutine, (void *)&data[i]);
    }

    for (size_t i = 0; i < NUM_OF_STREAMS; ++i) {
        pthread_join(VencThread[i], NULL);
#ifdef TEST_FILE_MODE
        fclose(file_bs[i]);
#endif
    }

    CVI_RTSP_Stop(gCtx);

    for (int i = 0; i < NUM_OF_STREAMS; ++i)
        CVI_RTSP_DestroySession(gCtx, session[i]);

    CVI_RTSP_Destroy(&gCtx);

    finalize();

    return 0;
}
