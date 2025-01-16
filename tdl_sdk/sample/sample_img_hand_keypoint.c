#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "core/utils/vpss_helper.h"
#include "cvi_tdl.h"
#include "cvi_tdl_media.h"

double __get_us(struct timeval t) { return (t.tv_sec * 1000000 + t.tv_usec); }

void run_hand_detection(cvitdl_handle_t tdl_handle, const char *model_path,
                        VIDEO_FRAME_INFO_S *p_frame, cvtdl_object_t *obj_meta) {
  printf("---------------------to do detection-----------------------\n");
  CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_HAND_DETECTION, model_path);
  CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_HAND_DETECTION, 0.5);

  struct timeval start_time, stop_time;
  gettimeofday(&start_time, NULL);
  CVI_TDL_Detection(tdl_handle, p_frame, CVI_TDL_SUPPORTED_MODEL_HAND_DETECTION, obj_meta);
  gettimeofday(&stop_time, NULL);
  printf("CVI_TDL_Hand_Detection Time use %f ms\n",
         (__get_us(stop_time) - __get_us(start_time)) / 1000);
  for (uint32_t i = 0; i < obj_meta->size; i++) {
    printf("[%f,%f,%f,%f,%d,%f]\n", obj_meta->info[i].bbox.x1, obj_meta->info[i].bbox.y1,
           obj_meta->info[i].bbox.x2, obj_meta->info[i].bbox.y2, obj_meta->info[i].classes,
           obj_meta->info[i].bbox.score);
  }
}

void run_hand_keypoint(cvitdl_handle_t tdl_handle, const char *model_path,
                       VIDEO_FRAME_INFO_S *p_frame, cvtdl_handpose21_meta_ts *hand_obj,
                       cvtdl_object_t *obj_meta) {
  printf("---------------------to do detection keypoint-----------------------\n");
  CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_HAND_KEYPOINT, model_path);
  hand_obj->size = obj_meta->size;
  hand_obj->info =
      (cvtdl_handpose21_meta_t *)malloc(sizeof(cvtdl_handpose21_meta_t) * hand_obj->size);
  hand_obj->height = p_frame->stVFrame.u32Height;
  hand_obj->width = p_frame->stVFrame.u32Width;
  for (uint32_t i = 0; i < hand_obj->size; i++) {
    hand_obj->info[i].bbox_x = obj_meta->info[i].bbox.x1;
    hand_obj->info[i].bbox_y = obj_meta->info[i].bbox.y1;
    hand_obj->info[i].bbox_w = obj_meta->info[i].bbox.x2 - obj_meta->info[i].bbox.x1;
    hand_obj->info[i].bbox_h = obj_meta->info[i].bbox.y2 - obj_meta->info[i].bbox.y1;
  }
  struct timeval start_time, stop_time;
  gettimeofday(&start_time, NULL);
  CVI_TDL_HandKeypoint(tdl_handle, p_frame, hand_obj);
  gettimeofday(&stop_time, NULL);
  printf("CVI_TDL_HandKeypoint Time use %f ms\n",
         (__get_us(stop_time) - __get_us(start_time)) / 1000);

  // generate detection result
  for (uint32_t i = 0; i < 21; i++) {
    printf("%f t  %f\n", hand_obj->info[0].x[i], hand_obj->info[0].y[i]);
  }
}

void run_hand_keypoint_cls(cvitdl_handle_t tdl_handle, const char *model_path,
                           cvtdl_handpose21_meta_ts *hand_obj, cvtdl_handpose21_meta_t *meta) {
  printf("---------------------to do detection keypoint classification-----------------------\n");
  CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_HAND_KEYPOINT_CLASSIFICATION, model_path);
  float keypoints[42];
  int index = 0;
  for (uint32_t i = 0; i < hand_obj->size; i++) {
    for (uint32_t j = 0; j < 21; j++) {
      keypoints[index++] = hand_obj->info[i].xn[j];
      keypoints[index++] = hand_obj->info[i].yn[j];
    }
  }

  if (index != 42) {
    printf("error size %d\n", index);
    hand_obj->info[0].label = -1;
    hand_obj->info[0].score = -1.;
  } else {
    CVI_U8 buffer[sizeof(keypoints)];
    memcpy(buffer, keypoints, sizeof(keypoints));
    VIDEO_FRAME_INFO_S Frame;
    Frame.stVFrame.pu8VirAddr[0] = buffer;
    Frame.stVFrame.u32Height = 1;
    Frame.stVFrame.u32Width = sizeof(keypoints) / sizeof(float);

    struct timeval start_time, stop_time;
    gettimeofday(&start_time, NULL);
    CVI_TDL_HandKeypointClassification(tdl_handle, &Frame, meta);
    gettimeofday(&stop_time, NULL);
    printf("CVI_TDL_HandKeypointClassification Time use %f ms\n",
           (__get_us(stop_time) - __get_us(start_time)) / 1000);
  }
  printf("meta.label = %d, meta.score = %f\n", meta->label, meta->score);
}

int main(int argc, char *argv[]) {
  char *hd_model_path = argv[1];
  char *kp_model_path = argv[2];
  char *kc_model_path = argv[3];
  char *image_path = argv[4];

  // Init VB pool size.
  const CVI_S32 vpssgrp_width = 1920;
  const CVI_S32 vpssgrp_height = 1080;

  CVI_S32 ret = MMF_INIT_HELPER2(vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888, 3,
                                 vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888_PLANAR, 3);
  if (ret != CVI_SUCCESS) {
    printf("Init sys failed with %#x!\n", ret);
    return ret;
  }

  cvitdl_handle_t tdl_handle = NULL;
  ret = CVI_TDL_CreateHandle(&tdl_handle);
  if (ret != CVI_SUCCESS) {
    printf("Create tdl handle failed with %#x!\n", ret);
    return ret;
  }

  VIDEO_FRAME_INFO_S fdFrame;
  imgprocess_t img_handle;
  CVI_TDL_Create_ImageProcessor(&img_handle);
  ret = CVI_TDL_ReadImage(img_handle, image_path, &fdFrame, PIXEL_FORMAT_RGB_888_PLANAR);
  if (ret != CVI_SUCCESS) {
    printf("open img failed with %#x!\n", ret);
    return ret;
  }

  cvtdl_object_t obj_meta = {0};
  cvtdl_handpose21_meta_ts hand_obj = {0};
  cvtdl_handpose21_meta_t meta = {0};
  run_hand_detection(tdl_handle, hd_model_path, &fdFrame, &obj_meta);
  run_hand_keypoint(tdl_handle, kp_model_path, &fdFrame, &hand_obj, &obj_meta);
  run_hand_keypoint_cls(tdl_handle, kc_model_path, &hand_obj, &meta);

  CVI_TDL_Free(&obj_meta);
  // CVI_TDL_Free(&hand_obj);
  CVI_TDL_ReleaseImage(img_handle, &fdFrame);
  CVI_TDL_DestroyHandle(tdl_handle);
  CVI_TDL_Destroy_ImageProcessor(img_handle);
  return ret;
}
