// #include "cvi_sys.h"
#include "cvi_ive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
int cpu_ref(const int channels, IVE_SRC_IMAGE_S *src1, IVE_SRC_IMAGE_S *src2,
            IVE_SRC_IMAGE_S *aplha, IVE_DST_IMAGE_S *dst);

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
  IVE_IMAGE_TYPE_E img_type =
      IVE_IMAGE_TYPE_S8C1;  // IVE_IMAGE_TYPE_S8C3_PLANAR;//IVE_IMAGE_TYPE_S8C1
  IVE_IMAGE_TYPE_E w_type = IVE_IMAGE_TYPE_U8C3_PLANAR;  // IVE_IMAGE_TYPE_S8C1
  IVE_IMAGE_S src1 = CVI_IVE_ReadImage(handle, file_name1, img_type);
  IVE_IMAGE_S src2 = CVI_IVE_ReadImage(handle, file_name2, img_type);

  int width = src1.u32Width;
  int height = src1.u32Height;
  int strideWidth = src1.u16Stride[0];

  int num_channel = 3;
  if (img_type == IVE_IMAGE_TYPE_S8C1) {
    num_channel = 1;
    w_type = IVE_IMAGE_TYPE_U8C1;
  }
  // create horizontal gradient alpha image
  IVE_DST_IMAGE_S alpha;
  CVI_IVE_CreateImage(handle, &alpha, w_type, width, height);
  int gradient_start = 0.3 * width;
  int gradient_end = 0.7 * width;
  int gradient_length = gradient_end - gradient_start;
  for (int c = 0; c < num_channel; c++) {
    for (int j = 0; j < height; j++) {
      for (int i = 0; i < width; i++) {
        // alpha.pu8VirAddr[c][i + j * strideWidth] = i;
        if (i < gradient_start) {
          alpha.pu8VirAddr[c][i + j * strideWidth] = 1;
        } else if (i > gradient_end) {
          alpha.pu8VirAddr[c][i + j * strideWidth] = 1;
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
  CVI_IVE_CreateImage(handle, &dst, img_type, width, height);

  printf("Run IVE alpha s8 blending.\n");
  // Run IVE blend.

  struct timeval t0, t1;
  gettimeofday(&t0, NULL);
  int ret = CVI_IVE_Blend_Pixel_S8_CLIP(handle, &src1, &src2, &alpha, &dst);
  gettimeofday(&t1, NULL);
  unsigned long elapsed_cpu = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;
  printf("Run IVE alpha blending done, elapsed: %lu us, ret = %s.\n", elapsed_cpu,
         ret == CVI_SUCCESS ? "Success" : "Fail");

  if (ret == CVI_SUCCESS) {
    // Refresh CPU cache before CPU use.
    CVI_IVE_BufRequest(handle, &src1);
    CVI_IVE_BufRequest(handle, &src2);
    CVI_IVE_BufRequest(handle, &dst);

    // printf("Save result to file.\n");
    // CVI_IVE_WriteImage(handle, "sample_alpha_blend_pixel.png", &dst);
    if (CVI_SUCCESS == cpu_ref(num_channel, &src1, &src2, &alpha, &dst)) {
      printf("check ok\n");
    }
  }

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src1);
  CVI_SYS_FreeI(handle, &src2);
  CVI_SYS_FreeI(handle, &dst);
  CVI_IVE_DestroyHandle(handle);
  // CVI_SYS_Exit();
  return 0;
}

int cpu_ref(const int channels, IVE_SRC_IMAGE_S *src1, IVE_SRC_IMAGE_S *src2,
            IVE_SRC_IMAGE_S *aplha, IVE_DST_IMAGE_S *dst) {
  int ret = CVI_SUCCESS;
  for (int c = 0; c < channels; c++) {
    CVI_S8 *src1_ptr = (CVI_S8 *)src1->pu8VirAddr[c];
    CVI_S8 *src2_ptr = (CVI_S8 *)src2->pu8VirAddr[c];
    CVI_U8 *alpha_ptr = aplha->pu8VirAddr[c];
    CVI_S8 *dst_ptr = (CVI_S8 *)dst->pu8VirAddr[c];
    for (size_t i = 0; i < src1->u32Width * src1->u32Height; i++) {
      CVI_S16 mul1 = src1_ptr[i] * alpha_ptr[i];

      CVI_S16 mul2 = src2_ptr[i] * (255 - alpha_ptr[i]);
      // if(i > 1000)continue;
      float retf = (mul1 + mul2) / 255.0;

      printf("srcf:%f\n", retf);
      CVI_S16 src_val = retf;
      // if(retf>0)
      //   src_val = retf + 0.5;
      // else{
      //   src_val = retf - 0.5;
      // }
      float diff = retf - dst_ptr[i];
      if (diff < 0) diff = -diff;
      // CVI_S8 ret8 = ret &0xFF;
      if (diff > 0.5) {
        printf("fail,[%zu] s8_a:%d,s8_b:%d,u8_w:%u,TPU %d, CPU %d,srcval:%d,binary:", i,
               src1_ptr[i], src2_ptr[i], alpha_ptr[i], dst_ptr[i], ret, src_val);
        printBits(2, &src_val);
        printf("\n");
        ret = CVI_FAILURE;
        break;
      } else {
        // printf("ok,[%zu] s8_a:%d,s8_b:%d,u8_w:%u,TPU %d, CPU %d,srcval:%d,binary:",
        //       i, src1_ptr[i], src2_ptr[i],alpha_ptr[i], dst_ptr[i],ret,src_val);
        // printBits(2,&src_val);
        // printf("\n");
      }
    }
  }
  return ret;
}
