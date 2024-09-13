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

  IVE_DST_MEM_INFO_S dstInteg;
  CVI_U32 dstIntegSize = (width + 1) * (height + 1) * sizeof(CVI_U32);
  CVI_IVE_CreateMemInfo(handle, &dstInteg, dstIntegSize);
  dstIntegSize = (width + 1) * (height + 1);

  printf("Run CPU Integral Image.\n");
  IVE_INTEG_CTRL_S pstIntegCtrl;

  pstIntegCtrl.enOutCtrl = IVE_INTEG_OUT_CTRL_SUM;

  struct timeval t0, t1;
  gettimeofday(&t0, NULL);
  CVI_IVE_Integ(handle, &src, &dstInteg, &pstIntegCtrl, 0);
  gettimeofday(&t1, NULL);
  unsigned long elapsed_cpu = ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec);

  CVI_IVE_BufRequest(handle, &src);

  printf("CPU time %lu\n", elapsed_cpu);
  // write result to disk
  printf("Output 10 Integral values.\n");
  for (size_t j = 0; j < 10; j++) {
    printf("%3d ", ((CVI_U32 *)dstInteg.pu8VirAddr)[width + j]);
  }
  printf("\n");

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src);
  CVI_SYS_FreeM(handle, &dstInteg);
  CVI_IVE_DestroyHandle(handle);

  return ret;
}
