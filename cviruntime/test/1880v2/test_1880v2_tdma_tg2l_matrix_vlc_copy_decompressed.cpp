#include "1880v2_test_util.h"

typedef bmk1880v2_tdma_tg2l_matrix_copy_decompressed_param_t param_t;

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
  mg_shape_t src_shape;
  ml_shape_t dst_shape;
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
#ifndef ENABEL_SIMPLE_BMK1880V2_VLC_TEST
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
#endif /* ifndef ENABEL_SIMPLE_BMK1880V2_VLC_TEST*/
};

static void tg2l_matrix_copy_ref(param_t *p, u8 ref_data[], u8 src_data[])
{
  u64 size = ml_shape_size(&p->dst->shape);

  for (u64 i = 0; i < size; i++)
    ref_data[i] = src_data[i];
}

static void test_param_g2l(CVI_RT_HANDLE *ctx, bmk_ctx_t *bmk, param_t *p, u8 *src_data, CommandInfo* cmd_info)
{
  print_param(stderr, p);

  u64 in_size = ml_shape_size(&p->dst->shape);
  size_t bs_size = 0;
  int is_signed = (p->dst->fmt == FMT_I8);
  size_t data_type = (p->dst->fmt == FMT_BF16) ? 1 : 0;

  u8 *bsbuf = vlc_compress(src_data, in_size, is_signed, data_type, &bs_size, cmd_info, NULL);

  put_compressed_mg_gmem(ctx, p->src, bsbuf, bs_size);
  free(bsbuf);
  bmk1880v2_tdma_g2l_matrix_copy_decompressed(bmk, p);
  test_submit(ctx);
  u8 *dst_data = get_matrix_l2g(ctx, bmk, p->dst);

  u8 *ref_data = (u8 *)malloc(sizeof(u8) * in_size);
  tg2l_matrix_copy_ref(p, ref_data, src_data);

  for (u64 i = 0; i < in_size; i++) {
    if (dst_data[i] != ref_data[i]) {
      fprintf(stderr, "comparing failed at dst[%" PRIu64 "], got %d, exp %d\n",
              i, dst_data[i], ref_data[i]);
      exit(-1);
    }
  }

  free(dst_data);
  free(ref_data);
}

static void destroy_param_g2l(CVI_RT_HANDLE *ctx, bmk_ctx_t *bmk, param_t *p)
{
  free_compressed_mg_gmem(ctx, p->src);
  free_ml(bmk, p->dst);
}

static void test_one_case(CVI_RT_HANDLE *ctx, bmk_ctx_t *bmk, case_t *c)
{
  fmt_t fmts[] = { FMT_I8, FMT_U8 };
  u8 fmts_sz = sizeof(fmts) / sizeof(fmts[0]);

  for (u32 row = 1; row < 13; row += 2) {
    c->src_shape.row = row;
    c->dst_shape.n = row;
    for (int mode = 0; mode < VLC_CMP_MODE_MAX; mode++) {
      for (int dst_align = 0; dst_align < 2; dst_align++) {
        for (u8 fmt_i = 0; fmt_i < fmts_sz; fmt_i++) {
          fmt_t fmt = fmts[fmt_i];
          param_t p;
          int is_signed = (fmt == FMT_I8);
          size_t data_type = (fmt == FMT_BF16) ? 1 : 0;
          CommandInfo cmd_info;

          memset(&cmd_info, 0, sizeof(CommandInfo));
          cmd_info.signedness = is_signed;

          memset(&p, 0, sizeof(p));

          // <! 1. alloc source
          p.dst = alloc_ml(bmk, c->dst_shape, fmt, dst_align);
          u64 in_size = ml_shape_size(&p.dst->shape);

          // <! 2 init input
          u8 *src_data = (u8 *)malloc(sizeof(u8) * in_size);
          vlc_init_testdata(src_data, in_size, fmt == FMT_I8, fmt == FMT_BF16);

          // <! 3 try to manual set bias0/bias1
          if (mode == VLC_CMP_MODE_COMPILER) {
            bm_vlc_est_weight_bias(src_data, in_size, (bool)is_signed, (bool)data_type, &cmd_info);
          }

          p.src = alloc_vlc_compressed_mg_gmem(ctx, c->src_shape, fmt, &cmd_info);

          //printf ("row %u mode %d is_align %d fmt %d\n", row, mode, dst_align, fmt);
          test_param_g2l(ctx, bmk, &p, src_data, &cmd_info);

          free(src_data);
          destroy_param_g2l(ctx, bmk, &p);
        }
      }
    }
  }
}

int main()
{
  CVI_RT_HANDLE ctx;
  bmk_ctx_t *bmk;
  test_init(&ctx, &bmk);

  u32 nr_cases = sizeof(g_cases) / sizeof(g_cases[0]);
  for (u32 i = 0; i < nr_cases; i++)
    test_one_case(&ctx, bmk, &g_cases[i]);

  test_exit(&ctx);
  return 0;
}
