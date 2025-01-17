#include "ive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#ifdef __ARM_ARCH
#include "arm_neon.h"
#endif

int main(int argc, char** argv) {
  if (argc != 3) {
    printf("Incorrect loop value. Usage: %s <file name> <loop in value (1-1000)>\n", argv[0]);
    return CVI_FAILURE;
  }
  const char* filename = argv[1];
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
  int width = src.u16Width;
  int height = src.u16Height;
  size_t img_data_sz = nChannels * src.u16Stride[0] * height;
  printf("Image size is %d X %d, channel %d\n", width, height, nChannels);
  IVE_SRC_IMAGE_S src_u16;
  CVI_IVE_CreateImage(handle, &src_u16, IVE_IMAGE_TYPE_U16C1, width, height);
  for (size_t i = 0; i < img_data_sz; i++) {
    ((CVI_U16*)src_u16.pu8VirAddr[0])[i] = src.pu8VirAddr[0][i];
  }
  CVI_IVE_BufFlush(handle, &src_u16);

  IVE_DST_IMAGE_S dst;
  CVI_IVE_CreateImage(handle, &dst, IVE_IMAGE_TYPE_U8C1, width, height);

  printf("Run CPU U16 to U8.\n");

  IVE_16BIT_TO_8BIT_CTRL_S ctrl;
  ctrl.enMode = IVE_16BIT_TO_8BIT_MODE_U16_TO_U8;

  struct timeval t0, t1;
  gettimeofday(&t0, NULL);
  for (size_t i = 0; i < total_run; i++) {
    ret = CVI_IVE_16BitTo8Bit(handle, &src_u16, &dst, &ctrl, 0);
  }
  gettimeofday(&t1, NULL);
  unsigned long elapsed_cpu = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;

  CVI_IVE_BufRequest(handle, &dst);

  if (total_run == 1) {
    printf("TPU avg time %s\n", "NA");
    printf("CPU time %lu\n", elapsed_cpu);
    // write result to disk
    printf("Save to image.\n");
    CVI_IVE_WriteImage(handle, "test_16bitTo8bit_c.png", &dst);
  }

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src);
  CVI_SYS_FreeI(handle, &src_u16);
  CVI_SYS_FreeI(handle, &dst);
  CVI_IVE_DestroyHandle(handle);

  return ret;
}
