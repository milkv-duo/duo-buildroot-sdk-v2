#pragma once
#include "core/core/cvtdl_core_types.h"
#include "core/core/cvtdl_errno.h"
#include "core/core/cvtdl_vpss_types.h"

#include "cvi_tdl_log.hpp"
#include "vpss_engine.hpp"

#include "bmlib_runtime.h"
#include "bmodel.hpp"
#include "bmruntime_common.h"
#include "bmruntime_profile.h"

#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include "profiler.hpp"

#define DEFAULT_MODEL_THRESHOLD 0.5
#define DEFAULT_MODEL_NMS_THRESHOLD 0.5

namespace cvitdl {

typedef enum { CVI_MEM_SYSTEM = 1, CVI_MEM_DEVICE = 2 } CVI_MEM_TYPE_E;

struct CvimodelConfig {
  bool debug_mode = false;
  int input_mem_type = CVI_MEM_SYSTEM;
};

struct CvimodelPair {
  std::vector<void *> raw_pointer;
  std::vector<bm_tensor_t> tensors;
  int32_t num = 0;
};

struct CvimodelInfo {
  CvimodelConfig conf;
  void *handle = nullptr;
  const char **net_names = NULL;
  CvimodelPair in;
  CvimodelPair out;
};

typedef struct {
  std::vector<int> dim;
  size_t dim_size;
} CVI_SHAPE;

struct TensorInfo {
  std::string tensor_name;
  void *raw_pointer;
  CVI_SHAPE shape;
  size_t tensor_size;
  int data_type;
  // number of tensor elements
  size_t tensor_elem;
  template <typename DataType>
  DataType *get() const {
    return static_cast<DataType *>(raw_pointer);
  }
  float qscale;
};

typedef enum {
  CVI_NN_PIXEL_RGB_PACKED = 0,
  CVI_NN_PIXEL_BGR_PACKED = 1,
  CVI_NN_PIXEL_RGB_PLANAR = 2,
  CVI_NN_PIXEL_BGR_PLANAR = 3,
  CVI_NN_PIXEL_YUV_NV12 = 11,
  CVI_NN_PIXEL_YUV_NV21 = 12,
  CVI_NN_PIXEL_YUV_420_PLANAR = 13,
  CVI_NN_PIXEL_GRAYSCALE = 15,
  CVI_NN_PIXEL_TENSOR = 100,
  CVI_NN_PIXEL_RGBA_PLANAR = 1000,
  // please don't use below values,
  // only for backward compatibility
  CVI_NN_PIXEL_PLANAR = 101,
  CVI_NN_PIXEL_PACKED = 102
} CVI_NN_PIXEL_FORMAT_E;
typedef CVI_NN_PIXEL_FORMAT_E CVI_FRAME_TYPE;
#define CVI_FRAME_PLANAR CVI_NN_PIXEL_PLANAR

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

  bm_handle_t bm_handle;
  virtual ~Core() = default;
  int modelOpen(const char *filepath);
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
  const InputPreParam &get_preparam() { return m_preprocess_param[0]; }
  virtual void set_preparam(const InputPreParam &pre_param) { m_preprocess_param[0] = pre_param; }

  bool isInitialized();
  void cleanupError();
  virtual bool allowExportChannelAttribute() const { return false; }
  void setUseMmap(bool mmap);
  void setraw(bool raw);
  virtual int after_inference();
  void set_perf_eval_interval(int interval) { model_timer_.Config("", interval); }
  int vpssCropImage(VIDEO_FRAME_INFO_S *srcFrame, VIDEO_FRAME_INFO_S *dstFrame, cvtdl_bbox_t bbox,
                    uint32_t rw, uint32_t rh, PIXEL_FORMAT_E enDstFormat,
                    VPSS_SCALE_COEF_E reize_mode = VPSS_SCALE_COEF_BICUBIC);
  int vpssChangeImage(VIDEO_FRAME_INFO_S *srcFrame, VIDEO_FRAME_INFO_S *dstFrame, uint32_t rw,
                      uint32_t rh, PIXEL_FORMAT_E enDstFormat);
  VpssEngine *get_vpss_instance() { return mp_vpss_inst; }

 protected:
  virtual int vpssPreprocess(VIDEO_FRAME_INFO_S *srcFrame, VIDEO_FRAME_INFO_S *dstFrame,
                             VPSSConfig &config);
  int run(std::vector<VIDEO_FRAME_INFO_S *> &frames);

  /*
   * Input/Output getter functions
   */
  void input_preprocess_config(const bm_net_info_t *net_info,
                               std::vector<VPSSConfig> &m_vpss_config);
  void setupOutputTensorInfo(const bm_net_info_t *net_info, CvimodelInfo *mp_mi_p,
                             std::map<std::string, TensorInfo> &tensor_info);
  void setupInputTensorInfo(const bm_net_info_t *net_info, CvimodelInfo *mp_mi_p,
                            std::map<std::string, TensorInfo> &tensor_info);
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

  std::unique_ptr<CvimodelInfo> &getModelInfo() { return mp_mi; }

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

 private:
  template <typename T>
  inline int __attribute__((always_inline)) registerFrame2Tensor(std::vector<T> &frames);

  std::map<std::string, TensorInfo> m_input_tensor_info;
  std::map<std::string, TensorInfo> m_output_tensor_info;

  // Preprocessing related control
  bool m_skip_vpss_preprocess = false;
  bool aligned_input = true;

  // Cvimodel related
  std::unique_ptr<CvimodelInfo> mp_mi;
  const bm_net_info_t *net_info;
  bool use_mmap;
  bool raw = false;
  uint8_t *register_temp_buffer = nullptr;
};
}  // namespace cvitdl
