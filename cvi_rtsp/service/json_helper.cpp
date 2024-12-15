#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include "ctx.h"
#include <cvi_rtsp_service/venc_json.h>

#define SET_CTX(config, key, val) if (config.contains(key)) {val = config.at(key);}
#define SET_CTX_STR(config, key, val) if (config.contains(key)) {std::string file = params.at(key); snprintf(val, sizeof(val), "%s", file.c_str());}
#define GET_DATA(config, key, data) if (config.contains(key)) {data = config.at(key);}


static void dump_json(const nlohmann::json &params)
{
    for (auto &j : params.items()) {
        std::cout << j.key() << " : " << j.value() << std::endl;
    }
}

int json_parse_from_file(const char *jsonFile, nlohmann::json &params)
{
    if (jsonFile == nullptr) {
        return -1;
    }

    std::ifstream file;
    file.open(jsonFile);
    if (!file.is_open()) {
        std::cout << "Open Json file: " << jsonFile << " failed" << std::endl;
        return -1;
    }

    try {
        file >> params;
    } catch (...) {
        std::cout << "fail to parse config: " << jsonFile << std::endl;
        file.close();
        return -1;
    }

    file.close();

    dump_json(params);

    return 0;
}

int json_parse_from_string(const char *jsonStr, nlohmann::json &params)
{
    if (jsonStr == nullptr) {
        return -1;
    }

    try {
        params = nlohmann::json::parse(jsonStr);
    } catch (...) {
        std::cout << "fail to parse json string: " << jsonStr << std::endl;
        return -1;
    }

    dump_json(params);

    return 0;
}

static COMPRESS_MODE_E get_compress_mode(const std::string &mode)
{
    if (mode == "none") {
        return COMPRESS_MODE_NONE;
    } else if (mode == "tile") {
        return COMPRESS_MODE_TILE;
    } else if (mode == "line") {
        return COMPRESS_MODE_LINE;
    } else if (mode == "frame") {
        return COMPRESS_MODE_FRAME;
    } else {
        std::cout << "invalid compress mode: " << mode << ", use COMPRESS_MODE_NONE" << std::endl;
        return COMPRESS_MODE_NONE;
    }
}

void set_videosrc(SERVICE_CTX_ENTITY *ent, const nlohmann::json &params)
{
    SET_CTX(params, "chn", ent->ViChn);
    SET_CTX(params, "buf-blk-cnt", ent->buf_blk_cnt);
    SET_CTX(params, "enable-3dnr", ent->bEnable3DNR);
    SET_CTX(params, "enable-lsc", ent->bEnableLSC);
    SET_CTX(params, "vpss-binding", ent->bVpssBinding);
    SET_CTX(params, "vpss-chn", ent->VpssChn);
    SET_CTX(params, "enable-isp-info-osd", ent->enableIspInfoOsd);

    if (params.contains("compress-mode")) {
        ent->compress_mode = get_compress_mode(params.at("compress-mode"));
    }
}

void set_venc(SERVICE_CTX_ENTITY *ent, const nlohmann::json &params)
{
    if (params.contains("codec")) {
        std::string codec = params.at("codec");
        snprintf(ent->venc_cfg.codec, sizeof(ent->venc_cfg.codec), "%s", codec.c_str());
    }

    SET_CTX(params, "venc-bind-vpss", ent->bVencBindVpss);
    SET_CTX(params, "bs-mode", ent->venc_cfg.bsMode);
    SET_CTX(params, "rc-mode", ent->venc_cfg.rcMode);
    SET_CTX(params, "gop", ent->venc_cfg.gop);
    SET_CTX(params, "iqp", ent->venc_cfg.iqp);
    SET_CTX(params, "pqp", ent->venc_cfg.pqp);
    SET_CTX(params, "bitrate", ent->venc_cfg.bitrate);
    SET_CTX(params, "max-bitrate", ent->venc_cfg.maxbitrate);
    SET_CTX(params, "firstqp", ent->venc_cfg.firstFrmstartQp);
    SET_CTX(params, "initialDelay", ent->venc_cfg.initialDelay);
    SET_CTX(params, "thrdLv", ent->venc_cfg.u32ThrdLv);
    SET_CTX(params, "h264EntropyMode", ent->venc_cfg.h264EntropyMode);
    SET_CTX(params, "maxIprop", ent->venc_cfg.maxIprop);
    SET_CTX(params, "max-qp", ent->venc_cfg.maxQp);
    SET_CTX(params, "min-qp", ent->venc_cfg.minQp);
    SET_CTX(params, "max-iqp", ent->venc_cfg.maxIqp);
    SET_CTX(params, "min-iqp", ent->venc_cfg.minIqp);
    SET_CTX(params, "ipQpDelta", ent->venc_cfg.s32IPQpDelta);
    SET_CTX(params, "change-pos", ent->venc_cfg.s32ChangePos);
    SET_CTX(params, "minStillPercent", ent->venc_cfg.s32MinStillPercent);
    SET_CTX(params, "maxStillQp", ent->venc_cfg.u32MaxStillQP);
    SET_CTX(params, "motionSense", ent->venc_cfg.u32MotionSensitivity);
    SET_CTX(params, "avbrFrmLostOpen", ent->venc_cfg.s32AvbrFrmLostOpen);
    SET_CTX(params, "avbrFrmGap", ent->venc_cfg.s32AvbrFrmGap);
    SET_CTX(params, "avbrPureStillThr", ent->venc_cfg.s32AvbrPureStillThr);
    SET_CTX(params, "bitstream-buf-size", ent->venc_cfg.bitstreamBufSize);

    if (params.contains("venc_json")) {
        std::string file = params.at("venc_json");
        snprintf(ent->venc_json, sizeof(ent->venc_json), "%s", file.c_str());
        CVI_RTSP_SERVICE_SetVencCfg(ent->venc_json, &ent->venc_cfg);
    }
}

void set_ai(SERVICE_CTX_ENTITY *ent, const nlohmann::json &params)
{
    SET_CTX(params, "enable-faceae", ent->enableRetinaFace);
}

void set_teaisppq(SERVICE_CTX_ENTITY *ent, const nlohmann::json &params)
{
    SET_CTX(params, "enable-teaisp-pq", ent->enableTeaisppq);
    if (ent->enableTeaisppq) {
        printf("set compress mode(none) for teaisppq...\n");
        ent->compress_mode = COMPRESS_MODE_NONE;
    }
}

void set_teaisp_drc(SERVICE_CTX_ENTITY *ent, const nlohmann::json &params)
{
    SET_CTX(params, "enable-teaisp-drc", ent->enableTeaispDrc);
}

void set_hdmi(SERVICE_CTX_ENTITY *ent, const nlohmann::json &params)
{
    SET_CTX(params, "enable-hdmi", ent->enableHdmi);
}

void set_teaisp_bnr(SERVICE_CTX_ENTITY *ent, const nlohmann::json &params)
{
    SET_CTX(params, "enable-teaisp-bnr", ent->enableTEAISPBnr);

    if (ent->enableTEAISPBnr) {
        printf("set compress mode(none) for teaisp...\n");
        ent->compress_mode = COMPRESS_MODE_NONE;
    }

    if (params.contains("teaisp_model_list")) {
        std::string file = params.at("teaisp_model_list");
        snprintf(ent->teaisp_model_list, sizeof(ent->teaisp_model_list), "%s", file.c_str());
    }
}

void set_rtsp(SERVICE_CTX_ENTITY *ent, const nlohmann::json &params)
{
    if (params.contains("rtsp-url")) {
        std::string url = params.at("rtsp-url");
        snprintf(ent->rtspURL, sizeof(ent->rtspURL), "%s", url.c_str());
    }

    SET_CTX(params, "rtsp-bitrate", ent->rtsp_bitrate);
}

void dump_venc_cfg(chnInputCfg *venc_cfg)
{
    printf("**** - venc_cfg.codec:%s\n", venc_cfg->codec);
    printf("**** - venc_cfg.bsMode:%d\n", venc_cfg->bsMode);
    printf("**** - venc_cfg.rcMode:%d\n", venc_cfg->rcMode);
    printf("**** - venc_cfg.gop:%d\n", venc_cfg->gop);
    printf("**** - venc_cfg.iqp:%d\n", venc_cfg->iqp);
    printf("**** - venc_cfg.pqp:%d\n", venc_cfg->pqp);
    printf("**** - venc_cfg.bitrate:%d\n", venc_cfg->bitrate);
    printf("**** - venc_cfg.maxbitrate:%d\n", venc_cfg->maxbitrate);
    printf("**** - venc_cfg.firstFrmstartQp:%d\n", venc_cfg->firstFrmstartQp);
    printf("**** - venc_cfg.initialDelay:%d\n", venc_cfg->initialDelay);
    printf("**** - venc_cfg.u32ThrdLv:%d\n", venc_cfg->u32ThrdLv);
    printf("**** - venc_cfg.h264EntropyMode:%d\n", venc_cfg->h264EntropyMode);
    printf("**** - venc_cfg.maxIprop:%d\n", venc_cfg->maxIprop);
    printf("**** - venc_cfg.maxQp:%d\n", venc_cfg->maxQp);
    printf("**** - venc_cfg.minQp:%d\n", venc_cfg->minQp);
    printf("**** - venc_cfg.maxIqp:%d\n", venc_cfg->maxIqp);
    printf("**** - venc_cfg.minIqp:%d\n", venc_cfg->minIqp);
    printf("**** - venc_cfg.s32IPQpDelta:%d\n", venc_cfg->s32IPQpDelta);
    printf("**** - venc_cfg.s32ChangePos:%d\n", venc_cfg->s32ChangePos);
    printf("**** - venc_cfg.s32MinStillPercent:%d\n", venc_cfg->s32MinStillPercent);
    printf("**** - venc_cfg.u32MaxStillQP:%d\n", venc_cfg->u32MaxStillQP);
    printf("**** - venc_cfg.u32MotionSensitivity:%d\n", venc_cfg->u32MotionSensitivity);
    printf("**** - venc_cfg.s32AvbrFrmLostOpen:%d\n", venc_cfg->s32AvbrFrmLostOpen);
    printf("**** - venc_cfg.s32AvbrFrmGap:%d\n", venc_cfg->s32AvbrFrmGap);
    printf("**** - venc_cfg.s32AvbrPureStillThr:%d\n", venc_cfg->s32AvbrPureStillThr);
    printf("**** - venc_cfg.bitstreamBufSize:%d\n", venc_cfg->bitstreamBufSize);
    printf("**** - venc_cfg.aspectRatioInfoPresentFlag:%d\n", venc_cfg->aspectRatioInfoPresentFlag);
    printf("**** - venc_cfg.overscanInfoPresentFlag:%d\n", venc_cfg->overscanInfoPresentFlag);
    printf("**** - venc_cfg.videoSignalTypePresentFlag:%d\n", venc_cfg->videoSignalTypePresentFlag);
    printf("**** - venc_cfg.videoFormat:%d\n", venc_cfg->videoFormat);
    printf("**** - venc_cfg.videoFullRangeFlag:%d\n", venc_cfg->videoFullRangeFlag);
    printf("**** - venc_cfg.colourDescriptionPresentFlag:%d\n", venc_cfg->colourDescriptionPresentFlag);
}

void dump_ctx_info(SERVICE_CTX *ctx)
{
    if (!ctx) {
        printf("* Empty ctx ptr\n");
        return;
    }

    /* common info */
    printf("*** dev_num:%d\n", ctx->dev_num);
    printf("*** rtspPort:%d\n", ctx->rtspPort);
    printf("*** rtsp_max_buf_size:%d\n", ctx->rtsp_max_buf_size);
    printf("*** vi_vpss_mode:%d\n", ctx->vi_vpss_mode);
    printf("*** buf1_blk_cnt:%d\n", ctx->buf1_blk_cnt);
    printf("*** max_use_tpu_num:%d\n", ctx->max_use_tpu_num);
    printf("*** model_path:%s\n", ctx->model_path);
    printf("*** teaisp-pq-bmodel:%s\n", ctx->teaisppq_model_path);
    printf("*** teaisp-drc-bmodel:%s\n", ctx->teaispdrc_model_path);
    printf("*** enable_set_sensor_config:%d\n", ctx->enable_set_sensor_config);
    printf("*** sensor_config_path:%s\n", ctx->sensor_config_path);
    printf("*** isp_debug_lvl:%d\n", ctx->isp_debug_lvl);
    printf("*** enable_set_pq_bin:%d\n", ctx->enable_set_pq_bin);
    printf("*** sdr_pq_bin_path:%s\n", ctx->sdr_pq_bin_path);
    printf("*** wdr_pq_bin_path:%s\n", ctx->wdr_pq_bin_path);

    /* per sensor info */
    for (int i=0; i<ctx->dev_num; i++) {
        SERVICE_CTX_ENTITY *pEntity = &ctx->entity[i];
        printf("**** video_src_info[%d]\n", i);
        printf("**** - rtspURL:%s\n", pEntity->rtspURL);

        printf("**** - ViChn:%d\n", pEntity->ViChn);
        printf("**** - buf_blk_cnt:%d\n", pEntity->buf_blk_cnt);
        printf("**** - bEnable3DNR:%d\n", pEntity->bEnable3DNR);
        printf("**** - bEnableLSC:%d\n", pEntity->bEnableLSC);
        printf("**** - bVpssBinding:%d\n", pEntity->bVpssBinding);
        printf("**** - VpssChn:%d\n", pEntity->VpssChn);
        printf("**** - enableIspInfoOsd:%d\n", pEntity->enableIspInfoOsd);
        printf("**** - compress_mode:%d\n", pEntity->compress_mode);

        printf("**** - bVencBindVpss:%d\n", pEntity->bVencBindVpss);
        printf("**** - enableRetinaFace:%d\n", pEntity->enableRetinaFace);
        printf("**** - enableTeaisppq:%d\n", pEntity->enableTeaisppq);
        printf("**** - enableTeaispDrc:%d\n", pEntity->enableTeaispDrc);
        printf("**** - enable-hdmi:%d\n", pEntity->enableHdmi);
        printf("**** - enable-teaisp-bnr:%d\n", pEntity->enableTEAISPBnr);
        printf("**** - teaisp_model_list:%s\n", pEntity->teaisp_model_list);

        dump_venc_cfg(&pEntity->venc_cfg);
    }
}

void set_replay(REPLAY_PARAM *paramPtr, const char *path)
{
	nlohmann::json params;
	if (json_parse_from_file(path, params) < 0) {
		printf("fail to parse replay config, use default param!");
		paramPtr->pixelFormat = 0;
		paramPtr->width = 2560;
		paramPtr->height = 1440;
		paramPtr->timingEnable = false;
		paramPtr->frameRate = 25;
		paramPtr->WDRMode = 0;
		paramPtr->bayerFormat = 0;
		paramPtr->compressMode = COMPRESS_MODE_NONE;
	} else {
		SET_CTX(params, "PixelFormat", paramPtr->pixelFormat);
		SET_CTX(params, "width", paramPtr->width);
		SET_CTX(params, "height", paramPtr->height);
		SET_CTX(params, "TimingEnable", paramPtr->timingEnable);
		SET_CTX(params, "FrameRate", paramPtr->frameRate);
		SET_CTX(params, "WDRMode", paramPtr->WDRMode);
		SET_CTX(params, "BayerFormat", paramPtr->bayerFormat);
		paramPtr->compressMode = get_compress_mode(params.at("CompressMode"));
		paramPtr->frameRate = paramPtr->frameRate > 0 ? paramPtr->frameRate : 25;
	}
}

int load_json_config(SERVICE_CTX *ctx, const nlohmann::json &params)
{
    if (params.empty()) {
        return 0;
    }

    SET_CTX(params, "rtsp-port", ctx->rtspPort);
    SET_CTX(params, "rtsp-max-buf-size", ctx->rtsp_max_buf_size);
    SET_CTX(params, "dev-num", ctx->rtsp_num);
    SET_CTX(params, "vi-vpss-mode", ctx->vi_vpss_mode);
    SET_CTX(params, "buf1-blk-cnt", ctx->buf1_blk_cnt);
    SET_CTX(params, "max-use-tpu-num", ctx->max_use_tpu_num);
    SET_CTX_STR(params, "teaisp-faceae-model", ctx->model_path);
    SET_CTX_STR(params, "teaisp-pq-model", ctx->teaisppq_model_path);
    SET_CTX_STR(params, "teaisp-drc-model", ctx->teaispdrc_model_path);
    SET_CTX(params, "sbm", ctx->sbm.enable);
    SET_CTX(params, "sbm-buf-line", ctx->sbm.bufLine);
    SET_CTX(params, "sbm-buf-size", ctx->sbm.bufSize);

    /* isp */
    SET_CTX(params, "enable-set-sensor-config", ctx->enable_set_sensor_config);
    SET_CTX_STR(params, "sensor-config-path", ctx->sensor_config_path);
    SET_CTX(params, "isp-debug-lvl", ctx->isp_debug_lvl);
    SET_CTX(params, "enable-set-pq-bin", ctx->enable_set_pq_bin);
    SET_CTX_STR(params, "sdr-pq-bin-path", ctx->sdr_pq_bin_path);
    SET_CTX_STR(params, "wdr-pq-bin-path", ctx->wdr_pq_bin_path);

    nlohmann::json video_src_info;
    GET_DATA(params, "video-src-info", video_src_info);
    ctx->dev_num = ctx->stIniCfg.devNum;

    if (ctx->dev_num > video_src_info.size()) {
        printf("*** Invalid dev number[%d], vidoeSrcSize[%zu]\n", ctx->dev_num, video_src_info.size());
        ctx->dev_num = video_src_info.size();
    }

    for (int i=0; i<ctx->dev_num; i++) {
        SERVICE_CTX_ENTITY *pEntity = &ctx->entity[i];
        set_rtsp(pEntity, video_src_info[i]);
        set_videosrc(pEntity, video_src_info[i]);
        set_venc(pEntity, video_src_info[i]);
        set_ai(pEntity, video_src_info[i]);
        set_teaisppq(pEntity, video_src_info[i]);
        set_teaisp_drc(pEntity, video_src_info[i]);
        set_hdmi(pEntity, video_src_info[i]);
        set_teaisp_bnr(pEntity, video_src_info[i]);
    }

    SET_CTX(params, "replay-mode", ctx->replayMode);
    if (ctx->replayMode) {
        std::string paramFile = params.at("replay-param");
        set_replay(&ctx->replayParam, paramFile.c_str());
        ctx->dev_num = 1;
        ctx->rtsp_num = 1;
        ctx->stIniCfg.devNum = 1;
        ctx->entity[0].compress_mode = ctx->replayParam.compressMode;
    }

    dump_ctx_info(ctx);

    return 0;
}
