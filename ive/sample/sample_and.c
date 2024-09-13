#include "cvi_ive.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

int main(int argc, char **argv) {
  if (argc != 4) {
    printf("Incorrect loop value. Usage: %s <w> <h> <file_name>\n", argv[0]);
    printf("Example: %s 352 288 data/00_352x288_y.yuv\n", argv[0]);
    return CVI_FAILURE;
  }
  // 00_352x288_y.yuv
  int input_w, input_h;

  input_w = atoi(argv[1]);
  input_h = atoi(argv[2]);

  const char *file_name = argv[3];
  // Create instance
  printf("Create instance.\n");
  IVE_HANDLE handle = CVI_IVE_CreateHandle();

  // Read image from file. CVI_IVE_ReadImage will do the flush for you.
  IVE_IMAGE_S src1;

  src1.u32Width = input_w;
  src1.u32Height = input_h;
  CVI_IVE_ReadRawImage(handle, &src1, (char *)file_name, IVE_IMAGE_TYPE_U8C1, input_w, input_h);

  int nChannels = 1;  // IVE_IMAGE_TYPE_U8C1 = 1 channel
  int width = src1.u32Width;
  int strideWidth = src1.u16Stride[0];
  int height = src1.u32Height;

  // Create second image to add with src1.
  IVE_SRC_IMAGE_S src2;

  CVI_IVE_CreateImage(handle, &src2, IVE_IMAGE_TYPE_U8C1, width, height);
  CVI_U32 imgSize = nChannels * strideWidth * height;

  memset(src2.pu8VirAddr[0], 0, imgSize);
  for (int j = height / 10; j < height * 9 / 10; j++) {
    for (int i = width / 10; i < width * 9 / 10; i++) {
      src2.pu8VirAddr[0][i + j * strideWidth] = 255;
    }
  }
  // Flush to DRAM before IVE function.
  CVI_IVE_BufFlush(handle, &src2);

  // Create dst image.
  IVE_DST_IMAGE_S dst;

  CVI_IVE_CreateImage(handle, &dst, IVE_IMAGE_TYPE_U8C1, width, height);

  printf("Run IVE And.\n");
  // Run IVE and.
  struct timeval t0, t1;

  gettimeofday(&t0, NULL);
  int ret = CVI_IVE_And(handle, &src1, &src2, &dst, 0);

  gettimeofday(&t1, NULL);

  unsigned long elapsed = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;

  if (ret != CVI_SUCCESS) {
    printf("Failed when run CVI_IVE_And ret: %x\n", ret);
    CVI_SYS_FreeI(handle, &src2);
    CVI_SYS_FreeI(handle, &dst);
    CVI_IVE_DestroyHandle(handle);
    return ret;
  }

  printf("elapsed time: %lu us\n", elapsed);

  // Refresh CPU cache before CPU use.
  CVI_IVE_BufRequest(handle, &dst);

  printf("Save result to file.\n");
  CVI_IVE_WriteImage(handle, "sample_and.png", &dst);

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src1);
  CVI_SYS_FreeI(handle, &src2);
  CVI_SYS_FreeI(handle, &dst);
  CVI_IVE_DestroyHandle(handle);

  return ret;
}