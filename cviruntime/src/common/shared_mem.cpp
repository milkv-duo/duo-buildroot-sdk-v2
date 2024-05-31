#include <inttypes.h>
#include <list>
#include <mutex>
#include <iostream>
#include <sstream>
#include <runtime/debug.h>
#include <runtime/stream.hpp>
#include "cviruntime_context.h"
#include "alloc.h"

namespace cvi {
namespace runtime {

static std::mutex gMutexLock;
static std::list<CVI_RT_MEM> gSharedMemList;
static size_t gMaxSharedMemSize = 0;

void setSharedMemSize(size_t size) {
  #define PAGESIZE 4096
  const std::lock_guard<std::mutex> lock(gMutexLock);
  uint32_t mask = PAGESIZE - 1;
  size = (size + mask) & (~mask);
  gMaxSharedMemSize = std::max(gMaxSharedMemSize, size);
}

CVI_RT_MEM allocateSharedMemory(CVI_RT_HANDLE ctx, size_t size) {
  const std::lock_guard<std::mutex> lock(gMutexLock);
  size = std::max(gMaxSharedMemSize, size);
  // check if a mem in shared list is big enough.
  for (auto &mem : gSharedMemList) {
    if (CVI_RT_MemGetSize(mem) >= size) {
      TPU_LOG_DEBUG("find shared memory(%" PRIu64 "),  saved:%zu \n",
                    CVI_RT_MemGetSize(mem), size);
      CVI_RT_MemIncRef(mem);
      return mem;
    }
  }

  // if no availabel stored mem, create a new one
  // and insert it in sharedMemList.
  CVI_RT_MEM mem = cviMemAlloc(ctx, size, CVI_ALLOC_SHARED, "SharedMemory");
  if (!mem) {
    return nullptr;
  }
  CVI_RT_MemIncRef(mem);
  if (gSharedMemList.empty()) {
    gSharedMemList.push_back(mem);
  } else {
    for (auto it = gSharedMemList.begin();
        it != gSharedMemList.end();
        ++it) {
      if (CVI_RT_MemGetSize(*it) < size) {
        gSharedMemList.insert(it, mem);
        break;
      }
    }
  }
  return mem;
}

void deallocateSharedMemory(CVI_RT_HANDLE ctx, CVI_RT_MEM mem) {
  const std::lock_guard<std::mutex> lock(gMutexLock);
  for (auto candidate : gSharedMemList) {
    if (candidate == mem) {
      // if ref drops to 0, free it.
      if (CVI_RT_MemDecRef(mem) == 0) {
        gSharedMemList.remove(mem);
        cviMemFree(ctx, mem);
      }
      // otherwise, do nothing.
      return;
    }
  }
  // not a shared mem, free it directly.
  cviMemFree(ctx, mem);
}

} // namespace runtime
} // namespace cvi

