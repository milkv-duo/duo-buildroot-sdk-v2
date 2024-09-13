#include "tpu_data.hpp"
#include "ive_log.hpp"

#ifdef WORKAROUND_SCALAR_4096_ALIGN_BUG
#define SCALAR_C_ALIGN 0x1000
#else
#define SCALAR_C_ALIGN 0x1
#endif

CviImg::CviImg() {}
CviImg::CviImg(CVI_RT_HANDLE rt_handle, uint32_t img_c, uint32_t img_h, uint32_t img_w,
               cvk_fmt_t fmt, CviImg *cvi_img) {
  Init(rt_handle, img_c, img_h, img_w, fmt, cvi_img);
}

CviImg::CviImg(CVI_RT_HANDLE rt_handle, const CviImg &img, uint32_t x1, uint32_t y1, uint32_t x2,
               uint32_t y2) {
  if (x1 > x2) {
    uint32_t tmp = x1;
    x1 = x2;
    x2 = tmp;
  }
  if (y1 > y2) {
    uint32_t tmp = y1;
    y1 = y2;
    y2 = tmp;
  }
  uint32_t x1_new = x1 > img.m_tg.shape.w ? img.m_tg.shape.w : x1;
  uint32_t y1_new = y1 > img.m_tg.shape.h ? img.m_tg.shape.h : y1;
  uint32_t x2_new = x2 > img.m_tg.shape.w ? img.m_tg.shape.w : x2;
  uint32_t y2_new = y2 > img.m_tg.shape.h ? img.m_tg.shape.h : y2;
  uint32_t new_width = x2_new - x1_new;
  uint32_t new_height = y2_new - y1_new;
  if (new_width == 0 || new_height == 0) {
    LOGE("Error! width %u or height %u cannot be 0.\n", new_width, new_height);
    return;
  }

  // Update subimage shape
  this->m_rtmem = img.m_rtmem;
  this->m_size = img.m_size;
  this->m_fmt = img.m_fmt;
  this->m_channel = img.m_channel;
  this->m_width = new_width;
  this->m_height = new_height;
  this->m_strides = img.m_strides;
  this->m_heights = img.m_heights;
  this->m_img_type = img.m_img_type;
  this->m_is_planar = img.m_is_planar;
  this->m_is_stride_ceq = img.m_is_stride_ceq;
  this->m_is_sub_img = true;
  if (img.m_is_stride_ceq && img.m_is_planar) {
    this->m_tg = img.m_tg;
    this->m_tg.shape.h = new_height;
    this->m_tg.shape.w = new_width;
    this->m_tg.stride.h = this->m_strides[0];
    this->m_tg.stride.c = new_height * this->m_strides[0];
    this->m_tg.stride.n = m_tg.shape.c * this->m_tg.stride.c;

    uint32_t start_offset = y1_new * img.m_tg.stride.h + x1_new * getFmtSize(img.m_tg.fmt);
    this->m_tg.start_address = img.m_tg.start_address + start_offset;
    this->m_paddr = img.m_paddr + start_offset;
    this->m_vaddr = img.m_vaddr + start_offset;
    this->m_coffsets = img.m_coffsets;
  } else {
    if (img.m_img_type == CVI_YUV420P) {
      this->m_tg = img.m_tg;
      this->m_tg.shape.h = new_height;
      this->m_tg.shape.w = new_width;
      uint32_t start_offset = y1_new * img.m_strides[0] + x1_new * getFmtSize(img.m_tg.fmt);
      this->m_tg.start_address = img.m_tg.start_address + start_offset;
      this->m_paddr = img.m_paddr + start_offset;
      this->m_vaddr = img.m_vaddr + start_offset;
      this->m_coffsets.push_back(img.m_coffsets[0]);
      for (size_t i = 1; i < img.m_coffsets.size(); i++) {
        uint32_t start_offset2 =
            y1_new / 2 * img.m_strides[i] + x1_new / 2 * getFmtSize(img.m_tg.fmt);
        this->m_coffsets.push_back(img.m_coffsets[i] - start_offset + start_offset2);
      }
    } else if (img.m_img_type == CVI_YUV422P) {
      this->m_tg = img.m_tg;
      this->m_tg.shape.h = new_height;
      this->m_tg.shape.w = new_width;
      uint32_t start_offset = y1_new * img.m_strides[0] + x1_new * getFmtSize(img.m_tg.fmt);
      this->m_tg.start_address = img.m_tg.start_address + start_offset;
      this->m_paddr = img.m_paddr + start_offset;
      this->m_vaddr = img.m_vaddr + start_offset;
      this->m_coffsets.push_back(img.m_coffsets[0]);
      for (size_t i = 1; i < img.m_coffsets.size(); i++) {
        uint32_t start_offset2 = y1_new * img.m_strides[i] + x1_new / 2 * getFmtSize(img.m_tg.fmt);
        this->m_coffsets.push_back(img.m_coffsets[i] - start_offset + start_offset2);
      }
    } else if (img.m_img_type == CVI_YUV420SP) {
      this->m_tg = img.m_tg;
      this->m_tg.shape.h = new_height;
      this->m_tg.shape.w = new_width;
      uint32_t start_offset = y1_new * img.m_strides[0] + x1_new * getFmtSize(img.m_tg.fmt);
      this->m_tg.start_address = img.m_tg.start_address + start_offset;
      this->m_paddr = img.m_paddr + start_offset;
      this->m_vaddr = img.m_vaddr + start_offset;
      this->m_coffsets.push_back(img.m_coffsets[0]);
      for (size_t i = 1; i < img.m_coffsets.size(); i++) {
        uint32_t start_offset2 = y1_new / 2 * img.m_strides[i] + x1_new * getFmtSize(img.m_tg.fmt);
        this->m_coffsets.push_back(img.m_coffsets[i] - start_offset + start_offset2);
      }
    } else {
      LOGE("Unsupported sub-image type.\n");
    }
  }
}

CviImg::CviImg(CVI_RT_HANDLE rt_handle, uint32_t img_h, uint32_t img_w,
               std::vector<uint32_t> strides, std::vector<uint32_t> heights, CVIIMGTYPE img_type,
               cvk_fmt_t fmt, CviImg *cvi_img) {
  if (strides.size() == 0) {
    LOGE("Strides are empty.\n");
    return;
  }
  if (strides.size() != heights.size()) {
    LOGE("Strides size and heights size must be the same.\n");
    return;
  }
  this->m_fmt = fmt;
  this->m_channel = strides.size();
  this->m_width = img_w;
  this->m_height = img_h;
  this->m_strides = strides;
  this->m_heights = heights;
  this->m_img_type = img_type;
  this->m_is_planar = IsImgPlanar(this->m_img_type);
  this->m_coffsets.clear();
  this->m_size = 0;
  this->m_coffsets.push_back(this->m_size);
  if (Is4096Workaound(img_type)) {
    for (size_t i = 0; i < strides.size(); i++) {
      this->m_size += Align64(strides[i] * heights[i], SCALAR_C_ALIGN);
      if (strides[i] != strides[0]) {
        m_is_stride_ceq = false;
      }
      this->m_coffsets.push_back(this->m_size);
    }
  } else {
    for (size_t i = 0; i < strides.size(); i++) {
      this->m_size += strides[i] * heights[i];
      if (strides[i] != strides[0]) {
        m_is_stride_ceq = false;
      }
      this->m_coffsets.push_back(this->m_size);
    }
  }

  if (cvi_img != nullptr) {
    if (this->m_size < cvi_img->m_size) {
      this->m_rtmem = cvi_img->m_rtmem;
    }
  }
  AllocateDevice(rt_handle);
  if (Is4096Workaound(img_type)) {
    uint64_t new_paddr = Align64(this->m_paddr, SCALAR_C_ALIGN);
    uint64_t offset = new_paddr - this->m_paddr;
    this->m_paddr = new_paddr;
    this->m_vaddr += offset;
    this->m_tg.start_address = this->m_paddr;
    if (m_is_stride_ceq) {
      this->m_tg.stride.c = Align64(m_tg.shape.h * this->m_tg.stride.h, SCALAR_C_ALIGN);
      this->m_tg.stride.n = m_tg.shape.c * this->m_tg.stride.c;
    }
  }
}
int CviImg::ReInit(uint32_t img_h, uint32_t img_w, std::vector<uint32_t> strides,
                   std::vector<uint32_t> heights, std::vector<uint32_t> u32_lengths, uint8_t *vaddr,
                   uint64_t paddr, CVIIMGTYPE img_type, cvk_fmt_t fmt) {
  if (strides.size() == 0) {
    LOGE("Strides are empty.\n");
    return CVI_FAILURE;
  }
  if (strides.size() != heights.size()) {
    LOGE("Strides size and heights size must be the same.\n");
    return CVI_FAILURE;
  }
  this->m_fmt = fmt;
  this->m_channel = strides.size();
  this->m_width = img_w;
  this->m_height = img_h;
  this->m_strides = strides;
  this->m_heights = heights;
  this->m_img_type = img_type;
  this->m_is_planar = IsImgPlanar(this->m_img_type);
  this->m_coffsets.clear();
  this->m_size = 0;
  this->m_coffsets.push_back(this->m_size);
  m_is_stride_ceq = true;
  if (Is4096Workaound(img_type)) {
    for (size_t i = 0; i < strides.size(); i++) {
      this->m_size += Align64(u32_lengths[i], SCALAR_C_ALIGN);
      if (strides[i] != strides[0]) {
        m_is_stride_ceq = false;
      }
      this->m_coffsets.push_back(this->m_size);
    }
  } else {
    for (size_t i = 0; i < strides.size(); i++) {
      this->m_size += u32_lengths[i];
      if (strides[i] != strides[0]) {
        m_is_stride_ceq = false;
      }
      this->m_coffsets.push_back(this->m_size);
    }
  }

  this->m_paddr = paddr;
  this->m_vaddr = vaddr;
  this->m_tg.start_address = this->m_paddr;
  this->m_tg.base_reg_index = 0;
  this->m_tg.fmt = this->m_fmt;
  // Note: If m_is_stride_ceq == false, m_tg only presents the first layer info.
  // TODO: The old tpu_data structure needs to be redesigned to support yuv images.
  uint32_t tg_channel = m_is_stride_ceq ? this->m_channel : 1;
  if (!this->m_is_planar && this->m_channel == 1) {
    if (this->m_strides[0] * this->m_height != u32_lengths[0]) {
      LOGE("Length shape != stride * height\n");
    }
    this->m_tg.shape = {1, tg_channel, this->m_height, this->m_strides[0]};
  } else {
    this->m_tg.shape = {1, tg_channel, this->m_height, this->m_width};
  }
  this->m_tg.stride.h = this->m_strides[0];
  this->m_tg.stride.c = u32_lengths[0];
  this->m_tg.stride.n = m_tg.shape.c * this->m_tg.stride.c;
  return CVI_SUCCESS;
}
CviImg::CviImg(uint32_t img_h, uint32_t img_w, std::vector<uint32_t> strides,
               std::vector<uint32_t> heights, std::vector<uint32_t> u32_lengths, uint8_t *vaddr,
               uint64_t paddr, CVIIMGTYPE img_type, cvk_fmt_t fmt) {
  m_magic_num = CVI_IMG_VIDEO_FRM_MAGIC_NUM;
  if (CVI_SUCCESS !=
      ReInit(img_h, img_w, strides, heights, u32_lengths, vaddr, paddr, img_type, fmt)) {
    printf("CviImg init failed\n");
    assert(0);
  }
}

void CviImg::SetupImageInfo(uint32_t img_c, uint32_t img_h, uint32_t img_w, cvk_fmt_t fmt) {
  this->m_fmt = fmt;
  this->m_channel = img_c;
  this->m_width = img_w;
  this->m_height = img_h;
  uint32_t w_stride = img_w * getFmtSize(this->m_fmt);
  this->m_strides.resize(this->m_channel, w_stride);
  this->m_heights.resize(this->m_channel, this->m_height);
  this->m_coffsets.clear();
  this->m_size = 0;
  for (size_t i = 0; i < this->m_strides.size(); i++) {
    this->m_coffsets.push_back(this->m_size);
    this->m_size += this->m_strides[i] * this->m_heights[i];
  }
  if (this->m_fmt == CVK_FMT_U8) {
    if (this->m_channel == 1) {
      this->m_img_type = CVI_GRAY;
    } else if (this->m_channel == 3) {
      this->m_img_type = CVI_RGB_PLANAR;
    } else if (this->m_channel == 4) {
      this->m_img_type = CVI_RGBA_PLANAR;
    } else {
      this->m_img_type = CVI_MULTI;
    }
  } else {
    this->m_img_type = (this->m_channel == 1) ? CVI_SINGLE : CVI_MULTI;
  }
  this->m_is_planar = true;
}

int CviImg::Init(CVI_RT_HANDLE rt_handle, uint32_t img_c, uint32_t img_h, uint32_t img_w,
                 cvk_fmt_t fmt, CviImg *cvi_img) {
  SetupImageInfo(img_c, img_h, img_w, fmt);
  if (cvi_img != nullptr) {
    if (this->m_size < cvi_img->m_size) {
      this->m_rtmem = cvi_img->m_rtmem;
    }
  }
  return AllocateDevice(rt_handle);
}

const bool CviImg::IsInit() { return this->m_vaddr == nullptr ? false : true; }

uint8_t *CviImg::GetVAddr() { return m_vaddr; }

uint64_t CviImg::GetPAddr() const { return m_paddr; }

const uint32_t CviImg::GetImgChannel() const { return m_channel; }

const uint32_t CviImg::GetImgWidth() const { return m_width; }

const uint32_t CviImg::GetImgHeight() const { return m_height; }

const std::vector<uint32_t> CviImg::GetImgCOffsets() const { return m_coffsets; }

const std::vector<uint32_t> CviImg::GetImgStrides() const { return m_strides; }

const std::vector<uint32_t> CviImg::GetImgHeights() const { return m_heights; }

const uint64_t CviImg::GetImgSize() const { return m_size; }

const CVIIMGTYPE CviImg::GetImgType() const { return m_img_type; }

const bool CviImg::IsSubImg() const { return m_is_sub_img; }

const bool CviImg::IsStideCEQ() const { return m_is_stride_ceq; }

const bool CviImg::IsPlanar() const { return m_is_planar; }

const uint64_t CviImg::GetAddrOffset(int plane, uint64_t cur_addr) {
  uint64_t plane_addr = this->m_paddr + this->m_coffsets[plane];
  uint64_t offset = cur_addr - plane_addr;
  return offset;
}
int CviImg::AllocateDevice(CVI_RT_HANDLE rt_handle) {
  if (this->m_rtmem == NULL) {
    this->m_rtmem = CVI_RT_MemAlloc(rt_handle, this->m_size);
  }
  m_vaddr = (uint8_t *)CVI_RT_MemGetVAddr(this->m_rtmem);
  m_paddr = CVI_RT_MemGetPAddr(this->m_rtmem);
  this->m_tg.start_address = m_paddr;
  this->m_tg.base_reg_index = 0;
  this->m_tg.fmt = this->m_fmt;
  uint32_t tg_channel = m_is_stride_ceq ? this->m_channel : 1;
  if (!this->m_is_planar && this->m_channel == 1) {
    this->m_tg.shape = {1, 1, this->m_height, this->m_strides[0]};
  } else {
    this->m_tg.shape = {1, tg_channel, this->m_height, this->m_width};
  }
  this->m_tg.stride.h = this->m_strides[0];
  this->m_tg.stride.c = m_tg.shape.h * this->m_tg.stride.h;
  this->m_tg.stride.n = m_tg.shape.c * this->m_tg.stride.c;
  return CVI_SUCCESS;
}

int CviImg::Free(CVI_RT_HANDLE rt_handle) {
  if (this->m_rtmem != NULL) {
    if (!m_is_sub_img) {
      CVI_RT_MemFree(rt_handle, this->m_rtmem);
    }
    this->m_rtmem = NULL;
  }
  return CVI_SUCCESS;
}
