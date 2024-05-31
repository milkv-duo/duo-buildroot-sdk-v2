#ifndef RUNTIME_PROGRAM_H
#define RUNTIME_PROGRAM_H

#include <map>
#include <list>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <runtime/neuron.hpp>
#include <runtime/stream.hpp>
#include <runtime/section.hpp>
#include <runtime/cpu_function.hpp>
#include <runtime/op_param.hpp>
#include <cviruntime_context.h>
#include "cviruntime.h"
#include <runtime/taskpool.hpp>
#include <cvibuilder/cvimodel_generated.h>
#include <cvibuilder/parameter_generated.h>

namespace cvi {
namespace runtime {

typedef std::unordered_map<std::string, CVI_RT_MEM> dmabuf_map_t;

class Routine;
class Program {
public:
  Program(CVI_RT_HANDLE ctx, TaskPool *pool,
          dmabuf_map_t &dmabuf_map,
          std::vector<CpuRuntimeFunction *> &functions,
          tensor_map_t &weight_map,
          CVI_RT_MEM weight_mem,
          const char *model_name,
          size_t max_shared_mem_size);
  ~Program();

  void setOptions(bool export_all_tensors,
                  bool skip_preprocess);
  CVI_RC load(const cvi::model::Program *fb_program);

  bool forward(CVI_TENSOR *inputs, int input_num,
               CVI_TENSOR *outputs, int output_num);

  void *forwardAsync(CVI_TENSOR *inputs, int input_num,
                     CVI_TENSOR *outputs, int output_num);

  CVI_RC forwardWait(void *task);

  const tensor_list_t &input_tensors() { return in_tensors; }
  const tensor_list_t &output_tensors() { return out_tensors; }

  CVI_TENSOR *exportInputs(int32_t &size);
  CVI_TENSOR *exportOutputs(int32_t &size);

  tensor_list_t in_tensors;
  tensor_list_t out_tensors;
  tensor_map_t neuron_map;
  tensor_map_t &weight_map;
  dmabuf_map_t &dmabuf_map;
  std::vector<CpuRuntimeFunction *> &cpu_functions;
  /* 0: shared_mem,
   * 1: weight_mem,
   * 2: private_mem,
   * 3~7: io_mem
   */
  uint64_t baseAddrArray[8];
  CVI_RT_MEM baseMemArray[8];

private:
  CVI_RC createNeuronSpace(const cvi::model::Program *fb_program);
  CVI_RC createNeuronMap(const cvi::model::Program *fb_program);
  CVI_RC createRoutines(const cvi::model::Program *fb_program);
  bool run();

  CVI_RT_HANDLE _ctx;
  CVI_RT_KHANDLE _cvk;
  bool _export_all_tensors;
  bool _skip_preprocess;
  TaskPool *_pool = nullptr;
  CVI_RT_MEM private_mem = nullptr;
  CVI_RT_MEM shared_mem = nullptr;
  std::list<std::shared_ptr<Routine>> _routines;
  std::string _model_name;
  size_t _max_shared_mem_size;
};

class Routine {
public:
  Routine(CVI_RT_HANDLE ctx, Program *program, bool tpu)
    : tpu(tpu), _ctx(ctx), _program(program) {}
  virtual ~Routine() {}
  virtual bool initialize(const cvi::model::Routine *routine) = 0;
  virtual CVI_RC run() = 0;
  virtual void reset() = 0;
  virtual CVI_RC prepare() { return CVI_RC_SUCCESS; }
  tensor_list_t inputs;
  tensor_list_t outputs;
  bool tpu;

protected:
  CVI_RT_HANDLE _ctx;
  Program *_program;
};

class TpuRoutine : public Routine {
public:
  TpuRoutine(CVI_RT_HANDLE ctx, Program *program)
    : Routine(ctx, program, true) {}
  ~TpuRoutine() {
  }

  bool initialize(const cvi::model::Routine *routine);
  int init_dmabuf (Program *program, const std::string &name);
  CVI_RC run();
  void reset();

private:
  CVI_RT_MEM buf_mem = nullptr;
  bool enable_pmu = false;
  bool encrypted = false;
};

class CpuRoutine : public Routine {
public:
  CpuRoutine(CVI_RT_HANDLE ctx, Program *program)
    : Routine(ctx, program, false) {}
  ~CpuRoutine() { delete _func; }

  bool initialize(const cvi::model::Routine *routine);
  CVI_RC run();
  CVI_RC prepare();
  void reset();

private:
  void fetchQscaleFromDequant(OpParam &param);
  void handleFuncArgs(const uint8_t *args, OpParam &param);
  ICpuFunctionCreate _func_open = nullptr;
  ICpuFunction *_func = nullptr;
};

} // namespace runtime
} // namespace cvi

#endif
