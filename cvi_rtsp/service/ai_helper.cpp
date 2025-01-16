#include "ctx.h"
#include "vpss_helper.h"
#include <cvi_tdl.h>
#include <dlfcn.h>
#include <sys/stat.h>

#define LOAD_SYMBOL(dl, sym, type, fn) \
do { \
    if (NULL == (fn = (type)dlsym(dl, sym))) { \
        printf("load symbol %s fail, %s\n", sym, dlerror()); \
        return -1; \
    } \
} while(0);

int load_ai_symbol(SERVICE_CTX *ctx)
{
    ctx->ai_dl = dlopen("libcviai.so", RTLD_LAZY);
    if (!ctx->ai_dl) {
        std::cout << "dlopen libcviai.so fail" << std::endl;
        return -1;
    }

    dlerror();

    for (int idx=0; idx<ctx->dev_num; idx++) {
        SERVICE_CTX_ENTITY *ent = &ctx->entity[idx];
        LOAD_SYMBOL(ctx->ai_dl, "CVI_AI_CreateHandle2", AI_CreateHandle2, ent->ai_create_handle2);
        LOAD_SYMBOL(ctx->ai_dl, "CVI_AI_DestroyHandle", AI_DestroyHandle, ent->ai_destroy_handle);
        LOAD_SYMBOL(ctx->ai_dl, "CVI_AI_SetSkipVpssPreprocess", AI_SetSkipVpssPreprocess, ent->ai_set_skip_vpss_preprocess);
        LOAD_SYMBOL(ctx->ai_dl, "CVI_AI_OpenModel", AI_OpenModel, ent->ai_open_model);
        LOAD_SYMBOL(ctx->ai_dl, "CVI_AI_GetVpssChnConfig", AI_GetVpssChnConfig, ent->ai_get_vpss_chn_config);
        LOAD_SYMBOL(ctx->ai_dl, "CVI_AI_RetinaFace", AI_RetinaFace, ent->ai_retinaface);

        LOAD_SYMBOL(ctx->ai_dl, "CVI_AI_Service_CreateHandle", AI_Service_CreateHandle, ent->ai_service_create_handle);
        LOAD_SYMBOL(ctx->ai_dl, "CVI_AI_Service_DestroyHandle", AI_Service_DestroyHandle, ent->ai_service_destroy_handle);
        LOAD_SYMBOL(ctx->ai_dl, "CVI_AI_Service_FaceDrawRect", AI_Service_FaceDrawRect, ent->ai_face_draw_rect);
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

    if (ctx->ai_dl) {
        dlclose(ctx->ai_dl);
        ctx->ai_dl = NULL;
    }

    if (ctx->ai_dl) {
        dlclose(ctx->ai_dl);
        ctx->ai_dl = NULL;
     }
}
