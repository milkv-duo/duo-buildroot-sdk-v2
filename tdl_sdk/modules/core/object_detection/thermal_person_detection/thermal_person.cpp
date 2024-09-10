#include "thermal_person.hpp"
#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem.h"
#include "core/cvi_tdl_types_mem_internal.h"
#include "core/utils/vpss_helper.h"
#include "core_utils.hpp"
#include "cvi_sys.h"

#define NUM_CLASSES 1
#define NAME_OUTPUT "output_Transpose_dequant"

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
                                     float prob_threshold,
                                     std::vector<cvtdl_object_info_t> &objects) {
  const int num_anchors = grid_strides.size();

  for (int anchor_idx = 0; anchor_idx < num_anchors; anchor_idx++) {
    const int grid0 = grid_strides[anchor_idx].grid0;
    const int grid1 = grid_strides[anchor_idx].grid1;
    const int stride = grid_strides[anchor_idx].stride;

    const int basic_pos = anchor_idx * (NUM_CLASSES + 5);

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
    for (int class_idx = 0; class_idx < NUM_CLASSES; class_idx++) {
      float box_cls_score = feat_ptr[basic_pos + 5 + class_idx];
      float box_prob = box_objectness * box_cls_score;
      if (box_prob > prob_threshold) {
        cvtdl_object_info_t obj;
        obj.bbox.x1 = x0;
        obj.bbox.y1 = y0;
        obj.bbox.x2 = x0 + w;
        obj.bbox.y2 = y0 + h;
        obj.bbox.score = box_prob;
        obj.classes = class_idx;
        objects.push_back(obj);
      }
    }  // class loop
  }    // point anchor loop
}

ThermalPerson::ThermalPerson() {
  // default param
  for (int i = 0; i < 3; i++) {
    m_preprocess_param[0].mean[i] = 0;
    m_preprocess_param[0].factor[i] = 1;
  }

  m_preprocess_param[0].format = PIXEL_FORMAT_BGR_888_PLANAR;
  alg_param_.cls = NUM_CLASSES;
  m_model_nms_threshold = 0.55;
}

ThermalPerson::~ThermalPerson() {}

int ThermalPerson::inference(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_object_t *obj) {
  std::vector<VIDEO_FRAME_INFO_S *> frames = {srcFrame};
  int ret = run(frames);
  if (ret != CVI_TDL_SUCCESS) {
    return ret;
  }

  CVI_SHAPE shape = getInputShape(0);
  outputParser(shape.dim[3], shape.dim[2], srcFrame->stVFrame.u32Width,
               srcFrame->stVFrame.u32Height, obj);
  return CVI_TDL_SUCCESS;
}

void ThermalPerson::outputParser(const int image_width, const int image_height,
                                 const int frame_width, const int frame_height,
                                 cvtdl_object_t *obj) {
  float *output_blob = getOutputRawPtr<float>(NAME_OUTPUT);

  std::vector<int> strides = {8, 16, 32};
  std::vector<GridAndStride> grid_strides;
  generate_grids_and_stride(image_width, image_height, strides, grid_strides);

  std::vector<cvtdl_object_info_t> vec_bbox;
  generate_yolox_proposals(grid_strides, output_blob, m_model_threshold, vec_bbox);

  // DO nms on output result
  std::vector<cvtdl_object_info_t> vec_bbox_nms;
  vec_bbox_nms.clear();
  NonMaximumSuppression(vec_bbox, vec_bbox_nms, m_model_nms_threshold, 'u');

  // fill obj
  CVI_TDL_MemAllocInit(vec_bbox_nms.size(), obj);
  obj->width = image_width;
  obj->height = image_height;
  obj->rescale_type = m_vpss_config[0].rescale_type;

  memset(obj->info, 0, sizeof(cvtdl_object_info_t) * obj->size);
  for (uint32_t i = 0; i < obj->size; ++i) {
    obj->info[i].bbox.x1 = vec_bbox_nms[i].bbox.x1;
    obj->info[i].bbox.y1 = vec_bbox_nms[i].bbox.y1;
    obj->info[i].bbox.x2 = vec_bbox_nms[i].bbox.x2;
    obj->info[i].bbox.y2 = vec_bbox_nms[i].bbox.y2;
    obj->info[i].bbox.score = vec_bbox_nms[i].bbox.score;
    obj->info[i].classes = vec_bbox_nms[i].classes;
    strncpy(obj->info[i].name, "person", sizeof(obj->info[i].name));
  }
  if (!hasSkippedVpssPreprocess()) {
    for (uint32_t i = 0; i < obj->size; ++i) {
      obj->info[i].bbox = box_rescale(frame_width, frame_height, obj->width, obj->height,
                                      obj->info[i].bbox, meta_rescale_type_e::RESCALE_CENTER);
      /*
      printf("RESCALE YOLOX: %s (%d): %*.2lf %*.2lf %*.2lf %*.2lf, score=%*.5lf\n",
             obj->info[i].name,
             obj->info[i].classes,
             7, obj->info[i].bbox.x1,
             7, obj->info[i].bbox.y1,
             7, obj->info[i].bbox.x2,
             7, obj->info[i].bbox.y2,
             7, obj->info[i].bbox.score);
      */
    }
    obj->width = frame_width;
    obj->height = frame_height;
  }
}

}  // namespace cvitdl