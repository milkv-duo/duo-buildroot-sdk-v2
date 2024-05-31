#ifndef _BM_RUNTIME_INTERNAL_H_
#define _BM_RUNTIME_INTERNAL_H_

#include <pthread.h>
#include <bmkernel/bm1822/bmkernel_1822.h>
#include <bmruntime.h>
#include <cvikernel/cvikernel.h>
#include "cvitpu_debug.h"
#include <bmkernel/bm_regcpu.h>
#include "bm_types.h"

#ifdef __cplusplus
	extern "C" {
#endif

bmerr_t cvi182x_dmabuf_size(uint8_t *cmdbuf, size_t sz, size_t *psize, size_t *pmu_size);
bmerr_t cvi182x_dmabuf_relocate(uint8_t *dmabuf, uint64_t dmabuf_devaddr, uint32_t original_size, uint32_t pmubuf_size);
bmerr_t cvi182x_dmabuf_convert(uint8_t *cmdbuf, size_t sz, uint8_t *dmabuf);
void cvi182x_dmabuf_dump(uint8_t * dmabuf);
void cvi182x_arraybase_set(uint8_t *dmabuf, uint32_t arraybase0L, uint32_t arraybase1L, uint32_t arraybase0H, uint32_t arraybase1H);
uint64_t cvi182x_get_pmusize(uint8_t * dmabuf);

uint32_t tpu_pmu_dump_main(uint8_t *v_dma_buf, uint64_t p_dma_buf);

#define TPU_PMUBUF_SIZE         (1024 * 1024 * 2)
#define TPU_DMABUF_HEADER_M     0xB5B5

#ifdef __cplusplus
}
#endif

#endif /* _BM_RUNTIME_INTERNAL_H_ */
