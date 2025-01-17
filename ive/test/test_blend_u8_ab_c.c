#include "ive.h"
// #include "cvi_sys.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
int cpu_ref(const int channels, IVE_SRC_IMAGE_S *src1, IVE_SRC_IMAGE_S *src2,
            IVE_SRC_IMAGE_S *alpha1, IVE_SRC_IMAGE_S *alpha2, IVE_DST_IMAGE_S *dst);

void printBits(size_t const size, void const *const ptr) {
  unsigned char *b = (unsigned char *)ptr;
  unsigned char byte;
  int i, j;

  for (i = size - 1; i >= 0; i--) {
    for (j = 7; j >= 0; j--) {
      byte = (b[i] >> j) & 1;
      printf("%u", byte);
    }
    printf(" ");
  }
}
void RGBToYUV420(IVE_IMAGE_S *rgb, IVE_IMAGE_S *yuv420) {
  for (uint32_t c = 0; c < 3; c++) {
    uint16_t height = c < 1 ? yuv420->u16Height : yuv420->u16Height / 2;
    memset(yuv420->pu8VirAddr[c], 0, yuv420->u16Stride[c] * height);
  }

  CVI_U8 *pY = yuv420->pu8VirAddr[0];
  CVI_U8 *pU = yuv420->pu8VirAddr[1];
  CVI_U8 *pV = yuv420->pu8VirAddr[2];

  for (uint16_t h = 0; h < rgb->u16Height; h++) {
    for (uint16_t w = 0; w < rgb->u16Width; w++) {
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
int generate_alpha_image(IVE_HANDLE handle, IVE_IMAGE_TYPE_E w_type, int width, int height,
                         IVE_DST_IMAGE_S *p_img) {
  int ret = CVI_IVE_CreateImage(handle, p_img, w_type, width, height);
  if (ret != CVI_SUCCESS) return ret;
  int num_channel = 3;
  if (w_type == IVE_IMAGE_TYPE_U8C1) {
    num_channel = 1;
  }
  srand(time(NULL));  // Initialization, should only be called once.
  for (int c = 0; c < num_channel; c++) {
    int plane_width = width;    // c < 1 ? width : width / 2;
    int plane_height = height;  // c < 1 ? height : height / 2;
    if (w_type == IVE_IMAGE_TYPE_YUV420P) {
      plane_width = c < 1 ? width : width / 2;
      plane_height = c < 1 ? height : height / 2;
    }
    int strideWidth = p_img->u16Stride[c];
    for (int j = 0; j < plane_height; j++) {
      for (int i = 0; i < plane_width; i++) {
        // int r = (i+100)*(j+300);
        int r = rand();
        uint8_t alpha_pixel = (uint8_t)(r % 255);
        p_img->pu8VirAddr[c][i + j * strideWidth] = alpha_pixel;
      }
    }
  }
  return ret;
}
int main(int argc, char **argv) {
  //  if (argc != 3) {
  //     printf("Incorrect parameter. Usage: %s <image1> <image2>\n", argv[0]);
  //     return CVI_FAILURE;
  //   }

  const char *file_name1 = argv[1];  //"/mnt/data/admin1_data/alios_test/a.jpg";
  const char *file_name2 = argv[2];  //"/mnt/data/admin1_data/alios_test/b.jpg";//argv[2];

  // Create instance
  printf("Create instance.\n");
  // CVI_SYS_Init();
  IVE_HANDLE handle = CVI_IVE_CreateHandle();

  // Read image from file. CVI_IVE_ReadImage will do the flush for you.
  IVE_IMAGE_TYPE_E img_type = IVE_IMAGE_TYPE_YUV420P;

  // IVE_IMAGE_TYPE_E w_type = IVE_IMAGE_TYPE_U8C1;//IVE_IMAGE_TYPE_S8C1
  IVE_IMAGE_S src1 = CVI_IVE_ReadImage(handle, file_name1, IVE_IMAGE_TYPE_U8C3_PLANAR);
  IVE_IMAGE_S src2 = CVI_IVE_ReadImage(handle, file_name2, IVE_IMAGE_TYPE_U8C3_PLANAR);

  int width = src1.u16Width;
  int height = src1.u16Height;
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

  CVI_SYS_FreeI(handle, &src1);
  CVI_SYS_FreeI(handle, &src2);
  int num_channel = 3;
  if (img_type == IVE_IMAGE_TYPE_U8C1) {
    num_channel = 1;
  }
  // create horizontal gradient alpha image
  IVE_DST_IMAGE_S alpha1, alpha2;
  int ret = generate_alpha_image(handle, img_type, width, height, &alpha1);
  ret = generate_alpha_image(handle, img_type, width, height, &alpha2);

  // Flush to DRAM before IVE function.
  CVI_IVE_BufFlush(handle, &alpha1);
  CVI_IVE_BufFlush(handle, &alpha2);

  IVE_DST_IMAGE_S dst;
  CVI_IVE_CreateImage(handle, &dst, img_type, width, height);

  printf("Run IVE alpha s8 blending.\n");
  // Run IVE blend.

  struct timeval t0, t1;
  gettimeofday(&t0, NULL);
  ret = CVI_IVE_Blend_Pixel_U8_AB(handle, &src1_yuv, &src2_yuv, &alpha1, &alpha2, &dst);
  gettimeofday(&t1, NULL);
  unsigned long elapsed_cpu = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;
  printf("Run IVE alpha blending done, elapsed: %lu us, ret = %s.\n", elapsed_cpu,
         ret == CVI_SUCCESS ? "Success" : "Fail");

  if (ret == CVI_SUCCESS) {
    // Refresh CPU cache before CPU use.
    CVI_IVE_BufRequest(handle, &src1_yuv);
    CVI_IVE_BufRequest(handle, &src2_yuv);
    CVI_IVE_BufRequest(handle, &dst);

    // printf("Save result to file.\n");
    // CVI_IVE_WriteImage(handle, "sample_alpha_blend_pixel.png", &dst);
    if (CVI_SUCCESS == cpu_ref(num_channel, &src1_yuv, &src2_yuv, &alpha1, &alpha2, &dst)) {
      printf("check ok\n");
    }
  }

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src1_yuv);
  CVI_SYS_FreeI(handle, &src2_yuv);
  CVI_SYS_FreeI(handle, &alpha1);
  CVI_SYS_FreeI(handle, &alpha2);
  CVI_SYS_FreeI(handle, &dst);
  CVI_IVE_DestroyHandle(handle);
  // CVI_SYS_Exit();
  return 0;
}

int cpu_ref(const int channels, IVE_SRC_IMAGE_S *src_img1, IVE_SRC_IMAGE_S *src_img2,
            IVE_SRC_IMAGE_S *alpha_img1, IVE_SRC_IMAGE_S *alpha_img2, IVE_DST_IMAGE_S *dst_img) {
  int ret = CVI_SUCCESS;
  for (int c = 0; c < channels; c++) {
    CVI_U8 *src1_ptr = src_img1->pu8VirAddr[c];
    CVI_U8 *src2_ptr = src_img2->pu8VirAddr[c];
    CVI_U8 *alpha_ptr1 = alpha_img1->pu8VirAddr[c];
    CVI_U8 *alpha_ptr2 = alpha_img2->pu8VirAddr[c];
    CVI_U8 *dst_ptr = dst_img->pu8VirAddr[c];
    int plane_width = src_img1->u16Width;    // c < 1 ? width : width / 2;
    int plane_height = src_img1->u16Height;  // c < 1 ? height : height / 2;
    if (src_img1->enType == IVE_IMAGE_TYPE_YUV420P) {
      plane_width = c < 1 ? plane_width : plane_width / 2;
      plane_height = c < 1 ? plane_height : plane_height / 2;
    }
    for (size_t r = 0; r < plane_height; r++) {
      CVI_U8 *src1 = src1_ptr + r * src_img1->u16Stride[c];
      CVI_U8 *src2 = src2_ptr + r * src_img2->u16Stride[c];
      CVI_U8 *alpha1 = alpha_ptr1 + r * alpha_img1->u16Stride[c];
      CVI_U8 *alpha2 = alpha_ptr2 + r * alpha_img2->u16Stride[c];
      CVI_U8 *dst = dst_ptr + r * dst_img->u16Stride[c];

      for (size_t i = 0; i < plane_width; i++) {
        int src_val = src1[i] * alpha1[i] + src2[i] * alpha2[i];
        int retval = src_val / 256.0 + 0.5;
        if (retval > 255) retval = 255;
        if (retval != dst[i]) {
          printf("fail,[%zu] u8_a:%d,u8_b:%d,u8_wa:%u,u8_wb:%u,TPU %d, CPU %d,srcval:%d,binary:", i,
                 src1[i], src2[i], alpha1[i], alpha2[i], dst[i], retval, src_val);
          printBits(2, &src_val);
          printf("\n");
          return CVI_FAILURE;
        }
      }
    }
  }
  return ret;
}
