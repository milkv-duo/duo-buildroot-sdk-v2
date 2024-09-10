#include "cvi_ive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

int main(int argc, char **argv) {
  if (argc != 4) {
    printf("Incorrect parameter. Usage: %s <image1> <image2> <alpha>\n", argv[0]);
    return CVI_FAILURE;
  }
  const char *file_name1 = argv[1];
  const char *file_name2 = argv[2];
  uint8_t alpha = (uint8_t)atoi(argv[3]);

  // Create instance
  printf("Create instance.\n");
  IVE_HANDLE handle = CVI_IVE_CreateHandle();

  // Read image from file. CVI_IVE_ReadImage will do the flush for you.
  IVE_IMAGE_S src1 = CVI_IVE_ReadImage(handle, file_name1, IVE_IMAGE_TYPE_U8C3_PLANAR);
  IVE_IMAGE_S src2 = CVI_IVE_ReadImage(handle, file_name2, IVE_IMAGE_TYPE_U8C3_PLANAR);

  int width = src1.u32Width;
  int height = src1.u32Height;
  IVE_DST_IMAGE_S dst;
  CVI_IVE_CreateImage(handle, &dst, IVE_IMAGE_TYPE_U8C3_PLANAR, width, height);

  printf("Run IVE alpha blending, alpha=%u.\n", alpha);
  // Run IVE blend.
  IVE_BLEND_CTRL_S blend_ctrl;
  blend_ctrl.u8Weight = alpha;
  struct timeval t0, t1;
  gettimeofday(&t0, NULL);
  int ret = CVI_IVE_Blend(handle, &src1, &src2, &dst, &blend_ctrl, false);
  gettimeofday(&t1, NULL);
  unsigned long elapsed_cpu = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;
  printf("Run IVE alpha blending done, elapsed: %lu us, ret = %s.\n", elapsed_cpu,
         ret == CVI_SUCCESS ? "Success" : "Fail");

  if (ret == CVI_SUCCESS) {
    // Refresh CPU cache before CPU use.
    CVI_IVE_BufRequest(handle, &dst);

    printf("Save result to file.\n");
    CVI_IVE_WriteImage(handle, "sample_alpha_blend.png", &dst);
  }

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src1);
  CVI_SYS_FreeI(handle, &src2);
  CVI_SYS_FreeI(handle, &dst);
  CVI_IVE_DestroyHandle(handle);

  return ret;
}