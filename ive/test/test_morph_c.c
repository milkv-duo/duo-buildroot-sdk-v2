#include "bmkernel/bm_kernel.h"

#include "bmkernel/bm1880v2/1880v2_fp_convert.h"
#include "cvi_ive.h"

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
  IVE_IMAGE_S src = CVI_IVE_ReadImage(handle, file_name, IVE_IMAGE_TYPE_U8C1);
  int width = src.u32Width;
  int height = src.u32Height;

  IVE_DST_IMAGE_S dst;
  CVI_IVE_CreateImage(handle, &dst, IVE_IMAGE_TYPE_U8C1, width, height);

  IVE_DST_IMAGE_S dst2, dst3;
  CVI_IVE_CreateImage(handle, &dst2, IVE_IMAGE_TYPE_U8C1, width, height);
  CVI_IVE_CreateImage(handle, &dst3, IVE_IMAGE_TYPE_U8C1, width, height);

  printf("Run TPU Threshold.\n");
  IVE_THRESH_CTRL_S iveThreshCtrl;  // Currently a dummy variable
  iveThreshCtrl.enMode = IVE_THRESH_MODE_BINARY;
  iveThreshCtrl.u8LowThr = 170;
  iveThreshCtrl.u8MinVal = 0;
  iveThreshCtrl.u8MaxVal = 255;
  CVI_IVE_Thresh(handle, &src, &dst, &iveThreshCtrl, 0);

  printf("Run TPU Dilate.\n");
  CVI_U8 arr[] = {0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0};
  IVE_DILATE_CTRL_S iveDltCtrl;
  memcpy(iveDltCtrl.au8Mask, arr, 25 * sizeof(CVI_U8));
  struct timeval t0, t1;
  gettimeofday(&t0, NULL);
  for (size_t i = 0; i < total_run; i++) {
    CVI_IVE_Dilate(handle, &dst, &dst2, &iveDltCtrl, 0);
  }
  gettimeofday(&t1, NULL);
  unsigned long elapsed_tpu_dilate =
      ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec) / total_run;

  printf("Run TPU Erode.\n");
  IVE_ERODE_CTRL_S iveErdCtrl = iveDltCtrl;
  gettimeofday(&t0, NULL);
  for (size_t i = 0; i < total_run; i++) {
    CVI_IVE_Erode(handle, &dst, &dst3, &iveErdCtrl, 0);
  }
  gettimeofday(&t1, NULL);
  unsigned long elapsed_tpu_erode =
      ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec) / total_run;

  if (total_run == 1) {
    printf("TPU dilate avg time %lu\n", elapsed_tpu_dilate);
    printf("TPU erode avg time %lu\n", elapsed_tpu_erode);
#ifdef __ARM_ARCH
    printf("CPU NEON time %s\n", "NA");
    printf("CPU time %s\n", "NA");
#endif
    // write result to disk
    printf("Save to image.\n");
    CVI_IVE_WriteImage(handle, "test_morph_thresh_c.png", &dst);
    CVI_IVE_WriteImage(handle, "test_dilate_c.png", &dst2);
    CVI_IVE_WriteImage(handle, "test_erode_c.png", &dst3);
  }
#ifdef __ARM_ARCH
  else {
    printf("OOO %-10s %10lu %10s %10s\n", "DILATE", elapsed_tpu_dilate, "NA", "NA");
    printf("OOO %-10s %10lu %10s %10s\n", "ERODE", elapsed_tpu_erode, "NA", "NA");
  }
#endif
  // Free memory, instance
  CVI_SYS_FreeI(handle, &src);
  CVI_SYS_FreeI(handle, &dst);
  CVI_SYS_FreeI(handle, &dst2);
  CVI_IVE_DestroyHandle(handle);

  return 0;
}
