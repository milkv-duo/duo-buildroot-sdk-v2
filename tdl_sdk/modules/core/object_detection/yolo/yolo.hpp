#pragma once
#include <bitset>
#include "core/object/cvtdl_object_types.h"
#include "obj_detection.hpp"

namespace cvitdl {

class Yolo final : public DetectionBase {
 public:
  Yolo();
  ~Yolo(){};
  int inference(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_object_t *obj_meta) override;

 private:
  int onModelOpened() override;

  void clip_bbox(int frame_width, int frame_height, cvtdl_bbox_t *bbox);
  cvtdl_bbox_t Yolo_box_rescale(int frame_width, int frame_height, int width, int height,
                                cvtdl_bbox_t bbox);
  void getYoloDetections(int8_t *ptr_int8, float *ptr_float, int num_per_pixel, float qscale,
                         int det_num, int channel, Detections &vec_obj);

  int vpssPreprocess(VIDEO_FRAME_INFO_S *srcFrame, VIDEO_FRAME_INFO_S *dstFrame,
                     VPSSConfig &vpss_config) override;

  void outputParser(const int image_width, const int image_height, const int frame_width,
                    const int frame_height, cvtdl_object_t *obj_meta);
  void YoloPostProcess(Detections &dets, int frame_width, int frame_height,
                       cvtdl_object_t *obj_meta);
};
}  // namespace cvitdl
