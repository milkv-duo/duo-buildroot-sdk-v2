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
#include "yolox.hpp"

namespace cvitdl {

static void convert_det_struct(const Detections &dets, cvtdl_object_t *out, int im_height,
                               int im_width, meta_rescale_type_e type) {
  CVI_TDL_MemAllocInit(dets.size(), out);
  out->height = im_height;
  out->width = im_width;
  out->rescale_type = type;

  memset(out->info, 0, sizeof(cvtdl_object_info_t) * out->size);
  for (uint32_t i = 0; i < out->size; ++i) {
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

float yolox_sigmoid(float x) { return 1.0 / (1.0 + exp(-x)); }

template <typename T>
void decode_yolox_bbox(T *ptr, float qscale, int basic_pos, int grid0, int grid1, int stride,
                       int label, float box_prob, PtrDectRect &det) {
  float x_center = (ptr[basic_pos + 0] * qscale + grid0) * stride;
  float y_center = (ptr[basic_pos + 1] * qscale + grid1) * stride;
  float w = std::exp(ptr[basic_pos + 2] * qscale) * stride;
  float h = std::exp(ptr[basic_pos + 3] * qscale) * stride;
  float x0 = x_center - w * 0.5f;
  float y0 = y_center - h * 0.5f;
  det->label = label;
  det->score = box_prob;
  det->x1 = x0;
  det->y1 = y0;
  det->x2 = x0 + w;
  det->y2 = y0 + h;
}

template <typename T>
int yolox_argmax(T *ptr, int basic_pos, int cls_len) {
  int max_idx = 0;
  for (int i = 0; i < cls_len; i++) {
    if (ptr[i + basic_pos] > ptr[max_idx + basic_pos]) {
      max_idx = i;
    }
  }
  return max_idx;
}

void YoloX::generate_yolox_proposals(Detections &detections) {
  CVI_SHAPE shape = getInputShape(0);
  int target_w = shape.dim[3];
  int target_h = shape.dim[2];

  for (auto stride : strides_) {
    TensorInfo oinfo_class = getOutputTensorInfo(class_out_names_[stride]);
    int num_per_pixel_class = oinfo_class.tensor_size / oinfo_class.tensor_elem;
    float qscale_class = num_per_pixel_class == 1 ? oinfo_class.qscale : 1;
    int8_t *ptr_int8_class = static_cast<int8_t *>(oinfo_class.raw_pointer);
    float *ptr_float_class = static_cast<float *>(oinfo_class.raw_pointer);

    TensorInfo oinfo_object = getOutputTensorInfo(object_out_names_[stride]);
    int num_per_pixel_object = oinfo_object.tensor_size / oinfo_object.tensor_elem;
    float qscale_object = num_per_pixel_object == 1 ? oinfo_object.qscale : 1;
    int8_t *ptr_int8_object = static_cast<int8_t *>(oinfo_object.raw_pointer);
    float *ptr_float_object = static_cast<float *>(oinfo_object.raw_pointer);

    TensorInfo oinfo_box = getOutputTensorInfo(box_out_names_[stride]);
    int num_per_pixel_box = oinfo_box.tensor_size / oinfo_box.tensor_elem;
    float qscale_box = num_per_pixel_box == 1 ? oinfo_box.qscale : 1;
    int8_t *ptr_int8_box = static_cast<int8_t *>(oinfo_box.raw_pointer);
    float *ptr_float_box = static_cast<float *>(oinfo_box.raw_pointer);

    int num_grid_w = target_w / stride;
    int num_grid_h = target_h / stride;

    int basic_pos_class = 0;
    int basic_pos_object = 0;
    int basic_pos_box = 0;

    for (int g1 = 0; g1 < num_grid_h; g1++) {
      for (int g0 = 0; g0 < num_grid_w; g0++) {
        float class_score = 0.0f;
        float box_objectness = 0.0f;
        int label = 0;

        // parse object conf
        if (num_per_pixel_class == 1) {
          box_objectness = ptr_int8_object[basic_pos_object] * qscale_object;
        } else {
          box_objectness = ptr_float_object[basic_pos_object];
        }

        // parse class score
        if (num_per_pixel_object == 1) {
          label = yolox_argmax<int8_t>(ptr_int8_class, basic_pos_class, alg_param_.cls);
          class_score = ptr_int8_class[basic_pos_class + label] * qscale_class;
        } else {
          label = yolox_argmax<float>(ptr_float_class, basic_pos_class, alg_param_.cls);
          class_score = ptr_float_class[basic_pos_class + label];
        }

        box_objectness = yolox_sigmoid(box_objectness);
        class_score = yolox_sigmoid(class_score);
        float box_prob = box_objectness * class_score;

        if (box_prob < m_model_threshold) {
          basic_pos_class += alg_param_.cls;
          basic_pos_box += 4;
          basic_pos_object += 1;
          continue;
        }

        // parse box point
        PtrDectRect det = std::make_shared<object_detect_rect_t>();
        if (num_per_pixel_box == 1) {
          decode_yolox_bbox<int8_t>(ptr_int8_box, qscale_box, basic_pos_box, g0, g1, stride, label,
                                    box_prob, det);
        } else {
          decode_yolox_bbox<float>(ptr_float_box, qscale_box, basic_pos_box, g0, g1, stride, label,
                                   box_prob, det);
        }

        clip_bbox(target_w, target_h, det);
        float box_width = det->x2 - det->x1;
        float box_height = det->y2 - det->y1;
        if (box_width > 1 && box_height > 1) {
          detections.push_back(det);
        }
        basic_pos_class += alg_param_.cls;
        basic_pos_box += 4;
        basic_pos_object += 1;
      }
    }
  }
}

YoloX::YoloX() {
  // default param
  for (int i = 0; i < 3; i++) {
    m_preprocess_param[0].factor[i] = 1.0;
    m_preprocess_param[0].mean[i] = 0.0;
  }
  m_preprocess_param[0].format = PIXEL_FORMAT_RGB_888_PLANAR;
  alg_param_.cls = 80;
}

int YoloX::onModelOpened() {
  CVI_SHAPE input_shape = getInputShape(0);

  int input_h = input_shape.dim[2];

  strides_.clear();
  for (size_t j = 0; j < getNumOutputTensor(); j++) {
    TensorInfo oinfo = getOutputTensorInfo(j);
    CVI_SHAPE output_shape = oinfo.shape;
    int feat_h = output_shape.dim[1];
    uint32_t channel = output_shape.dim[3];
    int stride_h = input_h / feat_h;

    LOGI("%s: (%d %d %d %d)\n", oinfo.tensor_name.c_str(), output_shape.dim[0], output_shape.dim[1],
         output_shape.dim[2], output_shape.dim[3]);

    if (channel == alg_param_.cls) {
      class_out_names_[stride_h] = oinfo.tensor_name;
      strides_.push_back(stride_h);
      LOGE("parse output name: %s, channel: %d, stride: %d\n", oinfo.tensor_name.c_str(), channel,
           stride_h);
    } else if (channel == 4) {
      box_out_names_[stride_h] = oinfo.tensor_name;
      LOGE("parse output name: %s, channel: %d, stride: %d\n", oinfo.tensor_name.c_str(), channel,
           stride_h);
    } else if (channel == 1) {
      object_out_names_[stride_h] = oinfo.tensor_name;
      LOGE("parse output name: %s, channel: %d, stride: %d\n", oinfo.tensor_name.c_str(), channel,
           stride_h);
    } else {
      LOGE("model channel num not match!\n");
      return CVI_TDL_FAILURE;
    }
  }

  for (size_t i = 0; i < strides_.size(); i++) {
    if (!class_out_names_.count(strides_[i]) || !box_out_names_.count(strides_[i]) ||
        !object_out_names_.count(strides_[i])) {
      return CVI_TDL_FAILURE;
    }
  }

  return CVI_TDL_SUCCESS;
}

YoloX::~YoloX() {}

int YoloX::inference(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_object_t *obj_meta) {
  std::vector<VIDEO_FRAME_INFO_S *> frames = {srcFrame};
  int ret = run(frames);
  if (ret != CVI_TDL_SUCCESS) {
    LOGE("YoloX run inference failed!\n");
    return ret;
  }

  CVI_SHAPE shape = getInputShape(0);
  outputParser(shape.dim[3], shape.dim[2], srcFrame->stVFrame.u32Width,
               srcFrame->stVFrame.u32Height, obj_meta);
  model_timer_.TicToc("post");
  return CVI_TDL_SUCCESS;
}

void YoloX::outputParser(const int image_width, const int image_height, const int frame_width,
                         const int frame_height, cvtdl_object_t *obj_meta) {
  Detections vec_obj;
  generate_yolox_proposals(vec_obj);

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
