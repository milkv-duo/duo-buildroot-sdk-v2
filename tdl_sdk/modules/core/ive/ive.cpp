#include "ive.hpp"
#include <cstring>
#include "impl_ive.hpp"

namespace ive {
IVEImage::IVEImage() : mpImpl(IVEImageImpl::create()) {}

IVEImage::~IVEImage() {}

CVI_S32 IVEImage::toFrame(VIDEO_FRAME_INFO_S *frame) { return mpImpl->toFrame(frame); }

CVI_S32 IVEImage::fromFrame(VIDEO_FRAME_INFO_S *frame) { return mpImpl->fromFrame(frame); }

CVI_S32 IVEImage::bufFlush(IVE *ive_instance) { return mpImpl->bufFlush(ive_instance->getImpl()); }

CVI_S32 IVEImage::bufRequest(IVE *ive_instance) {
  return mpImpl->bufRequest(ive_instance->getImpl());
}

CVI_S32 IVEImage::create(IVE *ive_instance, ImageType enType, CVI_U32 u32Width, CVI_U32 u32Height,
                         bool cached) {
  return mpImpl->create(ive_instance->getImpl(), enType, u32Width, u32Height, cached);
}

CVI_S32 IVEImage::create(IVE *ive_instance, ImageType enType, CVI_U32 u32Width, CVI_U32 u32Height,
                         IVEImage *buf, bool cached) {
  return mpImpl->create(ive_instance->getImpl(), enType, u32Width, u32Height, buf->getImpl(),
                        cached);
}

CVI_S32 IVEImage::create(IVE *ive_instance) { return mpImpl->create(ive_instance->getImpl()); }

IVEImageImpl *IVEImage::getImpl() { return mpImpl.get(); }

CVI_U32 IVEImage::getHeight() { return mpImpl->getHeight(); }

CVI_U32 IVEImage::getWidth() { return mpImpl->getWidth(); }

ImageType IVEImage::getType() { return mpImpl->getType(); }

std::vector<CVI_U32> IVEImage::getStride() { return mpImpl->getStride(); }

std::vector<CVI_U8 *> IVEImage::getVAddr() { return mpImpl->getVAddr(); }

std::vector<CVI_U64> IVEImage::getPAddr() { return mpImpl->getPAddr(); }

CVI_S32 IVEImage::setZero(IVE *ive_instance) {
  std::vector<CVI_U8 *> v_addrs = getVAddr();
  std::vector<CVI_U32> strides = getStride();

  if (v_addrs.size() != strides.size()) {
    // LOGE("vaddrs num:%d,strides num:%d\n",(int)v_addrs.size(),(int)strides.size());
    return CVI_FAILURE;
  }
  CVI_U32 imh = getHeight();
  for (uint32_t i = 0; i < v_addrs.size(); i++) {
    memset(v_addrs[i], 0, imh * strides[i]);
  }
  return bufFlush(ive_instance);
}

CVI_S32 IVEImage::free() { return mpImpl->free(); }

CVI_S32 IVEImage::write(const std::string &fname) { return mpImpl->write(fname); }

IVE::IVE() : mpImpl(IVEImpl::create()) {}

IVE::~IVE() {}

CVI_S32 IVE::init() { return mpImpl->init(); }

CVI_S32 IVE::destroy() { return mpImpl->destroy(); }

IVEImpl *IVE::getImpl() { return mpImpl.get(); }

CVI_U32 IVE::getAlignedWidth(uint32_t width) { return mpImpl->getAlignedWidth(width); }

CVI_S32 IVE::fillConst(IVEImage *pSrc, float value) {
  return mpImpl->fillConst(pSrc->getImpl(), value);
}

CVI_S32 IVE::dma(IVEImage *pSrc, IVEImage *pDst, DMAMode mode, CVI_U64 u64Val, CVI_U8 u8HorSegSize,
                 CVI_U8 u8ElemSize, CVI_U8 u8VerSegRows) {
  return mpImpl->dma(pSrc->getImpl(), pDst->getImpl(), mode, u64Val, u8HorSegSize, u8ElemSize,
                     u8VerSegRows);
}

CVI_S32 IVE::sub(IVEImage *pSrc1, IVEImage *pSrc2, IVEImage *pDst, SubMode mode) {
  return mpImpl->sub(pSrc1->getImpl(), pSrc2->getImpl(), pDst->getImpl(), mode);
}

CVI_S32 IVE::roi(IVEImage *pSrc, IVEImage *pDst, uint32_t x1, uint32_t x2, uint32_t y1,
                 uint32_t y2) {
  return mpImpl->roi(pSrc->getImpl(), pDst->getImpl(), x1, x2, y1, y2);
}

CVI_S32 IVE::andImage(IVEImage *pSrc1, IVEImage *pSrc2, IVEImage *pDst) {
  return mpImpl->andImage(pSrc1->getImpl(), pSrc2->getImpl(), pDst->getImpl());
}

CVI_S32 IVE::orImage(IVEImage *pSrc1, IVEImage *pSrc2, IVEImage *pDst) {
  return mpImpl->orImage(pSrc1->getImpl(), pSrc2->getImpl(), pDst->getImpl());
}

CVI_S32 IVE::erode(IVEImage *pSrc1, IVEImage *pDst, const std::vector<CVI_S32> &mask) {
  return mpImpl->erode(pSrc1->getImpl(), pDst->getImpl(), mask);
}

CVI_S32 IVE::dilate(IVEImage *pSrc1, IVEImage *pDst, const std::vector<CVI_S32> &mask) {
  return mpImpl->dilate(pSrc1->getImpl(), pDst->getImpl(), mask);
}

CVI_S32 IVE::add(IVEImage *pSrc1, IVEImage *pSrc2, IVEImage *pDst, float alpha, float beta) {
  return mpImpl->add(pSrc1->getImpl(), pSrc2->getImpl(), pDst->getImpl(), alpha, beta);
}

CVI_S32 IVE::add(IVEImage *pSrc1, IVEImage *pSrc2, IVEImage *pDst, unsigned short alpha,
                 unsigned short beta) {
  return mpImpl->add(pSrc1->getImpl(), pSrc2->getImpl(), pDst->getImpl(), alpha, beta);
}

CVI_S32 IVE::thresh(IVEImage *pSrc, IVEImage *pDst, ThreshMode mode, CVI_U8 u8LowThr,
                    CVI_U8 u8HighThr, CVI_U8 u8MinVal, CVI_U8 u8MidVal, CVI_U8 u8MaxVal) {
  return mpImpl->thresh(pSrc->getImpl(), pDst->getImpl(), mode, u8LowThr, u8HighThr, u8MinVal,
                        u8MidVal, u8MaxVal);
}

CVI_S32 IVE::frame_diff(IVEImage *pSrc1, IVEImage *pSrc2, IVEImage *pDst, CVI_U8 threshold) {
  return mpImpl->frame_diff(pSrc1->getImpl(), pSrc2->getImpl(), pDst->getImpl(), threshold);
}

}  // namespace ive
