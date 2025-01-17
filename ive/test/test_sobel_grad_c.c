#include "bmkernel/bm_kernel.h"

#include "bmkernel/bm1880v2/1880v2_fp_convert.h"
#include "ive.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#ifdef __ARM_ARCH
#include "arm_neon.h"
#endif

int cpu_ref(const int channels, IVE_SRC_IMAGE_S *src, IVE_DST_IMAGE_S *dstH, IVE_DST_IMAGE_S *dstV,
            IVE_DST_IMAGE_S *dstMagL1, IVE_DST_IMAGE_S *dstMagL2, IVE_DST_IMAGE_S *dstAng);

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

  IVE_DST_IMAGE_S dstH, dstV;
  CVI_IVE_CreateImage(handle, &dstV, IVE_IMAGE_TYPE_BF16C1, width, height);
  CVI_IVE_CreateImage(handle, &dstH, IVE_IMAGE_TYPE_BF16C1, width, height);

  IVE_DST_IMAGE_S dstH_u8, dstV_u8;
  CVI_IVE_CreateImage(handle, &dstH_u8, IVE_IMAGE_TYPE_U8C1, width, height);
  CVI_IVE_CreateImage(handle, &dstV_u8, IVE_IMAGE_TYPE_U8C1, width, height);

  IVE_DST_IMAGE_S dstMagL1, dstMagL2, dstAng;
  CVI_IVE_CreateImage(handle, &dstMagL1, IVE_IMAGE_TYPE_BF16C1, width, height);
  CVI_IVE_CreateImage(handle, &dstMagL2, IVE_IMAGE_TYPE_BF16C1, width, height);
  CVI_IVE_CreateImage(handle, &dstAng, IVE_IMAGE_TYPE_BF16C1, width, height);

  IVE_DST_IMAGE_S dstMagL1_u8, dstMagL2_u8, dstAng_u8;
  CVI_IVE_CreateImage(handle, &dstMagL1_u8, IVE_IMAGE_TYPE_U8C1, width, height);
  CVI_IVE_CreateImage(handle, &dstMagL2_u8, IVE_IMAGE_TYPE_U8C1, width, height);
  CVI_IVE_CreateImage(handle, &dstAng_u8, IVE_IMAGE_TYPE_U8C1, width, height);

  printf("Run TPU Sobel Grad.\n");
  IVE_SOBEL_CTRL_S iveSblCtrl;
  iveSblCtrl.enOutCtrl = IVE_SOBEL_OUT_CTRL_BOTH;
  iveSblCtrl.u8MaskSize = 3;
  IVE_MAG_AND_ANG_CTRL_S pstMaaCtrl;
  pstMaaCtrl.enOutCtrl = IVE_MAG_AND_ANG_OUT_CTRL_MAG_AND_ANG;
  pstMaaCtrl.enDistCtrl = IVE_MAG_DIST_L2;
  unsigned long total_s = 0;
  unsigned long total_mag = 0;
  struct timeval t0, t1, t2;
  for (size_t i = 0; i < total_run; i++) {
    gettimeofday(&t0, NULL);
    CVI_IVE_Sobel(handle, &src, &dstH, &dstV, &iveSblCtrl, 0);
    gettimeofday(&t1, NULL);
    CVI_IVE_MagAndAng(handle, &dstH, &dstV, &dstMagL2, &dstAng, &pstMaaCtrl, 0);
    gettimeofday(&t2, NULL);
    unsigned long elapsed = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;
    unsigned long elapsed2 = (t2.tv_sec - t1.tv_sec) * 1000000 + t2.tv_usec - t1.tv_usec;
    total_s += elapsed;
    total_mag += elapsed2;
  }
  total_s /= total_run;
  total_mag /= total_run;

  pstMaaCtrl.enOutCtrl = IVE_MAG_AND_ANG_OUT_CTRL_MAG;
  pstMaaCtrl.enDistCtrl = IVE_MAG_DIST_L1;
  CVI_IVE_MagAndAng(handle, &dstH, &dstV, &dstMagL1, NULL, &pstMaaCtrl, 0);

  printf("Normalize result to 0-255.\n");
  IVE_ITC_CRTL_S iveItcCtrl;
  iveItcCtrl.enType = IVE_ITC_NORMALIZE;
  CVI_IVE_ImageTypeConvert(handle, &dstV, &dstV_u8, &iveItcCtrl, 0);
  CVI_IVE_ImageTypeConvert(handle, &dstH, &dstH_u8, &iveItcCtrl, 0);
  CVI_IVE_ImageTypeConvert(handle, &dstMagL1, &dstMagL1_u8, &iveItcCtrl, 0);
  CVI_IVE_ImageTypeConvert(handle, &dstMagL2, &dstMagL2_u8, &iveItcCtrl, 0);
  CVI_IVE_ImageTypeConvert(handle, &dstAng, &dstAng_u8, &iveItcCtrl, 0);

  CVI_IVE_BufRequest(handle, &src);
  CVI_IVE_BufRequest(handle, &dstH);
  CVI_IVE_BufRequest(handle, &dstV);
  CVI_IVE_BufRequest(handle, &dstMagL1);
  CVI_IVE_BufRequest(handle, &dstMagL2);
  CVI_IVE_BufRequest(handle, &dstAng);
  int ret = cpu_ref(nChannels, &src, &dstH, &dstV, &dstMagL1, &dstMagL2, &dstAng);

  if (total_run == 1) {
    printf("TPU Sobel avg time %lu\n", total_s);
    printf("TPU MagAndAng avg time %lu\n", total_mag);
    // write result to disk
    printf("Save to image.\n");
    CVI_IVE_WriteImage(handle, "test_sobelV_c.png", &dstV_u8);
    CVI_IVE_WriteImage(handle, "test_sobelH_c.png", &dstH_u8);
    CVI_IVE_WriteImage(handle, "test_magL1_c.png", &dstMagL1_u8);
    CVI_IVE_WriteImage(handle, "test_magL2_c.png", &dstMagL2_u8);
    CVI_IVE_WriteImage(handle, "test_ang_c.png", &dstAng_u8);
  }
#ifdef __ARM_ARCH
  else {
    printf("OOO %-10s %10lu %10s %10s\n", "SOBEL GRAD", total_s, "NA", "NA");
    printf("OOO %-10s %10lu %10s %10s\n", "MagnAng", total_mag, "NA", "NA");
  }
#endif

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src);
  CVI_SYS_FreeI(handle, &dstH);
  CVI_SYS_FreeI(handle, &dstH_u8);
  CVI_SYS_FreeI(handle, &dstV);
  CVI_SYS_FreeI(handle, &dstV_u8);
  CVI_SYS_FreeI(handle, &dstMagL1);
  CVI_SYS_FreeI(handle, &dstMagL1_u8);
  CVI_SYS_FreeI(handle, &dstMagL2);
  CVI_SYS_FreeI(handle, &dstMagL2_u8);
  CVI_SYS_FreeI(handle, &dstAng);
  CVI_SYS_FreeI(handle, &dstAng_u8);
  CVI_IVE_DestroyHandle(handle);

  return ret;
}

int cpu_ref(const int channels, IVE_SRC_IMAGE_S *src, IVE_DST_IMAGE_S *dstH, IVE_DST_IMAGE_S *dstV,
            IVE_DST_IMAGE_S *dstMagL1, IVE_DST_IMAGE_S *dstMagL2, IVE_DST_IMAGE_S *dstAng) {
  int ret = CVI_SUCCESS;
  uint16_t *dstH_ptr = (uint16_t *)dstH->pu8VirAddr[0];
  uint16_t *dstV_ptr = (uint16_t *)dstV->pu8VirAddr[0];
  uint16_t *dstMagL1_ptr = (uint16_t *)dstMagL1->pu8VirAddr[0];
  uint16_t *dstMagL2_ptr = (uint16_t *)dstMagL2->pu8VirAddr[0];
  uint16_t *dstAng_ptr = (uint16_t *)dstAng->pu8VirAddr[0];
  float mul_val = 180.f / M_PI;
  float abs_epsilon = 1;
  float sqrt_epsilon = 2;
  float ang_abs_limit = 1;
  printf("Check Mag Abs:\n");
  for (size_t i = 0; i < channels * src->u16Height; i++) {
    for (size_t j = 0; j < src->u16Width; j++) {
      size_t idx = j + i * src->u16Stride[0];
      float dstH_f = convert_bf16_fp32(dstH_ptr[idx]);
      float dstV_f = convert_bf16_fp32(dstV_ptr[idx]);
      float dstMag_f = convert_bf16_fp32(dstMagL1_ptr[idx]);
      float abs_res = fabs(dstV_f) + fabs(dstH_f);
      float error = fabs(abs_res - dstMag_f);
      if (error > abs_epsilon) {
        printf("[%zu] (|%f| + |%f|) / 2 = TPU %f, CPU %f. eplison = %f\n", idx, dstV_f, dstH_f,
               dstMag_f, abs_res, error);
        ret = CVI_FAILURE;
      }
    }
  }
  printf("Check Mag Sqrt:\n");
  for (size_t i = 0; i < channels * src->u16Height; i++) {
    for (size_t j = 0; j < src->u16Width; j++) {
      size_t idx = j + i * src->u16Stride[0];
      float dstH_f = convert_bf16_fp32(dstH_ptr[idx]);
      float dstV_f = convert_bf16_fp32(dstV_ptr[idx]);
      float dstMag_f = convert_bf16_fp32(dstMagL2_ptr[idx]);
      float sqrt_res = sqrtf(dstV_f * dstV_f + dstH_f * dstH_f);
      float error = fabs(sqrt_res - dstMag_f);
      if (error > sqrt_epsilon) {
        printf("[%zu] sqrt( %f^2 + %f^2) = TPU %f, CPU %f. eplison = %f\n", idx, dstV_f, dstH_f,
               dstMag_f, sqrt_res, error);
        ret = CVI_FAILURE;
      }
    }
  }

  printf("Check Ang:\n");
  for (size_t i = 0; i < channels * src->u16Height; i++) {
    for (size_t j = 0; j < src->u16Width; j++) {
      size_t idx = j + i * src->u16Stride[0];
      float dstH_f = convert_bf16_fp32(dstH_ptr[idx]);
      float dstV_f = convert_bf16_fp32(dstV_ptr[idx]);
      float dstAng_f = convert_bf16_fp32(dstAng_ptr[idx]);
      float atan2_res = (float)atan2(dstV_f, dstH_f) * mul_val;
      float error = fabs(atan2_res - dstAng_f);
      if (error > ang_abs_limit) {
        printf("[%zu] atan2( %f, %f) = TPU %f, CPU %f. eplison = %f\n", idx, dstV_f, dstH_f,
               dstAng_f, atan2_res, error);
        ret = CVI_FAILURE;
      }
    }
  }
  return ret;
}