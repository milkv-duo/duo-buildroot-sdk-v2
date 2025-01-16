#pragma once
#include "core.hpp"
#include "core/object/cvtdl_object_types.h"
#include "cvi_comm.h"

namespace cvitdl {
class TopformerSeg final : public Core {
 public:
  TopformerSeg();
  virtual ~TopformerSeg();
  int inference(VIDEO_FRAME_INFO_S *frame, cvtdl_seg_t *filter);
  void setDownRato(int down_rato);
  virtual bool allowExportChannelAttribute() const override { return true; }

 private:
  int vpssPreprocess(VIDEO_FRAME_INFO_S *srcFrame, VIDEO_FRAME_INFO_S *dstFrame,
                     VPSSConfig &vpss_config) override;

  void nearestNeighborInterpolation(cvtdl_seg_t *filter, float *out_id, float *out_conf);

  int outputParser(cvtdl_seg_t *filter);

  int oriW, oriH;
  int outW, outH;
  int preW, preH;
  int outShapeH, outShapeW;
  int downRato;
};

}  // namespace cvitdl