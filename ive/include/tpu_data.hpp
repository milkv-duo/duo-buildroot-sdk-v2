#pragma once
#ifdef CV180X
#include "linux/cvi_type.h"
#else
#include "cvi_type.h"
#endif
#include "ive_log.hpp"

#include <cvikernel/cvikernel.h>
#include <cvimath/cvimath_internal.h>
#include <cviruntime.h>
#include <cviruntime_context.h>
#include <string.h>
#include <iostream>
#include <vector>
#define CVI_IMG_VIDEO_FRM_MAGIC_NUM 123456
/**
 * @brief Convert cvk_fmt_t to actual data type size.
 *
 * @param fmt cvk_fmt_t defined in kernel
 * @return int Actual data type size
 */
static int getFmtSize(cvk_fmt_t fmt) {
  int fmt_size = 0;
  switch (fmt) {
    case CVK_FMT_I8:
    case CVK_FMT_U8:
      fmt_size = 1;
      break;
    case CVK_FMT_U16:
    case CVK_FMT_I16:
    case CVK_FMT_BF16:
      fmt_size = 2;
      break;
    case CVK_FMT_U32:
    case CVK_FMT_I32:
    case CVK_FMT_F32:
      fmt_size = 4;
      break;
    default:
      LOGE("Unsupported fmt type: %u.\n.", fmt);
  }
  return fmt_size;
}

/**
 * @brief Information of a sliced image for one dimension.
 *
 */
struct sliceUnit {
  uint32_t slice;  // rounded length
  uint32_t skip;   // like stride
  uint32_t turn;   // rounded loop
  uint32_t left;   // left
  uint32_t c_multiplier = 1;
};

/**
 * @brief Memory layout information for auto slicing function.
 *
 */
struct SliceInfo {
  cvk_fmt_t io_fmt = CVK_FMT_INVALID;
  uint32_t ping_pong_size = 1;
  uint32_t ping_pong_share_tl = 0;
  uint32_t nums_of_tl = 2;
  uint32_t fix_lmem_size = 0;
  uint32_t nums_of_table = 0;
};

struct SliceRes {
  sliceUnit h;
  sliceUnit w;
};

struct TLMemInfo {
  cvk_tl_shape_t shape;
  cvk_tl_stride_t stride;
};
struct TGMemInfo {
  cvk_tg_shape_t shape;
  cvk_tg_stride_t stride;
};
struct TensorSliceInfo {
  TLMemInfo tl_load;
  TLMemInfo tl_store;
  TGMemInfo tg_load;
  TGMemInfo tg_store;
};

/**
 * @brief Kernel information for IveCore base class.
 *
 */
struct kernelInfo {
  uint32_t nums_of_kernel = 0;
  bool use_multiplier = 0;
  uint32_t pad[4] = {0};  // L R T B
  uint32_t size = 1;
  uint32_t default_stride_x = 1;
  uint32_t default_stride_y = 1;
};

/**
 * @brief img_multiplier used by IveKernel.
 *
 */
struct img_multiplier {
  float f = 1.f;
  uint32_t base = 2147483647;
  int shift = 0;
};

// FIXME: RGB images stores the data in the order of RGB by default.
// But the enum here only present the data type not the order.
// Need to be fixed.
enum CVIIMGTYPE {
  CVI_RGB_PLANAR = 0,
  CVI_RGB_PACKED,
  CVI_RGBA_PLANAR,
  CVI_GRAY,
  CVI_YUV420SP,
  CVI_YUV420P,
  CVI_YUV422SP,
  CVI_YUV422P,
  CVI_SINGLE,
  CVI_MULTI
};

inline bool IsImgPlanar(CVIIMGTYPE img_type) {
  bool is_planar = true;
  switch (img_type) {
    case CVI_RGB_PACKED:
    case CVI_YUV420SP:
    case CVI_YUV422SP:
      is_planar = false;
      break;
    default:
      break;
  }
  return is_planar;
}

inline bool Is4096Workaound(CVIIMGTYPE img_type) {
#ifdef WORKAROUND_SCALAR_4096_ALIGN_BUG
  switch (img_type) {
    case CVI_YUV420P:
    case CVI_YUV422P:
    case CVI_RGB_PLANAR:
      return true;
      break;
    default:
      break;
  }
  return false;
#else
  return false;
#endif
}

inline uint32_t WidthAlign(const uint32_t width, const uint32_t align) {
  uint32_t stride = (uint32_t)(width / align) * align;
  if (stride < width) {
    stride += align;
  }
  return stride;
}

inline uint64_t Align64(const uint64_t length, const uint64_t align) {
  uint64_t stride = (uint64_t)(length / align) * align;
  if (stride < length) {
    stride += align;
  }
  return stride;
}

/**
 * @brief A wrapper for TPU device memory defined in runtime.
 *        This is a class originally designed for TPU, so the default setup for image is planar.
 *        The sub-image constructor only supports planar images. If you want to create packed
 *        image, use the constructor with CVIIMGTYPE variable input.
 *
 */
class CviImg {
 public:
  /**
   * @brief Default constructor
   *
   */
  CviImg();

  /**
   * @brief Construct a new CviImg object
   *
   * @param rt_handle bm context
   * @param img_c Image channel
   * @param img_h Image height
   * @param img_w Image width
   * @param fmt cvk_fmt_t type
   */
  CviImg(CVI_RT_HANDLE rt_handle, uint32_t img_c, uint32_t img_h, uint32_t img_w, cvk_fmt_t fmt,
         CviImg *cvi_img = nullptr);

  /**
   * @brief Construct a new CviImg object from an existing CviImg with given region.
   *
   * @param rt_handle bm context
   * @param img cvi_img
   */
  CviImg(CVI_RT_HANDLE rt_handle, const CviImg &img, uint32_t x1, uint32_t y1, uint32_t x2,
         uint32_t y2);

  /**
   * @brief Construct a new CviImg object with given strides.
   *
   * @param rt_handle bm context
   * @param img_h Image height.
   * @param img_w Image width.
   * @param strides Image strides, the channel of CviImg will be set to the size of strides.
   * @param heights Image heights.
   * @param img_type CviImg type enum.
   * @param fmt cvk_fmt_t type
   */
  CviImg(CVI_RT_HANDLE rt_handle, uint32_t img_h, uint32_t img_w, std::vector<uint32_t> strides,
         std::vector<uint32_t> heights, CVIIMGTYPE img_type, cvk_fmt_t fmt,
         CviImg *cvi_img = nullptr);

  /**
   * @brief Construct a new CviImg from VIDEO_FRAME_S
   *
   * @param img_h Image height.
   * @param img_w Image width.
   * @param strides Image strides, the channel of CviImg will be set to the size of strides.
   * @param heights Image heights.
   * @param u32_length Image channel offset.
   * @param vaddr Virtual addresses.
   * @param paddr Physical addresses.
   * @param img_type CviImg type enum.
   * @param fmt cvk_fmt_t type
   */
  CviImg(uint32_t img_h, uint32_t img_w, std::vector<uint32_t> strides,
         std::vector<uint32_t> heights, std::vector<uint32_t> u32_lengths, uint8_t *vaddr,
         uint64_t paddr, CVIIMGTYPE img_type, cvk_fmt_t fmt);

  int ReInit(uint32_t img_h, uint32_t img_w, std::vector<uint32_t> strides,
             std::vector<uint32_t> heights, std::vector<uint32_t> u32_lengths, uint8_t *vaddr,
             uint64_t paddr, CVIIMGTYPE img_type, cvk_fmt_t fmt);

  /**
   * @brief Init CviImg if default constructor is used.
   *
   * @param rt_handle bm context
   * @param img_c Image channel
   * @param img_h Image height
   * @param img_w Image width
   * @param fmt cvk_fmt_t type
   * @return int Return 0 if success
   */
  int Init(CVI_RT_HANDLE rt_handle, uint32_t img_c, uint32_t img_h, uint32_t img_w, cvk_fmt_t fmt,
           CviImg *img_ptr);

  /**
   * @brief Check if device memory is allocated.
   *
   * @return true Device memory is allocated
   * @return false Deive memory is not allocated
   */
  const bool IsInit();

  /**
   * @brief Get virtual memory address from device memory.
   *
   * @return const uint8_t* Address pointer
   */
  uint8_t *GetVAddr();

  /**
   * @brief Get physical offset from device memory.
   *
   * @return const uint64_t Address offset
   */
  uint64_t GetPAddr() const;

  /**
   * @brief Get the channel of the image.
   *
   * @return const uint32_t image channel.
   */
  const uint32_t GetImgChannel() const;

  /**
   * @brief Get the width of the image.
   *
   * @return const uint32_t image width.
   */
  const uint32_t GetImgWidth() const;

  /**
   * @brief Get the height of the image.
   *
   * @return const uint32_t image height.
   */
  const uint32_t GetImgHeight() const;

  /**
   * @brief Get the channel offset of the image.
   *
   * @return const std::vector<uint32_t> image channel offsets.
   */
  const std::vector<uint32_t> GetImgCOffsets() const;

  /**
   * @brief Get the strides of the image.
   *
   * @return const std::vector<uint32_t> image strides.
   */
  const std::vector<uint32_t> GetImgStrides() const;

  /**
   * @brief Get the heights of the image.
   *
   * @return const std::vector<uint32_t> image heights.
   */
  const std::vector<uint32_t> GetImgHeights() const;

  /**
   * @brief Get the size of the image in bytes.
   *
   * @return const uint64_t image size
   */
  const uint64_t GetImgSize() const;

  /**
   * @brief Get the Img's CVIIMGTYPE.
   *
   * @return const CVIIMGTYPE image type.
   */
  const CVIIMGTYPE GetImgType() const;

  /**
   * @brief Tells if this image instance is a sub-image of an image.
   *
   * @return true Is sub-image.
   * @return false Not a sub-image.
   */
  const bool IsSubImg() const;

  /**
   * @brief Tells if the image's strides differ bwtween channels.
   *
   * @return true All strides are the same.
   * @return false Not the same.
   */
  const bool IsStideCEQ() const;

  /**
   * @brief Tells if the image bwtween channels is planar.
   *
   * @return true Is planar.
   * @return false Not planar.
   */
  const bool IsPlanar() const;
  /**
   * @brief Release allocated device memory.
   *
   * @param rt_handle bm context
   * @return int return 0 if success
   */
  int Free(CVI_RT_HANDLE rt_handle);

  /**
   * @brief Flush cache data to RAM.
   *
   * @param rt_handle bm context.
   * @return int return 0 if success.
   */
  int Flush(CVI_RT_HANDLE rt_handle) {
    if (m_rtmem != NULL) {
      return CVI_RT_MemFlush(rt_handle, m_rtmem) == CVI_RC_SUCCESS ? CVI_SUCCESS : CVI_FAILURE;
    } else {
      return CVI_SUCCESS;
    }
  }

  /**
   * @brief Update cache data from RAM.
   *
   * @param rt_handle bm context.
   * @return int return 0 if success.
   */
  int Invld(CVI_RT_HANDLE rt_handle) {
    if (m_rtmem != NULL) {
      return CVI_RT_MemInvld(rt_handle, m_rtmem) == CVI_RC_SUCCESS ? CVI_SUCCESS : CVI_FAILURE;
    } else {
      return CVI_SUCCESS;
    }
  }
  bool IsNullMem() { return m_rtmem == NULL; }
  int GetMagicNum() { return m_magic_num; }

  const uint64_t GetAddrOffset(int plane, uint64_t cur_addr);
  cvk_tg_t m_tg;

 private:
  /**
   * @brief Setup image structure information. This mode does not support padding and packed mode.
   *
   * @param img_c Input image channel.
   * @param img_h Input image height.
   * @param img_w Input image width.
   * @param fmt fmt type.
   */
  inline void SetupImageInfo(uint32_t img_c, uint32_t img_h, uint32_t img_w, cvk_fmt_t fmt);

  /**
   * @brief Allocate device memory.
   *
   * @param rt_handle bm context
   * @return int Return 0 if success
   */
  int AllocateDevice(CVI_RT_HANDLE rt_handle);

  uint32_t m_channel = 0;
  uint32_t m_width = 0;
  uint32_t m_height = 0;
  std::vector<uint32_t> m_coffsets;
  std::vector<uint32_t> m_strides;
  std::vector<uint32_t> m_heights;
  cvk_fmt_t m_fmt = CVK_FMT_U8;
  uint64_t m_size = 0;  // Total size of memory

  CVI_RT_MEM m_rtmem = NULL;   // Set to NULL if not initialized
  uint64_t m_paddr = -1;       // Set to maximum of uint64_t if not initaulized
  uint8_t *m_vaddr = nullptr;  // Set to nullptr if not initualized

  /**
   * These are variables used for different framework.
   * In order to know the format stored in CvImage from CSC, CVIIMAGETYPE is used. m_is_planar,
   * m_is_stride_ceq variables are used to check if TPU supports. As for m_is_sub_img, this
   * variable is used to check if ive should use channelExtension or not.
   */
  CVIIMGTYPE m_img_type;
  bool m_is_planar = true;      // Is image planar.
  bool m_is_sub_img = false;    // Is sub-image flag.
  bool m_is_stride_ceq = true;  // Are all the strides in every channel equal.
  int m_magic_num;
};

/**
 * @brief Ive kernel structure, for filter operations.
 *
 */
struct IveKernel {
  CviImg img;
  img_multiplier multiplier;
};

class FmtnSize {
 public:
  FmtnSize() {}
  FmtnSize(cvk_fmt_t fmt) { setFmt(fmt); }
  void setFmt(cvk_fmt_t fmt) {
    m_fmt = fmt;
    m_fmt_size = getFmtSize(m_fmt);
  }

  const cvk_fmt_t getFmt() const { return m_fmt; }
  const uint32_t getSize() const { return m_fmt_size; }

 private:
  cvk_fmt_t m_fmt = CVK_FMT_U8;
  uint32_t m_fmt_size = 1;
};

struct BMAddrInfo {
  std::vector<uint64_t> addr_vec;
  std::vector<FmtnSize> fns_vec;
};

struct TLInfo {
  std::vector<cvk_tl_t *> lmem_vec;
  std::vector<FmtnSize> fns_vec;
};
