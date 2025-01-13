#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "core/utils/vpss_helper.h"
#include "cvi_tdl.h"
#include "cvi_tdl_media.h"
#include "sample_utils.h"
#include "sys_utils.h"
#define MIN(a, b) ((a) < (b) ? (a) : (b))

int main(int argc, char* argv[]) {
  int vpssgrp_width = 1920;
  int vpssgrp_height = 1080;
  CVI_S32 ret = MMF_INIT_HELPER2(vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888, 1,
                                 vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888, 1);
  if (ret != CVI_TDL_SUCCESS) {
    printf("Init sys failed with %#x!\n", ret);
    return ret;
  }

  cvitdl_handle_t tdl_handle = NULL;
  ret = CVI_TDL_CreateHandle(&tdl_handle);
  if (ret != CVI_SUCCESS) {
    printf("Create tdl handle failed with %#x!\n", ret);
    return ret;
  }

  char* pd_model = argv[1];    // person detection ai_model
  char* pose_model = argv[2];  // simcc pose detection ai_model
  char* img = argv[3];         // img path;
  int show = atoi(argv[4]);    // 1 for show keypoints, 0 for not show;
  char* show_save_path = argv[5];

  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PEDESTRIAN, pd_model);
  if (ret != CVI_SUCCESS) {
    printf("open CVI_TDL_SUPPORTED_MODEL_SCRFDFACE model failed with %#x!\n", ret);
    return ret;
  }

  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_SIMCC_POSE, pose_model);
  if (ret != CVI_SUCCESS) {
    printf("open CVI_TDL_SUPPORTED_MODEL_SIMCC_POSE model failed with %#x!\n", ret);
    return ret;
  }

  cvtdl_object_t obj_meta = {0};

  VIDEO_FRAME_INFO_S bg;
  imgprocess_t img_handle;
  CVI_TDL_Create_ImageProcessor(&img_handle);
  ret = CVI_TDL_ReadImage(img_handle, img, &bg, PIXEL_FORMAT_BGR_888);
  if (ret != CVI_SUCCESS) {
    printf("failed to open file:%s\n", img);
    return ret;
  } else {
    printf("image read,width:%d\n", bg.stVFrame.u32Width);
  }

  ret =
      CVI_TDL_Detection(tdl_handle, &bg, CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PEDESTRIAN, &obj_meta);
  if (ret != CVI_SUCCESS) {
    printf("CVI_TDL_MOBILEDETV2_PEDESTRIAN failed with %#x!\n", ret);
    return ret;
  }

  if (obj_meta.size > 0) {
    ret = CVI_TDL_PoseDetection(tdl_handle, &bg, CVI_TDL_SUPPORTED_MODEL_SIMCC_POSE, &obj_meta);
    if (ret != CVI_SUCCESS) {
      printf("CVI_TDL_Simcc_Pose failed with %#x!\n", ret);
      return ret;
    }
  } else {
    printf("cannot find person\n");
  }

  int det_num = MIN(obj_meta.size, 5);  // max det_num set to 5
  for (int i = 0; i < det_num; i++) {
    printf("[%d,%d,%d,%d],\n", obj_meta.info[i].bbox.x1, obj_meta.info[i].bbox.y1,
           obj_meta.info[i].bbox.x2, obj_meta.info[i].bbox.y2);

    for (int j = 0; j < 17; j++) {
      printf("%d: %f %f %f\n", j, obj_meta.info[i].pedestrian_properity->pose_17.x[j],
             obj_meta.info[i].pedestrian_properity->pose_17.y[j],
             obj_meta.info[i].pedestrian_properity->pose_17.score[j]);
    }
  }

  if (show) {  // img format should be PIXEL_FORMAT_BGR_888
    float score;
    CVI_TDL_GetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PEDESTRIAN, &score);
    CVI_TDL_ShowKeypoints(&bg, &obj_meta, score, show_save_path);
  }

  CVI_TDL_ReleaseImage(img_handle, &bg);
  CVI_TDL_Free(&obj_meta);
  CVI_TDL_DestroyHandle(tdl_handle);
  CVI_TDL_Destroy_ImageProcessor(img_handle);
  return ret;
}
