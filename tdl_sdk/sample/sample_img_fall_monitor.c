#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "core/utils/vpss_helper.h"
#include "cvi_tdl.h"
#include "cvi_tdl_app/cvi_tdl_app.h"
#include "cvi_tdl_media.h"
#include "sample_comm.h"
#include "stb_image.h"
#include "stb_image_write.h"
#include "sys_utils.h"
#include "cvi_kit.h"

typedef enum { fast = 0, interval, leave, intelligent } APP_MODE_e;

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
  char name[128];
  float match_score;
  uint64_t frame_id;
} IOData;

SMT_MUTEXAUTOLOCK_INIT(IOMutex);
SMT_MUTEXAUTOLOCK_INIT(VOMutex);

/* global variables */
static volatile bool bExit = false;
static volatile bool bRunImageWriter = true;
static volatile bool bRunVideoOutput = true;

#ifdef VISUAL_FACE_LANDMARK
void FREE_FACE_PTS(cvtdl_face_t *face_meta);
#endif

void set_sample_mot_config(cvtdl_deepsort_config_t *ds_conf) {
  ds_conf->ktracker_conf.max_unmatched_num = 10;
  ds_conf->ktracker_conf.accreditation_threshold = 10;
  ds_conf->ktracker_conf.P_beta[2] = 0.1;
  ds_conf->ktracker_conf.P_beta[6] = 2.5e-2;
  ds_conf->kfilter_conf.Q_beta[2] = 0.1;
  ds_conf->kfilter_conf.Q_beta[6] = 2.5e-2;
}

static void SampleHandleSig(CVI_S32 signo) {
  signal(SIGINT, SIG_IGN);
  signal(SIGTERM, SIG_IGN);

  if (SIGINT == signo || SIGTERM == signo) {
    bExit = true;
  }
}
CVI_S32 SAMPLE_COMM_VPSS_Stop(VPSS_GRP VpssGrp, CVI_BOOL *pabChnEnable) {
  CVI_S32 j;
  CVI_S32 s32Ret = CVI_SUCCESS;
  VPSS_CHN VpssChn;

  for (j = 0; j < VPSS_MAX_PHY_CHN_NUM; j++) {
    if (pabChnEnable[j]) {
      VpssChn = j;
      s32Ret = CVI_VPSS_DisableChn(VpssGrp, VpssChn);
      if (s32Ret != CVI_SUCCESS) {
        printf("failed with %#x!\n", s32Ret);
        return CVI_FAILURE;
      }
    }
  }

  s32Ret = CVI_VPSS_StopGrp(VpssGrp);
  if (s32Ret != CVI_SUCCESS) {
    printf("failed with %#x!\n", s32Ret);
    return CVI_FAILURE;
  }

  s32Ret = CVI_VPSS_DestroyGrp(VpssGrp);
  if (s32Ret != CVI_SUCCESS) {
    printf("failed with %#x!\n", s32Ret);
    return CVI_FAILURE;
  }

  return CVI_SUCCESS;
}

void release_system(cvitdl_handle_t tdl_handle, cvitdl_service_handle_t service_handle,
                    cvitdl_app_handle_t app_handle) {
  // CVI_TDL_APP_DestroyHandle(app_handle);
  if (service_handle != NULL) CVI_TDL_Service_DestroyHandle(service_handle);
  CVI_TDL_DestroyHandle(tdl_handle);
  // DestroyVideoSystem(&vs_ctx);
  CVI_SYS_Exit();
  CVI_VB_Exit();
}

int main(int argc, char *argv[]) {
  // argv[1] : 图片文件夹路径
  // argv[2] : 预测结果保存文件夹路径
  // argv[3] : 模型路径
  // argv[4] : 图片数量，自动获取
  // argv[5] : 关键点可视化图片保存文件夹，自动获取
  // argv[6] : 关键点预测结果文件夹
  // argv[7] : 0 或 1 ，是否保存关键点图片
  // argv[8] : fps

  CVI_S32 ret = CVI_SUCCESS;
  // Set signal catch
  signal(SIGINT, SampleHandleSig);
  signal(SIGTERM, SampleHandleSig);
  CVI_BOOL abChnEnable[VPSS_MAX_CHN_NUM] = {
      CVI_TRUE,
  };

  CVI_TDL_SUPPORTED_MODEL_E enOdModelId = CVI_TDL_SUPPORTED_MODEL_YOLOV8POSE;

  for (VPSS_GRP VpssGrp = 0; VpssGrp < VPSS_MAX_GRP_NUM; ++VpssGrp)
    SAMPLE_COMM_VPSS_Stop(VpssGrp, abChnEnable);

  // CVI_TDL_SUPPORTED_MODEL_E model = CVI_TDL_SUPPORTED_MODEL_YOLOV8POSE;
  char modelf[256];
  strcpy(modelf, "/mnt/data/admin1_data/AI_CV/cv182x/ai_models_output/cv181x/scrfd_500m_bnkps_432_768.cvimodel");

  //   const char *config_path = "NULL";  // argv[4];//NULL
  char str_image_root[256];
  char dst_dir[256];
  strcpy(str_image_root, argv[1]);
  strcpy(dst_dir, argv[2]);

  const CVI_S32 vpssgrp_width = 1920;
  const CVI_S32 vpssgrp_height = 1080;

  ret = MMF_INIT_HELPER2(vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888, 1, vpssgrp_width,
                         vpssgrp_height, PIXEL_FORMAT_RGB_888_PLANAR, 1);

  cvitdl_handle_t tdl_handle = NULL;
  cvitdl_service_handle_t service_handle = NULL;
  cvitdl_app_handle_t app_handle = NULL;

  // int vpss_grp = 1;
  // ret = CVI_TDL_CreateHandle2(&tdl_handle, vpss_grp, 0);
  ret = CVI_TDL_CreateHandle(&tdl_handle);
  // ret |= CVI_TDL_SetVBPool(tdl_handle, 0, 2);
  ret |= CVI_TDL_Service_CreateHandle(&service_handle, tdl_handle);
  // ret |= CVI_TDL_Service_EnableTPUDraw(service_handle, true);

  CVI_TDL_OpenModel(tdl_handle, enOdModelId, argv[3]);

  CVI_TDL_Set_Fall_FPS(tdl_handle, atof(argv[8]));

  printf("to quick setup\n");

  // Init DeepSORT
  CVI_TDL_DeepSORT_Init(tdl_handle, true);
  cvtdl_deepsort_config_t ds_conf;
  CVI_TDL_DeepSORT_GetDefaultConfig(&ds_conf);
  set_sample_mot_config(&ds_conf);
  CVI_TDL_DeepSORT_SetConfig(tdl_handle, &ds_conf, -1, false);

  if (ret != CVI_SUCCESS) {
    release_system(tdl_handle, service_handle, app_handle);
    return CVI_FAILURE;
  }

  printf("to start:\n");

  int num_end = atoi(argv[4]);

  char dir_name[256];
  char *last_slash = strrchr(str_image_root, '/');
  if (last_slash != NULL) {
    strcpy(dir_name, last_slash + 1);
  } else {
    strcpy(dir_name, str_image_root);
  }
  
  char dst_root[512];
  snprintf(dst_root, sizeof(dst_root), "%s/%s.txt", dst_dir, dir_name);

  FILE *fp = fopen(dst_root, "w");
  bool fall = false;
  imgprocess_t img_handle;
  CVI_TDL_Create_ImageProcessor(&img_handle);
  for (int img_idx = 0; img_idx < num_end; img_idx++) {
    cvtdl_object_t stObjMeta = {0};
    cvtdl_tracker_t stTrackerMeta = {0};
    char save_img_[256];
    char save_img[256];
    char text_[256];
    char text[256];

    char keypoint_path_[256];
    char keypoint_path[256];
    float **dets = NULL;
    size_t dets_size = 0;
    size_t dets_capacity = 16;  // 初始容量
    char f_line[1024];
    float val;

    sprintf(keypoint_path_, "/mnt/data/3_data/infer/fall_det/kps_result/%s/%s#%d.jpg.txt", argv[6],
            dir_name, img_idx);
    strcpy(keypoint_path, keypoint_path_);

    FILE *kps_file = fopen(keypoint_path, "r");
    if (kps_file == NULL) {
      printf("########### Failed to open keypoint file #########\n");
      return -1;
    }

    if (bExit) break;
    char szimg[256];
    // sprintf(szimg, "%s/%08d.jpg", str_image_root.c_str(), img_idx);
    sprintf(szimg, "%s/%d.jpg", str_image_root, img_idx);
    printf("processing: %s\n", szimg);

    VIDEO_FRAME_INFO_S fdFrame;
    ret = CVI_TDL_ReadImage(img_handle, szimg, &fdFrame, PIXEL_FORMAT_BGR_888);

    while (fgets(f_line, sizeof(f_line), kps_file)) {
      float box[56];  // 存储一行的数据 (17个关键点 * 3 + 5)
      char *token = strtok(f_line, " \t\n");
      int count = 0;
      
      while (token != NULL && count < 56) {
        box[count++] = atof(token);
        token = strtok(NULL, " \t\n");
      }

      if (box[4] > 0.4) {
        if (dets_size >= dets_capacity) {
          dets_capacity *= 2;
          dets = (float **)realloc(dets, dets_capacity * sizeof(float *));
        }
        dets[dets_size] = (float *)malloc(56 * sizeof(float));
        memcpy(dets[dets_size], box, 56 * sizeof(float));
        dets_size++;
      }
    }

    fclose(kps_file);
    // if (dets.size() > 0) {
    //   printf("dets.size(): %d, dets[0].size()%d\n", dets.size(), dets[0].size());
    // }

    // getchar();

    stObjMeta.info = (cvtdl_object_info_t *)malloc(sizeof(cvtdl_object_info_t) * dets_size);
    memset(stObjMeta.info, 0, sizeof(cvtdl_object_info_t) * stObjMeta.size);

    for (uint32_t i = 0; i < dets_size; i++) {
      stObjMeta.info[i].bbox.x1 = dets[i][0];
      stObjMeta.info[i].bbox.y1 = dets[i][1];
      stObjMeta.info[i].bbox.x2 = dets[i][2];
      stObjMeta.info[i].bbox.y2 = dets[i][3];
      stObjMeta.info[i].bbox.score = dets[i][4];
      stObjMeta.info[i].classes = 0;

      stObjMeta.info[i].pedestrian_properity =
          (cvtdl_pedestrian_meta *)malloc(sizeof(cvtdl_pedestrian_meta));

      for (int j = 0; j < 17; j++) {
        stObjMeta.info[i].pedestrian_properity->pose_17.x[j] = dets[i][5 + j * 3];
        stObjMeta.info[i].pedestrian_properity->pose_17.y[j] = dets[i][5 + j * 3 + 1];
        stObjMeta.info[i].pedestrian_properity->pose_17.score[j] = dets[i][5 + j * 3 + 2];
      }
    }

    ret = CVI_TDL_DeepSORT_Obj(tdl_handle, &stObjMeta, &stTrackerMeta, false);
    if (ret != CVI_SUCCESS) {
      printf("CVI_TDL_DeepSORT_Obj failed with %#x!\n", ret);
      goto inf_error;
    }
#ifdef DEBUG_FALL
    printf("#####################  stObjMeta size: %d #########################\n", stObjMeta.size);
    for (int i = 0; i < stObjMeta.size; i++) {
        printf("[%.2f,%.2f,%.2f,%.2f],\n",
             stObjMeta.info[i].bbox.x1,
             stObjMeta.info[i].bbox.y1,
             stObjMeta.info[i].bbox.x2,
             stObjMeta.info[i].bbox.y2);
      // for (int j = 0; j < 17; j++) {
      //   std::cout << j << ": " << stObjMeta.info[i].pedestrian_properity->pose_17.x[j] << " "
      //             << stObjMeta.info[i].pedestrian_properity->pose_17.y[j] << " "
      //             << stObjMeta.info[i].pedestrian_properity->pose_17.score[j] << std::endl;
      // }
    }
#endif
    ret = CVI_TDL_Fall_Monitor(tdl_handle, &stObjMeta);
    if (ret != CVI_SUCCESS) {
      printf("monitor failed with %#x!\n", ret);
      return -1;
    }

    for (uint32_t i = 0; i < stObjMeta.size; i++) {
      if (stObjMeta.info[i].pedestrian_properity->fall) {
        printf("Falling !!!\n");
        fall = true;
      }
    }

    sprintf(save_img_, "/mnt/data/3_data/infer/fall_det/show/%s/%d.jpg", argv[5], img_idx);
    printf("save_img_: %s\n", save_img_);

    // if (stObjMeta.size > 0) {
    //   sprintf(text_, "id:%ld, a:%.1f,r:%.2f,s:%.1f,m:%d,st:%d", stObjMeta.info[0].unique_id,
    //           stObjMeta.info[0].human_angle, stObjMeta.info[0].aspect_ratio,
    //           stObjMeta.info[0].speed, stObjMeta.info[0].is_moving, stObjMeta.info[0].status);
    // }
    printf("#####################  stObjMeta size: %d #########################\n", stObjMeta.size);

    if (atoi(argv[7])) {
      CVI_TDL_ShowKeypointsAndText(&fdFrame, &stObjMeta, save_img_, text_, 0.5);
    }

    // printf("to release frame\n");
  inf_error:
    CVI_TDL_ReleaseImage(img_handle, &fdFrame);
    CVI_TDL_Free(&stObjMeta);
    CVI_TDL_Free(&stTrackerMeta);
  }

  int prediction = fall == true ? 1 : 0;
  char save_info[256];
  sprintf(save_info, "%s %d", str_image_root, prediction);
  fwrite(save_info, strlen(save_info), 1, fp);
  fclose(fp);
  // getchar();

  printf("to release system\n");
  // CVI_IVE_DestroyHandle(ive_handle);
  bRunImageWriter = false;
  bRunVideoOutput = false;
  // pthread_join(io_thread, NULL);
  CVI_TDL_Destroy_ImageProcessor(img_handle);
  // CVI_TDL_APP_DestroyHandle(app_handle);
  CVI_TDL_Service_DestroyHandle(service_handle);
  CVI_TDL_DestroyHandle(tdl_handle);
  // DestroyVideoSystem(&vs_ctx);
  CVI_SYS_Exit();
  CVI_VB_Exit();
}
