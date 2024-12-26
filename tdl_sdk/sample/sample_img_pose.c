#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "core/utils/vpss_helper.h"
#include "cvi_tdl.h"
#include "cvi_tdl_media.h"
#include "cvi_kit.h"
#include "sys_utils.h"

int process_img_simcc(cvitdl_handle_t tdl_handle, char* pd_model, char* pose_model,
                      VIDEO_FRAME_INFO_S* bg, cvtdl_object_t* p_obj) {
  int ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PEDESTRIAN, pd_model);
  if (ret != CVI_SUCCESS) {
    printf("open CVI_TDL_SUPPORTED_MODEL_SCRFDFACE model failed with %#x!\n", ret);
    return ret;
  }

  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_SIMCC_POSE, pose_model);
  if (ret != CVI_SUCCESS) {
    printf("open CVI_TDL_SUPPORTED_MODEL_SIMCC_POSE model failed with %#x!\n", ret);
    return ret;
  }

  //   CVI_TDL_SetMaxDetNum(tdl_handle, CVI_TDL_SUPPORTED_MODEL_SIMCC_POSE, 1);

  ret = CVI_TDL_Detection(tdl_handle, bg, CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PEDESTRIAN, p_obj);
  if (ret != CVI_SUCCESS) {
    printf("CVI_TDL_MOBILEDETV2_PEDESTRIAN failed with %#x!\n", ret);
    return ret;
  }

  if (p_obj->size > 0) {
    ret = CVI_TDL_PoseDetection(tdl_handle, bg, CVI_TDL_SUPPORTED_MODEL_SIMCC_POSE, p_obj);
    if (ret != CVI_SUCCESS) {
      printf("CVI_TDL_Simcc_Pose failed with %#x!\n", ret);
      return ret;
    }
  } else {
    printf("cannot find person\n");
  }

  return ret;
}

int process_img_yolov8pose(cvitdl_handle_t tdl_handle, char* pose_model, VIDEO_FRAME_INFO_S* bg,
                           cvtdl_object_t* p_obj) {
  int ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8POSE, pose_model);
  if (ret != CVI_SUCCESS) {
    printf("open CVI_TDL_SUPPORTED_MODEL_YOLOV8POSE model failed with %#x!\n", ret);
    return ret;
  }

  ret = CVI_TDL_PoseDetection(tdl_handle, bg, CVI_TDL_SUPPORTED_MODEL_YOLOV8POSE, p_obj);
  if (ret != CVI_SUCCESS) {
    printf("CVI_TDL_Yolov8_Pose failed with %#x!\n", ret);
    return ret;
  }

  return ret;
}

int main(int argc, char* argv[]) {
  char* pose_model = argv[1];    // pose detection model path
  char* process_flag = argv[2];  // simcc | yolov8pose
  char* imgf = argv[3];          // img path;
  int show = atoi(argv[4]);      // 1 for show keypoints, 0 for not show;

  char* pd_model;  // person detection model path,
  if (process_flag == "simcc") {
    pd_model = argv[5];
  }

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

  PIXEL_FORMAT_E img_format = PIXEL_FORMAT_RGB_888_PLANAR;
  if (show) img_format = PIXEL_FORMAT_BGR_888;

  VIDEO_FRAME_INFO_S bg;
  imgprocess_t img_handle;
  CVI_TDL_Create_ImageProcessor(&img_handle);
  ret = CVI_TDL_ReadImage(img_handle, imgf, &bg, img_format);
  if (ret != CVI_SUCCESS) {
    printf("failed to open file:%s\n", imgf);
    return ret;
  } else {
    printf("image read,width:%d\n", bg.stVFrame.u32Width);
  }

  cvtdl_object_t obj_meta = {0};

  if (process_flag == "simcc") {
    process_img_simcc(tdl_handle, pd_model, pose_model, &bg, &obj_meta);
  } else {
    process_img_yolov8pose(tdl_handle, pose_model, &bg, &obj_meta);
  }

  printf("obj_meta.size:%d\n", obj_meta.size);
  for (int i = 0; i < obj_meta.size; i++) {
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
    char* show_save_path = "/mnt/data/xiaochuan.liao/sdk_package/cviai/test_simcc.jpg";
    CVI_TDL_ShowKeypoints(&bg, &obj_meta, score, show_save_path);
    // cvitdl::service::DrawKeypoints(&bg, &obj_meta, save_path, score );
    // cvitdl::service::DrawPose17(&obj_meta, &bg);
  }

  CVI_TDL_ReleaseImage(img_handle, &bg);
  CVI_TDL_Free(&obj_meta);
  CVI_TDL_DestroyHandle(tdl_handle);
  CVI_TDL_Destroy_ImageProcessor(img_handle);
  return ret;
}
