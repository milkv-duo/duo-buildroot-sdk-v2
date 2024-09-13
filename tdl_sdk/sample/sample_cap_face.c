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
#define CAPTRUE 1

float g_draw_clrs[] = {0,   0,   0,   255, 0,   0, 0,   255, 0,   0,  0,
                       255, 255, 255, 0,   255, 0, 255, 0,   255, 255};

bool g_use_face_attribute;

typedef struct {
  float x1;
  float y1;
  float x2;
  float y2;
  float score;
} face_bbox;

typedef struct {
  face_bbox bbox;
  uint64_t u_id;
  float quality;
  cvtdl_image_t image;
  tracker_state_e state;
  uint32_t counter;
  uint64_t frame_id;
  float boxw;
  float eye_dist;
  float gender;
  float age;
  float glass;
  float mask;
} IOData;

typedef struct {
  CVI_S32 voType;
  SAMPLE_TDL_MW_CONTEXT *p_mv_ctx;
  cvitdl_service_handle_t service_handle;
} pVOArgs;

MUTEXAUTOLOCK_INIT(IOMutex);
MUTEXAUTOLOCK_INIT(VOMutex);

/* global variables */
static volatile bool bExit = false;
static volatile bool bRunImageWriter = true;
static volatile bool bRunVideoOutput = true;

int rear_idx = 0;
int front_idx = 0;
static IOData data_buffer[OUTPUT_BUFFER_SIZE];

static cvtdl_face_t g_face_meta_0;   // face
static cvtdl_object_t g_obj_meta_0;  // person
char g_out_dir[128];
uint64_t g_frmcounter = 0;
int COUNT_ALIVE(face_capture_t *face_cpt_info);

static uint32_t get_time_in_ms() {
  struct timeval tv;
  if (gettimeofday(&tv, NULL) < 0) {
    return 0;
  }
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static void SampleHandleSig(CVI_S32 signo) {
  signal(SIGINT, SIG_IGN);
  signal(SIGTERM, SIG_IGN);

  if (SIGINT == signo || SIGTERM == signo) {
    bExit = true;
  }
}

void write_jpg_file(const char *filename, int target_idx) {
  FILE *f;
  f = fopen(filename, "wb");
  if (!f) {
    printf("open file fail\n");
  }
  fwrite(data_buffer[target_idx].image.full_img, 1, data_buffer[target_idx].image.full_length, f);
  fclose(f);
}

/* Consumer */
static void *pImageWrite(void *args) {
  printf("[APP] Image Write Up\n");
  while (bRunImageWriter) {
    /* only consumer write front_idx */
    bool empty;
    {
      MutexAutoLock(IOMutex, lock);
      empty = front_idx == rear_idx;
    }
    if (empty) {
      // printf("I/O Buffer is empty.\n");
      usleep(100 * 1000);
      continue;
    }
    int target_idx = (front_idx + 1) % OUTPUT_BUFFER_SIZE;
    int frame_id = (int)data_buffer[target_idx].frame_id;
    int track_id = (int)data_buffer[target_idx].u_id;

    char filename[512];
    if (data_buffer[target_idx].image.width == 0) {
      printf("[WARNING] Target image is empty.\n");
    } else {
      // TODO test and verify
      if (CAPTRUE) {
        // save encode jpg file
        sprintf(filename, "%s/face_%d_%u_frm_%d_qua_%.3f_eyedist_%.1f_full.jpg", g_out_dir,
                track_id, data_buffer[target_idx].counter, frame_id,
                data_buffer[target_idx].quality, data_buffer[target_idx].eye_dist);
        printf("to output :%s %d\n", filename, data_buffer[target_idx].image.pix_format);
        write_jpg_file(filename, target_idx);

        if (!g_use_face_attribute) {
          sprintf(filename, "%s/face_%d_%u_frm_%d_qua_%.3f_eyedist_%.1f_crop.png", g_out_dir,
                  track_id, data_buffer[target_idx].counter, frame_id,
                  data_buffer[target_idx].quality, data_buffer[target_idx].eye_dist);
          printf("to output :%s %d\n", filename, data_buffer[target_idx].image.pix_format);
          stbi_write_png(filename, data_buffer[target_idx].image.width,
                         data_buffer[target_idx].image.height, STBI_rgb,
                         data_buffer[target_idx].image.pix[0],
                         data_buffer[target_idx].image.stride[0]);
        } else if (g_use_face_attribute) {
          sprintf(
              filename,
              "%s/"
              "face_%d_%u_frm_%d_qua_%.3f_eyedist_%.1f_gender_%.3f_age_%.3f_glass_%.3f_mask_%.3f"
              "_crop.png",
              g_out_dir, track_id, data_buffer[target_idx].counter, frame_id,
              data_buffer[target_idx].quality, data_buffer[target_idx].eye_dist,
              data_buffer[target_idx].gender, data_buffer[target_idx].age,
              data_buffer[target_idx].glass, data_buffer[target_idx].mask);
          printf("to output :%s %d\n", filename, data_buffer[target_idx].image.pix_format);
          stbi_write_png(filename, data_buffer[target_idx].image.width,
                         data_buffer[target_idx].image.height, STBI_rgb,
                         data_buffer[target_idx].image.pix[0],
                         data_buffer[target_idx].image.stride[0]);
        }
      } else {
        if (data_buffer[target_idx].image.pix_format == PIXEL_FORMAT_RGB_888) {
          sprintf(filename, "%s/face_%d_%u_frm_%d_qua_%.3f_boxw_%.1f_eyedist_%.1f.png", g_out_dir,
                  track_id, data_buffer[target_idx].counter, frame_id,
                  data_buffer[target_idx].quality, data_buffer[target_idx].boxw,
                  data_buffer[target_idx].eye_dist);
          printf("to output:%s %d\n", filename, data_buffer[target_idx].image.pix_format);
          stbi_write_png(filename, data_buffer[target_idx].image.width,
                         data_buffer[target_idx].image.height, STBI_rgb,
                         data_buffer[target_idx].image.pix[0],
                         data_buffer[target_idx].image.stride[0]);
        }
      }
    }

    CVI_TDL_Free(&data_buffer[target_idx].image);
    {
      MutexAutoLock(IOMutex, lock);
      front_idx = target_idx;
    }
  }

  printf("[APP] free buffer data...\n");
  while (front_idx != rear_idx) {
    CVI_TDL_Free(&data_buffer[(front_idx + 1) % OUTPUT_BUFFER_SIZE].image);
    {
      MutexAutoLock(IOMutex, lock);
      front_idx = (front_idx + 1) % OUTPUT_BUFFER_SIZE;
    }
  }

  return NULL;
}

void visualize_frame(cvitdl_service_handle_t service_handle, VIDEO_FRAME_INFO_S stVOFrame,
                     cvtdl_face_t *face_meta_0, cvtdl_object_t *obj_meta_0) {
  // cvitdl_service_handle_t service_handle = vo_args->service_handle;

  size_t image_size = stVOFrame.stVFrame.u32Length[0] + stVOFrame.stVFrame.u32Length[1] +
                      stVOFrame.stVFrame.u32Length[2];
  stVOFrame.stVFrame.pu8VirAddr[0] =
      (uint8_t *)CVI_SYS_Mmap(stVOFrame.stVFrame.u64PhyAddr[0], image_size);
  stVOFrame.stVFrame.pu8VirAddr[1] =
      stVOFrame.stVFrame.pu8VirAddr[0] + stVOFrame.stVFrame.u32Length[0];
  stVOFrame.stVFrame.pu8VirAddr[2] =
      stVOFrame.stVFrame.pu8VirAddr[1] + stVOFrame.stVFrame.u32Length[1];

  CVI_TDL_RescaleMetaCenterFace(&stVOFrame, face_meta_0);
  CVI_TDL_RescaleMetaCenterObj(&stVOFrame, obj_meta_0);
  char strinfo[128];
  for (int i = 0; i < face_meta_0->size; i++) {
    int lb = (int)face_meta_0->info[i].unique_id;

    cvtdl_service_brush_t brushi;
    int num_clr = 7;
    int ind = lb % num_clr;

    brushi.color.r = g_draw_clrs[3 * ind];
    brushi.color.g = g_draw_clrs[3 * ind + 1];
    brushi.color.b = g_draw_clrs[3 * ind + 2];
    brushi.size = 4;
    cvtdl_face_t face_met_tdl = *face_meta_0;
    face_met_tdl.size = 1;
    face_met_tdl.info = &face_meta_0->info[i];
    CVI_TDL_Service_FaceDrawRect(service_handle, &face_met_tdl, &stVOFrame, false, brushi);

    int tid = (int)face_meta_0->info[i].unique_id;
    float quality = face_meta_0->info[i].face_quality;
    float wdith = face_meta_0->info[i].bbox.x2 - face_meta_0->info[i].bbox.x1;
    sprintf(strinfo, "id:%d_%.2f_%d", tid, quality, (int)wdith);
    CVI_TDL_Service_ObjectWriteText(strinfo, face_meta_0->info[i].bbox.x1,
                                    face_meta_0->info[i].bbox.y1, &stVOFrame, 200.1, 0, 0);
  }
  if (face_meta_0->size > 0) {
    sprintf(strinfo, "frm:%u", (uint32_t)g_frmcounter);
    CVI_TDL_Service_ObjectWriteText(strinfo, 0, 100, &stVOFrame, 255, 0, 0);
  }
  for (int i = 0; i < obj_meta_0->size; i++) {
    int lb = (int)obj_meta_0->info[i].unique_id;

    cvtdl_service_brush_t brushi;
    int num_clr = 7;
    int ind = lb % num_clr;

    brushi.color.r = g_draw_clrs[3 * ind];
    brushi.color.g = g_draw_clrs[3 * ind + 1];
    brushi.color.b = g_draw_clrs[3 * ind + 2];
    brushi.size = 4;
    cvtdl_object_t obj_mete_tdl = *obj_meta_0;
    obj_mete_tdl.size = 1;
    obj_mete_tdl.info = &obj_meta_0->info[i];
    CVI_TDL_Service_ObjectDrawRect(service_handle, &obj_mete_tdl, &stVOFrame, false, brushi);

    int tid = (int)obj_meta_0->info[i].unique_id;
    sprintf(strinfo, "id:%d", tid);
    CVI_TDL_Service_ObjectWriteText(strinfo, obj_meta_0->info[i].bbox.x1,
                                    obj_meta_0->info[i].bbox.y1, &stVOFrame, 0, 200, 0);
  }

  CVI_SYS_Munmap((void *)stVOFrame.stVFrame.pu8VirAddr[0], image_size);
  stVOFrame.stVFrame.pu8VirAddr[0] = NULL;
  stVOFrame.stVFrame.pu8VirAddr[1] = NULL;
  stVOFrame.stVFrame.pu8VirAddr[2] = NULL;
}
static void *pVideoOutput(void *args) {
  printf("[APP] Video Output Up\n");
  pVOArgs *vo_args = (pVOArgs *)args;
  if (!vo_args->voType) {
    return NULL;
  }

  cvitdl_service_handle_t service_handle = vo_args->service_handle;
  CVI_S32 s32Ret = CVI_SUCCESS;

  cvtdl_face_t face_meta_0;
  cvtdl_object_t obj_meta_0;
  memset(&face_meta_0, 0, sizeof(cvtdl_face_t));
  memset(&obj_meta_0, 0, sizeof(cvtdl_object_t));

  /*tdl_handle_t tdl_handle = vo_args->tdl_handle;
  bool do_ped = strlen(vo_args->sz_ped_model)>0;
  if(do_ped){
     CVI_TDL_SUPPORTED_MODEL_E ped_model_id = CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PEDESTRIAN_D0;
     CVI_TDL_OpenModel(tdl_handle, ped_model_id, vo_args->sz_ped_model);

      CVI_TDL_SetModelThreshold(tdl_handle, ped_model_id, 0.5);
  }*/

  VIDEO_FRAME_INFO_S stVOFrame;
  int grp_id = 0;
  int vo_chn = VPSS_CHN1;
  while (bRunVideoOutput) {
    s32Ret = CVI_VPSS_GetChnFrame(grp_id, vo_chn, &stVOFrame, 1000);

    if (s32Ret != CVI_SUCCESS) {
      printf("CVI_VPSS_GetChnFrame preview failed with %#x\n", s32Ret);
      usleep(1000);
      continue;
    }

    {
      MutexAutoLock(VOMutex, lock);
      CVI_TDL_CopyFaceMeta(&g_face_meta_0, &face_meta_0);
      CVI_TDL_CopyObjectMeta(&g_obj_meta_0, &obj_meta_0);
    }
    visualize_frame(service_handle, stVOFrame, &face_meta_0, &obj_meta_0);
    s32Ret = SAMPLE_TDL_Send_Frame_RTSP(&stVOFrame, vo_args->p_mv_ctx);
    s32Ret = CVI_VPSS_ReleaseChnFrame(grp_id, vo_chn, &stVOFrame);
    if (s32Ret != CVI_SUCCESS) {
      printf("CVI_VPSS_ReleaseChnFrame chn0 NG\n");
      break;
    }

    CVI_TDL_Free(&face_meta_0);
    CVI_TDL_Free(&obj_meta_0);
  }
  return NULL;
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
      .u32Width = 1280,
      .u32Height = 720,
  };

  // VBPool configurations
  //////////////////////////////////////////////////
  pstMWConfig->stVBPoolConfig.u32VBPoolCount = 3;

  // VBPool 0 for VI
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[0].enFormat = VI_PIXEL_FORMAT;
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[0].u32BlkCount = 4;
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
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[2].enFormat = PIXEL_FORMAT_BGR_888_PLANAR;
  // TDL SDK use only 1 buffer at the same time.
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[2].u32BlkCount = 3;
  // Considering the maximum input size of object detection model is 1024x768, we set same size
  // here.
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[2].u32Height = 432;
  pstMWConfig->stVBPoolConfig.astVBPoolSetup[2].u32Width = 768;
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
  VPSS_CHN_DEFAULT_HELPER(&pstVpssConfig->astVpssChnAttr[0], 2560, 1440, VI_PIXEL_FORMAT, true);
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
int main(int argc, char *argv[]) {
  if (argc != 4 && argc != 5 && argc != 6) {
    printf("Usage: %s fdmodel_path ldmodel_path capture_path\n", argv[0]);
    printf("Usage: %s fdmodel_path pedmodel_path ldmodel_path capture_path\n", argv[0]);
    printf("Usage: %s fdmodel_path pedmodel_path ldmodel_path famodel_path capture_path\n",
           argv[0]);
    return CVI_FAILURE;
  }
  CVI_S32 ret = CVI_SUCCESS;
  // Set signal catch
  signal(SIGINT, SampleHandleSig);
  signal(SIGTERM, SampleHandleSig);

  const char *fd_model_path = argv[1];
  const char *ped_model_path = "NULL";
  const char *ld_model_path = "NULL";
  const char *fa_model_path = "NULL";
  if (argc == 4) {
    ld_model_path = argv[2];
    sprintf(g_out_dir, "%s", argv[3]);
  } else if (argc == 5) {
    ped_model_path = argv[2];
    ld_model_path = argv[3];

    sprintf(g_out_dir, "%s", argv[4]);
  } else if (argc == 6) {
    ped_model_path = argv[2];
    ld_model_path = argv[3];
    fa_model_path = argv[4];

    sprintf(g_out_dir, "%s", argv[5]);
  }
  printf("ped_model:%s\n", ped_model_path);

  int buffer_size = 20;        // atoi(argv[8]);
  float det_threshold = 0.45;  // atof(argv[9]);
  bool write_image = 1;        // atoi(argv[10]) == 1;
  int voType = 2;              // atoi(argv[11]);

  CVI_TDL_SUPPORTED_MODEL_E fd_model_id = CVI_TDL_SUPPORTED_MODEL_SCRFDFACE;
  CVI_TDL_SUPPORTED_MODEL_E fr_model_id = CVI_TDL_SUPPORTED_MODEL_FACERECOGNITION;

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

  cvitdl_handle_t stTDLHandle = NULL;
  GOTO_IF_FAILED(CVI_TDL_CreateHandle2(&stTDLHandle, 2, 0), s32Ret, setup_tdl_fail);

  // Assign VBPool ID 2 to the first VPSS in TDL SDK.
  GOTO_IF_FAILED(CVI_TDL_SetVBPool(stTDLHandle, 0, 2), s32Ret, setup_tdl_fail);

  CVI_TDL_SetVpssTimeout(stTDLHandle, 1000);

  cvitdl_service_handle_t stServiceHandle = NULL;
  GOTO_IF_FAILED(CVI_TDL_Service_CreateHandle(&stServiceHandle, stTDLHandle), s32Ret,
                 setup_tdl_fail);

  cvitdl_handle_t tdl_handle = stTDLHandle;
  cvitdl_service_handle_t service_handle = stServiceHandle;
  cvitdl_app_handle_t app_handle = NULL;

  ret |= CVI_TDL_APP_CreateHandle(&app_handle, tdl_handle);
  ret |= CVI_TDL_APP_FaceCapture_Init(app_handle, (uint32_t)buffer_size);
  ret |= CVI_TDL_APP_FaceCapture_QuickSetUp(
      app_handle, fd_model_id, fr_model_id, fd_model_path, NULL, NULL,
      (!strcmp(ld_model_path, "NULL")) ? NULL : ld_model_path,
      (!strcmp(fa_model_path, "NULL")) ? NULL : fa_model_path);
  g_use_face_attribute = app_handle->face_cpt_info->fa_flag;
  if (strcmp(ped_model_path, "NULL")) {
    CVI_TDL_SUPPORTED_MODEL_E ped_model_id = CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PEDESTRIAN;
    ret |= CVI_TDL_APP_FaceCapture_FusePedSetup(app_handle, ped_model_id, ped_model_path);
    CVI_TDL_SetModelThreshold(tdl_handle, ped_model_id, det_threshold);
    printf("face fuse track with ped\n");
  }
  if (ret != CVI_SUCCESS) {
    printf("failed with %#x!\n", ret);
    goto setup_tdl_fail;
  }

  CVI_TDL_SetModelThreshold(tdl_handle, fd_model_id, det_threshold);
  CVI_TDL_APP_FaceCapture_SetMode(app_handle, AUTO);

  face_capture_config_t app_cfg;
  CVI_TDL_APP_FaceCapture_GetDefaultConfig(&app_cfg);
  app_cfg.thr_quality = 0.1;
  app_cfg.thr_size_min = 25;
  app_cfg.miss_time_limit = 30;
  app_cfg.m_interval = 150;  // 3s
  app_cfg.m_capture_num = 1;
  app_cfg.store_feature = true;
  app_cfg.qa_method = 0;
  app_cfg.img_capture_flag = CAPTRUE;
  app_cfg.eye_dist_thresh = 20;

  CVI_TDL_APP_FaceCapture_SetConfig(app_handle, &app_cfg);

  VIDEO_FRAME_INFO_S stfdFrame;
  memset(&g_face_meta_0, 0, sizeof(cvtdl_face_t));
  memset(&g_obj_meta_0, 0, sizeof(cvtdl_object_t));

  pthread_t io_thread, vo_thread;
  pthread_create(&io_thread, NULL, pImageWrite, NULL);
  pVOArgs vo_args = {0};
  vo_args.voType = voType;
  vo_args.service_handle = service_handle;
  vo_args.p_mv_ctx = &stMWContext;
  pthread_create(&vo_thread, NULL, pVideoOutput, (void *)&vo_args);

  size_t counter = 0;
  uint32_t last_time_ms = get_time_in_ms();
  size_t last_counter = 0;
  int grp_id = 0;
  int tdl_chn = VPSS_CHN0;
  double total_ts = 0;
  while (bExit == false) {
    counter += 1;
    // printf("to get ai frame\n");
    g_frmcounter = counter;
    ret = CVI_VPSS_GetChnFrame(grp_id, tdl_chn, &stfdFrame, 2000);

    // printf("got ai frame,width:%u\n",stfdFrame.stVFrame.u32Width);
    if (ret != CVI_SUCCESS) {
      printf("CVI_VPSS_GetChnFrame chn0 failed with %#x\n", ret);
      usleep(1000);
      continue;
      // break;
    }

    int alive_face_num = COUNT_ALIVE(app_handle->face_cpt_info);
    int frm_diff = counter - last_counter;
    if (frm_diff > 20) {
      uint32_t cur_ts_ms = get_time_in_ms();
      float fps = frm_diff * 1000.0 / (cur_ts_ms - last_time_ms);
      last_time_ms = cur_ts_ms;
      last_counter = counter;
      printf("++++++++++++ALIVE Facesxx: %d,frame:%d,fps:%.2f,width:%u,inferts:%.3f\n",
             alive_face_num, (int)counter, fps, stfdFrame.stVFrame.u32Width, total_ts / counter);
    }
    uint32_t t0 = get_time_in_ms();
    ret = CVI_TDL_APP_FaceCapture_Run(app_handle, &stfdFrame);
    total_ts += get_time_in_ms() - t0;

    if (ret != CVI_SUCCESS) {
      printf("CVI_TDL_APP_FaceCapture_Run failed with %#x\n", ret);
      usleep(1000);
      CVI_VPSS_ReleaseChnFrame(grp_id, tdl_chn, &stfdFrame);
      continue;
    }

    {
      MutexAutoLock(VOMutex, lock);
      CVI_TDL_Free(&g_face_meta_0);
      CVI_TDL_Free(&g_obj_meta_0);
      CVI_TDL_CopyFaceMeta(&app_handle->face_cpt_info->last_faces, &g_face_meta_0);
      CVI_TDL_CopyObjectMeta(&app_handle->face_cpt_info->last_objects, &g_obj_meta_0);
      // RESTRUCTURING_FACE_META(app_handle->face_cpt_info, &g_face_meta_0, &g_face_meta_1);
    }

    /* Producer */
    if (write_image) {
      for (uint32_t i = 0; i < app_handle->face_cpt_info->size; i++) {
        if (!app_handle->face_cpt_info->_output[i]) continue;

        tracker_state_e state = app_handle->face_cpt_info->data[i].state;
        uint32_t counter = app_handle->face_cpt_info->data[i]._out_counter;
        uint64_t u_id = app_handle->face_cpt_info->data[i].info.unique_id;
        float face_quality = app_handle->face_cpt_info->data[i].info.face_quality;

        /* Check output buffer space */
        bool full;
        int target_idx;
        {
          MutexAutoLock(IOMutex, lock);
          target_idx = (rear_idx + 1) % OUTPUT_BUFFER_SIZE;
          full = target_idx == front_idx;
        }
        if (full) {
          printf("[WARNING] Buffer is full! Drop out!");
          CVI_VPSS_ReleaseChnFrame(grp_id, tdl_chn, &stfdFrame);
          continue;
        }
        printf("to output cropface:%d,frameid:%d,counter:%u\n", (int)u_id,
               (int)app_handle->face_cpt_info->data[i].cap_timestamp, counter);
        /* Copy image data to buffer */
        memcpy(&data_buffer[target_idx].bbox, &app_handle->face_cpt_info->data[i].info.bbox,
               sizeof(face_bbox));
        data_buffer[target_idx].u_id = u_id;
        data_buffer[target_idx].quality = face_quality;
        data_buffer[target_idx].state = state;
        data_buffer[target_idx].counter = counter;
        data_buffer[target_idx].frame_id = app_handle->face_cpt_info->data[i].cap_timestamp;
        data_buffer[target_idx].boxw = app_handle->face_cpt_info->data[i].info.bbox.x2 -
                                       app_handle->face_cpt_info->data[i].info.bbox.x1;
        data_buffer[target_idx].eye_dist = app_handle->face_cpt_info->data[i].info.pts.x[1] -
                                           app_handle->face_cpt_info->data[i].info.pts.x[0];
        if (g_use_face_attribute) {
          data_buffer[target_idx].gender =
              app_handle->face_cpt_info->data[i].face_data.info->gender_score;
          data_buffer[target_idx].age = app_handle->face_cpt_info->data[i].face_data.info->age;
          data_buffer[target_idx].glass = app_handle->face_cpt_info->data[i].face_data.info->glass;
          data_buffer[target_idx].mask =
              app_handle->face_cpt_info->data[i].face_data.info->mask_score;
        }
        /* NOTE: Make sure the image type is IVE_IMAGE_TYPE_U8C3_PACKAGE */
        CVI_TDL_CopyImage(&app_handle->face_cpt_info->data[i].image,
                          &data_buffer[target_idx].image);
        {
          MutexAutoLock(IOMutex, lock);
          rear_idx = target_idx;
        }
      }
    }
    ret = CVI_VPSS_ReleaseChnFrame(grp_id, tdl_chn, &stfdFrame);
    if (ret != CVI_SUCCESS) {
      printf("CVI_VPSS_ReleaseChnFrame chn0 NG\n");
      break;
    }
  }
  bRunImageWriter = false;
  bRunVideoOutput = false;
  // pthread_join(io_thread, NULL);
  // pthread_join(vo_thread, NULL);
  CVI_TDL_Free(&g_face_meta_0);
  CVI_TDL_Free(&g_obj_meta_0);

  CVI_TDL_APP_DestroyHandle(app_handle);
setup_tdl_fail:
  CVI_TDL_Service_DestroyHandle(stServiceHandle);
  CVI_TDL_DestroyHandle(stTDLHandle);
  SAMPLE_TDL_Destroy_MW(&stMWContext);
}

int COUNT_ALIVE(face_capture_t *face_cpt_info) {
  int counter = 0;
  for (uint32_t j = 0; j < face_cpt_info->size; j++) {
    if (face_cpt_info->data[j].state == ALIVE) {
      counter += 1;
    }
  }
  return counter;
}
