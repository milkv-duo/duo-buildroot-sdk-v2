#include "eye_classification.hpp"
#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem_internal.h"
#include "core/utils/vpss_helper.h"
#include "core_utils.hpp"
#include "opencv2/imgproc.hpp"

#define EYECLASSIFICATION_SCALE (1.0 / (255.0))
#define NAME_SCORE "prob_Sigmoid_dequant"
#define MINIMUM_SIZE 10
#define INPUT_SIZE 32

namespace cvitdl {

EyeClassification::EyeClassification() : Core(CVI_MEM_SYSTEM) {}

int EyeClassification::inference(VIDEO_FRAME_INFO_S *frame, cvtdl_face_t *meta) {
  if (frame->stVFrame.enPixelFormat != PIXEL_FORMAT_RGB_888) {
    LOGE("Error: pixel format not match PIXEL_FORMAT_RGB_888.\n");
    return CVI_TDL_ERR_INVALID_ARGS;
  }

  int img_width = frame->stVFrame.u32Width;
  int img_height = frame->stVFrame.u32Height;
  frame->stVFrame.pu8VirAddr[0] =
      (CVI_U8 *)CVI_SYS_Mmap(frame->stVFrame.u64PhyAddr[0], frame->stVFrame.u32Length[0]);
  cv::Mat image(img_height, img_width, CV_8UC3, frame->stVFrame.pu8VirAddr[0],
                frame->stVFrame.u32Stride[0]);

  // just one face
  for (uint32_t i = 0; i < 1; i++) {
    cvtdl_face_info_t face_info =
        info_rescale_c(frame->stVFrame.u32Width, frame->stVFrame.u32Height, *meta, i);
    int eye_w = abs(meta->dms->landmarks_5.x[0] - meta->dms->landmarks_5.x[1]);

    float q_w = eye_w / 3;

    cv::Rect r_roi(int(std::max(meta->dms->landmarks_5.x[0] - q_w, face_info.bbox.x1)),
                   int(std::max(meta->dms->landmarks_5.y[0] - q_w, face_info.bbox.y1)),
                   int(std::min(meta->dms->landmarks_5.x[0] + q_w, face_info.bbox.x2)) -
                       int(std::max(meta->dms->landmarks_5.x[0] - q_w, face_info.bbox.x1)),
                   int(std::min(meta->dms->landmarks_5.y[0] + q_w, face_info.bbox.y2)) -
                       int(std::max(meta->dms->landmarks_5.y[0] - q_w, face_info.bbox.y1)));

    cv::Rect l_roi(int(std::max(meta->dms->landmarks_5.x[1] - q_w, face_info.bbox.x1)),
                   int(std::max(meta->dms->landmarks_5.y[1] - q_w, face_info.bbox.y1)),
                   int(std::min(meta->dms->landmarks_5.x[1] + q_w, face_info.bbox.x2)) -
                       int(std::max(meta->dms->landmarks_5.x[1] - q_w, face_info.bbox.x1)),
                   int(std::min(meta->dms->landmarks_5.y[1] + q_w, face_info.bbox.y2)) -
                       int(std::max(meta->dms->landmarks_5.y[1] - q_w, face_info.bbox.y1)));
    if (r_roi.width < MINIMUM_SIZE || r_roi.height < MINIMUM_SIZE) {  // small images filter
      meta->dms->reye_score = 0.0;
    } else {
      cv::Mat r_Image = image(r_roi);
      cv::resize(r_Image, r_Image, cv::Size(INPUT_SIZE, INPUT_SIZE));
      cv::cvtColor(r_Image, r_Image, cv::COLOR_RGB2GRAY);
      prepareInputTensor(r_Image);

      std::vector<VIDEO_FRAME_INFO_S *> frames = {frame};
      int ret = run(frames);
      if (ret != CVI_TDL_SUCCESS) {
        return ret;
      }

      float *score = getOutputRawPtr<float>(NAME_SCORE);
      meta->dms->reye_score = score[0];
    }
    // left eye patch
    if (l_roi.width < MINIMUM_SIZE || l_roi.height < MINIMUM_SIZE) {  // mall images filter
      meta->dms->leye_score = 0.0;
    } else {
      cv::Mat l_Image = image(l_roi);

      cv::resize(l_Image, l_Image, cv::Size(INPUT_SIZE, INPUT_SIZE));
      cv::cvtColor(l_Image, l_Image, cv::COLOR_RGB2GRAY);

      prepareInputTensor(l_Image);

      std::vector<VIDEO_FRAME_INFO_S *> frames = {frame};
      run(frames);

      float *score = getOutputRawPtr<float>(NAME_SCORE);
      meta->dms->leye_score = score[0];
    }
    CVI_TDL_FreeCpp(&face_info);
  }

  CVI_SYS_Munmap((void *)frame->stVFrame.pu8VirAddr[0], frame->stVFrame.u32Length[0]);
  frame->stVFrame.pu8VirAddr[0] = NULL;
  frame->stVFrame.pu8VirAddr[1] = NULL;
  frame->stVFrame.pu8VirAddr[2] = NULL;

  return CVI_TDL_SUCCESS;
}

void EyeClassification::prepareInputTensor(cv::Mat &input_mat) {
  const TensorInfo &tinfo = getInputTensorInfo(0);
  float *input_ptr = tinfo.get<float>();
  cv::Mat temp_mat;
  input_mat.convertTo(temp_mat, CV_32FC1, EYECLASSIFICATION_SCALE * 1.0, 0);
  cv::add(-0.5, temp_mat, temp_mat);
  cv::multiply(temp_mat, cv::Scalar(2), temp_mat);
  for (int r = 0; r < temp_mat.rows; ++r) {
    memcpy(input_ptr + temp_mat.cols * r, (float *)temp_mat.ptr(r, 0),
           sizeof(float) * temp_mat.cols);
  }
}

}  // namespace cvitdl
