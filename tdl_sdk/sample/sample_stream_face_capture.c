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

#include <sys/time.h>
#include "stb_image.h"
#include "stb_image_write.h"
#define OUTPUT_BUFFER_SIZE 10

float g_draw_clrs[] = {0,   0,   0,   255, 0,   0, 0,   255, 0,   0,  0,
                       255, 255, 255, 0,   255, 0, 255, 0,   255, 255};

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
  uint64_t frame_id;
} IOData;

typedef struct {
  VideoSystemContext vs_ctx;
  cvitdl_service_handle_t service_handle;
  cvitdl_handle_t tdl_handle;
  char sz_ped_model[128];
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

static cvtdl_face_t g_face_meta_0;   // face
static cvtdl_object_t g_obj_meta_0;  // person
char g_out_dir[128];

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
    int frame_id = (int)data_buffer[target_idx].frame_id;
    int track_id = (int)data_buffer[target_idx].u_id;

    char filename[512];
    if (data_buffer[target_idx].image.width == 0) {
      printf("[WARNING] Target image is empty.\n");
    } else {
      if (data_buffer[target_idx].image.pix_format == PIXEL_FORMAT_RGB_888) {
        sprintf(filename, "%s/frm_%d_face_%d_%u_qua_%.3f.png", g_out_dir, frame_id, track_id,
                data_buffer[target_idx].counter, data_buffer[target_idx].quality);
        printf("to output:%s\n", filename);
        stbi_write_png(filename, data_buffer[target_idx].image.width,
                       data_buffer[target_idx].image.height, STBI_rgb,
                       data_buffer[target_idx].image.pix[0],
                       data_buffer[target_idx].image.stride[0]);
      } else {
        printf("to output image format:%d,not :%d\n", (int)data_buffer[target_idx].image.pix_format,
               (int)PIXEL_FORMAT_RGB_888);
        // sprintf(filename, "image/frm_%d_face_%d_%u_score_%.3f_qua_%.3f_name_%s.bin",
        //         int(data_buffer[target_idx].frame_id), int(data_buffer[target_idx].u_id),
        //         data_buffer[target_idx].counter, data_buffer[target_idx].match_score,
        //         data_buffer[target_idx].quality, data_buffer[target_idx].name);
        // CVI_TDL_DumpImage(filename, &data_buffer[target_idx].image);
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

void visualize_frame(VideoSystemContext *vs_ctx, cvitdl_service_handle_t service_handle,
                     VIDEO_FRAME_INFO_S stVOFrame, cvtdl_face_t *face_meta_0,
                     cvtdl_object_t *obj_meta_0) {
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
    char strinfo[128];
    int tid = (int)face_meta_0->info[i].unique_id;
    float quality = face_meta_0->info[i].face_quality;
    sprintf(strinfo, "id:%d_%.3f", tid, quality);
    CVI_TDL_Service_ObjectWriteText(strinfo, face_meta_0->info[i].bbox.x1,
                                    face_meta_0->info[i].bbox.y1, &stVOFrame, 200.1, 0, 0);
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
    cvtdl_object_t obj_met_tdl = *obj_meta_0;
    obj_met_tdl.size = 1;
    obj_met_tdl.info = &obj_meta_0->info[i];
    CVI_TDL_Service_ObjectDrawRect(service_handle, &obj_met_tdl, &stVOFrame, false, brushi);
    char strinfo[128];
    int tid = (int)obj_meta_0->info[i].unique_id;
    sprintf(strinfo, "id:%d", tid);
    CVI_TDL_Service_ObjectWriteText(strinfo, obj_meta_0->info[i].bbox.x1,
                                    obj_meta_0->info[i].bbox.y1, &stVOFrame, 0, 200, 0);
  }

  CVI_SYS_Munmap((void *)stVOFrame.stVFrame.pu8VirAddr[0], image_size);
  stVOFrame.stVFrame.pu8VirAddr[0] = NULL;
  stVOFrame.stVFrame.pu8VirAddr[1] = NULL;
  stVOFrame.stVFrame.pu8VirAddr[2] = NULL;

  int s32Ret = SendOutputFrame(&stVOFrame, &vs_ctx->outputContext);
  if (s32Ret != CVI_SUCCESS) {
    printf("Send Output Frame NG\n");
  }
}
static void *pVideoOutput(void *args) {
  printf("[APP] Video Output Up\n");
  pVOArgs *vo_args = (pVOArgs *)args;

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
  while (bRunVideoOutput) {
    s32Ret = CVI_VPSS_GetChnFrame(vo_args->vs_ctx.vpssConfigs.vpssGrp,
                                  vo_args->vs_ctx.vpssConfigs.vpssChnVideoOutput, &stVOFrame, 1000);
    if (s32Ret != CVI_SUCCESS) {
      printf("CVI_VPSS_GetChnFrame preview failed with %#x\n", s32Ret);
      usleep(1000);
      continue;
    }

    {
      SMT_MutexAutoLock(VOMutex, lock);
      CVI_TDL_CopyFaceMeta(&g_face_meta_0, &face_meta_0);
      CVI_TDL_CopyObjectMeta(&g_obj_meta_0, &obj_meta_0);
    }
    visualize_frame(&vo_args->vs_ctx, service_handle, stVOFrame, &face_meta_0, &obj_meta_0);
    s32Ret = CVI_VPSS_ReleaseChnFrame(vo_args->vs_ctx.vpssConfigs.vpssGrp,
                                      vo_args->vs_ctx.vpssConfigs.vpssChnVideoOutput, &stVOFrame);
    if (s32Ret != CVI_SUCCESS) {
      printf("CVI_VPSS_ReleaseChnFrame chn0 NG\n");
      break;
    }

    CVI_TDL_Free(&face_meta_0);
    CVI_TDL_Free(&obj_meta_0);
  }
  return NULL;
}

int main(int argc, char *argv[]) {
  if (argc != 3 && argc != 4 && argc != 5) {
    printf("Usage: %s fdmodel_path path capture_path\n", argv[0]);
    printf("Usage: %s fdmodel_path ped_model path capture_path\n", argv[0]);
    printf("Usage: %s fdmodel_path ped_model path famodel_path capture_path\n", argv[0]);
    return CVI_FAILURE;
  }
  CVI_S32 ret = CVI_SUCCESS;
  // Set signal catch
  signal(SIGINT, SampleHandleSig);
  signal(SIGTERM, SampleHandleSig);

  const char *fd_model_path = argv[1];
  const char *ped_model_path = "NULL";
  const char *fa_model_path = "NULL";
  if (argc == 3) {
    sprintf(g_out_dir, "%s", argv[2]);
  } else if (argc == 4) {
    ped_model_path = argv[2];
    sprintf(g_out_dir, "%s", argv[3]);
  } else if (argc == 5) {
    ped_model_path = argv[2];
    fa_model_path = argv[3];
    sprintf(g_out_dir, "%s", argv[4]);
  }
  printf("ped_model:%s\n", ped_model_path);

  const char *fr_model_path = "NULL";  // argv[4];
  const char *fq_model_path = "NULL";  // argv[5];

  int buffer_size = 10;       // atoi(argv[8]);
  float det_threshold = 0.5;  // atof(argv[9]);
  bool write_image = 1;       // atoi(argv[10]) == 1;

  CVI_TDL_SUPPORTED_MODEL_E fd_model_id = CVI_TDL_SUPPORTED_MODEL_SCRFDFACE;
  CVI_TDL_SUPPORTED_MODEL_E fr_model_id = CVI_TDL_SUPPORTED_MODEL_FACERECOGNITION;

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

  ret = CVI_TDL_CreateHandle2(&tdl_handle, 2, 0);
  ret |= CVI_TDL_Service_CreateHandle(&service_handle, tdl_handle);
  // ret |= CVI_TDL_Service_EnableTPUDraw(service_handle, true);
  ret |= CVI_TDL_APP_CreateHandle(&app_handle, tdl_handle);
  ret |= CVI_TDL_APP_FaceCapture_Init(app_handle, (uint32_t)buffer_size);
  ret |= CVI_TDL_APP_FaceCapture_QuickSetUp(
      app_handle, fd_model_id, fr_model_id, fd_model_path,
      (!strcmp(fr_model_path, "NULL")) ? NULL : fr_model_path,
      (!strcmp(fq_model_path, "NULL")) ? NULL : fq_model_path, NULL,
      (!strcmp(fa_model_path, "NULL")) ? NULL : fa_model_path);
  if (strcmp(ped_model_path, "NULL")) {
    CVI_TDL_SUPPORTED_MODEL_E ped_model_id = CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PEDESTRIAN;
    ret |= CVI_TDL_APP_FaceCapture_FusePedSetup(app_handle, ped_model_id, ped_model_path);
    CVI_TDL_SetModelThreshold(tdl_handle, ped_model_id, det_threshold);
    printf("face fuse track with ped\n");
  }
  if (ret != CVI_SUCCESS) {
    printf("failed with %#x!\n", ret);
    goto CLEANUP_SYSTEM;
  }
  CVI_TDL_SetVpssTimeout(tdl_handle, 1000);

  CVI_TDL_SetModelThreshold(tdl_handle, fd_model_id, det_threshold);

  CVI_TDL_APP_FaceCapture_SetMode(app_handle, FAST);

  face_capture_config_t app_cfg;
  CVI_TDL_APP_FaceCapture_GetDefaultConfig(&app_cfg);
  app_cfg.thr_quality = 0.1;
  app_cfg.thr_size_min = 20;
  app_cfg.miss_time_limit = 40;
  app_cfg.m_interval = 500;
  app_cfg.m_capture_num = 5;
  app_cfg.store_feature = true;
  app_cfg.qa_method = 0;
  app_cfg.img_capture_flag = 0;

  CVI_TDL_APP_FaceCapture_SetConfig(app_handle, &app_cfg);

  VIDEO_FRAME_INFO_S stfdFrame;
  memset(&g_face_meta_0, 0, sizeof(cvtdl_face_t));
  memset(&g_obj_meta_0, 0, sizeof(cvtdl_object_t));

  pthread_t io_thread, vo_thread;
  pthread_create(&io_thread, NULL, pImageWrite, NULL);
  pVOArgs vo_args = {0};
  vo_args.service_handle = service_handle;
  vo_args.vs_ctx = vs_ctx;
  pthread_create(&vo_thread, NULL, pVideoOutput, (void *)&vo_args);

  size_t counter = 0;
  uint32_t last_time_ms = get_time_in_ms();
  size_t last_counter = 0;
  while (bExit == false) {
    counter += 1;

    ret = CVI_VPSS_GetChnFrame(vs_ctx.vpssConfigs.vpssGrp, vs_ctx.vpssConfigs.vpssChntdl,
                               &stfdFrame, 2000);
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
      printf("++++++++++++ALIVE Faces: %d,frame:%d,fps:%.2f\n", alive_face_num, (int)counter, fps);
    }

    ret = CVI_TDL_APP_FaceCapture_Run(app_handle, &stfdFrame);
    if (ret != CVI_SUCCESS) {
      printf("CVI_TDL_APP_FaceCapture_Run failed with %#x\n", ret);
      usleep(1000);
      CVI_VPSS_ReleaseChnFrame(vs_ctx.vpssConfigs.vpssGrp, vs_ctx.vpssConfigs.vpssChntdl,
                               &stfdFrame);
      continue;
      ;
    }

    {
      SMT_MutexAutoLock(VOMutex, lock);
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
          SMT_MutexAutoLock(IOMutex, lock);
          target_idx = (rear_idx + 1) % OUTPUT_BUFFER_SIZE;
          full = target_idx == front_idx;
        }
        if (full) {
          printf("[WARNING] Buffer is full! Drop out!");
          CVI_VPSS_ReleaseChnFrame(vs_ctx.vpssConfigs.vpssGrp, vs_ctx.vpssConfigs.vpssChntdl,
                                   &stfdFrame);
          continue;
        }
        printf("to output cropface:%d,frameid:%d\n", (int)u_id,
               (int)app_handle->face_cpt_info->data[i].cap_timestamp);
        /* Copy image data to buffer */
        data_buffer[target_idx].u_id = u_id;
        data_buffer[target_idx].quality = face_quality;
        data_buffer[target_idx].state = state;
        data_buffer[target_idx].counter = counter;
        data_buffer[target_idx].frame_id = app_handle->face_cpt_info->data[i].cap_timestamp;
        /* NOTE: Make sure the image type is IVE_IMAGE_TYPE_U8C3_PACKAGE */
        CVI_TDL_CopyImage(&app_handle->face_cpt_info->data[i].image,
                          &data_buffer[target_idx].image);
        {
          SMT_MutexAutoLock(IOMutex, lock);
          rear_idx = target_idx;
        }
      }
    }
    ret = CVI_VPSS_ReleaseChnFrame(vs_ctx.vpssConfigs.vpssGrp, vs_ctx.vpssConfigs.vpssChntdl,
                                   &stfdFrame);
    if (ret != CVI_SUCCESS) {
      printf("CVI_VPSS_ReleaseChnFrame chn0 NG\n");
      break;
    }
  }
  bRunImageWriter = false;
  bRunVideoOutput = false;
  pthread_join(io_thread, NULL);
  pthread_join(vo_thread, NULL);
  CVI_TDL_Free(&g_face_meta_0);
  CVI_TDL_Free(&g_obj_meta_0);

CLEANUP_SYSTEM:
  CVI_TDL_APP_DestroyHandle(app_handle);
  CVI_TDL_Service_DestroyHandle(service_handle);
  CVI_TDL_DestroyHandle(tdl_handle);
  DestroyVideoSystem(&vs_ctx);
  CVI_SYS_Exit();
  CVI_VB_Exit();
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
