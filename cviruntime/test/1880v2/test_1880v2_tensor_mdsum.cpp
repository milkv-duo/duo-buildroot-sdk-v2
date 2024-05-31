#include "1880v2_test_util.h"

typedef struct {
  bmk1880v2_tiu_mdsum_param_t p;
  s8 *res;
  s8 *input;
} mdsum_case_t;

static void destroy_mdsum_param(
    bmk_ctx_t *bk_ctx,
    bmk1880v2_tiu_mdsum_param_t *p)
{
  free_tl(bk_ctx, p->res);
  free_tl(bk_ctx, p->input);
}

static void destroy_mdsum_case(
    bmk_ctx_t *bk_ctx,
    mdsum_case_t *mc)
{
  destroy_mdsum_param(bk_ctx, &mc->p);
  free(mc->res);
  free(mc->input);
}

static void mdsum_case_ref(mdsum_case_t *mc)
{
  bmk1880v2_tiu_mdsum_param_t *p = &mc->p;
  int n = p->input->shape.n;
  int c = p->input->shape.c;
  int h = p->input->shape.h;
  int w = p->input->shape.w;
  int res_sign = (p->res->fmt == FMT_I8);

  s32 *tmp_res = (s32 *)xmalloc(c * sizeof(s32));
  for (int i = 0; i < c; i++)
    tmp_res[i] = 0;

  for (int ni = 0; ni < n; ni++) {
    for (int ci = 0; ci < c; ci++) {
      for (int hi = 0; hi < h; hi++) {
        for (int wi = 0; wi < w; wi++) {
          u64 h_size = w;
          u64 c_size = h * h_size;
          u64 n_size = c * c_size;
          u64 i = ni * n_size + ci * c_size + hi * h_size + wi;
          s8 input = mc->input[i];
          tmp_res[ci] += res_sign? input: (u8)input;
        }
      }
    }
  }

  int arith_shift = (p->res->fmt == FMT_I8);
  if (arith_shift)
    arith_right_shift(tmp_res, c, p->rshift_bits, 1);
  else
    logic_right_shift(tmp_res, c, p->rshift_bits, 1);

  if (p->res_is_int8)
    saturate_to_int8(tmp_res, c, res_sign);
  else
    saturate_to_int16(tmp_res, c, res_sign);

  for (int i = 0; i < c; i++)
    mc->res[i] = tmp_res[i];

  if (!p->res_is_int8)
    for (int i = 0; i < c; i++)
      mc->res[c + i] = tmp_res[i] >> 8;

  free(tmp_res);
}

static void execute_mdsum_case(
    CVI_RT_HANDLE *ctx,
    bmk_ctx_t *bk_ctx,
    mdsum_case_t *mc)
{
  bmk1880v2_tiu_mdsum_param_t *p = &mc->p;

  put_tensor_g2l(ctx, bk_ctx, p->input, (u8 *)mc->input);
  bmk1880v2_tiu_mdsum(bk_ctx, p);
  u8 *res = get_tensor_l2g(ctx, bk_ctx, p->res);

  mdsum_case_ref(mc);

  int size = p->input->shape.c;
  if (!p->res_is_int8)
    size *= 2;

  for (int i = 0; i < size; i++) {
    if ((s8)res[i] != mc->res[i]) {
      fprintf(stderr, "comparing failed at res[%d], got %d, exp %d\n",
              i, (s8)res[i], mc->res[i]);
      exit(-1);
    }
  }

  free(res);
}

static void init_mdsum_case_0(bmk_ctx_t *bk_ctx, mdsum_case_t *mc)
{
  int n = 4;
  int c = 16;
  int h = 1;
  int w = 17;

  tl_shape_t a_shape;
  a_shape.n = n;
  a_shape.c = c;
  a_shape.h = h;
  a_shape.w = w;

  tl_shape_t res_shape;
  res_shape.n = 1;
  res_shape.c = c;
  res_shape.h = 1;
  res_shape.w = 1;

  mc->p.res_is_int8 = 1;
  mc->p.input = alloc_tl(bk_ctx, a_shape, FMT_I8, 1);
  mc->p.res = alloc_tl(bk_ctx, res_shape, FMT_I8, 0);
  mc->p.rshift_bits = 3;

  u64 input_size = n * c * h * w;
  mc->input = (s8 *)xmalloc(input_size);
  for (u64 i = 0; i < input_size; i++)
    mc->input[i] = (i % 13) - (i % 17) + (i % 5) - (i % 3);

  u64 res_size = c * 2;
  mc->res = (s8 *)xmalloc(res_size);
}

static void init_mdsum_case_1(bmk_ctx_t *bk_ctx, mdsum_case_t *mc)
{
  int n = 4;
  int c = 16;
  int h = 1;
  int w = 17;

  tl_shape_t a_shape;
  a_shape.n = n;
  a_shape.c = c;
  a_shape.h = h;
  a_shape.w = w;

  tl_shape_t res_shape;
  res_shape.n = 2;
  res_shape.c = c;
  res_shape.h = 1;
  res_shape.w = 1;

  mc->p.res_is_int8 = 0;
  mc->p.input = alloc_tl(bk_ctx, a_shape, FMT_I8, 1);
  mc->p.res = alloc_tl(bk_ctx, res_shape, FMT_I8, 0);
  mc->p.rshift_bits = 3;

  u64 input_size = n * c * h * w;
  mc->input = (s8 *)xmalloc(input_size);
  for (u64 i = 0; i < input_size; i++)
    mc->input[i] = (i % 13) - (i % 17) + i - 30;

  u64 res_size = c * 2;
  mc->res = (s8 *)xmalloc(res_size);
}

static void init_mdsum_case_2(bmk_ctx_t *bk_ctx, mdsum_case_t *mc)
{
  int n = 4;
  int c = 16;
  int h = 1;
  int w = 17;

  tl_shape_t a_shape;
  a_shape.n = n;
  a_shape.c = c;
  a_shape.h = h;
  a_shape.w = w;

  tl_shape_t res_shape;
  res_shape.n = 2;
  res_shape.c = c;
  res_shape.h = 1;
  res_shape.w = 1;

  mc->p.res_is_int8 = 0;
  mc->p.input = alloc_tl(bk_ctx, a_shape, FMT_U8, 1);
  mc->p.res = alloc_tl(bk_ctx, res_shape, FMT_U8, 0);
  mc->p.rshift_bits = 3;

  u64 input_size = n * c * h * w;
  mc->input = (s8 *)xmalloc(input_size);
  for (u64 i = 0; i < input_size; i++)
    mc->input[i] = (i % 13) - (i % 17) + i - 30;

  u64 res_size = c * 2;
  mc->res = (s8 *)xmalloc(res_size);
}

static void test_tl_mdsum(CVI_RT_HANDLE *ctx, bmk_ctx_t *bk_ctx)
{
  mdsum_case_t mc;

  init_mdsum_case_0(bk_ctx, &mc);
  execute_mdsum_case(ctx, bk_ctx, &mc);
  destroy_mdsum_case(bk_ctx, &mc);

  init_mdsum_case_1(bk_ctx, &mc);
  execute_mdsum_case(ctx, bk_ctx, &mc);
  destroy_mdsum_case(bk_ctx, &mc);

  init_mdsum_case_2(bk_ctx, &mc);
  execute_mdsum_case(ctx, bk_ctx, &mc);
  destroy_mdsum_case(bk_ctx, &mc);
}

int main()
{
  CVI_RT_HANDLE ctx;
  bmk_ctx_t *bk_ctx;
  test_init(&ctx, &bk_ctx);
  test_tl_mdsum(&ctx, bk_ctx);
  test_exit(&ctx);
  return 0;
}
