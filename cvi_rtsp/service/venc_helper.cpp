#include <cvi_venc.h>
#include <cvi_comm_isp.h>
#include <sample_comm.h>
#include "venc_helper.h"

static uint32_t gu32VencStatus = 0;
static int getValidVencChn(void) {
    for (int i=0; i<32; i++) {
        if ((gu32VencStatus & 1<<i)==0) {
            gu32VencStatus |= 1<<i;
            return i;
        }
    }
    return -1;
}

static void clearUsedVencChn(int i) {
    gu32VencStatus &= ~(1<<i);
}

void init_venc_cfg(chnInputCfg *pIc)
{
    SAMPLE_COMM_VENC_InitChnInputCfg(pIc);
    strcpy(pIc->codec, "264");
    pIc->u32Profile = DEFAULT_H264_PROFILE_DEFAULT;
    pIc->bsMode = DEFAULT_BS_MODE;  // --getBsMode=0
    pIc->rcMode = DEFAULT_RC_MODE;
    pIc->statTime = DEFAULT_STAT_TIME;
    pIc->gop = DEFAULT_GOP;
    pIc->iqp = DEFAULT_IQP;
    pIc->pqp = DEFAULT_PQP;
    pIc->bitrate = DEFAULT_KBITRATE;
    pIc->maxbitrate = CVI_H26x_MAX_BITRATE_DEFAULT;
    pIc->firstFrmstartQp = DEFAULT_FIRSTQP;
    pIc->initialDelay = CVI_H26x_INITIAL_DELAY_DEFAULT;
    pIc->u32ThrdLv = CVI_H26x_THRD_LV_DEFAULT;
    pIc->h264EntropyMode = CVI_H264_ENTROPY_MODE_DEFAULT;
    pIc->maxIprop = CVI_H26x_MAX_I_PROP_DEFAULT;
    pIc->minIprop = CVI_H26x_MIN_I_PROP_DEFAULT;
    pIc->h264ChromaQpOffset = CVI_H264_CHROMA_QP_OFFSET_DEFAULT;
    pIc->u32IntraCost = CVI_H26X_INTRACOST_DEFAULT;
    pIc->u32RowQpDelta = CVI_H26X_ROW_QP_DELTA_DEFAULT;
    pIc->num_frames = -1;
    pIc->srcFramerate = DEFAULT_FPS;
    pIc->framerate = DEFAULT_FPS;
    pIc->maxQp = DEF_264_MAXIQP;
    pIc->minQp = DEF_264_MINIQP;
    pIc->maxIqp = DEF_264_MAXQP;
    pIc->minIqp = DEF_264_MINQP;
    pIc->s32IPQpDelta = CVI_H26x_IP_QP_DELTA_DEFAULT;
    pIc->s32ChangePos = CVI_H26x_CHANGE_POS_DEFAULT;
    pIc->s32MinStillPercent = CVI_H26x_MIN_STILL_PERCENT_DEFAULT;
    pIc->u32MaxStillQP = CVI_H26x_MAX_STILL_QP_DEFAULT;
    pIc->u32MotionSensitivity = CVI_H26x_MOTION_SENSITIVITY_DEFAULT;
    pIc->s32AvbrFrmLostOpen = DEFAULT_AVBR_FRM_LOST_OPEN;
    pIc->s32AvbrFrmGap = CVI_H26x_AVB_FRM_GAP_DEFAULT;
    pIc->s32AvbrPureStillThr = CVI_H26x_AVB_PURE_STILL_THR_DEFAULT;
    // Special case, set default to 0 instead of CVI_H26X_VIDEO_FORMAT_DEFAULT (5)
    pIc->videoFormat = 0;

    pIc->bind_mode = VENC_BIND_VPSS;
    pIc->vpssGrp = 0;
    pIc->vpssChn = 0;
}

inline PIC_SIZE_E MapSizeToPicSize(SIZE_S stSize) {
    CVI_U32 width, height;
    width = stSize.u32Width;
    height = stSize.u32Height;

    if (width == 352 && height == 288) {
        return PIC_CIF;
    } else if (width == 720 && height == 576)
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
    else if (width == 2688 && height == 1520)
        return PIC_2688x1520;
    else
        return PIC_CUSTOMIZE;
}

int init_venc_withCtx(SERVICE_CTX *ctx)
{
    int iRet = 0;
    for (int idx=0; idx<ctx->dev_num; idx++) {
        SERVICE_CTX_ENTITY *ent = (SERVICE_CTX_ENTITY *) &ctx->entity[idx];
        iRet = init_venc(ent);
        if (iRet !=0) return iRet;
    }
    return iRet;
}

int init_venc(SERVICE_CTX_ENTITY *ent)
{
    CVI_S32 s32Ret = 0;
    chnInputCfg *pIc = &ent->venc_cfg;
    pIc->width = ent->dst_width;
    pIc->height = ent->dst_height;

    // Rate Control
    SAMPLE_RC_E enRcMode = (SAMPLE_RC_E) pIc->rcMode;

    // GOP
    VENC_GOP_ATTR_S gopAttr = {};
    VENC_GOP_MODE_E gopMode = (VENC_GOP_MODE_E) pIc->gopMode;
    s32Ret = SAMPLE_COMM_VENC_GetGopAttr(gopMode, &gopAttr);
    if (s32Ret != CVI_SUCCESS) {
        printf("SAMPLE_COMM_VENC_GetGopAttr Failed with %#x!\n", s32Ret);
        return -1;
    }

    PAYLOAD_TYPE_E EnPayLoad;
    if (!strcmp(pIc->codec, "mjp")) {
        EnPayLoad = PT_MJPEG;
    } else if (!strcmp(pIc->codec, "jpg")) {
        EnPayLoad = PT_JPEG;
    } else if (!strcmp(pIc->codec, "264")) {
        EnPayLoad = PT_H264;
    } else if (!strcmp(pIc->codec, "265")) {
        EnPayLoad = PT_H265;
    } else {
        printf("Unsupport Codec %s, only support mjp/jpg/h265/h264!\n", pIc->codec);
        return -1;
    }

    SIZE_S stSize = {pIc->width, pIc->height};
    PIC_SIZE_E enSize = MapSizeToPicSize(stSize);
    CVI_U32 u32Profile = pIc->u32Profile;

    /* Set the pixelformat to NV21 in 182x */
    pIc->pixel_format = (SAMPLE_PIXEL_FORMAT == PIXEL_FORMAT_NV21) ? 3 : 0 ;

    if (ent->bVencBindVpss) {
        pIc->bind_mode = VENC_BIND_VPSS;
        pIc->vpssGrp = ent->VpssGrp;
        pIc->vpssChn = ent->VpssChn;
    } else {
        pIc->bind_mode = VENC_BIND_DISABLE;
    }

    if (-1 == (ent->VencChn = getValidVencChn())) {
        printf("fail to get valid enc chn\n");
        return -1;
    }

    if (((SERVICE_CTX *)ent->ctx)->sbm.enable) {
        commonInputCfg pCic;
        SAMPLE_COMM_VENC_InitCommonInputCfg(&pCic);
        SAMPLE_COMM_VENC_SetModParam(&pCic);

        pIc->bIsoSendFrmEn = true;
        CVI_S32 bind_mode = pIc->bind_mode;
        pIc->bind_mode = VENC_BIND_DISABLE;
        s32Ret = SAMPLE_COMM_VENC_Start(pIc, ent->VencChn, EnPayLoad, enSize, enRcMode, u32Profile, CVI_FALSE, &gopAttr);

        if (s32Ret != CVI_SUCCESS) {
            printf("SAMPLE_COMM_VENC_Start failed with %#x!\n", s32Ret);
            return -1;
        }
        pIc->bind_mode = bind_mode;
        if (pIc->bind_mode == VENC_BIND_VI) {
            VI_PIPE ViPipe = 0;
            VI_CHN ViChn = 0;

            printf("VI_Bind_VENC, VencChn = %d\n", ent->VencChn);
            SAMPLE_COMM_VI_Bind_VENC(ViPipe, ViChn, ent->VencChn);
        } else if (pIc->bind_mode == VENC_BIND_VPSS) {
            printf("VPSS_Bind_VENC, vpss Grp = %d, Chn = %d, VencChn = %d\n",
                    pIc->vpssGrp, pIc->vpssChn, ent->VencChn);
            SAMPLE_COMM_VPSS_Bind_VENC(pIc->vpssGrp, pIc->vpssChn, ent->VencChn);
        }
    } else {

        commonInputCfg pCic;
        SAMPLE_COMM_VENC_InitCommonInputCfg(&pCic);
        SAMPLE_COMM_VENC_SetModParam(&pCic);
        s32Ret = SAMPLE_COMM_VENC_Start(pIc, ent->VencChn, EnPayLoad, enSize, enRcMode, u32Profile, CVI_FALSE, &gopAttr);

        if (s32Ret != CVI_SUCCESS) {
            printf("SAMPLE_COMM_VENC_Start failed with %#x!\n", s32Ret);
            return -1;
        }
    }

    return 0;
}

void deinit_venc(SERVICE_CTX_ENTITY *ent)
{
    if (ent->VencChn == -1)
        return;
    if (ent->bVencBindVpss) {
        SAMPLE_COMM_VPSS_UnBind_VENC(ent->VpssGrp, ent->VpssChn, ent->VencChn);
    }
    SAMPLE_COMM_VENC_Stop(ent->VencChn);
    clearUsedVencChn(ent->VencChn);
    ent->VencChn = -1;
    ent->venc_cfg.bCreateChn = 0;
}
