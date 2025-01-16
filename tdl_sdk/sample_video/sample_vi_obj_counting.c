/**
 * This is a sample code for object counting. Tracking targets contain person, car.
 */
#define LOG_TAG "SampleObjectCounting"
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

#define WRITE_RESULT_TO_FILE 1
#define TARGET_NUM 2

static volatile bool bExit = false;

MUTEXAUTOLOCK_INIT(ResultMutex);

typedef struct {
  SAMPLE_TDL_MW_CONTEXT *pstMWContext;
  cvitdl_service_handle_t stServiceHandle;
} SAMPLE_TDL_VENC_THREAD_ARG_S;

typedef struct {
  ODInferenceFunc object_detect;
  CVI_TDL_SUPPORTED_MODEL_E enOdModelId;
  cvitdl_handle_t stTDLHandle;
  bool bUseStableCounter;
} SAMPLE_TDL_TDL_THREAD_ARG_S;

typedef struct {
  int classes_id[TARGET_NUM];  // {CVI_TDL_DET_TYPE_PERSON, CVI_TDL_DET_TYPE_CAR, ...}
  uint64_t classes_count[TARGET_NUM];
  uint64_t classes_maxID[TARGET_NUM];
} obj_counter_t;

static cvtdl_object_t g_stObjMeta = {0};
static obj_counter_t g_stObjCounter = {0};

void setSampleMOTConfig(cvtdl_deepsort_config_t *ds_conf) {
  ds_conf->ktracker_conf.accreditation_threshold = 10;
  ds_conf->ktracker_conf.P_beta[2] = 0.01;
  ds_conf->ktracker_conf.P_beta[6] = 1e-5;
  ds_conf->kfilter_conf.Q_beta[2] = 0.01;
  ds_conf->kfilter_conf.Q_beta[6] = 1e-5;
  ds_conf->kfilter_conf.R_beta[2] = 0.1;
}

// Not completed
/* stable tracking counter */
void update_obj_counter_stable(obj_counter_t *obj_counter, cvtdl_object_t *obj_meta,
                               cvtdl_tracker_t *tracker_meta) {
  uint64_t new_maxID[TARGET_NUM];
  uint64_t newID_num[TARGET_NUM];
  memset(newID_num, 0, sizeof(uint64_t) * TARGET_NUM);
  // Add the number of IDs which greater than original maxID.
  // Finally update new maxID for each counter.
  for (int j = 0; j < TARGET_NUM; j++) {
    new_maxID[j] = obj_counter->classes_maxID[j];
  }

  for (uint32_t i = 0; i < obj_meta->size; i++) {
    // Skip the bbox whoes tracker state is not stable
    if (tracker_meta->info[i].state != CVI_TRACKER_STABLE) {
      continue;
    }
    // Find the index of object counter for this class
    int class_index = -1;
    for (int j = 0; j < TARGET_NUM; j++) {
      if (obj_meta->info[i].classes == obj_counter->classes_id[j]) {
        class_index = j;
        break;
      }
    }
    if (obj_meta->info[i].unique_id > obj_counter->classes_maxID[class_index]) {
      newID_num[class_index] += 1;
      if (obj_meta->info[i].unique_id > new_maxID[class_index]) {
        new_maxID[class_index] = obj_meta->info[i].unique_id;
      }
    }
  }

  for (int j = 0; j < TARGET_NUM; j++) {
    obj_counter->classes_count[j] += newID_num[j];
    obj_counter->classes_maxID[j] = new_maxID[j];
  }
}

/* simple tracking counter */
void update_obj_counter_simple(obj_counter_t *obj_counter, cvtdl_object_t *obj_meta) {
  for (uint32_t i = 0; i < obj_meta->size; i++) {
    // Find the index of object counter for this class
    int class_index = -1;
    for (int j = 0; j < TARGET_NUM; j++) {
      if (obj_meta->info[i].classes == obj_counter->classes_id[j]) {
        class_index = j;
        break;
      }
    }
    if (obj_meta->info[i].unique_id > obj_counter->classes_count[class_index]) {
      obj_counter->classes_count[class_index] = obj_meta->info[i].unique_id;
    }
  }
}

void *run_venc(void *args) {
  printf("Enter encoder thread\n");
  SAMPLE_TDL_VENC_THREAD_ARG_S *pstArgs = (SAMPLE_TDL_VENC_THREAD_ARG_S *)args;
  VIDEO_FRAME_INFO_S stFrame;
  CVI_S32 s32Ret;
  cvtdl_object_t stObjMeta = {0};
  obj_counter_t stObjCounter = {0};

  while (bExit == false) {
    s32Ret = CVI_VPSS_GetChnFrame(0, 0, &stFrame, 2000);
    if (s32Ret != CVI_SUCCESS) {
      printf("CVI_VPSS_GetChnFrame chn0 failed with %#x\n", s32Ret);
      break;
    }

    {
      MutexAutoLock(ResultMutex, lock);
      CVI_TDL_CopyObjectMeta(&g_stObjMeta, &stObjMeta);
      memcpy(&stObjCounter, &g_stObjCounter, sizeof(obj_counter_t));
    }

    s32Ret = CVI_TDL_Service_ObjectDrawRect(pstArgs->stServiceHandle, &stObjMeta, &stFrame, false,
                                            CVI_TDL_Service_GetDefaultBrush());
    if (s32Ret != CVI_TDL_SUCCESS) {
      CVI_VPSS_ReleaseChnFrame(0, 0, &stFrame);
      printf("Draw fame fail!, ret=%x\n", s32Ret);
      goto error;
    }

    // draw unique id
    for (uint32_t i = 0; i < stObjMeta.size; i++) {
      char *obj_ID = calloc(64, sizeof(char));
      sprintf(obj_ID, "%" PRIu64 "", stObjMeta.info[i].unique_id);
      CVI_TDL_Service_ObjectWriteText(obj_ID, stObjMeta.info[i].bbox.x1, stObjMeta.info[i].bbox.y1,
                                      &stFrame, -1, -1, -1);
      free(obj_ID);
    }

    // draw counter
    for (int i = 0; i < TARGET_NUM; i++) {
      char *counter_info = calloc(64, sizeof(char));
      sprintf(counter_info, "class: %d, count: %" PRIu64 "", stObjCounter.classes_id[i],
              stObjCounter.classes_count[i]);
      CVI_TDL_Service_ObjectWriteText(counter_info, 0, (i + 1) * 40, &stFrame, -1, -1, -1);
      free(counter_info);
    }

    s32Ret = SAMPLE_TDL_Send_Frame_RTSP(&stFrame, pstArgs->pstMWContext);
    if (s32Ret != CVI_SUCCESS) {
      CVI_VPSS_ReleaseChnFrame(0, 0, &stFrame);
      printf("Send Output Frame NG, ret=%x\n", s32Ret);
      goto error;
    }

  error:
    CVI_TDL_Free(&stObjMeta);
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
  cvtdl_object_t stObjMeta = {0};
  cvtdl_tracker_t stTrackerMeta = {0};
  obj_counter_t stObjCounter = {0};
  stObjCounter.classes_id[0] = CVI_TDL_DET_TYPE_PERSON;
  stObjCounter.classes_id[1] = CVI_TDL_DET_TYPE_CAR;

  CVI_S32 s32Ret;
  while (bExit == false) {
    s32Ret = CVI_VPSS_GetChnFrame(0, VPSS_CHN1, &stFrame, 2000);

    if (s32Ret != CVI_SUCCESS) {
      printf("CVI_VPSS_GetChnFrame failed with %#x\n", s32Ret);
      goto get_frame_failed;
    }

    //*******************************************
    // Step 1: Object detect inference.
    s32Ret = pstTDLArgs->object_detect(pstTDLArgs->stTDLHandle, &stFrame, pstTDLArgs->enOdModelId,
                                       &stObjMeta);
    if (s32Ret != CVI_TDL_SUCCESS) {
      printf("inference failed!, ret=%x\n", s32Ret);
      goto inf_error;
    }

    // Step 2: Extract ReID feature for all person bbox.
    for (uint32_t i = 0; i < stObjMeta.size; i++) {
      if (stObjMeta.info[i].classes == CVI_TDL_DET_TYPE_PERSON) {
        s32Ret = CVI_TDL_OSNetOne(pstTDLArgs->stTDLHandle, &stFrame, &stObjMeta, (int)i);
        if (s32Ret != CVI_TDL_SUCCESS) {
          printf("inference failed!, ret=%x\n", s32Ret);
          goto inf_error;
        }
      }
    }

    // Step 3: Multi-Object Tracking inference.
    s32Ret = CVI_TDL_DeepSORT_Obj(pstTDLArgs->stTDLHandle, &stObjMeta, &stTrackerMeta, false);
    if (s32Ret != CVI_TDL_SUCCESS) {
      printf("inference failed!, ret=%x\n", s32Ret);
      goto inf_error;
    }
    //*******************************************

    if (pstTDLArgs->bUseStableCounter) {
      update_obj_counter_stable(&stObjCounter, &stObjMeta, &stTrackerMeta);
    } else {
      update_obj_counter_simple(&stObjCounter, &stObjMeta);
    }

    for (int i = 0; i < TARGET_NUM; i++) {
      printf("[%d] %" PRIu64 "\n", stObjCounter.classes_id[i], stObjCounter.classes_count[i]);
    }

    {
      MutexAutoLock(ResultMutex, lock);
      CVI_TDL_CopyObjectMeta(&stObjMeta, &g_stObjMeta);
      memcpy(&g_stObjCounter, &stObjCounter, sizeof(obj_counter_t));
    }

  inf_error:
    CVI_VPSS_ReleaseChnFrame(0, 1, &stFrame);
  get_frame_failed:
    CVI_TDL_Free(&stObjMeta);
    CVI_TDL_Free(&stTrackerMeta);
    if (s32Ret != CVI_SUCCESS) {
      bExit = true;
    }
  }

  printf("Exit TDL thread\n");
  pthread_exit(NULL);
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
  if (argc != 5) {
    printf(
        "\nUsage: %s DET_MODEL_NAME DET_MODEL_PATH REID_MODEL_PATH STABLE_COUNTER\n\n"
        "\tDET_MODEL_NAME, detection model name, should be one of {mobiledetv2-person-vehicle, "
        "mobiledetv2-person-pets, "
        "mobiledetv2-coco80, "
        "mobiledetv2-vehicle, "
        "mobiledetv2-pedestrian, "
        "yolov3}.\n"
        "\tDET_MODEL_PATH path to detection model.\n"
        "\tREID_MODEL_PATH path to ReID model.\n"
        "\tSTABLE_COUNTER, use stable counter, should be 0 or 1.\n",
        argv[0]);
    return -1;
  }

  ODInferenceFunc inference_func;
  CVI_TDL_SUPPORTED_MODEL_E enOdModelId;
  if (get_od_model_info(argv[1], &enOdModelId, &inference_func) == CVI_TDL_FAILURE) {
    printf("unsupported model: %s\n", argv[1]);
    return -1;
  }

  signal(SIGINT, SampleHandleSig);
  signal(SIGTERM, SampleHandleSig);

  SAMPLE_TDL_MW_CONFIG_S stMWConfig = {0};

  CVI_S32 s32Ret = SAMPLE_TDL_Get_VI_Config(&stMWConfig.stViConfig);
  if (s32Ret != CVI_SUCCESS || stMWConfig.stViConfig.s32WorkingViNum <= 0) {
    printf("Failed to get senor infomation from ini file (/mnt/data/sensor_cfg.ini).\n");
    return -1;
  }

  // Get VI size
  PIC_SIZE_E enPicSize;
  s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stMWConfig.stViConfig.astViInfo[0].stSnsInfo.enSnsType,
                                          &enPicSize);
  if (s32Ret != CVI_SUCCESS) {
    printf("Cannot get senor size\n");
    return -1;
  }

  SIZE_S stSensorSize;
  s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSensorSize);
  if (s32Ret != CVI_SUCCESS) {
    printf("Cannot get senor size\n");
    return -1;
  }

  // Setup frame size of video encoder to 1080p
  SIZE_S stVencSize = {
      .u32Width = 1920,
      .u32Height = 1080,
  };

  stMWConfig.stVBPoolConfig.u32VBPoolCount = 3;

  // VBPool 0 for VPSS Grp0 Chn0
  stMWConfig.stVBPoolConfig.astVBPoolSetup[0].enFormat = VI_PIXEL_FORMAT;
  stMWConfig.stVBPoolConfig.astVBPoolSetup[0].u32BlkCount = 3;
  stMWConfig.stVBPoolConfig.astVBPoolSetup[0].u32Height = stSensorSize.u32Height;
  stMWConfig.stVBPoolConfig.astVBPoolSetup[0].u32Width = stSensorSize.u32Width;
  stMWConfig.stVBPoolConfig.astVBPoolSetup[0].bBind = true;
  stMWConfig.stVBPoolConfig.astVBPoolSetup[0].u32VpssChnBinding = VPSS_CHN0;
  stMWConfig.stVBPoolConfig.astVBPoolSetup[0].u32VpssGrpBinding = (VPSS_GRP)0;

  // VBPool 1 for VPSS Grp0 Chn1
  stMWConfig.stVBPoolConfig.astVBPoolSetup[1].enFormat = VI_PIXEL_FORMAT;
  stMWConfig.stVBPoolConfig.astVBPoolSetup[1].u32BlkCount = 3;
  stMWConfig.stVBPoolConfig.astVBPoolSetup[1].u32Height = stVencSize.u32Height;
  stMWConfig.stVBPoolConfig.astVBPoolSetup[1].u32Width = stVencSize.u32Width;
  stMWConfig.stVBPoolConfig.astVBPoolSetup[1].bBind = true;
  stMWConfig.stVBPoolConfig.astVBPoolSetup[1].u32VpssChnBinding = VPSS_CHN1;
  stMWConfig.stVBPoolConfig.astVBPoolSetup[1].u32VpssGrpBinding = (VPSS_GRP)0;

  // VBPool 2 for TDL preprocessing
  stMWConfig.stVBPoolConfig.astVBPoolSetup[2].enFormat = PIXEL_FORMAT_BGR_888_PLANAR;
  stMWConfig.stVBPoolConfig.astVBPoolSetup[2].u32BlkCount = 1;
  stMWConfig.stVBPoolConfig.astVBPoolSetup[2].u32Height = 768;
  stMWConfig.stVBPoolConfig.astVBPoolSetup[2].u32Width = 1024;
  stMWConfig.stVBPoolConfig.astVBPoolSetup[2].bBind = false;

  // Setup VPSS Grp0
  stMWConfig.stVPSSPoolConfig.u32VpssGrpCount = 1;
#ifndef __CV186X__
  stMWConfig.stVPSSPoolConfig.stVpssMode.aenInput[0] = VPSS_INPUT_MEM;
  stMWConfig.stVPSSPoolConfig.stVpssMode.enMode = VPSS_MODE_DUAL;
  stMWConfig.stVPSSPoolConfig.stVpssMode.ViPipe[0] = 0;
  stMWConfig.stVPSSPoolConfig.stVpssMode.aenInput[1] = VPSS_INPUT_ISP;
  stMWConfig.stVPSSPoolConfig.stVpssMode.ViPipe[1] = 0;
#endif

  SAMPLE_TDL_VPSS_CONFIG_S *pstVpssConfig = &stMWConfig.stVPSSPoolConfig.astVpssConfig[0];
  pstVpssConfig->bBindVI = true;

  // Assign device 1 to VPSS Grp0, because device1 has 3 outputs in dual mode.
  VPSS_GRP_DEFAULT_HELPER2(&pstVpssConfig->stVpssGrpAttr, stSensorSize.u32Width,
                           stSensorSize.u32Height, VI_PIXEL_FORMAT, 1);
  pstVpssConfig->u32ChnCount = 2;
  pstVpssConfig->u32ChnBindVI = 0;
  VPSS_CHN_DEFAULT_HELPER(&pstVpssConfig->astVpssChnAttr[0], stVencSize.u32Width,
                          stVencSize.u32Height, VI_PIXEL_FORMAT, true);
  VPSS_CHN_DEFAULT_HELPER(&pstVpssConfig->astVpssChnAttr[1], stVencSize.u32Width,
                          stVencSize.u32Height, VI_PIXEL_FORMAT, true);

  // Get default VENC configurations
  SAMPLE_TDL_Get_Input_Config(&stMWConfig.stVencConfig.stChnInputCfg);
  stMWConfig.stVencConfig.u32FrameWidth = stVencSize.u32Width;
  stMWConfig.stVencConfig.u32FrameHeight = stVencSize.u32Height;

  // Get default RTSP configurations
  SAMPLE_TDL_Get_RTSP_Config(&stMWConfig.stRTSPConfig.stRTSPConfig);

  SAMPLE_TDL_MW_CONTEXT stMWContext = {0};
  s32Ret = SAMPLE_TDL_Init_WM(&stMWConfig, &stMWContext);
  if (s32Ret != CVI_SUCCESS) {
    printf("init middleware failed! ret=%x\n", s32Ret);
    return -1;
  }

  cvitdl_handle_t stTDLHandle = NULL;

  // Create TDL handle and assign VPSS Grp1 Device 0 to TDL SDK
  GOTO_IF_FAILED(CVI_TDL_CreateHandle2(&stTDLHandle, 1, 0), s32Ret, create_tdl_fail);

  GOTO_IF_FAILED(CVI_TDL_SetVBPool(stTDLHandle, 0, 2), s32Ret, create_service_fail);

  CVI_TDL_SetVpssTimeout(stTDLHandle, 1000);

  cvitdl_service_handle_t stServiceHandle = NULL;
  GOTO_IF_FAILED(CVI_TDL_Service_CreateHandle(&stServiceHandle, stTDLHandle), s32Ret,
                 create_service_fail);

  GOTO_IF_FAILED(CVI_TDL_OpenModel(stTDLHandle, enOdModelId, argv[2]), s32Ret, setup_tdl_fail);
  GOTO_IF_FAILED(CVI_TDL_OpenModel(stTDLHandle, CVI_TDL_SUPPORTED_MODEL_OSNET, argv[3]), s32Ret,
                 setup_tdl_fail);

  GOTO_IF_FAILED(CVI_TDL_SelectDetectClass(stTDLHandle, enOdModelId, 2, CVI_TDL_DET_TYPE_PERSON,
                                           CVI_TDL_DET_TYPE_CAR),
                 s32Ret, setup_tdl_fail);

  // Init DeepSORT
  CVI_TDL_DeepSORT_Init(stTDLHandle, true);
  cvtdl_deepsort_config_t ds_conf;
  CVI_TDL_DeepSORT_GetDefaultConfig(&ds_conf);
  setSampleMOTConfig(&ds_conf);
  CVI_TDL_DeepSORT_SetConfig(stTDLHandle, &ds_conf, -1, false);

  int use_stable_counter = atoi(argv[4]);
  if (use_stable_counter) {
    printf("Use Stable Counter.\n");
  }

  pthread_t stVencThread, stTDLThread;
  SAMPLE_TDL_VENC_THREAD_ARG_S venc_args = {
      .pstMWContext = &stMWContext,
      .stServiceHandle = stServiceHandle,
  };

  SAMPLE_TDL_TDL_THREAD_ARG_S ai_args = {
      .enOdModelId = enOdModelId,
      .object_detect = inference_func,
      .stTDLHandle = stTDLHandle,
      .bUseStableCounter = use_stable_counter,
  };

  pthread_create(&stVencThread, NULL, run_venc, &venc_args);
  pthread_create(&stTDLThread, NULL, run_tdl_thread, &ai_args);

  pthread_join(stVencThread, NULL);
  pthread_join(stTDLThread, NULL);

setup_tdl_fail:
  CVI_TDL_Service_DestroyHandle(stServiceHandle);
create_service_fail:
  CVI_TDL_DestroyHandle(stTDLHandle);
create_tdl_fail:
  SAMPLE_TDL_Destroy_MW(&stMWContext);

  return 0;
}
