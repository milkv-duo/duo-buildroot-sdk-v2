#include "core/utils/vpss_helper.h"
#include "cvi_tdl.h"
#include "cvi_tdl_app.h"
#include "sample_comm.h"
#include "vi_vo_utils.h"

#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

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

typedef struct {
  VideoSystemContext vs_ctx;
  cvitdl_service_handle_t service_handle;
} pVOArgs;

SMT_MUTEXAUTOLOCK_INIT(IOMutex);
SMT_MUTEXAUTOLOCK_INIT(VOMutex);

/* global variables */
static volatile bool bExit = false;
static volatile bool bRunImageWriter = true;
static volatile bool bRunVideoOutput = true;

int rear_idx = 0;
int front_idx = 0;
static IOData data_buffer[OUTPUT_BUFFER_SIZE];

static cvtdl_object_t g_obj_meta_0;
static cvtdl_object_t g_obj_meta_1;

static APP_MODE_e app_mode;

/* helper functions */
bool READ_CONFIG(const char *config_path, person_capture_config_t *app_config);

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
      printf("I/O Buffer is empty.\n");
      usleep(100 * 1000);
      continue;
    }
    int target_idx = (front_idx + 1) % OUTPUT_BUFFER_SIZE;
    char *filename = calloc(64, sizeof(char));
    if ((app_mode == leave || app_mode == intelligent) && data_buffer[target_idx].state == MISS) {
      sprintf(filename, "images/person_%" PRIu64 "_out.png", data_buffer[target_idx].u_id);
    } else {
      sprintf(filename, "images/person_%" PRIu64 "_%u.png", data_buffer[target_idx].u_id,
              data_buffer[target_idx].counter);
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
          sprintf(filename, "images/person_%" PRIu64 "_1.png", data_buffer[target_idx].u_id);
          stbi_write_png(filename, data_buffer[target_idx].image.width,
                         data_buffer[target_idx].image.height, STBI_rgb,
                         data_buffer[target_idx].image.pix[0],
                         data_buffer[target_idx].image.stride[0]);
        }
      }
    }

    free(filename);
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

static void *pVideoOutput(void *args) {
  printf("[APP] Video Output Up\n");
  pVOArgs *vo_args = (pVOArgs *)args;

  cvitdl_service_handle_t service_handle = vo_args->service_handle;
  CVI_S32 s32Ret = CVI_SUCCESS;

  cvtdl_service_brush_t brush_0 = {.size = 4, .color.r = 0, .color.g = 64, .color.b = 255};
  cvtdl_service_brush_t brush_1 = {.size = 8, .color.r = 0, .color.g = 255, .color.b = 0};

  cvtdl_object_t obj_meta_0;
  cvtdl_object_t obj_meta_1;
  memset(&obj_meta_0, 0, sizeof(cvtdl_object_t));
  memset(&obj_meta_1, 0, sizeof(cvtdl_object_t));

  VIDEO_FRAME_INFO_S stVOFrame;
  while (bRunVideoOutput) {
    s32Ret = CVI_VPSS_GetChnFrame(vo_args->vs_ctx.vpssConfigs.vpssGrp,
                                  vo_args->vs_ctx.vpssConfigs.vpssChnVideoOutput, &stVOFrame, 1000);
    if (s32Ret != CVI_SUCCESS) {
      printf("CVI_VPSS_GetChnFrame chn0 failed with %#x\n", s32Ret);
      break;
    }

    {
      SMT_MutexAutoLock(VOMutex, lock);
      memcpy(&obj_meta_0, &g_obj_meta_0, sizeof(cvtdl_object_t));
      memcpy(&obj_meta_1, &g_obj_meta_1, sizeof(cvtdl_object_t));
      obj_meta_0.info =
          (cvtdl_object_info_t *)malloc(sizeof(cvtdl_object_info_t) * g_obj_meta_0.size);
      obj_meta_1.info =
          (cvtdl_object_info_t *)malloc(sizeof(cvtdl_object_info_t) * g_obj_meta_1.size);
      memset(obj_meta_0.info, 0, sizeof(cvtdl_object_info_t) * obj_meta_0.size);
      memset(obj_meta_1.info, 0, sizeof(cvtdl_object_info_t) * obj_meta_1.size);
      for (uint32_t i = 0; i < g_obj_meta_0.size; i++) {
        obj_meta_0.info[i].unique_id = g_obj_meta_0.info[i].unique_id;
        // face_meta_0.info[i].face_quality = g_face_meta_0.info[i].face_quality;
        memcpy(&obj_meta_0.info[i].bbox, &g_obj_meta_0.info[i].bbox, sizeof(cvtdl_bbox_t));
      }
      for (uint32_t i = 0; i < g_obj_meta_1.size; i++) {
        obj_meta_1.info[i].unique_id = g_obj_meta_1.info[i].unique_id;
        // face_meta_1.info[i].face_quality = g_face_meta_1.info[i].face_quality;
        memcpy(&obj_meta_1.info[i].bbox, &g_obj_meta_1.info[i].bbox, sizeof(cvtdl_bbox_t));
      }
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

#if 1
    for (uint32_t j = 0; j < obj_meta_0.size; j++) {
      char *id_num = calloc(64, sizeof(char));
      sprintf(id_num, "[%" PRIu64 "]", obj_meta_0.info[j].unique_id);
      CVI_TDL_Service_ObjectWriteText(id_num, obj_meta_0.info[j].bbox.x1,
                                      obj_meta_0.info[j].bbox.y1, &stVOFrame, 1, 1, 1);
      free(id_num);
    }
    for (uint32_t j = 0; j < obj_meta_1.size; j++) {
      char *id_num = calloc(64, sizeof(char));
      sprintf(id_num, "[%" PRIu64 "]", obj_meta_1.info[j].unique_id);
      CVI_TDL_Service_ObjectWriteText(id_num, obj_meta_1.info[j].bbox.x1,
                                      obj_meta_1.info[j].bbox.y1, &stVOFrame, 1, 1, 1);
      free(id_num);
    }
#endif

    CVI_SYS_Munmap((void *)stVOFrame.stVFrame.pu8VirAddr[0], image_size);
    stVOFrame.stVFrame.pu8VirAddr[0] = NULL;
    stVOFrame.stVFrame.pu8VirAddr[1] = NULL;
    stVOFrame.stVFrame.pu8VirAddr[2] = NULL;

    s32Ret = SendOutputFrame(&stVOFrame, &vo_args->vs_ctx.outputContext);
    if (s32Ret != CVI_SUCCESS) {
      printf("Send Output Frame NG\n");
    }

    s32Ret = CVI_VPSS_ReleaseChnFrame(vo_args->vs_ctx.vpssConfigs.vpssGrp,
                                      vo_args->vs_ctx.vpssConfigs.vpssChnVideoOutput, &stVOFrame);
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
  if (argc != 11) {
    printf(
        "Usage: %s <object detection model name>\n"
        "          <object detection model path>\n"
        "          <person ReID model path> (NULL: disable DeepSORT)\n"
        "          <config_path>\n"
        "          mode, 0: fast, 1: interval, 2: leave, 3: intelligent\n"
        "          tracking buffer size\n"
        "          OD threshold\n"
        "          write image (0/1)\n"
        "          video output, 0: disable, 1: output to panel, 2: output through rtsp"
        "          video input format, 0: rgb, 1: nv21, 2: yuv420, 3: rgb(planar)\n",
        argv[0]);
    return CVI_TDL_FAILURE;
  }
  CVI_S32 ret = CVI_TDL_SUCCESS;
  // Set signal catch
  signal(SIGINT, SampleHandleSig);
  signal(SIGTERM, SampleHandleSig);
  const char *od_model_name = argv[1];
  const char *od_model_path = argv[2];
  const char *reid_model_path = argv[3];
  const char *config_path = argv[4];
  const char *mode_id = argv[5];
  int buffer_size = atoi(argv[6]);
  float det_threshold = atof(argv[7]);
  bool write_image = atoi(argv[8]) == 1;

  if (buffer_size <= 0) {
    printf("buffer size must be larger than 0.\n");
    return CVI_FAILURE;
  }

  VideoSystemContext vs_ctx = {0};
  int fps = 25;
  if (InitVideoSystem(&vs_ctx, fps) != CVI_SUCCESS) {
    printf("failed to init video system\n");
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
  if (ret != CVI_TDL_SUCCESS) {
    printf("failed with %#x!\n", ret);
    goto CLEANUP_SYSTEM;
  }
  CVI_TDL_SetVpssTimeout(tdl_handle, 1000);

  CVI_TDL_SetModelThreshold(tdl_handle, app_handle->person_cpt_info->od_model_index, det_threshold);
  CVI_TDL_SelectDetectClass(tdl_handle, app_handle->person_cpt_info->od_model_index, 1,
                            CVI_TDL_DET_TYPE_PERSON);

  app_mode = atoi(mode_id);
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
      goto CLEANUP_SYSTEM;
  }

  person_capture_config_t app_cfg;
  CVI_TDL_APP_PersonCapture_GetDefaultConfig(&app_cfg);
  if (!strcmp(config_path, "NULL")) {
    printf("Use Default Config...\n");
  } else {
    printf("Read Specific Config: %s\n", config_path);
    if (!READ_CONFIG(config_path, &app_cfg)) {
      printf("[ERROR] Read Config Failed.\n");
      goto CLEANUP_SYSTEM;
    }
  }
  CVI_TDL_APP_PersonCapture_SetConfig(app_handle, &app_cfg);

  VIDEO_FRAME_INFO_S stVIFrame;

  memset(&g_obj_meta_0, 0, sizeof(cvtdl_object_t));
  memset(&g_obj_meta_1, 0, sizeof(cvtdl_object_t));

  pthread_t io_thread, vo_thread;
  pthread_create(&io_thread, NULL, pImageWrite, NULL);
  pVOArgs vo_args = {0};
  vo_args.service_handle = service_handle;
  vo_args.vs_ctx = vs_ctx;
  pthread_create(&vo_thread, NULL, pVideoOutput, (void *)&vo_args);

  size_t counter = 0;
  while (bExit == false) {
    counter += 1;
    printf("\nGet Frame %zu\n", counter);

    ret = CVI_VPSS_GetChnFrame(vs_ctx.vpssConfigs.vpssGrp, vs_ctx.vpssConfigs.vpssChntdl,
                               &stVIFrame, 2000);
    if (ret != CVI_SUCCESS) {
      printf("CVI_VPSS_GetChnFrame chn0 failed with %#x\n", ret);
      break;
    }

    int alive_person_num = COUNT_ALIVE(app_handle->person_cpt_info);
    printf("ALIVE persons: %d\n", alive_person_num);

    ret = CVI_TDL_APP_PersonCapture_Run(app_handle, &stVIFrame);
    if (ret != CVI_TDL_SUCCESS) {
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
        }
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

    /* Generate output image data */
#ifdef USE_OUTPUT_DATA_API
    IOData *sample_output_data = NULL;
    uint32_t output_num = GENERATE_OUTPUT_DATA(&sample_output_data, app_handle->person_cpt_info);
    printf("Output Data (Size = %u)\n", output_num);
    for (uint32_t i = 0; i < output_num; i++) {
      printf("Person[%u] ID: %" PRIu64 ", Quality: %.4f, Size: (%hu,%hu)\n", i,
             sample_output_data[i].u_id, sample_output_data[i].quality,
             sample_output_data[i].image.height, sample_output_data[i].image.width);
    }
    FREE_OUTPUT_DATA(sample_output_data, output_num);
#endif

    ret = CVI_VPSS_ReleaseChnFrame(vs_ctx.vpssConfigs.vpssGrp, vs_ctx.vpssConfigs.vpssChntdl,
                                   &stVIFrame);
    if (ret != CVI_SUCCESS) {
      printf("CVI_VPSS_ReleaseChnFrame chn0 NG\n");
      break;
    }
  }
  bRunImageWriter = false;
  bRunVideoOutput = false;
  pthread_join(io_thread, NULL);
  pthread_join(vo_thread, NULL);

CLEANUP_SYSTEM:
  CVI_TDL_APP_DestroyHandle(app_handle);
  CVI_TDL_Service_DestroyHandle(service_handle);
  CVI_TDL_DestroyHandle(tdl_handle);
  DestroyVideoSystem(&vs_ctx);
  CVI_SYS_Exit();
  CVI_VB_Exit();
}

#define CHAR_SIZE 64
bool READ_CONFIG(const char *config_path, person_capture_config_t *app_config) {
  char name[CHAR_SIZE];
  char value[CHAR_SIZE];
  FILE *fp = fopen(config_path, "r");
  if (fp == NULL) {
    return false;
  }
  while (!feof(fp)) {
    memset(name, 0, CHAR_SIZE);
    memset(value, 0, CHAR_SIZE);
    /*Read Data*/
    fscanf(fp, "%s %s\n", name, value);
    if (!strcmp(name, "Miss_Time_Limit")) {
      app_config->miss_time_limit = (uint32_t)atoi(value);
    } else if (!strcmp(name, "Threshold_Area_Min")) {
      app_config->thr_area_min = atoi(value);
    } else if (!strcmp(name, "Threshold_Area_Max")) {
      app_config->thr_area_max = atoi(value);
    } else if (!strcmp(name, "Threshold_Area_Base")) {
      app_config->thr_area_base = atoi(value);
    } else if (!strcmp(name, "Threshold_Aspect_Ratio_Min")) {
      app_config->thr_aspect_ratio_min = atof(value);
    } else if (!strcmp(name, "Threshold_Aspect_Ratio_Max")) {
      app_config->thr_aspect_ratio_max = atof(value);
    } else if (!strcmp(name, "Threshold_Quality")) {
      app_config->thr_quality = atof(value);
    } else if (!strcmp(name, "FAST_Mode_Interval")) {
      app_config->fast_m_interval = (uint32_t)atoi(value);
    } else if (!strcmp(name, "FAST_Mode_Capture_Num")) {
      app_config->fast_m_capture_num = (uint32_t)atoi(value);
    } else if (!strcmp(name, "CYCLE_Mode_Interval")) {
      app_config->cycle_m_interval = (uint32_t)atoi(value);
    } else if (!strcmp(name, "AUTO_Mode_Time_Limit")) {
      app_config->auto_m_time_limit = (uint32_t)atoi(value);
    } else if (!strcmp(name, "AUTO_Mode_Fast_Cap")) {
      app_config->auto_m_fast_cap = atoi(value) == 1;
    } else if (!strcmp(name, "Store_RGB888")) {
      app_config->store_RGB888 = atoi(value) == 1;
    } else {
      printf("Unknow Arg: %s\n", name);
      return false;
    }
  }
  fclose(fp);

  return true;
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

#ifdef USE_OUTPUT_DATA_API
uint32_t GENERATE_OUTPUT_DATA(IOData **output_data, person_capture_t *person_cpt_info) {
  uint32_t output_num = 0;
  for (uint32_t i = 0; i < person_cpt_info->size; i++) {
    if (CHECK_OUTPUT_CONDITION(person_cpt_info, i, app_mode)) {
      output_num += 1;
    }
  }
  if (output_num == 0) {
    *output_data = NULL;
    return 0;
  }
  *output_data = (IOData *)malloc(sizeof(IOData) * output_num);
  memset(*output_data, 0, sizeof(IOData) * output_num);

  uint32_t tmp_idx = 0;
  for (uint32_t i = 0; i < person_cpt_info->size; i++) {
    if (!CHECK_OUTPUT_CONDITION(person_cpt_info, i, app_mode)) {
      continue;
    }
    tracker_state_e state = person_cpt_info->data[i].state;
    uint32_t counter = person_cpt_info->data[i]._out_counter;
    uint64_t u_id = person_cpt_info->data[i].info.unique_id;
    float quality = person_cpt_info->data[i].quality;
    /* Copy image data to buffer */
    (*output_data)[tmp_idx].u_id = u_id;
    (*output_data)[tmp_idx].quality = quality;
    (*output_data)[tmp_idx].state = state;
    (*output_data)[tmp_idx].counter = counter;

    CVI_TDL_CopyImage(&person_cpt_info->data[i].image, &(*output_data)[tmp_idx].image);
    tmp_idx += 1;
  }
  return output_num;
}

void FREE_OUTPUT_DATA(IOData *output_data, uint32_t size) {
  if (size == 0) return;
  for (uint32_t i = 0; i < size; i++) {
    CVI_TDL_Free(&output_data[i].image);
  }
  free(output_data);
}
#endif