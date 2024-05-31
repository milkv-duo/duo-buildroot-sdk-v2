#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <runtime/neuron.hpp>
#include <runtime/cpu_function.hpp>


namespace cvi {
namespace runtime {

enum Decode_CodeType {
  PriorBoxParameter_CodeType_CORNER = 1,
  PriorBoxParameter_CodeType_CENTER_SIZE = 2,
  PriorBoxParameter_CodeType_CORNER_SIZE = 3
};

class BBox_l {
  public:
  float num;
  float label;
  float score;
  union {
    struct {
      float xmin;
      float ymin;
      float xmax;
      float ymax;
    } s;
    float b[4]; //for neon used
  } xy;
  float size;

  void CalcSize() {
    if (xy.s.xmax < xy.s.xmin || xy.s.ymax < xy.s.ymin) {
      size = 0;
    } else {
      float width = xy.s.xmax - xy.s.xmin;
      float height = xy.s.ymax - xy.s.ymin;
      size = width * height;
    }
  }
};

typedef Decode_CodeType CodeType;
typedef std::map<int, std::vector<BBox_l> > LabelBBox_l;

class SSDDetectionFunc : public ICpuFunction {

public:
  SSDDetectionFunc() {}

  ~SSDDetectionFunc();
  void setup(tensor_list_t &inputs,
             tensor_list_t &outputs,
             OpParam &param);
  void run();
  void neon_run(float* top_data, bool variance_encoded_in_target_,
      int num_loc_classes, float eta_, Decode_CodeType code_type_);

  static ICpuFunction *open() { return new SSDDetectionFunc(); }
  static void close(ICpuFunction *func) { delete func; }

private:
  tensor_list_t _bottoms;
  tensor_list_t _tops;

  void ApplyNMSFast_opt(const std::vector<BBox_l> &bboxes,
                        const std::vector<std::pair<float, int>> &conf_score,
                        const float nms_threshold, const float eta, int top_k,
                        std::vector<std::pair<float, int>> *indices);

  int _num_classes;
  bool _share_location{true};
  int _background_label_id;
  int _top_k;
  std::string _code_type;
  float _nms_threshold;
  float _obj_threshold;
  int _keep_topk;
};

}
}
