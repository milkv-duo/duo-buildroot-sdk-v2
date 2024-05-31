#include "1822_test_util.h"

typedef bmk1822_tdma_l2tg_tensor_copy_compressed_param_t param_t;

static void __print_param(const char *tag, FILE *f, param_t *p)
{
  fprintf(
      f, "%s: (%u, %u, %u, %u) => %d-bit %s\n",
      tag,
      p->src->shape.n, p->src->shape.c, p->src->shape.h, p->src->shape.w,
      p->dst->bit_length,
      (p->src->fmt == FMT_I8)? "signed": "unsigned");
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

static u64 l2tg_tensor_copy_vlc_compressed_ref(
    param_t *p, u8 ref_data[], u8 src_data[], CommandInfo *cmd_info)
{
  u64 in_size = tl_shape_size(&p->src->shape);
  size_t bs_size = 0;

  bm_vlc_enc_int8(src_data, in_size, ref_data, &bs_size, cmd_info);
  return bs_size;
}

static int test_param_l2g(bmctx_t *ctx, bmk_ctx_t *bmk, param_t *p, CommandInfo* cmd_info_est, u8 *src_data)
{
  print_param(stderr, p);
  int ret = 0;
  u64 size = tl_shape_size(&p->src->shape);

  put_tensor_g2l(ctx, bmk, p->src, src_data);
  bmk1822_tdma_l2g_tensor_copy_compressed(bmk, p);
  test_submit(ctx);

  u8 *dst_data = get_compressed_tg_gmem(ctx, p->dst);
  u8 *ref_data = (u8 *)malloc(sizeof(u8) * p->dst->reserved_size); //<! bs_buf_size
  if (!dst_data || !ref_data) {
    ret = -1;
    goto fail_exit;
  }

  size = l2tg_tensor_copy_vlc_compressed_ref(p, ref_data, src_data, cmd_info_est);

  for (u64 i = 0; i < size ; i++) {
    if (dst_data[i] != ref_data[i]) {
      fprintf(stderr, "vlc compress comparing failed at dst[%" PRIu64 "], got %d, exp %d\n",
              i, dst_data[i], ref_data[i]);

      ret = -1;
      break;
    }
  }

fail_exit:
  free(dst_data);
  free(ref_data);

  return ret;
}

static void destroy_param_l2g(bmctx_t *ctx, bmk_ctx_t *bmk, param_t *p)
{
  free_compressed_tg_gmem(ctx, p->dst);
  free_tl(bmk, p->src);
}

static int test_one_case(bmctx_t *ctx, bmk_ctx_t *bmk, case_t *c)
{
  int ret = 0;
  fmt_t fmts[] = { FMT_I8, FMT_U8 };

  for (int src_align = 0; src_align < 2; src_align++) {
    for (u8 fmt_i = 0; fmt_i < 2; fmt_i++) {
      fmt_t fmt = fmts[fmt_i];
      param_t p;
      memset(&p, 0, sizeof(p));

      p.src = alloc_tl(bmk, c->lmem_shape, fmt, src_align);
      assert(p.src);

      CommandInfo cmd_info;
      memset(&cmd_info, 0, sizeof(CommandInfo));
      u64 in_size = tl_shape_size(&p.src->shape);

      u8 *src_data = (u8 *)malloc(sizeof(u8) * in_size);
      vlc_init_testdata(src_data, in_size, fmt == FMT_I8, fmt == FMT_BF16);

      int is_signed = (p.src->fmt == FMT_I8);
      cmd_info.signedness = is_signed;

      // <! not support bias0/1 setting compress by hw
      //bm_vlc_est_weight_bias(src_data, in_size, (bool)is_signed, (bool)data_type, &cmd_info);

      p.dst = _alloc_vlc_compressed_tg_gmem(ctx,
          &c->lmem_shape, fmt, &cmd_info);
      ret |= test_param_l2g(ctx, bmk, &p, &cmd_info, src_data);
      destroy_param_l2g(ctx, bmk, &p);

      free(src_data);
    }
  }

  return ret;
}

int main()
{
  int ret = 0;
  bmctx_t ctx;
  bmk_ctx_t *bmk;
  test_init(&ctx, &bmk);

  u32 nr_cases = sizeof(g_cases) / sizeof(g_cases[0]);
  for (u32 i = 0; i < nr_cases; i++)
    ret |= test_one_case(&ctx, bmk, &g_cases[i]);

  test_exit(&ctx);
  return ret;
}
