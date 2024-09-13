#include "license_plate_detection.hpp"

#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem.h"
#include "core/cvi_tdl_types_mem_internal.h"
#include "core_utils.hpp"
#include "cvi_sys.h"

#include "core/utils/vpss_helper.h"

#include <iostream>
#include <sstream>
#define MIN(a, b) ((a) <= (b) ? (a) : (b))
#define MAX(a, b) ((a) >= (b) ? (a) : (b))
#define DEBUG_LICENSE_PLATE_DETECTION 0
#if DEBUG_LICENSE_PLATE_DETECTION2 && !NO_OPENCV
#include <opencv2/opencv.hpp>
#include "opencv2/imgproc.hpp"
#endif

#define OUTPUT_NAME_PROBABILITY "conv2d_25_dequant"
#define OUTPUT_NAME_TRANSFORM "conv2d_26_dequant"

namespace cvitdl {

LicensePlateDetection::LicensePlateDetection() : Core(CVI_MEM_DEVICE) {
  for (int i = 0; i < 3; i++) {
    /* VPSS clip image to 128, we divide it by 2 first, then multiply by 2 in cvimodel preprocess */
    m_preprocess_param[0].factor[i] = 0.5;
    m_preprocess_param[0].mean[i] = 0.0;
  }
  m_preprocess_param[0].rescale_type = RESCALE_RB;
  m_preprocess_param[0].use_crop = true;
  m_preprocess_param[0].keep_aspect_ratio = true;
}

LicensePlateDetection::~LicensePlateDetection() {}

int LicensePlateDetection::onModelOpened() {
  /* set model config */
  CVI_SHAPE in_tensor_shape = getInputShape("input_raw");
  vehicle_h = static_cast<int>(in_tensor_shape.dim[2]);
  vehicle_w = static_cast<int>(in_tensor_shape.dim[3]);
  out_tensor_h = vehicle_h / 16;
  out_tensor_w = vehicle_w / 16;

  return CVI_TDL_SUCCESS;
}

int LicensePlateDetection::vpssPreprocess(VIDEO_FRAME_INFO_S *srcFrame,
                                          VIDEO_FRAME_INFO_S *dstFrame, VPSSConfig &vpss_config) {
  auto &vpssChnAttr = vpss_config.chn_attr;
  auto &vpssCropAttr = vpss_config.crop_attr;
  auto &factor = vpssChnAttr.stNormalize.factor;
  auto &mean = vpssChnAttr.stNormalize.mean;
  VPSS_CHN_SQ_RB_HELPER(&vpssChnAttr, vpssCropAttr.stCropRect.u32Width,
                        vpssCropAttr.stCropRect.u32Height, vpssChnAttr.u32Width,
                        vpssChnAttr.u32Height, PIXEL_FORMAT_RGB_888_PLANAR, factor, mean, false);
  if (mp_vpss_inst->sendCropChnFrame(srcFrame, &vpss_config.crop_attr, &vpss_config.chn_attr,
                                     &vpss_config.chn_coeff, 1) != CVI_SUCCESS) {
    return CVI_TDL_ERR_VPSS_SEND_FRAME;
  }

  if (mp_vpss_inst->getFrame(dstFrame, 0) != CVI_SUCCESS) {
    return CVI_TDL_ERR_VPSS_GET_FRAME;
  }
  return CVI_TDL_SUCCESS;
}

static void get_resize_scale(cvtdl_object_t *vehicle_meta, std::vector<float> &rs_scale,
                             int vehicle_h, int vehicle_w) {
  for (uint32_t i = 0; i < vehicle_meta->size; i++) {
    float x = vehicle_meta->info[i].bbox.x1;
    float y = vehicle_meta->info[i].bbox.y1;
    float width = vehicle_meta->info[i].bbox.x2 - x;
    float height = vehicle_meta->info[i].bbox.y2 - y;

    int cols = static_cast<int>(width);
    int rows = static_cast<int>(height);
    float h_scale = (float)vehicle_h / rows;
    float w_scale = (float)vehicle_w / cols;
    float scale = MIN(h_scale, w_scale);
    rs_scale[i] = scale;
  }
}

int LicensePlateDetection::inference(VIDEO_FRAME_INFO_S *frame, cvtdl_object_t *vehicle_meta) {
  if (frame->stVFrame.enPixelFormat != PIXEL_FORMAT_RGB_888) {
    LOGE("Error: pixel format not match PIXEL_FORMAT_RGB_888.\n");
    return CVI_TDL_ERR_INVALID_ARGS;
  }
#if DEBUG_LICENSE_PLATE_DETECTION
  std::stringstream s_str;
  printf("[%s:%d] inference\n", __FILE__, __LINE__);
#endif
  if (vehicle_meta->size == 0) {
    return CVI_TDL_SUCCESS;
  }

  std::vector<float> rs_scale(vehicle_meta->size);  // resize scale
  get_resize_scale(vehicle_meta, rs_scale, vehicle_h, vehicle_w);

  for (uint32_t n = 0; n < vehicle_meta->size; n++) {
    m_vpss_config[0].crop_attr.enCropCoordinate = VPSS_CROP_ABS_COOR;
    float x1 = vehicle_meta->info[n].bbox.x1;
    float y1 = vehicle_meta->info[n].bbox.y1;
    float x2 = vehicle_meta->info[n].bbox.x2;
    float y2 = vehicle_meta->info[n].bbox.y2;
    m_vpss_config[0].crop_attr.stCropRect = {(int32_t)x1, (int32_t)y1, (uint32_t)(x2 - x1),
                                             (uint32_t)(y2 - y1)};
    std::vector<VIDEO_FRAME_INFO_S *> frames = {frame};
    int ret = run(frames);
    if (ret != CVI_TDL_SUCCESS) {
      return ret;
    }

    float *out_p = getOutputRawPtr<float>(OUTPUT_NAME_PROBABILITY);
    float *out_t = getOutputRawPtr<float>(OUTPUT_NAME_TRANSFORM);

    // TODO:
    //   return more corner points, use std::vector<CornerPts> ?
    CornerPts corner_pts;
    float score;
    bool detection_result = reconstruct(out_p, out_t, corner_pts, score, 0.9);

    // TODO:
    //   add more points to bpts for false positive
    if (detection_result) {
      vehicle_meta->info[n].vehicle_properity =
          (cvtdl_vehicle_meta *)malloc(sizeof(cvtdl_vehicle_meta));
      for (int m = 0; m < 4; m++) {
        vehicle_meta->info[n].vehicle_properity->license_pts.x[m] =
            corner_pts(0, m) / rs_scale[n] + vehicle_meta->info[n].bbox.x1;
        vehicle_meta->info[n].vehicle_properity->license_pts.y[m] =
            corner_pts(1, m) / rs_scale[n] + vehicle_meta->info[n].bbox.y1;
      }

      cvtdl_bbox_t &bbox = vehicle_meta->info[n].vehicle_properity->license_bbox;
      cvtdl_4_pts_t &pts = vehicle_meta->info[n].vehicle_properity->license_pts;

      bbox.x1 = std::min({pts.x[0], pts.x[1], pts.x[2], pts.x[3]});
      bbox.x2 = std::max({pts.x[0], pts.x[1], pts.x[2], pts.x[3]});
      bbox.y1 = std::min({pts.y[0], pts.y[1], pts.y[2], pts.y[3]});
      bbox.y2 = std::max({pts.y[0], pts.y[1], pts.y[2], pts.y[3]});
      bbox.score = score;
    }
  }
  return CVI_TDL_SUCCESS;
}

bool LicensePlateDetection::reconstruct(float *t_prob, float *t_trans, CornerPts &c_pts,
                                        float &ret_prob, float threshold_prob) {
#if DEBUG_LICENSE_PLATE_DETECTION
  printf("[%s:%d] reconstruct\n", __FILE__, __LINE__);
#endif
  Eigen::Matrix<float, 3, 4> anchors;
  // anchors = [[ -0.5,  0.5,  0.5, -0.5]
  //            [ -0.5, -0.5,  0.5,  0.5]
  //            [  1.0,  1.0,  1.0,  1.0]]
  anchors << -0.5, 0.5, 0.5, -0.5, -0.5, -0.5, 0.5, 0.5, 1.0, 1.0, 1.0, 1.0;
  Eigen::Matrix<float, 2, 1> tensor_wh;
  tensor_wh << out_tensor_w, out_tensor_h;
  int tensor_size = out_tensor_h * out_tensor_w;
  std::vector<LicensePlateObjBBox> lp_cands;
  for (int i = 0; i < out_tensor_h; i++) {
    for (int j = 0; j < out_tensor_w; j++) {
      int ij = i * out_tensor_w + j;
      float prob_pos = t_prob[ij];
      float prob_neg = t_prob[tensor_size + ij];
      float exp_pos = std::exp(prob_pos);
      float exp_neg = std::exp(prob_neg);
      float softmax = exp_pos / (exp_pos + exp_neg);  // softmax
      if (softmax < threshold_prob) continue;
      Eigen::Matrix<float, 2, 1> center_pts;
      center_pts << (float)j + 0.5, (float)i + 0.5;
      Eigen::Matrix<float, 2, 3> affine;
      affine << t_trans[tensor_size * 0 + ij], t_trans[tensor_size * 1 + ij],
          t_trans[tensor_size * 2 + ij], t_trans[tensor_size * 3 + ij],
          t_trans[tensor_size * 4 + ij], t_trans[tensor_size * 5 + ij];
      affine(0, 0) = MAX(affine(0, 0), 0);
      affine(1, 1) = MAX(affine(1, 1), 0);

      Eigen::Matrix<float, 2, 4> affine_pts;
      affine_pts = SIDE * affine * anchors;
      affine_pts = affine_pts.colwise() + center_pts;
      for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 4; j++) {
          affine_pts(i, j) = affine_pts(i, j) / tensor_wh(i, 0);
        }
      }
      // TODO:
      //   [fix]  affine_pts = affine_pts.colwise() / tensor_wh;

      lp_cands.push_back(LicensePlateObjBBox(0, affine_pts, softmax));
    }
  }
  std::vector<LicensePlateObjBBox> selected_LP;
  nms(lp_cands, selected_LP);

  if (selected_LP.size() > 0) {
    for (auto it = selected_LP.begin(); it != selected_LP.end(); it++) {
      Eigen::Matrix<float, 2, 4> corner_pts = it->getCornerPts(vehicle_w, vehicle_h);
      c_pts = corner_pts;
      ret_prob = it->prob();
      return true;
#if DEBUG_LICENSE_PLATE_DETECTION
      cv::Point2f src_points[4] = {
          cv::Point2f(corner_pts(0, 0), corner_pts(1, 0)),
          cv::Point2f(corner_pts(0, 1), corner_pts(1, 1)),
          cv::Point2f(corner_pts(0, 2), corner_pts(1, 2)),
          cv::Point2f(corner_pts(0, 3), corner_pts(1, 3)),
      };
      cv::Point2f dst_points[4] = {
          cv::Point2f(0, 0),
          cv::Point2f(LICENSE_PLATE_WIDTH, 0),
          cv::Point2f(LICENSE_PLATE_WIDTH, LICENSE_PLATE_HEIGHT),
          cv::Point2f(0, LICENSE_PLATE_HEIGHT),
      };

      cv::Mat M = cv::getPerspectiveTransform(src_points, dst_points);

      std::cout << "M: " << std::endl << M << std::endl;
#endif
    }
  }
  return false;
}

}  // namespace cvitdl