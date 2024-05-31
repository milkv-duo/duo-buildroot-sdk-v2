#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <inttypes.h>

#include "test_cvikernel_util.h"

typedef cvk_tdma_g2l_tensor_copy_chw_rotated_param_t param_t;

static void __print_param(const char *tag, FILE *f, param_t *p)
{
  fprintf(
      f, "%s: (%u, %u, %u, %u) => (%u, %u, %u, %u)\n",
      tag,
      p->src->shape.n, p->src->shape.h, p->src->shape.w, p->src->shape.c,
      p->dst->shape.n, p->dst->shape.c, p->dst->shape.h, p->dst->shape.w);
}

#define print_param(f, p) __print_param(__func__, f, p)

typedef struct {
  cvk_tg_shape_t src_shape;
  cvk_tl_shape_t dst_shape;
} case_t;

static case_t g_cases[] = {
  {
    { 1, 3, 1, 1 }, // nchw for neuron
    { 1, 3, 1, 1 }, // nchw for neuron
  }, {
    { 1, 4, 1, 1 },
    { 1, 4, 1, 1 },
  }, {
    { 1, 3, 1, 7 },
    { 1, 3, 1, 7 },
  }, {
    { 1, 4, 1, 7 },
    { 1, 4, 1, 7 },
  }, {
    { 1, 3, 1, 17 },
    { 1, 3, 1, 17 },
  }, {
    { 1, 4, 1, 17 },
    { 1, 4, 1, 17 },
  }, {
    { 1, 3, 2, 1 },
    { 1, 3, 2, 1 },
  }, {
    { 1, 4, 2, 1 },
    { 1, 4, 2, 1 },
  }, {
    {  2, 3, 17, 1 },
    {  2, 3, 17, 1 },
  }, {
    {  2, 4, 17, 1 },
    {  2, 4, 17, 1 },
  }, {
    {  2, 3, 17, 3 },
    {  2, 3, 17, 3 },
  }, {
    {  2, 4, 17, 3 },
    {  2, 4, 17, 3 },
  }, {
    {  3, 3, 16, 7 },
    {  3, 3, 16, 7 },
  }, {
    {  3, 4, 16, 7 },
    {  3, 4, 16, 7 },
  }, {
    {  3, 3, 39, 17 },
    {  3, 3, 39, 17 },
  }, {
    {  3, 4, 39, 17 },
    {  3, 4, 39, 17 },
  }, {
    {  3, 3, 36, 16 },
    {  3, 3, 36, 16 },
  }, {
    {  3, 4, 36, 16 },
    {  3, 4, 36, 16 },
  }, {
    {  5, 3, 39, 17 },
    {  5, 3, 39, 17 },
  }, {
    {  5, 4, 39, 17 },
    {  5, 4, 39, 17 },
  }, {
    { 20, 3, 35, 2 },
    { 20, 3, 35, 2 },
  }, {
    { 20, 4, 35, 2 },
    { 20, 4, 35, 2 },
  }, {
    { 20, 3, 35, 3 },
    { 20, 3, 35, 3 },
  }, {
    { 20, 4, 35, 3 },
    { 20, 4, 35, 3 },
  }
};

static void g2l_tensor_copy_chw_rotated_ref(
    param_t *p, uint8_t ref_data[], uint8_t src_data[])
{
  cvk_tg_shape_t s = p->src->shape;
  // change nhwc -> nchw by HW design automatically
  uint32_t n = s.n;
  uint32_t c = s.h;
  uint32_t h = s.w;
  uint32_t w = s.c;
  for (uint32_t ni = 0; ni < n; ni++) {
    for (uint32_t ci = 0; ci < c; ci++) {
      for (uint32_t hi = 0; hi < h; hi++) {
        for (uint32_t wi = 0; wi < w; wi++) {
          uint32_t src_i = ni * c * h * w + ci * h * w + hi * w + wi;
          uint32_t dst_i = ni * w * c * h + wi * c * h + ci * h + hi;
          ref_data[dst_i] = src_data[src_i];
        }
      }
    }
  }
}

static void test_param_g2l(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, param_t *p)
{
  print_param(stderr, p);
  uint64_t size = tg_shape_size(&p->src->shape, p->src->fmt);
  uint8_t *src_data = (uint8_t *)malloc(sizeof(uint8_t) * size);
  if (!src_data)
    return;

  for (uint64_t i = 0; i < size; i++)
    src_data[i] = 200 + i;

  tensor_copy_s2d(rt_handle, p->src, src_data);
  cvk_ctx->ops->tdma_g2l_tensor_copy_chw_rotated(cvk_ctx, p);
  CVI_RT_Submit(cvk_ctx);
  uint8_t *dst_data = tensor_copy_l2g_d2s(rt_handle, cvk_ctx, p->dst);

  uint8_t *ref_data = (uint8_t *)malloc(sizeof(uint8_t) * size);
  if (!dst_data || !ref_data)
    goto fail_exit;

  g2l_tensor_copy_chw_rotated_ref(p, ref_data, src_data);
 
  for (uint64_t i = 0; i < size; i++) {
    if (dst_data[i] != ref_data[i]) {
      fprintf(stderr, "comparing failed at dst[%" PRIu64 "], got %d, exp %d\n",
              i, dst_data[i], ref_data[i]);
      exit(-1);
    }
  }

fail_exit:
  free(src_data);
  free(dst_data);
  free(ref_data);
}

static void destroy_param_g2l(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, param_t *p)
{
  free_tensor_dev_mem(rt_handle, p->src);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, p->dst);
}

static void test_one_case(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, case_t *c)
{

  param_t p;
  memset(&p, 0, sizeof(p));

  p.src = alloc_tensor_dev_mem(rt_handle, cvk_ctx, c->src_shape, CVK_FMT_I8);
  p.dst = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, c->dst_shape, CVK_FMT_I8, 1);
  test_param_g2l(rt_handle, cvk_ctx, &p);
  destroy_param_g2l(rt_handle, cvk_ctx, &p);

}

int main(int argc, char **argv)
{
  CVI_RT_HANDLE rt_handle = NULL;
  cvk_context_t *cvk_ctx = NULL;
 
  if (!argc)
    return -1;
  if (!argv)
    return -1;

  CVI_RT_Init(&rt_handle);
  cvk_ctx = CVI_RT_RegisterKernel(rt_handle, CMDBUF_SIZE);
  if (!rt_handle || !cvk_ctx) {
    printf("%s fail\n", __FILENAME__);
    return -1;
  }

  uint32_t nr_cases = sizeof(g_cases) / sizeof(g_cases[0]);
  for (uint32_t i = 0; i < nr_cases; i++)
    test_one_case(rt_handle, cvk_ctx, &g_cases[i]);

  CVI_RT_UnRegisterKernel(cvk_ctx);
  CVI_RT_DeInit(rt_handle);

  return 0;
}
