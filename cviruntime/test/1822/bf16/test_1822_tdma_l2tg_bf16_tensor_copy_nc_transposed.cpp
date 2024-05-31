#include "../1822_test_util.h"
#include "1822_bf16_util.h"

typedef bmk1822_tdma_l2tg_tensor_copy_nc_transposed_param_t param_t;

static void __print_param(const char *tag, FILE *f, param_t *p)
{
  fprintf(
      f, "%s: (%u, %u, %u, %u) => (%u, %u, %u, %u)\n",
      tag,
      p->src->shape.n, p->src->shape.c, p->src->shape.h, p->src->shape.w,
      p->dst->shape.n, p->dst->shape.c, p->dst->shape.h, p->dst->shape.w);
}

#define print_param(f, p) __print_param(__func__, f, p)

static fmt_type input_fmt[] = {
 {FMT_BF16, FMT_BF16},
 {FMT_BF16, FMT_I8},
 {FMT_BF16, FMT_U8},
};

typedef struct {
  tl_shape_t src_shape;
  tg_shape_t dst_shape;
} case_t;

static case_t g_cases[] = {
  {
    { 1, 1, 1, 1 },
    { 1, 1, 1, 1 },
  }, {
    { 1, 1, 1, 2 },
    { 1, 1, 1, 2 },
  }, {
    { 1, 1, 1, 2 },
    { 1, 1, 2, 1 },
  }, {
    { 1, 1, 7, 2 },
    { 1, 1, 7, 2 },
  }, {
    { 1, 1, 7, 2 },
    { 1, 1, 2, 7 },
  }, {
    { 1, 1, 17, 13 },
    { 1, 1, 17, 13 },
  }, {
    { 1, 1, 17, 13 },
    { 1, 1, 13, 17 },
  }, {
    { 1, 1, 10, 60 },
    { 1, 1, 10, 60 },
  }, {
    { 1, 1, 10, 60 },
    { 1, 1, 2, 300 },
  }, {
    { 1, 1, 10, 60 },
    { 1, 1, 3, 200 },
  }, {
    { 1, 1, 10, 60 },
    { 1, 1, 4, 150 },
  }, {
    { 1, 1, 10, 60 },
    { 1, 1, 5, 120 },
  }, {
    { 1, 1, 10, 60 },
    { 1, 1, 60, 10 },
  }, {
    { 1, 1, 120, 5 },
    { 1, 1, 120, 5 },
  }, {
    { 1, 2, 1, 1 },
    { 2, 1, 1, 1 },
  }, {
    { 2, 17, 1, 4 },
    { 17, 2, 1, 4 },
  }, {
    { 2, 17, 1, 4 },
    { 17, 2, 2, 2 },
  }, {
    { 2, 17, 1, 4 },
    { 17, 2, 4, 1 },
  }, {
    { 17, 2, 2, 2 },
    { 2, 17, 2, 2 },
  }, {
    { 17, 2, 4, 1 },
    { 2, 17, 4, 1 },
  }, {
    {  3, 16, 1, 1 },
    { 16,  3, 1, 1 },
  }, {
    { 3, 39, 23, 17 },
    { 39, 3, 23, 17 },
  }, {
    { 3, 39, 17, 23 },
    { 39, 3, 17, 23 },
  }, {
    { 3, 36,  16, 20 },
    { 36, 3,  16, 20 },
  }, {
    { 3, 36,  16, 20 },
    { 36, 3,  2, 160 },
  }, {
    { 3, 36,  16, 20 },
    { 36, 3,  4, 80 },
  }, {
    { 3, 36,  16, 20 },
    { 36, 3,  8, 40 },
  }, {
    { 3, 36,  16, 20 },
    { 36, 3,  20, 16 },
  }, {
    { 3, 36,  16, 20 },
    { 36, 3,  32, 10 },
  }, {
    { 3, 36,  16, 20 },
    { 36, 3,  64, 5 },
  }, {
    { 5, 39, 17, 23 },
    { 39, 5, 17, 23 },
  }, {
    { 20, 35, 2, 2 },
    { 35, 20, 2, 2 },
  }, {
    { 35, 20, 2, 2 },
    { 20, 35, 2, 2 },
  }, {
    { 36, 3, 160, 2 },
    { 3, 36, 160, 2 },
  }, {
    { 36, 3, 160, 2 },
    { 3, 36, 2, 160 },
  }, {
    { 36, 3, 160, 2 },
    { 3, 36, 4, 80 },
  }, {
    { 36, 3, 160, 2 },
    { 3, 36, 8, 40 },
  }, {
    { 36, 3, 160, 2 },
    { 3, 36, 10, 32 },
  }, {
    { 36, 3, 160, 2 },
    { 3, 36, 20, 16 },
  }, {
    { 39, 5, 23, 17 },
    { 5, 39, 23, 17 },
  }    
};

static void l2tg_tensor_copy_nc_transposed_ref(
    param_t *p, u16 ref_data[], u16 src_data[])
{
  tl_shape_t s = p->src->shape;
  u32 n = s.n;
  u32 c = s.c;
  u32 hw = s.h * s.w;

  for (u32 ni = 0; ni < n; ni++) {
    for (u32 ci = 0; ci < c; ci++) {
      for (u32 hwi = 0; hwi < hw; hwi++) {
        u32 src_i = ni * c * hw + ci * hw + hwi;
        u32 dst_i = ci * n * hw + ni * hw + hwi;
        if(p->src->fmt == FMT_BF16 && p->dst->fmt == FMT_BF16)
          ref_data[dst_i] = src_data[src_i];
        else if (p->src->fmt == FMT_BF16 && (p->dst->fmt == FMT_I8 || p->dst->fmt == FMT_U8)) {
          u8 sign = p->dst->fmt == FMT_I8 ? 1 : 0;
          u8 val = sign ? (u8) convert_bf16_s8(src_data[src_i]) : (u8) convert_bf16_u8(src_data[src_i]);
          ref_data[dst_i] = u8 (val);
        } else if(p->dst->fmt == p->src->fmt){ //i8->i8
          ref_data[dst_i] = src_data[src_i];
        } else {
          fprintf(stderr, "Error src_fmt_type (%d) or dst_fmttype (%d)", p->src->fmt, p->dst->fmt);
          exit(-1);
        }
      }
    }
  }
}

static void test_param_l2g(bmctx_t *ctx, bmk_ctx_t *bmk, param_t *p)
{
  print_param(stderr, p);
  u64 size = tl_shape_size(&p->src->shape);

  u16 *src_data = (u16 *)malloc(sizeof(u16) * size);
  float val = -100;
  for (u64 i = 0; i < size; i++) {
    src_data[i] = generate_bf16_corner_val(val);
    val += 0.1;
  }

  put_bf16_tensor_g2l(ctx, bmk, p->src, src_data,  p->src->fmt);
  bmk1822_tdma_l2g_bf16_tensor_copy_nc_transposed(bmk, p);
  test_submit(ctx);
  u16 *dst_data = (u16*) get_tg_bf16_gmem(ctx, p->dst);

  u16 *ref_data = (u16 *)malloc(sizeof(u16) * size);
  l2tg_tensor_copy_nc_transposed_ref(p, ref_data, src_data);

  if(p->dst->fmt == FMT_BF16 && p->src->fmt == FMT_BF16) {
    for (u64 i = 0; i < size; i++) {
      if (dst_data[i] != ref_data[i]) {
        fprintf(stderr, "comparing failed at dst[%" PRIu64 "], got %d, exp %d\n",
                i, dst_data[i], ref_data[i]);
        exit(-1);
      }
    }
  } else if(p->dst->fmt == FMT_U8 || p->dst->fmt == FMT_I8) {
    for (u64 i = 0; i < size; i++) {
      u32 shift = (i%2)*8;
      if ((u8)(dst_data[i/2] >> shift) != (u8)ref_data[i]) {
        fprintf(stderr, "comparing (bf16->i8/u8) failed at dst[%" PRIu64 "], got %x, exp %x\n",
                i,(u8) (dst_data[i/2] >> shift) , ref_data[i]);
        exit(-1);
      }
    }
  } else {
    fprintf(stderr, "Error compreing type src_fmt_type (%d) or dst_fmttype (%d)", p->src->fmt, p->dst->fmt);
  }

  free(src_data);
  free(dst_data);
  free(ref_data);
}

static void destroy_param_l2g(bmctx_t *ctx, bmk_ctx_t *bmk, param_t *p)
{
  free_tg_gmem(ctx, p->dst);
  free_tl(bmk, p->src);
}

static void test_one_case(bmctx_t *ctx, bmk_ctx_t *bmk, case_t *c)
{
  u32 nr_fmt = sizeof(input_fmt) / sizeof(input_fmt[0]);
  for (u32 i = 0; i < nr_fmt; i++) {
    for (int src_align = 0; src_align < 2; src_align++) {
      param_t p;
      memset(&p, 0, sizeof(p));
  
      p.src = alloc_tl(bmk, c->src_shape, input_fmt[i].src_fmt, src_align);
      p.dst = alloc_tg_bf16_gmem(ctx, c->dst_shape, input_fmt[i].dst_fmt);
      test_param_l2g(ctx, bmk, &p);
      destroy_param_l2g(ctx, bmk, &p);
  
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
