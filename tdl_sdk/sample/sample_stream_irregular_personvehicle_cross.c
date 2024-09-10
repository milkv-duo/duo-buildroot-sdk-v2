#include "core/utils/vpss_helper.h"
#include "cvi_tdl.h"
#include "cvi_tdl_app/cvi_tdl_app.h"
#include "sample_comm.h"
#include "vi_vo_utils.h"

#include <cvi_sys.h>
#include <cvi_vb.h>
#include <cvi_vi.h>

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
static cvtdl_object_t g_obj_meta_0;
static cvtdl_object_t g_obj_meta_1;
static cvtdl_pts_t pts1;
static cvtdl_pts_t pts2;
static bool cross_mark;  // Crossing mark

// #define VISUAL_FACE_LANDMARK
// #define USE_OUTPUT_DATA_API

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
  uint32_t counter;
} IOData;

typedef struct {
  CVI_S32 voType;
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

void RESTRUCTURING_OBJ_META(cvtdl_object_t *origin_obj, cvtdl_object_t *obj_meta) {
  obj_meta->size = 0;
  for (uint32_t i = 0; i < origin_obj->size; i++) {
    obj_meta->size += 1;
  }

  obj_meta->info = (cvtdl_object_info_t *)malloc(sizeof(cvtdl_object_info_t) * obj_meta->size);
  memset(obj_meta->info, 0, sizeof(cvtdl_object_info_t) * obj_meta->size);
  obj_meta->rescale_type = origin_obj->rescale_type;
  obj_meta->height = origin_obj->height;
  obj_meta->width = origin_obj->width;

  cvtdl_object_info_t *info_ptr_0 = obj_meta->info;
  for (uint32_t i = 0; i < origin_obj->size; i++) {
    cvtdl_object_info_t **tmp_ptr = &info_ptr_0;
    (*tmp_ptr)->unique_id = origin_obj->info[i].unique_id;
    (*tmp_ptr)->classes = origin_obj->info[i].classes;
    memcpy(&(*tmp_ptr)->bbox, &origin_obj->info[i].bbox, sizeof(cvtdl_bbox_t));
    (*tmp_ptr)->is_cross = origin_obj->info[i].is_cross;
    cross_mark |= origin_obj->info[i].is_cross;
    *tmp_ptr += 1;
  }
  return;
}

static void *pVideoOutput(void *args) {
  printf("[APP] Video Output Up\n");
  pVOArgs *vo_args = (pVOArgs *)args;
  if (!vo_args->voType) {
    return NULL;
  }
  cvitdl_service_handle_t service_handle = vo_args->service_handle;
  CVI_S32 s32Ret = CVI_SUCCESS;

  cvtdl_service_brush_t brush_0 = {.size = 4, .color.r = 0, .color.g = 64, .color.b = 255};
  cvtdl_service_brush_t brush_2 = {.size = 4, .color.r = 255, .color.g = 0, .color.b = 0};

  cvtdl_object_t obj_meta_0, obj_meta_1;
  memset(&obj_meta_0, 0, sizeof(cvtdl_object_t));
  memset(&obj_meta_1, 0, sizeof(cvtdl_object_t));

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

      memcpy(&obj_meta_0, &g_obj_meta_0, sizeof(cvtdl_object_t));

      obj_meta_0.info =
          (cvtdl_object_info_t *)malloc(sizeof(cvtdl_object_info_t) * g_obj_meta_0.size);
      memset(obj_meta_0.info, 0, sizeof(cvtdl_object_info_t) * obj_meta_0.size);

      for (uint32_t i = 0; i < g_obj_meta_0.size; i++) {
        obj_meta_0.info[i].unique_id = MAX(0, g_obj_meta_0.info[i].unique_id);
        memcpy(&obj_meta_0.info[i].bbox, &g_obj_meta_0.info[i].bbox, sizeof(cvtdl_bbox_t));
        obj_meta_0.info[i].classes = g_obj_meta_0.info[i].classes;
      }
    }
    size_t image_size = stVOFrame.stVFrame.u32Length[0] + stVOFrame.stVFrame.u32Length[1] +
                        stVOFrame.stVFrame.u32Length[2];
    stVOFrame.stVFrame.pu8VirAddr[0] =
        (uint8_t *)CVI_SYS_MmapCache(stVOFrame.stVFrame.u64PhyAddr[0], image_size);
    stVOFrame.stVFrame.pu8VirAddr[1] =
        stVOFrame.stVFrame.pu8VirAddr[0] + stVOFrame.stVFrame.u32Length[0];
    stVOFrame.stVFrame.pu8VirAddr[2] =
        stVOFrame.stVFrame.pu8VirAddr[1] + stVOFrame.stVFrame.u32Length[1];
    CVI_TDL_Service_ObjectDrawRect(service_handle, &obj_meta_0, &stVOFrame, false, brush_0);
    CVI_TDL_Service_DrawPolygon(service_handle, &stVOFrame, &pts1, brush_2);
    CVI_TDL_Service_DrawPolygon(service_handle, &stVOFrame, &pts2, brush_2);
    for (uint32_t j = 0; j < obj_meta_0.size; j++) {
      char *id_num = calloc(64, sizeof(char));
      sprintf(id_num, "%" PRIu64, obj_meta_0.info[j].unique_id);
      CVI_TDL_Service_ObjectWriteText(id_num, obj_meta_0.info[j].bbox.x1,
                                      obj_meta_0.info[j].bbox.y1, &stVOFrame, 1, 1, 1);
      free(id_num);
    }

    char *ic = calloc(32, sizeof(char));
    CVI_TDL_Service_ObjectWriteText(ic, 200, 100, &stVOFrame, 2, 2, 2);
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

    CVI_TDL_Free(&obj_meta_0);
  }
  return NULL;
}

int main(int argc, char *argv[]) {
  CVI_S32 ret = CVI_TDL_SUCCESS;
  // Set signal catch
  signal(SIGINT, SampleHandleSig);
  signal(SIGTERM, SampleHandleSig);
  const char *od_model_path = argv[1];
  float det_threshold = atof(argv[2]);

  /*
  eg: input: 2 1 1 1

            0 1

      input: 2 2 2 1 3

           0 1
           0 1
  */
  // width num,height num
  int w_num = atoi(argv[3]);
  int h_num = atoi(argv[4]);

  // set regins number
  int regins_num = atoi(argv[5]);  // the number of areas
  bool regin_flags[1000];
  memset(regin_flags, false, sizeof(bool));
  for (int i = 0; i < regins_num; i++) {
    regin_flags[atoi(argv[6 + i])] = true;
  }
  const char *od_model_name = "yolov8";
  const char *reid_model_path = "NULL";
  int buffer_size = 10;
  int voType = 2;
  int vi_format = 0;

  VideoSystemContext vs_ctx = {0};
  SIZE_S aiInputSize = {.u32Width = 1280, .u32Height = 720};

  PIXEL_FORMAT_E aiInputFormat;
  if (vi_format == 0) {
    aiInputFormat = PIXEL_FORMAT_RGB_888;
  } else if (vi_format == 1) {
    aiInputFormat = PIXEL_FORMAT_NV21;
  } else if (vi_format == 2) {
    aiInputFormat = PIXEL_FORMAT_YUV_PLANAR_420;
  } else if (vi_format == 3) {
    aiInputFormat = PIXEL_FORMAT_RGB_888_PLANAR;
  } else {
    printf("vi format[%d] unknown.\n", vi_format);
    return CVI_FAILURE;
  }
  int fps = 25;
  if (InitVideoSystem(&vs_ctx, fps) != CVI_SUCCESS) {
    printf("failed to init video system\n");
    return CVI_FAILURE;
  }
  // if (InitVideoSystem(&vs_ctx, &aiInputSize, aiInputFormat, voType) != CVI_SUCCESS) {
  //   printf("failed to init video system\n");
  //   return CVI_FAILURE;
  // }
  cvitdl_handle_t tdl_handle = NULL;
  cvitdl_service_handle_t service_handle = NULL;
  cvitdl_app_handle_t app_handle = NULL;
  ret = CVI_TDL_CreateHandle2(&tdl_handle, 1, 0);

  ret = CVI_TDL_Service_CreateHandle(&service_handle, tdl_handle);

  ret |= CVI_TDL_APP_CreateHandle(&app_handle, tdl_handle);

  ret |= CVI_TDL_APP_PersonVehicleCapture_Init(app_handle, (uint32_t)buffer_size);

  ret |= CVI_TDL_APP_PersonVehicleCapture_QuickSetUp(
      app_handle, od_model_name, od_model_path,
      (!strcmp(reid_model_path, "NULL")) ? NULL : reid_model_path);
  ret |= CVI_TDL_APP_PersonVehicleCaptureIrregular_Region(app_handle, h_num, w_num,
                                                          regin_flags);  // set region
  if (ret != CVI_TDL_SUCCESS) {
    printf("failed with %#x!\n", ret);
    goto CLEANUP_SYSTEM;
  }

  pts1.size = 2;
  pts1.x = (float *)malloc(2);
  pts1.y = (float *)malloc(2);
  pts1.x[0] = app_handle->personvehicle_cpt_info->rect.lt_x;
  pts1.y[0] = app_handle->personvehicle_cpt_info->rect.lt_y;

  pts1.x[1] = app_handle->personvehicle_cpt_info->rect.rt_x;
  pts1.y[1] = app_handle->personvehicle_cpt_info->rect.rt_y;

  pts2.size = 2;
  pts2.x = (float *)malloc(2);
  pts2.y = (float *)malloc(2);
  pts2.x[0] = app_handle->personvehicle_cpt_info->rect.lb_x;
  pts2.y[0] = app_handle->personvehicle_cpt_info->rect.lb_y;

  pts2.x[1] = app_handle->personvehicle_cpt_info->rect.rb_x;
  pts2.y[1] = app_handle->personvehicle_cpt_info->rect.rb_y;

  CVI_TDL_SetVpssTimeout(tdl_handle, 1000);
  CVI_TDL_SetModelThreshold(tdl_handle, app_handle->personvehicle_cpt_info->od_model_index,
                            det_threshold);

  personvehicle_capture_config_t app_cfg;
  CVI_TDL_APP_PersonVehicleCapture_GetDefaultConfig(&app_cfg);
  CVI_TDL_APP_PersonVehicleCapture_SetConfig(app_handle, &app_cfg);
  pthread_t vo_thread;
  pVOArgs vo_args = {0};
  vo_args.voType = voType;
  vo_args.service_handle = service_handle;
  vo_args.vs_ctx = vs_ctx;

  pthread_create(&vo_thread, NULL, pVideoOutput, (void *)&vo_args);
  memset(&g_obj_meta_0, 0, sizeof(cvtdl_object_t));
  VIDEO_FRAME_INFO_S stfdFrame;

  size_t counter = 0;
  while (bExit == false) {
    counter += 1;
    ret = CVI_VPSS_GetChnFrame(vs_ctx.vpssConfigs.vpssGrp, vs_ctx.vpssConfigs.vpssChntdl,
                               &stfdFrame, 2000);

    if (ret != CVI_SUCCESS) {
      printf("CVI_VPSS_GetChnFrame chn0 failed with %#x\n", ret);
      break;
    }
    ret = CVI_TDL_APP_PersonVehicleCaptureIrregular_Run(app_handle, &stfdFrame);

    if (ret != CVI_SUCCESS) {
      printf("failed to run consumer counting\n");
      return ret;
    }
    {
      SMT_MutexAutoLock(VOMutex, lock);
      CVI_TDL_Free(&g_obj_meta_0);
      CVI_TDL_Free(&g_obj_meta_1);
      RESTRUCTURING_OBJ_META(&app_handle->personvehicle_cpt_info->last_objects, &g_obj_meta_0);
    }
    ret = CVI_VPSS_ReleaseChnFrame(vs_ctx.vpssConfigs.vpssGrp, vs_ctx.vpssConfigs.vpssChntdl,
                                   &stfdFrame);
    if (ret != CVI_SUCCESS) {
      printf("CVI_VPSS_ReleaseChnFrame chn0 NG\n");
      break;
    }
    // ******************  There is a target crossing the boundary here ****************
    if (cross_mark)
      printf("true\n");
    else
      printf("false\n");
    cross_mark = false;
    // CVI_TDL_Free(&app_handle->person_cpt_info->last_ped);
    // CVI_TDL_Free(&app_handle->person_cpt_info->last_head);
    // CVI_TDL_Free(&app_handle->person_cpt_info->last_objects);
  }
  bRunVideoOutput = false;
  pthread_join(vo_thread, NULL);
CLEANUP_SYSTEM:
  CVI_TDL_Service_DestroyHandle(service_handle);
  CVI_TDL_DestroyHandle(tdl_handle);
  DestroyVideoSystem(&vs_ctx);
  CVI_SYS_Exit();
  CVI_VB_Exit();
}

#define CHAR_SIZE 64
