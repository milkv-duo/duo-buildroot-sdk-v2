#include "cvi_ive.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#ifdef __ARM_ARCH
#include "arm_neon.h"
#endif

int main(int argc, char **argv) {
  if (argc != 3) {
    printf("Incorrect loop value. Usage: %s <file_name> <loop in value (1-1000)>\n", argv[0]);
    return CVI_FAILURE;
  }
  const char *file_name = argv[1];
  size_t total_run = atoi(argv[2]);
  printf("Loop value: %zu\n", total_run);
  if (total_run > 1000 || total_run == 0) {
    printf("Incorrect loop value. Usage: %s <file_name> <loop in value (1-1000)>\n", argv[0]);
    return CVI_FAILURE;
  }
  // Create instance
  IVE_HANDLE handle = CVI_IVE_CreateHandle();
  printf("BM Kernel init.\n");

  // Fetch image information
  IVE_IMAGE_S src1 = CVI_IVE_ReadImage(handle, file_name, IVE_IMAGE_TYPE_U8C1);
  int nChannels = 1;
  int width = src1.u32Width;
  int stride = src1.u16Stride[0];
  int height = src1.u32Height;

  IVE_SRC_IMAGE_S src2;
  CVI_IVE_CreateImage(handle, &src2, IVE_IMAGE_TYPE_U8C1, width, height);
  memset(src2.pu8VirAddr[0], 255, nChannels * stride * height);
  for (int j = height / 10; j < height * 9 / 10; j++) {
    for (int i = width / 10; i < width * 9 / 10; i++) {
      src2.pu8VirAddr[0][i + j * stride] = 0;
    }
  }
  CVI_IVE_BufFlush(handle, &src2);

  IVE_DST_IMAGE_S dst;
  CVI_IVE_CreateImage(handle, &dst, IVE_IMAGE_TYPE_U8C1, width, height);

  printf("Run TPU Or.\n");
  struct timeval t0, t1;
  gettimeofday(&t0, NULL);
  for (size_t i = 0; i < total_run; i++) {
    CVI_IVE_Or(handle, &src1, &src2, &dst, 0);
  }
  gettimeofday(&t1, NULL);
  unsigned long elapsed_tpu =
      ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec) / total_run;

  if (total_run == 1) {
    printf("TPU avg time %lu\n", elapsed_tpu);
#ifdef __ARM_ARCH
    printf("CPU NEON time %s\n", "NA");
    printf("CPU time %s\n", "NA");
#endif
    // write result to disk
    printf("Save to image.\n");
    CVI_IVE_WriteImage(handle, "test_or_c.png", &dst);
  }
#ifdef __ARM_ARCH
  else {
    printf("OOO %-10s %10lu %10s %10s\n", "OR", elapsed_tpu, "NA", "NA");
  }
#endif

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src1);
  CVI_SYS_FreeI(handle, &src2);
  CVI_SYS_FreeI(handle, &dst);
  CVI_IVE_DestroyHandle(handle);

  return 0;
}