#include "core/utils/vpss_helper.h"
#include "cvi_tdl.h"
#include "cvi_tdl_app/cvi_tdl_app.h"
#include "sample_comm.h"
// #include "vi_vo_utils.h"

#include <cvi_sys.h>
#include <cvi_vb.h>
#include <cvi_vi.h>

#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <chrono>
#include <sstream>
#include <string>
#include "cvi_tdl_media.h"
#include "stb_image.h"
#include "stb_image_write.h"
#include "sys_utils.hpp"
#define OUTPUT_BUFFER_SIZE 10
#define MODE_DEFINITION 0

// #define USE_OUTPUT_DATA_API

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
} IOData;

SMT_MUTEXAUTOLOCK_INIT(IOMutex);
SMT_MUTEXAUTOLOCK_INIT(VOMutex);

/* global variables */
static volatile bool bExit = false;

int rear_idx = 0;
int front_idx = 0;

std::string g_out_dir;

/* helper functions */

bool CHECK_OUTPUT_CONDITION(person_capture_t *person_cpt_info, uint32_t idx, APP_MODE_e mode);

/**
 * Restructure the object meta of the person capture to 2 output object struct.
 * 0: Low quality, 1: Otherwise (Ignore unstable trackers)
 */
void RESTRUCTURING_OBJ_META(person_capture_t *person_cpt_info, cvtdl_object_t *obj_meta_0,
                            cvtdl_object_t *obj_meta_1);

#ifdef USE_OUTPUT_DATA_API
uint32_t GENERATE_OUTPUT_DATA(IOData **output_data, person_capture_t *person_cpt_info);
void FREE_OUTPUT_DATA(IOData *output_data, uint32_t size);
#endif

static void SampleHandleSig(CVI_S32 signo) {
  signal(SIGINT, SIG_IGN);
  signal(SIGTERM, SIG_IGN);

  if (SIGINT == signo || SIGTERM == signo) {
    bExit = true;
  }
}

std::string capobj_to_str(person_cpt_data_t *p_obj, float w, float h, int lb) {
  std::stringstream ss;
  // ss<<p_obj->_timestamp<<",4,";
  float ctx = (p_obj->info.bbox.x1 + p_obj->info.bbox.x2) / 2.0 / w;
  float cty = (p_obj->info.bbox.y1 + p_obj->info.bbox.y2) / 2.0 / h;
  float ww = (p_obj->info.bbox.x2 - p_obj->info.bbox.x1) / w;
  float hh = (p_obj->info.bbox.y2 - p_obj->info.bbox.y1) / h;
  ss << p_obj->_timestamp << "," << lb << "," << ctx << "," << cty << "," << ww << "," << hh << ","
     << p_obj->info.unique_id << "," << p_obj->info.bbox.score << "\n";
  return ss.str();
}

void export_tracking_info(personvehicle_capture_t *p_cap_info, const std::string &str_dst_dir,
                          int frame_id, float imgw, float imgh, int lb) {
  char sz_dstf[128];
  sprintf(sz_dstf, "%s/%08d.txt", str_dst_dir.c_str(), frame_id);
  printf("txt  :%s\n", sz_dstf);
  FILE *fp = fopen(sz_dstf, "w");

  cvtdl_object_t *p_objinfo = &(p_cap_info->last_objects);
  if (p_objinfo->size == 0) return;
  for (uint32_t i = 0; i < p_objinfo->size; i++) {
    char szinfo[128];
    float ww = (p_objinfo->info[i].bbox.x2 - p_objinfo->info[i].bbox.x1);
    float hh = (p_objinfo->info[i].bbox.y2 - p_objinfo->info[i].bbox.y1);
    float score = p_objinfo->info[i].bbox.score;
    bool is_cross = p_objinfo->info[i].is_cross;
    sprintf(szinfo, "%d,%" PRIu64 ",%f,%f,%f,%f,%f,%d,-1,-1,-1,0\n", frame_id,
            p_objinfo->info[i].unique_id, p_objinfo->info[i].bbox.x1, p_objinfo->info[i].bbox.y1,
            ww, hh, score, is_cross);
    fwrite(szinfo, 1, strlen(szinfo), fp);
    // }
  }

  fclose(fp);
}
void release_system(cvitdl_handle_t tdl_handle, cvitdl_service_handle_t service_handle,
                    cvitdl_app_handle_t app_handle) {
  CVI_TDL_APP_DestroyHandle(app_handle);
  CVI_TDL_Service_DestroyHandle(service_handle);
  CVI_TDL_DestroyHandle(tdl_handle);
  // DestroyVideoSystem(&vs_ctx);
  CVI_SYS_Exit();
  CVI_VB_Exit();
}

int main(int argc, char *argv[]) {
  CVI_S32 ret = CVI_SUCCESS;
  // Set signal catch
  signal(SIGINT, SampleHandleSig);
  signal(SIGTERM, SampleHandleSig);

  std::string str_model_file = argv[1];
  std::string str_image_root(argv[2]);
  std::string str_dst_root(argv[3]);
  // width num,height num
  int w_num = atoi(argv[4]);
  int h_num = atoi(argv[5]);

  // set regins number
  int regins_num = atoi(argv[6]);  // the number of areas
  bool regin_flags[1000];
  memset(regin_flags, false, sizeof(bool));
  for (int i = 0; i < regins_num; i++) {
    regin_flags[atoi(argv[7 + i])] = true;
  }
  // model path
  const char *od_model_name = "yolov8";

  const char *od_model_path = str_model_file.c_str();
  const char *reid_model_path = "NULL";
  // const char *mode_id = intelligent;//argv[5];//leave=2,intelligent=3

  int buffer_size = 10;       // atoi(argv[6]);//10
  float det_threshold = 0.5;  // atof(argv[7]);//0.5

  // saving dir path
  std::string str_dst_video = join_path(str_dst_root, get_directory_name(str_image_root));
  if (!create_directory(str_dst_video)) {
    std::cout << "create directory:" << str_dst_video << " failed\n";
  }
  g_out_dir = str_dst_video;

  if (buffer_size <= 0) {
    printf("buffer size must be larger than 0.\n");
    return CVI_FAILURE;
  }

  cvitdl_handle_t tdl_handle = NULL;
  cvitdl_service_handle_t service_handle = NULL;
  cvitdl_app_handle_t app_handle = NULL;

  ret = CVI_TDL_CreateHandle2(&tdl_handle, 1, 0);
  if (ret != CVI_SUCCESS) {
    printf("failed with %#x!\n", ret);
    release_system(tdl_handle, service_handle, app_handle);
    return CVI_FAILURE;
  }
  ret |= CVI_TDL_Service_CreateHandle(&service_handle, tdl_handle);

  if (ret != CVI_SUCCESS) {
    printf("failed with %#x!\n", ret);
    release_system(tdl_handle, service_handle, app_handle);
    return CVI_FAILURE;
  }
  ret |= CVI_TDL_APP_CreateHandle(&app_handle, tdl_handle);

  if (ret != CVI_SUCCESS) {
    printf("failed with %#x!\n", ret);
    release_system(tdl_handle, service_handle, app_handle);
    return CVI_FAILURE;
  }
  ret |= CVI_TDL_APP_PersonVehicleCapture_Init(app_handle, (uint32_t)buffer_size);
  if (ret != CVI_SUCCESS) {
    printf("failed with %#x!\n", ret);
    release_system(tdl_handle, service_handle, app_handle);
    return CVI_FAILURE;
  }
  ret |= CVI_TDL_APP_PersonVehicleCapture_QuickSetUp(
      app_handle, od_model_name, od_model_path,
      (!strcmp(reid_model_path, "NULL")) ? NULL : reid_model_path);

  ret |= CVI_TDL_APP_PersonVehicleCaptureIrregular_Region(app_handle, h_num, w_num,
                                                          regin_flags);  // set region
  if (ret != CVI_TDL_SUCCESS) {
    printf("failed with %#x!\n", ret);
    release_system(tdl_handle, service_handle, app_handle);
    return CVI_FAILURE;
  }
  CVI_TDL_SetModelThreshold(tdl_handle, app_handle->personvehicle_cpt_info->od_model_index,
                            det_threshold);

  const CVI_S32 vpssgrp_width = 1920;
  const CVI_S32 vpssgrp_height = 1080;

  ret = MMF_INIT_HELPER2(vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888, 3, vpssgrp_width,
                         vpssgrp_height, PIXEL_FORMAT_RGB_888_PLANAR, 3);
  if (ret != CVI_SUCCESS) {
    printf("Init sys failed with %#x!\n", ret);
    return ret;
  }

  personvehicle_capture_config_t app_cfg;
  CVI_TDL_APP_PersonVehicleCapture_GetDefaultConfig(&app_cfg);
  CVI_TDL_APP_PersonVehicleCapture_SetConfig(app_handle, &app_cfg);

  std::string cap_result = str_dst_video + std::string("/cap_result.log");
  //   FILE *fp = fopen(cap_result.c_str(), "w");

  int num_append = 0;
  double time_elapsed = 0;
  int num_images = 0;
  imgprocess_t img_handle;

  printf("strat.............................\n");
  std::vector<std::string> image_files = getImgList(str_image_root);
  for (uint32_t img_idx = 0; img_idx < 7000; img_idx++) {
    // std::string img_path = str_image_root + "/" + image_files[img_idx];
    char img_path[256];

    sprintf(img_path, "%s/%08d.jpg", str_image_root.c_str(), img_idx);

    std::cout << "processing:" << img_idx << "/" << 7000 << ":" << img_path << "\n";
    if (num_append > 30) break;

    bool empty_img = false;
    VIDEO_FRAME_INFO_S fdFrame;

    ret = CVI_TDL_ReadImage(img_handle, img_path, &fdFrame, PIXEL_FORMAT_RGB_888_PLANAR);

    // ret = CVI_TDL_ReadImage(img_path, &fdFrame, PIXEL_FORMAT_RGB_888_PLANAR);
    if (ret != CVI_SUCCESS) {
      std::cout << "Convert to video frame failed with:" << ret << ",file:" << str_image_root
                << std::endl;
      num_append += 1;
      continue;
    }

    if (num_append >= 5) break;

    // printf("ALIVE persons: %d\n", alive_person_num);
    std::chrono::steady_clock::time_point tick0 = std::chrono::steady_clock::now();
    ret = CVI_TDL_APP_PersonVehicleCaptureIrregular_Run(app_handle, &fdFrame);
    std::chrono::steady_clock::time_point tick1 = std::chrono::steady_clock::now();
    if (!empty_img) {
      time_elapsed +=
          std::chrono::duration_cast<std::chrono::microseconds>(tick1 - tick0).count() / 1000.0;
      num_images += 1;
    }

    if (ret != CVI_SUCCESS) {
      printf("CVI_TDL_APP_PersonVehicleCapture_Run failed with %#x\n", ret);
      break;
    }
    export_tracking_info(app_handle->personvehicle_cpt_info, str_dst_video, img_idx,
                         fdFrame.stVFrame.u32Width, fdFrame.stVFrame.u32Height, 4);

    // CVI_TDL_ReleaseImage(&fdFrame);
    CVI_TDL_ReleaseImage(img_handle, &fdFrame);
  }
  //   fclose(fp);

  // bRunVideoOutput = false;
  //   pthread_join(io_thread, NULL);

  //   CVI_IVE_DestroyHandle(ive_handle);
  CVI_TDL_Service_DestroyHandle(service_handle);
  // DestroyVideoSystem(&vs_ctx);
  CVI_SYS_Exit();
  CVI_VB_Exit();
  CVI_TDL_DestroyHandle(tdl_handle);
  CVI_TDL_APP_DestroyHandle(app_handle);

  // std::cout << "numimgs:" << num_images << ",ms_per_frame:" << time_elapsed / num_images
  //           << std::endl;
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
    *tmp_ptr += 1;
  }
  return;
}
