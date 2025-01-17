#include "ive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// clang-format off
static char test_array[] = {
  1, 2, 3, 4, 5, 2, 2, 2, 2, 2, 7, 7, 7, 7, 7, 9, 9, 9, 9, 9, 1, 2, 3, 4, 5,
  1, 2, 3, 4, 5, 2, 2, 2, 2, 2, 7, 7, 7, 7, 7, 9, 9, 9, 9, 9, 1, 2, 3, 4, 5,
  1, 2, 3, 4, 5, 2, 2, 2, 2, 2, 7, 7, 7, 7, 7, 9, 9, 9, 9, 9, 1, 2, 3, 4, 5,
  1, 2, 3, 4, 5, 2, 2, 2, 2, 2, 7, 7, 7, 7, 7, 9, 9, 9, 9, 9, 1, 2, 3, 4, 5,
  1, 2, 3, 4, 5, 2, 2, 2, 2, 2, 7, 7, 7, 7, 7, 9, 9, 9, 9, 9, 1, 2, 3, 4, 5, //
  6, 6, 6, 6, 6, 4, 4, 4, 4, 4, 1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 7, 7, 7, 7, 7,
  6, 6, 6, 6, 6, 4, 4, 4, 4, 4, 1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 7, 7, 7, 7, 7,
  6, 6, 6, 6, 6, 4, 4, 4, 4, 4, 1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 7, 7, 7, 7, 7,
  6, 6, 6, 6, 6, 4, 4, 4, 4, 4, 1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 7, 7, 7, 7, 7,
  6, 6, 6, 6, 6, 4, 4, 4, 4, 4, 1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 7, 7, 7, 7, 7, //
  1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 1, 1, 1, 1, 1, 1, 2, 3, 4, 5, 1, 2, 3, 4, 5,
  1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 1, 1, 1, 1, 1, 1, 2, 3, 4, 5, 1, 2, 3, 4, 5,
  1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 1, 1, 1, 1, 1, 1, 2, 3, 4, 5, 1, 2, 3, 4, 5,
  1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 1, 1, 1, 1, 1, 1, 2, 3, 4, 5, 1, 2, 3, 4, 5,
  1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 1, 1, 1, 1, 1, 1, 2, 3, 4, 5, 1, 2, 3, 4, 5, //
  1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 8, 8, 8, 8, 8, 1, 2, 3, 4, 5, 1, 2, 3, 4, 5,
  1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 8, 8, 8, 8, 8, 1, 2, 3, 4, 5, 1, 2, 3, 4, 5,
  1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 8, 8, 8, 8, 8, 1, 2, 3, 4, 5, 1, 2, 3, 4, 5,
  1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 8, 8, 8, 8, 8, 1, 2, 3, 4, 5, 1, 2, 3, 4, 5,
  1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 8, 8, 8, 8, 8, 1, 2, 3, 4, 5, 1, 2, 3, 4, 5, //
};
//clang-format on

#define TEST_W 25
#define TEST_H 20
#define CELL_SZ 5

int main(int argc, char **argv) {
  if (argc != 1) {
    printf("Incorrect loop value. Usage: %s\n", argv[0]);
    return CVI_FAILURE;
  }
  // Create instance
  IVE_HANDLE handle = CVI_IVE_CreateHandle();
  printf("BM Kernel init.\n");

  // Get test matrix.
  IVE_SRC_IMAGE_S src;
  CVI_IVE_CreateImage(handle, &src, IVE_IMAGE_TYPE_U8C1, TEST_W, TEST_H);
  // Print the array.
  uint16_t stride = src.u16Stride[0];
  for (size_t i = 0; i < TEST_H; i++) {
    for (size_t j = 0; j < TEST_W; j++) {
      src.pu8VirAddr[0][i * stride + j] = test_array[i * TEST_W + j];
      printf("%3u ", src.pu8VirAddr[0][i * stride + j]);
    }
    printf("\n");
  }
  // Flush data to DRAM.
  CVI_IVE_BufFlush(handle, &src);

  // CVI_IVE_BLOCK is a function that average the numbers in a cell for you.
  // You can also setup the additional f32ScaleSize to do additional division.
  // Y = averageInACell(X1...Xn) / f32ScaleSize
  IVE_BLOCK_CTRL_S iveBlkCtrl;
  iveBlkCtrl.f32ScaleSize = 2;
  iveBlkCtrl.u32CellSize = CELL_SZ;
  // The size of the output must meet the requirement of
  // ==> src.width % dst.width == 0
  // ==> src.height % dst.height == 0
  int res_w = TEST_W / CELL_SZ;
  int res_h = TEST_H / CELL_SZ;
  IVE_DST_IMAGE_S dst, dst_bf16, dst_fp32;
  CVI_IVE_CreateImage(handle, &dst, IVE_IMAGE_TYPE_U8C1, res_w, res_h);
  CVI_IVE_CreateImage(handle, &dst_bf16, IVE_IMAGE_TYPE_BF16C1, res_w, res_h);
  CVI_IVE_CreateImage(handle, &dst_fp32, IVE_IMAGE_TYPE_FP32C1, res_w, res_h);

  printf("Run TPU Block.\n");
  int ret = CVI_IVE_BLOCK(handle, &src, &dst, &iveBlkCtrl, 0);

  ret |= CVI_IVE_BLOCK(handle, &src, &dst_bf16, &iveBlkCtrl, 0);

  // Convert to float for CPU.
  IVE_ITC_CRTL_S iveItcCtrl;
  iveItcCtrl.enType = IVE_ITC_SATURATE;
  CVI_IVE_ImageTypeConvert(handle, &dst_bf16, &dst_fp32, &iveItcCtrl, 0);

  // Refresh CPU cache before CPU use.
  CVI_IVE_BufRequest(handle, &dst);
  CVI_IVE_BufRequest(handle, &dst_fp32);

  printf("U8 result:\n");
  for (size_t i = 0; i < res_h; i++) {
    for (size_t j = 0; j < res_w; j++) {
      printf("%3u ", dst.pu8VirAddr[0][i * stride + j]);
    }
    printf("\n");
  }
  printf("BF16 result:\n");
  uint16_t stride2 = dst_fp32.u16Stride[0];
  for (size_t i = 0; i < res_h; i++) {
    float *line = (float *)(dst_fp32.pu8VirAddr[0] + i * stride2);
    for (size_t j = 0; j < res_w; j++) {
      printf("%.3f ", line[j]);
    }
    printf("\n");
  }

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src);
  CVI_SYS_FreeI(handle, &dst);
  CVI_SYS_FreeI(handle, &dst_bf16);
  CVI_SYS_FreeI(handle, &dst_fp32);
  CVI_IVE_DestroyHandle(handle);

  return ret;
}