#include "../1822_test_util.h"
#include "../bm_vlc_compress.h"

typedef bmk1822_tdma_tg2l_tensor_copy_decompressed_param_t param_t;

static void __print_param(const char *tag, FILE *f, param_t *p)
{
  fprintf(
      f, "%s: (%u, %u, %u, %u) => fmt(%d) bias0/1/zero is (%u/%u/%u) %s\n",
      tag,
      p->dst->shape.n, p->dst->shape.c, p->dst->shape.h, p->dst->shape.w,
      p->dst->fmt,
      p->src->bias0, p->src->bias1, p->src->zero_guard_en,
      (p->dst->fmt == FMT_I8)? "signed": "unsigned");
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

static void test_param_g2l(bmctx_t *ctx, bmk_ctx_t *bmk, param_t *p, u16 *src_data, CommandInfo* cmd_info)
{
  print_param(stderr, p);
  u64 size = tl_shape_size(&p->dst->shape);
  size_t bs_size = 0;
  int is_signed = (p->dst->fmt == FMT_I8);
  u8 data_type = (p->dst->fmt == FMT_BF16) ? 1 : 0;
  u64 bytesize = size * bytesize_of_fmt(p->dst->fmt);

  u8 *bsbuf = vlc_compress((u8 *)src_data, bytesize, is_signed, data_type, &bs_size, cmd_info, NULL);

  u16 *ref_data = (u16 *)malloc(sizeof(u16) * size);
  bm_vlc_dec_bf16(bsbuf, bytesize, (u16* )ref_data);

  put_compressed_tg_gmem(ctx, p->src, bsbuf, bs_size);
  bmk1822_tdma_g2l_tensor_copy_decompressed(bmk, p);
  test_submit(ctx);

  u16 *dst_data = (u16* )get_bf16_tensor_l2g(ctx, bmk, p->dst, p->dst->fmt);


  for (u64 i = 0; i < size; i++) {
    if (dst_data[i] != ref_data[i]) {
      fprintf(stderr, "vlc decompress comparing failed at dst[%" PRIu64 "], got %d, exp %d\n",
          i, dst_data[i], ref_data[i]);
      exit(-1);
    }
  }

  free(bsbuf);
  free(dst_data);
  free(ref_data);
}

static void destroy_param_g2l(bmctx_t *ctx, bmk_ctx_t *bmk, param_t *p)
{
  free_compressed_tg_gmem(ctx, p->src);
  free_tl(bmk, p->dst);
}

static void test_one_case(bmctx_t *ctx, bmk_ctx_t *bmk, case_t *c)
{
  fmt_t fmts[] = { FMT_BF16 };
  u8 fmts_sz = sizeof(fmts) / sizeof(fmts[0]);

  for (int dst_align = 0; dst_align < 2; dst_align++) {
    for (int mode = 0; mode < VLC_CMP_MODE_MAX; mode++) {
      for (u8 fmt_i = 0; fmt_i < fmts_sz; fmt_i++) {
        fmt_t fmt = fmts[fmt_i];
        param_t p;
        memset(&p, 0, sizeof(p));
        p.dst = alloc_tl(bmk, c->lmem_shape, fmt, dst_align);
        assert(p.dst);

        u64 size = tl_shape_size(&p.dst->shape);
        u16 *src_data = (u16 *)malloc(sizeof(u16) * size);
        vlc_init_testdata(src_data, size, fmt == FMT_I8, fmt == FMT_BF16);

        CommandInfo cmd_info;
        memset(&cmd_info, 0, sizeof(CommandInfo));
        int is_signed = (fmt == FMT_I8);
        u8 data_type = (fmt == FMT_BF16) ? 1 : 0;

        cmd_info.signedness = is_signed;
        cmd_info.is_bfloat16 = data_type;

        if (mode == VLC_CMP_MODE_COMPILER) {
          bm_vlc_est_weight_bias((u8* )src_data, size * sizeof(u16), (bool)is_signed, (bool)data_type, &cmd_info);
        }

        p.src = _alloc_vlc_compressed_tg_gmem(ctx, &c->lmem_shape, fmt, &cmd_info);

        test_param_g2l(ctx, bmk, &p, src_data, &cmd_info);

        free(src_data);
        destroy_param_g2l(ctx, bmk, &p);
      }
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
