#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <inttypes.h>

#include "test_cvikernel_util.h"

typedef cvk_tdma_g2l_matrix_copy_decompressed_param_t param_t;

static void __print_param(const char *tag, FILE *f, param_t *p)
{
  fprintf(
      f, "%s: (%u, %u) => (%u, %u, %u, %u)\n",
      tag,
      p->src->m.shape.row, p->src->m.shape.col,
      p->dst->shape.n, p->dst->shape.c,
      p->dst->shape.w, p->dst->shape.col);
}

#define print_param(f, p) __print_param(__func__, f, p)

typedef struct {
  cvk_mg_shape_t src_shape;
  cvk_ml_shape_t dst_shape;
} case_t;

static case_t g_cases[] = {
  {
    { 0, 17 },
    { 0, 3, 7, 17 },
  },
  {
    { 0, 7 },
    { 0, 7, 1, 7 },
  },
  {
    { 0, 60 },
    { 0, 30, 2, 60 },
  },
#ifndef ENABEL_SIMPLE_VLC_TEST
  {
    { 0, 1 },
    { 0, 1, 1, 1 },
  },
  {
    { 0, 2 },
    { 0, 1, 2, 2 },
  },
  {
    { 0, 2 },
    { 0, 2, 1, 2 },
  },
  {
    { 0, 7 },
    { 0, 1, 7, 7 },
  },
  {
    { 0, 7 },
    { 0, 2, 4, 7 },
  },
  {
    { 0, 17 },
    { 0, 1, 17, 17 },
  },
  {
    { 0, 17 },
    { 0, 17, 1, 17 },
  },
  {
    { 0, 60 },
    { 0, 1, 60, 60 },
  },
  {
    { 0, 60 },
    { 0, 60, 1, 60 },
  }
#endif /* ifndef ENABEL_SIMPLE_VLC_TEST*/
};

static void g2l_matrix_copy_ref(param_t *p, uint8_t ref_data[], uint8_t src_data[])
{
  uint64_t size = ml_shape_size(&p->dst->shape, p->dst->fmt);

  for (uint64_t i = 0; i < size; i++)
    ref_data[i] = src_data[i];
}

static void test_param_g2l(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, param_t *p, uint8_t *src_data, CommandInfo* cmd_info)
{
  print_param(stderr, p);

  uint64_t in_size = ml_shape_size(&p->dst->shape, p->dst->fmt);
  size_t bs_size = 0;
  int is_signed = (p->dst->fmt == CVK_FMT_I8);
  size_t data_type = (p->dst->fmt == CVK_FMT_BF16) ? 1 : 0;

  uint8_t *bsbuf = test_vlc_compress(src_data, in_size, is_signed, data_type, &bs_size, cmd_info, NULL);
  cmpr_matrix_copy_s2d(rt_handle, p->src, bsbuf);
  free(bsbuf);

  cvk_ctx->ops->tdma_g2l_matrix_copy_decompressed(cvk_ctx, p);
  CVI_RT_Submit(cvk_ctx);
  uint8_t *dst_data = matrix_copy_l2g_d2s(rt_handle, cvk_ctx, p->dst);

  uint8_t *ref_data = (uint8_t *)malloc(sizeof(uint8_t) * in_size);
  g2l_matrix_copy_ref(p, ref_data, src_data);

  for (uint64_t i = 0; i < in_size; i++) {
    if (dst_data[i] != ref_data[i]) {
      fprintf(stderr, "comparing failed at dst[%" PRIu64 "], got %d, exp %d\n",
              i, dst_data[i], ref_data[i]);
      exit(-1);
    }
  }

  free(dst_data);
  free(ref_data);
}

static void destroy_param_g2l(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, param_t *p)
{
  free_cmpr_matrix_dev_mem(rt_handle, p->src);
  cvk_ctx->ops->lmem_free_matrix(cvk_ctx, p->dst);
}

static void test_one_case(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, case_t *c)
{
  cvk_fmt_t fmts[] = { CVK_FMT_I8, CVK_FMT_U8 };
  uint8_t fmts_sz = sizeof(fmts) / sizeof(fmts[0]);

  for (uint32_t row = 1; row < 13; row += 2) {
    c->src_shape.row = row;
    c->dst_shape.n = row;
    for (int mode = 0; mode < VLC_CMP_MODE_MAX; mode++) {
      for (int dst_align = 0; dst_align < 2; dst_align++) {
        for (uint8_t fmt_i = 0; fmt_i < fmts_sz; fmt_i++) {
          cvk_fmt_t fmt = fmts[fmt_i];
          param_t p;

          memset(&p, 0, sizeof(p));

          int is_signed = (fmt == CVK_FMT_I8);
          size_t data_type = (fmt == CVK_FMT_BF16) ? 1 : 0;
          CommandInfo cmd_info;

          memset(&cmd_info, 0, sizeof(CommandInfo));
          cmd_info.signedness = is_signed;

          // <! 1. alloc source
          p.dst = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, c->dst_shape, fmt, dst_align);
          uint64_t in_size = ml_shape_size(&p.dst->shape, p.dst->fmt);

          // <! 2 init input
          uint8_t *src_data = (uint8_t *)malloc(sizeof(uint8_t) * in_size);
          test_vlc_init_testdata(src_data, in_size, fmt == CVK_FMT_I8, fmt == CVK_FMT_BF16);

          // <! 3 try to manual set bias0/bias1
          if (mode == VLC_CMP_MODE_COMPILER) {
            cvk_vlc_est_weight_bias(src_data, in_size, (bool)is_signed, (bool)data_type, &cmd_info);
          }

          p.src = alloc_cmpr_matrix_dev_mem(rt_handle, c->src_shape, fmt, &cmd_info);

          //printf ("row %u mode %d is_align %d fmt %d\n", row, mode, dst_align, fmt);
          test_param_g2l(rt_handle, cvk_ctx, &p, src_data, &cmd_info);

          free(src_data);
          destroy_param_g2l(rt_handle, cvk_ctx, &p);
        }
      }
    }
  }
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
