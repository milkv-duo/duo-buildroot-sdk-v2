#include "cvi_ive.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#define TEST_W 16
#define TEST_H 10

int cpu_ref(IVE_SRC_IMAGE_S *src, IVE_MEM_INFO_S *table, IVE_DST_IMAGE_S *dst);

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
  CVI_IVE_CreateImage(handle, &src, IVE_IMAGE_TYPE_U8C1, TEST_W, TEST_H);
  CVI_U32 srcLength = (CVI_U32)(TEST_W * TEST_H);
  for (CVI_U32 i = 0; i < srcLength; i++) {
    src.pu8VirAddr[0][i] = rand() % 256;
  }
  CVI_IVE_BufFlush(handle, &src);

  IVE_DST_IMAGE_S dst;
  CVI_IVE_CreateImage(handle, &dst, IVE_IMAGE_TYPE_U8C1, TEST_W, TEST_H);

  IVE_DST_MEM_INFO_S dstTbl;
  CVI_U32 dstTblByteSize = 256;
  CVI_IVE_CreateMemInfo(handle, &dstTbl, dstTblByteSize);
  for (CVI_U32 i = 0; i < dstTblByteSize; i++) {
    dstTbl.pu8VirAddr[i] = dstTblByteSize - 1 - i;
  }

  printf("Run TPU Map.\n");
  struct timeval t0, t1;
  gettimeofday(&t0, NULL);
  for (size_t i = 0; i < total_run; i++) {
    CVI_IVE_Map(handle, &src, &dstTbl, &dst, 0);
  }
  gettimeofday(&t1, NULL);
  unsigned long elapsed_tpu =
      ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec) / total_run;

  CVI_IVE_BufRequest(handle, &src);
  CVI_IVE_BufRequest(handle, &dst);
  int ret = cpu_ref(&src, &dstTbl, &dst);

  if (total_run == 1) {
    printf("TPU avg time %lu\n", elapsed_tpu);
  }
#ifdef __ARM_ARCH
  else {
    printf("OOO %-10s %10lu %10s %10s\n", "MAP", elapsed_tpu, "NA", "NA");
  }
#endif

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src);
  CVI_SYS_FreeI(handle, &dst);
  CVI_SYS_FreeM(handle, &dstTbl);
  CVI_IVE_DestroyHandle(handle);

  return ret;
}

int cpu_ref(IVE_SRC_IMAGE_S *src, IVE_MEM_INFO_S *table, IVE_DST_IMAGE_S *dst) {
  printf("Check table result: ");
  int ret = CVI_SUCCESS;
  CVI_U32 srcLength = (CVI_U32)(TEST_W * TEST_H);
  for (CVI_U32 i = 0; i < srcLength; i++) {
    CVI_U8 src_val = src->pu8VirAddr[0][i];
    CVI_U8 value = table->pu8VirAddr[src_val];
    if (value != dst->pu8VirAddr[0][i]) {
      printf("Value at index %u are not the same: TPU %u, CPU %u\n", i, dst->pu8VirAddr[0][i],
             value);
      ret = CVI_FAILURE;
    }
  }
  printf("%s\n", ret == CVI_SUCCESS ? "passed" : "failed");
  return ret;
}