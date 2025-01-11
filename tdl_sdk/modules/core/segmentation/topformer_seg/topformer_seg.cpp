#include "topformer_seg.hpp"

#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem.h"
#include "core/utils/vpss_helper.h"
#include "core_utils.hpp"
#include "cvi_sys.h"
#include "error_msg.hpp"

#define R_SCALE 0.01712475
#define G_SCALE 0.0175070
#define B_SCALE 0.0174291
#define NAME_SCORE 0

namespace cvitdl {
TopformerSeg::TopformerSeg() : Core(CVI_MEM_DEVICE)  {
  m_preprocess_param[0].factor[0] = R_SCALE;
  m_preprocess_param[0].factor[1] = G_SCALE;
  m_preprocess_param[0].factor[2] = B_SCALE;

  m_preprocess_param[0].mean[0] = 2.117903;
  m_preprocess_param[0].mean[1] = 2.035714;
  m_preprocess_param[0].mean[2] = 1.804444;

  m_preprocess_param[0].keep_aspect_ratio = true;
  m_preprocess_param[0].rescale_type = RESCALE_RB;
  oriW = 0;
  oriH = 0;
  outW = 0;
  outH = 0;
  preW = 0;
  preH = 0;
  outShapeH = 0;
  outShapeW = 0;
  downRato = 16;
}
TopformerSeg::~TopformerSeg() {}
int TopformerSeg::vpssPreprocess(VIDEO_FRAME_INFO_S* srcFrame, VIDEO_FRAME_INFO_S* dstFrame,
                                 VPSSConfig& vpss_config) {
  auto& vpssChnAttr = vpss_config.chn_attr;
  auto& factor = vpssChnAttr.stNormalize.factor;
  auto& mean = vpssChnAttr.stNormalize.mean;
  VPSS_CHN_SQ_RB_HELPER(&vpssChnAttr, srcFrame->stVFrame.u32Width, srcFrame->stVFrame.u32Height,
                        vpssChnAttr.u32Width, vpssChnAttr.u32Height, PIXEL_FORMAT_RGB_888_PLANAR,
                        factor, mean, false);
  // std::cout<<"vpss_config.chn_coeff:"<<vpss_config.chn_coeff<<std::endl;
  int ret = mp_vpss_inst->sendFrame(srcFrame, &vpssChnAttr, &vpss_config.chn_coeff, 1);
  if (ret != CVI_SUCCESS) {
    LOGE("vpssPreprocess Send frame failed: %s!\n", get_vpss_error_msg(ret));
    return CVI_TDL_ERR_ALLOC_ION_FAIL;
  }

  ret = mp_vpss_inst->getFrame(dstFrame, 0, m_vpss_timeout);
  if (ret != CVI_SUCCESS) {
    LOGE("get frame failed: %s!\n", get_vpss_error_msg(ret));
    return CVI_TDL_ERR_VPSS_GET_FRAME;
  }

  return CVI_TDL_SUCCESS;
}

int TopformerSeg::inference(VIDEO_FRAME_INFO_S* frame, cvtdl_seg_t* filter) {
  std::vector<VIDEO_FRAME_INFO_S*> frames = {frame};
  filter->srcWidth = frame->stVFrame.u32Width;
  filter->srcHeight = frame->stVFrame.u32Height;
  int ret = run(frames);
  if (ret != CVI_TDL_SUCCESS) {
    return ret;
  }

  oriW = filter->srcWidth;
  oriH = filter->srcHeight;
  outputParser(filter);
  model_timer_.TicToc("post");
  return CVI_TDL_SUCCESS;
}

int TopformerSeg::outputParser(cvtdl_seg_t* filter) {
  float* out_id = getOutputRawPtr<float>(0);
  float* out_conf = getOutputRawPtr<float>(1);

  CVI_SHAPE output_shape = getOutputShape(0);
  outShapeH = output_shape.dim[1];
  outShapeW = output_shape.dim[2];

  float oriWHRato = static_cast<float>(oriW) / static_cast<float>(oriH);
  float preWHRato =
      static_cast<float>(output_shape.dim[2]) / static_cast<float>(output_shape.dim[1]);

  if (preWHRato > oriWHRato) {
    preW = std::ceil(output_shape.dim[1] * oriWHRato);
    preH = output_shape.dim[1];
  } else {
    preW = output_shape.dim[2];
    preH = std::ceil(output_shape.dim[2] / oriWHRato);
  }

  outW = std::ceil(static_cast<float>(filter->srcWidth) / downRato);
  outH = std::ceil(static_cast<float>(filter->srcHeight) / downRato);

  nearestNeighborInterpolation(filter, out_id, out_conf);
  return CVI_TDL_SUCCESS;
}
void TopformerSeg::setDownRato(int down_rato) { downRato = down_rato; }

void TopformerSeg::nearestNeighborInterpolation(cvtdl_seg_t* filter, float* out_id,
                                                float* out_conf) {
  filter->class_id = (uint8_t*)malloc(outH * outW * sizeof(uint8_t));
  filter->class_conf = (uint8_t*)malloc(outH * outW * sizeof(uint8_t));
  uint8_t* class_id = filter->class_id;
  uint8_t* class_conf = filter->class_conf;

  float scale_H = static_cast<float>(preH - 1) / static_cast<float>(outH - 1);
  float scale_W = static_cast<float>(preW - 1) / static_cast<float>(outW - 1);
  int* srcX_s = new int[outH];
  int* srcY_s = new int[outW];

  uint8_t* cast_out_id = new uint8_t[outShapeH * outShapeW];
  uint8_t* cast_out_conf = new uint8_t[outShapeH * outShapeW];
  for (int x = 0; x < outShapeH; ++x) {
    for (int y = 0; y < outShapeW; ++y) {
      cast_out_id[x * outShapeW + y] = static_cast<uint8_t>(out_id[x * outShapeW + y]);
      cast_out_conf[x * outShapeW + y] = static_cast<uint8_t>(out_conf[x * outShapeW + y]);
    }
  }

  for (int x = 0; x < outH; ++x) {
    srcX_s[x] = static_cast<int>((x + 0.5) * scale_H);
  }
  for (int y = 0; y < outW; ++y) {
    srcY_s[y] = static_cast<int>((y + 0.5) * scale_W);
  }
  for (int x = 0; x < outH; ++x) {
    int srcX = srcX_s[x];
    for (int y = 0; y < outW; ++y) {
      int srcY = srcY_s[y];
      class_id[x * outW + y] = cast_out_id[srcX * outShapeW + srcY];
      class_conf[x * outW + y] = cast_out_conf[srcX * outShapeW + srcY];
    }
  }
  delete[] cast_out_id;
  delete[] cast_out_conf;
  delete[] srcX_s;
  delete[] srcY_s;
}

}  // namespace cvitdl
