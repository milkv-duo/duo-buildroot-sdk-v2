#include "../1880v2_test_util.h"
#include "1880v2_bf16_util.h"

typedef bmk1880v2_tdma_l2tg_tensor_copy_param_t param_t;

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
    { 1, 1, 2, 1 },
  }, {
    { 1, 1, 7, 2 },
    { 1, 1, 2, 7 },
  }, {
    { 1, 1, 17, 13 },
    { 1, 1, 13, 17 },
  }, {
    { 1, 1, 10, 60 },
    { 1, 1, 120, 5 },
  }, {
    { 1, 2, 1, 1 },
    { 1, 1, 1, 2 },
  }, {
    { 2, 17, 1,  4 },
    { 2,  1, 4, 17 },
  }, {
    { 2, 17, 1,  4 },
    { 2,  1, 17, 4 },
  }, {
    { 3, 16, 1, 1 },
    { 3,  1, 2, 8 },
  }, {
    { 3, 39, 17, 23 },
    { 3, 17, 39, 23 },
  }, {
    { 3, 36, 16,  20 },
    { 3, 18,  1, 640 },
  }, {
    { 5, 39, 17, 23 },
    { 5, 17, 39, 23 },
  }, {
    { 20, 35,  2, 2 },
    { 20,  7, 10, 2 },
  }    
};

static void l2tg_tensor_copy_ref(param_t *p, u16 ref_data[], u16 src_data[])
{
  u64 size = tl_shape_size(&p->src->shape);

  for (u64 i = 0; i < size; i++) {
    if(p->src->fmt == FMT_BF16 && p->dst->fmt == FMT_BF16)
      ref_data[i] = src_data[i];
    else if (p->src->fmt == FMT_BF16 && (p->dst->fmt == FMT_I8 || p->dst->fmt == FMT_U8)) {
      u8 sign = p->dst->fmt == FMT_I8 ? 1 : 0;
      s16 val = sign ? (s16) convert_bf16_s8(src_data[i]) : (u16) convert_bf16_u8(src_data[i]);
      ref_data[i] = u16 (val);
    } else if(p->dst->fmt == p->src->fmt){ //i8->i8
      ref_data[i] = src_data[i];
    } else {
      fprintf(stderr, "Error src_fmt_type (%d) or dst_fmttype (%d)", p->src->fmt, p->dst->fmt);
      exit(-1);
    }
  }
}

static void test_param_l2g(CVI_RT_HANDLE *ctx, bmk_ctx_t *bmk, param_t *p)
{
  print_param(stderr, p);
  u64 size = tl_shape_size(&p->src->shape);

  u16 *src_data = (u16 *)malloc(sizeof(u16) * size);
  float val = -100;
  for(u64 i = 0; i < size; i++) {
    src_data[i] = generate_bf16_corner_val(val);
    val += 0.1;
  }

  put_bf16_tensor_g2l(ctx, bmk, p->src, src_data, p->src->fmt);
  bmk1880v2_tdma_l2g_bf16_tensor_copy(bmk, p);
  test_submit(ctx);
  u16 *dst_data = (u16*)get_tg_bf16_gmem(ctx, p->dst);

  u16 *ref_data = (u16 *)malloc(sizeof(u16) * size);
  l2tg_tensor_copy_ref(p, ref_data, src_data);

  if(p->dst->fmt == FMT_BF16 && p->src->fmt == FMT_BF16) {
    for (u64 i = 0; i < size; i++) {
      if (dst_data[i] != ref_data[i]) {
        fprintf(stderr, "comparing(%d->%d) failed at dst[%" PRIu64 "], got %x, exp %x\n",
                p->src->fmt, p->dst->fmt, i, dst_data[i], ref_data[i]);
        exit(-1);
      }
    }
  } else if(p->dst->fmt == FMT_U8 || p->dst->fmt == FMT_I8) {
    for (u64 i = 0; i < size; i++) {
      u32 shift = (i%2)*8;
      if ((u8)(dst_data[i/2] >> shift) != (u8)ref_data[i]) {
        fprintf(stderr, "comparing (bf16->i8/u8) failed at dst[%" PRIu64 "], got %x, exp %x\n",
                i, (dst_data[i/2] >> shift) , ref_data[i]);
        exit(-1);
      }
    }
  } else {
    fprintf(stderr, "Error compreing type src_fmt_type (%d) or dst_fmttype (%d)", p->src->fmt, p->dst->fmt);
    exit(-1);
  }

  free(src_data);
  free(dst_data);
  free(ref_data);
}


static void destroy_param_l2g(CVI_RT_HANDLE *ctx, bmk_ctx_t *bmk, param_t *p)
{
  free_tg_gmem(ctx, p->dst);
  free_tl(bmk, p->src);
}

static void test_one_case(CVI_RT_HANDLE *ctx, bmk_ctx_t *bmk, case_t *c)
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
  CVI_RT_HANDLE ctx;
  bmk_ctx_t *bmk;
  test_init(&ctx, &bmk);

  u32 nr_cases = sizeof(g_cases) / sizeof(g_cases[0]);
  for (u32 i = 0; i < nr_cases; i++)
    test_one_case(&ctx, bmk, &g_cases[i]);

  test_exit(&ctx);
  return 0;
}
