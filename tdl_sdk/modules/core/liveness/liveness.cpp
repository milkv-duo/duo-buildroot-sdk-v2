#include "liveness.hpp"
#include "template_matching.hpp"

#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem.h"
#include "core_utils.hpp"

#include "cvi_sys.h"

#include "opencv2/imgproc.hpp"

#define RESIZE_SIZE 112
#define LIVENESS_SCALE (1 / 255.0)
#define LIVENESS_N 1
#define LIVENESS_C 6
#define LIVENESS_WIDTH 32
#define LIVENESS_HEIGHT 32
#define CROP_NUM 9
#define MIN_FACE_WIDTH 50
#define MIN_FACE_HEIGHT 50
#define MATCH_IOU_THRESHOLD 0.2
#define OUTPUT_NAME "fc2_dequant"

namespace cvitdl {

static std::vector<std::vector<cv::Mat>> image_preprocess(VIDEO_FRAME_INFO_S *frame,
                                                          VIDEO_FRAME_INFO_S *sink_buffer,
                                                          cvtdl_face_t *rgb_meta,
                                                          cvtdl_face_t *ir_meta) {
  frame->stVFrame.pu8VirAddr[0] =
      (CVI_U8 *)CVI_SYS_Mmap(frame->stVFrame.u64PhyAddr[0], frame->stVFrame.u32Length[0]);
  cv::Mat rgb_frame(frame->stVFrame.u32Height, frame->stVFrame.u32Width, CV_8UC3,
                    frame->stVFrame.pu8VirAddr[0], frame->stVFrame.u32Stride[0]);
  if (rgb_frame.data == nullptr) {
    LOGE("src image is empty!\n");
    return std::vector<std::vector<cv::Mat>>{};
  }

  sink_buffer->stVFrame.pu8VirAddr[0] = (CVI_U8 *)CVI_SYS_Mmap(sink_buffer->stVFrame.u64PhyAddr[0],
                                                               sink_buffer->stVFrame.u32Length[0]);
  cv::Mat ir_frame(sink_buffer->stVFrame.u32Height, sink_buffer->stVFrame.u32Width, CV_8UC3,
                   sink_buffer->stVFrame.pu8VirAddr[0], sink_buffer->stVFrame.u32Stride[0]);
  if (ir_frame.data == nullptr) {
    LOGE("sink image is empty!\n");
    CVI_SYS_Munmap((void *)frame->stVFrame.pu8VirAddr[0], frame->stVFrame.u32Length[0]);
    frame->stVFrame.pu8VirAddr[0] = NULL;
    return std::vector<std::vector<cv::Mat>>{};
  }

  std::vector<cv::Rect> rgb_boxs;
  std::vector<cv::Rect> ir_boxs;

  for (uint32_t i = 0; i < rgb_meta->size; i++) {
    cvtdl_face_info_t rgb_face_info =
        info_rescale_c(frame->stVFrame.u32Width, frame->stVFrame.u32Height, *rgb_meta, i);
    cv::Rect rgb_box;
    rgb_box.x = rgb_face_info.bbox.x1;
    rgb_box.y = rgb_face_info.bbox.y1;
    rgb_box.width = rgb_face_info.bbox.x2 - rgb_box.x;
    rgb_box.height = rgb_face_info.bbox.y2 - rgb_box.y;
    rgb_boxs.push_back(rgb_box);
    CVI_TDL_FreeCpp(&rgb_face_info);
  }

  for (uint32_t i = 0; i < ir_meta->size; i++) {
    // cvtdl_face_info_t ir_face_info = rgb_face_info;
    cvtdl_face_info_t ir_face_info = info_rescale_c(sink_buffer->stVFrame.u32Width,
                                                    sink_buffer->stVFrame.u32Height, *ir_meta, i);

    cv::Rect ir_box;
    ir_box.x = ir_face_info.bbox.x1;
    ir_box.y = ir_face_info.bbox.y1;
    ir_box.width = ir_face_info.bbox.x2 - ir_box.x;
    ir_box.height = ir_face_info.bbox.y2 - ir_box.y;
    ir_boxs.push_back(ir_box);
    CVI_TDL_FreeCpp(&ir_face_info);
  }

  std::vector<std::vector<cv::Mat>> input_mat(rgb_boxs.size(), std::vector<cv::Mat>());
  for (uint32_t i = 0; i < rgb_boxs.size(); i++) {
    std::vector<float> iou_result;
    for (uint32_t j = 0; j < ir_boxs.size(); j++) {
      cv::Rect rect_uni = rgb_boxs[i] | ir_boxs[j];
      cv::Rect rect_int = rgb_boxs[i] & ir_boxs[j];
      // IOU for each pairs
      float iou = rect_int.area() * 1.0 / rect_uni.area();
      iou_result.push_back(iou);
    }

    int match_index = -1;
    cv::Rect match_result;
    if (iou_result.size() > 0) {
      float maxElement = *max_element(iou_result.begin(), iou_result.end());
      if (maxElement > MATCH_IOU_THRESHOLD) {
        match_index = max_element(iou_result.begin(), iou_result.end()) - iou_result.begin();
        match_result = ir_boxs[match_index];
        ir_boxs.erase(ir_boxs.begin() + match_index);
      }
    }

    if (match_index == -1) continue;

    cv::Rect rgb_box = rgb_boxs[i];
    cv::Rect ir_box = match_result;

    if (rgb_box.width <= MIN_FACE_WIDTH || rgb_box.height <= MIN_FACE_HEIGHT ||
        ir_box.width <= MIN_FACE_WIDTH || ir_box.height <= MIN_FACE_HEIGHT)
      continue;
    cv::Mat crop_rgb_frame = rgb_frame(rgb_box);
    cv::Mat crop_ir_frame = ir_frame(ir_box);

    cv::Mat color, ir;
    cv::resize(crop_rgb_frame, color, cv::Size(RESIZE_SIZE, RESIZE_SIZE));
    cv::resize(crop_ir_frame, ir, cv::Size(RESIZE_SIZE, RESIZE_SIZE));

    std::vector<cv::Mat> colors = TTA_9_cropps(color);
    std::vector<cv::Mat> irs = TTA_9_cropps(ir);

    std::vector<cv::Mat> input_v;
    for (uint32_t j = 0; j < colors.size(); j++) {
      cv::Mat temp;
      cv::merge(std::vector<cv::Mat>{colors[j], irs[j]}, temp);
      input_v.push_back(temp);
    }
    input_mat[i] = input_v;
  }

  CVI_SYS_Munmap((void *)frame->stVFrame.pu8VirAddr[0], frame->stVFrame.u32Length[0]);
  frame->stVFrame.pu8VirAddr[0] = NULL;
  CVI_SYS_Munmap((void *)sink_buffer->stVFrame.pu8VirAddr[0], sink_buffer->stVFrame.u32Length[0]);
  sink_buffer->stVFrame.pu8VirAddr[0] = NULL;

  return input_mat;
}

Liveness::Liveness() : Core(CVI_MEM_SYSTEM) {}

int Liveness::inference(VIDEO_FRAME_INFO_S *rgbFrame, VIDEO_FRAME_INFO_S *irFrame,
                        cvtdl_face_t *rgb_meta, cvtdl_face_t *ir_meta) {
  if (rgb_meta->size <= 0) {
    LOGE("rgb_meta->size <= 0");
    return CVI_TDL_ERR_INVALID_ARGS;
  }

  std::vector<std::vector<cv::Mat>> input_mats =
      image_preprocess(rgbFrame, irFrame, rgb_meta, ir_meta);
  if (input_mats.empty() || input_mats.size() != rgb_meta->size) {
    LOGE("Get input_mats failed");
    return CVI_TDL_ERR_INVALID_ARGS;
  }

  for (uint32_t i = 0; i < input_mats.size(); i++) {
    float conf0 = 0.0;
    float conf1 = 0.0;

    std::vector<cv::Mat> input = input_mats[i];
    if (input.empty()) {
      rgb_meta->info[i].liveness_score = -1.0;
      continue;
    }

    prepareInputTensor(input);

    std::vector<VIDEO_FRAME_INFO_S *> frames = {rgbFrame};
    int ret = run(frames);
    if (ret != CVI_TDL_SUCCESS) {
      return ret;
    }

    float *out_data = getOutputRawPtr<float>(OUTPUT_NAME);
    for (int j = 0; j < CROP_NUM; j++) {
      conf0 += out_data[j * 2];
      conf1 += out_data[(j * 2) + 1];
    }

    conf0 /= input.size();
    conf1 /= input.size();

    float max = std::max(conf0, conf1);
    float f0 = std::exp(conf0 - max);
    float f1 = std::exp(conf1 - max);
    float score = f1 / (f0 + f1);

    rgb_meta->info[i].liveness_score = score;
    // cout << "Face[" << i << "] liveness score: " << score << endl;
  }

  return CVI_TDL_SUCCESS;
}

void Liveness::prepareInputTensor(std::vector<cv::Mat> &input_mat) {
  const TensorInfo &tinfo = getInputTensorInfo(0);
  int8_t *input_ptr = tinfo.get<int8_t>();
  float quant_scale = getInputQuantScale(0);

  for (int j = 0; j < CROP_NUM; j++) {
    cv::Mat tmpchannels[LIVENESS_C];
    cv::split(input_mat[j], tmpchannels);

    for (int c = 0; c < LIVENESS_C; ++c) {
      tmpchannels[c].convertTo(tmpchannels[c], CV_8SC1, LIVENESS_SCALE * quant_scale, 0);

      int size = tmpchannels[c].rows * tmpchannels[c].cols;
      for (int r = 0; r < tmpchannels[c].rows; ++r) {
        memcpy(input_ptr + size * c + tmpchannels[c].cols * r, tmpchannels[c].ptr(r, 0),
               tmpchannels[c].cols);
      }
    }
    input_ptr += tinfo.tensor_elem / CROP_NUM;
  }
}

}  // namespace cvitdl