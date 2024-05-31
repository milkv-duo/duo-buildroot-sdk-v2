#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <errno.h>
#include <assert.h>

#include <test_native_ref.h>

#define math_min(x, y)          ((x) < (y) ? (x) : (y))
#define math_max(x, y)          ((x) > (y) ? (x) : (y))

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef u32 bmerr_t;

#define BM_SUCCESS 0               // The operation was successful
#define BM_ERR_AGAIN 1             // Not ready yet
#define BM_ERR_FAILURE 2           // General failure
#define BM_ERR_TIMEOUT 3           // Timeout
#define BM_ERR_UNINITIALIZED 4     // Uninitialzed
#define BM_ERR_INVALID_ARGUMENT 5  // Arguments invalid
#define BM_ERR_NOMEM 6             // Not enough memory
#define BM_ERR_DATA 7              // Data error
#define BM_ERR_BUSY 8              // Busy
#define BM_ERR_NOT_SUPPORTED 9     // Not supported yet

typedef u32 BLOB_OP;
#define BLOB_ADD 0
#define BLOB_SUB 1
#define BLOB_MUL 2
#define BLOB_DIV 3
#define BLOB_INVALID 4



static inline int calc_offset(int *shape, int *offset)
{
  return ((offset[0] * shape[1] + offset[1]) * shape[2] + offset[2])
      * shape[3] + offset[3];
}

static int index_get(int h, int w1, int w2)
{
  return h * w1 + w2;
}

int array_cmp_float_rel(const char * const info, float *p_exp, float *p_got,
    int count, float delta)
{
  int idx = 0;
  for (idx = 0; idx < count; idx++) {
    if (math_max( fabs(p_exp[idx]), fabs(p_got[idx])) > 1.0 ) {
      // compare rel
      if( math_min(fabs(p_exp[idx]), fabs(p_got[idx])) < 1e-20 ) {
        printf("%s rel error at index %d exp %.20f got %.20f\n",
            info, idx, p_exp[idx], p_got[idx]);
        if(isnan(p_exp[idx]) && isnan(p_got[idx])){
          printf("both exp and got are NAN");
          return 0;
        }
        return -1;
      }
      if (fabs(p_exp[idx] - p_got[idx]) >
          delta * math_min(fabs(p_exp[idx]), fabs( p_got[idx]))) {
        printf("%s rel error at index %d exp %.20f got %.20f\n",
            info, idx, p_exp[idx], p_got[idx]);
        if(isnan(p_exp[idx]) && isnan(p_got[idx])){
          printf("both exp and got are NAN");
          return 0;
        }
        return -1;
      }
    } else {
      if ( fabs(p_exp[idx] - p_got[idx]) > delta ) {
        printf("%s abs error at index %d exp %.20f got %.20f\n",
            info, idx, p_exp[idx], p_got[idx]);
        if(isnan(p_exp[idx]) && isnan(p_got[idx])){
          printf("both exp and got are NAN");
          return 0;
        }
        return -1;
      }
    }

    if ( isnan(p_got[idx]) && !isnan(p_exp[idx])) {
      printf("%s, found nans idx %d\n", info , idx);
      printf("floating from exp %.10f got %.10f\n", p_exp[idx], p_got[idx]);
      IF_VAL exp, got;
      exp.fval = p_exp[idx];
      got.fval = p_got[idx];
      printf("hex form exp %8.8x got %8.8x\n", exp.ival, got.ival);
      return -2;
    }
  }
  return 0;
}

int array_cmp_float(const char * const info, float *p_exp, float *p_got,
    int count, float delta)
{
  if (delta == 0.0f) {
    for (int idx = 0; idx < count; idx++) {
      if (p_exp[idx] != p_got[idx]) {
        printf("%s error at index %d exp %.20f got %.20f\n",
            info, idx, p_exp[idx], p_got[idx]);
        if(isnan(p_exp[idx]) && isnan(p_got[idx])){
          printf("both exp and got are NAN\n");
          return 0;
        }
        return -1;
      }
    }
  } else {
    return array_cmp_float_rel(info, p_exp, p_got, count, delta);
  }
  return 0;
}

int array_cmp_int(const char * const info, int *p_exp, int *p_got, int count)
{
  int idx;
  for (idx = 0; idx < count; idx++) {
    if (p_exp[idx] != p_got[idx]) {
      printf("%s error at index %d exp %d got %d\n",
          info, idx, p_exp[idx], p_got[idx]);
      return -1;
    }
  }
  return 0;
}

int array_cmp_int8(const char * const info, const int8_t *p_exp,
    const int8_t *p_got, int count)
{
  int idx;
  for (idx = 0; idx < count; idx++) {
    if (p_exp[idx] != p_got[idx]) {
      printf("%s error at index %d exp %d got %d\n",
          info, idx, p_exp[idx], p_got[idx]);
      return -1;
    }
  }
  return 0;
}

int calc_dilute_hw(int h, int ins_h, int ins_h_l, int pad_h_b, int pad_h_t)
{
  return (h - 1) * (ins_h + 1) + ins_h_l +
                    1 + pad_h_t + pad_h_b;
}

int calc_output_hw(int hw, int khw, int stride)
{
  return (hw - khw)/stride + 1;
}


int fill_pad_fmap_int8(
    const int8_t *before, int8_t **pafter, int val,
    int pad_l, int pad_r, int pad_t, int pad_b,
    int ins_h, int ins_w, int ins_h_last, int ins_w_last,
    int h_before, int w_before)
{
  if (!before || !pafter)
      return BM_ERR_INVALID_ARGUMENT;

  int w_after = (w_before - 1) * (ins_w + 1) + ins_w_last + 1 + pad_l + pad_r;
  int h_after = (h_before - 1) * (ins_h + 1) + ins_h_last + 1 + pad_t + pad_b;
  int8_t *after = *pafter;

  if (!after) {
    after = malloc(sizeof(int8_t) * w_after * h_after);
    if (!after)
        return BM_ERR_NOMEM;
  }

  memset(after, val, w_after * h_after);
  for (int h = 0; h < h_before; h++) {
    for (int w = 0; w < w_before; w++) {
      int i = (h * (ins_h + 1) + pad_t) * w_after + w * (ins_w + 1) + pad_l;
      after[i] = before[h * w_before + w];
    }
  }

  *pafter = after;
  return BM_SUCCESS;
}

int fill_pad_fmap_bf16(
    const u16 *before, u16 **pafter, int val,
    int pad_l, int pad_r, int pad_t, int pad_b,
    int ins_h, int ins_w, int ins_h_last, int ins_w_last,
    int h_before, int w_before)
{
  if (!before || !pafter)
      return BM_ERR_INVALID_ARGUMENT;

  int w_after = (w_before - 1) * (ins_w + 1) + ins_w_last + 1 + pad_l + pad_r;
  int h_after = (h_before - 1) * (ins_h + 1) + ins_h_last + 1 + pad_t + pad_b;
  u16 *after = *pafter;
  if (!after) {
    after = malloc(sizeof(u16) * w_after * h_after);
    if (!after)
        return BM_ERR_NOMEM;
  }
  for(int i=0 ; i < w_after * h_after; i ++)
    after[i] = val;

  for (int h = 0; h < h_before; h++) {
    for (int w = 0; w < w_before; w++) {
      int i = (h * (ins_h + 1) + pad_t) * w_after + w * (ins_w + 1) + pad_l;
      after[i] = before[h * w_before + w];
    }
  }
#if 0
  printf("bf16 padding:\n");
  for(int i=0;i<h_after;i++) {
    printf("[\n");
    for(int j=0;j<w_after;j++)
      printf("%04x ", (after[i*w_after+j]));
    printf("\n");
  }
 printf("]\n");
#endif
  *pafter = after;
  return BM_SUCCESS;
}

void fill_int_with_int8(int* pdest, int8_t * psrc, int len)
{
  for(int ii=0; ii<len; ii++)
    pdest[ii] = (int)psrc[ii];
}

void fill_int_with_uint8(int *pdest, uint8_t *psrc, int len)
{
  for(int ii=0; ii<len; ii++)
    pdest[ii] = psrc[ii];
}

void fill_int_with_int16(int* pdest, int16_t* psrc, int len)
{
  for(int ii=0; ii<len; ii++) {
    pdest[ii] = (int16_t)psrc[ii];
  }
}

void inner_product(const int* a, const int* b, int len, int *c)
{
  *c = 0;
  for(int ii=0; ii<len; ii++) {
    *c += (a[ii]*b[ii]);
  }
}

void inner_float_product(const float* a, const float* b, int len, float *c)
{
  *c = 0;
  for(int ii=0; ii<len; ii++) {
    *c += (a[ii]*b[ii]);
  }
}

int fill_pad_fmap_fp32(const float *before, float **after, float pad_value,
    int pad_h_t, int pad_h_b, int pad_w_l, int pad_w_r,
    int ins_h, int ins_w, int ins_h_l, int ins_w_l,
    int h, int w)
{
  int h_after = calc_dilute_hw(h, ins_h, ins_h_l, pad_h_b, pad_h_t);
  int w_after = calc_dilute_hw(w, ins_w, ins_w_l, pad_w_l, pad_w_r);
  float *ofmap = NULL;

  if (before == NULL || after == NULL) {
    return BM_ERR_INVALID_ARGUMENT;
  }
  if (*after == NULL
      && (*after = malloc(sizeof(float)*h_after*w_after)) == NULL) {
    printf("No enough memory: [h_after, w_after]=[%i, %i].\n",
           h_after, w_after);
    return BM_ERR_NOMEM;
  }

  ofmap = *after;
  for (int i = 0; i < h_after*w_after; i++) {
    ofmap[i] = pad_value;
  }
  for (int i = 0; i < h; i++) {
    float *start_addr = ofmap + (pad_h_t+i*(ins_h+1))*w_after + pad_w_l;
    int ins_h_count = (i == h-1) ? ins_h_l : ins_h;

    for (int j = 0; j < ins_h_count+1; j++) {
      memset(start_addr+j*w_after, 0,
             sizeof(float)*(w_after-pad_w_l-pad_w_r));
    }
    for (int j = 0; j < w; j++) {
      start_addr[j*(ins_w+1)] = before[i*w+j];
    }
  }

  return BM_SUCCESS;
}

void native_md_scalar(float *a, float *b, float *r,
    int N, int C, int H, int W, int op, bool result_add)
{
  int count = N * C * H * W;
  for (int i = 0; i < count; i++) {
    switch (op) {
    case BLOB_ADD:
      r[i] = a[i] + b[i];
      break;
    case BLOB_SUB:
      r[i] = a[i] - b[i];
      break;
    case BLOB_MUL:
      r[i] = result_add ? r[i] : 0;
      r[i] += a[i] * b[i];
      break;
    case BLOB_DIV:
      r[i] = a[i] / b[i];
      break;
    default:
      assert(0);
      break;
    }
  }
}

void native_md_scalar_int8(int8_t *a, int8_t *b, int8_t *r,
    int N, int C, int H, int W, int op, bool result_add)
{
  int count = N * C * H * W;
  for (int i = 0; i < count; i++) {
    switch (op) {
    case BLOB_ADD:
      r[i] = a[i] + b[i];
      break;
    case BLOB_SUB:
      r[i] = a[i] - b[i];
      break;
    case BLOB_MUL:
      r[i] = result_add ? r[i] : 0;
      r[i] += a[i] * b[i];
      break;
    case BLOB_DIV:
      r[i] = a[i] / b[i];
      break;
    default:
      assert(0);
      break;
    }
  }
}

static int matrix_dot_mult(
    int8_t *A, int8_t *B, int dim_n, int dim_m,
    int opd0_sign)
{
  int sum = 0;
  for (int i=0; i<dim_n; i++){
    for (int j=0; j<dim_m; j++){
      int index = index_get(i, dim_m, j);
      if(opd0_sign) {
        sum += A[index] * B [index];
      } else {
        sum += (int)((uint8_t)A[index]) * B [index];
      }
    }
  }
  return sum;
}

int native_conv_int8(
    const int8_t *ifmap, const int8_t *weight, const int16_t *bias,
    int8_t *ofmap,
    int in, int ic, int ih, int iw, int oc,
    int kh, int kw, int dh, int dw,
    int pad_h_t, int pad_h_b, int pad_w_l, int pad_w_r,
    int stride_h, int stride_w,
    int ins_h, int ins_w,
    int ins_h_last, int ins_w_last,
    int input_sign, int r_shift_width, int do_relu)
{
  int ih_ext = calc_dilute_hw(ih, ins_h, ins_h_last,  pad_h_t, pad_h_b);
  int iw_ext = calc_dilute_hw(iw, ins_w, ins_w_last,  pad_w_l, pad_w_r);
  int kh_ext = calc_dilute_hw(kh, dh - 1, 0, 0, 0);
  int kw_ext = calc_dilute_hw(kw, dw - 1, 0, 0, 0);

  int oh = calc_output_hw(ih_ext, kh_ext, stride_h);
  int ow = calc_output_hw(iw_ext, kw_ext, stride_w);

  int *result = (int *)malloc(sizeof(int) * in * oc * oh * ow);
  int8_t *i_fmap_pad_ker = (int8_t *)malloc(kh_ext * kw_ext);
  if (!result || !i_fmap_pad_ker) {
    free(result);
    free(i_fmap_pad_ker);
    return -1;
  }

  memset(result, 0, sizeof(int) * in * oc * oh * ow);

  int ret = BM_SUCCESS;

  int8_t *i_fmap_pad = NULL;
  int8_t *kernel_after = NULL;
  for (int n = 0; n < in; ++n) {
    for (int c = 0; c < oc; ++c) {
      for (int cc = 0; cc < ic; ++cc) {
        fill_pad_fmap_int8(
            (int8_t*)ifmap + n*ic*ih*iw + cc*ih*iw, &i_fmap_pad, 0,
            pad_w_l, pad_w_r, pad_h_t, pad_h_b,
            ins_h, ins_w, ins_h_last, ins_w_last, ih, iw);

        //kernel_dilation(
        fill_pad_fmap_int8(
            (weight + c*ic*kh*kw + cc*kh*kw), &kernel_after, 0,
            0, 0, 0, 0,  // no padding
            dh - 1, dw - 1, 0, 0,
            kh, kw);

        for (int ph = 0; ph < oh; ++ph) {
          for (int pw = 0; pw < ow; ++pw) {
            for (int idxh = 0; idxh < kh_ext; ++idxh)
              for (int idxw = 0; idxw < kw_ext; ++idxw){
                i_fmap_pad_ker[idxh * kw_ext + idxw] =
                    i_fmap_pad[(idxh+ph*stride_h) * iw_ext +
                               idxw + pw*stride_w];
              }
            result[n*oc*oh*ow + c*oh*ow + ph*ow + pw] +=
                matrix_dot_mult(i_fmap_pad_ker, kernel_after,
                                kh_ext, kw_ext, input_sign);
          }
        }
      }

      if (bias) {
        for (int ph = 0; ph < oh; ++ph) {
          for (int pw = 0; pw < ow; ++pw) {
            result[n*oc*oh*ow + c*oh*ow + ph*ow + pw] += bias[c]; //bias+c ;
          }
        }
      }

      ret = satu_2_8bit(&result[n*oc*oh*ow + c*oh*ow], oh * ow,
              &ofmap[n*oc*oh*ow + c*oh*ow], r_shift_width, 1,
              !do_relu);

      if (ret != BM_SUCCESS)
        goto error_release;
    } //end for (int c = 0; c < oc; ++c)
  } //end for (int n = 0; n < in; n++)

error_release:
  free(i_fmap_pad);
  free(kernel_after);
  free(i_fmap_pad_ker);
  free(result);

  return ret;
}

int native_depthwise_fp32(
    const float *ifmap, const float *weight, const float *bias, float *ofmap,
    int in, int ic, int ih, int iw,
    int kh, int kw, int dh, int dw,
    int pad_h_t, int pad_h_b, int pad_w_l, int pad_w_r,
    int stride_h, int stride_w,
    int ins_h, int ins_w,
    int ins_h_last, int ins_w_last)
{
  int h_after = calc_dilute_hw(ih, ins_h, ins_h_last, pad_h_t, pad_h_b);
  int w_after = calc_dilute_hw(iw, ins_w, ins_w_last, pad_w_l, pad_w_r);
  int kh_dilation = (kh-1)*dh + 1, kw_dilatoin = (kw-1)*dw + 1;
  int oh = calc_output_hw(h_after, kh_dilation, stride_h);
  int ow = calc_output_hw(w_after, kw_dilatoin, stride_w);
  float *ifmap_after = malloc(sizeof(float)*h_after*w_after);
  float *weight_dilation = malloc(sizeof(float)*kh_dilation*kw_dilatoin);

  if (ifmap_after == NULL || weight_dilation == NULL) {
    printf("No enough memory.\n");
    free(ifmap_after);
    free(weight_dilation);

    return BM_ERR_NOMEM;
  }

  for (int n = 0; n < in; n++) {
    for (int c = 0; c < ic; c++, ifmap +=ih*iw, ofmap += oh*ow) {
      float init_value = bias ? bias[c] : 0;
      int ret_ifmap = fill_pad_fmap_fp32(ifmap, &ifmap_after, 0,
                        pad_h_t, pad_h_b, pad_w_l, pad_w_r,
                        ins_h, ins_w, ins_h_last, ins_w_last,
                        ih, iw);
      int ret_weight = fill_pad_fmap_fp32(weight+c*kh*kw, &weight_dilation, 0,
                         0, 0, 0, 0,
                         dh-1, dw-1, 0, 0,
                         kh, kw);

      if ((ret_ifmap != BM_SUCCESS) || (ret_weight != BM_SUCCESS)) {
        printf("failed to pad ifmap or weight.\n");
        return BM_ERR_FAILURE;
      }

      for (int h = 0; h < oh; h++) {
        for (int w = 0; w < ow; w++) {
          int rf_h = h*stride_h, rf_w = w*stride_w;
          int kh_end = math_min(kh_dilation, h_after-rf_h);
          int kw_end = math_min(kw_dilatoin, w_after-rf_w);
          float *rf_addr = ifmap_after + rf_h*w_after + rf_w;
          float dot_product_even = 0.0, dot_product_odd = 0.0;

          for (int i = 0; i < kh_end; i++) {
            for (int j = 0; j < kw_end; j++) {
              if ((i*kw_end+j)%2) {
                dot_product_odd += rf_addr[i*w_after+j]
                                * weight_dilation[i*kw_dilatoin+j];
              }
              else {
                dot_product_even += rf_addr[i*w_after+j]
                                 * weight_dilation[i*kw_dilatoin+j];
              }
            }
          }
          ofmap[h*ow+w] = dot_product_even + dot_product_odd + init_value;
        }
      }
    }
  }

  free(ifmap_after);
  free(weight_dilation);

  return BM_SUCCESS;
}

void native_conv_ref(
    const void *ifmap, void *ofmap, const void *weight,
    int input_n, int input_c, int input_h, int input_w,
    int output_c, int output_h, int output_w,
    int groups,
    int kh, int kw,
    int dilation_h, int dilation_w,
    int pad_h, int pad_w,
    int stride_h, int stride_w,
    int flip,
    int using_bias,
    const void *bias,
    int result_add)
{
  int kh_extent = dilation_h * (kh - 1) + 1;
  int kw_extent = dilation_w * (kw - 1) + 1;
  int output_h_expect = (input_h + 2 * pad_h - kh_extent) / stride_h + 1;
  int output_w_expect = (input_w + 2 * pad_w - kw_extent) / stride_w + 1;
  assert(output_h == output_h_expect && "Expect same output_h");
  assert(output_w == output_w_expect && "Expect same output_w");

  if (!result_add) {
    memset(ofmap, 0, input_n * output_c * output_h * output_w * sizeof(float));
  }

  float *ifmap_f = (float *)ifmap;
  float *ofmap_f = (float *)ofmap;
  float *weight_f = (float *)weight;
  float *bias_f = (float *)bias;
  int i_shape[4];
  i_shape[0] = input_n;
  i_shape[1] = input_c;
  i_shape[2] = input_h;
  i_shape[3] = input_w;
  int o_shape[4];
  o_shape[0] = input_n;
  o_shape[1] = output_c;
  o_shape[2] = output_h;
  o_shape[3] = output_w;
  int k_shape[4];
  k_shape[0] = output_c;
  k_shape[1] = input_c / groups;
  k_shape[2] = kh;
  k_shape[3] = kw;

  int o_g = output_c / groups;
  int k_g = input_c / groups;
  int o_head, k_head;
  int weight_offset[4];
  int in_offset[4];
  int out_offset[4];

  for (int n = 0; n < input_n; n++) {
    for (int g = 0; g < groups; g++) {
      o_head = o_g * g;
      k_head = k_g * g;
      for (int o = 0; o < o_g; o++) {
        for (int y = 0; y < output_h; y++) {
          for (int x = 0; x < output_w; x++) {
            out_offset[0] = n;
            out_offset[1] = o + o_head;
            out_offset[2] = y;
            out_offset[3] = x;
            float result_init = ofmap_f[calc_offset(o_shape, out_offset)];
            ofmap_f[calc_offset(o_shape, out_offset)] = 0.0f;
            for (int k = 0; k < k_g; k++) {
              for (int p = 0; p < kh; p++) {
                for (int q = 0; q < kw; q++) {
                  int in_y = y * stride_h - pad_h + p * dilation_h;
                  int in_x = x * stride_w - pad_w + q * dilation_w;
                  if (in_y >= 0 && in_y < input_h
                      && in_x >= 0 && in_x < input_w) {
                    weight_offset[0] = o + o_head;
                    weight_offset[1] = k;
                    if (flip) {
                      weight_offset[2] = (kh - 1 - p);
                      weight_offset[3] = (kw - 1 - q);
                    } else {
                      weight_offset[2] = p;
                      weight_offset[3] = q;
                    }
                    in_offset[0] = n;
                    in_offset[1] = k + k_head;
                    in_offset[2] = in_y;
                    in_offset[3] = in_x;
                    ofmap_f[calc_offset(o_shape, out_offset)] +=
                        ifmap_f[calc_offset(i_shape, in_offset)]
                        * weight_f[calc_offset(k_shape, weight_offset)];
                    if(k_g==1&&kh==1&&kw==1){
                      ofmap_f[calc_offset(o_shape, out_offset)] =
                          ifmap_f[calc_offset(i_shape, in_offset)]
                          * weight_f[calc_offset(k_shape, weight_offset)];
                    }
                  }
                }
              }
            }
            if (using_bias) {
              ofmap_f[calc_offset(o_shape, out_offset)] += bias_f[o + o_head];
            }
            if (result_add) {
              ofmap_f[calc_offset(o_shape, out_offset)] += result_init;
            }
          }
        }
      }
    }
  }
}

int native_fc_int8(
    const int8_t *L, const int8_t *R, const int16_t *B, const int16_t *Y,
    int *Y_ref,
    int L_row_num, int L_col_num, int R_col_num,
    int L_sign, int R_sign, int B_sign,
    int l_shift_width, int r_shift_width,
    int is_result_int8, int do_relu)
{
  const uint8_t *uL = (const uint8_t*)L;
  const uint8_t *uR = (const uint8_t*)R;
  const uint16_t *uB = (const uint16_t*)B;

  int opd0, opd1, opd2;
  int ret = BM_SUCCESS;

  for (int hidx = 0; hidx < L_row_num; hidx++) {
    for (int widx = 0; widx < R_col_num; widx++) {
      int Y1 = 0;
      int Y2 = 0;
      int sum_idx = 0;
      for (sum_idx = 0; sum_idx < L_col_num; sum_idx++) {
        int idx_L = index_get(hidx, L_col_num,  sum_idx);
        int idx_R = index_get(sum_idx, R_col_num, widx);
        opd0 = (L_sign) ? L[idx_L] : uL[idx_L];
        opd1 = (R_sign) ? R[idx_R] : uR[idx_R];
        if((sum_idx % 2 == 0 && sum_idx != 2) || sum_idx == 1){
          Y1 += opd0 * opd1;
        } else {
          Y2 += opd0 * opd1;
        }
      }
      sum_idx++;

      if (B){
        opd2 = (B_sign) ? (int)B[widx] : (int)uB[widx];
        if((sum_idx % 2 == 0 && sum_idx != 2) || sum_idx == 1){
          Y1 += opd2;
        } else {
          Y2 += opd2;
        }
        sum_idx++;
      }

      int idx_Y = index_get(hidx, R_col_num, widx);
      if (Y){
        if((sum_idx % 2 == 0 && sum_idx != 2) || sum_idx == 1){
          Y1 += (Y[idx_Y] << l_shift_width);
        } else {
          Y2 += (Y[idx_Y] << l_shift_width);
        }
      }

      Y_ref[idx_Y] = Y1 + Y2;
    }
  }
  uint8_t* Yout_int8 = malloc(sizeof(int8_t) * L_row_num * R_col_num);
  uint16_t* Yout_int16 = malloc(sizeof(int16_t) * L_row_num * R_col_num);

  if(is_result_int8)  {
    ret = satu_2_8bit(Y_ref, L_row_num * R_col_num,
                      (int8_t *)Yout_int8, r_shift_width, 1, !do_relu);
    if (ret != BM_SUCCESS)
      goto error_release;

    fill_int_with_int8(Y_ref, (int8_t *)Yout_int8, L_row_num * R_col_num);
  } else {
    ret = satu_2_16bit(Y_ref, L_row_num * R_col_num,
                       (int16_t *)Yout_int16, r_shift_width, 1, !do_relu);
    if (ret != BM_SUCCESS)
      goto error_release;

    fill_int_with_int16(Y_ref, (int16_t *)Yout_int16, L_row_num * R_col_num);
  }

error_release:
  free(Yout_int8);
  free(Yout_int16);

  return ret;
}

int native_pooling_ave_int8(
    const int8_t* i_fmap,
    const void* weight,
    const int16_t *bias,
    int8_t* o_fmap,
    int input_n, int input_c, int input_h, int input_w,
    int kh, int kw,
    int pad_h_t, int pad_h_b, int pad_w_l, int pad_w_r,
    int stride_h, int stride_w,
    int ins_h, int ins_w,
    int ins_h_last, int ins_w_last,
    int input_sign, int satu_sign,
    int r_shift_width, int const_weight)
{
  if (kh * kw <= 0)
    return BM_ERR_INVALID_ARGUMENT;

  int *avg_pooling_mac_a = (int *)malloc(kh * kw * sizeof(int));
  int *avg_pooling_mac_b = (int *)malloc(kh * kw * sizeof(int));

  uint8_t avg_const_weight = *(uint8_t *)weight;
  const int8_t *weight_arr = weight;

  int h_after = calc_dilute_hw(input_h, ins_h, ins_h_last, pad_h_t, pad_h_b);
  int w_after = calc_dilute_hw(input_w, ins_w, ins_w_last, pad_w_l, pad_w_r);

  int output_h = calc_output_hw(h_after, kh, stride_h);
  int output_w = calc_output_hw(w_after, kw, stride_w);

  int8_t *i_fmap_pad = NULL;
  for (int n = 0; n < input_n; n++) {
    if (const_weight == 0)
      weight_arr = weight;

    for (int c = 0; c < input_c; ++c) {
      fill_pad_fmap_int8(i_fmap, &i_fmap_pad, 0,
          pad_w_l, pad_w_r, pad_h_t, pad_h_b,
          ins_h, ins_w, ins_h_last, ins_w_last,
          input_h, input_w);
      for (int ph = 0; ph < output_h; ++ph) {
        for (int pw = 0; pw < output_w; ++pw) {
          int hstart = ph * stride_h;
          int wstart = pw * stride_w;
          int pool_index = index_get(ph, output_w, pw);
          int mac_index = 0;
          int avg_pool_result;

          for (int h = 0; h < kh; h++) {
            for (int w = 0; w < kw; w++) {
              int index = index_get((hstart+h), w_after, (w+wstart));
              mac_index = index_get(h, kw, w);
              avg_pooling_mac_a[mac_index] = input_sign ?
                  i_fmap_pad[index] : (uint8_t)(i_fmap_pad[index]);

              avg_pooling_mac_b[mac_index] = const_weight ?
                  avg_const_weight : weight_arr[mac_index];
            }
          }

          inner_product(avg_pooling_mac_a, avg_pooling_mac_b, kh * kw,
              &avg_pool_result);

          if(bias) {
            avg_pool_result += bias[c];
          }

          int ret = satu_2_8bit(&avg_pool_result, sizeof(int8_t),
                        o_fmap + pool_index, r_shift_width, 1,
                        satu_sign);

          if (ret != BM_SUCCESS) {
            free(i_fmap_pad);
            free(avg_pooling_mac_a);
            free(avg_pooling_mac_b);

            return BM_ERR_INVALID_ARGUMENT;
          }
        }
      }
      i_fmap += input_w * input_h;
      if (const_weight == 0)
        weight_arr += kh * kw;

      o_fmap += output_w * output_h;
    }
  }
  free(i_fmap_pad);

  free(avg_pooling_mac_a);
  free(avg_pooling_mac_b);

  return BM_SUCCESS;
}

int native_pooling_max_int8(
    const int8_t* i_fmap,
    int8_t* o_fmap,
    int input_n, int input_c, int input_h, int input_w,
    int kh, int kw,
    int pad_h_t, int pad_h_b, int pad_w_l, int pad_w_r,
    int stride_h, int stride_w,
    int ins_h, int ins_w,
    int ins_h_last, int ins_w_last,
    int input_sign)
{
  if (ins_h != 0 || ins_w != 0 || ins_h_last != 0  || ins_w_last !=0)
    return BM_ERR_INVALID_ARGUMENT;

  int h_after = calc_dilute_hw(input_h, ins_h, ins_h_last, pad_h_t, pad_h_b);
  int w_after = calc_dilute_hw(input_w, ins_w, ins_w_last, pad_w_l, pad_w_r);

  int output_h = calc_output_hw(h_after, kh, stride_h);
  int output_w = calc_output_hw(w_after, kw, stride_w);

  const int max_init = input_sign? -128: 0;
  int8_t *i_fmap_pad = NULL;
  for (int nc = 0; nc < input_n * input_c; nc++) {
    fill_pad_fmap_int8(i_fmap, &i_fmap_pad, max_init,
      pad_w_l, pad_w_r, pad_h_t, pad_h_b,
      0, 0, 0, 0, input_h, input_w);

    for (int ph = 0; ph < output_h; ++ph) {
      for (int pw = 0; pw < output_w; ++pw) {
        int hstart = ph * stride_h;
        int wstart = pw * stride_w;
        int pool_index = index_get(ph, output_w, pw);
        int max = max_init;
        for (int h = 0; h < kh; h++) {
          for (int w = 0; w < kw; w++) {
            int index = index_get((hstart + h), (input_w + pad_w_l + pad_w_r),
                            (w + wstart));
            int val = input_sign ? i_fmap_pad[index]: (uint8_t)i_fmap_pad[index];
            max = (val > max)? val: max;
          }
        }
        o_fmap[pool_index] = max;
      }
    }
    i_fmap += input_w * input_h;
    o_fmap += output_w * output_h;
  }
  free(i_fmap_pad);

  return BM_SUCCESS;
}

int native_pooling_min_int8(
    const int8_t* i_fmap,
    int8_t* o_fmap,
    int input_n, int input_c, int input_h, int input_w,
    int kh, int kw,
    int pad_h_t, int pad_h_b, int pad_w_l, int pad_w_r,
    int stride_h, int stride_w,
    int ins_h, int ins_w,
    int ins_h_last, int ins_w_last,
    int input_sign)
{
  if (ins_h != 0 || ins_w != 0 || ins_h_last != 0  || ins_w_last !=0)
    return BM_ERR_INVALID_ARGUMENT;

  int h_after = calc_dilute_hw(input_h, ins_h, ins_h_last, pad_h_t, pad_h_b);
  int w_after = calc_dilute_hw(input_w, ins_w, ins_w_last, pad_w_l, pad_w_r);

  int output_h = calc_output_hw(h_after, kh, stride_h);
  int output_w = calc_output_hw(w_after, kw, stride_w);

  const int min_init = input_sign? 127: 0xFF;
  int8_t *i_fmap_pad = NULL;
  for (int nc = 0; nc < input_n * input_c; nc++) {
    fill_pad_fmap_int8(i_fmap, &i_fmap_pad, min_init,
      pad_w_l, pad_w_r, pad_h_t, pad_h_b,
      0, 0, 0, 0, input_h, input_w);

    for (int ph = 0; ph < output_h; ++ph) {
      for (int pw = 0; pw < output_w; ++pw) {
        int hstart = ph * stride_h;
        int wstart = pw * stride_w;
        int pool_index = index_get(ph, output_w, pw);
        int min = min_init;
        for (int h = 0; h < kh; h++) {
          for (int w = 0; w < kw; w++) {
            int index = index_get((hstart + h), (input_w + pad_w_l + pad_w_r),
                            (w + wstart));
            int val = input_sign ? i_fmap_pad[index]: (uint8_t)i_fmap_pad[index];
            min = (val < min)? val: min;
          }
        }
        o_fmap[pool_index] = min;
      }
    }
    i_fmap += input_w * input_h;
    o_fmap += output_w * output_h;
  }
  free(i_fmap_pad);

  return BM_SUCCESS;
}


int native_pooling_max_fp32(
    const float *ifmap, float *ofmap,
    int input_n, int input_c, int input_h, int input_w,
    int kh, int kw,
    int pad_h_t, int pad_h_b, int pad_w_l, int pad_w_r,
    int stride_h, int stride_w,
    int ins_h, int ins_w,
    int ins_h_last, int ins_w_last
    )
{
  int h_after = calc_dilute_hw(input_h, ins_h, ins_h_last, pad_h_t, pad_h_b);
  int w_after = calc_dilute_hw(input_w, ins_w, ins_w_last, pad_w_l, pad_w_r);
  int output_h = calc_output_hw(h_after, kh, stride_h);
  int output_w = calc_output_hw(w_after, kw, stride_w);
  float *ifmap_after = malloc(sizeof(float)*h_after*w_after);

  if (ifmap_after == NULL) {
    printf("No enough memory[h_after, w_after]: [%u, %u].\n", h_after, w_after);
    return BM_ERR_NOMEM;
  }

  for (int n = 0; n < input_n; n++) {
    for (int c = 0; c < input_c; c++) {
      int ret = fill_pad_fmap_fp32(ifmap, &ifmap_after, -FLT_MAX,
                  pad_h_t, pad_h_b, pad_w_l, pad_w_r,
                  ins_h, ins_w, ins_h_last, ins_w_last,
                  input_h, input_w);

      if (ret != BM_SUCCESS) {
        printf("Failed to pad input fmap.\n");
        free(ifmap_after);
        return BM_ERR_FAILURE;
      }

      for (int h = 0; h < output_h; h++) {
        for (int w = 0; w < output_w; w++) {
          int rf_h = h*stride_h, rf_w = w*stride_w;
          int kh_end = math_min(kh, h_after-rf_h);
          int kw_end = math_min(kw, w_after-rf_w);
          float *rf_addr = ifmap_after + rf_h*w_after + rf_w;
          float max_val = -FLT_MAX;

          for (int i = 0; i < kh_end; i++) {
            for (int j = 0; j < kw_end; j++) {
              max_val = math_max(rf_addr[i*w_after+j], max_val);
            }
          }
          ofmap[h*output_w+w] = max_val;
        }
      }

      ifmap += input_h*input_w;
      ofmap += output_h*output_w;
    }
  }

  free(ifmap_after);
  return BM_SUCCESS;
}

int native_pooling_avg_fp32(
    const float* ifmap,
    float* ofmap,
    int input_n, int input_c, int input_h, int input_w,
    int kh, int kw,
    int pad_h_t, int pad_h_b, int pad_w_l, int pad_w_r,
    int stride_h, int stride_w,
    int ins_h, int ins_w,
    int ins_h_last, int ins_w_last,
    float avg_pooling_const
    )
{
  int h_after = calc_dilute_hw(input_h, ins_h, ins_h_last, pad_h_t, pad_h_b);
  int w_after = calc_dilute_hw(input_w, ins_w, ins_w_last, pad_w_l, pad_w_r);
  int output_h = calc_output_hw(h_after, kh, stride_h);
  int output_w = calc_output_hw(w_after, kw, stride_w);
  float *ifmap_after = malloc(sizeof(float)*h_after*w_after);

  if (ifmap_after == NULL) {
    printf("No enough memory[h_after, w_after]: [%u, %u].\n", h_after, w_after);
    return BM_ERR_NOMEM;
  }

  for (int n = 0; n < input_n; n++) {
    for (int c = 0; c < input_c; c++) {
      int ret = fill_pad_fmap_fp32(ifmap, &ifmap_after, 0,
                  pad_h_t, pad_h_b, pad_w_l, pad_w_r,
                  ins_h, ins_w, ins_h_last, ins_w_last,
                  input_h, input_w);

      if (ret != BM_SUCCESS) {
        printf("Failed to pad input fmap.\n");
        free(ifmap_after);
        return BM_ERR_FAILURE;
      }

      for (int h = 0; h < output_h; h++) {
        for (int w = 0; w < output_w; w++) {
          int rf_h = h*stride_h, rf_w = w*stride_w;
          int kh_end = math_min(kh, h_after-rf_h);
          int kw_end = math_min(kw, w_after-rf_w);
          float *rf_addr = ifmap_after + rf_h*w_after + rf_w;
          float dot_product_even = 0.0, dot_product_odd = 0.0;

          for (int i = 0; i < kh_end; i++) {
            for (int j = 0; j < kw_end; j++) {
              if ((i*kw_end+j)%2) {
                dot_product_odd += rf_addr[i*w_after+j]*avg_pooling_const;
              }
              else {
                dot_product_even += rf_addr[i*w_after+j]*avg_pooling_const;
              }
            }
          }
          ofmap[h*output_w+w] = dot_product_even + dot_product_odd;
        }
      }

      ifmap += input_h*input_w;
      ofmap += output_h*output_w;
    }
  }

  free(ifmap_after);
  return BM_SUCCESS;
}

void native_pooling_forward_max(
    const float* bottom_data, float* top_data,
    int* mask_data,
    const int count,
    const int num, const int channels,
    const int height, const int width,
    const int pooled_height, const int pooled_width,
    const int kernel_h, const int kernel_w,
    const int stride_h, const int stride_w,
    const int pad_h, const int pad_w)
{
  (void)num;
  for (int index = 0; index < count; ++index) {
    const int pw = index % pooled_width;
    const int ph = (index / pooled_width) % pooled_height;
    const int c = (index / pooled_width / pooled_height) % channels;
    const int n = index / pooled_width / pooled_height / channels;
    int hstart = ph * stride_h - pad_h;
    int wstart = pw * stride_w - pad_w;
    const int hend = math_min(hstart + kernel_h, height);
    const int wend = math_min(wstart + kernel_w, width);
    hstart = math_max(hstart, 0);
    wstart = math_max(wstart, 0);
    float maxval = -FLT_MAX;
    int maxidx = -1;
    const float* const bottom_slice =
        bottom_data + (n * channels + c) * height * width;
    for (int h = hstart; h < hend; ++h) {
      for (int w = wstart; w < wend; ++w) {
        if (bottom_slice[h * width + w] > maxval) {
          maxidx = h * width + w;
          maxval = bottom_slice[maxidx];
        }
      }
    }
    top_data[index] = maxval;
    mask_data[index] = maxidx;
  }
}

void native_pooling_forward_ave(
    const float* bottom_data, float* top_data,
    const int count,
    const int num, const int channels,
    const int height, const int width,
    const int pooled_height, const int pooled_width,
    const int kernel_h, const int kernel_w,
    const int stride_h, const int stride_w,
    const int pad_h, const int pad_w)
{
  (void)num;
  for (int index = 0; index < count; ++index) {
    const int pw = index % pooled_width;
    const int ph = (index / pooled_width) % pooled_height;
    const int c = (index / pooled_width / pooled_height) % channels;
    const int n = index / pooled_width / pooled_height / channels;
    int hstart = ph * stride_h - pad_h;
    int wstart = pw * stride_w - pad_w;
    int hend = math_min(hstart + kernel_h, height + pad_h);
    int wend = math_min(wstart + kernel_w, width + pad_w);
    const int pool_size = (hend - hstart) * (wend - wstart);
    hstart = math_max(hstart, 0);
    wstart = math_max(wstart, 0);
    hend = math_min(hend, height);
    wend = math_min(wend, width);
    float aveval = 0;
    const float* const bottom_slice =
        bottom_data + (n * channels + c) * height * width;
    for (int h = hstart; h < hend; ++h) {
      for (int w = wstart; w < wend; ++w) {
        aveval += bottom_slice[h * width + w];
      }
    }
    top_data[index] = aveval / pool_size;
  }
}

int satu_2_8bit(
    const int* pBuff, int len, int8_t* pByteOut, int rshiftbits,
    int round_floor, int sign_unsign)
{
  if (rshiftbits < 0)
    return BM_ERR_INVALID_ARGUMENT;

  int temp;
  int satu_max = sign_unsign ? 127 : 255;
  int satu_min = sign_unsign ? -128 : 0;
  if(rshiftbits == 0)  {
    for (int ii=0; ii<len; ii++)  {
      temp = (pBuff[ii]>satu_max) ? satu_max : ((pBuff[ii]<satu_min) ? satu_min : pBuff[ii]);
      memcpy(pByteOut+ii, &temp, 1);
    }
  } else {  // rshiftbits>0
    for (int ii=0; ii<len; ii++)  {
      if(round_floor == 1)
        temp = ((pBuff[ii]>>(rshiftbits-1))+1)>>1;
      else
        temp = pBuff[ii]>>rshiftbits;
      temp = (temp>satu_max) ? satu_max : ((temp<satu_min) ? satu_min : temp);
      memcpy(pByteOut+ii, &temp, 1);
    }
  }

  return BM_SUCCESS;
}

int satu_2_16bit(
    const int* pBuff, int len, short* pByteOut,
    int rshiftbits, int round_floor, int sign_unsign)
{
  if (rshiftbits < 0)
    return BM_ERR_INVALID_ARGUMENT;

  int ii;
  int temp;
  int satu_max = sign_unsign ? 32767 : 65535;
  int satu_min = sign_unsign ? -32768 : 0;
  if(rshiftbits==0)  {
    for(ii=0; ii<len; ii++)  {
      temp = (pBuff[ii]>satu_max) ? satu_max : ((pBuff[ii]<satu_min) ? satu_min : pBuff[ii]);
      memcpy(pByteOut+ii, &temp, 2);
    }
  } else {  // rshiftbits>0
    for(ii=0; ii<len; ii++)  {
      if(round_floor==1)
        temp = ((pBuff[ii]>>(rshiftbits-1))+1)>>1;
      else
        temp = pBuff[ii]>>rshiftbits;
      temp = (temp>satu_max) ? satu_max : ((temp<satu_min) ? satu_min : temp);
      memcpy(pByteOut+ii, &temp, 2);
    }
  }

  return BM_SUCCESS;
}

void relu(int32_t *buf, uint32_t size)
{
  for (uint64_t i = 0; i < size; i++)
    if (buf[i] < 0)
      buf[i] = 0;
}
