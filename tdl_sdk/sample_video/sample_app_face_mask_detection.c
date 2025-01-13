/*
input:
    fd_model_path
    f_mask_cls_model_path
    image_path
output:
    mask face info
*/

#include "core/utils/vpss_helper.h"
#include "cvi_tdl.h"
#include "cvi_tdl_app.h"
#include "cvi_tdl_media.h"
#include "sample_comm.h"
#include "vi_vo_utils.h"

#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "stb_image.h"
#include "stb_image_write.h"

int main(int argc, char *argv[]) {
  if (argc != 4) {
    printf("need fr_model_path, fd_model_path, image_path");
    return CVI_FAILURE;
  }
  int vpssgrp_width = 640;
  int vpssgrp_height = 640;
  CVI_S32 ret = MMF_INIT_HELPER2(vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888, 1,
                                 vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888, 1);
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
  cvtdl_face_t p_obj = {0};

  const char *fd_model_path = argv[1];
  const char *fm_model_path = argv[2];
  const char *img_path = argv[3];  // /mnt/data/admin1_data/alios_test/a.bin
  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_SCRFDFACE, fd_model_path);
  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_MASKCLASSIFICATION, fm_model_path);
  VIDEO_FRAME_INFO_S bg;
  ret = CVI_TDL_LoadBinImage(img_path, &bg, PIXEL_FORMAT_RGB_888_PLANAR);
  if (ret != CVI_SUCCESS) {
    printf("failed to open file\n");
    return ret;
  } else {
    printf("image read,width:%d\n", bg.stVFrame.u32Width);
  }

  // get bbox
  ret = CVI_TDL_FaceDetection(tdl_handle, &bg, CVI_TDL_SUPPORTED_MODEL_SCRFDFACE, &p_obj);
  if (ret != CVI_SUCCESS) {
    printf("failed to run face detection\n");
    return ret;
  }
  ret = CVI_TDL_MaskClassification(tdl_handle, &bg, &p_obj);
  CVI_VPSS_ReleaseChnFrame(0, 0, &bg);  //(&bg);
  printf("boxes=[");
  for (uint32_t i = 0; i < p_obj.size; i++) {
    printf("[%f,%f,%f,%f,%f]\n", p_obj.info[i].bbox.x1, p_obj.info[i].bbox.y1,
           p_obj.info[i].bbox.x2, p_obj.info[i].bbox.y2, p_obj.info[i].mask_score);
  }
  printf("]");
  CVI_TDL_DestroyHandle(tdl_handle);
  return true;
}
