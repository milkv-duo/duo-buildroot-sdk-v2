#include "bmkernel/bm_kernel.h"

#include "bmkernel/bm1880v2/1880v2_fp_convert.h"
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
  int ret = CVI_SUCCESS;
  printf("BM Kernel init.\n");

  // Fetch image information
  IVE_IMAGE_S src = CVI_IVE_ReadImage(handle, filename, IVE_IMAGE_TYPE_U8C1);
  int nChannels = 1;
  int width = src.u32Width;
  int height = src.u32Height;
  // size_t img_data_sz = nChannels * src.u16Stride[0] * height;
  printf("Image size is %d X %d, channel %d\n", width, height, nChannels);
  CVI_IVE_BufFlush(handle, &src);

  IVE_DST_MEM_INFO_S dstHist;
  CVI_U32 dstHistSize = 256 * sizeof(uint32_t);
  CVI_IVE_CreateMemInfo(handle, &dstHist, dstHistSize);
  dstHistSize = 256;

  printf("Run CPU Histogram.\n");

  struct timeval t0, t1;
  gettimeofday(&t0, NULL);
  for (size_t i = 0; i < total_run; i++) {
    CVI_IVE_Hist(handle, &src, &dstHist, 0);
  }
  gettimeofday(&t1, NULL);
  unsigned long elapsed_cpu =
      ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec) / total_run;

  CVI_IVE_BufRequest(handle, &src);
  if (total_run == 1) {
    printf("CPU time %lu\n", elapsed_cpu);
    printf("Output Histogram feature.\n");
    for (size_t i = 0; i < dstHistSize; i++) {
      printf("%3d ", ((uint32_t *)dstHist.pu8VirAddr)[i]);
    }
    printf("\n");
  }

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src);
  CVI_SYS_FreeM(handle, &dstHist);
  CVI_IVE_DestroyHandle(handle);

  return ret;
}
