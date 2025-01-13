#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "core/utils/vpss_helper.h"
#include "cvi_tdl.h"
#include "cvi_tdl_media.h"

int main(int argc, char *argv[]) {
  if (argc != 5) {
    printf(
        "Usage: %s <model path> <left image directory> <right image directory> <output "
        "directory>\n",
        argv[0]);
    return -1;
  }

  int vpssgrp_width = 1920;
  int vpssgrp_height = 1080;
  CVI_S32 ret = MMF_INIT_HELPER2(vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888_PLANAR, 4,
                                 vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888_PLANAR, 4);
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

  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_STEREO, argv[1]);
  if (ret != CVI_SUCCESS) {
    printf("Open model failed with %#x!\n", ret);
    return ret;
  }

  imgprocess_t img_handle;
  CVI_TDL_Create_ImageProcessor(&img_handle);

  VIDEO_FRAME_INFO_S left_img;
  VIDEO_FRAME_INFO_S right_img;

  ret = CVI_TDL_ReadImage(img_handle, argv[2], &left_img, PIXEL_FORMAT_RGB_888_PLANAR);
  ret = CVI_TDL_ReadImage(img_handle, argv[3], &right_img, PIXEL_FORMAT_RGB_888_PLANAR);

  cvtdl_depth_logits_t depth_logist;
  depth_logist.int_logits = NULL;
  int pix_size = 0;
  for (int i = 0; i < 10000; i++) {
    CVI_TDL_Depth_Stereo(tdl_handle, &left_img, &right_img, &depth_logist);
    pix_size = depth_logist.w * depth_logist.h;

    VIDEO_FRAME_INFO_S stFrame_nv21;
    CREATE_ION_HELPER(&stFrame_nv21, depth_logist.w, depth_logist.h, PIXEL_FORMAT_NV21,
                      "cvitdl/image");
    memcpy(stFrame_nv21.stVFrame.pu8VirAddr[0], depth_logist.int_logits, pix_size);
    memset(stFrame_nv21.stVFrame.pu8VirAddr[1], 128, pix_size / 2);
    // send_rtsp
    /**********************/
    // after send, free stFrame_nv21
    CVI_SYS_IonFree(stFrame_nv21.stVFrame.u64PhyAddr[0], stFrame_nv21.stVFrame.pu8VirAddr[0]);
  }
  free(depth_logist.int_logits);

  FILE *outFile = fopen(argv[4], "wb");
  if (!outFile) {
    perror("Cannot open output file");
    return -1;
  }
  fwrite(depth_logist.int_logits, sizeof(int8_t), pix_size, outFile);
  fclose(outFile);

  CVI_TDL_ReleaseImage(img_handle, &left_img);
  CVI_TDL_ReleaseImage(img_handle, &right_img);
  CVI_TDL_DestroyHandle(tdl_handle);
  CVI_TDL_Destroy_ImageProcessor(img_handle);

  return ret;
}
