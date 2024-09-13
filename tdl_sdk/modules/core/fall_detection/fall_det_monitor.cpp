
#include "fall_det_monitor.hpp"
#include <map>
#include <vector>
#include "core/core/cvtdl_core_types.h"
#include "core/core/cvtdl_errno.h"

FallDetMonitor::FallDetMonitor() {}

int FallDetMonitor::set_fps(float fps) {
  FPS = fps;
  return CVI_TDL_SUCCESS;
}

int FallDetMonitor::monitor(cvtdl_object_t* obj_meta) {
  std::map<int, int> track_index;
  std::vector<int> new_index;
  for (uint32_t i = 0; i < obj_meta->size; i++) {
    if (obj_meta->info[i].track_state == cvtdl_trk_state_type_t::CVI_TRACKER_NEW) {
      new_index.push_back(i);

    } else {
      track_index[obj_meta->info[i].unique_id] = i;
    }

#ifdef DEBUG_FALL
    printf("unique_id: %d, track_state: %d\n", obj_meta->info[i].unique_id,
           obj_meta->info[i].track_state);
#endif
  }
#ifdef DEBUG_FALL
  printf("0 muti_person size: %d\n", muti_person.size());
#endif
  for (auto it = muti_person.begin(); it != muti_person.end();) {
    if (track_index.count(it->uid) == 0) {
      it->unmatched_times += 1;
#ifdef DEBUG_FALL
      printf("it->unmatched_times += 1\n");
#endif
      if (it->unmatched_times == it->MAX_UNMATCHED_TIME) {
        it = muti_person.erase(it);
      } else {
        it->update_queue(it->valid_list, 0);
        it++;
      }

    } else {
#ifdef DEBUG_FALL
      printf("matched, start to detect\n ");
#endif
      it->detect(&obj_meta->info[track_index[it->uid]], FPS);
      it->unmatched_times = 0;
      it++;
    }
  }

  for (uint32_t i = 0; i < new_index.size(); i++) {
#ifdef DEBUG_FALL
    printf("new_index.size(): %d\n", new_index.size());
#endif
    FallDet person(obj_meta->info[new_index[i]].unique_id);

    person.detect(&obj_meta->info[new_index[i]], FPS);

    muti_person.push_back(person);
  }
#ifdef DEBUG_FALL
  printf("1 muti_person size: %d\n", muti_person.size());
#endif
  return CVI_TDL_SUCCESS;
}