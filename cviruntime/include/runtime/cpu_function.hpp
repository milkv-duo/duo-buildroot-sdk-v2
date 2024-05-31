/*
* Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
*/

#ifndef RUNTIME_CPU_FUNCTION_H
#define RUNTIME_CPU_FUNCTION_H
#include <vector>
#include <stdint.h>
#include <memory>
#include <runtime/neuron.hpp>
#include <runtime/op_param.hpp>

namespace cvi {
namespace runtime {

class ICpuFunction {
public:
  ICpuFunction() {}
  virtual ~ICpuFunction() {}
  virtual void setup(tensor_list_t &inputs,
                     tensor_list_t &outputs,
                     OpParam &param) = 0;
  virtual void run() = 0;

protected:
  template <typename T>
  void print_data(T data) {
    if (sizeof(T) == 4) {
      printf("%e ", (float)data);
    } else if (sizeof(T) == 1) {
      printf("%4d ", (int)data);
    } else {
      assert(0);
      std::cout << data << " ";
    }
  }

  template <typename T>
  void dump(const std::string &tag, const T *data, size_t count) {
    auto ptr = (T *)data;
    int loop = count / 10;
    std::cout << "-------Dump " << tag << ", size:" << count << "\n";

    for (int i = 0; i < loop; i++) {
      for (int j = 0; j < 10; j++) {
        print_data(*(ptr++));
      }
      std::cout << "\n";
    }
    for (int j = 0; j < (int)(count % 10); j++) {
      print_data(*(ptr++));
    }
    std::cout << "\n";
  }
};

typedef ICpuFunction *(*ICpuFunctionCreate)();

} // namespace runtime
} // namespace cvi

typedef struct {
  char *name;
  cvi::runtime::ICpuFunctionCreate func;
} CustomOpRuntimeFunc;

#define REGISTER_OP_RUNTIME_FUNCS(X, ...)                                                \
  extern "C" {                                                                           \
  CustomOpRuntimeFunc customOpRuntimeFuncs[] = {X, ##__VA_ARGS__};                       \
  int customOpRuntimeFuncsNum =                                                          \
      sizeof(customOpRuntimeFuncs) / sizeof(CustomOpRuntimeFunc);                        \
  }

#endif
