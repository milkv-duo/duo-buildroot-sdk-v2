#pragma once
#include "core.hpp"
#include "core/object/cvtdl_object_types.h"
#include "cvi_comm.h"
#include "decode_tool.hpp"

#include "lp_recognition_base.hpp"
#include "opencv2/core.hpp"
namespace cvitdl {

/* WPODNet */
class LicensePlateRecognition final : public LicensePlateRecognitionBase {
 public:
  LicensePlateRecognition(LP_FORMAT format);
  ~LicensePlateRecognition(){};
  int inference(VIDEO_FRAME_INFO_S *frame, cvtdl_object_t *vehicle_plate_meta) override;

 private:
  void prepareInputTensor(cv::Mat &input_mat);
  LP_FORMAT format;
  int lp_height, lp_width;
};
}  // namespace cvitdl