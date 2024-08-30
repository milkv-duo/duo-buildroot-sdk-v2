#include <algorithm>
#include <cmath>
#include <iterator>

#include "coco_utils.hpp"
#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem.h"
#include "core/cvi_tdl_types_mem_internal.h"
#include "core_utils.hpp"
#include "cvi_sys.h"
#include "object_utils.hpp"
#include "ppyoloe.hpp"

namespace cvitdl {

static void convert_det_struct(const Detections &dets, cvtdl_object_t *out, int im_height,
                               int im_width, meta_rescale_type_e type) {
  CVI_TDL_MemAllocInit(dets.size(), out);
  out->height = im_height;
  out->width = im_width;
  out->rescale_type = type;

  memset(out->info, 0, sizeof(cvtdl_object_info_t) * out->size);
  for (uint32_t i = 0; i < out->size; ++i) {
    clip_bbox(im_height, im_width, dets[i]);
    out->info[i].bbox.x1 = dets[i]->x1;
    out->info[i].bbox.y1 = dets[i]->y1;
    out->info[i].bbox.x2 = dets[i]->x2;
    out->info[i].bbox.y2 = dets[i]->y2;
    out->info[i].bbox.score = dets[i]->score;
    out->info[i].classes = dets[i]->label;
    const std::string &classname = coco_utils::class_names_91[out->info[i].classes];
    strncpy(out->info[i].name, classname.c_str(), sizeof(out->info[i].name));
  }
}

float yoloe_sigmoid(float x) { return 1.0 / (1.0 + exp(-x)); }

template <typename T>
void decode_yoloe_bbox(T *ptr, float qscale, int basic_pos, int grid0, int grid1, int stride,
                       int label, float box_prob, PtrDectRect &det) {
  float x1 = (-ptr[basic_pos + 0] * qscale + grid0 + 0.5) * stride;
  float y1 = (-ptr[basic_pos + 1] * qscale + grid1 + 0.5) * stride;
  float x2 = (ptr[basic_pos + 2] * qscale + grid0 + 0.5) * stride;
  float y2 = (ptr[basic_pos + 3] * qscale + grid1 + 0.5) * stride;
  det->label = label;
  det->score = box_prob;
  det->x1 = x1;
  det->y1 = y1;
  det->x2 = x2;
  det->y2 = y2;
}

template <typename T>
int yoloe_argmax(T *ptr, int basic_pos, int cls_len) {
  int max_idx = 0;
  for (int i = 0; i < cls_len; i++) {
    if (ptr[i + basic_pos] > ptr[max_idx + basic_pos]) {
      max_idx = i;
    }
  }
  return max_idx;
}

void PPYoloE::generate_ppyoloe_proposals(Detections &detections, int frame_width,
                                         int frame_height) {
  CVI_SHAPE shape = getInputShape(0);
  int target_w = shape.dim[3];
  int target_h = shape.dim[2];

  for (auto stride : strides_) {
    TensorInfo oinfo_box = getOutputTensorInfo(box_out_names_[stride]);
    int num_per_pixel_box = oinfo_box.tensor_size / oinfo_box.tensor_elem;
    float qscale_box = num_per_pixel_box == 1 ? oinfo_box.qscale : 1;
    int8_t *ptr_int8_box = static_cast<int8_t *>(oinfo_box.raw_pointer);
    float *ptr_float_box = static_cast<float *>(oinfo_box.raw_pointer);

    TensorInfo oinfo_cls = getOutputTensorInfo(class_out_names_[stride]);
    int num_per_pixel_cls = oinfo_cls.tensor_size / oinfo_cls.tensor_elem;
    float qscale_cls = num_per_pixel_cls == 1 ? oinfo_cls.qscale : 1;
    int8_t *ptr_int8_cls = static_cast<int8_t *>(oinfo_cls.raw_pointer);
    float *ptr_float_cls = static_cast<float *>(oinfo_cls.raw_pointer);

    int num_grid_w = target_w / stride;
    int num_grid_h = target_h / stride;

    int basic_pos_cls = 0;
    int basic_pos_box = 0;
    for (int g1 = 0; g1 < num_grid_h; g1++) {
      for (int g0 = 0; g0 < num_grid_w; g0++) {
        float class_score = 0.0f;
        int label = 0;
        if (num_per_pixel_cls == 1) {
          label = yoloe_argmax<int8_t>(ptr_int8_cls, basic_pos_cls, alg_param_.cls);
          class_score = ptr_int8_cls[label + basic_pos_cls] * qscale_cls;
        } else {
          label = yoloe_argmax<float>(ptr_float_cls, basic_pos_cls, alg_param_.cls);
          class_score = ptr_float_cls[label + basic_pos_cls];
        }

        class_score = yoloe_sigmoid(class_score);
        if (class_score < m_model_threshold) {
          basic_pos_cls += alg_param_.cls;
          basic_pos_box += 4;
          continue;
        }

        PtrDectRect det = std::make_shared<object_detect_rect_t>();
        if (num_per_pixel_box == 1) {
          decode_yoloe_bbox<int8_t>(ptr_int8_box, qscale_box, basic_pos_box, g0, g1, stride, label,
                                    class_score, det);
        } else {
          decode_yoloe_bbox<float>(ptr_float_box, qscale_box, basic_pos_box, g0, g1, stride, label,
                                   class_score, det);
        }
        basic_pos_cls += alg_param_.cls;
        basic_pos_box += 4;
        clip_bbox(frame_width, frame_height, det);
        detections.push_back(det);
      }
    }
  }
}

PPYoloE::PPYoloE() {
  // default param
  float mean[3] = {123.675, 116.28, 103.52};
  float std[3] = {58.395, 57.12, 57.375};

  for (int i = 0; i < 3; i++) {
    m_preprocess_param[0].mean[i] = mean[i] / std[i];
    m_preprocess_param[0].factor[i] = 1.0 / std[i];
  }

  m_preprocess_param[0].format = PIXEL_FORMAT_RGB_888_PLANAR;
  alg_param_.cls = 80;
}

int PPYoloE::onModelOpened() {
  CVI_SHAPE input_shape = getInputShape(0);

  int input_h = input_shape.dim[2];

  strides_.clear();
  std::unordered_map<std::string, int> setting_out_names_index_map;
  if (!setting_out_names_.empty()) {
    for (size_t i = 0; i < setting_out_names_.size(); i++) {
      setting_out_names_index_map[setting_out_names_[i]] = i;
    }
  }
  for (size_t j = 0; j < getNumOutputTensor(); j++) {
    TensorInfo oinfo = getOutputTensorInfo(j);
    CVI_SHAPE output_shape = oinfo.shape;
    LOGI("output layer: %s output shape: %d %d %d %d\n", oinfo.tensor_name.c_str(),
         output_shape.dim[0], output_shape.dim[1], output_shape.dim[2], output_shape.dim[3]);
    int feat_h = output_shape.dim[1];
    uint32_t channel = output_shape.dim[3];
    int stride_h = input_h / feat_h;

    if (setting_out_names_.empty() || setting_out_names_.size() != getNumOutputTensor()) {
      if (j < 3) {
        box_out_names_[stride_h] = oinfo.tensor_name;
        strides_.push_back(stride_h);
      } else {
        class_out_names_[stride_h] = oinfo.tensor_name;
      }
    } else {
      if (setting_out_names_index_map[oinfo.tensor_name] < 3) {
        box_out_names_[stride_h] = oinfo.tensor_name;
        strides_.push_back(stride_h);
      } else {
        class_out_names_[stride_h] = oinfo.tensor_name;
      }
    }
  }

  for (auto stride : strides_) {
    if (class_out_names_.count(stride) == 0 || box_out_names_.count(stride) == 0) {
      return CVI_TDL_FAILURE;
    }
  }

  return CVI_TDL_SUCCESS;
}

PPYoloE::~PPYoloE() {}

int PPYoloE::inference(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_object_t *obj_meta) {
  std::vector<VIDEO_FRAME_INFO_S *> frames = {srcFrame};
  int ret = run(frames);
  if (ret != CVI_TDL_SUCCESS) {
    return ret;
  }

  CVI_SHAPE shape = getInputShape(0);
  outputParser(shape.dim[3], shape.dim[2], srcFrame->stVFrame.u32Width,
               srcFrame->stVFrame.u32Height, obj_meta);
  model_timer_.TicToc("post");
  return CVI_TDL_SUCCESS;
}

void PPYoloE::outputParser(const int image_width, const int image_height, const int frame_width,
                           const int frame_height, cvtdl_object_t *obj_meta) {
  Detections vec_obj;
  generate_ppyoloe_proposals(vec_obj, image_width, image_height);

  // Do nms on output result
  Detections final_dets = nms_multi_class(vec_obj, m_model_nms_threshold);

  CVI_SHAPE shape = getInputShape(0);

  convert_det_struct(final_dets, obj_meta, shape.dim[2], shape.dim[3],
                     m_vpss_config[0].rescale_type);

  if (!hasSkippedVpssPreprocess()) {
    for (uint32_t i = 0; i < obj_meta->size; ++i) {
      obj_meta->info[i].bbox =
          box_rescale(frame_width, frame_height, obj_meta->width, obj_meta->height,
                      obj_meta->info[i].bbox, obj_meta->rescale_type);
    }
    obj_meta->width = frame_width;
    obj_meta->height = frame_height;
  }
}

}  // namespace cvitdl
