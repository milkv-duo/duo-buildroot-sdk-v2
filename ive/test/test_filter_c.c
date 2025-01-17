#include "ive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#ifdef __ARM_ARCH
#include "arm_neon.h"
#endif

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

  IVE_DST_IMAGE_S dst;
  CVI_IVE_CreateImage(handle, &dst, IVE_IMAGE_TYPE_U8C1, width, height);

  printf("Run TPU Filter.\n");
  CVI_S8 arr[] = {1,  4,  7, 4, 1,  4,  16, 26, 16, 4, 7, 26, 26,
                  41, 26, 7, 4, 16, 26, 16, 4,  1,  4, 7, 4,  1};
  IVE_FILTER_CTRL_S iveFltCtrl;
  iveFltCtrl.u8MaskSize = 5;
  memcpy(iveFltCtrl.as8Mask, arr, 25 * sizeof(CVI_S8));
  iveFltCtrl.u32Norm = 273;
  struct timeval t0, t1;
  gettimeofday(&t0, NULL);
  for (size_t i = 0; i < total_run; i++) {
    CVI_IVE_Filter(handle, &src, &dst, &iveFltCtrl, 0);
  }
  gettimeofday(&t1, NULL);
  unsigned long elapsed_tpu =
      ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec) / total_run;

  if (total_run == 1) {
    // write result to disk
    printf("Save to image.\n");
    CVI_IVE_WriteImage(handle, "test_filter_c.png", &dst);
  } else {
    printf("OOO %-10s %10lu %10s %10s\n", "FILTER", elapsed_tpu, "NA", "NA");
  }

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src);
  CVI_SYS_FreeI(handle, &dst);
  CVI_IVE_DestroyHandle(handle);

  return 0;
}
