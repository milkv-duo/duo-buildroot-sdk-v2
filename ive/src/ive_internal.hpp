#pragma once
#include "version.hpp"

#include "ive.h"
#include "ive_experimental.h"

#include "tracer/tracer.h"

#include "kernel_generator.hpp"
#include "table_manager.hpp"
#include "tpu_data.hpp"

#include "ive_draw.h"
#include "tpu/tpu_add.hpp"
#include "tpu/tpu_alpha_blend.hpp"
#include "tpu/tpu_alpha_blend_pixel.hpp"
#include "tpu/tpu_and.hpp"
#include "tpu/tpu_blend_pixel_ab.hpp"
#include "tpu/tpu_block.hpp"
#include "tpu/tpu_cmp.hpp"
#include "tpu/tpu_cmp_sat.hpp"
#include "tpu/tpu_convert_scale_abs.hpp"
#include "tpu/tpu_copy.hpp"
#include "tpu/tpu_downsample.hpp"
#include "tpu/tpu_fill.hpp"
#include "tpu/tpu_filter.hpp"
#include "tpu/tpu_magandang.hpp"
#include "tpu/tpu_mask.hpp"
#include "tpu/tpu_morph.hpp"
#include "tpu/tpu_mulsum.hpp"
#include "tpu/tpu_normalize.hpp"
#include "tpu/tpu_or.hpp"
#include "tpu/tpu_sad.hpp"
#include "tpu/tpu_sigmoid.hpp"
#include "tpu/tpu_sobel.hpp"
#include "tpu/tpu_sub.hpp"
#include "tpu/tpu_table.hpp"
#include "tpu/tpu_threshold.hpp"
#include "tpu/tpu_xor.hpp"

#include "2ddraw/tpu_draw.hpp"

#include <stdarg.h>
#include <cmath>
#include <deque>
#include <limits>
#include <regex>

/**
 * @brief stringfy #define
 *
 * STRFY stringfy the variable itself.
 * VSTRFY stringfy the value saved in the variable.
 *
 */
#define STRFY(s) #s
#define VSTRFY(s) STRFY(s)

extern const char *cviIveImgEnTypeStr[];

// We use initializer_list to make sure the variadic input are correct.
namespace detail {
inline bool IsValidImageType(IVE_IMAGE_S *pstImg, std::string pstImgStr,
                             std::initializer_list<const IVE_IMAGE_TYPE_E> enType) {
  if (pstImg == NULL) {
    LOGE("%s cannot be NULL.\n", pstImgStr.c_str());
    return false;
  }
  for (auto it : enType) {
    if (pstImg->enType == it) {
      return true;
    }
  }

  std::string msg = pstImgStr + " only supports ";
  for (auto it : enType) {
    msg += (std::string(cviIveImgEnTypeStr[it]) + std::string(" "));
  }
  LOGE("%s.\n", msg.c_str());
  return false;
}
}  // namespace detail

CVI_S32 CVI_IVE_ImageInit(IVE_IMAGE_S *pstSrc);

// The variadic function.
template <typename... Types>
inline bool IsValidImageType(IVE_IMAGE_S *pstImg, std::string pstImgStr, const Types... enType) {
  bool ret = detail::IsValidImageType(pstImg, pstImgStr, {enType...});
  if (ret) {
    if (CVI_IVE_ImageInit(pstImg) != CVI_SUCCESS) {
      LOGE("%s cannot be inited.\n", pstImgStr.c_str());
      return false;
    }
  }
  return ret;
}
inline bool IsSignedImageType(IVE_IMAGE_S *pstImg) {
  if (pstImg->enType == IVE_IMAGE_TYPE_S8C1 || pstImg->enType == IVE_IMAGE_TYPE_S8C3_PACKAGE ||
      pstImg->enType == IVE_IMAGE_TYPE_S8C3_PLANAR)
    return true;
  else
    return false;
}
struct TPU_HANDLE {
  TblMgr t_tblmgr;
  IveTPUAdd t_add;
  IveTPUAddSigned t_add_signed;
  IveTPUAddBF16 t_add_bf16;
  IveTPUAnd t_and;
  IveTPUBlock t_block;
  IveTPUBlockBF16 t_block_bf16;
  IveTPUConstFill t_const_fill;
  IveTPUCopyInterval t_copy_int;
  IveTPUDownSample t_downsample;
  IveTPUErode t_erode;
  IveTPUFilter t_filter;
  IveTPUFilterBF16 t_filter_bf16;
  IveTPUMagAndAng t_magandang;
  IveTPUMask t_mask;
  IveTPUMax t_max;
  IveTPUMulSum t_mulsum;
  IveTPUMin t_min;
  IveTPUNormalize t_norm;
  IveTPUOr t_or;
  IveTPUSAD t_sad;
  IveTPUSigmoid t_sig;
  IveTPUSobelGradOnly t_sobel_gradonly;
  IveTPUSobel t_sobel;
  IveTPUSubAbs t_sub_abs;
  IveTPUSub t_sub;
  IveTPUTbl t_tbl;
  IveTPUTbl512 t_tbl512;
  IveTPUThreshold t_thresh;
  IveTPUThresholdHighLow t_thresh_hl;
  IveTPUThresholdSlope t_thresh_s;
  IveTPUXOr t_xor;
  IveTPUBlend t_blend;
  IveTPUBlendPixel t_blend_pixel;
  IveTPUBlendPixelAB t_blend_pixel_ab;
  IveTPUConvertScaleAbs t_convert_scale_abs;
  IveTPUCmpSat t_cmp_sat;
};

struct IVE_HANDLE_CTX {
  CVI_RT_HANDLE rt_handle = NULL;
  cvk_context_t *cvk_ctx = NULL;
  TPU_HANDLE t_h;
  // VIP
};
