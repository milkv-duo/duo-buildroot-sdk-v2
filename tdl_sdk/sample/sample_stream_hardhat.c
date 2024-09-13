#include "core/utils/vpss_helper.h"
#include "cvi_tdl.h"
#include "cvi_tdl_app.h"
#include "sample_comm.h"
#include "vi_vo_utils.h"

#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

#define OUTPUT_BUFFER_SIZE 10
#define MAX(a, b) ((a) > (b) ? (a) : (b))
static cvtdl_face_t g_face_meta_0;

#define SMT_MUTEXAUTOLOCK_INIT(mutex) pthread_mutex_t AUTOLOCK_##mutex = PTHREAD_MUTEX_INITIALIZER;

#define SMT_MutexAutoLock(mutex, lock)                                            \
  __attribute__((cleanup(AutoUnLock))) pthread_mutex_t *lock = &AUTOLOCK_##mutex; \
  pthread_mutex_lock(lock);

__attribute__((always_inline)) inline void AutoUnLock(void *mutex) {
  pthread_mutex_unlock(*(pthread_mutex_t **)mutex);
}

typedef struct {
  uint64_t u_id;
  float quality;
  cvtdl_image_t image;
  tracker_state_e state;
} IOData;

typedef struct {
  VideoSystemContext vs_ctx;
  cvitdl_service_handle_t service_handle;
} pVOArgs;

SMT_MUTEXAUTOLOCK_INIT(VOMutex);

/* global variables */
static volatile bool bExit = false;
static volatile bool bRunVideoOutput = true;

int rear_idx = 0;
int front_idx = 0;
// static IOData data_buffer[OUTPUT_BUFFER_SIZE];

static void SampleHandleSig(CVI_S32 signo) {
  signal(SIGINT, SIG_IGN);
  signal(SIGTERM, SIG_IGN);

  if (SIGINT == signo || SIGTERM == signo) {
    bExit = true;
  }
}

void RESTRUCTURING_FACE_META(cvtdl_face_t *face_cpt_info, cvtdl_face_t *face_meta) {
  face_meta->size = 0;
  for (uint32_t i = 0; i < face_cpt_info->size; i++) {
    face_meta->size += 1;
  }

  face_meta->info = (cvtdl_face_info_t *)malloc(sizeof(cvtdl_face_info_t) * face_meta->size);
  memset(face_meta->info, 0, sizeof(cvtdl_face_info_t) * face_meta->size);
  face_meta->rescale_type = face_cpt_info->rescale_type;
  face_meta->height = face_cpt_info->height;
  face_meta->width = face_cpt_info->width;

  cvtdl_face_info_t *info_ptr_0 = face_meta->info;
  for (uint32_t i = 0; i < face_cpt_info->size; i++) {
    cvtdl_face_info_t **tmp_ptr = &info_ptr_0;
    (*tmp_ptr)->unique_id = face_cpt_info->info[i].unique_id;
    (*tmp_ptr)->face_quality = face_cpt_info->info[i].face_quality;
    (*tmp_ptr)->mask_score = face_cpt_info->info[i].mask_score;
    (*tmp_ptr)->hardhat_score = face_cpt_info->info[i].hardhat_score;
    memcpy(&(*tmp_ptr)->bbox, &face_cpt_info->info[i].bbox, sizeof(cvtdl_bbox_t));
    *tmp_ptr += 1;
  }

  return;
}

static void *pVideoOutput(void *args) {
  printf("[APP] Video Output Up\n");
  pVOArgs *vo_args = (pVOArgs *)args;
  cvitdl_service_handle_t service_handle = vo_args->service_handle;
  CVI_S32 s32Ret = CVI_SUCCESS;

  cvtdl_service_brush_t brush_0 = {.size = 4, .color.r = 0, .color.g = 64, .color.b = 255};
  cvtdl_face_t face_meta_0;
  memset(&face_meta_0, 0, sizeof(cvtdl_face_t));

  VIDEO_FRAME_INFO_S stVOFrame;
  while (bRunVideoOutput) {
    s32Ret = CVI_VPSS_GetChnFrame(vo_args->vs_ctx.vpssConfigs.vpssGrp,
                                  vo_args->vs_ctx.vpssConfigs.vpssChnVideoOutput, &stVOFrame, 1000);
    if (s32Ret != CVI_SUCCESS) {
      printf("CVI_VPSS_GetChnFrame chn0 failed with %#x\n", s32Ret);
      break;
    }
    {
      SMT_MutexAutoLock(VOMutex, lock);

      memcpy(&face_meta_0, &g_face_meta_0, sizeof(cvtdl_face_t));
      face_meta_0.info =
          (cvtdl_face_info_t *)malloc(sizeof(cvtdl_face_info_t) * g_face_meta_0.size);
      memset(face_meta_0.info, 0, sizeof(cvtdl_face_info_t) * face_meta_0.size);
      for (uint32_t i = 0; i < g_face_meta_0.size; i++) {
        face_meta_0.info[i].hardhat_score = MAX(0, g_face_meta_0.info[i].hardhat_score);
        memcpy(&face_meta_0.info[i].bbox, &g_face_meta_0.info[i].bbox, sizeof(cvtdl_bbox_t));
      }
    }
    size_t image_size = stVOFrame.stVFrame.u32Length[0] + stVOFrame.stVFrame.u32Length[1] +
                        stVOFrame.stVFrame.u32Length[2];
    stVOFrame.stVFrame.pu8VirAddr[0] =
        (uint8_t *)CVI_SYS_Mmap(stVOFrame.stVFrame.u64PhyAddr[0], image_size);
    stVOFrame.stVFrame.pu8VirAddr[1] =
        stVOFrame.stVFrame.pu8VirAddr[0] + stVOFrame.stVFrame.u32Length[0];
    stVOFrame.stVFrame.pu8VirAddr[2] =
        stVOFrame.stVFrame.pu8VirAddr[1] + stVOFrame.stVFrame.u32Length[1];
    CVI_TDL_Service_FaceDrawRect(service_handle, &face_meta_0, &stVOFrame, false, brush_0);

    for (uint32_t j = 0; j < face_meta_0.size; j++) {
      char *id_num = calloc(64, sizeof(char));
      // sprintf(id_num, "%" PRIu64 "", face_meta_0.info[j].unique_id);
      sprintf(id_num, "%.4f", face_meta_0.info[j].hardhat_score);
      CVI_TDL_Service_ObjectWriteText(id_num, face_meta_0.info[j].bbox.x1,
                                      face_meta_0.info[j].bbox.y1, &stVOFrame, 1, 1, 1);
      free(id_num);
    }

    CVI_SYS_Munmap((void *)stVOFrame.stVFrame.pu8VirAddr[0], image_size);
    stVOFrame.stVFrame.pu8VirAddr[0] = NULL;
    stVOFrame.stVFrame.pu8VirAddr[1] = NULL;
    stVOFrame.stVFrame.pu8VirAddr[2] = NULL;

    s32Ret = SendOutputFrame(&stVOFrame, &vo_args->vs_ctx.outputContext);
    if (s32Ret != CVI_SUCCESS) {
      printf("Send Output Frame NG\n");
    }

    s32Ret = CVI_VPSS_ReleaseChnFrame(vo_args->vs_ctx.vpssConfigs.vpssGrp,
                                      vo_args->vs_ctx.vpssConfigs.vpssChnVideoOutput, &stVOFrame);
    if (s32Ret != CVI_SUCCESS) {
      printf("CVI_VPSS_ReleaseChnFrame chn0 NG\n");
      break;
    }

    CVI_TDL_Free(&face_meta_0);
  }
  return NULL;
}

int main(int argc, char *argv[]) {
  // Set signal catch
  signal(SIGINT, SampleHandleSig);
  signal(SIGTERM, SampleHandleSig);
  const char *fd_model_path = argv[1];
  float det_threshold = atof(argv[2]);

  CVI_S32 ret = CVI_TDL_SUCCESS;
  VideoSystemContext vs_ctx = {0};
  int fps = 25;
  if (InitVideoSystem(&vs_ctx, fps) != CVI_SUCCESS) {
    printf("failed to init video system\n");
    goto CLEANUP_VIDEO;
  }

  cvitdl_handle_t tdl_handle = NULL;
  cvitdl_service_handle_t service_handle = NULL;
  ret = CVI_TDL_CreateHandle2(&tdl_handle, 1, 0);
  ret = CVI_TDL_Service_CreateHandle(&service_handle, tdl_handle);
  if (ret != CVI_TDL_SUCCESS) {
    printf("failed with %#x!\n", ret);
    goto CLEANUP_TDL_HANDLE;
  }

  ret |= CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_RETINAFACE_IR, fd_model_path);
  ret |= CVI_TDL_SetVpssTimeout(tdl_handle, 1000);
  ret |=
      CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_RETINAFACE_IR, det_threshold);
  if (ret != CVI_SUCCESS) {
    printf("failed to set model\n");
    goto CLEANUP_TDL_SERVICE;
  }

  memset(&g_face_meta_0, 0, sizeof(cvtdl_face_t));

  cvtdl_face_t p_obj = {0};
  pthread_t vo_thread;
  pVOArgs vo_args = {0};
  vo_args.service_handle = service_handle;
  vo_args.vs_ctx = vs_ctx;
  pthread_create(&vo_thread, NULL, pVideoOutput, (void *)&vo_args);

  VIDEO_FRAME_INFO_S stfdFrame;
  while (bExit == false) {
    ret = CVI_VPSS_GetChnFrame(vs_ctx.vpssConfigs.vpssGrp, vs_ctx.vpssConfigs.vpssChntdl,
                               &stfdFrame, 2000);
    if (ret != CVI_SUCCESS) {
      printf("CVI_VPSS_GetChnFrame chn0 failed with %#x\n", ret);
      break;
    }

    // get bbox and hardhat
    ret = CVI_TDL_FaceDetection(tdl_handle, &stfdFrame, CVI_TDL_SUPPORTED_MODEL_RETINAFACE_IR,
                                &p_obj);
    if (ret != CVI_SUCCESS) {
      printf("failed to run face detection\n");
      break;
    }

    SMT_MutexAutoLock(VOMutex, lock);
    CVI_TDL_Free(&g_face_meta_0);
    RESTRUCTURING_FACE_META(&p_obj, &g_face_meta_0);

    ret = CVI_VPSS_ReleaseChnFrame(vs_ctx.vpssConfigs.vpssGrp, vs_ctx.vpssConfigs.vpssChntdl,
                                   &stfdFrame);
    if (ret != CVI_SUCCESS) {
      printf("CVI_VPSS_ReleaseChnFrame chn0 NG\n");
      break;
    }
    CVI_TDL_Free(&p_obj);
  }
  bRunVideoOutput = false;
  pthread_join(vo_thread, NULL);

CLEANUP_TDL_SERVICE:
  CVI_TDL_Service_DestroyHandle(service_handle);
CLEANUP_TDL_HANDLE:
  CVI_TDL_DestroyHandle(tdl_handle);
CLEANUP_VIDEO:
  DestroyVideoSystem(&vs_ctx);
}
