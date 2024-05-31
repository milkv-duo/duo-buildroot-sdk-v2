#include <stdio.h>
#include <fstream>
#include <string>
#include <cviruntime.h>
#include <opencv2/opencv.hpp>

#define IMG_RESIZE_DIMS 256,256
#define BGR_MEAN        103.94,116.78,123.68
#define INPUT_SCALE     0.017

static void usage(char **argv) {
  printf("Usage:\n");
  printf("   %s cvimodel image.jpg label_file\n", argv[0]);
}

int main(int argc, char **argv) {
  if (argc != 4) {
    usage(argv);
    exit(-1);
  }

  // load model file
  const char *model_file = argv[1];
  CVI_MODEL_HANDLE model = nullptr;
  int ret = CVI_NN_RegisterModel(model_file, &model);
  if (CVI_RC_SUCCESS != ret) {
    printf("CVI_NN_RegisterModel failed, err %d\n", ret);
    exit(1);
  }
  printf("CVI_NN_RegisterModel succeeded\n");

  // get input output tensors
  CVI_TENSOR *input_tensors;
  CVI_TENSOR *output_tensors;
  int32_t input_num;
  int32_t output_num;
  CVI_NN_GetInputOutputTensors(model, &input_tensors, &input_num, &output_tensors,
                               &output_num);
  CVI_TENSOR *input = CVI_NN_GetTensorByName(CVI_NN_DEFAULT_TENSOR, input_tensors, input_num);
  assert(input);
  CVI_TENSOR *output = CVI_NN_GetTensorByName(CVI_NN_DEFAULT_TENSOR, output_tensors, output_num);
  assert(output);
  CVI_SHAPE shape = CVI_NN_TensorShape(input);

  // nchw
  int32_t height = shape.dim[2];
  int32_t width = shape.dim[3];

  // imread
  cv::Mat image;
  image = cv::imread(argv[2]);
  if (!image.data) {
    printf("Could not open or find the image\n");
    return -1;
  }

  // resize
  cv::resize(image, image, cv::Size(IMG_RESIZE_DIMS)); // linear is default
  // crop
  cv::Size size = cv::Size(height, width);
  cv::Rect crop(cv::Point(0.5 * (image.cols - size.width),
                          0.5 * (image.rows - size.height)), size);
  image = image(crop);

  // split
  cv::Mat channels[3];
  for (int i = 0; i < 3; i++) {
    channels[i] = cv::Mat(height, width, CV_8UC1);
  }
  cv::split(image, channels);
  // normalize
  float mean[] = {BGR_MEAN};
  for (int i = 0; i < 3; ++i) {
    channels[i].convertTo(channels[i], CV_32FC1, INPUT_SCALE,
                          -1 * mean[i] * INPUT_SCALE);
  }

  // fill to input tensor
  float *ptr = (float *)CVI_NN_TensorPtr(input);
  int channel_size = height * width;
  for (int i = 0; i < 3; ++i) {
    memcpy(ptr + i * channel_size, channels[i].data, channel_size*sizeof(float));
  }

  // run inference
  CVI_NN_Forward(model, input_tensors, input_num, output_tensors, output_num);
  printf("CVI_NN_Forward succeeded\n");

  // output result
  std::vector<std::string> labels;
  std::ifstream file(argv[3]);
  if (!file) {
    printf("Didn't find synset_words file\n");
    exit(1);
  } else {
    std::string line;
    while (std::getline(file, line)) {
      labels.push_back(std::string(line));
    }
  }

  int32_t top_num = 5;
  float *prob = (float *)CVI_NN_TensorPtr(output);
  int32_t count = CVI_NN_TensorCount(output);

  int32_t top_k_idx[top_num] = {-1};
  float top_k[top_num] = {0};

  // find top-k prob and cls
  for (int32_t i = 0; i < count; ++i) {
    for (int32_t k = 0; k < top_num; ++k) {
      if (prob[i] > top_k[k]) {
        top_k[k] = prob[i];
        top_k_idx[k] = i;
        break;
      }
    }
  }
  // sort
  for (int32_t i = 0; i < top_num - 1; ++i) {
    for (int32_t j = 0; j < top_num - 1 - i; ++j) {
      if (top_k[j] < top_k[j + 1]) {
        std::swap(top_k[j], top_k[j + 1]);
        std::swap(top_k_idx[j], top_k_idx[j + 1]);
      }
    }
  }
  // show results.
  printf("------\n");
  for (size_t i = 0; i < top_num; i++) {
    printf("  %f, idx %d", top_k[i], top_k_idx[i]);
    if (!labels.empty())
      printf(", %s", labels[top_k_idx[i]].c_str());
    printf("\n");
  }
  printf("------\n");
  CVI_NN_CleanupModel(model);
  printf("CVI_NN_CleanupModel succeeded\n");
  return 0;
}