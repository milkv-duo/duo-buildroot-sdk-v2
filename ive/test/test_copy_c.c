#include "ive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#ifdef __ARM_ARCH
#include "arm_neon.h"
#endif

int cpu_ref(const int channels, const int x_sz, const int y_sz, IVE_SRC_IMAGE_S *src,
            IVE_DST_IMAGE_S *dst_copy, IVE_DST_IMAGE_S *dst_interval);

int main(int argc, char **argv) {
  if (argc != 3) {
    printf("Incorrect loop value. Usage: %s <file_name> <loop in value (1-1000)>\n", argv[0]);
    return CVI_FAILURE;
  }
  const char *file_name = argv[1];
  size_t total_run = atoi(argv[2]);
  printf("Loop value: %zu\n", total_run);
  if (total_run > 1000 || total_run == 0) {
    printf("Incorrect loop value. Usage: %s <file_name> <loop in value (1-1000)>\n", argv[0]);
    return CVI_FAILURE;
  }
  // Create instance
  IVE_HANDLE handle = CVI_IVE_CreateHandle();
  printf("BM Kernel init.\n");

  // Fetch image information
  IVE_IMAGE_S src = CVI_IVE_ReadImage(handle, file_name, IVE_IMAGE_TYPE_U8C1);
  int nChannels = 1;
  int width = src.u16Width;
  int height = src.u16Height;

  IVE_DST_IMAGE_S dst;
  CVI_IVE_CreateImage(handle, &dst, IVE_IMAGE_TYPE_U8C1, width, height);

  printf("Run TPU Direct Copy.\n");
  IVE_DMA_CTRL_S iveDmaCtrl;
  iveDmaCtrl.enMode = IVE_DMA_MODE_DIRECT_COPY;
  struct timeval t0, t1;
  gettimeofday(&t0, NULL);
  for (size_t i = 0; i < total_run; i++) {
    CVI_IVE_DMA(handle, &src, &dst, &iveDmaCtrl, 0);
  }
  gettimeofday(&t1, NULL);
  unsigned long elapsed_tpu_dcopy =
      ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec) / total_run;

  IVE_DST_IMAGE_S dst2;
  CVI_IVE_CreateImage(handle, &dst2, IVE_IMAGE_TYPE_U8C1, width * 2, height * 2);

  printf("Run TPU Interval Copy.\n");
  iveDmaCtrl.enMode = IVE_DMA_MODE_INTERVAL_COPY;
  iveDmaCtrl.u8HorSegSize = 2;
  iveDmaCtrl.u8VerSegRows = 2;
  gettimeofday(&t0, NULL);
  for (size_t i = 0; i < total_run; i++) {
    CVI_IVE_DMA(handle, &src, &dst2, &iveDmaCtrl, 0);
  }
  gettimeofday(&t1, NULL);
  unsigned long elapsed_tpu_icopy =
      ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec) / total_run;

  IVE_DST_IMAGE_S src_crop;
  CVI_IVE_SubImage(handle, &src, &src_crop, 100, 100, 1380, 820);
  IVE_DST_IMAGE_S dst3;
  CVI_IVE_CreateImage(handle, &dst3, IVE_IMAGE_TYPE_U8C1, src_crop.u16Width, src_crop.u16Height);

  printf("Run TPU Sub Direct Copy.\n");
  iveDmaCtrl.enMode = IVE_DMA_MODE_DIRECT_COPY;
  gettimeofday(&t0, NULL);
  for (size_t i = 0; i < total_run; i++) {
    CVI_IVE_DMA(handle, &src_crop, &dst3, &iveDmaCtrl, 0);
  }
  gettimeofday(&t1, NULL);
  unsigned long elapsed_tpu_scopy =
      ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec) / total_run;

  CVI_IVE_BufRequest(handle, &src);
  CVI_IVE_BufRequest(handle, &dst);
  CVI_IVE_BufRequest(handle, &dst2);
  CVI_IVE_BufRequest(handle, &dst3);
  int ret = cpu_ref(nChannels, iveDmaCtrl.u8HorSegSize, iveDmaCtrl.u8VerSegRows, &src, &dst, &dst2);

  int total_size = nChannels * width * height;
  CVI_U8 *cpu_dst = malloc(sizeof(CVI_U8) * total_size);
  gettimeofday(&t0, NULL);
  memcpy(cpu_dst, src.pu8VirAddr[0], sizeof(CVI_U8) * total_size);
  gettimeofday(&t1, NULL);
  unsigned long elapsed_tpu_dcopy_cpu =
      ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec) / total_run;
  free(cpu_dst);

  if (total_run == 1) {
    // write result to disk
    printf("Save to image.\n");
    CVI_IVE_WriteImage(handle, "test_dcopy_c.png", &dst);
    CVI_IVE_WriteImage(handle, "test_icopy_c.png", &dst2);
    CVI_IVE_WriteImage(handle, "test_scopy_c.png", &dst3);
  } else {
    printf("OOO %-10s %10lu %10s %10lu\n", "Dir Copy", elapsed_tpu_dcopy, "NA",
           elapsed_tpu_dcopy_cpu);
    printf("OOO %-10s %10lu %10s %10s\n", "Int Copy", elapsed_tpu_icopy, "NA", "NA");
    printf("OOO %-10s %10lu %10s %10s\n", "Sub Copy", elapsed_tpu_scopy, "NA", "NA");
  }

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src);
  CVI_SYS_FreeI(handle, &dst);
  CVI_SYS_FreeI(handle, &dst2);
  CVI_SYS_FreeI(handle, &dst3);
  CVI_IVE_DestroyHandle(handle);

  return ret;
}

int cpu_ref(const int channels, const int x_sz, const int y_sz, IVE_SRC_IMAGE_S *src,
            IVE_DST_IMAGE_S *dst_copy, IVE_DST_IMAGE_S *dst_interval) {
  int ret = CVI_SUCCESS;
  for (size_t i = 0; i < channels * src->u16Width * src->u16Height; i++) {
    if (src->pu8VirAddr[0][i] != dst_copy->pu8VirAddr[0][i]) {
      printf("[%zu] original %d copied %d", i, src->pu8VirAddr[0][i], dst_copy->pu8VirAddr[0][i]);
      ret = CVI_FAILURE;
      break;
    }
  }
  for (size_t k = 0; k < channels; k++) {
    for (size_t i = 0; i < 6; i++) {
      for (size_t j = 0; j < dst_interval->u16Width; j++) {
        char src_val = src->pu8VirAddr[0][(j / x_sz) + (i / y_sz) * src->u16Stride[0] +
                                          k * src->u16Stride[0] * src->u16Height];
        char dst_val =
            dst_interval->pu8VirAddr[0][j + i * dst_interval->u16Stride[0] +
                                        k * dst_interval->u16Stride[0] * dst_interval->u16Height];
        if (i % y_sz != 0 || j % x_sz != 0) {
          src_val = 0;
        }
        if (src_val != dst_val) {
          printf("[%zu][%zu][%zu] original %d copied %d\n", k, i, j, src_val, dst_val);
          ret = CVI_FAILURE;
          break;
        }
      }
    }
  }
  return ret;
}