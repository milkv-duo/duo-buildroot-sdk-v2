#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <inttypes.h>

#include "test_cvikernel_util.h"

typedef cvk_tdma_l2g_matrix_copy_compressed_param_t param_t;

static void __print_param(const char *tag, FILE *f, param_t *p)
{
  fprintf(
      f, "%s: (%u, %u, %u, %u) => (%u, %u)\n",
      tag,
      p->src->shape.n, p->src->shape.c, p->src->shape.w, p->src->shape.col,
      p->dst->m.shape.row, p->dst->m.shape.col);
}

#define print_param(f, p) __print_param(__func__, f, p)

typedef struct {
  cvk_ml_shape_t src_shape;
  cvk_mg_shape_t dst_shape;
} case_t;

static case_t g_cases[] = {
 {
    { 0, 2, 4, 7 },
    { 0, 7 },
  },
 {
    { 0, 1, 2, 2 },
    { 1, 2 },
  },
 {
    { 0, 3, 7, 17 },
    { 0, 17 },
  },
 {
    { 0, 17, 1, 17 },
    { 0, 17 },
  },
 {
    { 0, 60, 1, 60 },
    { 0, 60 },
  },
#ifndef ENABEL_SIMPLE_VLC_TEST
  {
    { 0, 1, 1, 1 },
    { 0, 1 },
  },
 {
    { 0, 2, 1, 2 },
    { 1, 2 },
  },
 {
    { 0, 1, 7, 7 },
    { 0, 7 },
  },
 {
    { 0, 7, 1, 7 },
    { 0, 7 },
  },
 {
    { 0, 1, 17, 17 },
    { 0, 17 },
  },
 {
    { 0, 1, 60, 60 },
    { 0, 60 },
  },
 {
    { 0, 30, 2, 60 },
    { 0, 60 },
  },
#endif /* ifndef ENABEL_SIMPLE_VLC_TEST*/
};

static void test_param_l2g(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, param_t *p, uint8_t* src_data, CommandInfo * cmd_info)
{
  print_param(stderr, p);
  uint64_t size = ml_shape_size(&p->src->shape, p->src->fmt);

  matrix_copy_s2d_g2l(rt_handle, cvk_ctx, p->src, src_data);
  cvk_ctx->ops->tdma_l2g_matrix_copy_compressed(cvk_ctx, p);
  CVI_RT_Submit(cvk_ctx);

  int is_signed = (p->src->fmt == CVK_FMT_I8);
  int data_type = (p->src->fmt == CVK_FMT_BF16) ? 1 : 0;
  size_t bs_size;

  uint8_t *ref_data = test_vlc_compress(src_data, size, is_signed, data_type, &bs_size, cmd_info, NULL);
  uint8_t *dst_data = cmpr_matrix_copy_d2s(rt_handle, p->dst);

  for (uint64_t i = 0; i < bs_size; i++) {
    if (dst_data[i] != ref_data[i]) {
      fprintf(stderr, "comparing failed at dst[%" PRIu64 "], got %d, exp %d\n",
          i, dst_data[i], ref_data[i]);
      exit(-1);
    }
  }

  free(dst_data);
  free(ref_data);
}


static void destroy_param_l2g(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, param_t *p)
{
  cvk_ctx->ops->lmem_free_matrix(cvk_ctx, p->src);
  free_cmpr_matrix_dev_mem(rt_handle, p->dst);
}

static void test_one_case(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, case_t *c)
{
  cvk_fmt_t fmts[] = { CVK_FMT_I8, CVK_FMT_U8 };
  uint8_t fmts_sz = sizeof(fmts) / sizeof(fmts[0]);

  for (uint32_t row = 1; row < 13; row += 2) {
    c->src_shape.n = row;
    c->dst_shape.row = row;
    for (int src_align = 0; src_align < 2; src_align++) {
      for (uint8_t fmt_i = 0; fmt_i < fmts_sz; fmt_i++) {
        cvk_fmt_t fmt = fmts[fmt_i];
        param_t p;
        memset(&p, 0, sizeof(p));
        p.src = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, c->src_shape, fmt, src_align);

        uint64_t size = ml_shape_size(&p.src->shape, p.src->fmt);
        uint8_t *src_data = (uint8_t *)malloc(sizeof(uint8_t) * size);
        test_vlc_init_testdata(src_data, size, fmt == CVK_FMT_I8, fmt == CVK_FMT_BF16);

        //size_t bs_size;
        CommandInfo cmd_info;
        memset(&cmd_info, 0, sizeof(CommandInfo));

        // <! not support bias0/1 setting compress by hw
        //get_vlc_compressed_meta(src_data, size, p.src->fmt, &bs_size, &cmd_info);

        int is_signed = (p.src->fmt == CVK_FMT_I8);
        cmd_info.signedness = is_signed;

        // <! max compressed size
        p.dst = alloc_cmpr_matrix_dev_mem(rt_handle, c->dst_shape, p.src->fmt, &cmd_info);

        //printf ("row %u is_align %d fmt %d\n", row, src_align, fmt);
        test_param_l2g(rt_handle, cvk_ctx, &p, src_data, &cmd_info);
        destroy_param_l2g(rt_handle, cvk_ctx, &p);
        free(src_data);
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
