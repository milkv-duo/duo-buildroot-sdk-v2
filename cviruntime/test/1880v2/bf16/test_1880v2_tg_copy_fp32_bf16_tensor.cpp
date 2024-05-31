#include "../1880v2_test_util.h"
#include "1880v2_bf16_util.h"
#include <sys/time.h>

typedef bmk1880v2_tdma_tg2tg_tensor_copy_param_t param_t;

static void __print_param(const char *tag, FILE *f, param_t *p) {
  fprintf(f, "%s: (%u, %u, %u, %u) => (%u, %u, %u, %u)\n", tag, p->src->shape.n, p->src->shape.c,
          p->src->shape.h, p->src->shape.w, p->dst->shape.n, p->dst->shape.c, p->dst->shape.h,
          p->dst->shape.w);
}

#define print_param(f, p) __print_param(__func__, f, p)

typedef struct {
  tg_shape_t src_shape;
  tg_shape_t dst_shape;
} case_t;

static fmt_type input_fmt[] = {
    {FMT_BF16, FMT_BF16},
};

static case_t g_cases[] = {
    {
        {1, 3, 3, 2},
        {1, 3, 3, 2},
    },
	{
        {4, 3, 3, 2},
        {4, 3, 3, 2},
    },

    //{
    //  // YOLOv2 concat layer
    //  {1, 256, 19, 19},
    //  {1, 256, 19, 19},
    //},
    {
        {1, 256, 19, 20},
        {1, 256, 19, 20},
    },
    {
        {1, 1280, 3, 4},
        {1, 1280, 3, 4},
    },
    {
        {1, 159 * 89, 36, 4},
        {1, 159 * 89, 36, 4},
    },
    {
        {159, 89, 36, 4},
        {159, 89, 36, 4},
    },
};

static void test_param_g2g(CVI_RT_HANDLE *ctx, bmk_ctx_t *bmk, param_t *p) {
  print_param(stderr, p);

  // 2 means source is fp32, occupy 2 * bf16 size
  u64 size = p->src->shape.n * p->src->shape.c * p->src->shape.h * p->src->shape.w / 2;
  u32 *src_data = (u32 *)malloc(sizeof(u32) * size);
  for (u64 i = 0; i < size; i++) {
    src_data[i] = ((0x1234 + i) << 16) + 0x5678 + i;
    // printf("src[%" PRIu64 "] 0x%x\n", i, src_data[i]);
  }

  put_tg_bf16_gmem(ctx, p->src, (u8 *)src_data);

  bf16_s2s_fp32_bf16(bmk, p->src->start_address, p->src->shape, p->dst->start_address,
                     p->dst->shape, FMT_BF16);

  long elapsed;
  struct timeval t0, t1;
  gettimeofday(&t0, NULL);

  test_submit(ctx);

  gettimeofday(&t1, NULL);
  elapsed = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;
  printf("kernel takes %ld us\n", elapsed);

  u16 *dst_data = (u16 *)get_tg_bf16_gmem(ctx, p->dst);

  for (u64 i = 0; i < size; i++) {
    u16 _src_data = (src_data[i] >> 16) & 0xffff;
    if (dst_data[i] != _src_data) {
      fprintf(stderr, "comparing failed at dst[%" PRIu64 "], got %x, exp %x\n", i, dst_data[i], _src_data);
      exit(-1);
    }
  }

  free(src_data);
  free(dst_data);
}

static void destroy_param_g2g(CVI_RT_HANDLE *ctx, param_t *p) {
  free_tg_gmem(ctx, p->src);
  free_tg_gmem(ctx, p->dst);
}

static void test_one_case(CVI_RT_HANDLE *ctx, bmk_ctx_t *bmk, case_t *c) {
  u32 nr_fmt = sizeof(input_fmt) / sizeof(input_fmt[0]);
  for (u32 i = 0; i < nr_fmt; i++) {
    param_t p;
    bmk1880v2_tensor_tgmem_t *src, *dst;

    memset(&p, 0, sizeof(p));

    src = alloc_tg_bf16_gmem(ctx, c->src_shape, input_fmt[i].src_fmt);
    dst = alloc_tg_bf16_gmem(ctx, c->dst_shape, input_fmt[i].dst_fmt);
    p.src = src;
    p.dst = dst;
    test_param_g2g(ctx, bmk, &p);
    destroy_param_g2g(ctx, &p);
  }
}

int main() {
  CVI_RT_HANDLE ctx;
  bmk_ctx_t *bmk;
  test_init(&ctx, &bmk);

  u32 nr_cases = sizeof(g_cases) / sizeof(g_cases[0]);
  for (u32 i = 0; i < nr_cases; i++) test_one_case(&ctx, bmk, &g_cases[i]);

  test_exit(&ctx);
  return 0;
}
