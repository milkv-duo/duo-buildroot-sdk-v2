#include "1880v2_test_util.h"

typedef bmk1880v2_tiu_depthwise_convolution_param_t depthwise_conv_param_t;

typedef bmk1880v2_tdma_l2tg_tensor_copy_cw_transposed_param_t l2tg_cw_param_t;
typedef bmk1880v2_tdma_tg2l_matrix_copy_row_col_transposed_param_t tg2l_matrix_param_t;
typedef bmk1880v2_tdma_l2l_tensor_copy_param_t l2l_tensor_copy_param_t;

typedef struct{
    s8  *depthwise_conv_input;
    s8  *depthwise_conv_weight;
    s16 *depthwise_conv_bias;
    u8  *depthwise_conv_output;
    s8  *depthwise_conv_output_ref;
    u8  *l2g_cw_src;
    u8  *l2g_cw_output;
    u8  *l2g_cw_output_ref;
    u8  *g2l_matrix_src;
    u8  *g2l_matrix_output;
    u8  *g2l_matrix_output_ref;
    u8  *l2l_tensor_src;
    u8  *l2l_tensor_output;
    u8  *l2l_tensor_output_ref;
}s_test_data;

depthwise_conv_param_t depthwise_conv_param;
l2tg_cw_param_t l2tg_cw_param;
tg2l_matrix_param_t tg2l_matrix_param;
l2l_tensor_copy_param_t l2l_tensor_copy_param;
s_test_data s8_test_data;

bmk1880v2_tensor_lmem_t *skip_tensor_lmem[10];
u32 skip_tensor_num=0;

void skip_tensor_lmem_size(bmk_ctx_t *bmk, const bmk1880v2_tensor_lmem_t *p)
{
  if (!p)
    return;

  u32 needed = align_up(p->shape.n * p->stride.n, BM1880V2_HW_EU_NUM);
  u32 start_addr = p->start_address + needed;
  u32 remain_size = start_addr % BM1880V2_HW_LMEM_BANK_SIZE ? (BM1880V2_HW_LMEM_BANK_SIZE - start_addr % BM1880V2_HW_LMEM_BANK_SIZE) : 0; // remain size for each lane
  if(remain_size)
  {
    tl_shape_t src_shape2 = {1, BM1880V2_HW_NPU_NUM, 1, remain_size};
    skip_tensor_lmem[skip_tensor_num] = alloc_tl(bmk, src_shape2, FMT_I8, 1); // skip the lmem size and next tl can alignment to bank si     ze
  }
  skip_tensor_num++;
}

void skip_matrix_lmem_size(bmk_ctx_t *bmk, const bmk1880v2_matrix_lmem_t *p)
{
  u32 needed = align_up(p->shape.n * p->stride.n, BM1880V2_HW_EU_NUM);

  u32 start_addr = p->start_address + needed; //src_shape.n*src_shape.c*src_shape.h*src_shape.w/32;
  u32 remain_size = start_addr % BM1880V2_HW_LMEM_BANK_SIZE ? (BM1880V2_HW_LMEM_BANK_SIZE - start_addr % BM1880V2_HW_LMEM_BANK_SIZE) : 0; // remain size for each lane
  if(remain_size)
  {
    tl_shape_t src_shape2 = {1, BM1880V2_HW_NPU_NUM, 1, remain_size};
    skip_tensor_lmem[skip_tensor_num] = alloc_tl(bmk, src_shape2, FMT_I8, 1); // skip the lmem size and next tl can alignment to bank si     ze
  }
  skip_tensor_num++;
}

void free_skip_tensor_lmem(bmk_ctx_t *ctx)
{
  if(skip_tensor_lmem[--skip_tensor_num]!=NULL)
    free_tl(ctx, skip_tensor_lmem[skip_tensor_num]);
}

static s8 * alloc_input(const depthwise_conv_param_t *p)
{
  u64 size = tl_shape_size(&p->ifmap->shape);
  s8 *buf = (s8 *)malloc(sizeof(s8) * size);

  for (u64 i = 0; i < size; i++)
    buf[i] = rand() % 256 - 128;

  return buf;
}

static s8 * alloc_weight(const depthwise_conv_param_t *p)
{
  int size = tl_shape_size(&p->weight->shape);
  s8 *buf = (s8 *)malloc(sizeof(s8) * size);
  for (int i = 0; i < size; i++)
    buf[i] = rand() % 256 - 128;

  return buf;
}

static s16 * alloc_bias(const depthwise_conv_param_t *p)
{
  int c = p->bias->shape.c;
  s16 *bias = (s16 *)malloc(sizeof(s16) * c);

  for (int i = 0; i < c; i++)
    bias[i] = rand() % 65536 - 32768;

  return bias;
}

static s8 *alloc_output(depthwise_conv_param_t *p)
{
  u64 size = tl_shape_size(&p->ofmap->shape);
  s8 *output = (s8 *)malloc(sizeof(s8) * size);
  return output;
}

static inline void relu8(s8 *buf, u64 size)
{
  for (u64 i = 0; i < size; i++)
    if (buf[i] < 0)
      buf[i] = 0;
}

static void generate_results(
    depthwise_conv_param_t *p,
    s8 input[],
    s8 weight[],
    s16 bias[]
    )
{
  int in = p->ifmap->shape.n;
  int ic = p->ifmap->shape.c;
  int ih = p->ifmap->shape.h;
  int iw = p->ifmap->shape.w;
  int kh = p->weight->shape.h;
  int kw = p->weight->shape.w;
  int opd0_sign = (p->ifmap->fmt == FMT_I8);
  int res0_sign = (p->ofmap->fmt == FMT_I8);
  s8_test_data.depthwise_conv_output_ref = alloc_output(p);

  bmerr_t ret = native_pooling_ave_int8(
      input, weight, p->bias ? bias : NULL, s8_test_data.depthwise_conv_output_ref,
      in, ic, ih, iw, kh, kw,
      p->pad_top, p->pad_bottom, p->pad_left, p->pad_right,
      p->stride_h, p->stride_w,
      p->ins_h, p->ins_w, p->ins_last_h, p->ins_last_w,
      opd0_sign, res0_sign, p->rshift_bits, 0);
  assert(ret == BM_SUCCESS);

  if(p->relu_enable )
    relu8(s8_test_data.depthwise_conv_output_ref, tl_shape_size(&p->ofmap->shape));
}

static int pooling_ih_ext(depthwise_conv_param_t *p, int ih)
{
  int ins = p->ins_h;
  int ins_last = p->ins_last_h;
  int pad = p->pad_top + p->pad_bottom;
  return (ih - 1) * (ins + 1) + ins_last + 1 + pad;
}

static int pooling_iw_ext(depthwise_conv_param_t *p, int iw)
{
  int ins = p->ins_w;
  int ins_last = p->ins_last_w;
  int pad = p->pad_left + p->pad_right;
  return (iw - 1) * (ins + 1) + ins_last + 1 + pad;
}

static int pooling_oh(depthwise_conv_param_t *p, int ih, int kh)
{
  int ih_ext = pooling_ih_ext(p, ih);
  return (ih_ext - kh) / p->stride_h + 1;
}

static int pooling_ow(depthwise_conv_param_t *p, int iw, int kw)
{
  int iw_ext = pooling_iw_ext(p, iw);
  return (iw_ext - kw) / p->stride_w + 1;
}

static void free_depthwise_param(
    bmk_ctx_t *ctx,
    depthwise_conv_param_t *p)
{
  if (p->bias)
  {
    free_skip_tensor_lmem(ctx);
    free_tl(ctx, p->bias);
  }
  if (p->weight)
  {
    free_skip_tensor_lmem(ctx);
    free_tl(ctx, p->weight);
  }
  if (p->ifmap)
  {
    free_skip_tensor_lmem(ctx);
    free_tl(ctx, p->ifmap);
  }
  if (p->ofmap)
  {
    free_skip_tensor_lmem(ctx);
    free_tl(ctx, p->ofmap);
  }
}

static void put_bias_tensor(
    CVI_RT_HANDLE *ctx,
    bmk_ctx_t *bk_ctx,
    const tl_t *tl,
    s16 data[])
{
  int c = tl->shape.c;

  u8 *lo_hi = (u8 *)xmalloc(2 * c);
  if (!lo_hi)
    return;

  for (int i = 0; i < c; i++) {
    lo_hi[i] = data[i] & 0xff;
    lo_hi[i + c] = (data[i] >> 8) & 0xff;
  }

  put_tensor_g2l(ctx, bk_ctx, tl, (u8 *)lo_hi);

  free(lo_hi);
}

static depthwise_conv_param_t random_depthwise_param(bmk_ctx_t *ctx)
{
  srand(clock());
  depthwise_conv_param_t p;

  memset(&p, 0, sizeof(p));

retry:
  int using_bias = 0;
  int n = 1;
  int c = 4000;
  int ih = 2;
  int iw = 8;
  int kh = 1;
  int kw = 1;
  int opd0_sign = 0;

  p.ins_h = rand() % kh;
  p.ins_w = rand() % kw;
  p.ins_last_h = rand() % kh;
  p.ins_last_w = rand() % kw;
  p.stride_h = rand() % kh + 1;
  p.stride_w = rand() % kw + 1;
  p.pad_top = 0;
  p.pad_bottom = 0;
  p.pad_left = 0;
  p.pad_right = 0;
  p.rshift_bits = 2;
  int oh = pooling_oh(&p, ih, kh);
  int ow = pooling_ow(&p, iw, kw);
  tl_shape_t ofmap_shape;
  ofmap_shape.n = n;
  ofmap_shape.c = c;
  ofmap_shape.h = oh;
  ofmap_shape.w = ow;
  tl_shape_t ifmap_shape;
  ifmap_shape.n = n;
  ifmap_shape.c = c;
  ifmap_shape.h = ih;
  ifmap_shape.w = iw;
  tl_shape_t weight_shape;
  weight_shape.n = 1;
  weight_shape.c = c;
  weight_shape.h = kh;
  weight_shape.w = kw;
  tl_shape_t bias_shape;
  bias_shape.n = 2;
  bias_shape.c = c;
  bias_shape.h = 1;
  bias_shape.w = 1;
  p.relu_enable = 1;
  /*test case ref does not support dilation !=1*/
  p.dilation_w = 1;
  p.dilation_h = 1;
  fmt_t ifmt = opd0_sign ? FMT_I8: FMT_U8;

  p.ofmap = bmk1880v2_lmem_alloc_tensor(ctx, ofmap_shape, FMT_I8, 1);
  skip_tensor_lmem_size(ctx, p.ofmap);
  p.ifmap = bmk1880v2_lmem_alloc_tensor(ctx, ifmap_shape, ifmt, 1);
  skip_tensor_lmem_size(ctx, p.ifmap);
  p.weight = bmk1880v2_lmem_alloc_tensor(ctx, weight_shape, FMT_I8, 1);
  skip_tensor_lmem_size(ctx, p.weight);
  p.bias = NULL;
  if (using_bias)
  {
    p.bias = bmk1880v2_lmem_alloc_tensor(ctx, bias_shape, FMT_I8, 0);
    skip_tensor_lmem_size(ctx, p.bias);
  }
  if ((kh > pooling_ih_ext(&p, ih))
      || (kw > pooling_iw_ext(&p, iw))
      || (p.pad_top >= (1 << 4))
      || (p.pad_bottom >= (1 << 4))
      || (p.pad_left >= (1 << 4))
      || (p.pad_right >= (1 << 4))
      || !p.ofmap
      || !p.ifmap
      || !p.weight
      || (using_bias && !p.bias)) {
    printf("retry init_pooling_param\n");
    free_depthwise_param(ctx, &p);
    goto retry;
  }
  return p;
}


static int test_pooling(CVI_RT_HANDLE ctx, bmk_ctx_t *bk_ctx)
{
  depthwise_conv_param = random_depthwise_param(bk_ctx);

  s8 *input = alloc_input(&depthwise_conv_param);
  s8 *weight = alloc_weight(&depthwise_conv_param);
  s16 *bias = NULL;
  if (depthwise_conv_param.bias)
    bias = alloc_bias(&depthwise_conv_param);

  put_tensor_g2l(&ctx, bk_ctx, depthwise_conv_param.ifmap, (u8 *)input);
  put_tensor_g2l(&ctx, bk_ctx, depthwise_conv_param.weight, (u8 *)weight);
  if (depthwise_conv_param.bias)
    put_bias_tensor(&ctx, bk_ctx, depthwise_conv_param.bias, bias);

  generate_results(&depthwise_conv_param, input, weight, bias);

  free(input);
  free(weight);
  free(bias);

  return 1;
}

static void l2tg_tensor_copy_cw_transposed_ref(
    l2tg_cw_param_t *p, u8 ref_data[], u8 src_data[])
{
  tl_shape_t s = p->src->shape;
  u32 n = s.n;
  u32 c = s.c;
  u32 h = s.h;
  u32 w = s.w;

  for (u32 ni = 0; ni < n; ni++) {
    for (u32 ci = 0; ci < c; ci++) {
      for (u32 hi = 0; hi < h; hi++) {
        for (u32 wi = 0; wi < w; wi++) {
          u32 src_i = ni * c * h * w + ci * h * w + hi * w + wi;
          u32 dst_i = ni * c * h * w + wi * h * c + hi * c + ci;
          ref_data[dst_i] = src_data[src_i];
        }
      }
    }
  }
}

static void test_param_l2g(CVI_RT_HANDLE *ctx, bmk_ctx_t *bmk, l2tg_cw_param_t *p)
{
  u64 size = tl_shape_size(&p->src->shape);

  s8_test_data.l2g_cw_src = (u8 *)malloc(sizeof(u8) * size);
  for (u64 i = 0; i < size; i++)
    s8_test_data.l2g_cw_src[i] = rand()%0x100;

  s8_test_data.l2g_cw_output_ref = (u8 *)malloc(sizeof(u8) * size);
  if (!s8_test_data.l2g_cw_output_ref)
    return;

  l2tg_tensor_copy_cw_transposed_ref(p, s8_test_data.l2g_cw_output_ref, s8_test_data.l2g_cw_src);

  put_tensor_g2l(ctx, bmk, p->src, s8_test_data.l2g_cw_src);
}

static void destroy_param_l2g(CVI_RT_HANDLE *ctx, bmk_ctx_t *bmk, l2tg_cw_param_t *p)
{
  free_tg_gmem(ctx, p->dst);
  free_skip_tensor_lmem(bmk);
  free_tl(bmk, p->src);
}

static void test_l2tg_cw_transpose(CVI_RT_HANDLE *ctx, bmk_ctx_t *bmk, l2tg_cw_param_t *p)
{
  tl_shape_t src_shape = {1, 0x100, 1, 0x080};
  tg_shape_t dst_shape = {1, 0x080, 1, 0x100};

  p->src = alloc_tl(bmk, src_shape, FMT_I8, 1);
  p->dst = alloc_tg_gmem(ctx, dst_shape, FMT_I8);
  skip_tensor_lmem_size(bmk, p->src);
  test_param_l2g(ctx, bmk, p);
}

static void tg2l_matrix_copy_row_col_transposed_ref(
    tg2l_matrix_param_t *p, u8 ref_data[], u8 src_data[])
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

static void test_param_g2l(CVI_RT_HANDLE *ctx, tg2l_matrix_param_t *p)
{
  u64 size = ml_shape_size(&p->dst->shape);

  s8_test_data.g2l_matrix_src = (u8 *)malloc(sizeof(u8) * size);
  if (!s8_test_data.g2l_matrix_src)
    return;

  for (u64 i = 0; i < size; i++)
    s8_test_data.g2l_matrix_src[i] = rand()%0x100;

  s8_test_data.g2l_matrix_output_ref = (u8 *)malloc(sizeof(u8) * size);
  if (!s8_test_data.g2l_matrix_output_ref)
    return;

  tg2l_matrix_copy_row_col_transposed_ref(p, s8_test_data.g2l_matrix_output_ref, s8_test_data.g2l_matrix_src);

  put_mg_gmem(ctx, p->src, s8_test_data.g2l_matrix_src);
}

static void destroy_param_g2l(CVI_RT_HANDLE *ctx, bmk_ctx_t *bmk, tg2l_matrix_param_t *p)
{
  free_mg_gmem(ctx, p->src);
  free_skip_tensor_lmem(bmk);
  free_ml(bmk, p->dst);
}


static void test_tg2l_matrix_transpose(CVI_RT_HANDLE *ctx, bmk_ctx_t *bmk, tg2l_matrix_param_t *p)
{
  //tg2l_matrix_param_t p;
  /*
   * Matrix transpose must be n/c stride alignment
   * for TDMA limitation
   */
  mg_shape_t src_shape={0x100, 0x80};
  ml_shape_t dst_shape={0x80, 0x10, 0x10, 0x100};

  int dst_align = 1;

  p->src = alloc_mg_gmem(ctx, src_shape);
  p->dst = alloc_ml(bmk, dst_shape, dst_align);
  skip_matrix_lmem_size(bmk, p->dst);
  test_param_g2l(ctx, p);
}

static void l2l_tensor_copy_ref(l2l_tensor_copy_param_t *p, u8 ref_data[], u8 src_data[])
{
  u64 size = tl_shape_size(&p->src->shape);

  for (u64 i = 0; i < size; i++)
    ref_data[i] = src_data[i];
}

static void test_l2l_param(CVI_RT_HANDLE *ctx, bmk_ctx_t *bmk, l2l_tensor_copy_param_t *p)
{
  u64 size = tl_shape_size(&p->src->shape);

  s8_test_data.l2l_tensor_src = (u8 *)malloc(sizeof(u8) * size);
  if (!s8_test_data.l2l_tensor_src)
    return;

  for (u64 i = 0; i < size; i++)
    s8_test_data.l2l_tensor_src[i] = rand()%0x100;

  s8_test_data.l2l_tensor_output_ref = (u8 *)malloc(sizeof(u8) * size);
  if (!s8_test_data.l2l_tensor_output_ref)
    return;

  l2l_tensor_copy_ref(p, s8_test_data.l2l_tensor_output_ref, s8_test_data.l2l_tensor_src);

  put_tensor_g2l(ctx, bmk, p->src, s8_test_data.l2l_tensor_src);
}

static void destroy_param_l2l(bmk_ctx_t *bmk, l2l_tensor_copy_param_t *p)
{
  free_skip_tensor_lmem(bmk);
  free_tl(bmk, p->dst);
  free_skip_tensor_lmem(bmk);
  free_tl(bmk, p->src);
}

static void test_l2l_tensor_copy(CVI_RT_HANDLE *ctx, bmk_ctx_t *bmk, l2l_tensor_copy_param_t *p)
{
  tl_shape_t src_shape = {1, 0x10, 0x1, 0x400};
  tl_shape_t dst_shape = {1, 0x10, 0x1, 0x400};

  p->src = alloc_tl(bmk, src_shape, FMT_I8, 1);
  skip_tensor_lmem_size(bmk, p->src);
  p->dst = alloc_tl(bmk, dst_shape, FMT_I8, 1);
  skip_tensor_lmem_size(bmk, p->dst);
  test_l2l_param(ctx, bmk, p);
}

void get_result(CVI_RT_HANDLE *ctx, bmk_ctx_t *bmk)
{
  s8_test_data.depthwise_conv_output = get_tensor_l2g(ctx, bmk, depthwise_conv_param.ofmap);
  s8_test_data.l2g_cw_output = get_tg_gmem(ctx, l2tg_cw_param.dst);
  s8_test_data.g2l_matrix_output = get_matrix_l2g(ctx, bmk, tg2l_matrix_param.dst);
  s8_test_data.l2l_tensor_output = get_tensor_l2g(ctx, bmk, l2l_tensor_copy_param.dst);
}

void check_result()
{

  int cmp_res = array_cmp_int8(
      "Comparing results ...\n", s8_test_data.depthwise_conv_output_ref,  (s8 *)s8_test_data.depthwise_conv_output,
      tl_shape_size(&depthwise_conv_param.ofmap->shape));
  if (cmp_res != 0) {
    printf("Comparison FAILED!!!\n");
    exit(-1);
  }

  for (u64 i = 0; i < tl_shape_size(&l2tg_cw_param.src->shape); i++) {
    if (s8_test_data.l2g_cw_output[i] != s8_test_data.l2g_cw_output_ref[i]) {
      fprintf(stderr, "l2g_cw comparing failed at dst[%" PRIu64 "], got %d, exp %d\n",
              i, s8_test_data.l2g_cw_output[i], s8_test_data.l2g_cw_output_ref[i]);
      exit(-1);
    }
  }
  for (u64 i = 0; i < ml_shape_size(&tg2l_matrix_param.dst->shape); i++) {
    if (s8_test_data.g2l_matrix_output[i] != s8_test_data.g2l_matrix_output_ref[i]) {
      fprintf(stderr, "g2l_matrix comparing failed at dst[%" PRIu64 "], got %d, exp %d\n",
              i, s8_test_data.g2l_matrix_output[i], s8_test_data.g2l_matrix_output_ref[i]);
      exit(-1);
    }
  }

  for (u64 i = 0; i < tl_shape_size(&l2l_tensor_copy_param.src->shape); i++) {
    if (s8_test_data.l2l_tensor_output[i] != s8_test_data.l2l_tensor_output_ref[i]) {
      fprintf(stderr, "l2l_tensor comparing failed at dst[%" PRIu64 "], got %d, exp %d\n",
              i, s8_test_data.l2l_tensor_output[i], s8_test_data.l2l_tensor_output_ref[i]);
      exit(-1);
    }
  }


}

void trigger_max_power(CVI_RT_HANDLE *ctx, bmk_ctx_t *bmk)
{
 bmk1880v2_parallel_enable(bmk);
 bmk1880v2_tdma_l2g_tensor_copy_cw_transposed(bmk, &l2tg_cw_param);
 bmk1880v2_tdma_g2l_matrix_copy_row_col_transposed(bmk, &tg2l_matrix_param);
 bmk1880v2_tdma_l2l_tensor_copy(bmk, &l2l_tensor_copy_param);
 bmk1880v2_tiu_depthwise_convolution(bmk, &depthwise_conv_param);
 bmk1880v2_parallel_disable(bmk);
 test_submit(ctx);
}

void free_s8_data()
{
  free(s8_test_data.depthwise_conv_input);
  free(s8_test_data.depthwise_conv_weight);
  free(s8_test_data.depthwise_conv_bias);
  free(s8_test_data.depthwise_conv_output);
  free(s8_test_data.depthwise_conv_output_ref);
  free(s8_test_data.l2g_cw_src);
  free(s8_test_data.l2g_cw_output);
  free(s8_test_data.l2g_cw_output_ref);
  free(s8_test_data.g2l_matrix_src);
  free(s8_test_data.g2l_matrix_output);
  free(s8_test_data.g2l_matrix_output_ref);
  free(s8_test_data.l2l_tensor_src);
  free(s8_test_data.l2l_tensor_output);
  free(s8_test_data.l2l_tensor_output_ref);
}

int main()
{
  CVI_RT_HANDLE ctx;
  bmk_ctx_t *bk_ctx;
  test_init(&ctx, &bk_ctx);

  printf("depthwise max_power test\n");

  test_pooling(ctx, bk_ctx);
  test_l2tg_cw_transpose(&ctx, bk_ctx, &l2tg_cw_param);
  test_tg2l_matrix_transpose(&ctx, bk_ctx, &tg2l_matrix_param);
  test_l2l_tensor_copy(&ctx, bk_ctx, &l2l_tensor_copy_param);

  trigger_max_power(&ctx, bk_ctx);
  get_result(&ctx, bk_ctx);
  check_result();

  destroy_param_l2l(bk_ctx,&l2l_tensor_copy_param);
  destroy_param_g2l(&ctx, bk_ctx, &tg2l_matrix_param);
  destroy_param_l2g(&ctx, bk_ctx, &l2tg_cw_param);
  free_depthwise_param(bk_ctx, &depthwise_conv_param);
  free_s8_data();
  test_exit(&ctx);
  return 0;
}
