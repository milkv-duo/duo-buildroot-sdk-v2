
#include "license_plate_recognitionv2.hpp"
#include <iostream>
#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem.h"
#include "core/face/cvtdl_face_types.h"
#include "rescale_utils.hpp"

#define SCALE (1 / 128.)
#define MEAN (127.5 / 128.)

#define OUTPUT_NAME_PROBABILITY "output_ReduceMean_f32"

namespace cvitdl {

std::vector<std::string> CHARS = {
    "jing", "hu",    "jin",    "yu",   "ji",  "jin",  "meng", "liao", "ji",    "hei",
    "su",   "zhe",   "wan",    "min",  "gan", "lu",   "yu",   "e",    "xiang", "yue",
    "gui",  "qiong", "chuang", "gui",  "yun", "zang", "shan", "gan",  "qing",  "ning",
    "xin",  "xue",   "jing",   "gang", "ao",  "gua",  "shi",  "ling", "min",   "shen",
    "wei",  "xian",  "kong",   "0",    "1",   "2",    "3",    "4",    "5",     "6",
    "7",    "8",     "9",      "A",    "B",   "C",    "D",    "E",    "F",     "G",
    "H",    "J",     "K",      "L",    "M",   "N",    "P",    "Q",    "R",     "S",
    "T",    "U",     "V",      "W",    "X",   "Y",    "Z",    "I",    "O",     "-"};

LicensePlateRecognitionV2::LicensePlateRecognitionV2()
    : LicensePlateRecognitionBase(CVI_MEM_SYSTEM) {
  for (int i = 0; i < 3; i++) {
    m_preprocess_param[0].factor[i] = SCALE;
    m_preprocess_param[0].mean[i] = MEAN;
  }
  m_preprocess_param[0].format = PIXEL_FORMAT_RGB_888_PLANAR;
  m_preprocess_param[0].keep_aspect_ratio = false;
}

void LicensePlateRecognitionV2::greedy_decode(float *prebs) {
  CVI_SHAPE shape = getOutputShape(0);
  // 80，18
  int rows = shape.dim[1];
  int cols = shape.dim[2];

  int index[cols];
  // argmax index
  for (int i = 0; i < cols; i++) {
    float max = prebs[cols];
    int maxIndex = 0;
    for (int j = 1; j < rows; j++) {
      if (prebs[i + j * cols] > max) {
        max = prebs[i + j * cols];
        maxIndex = j;
      }
    }
    index[i] = maxIndex;
  }

  std::vector<int> no_repeat_blank_label;
  uint32_t pre_c = index[0];
  if (pre_c != CHARS.size() - 1) {
    no_repeat_blank_label.push_back(pre_c);
  }
  for (int i = 0; i < cols; i++) {
    uint32_t c = index[i];
    if ((pre_c == c) || (c == CHARS.size() - 1)) {
      if (c == CHARS.size() - 1) {
        pre_c = c;
      }
      continue;
    }
    no_repeat_blank_label.push_back(c);
    pre_c = c;
  }

  std::string lb = "";
  for (int k : no_repeat_blank_label) {
    lb += CHARS[k];
    lb += " ";
  }
  std::cout << "Prediction: " << lb << std::endl;
}

int LicensePlateRecognitionV2::inference(VIDEO_FRAME_INFO_S *frame, cvtdl_object_t *vehicle_meta) {
  if (frame->stVFrame.enPixelFormat != PIXEL_FORMAT_RGB_888_PLANAR &&
      frame->stVFrame.enPixelFormat != PIXEL_FORMAT_BGR_888_PLANAR) {
    LOGE(
        "Error: pixel format not match PIXEL_FORMAT_RGB_888_PLANAR or PIXEL_FORMAT_BGR_888_PLANAR");
    return CVI_TDL_ERR_INVALID_ARGS;
  }

  for (uint32_t i = 0; i < vehicle_meta->size; ++i) {
    cvtdl_object_info_t obj_info = info_extern_crop_resize_img(
        frame->stVFrame.u32Width, frame->stVFrame.u32Height, &(vehicle_meta->info[i]));
    VIDEO_FRAME_INFO_S *f = new VIDEO_FRAME_INFO_S;
    memset(f, 0, sizeof(VIDEO_FRAME_INFO_S));
    CVI_SHAPE shape = getInputShape(0);
    int height = shape.dim[2];
    int width = shape.dim[3];
    vpssCropImage(frame, f, obj_info.bbox, width, height, PIXEL_FORMAT_RGB_888_PLANAR);

    std::vector<VIDEO_FRAME_INFO_S *> frames = {f};
    int ret = run(frames);
    if (ret != CVI_TDL_SUCCESS) {
      mp_vpss_inst->releaseFrame(f, 0);
      delete f;
      return ret;
    }

    float *out = getOutputRawPtr<float>(OUTPUT_NAME_PROBABILITY);
    greedy_decode(out);
    mp_vpss_inst->releaseFrame(f, 0);
    delete f;

    CVI_TDL_FreeCpp(&obj_info);
  }

  return CVI_TDL_SUCCESS;
}

}  // namespace cvitdl