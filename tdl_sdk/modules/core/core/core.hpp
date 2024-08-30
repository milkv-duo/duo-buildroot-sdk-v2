#pragma once
#include "core/core/cvtdl_core_types.h"
#include "core/core/cvtdl_errno.h"
#include "core/core/cvtdl_vpss_types.h"

#include "cvi_tdl_log.hpp"
#ifndef CONFIG_ALIOS
#include "model_debugger.hpp"
#endif
#include "vpss_engine.hpp"

#include <cviruntime.h>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "profiler.hpp"
#define DEFAULT_MODEL_THRESHOLD 0.5
#define DEFAULT_MODEL_NMS_THRESHOLD 0.5

namespace cvitdl {

struct CvimodelConfig {
  bool debug_mode = false;
  int input_mem_type = CVI_MEM_SYSTEM;
};

struct CvimodelPair {
  CVI_TENSOR *tensors = nullptr;
  int32_t num = 0;
};

struct CvimodelInfo {
  CvimodelConfig conf;
  CVI_MODEL_HANDLE handle = nullptr;
  CvimodelPair in;
  CvimodelPair out;
};

struct TensorInfo {
  std::string tensor_name;
  void *raw_pointer;
  CVI_SHAPE shape;
  CVI_TENSOR *tensor_handle;
#ifndef CONFIG_ALIOS
  int data_type;
#endif
  // Tensor size = (number of tensor elements) * typeof(tensor type))
  size_t tensor_size;

  // number of tensor elements
  size_t tensor_elem;
  template <typename DataType>
  DataType *get() const {
    return static_cast<DataType *>(raw_pointer);
  }
  float qscale;
};
struct VPSSConfig {
  meta_rescale_type_e rescale_type = RESCALE_CENTER;
  CVI_FRAME_TYPE frame_type = CVI_FRAME_PLANAR;
  VPSS_SCALE_COEF_E chn_coeff = VPSS_SCALE_COEF_BICUBIC;
  VPSS_CHN_ATTR_S chn_attr;
  VPSS_CROP_INFO_S crop_attr;
};

class Core {
 public:
  Core(CVI_MEM_TYPE_E input_mem_type);
  Core();
  Core(const Core &) = delete;
  Core &operator=(const Core &) = delete;

  virtual ~Core() = default;
  int modelOpen(const char *filepath);
  int modelOpen(const int8_t *buf, uint32_t size);
  int getInputMemType();
  const char *getModelFilePath() const { return m_model_file.c_str(); }
  int modelClose();
  int setVpssTimeout(uint32_t timeout);
  const uint32_t getVpssTimeout() const { return m_vpss_timeout; }
  int setVpssEngine(VpssEngine *engine);
  void skipVpssPreprocess(bool skip);

  bool hasSkippedVpssPreprocess() const { return m_skip_vpss_preprocess; }
  int setVpssDepth(uint32_t in_index, uint32_t depth);
  int getVpssDepth(uint32_t in_index, uint32_t *depth);
  virtual int getChnConfig(const uint32_t width, const uint32_t height, const uint32_t idx,
                           cvtdl_vpssconfig_t *chn_config);
  const float &getModelThreshold() { return m_model_threshold; }
  virtual void setModelThreshold(const float &threshold) { m_model_threshold = threshold; };
  const float &getModelNmsThreshold() { return m_model_nms_threshold; }
  virtual void setModelNmsThreshold(const float &threshold) { m_model_nms_threshold = threshold; };
  // @todo
  // 正常来说get方法就应该返回const常量不被修改，但仓库demo中经常会使用get获取原本参数，修改部分后再set回去
  const InputPreParam &get_preparam() { return m_preprocess_param[0]; }
  virtual void set_preparam(const InputPreParam &pre_param) { m_preprocess_param[0] = pre_param; }

  int setUseMmap(bool mmap) { return true; }
  virtual int after_inference() { return CVI_TDL_SUCCESS; }
  bool isInitialized();
  // TODO:remove this interface
  virtual bool allowExportChannelAttribute() const { return false; }
#ifndef CONFIG_ALIOS
  void enableDebugger(bool enable) { m_debugger.setEnable(enable); }
  void setDebuggerOutputPath(const std::string &dump_path) {
    m_debugger.setDirPath(dump_path);

    if (m_debugger.isEnable()) {
      LOGW("************************TDL SDK Debugger***********************\n");
      LOGW("TDL SDK Debugger is enabled!\n");
      LOGW("execute 'echo 1 > %s/enable' command to turn on debugger\n", dump_path.c_str());
      LOGW("execute 'echo 0 > %s/enable' command to turn off debugger\n", dump_path.c_str());
      LOGW("**************************************************************\n");
    }
  }
#endif

  void set_perf_eval_interval(int interval) { model_timer_.Config("", interval); }
  int vpssCropImage(VIDEO_FRAME_INFO_S *srcFrame, VIDEO_FRAME_INFO_S *dstFrame, cvtdl_bbox_t bbox,
                    uint32_t rw, uint32_t rh, PIXEL_FORMAT_E enDstFormat,
                    VPSS_SCALE_COEF_E reize_mode = VPSS_SCALE_COEF_BICUBIC);
  int vpssChangeImage(VIDEO_FRAME_INFO_S *srcFrame, VIDEO_FRAME_INFO_S *dstFrame, uint32_t rw,
                      uint32_t rh, PIXEL_FORMAT_E enDstFormat);
  VpssEngine *get_vpss_instance() { return mp_vpss_inst; }
#ifndef CONFIG_ALIOS
  void setraw(bool raw);
#endif

 protected:
  virtual int vpssPreprocess(VIDEO_FRAME_INFO_S *srcFrame, VIDEO_FRAME_INFO_S *dstFrame,
                             VPSSConfig &config);
  int run(std::vector<VIDEO_FRAME_INFO_S *> &frames);

  /*
   * Input/Output getter functions
   */
  CVI_TENSOR *getInputTensor(int idx);
  CVI_TENSOR *getOutputTensor(int idx);

  const TensorInfo &getOutputTensorInfo(const std::string &name);
  const TensorInfo &getInputTensorInfo(const std::string &name);

  const TensorInfo &getOutputTensorInfo(size_t index);
  const TensorInfo &getInputTensorInfo(size_t index);

  size_t getNumInputTensor() const;
  size_t getNumOutputTensor() const;

  CVI_SHAPE getInputShape(size_t index);
  CVI_SHAPE getOutputShape(size_t index);
  CVI_SHAPE getInputShape(const std::string &name);
  CVI_SHAPE getOutputShape(const std::string &name);

  size_t getOutputTensorElem(size_t index);
  size_t getOutputTensorElem(const std::string &name);
  size_t getInputTensorElem(size_t index);
  size_t getInputTensorElem(const std::string &name);

  size_t getOutputTensorSize(size_t index);
  size_t getOutputTensorSize(const std::string &name);
  size_t getInputTensorSize(size_t index);
  size_t getInputTensorSize(const std::string &name);

  float getInputQuantScale(size_t index);
  float getInputQuantScale(const std::string &name);

  template <typename DataType>
  DataType *getInputRawPtr(size_t index) {
    return getInputTensorInfo(index).get<DataType>();
  }

  template <typename DataType>
  DataType *getOutputRawPtr(size_t index) {
    return getOutputTensorInfo(index).get<DataType>();
  }

  template <typename DataType>
  DataType *getInputRawPtr(const std::string &name) {
    return getInputTensorInfo(name).get<DataType>();
  }

  template <typename DataType>
  DataType *getOutputRawPtr(const std::string &name) {
    return getOutputTensorInfo(name).get<DataType>();
  }
  ////////////////////////////////////////////////////

  virtual int onModelOpened() { return CVI_TDL_SUCCESS; }
  virtual int onModelClosed() { return CVI_TDL_SUCCESS; }

  void setInputMemType(CVI_MEM_TYPE_E type) { mp_mi->conf.input_mem_type = type; }
  std::vector<VPSSConfig> m_vpss_config;
  std::vector<InputPreParam> m_preprocess_param;

  // Post processing related control
  float m_model_threshold = DEFAULT_MODEL_THRESHOLD;
  float m_model_nms_threshold = DEFAULT_MODEL_NMS_THRESHOLD;

  // External handle
  VpssEngine *mp_vpss_inst = nullptr;
  Timer model_timer_;

 protected:
  // vpss related control
  int32_t m_vpss_timeout = 100;
  std::string m_model_file;
#ifndef CONFIG_ALIOS
  debug::ModelDebugger m_debugger;
#endif

 private:
  template <typename T>
  inline int __attribute__((always_inline)) registerFrame2Tensor(std::vector<T> &frames);

  void setupTensorInfo(CVI_TENSOR *tensor, int32_t num_tensors,
                       std::map<std::string, TensorInfo> *tensor_info);

  std::map<std::string, TensorInfo> m_input_tensor_info;
  std::map<std::string, TensorInfo> m_output_tensor_info;

  // Preprocessing related control
  bool m_skip_vpss_preprocess = false;
  bool aligned_input = true;

  // Cvimodel related
  std::unique_ptr<CvimodelInfo> mp_mi;
#ifndef CONFIG_ALIOS
  bool raw = false;
#endif
};
}  // namespace cvitdl
