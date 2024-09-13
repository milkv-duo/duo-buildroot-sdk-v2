#include "cvi_ive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Usage: %s <image file>\n", argv[0]);
    return CVI_FAILURE;
  }

  const char *file_name = argv[1];

  // Create instance
  IVE_HANDLE handle = CVI_IVE_CreateHandle();
  printf("BM Kernel init.\n");

  IVE_IMAGE_S src = CVI_IVE_ReadImage(handle, file_name, IVE_IMAGE_TYPE_U8C1);

  printf("Run TPU Block.\n");
  struct timeval t0, t1;
  gettimeofday(&t0, NULL);
  float average;
  int ret = CVI_IVE_Average(handle, &src, &average, 0);
  gettimeofday(&t1, NULL);
  unsigned long elapsed_tpu = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;

  printf("average of image=%f, elapsed_cpu=%lu us\n", average, elapsed_tpu);

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src);
  CVI_IVE_DestroyHandle(handle);

  return ret;
}