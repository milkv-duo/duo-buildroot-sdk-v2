/*
* Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
*/
#ifndef ROI_ALIGN_OP_H_
#define ROI_ALIGN_OP_H_

#include "tpuc/CustomOp.h"

namespace cvi {

class ROIAlignOp : public CustomOp {
public:
  ROIAlignOp(OpParam &param) : CustomOp(param) {}

  void interpretFp32(std::vector<std::shared_ptr<std::vector<float>>> &operand_tensors,
                     std::vector<std::vector<int64_t>> &operand_shapes,
                     std::shared_ptr<std::vector<float>> &result_tensor,
                     std::vector<int64_t> &result_shape);
};

} // namespace cvi
#endif
