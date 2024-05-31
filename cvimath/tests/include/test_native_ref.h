#ifndef _BM_NATIVE_REF_H_
#define _BM_NATIVE_REF_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef union {
  uint32_t ival;
  float fval;
} IF_VAL;

/*
 * fp32 version
 */

int array_cmp_float(const char *const info, float *p_exp, float *p_got, int count, float delta);
int array_cmp_int(const char *const info, int *p_exp, int *p_got, int count);

/**
 * @name    calc_dilute_hw
 * @brief   calculate diluted dimention
 * @ingroup libbmutils
 *
 * @param [in] h       origin dimention
 * @param [in] ins_h   scaleing factor, 0 -> no scaling
 * @param [in] ins_h_l compensation value after last value in each row
 * @param [in] pad_h_b extra padding left ofr bottom
 * @param [in] pad_h_t extra padding right or top
 *
 * @retval diluted value
 */
int calc_dilute_hw(int h, int ins_h, int ins_h_l, int pad_h_b, int pad_h_t);

/**
 * @name    calc_output_hw
 * @brief   calculate output dimention by kernel and stride size
 * @ingroup libbmutils
 *
 * @param [in] hw       origin dimention
 * @param [in] kwh      scaling factor, 0 -> no scaling
 * @param [in] stride   compensation value after last value in each row
 *
 * @retval output dimention
 */
int calc_output_hw(int hw, int khw, int stride);

/**
 * @name    fill_pad_fmap_fp32
 * @brief   fill padded feature map with unpadded map
 * @ingroup libbmutils
 *
 * @param [in]  before       input array
 * @param [out] pbefore      output array reference, if NULL, alloc a new one
 * @param [in]  pad_val      padding value
 * @param [in]  pad_l        padding left size
 * @param [in]  pad_r        padding right size
 * @param [in]  pad_t        padding top size
 * @param [in]  pad_b        padding bottom size
 * @param [in]  ins_h        scaling factor h
 * @param [in]  ins_w        scaling factor w
 * @param [in]  ins_h_last   compensation value after last value in each row
 * @param [in]  ins_w_last   compensation value after last value in each col
 * @param [in]  h_before     origin height
 * @param [in]  w_before     origin width
 *
 * @retval BM_SUCCESS               success
 * @retval BM_ERR_INVALID_ARGUMENT  before or pafter is null pointer
 * @retval BM_ERR_NOMEM             can't alloc new output array
 */
int fill_pad_fmap_fp32(const float *before, float **after, float pad_value, int pad_t, int pad_b,
                       int pad_l, int pad_r, int ins_h, int ins_w, int ins_h_last, int ins_w_last,
                       int h_before, int w_before);

void native_md_scalar(float *a, float *b, float *r, int N, int C, int H, int W, int op,
                      bool result_add);

void native_conv_ref(const void *ifmap, void *ofmap, const void *weight, int input_n, int input_c,
                     int input_h, int input_w, int output_c, int output_h, int output_w, int groups,
                     int kh, int kw, int dilation_h, int dilation_w, int pad_h, int pad_w,
                     int stride_h, int stride_w, int flip, int using_bias, const void *bias,
                     int result_add);

void native_pooling_forward_max(const float *bottom_data, float *top_data, int *mask_data,
                                const int count, const int num, const int channels,
                                const int height, const int width, const int pooled_height,
                                const int pooled_width, const int kernel_h, const int kernel_w,
                                const int stride_h, const int stride_w, const int pad_h,
                                const int pad_w);

void native_pooling_forward_ave(const float *bottom_data, float *top_data, const int count,
                                const int num, const int channels, const int height,
                                const int width, const int pooled_height, const int pooled_width,
                                const int kernel_h, const int kernel_w, const int stride_h,
                                const int stride_w, const int pad_h, const int pad_w);

/*
 *  int8 vresion
 */

/**
 * @name    array_cmp_int8
 * @brief   compare the contect of p_exp and p_got and print the error index
 *          and value
 * @ingroup libbmutils
 *
 * @param [in] info   informataion string printed when encounter error
 * @param [in]  p_exp  input array
 * @param [in]  p_got  length of input array
 * @param [in]  len    length of input array
 * @retval      0      no error
 * @retval      -1     error occur
 */
int array_cmp_int8(const char *const info, const int8_t *p_exp, const int8_t *p_got, int count);

/**
 * @name    fill_pad_fmap_int8
 * @brief   fill padded feature map with unpadded map
 * @ingroup libbmutils
 *
 * @param [in]  before       input array
 * @param [out] pbefore      output array reference, if NULL, alloc a new one
 * @param [in]  pad_val      padding value
 * @param [in]  pad_l        padding left size
 * @param [in]  pad_r        padding right size
 * @param [in]  pad_t        padding top size
 * @param [in]  pad_b        padding bottom size
 * @param [in]  ins_h        scaling factor h
 * @param [in]  ins_w        scaling factor w
 * @param [in]  ins_h_last   compensation value after last value in each row
 * @param [in]  ins_w_last   compensation value after last value in each col
 * @param [in]  h_before     origin height
 * @param [in]  w_before     origin width
 *
 * @retval BM_SUCCESS               success
 * @retval BM_ERR_INVALID_ARGUMENT  before or pafter is null pointer
 * @retval BM_ERR_NOMEM             can't alloc new output array
 */
int fill_pad_fmap_int8(const int8_t *before, int8_t **pafter, int pad_val, int pad_l, int pad_r,
                       int pad_t, int pad_b, int ins_h, int ins_w, int ins_h_last, int ins_w_last,
                       int h_before, int w_before);

int fill_pad_fmap_bf16(const unsigned short *before, unsigned short **pafter, int pad_val,
                       int pad_l, int pad_r, int pad_t, int pad_b, int ins_h, int ins_w,
                       int ins_h_last, int ins_w_last, int h_before, int w_before);

/**
 * @name    fill_int_with_int8
 * @brief   (int) pdest[i] = (int8_t)pdest[i] for each element
 * @ingroup libbmutils
 *
 * @param [out] pdest  output array
 * @param [in]  psrc   input array
 * @param [in]  len    length of input array
 */
void fill_int_with_int8(int *pdest, int8_t *psrc, int len);

/**
 * @name    fill_int_with_uint8
 * @brief   (int) pdest[i] = (int16_t)pdest[i] for each element
 * @ingroup libbmutils
 *
 * @param [out] pdest  output array
 * @param [in]  psrc   input array
 * @param [in]  len    length of input array
 */
void fill_int_with_uint8(int *pdest, uint8_t *psrc, int len);

/**
 * @name    fill_int_with_int16
 * @brief   (int) pdest[i] = (int16_t)pdest[i] for each element
 * @ingroup libbmutils
 *
 * @param [out] pdest  output array
 * @param [in]  psrc   input array
 * @param [in]  len    length of input array
 */
void fill_int_with_int16(int *pdest, int16_t *psrc, int len);

void native_md_scalar_int8(int8_t *a, int8_t *b, int8_t *r, int N, int C, int H, int W, int op,
                           bool result_add);

/**
 * @name    inner_product
 * @brief   inner product of two array
 * @ingroup libbmutils
 *
 * @param [in]  a    input array 0
 * @param [in]  b    input array 1
 * @param [in]  len  length of a or b
 * @param [out] c    store the summation
 */
void inner_product(const int *a, const int *b, int len, int *c);
void inner_float_product(const float *a, const float *b, int len, float *c);

/**
 * @name    native_conv_int8
 * @brief   do convolution specific 8bit feature map
 * @ingroup libbmutils
 *
 * @param [in]  ifmap         input array
 * @param [in]  weight        weight data array
 * @param [in]  bias          bias array if !NULL, add bias
 * @param [out] ofmap         lenght of input array
 * @param [in]  in            input batch size
 * @param [in]  ic            input channel size
 * @param [in]  ih            input height
 * @param [in]  iw            input width
 * @param [in]  oc            output channle size
 * @param [in]  kh            kernel height
 * @param [in]  kw            kernel width
 * @param [in]  dh            kernel dilute height factor
 * @param [in]  dw            kernel dilute width factor
 * @param [in]  pad_h_t       padding top size
 * @param [in]  pad_h_b       padding bottom size
 * @param [in]  pad_w_l       padding left size
 * @param [in]  pad_w_r       padding right size
 * @param [in]  stride_h      stride height
 * @param [in]  stride_w      stride width
 * @param [in]  ins_h         insert extra element for each i_fmap row
 * @param [in]  ins_w         insert extra element for each i_fmap col
 * @param [in]  ins_h_last    insert extra element for last i_fmap row
 * @param [in]  ins_w_last    insert extra element for last i_fmap col
 * @param [in]  input_sign    i_fmap data type. 0 => signed, 1 => unsigned
 * @param [in]  r_shift_width scale bit for saturation
 *
 * @retval BM_SUCCESS               success
 * @retval other                    saturation failed
 */
int native_conv_int8(const int8_t *ifmap, const int8_t *weight, const int16_t *bias, int8_t *ofmap,
                     int in, int ic, int ih, int iw, int oc, int kh, int kw, int dh, int dw,
                     int pad_h_t, int pad_h_b, int pad_w_l, int pad_w_r, int stride_h, int stride_w,
                     int ins_h, int ins_w, int ins_h_last, int ins_w_last, int input_sign,
                     int r_shift_width, int do_relu);

/**
 * @name    native_fc_int8
 * @brief   do full-connected layer for specific feature map
 * @ingroup libbmutils
 *
 * @param [in]  L              input array
 * @param [in]  R              weight array
 * @param [in]  B              bias array if !NULL, add bias
 * @param [in]  Y              accumulation array if !NULL, add this
 * @param [out] Y_ref          output array
 * @param [in]  L_row_num      input row size
 * @param [in]  L_col_num      input col size
 * @param [in]  R_col_num      weight
 * @param [in]  L_sign         padding top size
 * @param [in]  R_sign         padding top size
 * @param [in]  B_sign         padding top size
 * @param [in]  L_shift_width  padding top size
 * @param [in]  R_shift_width  padding top size
 * @param [in]  is_result_int8 padding top size
 * @param [in]  do_relu        padding top size
 *
 * @retval BM_SUCCESS               success
 * @retval other                    saturation failed
 */
int native_fc_int8(const int8_t *L, const int8_t *R, const int16_t *B, const int16_t *Y, int *Y_ref,
                   int L_row_num, int L_col_num, int R_col_num, int L_sign, int R_sign, int B_sign,
                   int l_shift_width, int r_shift_width, int is_result_int8, int do_relu);

/**
 * @name    native_pooling_ave_int8
 * @brief   do average pooling for specific feature map
 * @ingroup libbmutils
 *
 * @param [in]  i_fmap        input array
 * @param [in]  weight        weight data array
 * @param [in]  bias          bias array if !NULL, add bias
 * @param [out] o_fmap        lenght of input array
 * @param [in]  pad_h_t       padding top size
 * @param [in]  pad_h_b       padding bottom size
 * @param [in]  pad_w_l       padding left size
 * @param [in]  pad_w_r       padding right size
 * @param [in]  stride_h      stride height
 * @param [in]  stride_w      stride width
 * @param [in]  ins_h         insert extra element for each i_fmap row
 * @param [in]  ins_w         insert extra element for each i_fmap col
 * @param [in]  ins_h_last    insert extra element for last i_fmap row
 * @param [in]  ins_w_last    insert extra element for last i_fmap col
 * @param [in]  input_sign    i_fmap data type. 0 => signed, 1 => unsigned
 * @param [in]  satu_sign     saturation data type. 0 => unsigned, 1 => signed
 * @param [in]  r_shift_width scale bit for saturation
 * @param [in]  const_weight  if weight array has one uint8_t value
 *
 * @retval BM_SUCCESS               success
 * @retval BM_ERR_INVALID_ARGUMENT  illegal kh/kw or r_shift_width
 */
int native_pooling_ave_int8(const int8_t *i_fmap, const void *weight, const int16_t *bias,
                            int8_t *o_fmap, int input_n, int input_c, int input_h, int input_w,
                            int kh, int kw, int pad_h_t, int pad_h_b, int pad_w_l, int pad_w_r,
                            int stride_h, int stride_w, int ins_w, int ins_h, int ins_w_last,
                            int ins_h_last, int input_sign, int satu_sign, int r_shift_width,
                            int const_weight);

/**
 * @name    native_pooling_max_int8
 * @brief   do max pooling for specific feature map
 * @ingroup libbmutils
 *
 * @param [in]  i_fmap        input array
 * @param [out] o_fmap        lenght of input array
 * @param [in]  pad_h_t       padding top size
 * @param [in]  pad_h_b       padding bottom size
 * @param [in]  pad_w_l       padding left size
 * @param [in]  pad_w_r       padding right size
 * @param [in]  stride_h      stride height
 * @param [in]  stride_w      stride width
 * @param [in]  ins_h         insert extra element for each i_fmap row
 * @param [in]  ins_w         insert extra element for each i_fmap col
 * @param [in]  ins_h_last    insert extra element for last i_fmap row
 * @param [in]  ins_w_last    insert extra element for last i_fmap col
 * @param [in]  input_sign    i_fmap data type. 0 => unsigned, 1 => signed
 *
 * @retval BM_SUCCESS               success
 * @retval BM_ERR_INVALID_ARGUMENT  illegal ins_h/w or ins_[hw]_last
 */
int native_pooling_max_int8(const int8_t *i_fmap, int8_t *o_fmap, int input_n, int input_c,
                            int input_h, int input_w, int kh, int kw, int pad_h_t, int pad_h_b,
                            int pad_w_l, int pad_w_r, int stride_h, int stride_w, int ins_h,
                            int ins_w, int ins_h_last, int ins_w_last, int input_sign);

int native_pooling_max_fp32(const float *i_fmap, float *o_fmap, int input_n, int input_c,
                            int input_h, int input_w, int kh, int kw, int pad_h_t, int pad_h_b,
                            int pad_w_l, int pad_w_r, int stride_h, int stride_w, int ins_h,
                            int ins_w, int ins_h_last, int ins_w_last);

int native_pooling_avg_fp32(const float *i_fmap, float *o_fmap, int input_n, int input_c,
                            int input_h, int input_w, int kh, int kw, int pad_h_t, int pad_h_b,
                            int pad_w_l, int pad_w_r, int stride_h, int stride_w, int ins_h,
                            int ins_w, int ins_h_last, int ins_w_last, float avg_pooling_const);

int native_depthwise_fp32(const float *ifmap, const float *weight, const float *bias, float *ofmap,
                          int in, int ic, int ih, int iw, int kh, int kw, int dh, int dw,
                          int pad_h_t, int pad_h_b, int pad_w_l, int pad_w_r, int stride_h,
                          int stride_w, int ins_h, int ins_w, int ins_h_last, int ins_w_last);

/**
 * @name    satu_2_8bit
 * @brief   saturate each signed or unsiged 8bit element in array
 * @ingroup libbmutils
 *
 * @param [in]  pBuff       input array
 * @param [in]  len         lenght of input array
 * @param [out] pyByteOut   output array
 * @param [in]  rshiftbits  right shift bit if round_floor && value != 0
 * @param [in]  round_floor enable floor rounding
 * @param [in]  sign_unsign 0 => unsigned, 1 => signed
 *
 * @retval BM_SUCCESS                success
 * @retval BM_ERR_INVALID_ARGUMENT   rshiftbits < 0
 */
int satu_2_8bit(const int *pBuff, int len, int8_t *pByteOut, int rshiftbits, int round_floor,
                int sign_unsign);

/**
 * @name    satu_2_16bit
 * @brief   saturate each signed or unsiged 16bit element in array
 * @ingroup libbmutils
 *
 * @param [in]  pBuff       input array
 * @param [in]  len         lenght of input array
 * @param [out] pyByteOut   output array
 * @param [in]  rshiftbits  right shift bit if round_floor && value != 0
 * @param [in]  round_floor enable floor rounding
 * @param [in]  sign_unsign 0 => unsigned, 1 => signed
 *
 * @retval BM_SUCCESS                success
 * @retval BM_ERR_INVALID_ARGUMENT   rshiftbits < 0
 */
int satu_2_16bit(const int *pBuff, int len, short *pByteOut, int rshiftbits, int round_floor,
                 int sign_unsign);
#ifdef __cplusplus
}
#endif

#endif /* _BM_NATIVE_REF_H_ */
