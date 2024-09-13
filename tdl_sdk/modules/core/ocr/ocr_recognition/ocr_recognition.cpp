#include "ocr_recognition.hpp"
#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem.h"
#include "core/cvi_tdl_types_mem_internal.h"
#include "core_utils.hpp"
#include "cvi_sys.h"

#include "core/utils/vpss_helper.h"

#include <codecvt>
#include <fstream>
#include <iostream>
#include <locale>
#include <sstream>
#ifndef NO_OPENCV
#include <opencv2/opencv.hpp>
#include "opencv2/imgproc.hpp"
namespace cvitdl {

void save_cropped_frame(VIDEO_FRAME_INFO_S* cropped_frame, const std::string& save_path) {
  CVI_U8* r_plane = (CVI_U8*)CVI_SYS_Mmap(cropped_frame->stVFrame.u64PhyAddr[0],
                                          cropped_frame->stVFrame.u32Length[0]);
  CVI_U8* g_plane = (CVI_U8*)CVI_SYS_Mmap(cropped_frame->stVFrame.u64PhyAddr[1],
                                          cropped_frame->stVFrame.u32Length[1]);
  CVI_U8* b_plane = (CVI_U8*)CVI_SYS_Mmap(cropped_frame->stVFrame.u64PhyAddr[2],
                                          cropped_frame->stVFrame.u32Length[2]);

  cv::Mat r_mat(cropped_frame->stVFrame.u32Height, cropped_frame->stVFrame.u32Width, CV_8UC1,
                r_plane);
  cv::Mat g_mat(cropped_frame->stVFrame.u32Height, cropped_frame->stVFrame.u32Width, CV_8UC1,
                g_plane);
  cv::Mat b_mat(cropped_frame->stVFrame.u32Height, cropped_frame->stVFrame.u32Width, CV_8UC1,
                b_plane);

  std::vector<cv::Mat> channels = {b_mat, g_mat, r_mat};
  cv::Mat img_rgb;
  cv::merge(channels, img_rgb);

  cv::imwrite(save_path, img_rgb);

  CVI_SYS_Munmap(r_plane, cropped_frame->stVFrame.u32Length[0]);
  CVI_SYS_Munmap(g_plane, cropped_frame->stVFrame.u32Length[1]);
  CVI_SYS_Munmap(b_plane, cropped_frame->stVFrame.u32Length[2]);
}
}  // namespace cvitdl

#endif

#define R_SCALE (0.003922)
#define G_SCALE (0.003922)
#define B_SCALE (0.003922)
#define R_MEAN (0)
#define G_MEAN (0)
#define B_MEAN (0)

namespace cvitdl {

OCRRecognition::OCRRecognition() : Core(CVI_MEM_DEVICE) {
  m_preprocess_param[0].factor[0] = R_SCALE;
  m_preprocess_param[0].factor[1] = G_SCALE;
  m_preprocess_param[0].factor[2] = B_SCALE;
  m_preprocess_param[0].mean[0] = R_MEAN;
  m_preprocess_param[0].mean[1] = G_MEAN;
  m_preprocess_param[0].mean[2] = B_MEAN;
}
OCRRecognition::~OCRRecognition() {}

int argmax(float* start, float* end) {
  float* max_iter = std::max_element(start, end);
  int max_idx = max_iter - start;
  return max_idx;
}

template <typename InputIt, typename T>
T my_accumulate(InputIt first, InputIt last, T init) {
  for (; first != last; ++first) {
    init = init + *first;
  }
  return init;
}

std::vector<std::string> ReadDict(const std::string& path) {
  std::ifstream in(path);
  std::string line;
  std::vector<std::string> m_vec;
  if (in) {
    while (getline(in, line)) {
      m_vec.push_back(line);
    }
  } else {
    std::cout << "no such label file: " << path << ", exit the program..." << std::endl;
    exit(1);
  }
  return m_vec;
}

std::pair<std::string, float> OCRRecognition::decode(const std::vector<int>& text_index,
                                                     const std::vector<float>& text_prob,
                                                     std::vector<std::string>& chars,
                                                     bool is_remove_duplicate) {
  std::string text;
  std::vector<float> conf_list;

  for (size_t idx = 0; idx < text_index.size(); ++idx) {
    if (is_remove_duplicate && idx > 0 && text_index[idx - 1] == text_index[idx]) {
      continue;
    }
    text += chars[text_index[idx]];
    conf_list.push_back(text_prob[idx]);
  }

  float average_conf = conf_list.empty() ? 0.0f
                                         : my_accumulate(conf_list.begin(), conf_list.end(), 0.0f) /
                                               conf_list.size();
  return {text, average_conf};
}

void OCRRecognition::greedy_decode(float* prebs, std::vector<std::string>& chars) {
  CVI_SHAPE output_shape = getOutputShape(0);
  int outShapeC = output_shape.dim[1];
  int outHeight = output_shape.dim[2];
  int outWidth = output_shape.dim[3];

  std::vector<int> argmax_results(outShapeC, 0);
  std::vector<float> confidences(outShapeC, 0.0f);

  for (int t = 0; t < outShapeC; ++t) {
    float* start = prebs + t * outWidth * outHeight;
    float* end = prebs + (t + 1) * outWidth * outHeight;
    int argmax_idx = argmax(start, end);

    argmax_results[t] = argmax_idx - 1;
    confidences[t] = *(start + argmax_idx);
  }
  auto result = decode(argmax_results, confidences, chars, true);
  std::cout << "Decoded Text: " << result.first << ", Average Confidence: " << result.second
            << std::endl;
}

// 可以不通过det，打开下方接口
//  int OCRRecognition::inference(VIDEO_FRAME_INFO_S *frame, cvtdl_object_t *obj_meta) {

//     std::vector<std::string> myCharacters;
//     std::string filePath = "./ppocr_keys_v1.txt";
//     myCharacters = ReadDict(filePath);

//     std::vector<VIDEO_FRAME_INFO_S *> frames = {frame};
//     int ret = run(frames);
//     if (ret != CVI_TDL_SUCCESS) {
//         return ret;
//     }

//     float *out = getOutputRawPtr<float>(0);
//     greedy_decode(out, myCharacters);

//     return CVI_TDL_SUCCESS;
//   }

cvtdl_object_info_t crop_resize_img(const float frame_width, const float frame_height,
                                    const cvtdl_object_info_t* obj_info) {
  cvtdl_object_info_t obj_info_new = {0};
  CVI_TDL_CopyInfoCpp(obj_info, &obj_info_new);

  cvtdl_bbox_t bbox = obj_info_new.bbox;
  int w_pad = (bbox.x2 - bbox.x1) * 0.2;
  int h_pad = (bbox.y2 - bbox.y1) * 0.5;

  // bbox new coordinate after extern
  float x1 = bbox.x1 - w_pad;
  float y1 = bbox.y1 - h_pad;
  float x2 = bbox.x2 + w_pad;
  float y2 = bbox.y2 + h_pad;

  cvtdl_bbox_t new_bbox;
  new_bbox.score = bbox.score;
  new_bbox.x1 = std::max(x1, (float)0);
  new_bbox.y1 = std::max(y1, (float)0);
  new_bbox.x2 = std::min(x2, (float)(frame_width - 1));
  new_bbox.y2 = std::min(y2, (float)(frame_height - 1));
  obj_info_new.bbox = new_bbox;

  return obj_info_new;
}

int OCRRecognition::inference(VIDEO_FRAME_INFO_S* frame, cvtdl_object_t* obj_meta) {
  if (obj_meta->size == 0) {
    return CVI_TDL_SUCCESS;
  }
  std::vector<std::string> myCharacters;
  std::string filePath = "./ppocr_keys_v1.txt";
  myCharacters = ReadDict(filePath);

  for (uint32_t i = 0; i < obj_meta->size; ++i) {
    cvtdl_object_info_t obj_info = info_extern_crop_resize_img(
        frame->stVFrame.u32Width, frame->stVFrame.u32Height, &(obj_meta->info[i]), 0.2, 0.5);
    VIDEO_FRAME_INFO_S* cropped_frame = new VIDEO_FRAME_INFO_S;
    memset(cropped_frame, 0, sizeof(VIDEO_FRAME_INFO_S));
    CVI_SHAPE shape = getInputShape(0);
    int height = shape.dim[2];
    int width = shape.dim[3];
    vpssCropImage(frame, cropped_frame, obj_info.bbox, width, height, PIXEL_FORMAT_RGB_888_PLANAR);

    // 可以保存cropped_frame,此功能必须提供opencv支持
    // std::string save_path = std::to_string(i) + ".jpg";
    // save_cropped_frame(cropped_frame, save_path);

    std::vector<VIDEO_FRAME_INFO_S*> frames = {cropped_frame};
    int ret = run(frames);
    if (ret != CVI_TDL_SUCCESS) {
      mp_vpss_inst->releaseFrame(cropped_frame, 0);
      delete cropped_frame;
      return ret;
    }

    float* out = getOutputRawPtr<float>(0);
    greedy_decode(out, myCharacters);
    mp_vpss_inst->releaseFrame(cropped_frame, 0);
    delete cropped_frame;
  }
  return CVI_TDL_SUCCESS;
}

}  // namespace cvitdl