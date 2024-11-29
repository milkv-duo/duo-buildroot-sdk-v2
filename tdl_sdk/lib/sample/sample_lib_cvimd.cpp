 #ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "cvi_md.h"
#include <cvi_ive.h>

int main(int argc, char *argv[]) {
  cvi_md_handle_t handle = NULL;
  printf("start to run img md\n");
  CVI_S32 ret = CVI_MD_Create_Handle(&handle);
  if (ret != CVI_SUCCESS) {
    printf("Create MD handle failed with %#x!\n", ret);
    return ret;
  }
  IVE_HANDLE ive_handle = CVI_IVE_CreateHandle();
  const char *strf1 = argv[1];
  const char *strf2 = argv[2];

  VIDEO_FRAME_INFO_S bg, frame;
  IVE_IMAGE_S image1 = CVI_IVE_ReadImage(ive_handle, strf1, IVE_IMAGE_TYPE_U8C1);
  ret = CVI_SUCCESS;

  int imgw = image1.u32Width;
  if (imgw == 0) {
    printf("Read image failed with %x!\n", ret);
    return CVI_FAILURE;
  } else {
    printf("Read image ok with %d!\n", imgw);
  }
  ret = CVI_IVE_Image2VideoFrameInfo(&image1, &bg);

  if (ret != CVI_SUCCESS) {
    printf("Convert to video frame failed with %#x!\n", ret);
    return ret;
  }
  IVE_IMAGE_S image2 = CVI_IVE_ReadImage(ive_handle, strf2, IVE_IMAGE_TYPE_U8C1);
  ret = CVI_SUCCESS;
  int imgw2 = image2.u32Width;
  if (imgw2 == 0) {
    printf("Read image failed with %x!\n", ret);
    return CVI_FAILURE;
  } else {
    printf("Read image1 ok with %d!\n", imgw2);
  }

  ret = CVI_IVE_Image2VideoFrameInfo(&image2, &frame);
  if (ret != CVI_SUCCESS) {
    printf("Convert to video frame failed with %#x!\n", ret);
    return ret;
  }
  printf("to set background\n");

  ret = CVI_MD_Set_Background(handle, &bg);
  printf("set background ret:%x\n", ret);

  MDROI roi_s;
  roi_s.num = 2;
  roi_s.pnt[0].x1 = 0;
  roi_s.pnt[0].y1 = 0;
  roi_s.pnt[0].x2 = 512;
  roi_s.pnt[0].y2 = 512;
  roi_s.pnt[1].x1 = 1000;
  roi_s.pnt[1].y1 = 150;
  roi_s.pnt[1].x2 = 1150;
  roi_s.pnt[1].y2 = 250;
  ret = CVI_MD_Set_ROI(handle, &roi_s);

  int *p_boxes;
  uint32_t num_boxes;
  printf("to do md detect \n");

  ret = CVI_MD_Detect(handle, &frame, &p_boxes, &num_boxes, 20, 50);
  printf("CVI_MD_Detect ret:%x\n", ret);

  for (uint32_t i = 0; i < num_boxes; i++) {
    printf("[%d,%d,%d,%d]\n", p_boxes[4 * i], p_boxes[4 * i + 1], p_boxes[4 * i + 2],
           p_boxes[4 * i + 3]);
  }
  free(p_boxes);
  CVI_SYS_FreeI(ive_handle, &image1);
  CVI_SYS_FreeI(ive_handle, &image2);

  CVI_MD_Destroy_Handle(handle);
  CVI_IVE_DestroyHandle(ive_handle);
  return ret;
}
