#pragma once
#include "ive_log.hpp"
#include "tpu_data.hpp"

#include <bmkernel/bm1880v2/1880v2_fp_convert.h>
#include <cviruntime.h>

#include <assert.h>
#include <limits.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <iostream>
#ifndef CV180X
#include <neon_utils.hpp>
#endif
#include <vector>

/*
 * For 4k images around 1.2MB. Take 2M.
 * For 1080p images around 300KB. Take 500KB.
 * For 1080p images around 134KB. Take 150KB.
 * For 480p images around 46KB. Take 50KB.
 */
#define CMDBUF4k 2300000
#define CMDBUF1080 500000
#define CMDBUF720 150000
#define CMDBUF640 50000

#define MULTIPLIER_ONLY_PACKED_DATA_SIZE 5

inline int createHandle(CVI_RT_HANDLE *rt_handle, cvk_context_t **cvk_ctx) {
  if (CVI_RT_Init(rt_handle) != CVI_SUCCESS) {
    LOGE("Runtime init failed.\n");
    return CVI_FAILURE;
  }
  struct sysinfo info;
  if (sysinfo(&info) < 0) {
    return CVI_FAILURE;
  }
  auto available_mem = info.freeram * info.mem_unit;
  uint64_t mem = CMDBUF4k;
  if (available_mem < CMDBUF720) {
    LOGI("Memory insufficient for 720p image, downgrade to 640p.\n");
    mem = CMDBUF640;
  } else if (available_mem < CMDBUF1080) {
    LOGI("Memory insufficient for 1080p image, downgrade to 720p.\n");
    mem = CMDBUF720;
  } else if (available_mem < CMDBUF4k) {
    LOGI("Memory insufficient for 4K image, downgrade to 1080p.\n");
    mem = CMDBUF1080;
  }
  *cvk_ctx = (cvk_context_t *)CVI_RT_RegisterKernel(*rt_handle, mem);
  return CVI_SUCCESS;
}

inline void destroyHandle(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx) {
  CVI_RT_UnRegisterKernel(cvk_ctx);
  CVI_RT_DeInit(rt_handle);
}

inline void genTableU8(const cvk_tl_shape_t &table_shape, const uint8_t *table_data,
                       uint8_t *tg_table) {
  int table_hw = table_shape.h * table_shape.w;

  // duplicate channel #0 to #31
  // TODO: tensor copy
  for (uint64_t i = 0; i < table_shape.c; i++) {
    memcpy(&tg_table[table_hw * i], table_data, sizeof(uint8_t) * table_hw);
  }
}

inline void genTableBF16(const cvk_tl_shape_t &table_shape, const float min_value,
                         const float max_value, uint16_t *table_pos_neg) {
  uint32_t half = table_shape.h * table_shape.w / 2;
  int table_hw = table_shape.h * table_shape.w;

  // data >= 0
  for (uint32_t i = 0; i < half; i++) {
    table_pos_neg[i] = convert_fp32_bf16(max_value);
  }

  // data < 0
  for (uint32_t i = half; i < half * 2; i++) {
    table_pos_neg[i] = convert_fp32_bf16(min_value);
  }

  // duplicate channel #1 to #31
  // TODO: tensor copy
  for (uint64_t i = 1; i < table_shape.c; i++) {
    memcpy(&table_pos_neg[table_hw * i], &table_pos_neg[0], sizeof(uint16_t) * table_hw);
  }
}

inline bool tgTLShapeCompare(cvk_tl_shape_t &tl_shape, cvk_tg_shape_t &tg_shape) {
  if (tg_shape.n == tl_shape.n && tg_shape.c == tl_shape.c && tg_shape.h == tl_shape.h &&
      tg_shape.w == tl_shape.w) {
    return true;
  }
  return false;
}

inline void cviImgFlush2TL(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, CviImg &img,
                           cvk_tl_t *lmem) {
  img.Flush(rt_handle);
  cvk_tdma_g2l_tensor_copy_param_t p;
  p.src = &img.m_tg;
  p.dst = lmem;
  cvk_ctx->ops->tdma_g2l_bf16_tensor_copy(cvk_ctx, &p);
}

inline void cviImg2TL(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, const CviImg &img,
                      cvk_tl_t *lmem) {
  cvk_tdma_g2l_tensor_copy_param_t p;
  p.src = &img.m_tg;
  p.dst = lmem;
  cvk_ctx->ops->tdma_g2l_bf16_tensor_copy(cvk_ctx, &p);
}

inline void constantFillTL(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, const uint16_t value,
                           cvk_tl_t *lmem) {
  if (lmem == nullptr) {
    LOGE("constantFillTL got nullptr input\n");
    return;
  }
  cvk_tdma_g2l_tensor_fill_constant_param_t p_fill;
  p_fill.constant = value;
  p_fill.dst = lmem;

  cvk_ctx->ops->tdma_g2l_bf16_tensor_fill_constant(cvk_ctx, &p_fill);
}

// FIXME: O0 may crash. Reason unknown.
inline void bf16LookupTable(cvk_context_t *cvk_ctx, const cvm_tiu_mask_param_t *mask) {
  cvk_tdma_l2l_tensor_copy_param_t p10;
  cvk_tl_t lmem = *mask->ofmap;
  lmem.fmt = CVK_FMT_I8;
  lmem.shape.h *= lmem.shape.w;
  lmem.shape.w = 1;
  lmem.stride = cvk_ctx->ops->tl_default_stride(cvk_ctx, lmem.shape, CVK_FMT_I8, 1);
  lmem.stride.h *= 2;
  p10.dst = &lmem;
  p10.src = mask->ifmap;
  p10.mv_lut_idx = true;
  cvk_ctx->ops->tdma_l2l_bf16_tensor_copy(cvk_ctx, &p10);
  p10.mv_lut_idx = false;

  cvk_tiu_lookup_table_param_t p12;
  p12.ofmap = mask->ofmap;
  p12.ifmap = mask->ifmap;
  p12.table = mask->pos_neg_table;
  cvk_ctx->ops->tiu_lookup_table(cvk_ctx, &p12);
}

inline void QuantizeMultiplierSmallerThanOne(float real_multiplier, uint32_t *quantized_multiplier,
                                             int *right_shift) {
  if (real_multiplier <= 0.f || real_multiplier > 1.f) {
    LOGE("Multiplier should be bigger than 0, smaller or euqal to 1.\n");
    *quantized_multiplier = 0;
    *right_shift = 0;
    return;
  } else if (real_multiplier == 1.f) {
    *quantized_multiplier = (uint32_t)(1ll << 31) - 1;
    *right_shift = 0;
  } else {
    int s = 0;
    // We want to bring the real multiplier into the interval [1/2, 1).
    // We can do so by multiplying it by two, and recording how many times
    // we multiplied by two so that we can compensate that by a right
    // shift by the same amount.
    while (real_multiplier < 0.5f) {
      real_multiplier *= 2.0f;
      s++;
    }
    // Now that the real multiplier is in [1/2, 1), we convert it
    // into a fixed-point number.
    int64_t q = static_cast<int64_t>(round(real_multiplier * (1ll << 31)));
    assert(q <= (1ll << 31));
    // Handle the special case when the real multiplier was so close to 1
    // that its fixed-point approximation was undistinguishable from 1.
    // We handle this by dividing it by two, and remembering to decrement
    // the right shift amount.
    if (q == (1ll << 31)) {
      q /= 2;
      s--;
    }
    assert(s >= 0);
    assert(q <= (int64_t)LONG_MAX);
    *quantized_multiplier = (uint32_t)q;
    *right_shift = s;
  }
}

inline void pack_per_chan_cal_data(uint32_t channels, bool has_bias, int32_t *bias,
                                   uint32_t *multiplier, int8_t *shift, uint8_t *packed_data) {
  uint8_t *ptr = packed_data;

  for (uint32_t i = 0; i < channels; i++) {
    if (has_bias) {
      uint32_t val = (uint32_t)bias[i];
      *ptr = val & 0xff;
      ptr++;
      *ptr = (val >> 8) & 0xff;
      ptr++;
      *ptr = (val >> 16) & 0xff;
      ptr++;
      *ptr = (val >> 24) & 0xff;
      ptr++;
    }

    {
      uint32_t val = multiplier[i];
      *ptr = val & 0xff;
      ptr++;
      *ptr = (val >> 8) & 0xff;
      ptr++;
      *ptr = (val >> 16) & 0xff;
      ptr++;
      *ptr = (val >> 24) & 0xff;
      ptr++;
    }

    {
      uint8_t val = shift[i];
      *ptr = val;
      ptr++;
    }
  }
}

inline void getPackedMultiplierArrayBuffer(const uint32_t c, const uint32_t &quantized_multiplier,
                                           const int &right_shift, uint8_t *cal_data) {
  // Create tl_multiplier
  uint32_t *multiplier_data = new uint32_t[c];
  int8_t *shift_data = new int8_t[c];
  for (unsigned int i = 0; i < c; ++i) {
    // multipliers typically range in [2^30 ; 2^31 - 1].
    // Values in [0, 2^30 - 1] are normally unused, but harmless.
    // Thus a good way to randomize multipliers is to subtract from them
    // a random value smaller than 2^30 but still significant compared to it.
    multiplier_data[i] = quantized_multiplier;

    // Our H/W only supports right shift
    shift_data[i] = right_shift > 0 ? right_shift : 0;

#ifdef ENABLE_DEBUG_MSG
    LOGE("      [oc=%d] multiplier_data %d, shift_data %d\n", i, p_param->multiplier_data[i],
         p_param->shift_data[i]);
#endif
  }

  pack_per_chan_cal_data(c, 0, NULL, multiplier_data, shift_data, cal_data);
  delete[] multiplier_data;
  delete[] shift_data;
}

inline uint8_t *getPackedMultiplierArray(const uint32_t c, const uint32_t &quantized_multiplier,
                                         const int &right_shift) {
  const int per_chan_cal_data_size = MULTIPLIER_ONLY_PACKED_DATA_SIZE;  // p_param->has_bias ? 9 :
                                                                        // 5;  // bias(4) +
                                                                        // multiplier(4) + shift(1)
  const int cal_data_size = c * per_chan_cal_data_size;
  uint8_t *cal_data = (uint8_t *)malloc(cal_data_size);
  getPackedMultiplierArrayBuffer(c, quantized_multiplier, right_shift, cal_data);
  return cal_data;
}

static inline CVI_RT_MEM get_tensor_l2g(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                                        const cvk_tl_t *tl) {
  cvk_tg_shape_t s;
  s.n = tl->shape.n;
  s.c = tl->shape.h;
  s.h = tl->shape.w;
  s.w = tl->shape.c;
  size_t total_size = s.n * s.c * s.h * s.w * getFmtSize(tl->fmt);
  cvk_tg_t tg;
  CVI_RT_MEM rt_dev = CVI_RT_MemAlloc(rt_handle, total_size);
  tg.base_reg_index = 0;
  tg.start_address = CVI_RT_MemGetPAddr(rt_dev);
  tg.fmt = tl->fmt;
  tg.shape = s;
  tg.stride = cvk_ctx->ops->tg_default_stride(cvk_ctx, s, tl->fmt);
  cvk_tdma_l2g_tensor_copy_param_t p;
  p.src = tl;
  p.dst = &tg;
  cvk_ctx->ops->tdma_l2g_bf16_tensor_copy(cvk_ctx, &p);
  return rt_dev;
}

static inline uint8_t *get_rt_vaddr(CVI_RT_HANDLE rt_handle, CVI_RT_MEM rt_dev) {
  if (CVI_RT_MemInvld(rt_handle, rt_dev) != CVI_RC_SUCCESS) {
    return nullptr;
  }
  return CVI_RT_MemGetVAddr(rt_dev);
}
