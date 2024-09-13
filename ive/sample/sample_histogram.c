#include "cvi_ive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Usage: %s <file name>\n", argv[0]);
    return CVI_FAILURE;
  }
  const char *filename = argv[1];
  // Create instance
  IVE_HANDLE handle = CVI_IVE_CreateHandle();
  int ret = CVI_SUCCESS;
  printf("BM Kernel init.\n");

  // Fetch image information
  IVE_IMAGE_S src = CVI_IVE_ReadImage(handle, filename, IVE_IMAGE_TYPE_U8C1);
  int nChannels = 1;
  int width = src.u32Width;
  int height = src.u32Height;
  printf("Image size is %d X %d, channel %d\n", width, height, nChannels);
  CVI_IVE_BufFlush(handle, &src);

  IVE_DST_MEM_INFO_S dstHist;
  CVI_U32 dstHistSize = 256 * sizeof(CVI_U32);
  CVI_IVE_CreateMemInfo(handle, &dstHist, dstHistSize);
  dstHistSize = 256;

  printf("Run CPU Histogram.\n");

  struct timeval t0, t1;
  gettimeofday(&t0, NULL);
  CVI_IVE_Hist(handle, &src, &dstHist, 0);

  gettimeofday(&t1, NULL);
  unsigned long elapsed_cpu = ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec);

  CVI_IVE_BufRequest(handle, &src);
  printf("CPU time %lu\n", elapsed_cpu);
  printf("Output 10 Histogram values.\n");
  for (size_t i = 0; i < 10; i++) {
    printf("%3d ", ((CVI_U32 *)dstHist.pu8VirAddr)[i]);
  }
  printf("\n");

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src);
  CVI_SYS_FreeM(handle, &dstHist);
  CVI_IVE_DestroyHandle(handle);

  return ret;
}
