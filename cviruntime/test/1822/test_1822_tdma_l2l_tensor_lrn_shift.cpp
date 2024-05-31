#include "1822_test_util.h"

typedef bmk1822_tdma_l2l_tensor_lrn_shift_param_t param_t;

#define ENABLE_TV_GEN_PATTERN // to reduce rom code size for RTL_SIM (rom size 128KB)

static void __print_param(const char *tag, FILE *f, param_t *p)
{
  fprintf(
      f, "%s: (%u, %u, %u, %u) %s%u%s (%u, %u, %u, %u)\n",
      tag,
      p->src->shape.n, p->src->shape.c, p->src->shape.h, p->src->shape.w,
      (p->right_shift? "": "<-"),
      p->lrn_step,
      (p->right_shift? "->": ""),
      p->dst->shape.n, p->dst->shape.c, p->dst->shape.h, p->dst->shape.w);
}

#define print_param(f, p) __print_param(__func__, f, p)

typedef struct {
  u32 n;
  u32 c;
  u32 src_h;
  u32 src_w;
  u32 dst_h;
  u32 dst_w;
} case_t;

static case_t g_cases[] = {
#ifdef ENABLE_TV_GEN_PATTERN
  { 0, 0, 3, 7, 7, 3 },
  { 0, 0, 4, 4, 2, 8 },
  { 0, 0, 14, 6, 12, 7 },
#else
  { 0, 0, 1, 1, 1, 1 },
  { 0, 0, 3, 7, 7, 3 },
  { 0, 0, 4, 4, 2, 8 },
  { 0, 0, 7, 7, 1, 49 },
  { 0, 0, 7, 8, 14, 4 },
  { 0, 0, 14, 6, 12, 7 },
#endif
};

static void destroy_param(bmk_ctx_t *bmk, param_t *p)
{
  free_tl(bmk, p->dst);
  free_tl(bmk, p->src);
}

static void lrn_left_shift_ref(param_t *p, u8 ref_data[], u8 src_data[])
{
  u32 n = p->src->shape.n;
  u32 c = p->src->shape.c;
  u32 hw = p->src->shape.h * p->src->shape.w;
  u64 size = tl_shape_size(&p->src->shape);

  for (u64 i = 0; i < size; i++)
    ref_data[i] = 0;

  for (u32 ni = 0; ni < n; ni++) {
    for (u32 ci = p->lrn_step; ci < c; ci++) {
      for (u32 hwi = 0; hwi < hw; hwi++) {
        u32 src_i = (ni * c + ci) * hw + hwi;
        u32 dst_i = src_i - p->lrn_step * hw;
        ref_data[dst_i] = src_data[src_i];
      }
    }
  }
}

static void lrn_right_shift_ref(param_t *p, u8 ref_data[], u8 src_data[])
{
  u32 n = p->src->shape.n;
  u32 c = p->src->shape.c;
  u32 hw = p->src->shape.h * p->src->shape.w;
  u64 size = tl_shape_size(&p->src->shape);

  for (u64 i = 0; i < size; i++)
    ref_data[i] = 0;

  for (u32 ni = 0; ni < n; ni++) {
    for (u32 ci = 0; ci < c - p->lrn_step; ci++) {
      for (u32 hwi = 0; hwi < hw; hwi++) {
        u32 src_i = (ni * c + ci) * hw + hwi;
        u32 dst_i = src_i + p->lrn_step * hw;
        ref_data[dst_i] = src_data[src_i];
      }
    }
  }
}

static void l2l_tensor_lrn_shift_ref(
    param_t *p, u8 ref_data[], u8 src_data[])
{
  if (p->right_shift)
    return lrn_right_shift_ref(p, ref_data, src_data);
  else
    return lrn_left_shift_ref(p, ref_data, src_data);
}

static void test_param(bmctx_t *ctx, bmk_ctx_t *bmk, param_t *p)
{
  print_param(stderr, p);
  u64 size = tl_shape_size(&p->src->shape);

  u8 *src_data = (u8 *)malloc(sizeof(u8) * size);
  for (u64 i = 0; i < size; i++)
    src_data[i] = 200 + i;

  put_tensor_g2l(ctx, bmk, p->src, src_data);
  bmk1822_tdma_l2l_tensor_lrn_shift(bmk, p);
  u8 *dst_data = get_tensor_l2g(ctx, bmk, p->dst);

  u8 *ref_data = (u8 *)malloc(sizeof(u8) * size);
  l2l_tensor_lrn_shift_ref(p, ref_data, src_data);

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

static void execute_case(bmctx_t *ctx, bmk_ctx_t *bmk, case_t *c)
{
  static const u32 steps[] = { 1, 3 }; // less than npu_num/2
  u32 nr_steps = sizeof(steps) / sizeof(steps[0]);

  for (int src_align = 0; src_align < 2; src_align++) {
    for (int dst_align = 0; dst_align < 2; dst_align++) {
      tl_shape_t src_shape, dst_shape;
      src_shape.n = c->n;
      src_shape.c = c->c;
      src_shape.h = c->src_h;
      src_shape.w = c->src_w;
      dst_shape.n = c->n;
      dst_shape.c = c->c;
      dst_shape.h = c->dst_h;
      dst_shape.w = c->dst_w;

      param_t p;
      memset(&p, 0, sizeof(p));
      p.src = alloc_tl(bmk, src_shape, FMT_I8, src_align);
      p.dst = alloc_tl(bmk, dst_shape, FMT_I8, dst_align);

      for (u32 i = 0; i < nr_steps; i++) {
        if (steps[i] >= p.src->shape.c)
          break;
        p.lrn_step = steps[i];

        p.right_shift = 0;
        test_param(ctx, bmk, &p);

        p.right_shift = 1;
        test_param(ctx, bmk, &p);
      }

      destroy_param(bmk, &p);
    }
  }
}

static void test_one_case(bmctx_t *ctx, bmk_ctx_t *bmk, case_t *ca)
{
  for (u32 n = 1; n < 8; n += 2) {
    ca->n = n;
    for (u32 c = 1; c < BM1822_HW_NPU_NUM + 1; c += 3) {
      ca->c = c;
      execute_case(ctx, bmk, ca);
    }
    for (u32 c = BM1822_HW_NPU_NUM + 1; c < BM1822_HW_NPU_NUM * 2; c += 7) {
      ca->c = c;
      execute_case(ctx, bmk, ca);
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
