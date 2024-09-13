#include "cvi_ive.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
  if (argc != 3) {
    printf("Incorrect loop value. Usage: %s <file_name_1> <file_name_2>\n", argv[0]);
    return CVI_FAILURE;
  }
  const char* filename1 = argv[1];
  const char* filename2 = argv[2];

  // Create instance
  IVE_HANDLE handle = CVI_IVE_CreateHandle();
  printf("BM Kernel init.\n");

  // Read image from file. CVI_IVE_ReadImage will do the flush for you.
  IVE_IMAGE_S srcL = CVI_IVE_ReadImage(handle, filename1, IVE_IMAGE_TYPE_U8C1);
  IVE_IMAGE_S srcR = CVI_IVE_ReadImage(handle, filename2, IVE_IMAGE_TYPE_U8C1);
  int width = srcL.u32Width;
  int height = srcL.u32Height;

  // SAD can output a U16C1 image, but thresholded image only supports U8C1.
  IVE_DST_IMAGE_S dst, dst_u8, dst_thresh;
  CVI_IVE_CreateImage(handle, &dst, IVE_IMAGE_TYPE_U16C1, width, height);
  CVI_IVE_CreateImage(handle, &dst_u8, IVE_IMAGE_TYPE_U8C1, width, height);
  CVI_IVE_CreateImage(handle, &dst_thresh, IVE_IMAGE_TYPE_U8C1, width, height);

  // Setup control parameters.
  IVE_SAD_CTRL_S iveSadCtrl;
  iveSadCtrl.enMode = IVE_SAD_MODE_MB_4X4;
  iveSadCtrl.enOutCtrl = IVE_SAD_OUT_CTRL_16BIT_BOTH;  // Output thresh and non-thresh image.
  iveSadCtrl.u16Thr = 1000;  // Treshold value for thresholding an U16 image into an U8 image.
  iveSadCtrl.u8MaxVal = 255;
  iveSadCtrl.u8MinVal = 0;

  printf("Run TPU SAD.\n");
  int ret = CVI_IVE_SAD(handle, &srcL, &srcR, &dst, &dst_thresh, &iveSadCtrl, 0);

  // Normalizing U16 image to U8.
  IVE_ITC_CRTL_S iveItcCtrl;
  iveItcCtrl.enType = IVE_ITC_NORMALIZE;
  CVI_IVE_ImageTypeConvert(handle, &dst, &dst_u8, &iveItcCtrl, 0);

  // Write result to disk. CVI_IVE_WriteImage will do the request for you.
  printf("Save to image.\n");
  CVI_IVE_WriteImage(handle, "sample_sad.png", &dst_u8);
  CVI_IVE_WriteImage(handle, "sample_sadthresh.png", &dst_thresh);

  // Free memory, instance.
  CVI_SYS_FreeI(handle, &srcL);
  CVI_SYS_FreeI(handle, &srcR);
  CVI_SYS_FreeI(handle, &dst);
  CVI_SYS_FreeI(handle, &dst_u8);
  CVI_SYS_FreeI(handle, &dst_thresh);
  CVI_IVE_DestroyHandle(handle);
  return ret;
}