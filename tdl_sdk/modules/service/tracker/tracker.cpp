#include "tracker.hpp"
#include <string.h>
#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem.h"
#include "core/cvi_tdl_types_mem_internal.h"

namespace cvitdl {
namespace service {
int Tracker::registerId(const CVI_U64 &timestamp, const int64_t &id, const float x, const float y) {
  m_timestamp = timestamp;
  tracker_pts_t pts;
  pts.x = x;
  pts.y = y;
  m_tracker[id].push_back({m_timestamp, pts});

  auto it = m_tracker.begin();
  while (it != m_tracker.end()) {
    auto &vec = it->second;
    if ((m_timestamp - vec[vec.size() - 1].first) > m_deleteduration) {
      it = m_tracker.erase(it);
    } else {
      ++it;
    }
  }
  return CVI_TDL_SUCCESS;
}
int Tracker::getLatestPos(const int64_t &id, float *x, float *y) {
  auto it = m_tracker.find(id);
  if (it != m_tracker.end()) {
    auto &vec = it->second;
    *x = vec[vec.size() - 1].second.x;
    *y = vec[vec.size() - 1].second.y;
    return CVI_TDL_SUCCESS;
  }
  return CVI_TDL_FAILURE;
}
}  // namespace service
}  // namespace cvitdl