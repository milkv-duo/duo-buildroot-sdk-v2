#ifndef RUNTIME_SHARED_MEM_H
#define RUNTIME_SHARED_MEM_H

#include <cviruntime_context.h>

namespace cvi {
namespace runtime {

void setSharedMemSize(size_t size);
CVI_RT_MEM allocateSharedMemory(CVI_RT_HANDLE ctx, size_t size);
void deallocateSharedMemory(CVI_RT_HANDLE ctx, CVI_RT_MEM mem);

} // namespace runtime
} // namespace cvi

#endif
