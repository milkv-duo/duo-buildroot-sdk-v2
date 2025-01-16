#pragma once

#include <iostream>
#include <pthread.h>
#include <sample_comm.h>
#include <cvi_venc.h>
#include <cvi_tdl.h>
#include <cvi_rtsp/rtsp.h>

typedef CVI_S32 (*FD_RescaleFunc)(const VIDEO_FRAME_INFO_S *frame, cvtdl_face_t *face);
typedef int (*AI_CreateHandle2)(void *, const int, const uint8_t);
typedef int (*AI_DestroyHandle)(void *);
typedef int (*AI_Service_CreateHandle)(void *, void *);
typedef int (*AI_Service_DestroyHandle)(void *);
typedef void (*AI_SetSkipVpssPreprocess)(void *, CVI_TDL_SUPPORTED_MODEL_E, bool);
typedef int (*AI_OpenModel)(void *, CVI_TDL_SUPPORTED_MODEL_E , const char *);
typedef int (*AI_GetVpssChnConfig)(void *, CVI_TDL_SUPPORTED_MODEL_E, const uint32_t, const uint32_t, const uint32_t i, cvtdl_vpssconfig_t *);
typedef int (*AI_RetinaFace)(const void *, VIDEO_FRAME_INFO_S *, cvtdl_face_t *);
typedef int (*AI_Service_FaceDrawRect)(void *, cvtdl_face_t *, VIDEO_FRAME_INFO_S *, const bool, cvtdl_service_brush_t);

struct SERVICE_CTX_ENTITY {
    pthread_t worker;
    bool running;
    pthread_mutex_t mutex;

    CVI_RTSP_SESSION *rtspSession;
    uint32_t rtsp_bitrate;
    char rtspURL[32];

    bool bVpssBinding;
    bool bVencBindVpss;
    bool bEnable3DNR;
    bool bEnableLSC;
    bool enableIspInfoOsd;

    int src_width;
    int src_height;
    int dst_width;
    int dst_height;

    uint32_t buf_blk_cnt;

    uint64_t vpssPrePTS;
    uint64_t vencPrePTS;

    COMPRESS_MODE_E compress_mode;
    VI_CHN ViChn;
    VI_PIPE ViPipe;
    VPSS_CHN VpssChn;
    VPSS_GRP VpssGrp;
    VENC_CHN VencChn;
    chnInputCfg venc_cfg;
    char venc_json[128];
    void *ctx;
    bool enableRetinaFace;
    cvtdl_vpssconfig_t retinaVpssConfig;
    cvitdl_handle_t ai_handle;
    cvitdl_service_handle_t ai_service_handle;
    FD_RescaleFunc rescale_fd;
    AI_CreateHandle2 ai_create_handle2;
    AI_DestroyHandle ai_destroy_handle;
    AI_Service_CreateHandle ai_service_create_handle;
    AI_Service_DestroyHandle ai_service_destroy_handle;
    AI_SetSkipVpssPreprocess ai_set_skip_vpss_preprocess;
    AI_OpenModel ai_open_model;
    AI_GetVpssChnConfig ai_get_vpss_chn_config;
    AI_RetinaFace ai_retinaface;
    AI_Service_FaceDrawRect ai_face_draw_rect;
};

#define SERVICE_CTX_ENTITY_MAX_NUM 2

struct SERVICE_SBM {
	uint8_t enable;
	uint16_t bufLine;
	uint8_t bufSize;
};

struct REPLAY_PARAM {
	uint8_t pixelFormat;
	uint16_t width;
	uint16_t height;
	bool timingEnable;
	uint32_t frameRate;
	uint8_t WDRMode;
	uint8_t bayerFormat;
	COMPRESS_MODE_E compressMode;
};

struct SERVICE_CTX {
    SERVICE_CTX_ENTITY entity[SERVICE_CTX_ENTITY_MAX_NUM];
    CVI_RTSP_CTX *rtspCtx;
    int rtspPort;
    uint32_t rtsp_max_buf_size;
    SAMPLE_VI_CONFIG_S stViConfig;
    SAMPLE_INI_CFG_S stIniCfg;
    uint8_t rtsp_num;
    uint8_t dev_num;
    VI_VPSS_MODE_E vi_vpss_mode;
    uint32_t buf1_blk_cnt;
    char model_path[128];
    void *ai_dl;
	SERVICE_SBM sbm;
    /*isp*/
    bool enable_set_sensor_config;
    char sensor_config_path[128];
    uint8_t isp_debug_lvl;
    bool enable_set_pq_bin;
    char sdr_pq_bin_path[128];
    char wdr_pq_bin_path[128];
    bool replayMode;
    REPLAY_PARAM replayParam;
};
