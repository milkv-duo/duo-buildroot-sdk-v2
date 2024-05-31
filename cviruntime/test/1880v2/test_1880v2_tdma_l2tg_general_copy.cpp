#include "1880v2_test_util.h"

typedef bmk1880v2_tdma_l2tg_general_copy_param_t param_t;

static void __print_param(const char *tag, FILE *f, param_t *p)
{
  fprintf(
      f, "%s: %u bytes from %x to %u:%lx\n", tag,
      p->bytes, p->src_address, p->dst_base_reg_index, p->dst_address);
}

#define print_param(f, p) __print_param(__func__, f, p)

typedef param_t case_t;

static case_t g_cases[] = {
  { 0, 0, 0, 1 },
  { 0, 0, 0, 39 },
  { 0, 0, 0, 4096 },
  { 0, 0, 100, 1 },
  { 0, 0, 200, 39 },
  { 0, 0, 1024, 4096 },
  { 39, 0, 100, 1 },
  { 47, 0, 200, 39 },
  { 2048, 0, 1024, 4096 },
};

static void l2tg_general_copy_ref(param_t *p, u8 ref_data[], u8 src_data[])
{
  for (u32 i = 0; i < p->bytes; i++)
    ref_data[i] = src_data[i];
}

static void test_param_l2g(CVI_RT_HANDLE *ctx, bmk_ctx_t *bmk, param_t *p)
{
  print_param(stderr, p);
  u64 size = p->bytes;

  u8 *src_data = (u8 *)malloc(sizeof(u8) * size);
  for (u64 i = 0; i < size; i++)
    src_data[i] = 200 + i;

  put_bytes_g2l(ctx, bmk, p->src_address, size, src_data);

  #if 1
  u8 *dst_data = get_bytes_l2g(ctx, bmk, p->src_address, size);

  #else
  bmk1880v2_tdma_l2g_general_copy(bmk, p);
  test_submit(ctx);
  u8 *dst_data = get_bytes_gmem(ctx, p->dst_address, size);
  #endif

  u8 *ref_data = (u8 *)malloc(sizeof(u8) * size);
  l2tg_general_copy_ref(p, ref_data, src_data);

  for (u64 i = 0; i < size; i++) {
    if (dst_data[i] != ref_data[i]) {
      fprintf(stderr, "comparing failed at dst[%" PRIu64 "], got %d, exp %d\n",
              i, dst_data[i], ref_data[i]);
      exit(-1);
    }
  }

  free(src_data);
  free(dst_data);
  free(ref_data);
}


static void test_one_case(CVI_RT_HANDLE *ctx, bmk_ctx_t *bmk, case_t *c)
{
  param_t *p = c;

  test_param_l2g(ctx, bmk, p);
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
