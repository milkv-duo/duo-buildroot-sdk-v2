#include "cvi_ive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#ifdef __ARM_ARCH
#include "arm_neon.h"
#endif

int main(int argc, char **argv) {
  if (argc != 1) {
    printf("Incorrect loop value. Usage: %s\n", argv[0]);
    return CVI_FAILURE;
  }
  // Create instance
  IVE_HANDLE handle = CVI_IVE_CreateHandle();
  printf("BM Kernel init.\n");

  // Fetch image information
  const int width = 640;
  const int height = 480;

  IVE_DST_IMAGE_S src, dst;
  CVI_IVE_CreateImage(handle, &src, IVE_IMAGE_TYPE_YUV420P, width, height);
  CVI_IVE_CreateImage(handle, &dst, IVE_IMAGE_TYPE_YUV420P, width, height);
  uint32_t lumaSize = src.u16Stride[0] * src.u32Height;
  uint32_t chromaSize = src.u16Stride[1] * src.u32Height / 2;
  for (uint32_t i = 0; i < 3; i++) {
    uint32_t length = i == 0 ? lumaSize : chromaSize;
    for (uint32_t j = 0; j < length; j++) {
      src.pu8VirAddr[i][j] = j % 256;
    }
  }
  CVI_IVE_BufFlush(handle, &src);

  printf("Run TPU Direct Copy.\n");
  IVE_DMA_CTRL_S iveDmaCtrl;
  iveDmaCtrl.enMode = IVE_DMA_MODE_DIRECT_COPY;
  CVI_IVE_DMA(handle, &src, &dst, &iveDmaCtrl, 0);

  CVI_IVE_BufRequest(handle, &src);
  CVI_IVE_BufRequest(handle, &dst);

  for (uint32_t i = 0; i < 3; i++) {
    uint32_t length = i == 0 ? lumaSize : chromaSize;
    for (uint32_t j = 0; j < length; j++) {
      if (src.pu8VirAddr[i][j] != dst.pu8VirAddr[i][j]) {
        printf("Not the same @ (%u, %u) = (%u, %u)\n", i, j, (uint32_t)src.pu8VirAddr[i][j],
               (uint32_t)dst.pu8VirAddr[i][j]);
        break;
      }
    }
  }

  printf("Multi-run test\n");
  for (uint32_t i = 0; i < 10; i++) {
    printf("No. %u\n", i);
    CVI_IVE_CreateImage(handle, &dst, IVE_IMAGE_TYPE_YUV420P, width, height);
    CVI_IVE_DMA(handle, &src, &dst, &iveDmaCtrl, 0);
    for (uint32_t i = 0; i < 3; i++) {
      uint32_t length = i == 0 ? lumaSize : chromaSize;
      for (uint32_t j = 0; j < length; j++) {
        if (src.pu8VirAddr[i][j] != dst.pu8VirAddr[i][j]) {
          printf("Not the same @ (%u, %u) = (%u, %u)\n", i, j, (uint32_t)src.pu8VirAddr[i][j],
                 (uint32_t)dst.pu8VirAddr[i][j]);
          break;
        }
      }
    }
    CVI_SYS_FreeI(handle, &dst);
  }

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src);
  CVI_IVE_DestroyHandle(handle);

  return CVI_SUCCESS;
}
