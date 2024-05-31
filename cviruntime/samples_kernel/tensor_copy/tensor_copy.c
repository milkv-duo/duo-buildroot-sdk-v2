#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include "cvikernel/cvikernel.h"
#include "cviruntime_context.h"

static void test_tdma_g2l(
    CVI_RT_HANDLE rt_handle,
    cvk_context_t *cvk_ctx,
    const cvk_tl_t *tl,
    uint64_t phy_addr,
    int nc_transpose)
{
  cvk_tg_shape_t s;
  if (nc_transpose) {
    s.n = tl->shape.c;
    s.c = tl->shape.n;
  } else {
    s.n = tl->shape.n;
    s.c = tl->shape.c;
  }
  s.h = tl->shape.h;
  s.w = tl->shape.w;

  // setup tg
  cvk_tg_t tg;
  tg.base_reg_index = 0;
  tg.start_address = phy_addr;
  tg.fmt = CVK_FMT_I8;
  tg.shape = s;
  tg.stride = cvk_ctx->ops->tg_default_stride(cvk_ctx, s, CVK_FMT_I8);

  // apply tdma
  if (nc_transpose) {
    cvk_tdma_g2l_tensor_copy_nc_transposed_param_t p;
    memset(&p, 0, sizeof(p));
    p.src = &tg;
    p.dst = tl;
    cvk_ctx->ops->tdma_g2l_tensor_copy_nc_transposed(cvk_ctx, &p);
  } else {
    cvk_tdma_g2l_tensor_copy_param_t p;
    memset(&p, 0, sizeof(p));
    p.src = &tg;
    p.dst = tl;
    cvk_ctx->ops->tdma_g2l_tensor_copy(cvk_ctx, &p);
  }

}

static void test_tdma_l2g(
  CVI_RT_HANDLE rt_handle,
  cvk_context_t *cvk_ctx,
  const cvk_tl_t *tl,
  uint64_t phy_addr)
{
  cvk_tg_shape_t s;
  s.n = tl->shape.n;
  s.c = tl->shape.c;
  s.h = tl->shape.h;
  s.w = tl->shape.w;

  // setup tg
  cvk_tg_t tg;
  tg.base_reg_index = 0;
  tg.start_address = phy_addr;
  tg.fmt = CVK_FMT_I8;
  tg.shape = s;
  tg.stride = cvk_ctx->ops->tg_default_stride(cvk_ctx, s, CVK_FMT_I8);

  // apply tdma
  cvk_tdma_l2g_tensor_copy_param_t p;
  memset(&p, 0, sizeof(p));
  p.src = tl;
  p.dst = &tg;
  cvk_ctx->ops->tdma_l2g_tensor_copy(cvk_ctx, &p);
}

static void tl_copy_ref(int8_t *a, int8_t *res, uint64_t size)
{
  for (uint64_t i = 0; i < size; i++)
    res[i] = a[i];
}

static void tl_copy_nc_transpose_ref(int8_t *a, int8_t *res,
    int n, int c, int h, int w)
{
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < c; j++) {
      for (int k = 0; k < h * w; k++) {
        res[j * n * h * w + i * h * w + k] = a[i * c * h * w + j * h * w + k];
      }
    }
  }
}

static void tl_copy_hw_transpose_ref(int8_t *a, int8_t *res,
    int n, int c, int h, int w)
{
  for (int i = 0; i < n * c; i++) {
    for (int j = 0; j < h; j++) {
      for (int k = 0; k < w; k++) {
        res[i * h * w + k * h + j] = a[i * h * w + j * w + k];
      }
    }
  }
}

static int test_tensor_copy(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
    int n, int c, int h, int w,
    int eu_align, int nc_transpose, int hw_transpose)
{
  printf("  %s: (%d,%d,%d,%d), eu_align %d, nc_tp %d, hw_tp %d\n",
          __func__, n, c, h, w, eu_align, nc_transpose, hw_transpose);

  int ret = 0;
  uint32_t size = n * c * h * w;
  int8_t *a_data = (int8_t *)malloc(size);
  assert(a_data && "Expect allocated a_data");

  int8_t *res_data = (int8_t *)malloc(size);
  assert(res_data && "Expect allocated res_data");

  for (uint32_t i = 0; i < size; i++)
    a_data[i] = (int8_t)(i % 256);

  int8_t *ref_data = (int8_t *)malloc(size);
  assert(ref_data && "Expect allocated ref_data");
  // calc ref
  if (nc_transpose) {
    tl_copy_nc_transpose_ref(a_data, ref_data, n, c, h, w);
  } else if (hw_transpose) {
    tl_copy_hw_transpose_ref(a_data, ref_data, n, c, h, w);
  } else {
    tl_copy_ref(a_data, ref_data, size);
  }

  cvk_tl_shape_t tl_shape;
  if (nc_transpose) {
    tl_shape.n = c;
    tl_shape.c = n;
  } else {
    tl_shape.n = n;
    tl_shape.c = c;
  }
  tl_shape.h = h;
  tl_shape.w = w;

  cvk_tl_t *tl_a   = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape,
                                                     CVK_FMT_I8, eu_align);
  cvk_tl_t *tl_res = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape,
                                                     CVK_FMT_I8, eu_align);

  // copy input content from system memory to device memory
  size_t input_mem_size = tl_shape.n * tl_shape.c * tl_shape.h * tl_shape.w * 1; // CVK_FMT_I8
  CVI_RT_MEM input_mem= CVI_RT_MemAlloc(rt_handle, input_mem_size);
  CVI_RT_MemCopyS2D(rt_handle, input_mem, a_data);

  // tdma copy descriptor generation, dram to sram
  test_tdma_g2l(rt_handle, cvk_ctx, tl_a, CVI_RT_MemGetPAddr(input_mem), nc_transpose);

  // tiu copy descriptor genereation, sram to sram
  cvk_tiu_copy_param_t p10;
  if (hw_transpose) {
    tl_res->stride.h = 1;
    tl_res->stride.w = tl_shape.h;
  }
  p10.dst = tl_res;
  p10.src = tl_a;
  cvk_ctx->ops->tiu_copy(cvk_ctx, &p10);

  cvk_tl_shape_t tl_shape_dst = tl_shape;
  if (hw_transpose) {
    tl_shape_dst.h = w;
    tl_shape_dst.w = h;
  }
  tl_res->shape = tl_shape_dst;
  tl_res->stride = cvk_ctx->ops->tl_default_stride(cvk_ctx, tl_shape_dst,
                                                   CVK_FMT_I8, eu_align);

  // tdma copy descriptor generation, sram to dram
  size_t result_mem_size = tl_res->shape.n * tl_res->shape.c * tl_res->shape.h * tl_res->shape.w * 1; // CVK_FMT_I8
  CVI_RT_MEM result_mem = CVI_RT_MemAlloc(rt_handle, result_mem_size);
  test_tdma_l2g(rt_handle, cvk_ctx, tl_res, CVI_RT_MemGetPAddr(result_mem));

  // driving tpu hardware by descriptor list
  CVI_RT_Submit(cvk_ctx);

  // copy result content from device memory to system memory
  CVI_RT_MemCopyD2S(rt_handle, res_data, result_mem);

  for (uint64_t i = 0; i < size; i++) {
    if (res_data[i] != ref_data[i]) {
      printf("comparing failed at ofmap_data[%" PRIu64 "], got %x, exp %x\n",
             i, res_data[i], ref_data[i]);
      ret = -1;
    }
  }

  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_res);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_a);

  free(a_data);
  free(res_data);
  free(ref_data);
  CVI_RT_MemFree(rt_handle, input_mem);
  CVI_RT_MemFree(rt_handle, result_mem);

  printf("  %s %s\n", __func__, ret ? "FAILED" : "PASSED");

  return ret;
}

#define CMDBUF_SIZE   (512*1024)  // Adjust based on test case

int main(int argc, char **argv)
{
  int ret = 0;

  CVI_RT_HANDLE rt_handle;
  cvk_context_t *cvk_ctx = NULL;

  //init
  CVI_RT_Init(&rt_handle);
  cvk_ctx = CVI_RT_RegisterKernel(rt_handle, CMDBUF_SIZE);

  printf("Init done\n");

  //use case
  ret = test_tensor_copy(rt_handle, cvk_ctx, 3, 39, 7, 37, 0, 0, 0);
  ret |= test_tensor_copy(rt_handle, cvk_ctx, 3, 39, 7, 37, 1, 0, 0);
  ret |= test_tensor_copy(rt_handle, cvk_ctx, 3, 39, 7, 37, 0, 1, 0);  // nc_transpose
  ret |= test_tensor_copy(rt_handle, cvk_ctx, 3, 39, 7, 37, 0, 0, 1);  // hw_transpose

  //deinit
  CVI_RT_UnRegisterKernel(cvk_ctx);
  CVI_RT_DeInit(rt_handle);

  printf("%s %s\n", __FILE__, ret ? "FAILED" : "PASSED");

  return ret;
}
