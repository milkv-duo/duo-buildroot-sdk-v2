#include "cvi_ive.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#define print_pixel(image, pi, pj)                                                             \
  do {                                                                                         \
    if ((image)->enType == IVE_IMAGE_TYPE_U8C1) {                                              \
      printf("%4d ", ((image)->pu8VirAddr[0][(pi) + (pj) * (image)->u16Stride[0]]));           \
    } else if ((image)->enType == IVE_IMAGE_TYPE_S8C1) {                                       \
      printf("%4d ", ((int8_t *)(image)->pu8VirAddr[0])[(pi) + (pj) * (image)->u16Stride[0]]); \
    }                                                                                          \
  } while (0)

void print_image(IVE_IMAGE_S *src1, IVE_IMAGE_S *src2, IVE_IMAGE_S *result, int num_pixels) {
  int visiable_start_index_x;
  int visiable_start_index_y;
  if (num_pixels > src1->u32Width || num_pixels > src1->u32Height) {
    num_pixels = MIN(src1->u32Width, src1->u32Height);
    visiable_start_index_x = visiable_start_index_y = 0;
  } else {
    visiable_start_index_x = src1->u32Width / 2;
    if ((visiable_start_index_x + num_pixels) > src1->u32Width) {
      visiable_start_index_x = 0;
    } else {
      visiable_start_index_x = visiable_start_index_x - num_pixels;
    }

    visiable_start_index_y = src1->u32Height / 2;
    if ((visiable_start_index_y + num_pixels) > src1->u32Height) {
      visiable_start_index_y = 0;
    } else {
      visiable_start_index_y = visiable_start_index_y - num_pixels;
    }
  }

  int visiable_end_index_x = visiable_start_index_x + num_pixels;
  int visiable_end_index_y = visiable_start_index_y + num_pixels;

  for (int j = visiable_start_index_y; j < visiable_end_index_y; j++) {
    for (int i = visiable_start_index_x; i < visiable_end_index_x; i++) {
      print_pixel(src1, i, j);
    }

    bool middle_of_height = j == (visiable_end_index_y + visiable_start_index_y) / 2;
    if (middle_of_height) {
      printf(" + ");
    } else {
      printf("   ");
    }

    for (int i = visiable_start_index_x; i < visiable_end_index_x; i++) {
      print_pixel(src2, i, j);
    }

    if (middle_of_height) {
      printf(" = ");
    } else {
      printf("   ");
    }

    for (int i = visiable_start_index_x; i < visiable_end_index_x; i++) {
      print_pixel(result, i, j);
    }

    printf("\n");
  }
}

typedef void (*ImageGenerator)(IVE_IMAGE_S *src1, IVE_IMAGE_S *src2);

typedef struct {
  ImageGenerator generator;
  IVE_IMAGE_TYPE_E src2_type;
} FuncParam;

void SrcGenerator(IVE_IMAGE_S *src1, IVE_IMAGE_S *src2) {
  int nChannels = 1;
  int strideWidth = src1->u16Stride[0];
  int height = src1->u32Height;
  CVI_U32 imgSize = nChannels * strideWidth * height;
  for (CVI_U32 i = 0; i < imgSize; i++) {
    src2->pu8VirAddr[0][i] = rand() % 256;
  }
}

int RunAdd(IVE_HANDLE handle, IVE_IMAGE_S *src1, int visible_pixel, FuncParam param) {
  int height = src1->u32Height;
  int width = src1->u32Width;

  // Create second image to add with src1.
  IVE_SRC_IMAGE_S src2;
  CVI_IVE_CreateImage(handle, &src2, param.src2_type, width, height);

  param.generator(src1, &src2);

  // Flush to DRAM before IVE function.
  CVI_IVE_BufFlush(handle, &src2);

  // Create dst image.
  IVE_DST_IMAGE_S dst;
  CVI_IVE_CreateImage(handle, &dst, IVE_IMAGE_TYPE_U8C1, width, height);

  // Setup control parameter.
  IVE_ADD_CTRL_S iveAddCtrl;
  iveAddCtrl.aX = 1.f;
  iveAddCtrl.bY = 1.f;

  // Run IVE Add.
  struct timeval t0, t1;
  gettimeofday(&t0, NULL);
  int ret = CVI_IVE_Add(handle, src1, &src2, &dst, &iveAddCtrl, 0);
  gettimeofday(&t1, NULL);

  unsigned long elapsed = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;

  if (ret != CVI_SUCCESS) {
    printf("Failed when run CVI_IVE_Add ret: %x\n", ret);
    goto failed;
  }

  // Refresh CPU cache before CPU use.
  CVI_IVE_BufRequest(handle, &dst);
  print_image(src1, &src2, &dst, visible_pixel);
  printf("elapsed time: %lu us\n", elapsed);

failed:
  CVI_SYS_FreeI(handle, &src2);
  CVI_SYS_FreeI(handle, &dst);
  return ret;
}

int main(int argc, char **argv) {
  if (argc != 4) {
    printf("Incorrect loop value. Usage: %s <w> <h> <file_name>\n", argv[0]);
    printf("Example: %s 352 288 data/00_352x288_y.yuv\n", argv[0]);
    return CVI_FAILURE;
  }
  // 00_352x288_y.yuv
  int input_w, input_h;

  input_w = atoi(argv[1]);
  input_h = atoi(argv[2]);

  // Create instance
  printf("Create instance.\n");
  IVE_HANDLE handle = CVI_IVE_CreateHandle();

  // Read image from file. CVI_IVE_ReadImage will do the flush for you.
  const char *filename = argv[3];

  IVE_IMAGE_S src1;

  src1.u32Width = input_w;
  src1.u32Height = input_h;
  CVI_IVE_ReadRawImage(handle, &src1, (char *)filename, IVE_IMAGE_TYPE_U8C1, input_w, input_h);

  srand(time(NULL));

  printf("Run Add (U8 + U8 => U8):\n");
  printf("---------------------------------\n");

  FuncParam param = {
      .generator = SrcGenerator,
      .src2_type = IVE_IMAGE_TYPE_U8C1,
  };

  CVI_S32 ret = RunAdd(handle, &src1, 10, param);

  printf("\n\n");
  if (ret != CVI_SUCCESS) {
    goto failed;
  }

  printf("Run Add (U8 + S8 => U8):\n");
  printf("---------------------------------\n");

  param.generator = SrcGenerator;
  param.src2_type = IVE_IMAGE_TYPE_S8C1;

  ret = RunAdd(handle, &src1, 10, param);
  printf("\n\n");
  if (ret != CVI_SUCCESS) {
    goto failed;
  }

failed:
  CVI_SYS_FreeI(handle, &src1);
  CVI_IVE_DestroyHandle(handle);

  return ret;
}
