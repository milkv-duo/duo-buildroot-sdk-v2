#include "bmkernel/bm_kernel.h"
#include "cvi_ive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#ifdef __ARM_ARCH
#include "arm_neon.h"
#endif

int cpu_ref(const int channels, IVE_SRC_IMAGE_S *src1, IVE_SRC_IMAGE_S *src2, IVE_DST_IMAGE_S *dst);
int cpu_ref_bf16(const int channels, IVE_SRC_IMAGE_S *src1, IVE_SRC_IMAGE_S *src2,
                 IVE_DST_IMAGE_S *dst, float a, float b);

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
  IVE_IMAGE_S src1 = CVI_IVE_ReadImage(handle, file_name, IVE_IMAGE_TYPE_U8C1);
  int nChannels = 1;
  int width = src1.u32Width;
  int height = src1.u32Height;
  uint32_t img_sz = nChannels * width * height;
  printf("Image size is %d X %d\n", width, height);
  IVE_SRC_IMAGE_S src2, src3;
  CVI_IVE_CreateImage(handle, &src3, IVE_IMAGE_TYPE_U8C1, width * 2, height * 2);
  CVI_IVE_CreateImage2(handle, &src2, IVE_IMAGE_TYPE_U8C1, width, height, &src3);
  memset(src2.pu8VirAddr[0], 255, img_sz);
  for (int j = height / 10; j < height * 9 / 10; j++) {
    for (int i = width / 10; i < width * 9 / 10; i++) {
      src2.pu8VirAddr[0][i + j * width] = 0;
    }
  }
  CVI_IVE_BufFlush(handle, &src2);

  IVE_DST_IMAGE_S dst, dst2;
  CVI_IVE_CreateImage(handle, &dst, IVE_IMAGE_TYPE_U8C1, width, height);
  CVI_IVE_CreateImage(handle, &dst2, IVE_IMAGE_TYPE_U8C1, width, height);

  printf("Run TPU Add.\n");
  IVE_ADD_CTRL_S iveAddCtrl;
  iveAddCtrl.aX = 1.f;
  iveAddCtrl.bY = 1.f;
  struct timeval t0, t1;
  gettimeofday(&t0, NULL);
  for (size_t i = 0; i < total_run; i++) {
    CVI_IVE_Add(handle, &src1, &src2, &dst, &iveAddCtrl, 0);
  }
  gettimeofday(&t1, NULL);
  unsigned long elapsed_tpu =
      ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec) / total_run;
  printf("Run TPU BF16 Add.\n");
  iveAddCtrl.aX = 1.f;
  iveAddCtrl.bY = 0.f;
  gettimeofday(&t0, NULL);
  for (size_t i = 0; i < total_run; i++) {
    CVI_IVE_Add(handle, &src1, &src2, &dst2, &iveAddCtrl, 0);
  }
  gettimeofday(&t1, NULL);
  unsigned long elapsed_tpu_bf16 =
      ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec) / total_run;
  CVI_IVE_BufRequest(handle, &src1);
  CVI_IVE_BufRequest(handle, &src2);
  CVI_IVE_BufRequest(handle, &dst);
  CVI_IVE_BufRequest(handle, &dst2);
  int ret = cpu_ref(nChannels, &src1, &src2, &dst);
  ret |= cpu_ref_bf16(nChannels, &src1, &src2, &dst2, iveAddCtrl.aX, iveAddCtrl.bY);

#ifdef __ARM_ARCH
  IVE_DST_IMAGE_S dst_cpu;
  CVI_IVE_CreateImage(handle, &dst_cpu, IVE_IMAGE_TYPE_U8C1, width, height);
  uint8_t *ptr1 = src1.pu8VirAddr[0];
  uint8_t *ptr2 = src2.pu8VirAddr[0];
  uint8_t *ptr3 = dst_cpu.pu8VirAddr[0];
  gettimeofday(&t0, NULL);
  size_t neon_turn = img_sz / 16;
  for (size_t i = 0; i < neon_turn; i++) {
    uint8x16_t val = vld1q_u8(ptr1);
    uint8x16_t val2 = vld1q_u8(ptr2);
    uint8x16_t result = vqaddq_u8(val, val2);
    vst1q_u8(ptr3, result);
    ptr1 += 16;
    ptr2 += 16;
    ptr3 += 16;
  }
  size_t neon_left = neon_turn * 16;
  ptr1 = src1.pu8VirAddr[0];
  ptr2 = src2.pu8VirAddr[0];
  ptr3 = dst_cpu.pu8VirAddr[0];
  for (size_t i = neon_left; i < img_sz; i++) {
    int res = ptr1[i] + ptr2[i];
    res = res > 255 ? 255 : res;
    ptr3[i] = res;
  }
  gettimeofday(&t1, NULL);
  unsigned long elapsed_neon = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;
  ptr1 = src1.pu8VirAddr[0];
  ptr2 = src2.pu8VirAddr[0];
  ptr3 = dst_cpu.pu8VirAddr[0];
  gettimeofday(&t0, NULL);
  for (size_t i = 0; i < img_sz; i++) {
    int res = (int)ptr1[i] + ptr2[i];
    ptr3[i] = res > 255 ? 255 : res;
  }
  gettimeofday(&t1, NULL);
  unsigned long elapsed_cpu = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;
  unsigned char *ptr4 = malloc(width * height * sizeof(unsigned char));
  gettimeofday(&t0, NULL);
  for (size_t i = 0; i < img_sz; i++) {
    int res = (int)ptr1[i] + ptr2[i];
    ptr4[i] = res > 255 ? 255 : res;
  }
  gettimeofday(&t1, NULL);
  unsigned long elapsed_cpu_malloc = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;
  printf("ptr3 addr %p, ptr4 addr %p\n", ptr3, ptr4);
  printf("Input bm devie memory. Output 1. bm device memory %lu, 2. cpu malloc %lu\n", elapsed_cpu,
         elapsed_cpu_malloc);
  free(ptr4);
  CVI_SYS_FreeI(handle, &dst_cpu);
#endif
  if (total_run == 1) {
    printf("TPU avg time %lu\n", elapsed_tpu);
    printf("TPU BF16 avg time %lu\n", elapsed_tpu_bf16);
#ifdef __ARM_ARCH
    printf("CPU NEON time %lu\n", elapsed_neon);
    printf("CPU time %lu\n", elapsed_cpu);
#endif
    // write result to disk
    printf("Save to image.\n");
    CVI_IVE_WriteImage(handle, "test_cvi_c.png", &dst);
    CVI_IVE_WriteImage(handle, "test_cviBf16_c.png", &dst2);
  }
#ifdef __ARM_ARCH
  else {
    printf("OOO %-10s %10lu %10lu %10lu\n", "ADD", elapsed_tpu, elapsed_neon, elapsed_cpu);
    printf("OOO %-10s %10lu %10s %10s\n", "ADD BF16", elapsed_tpu, "NA", "NA");
  }
#endif

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src1);
  CVI_SYS_FreeI(handle, &src3);
  CVI_SYS_FreeI(handle, &dst);
  CVI_SYS_FreeI(handle, &dst2);
  CVI_IVE_DestroyHandle(handle);

  return ret;
}

int cpu_ref(const int channels, IVE_SRC_IMAGE_S *src1, IVE_SRC_IMAGE_S *src2,
            IVE_DST_IMAGE_S *dst) {
  int ret = CVI_SUCCESS;
  CVI_U8 *src1_ptr = src1->pu8VirAddr[0];
  CVI_U8 *src2_ptr = src2->pu8VirAddr[0];
  CVI_U8 *dst_ptr = dst->pu8VirAddr[0];
  for (size_t i = 0; i < channels * src1->u32Width * src1->u32Height; i++) {
    int res = src1_ptr[i] + src2_ptr[i];
    res = res > 255 ? 255 : res;
    if (res != dst_ptr[i]) {
      printf("[%zu] %d + %d = TPU %d, CPU %d\n", i, src1_ptr[i], src2_ptr[i], dst_ptr[i], (int)res);
      ret = CVI_FAILURE;
      break;
    }
  }
  return ret;
}

int cpu_ref_bf16(const int channels, IVE_SRC_IMAGE_S *src1, IVE_SRC_IMAGE_S *src2,
                 IVE_DST_IMAGE_S *dst, float a, float b) {
  int ret = CVI_SUCCESS;
  CVI_U8 *src1_ptr = src1->pu8VirAddr[0];
  CVI_U8 *src2_ptr = src2->pu8VirAddr[0];
  CVI_U8 *dst_ptr = dst->pu8VirAddr[0];
  for (size_t i = 0; i < channels * src1->u32Width * src1->u32Height; i++) {
    int res = a * src1_ptr[i] + b * src2_ptr[i];
    res = res > 255 ? 255 : res;
    if (res != dst_ptr[i]) {
      printf("[%zu] %d + %d = TPU %d, CPU %d\n", i, src1_ptr[i], src2_ptr[i], dst_ptr[i], (int)res);
      ret = CVI_FAILURE;
      break;
    }
  }
  return ret;
}