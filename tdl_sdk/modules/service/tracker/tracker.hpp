#pragma once
#include <map>
#include <string>
#include <utility>
#include <vector>
#include "cvi_comm.h"

namespace cvitdl {
namespace service {

typedef struct {
  float x;
  float y;
} tracker_pts_t;

class Tracker {
 public:
  int registerId(const CVI_U64 &timestamp, const int64_t &id, const float x, const float y);
  int getLatestPos(const int64_t &id, float *x, float *y);

 private:
  std::map<int64_t, std::vector<std::pair<CVI_U64, tracker_pts_t>>> m_tracker;
  CVI_U64 m_timestamp;
  CVI_U64 m_deleteduration = 3000;  // 3ms
};
}  // namespace service
}  // namespace cvitdl
