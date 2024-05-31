#ifndef RUNTIME_SECTION_H
#define RUNTIME_SECTION_H

#include <vector>
#include <map>
#include <memory>
#include <string>
#include <runtime/stream.hpp>
#include <runtime/cpu_function.hpp>
#include "alloc.h"

namespace cvi {
namespace runtime {

class WeightSection {
public:
  WeightSection(size_t offset, size_t size) : offset(offset), size(size) {}

  size_t offset;
  size_t size;
};

class CpuRuntimeFunction {
public:
  CpuRuntimeFunction(const std::string &name, ICpuFunctionCreate func_open)
      : name(name), func_open(func_open) {}
  ~CpuRuntimeFunction() = default;

  const std::string name;
  ICpuFunctionCreate func_open;
};

class CustomFunctionSection {
public:
  CustomFunctionSection() = default;
  ~CustomFunctionSection();
  bool load(BaseStream *stream, size_t offset, size_t size,
            std::vector<CpuRuntimeFunction *> &cpu_functions);

private:
  int shm_fd = 0;
  void *dso_handle = nullptr;
};

} // namespace runtime
} // namespace cvi

#endif
