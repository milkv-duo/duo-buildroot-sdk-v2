#include "cvi_ive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#ifdef __ARM_ARCH
#include "arm_neon.h"
#endif
int cpu_ref(const int channels, IVE_SRC_IMAGE_S *src1, int thresh, IVE_DST_IMAGE_S *dst);
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
  int nChannels = 1;
  int width = src.u32Width;
  int height = src.u32Height;
  printf("Image size is %d X %d, channel %d\n", width, height, nChannels);

  IVE_DST_IMAGE_S dst;
  CVI_IVE_CreateImage(handle, &dst, IVE_IMAGE_TYPE_U8C1, width, height);

  printf("Run TPU Threshold.\n");
  IVE_THRESH_CTRL_S iveThreshCtrl;  // Currently a dummy variable
  iveThreshCtrl.enMode = IVE_THRESH_MODE_BINARY;
  iveThreshCtrl.u8LowThr = 170;
  iveThreshCtrl.u8MinVal = 0;
  iveThreshCtrl.u8MaxVal = 255;
  struct timeval t0, t1;
  gettimeofday(&t0, NULL);
  for (size_t i = 0; i < total_run; i++) {
    CVI_IVE_Thresh(handle, &src, &dst, &iveThreshCtrl, 0);
  }
  gettimeofday(&t1, NULL);
  unsigned long elapsed_tpu =
      ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec) / total_run;
  int ret = cpu_ref(1, &src, iveThreshCtrl.u8LowThr, &dst);
  if (ret == CVI_SUCCESS) {
    printf("check equality ok\n");
  }
#ifdef __ARM_ARCH
  IVE_DST_IMAGE_S dst_cpu;
  CVI_IVE_CreateImage(handle, &dst_cpu, IVE_IMAGE_TYPE_U8C1, width, height);
  gettimeofday(&t0, NULL);
  CVI_U8 *ptr1 = src.pu8VirAddr[0];
  CVI_U8 *ptr2 = dst_cpu.pu8VirAddr[0];
  for (size_t i = 0; i < nChannels * src.u32Width * src.u32Height; i++) {
    ptr2[i] = ptr1[i] >= iveThreshCtrl.u8LowThr ? iveThreshCtrl.u8MaxVal : iveThreshCtrl.u8MinVal;
  }
  gettimeofday(&t1, NULL);
  unsigned long elapsed_cpu = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;
  CVI_SYS_FreeI(handle, &dst_cpu);
#endif
  if (total_run == 1) {
    printf("TPU avg time %lu\n", elapsed_tpu);
#ifdef __ARM_ARCH
    printf("CPU NEON time %s\n", "NA");
    printf("CPU time %lu\n", elapsed_cpu);
#endif
    // write result to disk
    printf("Save to image.\n");
    CVI_IVE_WriteImage(handle, "test_threshold_c.png", &dst);
  }
#ifdef __ARM_ARCH
  else {
    printf("OOO %-10s %10lu %10s %10lu\n", "THRESH", elapsed_tpu, "NA", elapsed_cpu);
  }
#endif

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src);
  CVI_SYS_FreeI(handle, &dst);
  CVI_IVE_DestroyHandle(handle);

  return 0;
}
int cpu_ref(const int channels, IVE_SRC_IMAGE_S *src1, int thresh, IVE_DST_IMAGE_S *dst) {
  int ret = CVI_SUCCESS;
  CVI_U8 *src1_ptr = src1->pu8VirAddr[0];
  // CVI_U8 *src2_ptr = src2->pu8VirAddr[0];
  CVI_U8 *dst_ptr = dst->pu8VirAddr[0];
  for (size_t i = 0; i < channels * src1->u32Width * src1->u32Height; i++) {
    int res = src1_ptr[i];
    if (res < thresh)
      res = 0;
    else
      res = 255;
    if (res != dst_ptr[i]) {
      printf("[%zu] %d  = TPU %d, CPU %d\n", i, src1_ptr[i], dst_ptr[i], (int)res);
      ret = CVI_FAILURE;
      break;
    }
  }
  return ret;
}
