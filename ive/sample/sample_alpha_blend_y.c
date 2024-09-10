#include "cvi_ive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

void RGBToYUV420(IVE_IMAGE_S *rgb, IVE_IMAGE_S *yuv420) {
  for (uint32_t c = 0; c < 3; c++) {
    uint16_t height = c < 1 ? yuv420->u32Height : yuv420->u32Height / 2;
    memset(yuv420->pu8VirAddr[c], 0, yuv420->u16Stride[c] * height);
  }

  CVI_U8 *pY = yuv420->pu8VirAddr[0];
  CVI_U8 *pU = yuv420->pu8VirAddr[1];
  CVI_U8 *pV = yuv420->pu8VirAddr[2];

  for (uint16_t h = 0; h < rgb->u32Height; h++) {
    for (uint16_t w = 0; w < rgb->u32Width; w++) {
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

void YUV420pToRGB(IVE_IMAGE_S *yuv420, IVE_IMAGE_S *rgb) {
  for (int i = 0; i < yuv420->u32Height; i++) {
    for (int j = 0; j < yuv420->u32Width; j++) {
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

int main(int argc, char **argv) {
  if (argc != 3) {
    printf("Incorrect parameter. Usage: %s <image1> <image2>\n", argv[0]);
    return CVI_FAILURE;
  }

  const char *file_name1 = argv[1];
  const char *file_name2 = argv[2];

  // Create instance
  printf("Create instance.\n");
  IVE_HANDLE handle = CVI_IVE_CreateHandle();

  // Read image from file. CVI_IVE_ReadImage will do the flush for you.
  IVE_IMAGE_S src1 = CVI_IVE_ReadImage(handle, file_name1, IVE_IMAGE_TYPE_U8C3_PLANAR);
  IVE_IMAGE_S src2 = CVI_IVE_ReadImage(handle, file_name2, IVE_IMAGE_TYPE_U8C3_PLANAR);

  int width = src1.u32Width;
  int height = src1.u32Height;

  printf("Convert image from RGB to YUV420P\n");
  IVE_IMAGE_S src1_yuv;
  CVI_IVE_CreateImage(handle, &src1_yuv, IVE_IMAGE_TYPE_YUV420P, width, height);

  RGBToYUV420(&src1, &src1_yuv);
  // Flush to DRAM before IVE function.
  CVI_IVE_BufFlush(handle, &src1_yuv);

  IVE_IMAGE_S src2_yuv;
  CVI_IVE_CreateImage(handle, &src2_yuv, IVE_IMAGE_TYPE_YUV420P, width, height);
  RGBToYUV420(&src2, &src2_yuv);
  // Flush to DRAM before IVE function.
  CVI_IVE_BufFlush(handle, &src2_yuv);

  printf("create alpha\n");
  // create horizontal gradient alpha image
  IVE_DST_IMAGE_S alpha;
  CVI_IVE_CreateImage(handle, &alpha, IVE_IMAGE_TYPE_YUV420P, width, height);

  for (int c = 0; c < 3; c++) {
    int plane_width = c < 1 ? width : width / 2;
    int plane_height = c < 1 ? height : height / 2;

    int gradient_start = 0.3 * plane_width;
    int gradient_end = 0.7 * plane_width;
    int gradient_length = gradient_end - gradient_start;
    int strideWidth = alpha.u16Stride[c];

    for (int j = 0; j < plane_height; j++) {
      for (int i = 0; i < plane_width; i++) {
        if (i < gradient_start) {
          alpha.pu8VirAddr[c][i + j * strideWidth] = 0;
        } else if (i > gradient_end) {
          alpha.pu8VirAddr[c][i + j * strideWidth] = 255;
        } else {
          uint8_t alpha_pixel = (uint8_t)(((float)(i - gradient_start) / gradient_length) * 255);
          alpha.pu8VirAddr[c][i + j * strideWidth] = alpha_pixel;
        }
      }
    }
  }

  // Flush to DRAM before IVE function.
  CVI_IVE_BufFlush(handle, &alpha);

  IVE_DST_IMAGE_S dst;
  CVI_IVE_CreateImage(handle, &dst, IVE_IMAGE_TYPE_YUV420P, width, height);

  printf("Run IVE alpha blending.\n");
  // Run IVE blend.

  struct timeval t0, t1;
  gettimeofday(&t0, NULL);

  VIDEO_FRAME_INFO_S vf_src1, vf_src2, vf_alpha;
  CVI_IVE_Image2VideoFrameInfo(&src1_yuv, &vf_src1);
  CVI_IVE_Image2VideoFrameInfo(&src2_yuv, &vf_src2);
  CVI_IVE_Image2VideoFrameInfo(&alpha, &vf_alpha);

  uint8_t *p_src2_y = new uint8_t[src2_yuv.u16Stride[0] * height];
  memcpy(p_src2_y, src2_yuv.pu8VirAddr[0], src2_yuv.u16Stride[0] * height);

  CVI_S32 ret = CVI_IVE_Blend_Pixel_Y(handle, &vf_src1, &vf_src2, &vf_alpha);
  gettimeofday(&t1, NULL);
  unsigned long elapsed_cpu = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;
  printf("Run IVE alpha blending done, elapsed: %lu us, ret = %s.\n", elapsed_cpu,
         ret == CVI_SUCCESS ? "Success" : "Fail");

  // check equality

  // // Free memory, instance
  CVI_SYS_FreeI(handle, &alpha);
  CVI_SYS_FreeI(handle, &src1);
  CVI_SYS_FreeI(handle, &src2);
  CVI_SYS_FreeI(handle, &src1_yuv);
  CVI_SYS_FreeI(handle, &src2_yuv);
  CVI_SYS_FreeI(handle, &dst);
  CVI_IVE_DestroyHandle(handle);
  return ret;
}