#pragma once
#include "core/object/cvtdl_object_types.h"

#include <deque>
#include <iostream>
#include <queue>

#define HISTORY_UPDATE 30

template <typename T, int MaxLen, typename Container = std::deque<T>>
class FixedQueue : public std::queue<T, Container> {
 public:
  void push(const T& value) {
    if (this->size() == MaxLen) {
      this->c.pop_front();
    }
    std::queue<T, Container>::push(value);
  }
};

class FallMD {
 public:
  FallMD();
  int detect(cvtdl_object_t* meta);

 private:
  FixedQueue<float, HISTORY_UPDATE> history_q_extra_pred_x;
  FixedQueue<float, HISTORY_UPDATE> history_q_extra_pred_y;
  FixedQueue<float, HISTORY_UPDATE> history_bbox_x1;
  FixedQueue<float, HISTORY_UPDATE> history_bbox_x2;
  FixedQueue<float, HISTORY_UPDATE> history_bbox_y1;
  FixedQueue<float, HISTORY_UPDATE> history_bbox_y2;
  bool isFall;
};
