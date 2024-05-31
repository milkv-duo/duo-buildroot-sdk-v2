#include <assert.h>
#include <cvikernel/cvikernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_cvikernel_util.h"

#define container_of(ptr, type, member)                \
  ({                                                   \
    const typeof(((type *)0)->member) *__mptr = (ptr); \
    (type *)((char *)__mptr - offsetof(type, member)); \
  })

typedef struct {
  cvk_tg_t tg;
  CVI_RT_MEM mem;
} test_tg_wrapper_t;

typedef struct {
  cvk_mg_t mg;
  CVI_RT_MEM mem;
} test_mg_wrapper_t;

void test_submit_comp(CVI_RT_HANDLE *bm_ctx, cvk_context_t *cvk_ctx) {
  (void)cvk_ctx;
  (void)bm_ctx;
  CVI_RT_Submit(cvk_ctx);
}

cvk_tg_t *test_alloc_tg_mem_comp(CVI_RT_HANDLE *bm_ctx, cvk_context_t *cvk_ctx,
                                 cvk_tg_shape_t shape, cvk_fmt_t fmt) {
  CVI_RT_HANDLE ctx = (CVI_RT_HANDLE)*bm_ctx;
  int alloc_sz = tg_shape_size(&shape) * bytesize_of_fmt(fmt);

  test_tg_wrapper_t *w = (test_tg_wrapper_t *)malloc(sizeof(*w));
  assert(w && "Expected allocated tg wrapper");

  w->tg.base_reg_index = 0;
  w->mem = CVI_RT_MemAlloc(ctx, alloc_sz);
  w->tg.start_address = CVI_RT_MemGetPAddr(w->mem);
  w->tg.fmt = fmt;
  w->tg.shape = shape;
  w->tg.stride = cvk_ctx->ops->tg_default_stride(cvk_ctx, shape, fmt);

  return &w->tg;
}

cvk_mg_t *test_alloc_mg_mem_comp(CVI_RT_HANDLE *bm_ctx, cvk_mg_shape_t s, cvk_fmt_t fmt) {
  int alloc_sz = mg_shape_size(&s) * bytesize_of_fmt(fmt);
  CVI_RT_HANDLE ctx = (CVI_RT_HANDLE)*bm_ctx;

  test_mg_wrapper_t *w = (test_mg_wrapper_t *)malloc(sizeof(*w));
  w->mem = CVI_RT_MemAlloc(ctx, alloc_sz);

  w->mg.base_reg_index = 0;
  w->mg.start_address = CVI_RT_MemGetPAddr(w->mem);
  w->mg.shape = s;
  w->mg.fmt = fmt;
  w->mg.stride.row = s.col * bytesize_of_fmt(fmt);

  return &w->mg;
}

void test_free_tg_mem_comp(CVI_RT_HANDLE *ctx, const cvk_tg_t *tg) {
  test_tg_wrapper_t *w = container_of(tg, test_tg_wrapper_t, tg);
  CVI_RT_MemFree(*ctx, w->mem);

  free(w);
}

void test_free_mg_mem_comp(CVI_RT_HANDLE *ctx, const cvk_mg_t *mg) {
  test_mg_wrapper_t *w = container_of(mg, test_mg_wrapper_t, mg);
  CVI_RT_MemFree(*ctx, w->mem);

  free(w);
}

void test_put_tg_mem_comp(CVI_RT_HANDLE *bm_ctx, const cvk_tg_t *tg, uint8_t data[]) {
  test_tg_wrapper_t *w = container_of(tg, test_tg_wrapper_t, tg);
  CVI_RT_MemCopyS2D(*bm_ctx, w->mem, data);
}

void test_put_mg_mem_comp(CVI_RT_HANDLE *ctx, const cvk_mg_t *mg, uint8_t data[]) {
  test_mg_wrapper_t *w = (typeof(w))mg;
  CVI_RT_MemCopyS2D(*ctx, w->mem, data);
}

uint8_t *test_get_tg_mem_comp(CVI_RT_HANDLE *bm_ctx, const cvk_tg_t *tg) {
  cvk_tg_shape_t s = tg->shape;

  int data_type_size = 1;
  if (tg->fmt == CVK_FMT_BF16) {
    data_type_size = 2;
  }

  uint32_t size = s.n * s.c * s.h * s.w * data_type_size;
  uint8_t *data = (uint8_t *)malloc(size);
  assert(data && "Expect allocated data for get tg mem");

  test_tg_wrapper_t *w = container_of(tg, test_tg_wrapper_t, tg);
  CVI_RT_MemCopyD2S(*bm_ctx, data, w->mem);

  return data;
}

uint8_t *test_get_mg_mem_comp(CVI_RT_HANDLE *ctx, const cvk_mg_t *mg) {
  cvk_mg_shape_t s = mg->shape;
  uint32_t size = s.row * s.col * (mg->fmt == CVK_FMT_BF16 ? 2 : 1);
  uint8_t *data = (uint8_t *)malloc(size);
  assert(data && "Expect allocated data for get mg mem");

  test_mg_wrapper_t *w = container_of(mg, test_mg_wrapper_t, mg);
  CVI_RT_MemCopyD2S(*ctx, data, w->mem);

  return data;
}

uint8_t *test_get_tensor_l2g_comp(CVI_RT_HANDLE *bm_ctx, cvk_context_t *cvk_ctx,
                                  const cvk_tl_t *tl) {
  cvk_tg_shape_t s;
  s.n = tl->shape.n;
  s.c = tl->shape.h;
  s.h = tl->shape.w;
  s.w = tl->shape.c;
  cvk_tg_t *tg = test_alloc_tg_mem_comp(bm_ctx, cvk_ctx, s, tl->fmt);

  cvk_tdma_l2g_tensor_copy_param_t p;
  p.src = tl;
  p.dst = tg;

  if (tl->fmt == CVK_FMT_BF16) {
    cvk_ctx->ops->tdma_l2g_bf16_tensor_copy(cvk_ctx, &p);
  } else {
    cvk_ctx->ops->tdma_l2g_tensor_copy(cvk_ctx, &p);
  }
  test_submit_comp(bm_ctx, cvk_ctx);
  uint8_t *data = test_get_tg_mem_comp(bm_ctx, tg);

  test_free_tg_mem_comp(bm_ctx, tg);
  return data;
}

uint8_t *test_get_matrix_l2g_comp(CVI_RT_HANDLE *ctx, cvk_context_t *cvk_ctx, const cvk_ml_t *ml) {
  cvk_mg_shape_t s;
  s.row = ml->shape.n;
  s.col = ml->shape.col;
  cvk_mg_t *mg = test_alloc_mg_mem_comp(ctx, s, ml->fmt);

  cvk_tdma_l2g_matrix_copy_param_t p;
  p.src = ml;
  p.dst = mg;

  if (ml->fmt == CVK_FMT_BF16) {
    cvk_ctx->ops->tdma_l2g_bf16_matrix_copy(cvk_ctx, &p);
  } else {
    cvk_ctx->ops->tdma_l2g_matrix_copy(cvk_ctx, &p);
  }

  test_submit_comp(ctx, cvk_ctx);

  uint8_t *data = test_get_mg_mem_comp(ctx, mg);

  test_free_mg_mem_comp(ctx, mg);

  return data;
}

void test_put_tensor_g2l_comp(CVI_RT_HANDLE *bm_ctx, cvk_context_t *cvk_ctx, const cvk_tl_t *tl,
                              uint8_t data[]) {
  cvk_tg_shape_t tg_shape;
  tg_shape.n = tl->shape.n;
  tg_shape.c = tl->shape.c;
  tg_shape.h = tl->shape.h;
  tg_shape.w = tl->shape.w;

  cvk_tg_t *tg = test_alloc_tg_mem_comp(bm_ctx, cvk_ctx, tg_shape, tl->fmt);

  cvk_tdma_g2l_tensor_copy_param_t p;
  p.src = tg;
  p.dst = tl;

  test_put_tg_mem_comp(bm_ctx, tg, data);

  if (tl->fmt == CVK_FMT_BF16) {
    cvk_ctx->ops->tdma_g2l_bf16_tensor_copy(cvk_ctx, &p);
  } else {
    cvk_ctx->ops->tdma_g2l_tensor_copy(cvk_ctx, &p);
  }
  test_submit_comp(bm_ctx, cvk_ctx);

  test_free_tg_mem_comp(bm_ctx, tg);
}

void test_put_matrix_g2l_comp(CVI_RT_HANDLE *bm_ctx, cvk_context_t *cvk_ctx, const cvk_ml_t *ml,
                              uint8_t data[]) {
  cvk_fmt_t mg_data_format = ml->fmt;
  cvk_mg_shape_t s;
  s.row = ml->shape.n;
  s.col = ml->shape.col;
  cvk_mg_t *mg = test_alloc_mg_mem_comp(bm_ctx, s, mg_data_format);

  cvk_tdma_g2l_matrix_copy_param_t p;
  p.src = mg;
  p.dst = ml;

  test_put_mg_mem_comp(bm_ctx, mg, data);
  if (ml->fmt == CVK_FMT_BF16) {
    cvk_ctx->ops->tdma_g2l_bf16_matrix_copy(cvk_ctx, &p);
  } else {
    cvk_ctx->ops->tdma_g2l_matrix_copy(cvk_ctx, &p);
  }

  test_submit_comp(bm_ctx, cvk_ctx);

  test_free_mg_mem_comp(bm_ctx, mg);
}

cvk_mg_t *test_put_matrix_g(CVI_RT_HANDLE *bm_ctx, const cvk_mg_shape_t s, cvk_fmt_t mg_data_format,
                            uint8_t data[]) {
  cvk_mg_t *mg = test_alloc_mg_mem_comp(bm_ctx, s, mg_data_format);

  test_put_mg_mem_comp(bm_ctx, mg, data);
  return mg;
}

cvk_tl_t *test_alloc_tl(cvk_context_t *cvk_ctx, cvk_tl_shape_t shape, cvk_fmt_t fmt, int eu_align) {
  cvk_tl_t *tl = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, shape, fmt, eu_align);
  return tl;
}

void test_free_tl(cvk_context_t *cvk_ctx, const cvk_tl_t *t) {
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, t);
}

#define CNV_SCALAR_C_ALIGN (0x1000)
inline uint64_t cnvAlign64(const uint64_t length, const uint64_t align) {
  uint64_t stride = (uint64_t)(length / align) * align;
  if (stride < length) {
    stride += align;
  }
  return stride;
}
uint8_t *test_get_vp_addr(bmctx_t *ctx, AddrInfo *pAddrInfo)
{

  if(pAddrInfo->vir_addr){
    test_free_vp_addr(ctx, pAddrInfo);
  }
  pAddrInfo->mem = bmmem_device_alloc_raw(*ctx, pAddrInfo->size_bytes);
  pAddrInfo->vir_addr = (uint8_t *)bmmem_device_v_addr(pAddrInfo->mem);;
  pAddrInfo->phy_addr = bmmem_device_addr(pAddrInfo->mem);


  uint64_t new_paddr = cnvAlign64(pAddrInfo->phy_addr, CNV_SCALAR_C_ALIGN);
  uint64_t offset = new_paddr - pAddrInfo->phy_addr;
  pAddrInfo->phy_addr = new_paddr;
  pAddrInfo->vir_addr += offset;

  return pAddrInfo->vir_addr;
}

void test_free_vp_addr(bmctx_t *ctx,  AddrInfo *pAddrInfo){

  bmmem_device_free(*ctx, pAddrInfo->mem);
  pAddrInfo->phy_addr = -1;
  pAddrInfo->vir_addr = NULL;
  //pAddrInfo->size_bytes = 0;

}
