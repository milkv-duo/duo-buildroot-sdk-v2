#include "impl_ive.hpp"
#include <cvi_ive.h>
#include <string.h>
#include <string>
#include "cvi_tdl_log.hpp"
#include "cvi_comm_ive.h"

namespace ive {

class HWIVEImage : public IVEImageImpl {
 public:
  HWIVEImage();
  virtual ~HWIVEImage() = default;
  HWIVEImage(const HWIVEImage &other) = delete;
  HWIVEImage &operator=(const HWIVEImage &other) = delete;

  virtual void *getHandle() override;
  virtual CVI_S32 toFrame(VIDEO_FRAME_INFO_S *frame, bool invertPackage = false) override;
  virtual CVI_S32 fromFrame(VIDEO_FRAME_INFO_S *frame) override;
  virtual CVI_S32 bufFlush(IVEImpl *ive_instance) override;
  virtual CVI_S32 bufRequest(IVEImpl *ive_instance) override;
  virtual CVI_S32 create(IVEImpl *ive_instance, ImageType enType, CVI_U16 u16Width,
                         CVI_U16 u16Height, bool cached) override;
  virtual CVI_S32 create(IVEImpl *ive_instance, ImageType enType, CVI_U16 u16Width,
                         CVI_U16 u16Height, IVEImageImpl *buf, bool cached) override;
  virtual CVI_S32 create(IVEImpl *ive_instance) override;
  virtual CVI_S32 free() override;
  static IVE_IMAGE_TYPE_E convert(ImageType type);
  static ImageType convert(IVE_IMAGE_TYPE_E type);
  virtual CVI_S32 write(const std::string &fname) override;
  virtual CVI_U32 getHeight() override;
  virtual CVI_U32 getWidth() override;
  virtual std::vector<CVI_U32> getStride() override;
  virtual std::vector<CVI_U8 *> getVAddr() override;
  virtual std::vector<CVI_U64> getPAddr() override;
  virtual ImageType getType() override;

 private:
  IVE_IMAGE_S ive_image;
  IVE_HANDLE m_handle;
};

class HWIVE : public IVEImpl {
 public:
  HWIVE();
  virtual ~HWIVE() = default;
  HWIVE(const HWIVE &other) = delete;
  HWIVE &operator=(const HWIVE &other) = delete;

  virtual CVI_S32 init() override;
  virtual CVI_S32 destroy() override;

  virtual CVI_S32 fillConst(IVEImageImpl *pSrc, float value) override;
  virtual CVI_S32 dma(IVEImageImpl *pSrc, IVEImageImpl *pDst, DMAMode mode = DIRECT_COPY,
                      CVI_U64 u64Val = 0, CVI_U8 u8HorSegSize = 0, CVI_U8 u8ElemSize = 0,
                      CVI_U8 u8VerSegRows = 0) override;
  virtual CVI_S32 sub(IVEImageImpl *pSrc1, IVEImageImpl *pSrc2, IVEImageImpl *pDst,
                      SubMode mode = ABS) override;
  virtual CVI_U32 getWidthAlign() override;
  virtual CVI_S32 roi(IVEImageImpl *pSrc, IVEImageImpl *pDst, uint32_t x1, uint32_t x2, uint32_t y1,
                      uint32_t y2) override;
  virtual CVI_S32 andImage(IVEImageImpl *pSrc1, IVEImageImpl *pSrc2, IVEImageImpl *pDst) override;
  virtual CVI_S32 orImage(IVEImageImpl *pSrc1, IVEImageImpl *pSrc2, IVEImageImpl *pDst) override;
  virtual CVI_S32 erode(IVEImageImpl *pSrc1, IVEImageImpl *pDst,
                        const std::vector<CVI_S32> &mask) override;
  virtual CVI_S32 dilate(IVEImageImpl *pSrc1, IVEImageImpl *pDst,
                         const std::vector<CVI_S32> &mask) override;
  virtual CVI_S32 add(IVEImageImpl *pSrc1, IVEImageImpl *pSrc2, IVEImageImpl *pDst,
                      float alpha = 1.0, float beta = 1.0) override;
  virtual CVI_S32 add(IVEImageImpl *pSrc1, IVEImageImpl *pSrc2, IVEImageImpl *pDst,
                      unsigned short alpha = std::numeric_limits<unsigned short>::max(),
                      unsigned short beta = std::numeric_limits<unsigned short>::max()) override;
  virtual CVI_S32 thresh(IVEImageImpl *pSrc, IVEImageImpl *pDst, ThreshMode mode, CVI_U8 u8LowThr,
                         CVI_U8 u8HighThr, CVI_U8 u8MinVal, CVI_U8 u8MidVal,
                         CVI_U8 u8MaxVal) override;
  virtual CVI_S32 frame_diff(IVEImageImpl *pSrc1, IVEImageImpl *pSrc2, IVEImageImpl *pDst,
                             CVI_U8 threshold) override;

  virtual void *getHandle() override;

  // helper function for conversion of IVE types.
  static IVE_THRESH_MODE_E convert(ThreshMode mode);
  static IVE_SUB_MODE_E convert(SubMode mode);
  static IVE_DMA_MODE_E convert(DMAMode mode);

 private:
  IVE_HANDLE m_handle;
};

#define UNWRAP(x) reinterpret_cast<IVE_IMAGE_S *>((x->getHandle()))

IVEImageImpl *IVEImageImpl::create() { return new HWIVEImage; }

HWIVEImage::HWIVEImage() : m_handle(NULL) { memset(&ive_image, 0, sizeof(IVE_IMAGE_S)); }

void *HWIVEImage::getHandle() { return &ive_image; }

CVI_S32 HWIVEImage::bufFlush(IVEImpl *ive_instance) {
  m_handle = reinterpret_cast<IVE_HANDLE>(ive_instance->getHandle());
  return CVI_IVE_BufFlush(m_handle, &ive_image);
}

CVI_S32 HWIVEImage::bufRequest(IVEImpl *ive_instance) {
  m_handle = reinterpret_cast<IVE_HANDLE>(ive_instance->getHandle());
  return CVI_IVE_BufRequest(m_handle, &ive_image);
}

CVI_S32 HWIVEImage::create(IVEImpl *ive_instance, ImageType enType, CVI_U16 u16Width,
                           CVI_U16 u16Height, bool cached) {
  auto m_handle_ = reinterpret_cast<IVE_HANDLE>(ive_instance->getHandle());
  if (m_handle_ == NULL) {
    LOGE("create cached  handle should not be null\n");
    return CVI_FAILURE;
  }
  m_handle = m_handle_;
  if (cached) {
    return CVI_IVE_CreateImage_Cached(m_handle, &ive_image, convert(enType), u16Width, u16Height);
  } else {
    return CVI_IVE_CreateImage(m_handle, &ive_image, convert(enType), u16Width, u16Height);
  }
}

CVI_S32 HWIVEImage::create(IVEImpl *ive_instance, ImageType enType, CVI_U16 u16Width,
                           CVI_U16 u16Height, IVEImageImpl *buf, bool cached) {
  auto m_handle_ = reinterpret_cast<IVE_HANDLE>(ive_instance->getHandle());
  if (m_handle_ == NULL) {
    LOGE("create buf handle should not be null\n");
    return CVI_FAILURE;
  }
  m_handle = m_handle_;
  LOGE("cannot create IVE image with another buffer: unsupported\n");
  return CVI_FAILURE;
}

CVI_S32 HWIVEImage::create(IVEImpl *ive_instance) {
  auto m_handle_ = reinterpret_cast<IVE_HANDLE>(ive_instance->getHandle());
  if (m_handle_ == NULL) {
    LOGE("create handle should not be null\n");
    return CVI_FAILURE;
  }
  m_handle = m_handle_;
  LOGE("cannot create IVE image with another buffer: unsupported\n");
  return CVI_FAILURE;
}

CVI_S32 HWIVEImage::free() {
  if (m_handle == NULL) {
    LOGE("free handle is null\n");
    return CVI_FAILURE;
  }
  return CVI_SYS_FreeI(m_handle, &ive_image);
}

CVI_S32 HWIVEImage::write(const std::string &fname) {
  return CVI_IVE_WriteImg(m_handle, fname.c_str(), &ive_image);
}

CVI_S32 HWIVEImage::toFrame(VIDEO_FRAME_INFO_S *frame, bool invertPackage) {
  frame->stVFrame.u64PhyAddr[0] = ive_image.u64PhyAddr[0];
  frame->stVFrame.u64PhyAddr[1] = ive_image.u64PhyAddr[1];
  frame->stVFrame.u64PhyAddr[2] = ive_image.u64PhyAddr[2];

  frame->stVFrame.pu8VirAddr[0] = reinterpret_cast<uint8_t *>(ive_image.u64VirAddr[0]);
  frame->stVFrame.pu8VirAddr[1] = reinterpret_cast<uint8_t *>(ive_image.u64VirAddr[1]);
  frame->stVFrame.pu8VirAddr[2] = reinterpret_cast<uint8_t *>(ive_image.u64VirAddr[2]);

  frame->stVFrame.u32Stride[0] = ive_image.u32Stride[0];
  frame->stVFrame.u32Stride[1] = ive_image.u32Stride[1];
  frame->stVFrame.u32Stride[2] = ive_image.u32Stride[2];

  frame->stVFrame.u32Width = ive_image.u32Width;
  frame->stVFrame.u32Height = ive_image.u32Height;
  return CVI_SUCCESS;
}

CVI_S32 HWIVEImage::fromFrame(VIDEO_FRAME_INFO_S *frame) {
  ive_image.u64PhyAddr[0] = frame->stVFrame.u64PhyAddr[0];
  ive_image.u64PhyAddr[1] = frame->stVFrame.u64PhyAddr[1];
  ive_image.u64PhyAddr[2] = frame->stVFrame.u64PhyAddr[2];

  ive_image.u64VirAddr[0] = reinterpret_cast<CVI_U64>(frame->stVFrame.pu8VirAddr[0]);
  ive_image.u64VirAddr[1] = reinterpret_cast<CVI_U64>(frame->stVFrame.pu8VirAddr[1]);
  ive_image.u64VirAddr[2] = reinterpret_cast<CVI_U64>(frame->stVFrame.pu8VirAddr[2]);

  ive_image.u32Stride[0] = frame->stVFrame.u32Stride[0];
  ive_image.u32Stride[1] = frame->stVFrame.u32Stride[1];
  ive_image.u32Stride[2] = frame->stVFrame.u32Stride[2];

  ive_image.u32Width = frame->stVFrame.u32Width;
  ive_image.u32Height = frame->stVFrame.u32Height;
  return CVI_SUCCESS;
}

CVI_U32 HWIVEImage::getHeight() { return ive_image.u32Height; }

CVI_U32 HWIVEImage::getWidth() { return ive_image.u32Width; }

std::vector<CVI_U32> HWIVEImage::getStride() {
  return std::vector<CVI_U32>(std::begin(ive_image.u32Stride), std::end(ive_image.u32Stride));
}

std::vector<CVI_U8 *> HWIVEImage::getVAddr() {
  return {reinterpret_cast<CVI_U8 *>(ive_image.u64VirAddr[0]),
          reinterpret_cast<CVI_U8 *>(ive_image.u64VirAddr[1]),
          reinterpret_cast<CVI_U8 *>(ive_image.u64VirAddr[2])};
}

std::vector<CVI_U64> HWIVEImage::getPAddr() {
  return std::vector<CVI_U64>(std::begin(ive_image.u64PhyAddr), std::end(ive_image.u64PhyAddr));
}

ImageType HWIVEImage::getType() { return convert(ive_image.enType); }

ImageType HWIVEImage::convert(IVE_IMAGE_TYPE_E type) {
  switch (type) {
    case IVE_IMAGE_TYPE_U8C1:
      return U8C1;
    case IVE_IMAGE_TYPE_S8C1:
      return S8C1;
    case IVE_IMAGE_TYPE_YUV420SP:
      return YUV420SP;
    case IVE_IMAGE_TYPE_YUV422SP:
      return YUV422SP;
    case IVE_IMAGE_TYPE_YUV420P:
      return YUV420P;
    case IVE_IMAGE_TYPE_YUV422P:
      return YUV422P;
    case IVE_IMAGE_TYPE_S8C2_PACKAGE:
      return S8C2_PACKAGE;
    case IVE_IMAGE_TYPE_S8C2_PLANAR:
      return S8C2_PLANAR;
    case IVE_IMAGE_TYPE_S16C1:
      return S16C1;
    case IVE_IMAGE_TYPE_U16C1:

      return U16C1;
    case IVE_IMAGE_TYPE_U8C3_PACKAGE:
      return U8C3_PACKAGE;
    case IVE_IMAGE_TYPE_U8C3_PLANAR:
      return U8C3_PLANAR;
    case IVE_IMAGE_TYPE_S32C1:
      return S32C1;
    case IVE_IMAGE_TYPE_U32C1:
      return U32C1;
    case IVE_IMAGE_TYPE_S64C1:
      return S64C1;
    case IVE_IMAGE_TYPE_U64C1:
      return U64C1;
    case IVE_IMAGE_TYPE_BF16C1:
      return BF16C1;
    case IVE_IMAGE_TYPE_FP32C1:
      return FP32C1;
    default:
      return FP32C1;
  }
}

IVE_IMAGE_TYPE_E HWIVEImage::convert(ImageType type) {
  switch (type) {
    case U8C1:
      return IVE_IMAGE_TYPE_U8C1;
    case S8C1:
      return IVE_IMAGE_TYPE_S8C1;
    case YUV420SP:
      return IVE_IMAGE_TYPE_YUV420SP;
    case YUV422SP:
      return IVE_IMAGE_TYPE_YUV422SP;
    case YUV420P:
      return IVE_IMAGE_TYPE_YUV420P;
    case YUV422P:
      return IVE_IMAGE_TYPE_YUV422P;
    case S8C2_PACKAGE:
      return IVE_IMAGE_TYPE_S8C2_PACKAGE;
    case S8C2_PLANAR:
      return IVE_IMAGE_TYPE_S8C2_PLANAR;
    case S16C1:
      return IVE_IMAGE_TYPE_S16C1;
    case U16C1:
      return IVE_IMAGE_TYPE_U16C1;
    case U8C3_PACKAGE:
      return IVE_IMAGE_TYPE_U8C3_PACKAGE;
    case U8C3_PLANAR:
      return IVE_IMAGE_TYPE_U8C3_PLANAR;
    case S32C1:
      return IVE_IMAGE_TYPE_S32C1;
    case U32C1:
      return IVE_IMAGE_TYPE_U32C1;
    case S64C1:
      return IVE_IMAGE_TYPE_S64C1;
    case U64C1:
      return IVE_IMAGE_TYPE_U64C1;
    case BF16C1:
      return IVE_IMAGE_TYPE_BF16C1;
    case FP32C1:
      return IVE_IMAGE_TYPE_FP32C1;
    default:
      return IVE_IMAGE_TYPE_BUTT;
  }
}

IVEImpl *IVEImpl::create() { return new HWIVE; }

void *HWIVE::getHandle() { return m_handle; }

IVE_THRESH_MODE_E HWIVE::convert(ThreshMode mode) {
  switch (mode) {
    case BINARY:
      return IVE_THRESH_MODE_BINARY;
    case TRUNC:
      return IVE_THRESH_MODE_TRUNC;
    case TO_MINVAL:
      return IVE_THRESH_MODE_TO_MINVAL;
    case MIN_MID_MAX:
      return IVE_THRESH_MODE_MIN_MID_MAX;
    case ORI_MID_MAX:
      return IVE_THRESH_MODE_ORI_MID_MAX;
    case MIN_MID_ORI:
      return IVE_THRESH_MODE_MIN_MID_ORI;
    case MIN_ORI_MAX:
      return IVE_THRESH_MODE_MIN_ORI_MAX;
    case ORI_MID_ORI:
      return IVE_THRESH_MODE_ORI_MID_ORI;
    default:
      return IVE_THRESH_MODE_BUTT;
  }
}

IVE_SUB_MODE_E HWIVE::convert(SubMode mode) {
  switch (mode) {
    case ABS:
      return IVE_SUB_MODE_ABS;
    case SHIFT:
      return IVE_SUB_MODE_SHIFT;
    default:
      return IVE_SUB_MODE_BUTT;
  }
}

IVE_DMA_MODE_E HWIVE::convert(DMAMode mode) {
  switch (mode) {
    case DIRECT_COPY:
      return IVE_DMA_MODE_DIRECT_COPY;
    case INTERVAL_COPY:
      return IVE_DMA_MODE_INTERVAL_COPY;
    case SET_3BYTE:
      return IVE_DMA_MODE_SET_3BYTE;
    case SET_8BYTE:
      return IVE_DMA_MODE_SET_8BYTE;
    default:
      return IVE_DMA_MODE_BUTT;
  }
}

struct IVE_HANDLE_CTX {
  int devfd;
  int used_time;
};

HWIVE::HWIVE() : m_handle(NULL) {}

CVI_S32 HWIVE::init() {
  m_handle = CVI_IVE_CreateHandle();
  IVE_HANDLE_CTX *ctx = static_cast<IVE_HANDLE_CTX *>(m_handle);
  if (ctx->devfd == ERR_IVE_OPEN_FILE) {
    return CVI_FAILURE;
  }
  return CVI_SUCCESS;
}

CVI_S32 HWIVE::destroy() { return CVI_IVE_DestroyHandle(m_handle); }

CVI_U32 HWIVE::getWidthAlign() { return LUMA_PHY_ALIGN; }

CVI_S32 HWIVE::fillConst(IVEImageImpl *pSrc, float value) {
  IVE_IMAGE_S *pIVEImage = UNWRAP(pSrc);

  // TODO: deal with different image type
  //   for (uint32_t i = 0; i < 3; i++) {
  uint8_t *pData = reinterpret_cast<uint8_t *>(pIVEImage->u64VirAddr[0]);
  uint32_t size = pIVEImage->u32Stride[0] * pIVEImage->u32Height;
  memset(pData, (uint8_t)value, size);
  //   }

  return CVI_SUCCESS;
}

IVE_DATA_S convertFrom(IVE_IMAGE_S *image) {
  IVE_DATA_S data;
  data.u32Height = image->u32Height;
  data.u32Stride = image->u32Stride[0];
  data.u32Width = image->u32Width;
  data.u64PhyAddr = image->u64PhyAddr[0];
  data.u64VirAddr = image->u64VirAddr[0];
  return data;
}

CVI_S32 HWIVE::dma(IVEImageImpl *pSrc, IVEImageImpl *pDst, DMAMode mode, CVI_U64 u64Val,
                   CVI_U8 u8HorSegSize, CVI_U8 u8ElemSize, CVI_U8 u8VerSegRows) {
  // TODO: deal with different format of image
  IVE_DMA_CTRL_S ctrl;
  ctrl.enMode = convert(mode);
  ctrl.u64Val = u64Val;
  ctrl.u8HorSegSize = u8HorSegSize;
  ctrl.u8ElemSize = u8ElemSize;
  ctrl.u8VerSegRows = u8VerSegRows;

  if (ctrl.enMode == IVE_DMA_MODE_BUTT) {
    LOGE("Unsupported DMA mode: %d\n", mode);
    return CVI_FAILURE;
  }

  if (pSrc->getType() != ImageType::U8C1 && pDst->getType() != ImageType::U8C1) {
    LOGE("Fail to perform DMA: Unsupported format=%d", pSrc->getType());
    return CVI_FAILURE;
  }

  IVE_DATA_S src_data = convertFrom(UNWRAP(pSrc));
  IVE_DATA_S src_dst = convertFrom(UNWRAP(pDst));

  return CVI_IVE_DMA(m_handle, &src_data, &src_dst, &ctrl, false);
}

CVI_S32 HWIVE::sub(IVEImageImpl *pSrc1, IVEImageImpl *pSrc2, IVEImageImpl *pDst, SubMode mode) {
  IVE_SUB_CTRL_S ctrl;
  ctrl.enMode = convert(mode);

  if (ctrl.enMode == IVE_SUB_MODE_BUTT) {
    LOGE("Unsupported Sub mode: %d\n", mode);
    return CVI_FAILURE;
  }
  return CVI_IVE_Sub(m_handle, UNWRAP(pSrc1), UNWRAP(pSrc2), UNWRAP(pDst), &ctrl, false);
}

CVI_S32 HWIVE::andImage(IVEImageImpl *pSrc1, IVEImageImpl *pSrc2, IVEImageImpl *pDst) {
  return CVI_IVE_And(m_handle, UNWRAP(pSrc1), UNWRAP(pSrc2), UNWRAP(pDst), false);
}

CVI_S32 HWIVE::orImage(IVEImageImpl *pSrc1, IVEImageImpl *pSrc2, IVEImageImpl *pDst) {
  return CVI_IVE_Or(m_handle, UNWRAP(pSrc1), UNWRAP(pSrc2), UNWRAP(pDst), false);
}

CVI_S32 HWIVE::erode(IVEImageImpl *pSrc1, IVEImageImpl *pDst, const std::vector<CVI_S32> &mask) {
  IVE_ERODE_CTRL_S ctrl;
  std::copy(mask.begin(), mask.end(), ctrl.au8Mask);
  return CVI_IVE_Erode(m_handle, UNWRAP(pSrc1), UNWRAP(pDst), &ctrl, false);
}

CVI_S32 HWIVE::dilate(IVEImageImpl *pSrc1, IVEImageImpl *pDst, const std::vector<CVI_S32> &mask) {
  IVE_DILATE_CTRL_S ctrl;
  std::copy(mask.begin(), mask.end(), ctrl.au8Mask);
  return CVI_IVE_Dilate(m_handle, UNWRAP(pSrc1), UNWRAP(pDst), &ctrl, false);
}

CVI_S32 HWIVE::add(IVEImageImpl *pSrc1, IVEImageImpl *pSrc2, IVEImageImpl *pDst, float alpha,
                   float beta) {
  IVE_ADD_CTRL_S ctrl;
  ctrl.u0q16X = alpha * std::numeric_limits<unsigned short>::max();
  ctrl.u0q16Y = beta * std::numeric_limits<unsigned short>::max();
  return CVI_IVE_Add(m_handle, UNWRAP(pSrc1), UNWRAP(pSrc2), UNWRAP(pDst), &ctrl, false);
}

CVI_S32 HWIVE::add(IVEImageImpl *pSrc1, IVEImageImpl *pSrc2, IVEImageImpl *pDst,
                   unsigned short alpha, unsigned short beta) {
  IVE_ADD_CTRL_S ctrl;
  ctrl.u0q16X = alpha;
  ctrl.u0q16Y = beta;
  return CVI_IVE_Add(m_handle, UNWRAP(pSrc1), UNWRAP(pSrc2), UNWRAP(pDst), &ctrl, false);
}

CVI_S32 HWIVE::roi(IVEImageImpl *pSrc, IVEImageImpl *pDst, uint32_t x1, uint32_t x2, uint32_t y1,
                   uint32_t y2) {
  IVE_IMAGE_S *pIVESrc = UNWRAP(pSrc);
  IVE_IMAGE_S *pIVEDst = UNWRAP(pDst);
  uint32_t roi_height = y2 - y1;
  uint32_t roi_width = x2 - x1;
  pIVEDst->u32Height = roi_height;
  pIVEDst->u32Width = roi_width;
  pIVEDst->u32Stride[0] = pIVESrc->u32Stride[0];
  pIVEDst->u32Stride[1] = pIVESrc->u32Stride[1];
  pIVEDst->u32Stride[2] = pIVESrc->u32Stride[2];

  pIVEDst->u64VirAddr[0] = pIVESrc->u64VirAddr[0] + x1 + (y1 * pIVESrc->u32Stride[0]);
  pIVEDst->u64VirAddr[1] = pIVESrc->u64VirAddr[1] + x1 + (y1 * pIVESrc->u32Stride[1]);
  pIVEDst->u64VirAddr[2] = pIVESrc->u64VirAddr[2] + x1 + (y1 * pIVESrc->u32Stride[2]);

  pIVEDst->u64PhyAddr[0] = pIVESrc->u64PhyAddr[0] + x1 + (y1 * pIVESrc->u32Stride[0]);
  pIVEDst->u64PhyAddr[1] = pIVESrc->u64PhyAddr[1] + x1 + (y1 * pIVESrc->u32Stride[0]);
  pIVEDst->u64PhyAddr[2] = pIVESrc->u64PhyAddr[2] + x1 + (y1 * pIVESrc->u32Stride[0]);
  pIVEDst->u32Reserved = pIVESrc->u32Reserved;
  pIVEDst->enType = pIVESrc->enType;
  return CVI_SUCCESS;
}

CVI_S32 HWIVE::thresh(IVEImageImpl *pSrc, IVEImageImpl *pDst, ThreshMode mode, CVI_U8 u8LowThr,
                      CVI_U8 u8HighThr, CVI_U8 u8MinVal, CVI_U8 u8MidVal, CVI_U8 u8MaxVal) {
  IVE_THRESH_CTRL_S ctrl;
  ctrl.enMode = convert(mode);
  ctrl.u8MinVal = u8MinVal;
  ctrl.u8MaxVal = u8MaxVal;
  ctrl.u8LowThr = u8LowThr;
  ctrl.u8MidVal = u8MidVal;
  ctrl.u8HighThr = u8HighThr;

  if (ctrl.enMode == IVE_THRESH_MODE_BUTT) {
    LOGE("Unsupported Thresh mode: %d\n", mode);
    return CVI_FAILURE;
  }

  return CVI_IVE_Thresh(m_handle, UNWRAP(pSrc), UNWRAP(pDst), &ctrl, false);
}

CVI_S32 HWIVE::frame_diff(IVEImageImpl *pSrc1, IVEImageImpl *pSrc2, IVEImageImpl *pDst,
                          CVI_U8 threshold) {
  IVE_FRAME_DIFF_MOTION_CTRL_S ctrl = {
      .enSubMode = IVE_SUB_MODE_ABS,
      .enThrMode = IVE_THRESH_MODE_BINARY,
      .u8ThrLow = threshold,
      .u8ThrHigh = 0,
      .u8ThrMinVal = 0,
      .u8ThrMidVal = 0,
      .u8ThrMaxVal = 255,
      .au8ErodeMask = {0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0},
      .au8DilateMask = {0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0},
  };

  return CVI_IVE_FrameDiffMotion(m_handle, UNWRAP(pSrc1), UNWRAP(pSrc2), UNWRAP(pDst), &ctrl,
                                 false);
}

}  // namespace ive
