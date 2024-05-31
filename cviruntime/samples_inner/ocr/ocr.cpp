#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <opencv2/opencv.hpp>
#include "cviruntime.h"
#include "cnpy.h"

class OcrTool {
public:
  OcrTool(const char *model_file);
  ~OcrTool();

  float* run(uint8_t *image);

public:
  CVI_TENSOR *input;
  CVI_TENSOR *output;

private:
  CVI_MODEL_HANDLE model = nullptr;
  CVI_TENSOR *input_tensors;
  CVI_TENSOR *output_tensors;
  int32_t input_num;
  int32_t output_num;
};

OcrTool::OcrTool(const char *model_file) {
  int ret = CVI_NN_RegisterModel(model_file, &model);
  if (ret != CVI_RC_SUCCESS) {
    printf("CVI_NN_RegisterModel failed, err %d\n", ret);
    exit(1);
  }
  ret = CVI_NN_GetInputOutputTensors(model, &input_tensors, &input_num,
                                     &output_tensors, &output_num);
  if (ret != CVI_RC_SUCCESS) {
    printf("CVI_NN_GetInputOutputTensors failed, err %d\n", ret);
    exit(1);
  }
  assert(input_num == 1);
  assert(output_num == 1);
  input = &input_tensors[0];
  output = &output_tensors[0];
}

OcrTool::~OcrTool() {
  if (model) {
    CVI_NN_CleanupModel(model);
  }
}

static int levenshtein_distance(const float* s, int n, const int* t, int m) {
   ++n; ++m;
   int* d = new int[n * m];
   memset(d, 0, sizeof(int) * n * m);
   for (int i = 1, im = 0; i < m; ++i, ++im) {
      for (int j = 1, jn = 0; j < n; ++j, ++jn) {
         if ((int)(s[jn]) == t[im]) {
            d[(i * n) + j] = d[((i - 1) * n) + (j - 1)];
         } else {
            d[(i * n) + j] = std::min(d[(i - 1) * n + j] + 1, /* A deletion. */
                                 std::min(d[i * n + (j - 1)] + 1, /* An insertion. */
                                     d[(i - 1) * n + (j - 1)] + 1)); /* A substitution. */
         }
      }
   }
   int r = d[n * m - 1];
   delete [] d;
   return r;
}


float* OcrTool::run(uint8_t *image) {
  // fill data to input tensor
  CVI_NN_SetTensorPtr(input, image);
  // run inference
  CVI_NN_Forward(model, input_tensors, input_num, output_tensors, output_num);
  return (float *)CVI_NN_TensorPtr(output);
}

int main(int argc, char **argv) {
  int ret = 0;
  if (argc < 4) {
    printf("Usage:\n");
    printf("   %s images_npz references_npz cvimodel\n", argv[0]);
    exit(1);
  }
  const char *images_npz = argv[1];
  const char *references_npz = argv[2];
  const char *cvimodel = argv[3];

  cnpy::npz_t images = cnpy::npz_load(images_npz);
  if (images.size() == 0) {
    printf("Failed to load images npz\n");
  }
  cnpy::npz_t references = cnpy::npz_load(references_npz);
  if (references.size() == 0) {
    printf("Failed to load references npz\n");
  }
  assert(images.size() == references.size());

  OcrTool tool(cvimodel);

  int all_cnt = 0;
  int correct_cnt = 0;
  for (auto &npy : images) {
    auto &name = npy.first;
    auto &image = npy.second;
    auto *ptr = image.data<uint8_t>();

    float* out = tool.run(ptr);

    auto &refer_vec = references[name];
    const int* refer = refer_vec.data<int>();
    int num = (int)refer_vec.num_vals;
    int dist = levenshtein_distance(out, num, refer, num);
    correct_cnt += std::max(num - dist, 0);
    all_cnt += num;
  }

  printf("acc: %f\n", 100.0 * correct_cnt / all_cnt);
  return 0;
}