#include <signal.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <inttypes.h>
#include <nlohmann/json.hpp>
#include <cvi_rtsp_service/service.h>
#include <cvi_rtsp_service/venc_json.h>
#include <cvi_ae.h>
#include <cvi_awb.h>
#include <OnDemandServerMediaSubsession.hh>

#include "vpss_helper.h"
#include "venc_helper.h"
#include "json_helper.h"
#include "ai_helper.h"
#include "teaisppq_helper.h"
#include "isp_info_osd.h"
#include "cvi_media_procunit.h"

// #define DEBUG_RTSP /* create a task for debug using */
#define DEFAULT_RTSP_BITRATE 20480
#define _USE_GETFD_

static CVI_RTSP_CTX *gRtspCtx = NULL;
static const char *teaisppq_scene_str[5] = {
    "SNOW",
    "FOG",
    "BACKLIGHT",
    "GRASS",
    "COMMON"
};


static int check_ctx(SERVICE_CTX *ctx)
{
    if (ctx->vi_vpss_mode > 3) {
        std::cout << "unsupport vi-vpss-mode: " << ctx->vi_vpss_mode << std::endl;
        return -1;
    }

    if (ctx->stIniCfg.devNum <= 0) {
        std::cout << "sensor_cfg.ini setting devNum: " << ctx->stIniCfg.devNum << std::endl;
        return -1;
    }

    for (int idx=0; idx<ctx->dev_num; idx++)
    {
        SERVICE_CTX_ENTITY *ent = &ctx->entity[idx];
        if (ent->enableRetinaFace) {
            if (ent->bVencBindVpss) {
                std::cout << "enable-retinaface and venc-bind-vpss conflict" << std::endl;
                return -1;
            }

            if (0 == strlen(ctx->model_path)) {
                std::cout << "model is empty" << std::endl;
                return -1;
            }
        }
    }

    return 0;
}
static int default_ctx(SERVICE_CTX *ctx)
{
    memset(ctx, 0, sizeof(SERVICE_CTX));
    ctx->dev_num = 1;
    ctx->rtsp_num = 1;
    ctx->vi_vpss_mode = VI_OFFLINE_VPSS_ONLINE;
    ctx->buf1_blk_cnt = ctx->dev_num < 2? 0: 4;
    ctx->rtsp_max_buf_size = 1048576; // 1 MB
    snprintf(ctx->model_path, sizeof(ctx->model_path), "%s", "/mnt/data/cvi_models/retinaface_mnet0.25_342_608.cvimodel");
    ctx->enable_set_sensor_config = false;
    snprintf(ctx->sensor_config_path, sizeof(ctx->sensor_config_path), "%s", "/mnt/data/sensor_cfg.ini");
    ctx->isp_debug_lvl = 255;//not set debug level
    ctx->enable_set_pq_bin = false;
    // teaisppq
    pthread_mutex_init(&ctx->teaisppq_mutex, NULL);
    pthread_cond_init(&ctx->teaisppq_cond, NULL);
    ctx->teaisppq_turn_pipe = 0;
    ctx->max_use_tpu_num = 1;  // default use 1 tpu for ai motion in mars platform
    for (int i=0; i<SERVICE_CTX_ENTITY_MAX_NUM; i++) {
        SERVICE_CTX_ENTITY *ent = &ctx->entity[i];
        snprintf(ent->rtspURL, sizeof(ent->rtspURL), "%s", "stream");
        ent->rtsp_bitrate = DEFAULT_RTSP_BITRATE;
        ent->bVpssBinding = true;
        ent->bVencBindVpss = true;
        ent->bEnable3DNR = true;
        ent->bEnableLSC = true;
        ent->compress_mode = COMPRESS_MODE_TILE;
        ent->buf_blk_cnt = 4; // FIXME
        ent->dst_width = 1920; // FIXME
        ent->dst_height = 1080; // FIXME
        ent->vpssPrePTS = 0;
        ent->vencPrePTS = 0;
        init_venc_cfg(&ent->venc_cfg);
        ent->VencChn = -1;
        ent->ctx = (void *)ctx;
        pthread_mutex_init(&ent->mutex, NULL);
    }

    // Get config from ini if found.
    if (SAMPLE_COMM_VI_ParseIni(&ctx->stIniCfg) != CVI_SUCCESS) {
        printf("SAMPLE_COMM_VI_ParseIni Failed!\n");
        return -1;
    }

    return 0;
}

static int init_ctx(SERVICE_CTX *ctx, const nlohmann::json &params)
{
    default_ctx(ctx);
    load_json_config(ctx, params);
    return check_ctx(ctx);
}

static void rtsp_connect(const char *ip, void *arg)
{
    std::cout << "connect: " << ip << std::endl;
}

static void rtsp_disconnect(const char *ip, void *arg)
{
    std::cout << "disconnect: " << ip << std::endl;
}

static void update_face_ae(cvtdl_face_t *face, CVI_U32 width, CVI_U32 height, VI_PIPE ViPipe) {
    ISP_SMART_INFO_S stFaceInfo;
    memset(&stFaceInfo, 0, sizeof(ISP_SMART_INFO_S));
    stFaceInfo.stROI[0].bEnable = 1;
    if (face->size > 0) {
        float max_area = 0.0;
        size_t max_idx = 0;
        for (size_t i = 0; i < face->size; i++) {
            cvtdl_bbox_t bbox = face->info[i].bbox;
            float area = std::abs(bbox.x2 - bbox.x1) * std::abs(bbox.y2 - bbox.y1);
            if (area > max_area) {
                max_area = area;
                max_idx = i;
            }
        }
        cvtdl_bbox_t max_bbox = face->info[max_idx].bbox;
        stFaceInfo.stROI[0].u8Num = 1;
        stFaceInfo.stROI[0].u16PosX[0] = max_bbox.x1;
        stFaceInfo.stROI[0].u16PosY[0] = max_bbox.y1;
        stFaceInfo.stROI[0].u16Width[0] = std::abs(max_bbox.x2 - max_bbox.x1);
        stFaceInfo.stROI[0].u16Height[0] = std::abs(max_bbox.y2 - max_bbox.y1);
        stFaceInfo.stROI[0].u16FrameWidth = width;
        stFaceInfo.stROI[0].u16FrameHeight = height;
        // cout << "Face AE Set PosX: " << stFaceInfo.u16FDPosX[0] << ", PosY: " << stFaceInfo.u16FDPosY[0]
        //     << ", Width: " << stFaceInfo.u16FDWidth[0] << ", Height: " << stFaceInfo.u16FDHeight[0] << endl;
        if (getenv("AI_DEBUG")) {
            printf("CVI_ISP_SetFaceAeInfo, ViPipe:%d, u8FDNum:%d\n", ViPipe, stFaceInfo.stROI[0].u8Num);
        }
       CVI_ISP_SetSmartInfo(ViPipe, &stFaceInfo, 50);
    }
}

static int run_retinaface(SERVICE_CTX_ENTITY *ent, VIDEO_FRAME_INFO_S *output)
{
    cvtdl_face_t face = {};
    cvtdl_service_brush_t brush = {};
    brush.color.b = 53.f;
    brush.color.g = 208.f;
    brush.color.r = 217.f;
    brush.size = 4;

    VIDEO_FRAME_INFO_S preprocessFrame = {};

    if (CVI_VPSS_GetChnFrame(ent->VpssGrp, VPSS_CHN1, &preprocessFrame, -1) != CVI_SUCCESS) {
        printf("CVI_VPSS_GetChnFrame OD Preprocess Frame Failed!");
        return -1;
    }

    ent->ai_retinaface(ent->ai_handle, &preprocessFrame, &face);
    ent->rescale_fd(output, &face);
    if (getenv("AI_DEBUG")) {
        printf("detect %d faces\n", face.size);
    }

    if (face.size > 0) {
        ent->ai_face_draw_rect(ent->ai_service_handle, &face, output, true, brush);
    }

    if ((((SERVICE_CTX *)ent->ctx)->dev_num == 1) &&
        (((SERVICE_CTX *)ent->ctx)->sensor_number >= 2) &&
        (ent->ViChn == 0)) {
        // sensor0
        //         -> vpss
        // sensor1
        MMF_CHN_S stSrcChn;
        MMF_CHN_S stDestChn;
        stDestChn.enModId = CVI_ID_VPSS;
        stDestChn.s32DevId = 0;
        stDestChn.s32ChnId = 0;
        CVI_SYS_GetBindbyDest(&stDestChn, &stSrcChn);
        update_face_ae(&face, output->stVFrame.u32Width, output->stVFrame.u32Height, stSrcChn.s32ChnId);
    } else {
        // sensor0 -> vpss0
        // sensor1 -> vpss1
        update_face_ae(&face, output->stVFrame.u32Width, output->stVFrame.u32Height, ent->ViChn);
    }

    CVI_VPSS_ReleaseChnFrame(ent->VpssGrp, VPSS_CHN1, &preprocessFrame);

    return 0;
}

static void teaisppq_put_text(SERVICE_CTX_ENTITY *ent, VIDEO_FRAME_INFO_S *pstVideoFrame)
{
    std::string scene_text;

    // the scene text in yuv
    int pos_x, pos_y;
    float c_r, c_g, c_b;

    pos_x = pos_y = 50;
    c_r = 255;
    c_g = 0;
    c_b = 255;

    // get teaisppq scene
    TEAISP_PQ_ATTR_S stTEAISPPQAttr;
    TEAISP_PQ_SCENE_INFO scene_info;
    int scene_id;

    CVI_TEAISP_PQ_GetAttr(ent->ViPipe, &stTEAISPPQAttr);
    CVI_TEAISP_PQ_GetDetectSceneInfo(ent->ViPipe, &scene_info);

    if (stTEAISPPQAttr.TuningMode != 0) {
        scene_info.scene_score = 100;
        scene_id = (stTEAISPPQAttr.TuningMode -1) % TEAISP_SCENE_NUM;
        scene_text +=  std::string("Tuning SCENE: ") + std::string(teaisppq_scene_str[scene_id]);
        scene_text += ", score: 100";
    } else {
        scene_id = scene_info.scene;
        scene_text += std::string("Detect SCENE: ") + std::string(teaisppq_scene_str[scene_id]);
        scene_text += std::string(", score: ") + std::to_string(scene_info.scene_score);
    }

    if (stTEAISPPQAttr.Enable && stTEAISPPQAttr.SceneBypass[scene_id] == 0 \
            && (scene_info.scene_score >= stTEAISPPQAttr.SceneConfThres[scene_id])) {
        scene_text += " (accpet)";
    } else {
        scene_text += " (bypass)";
        if (!stTEAISPPQAttr.Enable)
            scene_text += "[disable]";
        else if (stTEAISPPQAttr.SceneBypass[scene_id] != 0)
            scene_text += "[scene bypass]";
        else
            scene_text += "[Low score]";
    }

    ent->ai_write_text((char *)scene_text.c_str(), pos_x, pos_y, pstVideoFrame, c_r, c_g, c_b);
}

static void *rtspTask(void *arg)
{
    int rc = -1;
    SERVICE_CTX_ENTITY *ent = (SERVICE_CTX_ENTITY *)arg;
#if defined(_USE_GETFD_)
    int vencFd = -1;
    struct timeval timeoutVal;
    fd_set readFds;
#endif
    while (ent->running) {
        pthread_mutex_lock(&ent->mutex);

        if (ent->VencChn == -1) {
            pthread_mutex_unlock(&ent->mutex);
            usleep(10000); // sleep 10ms
            continue;
        }
        VENC_CHN VencChn = ent->VencChn;
        VENC_CHN_STATUS_S stStat = {};
        VENC_STREAM_S stStream = {};
        VIDEO_FRAME_INFO_S stVideoFrame = {};

        if (!ent->bVencBindVpss || ent->enableRetinaFace) {
            if (0 != (rc = CVI_VPSS_GetChnFrame(ent->VpssGrp, ent->VpssChn, &stVideoFrame, 3000))) {
                printf("CVI_VPSS_GetChnFrame Grp: %u Chn: %u failed with %#x\n", ent->VpssGrp, ent->VpssChn, rc);
                pthread_mutex_unlock(&ent->mutex);
                continue;
            }

            if (getenv("VPSS_DEBUG")) {
                uint64_t vpss_pts_diff = (stVideoFrame.stVFrame.u64PTS - ent->vpssPrePTS);
                printf("VPSS PTS: %llu, prePTS: %" PRIu64 ", Diff: %" PRIu64 "\n",
                stVideoFrame.stVFrame.u64PTS, ent->vpssPrePTS, vpss_pts_diff);
                ent->vpssPrePTS = stVideoFrame.stVFrame.u64PTS;
            }

            if (ent->enableRetinaFace) {
                rc = run_retinaface(ent, &stVideoFrame);
                if (0 != rc) {
                    std::cout << "fail to handle retinaface" << std::endl;
                    pthread_mutex_unlock(&ent->mutex);
                    continue;
                }
            }

            if (ent->enableTeaisppq) {
                if (access("teaisppq_put_text", F_OK) == 0) {
                    teaisppq_put_text(ent, &stVideoFrame);
                }
            }

            CVI_S32 s32SetFrameMilliSec = 20000;
            if (0 != (rc = CVI_VENC_SendFrame(VencChn, &stVideoFrame, s32SetFrameMilliSec))) {
                printf("CVI_VENC_SendFrame failed! %d\n", rc);
                pthread_mutex_unlock(&ent->mutex);
                continue;
            }
        }
#if defined(_USE_GETFD_)

        if (vencFd < 0) {
            vencFd = CVI_VENC_GetFd(VencChn);
            if (vencFd < 0) {
                printf("CVI_VENC_GetFd failed with%#x!\n", vencFd);
                break;
            }
        }
        FD_ZERO(&readFds);
        FD_SET(vencFd, &readFds);
        timeoutVal.tv_sec = 2;
        timeoutVal.tv_usec = 0;
        rc = select(vencFd + 1, &readFds, NULL, NULL, &timeoutVal);
        if (rc < 0) {
            if (errno == EINTR) {
                pthread_mutex_unlock(&ent->mutex);
                continue;
            }
            printf("select failed!\n");
            pthread_mutex_unlock(&ent->mutex);
            break;
        } else if (rc == 0) {
            printf("select timeout 1\n");
            pthread_mutex_unlock(&ent->mutex);
        }
#endif
        if (0 != (rc = CVI_VENC_QueryStatus(VencChn, &stStat))) {
            printf("CVI_VENC_QueryStatus failed with %#x!\n", rc);
            pthread_mutex_unlock(&ent->mutex);
            continue;
        }

        if (!stStat.u32CurPacks) {
            printf("NOTE: Current frame is NULL!\n");
            pthread_mutex_unlock(&ent->mutex);
            continue;
        }

        stStream.pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
        if (stStream.pstPack == NULL) {
            printf("malloc memory failed!\n");
            pthread_mutex_unlock(&ent->mutex);
            continue;
        }
        if (0 != (rc = CVI_VENC_GetStream(VencChn, &stStream, 2000))) {
            printf("CVI_VENC_GetStream failed with %#x!\n", rc);
            free(stStream.pstPack);
            stStream.pstPack = NULL;
            usleep(40 * 1000);
            pthread_mutex_unlock(&ent->mutex);
            continue;
        }
        if (!ent->bVencBindVpss) {
            if (0 != CVI_VPSS_ReleaseChnFrame(ent->VpssGrp, ent->VpssChn, &stVideoFrame)) {
                printf("CVI_VPSS_ReleaseChnFrame Grp: %u Chn: %u NG\n", ent->VpssGrp, ent->VpssChn);
            }
        }
        if (getenv("VENC_DEBUG")) {
            uint64_t venc_pts_diff = (stStream.pstPack[0].u64PTS - ent->vencPrePTS);
            ent->vencPrePTS = stStream.pstPack[0].u64PTS;
            printf("VENC pts: %llu, prePTS: %" PRIu64 ", duration: %" PRIu64 "\n", stStream.pstPack[0].u64PTS, ent->vencPrePTS, venc_pts_diff);
        }

        CVI_MEDIA_ListPushBack(VencChn, &stStream);

        if (0 != (rc = CVI_VENC_ReleaseStream(VencChn, &stStream))) {
            printf("CVI_VENC_ReleaseStream, s32Ret = %d\n", rc);
            free(stStream.pstPack);
            stStream.pstPack = NULL;
            pthread_mutex_unlock(&ent->mutex);
            continue;
        }

        free(stStream.pstPack);
        stStream.pstPack = NULL;
        pthread_mutex_unlock(&ent->mutex);
    }

    return NULL;
}

static int init(SERVICE_CTX *ctx, const nlohmann::json &params)
{
    if (0 > init_ctx(ctx, params)) {
        std::cout << "init ctx fail" << std::endl;
        return -1;
    }

    if (0 > init_vi(ctx)) {
        std::cout << "init vi fail" << std::endl;
        return -1;
    }

    if (0 > init_vpss(ctx)) {
        std::cout << "init vpss fail" << std::endl;
        return -1;
    }

    if (0 > isp_info_osd_start(ctx)) {
        std::cout << "isp_osd_start fail" << std::endl;
        return -1;
    }

    if (0 > init_ai(ctx)) {
        std::cout << "init ai fail" << std::endl;
        return -1;
    }

    if (0 > init_teaisppq(ctx)) {
        std::cout << "init teaisppq fail" << std::endl;
        return -1;
    }

    if (0 > init_teaisp_bnr(ctx)) {
        std::cout << "init teaisp bnr fail" << std::endl;
        return -1;
    }

    return 0;
}

static void deinit(SERVICE_CTX *ctx)
{
    isp_info_osd_stop(ctx);

    deinit_ai(ctx);

    deinit_teaisp_bnr(ctx);

    deinit_teaisppq(ctx);

    for (int idx=0; idx<ctx->rtsp_num; idx++) {
        deinit_venc(&(ctx->entity[idx]));
    }
    deinit_vi(ctx);
}

static void rtsp_play(int references, void *arg)
{
    SERVICE_CTX_ENTITY *ent =(SERVICE_CTX_ENTITY *)arg;

    pthread_mutex_lock(&ent->mutex);
    CVI_RTSP_SERVICE_SetVencCfg(ent->venc_json, &ent->venc_cfg);

    const char *venc_debug = getenv("RTSP_VENC_DEBUG");
    if (venc_debug && 0 == strcmp(venc_debug, "1")) {
        dump_venc_cfg(&ent->venc_cfg);
    }
    if (0 > init_venc(ent)) {
        ent->VencChn = -1;
        ent->venc_cfg.bCreateChn = 0;
        std::cout << "init venc chn fail" << std::endl;
    }

    pthread_mutex_unlock(&ent->mutex);
}

static void rtsp_teardown(int references, void *arg)
{
    SERVICE_CTX_ENTITY *ent =(SERVICE_CTX_ENTITY *)arg;

    pthread_mutex_lock(&ent->mutex);
    deinit_venc(ent);
    pthread_mutex_unlock(&ent->mutex);
}

#ifdef DEBUG_RTSP
#define DEBUG_PATH "/usr/local/etc/vichn"
static bool gDbgTaskRun = false;
static pthread_t gDbgTask;
static int curViChn = 0;

static void dbg_init_vichn(int ViChn) {
    MMF_CHN_S stSrcChn, stDstChn;
    FILE *fp = NULL;

    fp = fopen(DEBUG_PATH, "w");
    if (!fp) return;

    stDstChn.enModId = CVI_ID_VPSS;
    stDstChn.s32DevId = 0;
    stDstChn.s32DevId = 0;
    if (CVI_SUCCESS != CVI_SYS_GetBindbyDest(&stDstChn, &stSrcChn)) {
        printf("[Error] Get Bind by src fail\n");
        fclose(fp);
        return;
    }

    fprintf(fp, "%d", stSrcChn.s32ChnId);
    fclose(fp);
}

static void dbg_set_vichn(int ViChn) {
    if (curViChn != ViChn) {
        MMF_CHN_S stSrcChn, stDstChn;

        stDstChn.enModId = CVI_ID_VPSS;
        stDstChn.s32DevId = 0;
        stDstChn.s32DevId = 0;
        if (CVI_SUCCESS != CVI_SYS_GetBindbyDest(&stDstChn, &stSrcChn)) {
            printf("[Error] Get Bind by src fail\n");
        }

        if (CVI_SUCCESS != CVI_SYS_UnBind(&stSrcChn, &stDstChn)) {
            printf("[Error] UnBind VI(%u, %u)-VPSS(%u, %u) fail\n", 0, curViChn, 0, 0);
        }

        stSrcChn.enModId = CVI_ID_VI;
        stSrcChn.s32DevId = 0;
        stSrcChn.s32ChnId = ViChn;

        if (CVI_SUCCESS != CVI_SYS_Bind(&stSrcChn, &stDstChn)) {
            printf("[Error] Bind VI(%u, %u)-VPSS(%u, %u) fail\n", 0, ViChn, 0, 0);
        }

        curViChn = ViChn;
    }
}

static void *dbg_task(void *arg)
{
    FILE *fp = NULL;
    while(gDbgTaskRun) {
        int newVal = 0;
        fp = fopen(DEBUG_PATH, "r");
        if (fp == NULL) {
            usleep(1000*1000);
            continue;
        }
        fscanf(fp, "%d", &newVal);
        if (newVal>=0 && newVal<=1) {
            dbg_set_vichn(newVal);
        }
        fclose(fp);
        fp = NULL;
        usleep(1000*1000);
    }

    return NULL;
}
#endif // DEBUG_RTSP

static CVI_S32 _meida_malloc(void **dst,void *src)
{

    if(!dst || !src) {
        return CVI_FAILURE;
    }
    CVI_U32 i = 0;
    VENC_STREAM_S *pdst = NULL;
    VENC_STREAM_S *psrc = (VENC_STREAM_S *) src;
    pdst = (VENC_STREAM_S *)malloc(sizeof(VENC_STREAM_S));
    if(pdst == NULL) {
        return CVI_FAILURE;
    }
    memcpy(pdst,psrc,sizeof(VENC_STREAM_S));
    pdst->pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S) * psrc->u32PackCount);
    if(!pdst->pstPack) {
        return CVI_FAILURE;
    }
    memset(pdst->pstPack,0,sizeof(VENC_PACK_S) * psrc->u32PackCount);
    for(i = 0;i < psrc->u32PackCount; i++) {
        memcpy(&pdst->pstPack[i],&psrc->pstPack[i],sizeof(VENC_PACK_S));
        pdst->pstPack[i].pu8Addr = (CVI_U8 *)malloc(psrc->pstPack[i].u32Len);
        if(!pdst->pstPack[i].pu8Addr) {
            goto exit;
        }
        memcpy(pdst->pstPack[i].pu8Addr,psrc->pstPack[i].pu8Addr,psrc->pstPack[i].u32Len);
    }
    *dst = (void *) pdst;
    return CVI_SUCCESS;
exit:
    if(pdst->pstPack) {
        for(i = 0;i < psrc->u32PackCount; i++) {
            if(pdst->pstPack[i].pu8Addr) {
                free(pdst->pstPack[i].pu8Addr);
            }
        }
        free(pdst->pstPack);
    }
    return CVI_FAILURE;
}

static CVI_S32 _media_relase(void **src)
{

    VENC_STREAM_S * psrc = NULL;

    if(src == NULL) {
        return CVI_FAILURE;
    }
    psrc = (VENC_STREAM_S *) *src;
    if(!psrc) {
        return CVI_FAILURE;
    }
    if(psrc->pstPack) {
        for(CVI_U32 i = 0;i < psrc->u32PackCount; i++) {
            if(psrc->pstPack[i].pu8Addr) {
                free(psrc->pstPack[i].pu8Addr);
            }
        }
        free(psrc->pstPack);
    }
    if(psrc) {
        free(psrc);
    }
    return CVI_SUCCESS;
}

CVI_S32 CVI_IPC_SendToRtsp(VENC_CHN vencChn, VENC_STREAM_S *pstStream, void *param)
{
    CVI_S32 ret = CVI_SUCCESS;
    VENC_PACK_S *ppack;
    CVI_RTSP_DATA data;
    SERVICE_CTX *pstService = (SERVICE_CTX *)param;

    memset(&data, 0, sizeof(CVI_RTSP_DATA));
    data.blockCnt = pstStream->u32PackCount;
    for (CVI_U32 i = 0; i < pstStream->u32PackCount; i++) {
        ppack = &pstStream->pstPack[i];
        data.dataPtr[i] = ppack->pu8Addr + ppack->u32Offset;
        data.dataLen[i] = ppack->u32Len - ppack->u32Offset;
    }

    ret = CVI_RTSP_WriteFrame(pstService->rtspCtx, pstService->entity[vencChn].rtspSession->video, &data);

    if (ret != CVI_SUCCESS) {
        SAMPLE_PRT("CVI_RTSP_WriteFrame failed\n");
    }
    return ret;
}

void CVI_IPC_MediaHander(int venc,void *parm,void *pstream)
{
    CVI_IPC_SendToRtsp(venc, (VENC_STREAM_S *)pstream, parm);
}

static int giRtspRef = 0;
static int service_create(RTSP_SERVICE_HANDLE *hdl, const nlohmann::json &params)
{
    SERVICE_CTX *ctx =(SERVICE_CTX *)malloc(sizeof(SERVICE_CTX));

    if (0 > init(ctx, params)) {
        free(ctx);
        return -1;
    }

    CVI_RTSP_CONFIG config = {0};
    config.port = ctx->rtspPort;

    if (!gRtspCtx) {
        if (0 > CVI_RTSP_Create(&gRtspCtx, &config)) {
            std::cout << "fail to create rtsp ctx" << std::endl;
            free(ctx);
            return -1;
        }
    }
    ctx->rtspCtx = gRtspCtx;

    for (int idx=0; idx<ctx->rtsp_num; idx++) {
        SERVICE_CTX_ENTITY *ent = &ctx->entity[idx];
        CVI_RTSP_SESSION_ATTR attr = {0};
        attr.video.codec = 0 == strcmp(ent->venc_cfg.codec, "264")? RTSP_VIDEO_H264: RTSP_VIDEO_H265;
        attr.video.bitrate = ent->rtsp_bitrate;

        snprintf(attr.name, sizeof(attr.name), "%s%d", ent->rtspURL, ent->ViChn);

        attr.video.play = rtsp_play;
        attr.video.playArg = (void *)ent;
        attr.video.teardown = rtsp_teardown;
        attr.video.teardownArg = (void *)ent;


        if (0 > CVI_RTSP_CreateSession(ctx->rtspCtx, &attr, &(ent->rtspSession))) {
            std::cout << "fail to create rtsp session" << std::endl;
            free(ctx);
            return -1;
        }
    }

    CVI_MEDIA_ProcUnitInit(ctx->rtsp_num,ctx,&CVI_IPC_MediaHander,&_meida_malloc,&_media_relase);

    OutPacketBuffer::maxSize = ctx->rtsp_max_buf_size;

    // set listener
    CVI_RTSP_STATE_LISTENER listener = {0};
    listener.onConnect = rtsp_connect;
    listener.onDisconnect = rtsp_disconnect;

    CVI_RTSP_SetListener(ctx->rtspCtx, &listener);

    if (giRtspRef==0){
        if (0 > CVI_RTSP_Start(ctx->rtspCtx)) {
            std::cout << "fail to start rtsp" << std::endl;
            free(ctx);
            return -1;
        }
    }
    giRtspRef++;

    for (int idx=0; idx<ctx->dev_num; idx++) {
        SERVICE_CTX_ENTITY *ent = &ctx->entity[idx];
        ent->running = true;
        pthread_create(&ent->worker, NULL, rtspTask, (void *)ent);
        pthread_create(&ent->teaisppq_worker, NULL, teaisppqTask, (void *)ent);
    }

#ifdef DEBUG_RTSP
    /* debug */
    dbg_init_vichn(ctx->entity[0].ViChn);
    gDbgTaskRun = true;
    pthread_create(&gDbgTask, NULL, dbg_task, NULL);
#endif

    *hdl = (void *)ctx;

    return 0;
}

int CVI_RTSP_SERVICE_CreateFromJsonFile(RTSP_SERVICE_HANDLE *hdl, const char *jsonFile)
{
    nlohmann::json params;
    if (jsonFile != nullptr && 0 > json_parse_from_file(jsonFile, params)) {
        return -1;
    }

    return service_create(hdl, params);
}

int CVI_RTSP_SERVICE_CreateFromJsonStr(RTSP_SERVICE_HANDLE *hdl, const char *jsonStr)
{
    nlohmann::json params;

    if (jsonStr != nullptr && 0 > json_parse_from_string(jsonStr, params)) {
        return -1;
    }

    return service_create(hdl, params);
}

int CVI_RTSP_SERVICE_Destroy(RTSP_SERVICE_HANDLE *hdl)
{
    SERVICE_CTX *ctx = (SERVICE_CTX *)(*hdl);

    for (int idx=0; idx<ctx->dev_num; idx++) {
        SERVICE_CTX_ENTITY *ent = &ctx->entity[idx];
        ent->running = false;
        pthread_join(ent->worker, NULL);
        pthread_join(ent->teaisppq_worker, NULL);
    }

    CVI_MEDIA_ProcUnitDeInit();

    for (int idx=0; idx<ctx->dev_num; idx++) {
        SERVICE_CTX_ENTITY *ent = &ctx->entity[idx];
        if (idx < ctx->rtsp_num)
            CVI_RTSP_DestroySession(ctx->rtspCtx, ent->rtspSession);
    }

#ifdef DEBUG_RTSP
    /* debug */
    gDbgTaskRun = false;
    pthread_join(gDbgTask, NULL);
#endif

    deinit(ctx);

    if (giRtspRef==1) {
        CVI_RTSP_Stop(ctx->rtspCtx);
        CVI_RTSP_Destroy(&ctx->rtspCtx);
        gRtspCtx = NULL;
    } else {
        giRtspRef--;
    }

    free(ctx);
    *hdl = nullptr;

    return 0;
}

static int service_hup(RTSP_SERVICE_HANDLE hdl, const nlohmann::json &params)
{
#if 0
    SERVICE_CTX *ctx = (SERVICE_CTX *)(hdl);

    ctx->running = false;
    pthread_join(ctx->worker, NULL);

    CVI_RTSP_Stop(ctx->rtspCtx);

    CVI_RTSP_DestroySession(ctx->rtspCtx, ctx->rtspSession);

    CVI_RTSP_Destroy(&ctx->rtspCtx);

    deinit(ctx);

    usleep(500000);

    if(0 > init(ctx, params)) {
        return -1;
    }

    CVI_RTSP_CONFIG config = {0};
    config.port = 554;

    if (!gRtspCtx) {
        if (0 > CVI_RTSP_Create(&gRtspCtx, &config)) {
            std::cout << "fail to create rtsp ctx" << std::endl;
            free(ctx);
            return -1;
        }
     }
    ctx->rtspCtx = gRtspCtx;

    CVI_RTSP_SESSION_ATTR attr = {0};
    attr.video.codec = 0 == strcmp(ctx->venc_cfg.codec, "264")? RTSP_VIDEO_H264: RTSP_VIDEO_H265;

    snprintf(attr.name, sizeof(attr.name), "%s%d", ctx->rtspURL, ctx->ViChn);

    CVI_RTSP_CreateSession(ctx->rtspCtx, &attr, &(ctx->rtspSession));

    // set listener
    CVI_RTSP_STATE_LISTENER listener = {0};
    listener.onConnect = rtsp_connect;
    listener.onDisconnect = rtsp_disconnect;

    CVI_RTSP_SetListener(ctx->rtspCtx, &listener);

    if (0 > CVI_RTSP_Start(ctx->rtspCtx)) {
        std::cout << "fail to start" << std::endl;
        return -1;
    }

    ctx->running = true;

    pthread_create(&ctx->worker, NULL, rtspTask, (void *)ctx);
#endif

    return 0;
}

int CVI_RTSP_SERVICE_HupFromJsonFile(RTSP_SERVICE_HANDLE hdl, const char *jsonFile)
{
    nlohmann::json params;
    if (jsonFile != nullptr && 0 > json_parse_from_file(jsonFile, params)) {
        return -1;
    }

    return service_hup(hdl, params);
}

int CVI_RTSP_SERVICE_HupFromJsonStr(RTSP_SERVICE_HANDLE hdl, const char *jsonStr)
{
    nlohmann::json params;
    if (jsonStr != nullptr && 0 > json_parse_from_string(jsonStr, params)) {
        return -1;
    }

    return service_hup(hdl, params);
}
