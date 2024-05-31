/*
* Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
*/
#include <runtime/cpu_function.hpp>
#include "SoftmaxOpRuntime.hpp"
#include "ROIAlignOpRuntime.hpp"

REGISTER_OP_RUNTIME_FUNCS(
  {(char *)"mysoftmax", SoftmaxOpRuntime::open},
  {(char *)"roialign", ROIAlignOpRuntime::open}
  // add more custom op runtime func here.
);
