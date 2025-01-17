#include "ive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Incorrect loop value. Usage: %s <file_name>\n", argv[0]);
    return CVI_FAILURE;
  }
  const char *file_name = argv[1];
  // Create instance
  IVE_HANDLE handle = CVI_IVE_CreateHandle();
  printf("BM Kernel init.\n");

  // Fetch image information. CVI_IVE_ReadImage will do the flush for you.
  IVE_IMAGE_S src = CVI_IVE_ReadImage(handle, file_name, IVE_IMAGE_TYPE_U8C1);
  int width = src.u16Width;
  int height = src.u16Height;

  IVE_DST_IMAGE_S dst;
  CVI_IVE_CreateImage(handle, &dst, IVE_IMAGE_TYPE_U8C1, width, height);

  // Direct copy using TPU.
  printf("Run TPU Direct Copy.\n");
  IVE_DMA_CTRL_S iveDmaCtrl;
  iveDmaCtrl.enMode = IVE_DMA_MODE_DIRECT_COPY;
  int ret = CVI_IVE_DMA(handle, &src, &dst, &iveDmaCtrl, 0);

  IVE_DST_IMAGE_S dst2;
  CVI_IVE_CreateImage(handle, &dst2, IVE_IMAGE_TYPE_U8C1, width * 2, height * 2);

  // Interval image with set size with 0.
  printf("Run TPU Interval Copy.\n");
  iveDmaCtrl.enMode = IVE_DMA_MODE_INTERVAL_COPY;
  iveDmaCtrl.u8HorSegSize = 2;
  iveDmaCtrl.u8VerSegRows = 2;
  ret |= CVI_IVE_DMA(handle, &src, &dst2, &iveDmaCtrl, 0);

  // Sub-image does not copy image, it just set a new ROI. DO NOT delete the original image.
  const CVI_U16 x1 = src.u16Width / 10;
  const CVI_U16 y1 = src.u16Height / 10;
  const CVI_U16 x2 = src.u16Width - x1;
  const CVI_U16 y2 = src.u16Height - y1;
  IVE_DST_IMAGE_S src_crop;
  memset(&src_crop, 0, sizeof(IVE_DST_IMAGE_S));
  CVI_IVE_SubImage(handle, &src, &src_crop, x1, y1, x2, y2);
  IVE_DST_IMAGE_S dst3;
  CVI_IVE_CreateImage(handle, &dst3, IVE_IMAGE_TYPE_U8C1, src_crop.u16Width, src_crop.u16Height);

  // Direct copy the sub-image to a new image buffer.
  printf("Run TPU Sub Direct Copy.\n");
  iveDmaCtrl.enMode = IVE_DMA_MODE_DIRECT_COPY;
  ret |= CVI_IVE_DMA(handle, &src_crop, &dst3, &iveDmaCtrl, 0);

  // Refresh CPU cache before CPU use.
  CVI_IVE_BufRequest(handle, &dst);
  CVI_IVE_BufRequest(handle, &dst2);
  CVI_IVE_BufRequest(handle, &dst3);

  // write result to disk
  printf("Save to image.\n");
  CVI_IVE_WriteImage(handle, "sample_copyDir.png", &dst);
  CVI_IVE_WriteImage(handle, "sample_copyInv.png", &dst2);
  CVI_IVE_WriteImage(handle, "sample_copySub.png", &dst3);

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src_crop);
  CVI_SYS_FreeI(handle, &src);
  CVI_SYS_FreeI(handle, &dst);
  CVI_SYS_FreeI(handle, &dst2);
  CVI_SYS_FreeI(handle, &dst3);
  CVI_IVE_DestroyHandle(handle);

  return ret;
}
