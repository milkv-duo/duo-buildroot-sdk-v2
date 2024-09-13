/**
 * This is a sample code for object tracking.
 */
#define LOG_TAG "SampleObjectTracking"
#define LOG_LEVEL LOG_LEVEL_INFO

#include "cvi_tdl_app.h"
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
#include <sys/time.h>
#include <unistd.h>

static const char *enumStr[] = {"NORMAL", "START", "WARNING"};

static volatile bool bExit = false;

MUTEXAUTOLOCK_INIT(ResultMutex);

typedef struct {
  SAMPLE_TDL_MW_CONTEXT *pstMWContext;
  cvitdl_service_handle_t stServiceHandle;
  int det_lane;
} SAMPLE_TDL_VENC_THREAD_ARG_S;

typedef struct {
  ODInferenceFunc object_detect;
  CVI_TDL_SUPPORTED_MODEL_E enOdModelId;
  cvitdl_handle_t stTDLHandle;
  bool bTrackingWithFeature;
} SAMPLE_TDL_TDL_THREAD_ARG_S;

static cvtdl_object_t g_stObjMeta = {0};
static cvtdl_lane_t g_stLaneMeta = {0};

static uint32_t get_time_in_ms() {
  struct timeval tv;
  if (gettimeofday(&tv, NULL) < 0) {
    return 0;
  }
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

void set_sample_mot_config(cvtdl_deepsort_config_t *ds_conf) {
  ds_conf->ktracker_conf.P_beta[2] = 0.01;
  ds_conf->ktracker_conf.P_beta[6] = 1e-5;

  // ds_conf.kfilter_conf.Q_beta[2] = 0.1;
  ds_conf->kfilter_conf.Q_beta[2] = 0.01;
  ds_conf->kfilter_conf.Q_beta[6] = 1e-5;
  ds_conf->kfilter_conf.R_beta[2] = 0.1;
}

cvtdl_service_brush_t get_random_brush(uint64_t seed, int min) {
  float scale = (256. - (float)min) / 256.;
  srand((uint32_t)seed);
  cvtdl_service_brush_t brush = {
      .color.r = (int)((floor(((float)rand() / (RAND_MAX)) * 256.)) * scale) + min,
      .color.g = (int)((floor(((float)rand() / (RAND_MAX)) * 256.)) * scale) + min,
      .color.b = (int)((floor(((float)rand() / (RAND_MAX)) * 256.)) * scale) + min,
      .size = 2,
  };

  return brush;
}

void *run_venc(void *args) {
  printf("Enter encoder thread\n");
  SAMPLE_TDL_VENC_THREAD_ARG_S *pstArgs = (SAMPLE_TDL_VENC_THREAD_ARG_S *)args;
  VIDEO_FRAME_INFO_S stFrame;
  CVI_S32 s32Ret;
  cvtdl_object_t stObjMeta = {0};
  cvtdl_lane_t stLaneMeta = {0};
  cvtdl_service_brush_t brush_lane = {.size = 4, .color.r = 0, .color.g = 255, .color.b = 0};

  cvtdl_pts_t pts;
  pts.size = 2;
  pts.x = (float *)malloc(2 * sizeof(float));
  pts.y = (float *)malloc(2 * sizeof(float));

  while (bExit == false) {
    s32Ret = CVI_VPSS_GetChnFrame(0, 0, &stFrame, 2000);
    if (s32Ret != CVI_SUCCESS) {
      printf("CVI_VPSS_GetChnFrame chn0 failed with %#x\n", s32Ret);
      break;
    }

    {
      MutexAutoLock(ResultMutex, lock);
      CVI_TDL_CopyObjectMeta(&g_stObjMeta, &stObjMeta);
      CVI_TDL_CopyLaneMeta(&g_stLaneMeta, &stLaneMeta);
    }

    cvtdl_service_brush_t *brushes = malloc(stObjMeta.size * sizeof(cvtdl_service_brush_t));
    for (uint32_t oid = 0; oid < stObjMeta.size; oid++) {
      brushes[oid] = get_random_brush(stObjMeta.info[oid].unique_id, 64);
    }

    // Fill name with unique id.
    for (uint32_t oid = 0; oid < stObjMeta.size; oid++) {
      snprintf(stObjMeta.info[oid].name, sizeof(stObjMeta.info[oid].name),
               "S:%.1f m, V:%.1f m/s, [%s]", stObjMeta.info[oid].adas_properity.dis,
               stObjMeta.info[oid].adas_properity.speed,
               enumStr[stObjMeta.info[oid].adas_properity.state]);
    }

    s32Ret = CVI_TDL_Service_ObjectDrawRect2(pstArgs->stServiceHandle, &stObjMeta, &stFrame, true,
                                             brushes);
    if (s32Ret != CVI_TDL_SUCCESS) {
      printf("Draw fame fail!, ret=%x\n", s32Ret);
      goto error;
    }

    if (pstArgs->det_lane) {
      char lane_info[64];
      int txt_x = (int)(0.5 * stFrame.stVFrame.u32Width);
      int txt_y = (int)(0.8 * stFrame.stVFrame.u32Height);
      if (stLaneMeta.lane_state == 0) {
        strcpy(lane_info, "NORMAL");
        s32Ret = CVI_TDL_Service_ObjectWriteText(lane_info, txt_x, txt_y, &stFrame, 0, 255, 0);
      } else {
        strcpy(lane_info, "LANE DEPARTURE WARNING !");
        s32Ret = CVI_TDL_Service_ObjectWriteText(lane_info, txt_x, txt_y, &stFrame, 255, 0, 0);
      }
      if (s32Ret != CVI_TDL_SUCCESS) {
        printf("Draw txt fail!, ret=%x\n", s32Ret);
        goto error;
      }

      for (uint32_t i = 0; i < stLaneMeta.size; i++) {
        pts.x[0] = (int)stLaneMeta.lane[i].x[0];
        pts.y[0] = (int)stLaneMeta.lane[i].y[0];
        pts.x[1] = (int)stLaneMeta.lane[i].x[1];
        pts.y[1] = (int)stLaneMeta.lane[i].y[1];

        s32Ret = CVI_TDL_Service_DrawPolygon(pstArgs->stServiceHandle, &stFrame, &pts, brush_lane);
        if (s32Ret != CVI_TDL_SUCCESS) {
          printf("Draw line fail!, ret=%x\n", s32Ret);
          goto error;
        }
      }
    }

    s32Ret = SAMPLE_TDL_Send_Frame_RTSP(&stFrame, pstArgs->pstMWContext);
  error:
    free(brushes);
    CVI_TDL_Free(&stObjMeta);
    CVI_VPSS_ReleaseChnFrame(0, 0, &stFrame);
    if (s32Ret != CVI_SUCCESS) {
      bExit = true;
    }
  }
  CVI_TDL_Free(&pts);
  printf("Exit encoder thread\n");
  pthread_exit(NULL);
}

void *run_tdl_thread(cvitdl_app_handle_t app_handle) {
  printf("Enter TDL thread\n");

  VIDEO_FRAME_INFO_S stFrame;
  size_t counter = 0;
  uint32_t last_time_ms = get_time_in_ms();
  size_t last_counter = 0;
  CVI_S32 s32Ret;
  while (bExit == false) {
    counter += 1;

    s32Ret = CVI_VPSS_GetChnFrame(0, VPSS_CHN1, &stFrame, 2000);

    if (s32Ret != CVI_SUCCESS) {
      printf("CVI_VPSS_GetChnFrame failed with %#x\n", s32Ret);
      goto get_frame_failed;
    }

    //*******************************************
    // Step 1: Object detect inference.

    int frm_diff = counter - last_counter;
    if (frm_diff > 20) {
      uint32_t cur_ts_ms = get_time_in_ms();
      float fps = frm_diff * 1000.0 / (cur_ts_ms - last_time_ms);
      last_time_ms = cur_ts_ms;
      last_counter = counter;
      printf("++++++++++++ frame:%d,fps:%.2f\n", (int)counter, fps);
      app_handle->adas_info->FPS = fps;  // set FPS, only set once if fps is stable
    }

    // update gsensor data every time if using gsensor:

    // app_handle->adas_info->gsensor_data.x = 0;
    // app_handle->adas_info->gsensor_data.y = 0;
    // app_handle->adas_info->gsensor_data.z = 0;
    // app_handle->adas_info->gsensor_data.counter += 1;

    s32Ret = CVI_TDL_APP_ADAS_Run(app_handle, &stFrame);

    if (s32Ret != CVI_TDL_SUCCESS) {
      printf("inference failed!, ret=%x\n", s32Ret);
      goto inf_error;
    }

    {
      MutexAutoLock(ResultMutex, lock);
      CVI_TDL_CopyObjectMeta(&app_handle->adas_info->last_objects, &g_stObjMeta);
      CVI_TDL_CopyLaneMeta(&app_handle->adas_info->lane_meta, &g_stLaneMeta);
    }

  inf_error:
    CVI_VPSS_ReleaseChnFrame(0, 1, &stFrame);
  get_frame_failed:
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
  if (argc != 3 && argc != 5) {
    printf(
        "\nUsage: %s \n"
        "person_vehicle_model_path\n"
        "det_type(0: only object, 1: object and lane) \n"
        "[lane_det_model_path](optional, if det_type=1, must exist) \n"
        "[lane_model_type](optional, 0 for lane_det model, 1 for lstr model, if det_type=1, must "
        "exist) \n",
        argv[0]);
    return -1;
  }

  ODInferenceFunc inference_func;
  CVI_TDL_SUPPORTED_MODEL_E enOdModelId;

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
#ifndef CV186X
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

  int det_type = atoi(argv[2]);
  uint32_t buffer_size = 20;
  cvitdl_handle_t tdl_handle = stTDLHandle;
  cvitdl_app_handle_t app_handle = NULL;
  s32Ret |= CVI_TDL_APP_CreateHandle(&app_handle, tdl_handle);
  s32Ret |= CVI_TDL_APP_ADAS_Init(app_handle, (uint32_t)buffer_size, det_type);

  if (s32Ret != CVI_SUCCESS) {
    printf("failed with %#x!\n", s32Ret);
    goto setup_tdl_fail;
  }

  // setup yolo algorithm preprocess
  cvtdl_det_algo_param_t yolov8_param =
      CVI_TDL_GetDetectionAlgoParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION);
  yolov8_param.cls = 7;

  s32Ret = CVI_TDL_SetDetectionAlgoParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION,
                                         yolov8_param);
  if (s32Ret != CVI_SUCCESS) {
    printf("Can not set yolov8 algorithm parameters %#x\n", s32Ret);
    return s32Ret;
  }

  GOTO_IF_FAILED(CVI_TDL_OpenModel(stTDLHandle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION, argv[1]),
                 s32Ret, setup_tdl_fail);

  if (argc == 5) {  // detect lane
    app_handle->adas_info->lane_model_type = atoi(argv[4]);
    CVI_TDL_SUPPORTED_MODEL_E lane_model_id;
    if (app_handle->adas_info->lane_model_type == 0) {
      lane_model_id = CVI_TDL_SUPPORTED_MODEL_LANE_DET;
    } else if (app_handle->adas_info->lane_model_type == 1) {
      lane_model_id = CVI_TDL_SUPPORTED_MODEL_LSTR;
    } else {
      printf(" err lane_model_type !\n");
      return -1;
    }

    GOTO_IF_FAILED(CVI_TDL_OpenModel(stTDLHandle, lane_model_id, argv[3]), s32Ret, setup_tdl_fail);
  }

  // Init DeepSORT
  CVI_TDL_DeepSORT_Init(stTDLHandle, true);
  cvtdl_deepsort_config_t ds_conf;
  CVI_TDL_DeepSORT_GetDefaultConfig(&ds_conf);
  set_sample_mot_config(&ds_conf);
  CVI_TDL_DeepSORT_SetConfig(stTDLHandle, &ds_conf, -1, false);

  pthread_t stVencThread, stTDLThread;
  SAMPLE_TDL_VENC_THREAD_ARG_S venc_args = {
      .pstMWContext = &stMWContext,
      .stServiceHandle = stServiceHandle,
      .det_lane = det_type,
  };

  pthread_create(&stVencThread, NULL, run_venc, &venc_args);
  pthread_create(&stTDLThread, NULL, run_tdl_thread, app_handle);

  pthread_join(stVencThread, NULL);
  pthread_join(stTDLThread, NULL);

  CVI_TDL_APP_DestroyHandle(app_handle);

setup_tdl_fail:
  CVI_TDL_Service_DestroyHandle(stServiceHandle);
create_service_fail:
  CVI_TDL_DestroyHandle(stTDLHandle);
create_tdl_fail:
  SAMPLE_TDL_Destroy_MW(&stMWContext);

  return 0;
}
