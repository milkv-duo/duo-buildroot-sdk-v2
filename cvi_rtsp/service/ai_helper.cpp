#include "ctx.h"
#include "vpss_helper.h"
#include <cvi_tdl.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include "cvi_isp.h"

#define AI_LIB "libcvi_tdl.so"

#define LOAD_SYMBOL(dl, sym, type, fn) \
do { \
    if (NULL == fn && NULL == (fn = (type)dlsym(dl, sym))) { \
        printf("load symbol %s fail, %s\n", sym, dlerror()); \
        return -1; \
    } \
} while(0);

int load_ai_symbol(SERVICE_CTX *ctx)
{
     if (!ctx->ai_dl) {
        std::cout << "Loading " AI_LIB " ..." << std::endl;
        ctx->ai_dl = dlopen(AI_LIB, RTLD_LAZY);

        if (!ctx->ai_dl) {
            std::cout << "dlopen " AI_LIB " fail: " << std::endl;
            std::cout << dlerror() << std::endl;
            return -1;
        }
    }

    dlerror();

    for (int idx=0; idx<ctx->dev_num; idx++) {
        SERVICE_CTX_ENTITY *ent = &ctx->entity[idx];
        LOAD_SYMBOL(ctx->ai_dl, "CVI_TDL_CreateHandle2", AI_CreateHandle2, ent->ai_create_handle2);
        LOAD_SYMBOL(ctx->ai_dl, "CVI_TDL_DestroyHandle", AI_DestroyHandle, ent->ai_destroy_handle);
        LOAD_SYMBOL(ctx->ai_dl, "CVI_TDL_SetSkipVpssPreprocess", AI_SetSkipVpssPreprocess, ent->ai_set_skip_vpss_preprocess);
        LOAD_SYMBOL(ctx->ai_dl, "CVI_TDL_OpenModel", AI_OpenModel, ent->ai_open_model);
        LOAD_SYMBOL(ctx->ai_dl, "CVI_TDL_GetVpssChnConfig", AI_GetVpssChnConfig, ent->ai_get_vpss_chn_config);
        LOAD_SYMBOL(ctx->ai_dl, "CVI_TDL_RetinaFace", AI_RetinaFace, ent->ai_retinaface);

        LOAD_SYMBOL(ctx->ai_dl, "CVI_TDL_Service_CreateHandle", AI_Service_CreateHandle, ent->ai_service_create_handle);
        LOAD_SYMBOL(ctx->ai_dl, "CVI_TDL_Service_DestroyHandle", AI_Service_DestroyHandle, ent->ai_service_destroy_handle);
        LOAD_SYMBOL(ctx->ai_dl, "CVI_TDL_Service_FaceDrawRect", AI_Service_FaceDrawRect, ent->ai_face_draw_rect);
    }

    return 0;
}

int init_ai(SERVICE_CTX *ctx)
{
    for (int idx=0; idx<ctx->dev_num; idx++) {
        SERVICE_CTX_ENTITY *ent = &ctx->entity[idx];
        if (!ent->enableRetinaFace) {
            continue;
        }

        if (0 > load_ai_symbol(ctx)) {
            printf("load_ai_symbol fail\n");
            return -1;
        }

        //ai default use dev1 create group
        if (ent->ai_create_handle2(&ent->ai_handle, CVI_VPSS_GetAvailableGrp(), 1) != CVI_SUCCESS) {
            printf("Create AISDK Handle2 failed!\n");
            return -1;
        }

        if (ent->ai_service_create_handle(&ent->ai_service_handle, ent->ai_handle) != CVI_SUCCESS) {
            printf("Create AISDK FR Service failed!\n");
            return -1;
        }

        struct stat st = {0};
        if (0 == strlen(ctx->model_path) || 0 != stat(ctx->model_path, &st)) {
            printf("retina model %s not found\n", ctx->model_path);
            return -1;
        }

        if (ent->ai_open_model(ent->ai_handle, CVI_TDL_SUPPORTED_MODEL_RETINAFACE, ctx->model_path) != CVI_SUCCESS) {
            printf("CVI_AI_SetModelPath: %s failed!\n", ctx->model_path);
            return -1;
        }

        ent->ai_set_skip_vpss_preprocess(ent->ai_handle, CVI_TDL_SUPPORTED_MODEL_RETINAFACE, true);

        if (ent->ai_get_vpss_chn_config(ent->ai_handle, CVI_TDL_SUPPORTED_MODEL_RETINAFACE, 1920, 1080, 0, &ent->retinaVpssConfig) != CVI_SUCCESS) {
            printf("CVI_AI_GetVpssChnConfig Failed!\n");
            return -1;
        }

        if (ent->retinaVpssConfig.chn_attr.stAspectRatio.enMode != ASPECT_RATIO_AUTO) {
            LOAD_SYMBOL(ctx->ai_dl, "CVI_AI_RescaleMetaCenterFace", FD_RescaleFunc, ent->rescale_fd);
        } else {
            LOAD_SYMBOL(ctx->ai_dl, "CVI_AI_RescaleMetaRBFace", FD_RescaleFunc, ent->rescale_fd);
        }

        int rc = -1;
        if (0 != (rc = CVI_VPSS_SetChnScaleCoefLevel(ent->VpssGrp, VPSS_CHN1, ent->retinaVpssConfig.chn_coeff))) {
            printf("CVI_VPSS_SetChnScaleCoefLevel fail, GRP: %d, CHN: %d, coeff: %d, rc: %d\n", ent->VpssGrp, VPSS_CHN1, ent->retinaVpssConfig.chn_coeff, rc);
            return -1;
        }

        if (0 != (rc = CVI_VPSS_SetChnAttr(ent->VpssGrp, VPSS_CHN1, &ent->retinaVpssConfig.chn_attr))) {
            printf("CVI_VPSS_SetChnAttr fail, GRP: %d, CHN: %d, rc: %d\n", ent->VpssGrp, VPSS_CHN1, rc);
            return -1;
        }

        if (0 != (rc = CVI_VPSS_EnableChn(ent->VpssGrp, VPSS_CHN1))) {
            printf("CVI_VPSS_EnableChn fail, GRP: %d, CHN: %d, rc: %d\n", ent->VpssGrp, VPSS_CHN1, rc);
            return -1;
        }

        if (0 != (rc = CVI_VPSS_SetGrpParamfromBin(ent->VpssGrp, 0))) {
            printf("CVI_VPSS_SetGrpParamfromBin, GRP: %d, rc: %d\n", ent->VpssGrp, rc);
            return -1;
        }
    }

    ctx->tdl_ref_count++;

    return 0;
}

void deinit_ai(SERVICE_CTX *ctx)
{
    for (int idx=0; idx<ctx->dev_num; idx++) {
        SERVICE_CTX_ENTITY *ent = &ctx->entity[idx];
        if (!ent->enableRetinaFace) continue;
        if (ent->ai_service_handle != NULL) {
            ent->ai_service_destroy_handle(ent->ai_service_handle);
        }

        if (ent->ai_handle != NULL) {
            ent->ai_destroy_handle(ent->ai_handle);
        }
    }

    if (ctx->ai_dl && --ctx->tdl_ref_count <= 0) {
        dlclose(ctx->ai_dl);
        ctx->ai_dl = NULL;
    }
}

static void load_bnr_model(VI_PIPE ViPipe, char *model_path)
{
    FILE *fp = fopen(model_path, "r");

    if (fp == NULL) {
        printf("open model path failed, %s\n", model_path);
        return;
    }

    char line[1024];
    char *token;
    char *rest;

    TEAISP_BNR_MODEL_INFO_S stModelInfo;

    while (fgets(line, sizeof(line), fp) != NULL) {

        printf("load model, list, %s", line);

        rest = line;
        token = strtok_r(rest, " ", &rest);

        memset(&stModelInfo, 0, sizeof(TEAISP_BNR_MODEL_INFO_S));
        snprintf(stModelInfo.path, TEAISP_MODEL_PATH_LEN, "%s", token);

        printf("load model, path, %s\n", token);

        token = strtok_r(rest, " ", &rest);

        int iso = atoi(token);

        stModelInfo.enterISO = iso;
        stModelInfo.tolerance = iso * 10 / 100;

        printf("load model, iso, %d, %d\n", stModelInfo.enterISO, stModelInfo.tolerance);
        CVI_TEAISP_BNR_SetModel(ViPipe, &stModelInfo);
    }

    fclose(fp);
}

typedef CVI_S32 (*TEAISP_INIT_FUN)(VI_PIPE ViPipe, CVI_S32 maxDev);

int init_teaisp_bnr(SERVICE_CTX *ctx)
{
    (void) ctx;

    void *teaisp_dl = NULL;
    TEAISP_INIT_FUN teaisp_init = NULL;

    for (int i = 0; i < ctx->dev_num; i++) {

        SERVICE_CTX_ENTITY *pEntity = &ctx->entity[i];

        if (!pEntity->enableTEAISPBnr) {
            continue;
        }

        if (teaisp_dl == NULL) {

            teaisp_dl = dlopen("libteaisp.so", RTLD_LAZY);
            if (!teaisp_dl) {
                std::cout << "dlopen libteaisp.so fail" << std::endl;
                return -1;
            }

            dlerror();

            LOAD_SYMBOL(teaisp_dl, "CVI_TEAISP_Init", TEAISP_INIT_FUN, teaisp_init);
        }

        teaisp_init(i, ctx->max_use_tpu_num);
        load_bnr_model(i, pEntity->teaisp_model_list);
    }

    return 0;
}

int deinit_teaisp_bnr(SERVICE_CTX *ctx)
{
    (void) ctx;

    return 0;
}