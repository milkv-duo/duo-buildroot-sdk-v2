#include "ive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

int main(int argc, char** argv) {
  if (argc != 2) {
    printf("Usage: %s <file name>\n", argv[0]);
    return CVI_FAILURE;
  }
  const char* filename = argv[1];
  // Create instance
  IVE_HANDLE handle = CVI_IVE_CreateHandle();
  printf("BM Kernel init.\n");

  // Fetch image information
  IVE_IMAGE_S src = CVI_IVE_ReadImage(handle, filename, IVE_IMAGE_TYPE_U8C1);
  int nChannels = 1;
  int width = src.u16Width;
  int height = src.u16Height;
  printf("Image size is %d X %d, channel %d\n", width, height, nChannels);

  IVE_DST_IMAGE_S dst;
  CVI_IVE_CreateImage(handle, &dst, IVE_IMAGE_TYPE_U8C1, width, height);

  printf("Run CPU Histogram Equalizatin.\n");

  IVE_EQUALIZE_HIST_CTRL_S ctrl;  // Currently a dummy variable

  struct timeval t0, t1;
  gettimeofday(&t0, NULL);
  CVI_IVE_EqualizeHist(handle, &src, &dst, &ctrl, 0);
  gettimeofday(&t1, NULL);
  unsigned long elapsed_cpu = ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec);

  CVI_IVE_BufRequest(handle, &dst);

  printf("CPU time %lu\n", elapsed_cpu);
  // write result to disk
  printf("Save to image.\n");
  CVI_IVE_WriteImage(handle, "sample_histEq.png", &dst);

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src);
  CVI_SYS_FreeI(handle, &dst);
  CVI_IVE_DestroyHandle(handle);

  return 0;
}
