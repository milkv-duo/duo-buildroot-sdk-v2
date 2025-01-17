#include "ive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#ifdef __ARM_ARCH
#include "arm_neon.h"
#endif

int main(int argc, char** argv) {
  if (argc != 3) {
    printf("Incorrect loop value. Usage: %s <file name> <loop in value (1-1000)>\n", argv[0]);
    return CVI_FAILURE;
  }
  const char* filename = argv[1];
  size_t total_run = atoi(argv[2]);
  printf("Loop value: %zu\n", total_run);
  if (total_run > 1000 || total_run == 0) {
    printf("Incorrect loop value. Usage: %s <file name> <loop in value (1-1000)>\n", argv[0]);
    return CVI_FAILURE;
  }
  // Create instance
  IVE_HANDLE handle = CVI_IVE_CreateHandle();
  printf("BM Kernel init.\n");

  // Fetch image information
  IVE_IMAGE_S src = CVI_IVE_ReadImage(handle, filename, IVE_IMAGE_TYPE_U8C3_PLANAR);
  int nChannels = 3;
  int width = src.u16Width;
  int height = src.u16Height;
  printf("Image size source is %d X %d, channel %d\n", width, height, nChannels);

  IVE_DST_IMAGE_S dst;
  int width_dst = src.u16Width;
  int height_dst = src.u16Height;
  CVI_IVE_CreateImage(handle, &dst, IVE_IMAGE_TYPE_U8C1, width_dst, height_dst);
  // printf("Image size destination is %d X %d, channel %d\n", dst.u16Width, dst.u16Height,
  // nChannels);

  printf("Run CPU CSC.\n");

  IVE_CSC_CTRL_S ctrl;  // Currently a dummy variable
  ctrl.enMode = IVE_CSC_MODE_PIC_RGB2GRAY;

  struct timeval t0, t1;
  gettimeofday(&t0, NULL);
  for (size_t i = 0; i < total_run; i++) {
    CVI_IVE_CSC(handle, &src, &dst, &ctrl, 0);
  }
  gettimeofday(&t1, NULL);
  unsigned long elapsed_cpu =
      ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec) / total_run;

  CVI_IVE_BufRequest(handle, &dst);

  if (total_run == 1) {
    printf("TPU avg time %s\n", "NA");

    printf("CPU NEON time %s\n", "NA");
    printf("CPU time %lu\n", elapsed_cpu);
    // write result to disk
    printf("Save to image.\n");
    CVI_IVE_WriteImage(handle, "test_csc_c.png", &dst);
  }

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src);
  CVI_SYS_FreeI(handle, &dst);
  CVI_IVE_DestroyHandle(handle);

  return 0;
}
