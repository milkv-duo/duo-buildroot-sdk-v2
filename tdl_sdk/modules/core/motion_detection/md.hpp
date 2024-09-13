#pragma once
#include <memory>
#include "core/core/cvtdl_core_types.h"
#include "cvi_comm.h"
#include "ive.hpp"
#include "profiler.hpp"

struct Padding {
  uint32_t left;
  uint32_t top;
  uint32_t right;
  uint32_t bottom;
};

class MotionDetection {
 public:
  MotionDetection() = delete;
  MotionDetection(ive::IVE *ive_instance);
  ~MotionDetection();

  CVI_S32 init(VIDEO_FRAME_INFO_S *init_frame);
  CVI_S32 detect(VIDEO_FRAME_INFO_S *frame, std::vector<std::vector<float>> &objs,
                 uint8_t threshold, double min_area);
  CVI_S32 update_background(VIDEO_FRAME_INFO_S *frame);
  CVI_S32 set_roi(MDROI_t *_roi_s);
  ive::IVE *get_ive_instance() { return ive_instance; }

 private:
  CVI_S32 construct_images(VIDEO_FRAME_INFO_S *init_frame);
  void free_all();

  CVI_S32 copy_image(VIDEO_FRAME_INFO_S *srcframe, ive::IVEImage *dst);
  ive::IVE *ive_instance;
  void *p_ccl_instance = NULL;
  ive::IVEImage background_img;
  ive::IVEImage md_output;
  ive::IVEImage tmp_cpy_img_;
  ive::IVEImage tmp_src_img_;
  uint32_t im_width;
  uint32_t im_height;
  MDROI_t roi_s;

  Padding m_padding;
  bool use_roi_ = false;
  Timer md_timer_;
};
