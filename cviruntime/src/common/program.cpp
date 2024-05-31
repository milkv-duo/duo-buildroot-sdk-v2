#include <unistd.h>
#include <dlfcn.h>
#include <iostream>
#include <sstream>
#include <mutex>
#include <set>
#include <algorithm>
#include <runtime/program.hpp>
#include <runtime/debug.h>
#include <runtime/shared_mem.hpp>
#include <cvibuilder/parameter_generated.h>
#include "cviruntime.h"
#include "alloc.h"

//#define MEASURE_TIME
#ifdef MEASURE_TIME
#include <sys/time.h>
#endif

namespace cvi {
namespace runtime {

Program::Program(CVI_RT_HANDLE ctx, TaskPool *pool,
                 dmabuf_map_t &dmabuf_map,
                 std::vector<CpuRuntimeFunction *> &functions,
                 tensor_map_t &weight_map, CVI_RT_MEM weight_mem,
                 const char *model_name,
                 size_t max_shared_mem_size)
    : weight_map(weight_map),
      dmabuf_map(dmabuf_map),
      cpu_functions(functions),
      _ctx(ctx), _pool(pool),
      _max_shared_mem_size(max_shared_mem_size) {

  _cvk = CVI_RT_RegisterKernel(ctx, 1024);
  for (int i = 0; i < 8; ++i) {
    baseAddrArray[i] = 0;
    baseMemArray[i] = nullptr;
  }
  baseMemArray[1] = weight_mem;
  baseAddrArray[1] = CVI_RT_MemGetPAddr(weight_mem);
  if (model_name) {
    _model_name = model_name;
  }
}

Program::~Program() {
  if (shared_mem) {
    deallocateSharedMemory(_ctx, shared_mem);
  }
  if (private_mem) {
    cviMemFree(_ctx, private_mem);
  }
  if (_cvk) {
    CVI_RT_UnRegisterKernel(_cvk);
  }
}

void Program::setOptions(bool export_all_tensors, bool skip_preprocess) {
  this->_export_all_tensors = export_all_tensors;
  this->_skip_preprocess = skip_preprocess;
}

CVI_RC Program::createNeuronSpace(const cvi::model::Program *fb_program) {
  // As for old version cvimodel that only has one big
  // neuron memory, the private gmem is same as shared gmem.
  if (fb_program->neuron_size()) {
    auto size = fb_program->neuron_size();
    private_mem = cviMemAlloc(_ctx, size, CVI_ALLOC_PROGRAM, _model_name.c_str());
    if (!private_mem) {
      TPU_LOG_ERROR("failed to alloc private gmem: %u\n", size);
      return CVI_RC_NOMEM;
    }
    baseMemArray[0] = private_mem;
    baseMemArray[2] = private_mem;
    baseAddrArray[0] = CVI_RT_MemGetPAddr(private_mem);
    baseAddrArray[2] = CVI_RT_MemGetPAddr(private_mem);
    return CVI_RC_SUCCESS;
  }

  auto size = fb_program->shared_gmem();
  if (size) {
    shared_mem = allocateSharedMemory(_ctx, _max_shared_mem_size);
    if (!shared_mem) {
      TPU_LOG_ERROR("failed to alloc shared gmem: %zu\n", _max_shared_mem_size);
      return CVI_RC_NOMEM;
    }
    baseMemArray[0] = shared_mem;
    baseAddrArray[0] = CVI_RT_MemGetPAddr(shared_mem);
  }

  size = fb_program->private_gmem();
  if (size) {
    private_mem = cviMemAlloc(_ctx, size, CVI_ALLOC_PROGRAM, _model_name.c_str());
    if (!private_mem) {
      TPU_LOG_ERROR("failed to alloc private gmem: %u\n", size);
      return CVI_RC_NOMEM;
    }
    baseMemArray[2] = private_mem;
    baseAddrArray[2] = CVI_RT_MemGetPAddr(private_mem);
  }
  return CVI_RC_SUCCESS;
}

CVI_RC Program::createNeuronMap(const cvi::model::Program *fb_program) {
  auto &tensor_vector = *fb_program->tensor_map();
  std::string in_name = fb_program->input_tensors()->begin()->str();
  for (auto t : tensor_vector) {
    auto tensor = std::make_shared<Neuron>(_ctx, _cvk, t,
                                           baseAddrArray, baseMemArray, _model_name.c_str());
    if (tensor->reserveIonMem(t->offset()) != CVI_RC_SUCCESS) {
      return CVI_RC_NOMEM;
    }
    neuron_map[t->name()->str()] = tensor;
  }

  auto &ins = *fb_program->input_tensors();
  for (auto i : ins) {
    in_tensors.push_back(neuron_map[i->str()]);
  }
  auto &outs = *fb_program->output_tensors();
  for (auto o : outs) {
    out_tensors.push_back(neuron_map[o->str()]);
  }
  return CVI_RC_SUCCESS;
}

CVI_RC Program::createRoutines(const cvi::model::Program *fb_program) {
  auto &routines = *fb_program->routines();
  for (auto r : routines) {
    std::shared_ptr<Routine> rt;
    if (r->type() == cvi::model::RoutineType_TPU) {
      rt = std::make_shared<TpuRoutine>(_ctx, this);
    } else {
#ifdef ENABLE_CPU_FUNC
      rt = std::make_shared<CpuRoutine>(_ctx, this);
#else
      TPU_LOG_ERROR("Cpu function is not supported! please recompile with ENABLE_CPU_FUNC\n");
      return CVI_RC_UNSUPPORT;
#endif
    }
    if (!rt->initialize(r)) {
      return CVI_RC_DATA_ERR;
    }
    _routines.push_back(rt);
  }
  return CVI_RC_SUCCESS;
}

CVI_RC Program::load(const cvi::model::Program *fb_program) {
  CVI_RC ret;

  ret = this->createNeuronSpace(fb_program);
  if (ret != CVI_RC_SUCCESS) {
    return ret;
  }

  ret = this->createNeuronMap(fb_program);
  if (ret != CVI_RC_SUCCESS) {
    return ret;
  }

  ret = this->createRoutines(fb_program);
  if (ret != CVI_RC_SUCCESS) {
    return ret;
  }

  for (auto &rt : _routines) {
    ret = rt->prepare();
    if (ret != CVI_RC_SUCCESS) {
      return ret;
    }
  }
  return CVI_RC_SUCCESS;
}

static void exportTensorInfo(void *program, const std::shared_ptr<Neuron> &neuron,
                             CVI_TENSOR *tensor) {
  tensor->name = const_cast<char *>(neuron->name.c_str());
  tensor->shape.dim[0] = neuron->shape[0];
  tensor->shape.dim[1] = neuron->shape[1];
  tensor->shape.dim[2] = neuron->shape[2];
  tensor->shape.dim[3] = neuron->shape[3];
  tensor->shape.dim_size = 4;
  tensor->fmt = (CVI_FMT)neuron->fmt;
  tensor->count = neuron->count();
  tensor->mem_type = CVI_MEM_SYSTEM;
  tensor->mem_size = neuron->size();
  tensor->sys_mem = neuron->sys_mem();
  tensor->paddr = neuron->paddr();
  tensor->qscale = neuron->qscale();
  tensor->zero_point = neuron->zero_point();
  tensor->pixel_format = neuron->pixel_format;
  tensor->aligned = neuron->aligned;
  for (int i = 0; i < (int)neuron->scale.size(); i++) {
    tensor->scale[i] = neuron->scale[i];
  }
  for (int i = 0; i < (int)neuron->mean.size(); i++) {
    tensor->mean[i] = neuron->mean[i];
  }
  tensor->owner = program;
}

CVI_TENSOR *Program::exportInputs(int32_t &size) {
  size = this->in_tensors.size();
  auto *tensors = new CVI_TENSOR[size];
  if (!tensors) {
    return nullptr;
  }
  for (int i = 0; i < size; i++) {
    exportTensorInfo(this, this->in_tensors[i], tensors + i);
  }
  return tensors;
}

CVI_TENSOR *Program::exportOutputs(int32_t &size) {
  int i = 0;
  CVI_TENSOR *tensors = nullptr;

  if (!_export_all_tensors) {
    size = (int)out_tensors.size();
    tensors = new CVI_TENSOR[size];
    if (!tensors) {
      goto Error;
    }
    for (; i < size; i++) {
      exportTensorInfo(this, out_tensors[i], tensors + i);
    }
  } else {
    for (auto &kv : neuron_map) {
      auto &tensor = kv.second;
      if (!tensor->overwrote())
        ++i;
    }
    size = i;
    tensors = new CVI_TENSOR[size];
    if (!tensors) {
      goto Error;
    }
    i = 0;
    for (auto &kv : neuron_map) {
      auto &tensor = kv.second;
      if (tensor->overwrote())
        continue;

      exportTensorInfo(this, tensor, tensors + i);
      ++i;
    }
    size = i;
  }
  return tensors;

Error:
  if (tensors) {
    delete[] tensors;
  }
  size = 0;
  return nullptr;
}

bool Program::forward(CVI_TENSOR *inputs, int input_num,
                      CVI_TENSOR *outputs, int output_num) {
#ifdef MEASURE_TIME
  struct timeval t0, t1;
  long elapsed;
  gettimeofday(&t0, NULL);
#endif

  TPU_ASSERT(input_num == (int)in_tensors.size(), nullptr);
  for (int i = 0; i < (int)in_tensors.size(); i++) {
    auto &tensor = this->in_tensors[i];
    tensor->load(inputs[i]);
  }

#ifdef MEASURE_TIME
  gettimeofday(&t1, NULL);
  elapsed = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;
  printf("  PERF: [load  ] %ld us\n", elapsed);
  t0 = t1;
#endif

  if (!this->run()) {
    return false;
  }

#ifdef MEASURE_TIME
  gettimeofday(&t1, NULL);
  elapsed = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;
  printf("  PERF: [run   ] %ld us\n", elapsed);
  t0 = t1;
#endif

  if (!_export_all_tensors) {
    TPU_ASSERT(output_num == (int)out_tensors.size(), nullptr);
    for (int i = 0; i < (int)out_tensors.size(); i++) {
      out_tensors[i]->store(outputs[i]);
    }
  } else {
    int i = 0;
    for (auto &kv : neuron_map) {
      auto &tensor = kv.second;
      if (tensor->overwrote())
        continue;

      tensor->store(outputs[i]);
      ++i;
    }
    TPU_ASSERT(output_num == i, nullptr);
  }

#ifdef MEASURE_TIME
  gettimeofday(&t1, NULL);
  elapsed = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;
  printf("  PERF: [store ] %ld us\n", elapsed);
#endif

  return true;
}

void *Program::forwardAsync(CVI_TENSOR *inputs, int input_num, CVI_TENSOR *outputs,
                            int output_num) {
  _pool->startPool();
  return new Task(_pool, (void *)this, inputs, input_num, outputs, output_num);
}

CVI_RC Program::forwardWait(void *task) {
  auto *myTask = (Task *)task;
  _pool->waitTask(myTask);
  return myTask->retCode;
}

bool Program::run() {
  // reset all routines for new inference.
  for (auto &r : _routines) {
    r->reset();
  }
  for (auto &r : _routines) {
    r->run();
  }
  return true;
}

void TpuRoutine::reset() {
  for (auto &neuron : outputs) {
    neuron->setState(Neuron::TPU_MEM);
  }
}

int TpuRoutine::init_dmabuf (Program *program, const std::string &name) {
  auto iter = program->dmabuf_map.find(name);
  if (program->dmabuf_map.end() == iter) {
    assert(0);
  }
  buf_mem = iter->second;
 
#ifdef ENABLE_PMU
  const char *pmu_enable_env = std::getenv("TPU_ENABLE_PMU");
  if (pmu_enable_env) {
    if (atoi(pmu_enable_env) > 0) {
      this->enable_pmu = true;
    }
  }
#else
  TPU_LOG_WARNING("Tpu pmu is not supported! please recompile with ENABLE_PMU\n");
#endif
  return CVI_RC_SUCCESS;
}

bool TpuRoutine::initialize(const cvi::model::Routine *routine) {
  // setup input & output tensors
  auto &in_tensors = *routine->in_tensors();
  for (auto i : in_tensors) {
    inputs.push_back(_program->neuron_map[i->str()]);
  }
  auto &out_tensors = *routine->out_tensors();
  for (auto o : out_tensors) {
    outputs.push_back(_program->neuron_map[o->str()]);
  }

  int ret = 0;
  // find cmdbuf section
  if (routine->tpu_routine()->cmdbuf_section()) {
    ret = init_dmabuf(_program, routine->tpu_routine()->cmdbuf_section()->str());
  } else if (routine->tpu_routine()->dmabuf_section()) {
    ret = init_dmabuf(_program, routine->tpu_routine()->dmabuf_section()->str());
  } else {
    TPU_LOG_ERROR("model not contain cmdbuf section and dmabuf section!\n");
    return false;
  }

  TPU_ASSERT(ret == 0, "CVI_RT_LoadCmdbuf failed");
  return true;
}

CVI_RC TpuRoutine::run() {
#ifdef MEASURE_TIME
  struct timeval t0, t1;
  long elapsed;
  gettimeofday(&t0, NULL);
#endif

  for (auto &neuron : inputs) {
    neuron->toTpu();
  }

#ifdef MEASURE_TIME
  gettimeofday(&t1, NULL);
  elapsed = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;
  printf("  PERF: [to_tpu ] %ld us\n", elapsed);
#endif

  CVI_RC ret = CVI_SUCCESS;
  CVI_RT_ARRAYBASE *baseArray =
      reinterpret_cast<CVI_RT_ARRAYBASE *>(&(_program->baseAddrArray[0]));
  if (this->encrypted) {
    ret = CVI_RT_RunCmdbufTee(_ctx, buf_mem, baseArray);
  } else {
    ret = CVI_RT_RunCmdbufEx(_ctx, buf_mem, baseArray);
  }

  if (ret != 0) {
    TPU_LOG_ERROR("run cmdbuf failed:%d\n", ret);
    return CVI_FAILURE;
  }

#ifdef MEASURE_TIME
  gettimeofday(&t1, NULL);
  elapsed = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;
  printf("  PERF: [tpu_run] %ld us\n", elapsed);
#endif

  // if we need to dump pmubuf
  if (this->enable_pmu) {
    uint8_t *pmubuf = nullptr;
    uint32_t buf_len = 0;
    CVI_RT_ParsePmuBuf(buf_mem, &pmubuf, &buf_len);
#ifdef DUMP_PMU_RAW
    if (pmubuf && buf_len) {
      const char *pmubuf_output_file_env = std::getenv("TPU_PMUBUF_OUTPUT_FILE");
      if (pmubuf_output_file_env) {
        std::fstream f_output(pmubuf_output_file_env,
                              std::ios::out | std::ios::trunc | std::ios::binary);
        f_output.write((char *)pmubuf, buf_len);
      }
    }
#endif
  }
  return CVI_SUCCESS;
}

void CpuRoutine::reset() {
  for (auto &neuron : outputs) {
    neuron->setState(Neuron::CPU_MEM);
  }
}

void CpuRoutine::handleFuncArgs(const uint8_t *args, OpParam &param) {
  auto packed_param = cvi::cpu_op::GetParameter(args);
  auto &attributes = *packed_param->attributes();
  for (auto attr : attributes) {
    if (attr->int_attr()) {
      auto _int = attr->int_attr();
      param.put<int32_t>(_int->key()->str(), _int->value());
    } else if (attr->float_attr()) {
      auto _float = attr->float_attr();
      param.put<float>(_float->key()->str(), _float->value());
    } else if (attr->bool_attr()) {
      auto _bool = attr->bool_attr();
      param.put<bool>(_bool->key()->str(), _bool->value());
    } else if (attr->str_attr()) {
      auto _str = attr->str_attr();
      param.put<std::string>(_str->key()->str(), _str->value()->str());
    } else if (attr->int_array_attr()) {
      auto _int_array = attr->int_array_attr();
      std::vector<int32_t> vec;
      auto &value = *_int_array->value();
      for (auto v : value) {
        vec.push_back(v);
      }
      param.put<std::vector<int32_t>>(_int_array->key()->str(), vec);
    } else if (attr->float_array_attr()) {
      auto _float_array = attr->float_array_attr();
      std::vector<float> vec;
      auto &value = *_float_array->value();
      for (auto v : value) {
        vec.push_back(v);
      }
      param.put<std::vector<float>>(_float_array->key()->str(), vec);
    } else {
      assert(0);
    }
  }
}

void CpuRoutine::fetchQscaleFromDequant(OpParam &param) {
  if (param.get<std::string>("from") == "NONE" &&
      param.get<std::string>("to") == "INT8") {
    float scale = param.has("threshold") ?
                  (128.0 / param.get<float>("threshold"))
                  : param.get<float>("scale");
    outputs[0]->setQScale(scale);
  }
}

bool CpuRoutine::initialize(const cvi::model::Routine *routine) {
  // setup input & output tensors
  auto &in_tensors = *routine->in_tensors();
  for (auto i : in_tensors) {
    auto name = i->str();
    auto it = _program->neuron_map.find(name);
    if (it != _program->neuron_map.end()) {
      auto &neuron = it->second;
      inputs.push_back(neuron);
    } else {
      //TPU_LOG_DEBUG("CpuRoutine need load weight, tensor name: %s\n", name.c_str());
      auto it = _program->weight_map.find(name);
      if (it == _program->weight_map.end()) {
        TPU_LOG_ERROR("Cannot find weight in map, %s\n", name.c_str());
        return false;
      }
      auto &neuron = it->second;
      neuron->toCpu();
      inputs.push_back(neuron);
    }
  }
  auto &out_tensors = *routine->out_tensors();
  for (auto o : out_tensors) {
    auto &neuron = _program->neuron_map[o->str()];
    outputs.push_back(neuron);
  }

  auto func_name = routine->cpu_routine()->function_section()->str();
  auto func_args = routine->cpu_routine()->function_args();
  for (auto f : _program->cpu_functions) {
    if (f->name == func_name) {
      _func_open = f->func_open;
      break;
    }
  }
  if (!_func_open) {
    TPU_LOG_ERROR("Cannot find runtime function of %s\n", func_name.c_str());
    return false;
  }
  _func = _func_open();
  OpParam param;
  if (func_args) {
    handleFuncArgs(func_args->data(), param);
    if (func_name == "quant") {
      fetchQscaleFromDequant(param);
    }
  }
  _func->setup(inputs, outputs, param);
  return true;
}

CVI_RC CpuRoutine::prepare() {
  CVI_RC ret;
  for (auto &input : inputs) {
    ret = input->reserveSysMem();
    if (ret != CVI_RC_SUCCESS) {
      return ret;
    }
  }
  for (auto &output : outputs) {
    ret = output->reserveSysMem();
    if (ret != CVI_RC_SUCCESS) {
      return ret;
    }
  }
  return CVI_RC_SUCCESS;
}

CVI_RC CpuRoutine::run() {
#ifdef MEASURE_TIME
  struct timeval t0, t1;
  long elapsed;
  gettimeofday(&t0, NULL);
#endif

  for (auto &neuron : inputs) {
    neuron->toCpu();
  }

#ifdef MEASURE_TIME
  gettimeofday(&t1, NULL);
  elapsed = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;
  printf("  PERF: [to_cpu ] %ld us\n", elapsed);
  t0 = t1;
#endif

  _func->run();

#ifdef MEASURE_TIME
  gettimeofday(&t1, NULL);
  elapsed = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;
  printf("  PERF: [cpu_run] %ld us, %s\n", elapsed, outputs[0]->name.c_str());
#endif

  return CVI_SUCCESS;
}

} // namespace runtime
} // namespace cvi
