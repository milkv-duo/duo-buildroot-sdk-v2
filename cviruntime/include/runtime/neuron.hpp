/*
* Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
*/

#ifndef RUNTIME_NEURON_H
#define RUNTIME_NEURON_H

#include <map>
#include <vector>
#include <memory>
#include "cviruntime.h"
#include "cviruntime_context.h"

namespace cvi {
namespace runtime {

class Neuron {
public:
  enum NeuronState {
    TPU_MEM = 0,
    CPU_MEM = 1,
  };

  enum NeuronType {
    WEIGHT      = 0,
    ACTIVATION  = 1,
  };

  Neuron(CVI_RT_HANDLE ctx, const void *model_tensor,
         CVI_RT_MEM weight_mem, const char *model_name);
  Neuron(CVI_RT_HANDLE ctx, CVI_RT_HANDLE cvk,
         const void *model_tensor,
         uint64_t *baseAddrArray,
         CVI_RT_MEM *baseMemArray,
         const char *model_name);
  ~Neuron();

  template <typename T>
  inline T* cpu_data() {
    _state = Neuron::CPU_MEM;
    return (T *)sys_mem();
  }

  inline size_t count() {
    return _count;
  }

  inline size_t size() {
    return _size;
  }

  inline size_t offset(int n, int c = 0, int h = 0, int w = 0) {
    return (((n * shape[1] + c) * shape[2] + h) * shape[3] + w);
  }

  inline bool overwrote() {
    return _overwrote;
  }

  inline void setState(NeuronState state) {
    _state = state;
  }

  inline uint8_t *sys_mem() {
    return (_vaddr ? _vaddr : _cpu_mem);
  }

  inline uint64_t paddr() {
    return _paddr;
  }

  inline float qscale() {
    return _qscale;
  }

  inline void setQScale(float scale) {
    _qscale = scale;
  }

  inline int zero_point(){
    return _zero_point;
  }

  CVI_RC preloadChannelAndCompact(int32_t channel_idx, uint64_t src_paddr);
  CVI_RC preloadFrameAndCompact(int32_t frame_idx, uint64_t src_paddr);
  CVI_RC preload(int32_t frame_idx, uint64_t src_paddr);

  void load(CVI_TENSOR &tensor);
  void store(CVI_TENSOR &tensor);
  void toCpu();
  void toTpu();
  CVI_RC reserveIonMem(int64_t offset);
  CVI_RC reserveSysMem();
  void updateBaseAddr(uint64_t paddr);
  bool isPacked();

private:
  void updateBaseAddr(CVI_RT_MEM mem);
  inline void setZeroPoint(int zp) { _zero_point = zp; }
  void setPixelFormatAndSize(const std::string &pixel_format, int32_t dsize);
  void setPixelAlign(CVI_NN_PIXEL_FORMAT_E format);
  uint32_t yuv_size(int n, int c, int h, int w, CVI_NN_PIXEL_FORMAT_E format);

public:
  std::string name;
  std::vector<int> shape;
  CVI_FMT fmt;
  NeuronType type;
  CVI_NN_PIXEL_FORMAT_E pixel_format;
  std::vector<float> scale;
  std::vector<float> mean;
  bool aligned = false;
  int vpss_w_align, vpss_y_align, vpss_channel_align;

private:
  CVI_RT_HANDLE _ctx;
  CVI_RT_KHANDLE _cvk;
  CVI_RT_MEM _streamCopyCmdbuf = nullptr;
  CVI_RT_MEM _channelPreloadCmdbuf = nullptr;
  CVI_RT_MEM _framePreloadCmdbuf = nullptr;
  CVI_RT_MEM _base_mem = nullptr;
  CVI_RT_MEM _gmem = nullptr;
  uint8_t* _cpu_mem = nullptr;
  uint8_t* _vaddr = nullptr;
  uint64_t _paddr = 0;
  NeuronState _state;
  uint32_t _id;
  uint32_t _count;
  uint32_t _size;
  uint32_t _tensor_size = 0;
  bool _overwrote = false;
  float _qscale = 1.0f;
  int _zero_point = 0;
  uint64_t *_baseAddrArray;
  CVI_RT_MEM *_baseMemArray;
  int32_t _baseAddrIndex = 1;
  std::string _model_name;
  std::string _module_name;
};

typedef std::map<std::string, std::shared_ptr<Neuron>> tensor_map_t;
typedef std::vector<std::shared_ptr<Neuron>> tensor_list_t;


} // namespace runtime
} // namespace cvi

#endif
