#include "cvi_ive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Incorrect arguments. Usage: %s <file name>\n", argv[0]);
    return CVI_FAILURE;
  }

  const char *filename = argv[1];
  // Create instance
  IVE_HANDLE handle = CVI_IVE_CreateHandle();
  printf("init.\n");

  // Fetch image information. CVI_IVE_ReadImage will do the flush for you.
  printf("read src image\n");
  IVE_IMAGE_S src = CVI_IVE_ReadImage(handle, filename, IVE_IMAGE_TYPE_U8C3_PLANAR);
  int width = src.u32Width;
  int height = src.u32Height;

  struct timeval t0, t1;

  IVE_DST_IMAGE_S dst;
  printf("create dst image\n");
  CVI_IVE_CreateImage(handle, &dst, IVE_IMAGE_TYPE_U8C3_PLANAR, width, height);

  // Setup 5x5 gaussian filter
  CVI_S8 arr[] = {1,  4,  7, 4, 1,  4,  16, 26, 16, 4, 7, 26, 26,
                  41, 26, 7, 4, 16, 26, 16, 4,  1,  4, 7, 4,  1};
  IVE_FILTER_CTRL_S iveFltCtrl;
  iveFltCtrl.u8MaskSize = 5;  // Set the length of the mask, can be 3 or 5.
  memcpy(iveFltCtrl.as8Mask, arr, iveFltCtrl.u8MaskSize * iveFltCtrl.u8MaskSize * sizeof(CVI_S8));
  iveFltCtrl.u32Norm = 273;
  // Since the mask only accepts S8 values, you can turn a float mask into (1/X) * (S8 mask).
  // Then set u32Norm to X.

  printf("Run 5x5 gaussian filter.\n");
  gettimeofday(&t0, NULL);
  int ret = CVI_IVE_Filter(handle, &src, &dst, &iveFltCtrl, 0);
  if (ret != CVI_SUCCESS) {
    printf("Failed to run CVI_IVE_Filter\n");
    goto failed;
  }
  gettimeofday(&t1, NULL);

  unsigned long elapsed = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;
  printf("Done, elapsed time=%lu us\n", elapsed);

  // write result to disk
  printf("Save result.\n\n");
  CVI_IVE_WriteImage(handle, "sample_filter_5x5_gaussian.png", &dst);

  // Setup 13x13 box filter
  iveFltCtrl.u8MaskSize = 13;
  memset(iveFltCtrl.as8Mask, 1, sizeof(iveFltCtrl.as8Mask));
  iveFltCtrl.u32Norm = 13 * 13;

  printf("Run 13x13 box filter.\n");
  gettimeofday(&t0, NULL);
  ret = CVI_IVE_Filter(handle, &src, &dst, &iveFltCtrl, 0);
  if (ret != CVI_SUCCESS) {
    printf("Failed to run CVI_IVE_Filter\n");
    goto failed;
  }
  gettimeofday(&t1, NULL);

  elapsed = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;
  printf("Done, elapsed time=%lu us\n", elapsed);

  // write result to disk
  printf("Save result.\n");
  CVI_IVE_WriteImage(handle, "sample_filter_13x13_box.png", &dst);

failed:
  // Free memory, instance
  CVI_SYS_FreeI(handle, &src);
  CVI_SYS_FreeI(handle, &dst);
  CVI_IVE_DestroyHandle(handle);

  return ret;
}