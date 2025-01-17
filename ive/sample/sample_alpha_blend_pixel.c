#include "ive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

int main(int argc, char **argv) {
  if (argc != 3) {
    printf("Incorrect parameter. Usage: %s <image1> <image2>\n", argv[0]);
    return CVI_FAILURE;
  }
  const char *file_name1 = argv[1];
  const char *file_name2 = argv[2];

  // Create instance
  printf("Create instance.\n");
  IVE_HANDLE handle = CVI_IVE_CreateHandle();

  // Read image from file. CVI_IVE_ReadImage will do the flush for you.
  IVE_IMAGE_S src1 = CVI_IVE_ReadImage(handle, file_name1, IVE_IMAGE_TYPE_U8C3_PLANAR);
  IVE_IMAGE_S src2 = CVI_IVE_ReadImage(handle, file_name2, IVE_IMAGE_TYPE_U8C3_PLANAR);

  int width = src1.u16Width;
  int height = src1.u16Height;
  int strideWidth = src1.u16Stride[0];

  // create horizontal gradient alpha image
  IVE_DST_IMAGE_S alpha;
  CVI_IVE_CreateImage(handle, &alpha, IVE_IMAGE_TYPE_U8C3_PLANAR, width, height);
  int gradient_start = 0.3 * width;
  int gradient_end = 0.7 * width;
  int gradient_length = gradient_end - gradient_start;
  for (int c = 0; c < 3; c++) {
    for (int j = 0; j < height; j++) {
      for (int i = 0; i < width; i++) {
        if (i < gradient_start) {
          alpha.pu8VirAddr[c][i + j * strideWidth] = 0;
        } else if (i > gradient_end) {
          alpha.pu8VirAddr[c][i + j * strideWidth] = 255;
        } else {
          uint8_t alpha_pixel = (uint8_t)(((float)(i - gradient_start) / gradient_length) * 255);
          alpha.pu8VirAddr[c][i + j * strideWidth] = alpha_pixel;
        }
      }
    }
  }

  // Flush to DRAM before IVE function.
  CVI_IVE_BufFlush(handle, &alpha);

  IVE_DST_IMAGE_S dst;
  CVI_IVE_CreateImage(handle, &dst, IVE_IMAGE_TYPE_U8C3_PLANAR, width, height);

  printf("Run IVE alpha blending.\n");
  // Run IVE blend.

  struct timeval t0, t1;
  gettimeofday(&t0, NULL);
  int ret = CVI_IVE_Blend_Pixel(handle, &src1, &src2, &alpha, &dst, false);
  gettimeofday(&t1, NULL);
  unsigned long elapsed_cpu = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;
  printf("Run IVE alpha blending done, elapsed: %lu us, ret = %s.\n", elapsed_cpu,
         ret == CVI_SUCCESS ? "Success" : "Fail");

  if (ret == CVI_SUCCESS) {
    // Refresh CPU cache before CPU use.
    CVI_IVE_BufRequest(handle, &dst);

    printf("Save result to file.\n");
    CVI_IVE_WriteImage(handle, "sample_alpha_blend_pixel.png", &dst);
  }

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src1);
  CVI_SYS_FreeI(handle, &src2);
  CVI_SYS_FreeI(handle, &dst);
  CVI_IVE_DestroyHandle(handle);

  return ret;
}
