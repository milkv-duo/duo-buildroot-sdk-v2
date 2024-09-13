#include "core_a2.hpp"
#include "core/utils/vpss_helper.h"
#include "error_msg.hpp"
#define ALIGN_SIZE 64

namespace cvitdl {

Core::Core(CVI_MEM_TYPE_E input_mem_type) {
  mp_mi = std::make_unique<CvimodelInfo>();
  mp_mi->conf = {.debug_mode = false, .input_mem_type = input_mem_type};
  if (input_mem_type == CVI_MEM_SYSTEM) {
    use_mmap = false;
  } else {
    use_mmap = true;
  }

  InputPreParam first_in_pre_param;
  std::fill(std::begin(first_in_pre_param.factor), std::end(first_in_pre_param.factor), 0.0f);
  std::fill(std::begin(first_in_pre_param.mean), std::end(first_in_pre_param.mean), 0.0f);
  first_in_pre_param.keep_aspect_ratio = true;
  first_in_pre_param.use_crop = false;
  first_in_pre_param.format = PIXEL_FORMAT_RGB_888_PLANAR;
  first_in_pre_param.rescale_type = RESCALE_CENTER;
  first_in_pre_param.resize_method = VPSS_SCALE_COEF_BICUBIC;
  m_preprocess_param.emplace_back(first_in_pre_param);
}

Core::Core() : Core(CVI_MEM_SYSTEM) {}

int Core::getInputMemType() { return mp_mi->conf.input_mem_type; }
void Core::setUseMmap(bool mmap) { use_mmap = mmap; }
void Core::setraw(bool raw) { this->raw = raw; }

void Core::cleanupError() {
  if (mp_mi->handle != nullptr) {
    bmrt_destroy(mp_mi->handle);
    mp_mi->handle = nullptr;
  }

  if (bm_handle != nullptr) {
    bm_dev_free(bm_handle);
    bm_handle = nullptr;
  }
}

int Core::modelOpen(const char *filepath) {
  if (mp_mi == nullptr || mp_mi->handle != nullptr) {
    LOGE("failed to open model: \"%s\" has already opened.\n", filepath);
    return CVI_TDL_ERR_OPEN_MODEL;
  }

  bm_status_t status = bm_dev_request(&bm_handle, 0);
  if (CVI_TDL_SUCCESS != status) {
    LOGE("bm_dev_request failed\n");
    return CVI_TDL_ERR_OPEN_MODEL;
  }

  mp_mi->handle = bmrt_create(bm_handle);
  if (nullptr == mp_mi->handle) {
    cleanupError();
    LOGE("bmrt_create failed\n");
    return CVI_TDL_ERR_OPEN_MODEL;
  }

  auto ret = bmrt_load_bmodel(mp_mi->handle, filepath);
  if (CVI_TRUE != ret) {
    cleanupError();
    LOGE("bmrt_load_bmodel failed\n");
    return CVI_TDL_ERR_OPEN_MODEL;
  }

  bmrt_get_network_names(mp_mi->handle, &mp_mi->net_names);
  auto net_num = bmrt_get_network_number(mp_mi->handle);
  if (net_num != 1) {
    cleanupError();
    LOGE("bmrt_get_network_number failed\n");
    return CVI_TDL_ERR_OPEN_MODEL;
  }

  net_info = bmrt_get_network_info(mp_mi->handle, mp_mi->net_names[0]);
  if (nullptr == net_info || net_info->stage_num != 1) {
    cleanupError();
    LOGE("net_info should value Or only support one stage bmodel\n");
    return CVI_TDL_ERR_OPEN_MODEL;
  }

  mp_mi->in.num = net_info->input_num;
  auto &in_tensor = mp_mi->in.tensors;
  in_tensor.reserve(mp_mi->in.num);
  auto &in_raw_pointer = mp_mi->in.raw_pointer;
  in_raw_pointer.reserve(mp_mi->in.num);

  mp_mi->out.num = net_info->output_num;
  auto &out_tensor = mp_mi->out.tensors;
  out_tensor.reserve(mp_mi->out.num);
  auto &out_raw_pointer = mp_mi->out.raw_pointer;
  out_raw_pointer.reserve(mp_mi->out.num);

  /* only alloc output device mem */
  auto &stage = net_info->stages[0];
  if (use_mmap) {
    for (auto i = 0; i < mp_mi->in.num; i++) {
      in_tensor[i].dtype = net_info->input_dtypes[i];
      in_tensor[i].st_mode = BM_STORE_1N;
      in_tensor[i].shape.num_dims = stage.input_shapes[i].num_dims;
      memcpy(in_tensor[i].shape.dims, stage.input_shapes[i].dims,
             in_tensor[i].shape.num_dims * sizeof(int));
    }
  } else {
    for (auto i = 0; i < mp_mi->in.num; i++) {
      bmrt_tensor(&in_tensor[i], mp_mi->handle, net_info->input_dtypes[i], stage.input_shapes[i]);
    }
  }

  for (auto i = 0; i < mp_mi->out.num; i++) {
    bmrt_tensor(&out_tensor[i], mp_mi->handle, net_info->output_dtypes[i], stage.output_shapes[i]);
    bm_status_t status =
        bm_mem_mmap_device_mem(bm_handle, &out_tensor[i].device_mem,
                               reinterpret_cast<long long unsigned int *>(&out_raw_pointer[i]));
    if (CVI_TDL_SUCCESS != status) {
      cleanupError();
      return CVI_TDL_ERR_OPEN_MODEL;
    }
  }

  if (mp_mi->conf.input_mem_type == CVI_MEM_SYSTEM) {
    for (auto i = 0; i < mp_mi->in.num; i++) {
      mp_mi->in.raw_pointer.push_back(new char[bmrt_tensor_bytesize(&in_tensor[i])]);
    }
  }

  m_input_tensor_info.clear();
  setupInputTensorInfo(net_info, mp_mi.get(), m_input_tensor_info);
  m_output_tensor_info.clear();
  setupOutputTensorInfo(net_info, mp_mi.get(), m_output_tensor_info);  // to update raw_pointer

  /* input preprocess param */
  m_vpss_config.clear();
  input_preprocess_config(net_info, m_vpss_config);

  ret = onModelOpened();
  if (ret != CVI_TDL_SUCCESS) {
    LOGE("return failed in onModelOpened\n");
    onModelClosed();
  }

  return 0;
}

void Core::input_preprocess_config(const bm_net_info_t *net_info,
                                   std::vector<VPSSConfig> &m_vpss_config) {
  bool use_quantize_scale;
  for (uint32_t i = 0; i < (uint32_t)mp_mi->in.num; i++) {
    use_quantize_scale = net_info->input_scales[i] == 1 ? false : true;
  }

  auto &stage = net_info->stages[0];
  // FIXME: Behavior will changed in 1822.
  constexpr float factor_limit = 8191.f / 8192;

  for (auto i = 0; i < mp_mi->in.num; i++) {
    if (use_quantize_scale) {
      float quant_scale = net_info->input_scales[i];

      for (uint32_t j = 0; j < 3; j++) {
        m_preprocess_param[i].factor[j] *= quant_scale;
        m_preprocess_param[i].mean[j] *= quant_scale;

        if (m_preprocess_param[i].factor[j] > factor_limit) {
          LOGW("factor[%d] is bigger than limit: %f\n", i, m_preprocess_param[i].factor[j]);
          m_preprocess_param[i].factor[j] = factor_limit;
        }
      }
    }

    VPSSConfig vcfg;
    // FIXME: Future support for nhwc input. Currently disabled.
    auto height = stage.input_shapes->dims[2];
    auto width = stage.input_shapes->dims[3];
    vcfg.frame_type = CVI_FRAME_PLANAR;

    vcfg.rescale_type = m_preprocess_param[i].rescale_type;
    vcfg.crop_attr.bEnable = m_preprocess_param[i].use_crop;

    if (net_info->input_dtypes[i] == BM_UINT8) {
      m_preprocess_param[i].format = PIXEL_FORMAT_UINT8_C3_PLANAR;
    }
    bool pad_reverse = false;
    VPSS_CHN_SQ_HELPER(&vcfg.chn_attr, width, height, m_preprocess_param[i].format,
                       m_preprocess_param[i].factor, m_preprocess_param[i].mean, pad_reverse);
    if (!m_preprocess_param[i].keep_aspect_ratio) {
      vcfg.chn_attr.stAspectRatio.enMode = ASPECT_RATIO_NONE;
    }
    vcfg.chn_coeff = m_preprocess_param[i].resize_method;
    m_vpss_config.push_back(vcfg);
  }
}

void Core::setupInputTensorInfo(const bm_net_info_t *net_info, CvimodelInfo *p_mi,
                                std::map<std::string, TensorInfo> &tensor_info) {
  for (int32_t i = 0; i < p_mi->in.num; i++) {
    TensorInfo tinfo;
    memset(&tinfo, 0, sizeof(tinfo));
    tinfo.tensor_name = net_info->input_names[i];
    auto &stages = net_info->stages[0];
    tinfo.shape.dim_size = stages.input_shapes[i].num_dims;
    tinfo.shape.dim = std::vector<int>(
        stages.input_shapes[i].dims, stages.input_shapes[i].dims + stages.input_shapes[i].num_dims);

    tinfo.tensor_elem = bmrt_shape_count(&stages.input_shapes[i]);
    tinfo.tensor_size = tinfo.tensor_elem * bmrt_data_type_size(net_info->input_dtypes[i]);
    tinfo.data_type = net_info->input_dtypes[i];
    tinfo.qscale = net_info->input_scales[i];
    if (mp_mi->conf.input_mem_type == CVI_MEM_SYSTEM) {
      tinfo.raw_pointer = p_mi->in.raw_pointer[i];
    }
    tensor_info.emplace(std::pair<std::string, TensorInfo>(tinfo.tensor_name, tinfo));
    LOGI("input:%s,elem_num:%d,elem_size:%d\n", tinfo.tensor_name.c_str(), int(tinfo.tensor_elem),
         int(tinfo.tensor_size));
  }
}

void Core::setupOutputTensorInfo(const bm_net_info_t *net_info, CvimodelInfo *p_mi,
                                 std::map<std::string, TensorInfo> &tensor_info) {
  for (int32_t i = 0; i < p_mi->out.num; i++) {
    TensorInfo tinfo;
    memset(&tinfo, 0, sizeof(tinfo));
    tinfo.tensor_name = net_info->output_names[i];
    auto &stages = net_info->stages[0];
    tinfo.shape.dim_size = stages.output_shapes[i].num_dims;
    tinfo.shape.dim =
        std::vector<int>(stages.output_shapes[i].dims,
                         stages.output_shapes[i].dims + stages.output_shapes[i].num_dims);

    tinfo.tensor_elem = bmrt_shape_count(&stages.output_shapes[i]);
    tinfo.tensor_size = tinfo.tensor_elem * bmrt_data_type_size(net_info->output_dtypes[i]);
    tinfo.data_type = net_info->output_dtypes[i];
    tinfo.qscale = net_info->output_scales[i];
    tinfo.raw_pointer = p_mi->out.raw_pointer[i];
    tensor_info.emplace(std::pair<std::string, TensorInfo>(tinfo.tensor_name, tinfo));
    LOGI("output:%s,elem_num:%d,elem_size:%d\n", tinfo.tensor_name.c_str(), int(tinfo.tensor_elem),
         int(tinfo.tensor_size));
  }
}

int Core::after_inference() { return CVI_TDL_SUCCESS; }

int Core::modelClose() {
  int ret = CVI_TDL_SUCCESS;

  if (mp_mi == nullptr || mp_mi->handle == nullptr) {
    LOGD("modelClose fail\n");
    return CVI_TDL_FAILURE;
  }

  if (!use_mmap) {
    for (int i = 0; i < net_info->input_num; ++i) {
      bm_free_device(bm_handle, mp_mi->in.tensors[i].device_mem);
    }

    for (auto ptr : mp_mi->out.raw_pointer) {
      delete[] ptr;
    }
  } else {
    for (int32_t i = 0; i < mp_mi->out.num; i++) {
      auto size = bm_mem_get_device_size(mp_mi->out.tensors[i].device_mem);
      bm_status_t status = bm_mem_unmap_device_mem(bm_handle, mp_mi->out.raw_pointer[i], size);

      if (CVI_TDL_SUCCESS != status) {
        LOGE("bm_mem_unmap_device_mem failed: %s\n");
        return CVI_TDL_FAILURE;
      }
    }
  }

  if (mp_mi->conf.input_mem_type == CVI_MEM_SYSTEM) {
    for (auto ptr : mp_mi->in.raw_pointer) {
      delete[] ptr;
    }
  }
  if (register_temp_buffer) {
    delete[] register_temp_buffer;
    register_temp_buffer = nullptr;
  }
  for (int32_t i = 0; i < mp_mi->out.num; i++) {
    bm_free_device(bm_handle, mp_mi->out.tensors[i].device_mem);
  }

  cleanupError();
  onModelClosed();
  return ret;
}

const TensorInfo &Core::getOutputTensorInfo(const std::string &name) {
  if (m_output_tensor_info.find(name) != m_output_tensor_info.end()) {
    return m_output_tensor_info[name];
  }
  throw std::invalid_argument("cannot find output tensor name: " + name);
}

const TensorInfo &Core::getInputTensorInfo(const std::string &name) {
  if (m_input_tensor_info.find(name) != m_input_tensor_info.end()) {
    return m_input_tensor_info[name];
  }
  throw std::invalid_argument("cannot find input tensor name: " + name);
}

const TensorInfo &Core::getOutputTensorInfo(size_t index) {
  size_t cur = 0;
  for (auto iter = m_output_tensor_info.begin(); iter != m_output_tensor_info.end(); iter++) {
    if (cur == index) {
      return iter->second;
    }
    cur++;
  }
  throw std::out_of_range("out of range");
}

const TensorInfo &Core::getInputTensorInfo(size_t index) {
  size_t cur = 0;
  for (auto iter = m_input_tensor_info.begin(); iter != m_input_tensor_info.end(); iter++) {
    if (cur == index) {
      return iter->second;
    }
    cur++;
  }
  throw std::out_of_range("out of range");
}

size_t Core::getNumInputTensor() const { return static_cast<size_t>(mp_mi->in.num); }

size_t Core::getNumOutputTensor() const { return static_cast<size_t>(mp_mi->out.num); }

int Core::setVpssTimeout(uint32_t timeout) {
  m_vpss_timeout = timeout;
  return CVI_TDL_SUCCESS;
}

int Core::setVpssEngine(VpssEngine *engine) {
  mp_vpss_inst = engine;
  return CVI_TDL_SUCCESS;
}

int Core::setVpssDepth(uint32_t in_index, uint32_t depth) {
  if (m_vpss_config.size() <= 0) {
    LOGE("Model is not opened yet! Please set vpss depth when model is ready.\n");
    return CVI_TDL_ERR_NOT_YET_INITIALIZED;
  }

  if (in_index >= m_vpss_config.size()) {
    LOGE("Wrong input index: %d\n", in_index);
    return CVI_TDL_ERR_INVALID_ARGS;
  }

  m_vpss_config[in_index].chn_attr.u32Depth = depth;
  return CVI_TDL_SUCCESS;
}

int Core::getVpssDepth(uint32_t in_index, uint32_t *depth) {
  if (m_vpss_config.size() <= 0) {
    LOGE("Model is not opened yet! Please set vpss depth when model is ready.\n");
    return CVI_TDL_ERR_NOT_YET_INITIALIZED;
  }

  if (in_index >= m_vpss_config.size()) {
    LOGE("Wrong input index: %d\n", in_index);
    return CVI_TDL_ERR_INVALID_ARGS;
  }
  *depth = m_vpss_config[in_index].chn_attr.u32Depth;
  return CVI_TDL_SUCCESS;
}

void Core::skipVpssPreprocess(bool skip) { m_skip_vpss_preprocess = skip; }

int Core::getChnConfig(const uint32_t width, const uint32_t height, const uint32_t idx,
                       cvtdl_vpssconfig_t *chn_config) {
  if (!allowExportChannelAttribute()) {
    LOGE("This model does not support exporting channel attributes.\n");
    return CVI_TDL_ERR_GET_VPSS_CHN_CONFIG;
  }

  if (!isInitialized()) {
    LOGE(
        "Model is not yet opened. Please call CVI_TDL_OpenModel to initialize model before getting "
        "channel config.\n");
    return CVI_TDL_ERR_NOT_YET_INITIALIZED;
  }

  if (idx >= (uint32_t)mp_mi->in.num) {
    LOGE("Input index exceed input tensor num.\n");
    return CVI_TDL_ERR_GET_VPSS_CHN_CONFIG;
  }

  if (!m_skip_vpss_preprocess) {
    LOGW("VPSS preprocessing is enabled. Remember to skip vpss preprocess.\n");
  }

  switch (m_vpss_config[idx].rescale_type) {
    case RESCALE_CENTER: {
      chn_config->chn_attr = m_vpss_config[idx].chn_attr;
    } break;
    case RESCALE_RB: {
      CVI_SHAPE input_shape = getInputShape(idx);
      auto input_h = input_shape.dim[2];
      auto input_w = input_shape.dim[3];
      auto &factor = m_vpss_config[idx].chn_attr.stNormalize.factor;
      auto &mean = m_vpss_config[idx].chn_attr.stNormalize.mean;
      VPSS_CHN_SQ_RB_HELPER(&chn_config->chn_attr, width, height, input_w, input_h,
                            m_vpss_config[idx].chn_attr.enPixelFormat, factor, mean, false);
      chn_config->chn_attr.stAspectRatio.u32BgColor =
          m_vpss_config[idx].chn_attr.stAspectRatio.u32BgColor;
    } break;
    default: {
      LOGE("Unsupported rescale type.\n");
      return CVI_TDL_ERR_GET_VPSS_CHN_CONFIG;
    } break;
  }
  chn_config->chn_coeff = m_vpss_config[idx].chn_coeff;
  return 0;
}

bool Core::isInitialized() { return mp_mi->handle == nullptr ? false : true; }

CVI_SHAPE Core::getInputShape(size_t index) { return getInputTensorInfo(index).shape; }

CVI_SHAPE Core::getOutputShape(size_t index) { return getOutputTensorInfo(index).shape; }

CVI_SHAPE Core::getInputShape(const std::string &name) { return getInputTensorInfo(name).shape; }

CVI_SHAPE Core::getOutputShape(const std::string &name) { return getOutputTensorInfo(name).shape; }

float Core::getInputQuantScale(size_t index) { return getInputTensorInfo(index).qscale; }

float Core::getInputQuantScale(const std::string &name) { return getInputTensorInfo(name).qscale; }

size_t Core::getOutputTensorElem(size_t index) { return getOutputTensorInfo(index).tensor_elem; }

size_t Core::getOutputTensorElem(const std::string &name) {
  return getOutputTensorInfo(name).tensor_elem;
}

size_t Core::getInputTensorElem(size_t index) { return getInputTensorInfo(index).tensor_elem; }

size_t Core::getInputTensorElem(const std::string &name) {
  return getInputTensorInfo(name).tensor_elem;
}

size_t Core::getOutputTensorSize(size_t index) { return getOutputTensorInfo(index).tensor_size; }

size_t Core::getOutputTensorSize(const std::string &name) {
  return getOutputTensorInfo(name).tensor_size;
}

size_t Core::getInputTensorSize(size_t index) { return getInputTensorInfo(index).tensor_size; }

size_t Core::getInputTensorSize(const std::string &name) {
  return getInputTensorInfo(name).tensor_size;
}

int Core::vpssPreprocess(VIDEO_FRAME_INFO_S *srcFrame, VIDEO_FRAME_INFO_S *dstFrame,
                         VPSSConfig &vpss_config) {
  int ret;
  LOGI("to vpss preprocess,crop_enable:%d\n", (int)vpss_config.crop_attr.bEnable);
  if (!vpss_config.crop_attr.bEnable) {
    ret = mp_vpss_inst->sendFrame(srcFrame, &vpss_config.chn_attr, &vpss_config.chn_coeff, 1);
  } else {
    ret = mp_vpss_inst->sendCropChnFrame(srcFrame, &vpss_config.crop_attr, &vpss_config.chn_attr,
                                         &vpss_config.chn_coeff, 1);
  }
  if (ret != CVI_SUCCESS) {
    LOGE("Send frame failed: %s!\n", get_vpss_error_msg(ret));
    return CVI_TDL_ERR_VPSS_SEND_FRAME;
  }

  ret = mp_vpss_inst->getFrame(dstFrame, 0, m_vpss_timeout);
  if (ret != CVI_SUCCESS) {
    LOGE("Get frame failed: %s!\n", get_vpss_error_msg(ret));
    return CVI_TDL_ERR_VPSS_GET_FRAME;
  }
  return 0;
}

int Core::run(std::vector<VIDEO_FRAME_INFO_S *> &frames) {
  int ret = CVI_TDL_SUCCESS;
  if (m_skip_vpss_preprocess && !allowExportChannelAttribute()) {
    return CVI_TDL_ERR_INVALID_ARGS;
  }
  std::vector<std::shared_ptr<VIDEO_FRAME_INFO_S>> dstFrames;

  if (frames.size() != 1) {
    LOGE("can only process one frame for aligninput,got frame_num:%d\n", int(frames.size()));
  }
  model_timer_.TicToc("runstart");
  if (mp_mi->conf.input_mem_type == CVI_MEM_DEVICE) {
    if (m_skip_vpss_preprocess) {
      // skip vpss preprocess is true, just register frame directly.
      ret = registerFrame2Tensor(frames);
    } else {
      if (m_vpss_config.size() != frames.size()) {
        LOGE("The size of vpss config does not match the number of frames. (%zu vs %zu)\n",
             m_vpss_config.size(), frames.size());
        return CVI_TDL_FAILURE;
      }

      dstFrames.reserve(frames.size());
      for (uint32_t i = 0; i < frames.size(); i++) {
        VIDEO_FRAME_INFO_S *f = new VIDEO_FRAME_INFO_S;
        memset(f, 0, sizeof(VIDEO_FRAME_INFO_S));

        auto releaseVideoFrame = [&](VIDEO_FRAME_INFO_S *f) {
          if (f->stVFrame.u64PhyAddr[0] != 0) {
            mp_vpss_inst->releaseFrame(f, 0);
          }
          delete f;
        };

        int vpssret = vpssPreprocess(frames[i], f, m_vpss_config[i]);
        if (vpssret != 0) {
          releaseVideoFrame(f);
          /* preprocess fail, auto delete frame. */
          LOGE("vpssPreprocess fail\n");
          return vpssret;
        } else {
          dstFrames.push_back(std::shared_ptr<VIDEO_FRAME_INFO_S>(f, releaseVideoFrame));
        }
      }

      ret = registerFrame2Tensor(dstFrames);
    }
  } else {
    for (size_t i = 0; i < mp_mi->in.num; i++) {
      bm_memcpy_s2d_partial(bm_handle, mp_mi->in.tensors[i].device_mem,
                            (void *)mp_mi->in.raw_pointer[i],
                            bmrt_tensor_bytesize(&mp_mi->in.tensors[i]));
    }
  }
  model_timer_.TicToc("vpss");
  if (ret != CVI_TDL_SUCCESS) {
    LOGE("registerFrame2Tensor failed: Unsupport operation.\n");
    return ret;
  }
  ret =
      bmrt_launch_tensor_ex(mp_mi->handle, mp_mi->net_names[0], mp_mi->in.tensors.data(),
                            mp_mi->in.num, mp_mi->out.tensors.data(), mp_mi->out.num, true, false);
  if (ret == true) {
    auto &out_tensor = mp_mi->out.tensors;
    auto &raw_pointer = mp_mi->out.raw_pointer;
    bm_thread_sync(bm_handle);

    for (auto i = 0; i < mp_mi->out.num; i++) {
      bm_status_t status = bm_mem_invalidate_device_mem(bm_handle, &(out_tensor[i].device_mem));
      if (CVI_TDL_SUCCESS != status) {
        cleanupError();
        return CVI_TDL_ERR_OPEN_MODEL;
      }
    }

  } else {
    LOGE("bmrt_launch_tensor_ex failed: Unsupport operation.\n");
    return ret;
  }
  model_timer_.TicToc("tpu");
  return CVI_TDL_SUCCESS;
}
template <typename T>
int Core::registerFrame2Tensor(std::vector<T> &frames) {
  CVI_SHAPE input_shape = getInputShape(0);
  uint32_t input_w = input_shape.dim[3];
  uint32_t input_h = input_shape.dim[2];

  if (!raw) {
    if (frames.size() != (uint32_t)mp_mi->in.num) {
      LOGE("frames.size() != (uint32_t)mp_mi->in.num\n");
      return CVI_TDL_FAILURE;
    }
  }

  for (uint32_t i = 0; i < (uint32_t)frames.size(); i++) {
    auto frame = frames[i];
    if (!raw) {
      if (input_w != frame->stVFrame.u32Width || input_h != frame->stVFrame.u32Height) {
        LOGE("input frame shape[%u,%u] not equal with tensor input[%u,%u]",
             frame->stVFrame.u32Width, frame->stVFrame.u32Height, input_w, input_h);
        return CVI_TDL_FAILURE;
      }
    }

    size_t image_size =
        frame->stVFrame.u32Length[0] + frame->stVFrame.u32Length[1] + frame->stVFrame.u32Length[2];

    auto size = bmrt_tensor_bytesize(&mp_mi->in.tensors[i]);
    if (use_mmap) {
      bm_set_device_mem(&(mp_mi->in.tensors[i].device_mem), size,
                        (unsigned long long)frame->stVFrame.u64PhyAddr[0]);

      bm_status_t status = bm_mem_flush_device_mem(bm_handle, &(mp_mi->in.tensors[i].device_mem));
      assert(BM_SUCCESS == status);
    } else {
      frame->stVFrame.pu8VirAddr[0] =
          (CVI_U8 *)CVI_SYS_Mmap(frame->stVFrame.u64PhyAddr[0], image_size);
      frame->stVFrame.pu8VirAddr[1] = frame->stVFrame.pu8VirAddr[0] + frame->stVFrame.u32Length[0];
      frame->stVFrame.pu8VirAddr[2] = frame->stVFrame.pu8VirAddr[1] + frame->stVFrame.u32Length[1];

      int input_height = mp_mi->in.tensors[i].shape.dims[2];
      int input_width = mp_mi->in.tensors[i].shape.dims[3];

      if (input_width % ALIGN_SIZE == 0) {
        bm_set_device_mem(&(mp_mi->in.tensors[i].device_mem), size,
                          (unsigned long long)frame->stVFrame.u64PhyAddr[0]);
      } else {
        int align_length = (input_width / ALIGN_SIZE + 1) * ALIGN_SIZE;
        uint64_t image_bgr_devaddr = bm_mem_get_device_addr(mp_mi->in.tensors[i].device_mem);
        int input_height_squared = input_width * input_height;
        //  for(for(s2s_cpy))+s2d_cpy greater than for(for(s2d_cpy))
        if (!register_temp_buffer) {
          register_temp_buffer = new uint8_t[3 * input_height_squared];
        }
        for (int c = 0; c < 3; c++) {
          for (int h = 0; h < input_height; h++) {
            memcpy(register_temp_buffer + c * input_height_squared + h * input_width,
                   frame->stVFrame.pu8VirAddr[c] + h * align_length, input_width);
          }
        }
        bm_device_mem_t dst_addr = bm_mem_from_device(image_bgr_devaddr, image_size);
        bm_memcpy_s2d_partial(bm_handle, dst_addr, (void *)(register_temp_buffer), image_size);
      }

      CVI_SYS_Munmap((void *)frame->stVFrame.pu8VirAddr[0], image_size);
      frame->stVFrame.pu8VirAddr[0] = NULL;
      frame->stVFrame.pu8VirAddr[1] = NULL;
      frame->stVFrame.pu8VirAddr[2] = NULL;
    }
  }
  return CVI_TDL_SUCCESS;
}

/* vpssCropImage api need new  dstFrame and remember delete and release frame*/
int Core::vpssCropImage(VIDEO_FRAME_INFO_S *srcFrame, VIDEO_FRAME_INFO_S *dstFrame,
                        cvtdl_bbox_t bbox, uint32_t rw, uint32_t rh, PIXEL_FORMAT_E enDstFormat,
                        VPSS_SCALE_COEF_E reize_mode /* = VPSS_SCALE_COEF_NEAREST*/) {
  VPSS_CROP_INFO_S cropAttr;
  cropAttr.bEnable = true;
  uint32_t u32Width = bbox.x2 - bbox.x1;
  uint32_t u32Height = bbox.y2 - bbox.y1;
  cropAttr.stCropRect = {(int)bbox.x1, (int)bbox.y1, u32Width, u32Height};
  VPSS_CHN_ATTR_S chnAttr;
  VPSS_CHN_DEFAULT_HELPER(&chnAttr, rw, rh, enDstFormat, false);
  int ret = mp_vpss_inst->sendCropChnFrame(srcFrame, &cropAttr, &chnAttr, &reize_mode, 1);
  if (ret != CVI_SUCCESS) return ret;
  ret = mp_vpss_inst->getFrame(dstFrame, 0, 2000);
  return ret;
}

/* vpssCropImage api need new  dstFrame and remember delete and release frame*/
int Core::vpssChangeImage(VIDEO_FRAME_INFO_S *srcFrame, VIDEO_FRAME_INFO_S *dstFrame, uint32_t rw,
                          uint32_t rh, PIXEL_FORMAT_E enDstFormat) {
  VPSS_CHN_ATTR_S chnAttr;
  VPSS_CHN_DEFAULT_HELPER(&chnAttr, rw, rh, enDstFormat, false);
  mp_vpss_inst->sendFrame(srcFrame, &chnAttr, 1);
  mp_vpss_inst->getFrame(dstFrame, 0, 2000);
  return 0;
}

}  // namespace cvitdl
