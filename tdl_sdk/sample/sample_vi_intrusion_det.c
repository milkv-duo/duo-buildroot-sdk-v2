#define LOG_TAG "SampleInstrusionDet"
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

static volatile bool bExit = false;

static cvtdl_object_t g_stObjMeta = {0};
static bool *g_abIntrusion = NULL;

MUTEXAUTOLOCK_INIT(ResultMutex);

/**
 * @brief Arguments for video encoder thread
 *
 */
typedef struct {
  SAMPLE_TDL_MW_CONTEXT *pstMWContext;
  cvitdl_service_handle_t stServiceHandle;
} SAMPLE_TDL_VENC_THREAD_ARG_S;

/**
 * @brief Arguments for ai thread
 *
 */
typedef struct {
  ODInferenceFunc inference_func;
  CVI_TDL_SUPPORTED_MODEL_E enOdModelId;
  cvitdl_handle_t stTDLHandle;
  cvitdl_service_handle_t stServiceHandle;
} SAMPLE_TDL_TDL_THREAD_ARG_S;

void *run_venc(void *args) {
  printf("Enter encoder thread\n");
  SAMPLE_TDL_VENC_THREAD_ARG_S *pstArgs = (SAMPLE_TDL_VENC_THREAD_ARG_S *)args;
  VIDEO_FRAME_INFO_S stFrame;
  CVI_S32 s32Ret;
  cvtdl_object_t stObjMeta = {0};
  bool *abIntrusion = NULL;

  cvtdl_service_brush_t stRedBrush = CVI_TDL_Service_GetDefaultBrush();
  stRedBrush.color.r = 255;
  stRedBrush.color.g = 0;
  stRedBrush.color.b = 0;

  cvtdl_service_brush_t stRegionBrush = CVI_TDL_Service_GetDefaultBrush();
  stRegionBrush.color.r = 0;
  stRegionBrush.color.g = 255;
  stRegionBrush.color.b = 255;

  // Get the vertices of convex we stored by using CVI_TDL_Service_Polygon_SetTarget.
  cvtdl_pts_t **pastConvexPts = NULL;
  uint32_t u32ConvexNum;
  s32Ret =
      CVI_TDL_Service_Polygon_GetTarget(pstArgs->stServiceHandle, &pastConvexPts, &u32ConvexNum);
  if (s32Ret != CVI_TDL_SUCCESS) {
    printf("Cannot get polygon target\n");
    pthread_exit(NULL);
  }

  while (bExit == false) {
    s32Ret = CVI_VPSS_GetChnFrame(0, VPSS_CHN0, &stFrame, 2000);
    if (s32Ret != CVI_SUCCESS) {
      printf("CVI_VPSS_GetChnFrame chn0 failed with %#x\n", s32Ret);
      break;
    }

    {
      // Get detection result from global
      MutexAutoLock(ResultMutex, lock);
      CVI_TDL_CopyObjectMeta(&g_stObjMeta, &stObjMeta);
      abIntrusion = (bool *)malloc(stObjMeta.size * sizeof(bool));
      memcpy(abIntrusion, g_abIntrusion, stObjMeta.size * sizeof(bool));
    }

    // Draw pre-defined regions
    for (uint32_t i = 0; i < u32ConvexNum; i++) {
      CVI_TDL_Service_DrawPolygon(pstArgs->stServiceHandle, &stFrame, pastConvexPts[i],
                                  stRegionBrush);
    }

    cvtdl_service_brush_t *brushes = malloc(stObjMeta.size * sizeof(cvtdl_service_brush_t));
    for (uint32_t oid = 0; oid < stObjMeta.size; oid++) {
      // Draw red bbox if intrusion with predefined polygons. Otherwise, draw with default color.
      brushes[oid] = abIntrusion[oid] ? stRedBrush : CVI_TDL_Service_GetDefaultBrush();
    }

    // Draw intrusion and non-intrusion result
    GOTO_IF_FAILED(CVI_TDL_Service_ObjectDrawRect2(pstArgs->stServiceHandle, &stObjMeta, &stFrame,
                                                   false, brushes),
                   s32Ret, error);

    // Encode frame and send to RTSP
    SAMPLE_TDL_Send_Frame_RTSP(&stFrame, pstArgs->pstMWContext);
  error:
    free(brushes);
    free(abIntrusion);
    CVI_TDL_Free(&stObjMeta);
    CVI_VPSS_ReleaseChnFrame(0, 0, &stFrame);
    if (s32Ret != CVI_SUCCESS) {
      bExit = true;
    }
  }

  free(pastConvexPts);
  printf("Exit encoder thread\n");
  pthread_exit(NULL);
}

void setup_regions(cvitdl_service_handle_t *stServiceHandle) {
  // This sample setup three regions. Any type of polygon is acceptable including convex and
  // non-convex.
  cvtdl_pts_t test_region_0;
  cvtdl_pts_t test_region_1;
  cvtdl_pts_t test_region_2;

  float r0[2][8] = {{0, 50, 0, 100, 200, 150, 200, 100}, {0, 100, 200, 150, 200, 100, 0, 50}};
  float r1[2][5] = {{380, 560, 500, 320, 260}, {160, 250, 500, 580, 220}};
  float r2[2][4] = {{780, 880, 840, 675}, {400, 420, 620, 580}};

  // Region 0
  test_region_0.size = (uint32_t)sizeof(r0) / (sizeof(float) * 2);
  test_region_0.x = malloc(sizeof(float) * test_region_0.size);
  test_region_0.y = malloc(sizeof(float) * test_region_0.size);
  memcpy(test_region_0.x, r0[0], sizeof(float) * test_region_0.size);
  memcpy(test_region_0.y, r0[1], sizeof(float) * test_region_0.size);

  // Region 1
  test_region_1.size = (uint32_t)sizeof(r1) / (sizeof(float) * 2);
  test_region_1.x = malloc(sizeof(float) * test_region_1.size);
  test_region_1.y = malloc(sizeof(float) * test_region_1.size);
  memcpy(test_region_1.x, r1[0], sizeof(float) * test_region_1.size);
  memcpy(test_region_1.y, r1[1], sizeof(float) * test_region_1.size);

  // Region 2
  test_region_2.size = (uint32_t)sizeof(r2) / (sizeof(float) * 2);
  test_region_2.x = malloc(sizeof(float) * test_region_2.size);
  test_region_2.y = malloc(sizeof(float) * test_region_2.size);
  memcpy(test_region_2.x, r2[0], sizeof(float) * test_region_2.size);
  memcpy(test_region_2.y, r2[1], sizeof(float) * test_region_2.size);

  // Set regions to TDL SDK.
  CVI_TDL_Service_Polygon_SetTarget(stServiceHandle, &test_region_0);
  CVI_TDL_Service_Polygon_SetTarget(stServiceHandle, &test_region_1);
  CVI_TDL_Service_Polygon_SetTarget(stServiceHandle, &test_region_2);

  CVI_TDL_Free(&test_region_0);
  CVI_TDL_Free(&test_region_1);
  CVI_TDL_Free(&test_region_2);
}

void *run_tdl_thread(void *args) {
  printf("Enter TDL thread\n");
  SAMPLE_TDL_TDL_THREAD_ARG_S *pstTDLArgs = (SAMPLE_TDL_TDL_THREAD_ARG_S *)args;
  VIDEO_FRAME_INFO_S stFrame;
  cvtdl_object_t stObjMeta = {0};

  CVI_S32 s32Ret;
  uint32_t counter = 0;
  while (bExit == false) {
    s32Ret = CVI_VPSS_GetChnFrame(0, VPSS_CHN1, &stFrame, 2000);

    if (s32Ret != CVI_SUCCESS) {
      printf("CVI_VPSS_GetChnFrame failed with %#x\n", s32Ret);
      goto get_frame_failed;
    }

    // Detect objects first.
    GOTO_IF_FAILED(pstTDLArgs->inference_func(pstTDLArgs->stTDLHandle, &stFrame,
                                              pstTDLArgs->enOdModelId, &stObjMeta),
                   s32Ret, inf_error);
    struct timeval t0, t1;
    gettimeofday(&t0, NULL);
    bool *aIntrusion = NULL;
    if (stObjMeta.size > 0) {
      aIntrusion = (bool *)malloc(stObjMeta.size * sizeof(bool));
      uint32_t u32IntrusionCount = 0;

      // Check which bbox has intersection with pre-defined regions.
      for (uint32_t i = 0; i < stObjMeta.size; i++) {
        bool bIntrusion;
        cvtdl_bbox_t stBbox = stObjMeta.info[i].bbox;
        GOTO_IF_FAILED(
            CVI_TDL_Service_Polygon_Intersect(pstTDLArgs->stServiceHandle, &stBbox, &bIntrusion),
            s32Ret, inter_error);
        aIntrusion[i] = bIntrusion;
        if (bIntrusion) {
          u32IntrusionCount++;
        }
      }
    }
    gettimeofday(&t1, NULL);
    unsigned long execution_time = ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec);
    if (counter++ % 5 == 0) {
      printf("object count: %d,intrusion_detect take %.2f ms,%u\n", stObjMeta.size,
             (float)execution_time / 1000, stFrame.stVFrame.u32Width);
    }
    {
      // Copy object detection results to global.
      MutexAutoLock(ResultMutex, lock);
      CVI_TDL_CopyObjectMeta(&stObjMeta, &g_stObjMeta);
      free(g_abIntrusion);
      g_abIntrusion = (bool *)malloc(stObjMeta.size * sizeof(bool));
      memcpy(g_abIntrusion, aIntrusion, stObjMeta.size * sizeof(bool));
    }

  inter_error:
    free(aIntrusion);
  inf_error:
    CVI_VPSS_ReleaseChnFrame(0, 1, &stFrame);
  get_frame_failed:
    CVI_TDL_Free(&stObjMeta);
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

static void SampleHandleSig(CVI_S32 signo) {
  signal(SIGINT, SIG_IGN);
  signal(SIGTERM, SIG_IGN);
  printf("handle signal, signo: %d\n", signo);
  if (SIGINT == signo || SIGTERM == signo) {
    bExit = true;
  }
}

int main(int argc, char *argv[]) {
  if (argc != 4 && argc != 3) {
    printf(
        "\nUsage: %s MODEL_NAME MODEL_PATH [THRESHOLD].\n\n"
        "\tMODEL_NAME, detection model name should be one of {mobiledetv2-person-vehicle, "
        "mobiledetv2-person-pets, "
        "mobiledetv2-coco80, "
        "mobiledetv2-vehicle, "
        "mobiledetv2-pedestrian, "
        "yolov3}.\n"
        "\tMODEL_PATH, cvimodel path.\n"
        "\tTHRESHOLD (optional), threshold for detection model (default: 0.5).\n",
        argv[0]);
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
  s32Ret = SAMPLE_TDL_Init_WM(&stMWConfig, &stMWContext);
  if (s32Ret != CVI_SUCCESS) {
    printf("init middleware failed! ret=%x\n", s32Ret);
    return -1;
  }

  // Step 2: Create and setup TDL SDK
  ///////////////////////////////////////////////////

  // Create TDL handle and assign VPSS Grp1 Device 0 to TDL SDK. VPSS Grp1 is created
  // during initialization of TDL SDK.
  cvitdl_handle_t stTDLHandle = NULL;
  GOTO_IF_FAILED(CVI_TDL_CreateHandle2(&stTDLHandle, 1, 0), s32Ret, create_tdl_fail);

  // Assign VBPool ID 2 to the first VPSS in TDL SDK.
  GOTO_IF_FAILED(CVI_TDL_SetVBPool(stTDLHandle, 0, 2), s32Ret, create_service_fail);

  CVI_TDL_SetVpssTimeout(stTDLHandle, 1000);

  cvitdl_service_handle_t stServiceHandle = NULL;
  GOTO_IF_FAILED(CVI_TDL_Service_CreateHandle(&stServiceHandle, stTDLHandle), s32Ret,
                 create_service_fail);

  // Step 3: Open and setup TDL models
  ///////////////////////////////////////////////////

  // Get inference function pointer and model id of object deteciton according to model name.
  ODInferenceFunc inference_func;
  CVI_TDL_SUPPORTED_MODEL_E enOdModelId;
  if (get_od_model_info(argv[1], &enOdModelId, &inference_func) == CVI_TDL_FAILURE) {
    printf("unsupported model: %s\n", argv[1]);
    return -1;
  }

  GOTO_IF_FAILED(CVI_TDL_OpenModel(stTDLHandle, enOdModelId, argv[2]), s32Ret, setup_tdl_fail);

  if (argc == 4) {
    float fThreshold = atof(argv[3]);
    if (fThreshold < 0.0 || fThreshold > 1.0) {
      printf("wrong threshold value: %f\n", fThreshold);
      s32Ret = CVI_FAILURE;
      goto setup_tdl_fail;
    } else {
      printf("set threshold to %f\n", fThreshold);
    }
    GOTO_IF_FAILED(CVI_TDL_SetModelThreshold(stTDLHandle, enOdModelId, fThreshold), s32Ret,
                   setup_tdl_fail);
  }

  // Select which classes we want to focus.
  GOTO_IF_FAILED(CVI_TDL_SelectDetectClass(stTDLHandle, enOdModelId, 2, CVI_TDL_DET_TYPE_PERSON,
                                           CVI_TDL_DET_GROUP_VEHICLE),
                 s32Ret, setup_tdl_fail);

  // Setup alarm regions
  setup_regions(stServiceHandle);

  // Step 4: Run models in thread.
  ///////////////////////////////////////////////////

  pthread_t stVencThread, stTDLThread;
  SAMPLE_TDL_VENC_THREAD_ARG_S venc_args = {
      .pstMWContext = &stMWContext,
      .stServiceHandle = stServiceHandle,
  };

  SAMPLE_TDL_TDL_THREAD_ARG_S ai_args = {
      .enOdModelId = enOdModelId,
      .inference_func = inference_func,
      .stTDLHandle = stTDLHandle,
      .stServiceHandle = stServiceHandle,
  };

  pthread_create(&stVencThread, NULL, run_venc, &venc_args);
  pthread_create(&stTDLThread, NULL, run_tdl_thread, &ai_args);

  // Thread for video encoder
  pthread_join(stVencThread, NULL);

  // Thread for TDL inference
  pthread_join(stTDLThread, NULL);

setup_tdl_fail:
  CVI_TDL_Service_DestroyHandle(stServiceHandle);
create_service_fail:
  CVI_TDL_DestroyHandle(stTDLHandle);
create_tdl_fail:
  SAMPLE_TDL_Destroy_MW(&stMWContext);

  return 0;
}
