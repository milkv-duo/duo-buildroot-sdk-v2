#include "1880v2_test_util.h"

typedef bmk1880v2_tdma_tg2tg_tensor_copy_param_t param_t;

static void __print_param(const char *tag, FILE *f, param_t *p)
{
  fprintf(
      f, "%s: (%u, %u, %u, %u) => (%u, %u, %u, %u)\n",
      tag,
      p->src->shape.n, p->src->shape.c, p->src->shape.h, p->src->shape.w,
      p->dst->shape.n, p->dst->shape.c, p->dst->shape.h, p->dst->shape.w);
}

#define print_param(f, p) __print_param(__func__, f, p)

typedef struct {
  tg_shape_t src_shape;
  tg_stride_t src_stride;
  tg_shape_t dst_shape;
  tg_stride_t dst_stride;
} case_t;

static case_t g_cases[] = {
  {
    {1, 3, 3, 3}, {27, 9, 3},
    {1, 3, 3, 3}, {27, 9, 3},
  },
  {
    // YOLOv2 concat layer
    {1, 256, 19, 19}, {92416, 361, 19},
    {1, 256, 19, 19}, {462080, 361, 19},
  }
};


static void test_param_g2g(CVI_RT_HANDLE *ctx, bmk_ctx_t *bmk, param_t *p)
{
  print_param(stderr, p);

  u64 size = p->src->shape.c * p->src->shape.h * p->src->shape.w;
  u8 *src_data = (u8 *)malloc(sizeof(u8) * size);
  for (u64 i = 0; i < size; i++)
    src_data[i] = 200 + i;

  put_tg_gmem(ctx, p->src, src_data);

  bmk1880v2_tdma_tg2tg_general_copy(bmk, p);

  test_submit(ctx);

  u8 *dst_data = get_tg_gmem(ctx, p->dst);

  for (u64 i = 0; i < size; i++) {
    if (dst_data[i] != src_data[i]) {
      fprintf(stderr, "comparing failed at dst[%" PRIu64 "], got %d, exp %d\n",
              i, dst_data[i], src_data[i]);
      exit(-1);
    }
  }

  free(src_data);
  free(dst_data);
}

static void destroy_param_g2g(CVI_RT_HANDLE *ctx, param_t *p)
{
  free_tg_gmem(ctx, p->src);
  free_tg_gmem(ctx, p->dst);
}

static void test_one_case(CVI_RT_HANDLE *ctx, bmk_ctx_t *bmk, case_t *c)
{
    param_t p;
    bmk1880v2_tensor_tgmem_t *src, *dst;

    src = alloc_tg_gmem(ctx, c->src_shape, FMT_I8);
    src->stride.n = c->src_stride.n;
    src->stride.c = c->src_stride.c;
    src->stride.h = c->src_stride.h;

    dst = alloc_tg_gmem(ctx, c->dst_shape, FMT_I8);
    dst->stride.n = c->dst_stride.n;
    dst->stride.c = c->dst_stride.c;
    dst->stride.h = c->dst_stride.h;

    memset(&p, 0, sizeof(p));
    p.src = src;
    p.dst = dst;
    test_param_g2g(ctx, bmk, &p);

    destroy_param_g2g(ctx, &p);
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
