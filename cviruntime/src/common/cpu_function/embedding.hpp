#include <iostream>
#include <runtime/cpu_function.hpp>
#include <runtime/neuron.hpp>
#include <unordered_map>
#include <vector>

namespace cvi {
namespace runtime {

class EmbeddingFunc : public ICpuFunction {

public:
  EmbeddingFunc() {}

  ~EmbeddingFunc();
  void setup(std::vector<std::shared_ptr<Neuron>> &Inputs,
             std::vector<std::shared_ptr<Neuron>> &outputs, OpParam &param);
  void run();

  static ICpuFunction *open() { return new EmbeddingFunc(); }
  static void close(ICpuFunction *func) { delete func; }

private:
  std::vector<std::shared_ptr<Neuron>> _bottoms;
  std::shared_ptr<Neuron> _top;

  template <typename T1, typename T2>
  void lookup();

  template <typename T1>
  void lookup();

  int _search_num;
  int _feature_len;
  int _table_len;
};

} // namespace runtime
} // namespace cvi
