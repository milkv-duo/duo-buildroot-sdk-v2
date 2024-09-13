#include "cvi_ive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#ifdef __ARM_ARCH
#include "arm_neon.h"
#endif

int cpu_ref_check_u8(IVE_SRC_IMAGE_S* src_s16, IVE_DST_IMAGE_S* dst_ive, IVE_DST_IMAGE_S* dst_cpu,
                     size_t img_data_sz);

int cpu_ref_check_s8(IVE_SRC_IMAGE_S* src_s16, IVE_DST_IMAGE_S* dst_ive, IVE_DST_IMAGE_S* dst_cpu,
                     size_t img_data_sz);

int main(int argc, char** argv) {
  if (argc != 3) {
    printf("Incorrect loop value. Usage: %s <file name> <loop in value (1-1000)>\n", argv[0]);
    return CVI_FAILURE;
  }
  const char* filename = argv[1];
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
  size_t img_data_sz = nChannels * src.u16Stride[0] * height;
  printf("Image size is %d X %d, channel %d\n", width, height, nChannels);
  IVE_SRC_IMAGE_S src_s16;
  CVI_IVE_CreateImage(handle, &src_s16, IVE_IMAGE_TYPE_S16C1, width, height);
  for (size_t i = 0; i < img_data_sz; i++) {
    ((CVI_S16*)src_s16.pu8VirAddr[0])[i] = src.pu8VirAddr[0][i];
    if (i == 15) {
      printf("Original %u int16_t %u\n", src.pu8VirAddr[0][i],
             ((CVI_S16*)src_s16.pu8VirAddr[0])[i]);
    }
  }
  CVI_IVE_BufFlush(handle, &src_s16);

  IVE_DST_IMAGE_S dst_mmm_u8, dst_mom_u8;
  CVI_IVE_CreateImage(handle, &dst_mmm_u8, IVE_IMAGE_TYPE_U8C1, width, height);
  CVI_IVE_CreateImage(handle, &dst_mom_u8, IVE_IMAGE_TYPE_U8C1, width, height);
  IVE_DST_IMAGE_S dst_mmm_s8, dst_mom_s8;
  CVI_IVE_CreateImage(handle, &dst_mmm_s8, IVE_IMAGE_TYPE_S8C1, width, height);
  CVI_IVE_CreateImage(handle, &dst_mom_s8, IVE_IMAGE_TYPE_S8C1, width, height);

  IVE_DST_IMAGE_S dst_cpu_mmm_u8, dst_cpu_mom_u8;
  CVI_IVE_CreateImage(handle, &dst_cpu_mmm_u8, IVE_IMAGE_TYPE_U8C1, width, height);
  CVI_IVE_CreateImage(handle, &dst_cpu_mom_u8, IVE_IMAGE_TYPE_U8C1, width, height);
  IVE_DST_IMAGE_S dst_cpu_mmm_s8, dst_cpu_mom_s8;
  CVI_IVE_CreateImage(handle, &dst_cpu_mmm_s8, IVE_IMAGE_TYPE_S8C1, width, height);
  CVI_IVE_CreateImage(handle, &dst_cpu_mom_s8, IVE_IMAGE_TYPE_S8C1, width, height);

  printf("Run TPU Threshold S16.\n");
  IVE_THRESH_S16_CTRL_S iveThreshS16Ctrl;
  iveThreshS16Ctrl.s16LowThr = 30;
  iveThreshS16Ctrl.s16HighThr = 170;
  iveThreshS16Ctrl.un8MinVal.u8Val = 0;
  iveThreshS16Ctrl.un8MidVal.u8Val = 125;
  iveThreshS16Ctrl.un8MaxVal.u8Val = 255;
  iveThreshS16Ctrl.enMode = IVE_THRESH_S16_MODE_S16_TO_U8_MIN_MID_MAX;
  struct timeval t0, t1;
  gettimeofday(&t0, NULL);
  for (size_t i = 0; i < total_run; i++) {
    CVI_IVE_Thresh_S16(handle, &src_s16, &dst_mmm_u8, &iveThreshS16Ctrl, 0);
  }
  gettimeofday(&t1, NULL);
#ifdef __ARM_ARCH
  unsigned long elapsed_neon =
      ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec) / total_run;
#endif

  gettimeofday(&t0, NULL);
  {
    CVI_S16* ptr1 = (CVI_S16*)src_s16.pu8VirAddr[0];
    CVI_U8* ptr2 = dst_cpu_mmm_u8.pu8VirAddr[0];
    for (size_t i = 0; i < img_data_sz; i++) {
      CVI_S16 tmp = ptr1[i] >= iveThreshS16Ctrl.s16LowThr ? iveThreshS16Ctrl.un8MidVal.u8Val
                                                          : iveThreshS16Ctrl.un8MinVal.u8Val;
      tmp = ptr1[i] >= iveThreshS16Ctrl.s16HighThr ? iveThreshS16Ctrl.un8MaxVal.u8Val : tmp;
      if (tmp > 255) tmp = 255;
      if (tmp < 0) tmp = 0;
      ptr2[i] = tmp;
    }
  }
  gettimeofday(&t1, NULL);
#ifdef __ARM_ARCH
  unsigned long elapsed_cpu = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;
#endif

  // Verify U8 MOM mode.
  iveThreshS16Ctrl.enMode = IVE_THRESH_S16_MODE_S16_TO_U8_MIN_ORI_MAX;
  CVI_IVE_Thresh_S16(handle, &src_s16, &dst_mom_u8, &iveThreshS16Ctrl, 0);
  {
    CVI_S16* ptr1 = (CVI_S16*)src_s16.pu8VirAddr[0];
    CVI_U8* ptr2 = dst_cpu_mom_u8.pu8VirAddr[0];
    for (size_t i = 0; i < img_data_sz; i++) {
      CVI_S16 tmp =
          ptr1[i] >= iveThreshS16Ctrl.s16LowThr ? ptr1[i] : iveThreshS16Ctrl.un8MinVal.u8Val;
      tmp = ptr1[i] >= iveThreshS16Ctrl.s16HighThr ? iveThreshS16Ctrl.un8MaxVal.u8Val : tmp;
      if (tmp > 255) tmp = 255;
      if (tmp < 0) tmp = 0;
      ptr2[i] = tmp;
    }
  }
  // Verify S8 MMM mode
  iveThreshS16Ctrl.s16LowThr = 30;
  iveThreshS16Ctrl.s16HighThr = 170;
  iveThreshS16Ctrl.un8MinVal.s8Val = -100;
  iveThreshS16Ctrl.un8MidVal.s8Val = -1;
  iveThreshS16Ctrl.un8MaxVal.s8Val = 50;

  iveThreshS16Ctrl.enMode = IVE_THRESH_S16_MODE_S16_TO_S8_MIN_MID_MAX;
  CVI_IVE_Thresh_S16(handle, &src_s16, &dst_mmm_s8, &iveThreshS16Ctrl, 0);
  {
    CVI_S16* ptr1 = (CVI_S16*)src_s16.pu8VirAddr[0];
    CVI_S8* ptr2 = (CVI_S8*)dst_cpu_mmm_s8.pu8VirAddr[0];
    for (size_t i = 0; i < img_data_sz; i++) {
      CVI_S16 tmp = ptr1[i] >= iveThreshS16Ctrl.s16LowThr ? iveThreshS16Ctrl.un8MidVal.s8Val
                                                          : iveThreshS16Ctrl.un8MinVal.s8Val;
      tmp = ptr1[i] >= iveThreshS16Ctrl.s16HighThr ? iveThreshS16Ctrl.un8MaxVal.s8Val : tmp;
      if (tmp > 127) tmp = 127;
      if (tmp < -128) tmp = -128;
      ptr2[i] = tmp;
    }
  }
  // Verify S8 MOM mode
  iveThreshS16Ctrl.enMode = IVE_THRESH_S16_MODE_S16_TO_S8_MIN_ORI_MAX;
  CVI_IVE_Thresh_S16(handle, &src_s16, &dst_mom_s8, &iveThreshS16Ctrl, 0);
  {
    CVI_S16* ptr1 = (CVI_S16*)src_s16.pu8VirAddr[0];
    CVI_S8* ptr2 = (CVI_S8*)dst_cpu_mom_s8.pu8VirAddr[0];
    for (size_t i = 0; i < img_data_sz; i++) {
      CVI_S16 tmp =
          ptr1[i] >= iveThreshS16Ctrl.s16LowThr ? ptr1[i] : iveThreshS16Ctrl.un8MinVal.s8Val;
      tmp = ptr1[i] >= iveThreshS16Ctrl.s16HighThr ? iveThreshS16Ctrl.un8MaxVal.s8Val : tmp;
      if (tmp > 127) tmp = 127;
      if (tmp < -128) tmp = -128;
      ptr2[i] = tmp;
    }
  }

  CVI_IVE_BufRequest(handle, &dst_mmm_u8);
  CVI_IVE_BufRequest(handle, &dst_mom_u8);
  CVI_IVE_BufRequest(handle, &dst_mmm_s8);
  CVI_IVE_BufRequest(handle, &dst_mom_s8);
  int ret = CVI_SUCCESS;
  {
    int ret2 = cpu_ref_check_u8(&src_s16, &dst_mmm_u8, &dst_cpu_mmm_u8, img_data_sz);
    const char* status = ret2 == CVI_SUCCESS ? "Passed" : "Failed";
    printf("Check TPU Threshold S16 to U8 MMM. %s\n", status);
    ret |= ret2;
  }
  {
    int ret2 = cpu_ref_check_u8(&src_s16, &dst_mom_u8, &dst_cpu_mom_u8, img_data_sz);
    const char* status = ret2 == CVI_SUCCESS ? "Passed" : "Failed";
    printf("Check TPU Threshold S16 to U8 MOM. %s\n", status);
    ret |= ret2;
  }
  {
    int ret2 = cpu_ref_check_s8(&src_s16, &dst_mmm_s8, &dst_cpu_mmm_s8, img_data_sz);
    const char* status = ret2 == CVI_SUCCESS ? "Passed" : "Failed";
    printf("Check TPU Threshold S16 to S8 MMM. %s\n", status);
    ret |= ret2;
  }
  {
    int ret2 = cpu_ref_check_s8(&src_s16, &dst_mom_s8, &dst_cpu_mom_s8, img_data_sz);
    const char* status = ret2 == CVI_SUCCESS ? "Passed" : "Failed";
    printf("Check TPU Threshold S16 to S8 MOM. %s\n", status);
    ret |= ret2;
  }

  if (total_run == 1) {
    printf("TPU avg time %s\n", "NA");
#ifdef __ARM_ARCH
    printf("CPU NEON time %lu\n", elapsed_neon);
    printf("CPU time %lu\n", elapsed_cpu);
#endif
    // write result to disk
    printf("Save to image.\n");
    CVI_IVE_WriteImage(handle, "test_thresholds16_c.png", &dst_mmm_u8);
  }
#ifdef __ARM_ARCH
  else {
    printf("OOO %-10s %10s %10lu %10lu\n", "THRESH S16", "NA", elapsed_neon, elapsed_cpu);
  }
#endif

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src);
  CVI_SYS_FreeI(handle, &src_s16);
  CVI_SYS_FreeI(handle, &dst_mmm_u8);
  CVI_SYS_FreeI(handle, &dst_mom_u8);
  CVI_SYS_FreeI(handle, &dst_mmm_s8);
  CVI_SYS_FreeI(handle, &dst_mom_s8);
  CVI_SYS_FreeI(handle, &dst_cpu_mmm_u8);
  CVI_SYS_FreeI(handle, &dst_cpu_mom_u8);
  CVI_SYS_FreeI(handle, &dst_cpu_mmm_s8);
  CVI_SYS_FreeI(handle, &dst_cpu_mom_s8);
  CVI_IVE_DestroyHandle(handle);

  return ret;
}

int cpu_ref_check_u8(IVE_SRC_IMAGE_S* src_s16, IVE_DST_IMAGE_S* dst_ive, IVE_DST_IMAGE_S* dst_cpu,
                     size_t img_data_sz) {
  CVI_S16* ptr = (CVI_S16*)src_s16->pu8VirAddr[0];
  CVI_U8* dst_ptr = dst_ive->pu8VirAddr[0];
  CVI_U8* cpu_ptr = dst_cpu->pu8VirAddr[0];
  for (size_t i = 0; i < img_data_sz; i++) {
    if (dst_ptr[i] != cpu_ptr[i]) {
      printf("index %zu val neon %u cpu %u, origin %u\n", i, dst_ptr[i], cpu_ptr[i], ptr[i]);
      return CVI_FAILURE;
    }
  }
  return CVI_SUCCESS;
}

int cpu_ref_check_s8(IVE_SRC_IMAGE_S* src_s16, IVE_DST_IMAGE_S* dst_ive, IVE_DST_IMAGE_S* dst_cpu,
                     size_t img_data_sz) {
  CVI_S16* ptr = (CVI_S16*)src_s16->pu8VirAddr[0];
  CVI_S8* dst_ptr = (CVI_S8*)dst_ive->pu8VirAddr[0];
  CVI_S8* cpu_ptr = (CVI_S8*)dst_cpu->pu8VirAddr[0];
  for (size_t i = 0; i < img_data_sz; i++) {
    if (dst_ptr[i] != cpu_ptr[i]) {
      printf("index %zu val neon %d cpu %d, origin %d\n", i, dst_ptr[i], cpu_ptr[i], ptr[i]);
      return CVI_FAILURE;
    }
  }
  return CVI_SUCCESS;
}