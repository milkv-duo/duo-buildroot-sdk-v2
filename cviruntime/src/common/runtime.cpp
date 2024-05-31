#include <sys/mman.h>
#include <unistd.h>
#include <dlfcn.h>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <mutex>
#include <string.h>
#include <runtime/debug.h>
#include <runtime/model.hpp>
#include <runtime/stream.hpp>
#include <runtime/shared_mem.hpp>
#include "cviruntime.h"
#include "cviruntime_context.h"
#include "alloc.h"

using namespace cvi::runtime;

static CVI_RT_HANDLE g_ctx = nullptr;
static int g_ctx_ref_count = 0;
static std::mutex g_ctx_mutex;
static int g_model_count = 0;

struct ModelInstance {
  ModelInstance(CviModel *model) :
      model(model), program(nullptr) {
    program_num = model->program_num;
  }

  ~ModelInstance() {
    if (inputs) {
      delete[] inputs;
    }
    if (outputs) {
      delete[] outputs;
    }
    if (program) {
      delete program;
    }
    model->release();
  }

  cvi::runtime::CviModel *model;
  cvi::runtime::Program *program = nullptr;
  CVI_TENSOR *inputs = nullptr;
  CVI_TENSOR *outputs = nullptr;
  int32_t input_num = 0;
  int32_t output_num = 0;
  int32_t program_id = 0;
  int32_t program_num = 1;
  bool output_all_tensors_for_debug = false;
  bool skip_preprocess = false;
};

static void setChipTypeForCmodel(const char *modelFile, const int8_t *buf, size_t size) {
#if defined(__x86_64__) || defined(_M_X64)
  std::string filename = modelFile != nullptr ? modelFile : "";
  std::string chip_target = CviModel::getChipType(filename, buf, size);
  setenv("SET_CHIP_NAME", chip_target.c_str(), 1);
  TPU_LOG_ERROR("setenv:%s\n", chip_target.c_str());
#endif
}

static void setChipTypeForCmodelFd(const int fd, const size_t ud_offset) {
#if defined(__x86_64__) || defined(_M_X64)
  BaseStream *stream = new FdStream(fd, ud_offset);
  if (stream->length() <= sizeof(MODEL_HEADER)) {
    TPU_LOG_ERROR("Error, invalid cvimodel file\n");
    assert(0);
  }
  MODEL_HEADER header;
  stream->read((uint8_t *)&header, 0, sizeof(header));
  delete stream;
  std::string chip_target = std::string(header.chip);
  setenv("SET_CHIP_NAME", chip_target.c_str(), 1);
  TPU_LOG_ERROR("setenv:%s\n", chip_target.c_str());
#endif
}

//According to the file descriptor and user defined offset to construct model.
CVI_RC CVI_NN_RegisterModelFromFd(const int fd, const size_t ud_offset, CVI_MODEL_HANDLE *model) {
  *model = NULL;
  const std::lock_guard<std::mutex> lock(g_ctx_mutex);

  if (!g_ctx) {
    setChipTypeForCmodelFd(fd, ud_offset);
    CVI_RT_Init(&g_ctx);
  }

  auto _model = new CviModel(g_ctx, g_model_count++);
  if (!_model) {
    TPU_LOG_ERROR("failed to create a CviModel Instance\n");
    return CVI_RC_FAILURE;
  }
  CVI_RC ret = _model->acquire(fd, ud_offset);
  if (ret != CVI_RC_SUCCESS) {
    _model->release();
    return ret;
  }
  auto instance = new ModelInstance(_model);
  if (!instance) {
    _model->release();
    return CVI_RC_FAILURE;
  }

  g_ctx_ref_count++;
  *model = (void *)instance;
  return CVI_RC_SUCCESS;
}

CVI_RC CVI_NN_RegisterModel(const char *modelFile, CVI_MODEL_HANDLE *model) {
  *model = NULL;
  const std::lock_guard<std::mutex> lock(g_ctx_mutex);

  if (!g_ctx) {
    setChipTypeForCmodel(modelFile, nullptr, 0);
    CVI_RT_Init(&g_ctx);
  }

  auto _model = new CviModel(g_ctx, g_model_count++);
  if (!_model) {
    TPU_LOG_ERROR("failed to create a CviModel Instance\n");
    return CVI_RC_FAILURE;
  }
  CVI_RC ret = _model->acquire(modelFile);
  if (ret != CVI_RC_SUCCESS) {
    _model->release();
    return ret;
  }
  auto instance = new ModelInstance(_model);
  if (!instance) {
    _model->release();
    return CVI_RC_FAILURE;
  }

  g_ctx_ref_count++;
  *model = (void *)instance;
  return CVI_RC_SUCCESS;
}

CVI_RC CVI_NN_RegisterModelFromBuffer(const int8_t *buf, uint32_t size,
                                      CVI_MODEL_HANDLE *model) {
  *model = NULL;
  const std::lock_guard<std::mutex> lock(g_ctx_mutex);

  if (!g_ctx) {
    setChipTypeForCmodel(nullptr, buf, size);
    CVI_RT_Init(&g_ctx);
  }

  auto _model = new CviModel(g_ctx, g_model_count++);
  if (!_model) {
    TPU_LOG_ERROR("failed to create a CviModel Instance\n");
    return CVI_RC_FAILURE;
  }
  CVI_RC ret = _model->acquire(buf, size);
  if (ret != CVI_RC_SUCCESS) {
    _model->release();
    return ret;
  }
  auto instance = new ModelInstance(_model);
  if (!instance) {
    _model->release();
    return CVI_RC_FAILURE;
  }

  g_ctx_ref_count++;
  *model = (void *)instance;
  return CVI_RC_SUCCESS;
}



CVI_RC CVI_NN_CloneModel(CVI_MODEL_HANDLE model, CVI_MODEL_HANDLE *clonedModel) {
  const std::lock_guard<std::mutex> lock(g_ctx_mutex);
  ++g_ctx_ref_count;
  auto instance = new ModelInstance(((struct ModelInstance *)model)->model);
  if (!instance) {
    --g_ctx_ref_count;
    return CVI_RC_FAILURE;
  }
  instance->model->refer();
  *clonedModel = (void *)instance;
  return CVI_RC_SUCCESS;
}

CVI_RC CVI_NN_GetModelVersion(CVI_MODEL_HANDLE model, int32_t *major, int32_t *minor) {
  auto instance = (struct ModelInstance *)model;
  *major = instance->model->major_ver;
  *minor = instance->model->minor_ver;
  return CVI_RC_SUCCESS;
}

const char *CVI_NN_GetModelTarget(CVI_MODEL_HANDLE model) {
  auto instance = (struct ModelInstance *)model;
  return instance->model->targetChipType.c_str();
}

CVI_RC CVI_NN_SetConfig(CVI_MODEL_HANDLE model, CVI_CONFIG_OPTION option, ...) {
  va_list valist;
  auto instance = (struct ModelInstance *)model;

  va_start(valist, option);
  switch (option) {
    case OPTION_BATCH_SIZE:
      instance->program_id = 0;
      break;
    case OPTION_OUTPUT_ALL_TENSORS:
      instance->output_all_tensors_for_debug = va_arg(valist, int32_t);
      break;
    case OPTION_PROGRAM_INDEX:
      instance->program_id = va_arg(valist, int32_t);
      assert(instance->program_id < instance->program_num);
      break;
    case OPTION_SKIP_PREPROCESS:
    case OPTION_SKIP_POSTPROCESS:
    case OPTION_INPUT_MEM_TYPE:
    case OPTION_OUTPUT_MEM_TYPE:
    case OPTION_PREPARE_BUF_FOR_INPUTS:
    case OPTION_PREPARE_BUF_FOR_OUTPUTS:
      TPU_LOG_WARNING("deprecated option:%d\n", (int)option);
      break;
    default:
      TPU_LOG_ERROR("unsupported option:%d\n", (int)option);
      assert(0);
  }
  va_end(valist);
  return CVI_RC_SUCCESS;
}

CVI_RC CVI_NN_GetInputOutputTensors(CVI_MODEL_HANDLE model, CVI_TENSOR **inputs,
                              int32_t *input_num, CVI_TENSOR **outputs,
                              int32_t *output_num) {
  CVI_RC ret;
  auto instance = (struct ModelInstance *)model;
  if (!instance->program) {
    ret = instance->model->loadProgram(
        &(instance->program), instance->program_id,
        instance->output_all_tensors_for_debug,
        instance->skip_preprocess);
    if (ret != CVI_RC_SUCCESS) {
      TPU_LOG_ERROR("ret:%d\n", ret);
      return ret;
    }
  }

  if (!instance->inputs) {
    instance->inputs = instance->program->exportInputs(instance->input_num);
    if (!instance->inputs) {
      return CVI_RC_FAILURE;
    }
  }
  if (!instance->outputs) {
    instance->outputs = instance->program->exportOutputs(instance->output_num);
    if (!instance->outputs) {
      return CVI_RC_FAILURE;
    }
  }

  if (inputs)
    *inputs = instance->inputs;
  if (input_num)
    *input_num = instance->input_num;
  if (outputs)
    *outputs = instance->outputs;
  if (output_num)
    *output_num = instance->output_num;
  return CVI_RC_SUCCESS;
}

CVI_RC CVI_NN_Forward(CVI_MODEL_HANDLE model, CVI_TENSOR inputs[], int32_t input_num,
                      CVI_TENSOR outputs[], int output_num) {
  auto instance = (struct ModelInstance *)model;
  if (instance->program->forward(inputs, input_num, outputs, output_num))
    return CVI_RC_SUCCESS;
  return CVI_RC_FAILURE;
}

CVI_RC CVI_NN_ForwardAsync(CVI_MODEL_HANDLE model, CVI_TENSOR inputs[], int input_num,
                           CVI_TENSOR outputs[], int output_num, void **taskNo) {
  auto instance = (struct ModelInstance *)model;
  *taskNo = instance->program->forwardAsync(inputs, input_num, outputs, output_num);
  return CVI_RC_SUCCESS;
}

CVI_RC CVI_NN_ForwardWait(CVI_MODEL_HANDLE model, void *taskNo) {
  auto instance = (struct ModelInstance *)model;
  return instance->program->forwardWait(taskNo);
}

CVI_RC CVI_NN_CleanupModel(CVI_MODEL_HANDLE model) {
  if (model) {
    delete (struct ModelInstance *)model;
  }

  const std::lock_guard<std::mutex> lock(g_ctx_mutex);
  g_ctx_ref_count--;
  if (g_ctx_ref_count == 0) {
    CVI_RT_DeInit(g_ctx);
    g_ctx = nullptr;
  }
  return CVI_RC_SUCCESS;
}

///
/// Helper functions
///
CVI_RC CVI_NN_GetInputTensors(CVI_MODEL_HANDLE model, CVI_TENSOR **inputs, int32_t *input_num) {
  return CVI_NN_GetInputOutputTensors(model, inputs, input_num, nullptr, nullptr);
}

CVI_RC CVI_NN_GetOutputTensors(CVI_MODEL_HANDLE model, CVI_TENSOR **outputs, int32_t *output_num) {
  return CVI_NN_GetInputOutputTensors(model, nullptr, nullptr, outputs, output_num);
}

CVI_TENSOR *CVI_NN_GetTensorByName(const char *name, CVI_TENSOR *tensors, int32_t num) {
  if (name == CVI_NN_DEFAULT_TENSOR) {
    if (num == 1) {
      return &tensors[0];
    } else {
      return NULL;
    }
  }
  // if last char of name is '*', use strncmp instead.
  int sz = strlen(name);
  bool has_wildcard = (name[sz - 1] == '*');
  for (int32_t i = 0; i < num; i++) {
    if (!has_wildcard) {
      if (strcmp(tensors[i].name, name) == 0) {
        return &tensors[i];
      }
    } else if (strncmp(tensors[i].name, name, sz - 1) == 0) {
      return &tensors[i];
    }
  }
  return NULL;
}

char *CVI_NN_TensorName(CVI_TENSOR *tensor) { return tensor->name; }

void *CVI_NN_TensorPtr(CVI_TENSOR *tensor) {
  if (tensor->mem_type == CVI_MEM_SYSTEM) {
    return (void *)tensor->sys_mem;
  } else if (tensor->mem_type == CVI_MEM_DEVICE) {
    TPU_LOG_ERROR("Try to get mem ptr with device memory\n");
    return nullptr;
  } else {
    TPU_LOG_ERROR("Try to get mem ptr with unknown type\n");
    return nullptr;
  }
}

size_t CVI_NN_TensorSize(CVI_TENSOR *tensor) {
  return tensor->mem_size;
}

size_t CVI_NN_TensorCount(CVI_TENSOR *tensor) {
  return tensor->count;
}

CVI_SHAPE CVI_NN_TensorShape(CVI_TENSOR *tensor) {
  return tensor->shape;
}

float CVI_NN_TensorQuantScale(CVI_TENSOR *tensor) {
  return tensor->qscale;
}

int CVI_NN_TensorQuantZeroPoint(CVI_TENSOR *tensor){
  return tensor->zero_point;
}

CVI_RC CVI_NN_SetTensorPtr(CVI_TENSOR *tensor, void *mem) {
  assert(mem);
  tensor->sys_mem = (uint8_t *)mem;
  tensor->mem_type = CVI_MEM_SYSTEM;
  return CVI_RC_SUCCESS;
}

CVI_RC CVI_NN_SetTensorPhysicalAddr(CVI_TENSOR *tensor, uint64_t paddr) {
  tensor->paddr = paddr;
  tensor->mem_type = CVI_MEM_DEVICE;

  assert(tensor->owner);
  auto program = static_cast<cvi::runtime::Program *>(tensor->owner);
  for (auto &input_tensor : program->input_tensors()) {
    if (input_tensor->name == tensor->name) {
      input_tensor->updateBaseAddr(paddr);
      return CVI_RC_SUCCESS;
    }
  }
  for (auto &output_tensor : program->output_tensors()) {
    if (output_tensor->name == tensor->name) {
      output_tensor->updateBaseAddr(paddr);
      return CVI_RC_SUCCESS;
    }
  }
  assert(0 && "invalid tensor");
  return CVI_RC_FAILURE;
}

static std::shared_ptr<Neuron> findTargetInput(CVI_TENSOR *tensor) {
  auto program = static_cast<cvi::runtime::Program *>(tensor->owner);
  for (auto &input : program->input_tensors()) {
    if (input->name == tensor->name) {
      return input;
    }
  }
  assert(0);
  return nullptr;
}

CVI_RC CVI_NN_SetTensorWithVideoFrame(
    CVI_MODEL_HANDLE model, CVI_TENSOR* tensor,
    CVI_VIDEO_FRAME_INFO* video_frame_info) {
  (void)model;

  // check param
  // check frame type
#if 0
  // video_frame_info->type is CVI_FRAME_PLANAR on early sampes,
  // so don't check
  if (tensor->pixel_format != video_frame_info->type) {
    TPU_LOG_ERROR("Frame format error! [need|%d] vs [input|%d]\n",
                  tensor->pixel_format, video_frame_info->type);
    return CVI_RC_DATA_ERR;
  }
#endif

  // check shape
  if (tensor->shape.dim[1] != video_frame_info->shape.dim[1] ||
      tensor->shape.dim[2] != video_frame_info->shape.dim[2] ||
      tensor->shape.dim[3] != video_frame_info->shape.dim[3]) {
      TPU_LOG_ERROR("Frame size error! [need|%d, %d, %d, %d] vs [input|%d, %d, %d, %d]\n",
                    tensor->shape.dim[0], tensor->shape.dim[1], tensor->shape.dim[2], tensor->shape.dim[3],
                    video_frame_info->shape.dim[0], video_frame_info->shape.dim[1],
                    video_frame_info->shape.dim[2], video_frame_info->shape.dim[3]);
      return CVI_RC_DATA_ERR;
  }

  int n = tensor->shape.dim[0];
  assert(n == 1);
  tensor->mem_type = CVI_MEM_DEVICE;

  auto input = findTargetInput(tensor);
  CVI_RC ret = CVI_RC_SUCCESS;
  if (!tensor->aligned) {
    int c = input->isPacked() ? 1 : tensor->shape.dim[1];
    assert(c <= 3);
    for (int i = 0; i < c; i++) {
      ret = input->preloadChannelAndCompact(i, video_frame_info->pyaddr[i]);
      if (ret != CVI_RC_SUCCESS) {
        TPU_LOG_ERROR("CVI_NN_SetTensorWithVideoFrame fail!");
        return ret;
      }
    }
  } else {
    /* check y_align w_align channel_align
       1.on cv183x channel_align is 0x1000
       2.on cv183x yuv_420_planar's y_align is 64, w_align is 32
       3.on cv182x yuv_420_planar's y_align is 128, w_align is 64
    */
    CVI_NN_SetTensorWithAlignedFrames(
        tensor, &(video_frame_info->pyaddr[0]), 1,
        video_frame_info->type);
  }
  return CVI_RC_SUCCESS;
}

CVI_RC CVI_NN_SetTensorWithAlignedFrames(
    CVI_TENSOR *tensor, uint64_t frame_paddrs[],
    int32_t frame_num, CVI_NN_PIXEL_FORMAT_E pixel_format) {

  assert(tensor->owner);
  assert(frame_num <= tensor->shape.dim[0]);
  tensor->mem_type = CVI_MEM_DEVICE;

  auto input = findTargetInput(tensor);
  CVI_RC ret = CVI_RC_SUCCESS;

  if (!tensor->aligned) {
    for (int i = 0; i < frame_num; i++) {
      ret = input->preloadFrameAndCompact(i, frame_paddrs[i]);
      if (ret != CVI_RC_SUCCESS) {
        TPU_LOG_ERROR("CVI_NN_SetTensorWithAlignedFrames unaligned fail!");
        return ret;
      }
    }
  } else {
    // check pixel format
    if (pixel_format != tensor->pixel_format) {
      TPU_LOG_ERROR("pixel_format is not correct, %d vs %d\n", tensor->pixel_format, pixel_format);
      assert(0);
    }
    if (frame_num == 1 && tensor->shape.dim[0] == 1) {
      CVI_NN_SetTensorPhysicalAddr(tensor, frame_paddrs[0]);
    } else {
      for (int i = 0; i < frame_num; i++) {
        ret = input->preload(i, frame_paddrs[i]);
        if (ret != CVI_RC_SUCCESS) {
            TPU_LOG_ERROR("CVI_NN_SetTensorWithAlignedFrames aligned fail!");
            return ret;
        }
      }
    }
  }
  return CVI_RC_SUCCESS;
}

CVI_RC CVI_NN_FeedTensorWithFrames(
    CVI_MODEL_HANDLE model, CVI_TENSOR *tensor,
    CVI_FRAME_TYPE type, CVI_FMT format,
    int32_t channel_num, uint64_t *channel_paddrs,
    int32_t height, int32_t width, uint32_t height_stride) {

  (void)model;
  (void)height_stride;
  (void)format;

  // check param
  // check frame type
#if 0
  // video_frame_info->type is CVI_FRAME_PLANAR on early sampes,
  // so don't check for now
  if (tensor->pixel_format != type) {
    TPU_LOG_ERROR("Frame format error! [need|%d] vs [input|%d]\n",
                  tensor->pixel_format, format);
    return CVI_RC_DATA_ERR;
  }
#endif

  auto input = findTargetInput(tensor);
  // check width height
  if (input->isPacked()) {
    if (tensor->shape.dim[1] != height || tensor->shape.dim[2] != width) {
      TPU_LOG_ERROR("Frame size error! [(w, h) need|%d, %d] vs [input|%d, %d]\n",
                      tensor->shape.dim[2], tensor->shape.dim[1], width, height);
      return CVI_RC_DATA_ERR;
    }
  } else {
    if (tensor->shape.dim[2] != height || tensor->shape.dim[3] != width) {
      TPU_LOG_ERROR("Frame size error! [(w, h) need|%d, %d] vs [input|%d, %d]\n",
                      tensor->shape.dim[3], tensor->shape.dim[2], width, height);
      return CVI_RC_DATA_ERR;
    }
  }

  int n = tensor->shape.dim[0];
  assert(n == 1);
  tensor->mem_type = CVI_MEM_DEVICE;
  CVI_RC ret = CVI_RC_SUCCESS;

  if (!tensor->aligned) {
    int c = input->isPacked() ? 1 : tensor->shape.dim[1];
    assert(channel_num <= c);
    for (int i = 0; i < channel_num; i++) {
      ret = input->preloadChannelAndCompact(i, channel_paddrs[i]);
      if (ret != CVI_RC_SUCCESS) {
        TPU_LOG_WARNING("FeedTensor failed\n");
        return CVI_RC_FAILURE;
      }
    }
  } else {
    /* check y_align w_align channel_align
       1.on cv183x channel_align is 0x1000
       2.on cv183x yuv_420_planar's y_align is 64, w_align is 32
       3.on cv182x yuv_420_planar's y_align is 128, w_align is 64
    */
    CVI_NN_SetTensorWithAlignedFrames(tensor, &(channel_paddrs[0]), 1, type);
  }
  return CVI_RC_SUCCESS;
}

CVI_RC CVI_RT_Global_SetMemAllocCallback(CVI_MEM_ALLOC_CB alloc_cb, CVI_MEM_FREE_CB free_cb) {
  return cviSetMemCallback(alloc_cb, free_cb);
}

void CVI_RT_Global_ResetMemAllocCallback() {
  return cviResetMemCallback();
}

void CVI_NN_Global_SetSharedMemorySize(size_t size) {
  setSharedMemSize(size);
}
