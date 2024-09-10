#pragma once

#include "cvi_deepsort_types_internal.hpp"

class Tracker {
 public:
  uint64_t id;
  int class_id;
  BBOX bbox; /* format: top-left(x, y), width, height */
};
