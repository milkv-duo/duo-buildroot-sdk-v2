#pragma once
#include "mot_base.hpp"

#include <map>
#include <set>
#include <vector>
#include "cvi_tdl.h"

typedef struct {
  TARGET_TYPE_e target_type;
  bool enable_DeepSORT;
  char *mot_data_path;
} MOT_EVALUATION_ARGS_t;

typedef struct {
  uint32_t stable_id_num;
  double total_entropy;
  double coverage_rate;
  double score;
} MOT_Performance_t;

CVI_S32 RUN_MOT_EVALUATION(cvitdl_handle_t tdl_handle, const MOT_EVALUATION_ARGS_t &args,
                           MOT_Performance_t &performance, bool output_result, char *result_path);

class MOT_Evaluation {
 public:
  MOT_Evaluation();
  ~MOT_Evaluation();
  CVI_S32 update(const cvtdl_tracker_t &trackers, const cvtdl_tracker_t &inact_trackers);
  void summary(MOT_Performance_t &performance);

  std::set<uint64_t> stable_id;
  std::set<uint64_t> alive_stable_id;
  std::map<uint64_t, std::pair<uint32_t, uint32_t>> tracking_performance;
  double entropy = 0.0;
  uint32_t new_id_counter = 0;
  uint32_t bbox_counter = 0;
  uint32_t stable_tracking_counter = 0;
  uint32_t time_counter = 0;
};

class GridIndexGenerator {
 public:
  GridIndexGenerator() = delete;
  GridIndexGenerator(const std::vector<uint32_t> &r);
  ~GridIndexGenerator();
  CVI_S32 next(std::vector<uint32_t> &next_idx);

  uint32_t counter = 0;
  std::vector<uint32_t> ranges;
  std::vector<uint32_t> idx;
};
