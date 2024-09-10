#include "bmkernel/bm_kernel.h"

#include "bmkernel/bm1880v2/1880v2_fp_convert.h"
#include "cvi_ive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#ifdef __ARM_ARCH
#include "arm_neon.h"
#endif

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Incorrect loop value. Usage: %s <loop in value (1-1000)>\n", argv[0]);
    return CVI_FAILURE;
  }
  size_t total_run = atoi(argv[1]);
  printf("Loop value: %zu\n", total_run);
  if (total_run > 1000 || total_run == 0) {
    printf("Incorrect loop value. Usage: %s <loop in value (1-1000)>\n", argv[0]);
    return CVI_FAILURE;
  }

  srand(time(NULL));

  // Create instance
  IVE_HANDLE handle = CVI_IVE_CreateHandle();
  printf("BM Kernel init.\n");
  int size = 1933155;
  int div = (size % 16) == 0 ? size / 16 : (size / 16) + 1;
  int width = 16;
  int height = div;
  // Generate information
  IVE_SRC_IMAGE_S src;
  CVI_IVE_CreateImage(handle, &src, IVE_IMAGE_TYPE_S8C1, width, height);
  CVI_S8 *src_ptr = (CVI_S8 *)src.pu8VirAddr[0];
  for (int j = 0; j < width * height / 2; j++) {
    src_ptr[j] = (CVI_S8)(rand() % 255 - 128);
  }
  CVI_IVE_BufFlush(handle, &src);

  IVE_DST_IMAGE_S dst;
  CVI_IVE_CreateImage(handle, &dst, IVE_IMAGE_TYPE_BF16C1, width, height);

  printf("Run TPU Sigmoid.\n");
  struct timeval t0, t1;
  gettimeofday(&t0, NULL);
  for (size_t i = 0; i < total_run; i++) {
    CVI_IVE_Sigmoid(handle, &src, &dst, 0);
  }
  gettimeofday(&t1, NULL);
  unsigned long elapsed_tpu =
      ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec) / total_run;
  CVI_IVE_BufRequest(handle, &src);
  CVI_IVE_BufRequest(handle, &dst);
  // int ret = cpu_ref(nChannels, &src1, &src2, &dst);

  if (total_run == 1) {
    printf("TPU avg time %lu\n", elapsed_tpu);
#ifdef __ARM_ARCH
    printf("CPU NEON time %s\n", "NA");
    printf("CPU time %s\n", "NA");
#endif
  }
#ifdef __ARM_ARCH
  else {
    printf("OOO %-10s %10lu %10s %10s\n", "SIG", elapsed_tpu, "NA", "NA");
  }
#endif

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src);
  CVI_SYS_FreeI(handle, &dst);
  CVI_IVE_DestroyHandle(handle);
  return CVI_SUCCESS;
}