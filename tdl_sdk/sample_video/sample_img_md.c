#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <cvi_ive.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "cvi_tdl.h"

int main(int argc, char *argv[]) {
  cvitdl_handle_t tdl_handle = NULL;
  printf("start to run img md\n");
  CVI_S32 ret = CVI_TDL_CreateHandle(&tdl_handle);
  if (ret != CVI_SUCCESS) {
    printf("Create tdl handle failed with %#x!\n", ret);
    return ret;
  }
  IVE_HANDLE ive_handle = CVI_IVE_CreateHandle();

  VIDEO_FRAME_INFO_S bg, frame;
  // printf("toread image:%s\n",argv[1]);
  const char *strf1 = "/mnt/data/admin1_data/alios_test/set/a.jpg";
  IVE_IMAGE_S image1 = CVI_IVE_ReadImage(ive_handle, strf1, IVE_IMAGE_TYPE_U8C1);
  ret = CVI_SUCCESS;

  int imgw = image1.u32Width;
  if (imgw == 0) {
    printf("Read image failed with %x!\n", ret);
    return CVI_FAILURE;
  }
  ret = CVI_IVE_Image2VideoFrameInfo(&image1, &bg);
  if (ret != CVI_SUCCESS) {
    printf("Convert to video frame failed with %#x!\n", ret);
    return ret;
  }
  const char *strf2 = "/mnt/data/admin1_data/alios_test/set/b.jpg";
  IVE_IMAGE_S image2 = CVI_IVE_ReadImage(ive_handle, strf2, IVE_IMAGE_TYPE_U8C1);
  ret = CVI_SUCCESS;
  int imgw2 = image2.u32Width;
  if (imgw2 == 0) {
    printf("Read image failed with %x!\n", ret);
    return CVI_FAILURE;
  }

  ret = CVI_IVE_Image2VideoFrameInfo(&image2, &frame);
  if (ret != CVI_SUCCESS) {
    printf("Convert to video frame failed with %#x!\n", ret);
    return ret;
  }
  cvtdl_object_t obj_meta;
  CVI_TDL_Set_MotionDetection_Background(tdl_handle, &bg);

  CVI_TDL_MotionDetection(tdl_handle, &frame, &obj_meta, 20, 50);
  // CVI_TDL_MotionDetection(tdl_handle, &frame, &obj_meta, 20, 50);
  // CVI_TDL_MotionDetection(tdl_handle, &frame, &obj_meta, 20, 50);
  // VIDEO_FRAME_INFO_S motionmap;
  // ret = CVI_TDL_GetMotionMap(tdl_handle, &motionmap);

  CVI_TDL_DumpImage("img1.bin", &bg);
  CVI_TDL_DumpImage("img2.bin", &frame);
  // CVI_TDL_DumpImage("md.bin", &motionmap);

  for (int i = 0; i < obj_meta.size; i++) {
    printf("[%f,%f,%f,%f]\n", obj_meta.info[i].bbox.x1, obj_meta.info[i].bbox.y1,
           obj_meta.info[i].bbox.x2, obj_meta.info[i].bbox.y2);
  }

  CVI_TDL_Free(&obj_meta);
  CVI_SYS_FreeI(ive_handle, &image1);
  CVI_SYS_FreeI(ive_handle, &image2);

  CVI_TDL_DestroyHandle(tdl_handle);
  CVI_IVE_DestroyHandle(ive_handle);
  return ret;
}
