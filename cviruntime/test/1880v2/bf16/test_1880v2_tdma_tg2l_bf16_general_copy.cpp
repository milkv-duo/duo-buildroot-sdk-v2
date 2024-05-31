#include "../1880v2_test_util.h"
#include "1880v2_bf16_util.h"

typedef bmk1880v2_tdma_tg2l_bf16_general_copy_param_t param_t;

static void __print_param(const char *tag, FILE *f, param_t *p)
{
  fprintf(
      f, "%s: %u bytes from %u:%lx to %x\n", tag,
      p->src_bytes, p->src_base_reg_index, p->src_address, p->dst_address);
}

#define print_param(f, p) __print_param(__func__, f, p)

typedef param_t case_t;

static fmt_type input_fmt[] = {
  {FMT_BF16, FMT_BF16},
};

static case_t g_cases[] = {
  { 0, 0, 0, 1 * 2 , FMT_F32, FMT_F32 },
  { 0, 0, 0, 39 * 2, FMT_F32, FMT_F32 },
  { 0, 0, 0, 4096 * 2, FMT_F32, FMT_F32 },
  { 0, 1, 0, 1 * 2, FMT_F32, FMT_F32 },
  { 0, 1, 0, 39 * 2, FMT_F32, FMT_F32 },
  { 0, 1, 0, 4096 * 2, FMT_F32, FMT_F32 },
  { 0, 1, 100, 1 * 2, FMT_F32, FMT_F32 },
  { 0, 1, 200, 39 * 2, FMT_F32, FMT_F32 },
  { 0, 1, 4096, 4096 * 2, FMT_F32, FMT_F32 },
  { 0, 257, 100, 1 * 2, FMT_F32, FMT_F32 },
  { 0, 349, 200, 39 * 2, FMT_F32, FMT_F32 },
  { 0, 3356, 4096, 4096 * 2, FMT_F32, FMT_F32 },
};

static void tg2l_general_copy_ref(param_t *p, u16 ref_data[], u16 src_data[])
{
  for (u32 i = 0; i < p->src_bytes/2; i++)
    ref_data[i] = src_data[i];
}

static void test_param_g2l(CVI_RT_HANDLE *ctx, bmk_ctx_t *bmk, param_t *p)
{
  print_param(stderr, p);
  u64 size = p->src_bytes/2;
  float val = -100;
  u16 *src_data = (u16 *)malloc(sizeof(u16) * size);
  for (u64 i = 0; i < size; i++) {
    src_data[i] = generate_bf16_corner_val(val);
    val += 0.1;
  }

  CVI_RT_MEM mem = CVI_RT_MemAlloc(*ctx, size * 2);
  u64 gmem_addr = CVI_RT_MemGetPAddr(mem);
  put_bytes_gmem(ctx, mem, (u8*)src_data);

  p->src_address = gmem_addr;
  bmk1880v2_tdma_g2l_bf16_general_copy(bmk, p);
  test_submit(ctx);
  CVI_RT_MemFree(*ctx, mem);


  u16 *dst_data = (u16*) get_bytes_l2g(ctx, bmk, p->dst_address, size * 2);

  u16 *ref_data = (u16 *)malloc(sizeof(u16) * size);
  tg2l_general_copy_ref(p, ref_data, src_data);

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
  u32 nr_fmt = sizeof(input_fmt) / sizeof(input_fmt[0]);
  for (u32 i = 0; i < nr_fmt; i++) {
    p->src_fmt = input_fmt[i].src_fmt;
    p->dst_fmt = input_fmt[i].dst_fmt;
    test_param_g2l(ctx, bmk, p);
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
