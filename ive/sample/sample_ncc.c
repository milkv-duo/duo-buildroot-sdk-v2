#include "ive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

int main(int argc, char** argv) {
  if (argc != 3) {
    printf("Usage: %s <file1 name> <file2 name>\n", argv[0]);
    return CVI_FAILURE;
  }
  const char* filename = argv[1];
  const char* filename2 = argv[2];
  // Create instance
  IVE_HANDLE handle = CVI_IVE_CreateHandle();
  printf("BM Kernel init.\n");

  // Fetch image information
  IVE_IMAGE_S src = CVI_IVE_ReadImage(handle, filename, IVE_IMAGE_TYPE_U8C1);
  int nChannels = 1;
  int width = src.u16Width;
  int height = src.u16Height;
  printf("Image size is %d X %d, channel %d\n", width, height, nChannels);
  IVE_IMAGE_S src2 = CVI_IVE_ReadImage(handle, filename2, IVE_IMAGE_TYPE_U8C1);
  int width2 = src2.u16Width;
  int height2 = src2.u16Height;

  IVE_DST_MEM_INFO_S dstNCC;
  CVI_U32 dstSize = sizeof(IVE_NCC_DST_MEM_S) * 4;
  CVI_IVE_CreateMemInfo(handle, &dstNCC, dstSize);
  dstSize = sizeof(IVE_NCC_DST_MEM_S);

  if (width != width2 || height != height2) {
    printf("Error: The size of two images is different.\n");
    CVI_SYS_FreeI(handle, &src);
    CVI_SYS_FreeI(handle, &src2);
    CVI_SYS_FreeM(handle, &dstNCC);
    CVI_IVE_DestroyHandle(handle);
    return 0;
  }

  printf("Run CPU NCC.\n");

  struct timeval t0, t1;
  gettimeofday(&t0, NULL);
  CVI_IVE_NCC(handle, &src, &src2, &dstNCC, 0);
  gettimeofday(&t1, NULL);
  unsigned long elapsed_cpu = ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec);

  printf("CPU time %lu\n", elapsed_cpu);
  printf("NCC value is %f.\n", ((float*)dstNCC.pu8VirAddr)[0]);

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src);
  CVI_SYS_FreeI(handle, &src2);
  CVI_SYS_FreeM(handle, &dstNCC);
  CVI_IVE_DestroyHandle(handle);

  return 0;
}
