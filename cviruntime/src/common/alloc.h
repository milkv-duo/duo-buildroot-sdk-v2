#pragma once
#include "cviruntime_context.h"

namespace cvi {
namespace runtime {

CVI_RT_MEM cviMemAlloc(CVI_RT_HANDLE rt_handle, uint64_t size, CVI_ALLOC_TYPE type, const char * name);
void cviMemFree(CVI_RT_HANDLE rt_handle, CVI_RT_MEM mem);
CVI_RC cviSetMemCallback(CVI_MEM_ALLOC_CB mem_alloc, CVI_MEM_FREE_CB mem_free);
void cviResetMemCallback();

} // namespace runtime
} // namespace cvi