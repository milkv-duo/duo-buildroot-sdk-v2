#include "1822_test_util.h"

typedef bmk1822_tdma_tg2l_matrix_copy_row_col_transposed_param_t param_t;

static void __print_param(const char *tag, FILE *f, param_t *p)
{
  fprintf(
      f, "%s: (%u, %u) => (%u, %u, %u, %u)\n",
      tag,
      p->src->shape.row, p->src->shape.col,
      p->dst->shape.n, p->dst->shape.c,
      p->dst->shape.w, p->dst->shape.col);
}

#define print_param(f, p) __print_param(__func__, f, p)

typedef struct {
  mg_shape_t src_shape;
  ml_shape_t dst_shape;
} case_t;

static case_t g_cases[] = {
  {
    { 1, 1 },
    { 1, 1, 1, 1 },
  }, {
    { 1, 2 },
    { 2, 1, 1, 1 },
  }, {
    { 1, 7 },
    { 7, 1, 1, 1 },
  }, {
    { 1, 17 },
    { 17, 1, 1, 1 },
  }, {
    { 1, 60 },
    { 60, 1, 1, 1 },
  }, {
    { 1, 139 },
    { 139, 1, 1, 1 },
  }, {
    { 2, 1 },
    { 1, 1, 2, 2 },
  }, {
    { 2, 1 },
    { 1, 2, 1, 2 },
  }, {
    { 2, 2 },
    { 2, 1, 2, 2 },
  }, {
    { 2, 2 },
    { 2, 2, 1, 2 },
  }, {
    { 2, 7 },
    { 7, 1, 2, 2 },
  }, {
    { 2, 7 },
    { 7, 2, 1, 2 },
  }, {
    { 2, 17 },
    { 17, 1, 2, 2 },
  }, {
    { 2, 17 },
    { 17, 2, 1, 2 },
  }, {
    { 2, 60 },
    { 60, 1, 2, 2 },
  }, {
    { 2, 60 },
    { 60, 2, 1, 2 },
  }, {
    { 2, 139 },
    { 139, 1, 2, 2 },
  }, {
    { 2, 139 },
    { 139, 2, 1, 2 },
  }, {
    { 7, 1 },
    { 1, 1, 7, 7 },
  }, {
    { 7, 1 },
    { 1, 2, 4, 7 },
  }, {
    { 7, 1 },
    { 1, 2, 5, 7 },
  }, {
    { 7, 1 },
    { 1, 2, 6, 7 },
  }, {
    { 7, 1 },
    { 1, 3, 3, 7 },
  }, {
    { 7, 1 },
    { 1, 4, 2, 7 },
  }, {
    { 7, 1 },
    { 1, 7, 1, 7 },
  }, {
    { 7, 2 },
    { 2, 1, 7, 7 },
  }, {
    { 7, 2 },
    { 2, 2, 4, 7 },
  }, {
    { 7, 2 },
    { 2, 2, 5, 7 },
  }, {
    { 7, 2 },
    { 2, 2, 6, 7 },
  }, {
    { 7, 2 },
    { 2, 3, 3, 7 },
  }, {
    { 7, 2 },
    { 2, 4, 2, 7 },
  }, {
    { 7, 2 },
    { 2, 7, 1, 7 },
  }, {
    { 7, 7 },
    { 7, 1, 7, 7 },
  }, {
    { 7, 7 },
    { 7, 3, 3, 7 },
  }, {
    { 7, 7 },
    { 7, 4, 2, 7 },
  }, {
    { 7, 7 },
    { 7, 7, 1, 7 },
  }, {
    { 7, 17 },
    { 17, 1, 7, 7 },
  }, {
    { 7, 17 },
    { 17, 4, 2, 7 },
  }, {
    { 7, 17 },
    { 17, 7, 1, 7 },
  }, {
    { 7, 60 },
    { 60, 1, 7, 7 },
  }, {
    { 7, 60 },
    { 60, 3, 3, 7 },
  }, {
    { 7, 60 },
    { 60, 7, 1, 7 },
  }, {
    { 7, 139 },
    { 139, 1, 7, 7 },
  }, {
    { 7, 139 },
    { 139, 3, 3, 7 },
  }, {
    { 7, 139 },
    { 139, 7, 1, 7 },
  }, {
    { 43, 1 },
    { 1, 1, 43, 43 },
  }, {
    { 43, 1 },
    { 1, 2, 22, 43 },
  }, {
    { 43, 1 },
    { 1, 2, 25, 43 },
  }, {
    { 43, 1 },
    { 1, 2, 37, 43 },
  }, {
    { 43, 1 },
    { 1, 2, 41, 43 },
  }, {
    { 43, 1 },
    { 1, 5, 9, 43 },
  }, {
    { 43, 1 },
    { 1, 5, 10, 43 },
  }, {
    { 43, 1 },
    { 1, 9, 5, 43 },
  }, {
    { 43, 1 },
    { 1, 22, 2, 43 },
  }, {
    { 43, 1 },
    { 1, 43, 1, 43 },
  }, {
    { 43, 2 },
    { 2, 1, 43, 43 },
  }, {
    { 43, 2 },
    { 2, 2, 27, 43 },
  }, {
    { 43, 2 },
    { 2, 22, 2, 43 },
  }, {
    { 43, 2 },
    { 2, 43, 1, 43 },
  }, {
    { 57, 7 },
    { 7, 1, 57, 57 },
  }, {
    { 57, 7 },
    { 7, 2, 37, 57 },
  }, {
    { 57, 7 },
    { 7, 2, 43, 57 },
  }, {
    { 57, 7 },
    { 7, 2, 55, 57 },
  }, {
    { 57, 7 },
    { 7, 2, 56, 57 },
  }, {
    { 57, 7 },
    { 7, 7, 9, 57 },
  }, {
    { 57, 7 },
    { 7, 8, 8, 57 },
  }, {
    { 57, 7 },
    { 7, 29, 2, 57 },
  }, {
    { 57, 7 },
    { 7, 57, 1, 57 },
  }, {
    { 67, 17 },
    { 17, 1, 67, 67 },
  }, {
    { 67, 17 },
    { 17, 2, 34, 67 },
  }, {
    { 67, 17 },
    { 17, 2, 49, 67 },
  }, {
    { 67, 17 },
    { 17, 2, 66, 67 },
  }, {
    { 67, 17 },
    { 17, 6, 12, 67 },
  }, {
    { 67, 17 },
    { 17, 6, 13, 67 },
  }, {
    { 67, 17 },
    { 17, 17, 4, 67 },
  }, {
    { 67, 17 },
    { 17, 34, 2, 67 },
  }, {
    { 67, 17 },
    { 17, 67, 1, 67 },
  }, {
    { 129, 139 },
    { 139, 1, 129, 129 },
  }, {
    { 129, 139 },
    { 139, 2, 65, 129 },
  }, {
    { 129, 139 },
    { 139, 2, 80, 129 },
  }, {
    { 129, 139 },
    { 139, 2, 120, 129 },
  }, {
    { 129, 139 },
    { 139, 2, 128, 129 },
  }, {
    { 129, 139 },
    { 139, 3, 43, 129 },
  }, {
    { 129, 139 },
    { 139, 3, 47, 129 },
  }, {
    { 129, 139 },
    { 139, 3, 59, 129 },
  }, {
    { 129, 139 },
    { 139, 3, 64, 129 },
  }, {
    { 129, 139 },
    { 139, 7, 19, 129 },
  }, {
    { 129, 139 },
    { 139, 7, 20, 129 },
  }, {
    { 129, 139 },
    { 139, 7, 21, 129 },
  }, {
    { 129, 139 },
    { 139, 43, 3, 129 },
  }, {
    { 129, 139 },
    { 139, 65, 2, 129 },
  }
// out of lmem size
//  , {
//    { 129, 139 },
//    { 139, 129, 1, 129 },
//  }
};

static void tg2l_matrix_copy_row_col_transposed_ref(
    param_t *p, u8 ref_data[], u8 src_data[])
{
  u64 row = p->src->shape.row;
  u64 col = p->src->shape.col;

  for (u64 ri = 0; ri < row; ri++) {
    for (u64 ci = 0; ci < col; ci++) {
      u64 src_i = ri * col + ci;
      u64 dst_i = ci * row + ri;
      ref_data[dst_i] = src_data[src_i];
    }
  }
}

static void test_param_g2l(bmctx_t *ctx, bmk_ctx_t *bmk, param_t *p)
{
  print_param(stderr, p);
  u64 size = ml_shape_size(&p->dst->shape);

  u8 *src_data = (u8 *)malloc(sizeof(u8) * size);
  for (u64 i = 0; i < size; i++)
    src_data[i] = 200 + i;

  put_mg_gmem(ctx, p->src, src_data);
  bmk1822_tdma_g2l_matrix_copy_row_col_transposed(bmk, p);
  test_submit(ctx);
  u8 *dst_data = get_matrix_l2g(ctx, bmk, p->dst);

  u8 *ref_data = (u8 *)malloc(sizeof(u8) * size);
  tg2l_matrix_copy_row_col_transposed_ref(p, ref_data, src_data);

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
  free_mg_gmem(ctx, p->src);
  free_ml(bmk, p->dst);
}

static void test_one_case(bmctx_t *ctx, bmk_ctx_t *bmk, case_t *c)
{
  param_t p;
  /*
   * Matrix transpose must be n/c stride alignment
   * for TDMA limitation
   */
  int dst_align = 1;

  memset(&p, 0, sizeof(p));

  p.src = alloc_mg_gmem(ctx, c->src_shape);
  p.dst = alloc_ml(bmk, c->dst_shape, dst_align);
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
