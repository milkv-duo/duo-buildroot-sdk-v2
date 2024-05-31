#include <iostream>
#include <sstream>
#include <mutex>
#include <runtime/debug.h>
#include "cviruntime.h"
#include <runtime/model.hpp>
#include <runtime/neuron.hpp>
#include <runtime/kernel_function.hpp>
#include <cvibuilder/cvimodel_generated.h>
#include "alloc.h"

#include <fstream>

namespace cvi {
namespace runtime {

// helper functions
static void fbDtypeToCVIFMTandSize(const cvi::model::DType dtype, CVI_FMT &fmt,
                                   int &dsize) {
  switch (dtype) {
    case cvi::model::DType_BF16:
      fmt = CVI_FMT_BF16;
      dsize = 2;
      break;
    case cvi::model::DType_INT8:
      fmt = CVI_FMT_INT8;
      dsize = 1;
      break;
    case cvi::model::DType_UINT8:
      fmt = CVI_FMT_UINT8;
      dsize = 1;
      break;
    case cvi::model::DType_FP32:
      fmt = CVI_FMT_FP32;
      dsize = 4;
      break;
    case cvi::model::DType_INT16:
      fmt = CVI_FMT_INT16;
      dsize = 2;
      break;
    case cvi::model::DType_UINT16:
      fmt = CVI_FMT_UINT16;
      dsize = 2;
      break;
    case cvi::model::DType_INT32:
      fmt = CVI_FMT_INT32;
      dsize = 4;
      break;
    case cvi::model::DType_UINT32:
      fmt = CVI_FMT_UINT32;
      dsize = 4;
      break;
    default:
      TPU_LOG_FATAL("unsupported dtype:%d\n", (int)dtype);
  }
}

static void fbShapeToVector(const cvi::model::Shape *shape,
                            std::vector<int> &shape_vec) {
  shape_vec.resize(4);
  shape_vec[0] = (int)shape->dim()->Get(0);
  shape_vec[1] = (int)shape->dim()->Get(1);
  shape_vec[2] = (int)shape->dim()->Get(2);
  shape_vec[3] = (int)shape->dim()->Get(3);
}

static inline int align_up(int x, int n) {
  return ((x + n - 1) / n) * n;
}

void Neuron::setPixelAlign(CVI_NN_PIXEL_FORMAT_E format) {
  if (CviModel::targetChipType == "cv183x") {
    vpss_y_align = 32;
    vpss_w_align = 32;
    vpss_channel_align = 0x1000;
    if (CVI_NN_PIXEL_YUV_420_PLANAR == format) {
      vpss_y_align = vpss_w_align * 2;
    }
  } else {
    vpss_y_align = 64;
    vpss_w_align = 64;
    vpss_channel_align = 64;
    if (CVI_NN_PIXEL_YUV_420_PLANAR == format) {
      vpss_y_align = vpss_w_align * 2;
    }
  }
}

uint32_t Neuron::yuv_size(int n, int c, int h, int w, CVI_NN_PIXEL_FORMAT_E format) {
  switch (format) {
    case CVI_NN_PIXEL_YUV_420_PLANAR: {
      assert(c == 3);
      int y_w_aligned  = align_up(w, vpss_y_align);
      int uv_w_aligned = align_up(w / 2, vpss_w_align);
      int u            = align_up(h * y_w_aligned, vpss_channel_align);
      int v            = align_up(u + h / 2 * uv_w_aligned, vpss_channel_align);
      int n_stride     = align_up(v + h / 2 * uv_w_aligned, vpss_channel_align);
      return n * n_stride;
    }
    case CVI_NN_PIXEL_YUV_NV21:
    case CVI_NN_PIXEL_YUV_NV12: {
      assert(c == 3);
      int y_w_aligned  = align_up(w, vpss_y_align);
      int uv_w_aligned = align_up(w, vpss_w_align);
      int uv            = align_up(h * y_w_aligned, vpss_channel_align);
      int n_stride     = align_up(uv + h / 2 * uv_w_aligned, vpss_channel_align);
      return n * n_stride;
    }
    default:
      TPU_LOG_FATAL("unsupported yuv pixel format:%d\n", format);
  }
  return 0;
}

Neuron::Neuron(CVI_RT_HANDLE ctx, const void *model_tensor,
               CVI_RT_MEM weight_mem, const char *model_name)
    : type(Neuron::WEIGHT), _ctx(ctx), _state(Neuron::TPU_MEM) {

  CVI_FMT fmt = CVI_FMT_FP32;
  int32_t dsize = 0;
  std::vector<int> shape;

  auto weight = (const cvi::model::Weight *)model_tensor;
  fbDtypeToCVIFMTandSize(weight->type(), fmt, dsize);
  fbShapeToVector(weight->shape(), shape);
  this->_id = 0;
  this->_count = shape[0] * shape[1] * shape[2] * shape[3];
  this->_size = _count * dsize;
  this->shape = shape;
  this->fmt = fmt;
  this->name = weight->name()->str();
  this->pixel_format = CVI_NN_PIXEL_TENSOR;
  this->type = Neuron::WEIGHT;
  if (model_name) {
    _model_name = model_name;
  }
  _module_name = _model_name + ":";
  _module_name += this->name;
  _gmem = CVI_RT_MemPreAlloc(weight_mem, weight->offset() & 0x0FFFFFFFFFF, _size);
  _vaddr = CVI_RT_MemGetVAddr(_gmem);
  _paddr = CVI_RT_MemGetPAddr(_gmem);
  _base_mem = weight_mem;
}

Neuron::Neuron(
    CVI_RT_HANDLE ctx, CVI_RT_KHANDLE cvk, const void *model_tensor,
    uint64_t *baseAddrArray, CVI_RT_MEM *baseMemArray, const char *model_name)
    : type(Neuron::ACTIVATION),
      _ctx(ctx), _cvk(cvk),
      _state(Neuron::TPU_MEM),
      _baseAddrArray(baseAddrArray),
      _baseMemArray(baseMemArray) {

  if (model_name) {
    _model_name = model_name;
  }
  CVI_FMT fmt = CVI_FMT_FP32;
  int32_t dsize = 0;
  std::vector<int> shape;

  auto tensor = (const cvi::model::Tensor *)model_tensor;
  fbDtypeToCVIFMTandSize(tensor->dtype(), fmt, dsize);
  fbShapeToVector(tensor->shape(), shape);
  this->_id = tensor->tensor_id();
  this->_count = shape[0] * shape[1] * shape[2] * shape[3];
  this->_overwrote = (bool)tensor->overwrote();
  this->shape = shape;
  this->fmt = fmt;
  this->name = tensor->name()->str();
  this->type = Neuron::ACTIVATION;

  this->aligned = tensor->aligned();
  this->_tensor_size = tensor->size();
  auto pixel_format = tensor->pixel_format() ?
      tensor->pixel_format()->str() : std::string("");
  setPixelFormatAndSize(pixel_format, dsize);

  _module_name = _model_name + ":";
  _module_name += this->name;

  if (tensor->scale()) {
    for (int i = 0; i < (int)tensor->scale()->size(); i++) {
      this->scale.push_back(tensor->scale()->Get(i));
    }
  }

  if (tensor->mean()) {
    for (int i = 0; i < (int)tensor->mean()->size(); i++) {
      this->mean.push_back(tensor->mean()->Get(i));
    }
  }

  if (tensor->quant()) {
    setQScale(tensor->quant()->qscale());
    setZeroPoint(tensor->quant()->zero_point());
  }

}

Neuron::~Neuron() {
  if (_gmem)
    cviMemFree(_ctx, _gmem);
  if (_channelPreloadCmdbuf)
    CVI_RT_MemFree(_ctx, _channelPreloadCmdbuf);
  if (_framePreloadCmdbuf)
    CVI_RT_MemFree(_ctx, _framePreloadCmdbuf);
  if (_streamCopyCmdbuf)
    CVI_RT_MemFree(_ctx, _streamCopyCmdbuf);
  if (_cpu_mem)
    free(_cpu_mem);
}

void Neuron::setPixelFormatAndSize(const std::string &pixel_format,
                                   int32_t dsize) {
  if (pixel_format.empty()) {
    assert(!this->aligned);
    this->pixel_format = CVI_NN_PIXEL_TENSOR;
  } else if (pixel_format == "BGR_PLANAR") {
    this->pixel_format = CVI_NN_PIXEL_BGR_PLANAR;
  } else if (pixel_format == "BGR_PACKED") {
    this->pixel_format = CVI_NN_PIXEL_BGR_PACKED;
  } else if (pixel_format == "RGB_PLANAR") {
    this->pixel_format = CVI_NN_PIXEL_RGB_PLANAR;
  } else if (pixel_format == "RGB_PACKED") {
    this->pixel_format = CVI_NN_PIXEL_RGB_PACKED;
  } else if (pixel_format == "GRAYSCALE") {
    this->pixel_format = CVI_NN_PIXEL_GRAYSCALE;
  } else if (pixel_format == "YUV_NV12") {
    this->pixel_format = CVI_NN_PIXEL_YUV_NV12;
  } else if (pixel_format == "YUV_NV21") {
    this->pixel_format = CVI_NN_PIXEL_YUV_NV21;
  } else if (pixel_format == "YUV420_PLANAR") {
    this->pixel_format = CVI_NN_PIXEL_YUV_420_PLANAR;
  } else if (pixel_format == "RGBA_PLANAR") {
    this->pixel_format = CVI_NN_PIXEL_RGBA_PLANAR;
  } else {
    TPU_LOG_FATAL("unkown pixel_format:%s\n", pixel_format.c_str());
  }
  setPixelAlign(this->pixel_format);

  if (!this->aligned) {
    _size = _count * dsize;
    return;
  }
  if (this->aligned && this->_tensor_size) {
    _size = this->_tensor_size;
    return;
  }

  switch(this->pixel_format) {
    case CVI_NN_PIXEL_GRAYSCALE:
      _size = shape[0] * shape[1] * shape[2] * align_up(shape[3], vpss_w_align);
      break;
    case CVI_NN_PIXEL_BGR_PLANAR:
    case CVI_NN_PIXEL_RGB_PLANAR:
    case CVI_NN_PIXEL_RGBA_PLANAR: {
      int align_w = align_up(shape[3], vpss_w_align);
      int align_c = align_up(align_w * shape[2], vpss_channel_align);
      _size = shape[0] * shape[1] * align_c;
      break;
    }
    case CVI_NN_PIXEL_BGR_PACKED:
    case CVI_NN_PIXEL_RGB_PACKED:
      _size = shape[0] * shape[2] * align_up(shape[3] * shape[1], vpss_w_align);
      break;
    case CVI_NN_PIXEL_YUV_NV12:
    case CVI_NN_PIXEL_YUV_NV21:
    case CVI_NN_PIXEL_YUV_420_PLANAR:
      _size = yuv_size(shape[0], shape[1], shape[2], shape[3], this->pixel_format);
      break;
    default:
      assert(0);
  }
}

bool Neuron::isPacked() {
  if (pixel_format == CVI_NN_PIXEL_TENSOR) {
    pixel_format = (shape[3] == 3) ? CVI_NN_PIXEL_PACKED :
                    CVI_NN_PIXEL_PLANAR;
  }
  if (pixel_format == CVI_NN_PIXEL_BGR_PACKED ||
      pixel_format == CVI_NN_PIXEL_RGB_PACKED ||
      pixel_format == CVI_NN_PIXEL_PACKED) {
    return true;
  }
  return false;
}

// preload cahnnel's data from vpss buffer, which w dimension is aligned by vpss_w_align.
// we need copy and unalign the data to compactly tensor using TDMA.
CVI_RC Neuron::preloadChannelAndCompact(int32_t channel_idx, uint64_t src_paddr) {
  uint32_t h, w;
  if (isPacked()) {
    h = shape[1];
    w = shape[2] * shape[3];
  } else {
    h = shape[2];
    w = shape[3];
  }
  CVI_RC ret = CVI_RC_SUCCESS;
  for (int i = 0; i < 3; ++i) {
      if (!_channelPreloadCmdbuf) {
          uint32_t hstride           = align_up(w, vpss_w_align);
          cvk_tg_shape_t tg_shape    = {1, 1, h, w};
          cvk_tg_stride_t src_stride = {1, 1, hstride, 1};
          cvk_tg_stride_t dst_stride = {1, 1, w, 1};
          _channelPreloadCmdbuf      = runtimeJitTdmaStrideCopy(
                   _ctx, _cvk, fmt, &tg_shape,
                   &dst_stride, &tg_shape, &src_stride);
          if (!_channelPreloadCmdbuf) {
              continue;
          }
      }
      ret = runtimeExecuteKernelFunction(
          _ctx, _channelPreloadCmdbuf, src_paddr,
          _paddr + channel_idx * h * w);
      if (ret != CVI_RC_SUCCESS) {
          TPU_LOG_ERROR("preloadChannelAndCompact fail!ret:%d", ret);
          CVI_RT_MemFree(_ctx, _channelPreloadCmdbuf);
          _channelPreloadCmdbuf = nullptr;
      } else {
          return CVI_RC_SUCCESS;
      }
  }
  return ret;
}

// preload frame's data from vpss buffer
// we need copy and unalign the data to compactly tensor using TDMA.
CVI_RC Neuron::preloadFrameAndCompact(int32_t frame_idx, uint64_t src_paddr) {
  uint32_t c, h, w;
  if (isPacked()) {
    c = 1;
    h = shape[1];
    w = shape[2] * shape[3];
  } else {
    c = shape[1];
    h = shape[2];
    w = shape[3];
  }
  CVI_RC ret = CVI_RC_SUCCESS;
  for (int i = 0; i < 3; ++i) {
      if (!_framePreloadCmdbuf) {
          uint32_t hstride           = align_up(w, vpss_w_align);
          cvk_tg_shape_t tg_shape    = {1, c, h, w};
          cvk_tg_stride_t src_stride = {1, h * hstride, hstride, 1};
          cvk_tg_stride_t dst_stride = {1, h * w, w, 1};
          _framePreloadCmdbuf        = runtimeJitTdmaStrideCopy(
                     _ctx, _cvk, fmt, &tg_shape,
                     &dst_stride, &tg_shape, &src_stride);
          if (!_framePreloadCmdbuf) {
              continue;
          }
      }
      ret = runtimeExecuteKernelFunction(
          _ctx, _framePreloadCmdbuf, src_paddr,
          _paddr + frame_idx * c * h * w);
      if (ret != CVI_RC_SUCCESS) {
          TPU_LOG_ERROR("preloadFrameAndCompact fail!ret:%d", ret);
          CVI_RT_MemFree(_ctx, _framePreloadCmdbuf);
          _framePreloadCmdbuf = nullptr;
      } else {
        return CVI_RC_SUCCESS;
      }
  }
  return ret;
}

// just copy vpss data to neuron and
// keep w and frame dimensions's alignment.
CVI_RC Neuron::preload(int32_t frame_idx, uint64_t src_paddr) {
  uint32_t frame_size = align_up(_size / shape[0], vpss_w_align);
  CVI_RC ret = CVI_RC_SUCCESS;
  for (int i = 0; i < 3; ++i) {
      if (!_framePreloadCmdbuf) {
          cvk_tg_shape_t tg_shape = {1, 1, frame_size / vpss_w_align, (uint32_t)vpss_w_align};
          _framePreloadCmdbuf     = runtimeJitTdmaStrideCopy(
                  _ctx, _cvk, CVI_FMT_INT8, &tg_shape,
                  nullptr, &tg_shape, nullptr);
          if (!_framePreloadCmdbuf) {
              continue;
          }
      }
      ret = runtimeExecuteKernelFunction(
          _ctx, _framePreloadCmdbuf, src_paddr,
          _paddr + frame_idx * frame_size);
      if (ret != CVI_RC_SUCCESS) {
          TPU_LOG_ERROR("preload fail!ret:%d", ret);
          CVI_RT_MemFree(_ctx, _framePreloadCmdbuf);
          _framePreloadCmdbuf = nullptr;
      } else {
        return CVI_RC_SUCCESS;
      }
  }
  return ret;
}

void Neuron::load(CVI_TENSOR &tensor) {
  // load data from system mem.
  if (tensor.mem_type == CVI_MEM_SYSTEM) {
    if (_vaddr) {
      if (_vaddr != tensor.sys_mem) {
        memcpy(_vaddr, tensor.sys_mem, _size);
      }
      TPU_ASSERT((int)CVI_RT_MemFlush(_ctx, _gmem) == 0, nullptr);
      _state = Neuron::TPU_MEM;
    } else {
      if (_cpu_mem != tensor.sys_mem) {
        memcpy(_cpu_mem, tensor.sys_mem, _size);
      }
      _state = Neuron::CPU_MEM;
    }
  } else { // load data from device mem.
    if (!_gmem && !_paddr) {
      assert(0 && "has no device mem allocated");
    }
    // needed to copy data using tdma.
    if (tensor.paddr != _paddr) {
      if (!_streamCopyCmdbuf) {
        cvk_tg_shape_t tg_shape;
        tg_shape.n = shape[0];
        tg_shape.c = shape[1];
        tg_shape.h = shape[2];
        tg_shape.w = shape[3];
        _streamCopyCmdbuf =
            runtimeJitTdmaStrideCopy(_ctx, _cvk, fmt, &tg_shape, nullptr, &tg_shape, nullptr);
      }
      runtimeExecuteKernelFunction(_ctx, _streamCopyCmdbuf, tensor.paddr, _paddr);
    }
    _state = Neuron::TPU_MEM;
  }
}

void Neuron::store(CVI_TENSOR &tensor) {
  if (tensor.mem_type == CVI_MEM_SYSTEM) {
    if (_state == Neuron::CPU_MEM) {
      if (tensor.sys_mem != sys_mem())
        memcpy(tensor.sys_mem, sys_mem(), _size);
    } else {
      TPU_ASSERT((int)CVI_RT_MemInvld(_ctx, _gmem) == 0,nullptr);
      if (tensor.sys_mem != _vaddr)
        memcpy(tensor.sys_mem, _vaddr, _size);
    }
  } else {
    if (tensor.paddr != _paddr) {
      if (!_streamCopyCmdbuf) {
        cvk_tg_shape_t tg_shape;
        tg_shape.n = shape[0];
        tg_shape.c = shape[1];
        tg_shape.h = shape[2];
        tg_shape.w = shape[3];
        _streamCopyCmdbuf =
            runtimeJitTdmaStrideCopy(_ctx, _cvk, fmt, &tg_shape, nullptr, &tg_shape, nullptr);
      }
      runtimeExecuteKernelFunction(_ctx, _streamCopyCmdbuf, _paddr, tensor.paddr);
    }
  }
}

void Neuron::toCpu() {
  if (_state != Neuron::CPU_MEM) {
    if (_cpu_mem) {
      CVI_RT_MemCopyD2S(_ctx, _cpu_mem, _gmem);
    } else {
      TPU_ASSERT((int)CVI_RT_MemInvld(_ctx, _gmem) == 0, nullptr);
    }
    _state = Neuron::CPU_MEM;
  }
}

void Neuron::toTpu() {
  if (_state != Neuron::TPU_MEM) {
    if (_cpu_mem) {
      CVI_RT_MemCopyS2D(_ctx, _gmem, _cpu_mem);
      //TPU_LOG_DEBUG("load data from cpu_mem (%p) to device_mem (%p)\n",
      //              (void *)_cpu_mem, (void *)_gmem);
    } else {
      CVI_RT_MemFlush(_ctx, _gmem);
      CVI_RT_MemInvld(_ctx, _base_mem);
      //TPU_LOG_DEBUG("flush device_mem (%p)\n", (void *)_vaddr);
    }
    _state = Neuron::TPU_MEM;
  }
}

CVI_RC Neuron::reserveIonMem(int64_t offset) {
  if (offset == -1) {
    return CVI_RC_SUCCESS;
  }

  _baseAddrIndex = (offset >> 40 & 0x07);
  assert(_baseAddrIndex < 8 && _baseAddrIndex != 1);
  uint64_t shift = offset & 0x0FFFFFFFFFF;
  if (_baseAddrIndex < 3) { // shared mem
    _gmem = CVI_RT_MemPreAlloc(_baseMemArray[_baseAddrIndex], shift, _size);
  } else {
    if (!_baseMemArray[_baseAddrIndex]) {
      assert(shift == 0);
      _gmem = cviMemAlloc(_ctx, _size, CVI_ALLOC_NEURON, _module_name.c_str());
      if (!_gmem) {
        TPU_LOG_ERROR("failed to alloc io mem\n");
        return CVI_RC_NOMEM;
      }
      _baseMemArray[_baseAddrIndex] = _gmem;
      updateBaseAddr(_gmem);
    } else {
      _gmem = CVI_RT_MemPreAlloc(_baseMemArray[_baseAddrIndex], shift, _size);
    }
  }
  _base_mem = _baseMemArray[_baseAddrIndex];
  _vaddr = CVI_RT_MemGetVAddr(_gmem);
  _paddr = CVI_RT_MemGetPAddr(_gmem);
  return CVI_RC_SUCCESS;
}

CVI_RC Neuron::reserveSysMem() {
  /*
   * if the tensor has device_mem, we can use vaddr
   * of device_mem as cpu mem. So no need to assign
   * memory in such case. Otherwise, we need allocate
   * memory from heap.
   */
  if (_gmem)
    return CVI_RC_SUCCESS;

  if (!_cpu_mem) {
    // _cpu_mem needed be aligned to 32bytes
    // with size aligned to 64 bytes.
    _cpu_mem = (uint8_t *)aligned_alloc(32, align_up(_size, 64));
    if (!_cpu_mem) {
      TPU_LOG_ERROR("alloc system memory for tensor failed, %s , size:%d\n", name.c_str(), _size);
      return CVI_RC_NOMEM;
    }
  }
  return CVI_RC_SUCCESS ;
}

void Neuron::updateBaseAddr(uint64_t paddr) {
  if (_baseAddrIndex < 3)
    return;
  _baseAddrArray[_baseAddrIndex] = paddr;
  _paddr = paddr;
  if (_gmem) {
    cviMemFree(_ctx, _gmem);
    _gmem = nullptr;
  }
}

void Neuron::updateBaseAddr(CVI_RT_MEM mem) {
  if (_baseAddrIndex < 3)
    return;
  _baseMemArray[_baseAddrIndex] = mem;
  _baseAddrArray[_baseAddrIndex] = CVI_RT_MemGetPAddr(mem);
}

} // namespace runtime
} // namespace cvi
