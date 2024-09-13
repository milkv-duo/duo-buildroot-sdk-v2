#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <cmath>
#include <iterator>

#include <core/core/cvtdl_errno.h>
#include <error_msg.hpp>
#include <iostream>
#include "coco_utils.hpp"
#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem.h"
#include "core/cvi_tdl_types_mem_internal.h"
#include "core/utils/vpss_helper.h"
#include "core_utils.hpp"
#include "cvi_sys.h"
#include "face_landmark_det3.hpp"

namespace cvitdl {

FaceLandmarkDet3::FaceLandmarkDet3() : Core(CVI_MEM_DEVICE) {
  m_preprocess_param[0].factor[0] = 0.0078125;
  m_preprocess_param[0].factor[1] = 0.0078125;
  m_preprocess_param[0].factor[2] = 0.0078125;
  m_preprocess_param[0].mean[0] = 0.99609375;
  m_preprocess_param[0].mean[1] = 0.99609375;
  m_preprocess_param[0].mean[2] = 0.99609375;
  m_preprocess_param[0].format = PIXEL_FORMAT_RGB_888_PLANAR;
}

int FaceLandmarkDet3::onModelOpened() {
  CVI_SHAPE shape = getInputShape(0);
  LOGI("FaceLandmarkDet3::onModelOpened,input shape:%d,%d,%d,%d\n", shape.dim[0], shape.dim[1],
       shape.dim[2], shape.dim[3]);
  for (size_t j = 0; j < getNumOutputTensor(); j++) {
    CVI_SHAPE oj = getOutputShape(j);
    TensorInfo oinfo = getOutputTensorInfo(j);
    printf("output:%s,dim:%d,%d,%d,%d\n", oinfo.tensor_name.c_str(), oj.dim[0], oj.dim[1],
           oj.dim[2], oj.dim[3]);
    int channel = oj.dim[1];
    if (channel == 2) {
      out_names_["score"] = oinfo.tensor_name;
      LOGI("parse score branch output:%s\n", oinfo.tensor_name.c_str());
    } else if (channel == 10) {
      out_names_["point"] = oinfo.tensor_name;
      LOGI("parse point output:%s\n", oinfo.tensor_name.c_str());
    }
  }
  if (out_names_.count("score") == 0 || out_names_.count("point") == 0) {
    return CVI_TDL_FAILURE;
  }
  return CVI_TDL_SUCCESS;
}

FaceLandmarkDet3::~FaceLandmarkDet3() {}

int FaceLandmarkDet3::inference(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_face_t *facemeta) {
  std::vector<VIDEO_FRAME_INFO_S *> frames = {srcFrame};

  // if(facemeta->size == 1){
  //     m_vpss_config[0].crop_attr.enCropCoordinate = VPSS_CROP_RATIO_COOR;
  //     int x1 = facemeta
  //     m_vpss_config[0].crop_attr.stCropRect = {box_x1, box_y1, box_w, box_h};
  // }

  int ret = run(frames);
  if (ret != CVI_TDL_SUCCESS) {
    LOGW("FaceLandmarkDet3 run inference failed\n");
    return ret;
  }
  CVI_SHAPE shape = getInputShape(0);

  outputParser(shape.dim[3], shape.dim[2], srcFrame->stVFrame.u32Width,
               srcFrame->stVFrame.u32Height, facemeta);
  model_timer_.TicToc("post");
  return CVI_TDL_SUCCESS;
}

void FaceLandmarkDet3::outputParser(const int image_width, const int image_height,
                                    const int frame_width, const int frame_height,
                                    cvtdl_face_t *facemeta) {
  TensorInfo oinfo = getOutputTensorInfo(out_names_["point"]);
  float *output_point = getOutputRawPtr<float>(oinfo.tensor_name);

  TensorInfo oinfo_cls = getOutputTensorInfo(out_names_["score"]);
  float *output_score = getOutputRawPtr<float>(oinfo_cls.tensor_name);
  // printf("to output parse,name:%s,addr:%p,name:%s,addr:%p\n", oinfo.tensor_name.c_str(),
  //        (void *)output_point, oinfo_cls.tensor_name.c_str(), (void *)output_score);
  float score = output_score[1];
  float ratio_height = image_width / (float)frame_height;
  float ratio_width = image_height / (float)frame_width;
  float ratio = 1;
  float pad_width, pad_height;
  if (ratio_height > ratio_width) {
    ratio = 1.0 / ratio_width;
    pad_width = 0;
    pad_height = (image_height - frame_height * ratio_width) / 2;
  } else {
    ratio = 1.0 / ratio_height;
    pad_width = (image_width - frame_width * ratio_height) / 2;
    pad_height = 0;
  }

  CVI_TDL_MemAllocInit(1, 5, facemeta);
  facemeta->width = frame_width;
  facemeta->height = frame_height;
  facemeta->info[0].pts.score = score;

  // printf("padw:%.3f,padh:%.3f,ratio:%.3f,score:%.3f\n",pad_width,pad_height,ratio,score);

  for (int i = 0; i < 5; i++) {
    float x = (output_point[i] * image_width - pad_width) * ratio;
    float y = (output_point[i + 5] * image_height - pad_height) * ratio;
    facemeta->info[0].pts.x[i] = x;
    facemeta->info[0].pts.y[i] = y;
  }
}
// namespace cvitdl
}  // namespace cvitdl
