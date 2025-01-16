#pragma once
#include "core/core/cvtdl_core_types.h"
#include "core/core/cvtdl_errno.h"
#include "core/core/cvtdl_vpss_types.h"

#ifdef __CV186X__
#include "bmlib_runtime.h"
#include "bmodel.hpp"
#include "bmruntime_common.h"
#include "bmruntime_profile.h"
#else
#include <cviruntime.h>
#endif

#include <map>
#include <memory>
#include <string>
#include <vector>
#include "cvi_comm.h"
#include "cvi_tdl_log.hpp"
#include "profiler.hpp"
#include "vpss_engine.hpp"
#define DEFAULT_MODEL_THRESHOLD 0.5
#define DEFAULT_MODEL_NMS_THRESHOLD 0.5

namespace cvitdl {
typedef enum { CVI_MEM_SYSTEM = 1, CVI_MEM_DEVICE = 2 } CVI_MEM_TYPE_E;

struct CvimodelConfig {
  bool debug_mode = false;
  int input_mem_type = CVI_MEM_SYSTEM;
};

#ifdef __CV186X__
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

#else
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
#endif
struct TensorInfo {
  std::string tensor_name;
  void *raw_pointer;
  CVI_SHAPE shape;
#ifndef __CV186X__
  CVI_TENSOR *tensor_handle;
#endif

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

  int getInputMemType();
  int modelClose();
  int setVpssTimeout(uint32_t timeout);
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
  const InputPreParam &getPreparam() { return m_preprocess_param[0]; }
  virtual void setPreparam(const InputPreParam &pre_param) { m_preprocess_param[0] = pre_param; }

  int setUseMmap(bool mmap) { return true; }
  virtual int afterInference() { return CVI_TDL_SUCCESS; }
  bool isInitialized();
  // TODO:remove this interface
  virtual bool allowExportChannelAttribute() const { return false; }

  void setPerfEvalInterval(int interval) { model_timer_.Config("", interval); }
  int vpssCropImage(VIDEO_FRAME_INFO_S *srcFrame, VIDEO_FRAME_INFO_S *dstFrame, cvtdl_bbox_t bbox,
                    uint32_t rw, uint32_t rh, PIXEL_FORMAT_E enDstFormat,
                    VPSS_SCALE_COEF_E reize_mode = VPSS_SCALE_COEF_BICUBIC);
  int vpssChangeImage(VIDEO_FRAME_INFO_S *srcFrame, VIDEO_FRAME_INFO_S *dstFrame, uint32_t rw,
                      uint32_t rh, PIXEL_FORMAT_E enDstFormat);
  VpssEngine *getVpssInstance() { return mp_vpss_inst; }
#ifndef CONFIG_ALIOS
  void setRaw(bool raw);
#endif

#ifdef __CV186X__
  void cleanupHandle();
  int getModelInputDType();
#else
  int modelOpen(const int8_t *buf, uint32_t size);
#endif
 protected:
  virtual int vpssPreprocess(VIDEO_FRAME_INFO_S *srcFrame, VIDEO_FRAME_INFO_S *dstFrame,
                             VPSSConfig &config);
  int run(std::vector<VIDEO_FRAME_INFO_S *> &frames);

  const TensorInfo &getOutputTensorInfo(const std::string &name);
  const TensorInfo &getInputTensorInfo(const std::string &name);

  const TensorInfo &getOutputTensorInfo(size_t index);
  const TensorInfo &getInputTensorInfo(size_t index);

  size_t getNumOutputTensor() const;

  CVI_SHAPE getInputShape(size_t index);
  CVI_SHAPE getOutputShape(size_t index);
  CVI_SHAPE getInputShape(const std::string &name);
  CVI_SHAPE getOutputShape(const std::string &name);

  size_t getOutputTensorElem(size_t index);
  size_t getOutputTensorElem(const std::string &name);

  float getInputQuantScale(size_t index);
  float getInputQuantScale(const std::string &name);

  template <typename DataType>
  DataType *getOutputRawPtr(size_t index) {
    return getOutputTensorInfo(index).get<DataType>();
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
  // vpss related control
  int32_t m_vpss_timeout = 100;
#ifdef __CV186X__
  void inputPreprocessConfig(const bm_net_info_t *net_info, std::vector<VPSSConfig> &m_vpss_config);
  void setupOutputTensorInfo(const bm_net_info_t *net_info, CvimodelInfo *mp_mi_p,
                             std::map<std::string, TensorInfo> &tensor_info);
  void setupInputTensorInfo(const bm_net_info_t *net_info, CvimodelInfo *mp_mi_p,
                            std::map<std::string, TensorInfo> &tensor_info);
  bm_handle_t bm_handle;
#else
  void setupTensorInfo(CVI_TENSOR *tensor, int32_t num_tensors,
                       std::map<std::string, TensorInfo> *tensor_info);
#endif
 private:
  template <typename T>
  inline int __attribute__((always_inline)) registerFrame2Tensor(std::vector<T> &frames);

  std::map<std::string, TensorInfo> m_input_tensor_info;
  std::map<std::string, TensorInfo> m_output_tensor_info;

  // Preprocessing related control
  bool m_skip_vpss_preprocess = false;
  std::vector<bool> aligned_input;
  bool use_input_num_check_ = true;

  // Cvimodel related
  std::unique_ptr<CvimodelInfo> mp_mi;
#ifndef CONFIG_ALIOS
  bool raw = false;
#endif

#ifdef __CV186X__
  const bm_net_info_t *net_info;
  uint8_t *register_temp_buffer = nullptr;
#endif
};
}  // namespace cvitdl
