#pragma once
#include <bitset>
#include "core/core/cvtdl_core_types.h"
#include "core/object/cvtdl_object_types.h"
#include "obj_detection.hpp"

namespace cvitdl {

class Yolov5 final : public DetectionBase {
 public:
  Yolov5();
  ~Yolov5();
  int inference(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_object_t *obj_meta) override;

  uint32_t set_roi(Point_t &roi);

 private:
  int onModelOpened() override;

  void generate_yolov5_proposals(Detections &vec_obj);
  void outputParser(const int image_width, const int image_height, const int frame_width,
                    const int frame_height, cvtdl_object_t *obj_meta);
  void Yolov5PostProcess(Detections &dets, int frame_width, int frame_height,
                         cvtdl_object_t *obj_meta);

  std::map<int, std::string> class_out_names_;
  std::map<int, std::string> conf_out_names_;
  std::map<int, std::string> box_out_names_;
  std::vector<int> strides_;
  cvtdl_bbox_t yolo_box;
  bool roi_flag = false;
};
}  // namespace cvitdl
