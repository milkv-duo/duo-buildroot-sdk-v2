#define LOG_TAG "SampleCLIP"
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
#include "cvi_kit.h"

static volatile bool bExit = false;

static int pic_class = -1;

MUTEXAUTOLOCK_INIT(ResultMutex);

/**
 * @brief Arguments for video encoder thread
 *
 */
typedef struct {
  SAMPLE_TDL_MW_CONTEXT *pstMWContext;
  cvitdl_service_handle_t stServiceHandle;
  cvitdl_handle_t stTDLHandle;
  char *imageModelDir;
  char *textModelDir;
  char *textFileDir;
  char *directoryFileDir;
  char *frequencyFileDir;
} SAMPLE_TDL_VENC_THREAD_ARG_S;

static const char *cls_name[] = {"this is a photo of mobilephone", "this is a photo of dog",
                                 "this is a photo of people"};

void *run_venc(void *args) {
  printf("Enter encoder thread\n");
  SAMPLE_TDL_VENC_THREAD_ARG_S *pstArgs = (SAMPLE_TDL_VENC_THREAD_ARG_S *)args;
  VIDEO_FRAME_INFO_S stFrame;
  CVI_S32 s32Ret;

  while (bExit == false) {
    s32Ret = CVI_VPSS_GetChnFrame(0, 0, &stFrame, 2000);
    if (s32Ret != CVI_SUCCESS) {
      printf("CVI_VPSS_GetChnFrame chn0 failed with %#x\n", s32Ret);
      break;
    }
    char *id_num = calloc(64, sizeof(char));
    {
      MutexAutoLock(ResultMutex, lock);
      if (pic_class == -1) {
        sprintf(id_num, "this pic is not in dataset.");
      } else {
        sprintf(id_num, "cls:%s", cls_name[pic_class]);
      }
    }
    CVI_TDL_Service_ObjectWriteText(id_num, 50, 50, &stFrame, 1, 1, 1);
    free(id_num);

    s32Ret = SAMPLE_TDL_Send_Frame_RTSP(&stFrame, pstArgs->pstMWContext);
    if (s32Ret != CVI_SUCCESS) {
      goto error;
    }
  error:
    CVI_VPSS_ReleaseChnFrame(0, 0, &stFrame);
    if (s32Ret != CVI_SUCCESS) {
      bExit = true;
    }
  }
  printf("Exit encoder thread\n");
  pthread_exit(NULL);
}

void *run_tdl_thread(void *args) {
  SAMPLE_TDL_VENC_THREAD_ARG_S *pstArgs = (SAMPLE_TDL_VENC_THREAD_ARG_S *)args;
  printf("Enter TDL thread\n");
  cvitdl_handle_t pstTDLHandle = pstArgs->stTDLHandle;
  char *imageModelDir = pstArgs->imageModelDir;
  char *textModelDir = pstArgs->textModelDir;
  char *textFileDir = pstArgs->textFileDir;
  char *directoryFileDir = pstArgs->directoryFileDir;
  char *frequencyFileDir = pstArgs->frequencyFileDir;

  CVI_U32 openclip_frame = 0;
  CVI_U32 line_count = 0;
  CVI_CHAR line[1024] = {0};
  int32_t **tokens = NULL;
  CVI_S32 s32Ret;
  static uint32_t count = 0;
  float **image_features = NULL;
  float **text_features = NULL;
  cvtdl_clip_feature clip_feature_text;

  CVI_TDL_OpenModel(pstTDLHandle, CVI_TDL_SUPPORTED_MODEL_CLIP_TEXT, textModelDir);
  FILE *fp = fopen(textFileDir, "r");
  while (fgets(line, sizeof(line), fp) != NULL) {
    line_count++;
  }
  tokens = (int32_t **)malloc(line_count * sizeof(int32_t *));

  s32Ret = CVI_TDL_Set_TextPreprocess(directoryFileDir, frequencyFileDir, textFileDir, tokens,
                                      line_count);

  fseek(fp, 0, SEEK_SET);

  text_features = (float **)malloc(line_count * sizeof(float *));

  VIDEO_FRAME_INFO_S stOpenclipFrame;
  for (int i = 0; i < line_count; i++) {
    CVI_U8 buffer[77 * sizeof(int32_t)];
    memcpy(buffer, tokens[i], sizeof(int32_t) * 77);
    stOpenclipFrame.stVFrame.pu8VirAddr[0] = buffer;
    stOpenclipFrame.stVFrame.u32Height = 1;
    stOpenclipFrame.stVFrame.u32Width = 77;

    s32Ret = CVI_TDL_Clip_Text_Feature(pstTDLHandle, &stOpenclipFrame, &clip_feature_text);
    if (s32Ret != CVI_SUCCESS) {
      printf("CVI_TDL_Clip_Feature failed with %#x\n", s32Ret);
    }

    text_features[i] = (float *)malloc(clip_feature_text.feature_dim * sizeof(float));
    if (text_features[i] == NULL) {
      printf("Failed to malloc area for text_features[%d]!\n", i);
    }

    for (int j = 0; j < clip_feature_text.feature_dim; j++) {
      text_features[i][j] = clip_feature_text.out_feature[j];
    }
    CVI_TDL_Free(&clip_feature_text);
  }

  CVI_TDL_OpenModel(pstTDLHandle, CVI_TDL_SUPPORTED_MODEL_CLIP_IMAGE, imageModelDir);
  // CVI_TDL_SetSkipVpssPreprocess(pstTDLHandle, CVI_TDL_SUPPORTED_MODEL_CLIP_IMAGE, true);
  image_features = (float **)malloc(sizeof(float *));
  printf("***********************TDL sub thread******************************\n");
  while (bExit == false) {
    VIDEO_FRAME_INFO_S stFrame;

    static uint32_t count = 0;
    s32Ret = CVI_VPSS_GetChnFrame(0, 1, &stFrame, 2000);

    cvtdl_clip_feature clip_feature_image;
    s32Ret = CVI_TDL_Clip_Image_Feature(pstTDLHandle, &stFrame, &clip_feature_image);
    if (s32Ret != CVI_SUCCESS) {
      printf("inference failed!\n");
      CVI_VPSS_ReleaseChnFrame(0, 1, &stFrame);
      goto inf_error;
    }
    image_features[0] = (float *)malloc(clip_feature_image.feature_dim * sizeof(float));
    for (int j = 0; j < clip_feature_image.feature_dim; j++) {
      image_features[0][j] = clip_feature_image.out_feature[j];
    }

    CVI_VPSS_ReleaseChnFrame(0, 1, &stFrame);
    float **probs = (float **)malloc(sizeof(float *));
    probs[0] = (float *)malloc(sizeof(float));
    float thres = 0.6;
    int function_id = 0;
    s32Ret = CVI_TDL_Set_ClipPostprocess(text_features, line_count, image_features, 1, probs);
    free(image_features[0]);
    float max_prob = 0;
    int top1_id = -1;
    for (int j = 0; j < line_count; j++) {
      if (probs[0][j] > max_prob) {
        max_prob = probs[0][j];
        top1_id = j;
      }
    }
    if (max_prob < thres) {
      top1_id = -1;
    }

    {
      MutexAutoLock(ResultMutex, lock);
      pic_class = top1_id;
    }

    free(probs[0]);
    free(probs);
    CVI_TDL_Free(&clip_feature_image);
  }
inf_error:
  // release:
  for (int i = 0; i < line_count; ++i) {
    free(text_features[i]);
  }
  free(text_features);
  free(tokens);
  free(image_features);
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
  printf("this is stSensorSize.u32Height:%d\n", stSensorSize.u32Height);
  // VBPool configurations
  //////////////////////////////////////////////////
  pstMWConfig->stVBPoolConfig.u32VBPoolCount = 3;

  // VBPool 0 for VI
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[0].enFormat = VI_PIXEL_FORMAT;
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[0].u32BlkCount = 2;
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[0].u32Height = stSensorSize.u32Height;
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[0].u32Width = stSensorSize.u32Width;
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[0].bBind = true;
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[0].u32VpssChnBinding = VPSS_CHN0;
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[0].u32VpssGrpBinding = (VPSS_GRP)0;

  // VBPool 1 for TDL frame
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[1].enFormat = PIXEL_FORMAT_RGB_888_PLANAR;
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[1].u32BlkCount = 2;
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[1].u32Height = stSensorSize.u32Height;
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[1].u32Width = stSensorSize.u32Width;
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[1].bBind = true;
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[1].u32VpssChnBinding = VPSS_CHN1;
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[1].u32VpssGrpBinding = (VPSS_GRP)0;

  // VBPool 2 for TDL preprocessing.
  // The input pixel format of TDL SDK models is eighter RGB 888 or RGB 888 Planar.
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[2].enFormat = PIXEL_FORMAT_RGB_888_PLANAR;
  // TDL SDK use only 1 buffer at the same time.
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[2].u32BlkCount = 2;
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
  pstVpssConfig->u32ChnBindVI = 0;
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
  if (argc != 7) {
    printf(
        "Usage: %s <clip image model path> <clip text model path> <input txt directory list.txt>"
        "<total txt directory list.txt> <frequency for charactors list.txt> <1:sub thread; 0:main "
        "thread> \n ",
        argv[0]);
    printf("clip image model path: Path to clip image bmodel.\n");
    printf("clip text model path: Path to clip text bmodel.\n");
    printf("Input text directory: Directory containing input text for clip.\n");
    printf("total txt directory: Directory containing all directory.\n");
    printf("frequency for charactors list: frequency of every word in directory.\n");
    // printf("min conf for dataset: If the top 1 classification is less than th, return -1,else
    // return top 1 classification.\n");
    printf("sub thread flag:1:sub thread; 0:main thread.\n");
    return CVI_FAILURE;
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

  // Step 3: Run venc in thread.
  ///////////////////////////////////////////////////

  pthread_t stVencThread, stTDLThread;
  SAMPLE_TDL_VENC_THREAD_ARG_S args = {
      .pstMWContext = &stMWContext,
      .stServiceHandle = stServiceHandle,
      .stTDLHandle = stTDLHandle,
      .imageModelDir = argv[1],
      .textModelDir = argv[2],
      .textFileDir = argv[3],
      .directoryFileDir = argv[4],
      .frequencyFileDir = argv[5],
  };

  pthread_create(&stVencThread, NULL, run_venc, &args);
  // if flag ==1, run stTDLThread and block main thread.
  ///////////////////////////////////////////////////
  if (atoi(argv[6])) {
    pthread_create(&stTDLThread, NULL, run_tdl_thread, &args);
    pthread_join(stVencThread, NULL);
    pthread_join(stTDLThread, NULL);
  }

  // Step 4: else: Open and setup TDL models
  ///////////////////////////////////////////////////

  GOTO_IF_FAILED(CVI_TDL_OpenModel(stTDLHandle, CVI_TDL_SUPPORTED_MODEL_CLIP_TEXT, argv[2]), s32Ret,
                 setup_tdl_fail);

  float **image_features = NULL;
  float **text_features = NULL;
  CVI_CHAR line[1024] = {0};
  int line_count = 0;
  int32_t **tokens = NULL;
  cvtdl_clip_feature clip_feature_text;

  FILE *fp = fopen(argv[3], "r");

  while (fgets(line, sizeof(line), fp) != NULL) {
    line_count++;
  }
  tokens = (int32_t **)malloc(line_count * sizeof(int32_t *));

  s32Ret = CVI_TDL_Set_TextPreprocess(argv[4], argv[5], argv[3], tokens, line_count);

  fseek(fp, 0, SEEK_SET);

  text_features = (float **)malloc(line_count * sizeof(float *));

  VIDEO_FRAME_INFO_S stOpenclipFrame;
  for (int i = 0; i < line_count; i++) {
    CVI_U8 buffer[77 * sizeof(int32_t)];
    memcpy(buffer, tokens[i], sizeof(int32_t) * 77);
    stOpenclipFrame.stVFrame.pu8VirAddr[0] = buffer;
    stOpenclipFrame.stVFrame.u32Height = 1;
    stOpenclipFrame.stVFrame.u32Width = 77;

    s32Ret = CVI_TDL_Clip_Text_Feature(stTDLHandle, &stOpenclipFrame, &clip_feature_text);
    if (s32Ret != CVI_SUCCESS) {
      printf("CVI_TDL_Clip_Feature failed with %#x\n", s32Ret);
      goto inf_error;
    }

    text_features[i] = (float *)malloc(clip_feature_text.feature_dim * sizeof(float));
    if (text_features[i] == NULL) {
      printf("Failed to malloc area for text_features[%d]!\n", i);
      goto inf_error;
    }

    for (int j = 0; j < clip_feature_text.feature_dim; j++) {
      text_features[i][j] = clip_feature_text.out_feature[j];
    }
    CVI_TDL_Free(&clip_feature_text);
  }

  GOTO_IF_FAILED(CVI_TDL_OpenModel(stTDLHandle, CVI_TDL_SUPPORTED_MODEL_CLIP_IMAGE, argv[1]),
                 s32Ret, setup_tdl_fail);
  image_features = (float **)malloc(sizeof(float *));
  printf("***********************main thread******************************\n");
  while (bExit == false) {
    VIDEO_FRAME_INFO_S stFrame;

    static uint32_t count = 0;
    s32Ret = CVI_VPSS_GetChnFrame(0, 1, &stFrame, 2000);
    cvtdl_clip_feature clip_feature_image;
    // printf("stFrame.stVFrame.u32Width:%d\n",stFrame.stVFrame.u32Width);
    s32Ret = CVI_TDL_Clip_Image_Feature(stTDLHandle, &stFrame, &clip_feature_image);
    image_features[0] = (float *)malloc(clip_feature_image.feature_dim * sizeof(float));
    for (int j = 0; j < clip_feature_image.feature_dim; j++) {
      image_features[0][j] = clip_feature_image.out_feature[j];
    }
    CVI_VPSS_ReleaseChnFrame(0, 1, &stFrame);

    float **probs = (float **)malloc(sizeof(float *));
    probs[0] = (float *)malloc(sizeof(float));
    float thres = 0.6;
    int function_id = 0;
    s32Ret = CVI_TDL_Set_ClipPostprocess(text_features, line_count, image_features, 1, probs);
    free(image_features[0]);
    float max_prob = 0;
    int top1_id = -1;
    for (int j = 0; j < line_count; j++) {
      if (probs[0][j] > max_prob) {
        max_prob = probs[0][j];
        top1_id = j;
      }
    }
    if (max_prob < thres) {
      top1_id = -1;
    }

    {
      MutexAutoLock(ResultMutex, lock);
      pic_class = top1_id;
    }

    free(probs[0]);
    free(probs);
    CVI_TDL_Free(&clip_feature_image);
  }

inf_error:
  // release:
  for (int i = 0; i < line_count; ++i) {
    free(text_features[i]);
  }
  free(text_features);
  free(tokens);
  free(image_features);
setup_tdl_fail:
  CVI_TDL_Service_DestroyHandle(stServiceHandle);
create_service_fail:
  CVI_TDL_DestroyHandle(stTDLHandle);
create_tdl_fail:
  SAMPLE_TDL_Destroy_MW(&stMWContext);

  return 0;
}
