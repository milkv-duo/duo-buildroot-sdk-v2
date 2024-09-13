#include "cvi_ive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

void print_table(IVE_DST_MEM_INFO_S *dstTbl, uint32_t tblByteSize) {
  int column_size = 64;
  for (CVI_U32 i = 0; i < tblByteSize / column_size; i++) {
    int start = i * column_size;
    int end = start + column_size;
    printf("INDEX: ");
    for (CVI_U32 j = start; j < end; j++) {
      printf("%3d ", j);
    }
    printf("\n-------");
    for (CVI_U32 j = start; j < end; j++) {
      printf("----");
    }
    printf("\n");

    printf("VALUE: ");
    for (CVI_U32 j = start; j < end; j++) {
      printf("%3d ", ((CVI_U8 *)dstTbl->pu8VirAddr)[j]);
    }
    printf("\n");
    printf("\n");
  }
}

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#define print_pixel(image, pi, pj)                                                             \
  do {                                                                                         \
    if ((image)->enType == IVE_IMAGE_TYPE_U8C1) {                                              \
      printf("%4d ", ((image)->pu8VirAddr[0][(pi) + (pj) * (image)->u16Stride[0]]));           \
    } else if ((image)->enType == IVE_IMAGE_TYPE_S8C1) {                                       \
      printf("%4d ", ((int8_t *)(image)->pu8VirAddr[0])[(pi) + (pj) * (image)->u16Stride[0]]); \
    } else if ((image)->enType == IVE_IMAGE_TYPE_U16C1) {                                      \
      printf("%4d ",                                                                           \
             ((uint16_t *)(image)->pu8VirAddr[0])[(pi) + (pj) * (image)->u16Stride[0] / 2]);   \
    }                                                                                          \
  } while (0)

void print_image(IVE_IMAGE_S *src1, IVE_IMAGE_S *result, int num_pixels) {
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
      printf(" => ");
    } else {
      printf("    ");
    }

    for (int i = visiable_start_index_x; i < visiable_end_index_x; i++) {
      print_pixel(result, i, j);
    }

    printf("\n");
  }
}

typedef enum {
  TBL_256 = 256,
  TBL_512 = 512,
} NumTableElem;

int run_map(IVE_HANDLE handle, NumTableElem table_elem) {
  // Create src image.
  const CVI_U16 width = 1280;
  const CVI_U16 height = 720;
  IVE_SRC_IMAGE_S src;
  IVE_IMAGE_TYPE_E img_type = table_elem == TBL_256 ? IVE_IMAGE_TYPE_U8C1 : IVE_IMAGE_TYPE_U16C1;
  CVI_IVE_CreateImage(handle, &src, img_type, width, height);

  const CVI_U16 stride = table_elem == TBL_256 ? src.u16Stride[0] : src.u16Stride[0] / 2;

  // Use rand to generate input data.
  CVI_U16 tableSize = table_elem;
  srand(time(NULL));
  for (CVI_U16 i = 0; i < height; i++) {
    for (CVI_U16 j = 0; j < width; j++) {
      if (table_elem == TBL_256) {
        src.pu8VirAddr[0][j + i * stride] = rand() % tableSize;
      } else {
        ((CVI_U16 *)src.pu8VirAddr[0])[j + i * stride] = (uint16_t)(rand() % tableSize);
      }
    }
  }

  // Flush data to DRAM before TPU use.
  CVI_IVE_BufFlush(handle, &src);

  IVE_DST_IMAGE_S dst;
  CVI_IVE_CreateImage(handle, &dst, IVE_IMAGE_TYPE_U8C1, width, height);

  // Generate table for CVI_IVE_Map.
  IVE_DST_MEM_INFO_S dstTbl;
  CVI_U16 dstTblByteSize = tableSize;
  CVI_IVE_CreateMemInfo(handle, &dstTbl, dstTblByteSize);
  for (CVI_U16 i = 0; i < tableSize; i++) {
    ((CVI_U8 *)dstTbl.pu8VirAddr)[i] = (tableSize % 256) - i - 1;
  }

  struct timeval t0, t1;
  gettimeofday(&t0, NULL);
  int ret = CVI_IVE_Map(handle, &src, &dstTbl, &dst, 0);
  if (ret != CVI_SUCCESS) {
    printf("Failed to run CVI_IVE_Map\n");
    goto failed;
  }
  gettimeofday(&t1, NULL);
  unsigned long elapsed = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;

  // Refresh CPU cache before CPU use.
  CVI_IVE_BufRequest(handle, &dst);

  printf("Table Content:\n");
  print_table(&dstTbl, tableSize);

  printf("\n");
  printf("Result: \n");
  print_image(&src, &dst, 10);
  printf("elapsed time: %lu us\n", elapsed);

failed:
  // Free memory, instance
  CVI_SYS_FreeI(handle, &src);
  CVI_SYS_FreeI(handle, &dst);
  CVI_SYS_FreeM(handle, &dstTbl);
  return ret;
}

int main(int argc, char **argv) {
  if (argc != 1) {
    printf("Incorrect loop value. Usage: %s\n", argv[0]);
    return CVI_FAILURE;
  }

  // Create instance
  IVE_HANDLE handle = CVI_IVE_CreateHandle();
  printf("init.\n");

  printf("Lookup 256d table\n");
  printf("-----------------\n\n");
  int ret = run_map(handle, TBL_256);
  if (ret != CVI_SUCCESS) {
    goto failed;
  }
  printf("\n\n");

  printf("Lookup 512d table\n");
  printf("-----------------\n\n");
  ret = run_map(handle, TBL_512);
  if (ret != CVI_SUCCESS) {
    goto failed;
  }

failed:
  CVI_IVE_DestroyHandle(handle);

  return ret;
}
