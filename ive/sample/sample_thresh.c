#include "ive.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

int main(int argc, char **argv) {
  if (argc != 4) {
    printf("Incorrect loop value. Usage: %s <w> <h> <file_name>\n", argv[0]);
    printf("Example: %s 352 288 data/00_352x288_y.yuv\n", argv[0]);
    return CVI_FAILURE;
  }
  // 00_352x288_y.yuv
  int input_w, input_h;

  input_w = atoi(argv[1]);
  input_h = atoi(argv[2]);

  const char *file_name = argv[3];

  // Create instance
  printf("Create instance.\n");
  IVE_HANDLE handle = CVI_IVE_CreateHandle();

  // Read image from file. CVI_IVE_ReadImage will do the flush for you.
  IVE_IMAGE_S src1;

  src1.u16Width = input_w;
  src1.u16Height = input_h;
  CVI_IVE_ReadRawImage(handle, &src1, (char *)file_name, IVE_IMAGE_TYPE_U8C1, input_w, input_h);

  // Create dst image.
  IVE_DST_IMAGE_S dst;

  CVI_IVE_CreateImage(handle, &dst, IVE_IMAGE_TYPE_U8C1, input_w, input_h);

  // Config Setting.
  IVE_THRESH_CTRL_S iveThreshCtrl;

  iveThreshCtrl.enMode = IVE_THRESH_MODE_BINARY;
  iveThreshCtrl.u8LowThr = 41;
  iveThreshCtrl.u8MinVal = 190;
  iveThreshCtrl.u8MaxVal = 225;

  // Run IVE
  printf("Run HW IVE Threashold BINARY.\n");
  struct timeval t0, t1;

  gettimeofday(&t0, NULL);
  int ret = CVI_IVE_Thresh(handle, &src1, &dst, &iveThreshCtrl, 1);

  gettimeofday(&t1, NULL);

  unsigned long elapsed = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;

  if (ret != CVI_SUCCESS) {
    printf("Failed when run CVI_IVE_Threashold ret: %x\n", ret);
    CVI_SYS_FreeI(handle, &dst);
    CVI_IVE_DestroyHandle(handle);
    return ret;
  }

  printf("elapsed time: %lu us\n", elapsed);

  // Refresh CPU cache before CPU use.
  CVI_IVE_BufRequest(handle, &dst);

  printf("Save result to file.\n");
  CVI_IVE_WriteImage(handle, "sample_Thresh_Binary.yuv", &dst);

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src1);
  CVI_SYS_FreeI(handle, &dst);
  CVI_IVE_DestroyHandle(handle);

  return ret;
}