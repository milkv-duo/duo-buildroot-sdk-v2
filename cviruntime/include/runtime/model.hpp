#ifndef RUNTIME_MODEL_H
#define RUNTIME_MODEL_H

#include <vector>
#include <map>
#include <memory>
#include <string>
#include <cvibuilder/cvimodel_generated.h>
#include <runtime/stream.hpp>
#include <runtime/program.hpp>
#include <runtime/neuron.hpp>
#include <runtime/cpu_function.hpp>
#include <runtime/taskpool.hpp>

namespace cvi {
namespace runtime {

typedef struct {
  char magic[8];
  uint32_t body_size;
  char major;
  char minor;
  char md5[16];
  char chip[16];
  char padding[2];
} MODEL_HEADER;

class CviModel {
public:
  CviModel(CVI_RT_HANDLE ctx, int count);

  CVI_RC acquire(const int8_t *buf, size_t size);
  CVI_RC acquire(const std::string &modelFile);
  CVI_RC acquire(const int fd, const size_t ud_offset);
  void refer() { ref++; }
  void release();

  CVI_RC loadProgram(Program **program,
      int program_id, bool export_all_tensors,
      bool skip_preprocess);

  static std::string getChipType(const std::string &modelFile,
      const int8_t *buf = nullptr, size_t size = 0);

  int32_t program_num;
  int32_t major_ver = 1;
  int32_t minor_ver = 2;

  // global info
  static std::string targetChipType;

private:
  ~CviModel();

  CVI_RC parse(BaseStream *stream);
  CVI_RC loadWeight(BaseStream *stream, size_t offset, size_t size);
  CVI_RC loadDmabuf(BaseStream *stream, size_t offset, size_t size, const cvi::model::Section *section);
  CVI_RC loadCmdbuf(BaseStream *stream, size_t offset, size_t size, const cvi::model::Section *section);
  CVI_RC extractSections(BaseStream *stream, size_t bin_offset);
  CVI_RC parseModelHeader(BaseStream *stream, size_t &payload_sz,
                          size_t &header_sz);
  bool checkIfMatchTargetChipType(std::string &target);
  CVI_RC showAndCheckVersion();
  void parseProgramNum();
  void createCpuWeightMap();

  CVI_RT_HANDLE _ctx;
  std::atomic<int32_t> ref;
  TaskPool *_pool = nullptr;
  cvi::model::Model *_fb_model;
  uint8_t *_model_body = nullptr;
  CVI_RT_MEM _weight_mem = nullptr;
  CustomFunctionSection _custom_section;
  std::vector<CpuRuntimeFunction *> _cpu_functions;
  tensor_map_t weight_map;
  dmabuf_map_t dmabuf_map;
  bool encrypt_model;
  bool isprotect = false; //protect cmdbuf_mem and weight_mem 
  int _count;
  std::string _model_name;
  size_t _max_shared_mem_size;
};

} // namespace runtime
} // namespace cvi

#endif
