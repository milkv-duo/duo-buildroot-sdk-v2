#include "bmkernel/bm_kernel.h"

#include "bmkernel/bm1880v2/1880v2_fp_convert.h"
#include "cvi_ive.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#ifdef __ARM_ARCH
#include "arm_neon.h"
#endif

#define HOG_SZ 1

#define CELL_SIZE 8
#define BLOCK_SIZE 2
#define STEP_X 2
#define STEP_Y 2
#define BIN_NUM 9

int cpu_ref(const int channels, IVE_SRC_IMAGE_S *src, IVE_DST_IMAGE_S *dstH, IVE_DST_IMAGE_S *dstV,
            IVE_DST_IMAGE_S *dstMag, IVE_DST_IMAGE_S *dstAng);

int main(int argc, char **argv) {
  if (argc != 3) {
    printf("Incorrect loop value. Usage: %s <file name> <loop in value (1-1000)>\n", argv[0]);
    return CVI_FAILURE;
  }
  const char *filename = argv[1];
  size_t total_run = atoi(argv[2]);
  printf("Loop value: %zu\n", total_run);
  if (total_run > 1000 || total_run == 0) {
    printf("Incorrect loop value. Usage: %s <file name> <loop in value (1-1000)>\n", argv[0]);
    return CVI_FAILURE;
  }
  // Create instance
  IVE_HANDLE handle = CVI_IVE_CreateHandle();
  printf("BM Kernel init.\n");

  // Fetch image information
  IVE_IMAGE_S src = CVI_IVE_ReadImage(handle, filename, IVE_IMAGE_TYPE_U8C1);
  int nChannels = 1;
  int width = src.u32Width;
  int height = src.u32Height;

  IVE_DST_IMAGE_S dstH, dstV;
  CVI_IVE_CreateImage(handle, &dstV, IVE_IMAGE_TYPE_BF16C1, width, height);
  CVI_IVE_CreateImage(handle, &dstH, IVE_IMAGE_TYPE_BF16C1, width, height);

  IVE_DST_IMAGE_S dstH_u8, dstV_u8;
  CVI_IVE_CreateImage(handle, &dstH_u8, IVE_IMAGE_TYPE_U8C1, width, height);
  CVI_IVE_CreateImage(handle, &dstV_u8, IVE_IMAGE_TYPE_U8C1, width, height);

  IVE_DST_IMAGE_S dstMag, dstAng;
  CVI_IVE_CreateImage(handle, &dstMag, IVE_IMAGE_TYPE_BF16C1, width, height);
  CVI_IVE_CreateImage(handle, &dstAng, IVE_IMAGE_TYPE_BF16C1, width, height);

  IVE_DST_IMAGE_S dstAng_u8;
  CVI_IVE_CreateImage(handle, &dstAng_u8, IVE_IMAGE_TYPE_U8C1, width, height);

  IVE_DST_MEM_INFO_S dstHist;
  CVI_U32 dstHistByteSize = 0;
  CVI_IVE_GET_HOG_SIZE(dstAng.u32Width, dstAng.u32Height, BIN_NUM, CELL_SIZE, BLOCK_SIZE, STEP_X,
                       STEP_Y, &dstHistByteSize);
  CVI_IVE_CreateMemInfo(handle, &dstHist, dstHistByteSize);

  printf("Run TPU HOG.\n");
  IVE_HOG_CTRL_S pstHogCtrl;
  pstHogCtrl.u8BinSize = BIN_NUM;
  pstHogCtrl.u32CellSize = CELL_SIZE;
  pstHogCtrl.u16BlkSizeInCell = BLOCK_SIZE;
  pstHogCtrl.u16BlkStepX = STEP_X;
  pstHogCtrl.u16BlkStepY = STEP_Y;
  struct timeval t0, t1;
  gettimeofday(&t0, NULL);
  for (size_t i = 0; i < total_run; i++) {
    CVI_IVE_HOG(handle, &src, &dstH, &dstV, &dstMag, &dstAng, &dstHist, &pstHogCtrl, 0);
  }
  gettimeofday(&t1, NULL);
  unsigned long elapsed_tpu =
      ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec) / total_run;

  printf("Normalize result to 0-255.\n");
  IVE_ITC_CRTL_S iveItcCtrl;
  iveItcCtrl.enType = IVE_ITC_NORMALIZE;
  CVI_IVE_ImageTypeConvert(handle, &dstV, &dstV_u8, &iveItcCtrl, 0);
  CVI_IVE_ImageTypeConvert(handle, &dstH, &dstH_u8, &iveItcCtrl, 0);
  CVI_IVE_ImageTypeConvert(handle, &dstAng, &dstAng_u8, &iveItcCtrl, 0);

  CVI_IVE_BufRequest(handle, &src);
  CVI_IVE_BufRequest(handle, &dstH);
  CVI_IVE_BufRequest(handle, &dstV);
  CVI_IVE_BufRequest(handle, &dstAng);
  int ret = cpu_ref(nChannels, &src, &dstH, &dstV, &dstMag, &dstAng);
  if (total_run == 1) {
    // write result to disk
    printf("Save to image.\n");
    CVI_IVE_WriteImage(handle, "test_sobelV_c.png", &dstV_u8);
    CVI_IVE_WriteImage(handle, "test_sobelH_c.png", &dstH_u8);
    CVI_IVE_WriteImage(handle, "test_ang_c.png", &dstAng_u8);
    printf("Output HOG feature.\n");
    uint32_t blkSize = BLOCK_SIZE * BLOCK_SIZE * BIN_NUM;
    uint32_t blkNum = dstHistByteSize / sizeof(float) / blkSize;
    for (size_t i = 0; i < blkNum; i++) {
      printf("\n");
      for (size_t j = 0; j < blkSize; j++) {
        printf("%.3f ", ((float *)dstHist.pu8VirAddr)[i * blkSize + j]);
      }
    }
    printf("\n");
  } else {
    printf("OOO %-10s %10lu %10s %10s\n", "HOG", elapsed_tpu, "NA", "NA");
  }

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src);
  CVI_SYS_FreeI(handle, &dstH);
  CVI_SYS_FreeI(handle, &dstH_u8);
  CVI_SYS_FreeI(handle, &dstV);
  CVI_SYS_FreeI(handle, &dstV_u8);
  CVI_SYS_FreeI(handle, &dstMag);
  CVI_SYS_FreeI(handle, &dstAng);
  CVI_SYS_FreeI(handle, &dstAng_u8);
  CVI_SYS_FreeM(handle, &dstHist);
  CVI_IVE_DestroyHandle(handle);

  return ret;
}

int cpu_ref(const int channels, IVE_SRC_IMAGE_S *src, IVE_DST_IMAGE_S *dstH, IVE_DST_IMAGE_S *dstV,
            IVE_DST_IMAGE_S *dstMag, IVE_DST_IMAGE_S *dstAng) {
  int ret = CVI_SUCCESS;
  uint8_t *src_ptr = src->pu8VirAddr[0];
  uint16_t *dstH_ptr = (uint16_t *)dstH->pu8VirAddr[0];
  uint16_t *dstV_ptr = (uint16_t *)dstV->pu8VirAddr[0];
  uint16_t *dstMag_ptr = (uint16_t *)dstMag->pu8VirAddr[0];
  uint16_t *dstAng_ptr = (uint16_t *)dstAng->pu8VirAddr[0];
  float mul_val = 180.f / M_PI;
  float sqrt_epsilon = 1;
  float ang_abs_limit = 1;
  printf("Check Sobel result:\n");
  for (size_t i = 1; i < src->u32Height - 1; i++) {
    for (size_t j = 1; j < src->u32Width - 1; j++) {
      size_t offset = i * src->u16Stride[0] + j;
      size_t offset_t = (i - 1) * src->u16Stride[0] + j;
      size_t offset_b = (i + 1) * src->u16Stride[0] + j;
#if HOG_SZ == 1
      int g_h = (int)src_ptr[offset_b] - src_ptr[offset_t];
      int g_v = (int)src_ptr[offset + 1] - src_ptr[offset - 1];
#else
      int g_h = -(int)src_ptr[offset_t - 1] - (2 * src_ptr[offset_t]) - src_ptr[offset_t + 1] +
                (int)src_ptr[offset_b - 1] + (2 * src_ptr[offset_b]) + src_ptr[offset_b + 1];
      int g_v = -(int)src_ptr[offset_t - 1] - (2 * src_ptr[offset - 1]) - src_ptr[offset_b - 1] +
                (int)src_ptr[offset_t + 1] + (2 * src_ptr[offset + 1]) + src_ptr[offset_b + 1];
#endif
      float dstH_f = convert_bf16_fp32(dstH_ptr[offset]);
      float dstV_f = convert_bf16_fp32(dstV_ptr[offset]);
      if ((int)dstH_f != g_h) {
#if HOG_SZ == 1
        printf("H (%zu, %zu) %u - %u = TPU %f CPU %d\n", j, i, src_ptr[offset + 1],
               src_ptr[offset - 1], dstH_f, g_h);
#else
        printf("H (%zu, %zu) TPU %f CPU %d\n", j, i, dstH_f, g_h);
#endif
        ret = CVI_FAILURE;
      }
      if ((int)dstV_f != g_v) {
#if HOG_SZ == 1
        printf("V (%zu, %zu) %u - %u = TPU %f CPU %d\n", j, i, src_ptr[offset_b], src_ptr[offset_t],
               dstV_f, g_v);
#else
        printf("V (%zu, %zu) TPU %f CPU %d\n", j, i, dstV_f, g_v);
#endif
        ret = CVI_FAILURE;
      }
    }
  }
  printf("Check Mag:\n");
  for (size_t i = 0; i < channels * src->u32Width * src->u32Height; i++) {
    float dstH_f = convert_bf16_fp32(dstH_ptr[i]);
    float dstV_f = convert_bf16_fp32(dstV_ptr[i]);
    float dstMag_f = convert_bf16_fp32(dstMag_ptr[i]);
    float sqrt_res = sqrtf(dstV_f * dstV_f + dstH_f * dstH_f);
    float error = fabs(sqrt_res - dstMag_f);
    if (error > sqrt_epsilon) {
      printf("[%zu] sqrt( %f^2 + %f^2) = TPU %f, CPU %f. eplison = %f\n", i, dstV_f, dstH_f,
             dstMag_f, sqrt_res, error);
      ret = CVI_FAILURE;
    }
  }
  printf("Check Ang:\n");
  for (size_t i = 0; i < channels * src->u32Width * src->u32Height; i++) {
    float dstH_f = convert_bf16_fp32(dstH_ptr[i]);
    float dstV_f = convert_bf16_fp32(dstV_ptr[i]);
    float dstAng_f = convert_bf16_fp32(dstAng_ptr[i]);
    float atan2_res = (float)atan2(dstV_f, dstH_f) * mul_val;
    float error = fabs(atan2_res - dstAng_f);
    if (error > ang_abs_limit) {
      printf("[%zu] atan2( %f, %f) = TPU %f, CPU %f. eplison = %f\n", i, dstV_f, dstH_f, dstAng_f,
             atan2_res, error);
      ret = CVI_FAILURE;
    }
  }
  return ret;
}