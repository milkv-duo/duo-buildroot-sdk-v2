#include "ive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

int main(int argc, char **argv) {
  if (argc != 3) {
    printf("Incorrect loop value. Usage: %s <file name> <loop in value (1-1000)>\n", argv[0]);
    return CVI_FAILURE;
  }
  const char *filename = argv[1];
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
  IVE_IMAGE_S src = CVI_IVE_ReadImage(handle, filename, IVE_IMAGE_TYPE_U8C1);
  int width = src.u16Width;
  int height = src.u16Height;

  IVE_DST_IMAGE_S dst, dst2;
  int dst_width = width - 2;
  int dst_height = height - 2;
  CVI_IVE_CreateImage(handle, &dst, IVE_IMAGE_TYPE_U8C1, dst_width, dst_height);
  CVI_IVE_CreateImage(handle, &dst2, IVE_IMAGE_TYPE_U8C1, dst_width, dst_height);

  printf("Run TPU Max.\n");
  IVE_ORD_STAT_FILTER_CTRL_S pstOrdStatFltCtrl;
  pstOrdStatFltCtrl.enMode = IVE_ORD_STAT_FILTER_MODE_MAX;
  struct timeval t0, t1;
  gettimeofday(&t0, NULL);
  for (size_t i = 0; i < total_run; i++) {
    CVI_IVE_OrdStatFilter(handle, &src, &dst, &pstOrdStatFltCtrl, 0);
  }
  gettimeofday(&t1, NULL);
  unsigned long elapsed_tpu_max =
      ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec) / total_run;

  printf("Run TPU Min.\n");
  pstOrdStatFltCtrl.enMode = IVE_ORD_STAT_FILTER_MODE_MIN;
  gettimeofday(&t0, NULL);
  for (size_t i = 0; i < total_run; i++) {
    CVI_IVE_OrdStatFilter(handle, &src, &dst2, &pstOrdStatFltCtrl, 0);
  }
  gettimeofday(&t1, NULL);
  unsigned long elapsed_tpu_min =
      ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec) / total_run;

  if (total_run == 1) {
    // write result to disk
    printf("Save to image.\n");
    CVI_IVE_WriteImage(handle, "test_max_c.png", &dst);
    CVI_IVE_WriteImage(handle, "test_min_c.png", &dst2);
  } else {
    printf("OOO %-10s %10lu %10s %10s\n", "TPU MAX", elapsed_tpu_max, "NA", "NA");
    printf("OOO %-10s %10lu %10s %10s\n", "TPU MIN", elapsed_tpu_min, "NA", "NA");
  }

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src);
  CVI_SYS_FreeI(handle, &dst);
  CVI_SYS_FreeI(handle, &dst2);
  CVI_IVE_DestroyHandle(handle);

  return 0;
}