#pragma once

#include <cvi_comm.h>
#include <limits>
#include <memory>
#include <vector>
#include "ive_common.hpp"

namespace ive {

class IVEImageImpl;
class IVEImpl;
class IVE;

class IVEImage {
 public:
  IVEImage();
  ~IVEImage();
  IVEImage(const IVEImage &other) = delete;
  IVEImage &operator=(const IVEImage &other) = delete;

  IVEImageImpl *getImpl();

  CVI_S32 bufFlush(IVE *ive_instance);
  CVI_S32 bufRequest(IVE *ive_instance);
  CVI_S32 create(IVE *ive_instance, ImageType enType, CVI_U32 u32Width, CVI_U32 u32Height,
                 bool cached = false);
  CVI_S32 create(IVE *ive_instance, ImageType enType, CVI_U32 u32Width, CVI_U32 u32Height,
                 IVEImage *buf, bool cached = false);
  CVI_S32 create(IVE *ive_instance);
  CVI_S32 free();
  CVI_S32 toFrame(VIDEO_FRAME_INFO_S *frame);
  CVI_S32 fromFrame(VIDEO_FRAME_INFO_S *frame);
  CVI_S32 write(const std::string &fname);
  CVI_U32 getHeight();
  CVI_U32 getWidth();
  CVI_S32 setZero(IVE *ive_instance);
  std::vector<CVI_U32> getStride();
  std::vector<CVI_U8 *> getVAddr();
  std::vector<CVI_U64> getPAddr();
  ImageType getType();

 private:
  std::shared_ptr<IVEImageImpl> mpImpl;
};

class IVE {
 public:
  IVE();
  ~IVE();
  IVE(const IVE &other) = delete;
  IVE &operator=(const IVE &other) = delete;

  CVI_S32 init();
  CVI_S32 destroy();
  CVI_U32 getAlignedWidth(uint32_t width);
  CVI_S32 dma(IVEImage *pSrc, IVEImage *pDst, DMAMode mode = DIRECT_COPY, CVI_U64 u64Val = 0,
              CVI_U8 u8HorSegSize = 0, CVI_U8 u8ElemSize = 0, CVI_U8 u8VerSegRows = 0);
  CVI_S32 sub(IVEImage *pSrc1, IVEImage *pSrc2, IVEImage *pDst, SubMode mode = ABS);
  CVI_S32 roi(IVEImage *pSrc, IVEImage *pDst, uint32_t x1, uint32_t x2, uint32_t y1, uint32_t y2);
  CVI_S32 andImage(IVEImage *pSrc1, IVEImage *pSrc2, IVEImage *pDst);
  CVI_S32 orImage(IVEImage *pSrc1, IVEImage *pSrc2, IVEImage *pDst);
  CVI_S32 erode(IVEImage *pSrc1, IVEImage *pDst, const std::vector<CVI_S32> &mask);
  CVI_S32 dilate(IVEImage *pSrc1, IVEImage *pDst, const std::vector<CVI_S32> &mask);
  CVI_S32 add(IVEImage *pSrc1, IVEImage *pSrc2, IVEImage *pDst, float alpha = 1.0,
              float beta = 1.0);
  CVI_S32 add(IVEImage *pSrc1, IVEImage *pSrc2, IVEImage *pDst,
              unsigned short alpha = std::numeric_limits<unsigned short>::max(),
              unsigned short beta = std::numeric_limits<unsigned short>::max());
  CVI_S32 fillConst(IVEImage *pSrc, float value);
  CVI_S32 thresh(IVEImage *pSrc, IVEImage *pDst, ThreshMode mode, CVI_U8 u8LowThr, CVI_U8 u8HighThr,
                 CVI_U8 u8MinVal, CVI_U8 u8MidVal, CVI_U8 u8MaxVal);
  CVI_S32 frame_diff(IVEImage *pSrc1, IVEImage *pSrc2, IVEImage *pDst, CVI_U8 threshold);
  IVEImpl *getImpl();

 private:
  std::shared_ptr<IVEImpl> mpImpl;
};

}  // namespace ive
