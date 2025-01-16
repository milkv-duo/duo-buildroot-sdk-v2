#include <cvi_ive.h>
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
#define OUTPUT_BUFFER_SIZE 10
#define MODE_DEFINITION 0
#define FACE_FEAT_SIZE 256
// #define USE_OUTPUT_DATA_API

typedef enum { fast = 0, interval, leave, intelligent } APP_MODE_e;

bool g_use_face_attribute;

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
  float gender;
  float age;
  float glass;
  float mask;
} IOData;

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

char* g_out_dir;

int COUNT_ALIVE(face_capture_t *face_cpt_info);

#ifdef VISUAL_FACE_LANDMARK
void FREE_FACE_PTS(cvtdl_face_t *face_meta);
#endif

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
/* Consumer */
static void *pImageWrite(void *args) {
  printf("[APP] pImageWrite thread up\n");
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
    if (bExit) {
      break;
    }
    int target_idx = (front_idx + 1) % OUTPUT_BUFFER_SIZE;
    char filename[256];
    if (data_buffer[target_idx].image.width == 0) {
      printf("[WARNING] Target image is empty.\n");
    } else {
      if (data_buffer[target_idx].image.pix_format == PIXEL_FORMAT_RGB_888) {
        if (!g_use_face_attribute) {
          sprintf(filename, "%s/frm_%d_face_%d_%u_score_%.3f_qua_%.3f_name_%s.png",
                  g_out_dir, (int)data_buffer[target_idx].frame_id,
                  (int)data_buffer[target_idx].u_id, data_buffer[target_idx].counter,
                  data_buffer[target_idx].match_score, data_buffer[target_idx].quality,
                  data_buffer[target_idx].name);
          stbi_write_png(filename, data_buffer[target_idx].image.width,
                         data_buffer[target_idx].image.height, STBI_rgb,
                         data_buffer[target_idx].image.pix[0],
                         data_buffer[target_idx].image.stride[0]);
        } else if (g_use_face_attribute) {
          sprintf(filename,
                  "%s/"
                  "frm_%d_face_%d_%u_score_%.3f_qua_%.3f_gender_%.3f_age_%.3f_glass_%.3f_mask_%.3f_"
                  "name_%s.png",
                  g_out_dir, (int)data_buffer[target_idx].frame_id,
                  (int)data_buffer[target_idx].u_id, data_buffer[target_idx].counter,
                  data_buffer[target_idx].match_score, data_buffer[target_idx].quality,
                  data_buffer[target_idx].name, data_buffer[target_idx].gender,
                  data_buffer[target_idx].age, data_buffer[target_idx].glass,
                  data_buffer[target_idx].mask);
          stbi_write_png(filename, data_buffer[target_idx].image.width,
                         data_buffer[target_idx].image.height, STBI_rgb,
                         data_buffer[target_idx].image.pix[0],
                         data_buffer[target_idx].image.stride[0]);
        }

      } else {
        printf("to output image format:%d,not :%d\n", (int)data_buffer[target_idx].image.pix_format,
               (int)PIXEL_FORMAT_RGB_888);
        sprintf(filename, "%s/frm_%d_face_%d_%u_score_%.3f_qua_%.3f_name_%s.bin", g_out_dir,
                (int)data_buffer[target_idx].frame_id, (int)data_buffer[target_idx].u_id,
                data_buffer[target_idx].counter, data_buffer[target_idx].match_score,
                data_buffer[target_idx].quality, data_buffer[target_idx].name);
        CVI_TDL_DumpImage(filename, &data_buffer[target_idx].image);
      }
    }

    CVI_TDL_Free(&data_buffer[target_idx].image);
    {
      SMT_MutexAutoLock(IOMutex, lock);
      front_idx = target_idx;
    }
  }

  printf("[APP] free buffer data...\n");
  for (int i = 0; i < OUTPUT_BUFFER_SIZE; i++) {
    CVI_TDL_Free(&data_buffer[i].image);
  }

  return NULL;
}

char* capobj_to_str(face_cpt_data_t *p_obj, float w, float h, int lb) {

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

void export_tracking_info(face_capture_t *p_cap_info, const char* str_dst_dir, int frame_id,
                          float imgw, float imgh, int lb) {
  cvtdl_face_t *p_objinfo = &(p_cap_info->last_faces);
  if (p_objinfo->size == 0) return;
  char sz_dstf[128];
  sprintf(sz_dstf, "%s/%08d.txt", str_dst_dir, frame_id);
  FILE *fp = fopen(sz_dstf, "w");
  printf("to parse,numobjs:%u,filename:%s\n", p_objinfo->size, sz_dstf);
  for (uint32_t i = 0; i < p_objinfo->size; i++) {
    // if(p_objinfo->info[i].unique_id != 0){
    // sprintf( buf, "\nOD DB File Size = %" PRIu64 " bytes \t"
    char szinfo[128];
    float ctx = (p_objinfo->info[i].bbox.x1 + p_objinfo->info[i].bbox.x2) / 2 / imgw;
    float cty = (p_objinfo->info[i].bbox.y1 + p_objinfo->info[i].bbox.y2) / 2 / imgh;
    float ww = (p_objinfo->info[i].bbox.x2 - p_objinfo->info[i].bbox.x1) / imgw;
    float hh = (p_objinfo->info[i].bbox.y2 - p_objinfo->info[i].bbox.y1) / imgh;
    // float score = p_objinfo->info[i].bbox.score;
    sprintf(szinfo, "%d %f %f %f %f %d %.3f\n", lb, ctx, cty, ww, hh,
            (int)p_objinfo->info[i].unique_id, p_objinfo->info[i].face_quality);

    fwrite(szinfo, 1, strlen(szinfo), fp);
  }

  cvtdl_object_t *p_objinfo2 = &(p_cap_info->last_objects);

  for (uint32_t i = 0; i < p_objinfo2->size; i++) {
    char szinfo[128];
    float ctx = (p_objinfo2->info[i].bbox.x1 + p_objinfo2->info[i].bbox.x2) / 2 / imgw;
    float cty = (p_objinfo2->info[i].bbox.y1 + p_objinfo2->info[i].bbox.y2) / 2 / imgh;
    float ww = (p_objinfo2->info[i].bbox.x2 - p_objinfo2->info[i].bbox.x1) / imgw;
    float hh = (p_objinfo2->info[i].bbox.y2 - p_objinfo2->info[i].bbox.y1) / imgh;
    sprintf(szinfo, "%d %f %f %f %f %d %.3f\n", lb, ctx, cty, ww, hh,
            (int)p_objinfo2->info[i].unique_id, p_objinfo2->info[i].bbox.score);

    fwrite(szinfo, 1, strlen(szinfo), fp);
  }
  printf("write done\n");
  fclose(fp);
}
void release_system(cvitdl_handle_t tdl_handle, cvitdl_service_handle_t service_handle,
                    cvitdl_app_handle_t app_handle) {
  CVI_TDL_APP_DestroyHandle(app_handle);
  if (service_handle != NULL) CVI_TDL_Service_DestroyHandle(service_handle);
  CVI_TDL_DestroyHandle(tdl_handle);
  // DestroyVideoSystem(&vs_ctx);
  CVI_SYS_Exit();
  CVI_VB_Exit();
}

char* get_img_name(const char* strf) {
  static char name[256];  
  const char* last_slash = strrchr(strf, '/');
  const char* last_dot = strrchr(strf, '.');
  
  if (last_slash == NULL || last_dot == NULL || last_dot < last_slash) {
    return NULL;
  }
  size_t len = last_dot - (last_slash + 1);
  if (len >= sizeof(name)) {
    return NULL;
  }
  memcpy(name, last_slash + 1, len);
  name[len] = '\0';
  return name;
}

int register_gallery_face(imgprocess_t img_handle, cvitdl_app_handle_t app_handle,
                         const char* strf, cvtdl_service_feature_array_t *p_feat_gallery,
                         char** gallery_names, int* gallery_size)  {
  VIDEO_FRAME_INFO_S fdFrame;

  int ret = CVI_TDL_ReadImage(img_handle, strf, &fdFrame, PIXEL_FORMAT_RGB_888_PLANAR);
  if (ret != CVI_SUCCESS) {
    printf("open img failed with %#x!\n", ret);
    return ret;
  }
  cvtdl_face_t faceinfo;
  memset(&faceinfo, 0, sizeof(faceinfo));
  ret = CVI_TDL_APP_FaceCapture_FDFR(app_handle, &fdFrame, &faceinfo);
  if (ret != CVI_SUCCESS) {
    printf("face extract failed\n");
  }
  printf("extract face num: %d\n", faceinfo.size);
  if (faceinfo.size == 0 || faceinfo.info[0].feature.ptr == NULL) {
    printf("face num error,got:%d\n", (int)faceinfo.size);
    ret = CVI_FAILURE;
  } else {
    printf("extract featsize: %u, addr: %p\n", 
           faceinfo.info[0].feature.size, 
           faceinfo.info[0].feature.ptr);
  }
  if (ret == CVI_FAILURE) {
    CVI_VPSS_ReleaseChnFrame(0, 0, &fdFrame);
    CVI_TDL_Free(&faceinfo);
    return ret;
  }

  int8_t *p_new_feat = NULL;
  size_t src_size = 0;
  if (p_feat_gallery->ptr == 0) {
    p_new_feat = (int8_t *)malloc(faceinfo.info[0].feature.size);
    p_feat_gallery->type = faceinfo.info[0].feature.type;
    p_feat_gallery->feature_length = faceinfo.info[0].feature.size;
    printf("allocate memory,size: %u\n", p_feat_gallery->feature_length);
  } else {
    if (p_feat_gallery->feature_length != faceinfo.info[0].feature.size) {
      printf("error,featsize not equal,curface:%u,gallery:%u\n", faceinfo.info[0].feature.size,
             p_feat_gallery->feature_length);
      ret = CVI_FAILURE;
    } else {
      src_size = p_feat_gallery->feature_length * p_feat_gallery->data_num;
      p_new_feat = (int8_t *)malloc(src_size + FACE_FEAT_SIZE);
      memcpy(p_new_feat, p_feat_gallery->ptr, src_size);
    }
  }
  printf("to copy mem\n");
  if (ret == CVI_SUCCESS) {
    if (p_feat_gallery->ptr != NULL) {
      memcpy(p_new_feat, p_feat_gallery->ptr + src_size, p_feat_gallery->feature_length);
      free(p_feat_gallery->ptr);
    }
    memcpy(p_new_feat + src_size, faceinfo.info[0].feature.ptr, faceinfo.info[0].feature.size);
    p_feat_gallery->data_num += 1;
    p_feat_gallery->ptr = p_new_feat;
    char* name = get_img_name(strf);
    if (name != NULL) {
      gallery_names[*gallery_size] = strdup(name);
      (*gallery_size)++;
      printf("register gallery: %s\n", name);
    }
  }
  printf("copy done\n");
  CVI_VPSS_ReleaseChnFrame(0, 0, &fdFrame);
  CVI_TDL_Free(&faceinfo);
  printf("register done\n");
  return ret;
}

int do_face_match(cvitdl_service_handle_t service_handle,
                  char **gallery_names, 
                  uint32_t gallery_names_count, 
                  cvtdl_face_info_t *p_face) {
    if (gallery_names_count == 0) {
        return CVI_FAILURE;
    }
  
    printf("to matchtrack: %u, featsize: %u\n", p_face->unique_id, p_face->feature.size);
    if (p_face->feature.size == 0) {
        return CVI_FAILURE;
    }
    uint32_t ind = 0;
    float score = 0;
    uint32_t size = 0;

    int ret = CVI_TDL_Service_FaceInfoMatching(service_handle, p_face, 1, 0.1, &ind, &score, &size);
    
    if (ind < gallery_names_count) { // 确保 ind 不越界
        printf("matchname, trackid: %u, name: %s, score: %f\n",
               p_face->unique_id, gallery_names[ind], score);
    } else {
        printf("matchname, trackid: %u, name: <invalid>, score: %f\n",
               p_face->unique_id, score);
    }
    
    p_face->recog_score = score;
   
    if (score > 0.5 && ind < gallery_names_count) {
        snprintf(p_face->name, sizeof(p_face->name), "%s", gallery_names[ind]);
    }

    return ret;
}

int main(int argc, char *argv[]) {
  CVI_S32 ret = CVI_SUCCESS;
  // Set signal catch
  signal(SIGINT, SampleHandleSig);
  signal(SIGTERM, SampleHandleSig);
  CVI_BOOL abChnEnable[VPSS_MAX_CHN_NUM] = {
      CVI_TRUE,
  };

  for (VPSS_GRP VpssGrp = 0; VpssGrp < VPSS_MAX_GRP_NUM; ++VpssGrp)
    SAMPLE_COMM_VPSS_Stop(VpssGrp, abChnEnable);

  CVI_TDL_SUPPORTED_MODEL_E model = CVI_TDL_SUPPORTED_MODEL_SCRFDFACE;
  char modelf[256] = "/mnt/data/wkz/faceCapture_pull_package/cviai/face_cvimodel/scrfd_500m_bnkps_432_768.cvimodel";
  char fl_modelf[256] = "/mnt/data/wkz/faceCapture_pull_package/cviai/face_cvimodel/pipnet_mbv1_at_50ep_v8_cv181x.cvimodel";
  char ped_modelf[256] = "/mnt/data/wkz/faceCapture_pull_package/cviai/face_cvimodel/mobiledetv2-pedestrian-d0-ls-448.cvimodel";

  char* fa_modelf = "NULL";

  char* str_model_file = modelf;
  CVI_TDL_SUPPORTED_MODEL_E fd_model_id = model;

  const char *fd_model_path = str_model_file;

  const char *config_path = "NULL";  // argv[4];//NULL

  int buffer_size = 15;
  float det_threshold = 0.5;
  bool write_image = true;
  char* str_image_root = argv[1];
  char* str_dst_root = argv[2];
  if (!create_directory(str_dst_root)) {
    printf("create directory: %s failed\n", str_dst_root);
  }
  char* str_dst_video = join_path(str_dst_root, get_directory_name(str_image_root));
  if (!create_directory(str_dst_video)) {
    printf("create directory: %s failed\n", str_dst_video);
    // return CVI_FAILURE;
  }
  g_out_dir = str_dst_video;
  if (buffer_size <= 0) {
    printf("buffer size must be larger than 0.\n");
    return CVI_FAILURE;
  }
  const CVI_S32 vpssgrp_width = 1920;
  const CVI_S32 vpssgrp_height = 1080;

  imgprocess_t img_handle;
  CVI_TDL_Create_ImageProcessor(&img_handle);

  ret = MMF_INIT_HELPER2(vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888, 3, vpssgrp_width,
                         vpssgrp_height, PIXEL_FORMAT_RGB_888_PLANAR, 3);
  cvitdl_handle_t tdl_handle = NULL;
  cvitdl_service_handle_t service_handle = NULL;
  cvitdl_app_handle_t app_handle = NULL;

  int vpss_grp = 1;
  ret = CVI_TDL_CreateHandle2(&tdl_handle, vpss_grp, 0);
  ret |= CVI_TDL_Service_CreateHandle(&service_handle, tdl_handle);
  // ret |= CVI_TDL_Service_EnableTPUDraw(service_handle, true);
  ret |= CVI_TDL_APP_CreateHandle(&app_handle, tdl_handle);
  printf("to facecap init\n");
  ret |= CVI_TDL_APP_FaceCapture_Init(app_handle, (uint32_t)buffer_size);
  if (ret != CVI_SUCCESS) {
    release_system(tdl_handle, service_handle, app_handle);
    return CVI_FAILURE;
  }
  printf("to quick setup\n");

  cvtdl_service_feature_array_t feat_gallery;
  memset(&feat_gallery, 0, sizeof(feat_gallery));
  CVI_TDL_SUPPORTED_MODEL_E fr_model_id = CVI_TDL_SUPPORTED_MODEL_FACERECOGNITION;
  ret |= CVI_TDL_APP_FaceCapture_QuickSetUp(app_handle, fd_model_id, fr_model_id, fd_model_path,
                                            NULL, NULL, fl_modelf, NULL);
  g_use_face_attribute = app_handle->face_cpt_info->fa_flag;
  CVI_TDL_SUPPORTED_MODEL_E ped_model_id = CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PEDESTRIAN;
  ret |= CVI_TDL_APP_FaceCapture_FusePedSetup(app_handle, ped_model_id, ped_modelf);

  if (ret != CVI_SUCCESS) {
    release_system(tdl_handle, service_handle, app_handle);
    return CVI_FAILURE;
  }

  CVI_TDL_SetModelThreshold(tdl_handle, fd_model_id, det_threshold);

  printf("finish init \n");

  char** gallery_names;
  int gallery_size=0;
  bool do_face_recog = false;

  if (do_face_recog) {
    const char *gimg = "/mnt/data/admin1_data/datasets/ivs_eval_set/image/yitong/register.jpg";
    ret = register_gallery_face(img_handle, app_handle, gimg, &feat_gallery, gallery_names, &gallery_size);
    printf("register ret:%#x\n",ret);
    if (ret == CVI_SUCCESS) {
      printf("to register gallery\n");
      ret = CVI_TDL_Service_RegisterFeatureArray(service_handle, feat_gallery, COS_SIMILARITY);
      printf("finish register gallery\n");
    }
  }
  printf("to start:\n");

  face_capture_config_t app_cfg;
  CVI_TDL_APP_FaceCapture_GetDefaultConfig(&app_cfg);
  if (!strcmp(config_path, "NULL")) {
    printf("Use Default Config...\n");
  }
  if (ret == CVI_FAILURE) {
    release_system(tdl_handle, service_handle, app_handle);
    return CVI_FAILURE;
  }
  app_cfg.thr_quality = 0.1;
  app_cfg.thr_size_min = 20;
  app_cfg.miss_time_limit = 20;
  app_cfg.store_feature = true;
  app_cfg.qa_method = 0;
  app_cfg.img_capture_flag = 0;  // capture whole frame
  app_cfg.m_interval = 1000;     // only export one when leaving
  CVI_TDL_APP_FaceCapture_SetConfig(app_handle, &app_cfg);
  CVI_TDL_APP_FaceCapture_SetMode(app_handle, AUTO);

  memset(&g_obj_meta_0, 0, sizeof(cvtdl_object_t));
  memset(&g_obj_meta_1, 0, sizeof(cvtdl_object_t));

  pthread_t io_thread;
  pthread_create(&io_thread, NULL, pImageWrite, NULL);
  const int face_label = 11;

  char cap_result[256];
  snprintf(cap_result, sizeof(cap_result), "%s/cap_result.log", str_dst_video);

  FILE *fp = fopen(cap_result, "w");

  int num_append = 0;
  PIXEL_FORMAT_E img_format = PIXEL_FORMAT_RGB_888_PLANAR;  // IVE_IMAGE_TYPE_U8C3_PACKAGE;
  for (int img_idx = 0; img_idx < 1100; img_idx++) {
    if (bExit) break;
    printf("processing: %d/1100\n",img_idx);
    char szimg[256];
    sprintf(szimg, "%s/%08d.jpg", str_image_root, img_idx);
    printf("processing: %d/1100,path: %s\n",img_idx, szimg);
    bool empty_img = false;
    IVE_IMAGE_S image;
    VIDEO_FRAME_INFO_S fdFrame;
    ret = CVI_TDL_ReadImage(img_handle, szimg, &fdFrame, img_format);
    printf("read image ret:%d width:%d\n", ret, (int)fdFrame.stVFrame.u32Width);
    if (ret != CVI_SUCCESS) {
      if (img_idx < 100) {
        CVI_TDL_ReleaseImage(img_handle, &fdFrame);
        release_system(tdl_handle, service_handle, app_handle);
        break;
      }
      printf("load image failed\n");
      empty_img = true;

      ret = CVI_TDL_ReadImage(
          img_handle, "/mnt/data/algo_pub/eval_data/dataset/face_cap_val_dataset/black_image.jpg",
          &fdFrame, img_format);

      num_append++;
      if (num_append > 30) {
        CVI_TDL_ReleaseImage(img_handle, &fdFrame);
        break;
      }
    }

    if (ret != CVI_SUCCESS) {
      printf("Convert to video frame failed with: %#x ,file: %s\n", ret, szimg);
      continue;
    }

    int alive_person_num = COUNT_ALIVE(app_handle->face_cpt_info);
    printf("ALIVE persons: %d\n", alive_person_num);
    ret = CVI_TDL_APP_FaceCapture_Run(app_handle, &fdFrame);
    if (ret != CVI_SUCCESS) {
      printf("CVI_TDL_APP_FaceCapture_Run failed with %#x\n", ret);
      break;
    }

    if (write_image) {
      printf("to export trackinginfo\n");
      if (!empty_img)
        export_tracking_info(app_handle->face_cpt_info, str_dst_video, img_idx,
                             fdFrame.stVFrame.u32Width, fdFrame.stVFrame.u32Height, face_label);
      printf("to capture trackinginfo\n");
      for (uint32_t i = 0; i < app_handle->face_cpt_info->size; i++) {
        if (!app_handle->face_cpt_info->_output[i]) continue;

        cvtdl_face_info_t *pface_info = &app_handle->face_cpt_info->data[i].info;
        do_face_match(service_handle, gallery_names, gallery_size, pface_info);
        tracker_state_e state = app_handle->face_cpt_info->data[i].state;
        uint32_t counter = app_handle->face_cpt_info->data[i]._out_counter;
        uint64_t u_id = app_handle->face_cpt_info->data[i].info.unique_id;
        float face_quality = app_handle->face_cpt_info->data[i].info.face_quality;

        char* str_res =
            capobj_to_str(&app_handle->face_cpt_info->data[i], fdFrame.stVFrame.u32Width,
                          fdFrame.stVFrame.u32Height, face_label);
        // ss<<str_res;
        fwrite(str_res, 1,  strlen(str_res), fp);
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
        printf("to export track:%d,frm:%d,capfrm:%d\n", (int)u_id, (int)i,
               (int)app_handle->face_cpt_info->data[i].cap_timestamp);
        /* Copy image data to buffer */
        memset(&data_buffer[target_idx], 0, sizeof(data_buffer[target_idx]));
        data_buffer[target_idx].u_id = u_id;
        data_buffer[target_idx].quality = face_quality;
        data_buffer[target_idx].state = state;
        data_buffer[target_idx].counter = counter;
        data_buffer[target_idx].match_score = pface_info->recog_score;
        data_buffer[target_idx].frame_id = app_handle->face_cpt_info->data[i].cap_timestamp;
        if (g_use_face_attribute) {
          data_buffer[target_idx].gender =
              app_handle->face_cpt_info->data[i].face_data.info->gender_score;
          data_buffer[target_idx].age = app_handle->face_cpt_info->data[i].face_data.info->age;
          data_buffer[target_idx].glass = app_handle->face_cpt_info->data[i].face_data.info->glass;
          data_buffer[target_idx].mask =
              app_handle->face_cpt_info->data[i].face_data.info->mask_score;
        }
        memcpy(data_buffer[target_idx].name, pface_info->name, 128);
        /* NOTE: Make sure the image type is IVE_IMAGE_TYPE_U8C3_PACKAGE */

        CVI_TDL_CopyImage(&app_handle->face_cpt_info->data[i].image,
                          &data_buffer[target_idx].image);
        {
          SMT_MutexAutoLock(IOMutex, lock);
          rear_idx = target_idx;
        }
      }
    }
    printf("to release frame\n");
    CVI_TDL_ReleaseImage(img_handle, &fdFrame);
  }
  fclose(fp);
  printf("to release system\n");
  bRunImageWriter = false;
  bRunVideoOutput = false;
  pthread_join(io_thread, NULL);
  CVI_TDL_Destroy_ImageProcessor(img_handle);
  CVI_TDL_APP_DestroyHandle(app_handle);
  CVI_TDL_Service_DestroyHandle(service_handle);
  CVI_TDL_DestroyHandle(tdl_handle);
  // DestroyVideoSystem(&vs_ctx);
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
