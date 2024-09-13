#include <unistd.h>
#include <algorithm>
#include <sample_comm.h>
#if MW_VER == 2
#include <cvi_comm_video.h>
#endif
#include <cvi_isp.h>
#include <cvi_ae.h>
#include <cvi_sys.h>
#include <cvi_buffer.h>
#include <cvi_bin.h>
#include "vpss_helper.h"

int vpss_init_helper(uint32_t vpssGrpId, uint32_t enSrcWidth, uint32_t enSrcHeight,
        PIXEL_FORMAT_E enSrcFormat, uint32_t enDstWidth, uint32_t enDstHeight,
        PIXEL_FORMAT_E enDstFormat, uint32_t enabledChannel,
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

#if MW_VER == 2
#if defined(__CV181X__) || defined(__CV180X__)
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
#endif
#endif

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

#if MW_VER == 1
    if (ctx->enable_set_pq_bin) {
        // read wdr mode from sensor_cfg.ini
        if (ctx->stIniCfg.enWDRMode <= WDR_MODE_QUDRA) {
            CVI_BIN_SetBinName(ctx->stIniCfg.enWDRMode, ctx->sdr_pq_bin_path);
            printf("SdrMode[%d] Set SdrPqBin[%s] done",
                ctx->stIniCfg.enWDRMode, ctx->sdr_pq_bin_path);
        } else {
            CVI_BIN_SetBinName(ctx->stIniCfg.enWDRMode, ctx->wdr_pq_bin_path);
            printf("WdrMode[%d] Set WdrPqBin[%s] done",
                ctx->stIniCfg.enWDRMode, ctx->wdr_pq_bin_path);
        }
    }
#endif

    CVI_VI_SetDevNum(ctx->dev_num);

#if MW_VER == 1
    //change vi_vpss_mode by check runTime status
    if (((ctx->stIniCfg.devNum >= 2) || (ctx->stIniCfg.enWDRMode > WDR_MODE_QUDRA)) &&
        ((ctx->vi_vpss_mode == VI_ONLINE_VPSS_OFFLINE) || (ctx->vi_vpss_mode == VI_ONLINE_VPSS_ONLINE))) {
        printf("When it's wdr mode or dual sensor, we can't set vi online mode\n");
        printf("preViVpssMode:%d curViVpssMode:%d\n", ctx->vi_vpss_mode, ctx->vi_vpss_mode - 2);
        ctx->vi_vpss_mode = (VI_VPSS_MODE_E)((int)(ctx->vi_vpss_mode) - 2);
    }
#endif

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

    int teaisp_bnr_enable_cnt = 0;

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

        if (ent->enableTEAISPBnr) {
            teaisp_bnr_enable_cnt++;
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

        ent->src_width = stSize.u32Width;
        ent->src_height = stSize.u32Height;
        ent->dst_width = ent->src_width;
        ent->dst_height = ent->src_height;

        stVbConf.u32MaxPoolCnt++;
        u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, VI_PIXEL_FORMAT, DATA_BITWIDTH_8,
                COMPRESS_MODE_NONE, DEFAULT_ALIGN);
        u32BlkRotSize = COMMON_GetPicBufferSize(stSize.u32Height, stSize.u32Width, VI_PIXEL_FORMAT,
                DATA_BITWIDTH_8, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
        u32BlkSize = std::max(u32BlkSize, u32BlkRotSize);
        stVbConf.astCommPool[i].u32BlkSize = u32BlkSize;
        stVbConf.astCommPool[i].u32BlkCnt = ent->buf_blk_cnt;
        stVbConf.astCommPool[i].enRemapMode = VB_REMAP_MODE_CACHED;
    }

    for (int j = 0; j < ctx->dev_num; j++) {
        if (teaisp_bnr_enable_cnt > 1) {
            pstViCfg->astViInfo[j].stPipeInfo.u32TEAISPMode = TEAISP_AFTER_FE_RAW_MODE;
        } else if (teaisp_bnr_enable_cnt == 1) {
            pstViCfg->astViInfo[j].stPipeInfo.u32TEAISPMode = TEAISP_BEFORE_FE_RAW_MODE;
        }
    }

    for (int k=0; k<ctx->dev_num; ++k) {
#if MW_VER == 1
        if (!ctx->entity[k].enableRetinaFace) continue;
#endif
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

        printf("enable online mode %d\n", ctx->vi_vpss_mode);
        //cv183x does not support different mode Settings for dual sensor
        stVIVPSSMode.aenMode[0] = stVIVPSSMode.aenMode[1] = ctx->vi_vpss_mode;

        s32Ret = CVI_SYS_SetVIVPSSMode(&stVIVPSSMode);
        if (s32Ret != CVI_SUCCESS) {
            printf("CVI_SYS_SetVIVPSSMode failed with %#x\n", s32Ret);
            return s32Ret;
        }


        if (s32Ret != CVI_SUCCESS) {
            printf("CVI_SYS_SetVPSSModeEx failed with %#x\n", s32Ret);
            return -1;
        }
     } else {


        if (s32Ret != CVI_SUCCESS) {
            printf("CVI_SYS_SetVPSSModeEx failed with %#x\n", s32Ret);
            return -1;
        }
    }

    s32Ret = SAMPLE_PLAT_VI_INIT(pstViCfg);
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
                        ent->enableRetinaFace?PIXEL_FORMAT_YUV_PLANAR_420:SAMPLE_PIXEL_FORMAT,  1, false, 1, ctx->sbm);
                CVI_VPSS_SetGrpParamfromBin(ent->VpssGrp, ent->VpssChn);
                if (ctx->vi_vpss_mode == VI_OFFLINE_VPSS_OFFLINE) {
#if MW_VER == 2
                    s32Ret = SAMPLE_COMM_VI_Bind_VPSS(0, ent->ViChn, ent->VpssGrp);
#else
                    s32Ret = SAMPLE_COMM_VI_Bind_VPSS(ent->ViChn, ent->VpssGrp);
#endif
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
#if MW_VER == 2
                MMF_CHN_S stDestChn, stSrcChn;
                stDestChn.enModId = CVI_ID_VPSS;
                stDestChn.s32DevId = ent->VpssGrp;
                stDestChn.s32ChnId = 0;

                CVI_SYS_GetBindbyDest(&stDestChn, &stSrcChn);
                SAMPLE_COMM_VI_UnBind_VPSS(0, stSrcChn.s32ChnId, ent->VpssGrp);
#else
                SAMPLE_COMM_VI_UnBind_VPSS(ent->ViChn, ent->VpssGrp);
#endif
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
