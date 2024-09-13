#include "cvi_ive.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
  if (argc != 2) {
    printf("Incorrect loop value. Usage: %s <file name>\n", argv[0]);
    return CVI_FAILURE;
  }
  const char* filename = argv[1];
  // Create instance
  IVE_HANDLE handle = CVI_IVE_CreateHandle();
  printf("BM Kernel init.\n");

  // Read image from file. CVI_IVE_ReadImage will do the flush for you.
  IVE_IMAGE_S src = CVI_IVE_ReadImage(handle, filename, IVE_IMAGE_TYPE_U8C1);
  int nChannels = 1;
  int width = src.u32Width;
  int height = src.u32Height;
  // Create U16 fake data by putting an image into U16C1 buffer.
  size_t img_data_sz = nChannels * src.u16Stride[0] * height;
  printf("Image size is %d X %d, channel %d\n", width, height, nChannels);
  IVE_SRC_IMAGE_S src_u16;
  CVI_IVE_CreateImage(handle, &src_u16, IVE_IMAGE_TYPE_U16C1, width, height);
  for (size_t i = 0; i < img_data_sz; i++) {
    ((CVI_U16*)src_u16.pu8VirAddr[0])[i] = src.pu8VirAddr[0][i];
  }
  CVI_IVE_BufFlush(handle, &src_u16);
  // Create S16 fake data by putting an image into S16C1 buffer.
  IVE_SRC_IMAGE_S src_s16;
  CVI_IVE_CreateImage(handle, &src_s16, IVE_IMAGE_TYPE_S16C1, width, height);
  for (size_t i = 0; i < img_data_sz; i++) {
    ((CVI_S16*)src_s16.pu8VirAddr[0])[i] = src.pu8VirAddr[0][i];
    if (i == 15) {
      printf("Original %u int16_t %u\n", src.pu8VirAddr[0][i],
             ((CVI_S16*)src_s16.pu8VirAddr[0])[i]);
    }
  }
  CVI_IVE_BufFlush(handle, &src_s16);

  IVE_DST_IMAGE_S dst_mmm_u16, dst_mmm_s16;
  CVI_IVE_CreateImage(handle, &dst_mmm_u16, IVE_IMAGE_TYPE_U8C1, width, height);
  CVI_IVE_CreateImage(handle, &dst_mmm_s16, IVE_IMAGE_TYPE_U8C1, width, height);

  // Setup control parameter.
  IVE_THRESH_U16_CTRL_S iveThreshU16Ctrl;
  iveThreshU16Ctrl.u16LowThr = 30;
  iveThreshU16Ctrl.u16HighThr = 170;
  iveThreshU16Ctrl.u8MinVal = 0;
  iveThreshU16Ctrl.u8MidVal = 125;
  iveThreshU16Ctrl.u8MaxVal = 255;
  iveThreshU16Ctrl.enMode = IVE_THRESH_U16_MODE_U16_TO_U8_MIN_MID_MAX;

  printf("Run TPU Threshold U16.\n");
  int ret = CVI_IVE_Thresh_U16(handle, &src_u16, &dst_mmm_u16, &iveThreshU16Ctrl, 0);

  // Setup control parameter.
  // S16 also supports I8 output result.
  IVE_THRESH_S16_CTRL_S iveThreshS16Ctrl;
  iveThreshS16Ctrl.s16LowThr = 30;
  iveThreshS16Ctrl.s16HighThr = 170;
  iveThreshS16Ctrl.un8MinVal.u8Val = 0;
  iveThreshS16Ctrl.un8MidVal.u8Val = 125;
  iveThreshS16Ctrl.un8MaxVal.u8Val = 255;
  iveThreshS16Ctrl.enMode = IVE_THRESH_S16_MODE_S16_TO_U8_MIN_MID_MAX;

  printf("Run TPU Threshold S16.\n");
  ret |= CVI_IVE_Thresh_S16(handle, &src_s16, &dst_mmm_s16, &iveThreshS16Ctrl, 0);

  // Refresh CPU cache before CPU use.
  CVI_IVE_BufRequest(handle, &dst_mmm_u16);
  CVI_IVE_BufRequest(handle, &dst_mmm_s16);

  printf("Save to image.\n");
  CVI_IVE_WriteImage(handle, "sample_threshu16.png", &dst_mmm_u16);
  CVI_IVE_WriteImage(handle, "sample_threshs16.png", &dst_mmm_s16);

  // Free memory, instance.
  CVI_SYS_FreeI(handle, &src);
  CVI_SYS_FreeI(handle, &src_u16);
  CVI_SYS_FreeI(handle, &src_s16);
  CVI_SYS_FreeI(handle, &dst_mmm_u16);
  CVI_SYS_FreeI(handle, &dst_mmm_s16);
  CVI_IVE_DestroyHandle(handle);

  return ret;
}