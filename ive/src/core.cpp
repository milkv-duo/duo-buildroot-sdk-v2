#include "core.hpp"
#include "ive_log.hpp"

#include <cmath>
#include <iostream>
#include <memory>

#ifdef WORKAROUND_SCALAR_4096_ALIGN_BUG
#define SCALAR_C_ALIGN 0x1000
#else
#define SCALAR_C_ALIGN 0x1
#endif

inline void GetSliceUnitProperty(const uint32_t length, const uint32_t slice, const int kernel_sz,
                                 const int default_stride, sliceUnit *unit) {
  unit->slice = slice > length ? length : slice;
  unit->slice = default_stride * (int)(unit->slice / default_stride);
  unit->skip = unit->slice - kernel_sz + default_stride;
  unit->skip = (uint32_t)default_stride > unit->skip ? default_stride : unit->skip;

  uint32_t kernel_pad = kernel_sz - 1;
  uint32_t left_pad = kernel_pad / 2;
  unit->turn = ((int64_t)length - unit->slice - left_pad) / unit->skip + 1;
  int64_t result = (int64_t)length - (int64_t)((unit->turn) * (unit->slice - kernel_pad));
  if (result >= kernel_sz) {
    // x + (x - 1) * default_stride
    int res_left = result - kernel_sz;
    int res_div = res_left / default_stride;
    int result_2 = default_stride * res_div + kernel_sz;
    unit->left = result_2;
    unit->turn++;
  } else if (result < 0 && unit->slice > std::abs(result)) {
    unit->left = unit->slice + result;
  } else {
    unit->left = 0;
  }
}

inline void categoryIOTLShape(const std::vector<cvk_tl_t *> &tl_vec,
                              std::vector<cvk_tg_shape_t> &s_in_vec,
                              std::vector<cvk_tg_shape_t> &s_out_vec,
                              std::vector<std::pair<int, cvk_tl_t *>> *tl_in_shape_lmem_vec,
                              std::vector<std::pair<int, cvk_tl_t *>> *tl_out_shape_lmem_vec) {
  tl_in_shape_lmem_vec->clear();
  tl_out_shape_lmem_vec->clear();
  for (size_t i = 0; i < tl_vec.size(); i++) {
    auto *lmem = tl_vec[i];
    bool skip = false;
    for (size_t k = 0; k < s_in_vec.size(); k++) {
      if (tgTLShapeCompare(lmem->shape, s_in_vec[k])) {
        tl_in_shape_lmem_vec->push_back({k, lmem});
        skip = true;
        break;
      }
    }
    if (skip) {
      continue;
    }
    for (size_t k = 0; k < s_out_vec.size(); k++) {
      if (tgTLShapeCompare(lmem->shape, s_out_vec[k])) {
        tl_out_shape_lmem_vec->push_back({k, lmem});
        break;
      }
    }
  }
}

inline void getTLInfo(const std::vector<cvk_tl_t *> &tl_vec, const std::vector<uint32_t> &tl_in_idx,
                      const std::vector<uint32_t> &tl_out_idx, TLInfo *tl_in_info,
                      TLInfo *tl_out_info) {
  for (size_t i = 0; i < tl_in_idx.size(); i++) {
    auto *lmem = tl_vec[tl_in_idx[i]];
    tl_in_info->lmem_vec.emplace_back(lmem);
    tl_in_info->fns_vec.push_back(FmtnSize(lmem->fmt));
  }
  for (size_t i = 0; i < tl_out_idx.size(); i++) {
    auto *lmem = tl_vec[tl_out_idx[i]];
    tl_out_info->lmem_vec.emplace_back(lmem);
    tl_out_info->fns_vec.push_back(FmtnSize(lmem->fmt));
  }
}

inline void getBMAddrInfo(const std::vector<CviImg *> &input, const std::vector<CviImg *> &output,
                          const int pad_left, const int pad_top, BMAddrInfo *bm_src_info,
                          BMAddrInfo *bm_dest_info) {
  for (size_t k = 0; k < input.size(); k++) {
    uint64_t bm_start_addr = input[k]->GetPAddr();
    bm_src_info->addr_vec.push_back(bm_start_addr);
    bm_src_info->fns_vec.push_back(FmtnSize(input[k]->m_tg.fmt));
  }
  for (size_t k = 0; k < output.size(); k++) {
    uint64_t bm_des_addr = output[k]->GetPAddr();
    FmtnSize fns(output[k]->m_tg.fmt);
    uint64_t new_bm_des_addr =
        bm_des_addr + (output[k]->m_tg.stride.h * pad_top) + (pad_left * fns.getSize());
    bm_dest_info->addr_vec.push_back(new_bm_des_addr);
    bm_dest_info->fns_vec.push_back(fns);
  }
}

inline int checkIsBufferOverflow(const std::vector<CviImg *> &input,
                                 const std::vector<CviImg *> &output, const BMAddrInfo &bm_src_info,
                                 const BMAddrInfo &bm_dest_info, const int &pad_l, const int &pad_t,
                                 const bool is_1d, const bool shift_pad_offset) {
#if DISABLE_OVERFLOWCHECK
  return CVI_SUCCESS;
#else
  int ret = CVI_SUCCESS;
  for (size_t k = 0; k < input.size(); k++) {
    const u64 bm_start_addr = input[k]->GetPAddr();
    u64 jumped_value = bm_src_info.addr_vec[k] - bm_start_addr;
    u32 total_addr = is_1d ? input[k]->m_tg.stride.n : input[k]->m_tg.stride.c;
    if (jumped_value != total_addr) {
      LOGE(
          "Error! Input %u jumped value %lu not align to image size %u, start addr "
          "%lu\n",
          (u32)k, (long unsigned int)jumped_value, total_addr, (long unsigned int)bm_start_addr);
      ret = CVI_FAILURE;
    }
  }
  for (size_t k = 0; k < output.size(); k++) {
    const u64 bm_des_addr = output[k]->GetPAddr();
    u64 jumped_value = bm_dest_info.addr_vec[k] - bm_des_addr;
    u32 pad_offset =
        shift_pad_offset
            ? ((output[k]->m_tg.stride.h * pad_t) + (pad_l * bm_dest_info.fns_vec[k].getSize()))
            : 0;
    u32 total_addr =
        is_1d ? (output[k]->m_tg.stride.n + pad_offset) : (output[k]->m_tg.stride.c + pad_offset);
    if (jumped_value != total_addr) {
      LOGE(
          "Error! Output %u jumped value %lu not align to image size %u, start addr "
          "%lu\n",
          (u32)k, (long unsigned int)jumped_value, total_addr, (long unsigned int)bm_des_addr);
      ret = CVI_FAILURE;
    }
  }
  return ret;
#endif
}

inline void updateTSIInfo(cvk_context_t *cvk_ctx, const uint32_t load_n, const uint32_t load_c,
                          const uint32_t load_h, const uint32_t load_w, const uint32_t store_n,
                          const uint32_t store_c, const uint32_t store_h, const uint32_t store_w,
                          const cvk_fmt_t io_fmt, const cvk_fmt_t tgin_fmt_type,
                          const cvk_fmt_t tgout_fmt_type, TensorSliceInfo *tl_info) {
  tl_info->tl_load.shape = {load_n, load_c, load_h, load_w};
  tl_info->tl_load.stride =
      cvk_ctx->ops->tl_default_stride(cvk_ctx, tl_info->tl_load.shape, io_fmt, 1);
  tl_info->tg_load.shape = {load_n, load_c, load_h, load_w};
  tl_info->tg_load.stride =
      cvk_ctx->ops->tg_default_stride(cvk_ctx, tl_info->tg_load.shape, tgin_fmt_type);
  tl_info->tl_store.shape = {store_n, store_c, store_h, store_w};
  tl_info->tl_store.stride =
      cvk_ctx->ops->tl_default_stride(cvk_ctx, tl_info->tl_store.shape, io_fmt, 1);
  tl_info->tg_store.shape = {store_n, store_c, store_h, store_w};
  tl_info->tg_store.stride =
      cvk_ctx->ops->tg_default_stride(cvk_ctx, tl_info->tg_store.shape, tgout_fmt_type);
}

// For channel Ext mode
inline int channelExtension(cvk_context_t *cvk_ctx, const uint32_t in_img_w,
                            const uint32_t out_img_w, const int ic, const int ih, const int iw,
                            const int h_cext_multiplier, const int pad_left, const int pad_right,
                            const int pad_top, const int pad_bottom, const int kh, const int kw,
                            const int k_stride_h, const int k_stride_w,
                            const cvk_fmt_t tgin_fmt_type, const cvk_fmt_t tgout_fmt_type,
                            const cvk_fmt_t tl_fmt_type, TensorSliceInfo *tsi) {
  // FIXME: Temporarily hack for cvm_reshape_channel_same not support "h_slice % c_multiplier
  // != 0" cases.
  if (h_cext_multiplier == 1) {
    const uint32_t in_ih = ih + pad_top + pad_bottom;
    updateTSIInfo(cvk_ctx, 1, ic, in_ih, iw, 1, ic, ih, iw, tl_fmt_type, tgin_fmt_type,
                  tgout_fmt_type, tsi);
    return CVI_SUCCESS;
  }
  cvk_tl_shape_t tl_weight_shape;
  cvk_tl_shape_t tl_bias_shape;

  if (cvm_reshape_channel_same(cvk_ctx, ic, ih, iw, kh, kw, pad_right, pad_left, k_stride_h,
                               k_stride_w, &tsi->tl_load.shape, &tsi->tl_load.stride,
                               &tsi->tg_load.shape, &tsi->tg_load.stride, &tl_weight_shape,
                               &tl_bias_shape, &tsi->tl_store.shape, tl_fmt_type, 1) == -1) {
    LOGE("Extend failed.\n");
    return CVI_FAILURE;
  }
  // FIXME: Temporarily solution for mix precision hack.
  if (tgin_fmt_type != tl_fmt_type) {
    float res = (float)getFmtSize(tgin_fmt_type) / getFmtSize(tl_fmt_type);
    tsi->tg_load.stride.n *= res;
    tsi->tg_load.stride.c *= res;
    tsi->tg_load.stride.h *= res;
  }
  tsi->tg_store.shape.n = tsi->tl_store.shape.n;
  tsi->tg_store.shape.c = tsi->tl_store.shape.c;
  tsi->tg_store.shape.h = tsi->tl_store.shape.h;
  tsi->tg_store.shape.w = tsi->tl_store.shape.w;
  tsi->tg_store.stride =
      cvk_ctx->ops->tg_default_stride(cvk_ctx, tsi->tg_store.shape, tgout_fmt_type);
  tsi->tl_store.stride =
      cvk_ctx->ops->tl_default_stride(cvk_ctx, tsi->tl_store.shape, tl_fmt_type, 1);
  // FIXME: Temporarily hack.
  if (tsi->tg_load.shape.w != in_img_w) {
    int l_n = tsi->tg_load.stride.n / tsi->tg_load.stride.c;
    int l_c = tsi->tg_load.stride.c / tsi->tg_load.stride.h;
    tsi->tg_load.stride.h = in_img_w * getFmtSize(tgin_fmt_type);
    tsi->tg_load.stride.c = l_c * tsi->tg_load.stride.h;
    tsi->tg_load.stride.n = l_n * tsi->tg_load.stride.c;
  }
  if (tsi->tg_store.shape.w != out_img_w) {
    int l_n = tsi->tg_store.stride.n / tsi->tg_store.stride.c;
    int l_c = tsi->tg_store.stride.c / tsi->tg_store.stride.h;
    tsi->tg_store.stride.h = out_img_w * getFmtSize(tgout_fmt_type);
    tsi->tg_store.stride.c = l_c * tsi->tg_store.stride.h;
    tsi->tg_store.stride.n = l_n * tsi->tg_store.stride.c;
  }
  // clang-format off
  LOGD("[Summary]\n" \
       " Original shape: n c h w %d %d %d %d\n" \
       " Kernel size: h w %d %d\n" \
       " Pad: left right %d %d\n" \
       " Kernel stride: h w %d %d\n" \
       " => tl load shape %d %d %d %d\n" \
       " => tl load stride %d %d %d %d\n" \
       " => tg load shape %d %d %d %d\n" \
       " => tg load stride %d %d %d\n" \
       " => tl store shape %d %d %d %d\n" \
       " => tl store stride %d %d %d %d\n" \
       " => tg store shape %d %d %d %d\n" \
       " => tg store stride %d %d %d\n",
       1, ic, ih, iw,
       kh, kw,
       pad_left, pad_right,
       k_stride_h, k_stride_w,
       tsi->tl_load.shape.n, tsi->tl_load.shape.c, tsi->tl_load.shape.h, tsi->tl_load.shape.w,
       tsi->tl_load.stride.n, tsi->tl_load.stride.c, tsi->tl_load.stride.h, tsi->tl_load.stride.w,
       tsi->tg_load.shape.n, tsi->tg_load.shape.c, tsi->tg_load.shape.h, tsi->tg_load.shape.w,
       tsi->tg_load.stride.n, tsi->tg_load.stride.c, tsi->tg_load.stride.h,
       tsi->tl_store.shape.n, tsi->tl_store.shape.c, tsi->tl_store.shape.h, tsi->tl_store.shape.w,
       tsi->tl_store.stride.n, tsi->tl_store.stride.c, tsi->tl_store.stride.h, tsi->tl_store.stride.w,
       tsi->tg_store.shape.n, tsi->tg_store.shape.c, tsi->tg_store.shape.h, tsi->tg_store.shape.w,
       tsi->tg_store.stride.n, tsi->tg_store.stride.c, tsi->tg_store.stride.h);
  // clang-format on
  uint32_t h_output_single_lane = (ih * ic / h_cext_multiplier);
  if (tsi->tg_store.shape.h != h_output_single_lane) {
    LOGE("H extend c multiplier: %u. Predicted h_slice not match: %u, %u.\n", h_cext_multiplier,
         tsi->tg_store.shape.h, h_output_single_lane);
    if ((uint32_t)(ic * ih) == tsi->tg_store.shape.c * tsi->tg_store.shape.h) {
      LOGW("[DEV] This is a dev warning only.\n");
    } else {
      LOGE("Slice failed.\n");
      return CVI_FAILURE;
    }
  }
  return CVI_SUCCESS;
}

inline bool calculateOutExtHSlice(const int &npu_num, const int &img_c, const uint32_t &max_slice,
                                  const uint32_t &out_slice, uint32_t *c_multiplier,
                                  uint32_t *left_pixels, bool channel_priority = true) {
  uint32_t c_mul = npu_num / img_c;
  if (channel_priority) {
    uint32_t diff = out_slice;
    uint32_t max_c_mul = c_mul;
    while (c_mul > 0) {
      uint32_t &&out_slice_tmp = out_slice + c_mul / 2;
      int64_t result = out_slice_tmp - (out_slice_tmp % c_mul);
      uint32_t &&result_limit = max_slice * c_mul;
      // Boundary check. Must 0 < result max_slice * c_mul;
      if (result > result_limit) result = result_limit;
      if (result < 0) result = 0;
      uint32_t tmp_diff = out_slice - result;
      if (tmp_diff < diff) {
        max_c_mul = c_mul;
        diff = tmp_diff;
        if (diff == 0) {
          break;
        }
      }
      c_mul--;
    }
    *c_multiplier = max_c_mul * img_c;
    *left_pixels = diff;
  } else {
    while (out_slice % c_mul != 0) {
      c_mul--;
    }
    uint32_t tmp_h = out_slice / c_mul;
    if (tmp_h > max_slice) {
      c_mul = npu_num / img_c;
      uint32_t unit_skip = max_slice;
      tmp_h = c_mul * unit_skip;
      while (tmp_h > out_slice) {
        c_mul--;
        tmp_h = c_mul * unit_skip;
      }
      uint32_t new_h_left = max_slice * c_mul;
      *c_multiplier = c_mul * img_c;
      *left_pixels = out_slice - new_h_left;
    } else {
      auto res = c_mul * max_slice;
      *c_multiplier = c_mul * img_c;
      *left_pixels = (res < out_slice) ? (out_slice - res) : 0;
    }
  }
  return (*left_pixels > 0) ? true : false;
}

inline void updateLMemSize(cvk_context_t *cvk_ctx, const int &npu_num, const cvk_fmt_t &io_fmt,
                           const TensorSliceInfo &tsi, const std::vector<IVETLType> &tl_type,
                           std::vector<std::pair<int, cvk_tl_t *>> *tl_in_shape_lmem_vec,
                           std::vector<std::pair<int, cvk_tl_t *>> *tl_out_shape_lmem_vec,
                           std::vector<cvk_tl_t *> *tl_vec) {
  for (size_t k = 0; k < tl_in_shape_lmem_vec->size(); k++) {
    // int &index = (*tl_in_shape_lmem_vec)[k].first;
    auto *lmem = (*tl_in_shape_lmem_vec)[k].second;
    lmem->shape = tsi.tl_load.shape;
    if (io_fmt == lmem->fmt) {
      lmem->stride = tsi.tl_load.stride;
    } else {
      lmem->stride = cvk_ctx->ops->tl_default_stride(cvk_ctx, lmem->shape, lmem->fmt, 1);
    }
  }
  for (size_t k = 0; k < tl_out_shape_lmem_vec->size(); k++) {
    // int &index = (*tl_out_shape_lmem_vec)[k].first;
    auto *lmem = (*tl_out_shape_lmem_vec)[k].second;
    lmem->shape = tsi.tl_store.shape;
    if (io_fmt == lmem->fmt) {
      lmem->stride = tsi.tl_store.stride;
    } else {
      lmem->stride = cvk_ctx->ops->tl_default_stride(cvk_ctx, lmem->shape, lmem->fmt, 1);
    }
  }
  for (size_t k = 0; k < tl_vec->size(); k++) {
    auto *lmem = (*tl_vec)[k];
    uint64_t &&align_up_res = align_up(lmem->shape.c, npu_num) / npu_num;
    int is_align = (lmem->stride.n != align_up_res) ? 1 : 0;
    if (lmem->shape.c != tsi.tl_load.shape.c && tl_type[k] != IVETLType::TABLE) {
      lmem->shape.c = tsi.tl_load.shape.c;
      lmem->stride = cvk_ctx->ops->tl_default_stride(cvk_ctx, lmem->shape, lmem->fmt, is_align);
    }
  }
}
// Ext end

IveCore::IveCore() {}
IveCore::~IveCore() {}
int IveCore::run(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                 const std::vector<CviImg *> &input, std::vector<CviImg *> &output,
                 bool legacy_mode) {
  m_chip_info = cvk_ctx->info;
  m_input_fmts.clear();
  m_output_fmts.clear();
  for (const auto &img : input) {
    if (!img->IsStideCEQ()) {
      LOGE("Input image ( %u, %u) appears does not have equal strides in different channels.\n",
           img->GetImgWidth(), img->GetImgHeight());
      return CVI_FAILURE;
    }
    m_input_fmts.push_back(img->m_tg.fmt);
  }
  for (const auto &img : output) {
    if (!img->IsStideCEQ()) {
      LOGE("Output image ( %u, %u) appears does not have equal strides in different channels.\n",
           img->GetImgWidth(), img->GetImgHeight());
      return CVI_FAILURE;
    }
    m_output_fmts.push_back(img->m_tg.fmt);
  }
  int ret = CVI_SUCCESS;
  if (legacy_mode) {
    if (m_force_addr_align_ && input.size() > 1) {
      ret = runSingleSizeKernelMultiBatch(rt_handle, cvk_ctx, input, output);
    } else {
      ret = runSingleSizeKernel(rt_handle, cvk_ctx, input, output);
    }
  } else {
    bool use_ext = false;
    if (input.size() > 1 && output.size() > 1) {
      uint32_t total_size = input[0]->m_tg.stride.n / getFmtSize(input[0]->m_tg.fmt);
      uint32_t total_size2 = output[0]->m_tg.stride.n / getFmtSize(output[0]->m_tg.fmt);
      if (total_size != total_size2) {
        use_ext |= true;
      }
    } else if (input.size() > 1) {
      uint32_t total_size = input[0]->m_tg.stride.n / getFmtSize(input[0]->m_tg.fmt);
      use_ext |= (total_size % 16 != 0) ? true : false;
    } else {
      uint32_t total_size = output[0]->m_tg.stride.n / getFmtSize(output[0]->m_tg.fmt);
      use_ext |= (total_size % 16 != 0) ? true : false;
    }
    for (const auto &img : input) {
      use_ext |= img->IsSubImg();
    }
    for (const auto &img : output) {
      use_ext |= img->IsSubImg();
    }
    if (m_kernel_info.size != 1) {
      use_ext |= true;
    }
    if (use_ext || m_force_use_ext) {
      // printf("run singlesize\n");
      ret = runSingleSizeExtKernel(rt_handle, cvk_ctx, input, output);
    } else {
      // printf("run nokernel\n");
      ret = runNoKernel(rt_handle, cvk_ctx, input, output);
    }
  }
  return ret;
}

int IveCore::getSlice(const uint32_t nums_of_lmem, const uint32_t nums_of_table,
                      const uint32_t fixed_lmem_size, const uint32_t n, const uint32_t c,
                      const uint32_t h, const uint32_t w, const uint32_t table_size,
                      const kernelInfo kernel_info, const int npu_num, sliceUnit *unit_h,
                      sliceUnit *unit_w, const bool enable_cext) {
  if (c > (uint32_t)npu_num) {
    LOGE("Channel exceed limitation.\n");
    return CVI_FAILURE;
  }
  // Calculate fixed kernel size
  uint32_t kernel_sz = (kernel_info.nums_of_kernel * kernel_info.size * kernel_info.size +
                        MULTIPLIER_ONLY_PACKED_DATA_SIZE * kernel_info.use_multiplier);
  // Find max available mem for one tl.
  int64_t result = m_chip_info.lmem_size - (int64_t)(kernel_sz + table_size * nums_of_table) -
                   (int64_t)fixed_lmem_size;
  if (result < 0) {
    LOGE("Insufficient memory: %u\n", (unsigned int)result);
    return CVI_FAILURE;
  }
  const uint32_t available_lmem_per_tl = (uint32_t)result / nums_of_lmem;
  uint32_t w_length = w;
  uint32_t h_tmp_slice = 0;
  int w_num = 1;
  // Here the default value for kernel size is 1. The h_slice should never smaller than kernel size.
  h_tmp_slice = available_lmem_per_tl / w_length;
  w_num++;
  if (kernel_info.size == 1 || (kernel_info.size == kernel_info.default_stride_x &&
                                kernel_info.size == kernel_info.default_stride_y)) {
    while (h_tmp_slice < kernel_info.size) {
      w_length = w / w_num;
      uint32_t h_tmp_res = available_lmem_per_tl / w_length;
      h_tmp_slice = h_tmp_res > h ? h : h_tmp_res;
      w_num++;
    }
  } else {
    while (h_tmp_slice < kernel_info.size) {
      w_length = w / w_num;
      if (w_length + 16 < w) {
        int res = w_length % 16;
        w_length += (res == 0 ? 16 : res);
      }
      uint32_t h_tmp_res = available_lmem_per_tl / w_length;
      h_tmp_slice = h_tmp_res > h ? h : h_tmp_res;
      w_num++;
    }
  }
  // If enable channel extending feature
  if (enable_cext) {
    if (kernel_info.default_stride_y == 1) {
      // Experimental
      uint32_t c_multiplier = 0, left_pixels = 0;
      uint32_t kernel_pad = kernel_info.size - 1;
      uint32_t out_max_slice = h_tmp_slice - kernel_pad;
      uint32_t out_h = (h - kernel_pad) / kernel_info.default_stride_y;
      calculateOutExtHSlice(npu_num, c, out_max_slice, out_h, &c_multiplier, &left_pixels);
      h_tmp_slice = (out_h * kernel_info.default_stride_y) - left_pixels + kernel_pad;
      unit_h->c_multiplier = c_multiplier;
    } else {
      // FIXME: Need better way to found best slice for channel ext.
      uint32_t new_h_tmp_slice = h_tmp_slice;
      if (new_h_tmp_slice >= h) {
        new_h_tmp_slice = kernel_info.size;
      }
      int c_multiplier = (int)(npu_num / c);
      uint32_t unit_skip = new_h_tmp_slice - kernel_info.pad[2] - kernel_info.pad[3] +
                           (kernel_info.default_stride_y - 1);
      new_h_tmp_slice += unit_skip * (c_multiplier - 1);
      while (new_h_tmp_slice > h) {
        new_h_tmp_slice -= unit_skip;
        c_multiplier--;
      }
      uint32_t res = (new_h_tmp_slice - kernel_info.pad[2] - kernel_info.pad[3]);
      if (res % c_multiplier != 0) {
        unit_h->c_multiplier = 1;
      } else {
        // Channel priority.
        c_multiplier = (int)(npu_num / c);
        while (res % c_multiplier != 0) {
          c_multiplier--;
        }
        h_tmp_slice = new_h_tmp_slice;
        unit_h->c_multiplier = c_multiplier;
      }
    }
  }

  // FIXME: Logic error
  GetSliceUnitProperty(h, h_tmp_slice, kernel_info.size, kernel_info.default_stride_y, unit_h);
  GetSliceUnitProperty(w, w_length, kernel_info.size, kernel_info.default_stride_x, unit_w);
  LOGD("H slice %d skip %d turn %d left %d\n", unit_h->slice, unit_h->skip, unit_h->turn,
       unit_h->left);
  LOGD("W slice %d skip %d turn %d left %d\n", unit_w->slice, unit_w->skip, unit_w->turn,
       unit_w->left);
  return CVI_SUCCESS;
}

cvk_tl_t *IveCore::allocTLMem(cvk_context_t *cvk_ctx, cvk_tl_shape_t tl_shape, cvk_fmt_t fmt,
                              int eu_align, IVETLType tl_type) {
  cvk_tl_t *lmem = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape, fmt, eu_align);
  if (lmem == NULL) {
    LOGE("Tensor allocate failed. ( n, c, h, w) = ( %u, %u, %u, %u).\n", tl_shape.n, tl_shape.c,
         tl_shape.h, tl_shape.w);
    m_allocate_failed_ = true;
    return nullptr;
  }

  m_tl_type.push_back(tl_type);
  // A safer way to pass pointer to vector of pointers.
  auto ptr = std::unique_ptr<cvk_tl_t>(lmem);
  m_tl_vec.emplace_back(ptr.get());
  ptr.release();
  return lmem;
}

int IveCore::freeTLMems(cvk_context_t *cvk_ctx) {
  for (int i = m_tl_vec.size() - 1; i >= 0; i--) {
    cvk_ctx->ops->lmem_free_tensor(cvk_ctx, m_tl_vec[i]);
  }
  m_tl_type.clear();
  m_tl_vec.clear();
  return CVI_SUCCESS;
}

int IveCore::sliceSetup(SliceRes &slice_res, SliceRes *tg_in_res, SliceRes *tg_out_res) {
  *tg_in_res = slice_res;
  *tg_out_res = slice_res;
  return CVI_SUCCESS;
}

void IveCore::beforeSubmit(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                           const std::vector<CviImg *> &input, std::vector<CviImg *> &output) {}

int IveCore::postProcess(CVI_RT_HANDLE rt_handle) { return CVI_SUCCESS; }

int IveCore::runSingleSizeKernel(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                                 const std::vector<CviImg *> &input, std::vector<CviImg *> &output,
                                 bool enable_min_max) {
  // FIXME: Support later
  if (m_slice_info.ping_pong_size != 1) {
    LOGI("Currently runSingleSizeKernel does not support ping pong.\n");
    m_slice_info.ping_pong_size = 1;
  }
  uint32_t batch = input[0]->m_tg.shape.n;
  uint32_t channel = input[0]->m_tg.shape.c;
  uint32_t height = input[0]->m_tg.shape.h;
  uint32_t width = input[0]->m_tg.shape.w;
  std::vector<bool> find_min_max;
  // Insert extra tl
  uint32_t nums_of_tl = m_slice_info.nums_of_tl;
  uint32_t fix_lmem_size = m_slice_info.fix_lmem_size;
#if 0  // Disable now
  if (enable_min_max) {
    nums_of_tl += 1;
    fix_lmem_size += (2 * output[0]->m_tg.shape.c);
  }
  for (size_t i = 0; i < output.size(); i++) {
    find_min_max.emplace_back(enable_min_max);
    if (enable_min_max) {
      nums_of_tl += 1;
      fix_lmem_size += (2 * 2 * output[i]->m_tg.shape.c);
    }
  }
#endif
  // FIXME: Move to constructor if possible.
  cvk_tl_shape_t tl_table_s;
  uint64_t result = cvm_lut_tbl_bytesize(cvk_ctx, &tl_table_s, CVK_FMT_U8);
  m_table_per_channel_size = result / m_chip_info.npu_num;  // 32 * 8 for bm1880v2
  SliceRes slice_res;
  int ret = getSlice(nums_of_tl, m_slice_info.nums_of_table, fix_lmem_size, batch, channel, height,
                     width, m_table_per_channel_size, m_kernel_info, m_chip_info.npu_num,
                     &slice_res.h, &slice_res.w, false);
  if (ret != CVI_SUCCESS) {
    return CVI_FAILURE;
  }

  SliceRes in_slice_res, out_slice_res;
  sliceSetup(slice_res, &in_slice_res, &out_slice_res);
  if (in_slice_res.h.turn != out_slice_res.h.turn) {
    LOGE("Input/ output h slice turn are not the same %u, %u.\n", in_slice_res.h.turn,
         out_slice_res.h.turn);
  }
  if (in_slice_res.w.turn != out_slice_res.w.turn) {
    LOGE("Input/ output w slice turn are not the same %u, %u.\n", in_slice_res.w.turn,
         out_slice_res.w.turn);
  }

  // Setup slice input/ output shapes and left shapes
  std::vector<cvk_tg_shape_t> s_in_vec, s_out_vec;
  for (size_t k = 0; k < input.size(); k++) {
    s_in_vec.push_back({1, channel, in_slice_res.h.slice, in_slice_res.w.slice});
  }
  for (size_t k = 0; k < output.size(); k++) {
    s_out_vec.push_back({1, channel, out_slice_res.h.slice, out_slice_res.w.slice});
  }
  std::vector<cvk_tg_shape_t> s_in_left_vec, s_out_left_vec;
  for (size_t k = 0; k < input.size(); k++) {
    s_in_left_vec.push_back({1, channel, in_slice_res.h.left, in_slice_res.w.left});
  }
  for (size_t k = 0; k < output.size(); k++) {
    s_out_left_vec.push_back({1, channel, out_slice_res.h.left, out_slice_res.w.left});
  }

  // allocate tl shape and get input/ output indices.
  std::vector<uint32_t> tl_in_idx, tl_out_idx;
  runSetup(rt_handle, cvk_ctx, s_in_vec, s_out_vec, &tl_in_idx, &tl_out_idx, false);
  if (m_allocate_failed_) {
    printf("allocate ive local mem failed\n");
    freeTLMems(cvk_ctx);
    return CVI_FAILURE;
  }
  // Dummy check, can be turned off in official release
  if (tl_in_idx.size() != input.size()) {
    LOGE("Input tl size not match input image num %u, %u\n", (uint32_t)tl_in_idx.size(),
         (uint32_t)input.size());
  }
  if (tl_out_idx.size() != output.size()) {
    LOGE("Output tl size not match input image num %u, %u\n", (uint32_t)tl_out_idx.size(),
         (uint32_t)output.size());
  }
  // Dummy check end
  // Category tl shapes
  std::vector<std::pair<int, cvk_tl_t *>> tl_in_shape_lmem_vec, tl_out_shape_lmem_vec;
  categoryIOTLShape(m_tl_vec, s_in_vec, s_out_vec, &tl_in_shape_lmem_vec, &tl_out_shape_lmem_vec);

  // Find and create input/ output fmt size pair.
  TLInfo tl_in_info, tl_out_info;
  getTLInfo(m_tl_vec, tl_in_idx, tl_out_idx, &tl_in_info, &tl_out_info);

  // Get device memory start offset
  BMAddrInfo bm_src_info, bm_dest_info;
  getBMAddrInfo(input, output, m_kernel_info.pad[0], m_kernel_info.pad[2], &bm_src_info,
                &bm_dest_info);

  // Create tg block
  cvk_tg_t tg_in;
  tg_in.base_reg_index = 0;
  cvk_tg_t tg_out;
  tg_out.base_reg_index = 0;

  // Main for loop
  for (uint32_t i = 0; i < slice_res.h.turn; i++) {
    // Re-assign head address to w.
    std::vector<uint64_t> bm_src_addr_w = bm_src_info.addr_vec;
    std::vector<uint64_t> bm_dest_addr_w = bm_dest_info.addr_vec;
    // Change H TL size to fit left shape in last turn
    for (size_t k = 0; k < tl_in_shape_lmem_vec.size(); k++) {
      int &index = tl_in_shape_lmem_vec[k].first;
      auto *lmem = tl_in_shape_lmem_vec[k].second;
      if (s_in_left_vec[index].h != 0) {
        if (i == 0) {
          lmem->shape.h = s_in_vec[index].h;
          lmem->stride = cvk_ctx->ops->tl_default_stride(cvk_ctx, lmem->shape, lmem->fmt, 1);
        } else if (i == in_slice_res.h.turn - 1) {
          lmem->shape.h = s_in_left_vec[index].h;
          lmem->stride = cvk_ctx->ops->tl_default_stride(cvk_ctx, lmem->shape, lmem->fmt, 1);
        }
      }
    }
    for (size_t k = 0; k < tl_out_shape_lmem_vec.size(); k++) {
      int &index = tl_out_shape_lmem_vec[k].first;
      auto *lmem = tl_out_shape_lmem_vec[k].second;
      if (s_out_left_vec[index].h != 0) {
        if (i == 0) {
          lmem->shape.h = s_out_vec[index].h;
          lmem->stride = cvk_ctx->ops->tl_default_stride(cvk_ctx, lmem->shape, lmem->fmt, 1);
        } else if (i == out_slice_res.h.turn - 1) {
          lmem->shape.h = s_out_left_vec[index].h;
          lmem->stride = cvk_ctx->ops->tl_default_stride(cvk_ctx, lmem->shape, lmem->fmt, 1);
        }
      }
    }

    for (uint32_t j = 0; j < slice_res.w.turn; j++) {
      // Change W TL size to fit left shape in last turn
      for (size_t k = 0; k < tl_in_shape_lmem_vec.size(); k++) {
        int &index = tl_in_shape_lmem_vec[k].first;
        auto *lmem = tl_in_shape_lmem_vec[k].second;
        if (s_in_left_vec[index].w != 0) {
          if (j == 0) {
            lmem->shape.w = s_in_vec[index].w;
            lmem->stride = cvk_ctx->ops->tl_default_stride(cvk_ctx, lmem->shape, lmem->fmt, 1);
          } else if (j == in_slice_res.w.turn - 1) {
            lmem->shape.w = s_in_left_vec[index].w;
            lmem->stride = cvk_ctx->ops->tl_default_stride(cvk_ctx, lmem->shape, lmem->fmt, 1);
          }
        }
      }
      for (size_t k = 0; k < tl_out_shape_lmem_vec.size(); k++) {
        int &index = tl_out_shape_lmem_vec[k].first;
        auto *lmem = tl_out_shape_lmem_vec[k].second;
        if (s_out_left_vec[index].w != 0) {
          if (j == 0) {
            lmem->shape.w = s_out_vec[index].w;
            lmem->stride = cvk_ctx->ops->tl_default_stride(cvk_ctx, lmem->shape, lmem->fmt, 1);
          } else if (j == out_slice_res.w.turn - 1) {
            lmem->shape.w = s_out_left_vec[index].w;
            lmem->stride = cvk_ctx->ops->tl_default_stride(cvk_ctx, lmem->shape, lmem->fmt, 1);
          }
        }
      }

      // tg2tl
      for (size_t k = 0; k < tl_in_info.lmem_vec.size(); k++) {
        tg_in.start_address = bm_src_addr_w[k];
        tg_in.shape.n = tl_in_info.lmem_vec[k]->shape.n;
        tg_in.shape.c = tl_in_info.lmem_vec[k]->shape.c;
        tg_in.shape.h = tl_in_info.lmem_vec[k]->shape.h;
        tg_in.shape.w = tl_in_info.lmem_vec[k]->shape.w;
        tg_in.fmt = bm_src_info.fns_vec[k].getFmt();
        tg_in.stride = input[k]->m_tg.stride;
        cvk_tdma_g2l_tensor_copy_param_t p_copy_in;
        memset(&p_copy_in, 0, sizeof(cvk_tdma_g2l_tensor_copy_param_t));
        p_copy_in.src = &tg_in;
        p_copy_in.dst = tl_in_info.lmem_vec[k];
        cvk_ctx->ops->tdma_g2l_bf16_tensor_copy(cvk_ctx, &p_copy_in);

        // Change src head addr
        bm_src_addr_w[k] += 1 * in_slice_res.w.skip * bm_src_info.fns_vec[k].getSize();
      }

      operation(rt_handle, cvk_ctx, 0);

      // tl2tg
      for (size_t k = 0; k < tl_out_info.lmem_vec.size(); k++) {
        tg_out.start_address = bm_dest_addr_w[k];
        tg_out.fmt = bm_dest_info.fns_vec[k].getFmt();
        tg_out.shape.n = tl_out_info.lmem_vec[k]->shape.n;
        tg_out.shape.c = tl_out_info.lmem_vec[k]->shape.c;
        tg_out.shape.h =
            tl_out_info.lmem_vec[k]->shape.h - (m_kernel_info.pad[2] + m_kernel_info.pad[3]);
        tg_out.shape.w =
            tl_out_info.lmem_vec[k]->shape.w - (m_kernel_info.pad[0] + m_kernel_info.pad[1]);
        tg_out.stride = output[k]->m_tg.stride;
        cvk_tl_t out_shape;
        auto &tl_out = tl_out_info.lmem_vec;
        // printf("st addr%d, tg st addr %lu\n", tl_out[k]->start_address, bm_dest_addr_w[k]);
        out_shape.start_address = tl_out[k]->start_address +
                                  (1 * tl_out[k]->stride.h * m_kernel_info.pad[2]) +
                                  (m_kernel_info.pad[0] * tl_out_info.fns_vec[k].getSize());
        out_shape.fmt = tl_out[k]->fmt;
        out_shape.cmprs_fmt = tl_out[k]->cmprs_fmt;
        out_shape.shape = tl_out[k]->shape;
        out_shape.shape.h = tg_out.shape.h;
        out_shape.shape.w = tg_out.shape.w;
        out_shape.stride = tl_out[k]->stride;
        cvk_tdma_l2g_tensor_copy_param_t p_copy_out;
        memset(&p_copy_out, 0, sizeof(cvk_tdma_l2g_tensor_copy_param_t));
        p_copy_out.src = &out_shape;
        p_copy_out.dst = &tg_out;
        cvk_ctx->ops->tdma_l2g_bf16_tensor_copy(cvk_ctx, &p_copy_out);

        // Change dest head addr
        bm_dest_addr_w[k] += 1 * out_slice_res.w.skip * bm_dest_info.fns_vec[k].getSize();
      }
    }
    // Change src/ dest head addr
    for (size_t k = 0; k < bm_src_info.addr_vec.size(); k++) {
      uint32_t jump_val = 0;
      if (i == in_slice_res.h.turn - 1) {
        jump_val = in_slice_res.h.left == 0 ? in_slice_res.h.slice : in_slice_res.h.left;
      } else {
        jump_val = in_slice_res.h.skip;
      }
      bm_src_info.addr_vec[k] += 1 * input[k]->m_tg.stride.h * jump_val;
    }
    for (size_t k = 0; k < bm_dest_info.addr_vec.size(); k++) {
      uint32_t jump_val = 0;
      if (i == out_slice_res.h.turn - 1) {
        jump_val = out_slice_res.h.left == 0 ? out_slice_res.h.slice : out_slice_res.h.left;
      } else {
        jump_val = out_slice_res.h.skip;
      }
      bm_dest_info.addr_vec[k] += 1 * output[k]->m_tg.stride.h * jump_val;
    }
  }
  LOGD("Slice info:\n");
  LOGD("{ h_slice, h_turn, h_skip, h_left} = { %d, %d, %d, %d}\n", in_slice_res.h.slice,
       in_slice_res.h.turn, in_slice_res.h.skip, in_slice_res.h.left);
  LOGD("{ w_slice, w_turn, w_skip, w_left} = { %d, %d, %d, %d}\n", in_slice_res.w.slice,
       in_slice_res.w.turn, in_slice_res.w.skip, in_slice_res.w.left);

  // Dummy gaurd for buffer overflow
  ret |= checkIsBufferOverflow(input, output, bm_src_info, bm_dest_info, m_kernel_info.pad[0],
                               m_kernel_info.pad[2], false, true);

  beforeSubmit(rt_handle, cvk_ctx, input, output);

  if (ret == CVI_SUCCESS) {
    CVI_RT_Submit(cvk_ctx);
  }

  freeTLMems(cvk_ctx);
  postProcess(rt_handle);
  return ret;
}

int IveCore::runSingleSizeKernelMultiBatch(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                                           const std::vector<CviImg *> &input,
                                           std::vector<CviImg *> &output, bool enable_min_max) {
  // FIXME: Support later
  if (m_slice_info.ping_pong_size != 1) {
    LOGD("Currently runSingleSizeKernel does not support ping pong.\n");
    m_slice_info.ping_pong_size = 1;
  }
  uint32_t batch = (uint32_t)input.size();
  uint32_t channel = 1;
  uint32_t height = input[0]->m_tg.shape.h;
  uint32_t width = input[0]->m_tg.shape.w;

  // Insert extra tl
  uint32_t nums_of_tl = m_slice_info.nums_of_tl;
  uint32_t fix_lmem_size = m_slice_info.fix_lmem_size;

  // FIXME: Move to constructor if possible.
  cvk_tl_shape_t tl_table_s;
  uint64_t result = cvm_lut_tbl_bytesize(cvk_ctx, &tl_table_s, CVK_FMT_U8);  // cv180:512
  m_table_per_channel_size = result / m_chip_info.npu_num;                   // cv180:256
  LOGD("localmemsize:%d,npu:%d,numbernpu:%d\n", (int)result, (int)m_chip_info.npu_num,
       (int)m_table_per_channel_size);
  SliceRes slice_res;
  int ret = getSlice(nums_of_tl, m_slice_info.nums_of_table, fix_lmem_size, 1, channel, height,
                     width, m_table_per_channel_size, m_kernel_info, m_chip_info.npu_num,
                     &slice_res.h, &slice_res.w, false);
  if (ret != CVI_SUCCESS) {
    return CVI_FAILURE;
  }

  SliceRes in_slice_res, out_slice_res;
  sliceSetup(slice_res, &in_slice_res, &out_slice_res);
  if (in_slice_res.h.turn != out_slice_res.h.turn) {
    LOGE("Input/ output h slice turn are not the same %u, %u.\n", in_slice_res.h.turn,
         out_slice_res.h.turn);
  }
  if (in_slice_res.w.turn != out_slice_res.w.turn) {
    LOGE("Input/ output w slice turn are not the same %u, %u.\n", in_slice_res.w.turn,
         out_slice_res.w.turn);
  }

  // Setup slice input/ output shapes and left shapes
  std::vector<cvk_tg_shape_t> s_in_vec, s_out_vec;
  for (size_t k = 0; k < 1; k++) {
    s_in_vec.push_back({1, channel, in_slice_res.h.slice, in_slice_res.w.slice});
  }
  for (size_t k = 0; k < 1; k++) {
    s_out_vec.push_back({1, channel, out_slice_res.h.slice, out_slice_res.w.slice});
  }
  LOGD("in hslice:%d,in_w_slice:%d,out_hslice:%d,out_w_slice:%d\n", (int)in_slice_res.h.slice,
       (int)in_slice_res.w.slice, (int)out_slice_res.h.slice, out_slice_res.w.slice);
  std::vector<cvk_tg_shape_t> s_in_left_vec, s_out_left_vec;
  for (size_t k = 0; k < 1; k++) {
    s_in_left_vec.push_back({1, channel, in_slice_res.h.left, in_slice_res.w.left});
  }
  for (size_t k = 0; k < 1; k++) {
    s_out_left_vec.push_back({1, channel, out_slice_res.h.left, out_slice_res.w.left});
  }

  // allocate tl shape and get input/ output indices.
  std::vector<uint32_t> tl_in_idx, tl_out_idx;
  runSetup(rt_handle, cvk_ctx, s_in_vec, s_out_vec, &tl_in_idx, &tl_out_idx, false);
  if (m_allocate_failed_) {
    LOGE("allocate ive local mem failed\n");
    freeTLMems(cvk_ctx);
    return CVI_FAILURE;
  }
  // Dummy check, can be turned off in official release
  if (tl_in_idx.size() != input.size()) {
    LOGW("runSingleSizeKernelMultiBatch Input tl size not match input image num %u, %u\n",
         (uint32_t)tl_in_idx.size(), (uint32_t)input.size());
  }
  if (tl_out_idx.size() != output.size()) {
    LOGW("runSingleSizeKernelMultiBatch Output tl size not match input image num %u, %u\n",
         (uint32_t)tl_out_idx.size(), (uint32_t)output.size());
  }
  // Dummy check end
  // Category tl shapes
  std::vector<std::pair<int, cvk_tl_t *>> tl_in_shape_lmem_vec, tl_out_shape_lmem_vec;
  categoryIOTLShape(m_tl_vec, s_in_vec, s_out_vec, &tl_in_shape_lmem_vec, &tl_out_shape_lmem_vec);

  // Find and create input/ output fmt size pair.
  TLInfo tl_in_info, tl_out_info;
  getTLInfo(m_tl_vec, tl_in_idx, tl_out_idx, &tl_in_info, &tl_out_info);

  // Get device memory start offset
  BMAddrInfo bm_src_info, bm_dest_info;
  getBMAddrInfo(input, output, m_kernel_info.pad[0], m_kernel_info.pad[2], &bm_src_info,
                &bm_dest_info);

  // Create tg block
  cvk_tg_t tg_in;
  tg_in.base_reg_index = 0;
  cvk_tg_t tg_out;
  tg_out.base_reg_index = 0;

  std::vector<uint64_t> bm_src_addr_bk = bm_src_info.addr_vec;
  std::vector<uint64_t> bm_dst_addr_bk = bm_dest_info.addr_vec;

  std::vector<uint64_t> processed_src_addr_bk = bm_src_info.addr_vec;
  std::vector<uint64_t> processed_dest_addr_bk = bm_dest_info.addr_vec;
  // Main for loop
  for (uint32_t b = 0; b < batch; b++) {
    LOGD("process yuv batch:%d,src_addr:%lx,dst_addr:%lx\n", (int)b, bm_src_addr_bk[b],
         bm_dst_addr_bk[b]);
    bm_src_info.addr_vec = {bm_src_addr_bk[b]};
    bm_dest_info.addr_vec = {bm_dst_addr_bk[b]};
    for (uint32_t i = 0; i < slice_res.h.turn; i++) {
      // Re-assign head address to w.
      std::vector<uint64_t> bm_src_addr_w = bm_src_info.addr_vec;
      std::vector<uint64_t> bm_dest_addr_w = bm_dest_info.addr_vec;
      // Change H TL size to fit left shape in last turn
      for (size_t k = 0; k < tl_in_shape_lmem_vec.size(); k++) {
        int &index = tl_in_shape_lmem_vec[k].first;
        auto *lmem = tl_in_shape_lmem_vec[k].second;
        if (s_in_left_vec[index].h != 0) {
          if (i == 0) {
            lmem->shape.h = s_in_vec[index].h;
            lmem->stride = cvk_ctx->ops->tl_default_stride(cvk_ctx, lmem->shape, lmem->fmt, 1);
          } else if (i == in_slice_res.h.turn - 1) {
            lmem->shape.h = s_in_left_vec[index].h;
            lmem->stride = cvk_ctx->ops->tl_default_stride(cvk_ctx, lmem->shape, lmem->fmt, 1);
          }
        }
      }
      for (size_t k = 0; k < tl_out_shape_lmem_vec.size(); k++) {
        int &index = tl_out_shape_lmem_vec[k].first;
        auto *lmem = tl_out_shape_lmem_vec[k].second;
        if (s_out_left_vec[index].h != 0) {
          if (i == 0) {
            lmem->shape.h = s_out_vec[index].h;
            lmem->stride = cvk_ctx->ops->tl_default_stride(cvk_ctx, lmem->shape, lmem->fmt, 1);
          } else if (i == out_slice_res.h.turn - 1) {
            lmem->shape.h = s_out_left_vec[index].h;
            lmem->stride = cvk_ctx->ops->tl_default_stride(cvk_ctx, lmem->shape, lmem->fmt, 1);
          }
        }
      }

      for (uint32_t j = 0; j < slice_res.w.turn; j++) {
        // Change W TL size to fit left shape in last turn
        for (size_t k = 0; k < tl_in_shape_lmem_vec.size(); k++) {
          int &index = tl_in_shape_lmem_vec[k].first;
          auto *lmem = tl_in_shape_lmem_vec[k].second;
          if (s_in_left_vec[index].w != 0) {
            if (j == 0) {
              lmem->shape.w = s_in_vec[index].w;
              lmem->stride = cvk_ctx->ops->tl_default_stride(cvk_ctx, lmem->shape, lmem->fmt, 1);
            } else if (j == in_slice_res.w.turn - 1) {
              lmem->shape.w = s_in_left_vec[index].w;
              lmem->stride = cvk_ctx->ops->tl_default_stride(cvk_ctx, lmem->shape, lmem->fmt, 1);
            }
          }
        }
        for (size_t k = 0; k < tl_out_shape_lmem_vec.size(); k++) {
          int &index = tl_out_shape_lmem_vec[k].first;
          auto *lmem = tl_out_shape_lmem_vec[k].second;
          if (s_out_left_vec[index].w != 0) {
            if (j == 0) {
              lmem->shape.w = s_out_vec[index].w;
              lmem->stride = cvk_ctx->ops->tl_default_stride(cvk_ctx, lmem->shape, lmem->fmt, 1);
            } else if (j == out_slice_res.w.turn - 1) {
              lmem->shape.w = s_out_left_vec[index].w;
              lmem->stride = cvk_ctx->ops->tl_default_stride(cvk_ctx, lmem->shape, lmem->fmt, 1);
            }
          }
        }

        // tg2tl
        for (size_t k = 0; k < tl_in_info.lmem_vec.size(); k++) {
          tg_in.start_address = bm_src_addr_w[k];
          tg_in.shape.n = tl_in_info.lmem_vec[k]->shape.n;
          tg_in.shape.c = tl_in_info.lmem_vec[k]->shape.c;
          tg_in.shape.h = tl_in_info.lmem_vec[k]->shape.h;
          tg_in.shape.w = tl_in_info.lmem_vec[k]->shape.w;
          tg_in.fmt = bm_src_info.fns_vec[k].getFmt();
          tg_in.stride = input[k]->m_tg.stride;
          cvk_tdma_g2l_tensor_copy_param_t p_copy_in;
          memset(&p_copy_in, 0, sizeof(cvk_tdma_g2l_tensor_copy_param_t));
          p_copy_in.src = &tg_in;
          p_copy_in.dst = tl_in_info.lmem_vec[k];
          cvk_ctx->ops->tdma_g2l_bf16_tensor_copy(cvk_ctx, &p_copy_in);

          // Change src head addr
          bm_src_addr_w[k] += 1 * in_slice_res.w.skip * bm_src_info.fns_vec[k].getSize();
        }

        operation(rt_handle, cvk_ctx, 0);

        // tl2tg
        for (size_t k = 0; k < tl_out_info.lmem_vec.size(); k++) {
          tg_out.start_address = bm_dest_addr_w[k];
          tg_out.fmt = bm_dest_info.fns_vec[k].getFmt();
          tg_out.shape.n = tl_out_info.lmem_vec[k]->shape.n;
          tg_out.shape.c = tl_out_info.lmem_vec[k]->shape.c;
          tg_out.shape.h =
              tl_out_info.lmem_vec[k]->shape.h - (m_kernel_info.pad[2] + m_kernel_info.pad[3]);
          tg_out.shape.w =
              tl_out_info.lmem_vec[k]->shape.w - (m_kernel_info.pad[0] + m_kernel_info.pad[1]);
          tg_out.stride = output[k]->m_tg.stride;
          cvk_tl_t out_shape;
          auto &tl_out = tl_out_info.lmem_vec;
          // printf("st addr%d, tg st addr %lu\n", tl_out[k]->start_address, bm_dest_addr_w[k]);
          out_shape.start_address = tl_out[k]->start_address +
                                    (1 * tl_out[k]->stride.h * m_kernel_info.pad[2]) +
                                    (m_kernel_info.pad[0] * tl_out_info.fns_vec[k].getSize());
          out_shape.fmt = tl_out[k]->fmt;
          out_shape.cmprs_fmt = tl_out[k]->cmprs_fmt;
          out_shape.shape = tl_out[k]->shape;
          out_shape.shape.h = tg_out.shape.h;
          out_shape.shape.w = tg_out.shape.w;
          out_shape.stride = tl_out[k]->stride;
          cvk_tdma_l2g_tensor_copy_param_t p_copy_out;
          memset(&p_copy_out, 0, sizeof(cvk_tdma_l2g_tensor_copy_param_t));
          p_copy_out.src = &out_shape;
          p_copy_out.dst = &tg_out;
          cvk_ctx->ops->tdma_l2g_bf16_tensor_copy(cvk_ctx, &p_copy_out);

          // Change dest head addr
          bm_dest_addr_w[k] += 1 * out_slice_res.w.skip * bm_dest_info.fns_vec[k].getSize();
        }
      }
      // Change src/ dest head addr
      for (size_t k = 0; k < bm_src_info.addr_vec.size(); k++) {
        uint32_t jump_val = 0;
        if (i == in_slice_res.h.turn - 1) {
          jump_val = in_slice_res.h.left == 0 ? in_slice_res.h.slice : in_slice_res.h.left;
        } else {
          jump_val = in_slice_res.h.skip;
        }
        bm_src_info.addr_vec[k] += 1 * input[k]->m_tg.stride.h * jump_val;
      }
      for (size_t k = 0; k < bm_dest_info.addr_vec.size(); k++) {
        uint32_t jump_val = 0;
        if (i == out_slice_res.h.turn - 1) {
          jump_val = out_slice_res.h.left == 0 ? out_slice_res.h.slice : out_slice_res.h.left;
        } else {
          jump_val = out_slice_res.h.skip;
        }
        bm_dest_info.addr_vec[k] += 1 * output[k]->m_tg.stride.h * jump_val;
      }
    }
    processed_src_addr_bk[b] = bm_src_info.addr_vec[0];
    processed_dest_addr_bk[b] = bm_dest_info.addr_vec[0];
  }
  // after batch process
  LOGD("Slice info:\n");
  LOGD("{ h_slice, h_turn, h_skip, h_left} = { %d, %d, %d, %d}\n", in_slice_res.h.slice,
       in_slice_res.h.turn, in_slice_res.h.skip, in_slice_res.h.left);
  LOGD("{ w_slice, w_turn, w_skip, w_left} = { %d, %d, %d, %d}\n", in_slice_res.w.slice,
       in_slice_res.w.turn, in_slice_res.w.skip, in_slice_res.w.left);

  beforeSubmit(rt_handle, cvk_ctx, input, output);
  bm_src_info.addr_vec = processed_src_addr_bk;
  bm_dest_info.addr_vec = processed_dest_addr_bk;

  ret |= checkIsBufferOverflow(input, output, bm_src_info, bm_dest_info, m_kernel_info.pad[0],
                               m_kernel_info.pad[2], false, true);
  if (ret == CVI_SUCCESS) {
    CVI_RT_Submit(cvk_ctx);
  }

  freeTLMems(cvk_ctx);
  postProcess(rt_handle);
  return ret;
}
int IveCore::runSingleSizeExtKernel(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                                    const std::vector<CviImg *> &input,
                                    std::vector<CviImg *> &output, bool enable_min_max) {
  if (m_slice_info.io_fmt == CVK_FMT_INVALID) {
    LOGE("Invalid fmt engine type.\n");
    return CVI_FAILURE;
  }
  // FIXME: Support later
  if (m_slice_info.ping_pong_size != 1) {
    LOGI("Currently runSingleSizeKernel does not support ping pong.\n");
    m_slice_info.ping_pong_size = 1;
  }
  if (input[0]->m_tg.shape.n != 1) {
    LOGE("Currently ext only supports single batch.\n");
    return CVI_FAILURE;
  }
  // TODO: FIXME: Due to HW limitation. We have to split channels into individual images
  // to process. Let channel = 1, do input[0]->m_tg.shape.c times.
  uint32_t batch = input[0]->m_tg.shape.c;
  uint32_t channel = 1;
  uint32_t height = input[0]->m_tg.shape.h;
  uint32_t width = input[0]->m_tg.shape.w;
  uint32_t w_from_stride = input[0]->m_tg.stride.h / getFmtSize(input[0]->m_tg.fmt);
  uint32_t w_from_stride_out =
      output.empty() ? w_from_stride : output[0]->m_tg.stride.h / getFmtSize(output[0]->m_tg.fmt);
  // Insert extra tl
  uint32_t nums_of_tl = m_slice_info.nums_of_tl;
  uint32_t fix_lmem_size = m_slice_info.fix_lmem_size;

  // FIXME: Move to constructor if possible.
  cvk_tl_shape_t tl_table_s;
  uint64_t result = cvm_lut_tbl_bytesize(cvk_ctx, &tl_table_s, CVK_FMT_U8);
  m_table_per_channel_size = result / m_chip_info.npu_num;  // 32 * 8 for bm1880v2
  SliceRes slice_res;
  // FIXME: batch is currently fixed to 1 due to HW limitation.
  int ret = getSlice(nums_of_tl, m_slice_info.nums_of_table, fix_lmem_size, 1, channel, height,
                     width, m_table_per_channel_size, m_kernel_info, m_chip_info.npu_num,
                     &slice_res.h, &slice_res.w, true);
  if (ret != CVI_SUCCESS) {
    return CVI_FAILURE;
  }
  // FIXME: Slice Setup not supported in this mode yet.
  // sliceSetup(slice_res, &in_slice_res, &out_slice_res);
  // channelExt will temporarily replace them.
  // Need mechanism for algorithm such as BLOCK
  SliceRes in_slice_res, out_slice_res;
  in_slice_res = slice_res;
  out_slice_res = slice_res;
  // Convert to API acceptance input.
  const int kernel_pad = m_kernel_info.size - 1;
  const int vertical_pad_total = m_kernel_info.pad[2] + m_kernel_info.pad[3];
  out_slice_res.h.slice -= kernel_pad;
  out_slice_res.h.left = out_slice_res.h.left == 0 ? 0 : out_slice_res.h.left - kernel_pad;
  // channel ext supports w, so no need to change out_slice_res.w
  // out_slice_res.w.slice -= (m_kernel_info.pad[0] + m_kernel_info.pad[1]);
  // out_slice_res.w.left -= (m_kernel_info.pad[0] + m_kernel_info.pad[1]);
  // Input/ output types may varies, so we calculate all the stride using CVK_FMT_U8 then multiply
  // the stride manually.
  cvk_fmt_t tgin_fmt_type = CVK_FMT_U8;
  cvk_fmt_t tgout_fmt_type = CVK_FMT_U8;
  TensorSliceInfo out_info, out_w_info, out_h_info, out_wh_info;
  channelExtension(cvk_ctx, w_from_stride, w_from_stride_out, channel, out_slice_res.h.slice,
                   out_slice_res.w.slice, out_slice_res.h.c_multiplier, m_kernel_info.pad[0],
                   m_kernel_info.pad[1], m_kernel_info.pad[2], m_kernel_info.pad[3],
                   m_kernel_info.size, m_kernel_info.size, m_kernel_info.default_stride_y,
                   m_kernel_info.default_stride_x, tgin_fmt_type, tgout_fmt_type,
                   m_slice_info.io_fmt, &out_info);
  uint32_t out_slice_res_h_left_pixels = 0;
  uint32_t in_slice_res_h_left_pixels = 0;
  uint32_t out_slice_res_w_left_pixels = 0;
  uint32_t in_slice_res_w_left_pixels = 0;
  if (slice_res.w.left != 0) {
    channelExtension(cvk_ctx, w_from_stride, w_from_stride_out, channel, out_slice_res.h.slice,
                     out_slice_res.w.left, out_slice_res.h.c_multiplier, m_kernel_info.pad[0],
                     m_kernel_info.pad[1], m_kernel_info.pad[2], m_kernel_info.pad[3],
                     m_kernel_info.size, m_kernel_info.size, m_kernel_info.default_stride_y,
                     m_kernel_info.default_stride_x, tgin_fmt_type, tgout_fmt_type,
                     m_slice_info.io_fmt, &out_w_info);
  } else {
    out_w_info = out_info;
  }
  if (out_slice_res.h.left != 0) {
    uint32_t c_multiplier = 0;
    uint32_t out_h_left_pixels = 0;
    if (calculateOutExtHSlice(m_chip_info.npu_num, channel, out_info.tg_store.shape.h,
                              out_slice_res.h.left, &c_multiplier, &out_h_left_pixels)) {
      uint32_t in_h_left_pixels = out_h_left_pixels + kernel_pad;
      out_slice_res.h.left -= out_h_left_pixels;
      out_slice_res.h.turn++;
      in_slice_res.h.left -= in_h_left_pixels;
      in_slice_res.h.turn++;
      slice_res.h.left = in_slice_res.h.left;
      slice_res.h.turn++;
      if (in_h_left_pixels >= m_kernel_info.size) {
        out_slice_res_h_left_pixels = out_h_left_pixels;
        in_slice_res_h_left_pixels = in_h_left_pixels;
      }
    }
    channelExtension(cvk_ctx, w_from_stride, w_from_stride_out, channel, out_slice_res.h.left,
                     out_slice_res.w.slice, c_multiplier, m_kernel_info.pad[0],
                     m_kernel_info.pad[1], m_kernel_info.pad[2], m_kernel_info.pad[3],
                     m_kernel_info.size, m_kernel_info.size, m_kernel_info.default_stride_y,
                     m_kernel_info.default_stride_x, tgin_fmt_type, tgout_fmt_type,
                     m_slice_info.io_fmt, &out_h_info);
  } else {
    out_h_info = out_info;
  }
  if (out_slice_res.h.left != 0 && out_slice_res.w.left != 0) {
    int c_multiplier = m_chip_info.npu_num / channel;
    while (out_slice_res.h.left % c_multiplier != 0) {
      c_multiplier--;
    }
    channelExtension(cvk_ctx, w_from_stride, w_from_stride_out, channel, out_slice_res.h.left,
                     out_slice_res.w.left, c_multiplier, m_kernel_info.pad[0], m_kernel_info.pad[1],
                     m_kernel_info.pad[2], m_kernel_info.pad[3], m_kernel_info.size,
                     m_kernel_info.size, m_kernel_info.default_stride_y,
                     m_kernel_info.default_stride_x, tgin_fmt_type, tgout_fmt_type,
                     m_slice_info.io_fmt, &out_wh_info);
  } else if (out_slice_res.h.left != 0 && out_slice_res.w.left == 0) {
    out_wh_info = out_h_info;
  } else if (out_slice_res.h.left == 0 && out_slice_res.w.left != 0) {
    out_wh_info = out_w_info;
  } else {
    out_wh_info = out_info;
  }

  // out_info       out_w_info          out_wlp_h_info
  // out_h_info     out_wh_info         out_wlp_h_left_info
  // out_hlp_w_info out_hlp_w_left_info out_wlp_hlp_info
  TensorSliceInfo out_hlp_w_info, out_hlp_w_left_info, out_wlp_h_info, out_wlp_h_left_info,
      out_wlp_hlp_info;
  if (out_slice_res_h_left_pixels != 0) {
    updateTSIInfo(cvk_ctx, 1, channel, in_slice_res_h_left_pixels, in_slice_res.w.slice, 1, channel,
                  out_slice_res_h_left_pixels, out_slice_res.w.slice, m_slice_info.io_fmt,
                  tgin_fmt_type, tgout_fmt_type, &out_hlp_w_info);
    if (out_slice_res.w.left != 0) {
      updateTSIInfo(cvk_ctx, 1, channel, in_slice_res_h_left_pixels, in_slice_res.w.left, 1,
                    channel, out_slice_res_h_left_pixels, out_slice_res.w.left, m_slice_info.io_fmt,
                    tgin_fmt_type, tgout_fmt_type, &out_hlp_w_left_info);
    } else {
      out_hlp_w_left_info = out_hlp_w_info;
    }
  } else {
    out_hlp_w_info = out_h_info;
    if (out_slice_res.w.left != 0) {
      out_hlp_w_left_info = out_wh_info;
    } else {
      out_hlp_w_left_info = out_hlp_w_info;
    }
  }
  if (out_slice_res_w_left_pixels != 0) {
    updateTSIInfo(cvk_ctx, 1, channel, in_slice_res.h.slice, in_slice_res_w_left_pixels, 1, channel,
                  out_slice_res.h.slice, out_slice_res_w_left_pixels, m_slice_info.io_fmt,
                  tgin_fmt_type, tgout_fmt_type, &out_wlp_h_info);
    if (out_slice_res.h.left != 0) {
      updateTSIInfo(cvk_ctx, 1, channel, in_slice_res.h.left, in_slice_res_w_left_pixels, 1,
                    channel, out_slice_res.h.left, out_slice_res_w_left_pixels, m_slice_info.io_fmt,
                    tgin_fmt_type, tgout_fmt_type, &out_wlp_h_left_info);
    } else {
      out_wlp_h_left_info = out_wlp_h_info;
    }
  } else {
    out_wlp_h_info = out_w_info;
    if (out_slice_res.w.left != 0) {
      out_wlp_h_left_info = out_wh_info;
    } else {
      out_wlp_h_left_info = out_wlp_h_info;
    }
  }
  if (out_slice_res_h_left_pixels != 0 && out_slice_res_w_left_pixels != 0) {
    updateTSIInfo(cvk_ctx, 1, channel, in_slice_res_h_left_pixels, in_slice_res_w_left_pixels, 1,
                  channel, out_slice_res_h_left_pixels, out_slice_res_w_left_pixels,
                  m_slice_info.io_fmt, tgin_fmt_type, tgout_fmt_type, &out_wlp_hlp_info);
  } else if (out_slice_res_h_left_pixels != 0 && out_slice_res_w_left_pixels == 0) {
    out_wlp_hlp_info = out_hlp_w_left_info;
  } else if (out_slice_res_h_left_pixels == 0 && out_slice_res_w_left_pixels != 0) {
    out_wlp_hlp_info = out_wlp_h_left_info;
  } else {
    out_wlp_hlp_info = out_wh_info;
  }

  // Setup slice input/ output shapes and left shapes
  std::vector<cvk_tg_shape_t> s_in_vec, s_out_vec;
  for (size_t k = 0; k < input.size(); k++) {
    s_in_vec.push_back(
        {1, out_info.tl_load.shape.c, out_info.tl_load.shape.h, out_info.tl_load.shape.w});
  }
  for (size_t k = 0; k < output.size(); k++) {
    s_out_vec.push_back(
        {1, out_info.tl_store.shape.c, out_info.tl_store.shape.h, out_info.tl_store.shape.w});
  }

  // allocate tl shape and get input/ output indices.
  std::vector<uint32_t> tl_in_idx, tl_out_idx;
  runSetup(rt_handle, cvk_ctx, s_in_vec, s_out_vec, &tl_in_idx, &tl_out_idx, true);
  if (m_allocate_failed_) {
    printf("allocate ive local mem failed\n");
    freeTLMems(cvk_ctx);
    return CVI_FAILURE;
  }
  // Dummy check, can be turned off in official release
  if (tl_in_idx.size() != input.size()) {
    LOGE("Input tl size not match input image num %u, %u.\n", (uint32_t)tl_in_idx.size(),
         (uint32_t)input.size());
  }
  if (tl_out_idx.size() != output.size()) {
    LOGE("Output tl size not match input image num %u, %u.\n", (uint32_t)tl_out_idx.size(),
         (uint32_t)output.size());
  }
  // Dummy check end

  // Category tl shapes
  std::vector<std::pair<int, cvk_tl_t *>> tl_in_shape_lmem_vec, tl_out_shape_lmem_vec;
  categoryIOTLShape(m_tl_vec, s_in_vec, s_out_vec, &tl_in_shape_lmem_vec, &tl_out_shape_lmem_vec);

  // Find and create input/ output fmt size pair.
  TLInfo tl_in_info, tl_out_info;
  getTLInfo(m_tl_vec, tl_in_idx, tl_out_idx, &tl_in_info, &tl_out_info);

  // Get device memory start offset
  BMAddrInfo bm_src_info, bm_dest_info;
  getBMAddrInfo(input, output, m_kernel_info.pad[0], m_kernel_info.pad[2], &bm_src_info,
                &bm_dest_info);

  // Experimental feature
  std::vector<bool> extend_channel(input.size(), false);
  uint32_t max_channel = 1;
  bool extend_error = false;
  for (uint32_t i = 0; i < input.size(); i++) {
    if (input[i]->m_tg.shape.c != 1) {
      if (max_channel != 1 && input[i]->m_tg.shape.c != max_channel) {
        extend_error = true;
        break;
      }
      max_channel = input[i]->m_tg.shape.c;
    } else {
      extend_channel[i] = true;
    }
  }
  if (extend_error) {
    LOGE("Failed to extend layer.\n");
    return CVI_FAILURE;
  }

  // Create tg block
  cvk_tg_t tg_in;
  tg_in.base_reg_index = 0;
  cvk_tg_t tg_out;
  tg_out.base_reg_index = 0;

  std::vector<uint64_t> bm_src_addr_bk = bm_src_info.addr_vec;
  std::vector<uint64_t> bm_dst_addr_bk = bm_dest_info.addr_vec;
  // Main for loop
  for (uint32_t b = 0; b < batch; b++) {
    TensorSliceInfo *tsi = &out_info;
    for (size_t k = 0; k < bm_src_info.addr_vec.size(); k++) {
      if (extend_channel[k]) {
        bm_src_info.addr_vec[k] = bm_src_addr_bk[k];
      }
    }
    for (uint32_t i = 0; i < slice_res.h.turn; i++) {
      // Re-assign head address to w.
      std::vector<uint64_t> bm_src_addr_w = bm_src_info.addr_vec;
      std::vector<uint64_t> bm_dest_addr_w = bm_dest_info.addr_vec;
      for (uint32_t j = 0; j < slice_res.w.turn; j++) {
        // Change H TL size to fit left shape in last turn
        // out_info       out_w_info          out_wlp_h_info
        // out_h_info     out_wh_info         out_wlp_h_left_info
        // out_hlp_w_info out_hlp_w_left_info out_wlp_hlp_info
        if (i == in_slice_res.h.turn - 1 && out_slice_res_h_left_pixels != 0) {
          if (j == 0) {
            tsi = &out_hlp_w_info;
            updateLMemSize(cvk_ctx, m_chip_info.npu_num, m_slice_info.io_fmt, *tsi, m_tl_type,
                           &tl_in_shape_lmem_vec, &tl_out_shape_lmem_vec, &m_tl_vec);
          } else if (j == in_slice_res.w.turn - 1 && out_slice_res_w_left_pixels != 0) {
            tsi = &out_wlp_hlp_info;
            updateLMemSize(cvk_ctx, m_chip_info.npu_num, m_slice_info.io_fmt, *tsi, m_tl_type,
                           &tl_in_shape_lmem_vec, &tl_out_shape_lmem_vec, &m_tl_vec);
          } else if ((j == in_slice_res.w.turn - 1 && out_slice_res_w_left_pixels == 0) ||
                     (j == in_slice_res.w.turn - 2 && out_slice_res_w_left_pixels != 0)) {
            tsi = &out_wlp_h_left_info;
            updateLMemSize(cvk_ctx, m_chip_info.npu_num, m_slice_info.io_fmt, *tsi, m_tl_type,
                           &tl_in_shape_lmem_vec, &tl_out_shape_lmem_vec, &m_tl_vec);
          }
        } else if ((i == in_slice_res.h.turn - 1 && out_slice_res_h_left_pixels == 0) ||
                   (i == in_slice_res.h.turn - 2 && out_slice_res_h_left_pixels != 0)) {
          if (j == 0) {
            tsi = &out_h_info;
            updateLMemSize(cvk_ctx, m_chip_info.npu_num, m_slice_info.io_fmt, *tsi, m_tl_type,
                           &tl_in_shape_lmem_vec, &tl_out_shape_lmem_vec, &m_tl_vec);
          } else if (j == in_slice_res.w.turn - 1 && out_slice_res_w_left_pixels != 0) {
            tsi = &out_wlp_h_left_info;
            updateLMemSize(cvk_ctx, m_chip_info.npu_num, m_slice_info.io_fmt, *tsi, m_tl_type,
                           &tl_in_shape_lmem_vec, &tl_out_shape_lmem_vec, &m_tl_vec);
          } else if ((j == in_slice_res.w.turn - 1 && out_slice_res_w_left_pixels == 0) ||
                     (j == in_slice_res.w.turn - 2 && out_slice_res_w_left_pixels != 0)) {
            tsi = &out_wh_info;
            updateLMemSize(cvk_ctx, m_chip_info.npu_num, m_slice_info.io_fmt, *tsi, m_tl_type,
                           &tl_in_shape_lmem_vec, &tl_out_shape_lmem_vec, &m_tl_vec);
          }
        } else {
          if (j == 0) {
            tsi = &out_info;
            updateLMemSize(cvk_ctx, m_chip_info.npu_num, m_slice_info.io_fmt, *tsi, m_tl_type,
                           &tl_in_shape_lmem_vec, &tl_out_shape_lmem_vec, &m_tl_vec);
          } else if (j == in_slice_res.w.turn - 1 && out_slice_res_w_left_pixels != 0) {
            tsi = &out_wlp_h_info;
            updateLMemSize(cvk_ctx, m_chip_info.npu_num, m_slice_info.io_fmt, *tsi, m_tl_type,
                           &tl_in_shape_lmem_vec, &tl_out_shape_lmem_vec, &m_tl_vec);
          } else if ((j == in_slice_res.w.turn - 1 && out_slice_res_w_left_pixels == 0) ||
                     (j == in_slice_res.w.turn - 2 && out_slice_res_w_left_pixels != 0)) {
            tsi = &out_w_info;
            updateLMemSize(cvk_ctx, m_chip_info.npu_num, m_slice_info.io_fmt, *tsi, m_tl_type,
                           &tl_in_shape_lmem_vec, &tl_out_shape_lmem_vec, &m_tl_vec);
          }
        }

        // tg2tl
        for (size_t k = 0; k < tl_in_info.lmem_vec.size(); k++) {
          auto &tl_in = tl_in_info.lmem_vec;
          tg_in.start_address = bm_src_addr_w[k];
          tg_in.shape = tsi->tg_load.shape;
          tg_in.stride.n = tsi->tg_load.stride.n * bm_src_info.fns_vec[k].getSize();
          tg_in.stride.c = tsi->tg_load.stride.c * bm_src_info.fns_vec[k].getSize();
          tg_in.stride.h = tsi->tg_load.stride.h * bm_src_info.fns_vec[k].getSize();
          tg_in.fmt = bm_src_info.fns_vec[k].getFmt();

          // clang-format off
          LOGD("[%zu] In\n"
               " tg start address %" PRIu64 "\n"
               " tg shape %d %d %d %d\n"
               " tg stride %d %d %d\n"
               " tl shape %d %d %d %d\n"
               " tl stride %u %u %u %u\n",
               k,
               tg_in.start_address,
               tg_in.shape.n, tg_in.shape.c, tg_in.shape.h, tg_in.shape.w,
               tg_in.stride.n, tg_in.stride.c, tg_in.stride.h,
               tl_in[k]->shape.n, tl_in[k]->shape.c, tl_in[k]->shape.h, tl_in[k]->shape.w,
               tl_in[k]->stride.n, tl_in[k]->stride.c, tl_in[k]->stride.h, tl_in[k]->stride.w);
          // clang-format on

          cvk_tdma_g2l_tensor_copy_param_t p_copy_in;
          memset(&p_copy_in, 0, sizeof(cvk_tdma_g2l_tensor_copy_param_t));
          p_copy_in.src = &tg_in;
          p_copy_in.dst = tl_in[k];
          cvk_ctx->ops->tdma_g2l_bf16_tensor_copy(cvk_ctx, &p_copy_in);

          // Change src head addr
          bm_src_addr_w[k] += 1 * in_slice_res.w.skip * bm_src_info.fns_vec[k].getSize();
        }

        operation(rt_handle, cvk_ctx, 0);

        // tl2tg
        for (size_t k = 0; k < tl_out_info.lmem_vec.size(); k++) {
          tg_out.start_address = bm_dest_addr_w[k];
          tg_out.fmt = bm_dest_info.fns_vec[k].getFmt();
          tg_out.shape = tsi->tg_store.shape;
          // Note: Channel extension does not support h padding.
          // tg_out.shape.h = tl_out[k]->shape.h - (m_kernel_info.pad[2] + m_kernel_info.pad[3]);
          tg_out.shape.w = tg_out.shape.w - (m_kernel_info.pad[0] + m_kernel_info.pad[1]);
          tg_out.stride.n = tsi->tg_store.stride.n * bm_dest_info.fns_vec[k].getSize();
          tg_out.stride.c = tsi->tg_store.stride.c * bm_dest_info.fns_vec[k].getSize();
          tg_out.stride.h = tsi->tg_store.stride.h * bm_dest_info.fns_vec[k].getSize();
          auto &tl_out = tl_out_info.lmem_vec;
          cvk_tl_t out_shape;
          // printf("st addr%d, tg st addr %lu\n", tl_out[k]->start_address, bm_dest_addr_w[k]);
          out_shape.start_address =
              tl_out[k]->start_address + (m_kernel_info.pad[0] * tl_out_info.fns_vec[k].getSize());
          out_shape.fmt = tl_out[k]->fmt;
          out_shape.cmprs_fmt = tl_out[k]->cmprs_fmt;
          out_shape.shape = tl_out[k]->shape;
          out_shape.stride = tl_out[k]->stride;
          out_shape.shape.w = tg_out.shape.w;

          // clang-format off
          LOGD("[%zu] Out\n"
               " tg start address %" PRIu64 "\n"
               " tg shape %d %d %d %d\n"
               " tg stride %d %d %d\n"
               " tl shape %d %d %d %d\n"
               " tl stride %u %u %u %u\n",
               k,
               tg_out.start_address,
               tg_out.shape.n, tg_out.shape.c, tg_out.shape.h, tg_out.shape.w,
               tg_out.stride.n, tg_out.stride.c, tg_out.stride.h,
               tl_out[k]->shape.n, tl_out[k]->shape.c, tl_out[k]->shape.h, tl_out[k]->shape.w,
               tl_out[k]->stride.n, tl_out[k]->stride.c, tl_out[k]->stride.h, tl_out[k]->stride.w);
          // clang-format on

          cvk_tdma_l2g_tensor_copy_param_t p_copy_out;
          memset(&p_copy_out, 0, sizeof(cvk_tdma_l2g_tensor_copy_param_t));
          p_copy_out.src = &out_shape;
          p_copy_out.dst = &tg_out;
          cvk_ctx->ops->tdma_l2g_bf16_tensor_copy(cvk_ctx, &p_copy_out);

          // Change dest head addr
          bm_dest_addr_w[k] += 1 * out_slice_res.w.skip * bm_dest_info.fns_vec[k].getSize();
        }
      }
      // Change src/ dest head addr
      for (size_t k = 0; k < bm_src_info.addr_vec.size(); k++) {
        uint32_t jump_val = 0;
        if (in_slice_res_h_left_pixels != 0) {
          if (i == in_slice_res.h.turn - 1) {
            jump_val = in_slice_res_h_left_pixels;
          } else if (i == in_slice_res.h.turn - 2 && in_slice_res.h.left != 0) {
            jump_val = in_slice_res.h.left;
          } else {
            jump_val = in_slice_res.h.skip;
          }
        } else {
          if (i == in_slice_res.h.turn - 1) {
            jump_val = in_slice_res.h.left == 0 ? in_slice_res.h.slice : in_slice_res.h.left;
          } else {
            jump_val = in_slice_res.h.skip;
          }
        }
        bm_src_info.addr_vec[k] += 1 * input[k]->m_tg.stride.h * jump_val;
      }
      for (size_t k = 0; k < bm_dest_info.addr_vec.size(); k++) {
        uint32_t jump_val = 0;
        if (out_slice_res_h_left_pixels != 0) {
          if (i == out_slice_res.h.turn - 1) {
            jump_val = out_slice_res_h_left_pixels + vertical_pad_total;
          } else if (i == out_slice_res.h.turn - 2 && out_slice_res.h.left != 0) {
            jump_val = out_slice_res.h.left;
          } else {
            jump_val = out_slice_res.h.skip;
          }
        } else {
          if (i == out_slice_res.h.turn - 1) {
            jump_val = (out_slice_res.h.left == 0 ? out_slice_res.h.slice : out_slice_res.h.left) +
                       vertical_pad_total;
          } else {
            jump_val = out_slice_res.h.skip;
          }
        }
        bm_dest_info.addr_vec[k] += 1 * output[k]->m_tg.stride.h * jump_val;
      }
    }

    for (size_t k = 0; k < bm_src_info.addr_vec.size(); k++) {
      if (Is4096Workaound(input[k]->GetImgType())) {
        uint32_t img_height = input[k]->m_tg.shape.h;
        uint32_t img_stride = input[k]->m_tg.stride.h / getFmtSize(input[k]->m_tg.fmt);

        bm_src_info.addr_vec[k] =
            bm_src_addr_bk[k] + (Align64(img_stride * img_height, SCALAR_C_ALIGN) * (b + 1));
        LOGD("aligned bm_src_info.addr_vec[%lu]=0x%" PRIu64 "\n", k, bm_src_info.addr_vec[k]);
      }
    }

    for (size_t k = 0; k < bm_dest_info.addr_vec.size(); k++) {
      if (Is4096Workaound(output[k]->GetImgType())) {
        uint32_t img_height = output[k]->m_tg.shape.h;
        uint32_t img_stride = output[k]->m_tg.stride.h / getFmtSize(output[k]->m_tg.fmt);
        bm_dest_info.addr_vec[k] =
            bm_dst_addr_bk[k] + (Align64(img_stride * img_height, SCALAR_C_ALIGN) * (b + 1));
        LOGD("aligned bm_dest_info.addr_vec[%lu]=0x%" PRIu64 "\n", k, bm_dest_info.addr_vec[k]);
      }
    }
  }
  LOGD("In slice info:\n");
  LOGD("{ h_slice, h_turn, h_skip, h_left} = { %d, %d, %d, %d}\n", in_slice_res.h.slice,
       in_slice_res.h.turn, in_slice_res.h.skip, in_slice_res.h.left);
  LOGD("{ w_slice, w_turn, w_skip, w_left} = { %d, %d, %d, %d}\n", in_slice_res.w.slice,
       in_slice_res.w.turn, in_slice_res.w.skip, in_slice_res.w.left);
  LOGD("Out slice info:\n");
  LOGD("{ h_slice, h_turn, h_skip, h_left} = { %d, %d, %d, %d}\n", out_slice_res.h.slice,
       out_slice_res.h.turn, out_slice_res.h.skip, out_slice_res.h.left);
  LOGD("{ w_slice, w_turn, w_skip, w_left} = { %d, %d, %d, %d}\n", out_slice_res.w.slice,
       out_slice_res.w.turn, out_slice_res.w.skip, out_slice_res.w.left);

  // Dummy gaurd for buffer overflow
  ret |= checkIsBufferOverflow(input, output, bm_src_info, bm_dest_info, m_kernel_info.pad[0],
                               m_kernel_info.pad[2], true, true);
  beforeSubmit(rt_handle, cvk_ctx, input, output);

  if (ret == CVI_SUCCESS) {
    CVI_RT_Submit(cvk_ctx);
  }

  freeTLMems(cvk_ctx);
  postProcess(rt_handle);
  return CVI_SUCCESS;
}

int IveCore::runNoKernel(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                         const std::vector<CviImg *> &input, std::vector<CviImg *> &output,
                         bool enable_min_max) {
  // Only supports kernel size = 1. NoKernel means kernel size = 1. You still can use depthwise
  // conv + qdm as uint8_t div.
  if (m_kernel_info.size != 1) {
    return CVI_FAILURE;
  }
  uint32_t total_size = input[0]->m_tg.stride.n / getFmtSize(input[0]->m_tg.fmt);
  if (total_size % 16) {
    LOGE("Image size %u is not 16 aligned.\n", total_size);
    return CVI_FAILURE;
  }
  cvk_tl_shape_t tl_table_s;
  uint64_t table_sz = cvm_lut_tbl_bytesize(cvk_ctx, &tl_table_s, CVK_FMT_U8);
  m_table_per_channel_size = table_sz / m_chip_info.npu_num;  // 32 * 8 for bm1880v2
  // Calculating slice
  // Calculate fixed kernel size
  uint32_t kernel_sz = (m_kernel_info.nums_of_kernel * m_kernel_info.size * m_kernel_info.size +
                        MULTIPLIER_ONLY_PACKED_DATA_SIZE * m_kernel_info.use_multiplier);
  // Find max available mem for one tl.
  int64_t result = m_chip_info.lmem_size -
                   (int64_t)(kernel_sz + m_table_per_channel_size * m_slice_info.nums_of_table) -
                   (int64_t)m_slice_info.fix_lmem_size;
  size_t max_hxw =
      std::floor(result / ((m_slice_info.nums_of_tl - m_slice_info.ping_pong_share_tl) *
                               m_slice_info.ping_pong_size +
                           m_slice_info.ping_pong_share_tl));
  uint32_t npu_num = m_chip_info.npu_num;
  uint32_t idiv_n = (uint32_t)(total_size / npu_num);
  uint32_t div = max_hxw;
  LOGD("numoftable:%u,result:%ld,maxhw:%lu,idiv_n:%u", m_slice_info.nums_of_table, result, max_hxw,
       idiv_n);
  LOGD("kernel_size:%u,lmemesize:%u,npunum:%u\n", kernel_sz, m_chip_info.lmem_size,
       m_chip_info.npu_num);
  // Find div value that idiv % div == 0 while div < max_hxw
  while (idiv_n % div != 0) {
    uint32_t val = std::ceil(float(idiv_n) / div);
    div = std::floor(float(idiv_n) / val);
    LOGD("val:%u,div:%u", val, div);
  }
  // Make w 16 align.
  uint32_t div_16 = div / 16;
  div = div_16 * 16;
  // FIXME: We assumed that h never exceeds 1024.
  cvk_tg_shape_t shape = {1, npu_num, div_16, 16};
  size_t loop_turn = (div == 0) ? 0 : (total_size / (npu_num * div)) / m_slice_info.ping_pong_size;
  // Check if any pixel left.
  size_t left_pixels = total_size - ((loop_turn * (npu_num * div)) * m_slice_info.ping_pong_size);

  if (loop_turn == 0 && left_pixels != 0) {
    // FIXME: Duplicate code below.
    uint32_t div = npu_num;
    while (left_pixels % div != 0) {
      uint32_t val = std::ceil(float(left_pixels) / div);
      div = std::floor(float(left_pixels) / val);
    }
    uint32_t hw = left_pixels / div;
    // FIXME: Again, we assumed that h and w may not exceed 1024.
    uint32_t w_val = 1024;
    while (hw % w_val != 0) {
      uint32_t val = std::ceil(float(hw) / w_val);
      w_val = std::floor(float(hw) / val);
    }
    shape.n = 1;
    shape.c = div;
    shape.h = hw / w_val;
    shape.w = w_val;
  }
  LOGD("Total size %u\n", total_size);
  LOGD("turn %zu left %zu\n", loop_turn, left_pixels);
  LOGD("shape:%u %u %u %u\n", shape.n, shape.c, shape.h, shape.w);

  std::vector<cvk_tg_shape_t> s_in_vec, s_out_vec;
  for (size_t k = 0; k < input.size(); k++) {
    s_in_vec.push_back({shape.n, shape.c, shape.h, shape.w});
  }
  for (size_t k = 0; k < output.size(); k++) {
    s_out_vec.push_back({shape.n, shape.c, shape.h, shape.w});
  }
  // allocate tl shape and get input/ output indices.
  std::vector<uint32_t> tl_in_idx, tl_out_idx;
  runSetup(rt_handle, cvk_ctx, s_in_vec, s_out_vec, &tl_in_idx, &tl_out_idx, false);
  if (m_allocate_failed_) {
    printf("allocate ive local mem failed\n");
    freeTLMems(cvk_ctx);
    return CVI_FAILURE;
  }

  // Find and create input/ output fmt size pair.
  TLInfo tl_in_info, tl_out_info;
  getTLInfo(m_tl_vec, tl_in_idx, tl_out_idx, &tl_in_info, &tl_out_info);

  // Get device memory start offset
  BMAddrInfo bm_src_info, bm_dest_info;
  getBMAddrInfo(input, output, m_kernel_info.pad[0], m_kernel_info.pad[2], &bm_src_info,
                &bm_dest_info);
  // Get reshaped stride
  std::vector<cvk_tg_stride_t> input_stride_vec, output_stride_vec;
  for (size_t i = 0; i < bm_src_info.addr_vec.size(); i++) {
    input_stride_vec.push_back(cvk_ctx->ops->tg_default_stride(cvk_ctx, shape, input[i]->m_tg.fmt));
  }
  for (size_t i = 0; i < bm_dest_info.addr_vec.size(); i++) {
    output_stride_vec.push_back(
        cvk_ctx->ops->tg_default_stride(cvk_ctx, shape, output[i]->m_tg.fmt));
  }

  // Create tg block
  cvk_tg_t tg_in;
  tg_in.base_reg_index = 0;
  cvk_tg_t tg_out;
  tg_out.base_reg_index = 0;

  size_t jump_src = shape.n * shape.c * shape.h * shape.w;
  size_t jump_dst = jump_src;
  for (size_t i = 0; i < loop_turn; i++) {
    for (size_t pp = 0; pp < m_slice_info.ping_pong_size; pp++) {
      uint32_t tl_idx = tl_in_info.lmem_vec.size() / m_slice_info.ping_pong_size;
      uint32_t pp_skip = pp * tl_idx;
      for (size_t k = 0; k < tl_idx; k++) {
        tg_in.start_address = bm_src_info.addr_vec[k];
        tg_in.shape.n = tl_in_info.lmem_vec[k + pp_skip]->shape.n;
        tg_in.shape.c = tl_in_info.lmem_vec[k + pp_skip]->shape.c;
        tg_in.shape.h = tl_in_info.lmem_vec[k + pp_skip]->shape.h;
        tg_in.shape.w = tl_in_info.lmem_vec[k + pp_skip]->shape.w;
        tg_in.fmt = bm_src_info.fns_vec[k].getFmt();
        tg_in.stride = input_stride_vec[k];
        cvk_tdma_g2l_tensor_copy_param_t p_copy_in;
        memset(&p_copy_in, 0, sizeof(cvk_tdma_g2l_tensor_copy_param_t));
        p_copy_in.src = &tg_in;
        p_copy_in.dst = tl_in_info.lmem_vec[k + pp_skip];
        cvk_ctx->ops->tdma_g2l_bf16_tensor_copy(cvk_ctx, &p_copy_in);
        LOGD("tg_in physical addr %" PRIu64 "\n", tg_in.start_address);
        // Change src head addr
        bm_src_info.addr_vec[k] += 1 * jump_src * bm_src_info.fns_vec[k].getSize();
      }
    }

    for (size_t pp = 0; pp < m_slice_info.ping_pong_size; pp++) {
      operation(rt_handle, cvk_ctx, pp);
    }

    // tl2tg
    for (size_t pp = 0; pp < m_slice_info.ping_pong_size; pp++) {
      uint32_t tl_idx = tl_out_info.lmem_vec.size() / m_slice_info.ping_pong_size;
      uint32_t pp_skip = pp * tl_idx;
      for (size_t k = 0; k < tl_idx; k++) {
        tg_out.start_address = bm_dest_info.addr_vec[k];
        tg_out.shape.n = tl_out_info.lmem_vec[k + pp_skip]->shape.n;
        tg_out.shape.c = tl_out_info.lmem_vec[k + pp_skip]->shape.c;
        tg_out.shape.h = tl_out_info.lmem_vec[k + pp_skip]->shape.h;
        tg_out.shape.w = tl_out_info.lmem_vec[k + pp_skip]->shape.w;
        tg_out.fmt = bm_dest_info.fns_vec[k].getFmt();
        tg_out.stride = output_stride_vec[k];
        cvk_tdma_l2g_tensor_copy_param_t p_copy_out;
        memset(&p_copy_out, 0, sizeof(cvk_tdma_l2g_tensor_copy_param_t));
        p_copy_out.src = tl_out_info.lmem_vec[k + pp_skip];
        p_copy_out.dst = &tg_out;
        cvk_ctx->ops->tdma_l2g_bf16_tensor_copy(cvk_ctx, &p_copy_out);
        //clang-format off
        LOGD(
            "[%zu] Out\n"
            " tg start address %" PRIu64
            "\n"
            " tg shape %d %d %d %d\n"
            " tg stride %d %d %d\n"
            " tl shape %d %d %d %d\n"
            " tl stride %u %u %u %u\n",
            k, tg_out.start_address, tg_out.shape.n, tg_out.shape.c, tg_out.shape.h, tg_out.shape.w,
            tg_out.stride.n, tg_out.stride.c, tg_out.stride.h,
            tl_out_info.lmem_vec[k + pp_skip]->shape.n, tl_out_info.lmem_vec[k + pp_skip]->shape.c,
            tl_out_info.lmem_vec[k + pp_skip]->shape.h, tl_out_info.lmem_vec[k + pp_skip]->shape.w,
            tl_out_info.lmem_vec[k + pp_skip]->stride.n,
            tl_out_info.lmem_vec[k + pp_skip]->stride.c,
            tl_out_info.lmem_vec[k + pp_skip]->stride.h,
            tl_out_info.lmem_vec[k + pp_skip]->stride.w);
        // clang-format on
        // Change dest head addr
        bm_dest_info.addr_vec[k] += 1 * jump_dst * bm_dest_info.fns_vec[k].getSize();
      }
    }
  }
  if (left_pixels != 0) {
    cvk_tg_shape_t left_shape = {0, 0, 0, 0};
    uint32_t div = npu_num;
    while (left_pixels % div != 0) {
      uint32_t val = std::ceil(float(left_pixels) / div);
      div = std::floor(float(left_pixels) / val);
    }
    uint32_t hw = left_pixels / div;
    // FIXME: Again, we assumed that h and w may not exceed 1024.
    uint32_t w_val = 1024;
    while (hw % w_val != 0) {
      uint32_t val = std::ceil(float(hw) / w_val);
      w_val = std::floor(float(hw) / val);
    }
    left_shape.n = 1;
    left_shape.c = div;
    left_shape.h = hw / w_val;
    left_shape.w = w_val;
    LOGD("%u %u %u %u\n", left_shape.n, left_shape.c, left_shape.h, left_shape.w);

    for (size_t i = 0; i < input_stride_vec.size(); i++) {
      input_stride_vec[i] =
          cvk_ctx->ops->tg_default_stride(cvk_ctx, left_shape, input[0]->m_tg.fmt);
    }
    for (size_t i = 0; i < output_stride_vec.size(); i++) {
      output_stride_vec[i] =
          cvk_ctx->ops->tg_default_stride(cvk_ctx, left_shape, output[0]->m_tg.fmt);
    }
    // Category tl shapes
    for (size_t i = 0; i < m_tl_vec.size(); i++) {
      auto *lmem = m_tl_vec[i];
      bool skip = false;
      for (size_t k = 0; k < s_in_vec.size(); k++) {
        if (tgTLShapeCompare(lmem->shape, s_in_vec[k])) {
          lmem->shape.n = left_shape.n;
          lmem->shape.c = left_shape.c;
          lmem->shape.h = left_shape.h;
          lmem->shape.w = left_shape.w;
          lmem->stride = cvk_ctx->ops->tl_default_stride(cvk_ctx, lmem->shape, lmem->fmt, 1);
          skip = true;
          break;
        }
      }
      if (skip) {
        continue;
      }
      for (size_t k = 0; k < s_out_vec.size(); k++) {
        if (tgTLShapeCompare(lmem->shape, s_out_vec[k])) {
          lmem->shape.n = left_shape.n;
          lmem->shape.c = left_shape.c;
          lmem->shape.h = left_shape.h;
          lmem->shape.w = left_shape.w;
          lmem->stride = cvk_ctx->ops->tl_default_stride(cvk_ctx, lmem->shape, lmem->fmt, 1);
          break;
        }
      }
    }
    size_t jump_src = left_shape.n * left_shape.c * left_shape.h * left_shape.w;
    size_t jump_dst = jump_src;
    uint32_t tl_idx = tl_in_info.lmem_vec.size() / m_slice_info.ping_pong_size;
    for (size_t k = 0; k < tl_idx; k++) {
      tg_in.start_address = bm_src_info.addr_vec[k];
      tg_in.shape.n = tl_in_info.lmem_vec[k]->shape.n;
      tg_in.shape.c = tl_in_info.lmem_vec[k]->shape.c;
      tg_in.shape.h = tl_in_info.lmem_vec[k]->shape.h;
      tg_in.shape.w = tl_in_info.lmem_vec[k]->shape.w;
      tg_in.fmt = bm_src_info.fns_vec[k].getFmt();
      tg_in.stride = input_stride_vec[k];
      cvk_tdma_g2l_tensor_copy_param_t p_copy_in;
      memset(&p_copy_in, 0, sizeof(cvk_tdma_g2l_tensor_copy_param_t));
      p_copy_in.src = &tg_in;
      p_copy_in.dst = tl_in_info.lmem_vec[k];
      cvk_ctx->ops->tdma_g2l_bf16_tensor_copy(cvk_ctx, &p_copy_in);
      LOGD("tg_in physical addr %" PRIu64 "\n", tg_in.start_address);
      // Change src head addr
      bm_src_info.addr_vec[k] += 1 * jump_src * bm_src_info.fns_vec[k].getSize();
    }

    operation(rt_handle, cvk_ctx, 0);

    // tl2tg
    tl_idx = tl_out_info.lmem_vec.size() / m_slice_info.ping_pong_size;
    for (size_t k = 0; k < tl_idx; k++) {
      tg_out.start_address = bm_dest_info.addr_vec[k];
      tg_out.fmt = bm_dest_info.fns_vec[k].getFmt();
      tg_out.shape.n = tl_out_info.lmem_vec[k]->shape.n;
      tg_out.shape.c = tl_out_info.lmem_vec[k]->shape.c;
      tg_out.shape.h = tl_out_info.lmem_vec[k]->shape.h;
      tg_out.shape.w = tl_out_info.lmem_vec[k]->shape.w;
      tg_out.stride = output_stride_vec[k];
      cvk_tdma_l2g_tensor_copy_param_t p_copy_out;
      memset(&p_copy_out, 0, sizeof(cvk_tdma_l2g_tensor_copy_param_t));
      p_copy_out.src = tl_out_info.lmem_vec[k];
      p_copy_out.dst = &tg_out;
      cvk_ctx->ops->tdma_l2g_bf16_tensor_copy(cvk_ctx, &p_copy_out);
      LOGD("tg_out physical addr %" PRIu64 "\n", tg_out.start_address);
      // Change dest head addr
      bm_dest_info.addr_vec[k] += 1 * jump_dst * bm_dest_info.fns_vec[k].getSize();
    }
  }
  int ret = CVI_SUCCESS;
  ret |= checkIsBufferOverflow(input, output, bm_src_info, bm_dest_info, m_kernel_info.pad[0],
                               m_kernel_info.pad[2], true, false);

  beforeSubmit(rt_handle, cvk_ctx, input, output);

  if (ret == CVI_SUCCESS) {
    CVI_RT_Submit(cvk_ctx);
  }

  freeTLMems(cvk_ctx);
  postProcess(rt_handle);
  return CVI_SUCCESS;
}
