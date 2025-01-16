#include <cstring>
#include <iostream>
// #include "cvi_ive.h"
#include "cvi_tdl_log.hpp"
#include "impl_ive.hpp"
#include "ive/cvi_comm_ive.h"
#include "ive/ive.h"
namespace ive {

class TPUIVEImage : public IVEImageImpl {
 public:
  TPUIVEImage();
  virtual ~TPUIVEImage();
  TPUIVEImage(const TPUIVEImage &other) = delete;
  TPUIVEImage &operator=(const TPUIVEImage &other) = delete;

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

class TPUIVE : public IVEImpl {
 public:
  TPUIVE();
  virtual ~TPUIVE() = default;
  TPUIVE(const TPUIVE &other) = delete;
  TPUIVE &operator=(const TPUIVE &other) = delete;

  virtual CVI_S32 init() override;
  virtual CVI_S32 destroy() override;
  virtual CVI_U32 getWidthAlign() override;
  virtual CVI_S32 fillConst(IVEImageImpl *pSrc, float value) override;
  virtual CVI_S32 dma(IVEImageImpl *pSrc, IVEImageImpl *pDst, DMAMode mode = DIRECT_COPY,
                      CVI_U64 u64Val = 0, CVI_U8 u8HorSegSize = 0, CVI_U8 u8ElemSize = 0,
                      CVI_U8 u8VerSegRows = 0) override;
  virtual CVI_S32 sub(IVEImageImpl *pSrc1, IVEImageImpl *pSrc2, IVEImageImpl *pDst,
                      SubMode mode = ABS) override;
  virtual CVI_S32 andImage(IVEImageImpl *pSrc1, IVEImageImpl *pSrc2, IVEImageImpl *pDst) override;
  virtual CVI_S32 orImage(IVEImageImpl *pSrc1, IVEImageImpl *pSrc2, IVEImageImpl *pDst) override;
  virtual CVI_S32 erode(IVEImageImpl *pSrc1, IVEImageImpl *pDst,
                        const std::vector<CVI_S32> &mask) override;
  virtual CVI_S32 dilate(IVEImageImpl *pSrc1, IVEImageImpl *pDst,
                         const std::vector<CVI_S32> &mask) override;
  virtual CVI_S32 roi(IVEImageImpl *pSrc, IVEImageImpl *pDst, uint32_t x1, uint32_t x2, uint32_t y1,
                      uint32_t y2) override;
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

IVEImageImpl *IVEImageImpl::create() { return new TPUIVEImage; }

TPUIVEImage::TPUIVEImage() : m_handle(NULL) { memset(&ive_image, 0, sizeof(IVE_IMAGE_S)); }
TPUIVEImage::~TPUIVEImage() {
  // ive_image has heap memory should be released manually
  CVI_SYS_FreeI(m_handle, &ive_image);
}
void *TPUIVEImage::getHandle() { return &ive_image; }

CVI_S32 TPUIVEImage::bufFlush(IVEImpl *ive_instance) {
  if (m_handle == NULL) return CVI_FAILURE;
  return CVI_IVE_BufFlush(m_handle, &ive_image);
}

CVI_S32 TPUIVEImage::bufRequest(IVEImpl *ive_instance) {
  if (m_handle == NULL) return CVI_FAILURE;
  return CVI_IVE_BufRequest(m_handle, &ive_image);
}

CVI_S32 TPUIVEImage::create(IVEImpl *ive_instance, ImageType enType, CVI_U16 u16Width,
                            CVI_U16 u16Height, bool cached) {
  m_handle = reinterpret_cast<IVE_HANDLE>(ive_instance->getHandle());
  return CVI_IVE_CreateImage(m_handle, &ive_image, convert(enType), u16Width, u16Height);
}

CVI_S32 TPUIVEImage::create(IVEImpl *ive_instance, ImageType enType, CVI_U16 u16Width,
                            CVI_U16 u16Height, IVEImageImpl *buf, bool cached) {
  m_handle = reinterpret_cast<IVE_HANDLE>(ive_instance->getHandle());
  return CVI_IVE_CreateImage2(m_handle, &ive_image, convert(enType), u16Width, u16Height,
                              UNWRAP(buf));
}

CVI_S32 TPUIVEImage::create(IVEImpl *ive_instance) {
  m_handle = reinterpret_cast<IVE_HANDLE>(ive_instance->getHandle());
  return CVI_SUCCESS;
}

CVI_S32 dump_ive_image_framex(const std::string &filepath, uint8_t *ptr_img, int w, int h,
                              int wstride) {
  FILE *fp = fopen(filepath.c_str(), "wb");
  if (fp == nullptr) {
    LOGE("failed to open: %s.\n", filepath.c_str());
    return CVI_FAILURE;
  }

  fwrite(&w, sizeof(uint32_t), 1, fp);
  fwrite(&h, sizeof(uint32_t), 1, fp);

  std::cout << "width:" << w << ",height:" << h << ",stride:" << wstride << std::endl;
  fwrite(ptr_img, w * h, 1, fp);

  // for(int j = 0 ; j < h; j++){
  //     uint8_t *ptrj = ptr + j*wstride;
  //     std::cout<<"write r:"<<j<<std::endl;
  //     fwrite(ptrj,w,1,fp);
  // }
  std::cout << "toclose:" << filepath << std::endl;
  fclose(fp);

  std::cout << "closed:" << filepath << std::endl;
  return CVI_SUCCESS;
}

CVI_S32 TPUIVEImage::write(const std::string &fname) {
  CVI_IVE_BufRequest(m_handle, &ive_image);
  std::cout << "towrite:" << fname << std::endl;
  // return CVI_IVE_WriteImage(m_handle, fname.c_str(), &ive_image);
  return dump_ive_image_framex(fname, ive_image.pu8VirAddr[0], ive_image.u16Width,
                               ive_image.u16Height, ive_image.u16Stride[0]);
}

CVI_S32 TPUIVEImage::free() {
  // if (m_handle == NULL) return CVI_FAILURE;
  return CVI_SYS_FreeI(m_handle, &ive_image);
}

CVI_S32 TPUIVEImage::toFrame(VIDEO_FRAME_INFO_S *frame, bool invertPackage) {
  return CVI_IVE_Image2VideoFrameInfo(&ive_image, frame, invertPackage);
}

CVI_S32 TPUIVEImage::fromFrame(VIDEO_FRAME_INFO_S *frame) {
  return CVI_IVE_VideoFrameInfo2Image(frame, &ive_image);
}

CVI_U32 TPUIVEImage::getHeight() { return ive_image.u16Height; }

CVI_U32 TPUIVEImage::getWidth() { return ive_image.u16Width; }

std::vector<CVI_U32> TPUIVEImage::getStride() {
  return std::vector<CVI_U32>(std::begin(ive_image.u16Stride), std::end(ive_image.u16Stride));
}

std::vector<CVI_U8 *> TPUIVEImage::getVAddr() {
  return std::vector<CVI_U8 *>(std::begin(ive_image.pu8VirAddr), std::end(ive_image.pu8VirAddr));
}

std::vector<CVI_U64> TPUIVEImage::getPAddr() {
  return std::vector<CVI_U64>(std::begin(ive_image.u64PhyAddr), std::end(ive_image.u64PhyAddr));
}

ImageType TPUIVEImage::getType() { return convert(ive_image.enType); }

ImageType TPUIVEImage::convert(IVE_IMAGE_TYPE_E type) {
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

IVE_IMAGE_TYPE_E TPUIVEImage::convert(ImageType type) {
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

IVEImpl *IVEImpl::create() { return new TPUIVE; }

void *TPUIVE::getHandle() { return m_handle; }

IVE_THRESH_MODE_E TPUIVE::convert(ThreshMode mode) {
  switch (mode) {
    case BINARY:
      return IVE_THRESH_MODE_BINARY;
    case SLOPE:
      return IVE_THRESH_MODE_SLOPE;
    default:
      return IVE_THRESH_MODE_BUTT;
  }
}

IVE_SUB_MODE_E TPUIVE::convert(SubMode mode) {
  switch (mode) {
    case NORMAL:
      return IVE_SUB_MODE_NORMAL;
    case ABS:
      return IVE_SUB_MODE_ABS;
    case SHIFT:
      return IVE_SUB_MODE_SHIFT;
    case ABS_THRESH:
      return IVE_SUB_MODE_ABS_THRESH;
    default:
      return IVE_SUB_MODE_BUTT;
  }
}

IVE_DMA_MODE_E TPUIVE::convert(DMAMode mode) {
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

TPUIVE::TPUIVE() : m_handle(NULL) {}

CVI_S32 TPUIVE::init() {
  m_handle = CVI_IVE_CreateHandle();
  return m_handle == NULL ? CVI_FAILURE : CVI_SUCCESS;
}

CVI_S32 TPUIVE::destroy() { return CVI_IVE_DestroyHandle(m_handle); }

CVI_U32 TPUIVE::getWidthAlign() { return DEFAULT_ALIGN; }

CVI_S32 TPUIVE::fillConst(IVEImageImpl *pSrc, float value) {
  return CVI_IVE_ConstFill(m_handle, value, UNWRAP(pSrc), false);
}
CVI_S32 TPUIVE::dma(IVEImageImpl *pSrc, IVEImageImpl *pDst, DMAMode mode, CVI_U64 u64Val,
                    CVI_U8 u8HorSegSize, CVI_U8 u8ElemSize, CVI_U8 u8VerSegRows) {
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

  return CVI_IVE_DMA(m_handle, UNWRAP(pSrc), UNWRAP(pDst), &ctrl, false);
}

CVI_S32 TPUIVE::roi(IVEImageImpl *pSrc, IVEImageImpl *pDst, uint32_t x1, uint32_t x2, uint32_t y1,
                    uint32_t y2) {
  return CVI_IVE_SubImage(m_handle, UNWRAP(pSrc), UNWRAP(pDst), static_cast<uint16_t>(x1),
                          static_cast<uint16_t>(y1), static_cast<uint16_t>(x2),
                          static_cast<uint16_t>(y2));
}

CVI_S32 TPUIVE::sub(IVEImageImpl *pSrc1, IVEImageImpl *pSrc2, IVEImageImpl *pDst, SubMode mode) {
  IVE_SUB_CTRL_S ctrl;
  ctrl.enMode = convert(mode);

  if (ctrl.enMode == IVE_SUB_MODE_BUTT) {
    LOGE("Unsupported Sub mode: %d\n", mode);
    return CVI_FAILURE;
  }
  return CVI_IVE_Sub(m_handle, UNWRAP(pSrc1), UNWRAP(pSrc2), UNWRAP(pDst), &ctrl, false);
}

CVI_S32 TPUIVE::andImage(IVEImageImpl *pSrc1, IVEImageImpl *pSrc2, IVEImageImpl *pDst) {
  return CVI_IVE_And(m_handle, UNWRAP(pSrc1), UNWRAP(pSrc2), UNWRAP(pDst), false);
}

CVI_S32 TPUIVE::orImage(IVEImageImpl *pSrc1, IVEImageImpl *pSrc2, IVEImageImpl *pDst) {
  return CVI_IVE_Or(m_handle, UNWRAP(pSrc1), UNWRAP(pSrc2), UNWRAP(pDst), false);
}

CVI_S32 TPUIVE::erode(IVEImageImpl *pSrc1, IVEImageImpl *pDst, const std::vector<CVI_S32> &mask) {
  IVE_ERODE_CTRL_S ctrl;
  std::copy(mask.begin(), mask.end(), ctrl.au8Mask);
  return CVI_IVE_Erode(m_handle, UNWRAP(pSrc1), UNWRAP(pDst), &ctrl, false);
}

CVI_S32 TPUIVE::dilate(IVEImageImpl *pSrc1, IVEImageImpl *pDst, const std::vector<CVI_S32> &mask) {
  IVE_DILATE_CTRL_S ctrl;
  std::copy(mask.begin(), mask.end(), ctrl.au8Mask);
  return CVI_IVE_Dilate(m_handle, UNWRAP(pSrc1), UNWRAP(pDst), &ctrl, false);
}

CVI_S32 TPUIVE::add(IVEImageImpl *pSrc1, IVEImageImpl *pSrc2, IVEImageImpl *pDst, float alpha,
                    float beta) {
  IVE_ADD_CTRL_S ctrl;
  ctrl.aX = alpha;
  ctrl.bY = beta;
  return CVI_IVE_Add(m_handle, UNWRAP(pSrc1), UNWRAP(pSrc2), UNWRAP(pDst), &ctrl, false);
}

CVI_S32 TPUIVE::add(IVEImageImpl *pSrc1, IVEImageImpl *pSrc2, IVEImageImpl *pDst,
                    unsigned short alpha, unsigned short beta) {
  IVE_ADD_CTRL_S ctrl;
  ctrl.aX = static_cast<float>(alpha) / std::numeric_limits<unsigned short>::max();
  ctrl.bY = static_cast<float>(beta) / std::numeric_limits<unsigned short>::max();
  return CVI_IVE_Add(m_handle, UNWRAP(pSrc1), UNWRAP(pSrc2), UNWRAP(pDst), &ctrl, false);
}

CVI_S32 TPUIVE::thresh(IVEImageImpl *pSrc, IVEImageImpl *pDst, ThreshMode mode, CVI_U8 u8LowThr,
                       CVI_U8 u8HighThr, CVI_U8 u8MinVal, CVI_U8 u8MidVal, CVI_U8 u8MaxVal) {
  IVE_THRESH_CTRL_S ctrl;
  ctrl.enMode = convert(mode);
  ctrl.u8MinVal = u8MinVal;
  ctrl.u8MaxVal = u8MaxVal;
  ctrl.u8LowThr = u8LowThr;

  if (ctrl.enMode == IVE_THRESH_MODE_BUTT) {
    LOGE("Unsupported Thresh mode: %d\n", mode);
    return CVI_FAILURE;
  }

  return CVI_IVE_Thresh(m_handle, UNWRAP(pSrc), UNWRAP(pDst), &ctrl, false);
}

CVI_S32 TPUIVE::frame_diff(IVEImageImpl *pSrc1, IVEImageImpl *pSrc2, IVEImageImpl *pDst,
                           CVI_U8 threshold) {
  CVI_S32 ret;
#ifdef DEBUG_MD
  LOGI("MD DEBUG: write: src.bin\n");
  pSrc1->write("/mnt/data/admin1_data/alios_test/md/src1.bin");
  pSrc2->write("/mnt/data/admin1_data/alios_test/md/src2.bin");
#endif
  // Sub - threshold - dilate
  ret = sub(pSrc1, pSrc2, pDst);
#ifdef DEBUG_MD
  LOGI("MD DEBUG: write: sub.bin\n");
  pDst->write("/mnt/data/admin1_data/alios_test/md/sub.bin");
#endif
  if (ret != CVI_SUCCESS) {
    LOGE("CVI_IVE_Sub fail %x\n", ret);
    return ret;
  }

  ret = thresh(pDst, pDst, ThreshMode::BINARY, threshold, 0, 0, 0, 255);
#ifdef DEBUG_MD
  LOGI("MD DEBUG: write: thresh.bin\n");
  // pDst->write("thresh.bin");
  pDst->write("/mnt/data/admin1_data/alios_test/md/thresh.bin");
#endif
  if (ret != CVI_SUCCESS) {
    return ret;
  }
#ifndef NO_OPENCV  // on opencv case,dilate would be done inside ccl
  ret = dilate(pDst, pDst,
               {0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0});
#endif
  if (ret != CVI_SUCCESS) {
    LOGE("dilate fail %x\n", ret);
    return ret;
  }

#ifdef DEBUG_MD
  LOGI("MD DEBUG: write: dialte.bin\n");
  // pDst->write("dilate.bin");
  pDst->write("/mnt/data/admin1_data/alios_test/md/dialte.bin");
#endif
  return ret;
}

}  // namespace ive
