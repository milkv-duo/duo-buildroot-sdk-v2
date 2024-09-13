#include "cvi_ive.h"
#include "ive_draw.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

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

  IVE_DRAW_RECT_CTRL_S drawCtrl;
  drawCtrl.numsOfRect = 1;
  drawCtrl.rect = (IVE_RECT_S *)malloc(sizeof(IVE_RECT_S));
  drawCtrl.rect[0].pts[0].x = 100;
  drawCtrl.rect[0].pts[0].y = 100;
  drawCtrl.rect[0].pts[1].x = 400;
  drawCtrl.rect[0].pts[1].y = 400;
  drawCtrl.color.r = 255;
  drawCtrl.color.g = 0;
  drawCtrl.color.b = 0;
  CVI_S32 ret = CVI_IVE_DrawRect(handle, &src, &drawCtrl, 0);
  free(drawCtrl.rect);

  writeYUV420P(handle, "draw_rect.yuv", 1920, 1080, &src);
  // Free memory, instance
  CVI_SYS_FreeI(handle, &src);
  CVI_IVE_DestroyHandle(handle);

  return ret;
}