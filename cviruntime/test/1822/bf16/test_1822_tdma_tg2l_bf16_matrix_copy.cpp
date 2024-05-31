#include "../1822_test_util.h"
#include "1822_bf16_util.h"

typedef bmk1822_tdma_tg2l_matrix_copy_param_t param_t;

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

static fmt_type input_fmt[] = {
 {FMT_BF16, FMT_BF16},
 {FMT_I8, FMT_BF16},
 {FMT_U8, FMT_BF16},
};

typedef struct {
  mg_shape_t src_shape;
  ml_shape_t dst_shape;
} case_t;

static case_t g_cases[] = {
  {
    { 0, 1 },
    { 0, 1, 1, 1 },
  }, {
    { 0, 2 },
    { 0, 1, 2, 2 },
  }, {
    { 0, 2 },
    { 0, 2, 1, 2 },
  }, {
    { 0, 7 },
    { 0, 1, 7, 7 },
  }, {
    { 0, 7 },
    { 0, 2, 4, 7 },
  }, {
    { 0, 7 },
    { 0, 7, 1, 7 },
  }, {
    { 0, 17 },
    { 0, 1, 17, 17 },
  }, {
    { 0, 17 },
    { 0, 3, 7, 17 },
  }, {
    { 0, 17 },
    { 0, 17, 1, 17 },
  }, {
    { 0, 60 },
    { 0, 1, 60, 60 },
  }, {
    { 0, 60 },
    { 0, 30, 2, 60 },
  }, {
    { 0, 60 },
    { 0, 60, 1, 60 },
  }
};

static void tg2l_matrix_copy_ref(param_t *p, u16 ref_data[], u16 src_data[])
{
  u64 size = ml_shape_size(&p->dst->shape);

  for (u64 i = 0; i < size; i++) {
    if(p->src->fmt == FMT_BF16) {
      ref_data[i] = src_data[i];
    } else {
      u8* u8src_data = (u8*)src_data;
      u8 sign = p->src->fmt == FMT_I8 ? 1 : 0;
      ref_data[i] = (u16)convert_int8_bf16(u8src_data[i], sign);
    }
  }
}

static void test_param_g2l(bmctx_t *ctx, bmk_ctx_t *bmk, param_t *p)
{
  print_param(stderr, p);
  u64 size = ml_shape_size(&p->dst->shape);

  u16 *u16src_data = (u16 *)malloc(sizeof(u16) * size);
  u8 *u8src_data = (u8 *)malloc(sizeof(u8) * size);
  u8 *src_data;

  if(p->src->fmt == FMT_BF16) {
    float val = -10;
    for(u64 i = 0; i < size; i++) {
      u16src_data[i] = generate_bf16_corner_val(val);
      val += 0.2;
    }
    src_data = (u8*)u16src_data;
  } else {
    for(u64 i = 0; i < size; i++) {
      u8src_data[i] = 200 + i;
    }
    src_data = u8src_data;
  }

  put_mg_bf16_gmem(ctx, p->src, (u8*) src_data);
  bmk1822_tdma_g2l_bf16_matrix_copy(bmk, p);
  test_submit(ctx);
  u16 *dst_data = (u16*) get_bf16_matrix_l2g(ctx, bmk, p->dst, p->dst->fmt);

  u16 *ref_data = (u16 *)malloc(sizeof(u16) * size);
  tg2l_matrix_copy_ref(p, ref_data, (u16*)src_data);

  if(p->dst->fmt == FMT_BF16 && p->src->fmt == FMT_BF16) {
    for (u64 i = 0; i < size; i++) {
      if (dst_data[i] != ref_data[i]) {
        fprintf(stderr, "comparing(%d->%d) failed at dst[%" PRIu64 "], got %x, exp %x\n",
                p->src->fmt, p->dst->fmt, i, dst_data[i], ref_data[i]);
        exit(-1);
      }
    }
  } else if(p->src->fmt == FMT_U8 || p->src->fmt == FMT_I8) {
    for (u64 i = 0; i < size; i++) {
      if (dst_data[i] != ref_data[i]) {
        fprintf(stderr, "comparing(%d->%d) failed at dst[%" PRIu64 "], got %x, exp %x\n",
                p->src->fmt, p->dst->fmt, i, dst_data[i] , ref_data[i]);
        exit(-1);
      }
    }
  } else {
    fprintf(stderr, "Error compreing type src_fmt_type (%d) or dst_fmttype (%d)", p->src->fmt, p->dst->fmt);
    exit(-1);
  }

  free(u8src_data);
  free(u16src_data);
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
  u32 nr_fmt = sizeof(input_fmt) / sizeof(input_fmt[0]);
  for (u32 i = 0; i < nr_fmt; i++) {
    for (u32 row = 1; row < 13; row += 2) {
      c->src_shape.row = row;
      c->dst_shape.n = row;
      for (int dst_align = 0; dst_align < 2; dst_align++) {
        param_t p;
        memset(&p, 0, sizeof(p));

        p.src = alloc_mg_bf16_gmem(ctx, c->src_shape, input_fmt[i].src_fmt);
        p.dst = alloc_ml_bf16(bmk, c->dst_shape, input_fmt[i].dst_fmt, dst_align);
        test_param_g2l(ctx, bmk, &p);
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
