#pragma once
#include "fall_det.hpp"

// obj->info[i].track_state = cvtdl_trk_state_type_t::CVI_TRACKER_NEW;

class FallDetMonitor {
 public:
  FallDetMonitor();
  int monitor(cvtdl_object_t* obj_meta);
  int set_fps(float fps);

 private:
  std::vector<FallDet> muti_person;
  float FPS = 21.0;
};