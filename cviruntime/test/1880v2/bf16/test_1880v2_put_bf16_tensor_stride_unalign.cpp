#include "../1880v2_test_util.h"
#include "1880v2_bf16_util.h"

static fmt_type input_fmt[] = {
 {FMT_BF16, FMT_BF16},
 {FMT_I8, FMT_I8},
};

static void put_tensor_g2l_stride_unalign_ref(
    void *ref,
    void *a,
    tl_shape_t tl_shape,
    bmk1880v2_tensor_tgmem_stride_t gmem_stride,
    fmt_t fmt)
{
  int n = tl_shape.n;
  int c = tl_shape.c;
  int h = tl_shape.h;
  int w = tl_shape.w;
  int nsrc_byte = 1;
  u16 *u16_ref = NULL;
  u16 *u16_src = NULL;
  u8 *u8_ref = NULL;
  u8 *u8_src = NULL;
    if (fmt == FMT_BF16) {
    nsrc_byte = 2; // FMT_BF16
    u16_ref = (u16 *)ref;
    u16_src = (u16 *)a;
  } else {
    nsrc_byte = 1; // FMT_U8, FMT_I8
    u8_ref = (u8 *)ref;
    u8_src = (u8 *)a;
  }
  int n_str = gmem_stride.n / nsrc_byte;
  int c_str = gmem_stride.c / nsrc_byte;
  int h_str = gmem_stride.h / nsrc_byte;
  int w_str = 1;

  for (int ni = 0; ni < n; ni++) {
    for (int ci = 0; ci < c; ci++) {
      for (int hi = 0; hi < h; hi++) {
        for (int wi = 0; wi < w; wi++) {
          u64 src_i = ni * n_str + ci * c_str + hi * h_str + wi * w_str;
          u64 dst_i = ci * n * h * w + ni * h * w + hi * w + wi;
          if (fmt == FMT_BF16) {
            u16_ref[dst_i] = u16_src[src_i];
          } else {
            u8_ref[dst_i] = u8_src[src_i];
          }
        }
      }
    }
  }

  if (fmt == FMT_BF16) {
    ref = (void *)u16_ref;
  } else {
    ref = (void *)u8_ref;
  }
}

static inline void put_tensor_g2l_stride(
    CVI_RT_HANDLE *ctx,
    bmk_ctx_t *bk_ctx,
    tl_t *tl,
    bmk1880v2_tensor_tgmem_stride_t tg_stride,
    void *data,
    fmt_t fmt)
{
  int n = tl->shape.n;
  int n_stride = tg_stride.n;
  bmshape_t bms = BM_TENSOR_WITH_FMT(n, n_stride, 1, 1,
                                     (fmt == FMT_BF16)? BM_FMT_BF16 : BM_FMT_INT8);
  CVI_RT_MEM devmem = CVI_RT_MemAlloc(*ctx, bmshape_get_size(&bms));
  int ret = CVI_RT_MemCopyS2D(*ctx, devmem, (u8 *)data);
  assert(ret == BM_SUCCESS);

  gaddr_t gaddr = CVI_RT_MemGetPAddr(devmem);

  tg_t tg;
  tg.base_reg_index = 0;
  tg.start_address = gaddr;
  tg.fmt = fmt;
  tg.shape.n = tl->shape.n;
  tg.shape.c = tl->shape.c;
  tg.shape.h = tl->shape.h;
  tg.shape.w = tl->shape.w;
  tg.stride = tg_stride;
  tg.base_reg_index = 0;

  bmk1880v2_tdma_tg2l_tensor_copy_param_t p;
  memset(&p, 0, sizeof(p));
  p.src = &tg;
  p.dst = tl;

  bmk1880v2_tdma_g2l_bf16_tensor_copy(bk_ctx, &p);
  test_submit(ctx);

  CVI_RT_MemFree(*ctx, devmem);
}

static void test_put_tensor_g2l_stride_unalign(
    CVI_RT_HANDLE *ctx, bmk_ctx_t *bk_ctx, fmt_t fmt, int eu_align)
{
  tl_t *tl_x = NULL;
  int n = 6;
  int c = 9; //just larger than (npu_num/2)
  int h = 1;
  int w = 8;

  tl_shape_t tl_shape;
  tl_shape.n = n;
  tl_shape.c = c;
  tl_shape.h = h;
  tl_shape.w = w;

  bmk1880v2_tensor_tgmem_stride_t gmem_stride;
  gmem_stride.h = w * 2;
  gmem_stride.c = gmem_stride.h * h * 2;
  gmem_stride.n = gmem_stride.c * c * 2;

  int size = n * c * h * w;
  int stride_size = gmem_stride.n * n;
  float val = -100;
  void *src_data = NULL;
  u16 *u16src_data = (u16 *)malloc(sizeof(u16) * stride_size);
  s8 *s8src_data = (s8 *)malloc(sizeof(s8) * stride_size);
  u8 *result_x = NULL;
  void *ref_x = NULL;

  // prepare source data
  ref_x = (u8 *)xmalloc(size * ((fmt == FMT_BF16) ?2:1) );
  for (int i = 0; i < stride_size; i++) {
    if(fmt == FMT_BF16) {
      u16src_data[i] = generate_bf16_corner_val(val);
      val += 0.1;
    } else {
      s8src_data[i] = i;
    }
  }
  src_data =  (fmt == FMT_BF16) ? (void *)u16src_data : (void *)s8src_data;

  // run tpu operations
  tl_x = alloc_tl(bk_ctx, tl_shape, fmt, eu_align);
  put_tensor_g2l_stride(ctx, bk_ctx, tl_x, gmem_stride, (u8 *)src_data, fmt);
  tl_x->shape.n = 1;
  tl_x->shape.c = c;
  tl_x->shape.h = n * h;
  tl_x->shape.w = w;
  result_x = get_bf16_tensor_l2g(ctx, bk_ctx, tl_x, fmt);
  put_tensor_g2l_stride_unalign_ref(ref_x, src_data, tl_shape, gmem_stride, fmt);

  // compare data
  if( COMPARE_PASS != compare_result( ref_x, result_x, fmt, size))
      exit(-1);

  // free variables
  free_tl(bk_ctx, tl_x);
  free(ref_x);
  free(result_x);
  free(s8src_data);
  free(u16src_data);
}

#define TEST_ALIGNED 1 // 1: test unalign only, 2: test both align/unalign
int main (void)
{
  CVI_RT_HANDLE ctx;
  bmk_ctx_t *bk_ctx;

  u32 nr_fmt = sizeof(input_fmt) / sizeof(input_fmt[0]);

  test_init(&ctx, &bk_ctx);
  for (u32 i = 0; i < nr_fmt; i++) {
    for (u8 u8_align = 0; u8_align < TEST_ALIGNED; u8_align++) {
        test_put_tensor_g2l_stride_unalign(&ctx, bk_ctx, input_fmt[i].src_fmt, u8_align);
    }
  }
  test_exit(&ctx);
  return 0;
}
