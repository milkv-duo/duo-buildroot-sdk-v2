#include "1822_test_util.h"

typedef bmk1822_tdma_tg2l_tensor_copy_chw_rotated_param_t param_t;

static void __print_param(const char *tag, FILE *f, param_t *p)
{
  fprintf(
      f, "%s: (%u, %u, %u, %u) => (%u, %u, %u, %u)\n",
      tag,
      p->src->shape.n, p->src->shape.h, p->src->shape.w, p->src->shape.c,
      p->dst->shape.n, p->dst->shape.c, p->dst->shape.h, p->dst->shape.w);
}

#define print_param(f, p) __print_param(__func__, f, p)

typedef struct {
  tg_shape_t src_shape;
  tl_shape_t dst_shape;
} case_t;

static case_t g_cases[] = {
  {
    { 1, 3, 1, 1 }, // nchw for neuron
    { 1, 3, 1, 1 }, // nchw for neuron
  }, {
    { 1, 4, 1, 1 },
    { 1, 4, 1, 1 },
  }, {
    { 1, 3, 1, 7 },
    { 1, 3, 1, 7 },
  }, {
    { 1, 4, 1, 7 },
    { 1, 4, 1, 7 },
  }, {
    { 1, 3, 1, 17 },
    { 1, 3, 1, 17 },
  }, {
    { 1, 4, 1, 17 },
    { 1, 4, 1, 17 },
  }, {
    { 1, 3, 2, 1 },
    { 1, 3, 2, 1 },
  }, {
    { 1, 4, 2, 1 },
    { 1, 4, 2, 1 },
  }, {
    {  2, 3, 17, 1 },
    {  2, 3, 17, 1 },
  }, {
    {  2, 4, 17, 1 },
    {  2, 4, 17, 1 },
  }, {
    {  2, 3, 17, 3 },
    {  2, 3, 17, 3 },
  }, {
    {  2, 4, 17, 3 },
    {  2, 4, 17, 3 },
  }, {
    {  3, 3, 16, 7 },
    {  3, 3, 16, 7 },
  }, {
    {  3, 4, 16, 7 },
    {  3, 4, 16, 7 },
  }, {
    {  3, 3, 39, 17 },
    {  3, 3, 39, 17 },
  }, {
    {  3, 4, 39, 17 },
    {  3, 4, 39, 17 },
  }, {
    {  3, 3, 36, 16 },
    {  3, 3, 36, 16 },
  }, {
    {  3, 4, 36, 16 },
    {  3, 4, 36, 16 },
  }, {
    {  5, 3, 39, 17 },
    {  5, 3, 39, 17 },
  }, {
    {  5, 4, 39, 17 },
    {  5, 4, 39, 17 },
  }, {
    { 20, 3, 35, 2 },
    { 20, 3, 35, 2 },
  }, {
    { 20, 4, 35, 2 },
    { 20, 4, 35, 2 },
  }, {
    { 20, 3, 35, 3 },
    { 20, 3, 35, 3 },
  }, {
    { 20, 4, 35, 3 },
    { 20, 4, 35, 3 },
  }
};

static void tg2l_tensor_copy_chw_rotated_ref(
    param_t *p, u8 ref_data[], u8 src_data[])
{
  tg_shape_t s = p->src->shape;
  // change nhwc -> nchw by HW design automatically
  u32 n = s.n;
  u32 c = s.h;
  u32 h = s.w;
  u32 w = s.c;
  for (u32 ni = 0; ni < n; ni++) {
    for (u32 ci = 0; ci < c; ci++) {
      for (u32 hi = 0; hi < h; hi++) {
        for (u32 wi = 0; wi < w; wi++) {
          u64 src_i = ni * c * h * w + ci * h * w + hi * w + wi;
          u64 dst_i = ni * w * c * h + wi * c * h + ci * h + hi;
          ref_data[dst_i] = src_data[src_i];
        }
      }
    }
  }
}

static void test_param_g2l(bmctx_t *ctx, bmk_ctx_t *bmk, param_t *p)
{
  print_param(stderr, p);
  u64 size = tg_shape_size(&p->src->shape);
  u8 *src_data = (u8 *)malloc(sizeof(u8) * size);
  for (u64 i = 0; i < size; i++)
    src_data[i] = 200 + i;

  put_tg_gmem(ctx, p->src, src_data);
  bmk1822_tdma_g2l_tensor_copy_chw_rotated(bmk, p);
  test_submit(ctx);
  u8 *dst_data = get_tensor_l2g(ctx, bmk, p->dst);

  u8 *ref_data = (u8 *)malloc(sizeof(u8) * size);
  tg2l_tensor_copy_chw_rotated_ref(p, ref_data, src_data);
 
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

static void destroy_param_g2l(bmctx_t *ctx, bmk_ctx_t *bmk, param_t *p)
{
  free_tg_gmem(ctx, p->src);
  free_tl(bmk, p->dst);
}

static void test_one_case(bmctx_t *ctx, bmk_ctx_t *bmk, case_t *c)
{

  param_t p;
  memset(&p, 0, sizeof(p));

  p.src = alloc_tg_gmem(ctx, c->src_shape, FMT_I8);
  p.dst = alloc_tl(bmk, c->dst_shape, FMT_I8, 1);
  test_param_g2l(ctx, bmk, &p);
  destroy_param_g2l(ctx, bmk, &p);

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
