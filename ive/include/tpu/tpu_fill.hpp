#pragma once
#include "core.hpp"

class IveTPUConstFill {
 public:
  static int run(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, const float value,
                 std::vector<CviImg *> &output);
};