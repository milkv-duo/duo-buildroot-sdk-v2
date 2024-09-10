#include "cvi_ive.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Incorrect loop value. Usage: %s <file_name>\n", argv[0]);
    return CVI_FAILURE;
  }
  const char *file_name = argv[1];
  // Create instance
  IVE_HANDLE handle = CVI_IVE_CreateHandle();
  printf("BM Kernel init.\n");

  // Fetch image information. CVI_IVE_ReadImage will do the flush for you.
  IVE_IMAGE_S src = CVI_IVE_ReadImage(handle, file_name, IVE_IMAGE_TYPE_U8C1);
  int width = src.u32Width;
  int height = src.u32Height;

  // Create output result for CVI_IVE_Sobel.
  IVE_DST_IMAGE_S dstH, dstV;
  CVI_IVE_CreateImage(handle, &dstV, IVE_IMAGE_TYPE_BF16C1, width, height);
  CVI_IVE_CreateImage(handle, &dstH, IVE_IMAGE_TYPE_BF16C1, width, height);
  // Create CVI_U8 output result for CVI_IVE_ImageTypeConvert.
  IVE_DST_IMAGE_S dstH_u8, dstV_u8;
  CVI_IVE_CreateImage(handle, &dstH_u8, IVE_IMAGE_TYPE_U8C1, width, height);
  CVI_IVE_CreateImage(handle, &dstV_u8, IVE_IMAGE_TYPE_U8C1, width, height);
  // Create output result for CVI_IVE_MagAndAng.
  IVE_DST_IMAGE_S dstMag, dstAng;
  CVI_IVE_CreateImage(handle, &dstMag, IVE_IMAGE_TYPE_BF16C1, width, height);
  CVI_IVE_CreateImage(handle, &dstAng, IVE_IMAGE_TYPE_BF16C1, width, height);
  // Create CVI_U8 output result for CVI_IVE_ImageTypeConvert.
  IVE_DST_IMAGE_S dstMag_u8, dstAng_u8;
  CVI_IVE_CreateImage(handle, &dstMag_u8, IVE_IMAGE_TYPE_U8C1, width, height);
  CVI_IVE_CreateImage(handle, &dstAng_u8, IVE_IMAGE_TYPE_U8C1, width, height);

  // Setup control parameters.
  IVE_SOBEL_CTRL_S iveSblCtrl;
  iveSblCtrl.enOutCtrl = IVE_SOBEL_OUT_CTRL_BOTH;
  iveSblCtrl.u8MaskSize = 3;
  IVE_MAG_AND_ANG_CTRL_S pstMaaCtrl;
  pstMaaCtrl.enOutCtrl = IVE_MAG_AND_ANG_OUT_CTRL_MAG_AND_ANG;
  pstMaaCtrl.enDistCtrl = IVE_MAG_DIST_L2;
  printf("Run TPU Sobel grad.\n");
  int ret = CVI_IVE_Sobel(handle, &src, &dstH, &dstV, &iveSblCtrl, 0);
  printf("Run TPU magnitude and angle.\n");
  ret |= CVI_IVE_MagAndAng(handle, &dstH, &dstV, &dstMag, &dstAng, &pstMaaCtrl, 0);

  // Visualize result to a human-readable image.
  printf("Normalize result to 0-255.\n");
  IVE_ITC_CRTL_S iveItcCtrl;
  iveItcCtrl.enType = IVE_ITC_NORMALIZE;
  ret |= CVI_IVE_ImageTypeConvert(handle, &dstV, &dstV_u8, &iveItcCtrl, 0);
  ret |= CVI_IVE_ImageTypeConvert(handle, &dstH, &dstH_u8, &iveItcCtrl, 0);
  ret |= CVI_IVE_ImageTypeConvert(handle, &dstMag, &dstMag_u8, &iveItcCtrl, 0);
  ret |= CVI_IVE_ImageTypeConvert(handle, &dstAng, &dstAng_u8, &iveItcCtrl, 0);

  // Write result to disk. No need to request cause CVI_IVE_WriteImage will do it for you.
  printf("Save to image.\n");
  ret |= CVI_IVE_WriteImage(handle, "sample_sobelV.png", &dstV_u8);
  ret |= CVI_IVE_WriteImage(handle, "sample_sobelH.png", &dstH_u8);
  ret |= CVI_IVE_WriteImage(handle, "sample_mag.png", &dstMag_u8);
  ret |= CVI_IVE_WriteImage(handle, "sample_ang.png", &dstAng_u8);

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src);
  CVI_SYS_FreeI(handle, &dstH);
  CVI_SYS_FreeI(handle, &dstH_u8);
  CVI_SYS_FreeI(handle, &dstV);
  CVI_SYS_FreeI(handle, &dstV_u8);
  CVI_SYS_FreeI(handle, &dstMag);
  CVI_SYS_FreeI(handle, &dstMag_u8);
  CVI_SYS_FreeI(handle, &dstAng);
  CVI_SYS_FreeI(handle, &dstAng_u8);
  CVI_IVE_DestroyHandle(handle);

  return ret;
}