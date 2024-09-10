#define LOG_TAG "SampleOD"
#define LOG_LEVEL LOG_LEVEL_INFO

#include "middleware_utils.h"
#include "sample_utils.h"
#include "vi_vo_utils.h"

#include <core/utils/vpss_helper.h>
#include <cvi_comm.h>
#include <rtsp.h>
#include <sample_comm.h>
#include "cvi_tdl.h"

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "app_ipcam_netctrl.h"
#include "app_ipcam_websocket.h"
#include "tdl_type.h"

static volatile bool bExit = false;
static cvtdl_object_t g_stObjMeta;
static cvtdl_face_t g_stFaceMeta;
MUTEXAUTOLOCK_INIT(ResultMutex);

static SAMPLE_TDL_TYPE g_ai_type = 0;
int clrs[7][3] = {{0, 0, 255},   {0, 255, 0},   {255, 0, 0}, {0, 255, 255},
                  {255, 0, 255}, {255, 255, 0}, {0, 0, 0}};

cvtdl_service_brush_t *get_obj_brush(cvtdl_object_t *p_obj_meta) {
  cvtdl_service_brush_t *p_brush =
      (cvtdl_object_t *)malloc(p_obj_meta->size * sizeof(cvtdl_object_t));
  const int max_clr = 7;
  for (size_t i = 0; i < p_obj_meta->size; i++) {
    int clr_idx = p_obj_meta->info[i].classes % max_clr;
    printf("obj cls:%d\n", p_obj_meta->info[i].classes);
    p_brush[i].color.b = clrs[clr_idx][0];
    p_brush[i].color.g = clrs[clr_idx][1];
    p_brush[i].color.r = clrs[clr_idx][2];
    p_brush[i].size = 4;
  }
  return p_brush;
}

SAMPLE_TDL_TYPE ai_param_get(void) { return g_ai_type; }

void ai_param_set(SAMPLE_TDL_TYPE ai_type) { g_ai_type = ai_type; }

int init_func(cvitdl_handle_t tdl_handle, SAMPLE_TDL_TYPE algo_type) {
  int ret = 0;
  if (algo_type == CVI_TDL_HAND) {
    const char *det_path = "/mnt/data/models/hand.cvimodel";
    ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_HAND_DETECTION, det_path);
    CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_HAND_DETECTION, 0.4);

    const char *cls_model_path = "/mnt/data/models/hand_cls_int8_cv182x.cvimodel";
    ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_HANDCLASSIFICATION, cls_model_path);
  } else if (algo_type == CVI_TDL_OBJECT) {
    const char *det_path = "/mnt/data/models//mobiledetv2-pedestrian-d0-ls-384.cvimodel";
    ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PEDESTRIAN, det_path);
  } else if (algo_type == CVI_TDL_PET) {
    const char *det_path = "/mnt/data/models/pet.cvimodel";
    ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_PERSON_PETS_DETECTION, det_path);
  } else if (algo_type == CVI_TDL_PERSON_VEHICLE) {
    const char *det_path = "/mnt/data/models/person_vehicle.cvimodel";
    ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_PERSON_VEHICLE_DETECTION, det_path);
  } else if (algo_type == CVI_TDL_FACE) {
    const char *det_path = "/mnt/data/models/scrfd_768_432_int8_1x.cvimodel";
    ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_SCRFDFACE, det_path);
    const char *recogntion_path = "/mnt/data/models/cviface-v5-s.cvimodel";
    ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_FACERECOGNITION, recogntion_path);
  } else if (algo_type == CVI_TDL_MEET) {
    const char *det_path = "/mnt/data/models/meet.cvimodel";
    ret =
        CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_HAND_FACE_PERSON_DETECTION, det_path);
    const char *cls_model_path = "/mnt/data/models/hand_cls_int8_cv182x.cvimodel";
    ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_HANDCLASSIFICATION, cls_model_path);
    if (ret != CVI_SUCCESS) {
      printf("failed to open face reg model %s\n", cls_model_path);
      return ret;
    }
  }

  return ret;
}

int release_func(cvitdl_handle_t tdl_handle, SAMPLE_TDL_TYPE algo_type) {
  int ret = 0;
  if (algo_type == CVI_TDL_HAND) {
    CVI_TDL_CloseModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_HAND_DETECTION);
    CVI_TDL_CloseModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_HANDCLASSIFICATION);
  } else if (algo_type == CVI_TDL_OBJECT) {
    CVI_TDL_CloseModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PEDESTRIAN);
  } else if (algo_type == CVI_TDL_PET) {
    CVI_TDL_CloseModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_PERSON_PETS_DETECTION);
  } else if (algo_type == CVI_TDL_PERSON_VEHICLE) {
    CVI_TDL_CloseModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_PERSON_VEHICLE_DETECTION);
  } else if (algo_type == CVI_TDL_FACE) {
    CVI_TDL_CloseModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_SCRFDFACE);
    CVI_TDL_CloseModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_FACERECOGNITION);
  } else if (algo_type == CVI_TDL_MEET) {
    CVI_TDL_CloseModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_HAND_FACE_PERSON_DETECTION);
    CVI_TDL_CloseModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_HANDCLASSIFICATION);
  }
  return ret;
}

int infer_func(cvitdl_handle_t tdl_handle, SAMPLE_TDL_TYPE algo_type, VIDEO_FRAME_INFO_S *ptr_frame,
               cvtdl_object_t *p_stObjMeta, cvtdl_face_t *p_face) {
  int ret = 0;
  if (algo_type == CVI_TDL_HAND) {
    ret = CVI_TDL_Detection(tdl_handle, ptr_frame, CVI_TDL_SUPPORTED_MODEL_HAND_DETECTION,
                            p_stObjMeta);
    if (ret != CVI_TDL_SUCCESS) {
      printf("inference failed!, ret=%x\n", ret);
      return ret;
    }
    ret = CVI_TDL_HandClassification(tdl_handle, ptr_frame, p_stObjMeta);
    if (ret != CVI_TDL_SUCCESS) {
      printf("inference failed!, ret=%x\n", ret);
    }
  } else if (algo_type == CVI_TDL_OBJECT) {
    ret = CVI_TDL_Detection(tdl_handle, ptr_frame, CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PEDESTRIAN,
                            p_stObjMeta);
  } else if (algo_type == CVI_TDL_PET) {
    ret = CVI_TDL_Detection(tdl_handle, ptr_frame, CVI_TDL_SUPPORTED_MODEL_PERSON_PETS_DETECTION,
                            p_stObjMeta);
  } else if (algo_type == CVI_TDL_PERSON_VEHICLE) {
    ret = CVI_TDL_PersonVehicle_Detection(tdl_handle, ptr_frame, p_stObjMeta);
  } else if (algo_type == CVI_TDL_FACE) {
    ret = CVI_TDL_FaceDetection(tdl_handle, ptr_frame, CVI_TDL_SUPPORTED_MODEL_SCRFDFACE, p_face);
    if (p_face->size > 0) {
      ret = CVI_TDL_FaceRecognition(tdl_handle, ptr_frame, p_face);
    }
  } else if (algo_type == CVI_TDL_MEET) {
    ret = CVI_TDL_Detection(tdl_handle, ptr_frame,
                            CVI_TDL_SUPPORTED_MODEL_HAND_FACE_PERSON_DETECTION, p_stObjMeta);
    ret = CVI_TDL_HandClassification(tdl_handle, ptr_frame, p_stObjMeta);
  }
  return ret;
}

/**
 * @brief Arguments for video encoder thread
 *
 */
typedef struct {
  SAMPLE_TDL_MW_CONTEXT *pstMWContext;
  cvitdl_service_handle_t stServiceHandle;
} SAMPLE_TDL_VENC_THREAD_ARG_S;

typedef struct {
  cvitdl_handle_t stAiHandle;
} SAMPLE_TDL_TDL_THREAD_ARG_S;

void *run_venc_thread(void *args) {
  printf("Enter encoder thread\n");
  SAMPLE_TDL_VENC_THREAD_ARG_S *pstArgs = (SAMPLE_TDL_VENC_THREAD_ARG_S *)args;
  VIDEO_FRAME_INFO_S stFrame;
  CVI_S32 s32Ret;
  cvtdl_object_t stObjMeta = {0};
  cvtdl_face_t stFaceMeta = {0};
  SAMPLE_TDL_TYPE ai_type = CVI_TDL_FACE;
  cvtdl_service_brush_t *p_brushes = NULL;
  cvtdl_service_brush_t brush_0 = {.size = 4, .color.r = 0, .color.g = 64, .color.b = 255};
  ;
  char name[256];

  while (bExit == false) {
    s32Ret = CVI_VPSS_GetChnFrame(0, VPSS_CHN0, &stFrame, 2000);
    if (s32Ret != CVI_SUCCESS) {
      printf("CVI_VPSS_GetChnFrame chn0 failed with %#x\n", s32Ret);
      break;
    }

    MutexAutoLock(ResultMutex, lock);
    ai_type = g_ai_type;
    if (ai_type == CVI_TDL_FACE) {
      memset(&stFaceMeta, 0, sizeof(cvtdl_face_t));
      if (g_stFaceMeta.size > 0 && g_stFaceMeta.info != NULL) {
        CVI_TDL_CopyFaceMeta(&g_stFaceMeta, &stFaceMeta);
        for (uint32_t oid = 0; oid < stFaceMeta.size; oid++) {
          sprintf(name, "%s: %.2f", stFaceMeta.info[oid].name, stFaceMeta.info[oid].bbox.score);
          memcpy(stFaceMeta.info[oid].name, name, sizeof(stFaceMeta.info[oid].name));
        }
        s32Ret = CVI_TDL_Service_FaceDrawRect(pstArgs->stServiceHandle, &stFaceMeta, &stFrame,
                                              false, brush_0);
      }
    } else {
      memset(&stObjMeta, 0, sizeof(cvtdl_object_t));
      if (g_stObjMeta.size > 0 && g_stObjMeta.info != NULL) {
        CVI_TDL_CopyObjectMeta(&g_stObjMeta, &stObjMeta);
        for (uint32_t oid = 0; oid < stObjMeta.size; oid++) {
          sprintf(name, "%s: %.2f", stObjMeta.info[oid].name, stObjMeta.info[oid].bbox.score);
          memcpy(stObjMeta.info[oid].name, name, sizeof(stObjMeta.info[oid].name));
        }
        p_brushes = get_obj_brush(&stObjMeta);
        s32Ret = CVI_TDL_Service_ObjectDrawRect2(pstArgs->stServiceHandle, &stObjMeta, &stFrame,
                                                 true, p_brushes);
        free(p_brushes);
      }
    }

    if (s32Ret != CVI_TDL_SUCCESS) {
      CVI_VPSS_ReleaseChnFrame(0, 0, &stFrame);
      printf("Draw fame fail!, ret=%x\n", s32Ret);
      goto error;
    }

    SAMPLE_TDL_Send_Frame_WEB(&stFrame, pstArgs->pstMWContext);
  error:
    if (ai_type == CVI_TDL_FACE) {
      CVI_TDL_Free(&stFaceMeta);
    } else {
      CVI_TDL_Free(&stObjMeta);
    }

    CVI_VPSS_ReleaseChnFrame(0, 0, &stFrame);
    if (s32Ret != CVI_SUCCESS) {
      bExit = true;
    }
  }
  printf("Exit encoder thread\n");
  pthread_exit(NULL);
}

void *run_tdl_thread(void *args) {
  printf("Enter TDL thread\n");
  SAMPLE_TDL_TDL_THREAD_ARG_S *pstTDLArgs = (SAMPLE_TDL_TDL_THREAD_ARG_S *)args;
  VIDEO_FRAME_INFO_S stFrame;

  CVI_S32 s32Ret;
  SAMPLE_TDL_TYPE ai_type = CVI_TDL_FACE;
  int ret = init_func(pstTDLArgs->stAiHandle, CVI_TDL_FACE);
  if (ret != 0) {
    goto inf_error;
  }

  cvtdl_object_t stObjMeta = {0};
  cvtdl_face_t stFaceMeta = {0};
  static uint32_t counter = 0;
  while (bExit == false) {
    if (ai_type != g_ai_type && g_ai_type < CVI_TDL_MAX) {
      release_func(pstTDLArgs->stAiHandle, ai_type);
      init_func(pstTDLArgs->stAiHandle, g_ai_type);
      ai_type = g_ai_type;
    }
    struct timeval t0, t1;
    gettimeofday(&t0, NULL);
    s32Ret = CVI_VPSS_GetChnFrame(0, VPSS_CHN1, &stFrame, 2000);
    if (s32Ret != CVI_SUCCESS) {
      printf("CVI_VPSS_GetChnFrame failed with %#x\n", s32Ret);
      goto get_frame_failed;
    }

    memset(&stFaceMeta, 0, sizeof(cvtdl_face_t));
    memset(&stObjMeta, 0, sizeof(cvtdl_object_t));
    ret = infer_func(pstTDLArgs->stAiHandle, ai_type, &stFrame, &stObjMeta, &stFaceMeta);
    if (ret != 0) {
      goto inf_error;
    }

    if (ai_type == CVI_TDL_FACE) {
      CVI_TDL_CopyFaceMeta(&stFaceMeta, &g_stFaceMeta);
      CVI_TDL_Free(&stFaceMeta);
    } else {
      CVI_TDL_CopyObjectMeta(&stObjMeta, &g_stObjMeta);
      CVI_TDL_Free(&stObjMeta);
    }

    if (s32Ret != CVI_TDL_SUCCESS) {
      printf("inference failed!, ret=%x\n", s32Ret);
      goto inf_error;
    }

    gettimeofday(&t1, NULL);
    unsigned long execution_time = ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec);
    if (counter % 30 == 0) {
      printf("ai thread run take %ld ms\n", execution_time / 1000);
      counter = 0;
    }
    counter++;
  inf_error:
    CVI_VPSS_ReleaseChnFrame(0, VPSS_CHN1, &stFrame);
  get_frame_failed:
    if (s32Ret != CVI_SUCCESS) {
      bExit = true;
    }
  }

  printf("Exit TDL thread\n");
  pthread_exit(NULL);
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
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[1].enFormat = PIXEL_FORMAT_RGB_888_PLANAR;
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
#ifndef CV186X
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
                          stVencSize.u32Height, PIXEL_FORMAT_RGB_888_PLANAR, true);

  // VENC
  //////////////////////////////////////////////////
  // Get default VENC configurations
  SAMPLE_TDL_Get_Input_Config(&pstMWConfig->stVencConfig.stChnInputCfg);
  pstMWConfig->stVencConfig.u32FrameWidth = stVencSize.u32Width;
  pstMWConfig->stVencConfig.u32FrameHeight = stVencSize.u32Height;

  return s32Ret;
}

static void SampleHandleSig(CVI_S32 signo) {
  signal(SIGINT, SIG_IGN);
  signal(SIGTERM, SIG_IGN);
  printf("handle signal, signo: %d\n", signo);
  if (SIGINT == signo || SIGTERM == signo) {
    bExit = true;
  }
}

int main(int argc, char *argv[]) {
  if (argc != 1) {
    printf("\nUsage: %s \n\n", argv[0]);
    return -1;
  }

  signal(SIGINT, SampleHandleSig);
  signal(SIGTERM, SampleHandleSig);

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
  s32Ret = SAMPLE_TDL_Init_WM_NO_RTSP(&stMWConfig, &stMWContext);
  if (s32Ret != CVI_SUCCESS) {
    printf("init middleware failed! ret=%x\n", s32Ret);
    return -1;
  }

  app_ipcam_NetCtrl_Init();
  app_ipcam_WebSocket_Init();

  // Step 2: Create and setup TDL SDK
  ///////////////////////////////////////////////////

  // Create TDL handle and assign VPSS Grp1 Device 0 to TDL SDK. VPSS Grp1 is created
  // during initialization of TDL SDK.
  cvitdl_handle_t stAiHandle = NULL;
  GOTO_IF_FAILED(CVI_TDL_CreateHandle2(&stAiHandle, 1, 0), s32Ret, create_tdl_fail);

  // Assign VBPool ID 2 to the first VPSS in TDL SDK.
  GOTO_IF_FAILED(CVI_TDL_SetVBPool(stAiHandle, 0, 2), s32Ret, create_service_fail);

  cvitdl_service_handle_t stServiceHandle = NULL;
  GOTO_IF_FAILED(CVI_TDL_Service_CreateHandle(&stServiceHandle, stAiHandle), s32Ret,
                 create_service_fail);

  // Step 3: Run models in thread.
  ///////////////////////////////////////////////////

  pthread_t stVencThread, stAiThread;
  SAMPLE_TDL_VENC_THREAD_ARG_S venc_args = {
      .pstMWContext = &stMWContext,
      .stServiceHandle = stServiceHandle,
  };

  SAMPLE_TDL_TDL_THREAD_ARG_S ai_args = {
      .stAiHandle = stAiHandle,
  };

  pthread_create(&stVencThread, NULL, run_venc_thread, &venc_args);
  pthread_create(&stAiThread, NULL, run_tdl_thread, &ai_args);

  // Thread for video encoder
  pthread_join(stVencThread, NULL);
  // Thread for TDL inference
  pthread_join(stAiThread, NULL);

  app_ipcam_WebSocket_DeInit();
  app_ipcam_NetCtrl_DeInit();

  CVI_TDL_Service_DestroyHandle(stServiceHandle);
create_service_fail:
  CVI_TDL_DestroyHandle(stAiHandle);
create_tdl_fail:
  SAMPLE_TDL_Destroy_MW_NO_RTSP(&stMWContext);

  return 0;
}
