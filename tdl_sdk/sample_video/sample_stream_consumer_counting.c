#include "cvi_tdl_app.h"
#include "middleware_utils.h"
#include "sample_utils.h"
#include "vi_vo_utils.h"

#include <core/utils/vpss_helper.h>
#include <sample_comm.h>

#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>

#include "stb_image.h"
#include "stb_image_write.h"

#define OUTPUT_BUFFER_SIZE 10
#define MAX(a, b) ((a) > (b) ? (a) : (b))
static cvtdl_object_t g_obj_meta_0;
static cvtdl_object_t g_obj_meta_1;
static cvtdl_pts_t pts;
MUTEXAUTOLOCK_INIT(VOMutex);
// #define VISUAL_FACE_LANDMARK
// #define USE_OUTPUT_DATA_API

#define SMT_MUTEXAUTOLOCK_INIT(mutex) pthread_mutex_t AUTOLOCK_##mutex = PTHREAD_MUTEX_INITIALIZER;

// #define SMT_MutexAutoLock(mutex, lock)

// __attribute__((always_inline)) inline void AutoUnLock(void *mutex) {
//   pthread_mutex_unlock(*(pthread_mutex_t **)mutex);
// }

typedef struct {
  uint64_t u_id;
  float quality;
  cvtdl_image_t image;
  tracker_state_e state;
  uint32_t counter;
} IOData;

typedef struct {
  // VideoSystemContext vs_ctx;
  CVI_S32 voType;
  SAMPLE_TDL_MW_CONTEXT *p_mv_ctx;
  cvitdl_service_handle_t service_handle;
} pVOArgs;

// SMT_MUTEXAUTOLOCK_INIT(VOMutex);

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

  obj_meta->entry_num = origin_obj->entry_num;
  obj_meta->miss_num = origin_obj->miss_num;

  cvtdl_object_info_t *info_ptr_0 = obj_meta->info;
  for (uint32_t i = 0; i < origin_obj->size; i++) {
    cvtdl_object_info_t **tmp_ptr = &info_ptr_0;
    (*tmp_ptr)->unique_id = origin_obj->info[i].unique_id;
    memcpy(&(*tmp_ptr)->bbox, &origin_obj->info[i].bbox, sizeof(cvtdl_bbox_t));
    *tmp_ptr += 1;
  }
  //  printf("e: %d   m:  %d\n",obj_meta->entry_num,obj_meta->miss_num);
  return;
}

CVI_S32 get_middleware_config(SAMPLE_TDL_MW_CONFIG_S *pstMWConfig) {
  // Video Pipeline of this sample:
  //                                                       +------+
  //                                    CHN0 (VBPool 0)    | VENC |--------> RTSP
  //  +----+      +----------------+---------------------> +------+
  //  | VI |----->| VPSS 0 (DEV 1) |            +-----------------------+
  //  +----+      +----------------+----------> | VPSS 1 (DEV 0) TDL SDK |------------> AI model
  //                            CHN1 (VBPool 1) +-----------------------+  CHN0 (VBPool 2)

  // VI configuration
  //////////////////////////////////////////////////
  // Get VI configurations from ini file.
  CVI_S32 s32Ret = SAMPLE_TDL_Get_VI_Config(&pstMWConfig->stViConfig);
  if (s32Ret != CVI_SUCCESS || pstMWConfig->stViConfig.s32WorkingViNum <= 0) {
    printf("Failed to get senor infomation from ini file (/mnt/data/sensor_cfg.ini).\n");
    return -1;
  }

  // Get VI size
  PIC_SIZE_E enPicSize;
  s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(pstMWConfig->stViConfig.astViInfo[0].stSnsInfo.enSnsType,
                                          &enPicSize);
  if (s32Ret != CVI_SUCCESS) {
    printf("Cannot get senor size\n");
    return s32Ret;
  }

  SIZE_S stSensorSize;
  s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSensorSize);
  if (s32Ret != CVI_SUCCESS) {
    printf("Cannot get senor size\n");
    return s32Ret;
  }

  // Setup frame size of video encoder to 1080p
  SIZE_S stVencSize = {
      .u32Width = 1920,
      .u32Height = 1080,
  };

  // VBPool configurations
  //////////////////////////////////////////////////
  pstMWConfig->stVBPoolConfig.u32VBPoolCount = 3;

  // VBPool 0 for VI
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[0].enFormat = VI_PIXEL_FORMAT;
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[0].u32BlkCount = 3;
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[0].u32Height = stSensorSize.u32Height;
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[0].u32Width = stSensorSize.u32Width;
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[0].bBind = true;
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[0].u32VpssChnBinding = VPSS_CHN0;
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[0].u32VpssGrpBinding = (VPSS_GRP)0;

  // VBPool 1 for TDL frame
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[1].enFormat = VI_PIXEL_FORMAT;
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[1].u32BlkCount = 3;
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[1].u32Height = stVencSize.u32Height;
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[1].u32Width = stVencSize.u32Width;
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[1].bBind = true;
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[1].u32VpssChnBinding = VPSS_CHN1;
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[1].u32VpssGrpBinding = (VPSS_GRP)0;

  // VBPool 2 for TDL preprocessing.
  // The input pixel format of TDL SDK models is eighter RGB 888 or RGB 888 Planar.
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[2].enFormat = PIXEL_FORMAT_RGB_888_PLANAR;
  // TDL SDK use only 1 buffer at the same time.
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[2].u32BlkCount = 1;
  // Considering the maximum input size of object detection model is 1024x768, we set same size
  // here.
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[2].u32Height = 768;
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[2].u32Width = 1024;
  // Don't bind with VPSS here, TDL SDK would bind this pool automatically when user assign this
  // pool through CVI_TDL_SetVBPool.
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[2].bBind = false;

  // VPSS configurations
  //////////////////////////////////////////////////

  // Create a VPSS Grp0 for main stream, video encoder, and TDL frame.
  pstMWConfig->stVPSSPoolConfig.u32VpssGrpCount = 1;
#ifndef __CV186X__
  pstMWConfig->stVPSSPoolConfig.stVpssMode.aenInput[0] = VPSS_INPUT_MEM;
  pstMWConfig->stVPSSPoolConfig.stVpssMode.enMode = VPSS_MODE_DUAL;
  pstMWConfig->stVPSSPoolConfig.stVpssMode.ViPipe[0] = 0;
  pstMWConfig->stVPSSPoolConfig.stVpssMode.aenInput[1] = VPSS_INPUT_ISP;
  pstMWConfig->stVPSSPoolConfig.stVpssMode.ViPipe[1] = 0;
#endif

  SAMPLE_TDL_VPSS_CONFIG_S *pstVpssConfig = &pstMWConfig->stVPSSPoolConfig.astVpssConfig[0];
  pstVpssConfig->bBindVI = true;

  // Assign device 1 to VPSS Grp0, because device1 has 3 outputs in dual mode.
  VPSS_GRP_DEFAULT_HELPER2(&pstVpssConfig->stVpssGrpAttr, stSensorSize.u32Width,
                           stSensorSize.u32Height, VI_PIXEL_FORMAT, 1);

  // Enable two channels for VENC and TDL frame
  pstVpssConfig->u32ChnCount = 2;

  // Bind VPSS Grp0 Ch0 with VI
  pstVpssConfig->u32ChnBindVI = VPSS_CHN0;
  VPSS_CHN_DEFAULT_HELPER(&pstVpssConfig->astVpssChnAttr[0], stVencSize.u32Width,
                          stVencSize.u32Height, VI_PIXEL_FORMAT, true);
  VPSS_CHN_DEFAULT_HELPER(&pstVpssConfig->astVpssChnAttr[1], stVencSize.u32Width,
                          stVencSize.u32Height, VI_PIXEL_FORMAT, true);

  // VENC
  //////////////////////////////////////////////////
  // Get default VENC configurations
  SAMPLE_TDL_Get_Input_Config(&pstMWConfig->stVencConfig.stChnInputCfg);
  pstMWConfig->stVencConfig.u32FrameWidth = stVencSize.u32Width;
  pstMWConfig->stVencConfig.u32FrameHeight = stVencSize.u32Height;

  // RTSP
  //////////////////////////////////////////////////
  // Get default RTSP configurations
  SAMPLE_TDL_Get_RTSP_Config(&pstMWConfig->stRTSPConfig.stRTSPConfig);

  return s32Ret;
}

static void *pVideoOutput(void *args) {
  printf("[APP] Video Output Up\n");
  pVOArgs *vo_args = (pVOArgs *)args;

  cvitdl_service_handle_t service_handle = vo_args->service_handle;
  CVI_S32 s32Ret = CVI_SUCCESS;

  cvtdl_service_brush_t brush_0 = {.size = 4, .color.r = 0, .color.g = 64, .color.b = 255};
  cvtdl_service_brush_t brush_1 = {.size = 4, .color.r = 64, .color.g = 0, .color.b = 255};
  cvtdl_service_brush_t brush_2 = {.size = 4, .color.r = 255, .color.g = 0, .color.b = 0};

  cvtdl_object_t obj_meta_0, obj_meta_1;
  memset(&obj_meta_0, 0, sizeof(cvtdl_object_t));
  memset(&obj_meta_1, 0, sizeof(cvtdl_object_t));
  int grp_id = 0;
  int vo_chn = VPSS_CHN1;
  VIDEO_FRAME_INFO_S stVOFrame;
  // vo_args->vs_ctx.outputContext.type = OUTPUT_TYPE_RTSP;
  while (bRunVideoOutput) {
    s32Ret = CVI_VPSS_GetChnFrame(grp_id, vo_chn, &stVOFrame, 1000);
    if (s32Ret != CVI_SUCCESS) {
      printf("CVI_VPSS_GetChnFrame chn0 failed with %#x\n", s32Ret);
      // break;
      usleep(1000);
      continue;
    }
    char *en = calloc(32, sizeof(char));
    char *mi = calloc(32, sizeof(char));
    {
      MutexAutoLock(VOMutex, lock);
      sprintf(en, "Number of entering: %d", g_obj_meta_0.entry_num);
      sprintf(mi, "Number of leaving: %d", g_obj_meta_0.miss_num);

      // memcpy(&obj_meta_0, &g_obj_meta_0, sizeof(cvtdl_object_t));
      // memcpy(&obj_meta_1, &g_obj_meta_1, sizeof(cvtdl_object_t));
      CVI_TDL_CopyObjectMeta(&g_obj_meta_0, &obj_meta_0);
      CVI_TDL_CopyObjectMeta(&g_obj_meta_1, &obj_meta_1);

      obj_meta_0.info =
          (cvtdl_object_info_t *)malloc(sizeof(cvtdl_object_info_t) * g_obj_meta_0.size);
      memset(obj_meta_0.info, 0, sizeof(cvtdl_object_info_t) * obj_meta_0.size);
      for (uint32_t i = 0; i < g_obj_meta_0.size; i++) {
        obj_meta_0.info[i].unique_id = MAX(0, g_obj_meta_0.info[i].unique_id);
        memcpy(&obj_meta_0.info[i].bbox, &g_obj_meta_0.info[i].bbox, sizeof(cvtdl_bbox_t));
      }
      obj_meta_0.entry_num = g_obj_meta_0.entry_num;
      obj_meta_0.miss_num = g_obj_meta_0.miss_num;

      obj_meta_1.info =
          (cvtdl_object_info_t *)malloc(sizeof(cvtdl_object_info_t) * g_obj_meta_1.size);
      memset(obj_meta_1.info, 0, sizeof(cvtdl_object_info_t) * obj_meta_1.size);
      for (uint32_t i = 0; i < g_obj_meta_1.size; i++) {
        obj_meta_1.info[i].unique_id = MAX(0, g_obj_meta_1.info[i].unique_id);
        memcpy(&obj_meta_1.info[i].bbox, &g_obj_meta_1.info[i].bbox, sizeof(cvtdl_bbox_t));
      }
      obj_meta_1.entry_num = g_obj_meta_1.entry_num;
      obj_meta_1.miss_num = g_obj_meta_1.miss_num;
    }
    size_t image_size = stVOFrame.stVFrame.u32Length[0] + stVOFrame.stVFrame.u32Length[1] +
                        stVOFrame.stVFrame.u32Length[2];
    stVOFrame.stVFrame.pu8VirAddr[0] =
        (uint8_t *)CVI_SYS_Mmap(stVOFrame.stVFrame.u64PhyAddr[0], image_size);
    stVOFrame.stVFrame.pu8VirAddr[1] =
        stVOFrame.stVFrame.pu8VirAddr[0] + stVOFrame.stVFrame.u32Length[0];
    stVOFrame.stVFrame.pu8VirAddr[2] =
        stVOFrame.stVFrame.pu8VirAddr[1] + stVOFrame.stVFrame.u32Length[1];
    CVI_TDL_Service_ObjectDrawRect(service_handle, &obj_meta_0, &stVOFrame, false, brush_0);
    CVI_TDL_Service_ObjectDrawRect(service_handle, &obj_meta_1, &stVOFrame, false, brush_1);
    CVI_TDL_Service_DrawPolygon(service_handle, &stVOFrame, &pts, brush_2);

    for (uint32_t j = 0; j < obj_meta_0.size; j++) {
      char *id_num = calloc(64, sizeof(char));
      sprintf(id_num, "%" PRIu64, obj_meta_0.info[j].unique_id);
      CVI_TDL_Service_ObjectWriteText(id_num, obj_meta_0.info[j].bbox.x1,
                                      obj_meta_0.info[j].bbox.y1, &stVOFrame, 1, 1, 1);
      free(id_num);
    }

    for (uint32_t j = 0; j < obj_meta_1.size; j++) {
      char *id_num = calloc(64, sizeof(char));
      sprintf(id_num, "%" PRIu64, obj_meta_1.info[j].unique_id);
      CVI_TDL_Service_ObjectWriteText(id_num, obj_meta_1.info[j].bbox.x1,
                                      obj_meta_1.info[j].bbox.y1, &stVOFrame, 1, 1, 1);
      free(id_num);
    }

    char *num_person = calloc(32, sizeof(char));
    sprintf(num_person, "Number of people: %d", obj_meta_0.size);
    CVI_TDL_Service_ObjectWriteText(num_person, 200, 100, &stVOFrame, 2, 2, 2);

    // sprintf(num_person, "Number of entering: %d", obj_meta_0.entry_num);
    CVI_TDL_Service_ObjectWriteText(en, 200, 200, &stVOFrame, 2, 2, 2);

    // sprintf(num_person, "Number of leaving: %d", obj_meta_0.miss_num);
    CVI_TDL_Service_ObjectWriteText(mi, 200, 300, &stVOFrame, 2, 2, 2);

    free(num_person);
    free(en);
    free(mi);
    CVI_SYS_Munmap((void *)stVOFrame.stVFrame.pu8VirAddr[0], image_size);
    stVOFrame.stVFrame.pu8VirAddr[0] = NULL;
    stVOFrame.stVFrame.pu8VirAddr[1] = NULL;
    stVOFrame.stVFrame.pu8VirAddr[2] = NULL;

    s32Ret = SAMPLE_TDL_Send_Frame_RTSP(&stVOFrame, vo_args->p_mv_ctx);

    // s32Ret = SendOutputFrame(&stVOFrame, &vo_args->vs_ctx.outputContext);
    if (s32Ret != CVI_SUCCESS) {
      printf("Send Output Frame NG\n");
    }
    // ***********************  todo **************************************
    // s32Ret = CVI_VPSS_ReleaseChnFrame(grp_id,
    //                                   VPSS_CHN1, &stVOFrame);
    // if (s32Ret != CVI_SUCCESS) {
    //   printf("CVI_VPSS_ReleaseChnFrame chn0 NG\n");
    //   break;
    // }
    s32Ret = CVI_VPSS_ReleaseChnFrame(grp_id, vo_chn, &stVOFrame);
    if (s32Ret != CVI_SUCCESS) {
      printf("CVI_VPSS_ReleaseChnFrame chn0 NG\n");
      break;
    }

    CVI_TDL_Free(&obj_meta_0);
    CVI_TDL_Free(&obj_meta_1);
  }
  return NULL;
}

int main(int argc, char *argv[]) {
  // Set signal catch
  signal(SIGINT, SampleHandleSig);
  signal(SIGTERM, SampleHandleSig);
  CVI_S32 ret = CVI_SUCCESS;

  const char *od_model_path = argv[1];
  float det_threshold = atof(argv[2]);
  statistics_mode s_mode =
      atoi(argv[3]);  // //  0 ->DOWN_UP: from down to up; 1 -> UP_DOWN: from up to down
  // line coordinate
  int A_x = atoi(argv[4]);
  int A_y = atoi(argv[5]);
  int B_x = atoi(argv[6]);
  int B_y = atoi(argv[7]);
  const char *od_model_name = "yolov8";
  const char *reid_model_path = "NULL";
  int buffer_size = 10;
  int voType = 2;
  // VideoSystemContext vs_ctx = {0};
  // int fps = 25;
  // if (InitVideoSystem(&vs_ctx, fps) != CVI_SUCCESS) {
  //   printf("failed to init video system\n");
  //   return CVI_FAILURE;
  // }
  //  Step 1: Initialize middleware stuff.
  ////////////////////////////////////////////////////

  // Get middleware configurations including VI, VB, VPSS
  SAMPLE_TDL_MW_CONFIG_S stMWConfig = {0};
  CVI_S32 s32Ret = get_middleware_config(&stMWConfig);
  if (s32Ret != CVI_SUCCESS) {
    printf("get middleware configuration failed! ret=%x\n", s32Ret);
    return -1;
  }

  // Initialize middleware.
  SAMPLE_TDL_MW_CONTEXT stMWContext = {0};
  s32Ret = SAMPLE_TDL_Init_WM(&stMWConfig, &stMWContext);
  if (s32Ret != CVI_SUCCESS) {
    printf("init middleware failed! ret=%x\n", s32Ret);
    return -1;
  }

  cvitdl_handle_t tdl_handle = NULL;
  cvitdl_service_handle_t service_handle = NULL;
  cvitdl_app_handle_t app_handle = NULL;
  ret = CVI_TDL_CreateHandle2(&tdl_handle, 1, 0);
  ret = CVI_TDL_SetVBPool(tdl_handle, 0, 2);
  ret = CVI_TDL_Service_CreateHandle(&service_handle, tdl_handle);

  ret |= CVI_TDL_APP_CreateHandle(&app_handle, tdl_handle);

  ret |= CVI_TDL_APP_PersonCapture_Init(app_handle, (uint32_t)buffer_size);

  ret |= CVI_TDL_APP_PersonCapture_QuickSetUp(
      app_handle, od_model_name, od_model_path,
      (!strcmp(reid_model_path, "NULL")) ? NULL : reid_model_path);
  ret |= CVI_TDL_APP_ConsumerCounting_Line(app_handle, A_x, A_y, B_x, B_y, s_mode);  // draw line
  if (ret != CVI_SUCCESS) {
    printf("failed with %#x!\n", ret);
    goto CLEANUP_SYSTEM;
  }

  pts.size = 2;
  pts.x = (float *)malloc(2);
  pts.y = (float *)malloc(2);
  pts.x[0] =
      (app_handle->person_cpt_info->rect.lt_x + app_handle->person_cpt_info->rect.lb_x) / 2.0;
  pts.y[0] =
      (app_handle->person_cpt_info->rect.lt_y + app_handle->person_cpt_info->rect.lb_y) / 2.0;

  pts.x[1] =
      (app_handle->person_cpt_info->rect.rt_x + app_handle->person_cpt_info->rect.rb_x) / 2.0;
  pts.y[1] =
      (app_handle->person_cpt_info->rect.rt_y + app_handle->person_cpt_info->rect.rb_y) / 2.0;
  CVI_TDL_SetVpssTimeout(tdl_handle, 1000);
  CVI_TDL_SetModelThreshold(tdl_handle, app_handle->person_cpt_info->od_model_index, det_threshold);

  person_capture_config_t app_cfg;
  CVI_TDL_APP_PersonCapture_GetDefaultConfig(&app_cfg);
  CVI_TDL_APP_PersonCapture_SetConfig(app_handle, &app_cfg);

  pthread_t vo_thread;
  pVOArgs vo_args = {0};
  vo_args.voType = voType;
  vo_args.service_handle = service_handle;
  vo_args.p_mv_ctx = &stMWContext;

  pthread_create(&vo_thread, NULL, pVideoOutput, (void *)&vo_args);
  memset(&g_obj_meta_0, 0, sizeof(cvtdl_object_t));
  memset(&g_obj_meta_1, 0, sizeof(cvtdl_object_t));

  VIDEO_FRAME_INFO_S stfdFrame;
  size_t counter = 0;
  int grp_id = 0;
  int tdl_chn = VPSS_CHN0;
  while (bExit == false) {
    counter += 1;
    ret = CVI_VPSS_GetChnFrame(grp_id, tdl_chn, &stfdFrame, 2000);
    if (ret != CVI_SUCCESS) {
      printf("CVI_VPSS_GetChnFrame chn0 failed with %#x\n", ret);
      usleep(1000);
      continue;
      // break;
    }

    ret = CVI_TDL_APP_ConsumerCounting_Run(app_handle, &stfdFrame);
    if (ret != CVI_SUCCESS) {
      printf("failed to run consumer counting\n");
      return ret;
    }
    // printf("num person:%d\n", app_handle->person_cpt_info->last_head.size);
    {
      MutexAutoLock(VOMutex, lock);
      CVI_TDL_Free(&g_obj_meta_0);
      CVI_TDL_Free(&g_obj_meta_1);
      RESTRUCTURING_OBJ_META(&app_handle->person_cpt_info->last_head, &g_obj_meta_0);
      RESTRUCTURING_OBJ_META(&app_handle->person_cpt_info->last_ped, &g_obj_meta_1);
    }

    ret = CVI_VPSS_ReleaseChnFrame(grp_id, tdl_chn, &stfdFrame);
    if (ret != CVI_SUCCESS) {
      printf("CVI_VPSS_ReleaseChnFrame chn0 NG\n");
      break;
    }

    // CVI_TDL_Free(&app_handle->person_cpt_info->last_ped);
    // CVI_TDL_Free(&app_handle->person_cpt_info->last_head);
    // CVI_TDL_Free(&app_handle->person_cpt_info->last_objects);
  }

  bRunVideoOutput = false;
  pthread_join(vo_thread, NULL);
CLEANUP_SYSTEM:
  CVI_TDL_Service_DestroyHandle(service_handle);
  CVI_TDL_DestroyHandle(tdl_handle);
  SAMPLE_TDL_Destroy_MW(&stMWContext);
  CVI_SYS_Exit();
  CVI_VB_Exit();
}

#define CHAR_SIZE 64
