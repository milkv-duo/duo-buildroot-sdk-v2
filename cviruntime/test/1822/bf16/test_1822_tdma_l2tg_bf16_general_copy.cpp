#include "../1822_test_util.h"
#include "1822_bf16_util.h"

typedef bmk1822_tdma_l2tg_bf16_general_copy_param_t param_t;

static void __print_param(const char *tag, FILE *f, param_t *p)
{
  fprintf(
      f, "%s: %u bytes from %" PRIu32 " to %u:%" PRIx64 "\n", tag,
      p->src_bytes, p->src_address, p->dst_base_reg_index, p->dst_address);
}

#define print_param(f, p) __print_param(__func__, f, p)

typedef param_t case_t;

static fmt_type input_fmt[] = {
  {FMT_BF16, FMT_BF16},
};

static case_t g_cases[] = {
  { 0, 0, 0, 1 * 2, FMT_F32, FMT_F32 },
  { 0, 0, 0, 39 * 2, FMT_F32, FMT_F32 },
  { 0, 0, 0, 4096 * 2, FMT_F32, FMT_F32 },
  { 0, 0, 100, 1 * 2, FMT_F32, FMT_F32 },
  { 0, 0, 200, 39 * 2, FMT_F32, FMT_F32 },
  { 0, 0, 1024, 4096 * 2, FMT_F32, FMT_F32 },
  { 39, 0, 100, 1 * 2, FMT_F32, FMT_F32 },
  { 47, 0, 200, 39 * 2, FMT_F32, FMT_F32 },
  { 2048, 0, 1024, 4096 * 2, FMT_F32, FMT_F32 },
};

static void l2tg_general_copy_ref(param_t *p, u16 ref_data[], u16 src_data[])
{
  for (u32 i = 0; i < p->src_bytes/2; i++)
    ref_data[i] = src_data[i];
}

static void test_param_l2g(bmctx_t *ctx, bmk_ctx_t *bmk, param_t *p)
{
  print_param(stderr, p);
  u64 size = p->src_bytes/2 ;
  u16 *src_data = (u16 *)malloc(sizeof(u16) * size);
  static float val = -100;
  for (u64 i = 0; i < size; i++) {
    src_data[i] = generate_bf16_corner_val(val);
    val += 0.1;
  }
  put_bytes_g2l(ctx, bmk, p->src_address, size * 2, (u8*)src_data);

  bmk1822_tdma_l2g_bf16_general_copy(bmk, p);
  test_submit(ctx);
  //u16 *dst_data = (u16*) get_bytes_gmem(ctx, p->dst_address, size * 2);
  u16 *dst_data = (u16*)get_bytes_l2g(ctx, bmk, p->src_address, size * 2);

  u16 *ref_data = (u16 *)malloc(sizeof(u16) * size);
  l2tg_general_copy_ref(p, ref_data, src_data);
  for (u64 i = 0; i < size; i++) {
    if (dst_data[i] != ref_data[i]) {
      exit(-1);
    }
  }
  free(src_data);
  free(dst_data);
  free(ref_data);
}


static void test_one_case(bmctx_t *ctx, bmk_ctx_t *bmk, case_t *c)
{
  param_t *p = c;
  u32 nr_fmt = sizeof(input_fmt) / sizeof(input_fmt[0]);
  for (u32 i = 0; i < nr_fmt; i++) {
    p->src_fmt = input_fmt[i].src_fmt;
    p->dst_fmt = input_fmt[i].dst_fmt;
    test_param_l2g(ctx, bmk, p);
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
