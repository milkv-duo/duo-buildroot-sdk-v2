#include "raw_image_classification.hpp"
#include <core/core/cvtdl_errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <cmath>
#include <error_msg.hpp>
#include <iostream>
#include <iterator>
#include <numeric>
#include <string>
#include <vector>
#include "coco_utils.hpp"
#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem.h"
#include "core/cvi_tdl_types_mem_internal.h"
#include "core/utils/vpss_helper.h"
#include "core_utils.hpp"
#include "cvi_sys.h"

#define topK 5

namespace cvitdl {

RawImageClassification::RawImageClassification() : Core(CVI_MEM_DEVICE) {
  this->setRaw(true);
  float mean[3] = {123.675, 116.28, 103.52};
  float std[3] = {58.395, 57.12, 57.375};

  for (int i = 0; i < 3; i++) {
    m_preprocess_param[0].mean[i] = mean[i] / std[i];
    m_preprocess_param[0].factor[i] = 1.0 / std[i];
  }

  m_preprocess_param[0].format = PIXEL_FORMAT_RGB_888_PLANAR;
  m_preprocess_param[0].rescale_type = RESCALE_CENTER;
  m_preprocess_param[0].keep_aspect_ratio = true;
}

int RawImageClassification::onModelOpened() {
  if (getNumOutputTensor() != 1) {
    LOGE("RawImageClassification only expected 1 output branch!\n");
  }

  return CVI_TDL_SUCCESS;
}
std::vector<int> RawImageClassification::TopKIndex(std::vector<float> &vec, int topk) {
  std::vector<int> topKIndex;
  topKIndex.clear();

  std::vector<size_t> vec_index(vec.size());
  std::iota(vec_index.begin(), vec_index.end(), 0);

  std::sort(vec_index.begin(), vec_index.end(),
            [&vec](size_t index_1, size_t index_2) { return vec[index_1] > vec[index_2]; });

  int k_num = std::min<int>(vec.size(), topk);

  for (int i = 0; i < k_num; ++i) {
    topKIndex.emplace_back(vec_index[i]);
  }

  return topKIndex;
}

RawImageClassification::~RawImageClassification() {}

int RawImageClassification::vpssPreprocess(VIDEO_FRAME_INFO_S *srcFrame,
                                           VIDEO_FRAME_INFO_S *dstFrame, VPSSConfig &vpss_config) {
  auto &vpssChnAttr = vpss_config.chn_attr;
  auto &factor = vpssChnAttr.stNormalize.factor;
  auto &mean = vpssChnAttr.stNormalize.mean;
  vpssChnAttr.stNormalize.bEnable = false;
  vpssChnAttr.stAspectRatio.enMode = ASPECT_RATIO_NONE;

  VPSS_CHN_SQ_RB_HELPER(&vpssChnAttr, srcFrame->stVFrame.u32Width, srcFrame->stVFrame.u32Height,
                        vpssChnAttr.u32Width, vpssChnAttr.u32Height, PIXEL_FORMAT_RGB_888_PLANAR,
                        factor, mean, false);
  int ret = mp_vpss_inst->sendFrame(srcFrame, &vpssChnAttr, &vpss_config.chn_coeff, 1);
  if (ret != CVI_SUCCESS) {
    LOGE("vpssPreprocess Send frame failed: %s!\n", get_vpss_error_msg(ret));
    return CVI_TDL_ERR_VPSS_SEND_FRAME;
  }

  ret = mp_vpss_inst->getFrame(dstFrame, 0, m_vpss_timeout);
  if (ret != CVI_SUCCESS) {
    LOGE("get frame failed: %s!\n", get_vpss_error_msg(ret));
    return CVI_TDL_ERR_VPSS_GET_FRAME;
  }

  return CVI_TDL_SUCCESS;
}
// int dump_frame_result(const std::string &filepath, VIDEO_FRAME_INFO_S *frame) {
//   FILE *fp = fopen(filepath.c_str(), "wb");
//   if (fp == nullptr) {
//     printf("failed to open: %s.\n", filepath.c_str());
//     return CVI_FAILURE;
//   }

//   if (frame->stVFrame.pu8VirAddr[0] == NULL) {
//     size_t image_size =
//         frame->stVFrame.u32Length[0] + frame->stVFrame.u32Length[1] +
//         frame->stVFrame.u32Length[2];
//     frame->stVFrame.pu8VirAddr[0] =
//         (CVI_U8 *)CVI_SYS_Mmap(frame->stVFrame.u64PhyAddr[0], image_size);
//     frame->stVFrame.pu8VirAddr[1] = frame->stVFrame.pu8VirAddr[0] + frame->stVFrame.u32Length[0];
//     frame->stVFrame.pu8VirAddr[2] = frame->stVFrame.pu8VirAddr[1] + frame->stVFrame.u32Length[1];
//   }
//   for (int c = 0; c < 3; c++) {
//     uint8_t *paddr = (uint8_t *)frame->stVFrame.pu8VirAddr[c];
//     std::cout << "towrite channel:" << c << ",towritelen:" << frame->stVFrame.u32Length[c]
//               << ",addr:" << (void *)paddr << std::endl;
//     fwrite(paddr, frame->stVFrame.u32Length[c], 1, fp);
//   }
//   fclose(fp);
//   return CVI_SUCCESS;
// }
int RawImageClassification::inference(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_class_meta_t *cls_meta) {
  std::vector<VIDEO_FRAME_INFO_S *> frames = {srcFrame};

  if (srcFrame->stVFrame.u64PhyAddr[0] == 0 && hasSkippedVpssPreprocess()) {
    const TensorInfo &tinfo = getInputTensorInfo(0);
    int8_t *input_ptr = tinfo.get<int8_t>();
    printf("Copy during inference\n");
    memcpy(input_ptr, srcFrame->stVFrame.pu8VirAddr[0], srcFrame->stVFrame.u32Length[0]);
  }

  // dump_frame_result("img_cls_pre.bin",srcFrame);
  int ret = run(frames);
  if (ret != CVI_TDL_SUCCESS) {
    LOGE("RawImageClassification run inference failed\n");
    return ret;
  }

  outputParser(cls_meta);
  model_timer_.TicToc("post");
  return CVI_TDL_SUCCESS;
}

void RawImageClassification::outputParser(cvtdl_class_meta_t *cls_meta) {
  TensorInfo oinfo = getOutputTensorInfo(0);
  int8_t *ptr_int8 = static_cast<int8_t *>(oinfo.raw_pointer);
  uint8_t *ptr_uint8 = static_cast<uint8_t *>(oinfo.raw_pointer);
  float *ptr_float = static_cast<float *>(oinfo.raw_pointer);

  int channel_len = oinfo.shape.dim[1];

  int num_per_pixel = oinfo.tensor_size / oinfo.tensor_elem;
  float qscale = num_per_pixel == 1 ? oinfo.qscale : 1;

  std::vector<float> scores;
  // printf("output num:%d,datatype:%d\n",channel_len,oinfo.data_type);
  // int8 output tensor need to multiply qscale
  for (int i = 0; i < channel_len; i++) {
    if (num_per_pixel == 1) {
      scores.push_back(ptr_int8[i] * qscale);
    } else {
      scores.push_back(ptr_float[i]);
    }
    printf("scales:%f\n", scores[i]);
  }

  std::vector<int> topKIndex = TopKIndex(scores, topK);

  // parse topK classes and scores
  for (uint32_t i = 0; i < topKIndex.size(); ++i) {
    cls_meta->cls[i] = topKIndex[i];
    cls_meta->score[i] = scores[topKIndex[i]];
  }
}
// namespace cvitdl
}  // namespace cvitdl
