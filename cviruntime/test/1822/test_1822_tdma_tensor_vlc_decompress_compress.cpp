#include "1822_test_util.h"

typedef bmk1822_tdma_tg2l_tensor_copy_decompressed_param_t decompress_param_t;
typedef bmk1822_tdma_l2tg_tensor_copy_compressed_param_t compress_param_t;

typedef struct{
  decompress_param_t dec_p;
  compress_param_t com_p;
} param_t;

static void __print_param(const char *tag, FILE *f, param_t *p)
{
  fprintf(
      f, "%s: (%u, %u, %u, %u) => %d-bit %s\n",
      tag,
      p->dec_p.dst->shape.n, p->dec_p.dst->shape.c, p->dec_p.dst->shape.h, p->dec_p.dst->shape.w,
      p->dec_p.src->bit_length,
      (p->dec_p.dst->fmt == FMT_I8)? "signed": "unsigned");
}

#define print_param(f, p) __print_param(__func__, f, p)

typedef struct {
  tl_shape_t lmem_shape;
} case_t;

static case_t g_cases[] = {
  {
    { 1, 1, 17, 13 }
  },
  {
    { 3, 39, 17, 23 }
  },
  {
    { 5, 39, 17, 23 }
  },
  {
    { 20, 35,  2, 2 }
  },
#ifndef ENABEL_SIMPLE_BMK1822_VLC_TEST
  {
    { 1, 1, 1, 1 }
  },
  {
    { 1, 1, 1, 2 }
  },
  {
    { 1, 1, 7, 2 }
  },
  {
    { 1, 1, 10, 60 }
  },
  {
    { 1, 2, 1, 1 }
  },
  {
    { 2, 17, 1,  4 }
  },
  {
    { 2, 17, 1,  4 }
  },
  {
    { 3, 16, 1, 1 }
  },
  {
    { 3, 36, 16,  20 }
  },
#endif /* ifndef ENABEL_SIMPLE_BMK1822_VLC_TEST*/
};

static void test_param_g2l(bmctx_t *ctx, bmk_ctx_t *bmk, param_t *p, compressed_tg_t* dst)
{
  print_param(stderr, p);
  u64 size = tl_shape_size(&p->dec_p.dst->shape);
  int is_signed = (p->dec_p.dst->fmt == FMT_I8);
  u8 *src_data = (u8 *)malloc(sizeof(u8) * size);
  vlc_init_testdata(src_data, size, p->dec_p.dst->fmt == FMT_I8, p->dec_p.dst->fmt == FMT_BF16);

  u8 *gmem_data;
  size_t total_size;
  size_t data_type = (p->dec_p.dst->fmt == FMT_BF16) ? 1 : 0;
  size_t in_size = size;
  size_t bs_buf_size = get_out_bs_buf_size(size, data_type);
  gmem_data = (uint8_t *) malloc(bs_buf_size * sizeof(uint8_t));

  // command info
  CommandInfo cmd_info;
  memset(&cmd_info, 0, sizeof(CommandInfo));
  cmd_info.signedness = is_signed;

  // <! not support bias0/1 setting compress by hw
  //bm_vlc_est_weight_bias(src_data, in_size, (bool)is_signed, (bool)data_type, &cmd_info);
  bm_vlc_enc_int8(src_data, in_size, gmem_data, &total_size, &cmd_info);

  put_compressed_tg_gmem(ctx, p->dec_p.src, gmem_data, total_size);
  bmk1822_tdma_g2l_tensor_copy_decompressed(bmk, &p->dec_p);
  test_submit(ctx);

  dst->zero_guard_en = cmd_info.zero_guard_en;
  dst->bias0 = cmd_info.bias0;
  dst->bias1 = cmd_info.bias1;
  p->com_p.dst = dst;
  bmk1822_tdma_l2g_tensor_copy_compressed(bmk, &p->com_p);
  test_submit(ctx);

  u8 *dst_data = get_compressed_tg_gmem(ctx, p->com_p.dst);

  for (u64 i = 0; i < total_size ; i++) {
    if (dst_data[i] != gmem_data[i]) {
      fprintf(stderr, "vlc compress comparing failed at dst[%" PRIu64 "], got %d, exp %d\n",
              i, dst_data[i], gmem_data[i]);
      exit(-1);
    }
  }

  free(src_data);
  free(dst_data);
  free(gmem_data);
}

static void destroy_param_g2l(bmctx_t *ctx, bmk_ctx_t *bmk, param_t *p)
{
  free_compressed_tg_gmem(ctx, p->dec_p.src);
  free_compressed_tg_gmem(ctx, p->com_p.dst);
  free_tl(bmk, p->dec_p.dst);
}

static void test_one_case(bmctx_t *ctx, bmk_ctx_t *bmk, case_t *c)
{
  fmt_t fmts[2] = { FMT_I8, FMT_U8 };

  for (int align = 0; align < 2; align++) {
    for (u8 fmt_i = 0; fmt_i < 2; fmt_i++) {
      fmt_t fmt = fmts[fmt_i];

      param_t p;
      memset(&p, 0, sizeof(p));
      p.dec_p.src = alloc_vlc_compressed_tg_gmem(ctx,
          &c->lmem_shape, fmt);
      p.dec_p.dst = alloc_tl(bmk, c->lmem_shape, fmt, align);
      assert(p.dec_p.dst);

      p.com_p.src = p.dec_p.dst; //alloc_tl(bmk, c->lmem_shape, fmt, align);
      assert(p.com_p.src);
      compressed_tg_t* dst = alloc_vlc_compressed_tg_gmem(ctx,
          &c->lmem_shape, fmt);

      test_param_g2l(ctx, bmk, &p, dst);
      destroy_param_g2l(ctx, bmk, &p);
    }
  }
}

int main()
{
  bmctx_t ctx;
  bmk_ctx_t *bmk;
  test_init(&ctx, &bmk);

  u32 nr_cases = sizeof(g_cases) / sizeof(g_cases[0]);
  for (u32 i = 0; i < nr_cases; i++)
    test_one_case(&ctx, bmk, &g_cases[i]);

  test_exit(&ctx);
  return 0;
}
