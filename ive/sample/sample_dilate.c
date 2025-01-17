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
    printf("Example: %s 352 288 data/bin_352x288_y.yuv\n", argv[0]);
    return CVI_FAILURE;
  }
  // bin_352x288_y.yuv
  int input_w, input_h;

  input_w = atoi(argv[1]);
  input_h = atoi(argv[2]);

  const char *file_name = argv[3];

  CVI_U8 arr3by3[25] = {0,   0, 0, 0, 0,   0, 0, 255, 0, 0, 0, 255, 255,
                        255, 0, 0, 0, 255, 0, 0, 0,   0, 0, 0, 0};

  CVI_U8 arr5by5[25] = {0,   0,   255, 0, 0,   0, 0, 255, 0, 0,   255, 255, 255,
                        255, 255, 0,   0, 255, 0, 0, 0,   0, 255, 0,   0};

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
  IVE_DILATE_CTRL_S stCtrlDilate;

  memcpy(stCtrlDilate.au8Mask, arr3by3, sizeof(CVI_U8) * 25);

  // Run IVE
  printf("Run HW IVE Dilate 3x3.\n");
  struct timeval t0, t1;

  gettimeofday(&t0, NULL);
  int ret = CVI_IVE_Dilate(handle, &src1, &dst, &stCtrlDilate, 1);

  gettimeofday(&t1, NULL);
  unsigned long elapsed = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;

  if (ret != CVI_SUCCESS) {
    printf("Failed when run CVI_IVE_Dilate ret: %x\n", ret);
    CVI_SYS_FreeI(handle, &dst);
    CVI_IVE_DestroyHandle(handle);
    return ret;
  }

  printf("elapsed time: %lu us\n", elapsed);

  // Refresh CPU cache before CPU use.
  CVI_IVE_BufRequest(handle, &dst);

  printf("Save result to file.\n");
  CVI_IVE_WriteImage(handle, "sample_Dilate_3x3_dilate_only.bin", &dst);

  memcpy(stCtrlDilate.au8Mask, arr5by5, sizeof(CVI_U8) * 25);
  printf("Run HW IVE Dilate 5x5.\n");
  ret |= CVI_IVE_Dilate(handle, &src1, &dst, &stCtrlDilate, 1);
  CVI_IVE_BufRequest(handle, &dst);
  CVI_IVE_WriteImage(handle, "sample_Dilate_5x5_dilate_only.bin", &dst);

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src1);
  CVI_SYS_FreeI(handle, &dst);
  CVI_IVE_DestroyHandle(handle);

  return ret;
}