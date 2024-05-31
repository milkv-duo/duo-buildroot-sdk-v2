#include <cvimath_internal.h>
#include <sys/time.h>
#include <test_cvikernel_util.h>

typedef cvk_tdma_g2g_tensor_copy_param_t param_t;

static void __print_param(const char *tag, FILE *f, param_t *p) {
  fprintf(f, "%s: (%u, %u, %u, %u) => (%u, %u, %u, %u)\n", tag, p->src->shape.n, p->src->shape.c,
          p->src->shape.h, p->src->shape.w, p->dst->shape.n, p->dst->shape.c, p->dst->shape.h,
          p->dst->shape.w);
}

#define print_param(f, p) __print_param(__func__, f, p)

typedef struct {
  cvk_tg_shape_t src_shape;
  cvk_tg_shape_t dst_shape;
} case_t;

static cvk_fmt_type input_fmt[] = {
    {CVK_FMT_BF16, CVK_FMT_BF16},
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

static void test_param_g2g(CVI_RT_HANDLE *ctx, cvk_context_t *bmk, param_t *p) {
  print_param(stderr, p);

  // 2 means source is fp32, occupy 2 * bf16 size
  uint64_t size = p->src->shape.n * p->src->shape.c * p->src->shape.h * p->src->shape.w / 2;
  uint32_t *src_data = new uint32_t[size];
  for (uint64_t i = 0; i < size; i++) {
    src_data[i] = ((0x1234 + i) << 16) + 0x5678 + i;
    // printf("src[%lu] 0x%x\n", i, src_data[i]);
  }

  test_put_tg_mem_comp(ctx, p->src, (uint8_t *)src_data);

  cvm_s2s_fp32_bf16(bmk, p->src->start_address, p->src->shape, p->dst->start_address, p->dst->shape,
                    CVK_FMT_BF16);

  long elapsed;
  struct timeval t0, t1;
  gettimeofday(&t0, NULL);

  test_submit_comp(ctx, bmk);

  gettimeofday(&t1, NULL);
  elapsed = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;
  printf("kernel takes %ld us\n", elapsed);

  uint16_t *dst_data = (uint16_t *)test_get_tg_mem_comp(ctx, p->dst);

  for (uint64_t i = 0; i < size; i++) {
    uint16_t _src_data = (src_data[i] >> 16) & 0xffff;
    if (dst_data[i] != _src_data) {
      fprintf(stderr, "comparing failed at dst[%lu], got %x, exp %x\n", i, dst_data[i], _src_data);
      exit(-1);
    }
  }

  delete[] src_data;
  free(dst_data);
}

static void destroy_param_g2g(CVI_RT_HANDLE *ctx, param_t *p) {
  test_free_tg_mem_comp(ctx, p->src);
  test_free_tg_mem_comp(ctx, p->dst);
}

static void test_one_case(CVI_RT_HANDLE *ctx, cvk_context_t *bmk, case_t *c) {
  uint32_t nr_fmt = sizeof(input_fmt) / sizeof(input_fmt[0]);
  for (uint32_t i = 0; i < nr_fmt; i++) {
    param_t p;
    cvk_tg_t *src, *dst;
    src = test_alloc_tg_mem_comp(ctx, bmk, c->src_shape, input_fmt[i].src_fmt);
    dst = test_alloc_tg_mem_comp(ctx, bmk, c->dst_shape, input_fmt[i].dst_fmt);
    p.src = src;
    p.dst = dst;
    test_param_g2g(ctx, bmk, &p);
    destroy_param_g2g(ctx, &p);
  }
}

int main() {
  CVI_RT_HANDLE ctx;
  cvk_context_t *bmk;

  test_init(&ctx, &bmk);

  uint32_t nr_cases = sizeof(g_cases) / sizeof(g_cases[0]);
  for (uint32_t i = 0; i < nr_cases; i++) test_one_case(&ctx, bmk, &g_cases[i]);

  test_exit(&ctx, bmk);
  return 0;
}
