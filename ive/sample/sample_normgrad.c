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

  // Output image buffer for horizontal, vertical, L2 norm Sobel result.
  IVE_DST_IMAGE_S dstH_u8, dstV_u8, dstHV_u8;
  CVI_IVE_CreateImage(handle, &dstH_u8, IVE_IMAGE_TYPE_U8C1, width, height);
  CVI_IVE_CreateImage(handle, &dstV_u8, IVE_IMAGE_TYPE_U8C1, width, height);
  CVI_IVE_CreateImage(handle, &dstHV_u8, IVE_IMAGE_TYPE_U8C1, width, height);

  // Output normalized horizontal, vertical gradient Sobel result.
  printf("Run norm gradient.\n");
  IVE_NORM_GRAD_CTRL_S pstNormGradCtrl;
  pstNormGradCtrl.enOutCtrl = IVE_NORM_GRAD_OUT_CTRL_HOR_AND_VER;
  pstNormGradCtrl.enDistCtrl = IVE_MAG_DIST_L2;
  pstNormGradCtrl.enITCType = IVE_ITC_NORMALIZE;
  pstNormGradCtrl.u8MaskSize = 3;
  int ret = CVI_IVE_NormGrad(handle, &src, &dstH_u8, &dstV_u8, NULL, &pstNormGradCtrl, 0);

  // Output normalized L2 norm Sobel result.
  printf("Run norm gradient combine.\n");
  pstNormGradCtrl.enOutCtrl = IVE_NORM_GRAD_OUT_CTRL_COMBINE;
  ret |= CVI_IVE_NormGrad(handle, &src, NULL, NULL, &dstHV_u8, &pstNormGradCtrl, 0);

  // Write result to disk.
  printf("Save to image.\n");
  CVI_IVE_WriteImage(handle, "sample_normV.png", &dstV_u8);
  CVI_IVE_WriteImage(handle, "sample_normH.png", &dstH_u8);
  CVI_IVE_WriteImage(handle, "sample_normHV.png", &dstHV_u8);

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src);
  CVI_SYS_FreeI(handle, &dstH_u8);
  CVI_SYS_FreeI(handle, &dstV_u8);
  CVI_SYS_FreeI(handle, &dstHV_u8);
  CVI_IVE_DestroyHandle(handle);

  return ret;
}