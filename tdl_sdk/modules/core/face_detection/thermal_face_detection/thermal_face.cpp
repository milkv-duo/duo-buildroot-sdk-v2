#include "thermal_face.hpp"
#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem.h"
#include "core/cvi_tdl_types_mem_internal.h"
#include "core/utils/vpss_helper.h"
#include "core_utils.hpp"
#include "cvi_sys.h"

#define SCALE_R float(1.0 / (255.0 * 0.229))
#define SCALE_G float(1.0 / (255.0 * 0.224))
#define SCALE_B float(1.0 / (255.0 * 0.225))
#define MEAN_R float(0.485 / 0.229)
#define MEAN_G float(0.456 / 0.224)
#define MEAN_B float(0.406 / 0.225)
#define NAME_BBOX "regression_dequant"
#define NAME_SCORE "classification_dequant"

namespace cvitdl {

static std::vector<cvtdl_bbox_t> generate_anchors(int base_size, const std::vector<float> &ratios,
                                                  const std::vector<float> &scales) {
  int num_anchors = ratios.size() * scales.size();
  std::vector<cvtdl_bbox_t> anchors(num_anchors, cvtdl_bbox_t());
  std::vector<float> areas(num_anchors, 0);

  for (size_t i = 0; i < anchors.size(); i++) {
    anchors[i].x2 = base_size * scales[i % scales.size()];
    anchors[i].y2 = base_size * scales[i % scales.size()];
    areas[i] = anchors[i].x2 * anchors[i].y2;

    anchors[i].x2 = std::sqrt(areas[i] / ratios[i / scales.size()]);
    anchors[i].y2 = anchors[i].x2 * ratios[i / scales.size()];

    anchors[i].x1 -= anchors[i].x2 * 0.5;
    anchors[i].x2 -= anchors[i].x2 * 0.5;
    anchors[i].y1 -= anchors[i].y2 * 0.5;
    anchors[i].y2 -= anchors[i].y2 * 0.5;
  }

  return anchors;
}

static std::vector<cvtdl_bbox_t> shift(const std::vector<int> &shape, int stride,
                                       const std::vector<cvtdl_bbox_t> &anchors) {
  std::vector<int> shift_x(shape[0] * shape[1], 0);
  std::vector<int> shift_y(shape[0] * shape[1], 0);

  for (int i = 0; i < shape[0]; i++) {
    for (int j = 0; j < shape[1]; j++) {
      shift_x[i * shape[1] + j] = (j + 0.5) * stride;
    }
  }
  for (int i = 0; i < shape[0]; i++) {
    for (int j = 0; j < shape[1]; j++) {
      shift_y[i * shape[1] + j] = (i + 0.5) * stride;
    }
  }

  std::vector<cvtdl_bbox_t> shifts(shape[0] * shape[1], cvtdl_bbox_t());
  for (size_t i = 0; i < shifts.size(); i++) {
    shifts[i].x1 = shift_x[i];
    shifts[i].y1 = shift_y[i];
    shifts[i].x2 = shift_x[i];
    shifts[i].y2 = shift_y[i];
  }

  std::vector<cvtdl_bbox_t> all_anchors(anchors.size() * shifts.size(), cvtdl_bbox_t());
  for (size_t i = 0; i < shifts.size(); i++) {
    for (size_t j = 0; j < anchors.size(); j++) {
      all_anchors[i * anchors.size() + j].x1 = anchors[j].x1 + shifts[i].x1;
      all_anchors[i * anchors.size() + j].y1 = anchors[j].y1 + shifts[i].y1;
      all_anchors[i * anchors.size() + j].x2 = anchors[j].x2 + shifts[i].x2;
      all_anchors[i * anchors.size() + j].y2 = anchors[j].y2 + shifts[i].y2;
    }
  }

  return all_anchors;
}

static void bbox_pred(const cvtdl_bbox_t &anchor, float *regress, std::vector<float> std,
                      cvtdl_bbox_t &bbox) {
  float width = anchor.x2 - anchor.x1 + 1;
  float height = anchor.y2 - anchor.y1 + 1;
  float ctr_x = anchor.x1 + 0.5 * (width - 1.0);
  float ctr_y = anchor.y1 + 0.5 * (height - 1.0);

  regress[0] *= std[0];
  regress[1] *= std[1];
  regress[2] *= std[2];
  regress[3] *= std[3];

  float pred_ctr_x = regress[0] * width + ctr_x;
  float pred_ctr_y = regress[1] * height + ctr_y;
  float pred_w = FastExp(regress[2]) * width;
  float pred_h = FastExp(regress[3]) * height;

  bbox.x1 = (pred_ctr_x - 0.5 * (pred_w - 1.0));
  bbox.y1 = (pred_ctr_y - 0.5 * (pred_h - 1.0));
  bbox.x2 = (pred_ctr_x + 0.5 * (pred_w - 1.0));
  bbox.y2 = (pred_ctr_y + 0.5 * (pred_h - 1.0));
}

ThermalFace::ThermalFace() {
  m_preprocess_param[0].factor[0] = static_cast<float>(SCALE_R);
  m_preprocess_param[0].factor[1] = static_cast<float>(SCALE_G);
  m_preprocess_param[0].factor[2] = static_cast<float>(SCALE_B);
  m_preprocess_param[0].mean[0] = static_cast<float>(MEAN_R);
  m_preprocess_param[0].mean[1] = static_cast<float>(MEAN_G);
  m_preprocess_param[0].mean[2] = static_cast<float>(MEAN_B);
  m_preprocess_param[0].rescale_type = RESCALE_RB;
}

int ThermalFace::onModelOpened() {
  std::vector<int> pyramid_levels = {3, 4, 5, 6, 7};
  std::vector<int> strides = {8, 16, 32, 64, 128};
  std::vector<int> sizes = {24, 48, 96, 192, 384};
  std::vector<float> ratios = {1, 2};
  std::vector<float> scales = {1, 1.25992105, 1.58740105};

  CVI_SHAPE input_shape = getInputShape(0);
  int input_w = input_shape.dim[3];
  int input_h = input_shape.dim[2];
  std::vector<std::vector<int>> image_shapes;
  image_shapes.reserve(strides.size());
  for (int s : strides) {
    image_shapes.push_back({(input_h + s - 1) / s, (input_w + s - 1) / s});
  }

  for (size_t i = 0; i < sizes.size(); i++) {
    std::vector<cvtdl_bbox_t> anchors = generate_anchors(sizes[i], ratios, scales);
    std::vector<cvtdl_bbox_t> shifted_anchors = shift(image_shapes[i], strides[i], anchors);
    m_all_anchors.insert(m_all_anchors.end(), shifted_anchors.begin(), shifted_anchors.end());
  }
  return CVI_TDL_SUCCESS;
}

int ThermalFace::vpssPreprocess(VIDEO_FRAME_INFO_S *srcFrame, VIDEO_FRAME_INFO_S *dstFrame,
                                VPSSConfig &vpss_config) {
  auto &vpssChnAttr = vpss_config.chn_attr;
  auto &factor = vpssChnAttr.stNormalize.factor;
  auto &mean = vpssChnAttr.stNormalize.mean;
  VPSS_CHN_SQ_RB_HELPER(&vpssChnAttr, srcFrame->stVFrame.u32Width, srcFrame->stVFrame.u32Height,
                        vpssChnAttr.u32Width, vpssChnAttr.u32Height, PIXEL_FORMAT_RGB_888_PLANAR,
                        factor, mean, false);

  int ret = mp_vpss_inst->sendFrame(srcFrame, &vpssChnAttr, 1);
  if (ret != CVI_SUCCESS) {
    LOGE("Send frame failed with %#x!\n", ret);
    return CVI_TDL_ERR_VPSS_SEND_FRAME;
  }

  ret = mp_vpss_inst->getFrame(dstFrame, 0, m_vpss_timeout);
  if (ret != CVI_SUCCESS) {
    LOGE("Get frame failed with %#x!\n", ret);
    return CVI_TDL_ERR_VPSS_GET_FRAME;
  }
  return CVI_TDL_SUCCESS;
}

int ThermalFace::inference(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_face_t *meta) {
  std::vector<VIDEO_FRAME_INFO_S *> frames = {srcFrame};
  int ret = run(frames);
  if (ret != CVI_TDL_SUCCESS) {
    return ret;
  }

  CVI_SHAPE shape = getInputShape(0);
  outputParser(shape.dim[3], shape.dim[2], srcFrame->stVFrame.u32Width,
               srcFrame->stVFrame.u32Height, meta);

  return CVI_TDL_SUCCESS;
}

void ThermalFace::outputParser(const int image_width, const int image_height, const int frame_width,
                               const int frame_height, cvtdl_face_t *meta) {
  std::vector<cvtdl_face_info_t> vec_bbox;
  std::vector<cvtdl_face_info_t> vec_bbox_nms;

  float *score_blob = getOutputRawPtr<float>(NAME_SCORE);

  std::string bbox_str = NAME_BBOX;
  float *bbox_blob = getOutputRawPtr<float>(NAME_BBOX);

  for (size_t i = 0; i < m_all_anchors.size(); i++) {
    cvtdl_face_info_t box;

    float conf = score_blob[i];
    if (conf <= m_model_threshold) {
      continue;
    }
    box.bbox.score = conf;
    float dx = bbox_blob[0 + i * 4];
    float dy = bbox_blob[1 + i * 4];
    float dw = bbox_blob[2 + i * 4];
    float dh = bbox_blob[3 + i * 4];
    float regress[4] = {dx, dy, dw, dh};
    bbox_pred(m_all_anchors[i], regress, {0.1, 0.1, 0.2, 0.2}, box.bbox);

    vec_bbox.push_back(box);
  }
  // DO nms on output result
  vec_bbox_nms.clear();
  NonMaximumSuppression(vec_bbox, vec_bbox_nms, 0.5, 'u');

  // Init face meta
  meta->width = image_width;
  meta->height = image_height;
  meta->rescale_type = m_vpss_config[0].rescale_type;
  if (vec_bbox_nms.size() == 0) {
    meta->size = vec_bbox_nms.size();
    meta->info = NULL;
    return;
  }
  CVI_TDL_MemAllocInit(vec_bbox_nms.size(), 0, meta);
  if (hasSkippedVpssPreprocess()) {
    for (uint32_t i = 0; i < meta->size; ++i) {
      clip_boxes(image_width, image_height, vec_bbox_nms[i].bbox);
      meta->info[i].bbox.x1 = vec_bbox_nms[i].bbox.x1;
      meta->info[i].bbox.x2 = vec_bbox_nms[i].bbox.x2;
      meta->info[i].bbox.y1 = vec_bbox_nms[i].bbox.y1;
      meta->info[i].bbox.y2 = vec_bbox_nms[i].bbox.y2;
      meta->info[i].bbox.score = vec_bbox_nms[i].bbox.score;
    }
  } else {
    // Recover coordinate if internal vpss engine is used.
    meta->width = frame_width;
    meta->height = frame_height;
    for (uint32_t i = 0; i < meta->size; ++i) {
      float ratio = 0;
      clip_boxes(image_width, image_height, vec_bbox_nms[i].bbox);
      cvtdl_bbox_t bbox = box_rescale_rb(frame_width, frame_height, image_width, image_height,
                                         vec_bbox_nms[i].bbox, &ratio);
      meta->info[i].bbox.x1 = bbox.x1;
      meta->info[i].bbox.x2 = bbox.x2;
      meta->info[i].bbox.y1 = bbox.y1;
      meta->info[i].bbox.y2 = bbox.y2;
      meta->info[i].bbox.score = bbox.score;
    }
  }
}

}  // namespace cvitdl
