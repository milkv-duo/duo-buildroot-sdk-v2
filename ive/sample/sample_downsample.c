#include "ive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CELL_SZ 4

void YUV420pToRGB(IVE_IMAGE_S *yuv420, IVE_IMAGE_S *rgb) {
  for (int i = 0; i < yuv420->u16Height; i++) {
    for (int j = 0; j < yuv420->u16Width; j++) {
      float Y = yuv420->pu8VirAddr[0][i * yuv420->u16Stride[0] + j];
      float U = yuv420->pu8VirAddr[1][(i / 2) * yuv420->u16Stride[1] + j / 2];
      float V = yuv420->pu8VirAddr[2][(i / 2) * yuv420->u16Stride[2] + j / 2];

      float R = Y + 1.402 * (V - 128);
      float G = Y - 0.344 * (U - 128) - 0.714 * (V - 128);
      float B = Y + 1.772 * (U - 128);

      if (R < 0) {
        R = 0;
      }
      if (G < 0) {
        G = 0;
      }
      if (B < 0) {
        B = 0;
      }
      if (R > 255) {
        R = 255;
      }
      if (G > 255) {
        G = 255;
      }
      if (B > 255) {
        B = 255;
      }

      rgb->pu8VirAddr[0][j + i * rgb->u16Stride[0]] = (CVI_U8)R;
      rgb->pu8VirAddr[1][j + i * rgb->u16Stride[1]] = (CVI_U8)G;
      rgb->pu8VirAddr[2][j + i * rgb->u16Stride[2]] = (CVI_U8)B;
    }
  }
}

void RGBToYUV420(IVE_IMAGE_S *rgb, IVE_IMAGE_S *yuv420) {
  for (uint32_t c = 0; c < 3; c++) {
    int height = c < 1 ? yuv420->u16Height : yuv420->u16Height / 2;
    memset(yuv420->pu8VirAddr[c], 0, yuv420->u16Stride[c] * height);
  }

  CVI_U8 *pY = yuv420->pu8VirAddr[0];
  CVI_U8 *pU = yuv420->pu8VirAddr[1];
  CVI_U8 *pV = yuv420->pu8VirAddr[2];

  for (int h = 0; h < rgb->u16Height; h++) {
    for (int w = 0; w < rgb->u16Width; w++) {
      int r = rgb->pu8VirAddr[0][w + h * rgb->u16Stride[0]];
      int g = rgb->pu8VirAddr[1][w + h * rgb->u16Stride[1]];
      int b = rgb->pu8VirAddr[2][w + h * rgb->u16Stride[2]];
      pY[w + h * yuv420->u16Stride[0]] = ((66 * r + 129 * g + 25 * b) >> 8) + 16;

      if (h % 2 == 0 && w % 2 == 0) {
        pU[w / 2 + h / 2 * yuv420->u16Stride[1]] = ((-38 * r - 74 * g + 112 * b) >> 8) + 128;
        pV[w / 2 + h / 2 * yuv420->u16Stride[2]] = ((112 * r - 94 * g - 18 * b) >> 8) + 128;
      }
    }
  }
}
void print_yuv420p(IVE_HANDLE handle, IVE_DST_IMAGE_S *img) {
  CVI_IVE_BufRequest(handle, img);
  printf("yuv420p val:%d,%d,%d\n", (int)img->pu8VirAddr[0][0], (int)img->pu8VirAddr[1][0],
         (int)img->pu8VirAddr[2][0]);
}
void writeYUV420P(IVE_HANDLE handle, const char *filename, const uint16_t width,
                  const uint16_t height, IVE_DST_IMAGE_S *img) {
  CVI_IVE_BufRequest(handle, img);
  FILE *yuv = fopen(filename, "wb");
  printf("writeYUV420P = %d %d %d %d %d\n", img->u16Stride[0], img->u16Stride[1], img->u16Stride[2],
         width, height);

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
  printf("argc:%d\n", argc);

  const char *file_name = argv[1];

  IVE_HANDLE handle = CVI_IVE_CreateHandle();
  printf("BM Kernel init.\n");

  IVE_IMAGE_S src1 = CVI_IVE_ReadImage(handle, file_name, IVE_IMAGE_TYPE_U8C3_PLANAR);
  int width = src1.u16Width;
  int height = src1.u16Height;

  printf("Convert image from RGB to YUV420P\n");
  IVE_IMAGE_S src1_yuv;
  CVI_IVE_CreateImage(handle, &src1_yuv, IVE_IMAGE_TYPE_YUV420P, width, height);
  // for debug src png
  CVI_IVE_WriteImage(handle, "down_img_src.png", &src1);

  RGBToYUV420(&src1, &src1_yuv);
  // Flush to DRAM before IVE function.
  CVI_IVE_BufFlush(handle, &src1_yuv);

  IVE_DST_IMAGE_S src_rgb;
  CVI_IVE_CreateImage(handle, &src_rgb, IVE_IMAGE_TYPE_U8C3_PLANAR, width, height);
  YUV420pToRGB(&src1_yuv, &src_rgb);
  CVI_IVE_WriteImage(handle, "img_src.png", &src_rgb);
  CVI_SYS_FreeI(handle, &src_rgb);

  // for debug src yuv
  writeYUV420P(handle, "down_img_src.yuv", width, height, &src1_yuv);
  printf("src1_yuv: w:%d, h:%d, t:%d, s:[%d,%d,%d]\n", src1_yuv.u16Width, src1_yuv.u16Height,
         src1_yuv.enType, src1_yuv.u16Stride[0], src1_yuv.u16Stride[1], src1_yuv.u16Stride[2]);

  IVE_DOWNSAMPLE_CTRL_S ive_ds_Ctrl;
  ive_ds_Ctrl.u8KnerlSize = 2;
  // The size of the output must meet the requirement of
  int res_w = (int)(width / ive_ds_Ctrl.u8KnerlSize);
  int res_h = (int)(height / ive_ds_Ctrl.u8KnerlSize);
  printf("dst_yuv size:[%d, %d]\n", res_w, res_h);
  IVE_DST_IMAGE_S dst_yuv, dst_tmp_rgb;
  printf("create dst_yuv yuv image\n");
  CVI_IVE_CreateImage(handle, &dst_yuv, src1_yuv.enType, res_w, res_h);
  CVI_IVE_CreateImage(handle, &dst_tmp_rgb, IVE_IMAGE_TYPE_U8C3_PLANAR, res_w, res_h);
  CVI_IVE_Zero(handle, &dst_tmp_rgb);
  RGBToYUV420(&dst_tmp_rgb, &dst_yuv);

  CVI_U16 rois[8] = {100, 60, 358, 320, 330, 80, 600, 340};

  for (int i = 1; i < 2; i++) {
    IVE_DST_IMAGE_S src_sub, dst_sub;
    memset(&src_sub, 0, sizeof(IVE_DST_IMAGE_S));
    memset(&dst_sub, 0, sizeof(IVE_DST_IMAGE_S));
    CVI_IVE_SubImage(handle, &src1_yuv, &src_sub, rois[i * 4], rois[i * 4 + 1], rois[i * 4 + 2],
                     rois[i * 4 + 3]);
    int dstw = rois[i * 4 + 2] - rois[i * 4];
    int dsth = rois[i * 4 + 3] - rois[i * 4 + 1];

    CVI_IVE_SubImage(handle, &dst_yuv, &dst_sub, 0, 0, dstw / 2, dsth / 2);
    printf("Run TPU DownSample sub\n");
    print_yuv420p(handle, &dst_yuv);
    CVI_IVE_DOWNSAMPLE(handle, &src_sub, &dst_sub, &ive_ds_Ctrl, 0);

    CVI_SYS_FreeI(handle, &src_sub);
    CVI_SYS_FreeI(handle, &dst_sub);
  }

  printf("dst_yuv:w:%d, h:%d, t:%d, s:[%d,%d,%d]\n", dst_yuv.u16Width, dst_yuv.u16Height,
         dst_yuv.enType, dst_yuv.u16Stride[0], dst_yuv.u16Stride[1], dst_yuv.u16Stride[2]);
  // for debug src yuv
  writeYUV420P(handle, "down_img_dst.yuv", res_w, res_h, &dst_yuv);
  CVI_IVE_BufRequest(handle, &dst_yuv);
  IVE_DST_IMAGE_S dst_rgb;
  printf("create dst_yuv planar image\n");
  CVI_IVE_CreateImage(handle, &dst_rgb, IVE_IMAGE_TYPE_U8C3_PLANAR, res_w, res_h);
  YUV420pToRGB(&dst_yuv, &dst_rgb);

  printf("Save result to file.\n");
  // for debug src png
  CVI_IVE_WriteImage(handle, "down_img_dst.png", &dst_rgb);

  CVI_SYS_FreeI(handle, &dst_rgb);
  CVI_SYS_FreeI(handle, &dst_tmp_rgb);

  CVI_SYS_FreeI(handle, &dst_yuv);
  CVI_SYS_FreeI(handle, &src1);
  CVI_IVE_DestroyHandle(handle);

  return 0;
}