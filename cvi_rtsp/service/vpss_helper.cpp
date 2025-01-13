#include <unistd.h>
#include <algorithm>
#include <sample_comm.h>
#include <linux/cvi_comm_video.h>
#include <cvi_isp.h>
#include <cvi_ae.h>
#include <cvi_awb.h>
#include <cvi_af.h>
#include <cvi_sys.h>
#include <cvi_buffer.h>
#include <cvi_bin.h>
#include "vpss_helper.h"

static CVI_S32 replay_vi_init(SERVICE_CTX *ctx);

int vpss_init_helper(uint32_t vpssGrpId, uint32_t enSrcWidth, uint32_t enSrcHeight,
        PIXEL_FORMAT_E enSrcFormat, uint32_t enDstWidth, uint32_t enDstHeight,
        PIXEL_FORMAT_E enDstFormat, VPSS_MODE_E mode, uint32_t enabledChannel,
        bool keepAspectRatio, uint8_t u8Dev, SERVICE_SBM sbm) {
    printf("VPSS init with src (%u, %u) dst (%u, %u).\n", enSrcWidth, enSrcHeight, enDstWidth, enDstHeight);
    int s32Ret = CVI_FAILURE;

    do {
        VPSS_GRP_ATTR_S stVpssGrpAttr;
        VPSS_CHN_ATTR_S stVpssChnAttr;
        vpss_grp_attr_default(&stVpssGrpAttr, enSrcWidth, enSrcHeight, enSrcFormat, u8Dev);
        vpss_chn_attr_default(&stVpssChnAttr, enDstWidth, enDstHeight, enDstFormat, keepAspectRatio);

        /*start vpss*/
        s32Ret = CVI_VPSS_CreateGrp(vpssGrpId, &stVpssGrpAttr);
        printf("CVI_VPSS_CreateGrp:%d, s32Ret=%d\n", vpssGrpId, s32Ret);
        if (s32Ret != CVI_SUCCESS) {
            syslog(LOG_ERR, "CVI_VPSS_CreateGrp(grp:%d) failed with %#x!\n", vpssGrpId, s32Ret);
            break;
        }
        s32Ret = CVI_VPSS_ResetGrp(vpssGrpId);
        if (s32Ret != CVI_SUCCESS) {
            syslog(LOG_ERR, "CVI_VPSS_ResetGrp(grp:%d) failed with %#x!\n", vpssGrpId, s32Ret);
            break;
        }
        if (enabledChannel > 4) {
            syslog(LOG_ERR, "Maximum value for enabledChannel is 4.");
            enabledChannel =4;
        }
        for (uint32_t i = 0; i < enabledChannel; i++) {
            s32Ret = CVI_VPSS_SetChnAttr(vpssGrpId, i, &stVpssChnAttr);
            if (s32Ret != CVI_SUCCESS) {
                syslog(LOG_ERR, "CVI_VPSS_SetChnAttr failed with %#x\n", s32Ret);
                break;
            }

            if (sbm.enable) {
                VPSS_CHN_BUF_WRAP_S stVpssChnBufWrap = {};
                stVpssChnBufWrap.bEnable = sbm.enable;
                stVpssChnBufWrap.u32BufLine = sbm.bufLine;
                stVpssChnBufWrap.u32WrapBufferSize = sbm.bufSize;

                if (vpssGrpId == 0 && i == VPSS_CHN0) {
                    s32Ret = CVI_VPSS_SetChnBufWrapAttr(vpssGrpId, i, &stVpssChnBufWrap);
                    if (s32Ret != CVI_SUCCESS) {
                        syslog(LOG_ERR, "CVI_VPSS_SetChnBufWrapAttr failed with %#x\n", s32Ret);
                        break;
                    }
                }
            }

            s32Ret = CVI_VPSS_EnableChn(vpssGrpId, i);
            if (s32Ret != CVI_SUCCESS) {
                syslog(LOG_ERR, "CVI_VPSS_EnableChn failed with %#x\n", s32Ret);
                break;
            }
        }
        s32Ret = CVI_VPSS_StartGrp(vpssGrpId);
        if (s32Ret != CVI_SUCCESS) {
            syslog(LOG_ERR, "CVI_VPSS_StartGrp failed with %#x\n", s32Ret);
            break;
        }
    } while (false);

    return s32Ret;
}

typedef struct {
    bool inited;
    // SAMPLE_INI_CFG_S stIniCfg;
    // SAMPLE_VI_CONFIG_S stViCfg;
    unsigned int DevFlags;
} STATUS_CTX_VI;
static STATUS_CTX_VI gCtxVi = {0};

int init_vi(SERVICE_CTX *ctx)
{
    if (gCtxVi.inited) {
        return 0;
    }

    SAMPLE_VI_CONFIG_S *pstViCfg = &ctx->stViConfig;

    if (ctx->enable_set_sensor_config) {
        SAMPLE_COMM_VI_SetIniPath(ctx->sensor_config_path);
        printf("SAMPLE_COMM_VI_SetIniPath[%s] done",
                ctx->sensor_config_path);
    }

    if (ctx->isp_debug_lvl <= CVI_DBG_DEBUG) {
        LOG_LEVEL_CONF_S log_conf;
        log_conf.enModId = CVI_ID_ISP;
        log_conf.s32Level = ctx->isp_debug_lvl;
        CVI_LOG_SetLevelConf(&log_conf);
        printf("Set log level[%d] done", log_conf.s32Level);
    }

    CVI_VI_SetDevNum(ctx->dev_num);

    if ((ctx->stIniCfg.devNum >= 2) &&
        (ctx->rtsp_num == 1) &&
        ((ctx->vi_vpss_mode == VI_ONLINE_VPSS_ONLINE) || (ctx->vi_vpss_mode == VI_OFFLINE_VPSS_ONLINE)) ) {
        printf("we can't set vpssOnlineMode if dualSns run oneRtsp, because sns can't be switched\n");
        if (ctx->vi_vpss_mode == VI_ONLINE_VPSS_ONLINE) {
            printf("preViVpssMode:%d curViVpssMode:%d\n", ctx->vi_vpss_mode, VI_ONLINE_VPSS_OFFLINE);
            ctx->vi_vpss_mode = VI_ONLINE_VPSS_OFFLINE;
        } else {
            printf("preViVpssMode:%d curViVpssMode:%d\n", ctx->vi_vpss_mode, VI_OFFLINE_VPSS_OFFLINE);
            ctx->vi_vpss_mode = VI_OFFLINE_VPSS_OFFLINE;
        }
    }

    PIC_SIZE_E enPicSize;
    SIZE_S stSize;
    CVI_S32 s32Ret = CVI_SUCCESS;
    VB_CONFIG_S stVbConf = {};
    CVI_U32 u32BlkSize, u32BlkRotSize, i;
    memset(&stVbConf, 0, sizeof(VB_CONFIG_S));

    SAMPLE_COMM_VI_IniToViCfg(&ctx->stIniCfg, pstViCfg);
    for (i = 0; i < ctx->dev_num; ++i) {
        SERVICE_CTX_ENTITY *ent = &ctx->entity[i];
        // FIXME
        pstViCfg->astViInfo[i].stChnInfo.enCompressMode = ent->compress_mode;
        if (ctx->vi_vpss_mode != VI_OFFLINE_VPSS_OFFLINE) {
            pstViCfg->astViInfo[i].stPipeInfo.enMastPipeMode = VI_OFFLINE_VPSS_OFFLINE;
            pstViCfg->astViInfo[i].stPipeInfo.aPipe[0] = i;
            pstViCfg->astViInfo[i].stPipeInfo.aPipe[1] = -1;
            pstViCfg->astViInfo[i].stPipeInfo.aPipe[2] = -1;
            pstViCfg->astViInfo[i].stPipeInfo.aPipe[3] = -1;
        }

        s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(pstViCfg->astViInfo[i].stSnsInfo.enSnsType, &enPicSize);
        if (s32Ret != CVI_SUCCESS) {
            printf("SAMPLE_COMM_VI_GetSizeBySensor failed with %#x\n", s32Ret);
            return -1;
        }

        s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
        if (s32Ret != CVI_SUCCESS) {
            printf("SAMPLE_COMM_SYS_GetPicSize failed with %#x\n", s32Ret);
            return -1;
        }

        PIXEL_FORMAT_E pixFomart = VI_PIXEL_FORMAT;

        ent->src_width = stSize.u32Width;
        ent->src_height = stSize.u32Height;
        ent->dst_width = ent->src_width;
        ent->dst_height = ent->src_height;
        if (ctx->replayMode) {
            ent->src_width = ent->dst_width = stSize.u32Width = ctx->replayParam.width;
            ent->src_height = ent->dst_height = stSize.u32Height = ctx->replayParam.height;
            if (ctx->replayParam.pixelFormat)
                pixFomart = (PIXEL_FORMAT_E)ctx->replayParam.pixelFormat;
        }
        stVbConf.u32MaxPoolCnt++;
        u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, pixFomart, DATA_BITWIDTH_8,
                COMPRESS_MODE_NONE, DEFAULT_ALIGN);
        u32BlkRotSize = COMMON_GetPicBufferSize(stSize.u32Height, stSize.u32Width, pixFomart,
                DATA_BITWIDTH_8, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
        u32BlkSize = std::max(u32BlkSize, u32BlkRotSize);
        stVbConf.astCommPool[i].u32BlkSize = u32BlkSize;
        stVbConf.astCommPool[i].u32BlkCnt = ent->buf_blk_cnt;
        stVbConf.astCommPool[i].enRemapMode = VB_REMAP_MODE_CACHED;
    }

    for (int k=0; k<ctx->dev_num; ++k) {
        if (ctx->buf1_blk_cnt > 0) {
            stVbConf.u32MaxPoolCnt++;
            CVI_U32 u32BlkTpuSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, PIXEL_FORMAT_RGB_888_PLANAR,
                    DATA_BITWIDTH_8, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
            stVbConf.astCommPool[i+k].u32BlkSize = u32BlkTpuSize;
            stVbConf.astCommPool[i+k].u32BlkCnt = ctx->buf1_blk_cnt;  // default: 4
            stVbConf.astCommPool[i+k].enRemapMode = VB_REMAP_MODE_CACHED;
        }
    }

    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (s32Ret != CVI_SUCCESS) {
        printf("sys init failed. s32Ret: 0x%x !\n", s32Ret);
        return -1;
    }

    if (ctx->vi_vpss_mode != VI_OFFLINE_VPSS_OFFLINE) { //online
        VI_VPSS_MODE_S stVIVPSSMode = {};
        VPSS_MODE_S stVPSSMode = {};

        printf("enable online mode %d\n", ctx->vi_vpss_mode);
        //cv183x does not support different mode Settings for dual sensor
        stVIVPSSMode.aenMode[0] = stVIVPSSMode.aenMode[1] = ctx->vi_vpss_mode;

        s32Ret = CVI_SYS_SetVIVPSSMode(&stVIVPSSMode);
        if (s32Ret != CVI_SUCCESS) {
            printf("CVI_SYS_SetVIVPSSMode failed with %#x\n", s32Ret);
            return s32Ret;
        }

        stVPSSMode.enMode = VPSS_MODE_DUAL;
        stVPSSMode.aenInput[0] = VPSS_INPUT_MEM;
        stVPSSMode.ViPipe[0] = 0;
        stVPSSMode.aenInput[1] = VPSS_INPUT_ISP;
        stVPSSMode.ViPipe[1] = 0;

        s32Ret = CVI_SYS_SetVPSSModeEx(&stVPSSMode);
        if (s32Ret != CVI_SUCCESS) {
            printf("CVI_SYS_SetVPSSModeEx failed with %#x\n", s32Ret);
            return -1;
        }
     } else {
        VPSS_MODE_S stVPSSMode = {};

        stVPSSMode.enMode = VPSS_MODE_DUAL;
        stVPSSMode.aenInput[0] = VPSS_INPUT_MEM;
        stVPSSMode.ViPipe[0] = 0;
        stVPSSMode.aenInput[1] = VPSS_INPUT_MEM;
        stVPSSMode.ViPipe[1] = 0;

        s32Ret = CVI_SYS_SetVPSSModeEx(&stVPSSMode);
        if (s32Ret != CVI_SUCCESS) {
            printf("CVI_SYS_SetVPSSModeEx failed with %#x\n", s32Ret);
            return -1;
        }
    }

    if (ctx->replayMode) {
        s32Ret = replay_vi_init(ctx);
    } else {
        s32Ret = SAMPLE_PLAT_VI_INIT(pstViCfg);
    }

    if (s32Ret != CVI_SUCCESS) {
        printf("vi init failed. s32Ret: 0x%x !\n", s32Ret);
        return -1;
    }

    for (i = 0; i < ctx->dev_num; ++i) {
        SERVICE_CTX_ENTITY *ent = &ctx->entity[i];
        ISP_PUB_ATTR_S stPubAttr = {0};

        CVI_ISP_GetPubAttr(ent->ViPipe, &stPubAttr);
        printf("CVI_ISP_GetPubAttr Get FPS: %f\n", stPubAttr.f32FrameRate);
    }

    /*
       if (stExpInfo.u32Fps) {
       cvivideosrc->fps = stExpInfo.u32Fps / 100;
       cvivideosrc->duration = 1000000000 / cvivideosrc->fps;
       GST_INFO_OBJECT(cvivideosrc, "FPS: %d, Duration: %lu", cvivideosrc->fps, cvivideosrc->duration);
       }
    */
    gCtxVi.inited = true;

    return 0;
}

int init_vpss(SERVICE_CTX *ctx)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    for (int idx = 0; idx < ctx->rtsp_num; ++idx) {
        SERVICE_CTX_ENTITY *ent = &ctx->entity[idx];
        if (ent->bVpssBinding || ent->src_width != ent->dst_width || ent->src_height != ent->dst_height) {
            ent->bVpssBinding = true;

            if (ent->VpssChn == 0) {
                // online mode use vpss grp 0 & 1
                if (ctx->vi_vpss_mode == VI_OFFLINE_VPSS_OFFLINE) {
                    ent->VpssGrp = CVI_VPSS_GetAvailableGrp();
                } else {
                    ent->VpssGrp = ent->ViChn==0 ? VPSS_ONLINE_GRP_0:VPSS_ONLINE_GRP_1;
                }

                gCtxVi.DevFlags |= 1<<ent->VpssGrp;

                printf("VpssChn0 , Enable Vpss Grp: %d\n", ent->VpssGrp);

                vpss_init_helper(ent->VpssGrp, ent->src_width, ent->src_height,
                        VI_PIXEL_FORMAT, ent->dst_width, ent->dst_height,
                        ent->enableRetinaFace?PIXEL_FORMAT_YUV_PLANAR_420:SAMPLE_PIXEL_FORMAT, VPSS_MODE_DUAL, 1, false, 1, ctx->sbm);
                CVI_VPSS_SetGrpParamfromBin(ent->VpssGrp, ent->VpssChn);
                if (ctx->vi_vpss_mode == VI_OFFLINE_VPSS_OFFLINE) {
                    s32Ret = SAMPLE_COMM_VI_Bind_VPSS(0, ent->ViChn, ent->VpssGrp);
                    if (s32Ret != CVI_SUCCESS) {
                        printf("SAMPLE_COMM_VI_Bind_VPSS Failed, s32Ret: 0x%x !\n", s32Ret);
                        return -1;
                    }
                }
            } else {
                printf("Enable VpssChn %d: %d x %d\n", ent->VpssChn, ent->dst_width, ent->dst_height);
                VPSS_CHN_ATTR_S vpss_chn_attr = {};
                vpss_chn_attr_default(&vpss_chn_attr, ent->dst_width, ent->dst_height, ent->enableRetinaFace?PIXEL_FORMAT_YUV_PLANAR_420:SAMPLE_PIXEL_FORMAT, false);
                printf("VPSS_CHN_ATTR_S: %d x %d\n", vpss_chn_attr.u32Width, vpss_chn_attr.u32Height);
                s32Ret = CVI_VPSS_SetChnAttr(ent->VpssGrp, ent->VpssChn, &vpss_chn_attr);
                if (s32Ret != CVI_SUCCESS) {
                    printf("CVI_VPSS_SetChnAttr Grp: %d, Chn: %d Failed, s32Ret: 0x%x\n", ent->VpssGrp, ent->VpssChn, s32Ret);
                    return -1;
                }

                s32Ret = CVI_VPSS_EnableChn(ent->VpssGrp, ent->VpssChn);
                if (s32Ret != CVI_SUCCESS) {
                    printf("CVI_VPSS_EnableChn Grp: %d, Chn: %d Failed, s32Ret: 0x%x\n", ent->VpssGrp, ent->VpssChn, s32Ret);
                    return -1;
                }
            }
        }
    }

    // VIDEO_FRAME_INFO_S stVideoFrame = {};
    // while(1) {
    //     for (int idx = 0; idx < ctx->dev_num; ++idx) {
    //         SERVICE_CTX_ENTITY *ent = &ctx->entity[idx];
    //         if (CVI_VPSS_GetChnFrame(ent->VpssGrp, VPSS_CHN0, &stVideoFrame, 1000) != CVI_SUCCESS) {
    //             printf("fail to get chn frame\n");
    //             //return -1;
    //         }
    //         CVI_VI_ReleaseChnFrame(ent->VpssGrp, VPSS_CHN0, &stVideoFrame);
    //     }
    // }
    return 0;
}

void deinit_vi(SERVICE_CTX *ctx)
{
    for (int idx = 0; idx < ctx->dev_num; ++idx) {
        SERVICE_CTX_ENTITY *ent = &ctx->entity[idx];

        if (!gCtxVi.inited) {
            printf("[%s, %d] not inited, return\n", __FUNCTION__, __LINE__);
            return;
        }

        if (!gCtxVi.DevFlags) {
            printf("[%s, %d] next vpssGrp not created, break\n", __FUNCTION__, __LINE__);
            break;
        }

        gCtxVi.DevFlags &= ~(1<<ent->VpssGrp);

        if (ent->bVpssBinding) {
            CVI_BOOL abChnEnable[VPSS_MAX_PHY_CHN_NUM] = {0};
            if (ctx->vi_vpss_mode == VI_OFFLINE_VPSS_OFFLINE) {
                SAMPLE_COMM_VI_UnBind_VPSS(0, ent->ViChn, ent->VpssGrp);
            }
            abChnEnable[0] = CVI_TRUE;
            if (ent->enableRetinaFace) {
                abChnEnable[1] = CVI_TRUE;
            }
            CVI_S32 s32Ret = SAMPLE_COMM_VPSS_Stop(ent->VpssGrp, abChnEnable);
            if (s32Ret != CVI_SUCCESS) {
                printf("SAMPLE_COMM_VPSS_Stop Grp:%d, s32Ret: 0x%x\n", 0, s32Ret);
                break;
            }
        }
    }

    SAMPLE_COMM_VI_DestroyIsp(&ctx->stViConfig);
    SAMPLE_COMM_VI_DestroyVi(&ctx->stViConfig);
    SAMPLE_COMM_SYS_Exit();
    gCtxVi.inited = false;
    usleep(60000);  // delay 30ms, if app behavior SAMPLE_COMM_SYS_Init/SAMPLE_COMM_SYS_Exit rapidly, ion
    // memory might release not fast enough
}

static VI_DEV_ATTR_S DEV_ATTR_SENSOR_DEFAULT = {
	VI_MODE_MIPI,
	VI_WORK_MODE_1Multiplex,
	VI_SCAN_PROGRESSIVE,
	{ -1, -1, -1, -1 },
	VI_DATA_SEQ_YUYV,
	{
		VI_VSYNC_PULSE, VI_VSYNC_NEG_LOW, VI_HSYNC_VALID_SINGNAL,
		VI_HSYNC_NEG_HIGH, VI_VSYNC_VALID_SIGNAL, VI_VSYNC_VALID_NEG_HIGH,
		{ 0, 1920, 0, 0, 1080, 0, 0, 0, 0 }
	},
	VI_DATA_TYPE_RGB,
	{ 1920, 1080 },
	{ WDR_MODE_NONE, 1080 },
	.enBayerFormat = BAYER_FORMAT_BG,
};

static VI_CHN_ATTR_S CHN_ATTR_DEFAULT = {
	{ 1920, 1080 },
	PIXEL_FORMAT_YUV_PLANAR_420,
	DYNAMIC_RANGE_SDR8,
	VIDEO_FORMAT_LINEAR,
	COMPRESS_MODE_NONE,
	CVI_FALSE, CVI_FALSE,
	0,
	{ -1, -1 },
	0
};

static ISP_PUB_ATTR_S ISP_PUB_ATTR_DEFAULT = {
	{ 0, 0, 1920, 1080 },
	{ 1920, 1080 },
	25,
	BAYER_RGGB,
	WDR_MODE_NONE,
	0
};

static CVI_S32 replay_startIsp(SERVICE_CTX *ctx)
{
	CVI_S32 s32Ret = 0;
	VI_PIPE ViPipe = 0;
	ISP_PUB_ATTR_S stPubAttr;
	ISP_BIND_ATTR_S stBindAttr;

	SAMPLE_COMM_ISP_Aelib_Callback(ViPipe);
	SAMPLE_COMM_ISP_Awblib_Callback(ViPipe);
	#if ENABLE_AF_LIB
	SAMPLE_COMM_ISP_Aflib_Callback(ViPipe);
	#endif

	snprintf(stBindAttr.stAeLib.acLibName, sizeof(CVI_AE_LIB_NAME), "%s", CVI_AE_LIB_NAME);
	stBindAttr.stAeLib.s32Id = ViPipe;
	stBindAttr.sensorId = 0;
	snprintf(stBindAttr.stAwbLib.acLibName, sizeof(CVI_AWB_LIB_NAME), "%s", CVI_AWB_LIB_NAME);
	stBindAttr.stAwbLib.s32Id = ViPipe;
	#if ENABLE_AF_LIB
	snprintf(stBindAttr.stAfLib.acLibName, sizeof(CVI_AF_LIB_NAME), "%s", CVI_AF_LIB_NAME);
	stBindAttr.stAfLib.s32Id = ViPipe;
	#endif
	s32Ret = CVI_ISP_SetBindAttr(ViPipe, &stBindAttr);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("Bind Algo failed with %#x!\n", s32Ret);
		return s32Ret;
	}

	s32Ret = CVI_ISP_MemInit(ViPipe);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("Init Ext memory failed with %#x!\n", s32Ret);
		return s32Ret;
	}

	memcpy(&stPubAttr, &ISP_PUB_ATTR_DEFAULT, sizeof(ISP_PUB_ATTR_S));
	stPubAttr.stSnsSize.u32Width = ctx->replayParam.width;
	stPubAttr.stSnsSize.u32Height = ctx->replayParam.height;
	stPubAttr.stWndRect.u32Width = ctx->replayParam.width;
	stPubAttr.stWndRect.u32Height = ctx->replayParam.height;
	stPubAttr.stWndRect.s32X = 0;
	stPubAttr.stWndRect.s32Y = 0;
	stPubAttr.enWDRMode = ctx->replayParam.WDRMode ? WDR_MODE_2To1_LINE : WDR_MODE_NONE;
	stPubAttr.f32FrameRate = ctx->replayParam.frameRate;
	stPubAttr.enBayer = (ISP_BAYER_FORMAT_E)ctx->replayParam.bayerFormat;
	s32Ret = CVI_ISP_SetPubAttr(ViPipe, &stPubAttr);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("SetPubAttr failed with %#x!\n", s32Ret);
		return s32Ret;
	}

	s32Ret = CVI_ISP_Init(ViPipe);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("ISP Init failed with %#x!\n", s32Ret);
		return s32Ret;
	}

	return CVI_SUCCESS;
}

static CVI_S32 replay_startViChn(SERVICE_CTX *ctx)
{
	CVI_S32             s32Ret = CVI_SUCCESS;
	VI_PIPE             ViPipe = 0;
	VI_CHN              ViChn = 0;
	VI_CHN_ATTR_S       stChnAttr;

	memcpy(&stChnAttr, &CHN_ATTR_DEFAULT, sizeof(VI_CHN_ATTR_S));
	stChnAttr.enPixelFormat = PIXEL_FORMAT_NV21;
	stChnAttr.stSize.u32Width = ctx->replayParam.width;
	stChnAttr.stSize.u32Height = ctx->replayParam.height;
	stChnAttr.enDynamicRange = ctx->replayParam.WDRMode ? DYNAMIC_RANGE_HDR10 : DYNAMIC_RANGE_SDR10;
	stChnAttr.enVideoFormat  = VIDEO_FORMAT_LINEAR;
	stChnAttr.enCompressMode = ctx->replayParam.compressMode;
	/* fill the sensor orientation */
	stChnAttr.bMirror = 0;
	stChnAttr.bFlip = 0;

	s32Ret = CVI_VI_SetChnAttr(ViPipe, ViChn, &stChnAttr);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("CVI_VI_SetChnAttr failed with %#x!\n", s32Ret);
		return s32Ret;
	}

	s32Ret = CVI_VI_EnableChn(ViPipe, ViChn);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("CVI_VI_EnableChn failed with %#x!\n", s32Ret);
		return s32Ret;
	}

	return CVI_SUCCESS;
}

static CVI_S32 replay_createIsp(SERVICE_CTX *ctx)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	ISP_PUB_ATTR_S stPubAttr;

	s32Ret = replay_startIsp(ctx);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("replay_startIsp failed !\n");
		return s32Ret;
	}

	s32Ret = SAMPLE_COMM_BIN_ReadParaFrombin();
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("read para fail: %#x,use default para!\n", s32Ret);
	}

	memcpy(&stPubAttr, &ISP_PUB_ATTR_DEFAULT, sizeof(ISP_PUB_ATTR_S));
	stPubAttr.stSnsSize.u32Width = ctx->replayParam.width;
	stPubAttr.stSnsSize.u32Height = ctx->replayParam.height;
	stPubAttr.stWndRect.u32Width = ctx->replayParam.width;
	stPubAttr.stWndRect.u32Height = ctx->replayParam.height;
	stPubAttr.stWndRect.s32X = 0;
	stPubAttr.stWndRect.s32Y = 0;
	stPubAttr.enWDRMode = ctx->replayParam.WDRMode ? WDR_MODE_2To1_LINE : WDR_MODE_NONE;
	stPubAttr.f32FrameRate = ctx->replayParam.frameRate;
	stPubAttr.enBayer = (ISP_BAYER_FORMAT_E)ctx->replayParam.bayerFormat;
	s32Ret = CVI_ISP_SetPubAttr(0, &stPubAttr);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("SetPubAttr failed with %#x!\n", s32Ret);
		return s32Ret;
	}

	s32Ret = SAMPLE_COMM_ISP_Run(0);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("ISP_Run failed with %#x!\n", s32Ret);
		return s32Ret;
	}

	return CVI_SUCCESS;
}

static CVI_S32 replay_vi_start_dev(SERVICE_CTX *ctx)
{
	CVI_S32 s32Ret;
	VI_DEV_ATTR_S stViDevAttr;

	memcpy(&stViDevAttr, &DEV_ATTR_SENSOR_DEFAULT, sizeof(VI_DEV_ATTR_S));

	if (!ctx->replayParam.pixelFormat) {
		stViDevAttr.stSize.u32Width = ctx->replayParam.width;
		stViDevAttr.stSize.u32Height = ctx->replayParam.height;
		if (ctx->replayParam.WDRMode) {
			stViDevAttr.stWDRAttr.enWDRMode = WDR_MODE_2To1_LINE;
			stViDevAttr.stWDRAttr.u32CacheLine = ctx->replayParam.width;
			stViDevAttr.stWDRAttr.bSyntheticWDR = 0;
		}
		stViDevAttr.enInputDataType = VI_DATA_TYPE_RGB;
		stViDevAttr.enBayerFormat = (BAYER_FORMAT_E)ctx->replayParam.bayerFormat;
	} else {
		stViDevAttr.stSize.u32Width =ctx->replayParam.width;
		stViDevAttr.stSize.u32Height = ctx->replayParam.height;
		stViDevAttr.stWDRAttr.u32CacheLine = ctx->replayParam.width;
		stViDevAttr.enDataSeq = VI_DATA_SEQ_YUYV;
		stViDevAttr.enInputDataType = VI_DATA_TYPE_YUV;
		stViDevAttr.enIntfMode = VI_MODE_MIPI_YUV422;
	}
	stViDevAttr.snrFps = ctx->replayParam.frameRate;

	s32Ret = CVI_VI_SetDevAttr(0, &stViDevAttr);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("CVI_VI_SetDevAttr failed with %#x!\n", s32Ret);
		return s32Ret;
	}

	s32Ret = CVI_VI_EnableDev(0);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("CVI_VI_EnableDev failed with %#x!\n", s32Ret);
		return s32Ret;
	}

	return CVI_SUCCESS;
}

static CVI_S32 replay_set_mode_config(SERVICE_CTX *ctx)
{
	CVI_S32 s32Ret;
	VI_DEV_TIMING_ATTR_S stTimingAttr;

	stTimingAttr.bEnable = ctx->replayParam.timingEnable;
	stTimingAttr.s32FrmRate = ctx->replayParam.frameRate;

	s32Ret = CVI_VI_SetPipeFrameSource(0, VI_PIPE_FRAME_SOURCE_USER_FE);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VI_SetPipeFrameSource failed with %#x\n", s32Ret);
		return s32Ret;
	}

	s32Ret = CVI_VI_SetDevTimingAttr(0, &stTimingAttr);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VI_SetDevTimingAttr failed with %#x\n", s32Ret);
		return s32Ret;
	}

	return CVI_SUCCESS;
}

static CVI_S32 replay_trig_pic(SERVICE_CTX *ctx)
{
	VI_PIPE PipeId[] = {0};
	const VIDEO_FRAME_INFO_S *pstVideoFrame[1];
	VIDEO_FRAME_INFO_S stVideoFrame;
	VB_BLK blk_le = VB_INVALID_HANDLE;
	VB_BLK blk_se = VB_INVALID_HANDLE;
	CVI_U32 u32BlkSize = 0;
	CVI_U64 u64PhyAddr_le = 0, u64PhyAddr_se = 0;
	CVI_VOID *puVirAddr_le = NULL, *puVirAddr_se = NULL;

	memset( &stVideoFrame, 0, sizeof(VIDEO_FRAME_INFO_S));
	stVideoFrame.stVFrame.enBayerFormat = (BAYER_FORMAT_E)ctx->replayParam.bayerFormat;
	stVideoFrame.stVFrame.enPixelFormat = (PIXEL_FORMAT_E)ctx->replayParam.pixelFormat;
	stVideoFrame.stVFrame.enCompressMode = ctx->replayParam.compressMode;
	stVideoFrame.stVFrame.enDynamicRange = ctx->replayParam.WDRMode ? DYNAMIC_RANGE_HDR10 : DYNAMIC_RANGE_SDR10;
	stVideoFrame.stVFrame.u32Width = ctx->replayParam.width;
	stVideoFrame.stVFrame.u32Height = ctx->replayParam.height;
	stVideoFrame.stVFrame.s16OffsetLeft = 0;
	stVideoFrame.stVFrame.s16OffsetTop = 0;
	stVideoFrame.stVFrame.s16OffsetRight = 0;
	stVideoFrame.stVFrame.s16OffsetBottom = 0;

	if (!stVideoFrame.stVFrame.enPixelFormat) {
		u32BlkSize = VI_GetRawBufferSize(stVideoFrame.stVFrame.u32Width,
					 stVideoFrame.stVFrame.u32Height, PIXEL_FORMAT_RGB_BAYER_12BPP,
					 stVideoFrame.stVFrame.enCompressMode, DEFAULT_ALIGN, 0);
	} else {
		stVideoFrame.stVFrame.enDynamicRange = DYNAMIC_RANGE_SDR10;
		u32BlkSize = COMMON_GetPicBufferSize(stVideoFrame.stVFrame.u32Width,
					stVideoFrame.stVFrame.u32Height, PIXEL_FORMAT_YUYV,
					DATA_BITWIDTH_8, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
	}

	if (ctx->replayParam.timingEnable) {
		blk_le = CVI_VB_GetBlock(VB_INVALID_POOLID, u32BlkSize);
		if (blk_le == VB_INVALID_HANDLE) {
			SAMPLE_PRT("get blk_le failed\n");
			return CVI_FAILURE;
		}
		u64PhyAddr_le = CVI_VB_Handle2PhysAddr(blk_le);
		puVirAddr_le = CVI_SYS_MmapCache(u64PhyAddr_le, u32BlkSize);
		memset(puVirAddr_le, 0, u32BlkSize);
		CVI_SYS_Munmap(puVirAddr_le, u32BlkSize);

		if (stVideoFrame.stVFrame.enDynamicRange == DYNAMIC_RANGE_HDR10 || stVideoFrame.stVFrame.enPixelFormat) {
			blk_se = CVI_VB_GetBlock(VB_INVALID_POOLID, u32BlkSize);
			if (blk_se == VB_INVALID_HANDLE) {
				SAMPLE_PRT("get blk_se failed\n");
				return CVI_FAILURE;
			}
			u64PhyAddr_se = CVI_VB_Handle2PhysAddr(blk_se);
			puVirAddr_se = CVI_SYS_MmapCache(u64PhyAddr_se, u32BlkSize);
			memset(puVirAddr_se, 0, u32BlkSize);
			CVI_SYS_Munmap(puVirAddr_se, u32BlkSize);
		}
	}
	stVideoFrame.stVFrame.u64PhyAddr[0] = u64PhyAddr_le;
	stVideoFrame.stVFrame.u64PhyAddr[1] = u64PhyAddr_se;
	pstVideoFrame[0] =  &stVideoFrame;

	if (CVI_VI_SendPipeRaw(1, PipeId, pstVideoFrame, 0) != CVI_SUCCESS) {
		SAMPLE_PRT("Trig vsync failed\n");
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

static CVI_S32 replay_vi_init(SERVICE_CTX *ctx)
{
	VI_PIPE ViPipe = 0;
	VI_PIPE_ATTR_S stPipeAttr;
	CVI_S32 s32Ret = CVI_SUCCESS;

	s32Ret = replay_set_mode_config(ctx);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("replay_set_mode_config failed with %#x!\n", s32Ret);
		return s32Ret;
	}

	s32Ret = replay_trig_pic(ctx);
	if (s32Ret != CVI_SUCCESS && ctx->replayParam.timingEnable) {
		SAMPLE_PRT("replay_trig_pic failed with %#x!\n", s32Ret);
		return s32Ret;
	}

	s32Ret = replay_vi_start_dev(ctx);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("replay_vi_start_dev failed with %#x!\n", s32Ret);
		return s32Ret;
	}

	stPipeAttr.bYuvSkip = CVI_FALSE;
	stPipeAttr.u32MaxW = ctx->replayParam.width;
	stPipeAttr.u32MaxH = ctx->replayParam.height;
	stPipeAttr.enPixFmt = PIXEL_FORMAT_RGB_BAYER_12BPP;
	stPipeAttr.enBitWidth = DATA_BITWIDTH_12;
	stPipeAttr.stFrameRate.s32SrcFrameRate = -1;
	stPipeAttr.stFrameRate.s32DstFrameRate = -1;
	stPipeAttr.bNrEn = CVI_TRUE;
	stPipeAttr.enCompressMode = ctx->replayParam.compressMode;
	stPipeAttr.bYuvBypassPath = ctx->replayParam.pixelFormat ? 1 : 0;
	s32Ret = CVI_VI_CreatePipe(ViPipe, &stPipeAttr);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("CVI_VI_CreatePipe failed with %#x!\n", s32Ret);
		return s32Ret;
	}

	s32Ret = CVI_VI_StartPipe(ViPipe);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("CVI_VI_StartPipe failed with %#x!\n", s32Ret);
		return s32Ret;
	}

	s32Ret = CVI_VI_GetPipeAttr(ViPipe, &stPipeAttr);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("CVI_VI_GetPipeAttr failed with %#x!\n", s32Ret);
		return s32Ret;
	}

	s32Ret = replay_createIsp(ctx);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("VI_CreateIsp failed with %#x!\n", s32Ret);
		return s32Ret;
	}

	s32Ret = replay_startViChn(ctx);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("VI_StartViChn failed with %#x!\n", s32Ret);
		return s32Ret;
	}

	return CVI_SUCCESS;
}
