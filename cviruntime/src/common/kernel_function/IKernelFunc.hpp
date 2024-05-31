#include <sys/mman.h>
#include <unistd.h>
#include <dlfcn.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cmath>
#include <functional>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <queue>
#include <vector>
#include <stdlib.h>
#include <string.h>
#include <runtime/debug.h>
#include <runtime/kernel_function.hpp>
#include "cviruntime_extra.h"

namespace cvi {
namespace runtime {

class IKernelFunc {
public:
  IKernelFunc(CVI_RT_HANDLE ctx) : ctx(ctx) {}
  virtual ~IKernelFunc() {
    if (cmdbuf_mem)
      CVI_RT_MemFree(ctx, cmdbuf_mem);
  }

  CVI_RT_HANDLE ctx;
  CVI_RT_MEM cmdbuf_mem = nullptr;
};

}
}