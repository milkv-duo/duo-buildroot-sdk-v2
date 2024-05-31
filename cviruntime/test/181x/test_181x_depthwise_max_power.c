#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <inttypes.h>

#include "test_cvikernel_util.h"
#include "test_native_ref.h"

typedef cvk_tiu_depthwise_pt_convolution_param_t depthwise_conv_param_t;
typedef cvk_tdma_l2g_tensor_copy_cw_transposed_param_t l2g_cw_param_t;
typedef cvk_tdma_g2l_matrix_copy_row_col_transposed_param_t g2l_matrix_param_t;
typedef cvk_tdma_l2l_tensor_copy_param_t l2l_tensor_copy_param_t;

typedef struct{
    int8_t  *depthwise_conv_input;
    int8_t  *depthwise_conv_weight;
    int16_t *depthwise_conv_bias;
    uint8_t  *depthwise_conv_output;
    int8_t  *depthwise_conv_output_ref;
    uint8_t  *l2g_cw_src;
    uint8_t  *l2g_cw_output;
    uint8_t  *l2g_cw_output_ref;
    uint8_t  *g2l_matrix_src;
    uint8_t  *g2l_matrix_output;
    uint8_t  *g2l_matrix_output_ref;
    uint8_t  *l2l_tensor_src;
    uint8_t  *l2l_tensor_output;
    uint8_t  *l2l_tensor_output_ref;
}s_test_data;

depthwise_conv_param_t depthwise_conv_param;
l2g_cw_param_t l2g_cw_param;
g2l_matrix_param_t g2l_matrix_param;
l2l_tensor_copy_param_t l2l_tensor_copy_param;
s_test_data s8_test_data;

cvk_tl_t *skip_tensor_lmem[10];
uint32_t skip_tensor_num=0;

void skip_tensor_lmem_size(cvk_context_t *cvk_ctx, const cvk_tl_t *p)
{
  uint32_t needed = align_up(p->shape.n * p->stride.n, cvk_ctx->info.eu_num);
  uint32_t start_addr = p->start_address + needed;
  uint32_t remain_size = start_addr % cvk_ctx->info.lmem_bank_size ? (cvk_ctx->info.lmem_bank_size - start_addr % cvk_ctx->info.lmem_bank_size) : 0; // remain size for each lane
  if(remain_size)
  {
    cvk_tl_shape_t src_shape2 = tl_shape_t4(1, cvk_ctx->info.npu_num, 1, remain_size);
    skip_tensor_lmem[skip_tensor_num] = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, src_shape2, CVK_FMT_I8, 1); // skip the lmem size and next tl can alignment to bank si     ze
  }
  skip_tensor_num++;
}

void skip_matrix_lmem_size(cvk_context_t *cvk_ctx, const cvk_ml_t *p)
{
  uint32_t needed = align_up(p->shape.n * p->stride.n, cvk_ctx->info.eu_num);

  uint32_t start_addr = p->start_address + needed; //src_shape.n*src_shape.c*src_shape.h*src_shape.w/32;
  uint32_t remain_size = start_addr % cvk_ctx->info.lmem_bank_size ? (cvk_ctx->info.lmem_bank_size - start_addr % cvk_ctx->info.lmem_bank_size) : 0; // remain size for each lane
  if(remain_size)
  {
    cvk_tl_shape_t src_shape2 = {1, cvk_ctx->info.npu_num, 1, remain_size};
    skip_tensor_lmem[skip_tensor_num] = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, src_shape2, CVK_FMT_I8, 1); // skip the lmem size and next tl can alignment to bank si     ze
  }
  skip_tensor_num++;
}

void free_skip_tensor_lmem(cvk_context_t *cvk_ctx)
{
  if(skip_tensor_lmem[--skip_tensor_num]!=NULL)
    cvk_ctx->ops->lmem_free_tensor(cvk_ctx, skip_tensor_lmem[skip_tensor_num]);
}

static int8_t * alloc_input(const depthwise_conv_param_t *p)
{
  uint64_t size = tl_shape_size(&p->ifmap->shape, p->ifmap->fmt);
  int8_t *buf = (int8_t *)malloc(sizeof(int8_t) * size);
  if (!buf)
    return NULL;

  for (uint64_t i = 0; i < size; i++)
    buf[i] = rand() % 256 - 128;

  return buf;
}

static int8_t * alloc_weight(const depthwise_conv_param_t *p)
{
  int size = tl_shape_size(&p->weight->shape, p->weight->fmt);
  int8_t *buf = (int8_t *)malloc(sizeof(int8_t) * size);
  if (!buf)
    return NULL;

  for (int i = 0; i < size; i++)
    buf[i] = rand() % 256 - 128;

  return buf;
}

static int16_t * alloc_bias(const depthwise_conv_param_t *p)
{
  int c = p->bias->shape.c;
  int16_t *bias = (int16_t *)malloc(sizeof(int16_t) * c);
  if (!bias)
    return NULL;

  for (int i = 0; i < c; i++)
    bias[i] = rand() % 65536 - 32768;

  return bias;
}

static int8_t *alloc_output(depthwise_conv_param_t *p)
{
  uint64_t size = tl_shape_size(&p->ofmap->shape, p->ofmap->fmt);
  int8_t *output = (int8_t *)malloc(sizeof(int8_t) * size);
  return output;
}

static inline void relu8(int8_t *buf, uint64_t size)
{
  for (uint64_t i = 0; i < size; i++)
    if (buf[i] < 0)
      buf[i] = 0;
}

static int generate_results(
    depthwise_conv_param_t *p,
    int8_t input[],
    int8_t weight[],
    int16_t bias[]
    )
{
  int in = p->ifmap->shape.n;
  int ic = p->ifmap->shape.c;
  int ih = p->ifmap->shape.h;
  int iw = p->ifmap->shape.w;
  int kh = p->weight->shape.h;
  int kw = p->weight->shape.w;
  int opd0_sign = (p->ifmap->fmt == CVK_FMT_I8);
  int res0_sign = (p->ofmap->fmt == CVK_FMT_I8);
  s8_test_data.depthwise_conv_output_ref = alloc_output(p);

  int ret = native_pooling_ave_int8(
      input, weight, p->bias ? bias : NULL, s8_test_data.depthwise_conv_output_ref,
      in, ic, ih, iw, kh, kw,
      p->pad_top, p->pad_bottom, p->pad_left, p->pad_right,
      p->stride_h, p->stride_w,
      p->ins_h, p->ins_w, p->ins_last_h, p->ins_last_w,
      opd0_sign, res0_sign, p->rshift_bits, 0);
  if (ret)
    return ret;

  if(p->relu_enable )
    relu8(s8_test_data.depthwise_conv_output_ref, tl_shape_size(&p->ofmap->shape, p->ofmap->fmt));

  return ret;
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
    cvk_context_t *cvk_ctx,
    depthwise_conv_param_t *p)
{
  if (p->bias)
  {
    free_skip_tensor_lmem(cvk_ctx);
    cvk_ctx->ops->lmem_free_tensor(cvk_ctx, p->bias);
  }
  if (p->weight)
  {
    free_skip_tensor_lmem(cvk_ctx);
    cvk_ctx->ops->lmem_free_tensor(cvk_ctx, p->weight);
  }
  if (p->ifmap)
  {
    free_skip_tensor_lmem(cvk_ctx);
    cvk_ctx->ops->lmem_free_tensor(cvk_ctx, p->ifmap);
  }
  if (p->ofmap)
  {
    free_skip_tensor_lmem(cvk_ctx);
    cvk_ctx->ops->lmem_free_tensor(cvk_ctx, p->ofmap);
  }
}

static void put_bias_tensor(
    CVI_RT_HANDLE rt_handle,
    cvk_context_t *cvk_ctx,
    const cvk_tl_t *tl,
    int16_t data[])
{
  int c = tl->shape.c;

  uint8_t *lo_hi = (uint8_t *)malloc(2 * c);
  if (!lo_hi)
    return;

  for (int i = 0; i < c; i++) {
    lo_hi[i] = data[i] & 0xff;
    lo_hi[i + c] = (data[i] >> 8) & 0xff;
  }

  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, tl, (uint8_t *)lo_hi);

  free(lo_hi);
}

static depthwise_conv_param_t random_depthwise_param(cvk_context_t *cvk_ctx)
{
  srand(clock());
  depthwise_conv_param_t p;
  int retry_cnt = 100;

  for (int i = 0; i < retry_cnt; i++) {
    int using_bias = 0;
    int n = 1;
    int c = 1000;
    int ih = 2;
    int iw = 8;
    int kh = 1;
    int kw = 1;
    int opd0_sign = 0;

    memset(&p, 0, sizeof(p));
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
    cvk_tl_shape_t ofmap_shape;
    ofmap_shape.n = n;
    ofmap_shape.c = c;
    ofmap_shape.h = oh;
    ofmap_shape.w = ow;
    cvk_tl_shape_t ifmap_shape;
    ifmap_shape.n = n;
    ifmap_shape.c = c;
    ifmap_shape.h = ih;
    ifmap_shape.w = iw;
    cvk_tl_shape_t weight_shape;
    weight_shape.n = 1;
    weight_shape.c = c;
    weight_shape.h = kh;
    weight_shape.w = kw;
    cvk_tl_shape_t bias_shape;
    bias_shape.n = 2;
    bias_shape.c = c;
    bias_shape.h = 1;
    bias_shape.w = 1;
    p.relu_enable = 1;
    /*test case ref does not support dilation !=1*/
    p.dilation_w = 1;
    p.dilation_h = 1;
    cvk_fmt_t ifmt = opd0_sign ? CVK_FMT_I8: CVK_FMT_U8;

    p.ofmap = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, ofmap_shape, CVK_FMT_I8, 1);
    skip_tensor_lmem_size(cvk_ctx, p.ofmap);
    p.ifmap = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, ifmap_shape, ifmt, 1);
    skip_tensor_lmem_size(cvk_ctx, p.ifmap);
    p.weight = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, weight_shape, CVK_FMT_I8, 1);
    skip_tensor_lmem_size(cvk_ctx, p.weight);
    p.bias = NULL;
    if (using_bias)
    {
      p.bias = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, bias_shape, CVK_FMT_I8, 0);
      skip_tensor_lmem_size(cvk_ctx, p.bias);
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
      free_depthwise_param(cvk_ctx, &p);
    } else
      break;
  }

  return p;
}


static int test_pooling(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx)
{
  depthwise_conv_param = random_depthwise_param(cvk_ctx);

  int8_t *input = alloc_input(&depthwise_conv_param);
  int8_t *weight = alloc_weight(&depthwise_conv_param);
  int16_t *bias = NULL;
  if (depthwise_conv_param.bias)
    bias = alloc_bias(&depthwise_conv_param);

  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, depthwise_conv_param.ifmap, (uint8_t *)input);
  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, depthwise_conv_param.weight, (uint8_t *)weight);
  if (depthwise_conv_param.bias)
    put_bias_tensor(rt_handle, cvk_ctx, depthwise_conv_param.bias, bias);

  int ret = generate_results(&depthwise_conv_param, input, weight, bias);

  free(input);
  free(weight);
  free(bias);

  return ret;
}

static void l2g_tensor_copy_cw_transposed_ref(
    l2g_cw_param_t *p, uint8_t ref_data[], uint8_t src_data[])
{
  cvk_tl_shape_t s = p->src->shape;
  uint32_t n = s.n;
  uint32_t c = s.c;
  uint32_t h = s.h;
  uint32_t w = s.w;

  for (uint32_t ni = 0; ni < n; ni++) {
    for (uint32_t ci = 0; ci < c; ci++) {
      for (uint32_t hi = 0; hi < h; hi++) {
        for (uint32_t wi = 0; wi < w; wi++) {
          uint32_t src_i = ni * c * h * w + ci * h * w + hi * w + wi;
          uint32_t dst_i = ni * c * h * w + wi * h * c + hi * c + ci;
          ref_data[dst_i] = src_data[src_i];
        }
      }
    }
  }
}

static void test_param_l2g(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, l2g_cw_param_t *p)
{
  uint64_t size = tl_shape_size(&p->src->shape, p->src->fmt);

  s8_test_data.l2g_cw_src = (uint8_t *)malloc(sizeof(uint8_t) * size);
  if (!s8_test_data.l2g_cw_src)
    return;

  for (uint64_t i = 0; i < size; i++)
    s8_test_data.l2g_cw_src[i] = rand()%0x100;

  s8_test_data.l2g_cw_output_ref = (uint8_t *)malloc(sizeof(uint8_t) * size);
  if (!s8_test_data.l2g_cw_output_ref)
    return;

  l2g_tensor_copy_cw_transposed_ref(p, s8_test_data.l2g_cw_output_ref, s8_test_data.l2g_cw_src);

  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, p->src, s8_test_data.l2g_cw_src);
}

static void destroy_param_l2g(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, l2g_cw_param_t *p)
{
  free_tensor_dev_mem(rt_handle, p->dst);
  free_skip_tensor_lmem(cvk_ctx);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, p->src);
}

static void test_l2g_cw_transpose(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, l2g_cw_param_t *p)
{
  cvk_tl_shape_t src_shape = {1, 0x100, 1, 0x020};
  cvk_tg_shape_t dst_shape = {1, 0x020, 1, 0x100};

  p->src = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, src_shape, CVK_FMT_I8, 1);
  p->dst = alloc_tensor_dev_mem(rt_handle, cvk_ctx, dst_shape, CVK_FMT_I8);
  skip_tensor_lmem_size(cvk_ctx, p->src);
  test_param_l2g(rt_handle, cvk_ctx, p);
}

static void g2l_matrix_copy_row_col_transposed_ref(
    g2l_matrix_param_t *p, uint8_t ref_data[], uint8_t src_data[])
{
  uint64_t row = p->src->shape.row;
  uint64_t col = p->src->shape.col;

  for (uint64_t ri = 0; ri < row; ri++) {
    for (uint64_t ci = 0; ci < col; ci++) {
      uint64_t src_i = ri * col + ci;
      uint64_t dst_i = ci * row + ri;
      ref_data[dst_i] = src_data[src_i];
    }
  }
}

static void test_param_g2l(CVI_RT_HANDLE rt_handle, g2l_matrix_param_t *p)
{
  uint64_t size = ml_shape_size(&p->dst->shape, p->dst->fmt);

  s8_test_data.g2l_matrix_src = (uint8_t *)malloc(sizeof(uint8_t) * size);
  if (!s8_test_data.g2l_matrix_src)
    return;

  for (uint64_t i = 0; i < size; i++)
    s8_test_data.g2l_matrix_src[i] = rand()%0x100;

  s8_test_data.g2l_matrix_output_ref = (uint8_t *)malloc(sizeof(uint8_t) * size);
  if (!s8_test_data.g2l_matrix_output_ref)
    return;

  g2l_matrix_copy_row_col_transposed_ref(p, s8_test_data.g2l_matrix_output_ref, s8_test_data.g2l_matrix_src);

  matrix_copy_s2d(rt_handle, p->src, s8_test_data.g2l_matrix_src);
}

static void destroy_param_g2l(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, g2l_matrix_param_t *p)
{
  free_matrix_dev_mem(rt_handle, p->src);
  free_skip_tensor_lmem(cvk_ctx);
  cvk_ctx->ops->lmem_free_matrix(cvk_ctx, p->dst);
}


static void test_g2l_matrix_transpose(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, g2l_matrix_param_t *p)
{
  //g2l_matrix_param_t p;
  /*
   * Matrix transpose must be n/c stride alignment
   * for TDMA limitation
   */
  cvk_mg_shape_t src_shape={0x100, 0x20};
  cvk_ml_shape_t dst_shape={0x20, 0x10, 0x10, 0x100};

  int dst_align = 1;
  cvk_fmt_t fmt = CVK_FMT_I8;
  p->src = alloc_matrix_dev_mem(rt_handle, src_shape, fmt);
  p->dst = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, dst_shape, fmt, dst_align);
  skip_matrix_lmem_size(cvk_ctx, p->dst);
  test_param_g2l(rt_handle, p);
}

static void l2l_tensor_copy_ref(l2l_tensor_copy_param_t *p, uint8_t ref_data[], uint8_t src_data[])
{
  uint64_t size = tl_shape_size(&p->src->shape, p->src->fmt);

  for (uint64_t i = 0; i < size; i++)
    ref_data[i] = src_data[i];
}

static void test_l2l_param(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, l2l_tensor_copy_param_t *p)
{
  uint64_t size = tl_shape_size(&p->src->shape, p->src->fmt);

  s8_test_data.l2l_tensor_src = (uint8_t *)malloc(sizeof(uint8_t) * size);
  if (!s8_test_data.l2l_tensor_src)
    return;

  for (uint64_t i = 0; i < size; i++)
    s8_test_data.l2l_tensor_src[i] = rand()%0x100;

  s8_test_data.l2l_tensor_output_ref = (uint8_t *)malloc(sizeof(uint8_t) * size);
  if (!s8_test_data.l2l_tensor_output_ref)
    return;

  l2l_tensor_copy_ref(p, s8_test_data.l2l_tensor_output_ref, s8_test_data.l2l_tensor_src);

  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, p->src, s8_test_data.l2l_tensor_src);
}

static void destroy_param_l2l(cvk_context_t *cvk_ctx, l2l_tensor_copy_param_t *p)
{
  free_skip_tensor_lmem(cvk_ctx);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, p->dst);
  free_skip_tensor_lmem(cvk_ctx);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, p->src);
}

static void test_l2l_tensor_copy(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, l2l_tensor_copy_param_t *p)
{
  cvk_tl_shape_t src_shape = {1, 0x10, 0x1, 0x100};
  cvk_tl_shape_t dst_shape = {1, 0x10, 0x1, 0x100};

  p->src = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, src_shape, CVK_FMT_I8, 1);
  skip_tensor_lmem_size(cvk_ctx, p->src);
  p->dst = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, dst_shape, CVK_FMT_I8, 1);
  skip_tensor_lmem_size(cvk_ctx, p->dst);
  test_l2l_param(rt_handle, cvk_ctx, p);
}

void get_result(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx)
{
  s8_test_data.depthwise_conv_output = tensor_copy_l2g_d2s(rt_handle, cvk_ctx, depthwise_conv_param.ofmap);
  s8_test_data.l2g_cw_output = tensor_copy_d2s(rt_handle, l2g_cw_param.dst);
  s8_test_data.g2l_matrix_output = matrix_copy_l2g_d2s(rt_handle, cvk_ctx, g2l_matrix_param.dst);
  s8_test_data.l2l_tensor_output = tensor_copy_l2g_d2s(rt_handle, cvk_ctx, l2l_tensor_copy_param.dst);
}

int check_result(void)
{

  int cmp_res = array_cmp_int8(
      "Comparing results ...\n", s8_test_data.depthwise_conv_output_ref,  (int8_t *)s8_test_data.depthwise_conv_output,
      tl_shape_size(&depthwise_conv_param.ofmap->shape, depthwise_conv_param.ofmap->fmt));
  if (cmp_res != 0) {
    printf("Comparison FAILED!!!\n");
    return -1;
  }

  for (uint64_t i = 0; i < tl_shape_size(&l2g_cw_param.src->shape, l2g_cw_param.src->fmt); i++) {
    if (s8_test_data.l2g_cw_output[i] != s8_test_data.l2g_cw_output_ref[i]) {
      fprintf(stderr, "l2g_cw comparing failed at dst[%" PRIu64 "], got %d, exp %d\n",
              i, s8_test_data.l2g_cw_output[i], s8_test_data.l2g_cw_output_ref[i]);
      return -1;
    }
  }
  for (uint64_t i = 0; i < ml_shape_size(&g2l_matrix_param.dst->shape, g2l_matrix_param.dst->fmt); i++) {
    if (s8_test_data.g2l_matrix_output[i] != s8_test_data.g2l_matrix_output_ref[i]) {
      fprintf(stderr, "g2l_matrix comparing failed at dst[%" PRIu64 "], got %d, exp %d\n",
              i, s8_test_data.g2l_matrix_output[i], s8_test_data.g2l_matrix_output_ref[i]);
      return -1;
    }
  }

  for (uint64_t i = 0; i < tl_shape_size(&l2l_tensor_copy_param.src->shape, l2l_tensor_copy_param.src->fmt); i++) {
    if (s8_test_data.l2l_tensor_output[i] != s8_test_data.l2l_tensor_output_ref[i]) {
      fprintf(stderr, "l2l_tensor comparing failed at dst[%" PRIu64 "], got %d, exp %d\n",
              i, s8_test_data.l2l_tensor_output[i], s8_test_data.l2l_tensor_output_ref[i]);
      return -1;
    }
  }

  return 0;
}

void trigger_max_power(cvk_context_t *cvk_ctx)
{
 cvk_ctx->ops->parallel_enable(cvk_ctx);
 cvk_ctx->ops->tdma_l2g_tensor_copy_cw_transposed(cvk_ctx, &l2g_cw_param);
 cvk_ctx->ops->tdma_g2l_matrix_copy_row_col_transposed(cvk_ctx, &g2l_matrix_param);
 cvk_ctx->ops->tdma_l2l_tensor_copy(cvk_ctx, &l2l_tensor_copy_param);
 cvk_ctx->ops->tiu_pt_depthwise_convolution(cvk_ctx, &depthwise_conv_param);
 cvk_ctx->ops->parallel_disable(cvk_ctx);
 CVI_RT_Submit(cvk_ctx);
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

int main(int argc, char **argv)
{
  int ret = 0;
  CVI_RT_HANDLE rt_handle = NULL;
  cvk_context_t *cvk_ctx = NULL;

  if (!argc)
    return -1;
  if (!argv)
    return -1;

  CVI_RT_Init(&rt_handle);
  cvk_ctx = CVI_RT_RegisterKernel(rt_handle, CMDBUF_SIZE);
  if (!rt_handle || !cvk_ctx) {
    printf("%s fail\n", __FILENAME__);
    return -1;
  }

  printf("depthwise max_power test\n");

  ret |= test_pooling(rt_handle, cvk_ctx);
  test_l2g_cw_transpose(rt_handle, cvk_ctx, &l2g_cw_param);
  test_g2l_matrix_transpose(rt_handle, cvk_ctx, &g2l_matrix_param);
  test_l2l_tensor_copy(rt_handle, cvk_ctx, &l2l_tensor_copy_param);

  trigger_max_power(cvk_ctx);
  get_result(rt_handle, cvk_ctx);
  ret |= check_result();

  destroy_param_l2l(cvk_ctx, &l2l_tensor_copy_param);
  destroy_param_g2l(rt_handle, cvk_ctx, &g2l_matrix_param);
  destroy_param_l2g(rt_handle, cvk_ctx, &l2g_cw_param);
  free_depthwise_param(cvk_ctx, &depthwise_conv_param);
  free_s8_data();

  CVI_RT_UnRegisterKernel(cvk_ctx);
  CVI_RT_DeInit(rt_handle);

  return ret;
}
