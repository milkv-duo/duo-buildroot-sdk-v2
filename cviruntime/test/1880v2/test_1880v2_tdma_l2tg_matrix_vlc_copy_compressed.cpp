#include "1880v2_test_util.h"

typedef bmk1880v2_tdma_l2tg_matrix_copy_compressed_param_t param_t;

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
  ml_shape_t src_shape;
  mg_shape_t dst_shape;
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
#ifndef ENABEL_SIMPLE_BMK1880V2_VLC_TEST
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
#endif /* ifndef ENABEL_SIMPLE_BMK1880V2_VLC_TEST*/
};

static void test_param_l2g(CVI_RT_HANDLE *ctx, bmk_ctx_t *bmk, param_t *p, u8* src_data, CommandInfo * cmd_info)
{
  print_param(stderr, p);
  u64 size = ml_shape_size(&p->src->shape);

  put_matrix_g2l(ctx, bmk, p->src, src_data);
  bmk1880v2_tdma_l2g_matrix_copy_compressed(bmk, p);
  test_submit(ctx);

  int is_signed = (p->src->fmt == FMT_I8);
  int data_type = (p->src->fmt == FMT_BF16) ? 1 : 0;
  size_t bs_size;

  size_t bs_buf_size = get_out_bs_buf_size(size, data_type);
  u8 *ref_data = vlc_compress(src_data, size, is_signed, data_type, &bs_size, cmd_info, NULL);
  u8 *dst_data = get_compressed_mg_gmem(ctx, p->dst, bs_buf_size);

  for (u64 i = 0; i < bs_size; i++) {
    if (dst_data[i] != ref_data[i]) {
      fprintf(stderr, "comparing failed at dst[%" PRIu64 "], got %d, exp %d\n",
          i, dst_data[i], ref_data[i]);
      exit(-1);
    }
  }

  free(dst_data);
  free(ref_data);
}


static void destroy_param_l2g(CVI_RT_HANDLE *ctx, bmk_ctx_t *bmk, param_t *p)
{
  free_ml(bmk, p->src);
  free_compressed_mg_gmem(ctx, p->dst);
}

static void test_one_case(CVI_RT_HANDLE *ctx, bmk_ctx_t *bmk, case_t *c)
{
  fmt_t fmts[] = { FMT_I8, FMT_U8 };
  u8 fmts_sz = sizeof(fmts) / sizeof(fmts[0]);

  for (u32 row = 1; row < 13; row += 2) {
    c->src_shape.n = row;
    c->dst_shape.row = row;
    for (int src_align = 0; src_align < 2; src_align++) {
      for (u8 fmt_i = 0; fmt_i < fmts_sz; fmt_i++) {
        fmt_t fmt = fmts[fmt_i];
        param_t p;
        memset(&p, 0, sizeof(p));
        p.src = alloc_ml(bmk, c->src_shape, fmt, src_align);

        u64 size = ml_shape_size(&p.src->shape);
        u8 *src_data = (u8 *)malloc(sizeof(u8) * size);
        vlc_init_testdata(src_data, size, fmt == FMT_I8, fmt == FMT_BF16);

        //size_t bs_size;
        CommandInfo cmd_info;
        memset(&cmd_info, 0, sizeof(CommandInfo));

        // <! not support bias0/1 setting compress by hw
        //get_vlc_compressed_meta(src_data, size, p.src->fmt, &bs_size, &cmd_info);

        int is_signed = (p.src->fmt == FMT_I8);
        cmd_info.signedness = is_signed;

        // <! max compressed size
        p.dst = alloc_vlc_compressed_mg_gmem(ctx, c->dst_shape, p.src->fmt, &cmd_info);

        //printf ("row %u is_align %d fmt %d\n", row, src_align, fmt);
        test_param_l2g(ctx, bmk, &p, src_data, &cmd_info);
        destroy_param_l2g(ctx, bmk, &p);
        free(src_data);
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
