#include "retinaface_yolox.hpp"
#include <Eigen/Eigen>
#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem.h"
#include "core/cvi_tdl_types_mem_internal.h"
#include "core_utils.hpp"
#include "cvi_sys.h"

#define R_SCALE (1 / (256.0 * 0.229))
#define G_SCALE (1 / (256.0 * 0.224))
#define B_SCALE (1 / (256.0 * 0.225))
#define R_MEAN (0.485 / 0.229)
#define G_MEAN (0.456 / 0.224)
#define B_MEAN (0.406 / 0.225)
#define NUM_CLASSES 2
#define NMS_THRESH 0.55
#define NAME_OUTPUT "output_Transpose_dequant"
#define FACE_POINTS_SIZE 5

namespace cvitdl {

struct GridAndStride {
  int grid0;
  int grid1;
  int stride;
};

static void generate_grids_and_stride(const int target_w, const int target_h,
                                      std::vector<int> &strides,
                                      std::vector<GridAndStride> &grid_strides) {
  for (auto stride : strides) {
    int num_grid_w = target_w / stride;
    int num_grid_h = target_h / stride;
    for (int g1 = 0; g1 < num_grid_h; g1++) {
      for (int g0 = 0; g0 < num_grid_w; g0++) {
        grid_strides.push_back((GridAndStride){g0, g1, stride});
      }
    }
  }
}

static void generate_yolox_proposals(std::vector<GridAndStride> grid_strides, const float *feat_ptr,
                                     float prob_threshold, std::vector<cvtdl_face_info_t> &faces) {
  const int num_anchors = grid_strides.size();

  for (int anchor_idx = 0; anchor_idx < num_anchors; anchor_idx++) {
    const int grid0 = grid_strides[anchor_idx].grid0;
    const int grid1 = grid_strides[anchor_idx].grid1;
    const int stride = grid_strides[anchor_idx].stride;

    const int basic_pos = anchor_idx * (NUM_CLASSES + 15);

    // yolox/models/yolo_head.py decode logic
    //  outputs[..., :2] = (outputs[..., :2] + grids) * strides
    //  outputs[..., 2:4] = torch.exp(outputs[..., 2:4]) * strides
    float x_center = (feat_ptr[basic_pos + 0] + grid0) * stride;
    float y_center = (feat_ptr[basic_pos + 1] + grid1) * stride;
    float w = exp(feat_ptr[basic_pos + 2]) * stride;
    float h = exp(feat_ptr[basic_pos + 3]) * stride;
    float x0 = x_center - w * 0.5f;
    float y0 = y_center - h * 0.5f;

    float box_objectness = feat_ptr[basic_pos + 4];
    // index basic_pos + 5 => normal face score
    // index basic_pos + 6 => mask face score
    float no_mask_score = sqrt(box_objectness * feat_ptr[basic_pos + 5]);
    float mask_score = sqrt(box_objectness * feat_ptr[basic_pos + 6]);
    float box_prob = std::max(no_mask_score, mask_score);
    if (box_prob > prob_threshold) {
      cvtdl_face_info_t face;

      memset(&face, 0, sizeof(face));
      face.pts.size = FACE_POINTS_SIZE;
      face.pts.x = (float *)malloc(sizeof(float) * face.pts.size);
      face.pts.y = (float *)malloc(sizeof(float) * face.pts.size);

      face.bbox.x1 = x0;
      face.bbox.y1 = y0;
      face.bbox.x2 = x0 + w;
      face.bbox.y2 = y0 + h;
      face.bbox.score = box_prob;
      face.mask_score = mask_score;
      for (size_t k = 0; k < face.pts.size; k++) {
        face.pts.x[k] = (feat_ptr[basic_pos + 2 * k + 7] + grid0) * stride;
        face.pts.y[k] = (feat_ptr[basic_pos + 2 * k + 8] + grid1) * stride;
      }
      faces.push_back(face);
    }  // class loop
  }    // point anchor loop
}

RetinafaceYolox::RetinafaceYolox() {
  m_preprocess_param[0].factor[0] = R_SCALE;
  m_preprocess_param[0].factor[1] = G_SCALE;
  m_preprocess_param[0].factor[2] = B_SCALE;
  m_preprocess_param[0].mean[0] = R_MEAN;
  m_preprocess_param[0].mean[1] = G_MEAN;
  m_preprocess_param[0].mean[2] = B_MEAN;
}

int RetinafaceYolox::inference(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_face_t *face_meta) {
  std::vector<VIDEO_FRAME_INFO_S *> frames = {srcFrame};
  int ret = run(frames);
  if (ret != CVI_TDL_SUCCESS) {
    return ret;
  }

  CVI_SHAPE shape = getInputShape(0);
  outputParser(shape.dim[3], shape.dim[2], srcFrame->stVFrame.u32Width,
               srcFrame->stVFrame.u32Height, face_meta);
  model_timer_.TicToc("post");
  return CVI_TDL_SUCCESS;
}

void RetinafaceYolox::outputParser(const int image_width, const int image_height,
                                   const int frame_width, const int frame_height,
                                   cvtdl_face_t *face_meta) {
  float *output_blob = getOutputRawPtr<float>(NAME_OUTPUT);

  std::vector<int> strides = {8, 16, 32};
  std::vector<GridAndStride> grid_strides;
  generate_grids_and_stride(image_width, image_height, strides, grid_strides);

  std::vector<cvtdl_face_info_t> vec_face;
  generate_yolox_proposals(grid_strides, output_blob, m_model_threshold, vec_face);

  // DO nms on output result
  std::vector<cvtdl_face_info_t> vec_face_nms;
  vec_face_nms.clear();
  NonMaximumSuppression(vec_face, vec_face_nms, NMS_THRESH, 'u');

  // fill obj
  face_meta->size = vec_face_nms.size();
  face_meta->width = image_width;
  face_meta->height = image_height;
  if (vec_face_nms.size() == 0) {
    face_meta->info = NULL;
    return;
  }
  face_meta->info = (cvtdl_face_info_t *)malloc(sizeof(cvtdl_face_info_t) * face_meta->size);
  face_meta->rescale_type = m_vpss_config[0].rescale_type;

  memset(face_meta->info, 0, sizeof(cvtdl_face_info_t) * face_meta->size);
  CVI_TDL_MemAllocInit(vec_face_nms.size(), FACE_POINTS_SIZE, face_meta);
  for (uint32_t i = 0; i < face_meta->size; ++i) {
    face_meta->info[i].bbox.x1 = vec_face_nms[i].bbox.x1;
    face_meta->info[i].bbox.y1 = vec_face_nms[i].bbox.y1;
    face_meta->info[i].bbox.x2 = vec_face_nms[i].bbox.x2;
    face_meta->info[i].bbox.y2 = vec_face_nms[i].bbox.y2;
    face_meta->info[i].bbox.score = vec_face_nms[i].bbox.score;
    face_meta->info[i].mask_score = vec_face_nms[i].mask_score;
    for (int j = 0; j < FACE_POINTS_SIZE; ++j) {
      face_meta->info[i].pts.x[j] = vec_face_nms[i].pts.x[j];
      face_meta->info[i].pts.y[j] = vec_face_nms[i].pts.y[j];
    }
  }
  if (!hasSkippedVpssPreprocess()) {
    // Recover coordinate if internal vpss engine is used.
    face_meta->width = frame_width;
    face_meta->height = frame_height;
    face_meta->rescale_type = m_vpss_config[0].rescale_type;
    for (uint32_t i = 0; i < face_meta->size; ++i) {
      clip_boxes(image_width, image_height, vec_face_nms[i].bbox);
      cvtdl_face_info_t info =
          info_rescale_c(image_width, image_height, frame_width, frame_height, vec_face_nms[i]);
      face_meta->info[i].bbox.x1 = info.bbox.x1;
      face_meta->info[i].bbox.x2 = info.bbox.x2;
      face_meta->info[i].bbox.y1 = info.bbox.y1;
      face_meta->info[i].bbox.y2 = info.bbox.y2;
      face_meta->info[i].bbox.score = info.bbox.score;
      face_meta->info[i].hardhat_score = info.hardhat_score;
      for (int j = 0; j < FACE_POINTS_SIZE; ++j) {
        face_meta->info[i].pts.x[j] = info.pts.x[j];
        face_meta->info[i].pts.y[j] = info.pts.y[j];
      }
      CVI_TDL_FreeCpp(&info);
    }
  }
  for (size_t i = 0; i < vec_face.size(); ++i) {
    CVI_TDL_FreeCpp(&vec_face[i].pts);
  }
}

}  // namespace cvitdl
