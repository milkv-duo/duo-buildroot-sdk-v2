#pragma once
#include "core/face/cvtdl_face_types.h"
#include "core_internel.hpp"
#include "cvi_comm.h"

namespace cvitdl {
typedef struct HeadInfo {
  std::string cls_layer;
  std::string dis_layer;
  int stride;
} HeadInfo;

class IncarObjectDetection final : public Core {
 public:
  IncarObjectDetection();
  int inference(VIDEO_FRAME_INFO_S* frame, cvtdl_face_t* meta);
  std::vector<HeadInfo> heads_info{
      // cls_pred|dis_pred|stride
      {"802_Transpose_dequant", "805_Transpose_dequant", 8},
      {"830_Transpose_dequant", "833_Transpose_dequant", 16},
      {"858_Transpose_dequant", "861_Transpose_dequant", 32},

  };

 private:
  void outputParser(int image_width, int image_height, cvtdl_face_t* meta);
  void decode_infer(float* cls_pred, float* dis_pred, int stride, float threshold,
                    std::vector<std::vector<cvtdl_dms_od_info_t>>& results);
  void disPred2Bbox(std::vector<std::vector<cvtdl_dms_od_info_t>>& results, const float*& dfl_det,
                    int label, float score, int x, int y, int stride);
  template <typename _Tp>
  int activation_function_softmax(const _Tp* src, _Tp* dst, int length);
  inline float fast_exp(float x);

  int input_size = 320;
  int num_class = 3;
  int reg_max = 7;
  char class_name[3][32] = {"cell phone", "bottle", "cup"};
};
}  // namespace cvitdl
