#pragma once
#include "core/face/cvtdl_face_types.h"

#include <string>
#include <vector>

namespace cvitdl {
namespace evaluation {
typedef struct {
  int cam_id;
  int pid;
  std::string img_path;
  cvtdl_feature_t feature;
} market_info;

class market1501Eval {
 public:
  market1501Eval(const char *fiilepath);
  ~market1501Eval();
  int getEvalData(const char *fiilepath);
  uint32_t getImageNum(bool is_query);
  void getPathIdPair(const int index, bool is_query, std::string *path, int *cam_id, int *pid);
  void insertFeature(const int index, bool is_query, const cvtdl_feature_t *feature);
  void evalCMC();
  void resetData();

 private:
  std::vector<market_info> m_querys;
  std::vector<market_info> m_gallerys;
};
}  // namespace evaluation
}  // namespace cvitdl