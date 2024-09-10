#include "cvi_ive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Incorrect loop value. Usage: %s <file name>\n", argv[0]);
    return CVI_FAILURE;
  }
  const char *filename = argv[1];

  // Create instance
  IVE_HANDLE handle = CVI_IVE_CreateHandle();
  printf("BM Kernel init.\n");

  // Fetch image information. CVI_IVE_ReadImage will do the flush for you.
  IVE_IMAGE_S src = CVI_IVE_ReadImage(handle, filename, IVE_IMAGE_TYPE_U8C1);
  int width = src.u32Width;
  int height = src.u32Height;

  // CVI_IVE_OrdStatFilter finds the min/ max of an image using a 3x3 mask.
  // Thus the output will slightly decrease by 2.
  // The function only accepts the correct image size.
  IVE_DST_IMAGE_S dst, dst2;
  int dst_width = width - 2;
  int dst_height = height - 2;
  CVI_IVE_CreateImage(handle, &dst, IVE_IMAGE_TYPE_U8C1, dst_width, dst_height);
  CVI_IVE_CreateImage(handle, &dst2, IVE_IMAGE_TYPE_U8C1, dst_width, dst_height);

  printf("Run TPU Max.\n");
  IVE_ORD_STAT_FILTER_CTRL_S pstOrdStatFltCtrl;
  pstOrdStatFltCtrl.enMode = IVE_ORD_STAT_FILTER_MODE_MAX;
  int ret = CVI_IVE_OrdStatFilter(handle, &src, &dst, &pstOrdStatFltCtrl, 0);

  printf("Run TPU Min.\n");
  pstOrdStatFltCtrl.enMode = IVE_ORD_STAT_FILTER_MODE_MIN;
  ret |= CVI_IVE_OrdStatFilter(handle, &src, &dst2, &pstOrdStatFltCtrl, 0);

  // write result to disk
  printf("Save to image.\n");
  CVI_IVE_WriteImage(handle, "sample_ordstatmax.png", &dst);
  CVI_IVE_WriteImage(handle, "sample_ordstatmin.png", &dst2);

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src);
  CVI_SYS_FreeI(handle, &dst);
  CVI_SYS_FreeI(handle, &dst2);
  CVI_IVE_DestroyHandle(handle);

  return ret;
}