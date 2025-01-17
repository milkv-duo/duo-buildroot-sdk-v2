#include "ive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void readYUV420P(IVE_HANDLE handle, const char *filename, const uint16_t width,
                 const uint16_t height, IVE_DST_IMAGE_S *img) {
  FILE *yuv = fopen(filename, "rb");
  uint8_t *ptr = (uint8_t *)malloc(width * height * 2 * sizeof(uint8_t));
  fread(ptr, width * height * 2, sizeof(uint8_t), yuv);
  fclose(yuv);
  CVI_IVE_CreateImage(handle, img, IVE_IMAGE_TYPE_YUV420P, width, height);
  uint8_t *ptr2 = ptr;
  for (int i = 0; i < height; i++) {
    memcpy(img->pu8VirAddr[0] + i * img->u16Stride[0], ptr2, width * sizeof(uint8_t));
    ptr2 += width;
  }
  uint16_t width_2 = width / 2;
  uint16_t height_2 = height / 2;
  for (int i = 0; i < height_2; i++) {
    memcpy(img->pu8VirAddr[1] + i * img->u16Stride[1], ptr2, width_2 * sizeof(uint8_t));
    ptr2 += width_2;
  }
  for (int i = 0; i < height_2; i++) {
    memcpy(img->pu8VirAddr[2] + i * img->u16Stride[2], ptr2, width_2 * sizeof(uint8_t));
    ptr2 += width_2;
  }
  free(ptr);
  CVI_IVE_BufFlush(handle, img);
}

void writeYUV420P(IVE_HANDLE handle, const char *filename, const uint16_t width,
                  const uint16_t height, IVE_DST_IMAGE_S *img) {
  CVI_IVE_BufRequest(handle, img);
  FILE *yuv = fopen(filename, "wb");
  for (int i = 0; i < height; i++) {
    fwrite(img->pu8VirAddr[0] + i * img->u16Stride[0], width, sizeof(uint8_t), yuv);
  }
  uint16_t width_2 = width / 2;
  uint16_t height_2 = height / 2;
  for (int i = 0; i < height_2; i++) {
    fwrite(img->pu8VirAddr[1] + i * img->u16Stride[1], width_2, sizeof(uint8_t), yuv);
  }
  for (int i = 0; i < height_2; i++) {
    fwrite(img->pu8VirAddr[2] + i * img->u16Stride[2], width_2, sizeof(uint8_t), yuv);
  }
  fclose(yuv);
}

int main(int argc, char **argv) {
  if (argc != 4) {
    printf("Incorrect loop value. Usage: %s <file_name> <width> <height>\n", argv[0]);
    return CVI_FAILURE;
  }
  const char *file_name = argv[1];
  // Create instance
  IVE_HANDLE handle = CVI_IVE_CreateHandle();
  printf("BM Kernel init.\n");

  uint16_t width = atoi(argv[2]);
  uint16_t height = atoi(argv[3]);
  IVE_SRC_IMAGE_S src;
  readYUV420P(handle, file_name, width, height, &src);

  IVE_DST_IMAGE_S dst;
  CVI_IVE_CreateImage(handle, &dst, IVE_IMAGE_TYPE_YUV420P, width, height);
  IVE_DMA_CTRL_S iveDmaCtrl;
  iveDmaCtrl.enMode = IVE_DMA_MODE_DIRECT_COPY;
  int ret = CVI_IVE_DMA(handle, &src, &dst, &iveDmaCtrl, 0);

  IVE_DST_IMAGE_S dst2, dst2_1, dst2_2, dst2_3, dst2_4;
  memset(&dst2_1, 0, sizeof(IVE_DST_IMAGE_S));
  memset(&dst2_2, 0, sizeof(IVE_DST_IMAGE_S));
  memset(&dst2_3, 0, sizeof(IVE_DST_IMAGE_S));
  memset(&dst2_4, 0, sizeof(IVE_DST_IMAGE_S));
  CVI_IVE_CreateImage(handle, &dst2, IVE_IMAGE_TYPE_YUV420P, width * 2, height * 2);
  CVI_IVE_SubImage(handle, &dst2, &dst2_1, 0, 0, width, height);
  CVI_IVE_SubImage(handle, &dst2, &dst2_2, width, 0, width * 2, height);
  CVI_IVE_SubImage(handle, &dst2, &dst2_3, 0, height, width, height * 2);
  CVI_IVE_SubImage(handle, &dst2, &dst2_4, width, height, width * 2, height * 2);
  ret |= CVI_IVE_DMA(handle, &src, &dst2_1, &iveDmaCtrl, 0);
  ret |= CVI_IVE_DMA(handle, &src, &dst2_2, &iveDmaCtrl, 0);
  ret |= CVI_IVE_DMA(handle, &src, &dst2_3, &iveDmaCtrl, 0);
  ret |= CVI_IVE_DMA(handle, &src, &dst2_4, &iveDmaCtrl, 0);

  uint16_t width_2 = width / 2;
  uint16_t height_2 = height / 2;
  IVE_DST_IMAGE_S src_1, src_2, src_3, src_4;
  memset(&src_1, 0, sizeof(IVE_DST_IMAGE_S));
  memset(&src_2, 0, sizeof(IVE_DST_IMAGE_S));
  memset(&src_3, 0, sizeof(IVE_DST_IMAGE_S));
  memset(&src_4, 0, sizeof(IVE_DST_IMAGE_S));
  CVI_IVE_SubImage(handle, &src, &src_4, 0, 0, width_2, height_2);
  CVI_IVE_SubImage(handle, &src, &src_3, width_2, 0, width, height_2);
  CVI_IVE_SubImage(handle, &src, &src_2, 0, height_2, width_2, height);
  CVI_IVE_SubImage(handle, &src, &src_1, width_2, height_2, width, height);
  IVE_DST_IMAGE_S dst3, dst3_1, dst3_2, dst3_3, dst3_4;
  memset(&dst3_1, 0, sizeof(IVE_DST_IMAGE_S));
  memset(&dst3_2, 0, sizeof(IVE_DST_IMAGE_S));
  memset(&dst3_3, 0, sizeof(IVE_DST_IMAGE_S));
  memset(&dst3_4, 0, sizeof(IVE_DST_IMAGE_S));
  CVI_IVE_CreateImage(handle, &dst3, IVE_IMAGE_TYPE_YUV420P, width, height);
  CVI_IVE_SubImage(handle, &dst3, &dst3_1, 0, 0, width_2, height_2);
  CVI_IVE_SubImage(handle, &dst3, &dst3_2, width_2, 0, width, height_2);
  CVI_IVE_SubImage(handle, &dst3, &dst3_3, 0, height_2, width_2, height);
  CVI_IVE_SubImage(handle, &dst3, &dst3_4, width_2, height_2, width, height);
  ret |= CVI_IVE_DMA(handle, &src_1, &dst3_1, &iveDmaCtrl, 0);
  ret |= CVI_IVE_DMA(handle, &src_2, &dst3_2, &iveDmaCtrl, 0);
  ret |= CVI_IVE_DMA(handle, &src_3, &dst3_3, &iveDmaCtrl, 0);
  ret |= CVI_IVE_DMA(handle, &src_4, &dst3_4, &iveDmaCtrl, 0);

  writeYUV420P(handle, "result.yuv", width, height, &dst);
  writeYUV420P(handle, "result_big.yuv", width * 2, height * 2, &dst2);
  writeYUV420P(handle, "result_rearrange.yuv", width, height, &dst3);

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src_1);
  CVI_SYS_FreeI(handle, &src_2);
  CVI_SYS_FreeI(handle, &src_3);
  CVI_SYS_FreeI(handle, &src_4);
  CVI_SYS_FreeI(handle, &src);
  CVI_SYS_FreeI(handle, &dst);
  CVI_SYS_FreeI(handle, &dst2_1);
  CVI_SYS_FreeI(handle, &dst2_2);
  CVI_SYS_FreeI(handle, &dst2_3);
  CVI_SYS_FreeI(handle, &dst2_4);
  CVI_SYS_FreeI(handle, &dst2);
  CVI_SYS_FreeI(handle, &dst3_1);
  CVI_SYS_FreeI(handle, &dst3_2);
  CVI_SYS_FreeI(handle, &dst3_3);
  CVI_SYS_FreeI(handle, &dst3_4);
  CVI_SYS_FreeI(handle, &dst3);

  CVI_IVE_DestroyHandle(handle);

  return ret;
}
