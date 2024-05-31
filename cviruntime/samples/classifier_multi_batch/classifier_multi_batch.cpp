#include <stdio.h>
#include <fstream>
#include <string>
#include <numeric>
#include <cviruntime.h>
#include <opencv2/opencv.hpp>

#define IMG_RESIZE_DIMS 256,256
#define BGR_MEAN        103.94,116.78,123.68
#define INPUT_SCALE     0.017

static void usage(char **argv) {
  printf("Usage:\n");
  printf("   %s cvimodel image.jpg label_file\n", argv[0]);
}

static int get_input_output_tensors(CVI_MODEL_HANDLE &model, CVI_TENSOR **input,
                                    CVI_TENSOR **output, CVI_TENSOR **input_tensors,
                                    CVI_TENSOR **output_tensors, int32_t &input_num,
                                    int32_t &output_num, CVI_SHAPE &shape, int32_t &batch,
                                    float &qscale) {
  int program_id = 0;
  if (batch == 1) {
    program_id = 0;
  } else if (batch == 4) {
    program_id = 1;
  } else {
    printf("error batch size\n");
    exit(1);
  }
  CVI_NN_SetConfig(model, OPTION_PROGRAM_INDEX, program_id);

  // get input output tensors
  CVI_NN_GetInputOutputTensors(model, input_tensors, &input_num, output_tensors,
                               &output_num);
  *input = CVI_NN_GetTensorByName(CVI_NN_DEFAULT_TENSOR, *input_tensors, input_num);
  assert(*input);
  printf("input, name:%s\n", (*input)->name);
  *output = CVI_NN_GetTensorByName(CVI_NN_DEFAULT_TENSOR, *output_tensors, output_num);
  assert(*output);

  qscale = CVI_NN_TensorQuantScale(*input);
  printf("qscale:%f\n", qscale);
  shape = CVI_NN_TensorShape(*input);

  if (shape.dim[0] != batch) {
    printf("ERROR : Program id %d is batch %d not batch %d\n", program_id, shape.dim[0], batch);
    exit(1);
  }

  return 0;
}

static void post_process(int top_num, int batch, CVI_TENSOR *output, std::vector<std::string> &labels) {
  float *batch_prob = (float *)CVI_NN_TensorPtr(output);
  int32_t count = CVI_NN_TensorCount(output) / batch;

  for (int b = 0; b < batch; ++b){
    // find top-k prob and cls
    float *prob = batch_prob + b * count;
    std::vector<size_t> idx(count);
    std::iota(idx.begin(), idx.end(), 0);
    std::sort(idx.begin(), idx.end(), [&prob](size_t idx_0, size_t idx_1) {return prob[idx_0] > prob[idx_1];});
    // show results.
    printf("--batch %d----\n", b);
    for (size_t i = 0; i < top_num; i++) {
      int top_k_idx = idx[i];
      printf("  %f, idx %d", prob[top_k_idx], top_k_idx);
      if (!labels.empty())
        printf(", %s", labels[top_k_idx].c_str());
      printf("\n");
    }
    printf("------\n");
  }
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

  // batch 1
  // get input output tensors
  CVI_TENSOR *input;
  CVI_TENSOR *output;
  CVI_TENSOR *input_tensors;
  CVI_TENSOR *output_tensors;
  int32_t input_num;
  int32_t output_num;
  CVI_SHAPE shape;
  float qscale;

  int32_t batch = 1;
  get_input_output_tensors(model, &input, &output, &input_tensors, &output_tensors,
                           input_num, output_num, shape, batch, qscale);

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
    channels[i] = cv::Mat(height, width, CV_8SC1);
  }
  cv::split(image, channels);
  // normalize
  float mean[] = {BGR_MEAN};
  for (int i = 0; i < 3; ++i) {
    channels[i].convertTo(channels[i], CV_8SC1, INPUT_SCALE * qscale,
                          -1 * mean[i] * INPUT_SCALE * qscale);
  }

  // fill to input tensor
  int8_t *ptr = (int8_t *)CVI_NN_TensorPtr(input);
  int channel_size = height * width;
  for (int i = 0; i < 3; ++i) {
    memcpy(ptr + i * channel_size, channels[i].data, channel_size);
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
  post_process(top_num, batch, output, labels);

  // batch 4
  CVI_MODEL_HANDLE batch4_model = nullptr;
  ret = CVI_NN_CloneModel(model, &batch4_model);
  if (CVI_RC_SUCCESS != ret) {
    printf("CVI_NN_CloneModel failed, err %d\n", ret);
    exit(1);
  }
  printf("CVI_NN_CloneModel succeeded\n");

  // get input output tensors
  batch = 4;
  get_input_output_tensors(batch4_model, &input, &output, &input_tensors, &output_tensors,
                           input_num, output_num, shape, batch, qscale);

  // nchw
  height = shape.dim[2];
  width = shape.dim[3];

  // fill to input tensor
  ptr = (int8_t *)CVI_NN_TensorPtr(input);
  channel_size = height * width;
  int batch_size = height * width * 3;
  for (int b = 0; b < batch; ++b) {
    for (int i = 0; i < 3; ++i) {
      memcpy(ptr + i * channel_size + b * batch_size, channels[i].data, channel_size);
    }
  }

  // run inference
  CVI_NN_Forward(batch4_model, input_tensors, input_num, output_tensors, output_num);
  printf("CVI_NN_Forward succeeded\n");

  // output result
  top_num = 5;
  post_process(top_num, batch, output, labels);

  CVI_NN_CleanupModel(model);
  CVI_NN_CleanupModel(batch4_model);
  printf("CVI_NN_CleanupModel succeeded\n");
  return 0;
}