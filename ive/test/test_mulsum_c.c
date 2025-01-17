#include "bmkernel/bm_kernel.h"

#include "bmkernel/bm1880v2/1880v2_fp_convert.h"
#include "ive_experimental.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#define TEST_W 64
#define TEST_H 1

int cpu_ref(IVE_SRC_IMAGE_S *src, double sum);

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Incorrect loop value. Usage: %s <file_name> <loop in value (1-1000)>\n", argv[0]);
    return CVI_FAILURE;
  }
  size_t total_run = atoi(argv[1]);
  printf("Loop value: %zu\n", total_run);
  if (total_run > 1000 || total_run == 0) {
    printf("Incorrect loop value. Usage: %s <file_name> <loop in value (1-1000)>\n", argv[0]);
    return CVI_FAILURE;
  }
  srand(time(NULL));
  // Create instance
  IVE_HANDLE handle = CVI_IVE_CreateHandle();
  printf("BM Kernel init.\n");

  IVE_SRC_IMAGE_S src;
  CVI_IVE_CreateImage(handle, &src, IVE_IMAGE_TYPE_BF16C1, TEST_W, TEST_H);
  CVI_U32 srcLength = (CVI_U32)src.u16Stride[0] * src.u16Height;
  for (CVI_U32 i = 0; i < srcLength; i++) {
    float val = (rand() % 100 + 1) / 100.f + 0.5f;
    ((CVI_U16 *)src.pu8VirAddr[0])[i] = convert_fp32_bf16(val);
  }
  CVI_IVE_BufFlush(handle, &src);

  printf("Run TPU Mul Sum.\n");
  double sum = 1.f;
  struct timeval t0, t1;
  gettimeofday(&t0, NULL);
  for (size_t i = 0; i < total_run; i++) {
    CVI_IVE_MulSum(handle, &src, &sum, 0);
  }
  gettimeofday(&t1, NULL);
  unsigned long elapsed_tpu =
      ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec) / total_run;

  CVI_IVE_BufRequest(handle, &src);

  gettimeofday(&t0, NULL);
  CVI_U16 *data = (CVI_U16 *)src.pu8VirAddr[0];
  double cpu_sum = 1.f;
  for (CVI_U32 i = 0; i < src.u16Height; i++) {
    for (CVI_U32 j = 0; j < src.u16Width; j++) {
      cpu_sum *= convert_bf16_fp32(data[j + i * src.u16Stride[0]]);
    }
  }
  gettimeofday(&t1, NULL);
  unsigned long elapsed_cpu = ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec);
  printf("cpu_sum %f\n", cpu_sum);

  int ret = cpu_ref(&src, sum);

  if (total_run == 1) {
    printf("TPU avg time %lu\n", elapsed_tpu);
    printf("CPU avg time %lu\n", elapsed_cpu);
  }
#ifdef __ARM_ARCH
  else {
    printf("OOO %-10s %10lu %10s %10lu\n", "MUL SUM", elapsed_tpu, "NA", elapsed_cpu);
  }
#endif

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src);
  CVI_IVE_DestroyHandle(handle);

  return ret;
}

int cpu_ref(IVE_SRC_IMAGE_S *src, double sum) {
  printf("Check mul sum result: ");
  double cpu_sum = 1.f;
  int ret = CVI_SUCCESS;
  CVI_U16 *data = (CVI_U16 *)src->pu8VirAddr[0];
  for (CVI_U32 i = 0; i < src->u16Height; i++) {
    for (CVI_U32 j = 0; j < src->u16Width; j++) {
      cpu_sum *= convert_bf16_fp32(data[j + i * src->u16Stride[0]]);
    }
  }
  printf("\n");
  if (cpu_sum != sum) {
    ret = CVI_FAILURE;
  }
  printf("CPU sum: %f, TPU sum: %f\n", cpu_sum, sum);
  printf("%s\n", ret == CVI_SUCCESS ? "passed" : "failed");
  return ret;
}
