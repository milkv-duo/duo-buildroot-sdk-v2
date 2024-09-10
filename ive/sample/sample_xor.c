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
  printf("Create instance.\n");
  IVE_HANDLE handle = CVI_IVE_CreateHandle();

  // Read image from file. CVI_IVE_ReadImage will do the flush for you.
  IVE_IMAGE_S src1 = CVI_IVE_ReadImage(handle, file_name, IVE_IMAGE_TYPE_U8C1);
  int nChannels = 1;  // IVE_IMAGE_TYPE_U8C1 = 1 channel
  int width = src1.u32Width;
  int strideWidth = src1.u16Stride[0];
  int height = src1.u32Height;

  // Create second image to add with src1.
  IVE_SRC_IMAGE_S src2;
  CVI_IVE_CreateImage(handle, &src2, IVE_IMAGE_TYPE_U8C1, width, height);
  CVI_U32 imgSize = nChannels * strideWidth * height;
  memset(src2.pu8VirAddr[0], 255, imgSize);
  for (int j = height / 10; j < height * 9 / 10; j++) {
    for (int i = width / 10; i < width * 9 / 10; i++) {
      src2.pu8VirAddr[0][i + j * strideWidth] = 0;
    }
  }
  // Flush to DRAM before IVE function.
  CVI_IVE_BufFlush(handle, &src2);

  // Create dst image.
  IVE_DST_IMAGE_S dst;
  CVI_IVE_CreateImage(handle, &dst, IVE_IMAGE_TYPE_U8C1, width, height);

  printf("Run IVE XOr.\n");
  // Run IVE XOr.
  int ret = CVI_IVE_Xor(handle, &src1, &src2, &dst, 0);

  // Refresh CPU cache before CPU use.
  CVI_IVE_BufRequest(handle, &dst);

  printf("Save result to file.\n");
  CVI_IVE_WriteImage(handle, "sample_xor.png", &dst);

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src1);
  CVI_SYS_FreeI(handle, &src2);
  CVI_SYS_FreeI(handle, &dst);
  CVI_IVE_DestroyHandle(handle);

  return ret;
}