#include <iostream>
#include <fstream>
#include <numeric>
#include "cviruntime.h"
#include "mapi.hpp"

#include "opencv2/opencv.hpp"


#ifndef SAMPLE_CHECK_RET
#define SAMPLE_CHECK_RET(express)                                                    \
    do {                                                                      \
        int rc = express;                                                     \
        if (rc != 0) {                                                        \
            printf("\nFailed at %s: %d  (rc:0x%#x!)\n",                       \
                    __FILE__, __LINE__, rc);                                  \
            return rc;                                                        \
        }                                                                     \
    } while (0)
#endif

static void usage(char **argv) {
  printf("Usage:\n");
  printf("   %s cvimodel image.jpg label_file\n", argv[0]);
}

static void get_frame_from_mat(VIDEO_FRAME_INFO_S &in_frame, const cv::Mat &mat) {
  CVI_MAPI_AllocateFrame(&in_frame, mat.cols, mat.rows, PIXEL_FORMAT_BGR_888);
  CVI_MAPI_FrameMmap(&in_frame, true);
  uint8_t *src_ptr = mat.data;
  uint8_t *dst_ptr = in_frame.stVFrame.pu8VirAddr[0];
  for (int h = 0; h < mat.rows; ++h) {
    memcpy(dst_ptr, src_ptr, mat.cols * mat.elemSize());
    src_ptr += mat.step[0];
    dst_ptr += in_frame.stVFrame.u32Stride[0];
  }
  CVI_MAPI_FrameFlushCache(&in_frame);
  CVI_MAPI_FrameMunmap(&in_frame);
}

int main(int argc, char **argv) {
  if (argc != 5) {
    usage(argv);
    exit(-1);
  }

  YuvType yuv_type = YUV_UNKNOWN;
  if (strcmp(argv[4], "YUV420_PLANAR") == 0) {
    yuv_type = YUV420_PLANAR;
    printf("YUV420_PLANAR mode\n");
  } else if (strcmp(argv[4], "YUV_NV12") == 0) {
    yuv_type = YUV_NV12;
    printf("YUV_NV12 mode\n");
  } else if (strcmp(argv[4], "YUV_NV21") == 0) {
    yuv_type = YUV_NV21;
    printf("YUV_NV21 mode\n");
  } else {
    assert(0 && "unsupported yuv type");
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
  printf("input, name:%s\n", input->name);
  CVI_TENSOR *output = CVI_NN_GetTensorByName(CVI_NN_DEFAULT_TENSOR, output_tensors, output_num);
  assert(output);

  float qscale = CVI_NN_TensorQuantScale(input);
  printf("qscale:%f\n", qscale);
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

  // init vb
  SAMPLE_CHECK_RET(CVI_MAPI_Media_Init(image.cols, image.rows, 2));

  //init vpss
  PreprocessArg arg;
  arg.width = width;
  arg.height = height;
  arg.yuv_type = yuv_type;
  init_vpss(image.cols, image.rows, &arg);

  VIDEO_FRAME_INFO_S frame_in;
  VIDEO_FRAME_INFO_S frame_preprocessed;
  memset(&frame_in, 0x00, sizeof(frame_in));
  get_frame_from_mat(frame_in, image);

  if (CVI_SUCCESS != CVI_VPSS_SendFrame(0, &frame_in, -1)) {
    printf("send frame failed\n");
    return -1;
  }
  if (CVI_SUCCESS != CVI_VPSS_GetChnFrame(0, 0, &frame_preprocessed, 1000)) {
    printf("get frame failed\n");
    return -1;
  }
  for (int i = 0; i < 3; ++i) {
    printf("YUV Frame height[%d] width[%d] stride[%d] addr[%llx]\n",
    frame_preprocessed.stVFrame.u32Height,
    frame_preprocessed.stVFrame.u32Width,
    frame_preprocessed.stVFrame.u32Stride[i],
    frame_preprocessed.stVFrame.u64PhyAddr[i]);
  }

  CVI_NN_SetTensorPhysicalAddr(input, (uint64_t)frame_preprocessed.stVFrame.u64PhyAddr[0]);

  CVI_MAPI_ReleaseFrame(&frame_in);

  // run inference
  CVI_NN_Forward(model, input_tensors, input_num, output_tensors, output_num);
  printf("CVI_NN_Forward succeeded\n");

  if (CVI_SUCCESS != CVI_VPSS_ReleaseChnFrame(0, 0, &frame_preprocessed)) {
    printf("release frame failed!\n");
    return -1;
  }

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
  std::vector<size_t> idx(count);
  std::iota(idx.begin(), idx.end(), 0);
  std::sort(idx.begin(), idx.end(), [&prob](size_t idx_0, size_t idx_1) {return prob[idx_0] > prob[idx_1];});
  // show results.
  printf("------\n");
  for (size_t i = 0; i < top_num; i++) {
    int top_k_idx = idx[i];
    printf("  %f, idx %d", prob[top_k_idx], top_k_idx);
    if (!labels.empty())
      printf(", %s", labels[top_k_idx].c_str());
    printf("\n");
  }
  printf("------\n");

  CVI_NN_CleanupModel(model);
  printf("CVI_NN_CleanupModel succeeded\n");
  SAMPLE_CHECK_RET(CVI_MAPI_Media_Deinit());
  vproc_deinit();
  return 0;
}
