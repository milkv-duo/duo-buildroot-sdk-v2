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
            IVE_DST_IMAGE_S *dstAng);

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
  int ret = CVI_SUCCESS;
  printf("BM Kernel init.\n");

  // Fetch image information
  IVE_IMAGE_S src = CVI_IVE_ReadImage(handle, filename, IVE_IMAGE_TYPE_U8C1);
  int nChannels = 1;
  int width = src.u16Width;
  int height = src.u16Height;
  printf("Image size is %d X %d, channel %d\n", width, height, nChannels);

  IVE_DST_MEM_INFO_S dstInteg;
  CVI_U32 dstIntegSize = (width + 1) * (height + 1) * sizeof(uint32_t);
  CVI_IVE_CreateMemInfo(handle, &dstInteg, dstIntegSize);
  dstIntegSize = (width + 1) * (height + 1);

  printf("Run CPU Integral Image.\n");
  IVE_INTEG_CTRL_S pstIntegCtrl;

  pstIntegCtrl.enOutCtrl = IVE_INTEG_OUT_CTRL_SUM;

  struct timeval t0, t1;
  gettimeofday(&t0, NULL);
  for (size_t i = 0; i < total_run; i++) {
    CVI_IVE_Integ(handle, &src, &dstInteg, &pstIntegCtrl, 0);
  }
  gettimeofday(&t1, NULL);
  unsigned long elapsed_cpu =
      ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec) / total_run;

  CVI_IVE_BufRequest(handle, &src);
  if (total_run == 1) {
    printf("CPU time %lu\n", elapsed_cpu);
    // write result to disk
    printf("Output Integral Image.\n");
    for (size_t j = 0; j < (width); j++) {
      printf("%3d ", ((uint32_t *)dstInteg.pu8VirAddr)[width + j]);
    }
    printf("\n");
  }

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src);
  CVI_SYS_FreeM(handle, &dstInteg);
  CVI_IVE_DestroyHandle(handle);

  return ret;
}

int cpu_ref(const int channels, IVE_SRC_IMAGE_S *src, IVE_DST_IMAGE_S *dstH, IVE_DST_IMAGE_S *dstV,
            IVE_DST_IMAGE_S *dstAng) {
  int ret = CVI_SUCCESS;
  uint16_t *dstH_ptr = (uint16_t *)dstH->pu8VirAddr[0];
  uint16_t *dstV_ptr = (uint16_t *)dstV->pu8VirAddr[0];
  uint16_t *dstAng_ptr = (uint16_t *)dstAng->pu8VirAddr[0];
  float mul_val = 180.f / M_PI;
  float ang_abs_limit = 1;

  printf("Check Ang:\n");
  for (size_t i = 0; i < channels * src->u16Width * src->u16Height; i++) {
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