#include "ive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#ifdef __ARM_ARCH
#include "arm_neon.h"
#endif

int cpu_ref_check(IVE_SRC_IMAGE_S* src_u16, IVE_DST_IMAGE_S* dst_ive, IVE_DST_IMAGE_S* dst_cpu,
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
  int width = src.u16Width;
  int height = src.u16Height;
  size_t img_data_sz = nChannels * src.u16Stride[0] * height;
  printf("Image size is %d X %d, channel %d\n", width, height, nChannels);
  IVE_SRC_IMAGE_S src_u16;
  CVI_IVE_CreateImage(handle, &src_u16, IVE_IMAGE_TYPE_U16C1, width, height);
  for (size_t i = 0; i < img_data_sz; i++) {
    ((CVI_U16*)src_u16.pu8VirAddr[0])[i] = src.pu8VirAddr[0][i];
  }
  CVI_IVE_BufFlush(handle, &src_u16);

  IVE_DST_IMAGE_S dst_mmm, dst_mom;
  CVI_IVE_CreateImage(handle, &dst_mmm, IVE_IMAGE_TYPE_U8C1, width, height);
  CVI_IVE_CreateImage(handle, &dst_mom, IVE_IMAGE_TYPE_U8C1, width, height);

  IVE_DST_IMAGE_S dst_cpu_mmm, dst_cpu_mom;
  CVI_IVE_CreateImage(handle, &dst_cpu_mmm, IVE_IMAGE_TYPE_U8C1, width, height);
  CVI_IVE_CreateImage(handle, &dst_cpu_mom, IVE_IMAGE_TYPE_U8C1, width, height);

  printf("Run TPU Threshold U16.\n");
  IVE_THRESH_U16_CTRL_S iveThreshU16Ctrl;
  iveThreshU16Ctrl.u16LowThr = 30;
  iveThreshU16Ctrl.u16HighThr = 170;
  iveThreshU16Ctrl.u8MinVal = 0;
  iveThreshU16Ctrl.u8MidVal = 125;
  iveThreshU16Ctrl.u8MaxVal = 255;
  iveThreshU16Ctrl.enMode = IVE_THRESH_U16_MODE_U16_TO_U8_MIN_MID_MAX;
  struct timeval t0, t1;
  gettimeofday(&t0, NULL);
  for (size_t i = 0; i < total_run; i++) {
    CVI_IVE_Thresh_U16(handle, &src_u16, &dst_mmm, &iveThreshU16Ctrl, 0);
  }
  gettimeofday(&t1, NULL);
#ifdef __ARM_ARCH
  unsigned long elapsed_neon =
      ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec) / total_run;
#endif

  gettimeofday(&t0, NULL);
  {
    CVI_U16* ptr1 = (CVI_U16*)src_u16.pu8VirAddr[0];
    CVI_U8* ptr2 = dst_cpu_mmm.pu8VirAddr[0];
    for (size_t i = 0; i < img_data_sz; i++) {
      CVI_U16 tmp = ptr1[i] >= iveThreshU16Ctrl.u16LowThr ? iveThreshU16Ctrl.u8MidVal
                                                          : iveThreshU16Ctrl.u8MinVal;
      tmp = ptr1[i] >= iveThreshU16Ctrl.u16HighThr ? iveThreshU16Ctrl.u8MaxVal : tmp;
      if (tmp > 255) tmp = 255;
      if (tmp < 0) tmp = 0;
      ptr2[i] = tmp;
    }
  }
  gettimeofday(&t1, NULL);
#ifdef __ARM_ARCH
  unsigned long elapsed_cpu = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;
#endif

  // Verify MOM mode.
  iveThreshU16Ctrl.enMode = IVE_THRESH_U16_MODE_U16_TO_U8_MIN_ORI_MAX;
  CVI_IVE_Thresh_U16(handle, &src_u16, &dst_mom, &iveThreshU16Ctrl, 0);
  {
    CVI_S16* ptr1 = (CVI_S16*)src_u16.pu8VirAddr[0];
    CVI_U8* ptr2 = dst_cpu_mom.pu8VirAddr[0];
    for (size_t i = 0; i < img_data_sz; i++) {
      CVI_U16 tmp = ptr1[i] >= iveThreshU16Ctrl.u16LowThr ? ptr1[i] : iveThreshU16Ctrl.u8MinVal;
      tmp = ptr1[i] >= iveThreshU16Ctrl.u16HighThr ? iveThreshU16Ctrl.u8MaxVal : tmp;
      if (tmp > 255) tmp = 255;
      if (tmp < 0) tmp = 0;
      ptr2[i] = tmp;
    }
  }
  CVI_IVE_BufRequest(handle, &dst_mmm);
  CVI_IVE_BufRequest(handle, &dst_mom);
  int ret = CVI_SUCCESS;
  {
    int ret2 = cpu_ref_check(&src_u16, &dst_mmm, &dst_cpu_mmm, img_data_sz);
    const char* status = ret2 == CVI_SUCCESS ? "Passed" : "Failed";
    printf("Check TPU Threshold U16 to U8 MMM. %s\n", status);
    ret |= ret2;
  }
  {
    int ret2 = cpu_ref_check(&src_u16, &dst_mom, &dst_cpu_mom, img_data_sz);
    const char* status = ret2 == CVI_SUCCESS ? "Passed" : "Failed";
    printf("Check TPU Threshold U16 to U8 MOM. %s\n", status);
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
    CVI_IVE_WriteImage(handle, "test_thresholdu16_c.png", &dst_mmm);
  }
#ifdef __ARM_ARCH
  else {
    printf("OOO %-10s %10s %10lu %10lu\n", "THRESH U16", "NA", elapsed_neon, elapsed_cpu);
  }
#endif

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src);
  CVI_SYS_FreeI(handle, &src_u16);
  CVI_SYS_FreeI(handle, &dst_mmm);
  CVI_SYS_FreeI(handle, &dst_mom);
  CVI_SYS_FreeI(handle, &dst_cpu_mmm);
  CVI_SYS_FreeI(handle, &dst_cpu_mom);
  CVI_IVE_DestroyHandle(handle);

  return ret;
}

int cpu_ref_check(IVE_SRC_IMAGE_S* src_u16, IVE_DST_IMAGE_S* dst_ive, IVE_DST_IMAGE_S* dst_cpu,
                  size_t img_data_sz) {
  CVI_U16* ptr = (CVI_U16*)src_u16->pu8VirAddr[0];
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