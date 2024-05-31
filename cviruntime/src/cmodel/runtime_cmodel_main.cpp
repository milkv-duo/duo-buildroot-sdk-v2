#include "string.h"
#include <memory>
#include <bmkernel/bm1822/bm1822_tpu_cfg.h>
#include <bmkernel/bm1822/bmkernel_1822.h>
#include <bmkernel/bm1880v2/bm1880v2_tpu_cfg.h>
#include <bmkernel/bm1880v2/bmkernel_1880v2.h>
#include <bmkernel/reg_tiu.h>
#include <bmruntime.h>
#include <cvikernel/cvikernel.h>
#include <cvikernel/cv181x/cv181x_tpu_cfg.h>
#include <cvikernel/cv180x/cv180x_tpu_cfg.h>

#include "cviruntime_context.h"
#include "cvitpu_debug.h"
#include "runtime_cmodel_internal.h"
#include <bmruntime_bmkernel.h>
#include "cmodel_cmdbuf_180x.h"
#include "cmodel_cmdbuf_181x.h"
#include "cmodel_cmdbuf_182x.h"
#include "cmodel_cmdbuf_183x.h"

using std::hex;
using std::showbase;

#define DEVICE_INDEX_NUM 0
#define SUBMIT_MAGIC 0x12345678
typedef struct _cvi_rt_submit {
  cvk_context_t cvk_ctx;
  bmctx_t rt_ctx;
  uint8_t *cmdbuf;
  uint32_t magic;
} cvi_rt_submit;

static bmdev_t g_device = nullptr;
static int g_device_ref = 0;
static char *g_run_chip = nullptr;
static std::unique_ptr<CModelCmdbuf> g_cmodel_cmdbuf_183x(new CModelCmdbuf183x());
static std::unique_ptr<CModelCmdbuf> g_cmodel_cmdbuf_182x(new CModelCmdbuf182x());
static std::unique_ptr<CModelCmdbuf> g_cmodel_cmdbuf_181x(new CModelCmdbuf181x());
static std::unique_ptr<CModelCmdbuf> g_cmodel_cmdbuf_180x(new CModelCmdbuf180x());

static inline CModelCmdbuf *getCmdbufPtr(const char *chip_ver) {
  if (!strcmp(chip_ver, CVI_TPU_VERSION_183X)) {
    return g_cmodel_cmdbuf_183x.get();
  } else if (!strcmp(chip_ver, CVI_TPU_VERSION_182X)) {
    return g_cmodel_cmdbuf_182x.get();
  } else if (!strcmp(chip_ver, CVI_TPU_VERSION_181X)) {
    return g_cmodel_cmdbuf_181x.get();
  } else if (!strcmp(chip_ver, CVI_TPU_VERSION_180X)) {
    return g_cmodel_cmdbuf_180x.get();
  } else {
    assert(0);
  }
}

static inline CModelCmdbuf *getCmdbufPtr(uint32_t chip_ver) {
  if (chip_ver == BM1880V2_VER) {
    return g_cmodel_cmdbuf_183x.get();
  } else if (chip_ver == BM1822_VER) {
    return g_cmodel_cmdbuf_182x.get();
  } else if (chip_ver == CV181X_VER) {
    return g_cmodel_cmdbuf_181x.get();
  } else if (chip_ver == CV180X_VER) {
    return g_cmodel_cmdbuf_180x.get();
  } else {
    assert(0);
  }
}

bmerr_t bm_device_open(int index, bmdev_t *dev) {

  if (!g_run_chip) {
    g_run_chip = getenv("SET_CHIP_NAME");
    if (!g_run_chip) {
      TPU_LOG_WARNING("Please export SET_CHIP_NAME=%s/%s/%s/%s\n",
                      CVI_TPU_VERSION_183X, CVI_TPU_VERSION_182X, CVI_TPU_VERSION_181X, CVI_TPU_VERSION_180X);
      return BM_ERR_FAILURE;
    }
    TPU_LOG_INFO("Start TPU Simulator for %s\n", g_run_chip);
  }

  if (g_device) {
    g_device_ref++;
    *dev = g_device;
    return BM_SUCCESS;
  }

  getCmdbufPtr(g_run_chip)->rt_device_open(index, &g_device);
  *dev = g_device;
  g_device_ref++;

  return BM_SUCCESS;
}

void bm_device_set_base_reg(bmctx_t ctx, u32 inx, uint64_t addr) {
  bm_cmodel_set_base_reg(ctx->dev->model, inx, addr);
}

uint64_t bm_device_read_base_reg(bmctx_t ctx, u32 inx) {
  return bm_cmodel_read_base_reg(ctx->dev->model, inx);
}

void bm_device_close(bmdev_t dev) {
  assert(dev == g_device);
  if (--g_device_ref > 0) {
    return;
  }
  mem_pool_destroy(dev->device_mem_pool);
  bm_cmodel_exit(dev->model);

  TPU_LOG_DEBUG("device[%d] closed\n", dev->index);
  g_device = nullptr;
  BMDEV_LOCK_DEINIT(dev);

  delete dev;
}

int bm_device_get_chip_ver(bmdev_t dev) { return dev->cvk_chip_info.version; }

bmerr_t bm_context_create(bmctx_t *ctx) {
  bm_context_t *pctx = new bm_context_t;
  pctx->dev = NULL;
  pctx->seq_no = 0;
  *ctx = pctx;
  return BM_SUCCESS;
}

void bm_context_destroy(bmctx_t ctx) {
  TPU_ASSERT(ctx != nullptr, nullptr);
  delete ctx;
}

bmerr_t bm_bind_device(bmctx_t ctx, bmdev_t dev) {
  TPU_ASSERT(ctx != nullptr, nullptr);
  ctx->dev = dev;
  return BM_SUCCESS;
}

void bm_unbind_device(bmctx_t ctx) {
  TPU_ASSERT(ctx != nullptr, nullptr);
  ctx->dev = NULL;
}

bmdev_t bm_get_device(bmctx_t ctx) {
  TPU_ASSERT(ctx->dev != nullptr, nullptr);
  return ctx->dev;
}

bmerr_t bm_init(int index, bmctx_t *ctx) {
  TPU_ASSERT(index == 0, nullptr);

  bmerr_t ret;
  bmdev_t dev = nullptr;

  ret = bm_device_open(index, &dev);
  TPU_ASSERT(ret == BM_SUCCESS, nullptr);

  ret = bm_context_create(ctx);
  TPU_ASSERT(ret == BM_SUCCESS, nullptr);

  ret = bm_bind_device(*ctx, dev);
  TPU_ASSERT(ret == BM_SUCCESS, nullptr);

  return ret;
}

void bm_exit(bmctx_t ctx) {
  bmdev_t dev = ctx->dev;
  bm_unbind_device(ctx);
  bm_context_destroy(ctx);
  bm_device_close(dev);
}

bmmem_device_t bmmem_device_alloc_raw(bmctx_t ctx, size_t size) {
  uint32_t chip_ver = bm_device_get_chip_ver(ctx->dev);

  return getCmdbufPtr(chip_ver)->rt_device_alloc_raw(ctx, size);
}

bmmem_device_t bmmem_device_prealloc_raw(bmctx_t ctx, bmmem_device_t mem,
                                         uint64_t offset, size_t size) {
  (void)ctx;
  bm_memory_t *device_mem = new bm_memory_t();
  device_mem->flags.u.is_prealloc = 1;
  device_mem->flags.u.type = BMMEM_TYPE_DEVICE;
  if (mem) {
    TPU_ASSERT(mem->size >= size + offset, nullptr);
    device_mem->p_addr = ((bm_memory_t *)mem)->p_addr + offset;
  } else {
    device_mem->p_addr = offset;
  }

  // device_mem->v_addr = NULL;
  // device_mem->v_addr = (void*)(device_mem->p_addr +
  // (ctx->dev->model.chip.gmem));
  device_mem->v_addr = (uint8_t *)((device_mem->p_addr) +
                                   ((unsigned long long)bm_cmodel_get_chipGmem(
                                       ctx->dev->model)));
  device_mem->size = size;
  return (bmmem_device_t)device_mem;
}

void bmmem_device_free(bmctx_t ctx, bmmem_device_t mem) {
  uint32_t chip_ver = bm_device_get_chip_ver(ctx->dev);
  getCmdbufPtr(chip_ver)->rt_device_free(ctx, mem);
}

size_t bmmem_device_size(bmmem_device_t mem) {
  bm_memory_t *device_mem = (bm_memory_t *)mem;
  if (device_mem == NULL)
    return 0;
  TPU_ASSERT(device_mem->flags.u.type == BMMEM_TYPE_DEVICE, nullptr);
  return device_mem->size;
}

uint64_t bmmem_device_addr(bmmem_device_t mem) {
  bm_memory_t *device_mem = (bm_memory_t *)mem;
  if (device_mem == NULL)
    return 0;
  TPU_ASSERT(device_mem->flags.u.type == BMMEM_TYPE_DEVICE, nullptr);
  return device_mem->p_addr;
}

uint8_t *bmmem_device_v_addr(bmmem_device_t mem) {
  bm_memory_t *device_mem = (bm_memory_t *)mem;
  TPU_ASSERT(device_mem->flags.u.type == BMMEM_TYPE_DEVICE, nullptr);
  return device_mem->v_addr;
}

int32_t bmmem_device_inc_ref(bmmem_device_t mem) {
  bm_memory_t *device_mem = (bm_memory_t *)mem;
  TPU_ASSERT(device_mem->flags.u.type == BMMEM_TYPE_DEVICE, nullptr);
  return (++device_mem->user_ref_cnt);
}

int32_t bmmem_device_dec_ref(bmmem_device_t mem) {
  bm_memory_t *device_mem = (bm_memory_t *)mem;
  TPU_ASSERT(device_mem->flags.u.type == BMMEM_TYPE_DEVICE, nullptr);
  return (--device_mem->user_ref_cnt);
}

bmerr_t bm_memcpy_s2d(bmctx_t ctx, bmmem_device_t dst, uint8_t *src) {
  bm_memory_t *mem = (bm_memory_t *)dst;
  bm_cmodel_write_gmem(ctx->dev->model, mem->p_addr, src, mem->size);
  return BM_SUCCESS;
}

bmerr_t bm_memcpy_s2d_ex(bmctx_t ctx, bmmem_device_t dst, uint8_t *src,
                         unsigned long long offset, size_t size) {
  bm_memory_t *mem = (bm_memory_t *)dst;
  bm_cmodel_write_gmem(ctx->dev->model, mem->p_addr + offset, src, size);
  return BM_SUCCESS;
}

bmerr_t bm_memcpy_d2s(bmctx_t ctx, uint8_t *dst, bmmem_device_t src) {
  bm_memory_t *mem = (bm_memory_t *)src;
  bm_cmodel_read_gmem(ctx->dev->model, mem->p_addr, dst, mem->size);
  return BM_SUCCESS;
}

bmerr_t bmmem_device_flush(bmctx_t ctx, bmmem_device_t dev) {
  (void)ctx;
  (void)dev;
  return BM_SUCCESS;
}

bmerr_t bmmem_device_flush_len(bmctx_t ctx, bmmem_device_t mem, size_t len) {
  (void)ctx;
  (void)mem;
  (void)len;
  return BM_SUCCESS;
}

bmerr_t bmmem_device_invld(bmctx_t ctx, bmmem_device_t dev) {
  (void)ctx;
  (void)dev;
  return BM_SUCCESS;
}

bmerr_t bmmem_device_invld_len(bmctx_t ctx, bmmem_device_t mem, size_t len) {
  (void)ctx;
  (void)mem;
  (void)len;
  return BM_SUCCESS;
}

bmerr_t bm_send_cmdbuf(bmctx_t ctx, uint8_t *cmdbuf, size_t sz,
                       uint16_t *seq_no) {
  uint32_t chip_ver = bm_device_get_chip_ver(ctx->dev);
  return getCmdbufPtr(chip_ver)->rt_send_cmdbuf(ctx, cmdbuf, sz, seq_no);
}

bmerr_t bm_wait_cmdbuf_done(bmctx_t ctx, uint16_t seq_no) {
  uint32_t chip_ver = bm_device_get_chip_ver(ctx->dev);
  return getCmdbufPtr(chip_ver)->rt_wait_cmdbuf_done(ctx, seq_no);
}

bmerr_t bm_load_cmdbuf(bmctx_t ctx, uint8_t *cmdbuf, size_t sz,
                       unsigned long long neuron_gaddr,
                       unsigned long long weight_gaddr, bool enable_pmu,
                       bmmem_device_t *cmdbuf_mem) {

  uint32_t chip_ver = bm_device_get_chip_ver(ctx->dev);

  return getCmdbufPtr(chip_ver)->rt_load_cmdbuf(
      ctx, cmdbuf, sz, neuron_gaddr, weight_gaddr, enable_pmu, cmdbuf_mem);
}

bmerr_t bm_load_dmabuf(bmctx_t ctx, bmmem_device_t dmabuf, size_t sz,
                       unsigned long long neuron_gaddr,
                       unsigned long long weight_gaddr, bool enable_pmu,
                       bmmem_device_t *dmabuf_mem) {

  uint32_t chip_ver = bm_device_get_chip_ver(ctx->dev);

  return getCmdbufPtr(chip_ver)->rt_load_dmabuf(
      ctx, dmabuf, sz, neuron_gaddr, weight_gaddr, enable_pmu, dmabuf_mem);
}

bmerr_t bm_run_cmdbuf(bmctx_t ctx, bmmem_device_t cmdbuf_mem,
                      uint16_t *seq_no) {
  uint32_t chip_ver = bm_device_get_chip_ver(ctx->dev);

  return getCmdbufPtr(chip_ver)->rt_run_cmdbuf(ctx, cmdbuf_mem, seq_no);
}

bmerr_t bm_run_cmdbuf_ex(bmctx_t ctx, bmmem_device_t cmdbuf_mem,
                         uint16_t *seq_no, uint64_t input_base_addr,
                         uint64_t output_base_addr) {
  uint32_t chip_ver = bm_device_get_chip_ver(ctx->dev);

  return getCmdbufPtr(chip_ver)->rt_run_cmdbuf_ex(ctx, cmdbuf_mem, seq_no,
                                             input_base_addr, output_base_addr);
}

bmerr_t bm_run_cmdbuf_ex2(bmctx_t ctx, bmmem_device_t cmdbuf_mem,
                          uint16_t *seq_no, cvi_array_base *p_array_base) {

  uint32_t chip_ver = bm_device_get_chip_ver(ctx->dev);

  return getCmdbufPtr(chip_ver)->rt_run_cmdbuf_ex2(ctx, cmdbuf_mem, seq_no,
                                              p_array_base);
}

bmerr_t bm_parse_pmubuf(bmmem_device_t cmdbuf_mem, uint8_t **buf_start,
                        uint32_t *buf_len) {
  (void)cmdbuf_mem;
  *buf_start = NULL;
  *buf_len = 0;
  return BM_SUCCESS;
}

bmerr_t bm_run_cmdbuf_pio(bmctx_t ctx, uint8_t *cmdbuf, size_t sz) {
  (void)ctx;
  (void)cmdbuf;
  (void)sz;
  assert(0); // not support
  return BM_SUCCESS;
}

void cviruntime_cvikernel_create(bmctx_t ctx, void **p_bk_ctx) {
  TPU_ASSERT(ctx != nullptr, nullptr);
  TPU_ASSERT(ctx->dev != nullptr, nullptr);

  cvk_chip_info_t info;
  if (bm_device_get_chip_ver(ctx->dev) == BM1880V2_VER) {
    info = bmk1880v2_chip_info();
  } else if (bm_device_get_chip_ver(ctx->dev) == BM1822_VER) {
    info = bmk1822_chip_info();
  } else
    assert(0);

  cvk_chip_info_t *dev_info = &info;

  bmk_info_t bmk_info;
  bmk_info.chip_version = dev_info->version;
  bmk_info.cmdbuf_size = 0x10000000;
  bmk_info.cmdbuf = (u8 *)malloc(bmk_info.cmdbuf_size);
  assert(bmk_info.cmdbuf);
  if (bm_device_get_chip_ver(ctx->dev) == BM1880V2_VER) {
    ctx->cvik_context = bmk1880v2_register(&bmk_info);
  } else if (bm_device_get_chip_ver(ctx->dev) == BM1822_VER) {
    ctx->cvik_context = bmk1822_register(&bmk_info);
  }
  ctx->cvik_cmdbuf = (void *)bmk_info.cmdbuf;

  *p_bk_ctx = ctx->cvik_context;
}

void cviruntime_cvikernel_submit(bmctx_t ctx) {
  u32 len;
  u8 *cmdbuf;
  if (bm_device_get_chip_ver(ctx->dev) == BM1880V2_VER) {
    cmdbuf = bmk1880v2_acquire_cmdbuf(ctx->cvik_context, &len);
  } else if (bm_device_get_chip_ver(ctx->dev) == BM1822_VER) {
    cmdbuf = bmk1822_acquire_cmdbuf(ctx->cvik_context, &len);
  } else
    assert(0);
  uint16_t seq_no;
  bm_send_cmdbuf(ctx, cmdbuf, (size_t)len, &seq_no);
  if (bm_device_get_chip_ver(ctx->dev) == BM1880V2_VER) {
    bmk1880v2_reset(ctx->cvik_context);
  } else if (bm_device_get_chip_ver(ctx->dev) == BM1822_VER) {
    bmk1822_reset(ctx->cvik_context);
  }
}

void cviruntime_cvikernel_destroy(bmctx_t ctx) {
  assert(ctx->cvik_context);
  assert(ctx->cvik_cmdbuf);
  if (bm_device_get_chip_ver(ctx->dev) == BM1880V2_VER) {
    bmk1880v2_cleanup(ctx->cvik_context);
  } else if (bm_device_get_chip_ver(ctx->dev) == BM1822_VER) {
    bmk1822_cleanup(ctx->cvik_context);
  }
  free(ctx->cvik_cmdbuf);
}

CVI_RC CVI_RT_DeInitBK(CVI_RT_HANDLE rt_handle) {
  bmctx_t ctx = (bmctx_t)rt_handle;

  // deinit kernel related
  if (ctx->cvik_context) {
    if (!strcmp(g_run_chip, CVI_TPU_VERSION_183X)) {
      bmk1880v2_cleanup(ctx->cvik_context);
    } else if (!strcmp(g_run_chip, CVI_TPU_VERSION_182X)) {
      bmk1822_cleanup(ctx->cvik_context);
    } else {
      assert(0);
      return CVI_FAILURE;
    }
  }

  if (ctx->cvk_context)
    ctx->cvk_context->ops->cleanup(ctx->cvk_context);

  if (ctx->cvik_cmdbuf) {
    free(ctx->cvik_cmdbuf);
  }

  // deinit basic context
  bm_exit(ctx);
  return CVI_SUCCESS;
}

CVI_RC CVI_RT_InitWithKernelBK(CVI_RT_HANDLE *rt_handle, uint32_t cmdbuf_size) {
  bmctx_t *ctx = (bmctx_t *)rt_handle;

  if (!g_run_chip)
    g_run_chip = getenv("SET_CHIP_NAME");

  if (!strcmp(g_run_chip, CVI_TPU_VERSION_183X)) {
    bm_init(DEVICE_INDEX_NUM, ctx);

    bmk1880v2_chip_info_t info = bmk1880v2_chip_info();
    bmk1880v2_chip_info_t *dev_info = &info;

    bmk_info_t bmk_info;
    bmk_info.chip_version = dev_info->version;
    bmk_info.cmdbuf_size = cmdbuf_size;
    bmk_info.cmdbuf = (u8 *)malloc(bmk_info.cmdbuf_size);
    assert(bmk_info.cmdbuf);

    (*ctx)->cvik_context = bmk1880v2_register(&bmk_info);
    (*ctx)->cvik_cmdbuf = (void *)bmk_info.cmdbuf;
    (*ctx)->cvk_context = nullptr;
    return CVI_SUCCESS;

  } else if (!strcmp(g_run_chip, CVI_TPU_VERSION_182X)) {
    bm_init(DEVICE_INDEX_NUM, ctx);

    bmk1822_chip_info_t info = bmk1822_chip_info();
    bmk1822_chip_info_t *dev_info = &info;

    bmk_info_t bmk_info;
    bmk_info.chip_version = dev_info->version;
    bmk_info.cmdbuf_size = cmdbuf_size;
    bmk_info.cmdbuf = (u8 *)malloc(bmk_info.cmdbuf_size);
    assert(bmk_info.cmdbuf);

    (*ctx)->cvik_context = bmk1822_register(&bmk_info);
    (*ctx)->cvik_cmdbuf = (void *)bmk_info.cmdbuf;
    (*ctx)->cvk_context = nullptr;
    return CVI_SUCCESS;
  } else {
    assert(0);
  }

  return CVI_FAILURE;
}

CVI_RC CVI_RT_SubmitBK(CVI_RT_HANDLE rt_handle) {
  cviruntime_cvikernel_submit((bmctx_t)rt_handle);
  return CVI_SUCCESS;
}

CVI_RT_KHANDLE CVI_RT_GetKHandleBK(CVI_RT_HANDLE rt_handle) {
  bmctx_t ctx = (bmctx_t)rt_handle;
  return (CVI_RT_KHANDLE)(ctx->cvik_context);
}

CVI_RC CVI_RT_SubmitPio(CVI_RT_HANDLE rt_handle) {
  (void)rt_handle;
  assert(0); // not support
  return CVI_SUCCESS;
}

CVI_RC CVI_RT_Init(CVI_RT_HANDLE *rt_handle) {
  bmctx_t *ctx = (bmctx_t *)rt_handle;
  bm_init(DEVICE_INDEX_NUM, ctx);
  return CVI_SUCCESS;
}

CVI_RC CVI_RT_DeInit(CVI_RT_HANDLE rt_handle) {
  bmctx_t ctx = (bmctx_t)rt_handle;

  // deinit basic context
  bm_exit(ctx);
  return CVI_SUCCESS;
}

CVI_RT_KHANDLE CVI_RT_RegisterKernel(CVI_RT_HANDLE rt_handle,
                                     uint32_t cmdbuf_size) {
  bmctx_t ctx = (bmctx_t)rt_handle;
  cvk_reg_info_t req_info;
  cvk_context_t *tmp_cvk_context;
  cvi_rt_submit *submit_handle;

  // reset req_info
  memset(&req_info, 0, sizeof(cvk_reg_info_t));

  if (!strcmp(g_run_chip, CVI_TPU_VERSION_183X)) {
    strncpy(req_info.chip_ver_str, "cv183x", sizeof(req_info.chip_ver_str) - 1);
  } else if (!strcmp(g_run_chip, CVI_TPU_VERSION_182X)) {
    strncpy(req_info.chip_ver_str, "cv182x", sizeof(req_info.chip_ver_str) - 1);
  } else if (!strcmp(g_run_chip, CVI_TPU_VERSION_181X)) {
    strncpy(req_info.chip_ver_str, "cv181x", sizeof(req_info.chip_ver_str) - 1);
  } else if (!strcmp(g_run_chip, CVI_TPU_VERSION_180X)) {
    strncpy(req_info.chip_ver_str, "cv180x", sizeof(req_info.chip_ver_str) - 1);
  } else {
    assert(0);
    return NULL;
  }

  req_info.cmdbuf_size = cmdbuf_size;
  req_info.cmdbuf = (uint8_t *)malloc(req_info.cmdbuf_size);
  assert(req_info.cmdbuf && "Expect allocated cmdbuf");

  // register cvikernel
  tmp_cvk_context = cvikernel_register(&req_info);
  submit_handle = (cvi_rt_submit *)malloc(sizeof(cvi_rt_submit));
  assert(submit_handle && "Expect allocated kernel context");
  memset(submit_handle, 0, sizeof(cvi_rt_submit));

  // assign handle mapping related, and reassign cvikernel handle
  memcpy(submit_handle, tmp_cvk_context, sizeof(cvk_context_t));
  submit_handle->rt_ctx = ctx;
  submit_handle->cmdbuf = req_info.cmdbuf;
  submit_handle->magic = SUBMIT_MAGIC;
  free(tmp_cvk_context);

  return submit_handle;
}

CVI_RC CVI_RT_UnRegisterKernel(CVI_RT_KHANDLE rt_khandle) {
  cvk_context_t *cvk_context = (cvk_context_t *)rt_khandle;
  cvi_rt_submit *submit_handle = (cvi_rt_submit *)rt_khandle;

  if (!cvk_context) {
    assert(0 && "CVI_RT_UnRegisterKernel() NULL kernel handle");
    return CVI_FAILURE;
  }

  if (cvk_context)
    cvk_context->ops->cleanup(cvk_context);

  if (cvk_context->priv_data) {
    // priv_data alloc by malloc of cvikernel_1880v2
    free(cvk_context->priv_data);
  }

  if (submit_handle->cmdbuf) {
    free(submit_handle->cmdbuf);
  }

  free(rt_khandle);
  return CVI_SUCCESS;
}

CVI_RC CVI_RT_Submit(CVI_RT_HANDLE rt_khandle) {
  cvi_rt_submit *submit_handle = (cvi_rt_submit *)rt_khandle;
  uint32_t len;
  uint16_t seq_no;

  if (submit_handle->magic != SUBMIT_MAGIC) {
    TPU_LOG_WARNING("incorrect submit handle input\n");
    return CVI_FAILURE;
  }

  cvk_context_t *cvk_context = &submit_handle->cvk_ctx;
  uint8_t *cmdbuf = cvk_context->ops->acquire_cmdbuf(cvk_context, &len);

  bm_send_cmdbuf(submit_handle->rt_ctx, cmdbuf, (size_t)len, &seq_no);
  cvk_context->ops->reset(cvk_context);

  return CVI_SUCCESS;
}

CVI_RC CVI_RT_SubmitAsync(CVI_RT_KHANDLE rt_khandle, uint8_t submit_previous) {
  (void)rt_khandle;
  (void)submit_previous;
  assert(0); // not support
  return CVI_FAILURE;
}
CVI_RC CVI_RT_WaitForAsync(CVI_RT_KHANDLE rt_khandle) {
  (void)rt_khandle;
  assert(0); // not support
  return CVI_FAILURE;
}

CVI_RC CVI_RT_LoadCmdbuf(CVI_RT_HANDLE rt_handle, uint8_t *cmdbuf,
                         uint64_t cmdbuf_sz, uint64_t gaddr_base0,
                         uint64_t gaddr_base1, bool enable_pmu,
                         CVI_RT_MEM *cmdbuf_mem) {
  return (CVI_RC)bm_load_cmdbuf((bmctx_t)rt_handle, cmdbuf, (size_t)cmdbuf_sz,
                                (unsigned long long)gaddr_base0,
                                (unsigned long long)gaddr_base1, enable_pmu,
                                (bmmem_device_t *)cmdbuf_mem);
}

CVI_RC CVI_RT_LoadDmabuf(
    CVI_RT_HANDLE rt_handle, CVI_RT_MEM dmabuf,
    uint64_t dmabuf_sz, uint64_t gaddr_base0,
    uint64_t gaddr_base1, bool enable_pmu, CVI_RT_MEM *dmabuf_mem) {
  return (CVI_RC)bm_load_dmabuf((bmctx_t)rt_handle, (bmmem_device_t)dmabuf,
                                (size_t)dmabuf_sz,
                                (unsigned long long)gaddr_base0,
                                (unsigned long long)gaddr_base1, enable_pmu,
                                (bmmem_device_t *)dmabuf_mem);
}

CVI_RC CVI_RT_RunCmdbuf(CVI_RT_HANDLE rt_handle, CVI_RT_MEM cmdbuf_mem,
                        uint64_t gaddr_base2, uint64_t gaddr_base3) {

  CVI_RC ret;
  uint16_t seq_no;
  ret = (CVI_RC)bm_run_cmdbuf_ex((bmctx_t)rt_handle, (bmmem_device_t)cmdbuf_mem,
                                 &seq_no, gaddr_base2, gaddr_base3);
  if (ret != 0)
    return ret;

  return (CVI_RC)bm_wait_cmdbuf_done((bmctx_t)rt_handle, seq_no);
}

CVI_RC CVI_RT_RunCmdbufEx(CVI_RT_HANDLE rt_handle, CVI_RT_MEM cmdbuf_mem,
                          CVI_RT_ARRAYBASE *p_array_base) {
  CVI_RC ret;
  uint16_t seq_no;

  ret = (CVI_RC)bm_run_cmdbuf_ex2((bmctx_t)rt_handle, (bmmem_device_t)cmdbuf_mem,
                                   &seq_no, (cvi_array_base *)p_array_base);
  if (ret != 0)
    return ret;

  return (CVI_RC)bm_wait_cmdbuf_done((bmctx_t)rt_handle, seq_no);
}

CVI_RT_MEM CVI_RT_MemAlloc(CVI_RT_HANDLE rt_handle, uint64_t size) {
  bmctx_t ctx = (bmctx_t)rt_handle;
  uint32_t chip_ver = bm_device_get_chip_ver(ctx->dev);

  return getCmdbufPtr(chip_ver)->rt_device_alloc_raw(ctx, size);
}

CVI_RT_MEM CVI_RT_MemPreAlloc(CVI_RT_MEM mem, uint64_t offset, uint64_t size) {
  bm_memory_t *dev_mem = (bm_memory_t *)mem;
  TPU_ASSERT(dev_mem != nullptr, nullptr);

  bm_memory_t *preAlloc_mem = new bm_memory_t();
  preAlloc_mem->flags.u.is_prealloc = 1;
  preAlloc_mem->flags.u.type = BMMEM_TYPE_DEVICE;
  TPU_ASSERT(dev_mem->size >= size + offset, nullptr);
  preAlloc_mem->p_addr = dev_mem->p_addr + offset;
  preAlloc_mem->v_addr = (uint8_t *)(dev_mem->v_addr + offset);
  preAlloc_mem->size = size;
  return (CVI_RT_MEM)preAlloc_mem;
}

void CVI_RT_MemFree(CVI_RT_HANDLE rt_handle, CVI_RT_MEM mem) {
  bm_memory_t *dev_mem = (bm_memory_t *)mem;
  bmctx_t ctx = (bmctx_t)rt_handle;
  uint32_t chip_ver = bm_device_get_chip_ver(ctx->dev);

  getCmdbufPtr(chip_ver)->rt_device_free(ctx, dev_mem);
}

uint64_t CVI_RT_MemGetSize(CVI_RT_MEM mem) {
  if (!mem)
    return 0;
  bm_memory_t *dev_mem = (bm_memory_t *)mem;
  TPU_ASSERT(dev_mem->flags.u.type == BMMEM_TYPE_DEVICE, nullptr);
  return dev_mem->size;
}

uint64_t CVI_RT_MemGetPAddr(CVI_RT_MEM mem) {
  if (!mem)
    return 0;
  bm_memory_t *dev_mem = (bm_memory_t *)mem;
  TPU_ASSERT(dev_mem->flags.u.type == BMMEM_TYPE_DEVICE, nullptr);
  return dev_mem->p_addr;
}

uint8_t *CVI_RT_MemGetVAddr(CVI_RT_MEM mem) {
  if (!mem)
    return 0;
  bm_memory_t *dev_mem = (bm_memory_t *)mem;
  TPU_ASSERT(dev_mem->flags.u.type == BMMEM_TYPE_DEVICE, nullptr);
  return dev_mem->v_addr;
}

int32_t CVI_RT_MemIncRef(CVI_RT_MEM mem) {
  bm_memory_t *device_mem = (bm_memory_t *)mem;
  TPU_ASSERT(device_mem->flags.u.type == BMMEM_TYPE_DEVICE, nullptr);
  return (++device_mem->user_ref_cnt);
}

int32_t CVI_RT_MemDecRef(CVI_RT_MEM mem) {
  bm_memory_t *device_mem = (bm_memory_t *)mem;
  TPU_ASSERT(device_mem->flags.u.type == BMMEM_TYPE_DEVICE, nullptr);
  return (--device_mem->user_ref_cnt);
}

CVI_RC CVI_RT_MemFlush(CVI_RT_HANDLE rt_handle, CVI_RT_MEM mem) {
  (void)rt_handle;
  (void)mem;
  return CVI_SUCCESS;
}

CVI_RC CVI_RT_MemInvld(CVI_RT_HANDLE rt_handle, CVI_RT_MEM mem) {
  (void)rt_handle;
  (void)mem;
  return CVI_SUCCESS;
}

CVI_RC CVI_RT_MemFlushEx(CVI_RT_HANDLE rt_handle, CVI_RT_MEM mem,
                         uint64_t len) {
  (void)rt_handle;
  (void)mem;
  (void)len;
  return CVI_SUCCESS;
}

CVI_RC CVI_RT_MemInvldEx(CVI_RT_HANDLE rt_handle, CVI_RT_MEM mem,
                         uint64_t len) {
  (void)rt_handle;
  (void)mem;
  (void)len;
  return CVI_SUCCESS;
}

CVI_RC CVI_RT_MemCopyS2D(CVI_RT_HANDLE rt_handle, CVI_RT_MEM dst,
                         uint8_t *src) {
  bmctx_t ctx = (bmctx_t)rt_handle;
  bm_memory_t *mem = (bm_memory_t *)dst;
  bm_cmodel_write_gmem(ctx->dev->model, mem->p_addr, src, mem->size);
  return CVI_SUCCESS;
}

CVI_RC CVI_RT_MemCopyS2DEx(CVI_RT_HANDLE rt_handle, CVI_RT_MEM dst,
                           uint64_t offset, uint64_t len, uint8_t *src) {
  bmctx_t ctx = (bmctx_t)rt_handle;
  bm_memory_t *mem = (bm_memory_t *)dst;
  TPU_ASSERT((size_t)(offset + len) <= mem->size, nullptr);
  bm_cmodel_write_gmem(ctx->dev->model, mem->p_addr + offset, src, len);
  return CVI_SUCCESS;
}

CVI_RC CVI_RT_MemCopyD2S(CVI_RT_HANDLE rt_handle, uint8_t *dst,
                         CVI_RT_MEM src) {
  bmctx_t ctx = (bmctx_t)rt_handle;
  bm_memory_t *mem = (bm_memory_t *)src;
  bm_cmodel_read_gmem(ctx->dev->model, mem->p_addr, dst, mem->size);
  return CVI_SUCCESS;
}

CVI_RC CVI_RT_ParsePmuBuf(CVI_RT_MEM cmdbuf_mem, uint8_t **buf_start,
                          uint32_t *buf_len) {
  return (CVI_RC)bm_parse_pmubuf((bmmem_device_t)cmdbuf_mem, buf_start,
                                 buf_len);
}

CVI_RC CVI_RT_SetBaseReg(CVI_RT_HANDLE rt_handle, uint32_t inx,
                         uint64_t base_addr) {
  bmctx_t ctx = (bmctx_t)rt_handle;
  bm_cmodel_set_base_reg(ctx->dev->model, inx, base_addr);
  return CVI_SUCCESS;
}

CVI_RC CVI_RT_LoadCmdbufTee(CVI_RT_HANDLE rt_handle, uint8_t *cmdbuf, size_t sz,
                            uint64_t neuron_gaddr, uint64_t weight_gaddr,
                            uint32_t weight_len, CVI_RT_MEM *cmdbuf_mem) {
  (void)rt_handle;
  (void)cmdbuf;
  (void)sz;
  (void)neuron_gaddr;
  (void)weight_gaddr;
  (void)weight_len;
  (void)cmdbuf_mem;
  assert(0); // not support
  return CVI_SUCCESS;
}

CVI_RC CVI_RT_RunCmdbufTee(CVI_RT_HANDLE rt_handle, CVI_RT_MEM cmdbuf_mem,
                           CVI_RT_ARRAYBASE *p_array_base) {
  (void)rt_handle;
  (void)p_array_base;
  (void)cmdbuf_mem;
  assert(0); // not support
  return CVI_SUCCESS;
}
