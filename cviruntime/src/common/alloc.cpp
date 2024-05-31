#include <mutex>
#include <functional>
#include "alloc.h"
#include "runtime/debug.h"

#define UNUSED(x) (void)(x)

namespace cvi {
namespace runtime {

static CVI_RT_MEM cvi_def_mem_alloc(CVI_RT_HANDLE rt_handle, uint64_t size,
                                    CVI_ALLOC_TYPE type, const char *name) {
    (void)type;
    (void)name;
    return CVI_RT_MemAlloc(rt_handle, size);
}

static CVI_MEM_ALLOC_CB mem_alloc_cb = cvi_def_mem_alloc;
static CVI_MEM_FREE_CB mem_free_cb = CVI_RT_MemFree;
static std::mutex gMutex;

CVI_RT_MEM cviMemAlloc(CVI_RT_HANDLE rt_handle, uint64_t size, CVI_ALLOC_TYPE type, const char *name) {
    return mem_alloc_cb(rt_handle, size, type, name);
}

void cviMemFree(CVI_RT_HANDLE rt_handle, CVI_RT_MEM mem) {
    return mem_free_cb(rt_handle, mem);
}

CVI_RC cviSetMemCallback(CVI_MEM_ALLOC_CB mem_alloc, CVI_MEM_FREE_CB mem_free) {
    std::unique_lock<std::mutex> lk(gMutex);
    if (!mem_alloc) {
        TPU_LOG_ERROR("CVI_MEM_ALLOC_CB is null\n");
        return -1;
    }
    if (!mem_free) {
        TPU_LOG_ERROR("CVI_MEM_FREE_CB is null\n");
        return -1;
    }
    mem_alloc_cb = mem_alloc;
    mem_free_cb = mem_free;
    return 0;
}

void cviResetMemCallback() {
    std::unique_lock<std::mutex> lk(gMutex);
    mem_alloc_cb = cvi_def_mem_alloc;
    mem_free_cb = CVI_RT_MemFree;
}

} // namespace runtime
} // namespace cvi
