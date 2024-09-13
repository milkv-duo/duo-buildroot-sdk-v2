#include "cvi_ive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Incorrect loop value. Usage: %s <file_name>\n", argv[0]);
    return CVI_FAILURE;
  }
  const char *file_name = argv[1];
  // Create instance
  IVE_HANDLE handle = CVI_IVE_CreateHandle();
  printf("BM Kernel init.\n");

  // Read image from file. CVI_IVE_ReadImage will do the flush for you.
  IVE_IMAGE_S src = CVI_IVE_ReadImage(handle, file_name, IVE_IMAGE_TYPE_U8C1);
  int width = src.u32Width;
  int height = src.u32Height;

  // Create threshold output result image.
  IVE_DST_IMAGE_S dst_thresh;
  CVI_IVE_CreateImage(handle, &dst_thresh, IVE_IMAGE_TYPE_U8C1, width, height);
  // Create dilate and erode output result image.
  IVE_DST_IMAGE_S dst_dilate, dst_erode;
  CVI_IVE_CreateImage(handle, &dst_dilate, IVE_IMAGE_TYPE_U8C1, width, height);
  CVI_IVE_CreateImage(handle, &dst_erode, IVE_IMAGE_TYPE_U8C1, width, height);

  // Setup threshold parameter.
  IVE_THRESH_CTRL_S iveThreshCtrl;
  iveThreshCtrl.enMode = IVE_THRESH_MODE_BINARY;
  iveThreshCtrl.u8LowThr = 170;
  iveThreshCtrl.u8MinVal = 0;
  iveThreshCtrl.u8MaxVal = 255;
  // Dilation and erosion needs binary image to process.
  printf("Run TPU Threshold.\n");
  CVI_IVE_Thresh(handle, &src, &dst_thresh, &iveThreshCtrl, 0);

  // Setup mask for dilation & erosion.
  CVI_U8 arr[] = {0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0};
  IVE_DILATE_CTRL_S iveDltCtrl;
  memcpy(iveDltCtrl.au8Mask, arr, 25 * sizeof(CVI_U8));
  printf("Run TPU Dilate.\n");
  int ret = CVI_IVE_Dilate(handle, &dst_thresh, &dst_dilate, &iveDltCtrl, 0);
  // Copy the mask from dilat control parameter.
  IVE_ERODE_CTRL_S iveErdCtrl = iveDltCtrl;
  printf("Run TPU Erode.\n");
  ret |= CVI_IVE_Erode(handle, &dst_thresh, &dst_erode, &iveErdCtrl, 0);

  // write result to disk
  printf("Save to image.\n");
  CVI_IVE_WriteImage(handle, "sample_thresh.png", &dst_thresh);
  CVI_IVE_WriteImage(handle, "sample_dilate.png", &dst_dilate);
  CVI_IVE_WriteImage(handle, "sample_erode.png", &dst_erode);

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src);
  CVI_SYS_FreeI(handle, &dst_thresh);
  CVI_SYS_FreeI(handle, &dst_dilate);
  CVI_SYS_FreeI(handle, &dst_erode);
  CVI_IVE_DestroyHandle(handle);

  return ret;
}