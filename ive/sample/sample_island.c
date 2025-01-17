#include "ive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// clang-format off
static char test_array[] = {
  1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
  0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0,
  0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0,
  0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, //
};
//clang-format on

#define TEST_W 25
#define TEST_H 20

int main(int argc, char **argv) {
  if (argc != 1) {
    printf("Incorrect loop value. Usage: %s\n", argv[0]);
    return CVI_FAILURE;
  }
  // Create instance
  IVE_HANDLE handle = CVI_IVE_CreateHandle();
  printf("BM Kernel init.\n");

  // Get test matrix.
  IVE_SRC_IMAGE_S src, dst;
  CVI_IVE_CreateImage(handle, &src, IVE_IMAGE_TYPE_U8C1, TEST_W, TEST_H);
  CVI_IVE_CreateImage(handle, &dst, IVE_IMAGE_TYPE_U8C1, TEST_W, TEST_H);
  // Print the array.
  printf("Src:\n");
  uint16_t stride = src.u16Stride[0];
  printf("stride %u\n", stride);
  for (size_t i = 0; i < TEST_H; i++) {
    for (size_t j = 0; j < TEST_W; j++) {
      src.pu8VirAddr[0][i * stride + j] = 255 * test_array[i * TEST_W + j];
      printf("%3u ", src.pu8VirAddr[0][i * stride + j]);
    }
    printf("\n");
  }
  // Flush data to DRAM.
  CVI_IVE_BufFlush(handle, &src);

  printf("Run CPU CC.\n");
  int numOfComponents = 0;
  IVE_CC_CTRL_S ccCtrl;
  ccCtrl.enMode = DIRECTION_8;
  int ret = CVI_IVE_CC(handle, &src, &dst, &numOfComponents, &ccCtrl, false);

  // Refresh CPU cache before CPU use.
  CVI_IVE_BufRequest(handle, &dst);
  // Print the array.
  for (size_t i = 0; i < TEST_H; i++) {
    for (size_t j = 0; j < TEST_W; j++) {
      printf("%3u ", src.pu8VirAddr[0][i * stride + j]);
    }
    printf("\n");
  }
  printf("Num of components %d. Dst:\n", numOfComponents);
  for (size_t i = 0; i < TEST_H; i++) {
    for (size_t j = 0; j < TEST_W; j++) {
      printf("%3u ", dst.pu8VirAddr[0][i * stride + j]);
    }
    printf("\n");
  }

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src);
  CVI_SYS_FreeI(handle, &dst);
  CVI_IVE_DestroyHandle(handle);

  return ret;
}