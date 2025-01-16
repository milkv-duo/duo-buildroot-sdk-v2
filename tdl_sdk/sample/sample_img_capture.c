#include "core/utils/vpss_helper.h"
#include "cvi_tdl.h"
#include "cvi_tdl_app/cvi_tdl_app.h"
#include "sample_comm.h"

#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "cvi_tdl_media.h"
#include "stb_image.h"
#include "stb_image_write.h"
#include "sys_utils.h"
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

// typedef struct {
//   CVI_S32 voType;
//   VideoSystemContext vs_ctx;
//   cvitdl_service_handle_t service_handle;
// } pVOArgs;

SMT_MUTEXAUTOLOCK_INIT(IOMutex);
SMT_MUTEXAUTOLOCK_INIT(VOMutex);

/* global variables */
static volatile bool bExit = false;
static volatile bool bRunImageWriter = true;
// static volatile bool bRunVideoOutput = true;

int rear_idx = 0;
int front_idx = 0;
static IOData data_buffer[OUTPUT_BUFFER_SIZE];

static cvtdl_object_t g_obj_meta_0;
static cvtdl_object_t g_obj_meta_1;

static APP_MODE_e app_mode;
char* g_out_dir;

/* helper functions */
// bool READ_CONFIG(const char *config_path, person_capture_config_t *app_config);

bool CHECK_OUTPUT_CONDITION(person_capture_t *person_cpt_info, uint32_t idx, APP_MODE_e mode);

/**
 * Restructure the object meta of the person capture to 2 output object struct.
 * 0: Low quality, 1: Otherwise (Ignore unstable trackers)
 */
void RESTRUCTURING_OBJ_META(person_capture_t *person_cpt_info, cvtdl_object_t *obj_meta_0,
                            cvtdl_object_t *obj_meta_1);

int COUNT_ALIVE(person_capture_t *person_cpt_info);

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

/* Consumer */
static void *pImageWrite(void *args) {
  printf("[APP] Image Write Up\n");
  while (bRunImageWriter) {
    /* only consumer write front_idx */
    bool empty;
    {
      SMT_MutexAutoLock(IOMutex, lock);
      empty = front_idx == rear_idx;
    }
    if (empty) {
      // printf("I/O Buffer is empty.\n");
      usleep(100 * 1000);
      continue;
    }
    int target_idx = (front_idx + 1) % OUTPUT_BUFFER_SIZE;
    char filename[128];
    if ((app_mode == leave || app_mode == intelligent) && data_buffer[target_idx].state == MISS) {
      sprintf(filename, "%s/person_%" PRIu64 "_out.png", g_out_dir,
              data_buffer[target_idx].u_id);
    } else {
      sprintf(filename, "%s/person_%" PRIu64 "_%u.png", g_out_dir,
              data_buffer[target_idx].u_id, data_buffer[target_idx].counter);
    }
    if (data_buffer[target_idx].image.pix_format != PIXEL_FORMAT_RGB_888) {
      printf("[WARNING] Image I/O unsupported format: %d\n",
             data_buffer[target_idx].image.pix_format);
    } else {
      if (data_buffer[target_idx].image.width == 0) {
        printf("[WARNING] Target image is empty.\n");
      } else {
        printf(" > (I/O) Write Image (Q: %.2f): %s ...\n", data_buffer[target_idx].quality,
               filename);
        stbi_write_png(filename, data_buffer[target_idx].image.width,
                       data_buffer[target_idx].image.height, STBI_rgb,
                       data_buffer[target_idx].image.pix[0],
                       data_buffer[target_idx].image.stride[0]);

        /* if there is no first capture target in INTELLIGENT mode, we need to create one */
        if (app_mode == intelligent && data_buffer[target_idx].counter == 0) {
          sprintf(filename, "%s/person_%" PRIu64 "_1.png", g_out_dir,
                  data_buffer[target_idx].u_id);
          stbi_write_png(filename, data_buffer[target_idx].image.width,
                         data_buffer[target_idx].image.height, STBI_rgb,
                         data_buffer[target_idx].image.pix[0],
                         data_buffer[target_idx].image.stride[0]);
        }
      }
    }

    CVI_TDL_Free(&data_buffer[target_idx].image);
    {
      SMT_MutexAutoLock(IOMutex, lock);
      front_idx = target_idx;
    }
  }

  printf("[APP] free buffer data...\n");
  while (front_idx != rear_idx) {
    CVI_TDL_Free(&data_buffer[(front_idx + 1) % OUTPUT_BUFFER_SIZE].image);
    {
      SMT_MutexAutoLock(IOMutex, lock);
      front_idx = (front_idx + 1) % OUTPUT_BUFFER_SIZE;
    }
  }

  return NULL;
}

char* capobj_to_str(person_cpt_data_t *p_obj, float w, float h, int lb) {

  static char buffer[256];
  
  float ctx = (p_obj->info.bbox.x1 + p_obj->info.bbox.x2) / 2.0f / w;
  float cty = (p_obj->info.bbox.y1 + p_obj->info.bbox.y2) / 2.0f / h;
  float ww = (p_obj->info.bbox.x2 - p_obj->info.bbox.x1) / w;
  float hh = (p_obj->info.bbox.y2 - p_obj->info.bbox.y1) / h;
  
  snprintf(buffer, sizeof(buffer), "%d,%d,%f,%f,%f,%f,%" PRIu64 ",%f\n",
           p_obj->_timestamp, lb, ctx, cty, ww, hh,
           p_obj->info.unique_id, p_obj->info.bbox.score);
           
  return buffer;
}

void export_tracking_info(person_capture_t *p_cap_info, const char* str_dst_dir,
                          int frame_id, float imgw, float imgh, int lb) {
  cvtdl_object_t *p_objinfo = &(p_cap_info->last_objects);
  if (p_objinfo->size == 0) return;
  char sz_dstf[128];
  sprintf(sz_dstf, "%s/%08d.txt", str_dst_dir, frame_id);
  FILE *fp = fopen(sz_dstf, "w");

  for (uint32_t i = 0; i < p_objinfo->size; i++) {
    // if(p_objinfo->info[i].unique_id != 0){
    // sprintf( buf, "\nOD DB File Size = %" PRIu64 " bytes \t"
    char szinfo[128];
    float ctx = (p_objinfo->info[i].bbox.x1 + p_objinfo->info[i].bbox.x2) / 2 / imgw;
    float cty = (p_objinfo->info[i].bbox.y1 + p_objinfo->info[i].bbox.y2) / 2 / imgh;
    float ww = (p_objinfo->info[i].bbox.x2 - p_objinfo->info[i].bbox.x1) / imgw;
    float hh = (p_objinfo->info[i].bbox.y2 - p_objinfo->info[i].bbox.y1) / imgh;
    float score = p_objinfo->info[i].bbox.score;
    sprintf(szinfo, "%d %f %f %f %f %" PRIu64 " %f\n", lb, ctx, cty, ww, hh,
            p_objinfo->info[i].unique_id, score);
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

  char str_model_root[256];
  char str_image_root[256];
  char str_dst_root[256];
  strcpy(str_model_root, argv[1]);
  strcpy(str_image_root, argv[2]); 
  strcpy(str_dst_root, argv[3]);

  // model path
  const char *od_model_name = "mobiledetv2-pedestrian";
  char* str_model_file =
      join_path(str_model_root, "mobiledetv2-pedestrian-d1.cvimodel");
  const char *od_model_path = str_model_file;
  const char *reid_model_path = "NULL";
  const char *config_path = "NULL";
  // const char *mode_id = intelligent;//argv[5];//leave=2,intelligent=3

  int buffer_size = 10;       // atoi(argv[6]);//10
  float det_threshold = 0.5;  // atof(argv[7]);//0.5
  bool write_image = true;    // 1

  // saving dir path
  char* str_dst_video = join_path(str_dst_root, get_directory_name(str_image_root));
  if (!create_directory(str_dst_video)) {
    printf("create directory: %s failed\n", str_dst_video);
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
  ret |= CVI_TDL_Service_CreateHandle(&service_handle, tdl_handle);
  ret |= CVI_TDL_APP_CreateHandle(&app_handle, tdl_handle);
  ret |= CVI_TDL_APP_PersonCapture_Init(app_handle, (uint32_t)buffer_size);
  ret |= CVI_TDL_APP_PersonCapture_QuickSetUp(
      app_handle, od_model_name, od_model_path,
      (!strcmp(reid_model_path, "NULL")) ? NULL : reid_model_path);
  if (ret != CVI_SUCCESS) {
    printf("failed with %#x!\n", ret);
    release_system(tdl_handle, service_handle, app_handle);
    return CVI_FAILURE;
  }

  CVI_TDL_SetModelThreshold(tdl_handle, app_handle->person_cpt_info->od_model_index, det_threshold);

  const CVI_S32 vpssgrp_width = 1920;
  const CVI_S32 vpssgrp_height = 1080;

  ret = MMF_INIT_HELPER2(vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888, 3, vpssgrp_width,
                         vpssgrp_height, PIXEL_FORMAT_RGB_888_PLANAR, 3);
  if (ret != CVI_SUCCESS) {
    printf("Init sys failed with %#x!\n", ret);
    return ret;
  }
  app_mode = intelligent;  // APP_MODE_e(atoi(mode_id));
  switch (app_mode) {
#if MODE_DEFINITION == 0
    case fast: {
      CVI_TDL_APP_PersonCapture_SetMode(app_handle, FAST);
    } break;
    case interval: {
      CVI_TDL_APP_PersonCapture_SetMode(app_handle, CYCLE);
    } break;
    case leave: {
      CVI_TDL_APP_PersonCapture_SetMode(app_handle, AUTO);
    } break;
    case intelligent: {
      CVI_TDL_APP_PersonCapture_SetMode(app_handle, AUTO);
    } break;
#elif MODE_DEFINITION == 1
    case high_quality: {
      CVI_TDL_APP_PersonCapture_SetMode(app_handle, AUTO);
    } break;
    case quick: {
      CVI_TDL_APP_PersonCapture_SetMode(app_handle, FAST);
    } break;
#else
#error "Unexpected value of MODE_DEFINITION."
#endif
    default:
      printf("Unknown mode %d\n", app_mode);
      release_system(tdl_handle, service_handle, app_handle);
      return CVI_FAILURE;
  }

  person_capture_config_t app_cfg;
  CVI_TDL_APP_PersonCapture_GetDefaultConfig(&app_cfg);
  if (!strcmp(config_path, "NULL")) {
    printf("Use Default Config...\n");
  } else {
    release_system(tdl_handle, service_handle, app_handle);
    return CVI_FAILURE;
  }
  app_cfg.thr_quality = 0.1;
  app_cfg.thr_aspect_ratio_min = 0.3;
  app_cfg.thr_aspect_ratio_max = 3.3;
  app_cfg.thr_area_min = 40 * 50;
  app_cfg.miss_time_limit = 10;
  app_cfg.store_RGB888 = true;

  CVI_TDL_APP_PersonCapture_SetConfig(app_handle, &app_cfg);

  memset(&g_obj_meta_0, 0, sizeof(cvtdl_object_t));
  memset(&g_obj_meta_1, 0, sizeof(cvtdl_object_t));

  pthread_t io_thread;
  pthread_create(&io_thread, NULL, pImageWrite, NULL);

  char cap_result[256];
  snprintf(cap_result, sizeof(cap_result), "%s/cap_result.log", str_dst_video);
  FILE *fp = fopen(cap_result, "w");

  int num_append = 0;
  uint32_t imgw = 0, imgh = 0;
  double time_elapsed = 0;
  int num_images = 0;

  imgprocess_t img_handle;
  CVI_TDL_Create_ImageProcessor(&img_handle);
  int file_count = 0;
  char** image_files = getImgList(str_image_root,&file_count);
  char img_path[256];
  for (uint32_t img_idx = 0; img_idx < file_count; img_idx++) {
    snprintf(img_path, sizeof(img_path), "%s/%s", str_image_root, image_files[img_idx]);
    printf("processing: %u/%d: %s\n", img_idx, file_count, img_path);
    if (num_append > 30) break;

    char szimg[256];
    bool empty_img = false;
    VIDEO_FRAME_INFO_S fdFrame;
    ret = CVI_TDL_ReadImage(img_handle, img_path, &fdFrame, PIXEL_FORMAT_RGB_888_PLANAR);
    if (ret != CVI_SUCCESS) {
      printf("Convert to video frame failed with: %d, file: %s\n", ret, str_image_root);
      num_append += 1;
      continue;
    }

    if (fdFrame.stVFrame.u32Width == 0 || fdFrame.stVFrame.u32Height == 0) {
      printf("read image failed: %s\n", szimg);
      num_append += 1;
      continue;
    }

    imgw = fdFrame.stVFrame.u32Width;
    imgh = fdFrame.stVFrame.u32Height;

    int alive_person_num = COUNT_ALIVE(app_handle->person_cpt_info);
    printf("ALIVE persons: %d\n", alive_person_num);
    struct timeval start, end;
    gettimeofday(&start, NULL);
    ret = CVI_TDL_APP_PersonCapture_Run(app_handle, &fdFrame);
    gettimeofday(&end, NULL);
    if (!empty_img) {
      time_elapsed += (end.tv_sec - start.tv_sec) * 1000.0 + 
                     (end.tv_usec - start.tv_usec) / 1000.0;
      num_images += 1;
    }

    if (ret != CVI_SUCCESS) {
      printf("CVI_TDL_APP_PersonCapture_Run failed with %#x\n", ret);
      break;
    }

    {
      SMT_MutexAutoLock(VOMutex, lock);
      CVI_TDL_Free(&g_obj_meta_0);
      CVI_TDL_Free(&g_obj_meta_1);
      RESTRUCTURING_OBJ_META(app_handle->person_cpt_info, &g_obj_meta_0, &g_obj_meta_1);
    }

    /* Producer */
    if (write_image) {
      if (!empty_img)
        export_tracking_info(app_handle->person_cpt_info, str_dst_video, img_idx,
                             fdFrame.stVFrame.u32Width, fdFrame.stVFrame.u32Height, 4);
      for (uint32_t i = 0; i < app_handle->person_cpt_info->size; i++) {
        if (!CHECK_OUTPUT_CONDITION(app_handle->person_cpt_info, i, app_mode)) {
          continue;
        }
        tracker_state_e state = app_handle->person_cpt_info->data[i].state;
        uint32_t counter = app_handle->person_cpt_info->data[i]._out_counter;
        uint64_t u_id = app_handle->person_cpt_info->data[i].info.unique_id;
        float quality = app_handle->person_cpt_info->data[i].quality;
        if (state == MISS) {
          printf("Produce person-%" PRIu64 "_out\n", u_id);
        } else {
          printf("Produce person-%" PRIu64 "_%u\n", u_id, counter);
          continue;
        }
        char* str_res = capobj_to_str(&app_handle->person_cpt_info->data[i], imgw, imgh, 4);
        // ss<<str_res;
        fwrite(str_res, 1, strlen(str_res), fp);
        /* Check output buffer space */
        bool full;
        int target_idx;
        {
          SMT_MutexAutoLock(IOMutex, lock);
          target_idx = (rear_idx + 1) % OUTPUT_BUFFER_SIZE;
          full = target_idx == front_idx;
        }
        if (full) {
          printf("[WARNING] Buffer is full! Drop out!");
          continue;
        }
        /* Copy image data to buffer */
        data_buffer[target_idx].u_id = u_id;
        data_buffer[target_idx].quality = quality;
        data_buffer[target_idx].state = state;
        data_buffer[target_idx].counter = counter;

        CVI_TDL_CopyImage(&app_handle->person_cpt_info->data[i].image,
                          &data_buffer[target_idx].image);
        {
          SMT_MutexAutoLock(IOMutex, lock);
          rear_idx = target_idx;
        }
      }
    }
    CVI_TDL_ReleaseImage(img_handle, &fdFrame);
  }
  fclose(fp);

  bRunImageWriter = false;
  // bRunVideoOutput = false;
  pthread_join(io_thread, NULL);

  // CLEANUP_SYSTEM:
  CVI_TDL_APP_DestroyHandle(app_handle);
  CVI_TDL_Service_DestroyHandle(service_handle);
  CVI_TDL_DestroyHandle(tdl_handle);
  CVI_TDL_Destroy_ImageProcessor(img_handle);
  // DestroyVideoSystem(&vs_ctx);
  CVI_SYS_Exit();
  CVI_VB_Exit();
}

int COUNT_ALIVE(person_capture_t *person_cpt_info) {
  int counter = 0;
  for (uint32_t j = 0; j < person_cpt_info->size; j++) {
    if (person_cpt_info->data[j].state == ALIVE) {
      counter += 1;
    }
  }
  return counter;
}

void RESTRUCTURING_OBJ_META(person_capture_t *person_cpt_info, cvtdl_object_t *obj_meta_0,
                            cvtdl_object_t *obj_meta_1) {
  obj_meta_0->size = 0;
  obj_meta_1->size = 0;
  for (uint32_t i = 0; i < person_cpt_info->last_objects.size; i++) {
    if (person_cpt_info->last_trackers.info[i].state != CVI_TRACKER_STABLE) {
      continue;
    }
    if (person_cpt_info->last_quality[i] >= person_cpt_info->cfg.thr_quality) {
      obj_meta_1->size += 1;
    } else {
      obj_meta_0->size += 1;
    }
  }

  obj_meta_0->info = (cvtdl_object_info_t *)malloc(sizeof(cvtdl_object_info_t) * obj_meta_0->size);
  memset(obj_meta_0->info, 0, sizeof(cvtdl_object_info_t) * obj_meta_0->size);
  obj_meta_0->rescale_type = person_cpt_info->last_objects.rescale_type;
  obj_meta_0->height = person_cpt_info->last_objects.height;
  obj_meta_0->width = person_cpt_info->last_objects.width;

  obj_meta_1->info = (cvtdl_object_info_t *)malloc(sizeof(cvtdl_object_info_t) * obj_meta_1->size);
  memset(obj_meta_1->info, 0, sizeof(cvtdl_object_info_t) * obj_meta_1->size);
  obj_meta_1->rescale_type = person_cpt_info->last_objects.rescale_type;
  obj_meta_1->height = person_cpt_info->last_objects.height;
  obj_meta_1->width = person_cpt_info->last_objects.width;

  cvtdl_object_info_t *info_ptr_0 = obj_meta_0->info;
  cvtdl_object_info_t *info_ptr_1 = obj_meta_1->info;
  for (uint32_t i = 0; i < person_cpt_info->last_objects.size; i++) {
    if (person_cpt_info->last_trackers.info[i].state != CVI_TRACKER_STABLE) {
      continue;
    }
    bool qualified = person_cpt_info->last_quality[i] >= person_cpt_info->cfg.thr_quality;
    cvtdl_object_info_t **tmp_ptr = (qualified) ? &info_ptr_1 : &info_ptr_0;
    (*tmp_ptr)->unique_id = person_cpt_info->last_objects.info[i].unique_id;
    // (*tmp_ptr)->face_quality = face_cpt_info->last_faces.info[i].face_quality;
    memcpy(&(*tmp_ptr)->bbox, &person_cpt_info->last_objects.info[i].bbox, sizeof(cvtdl_bbox_t));
    *tmp_ptr += 1;
  }
  return;
}

bool CHECK_OUTPUT_CONDITION(person_capture_t *person_cpt_info, uint32_t idx, APP_MODE_e mode) {
  if (!person_cpt_info->_output[idx]) return false;
  if (mode == leave && person_cpt_info->data[idx].state != MISS) return false;
  return true;
}
