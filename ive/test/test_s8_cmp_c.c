// #include "cvi_sys.h"
#include "cvi_ive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
int cpu_ref(const int channels, IVE_SRC_IMAGE_S *src1, IVE_SRC_IMAGE_S *src2, IVE_DST_IMAGE_S *dst);

// void printBits(size_t const size, void const * const ptr)
// {
//     unsigned char *b = (unsigned char*) ptr;
//     unsigned char byte;
//     int i, j;

//     for (i = size-1; i >= 0; i--) {
//         for (j = 7; j >= 0; j--) {
//             byte = (b[i] >> j) & 1;
//             printf("%u", byte);
//         }
//         printf(" ");
//     }
// }
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
  IVE_IMAGE_S src1 = CVI_IVE_ReadImage(handle, file_name1, img_type);
  IVE_IMAGE_S src2 = CVI_IVE_ReadImage(handle, file_name2, img_type);

  int width = src1.u32Width;
  int height = src1.u32Height;

  int num_channel = 3;
  if (img_type == IVE_IMAGE_TYPE_S8C1) {
    num_channel = 1;
  }

  IVE_DST_IMAGE_S dst;
  CVI_IVE_CreateImage(handle, &dst, IVE_IMAGE_TYPE_U8C1, width, height);

  struct timeval t0, t1;
  gettimeofday(&t0, NULL);
  int ret = CVI_IVE_CMP_S8_BINARY(handle, &src1, &src2, &dst);
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
    if (CVI_SUCCESS == cpu_ref(num_channel, &src1, &src2, &dst)) {
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
            IVE_DST_IMAGE_S *dst) {
  int ret = CVI_SUCCESS;
  for (int c = 0; c < channels; c++) {
    CVI_S8 *src1_ptr = (CVI_S8 *)src1->pu8VirAddr[c];
    CVI_S8 *src2_ptr = (CVI_S8 *)src2->pu8VirAddr[c];
    CVI_U8 *dst_ptr = (CVI_U8 *)dst->pu8VirAddr[c];
    for (size_t i = 0; i < src1->u32Width * src1->u32Height; i++) {
      int ret = abs(src1_ptr[i]);
      int ret2 = abs(src2_ptr[i]);

      if (ret > ret2)
        ret = 255;
      else
        ret = 0;
      CVI_U8 tpu = dst_ptr[i];
      if (ret != tpu) {
        printf("fail,[%zu] s8_a:%d,s8_b:%d,TPU %d, CPU %d\n", i, src1_ptr[i], src2_ptr[i], tpu,
               ret);

        ret = CVI_FAILURE;
        break;
      } else {
        // printf("ok,[%zu] s8_a:%d,s8_b:%d,TPU %d, CPU %d\n",
        //         i, src1_ptr[i], src2_ptr[i], tpu,ret);
      }
    }
  }
  return ret;
}
