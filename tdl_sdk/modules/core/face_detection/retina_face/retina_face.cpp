#include "retina_face.hpp"
#include "retina_face_utils.hpp"

#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem.h"
#include "core/cvi_tdl_types_mem_internal.h"

#define NAME_BBOX "face_rpn_bbox_pred_"
#define NAME_SCORE "face_rpn_cls_prob_reshape_"
#define NAME_LANDMARK "face_rpn_landmark_pred_"
#define FACE_POINTS_SIZE 5

#define MEAN_R 123
#define MEAN_G 117
#define MEAN_B 104

#ifdef CV186X
#define suffix_info "_f32"
#else
#define suffix_info "_dequant"
#endif

namespace cvitdl {

RetinaFace::RetinaFace(PROCESS process) : process_(process) {
  std::vector<float> mean = {MEAN_R, MEAN_G, MEAN_B};
  for (int i = 0; i < 3; i++) {
    m_preprocess_param[0].factor[i] = 1;
    if (this->process_ == PYTORCH) {
      m_preprocess_param[0].mean[i] = mean[i];
    }
  }
  if (this->process_ == PYTORCH) {
    m_preprocess_param[0].format = PIXEL_FORMAT_BGR_888_PLANAR;
  }
}

int RetinaFace::onModelOpened() {
  std::vector<anchor_cfg> cfg;
  anchor_cfg tmp;

  if (this->process_ == CAFFE) {
    m_feat_stride_fpn = {32, 16, 8};

    tmp.SCALES = {32, 16};
    tmp.BASE_SIZE = 16;
    tmp.RATIOS = {1.0};
    tmp.ALLOWED_BORDER = 9999;
    tmp.STRIDE = 32;
    cfg.push_back(tmp);

    tmp.SCALES = {8, 4};
    tmp.BASE_SIZE = 16;
    tmp.RATIOS = {1.0};
    tmp.ALLOWED_BORDER = 9999;
    tmp.STRIDE = 16;
    cfg.push_back(tmp);

    tmp.SCALES = {2, 1};
    tmp.BASE_SIZE = 16;
    tmp.RATIOS = {1.0};
    tmp.ALLOWED_BORDER = 9999;
    tmp.STRIDE = 8;
    cfg.push_back(tmp);
  }

  if (this->process_ == PYTORCH) {
    m_feat_stride_fpn = {8, 16, 32};

    tmp.SCALES = {1, 2};
    tmp.BASE_SIZE = 16;
    tmp.RATIOS = {1.0};
    tmp.ALLOWED_BORDER = 9999;
    tmp.STRIDE = 8;
    cfg.push_back(tmp);

    tmp.SCALES = {4, 8};
    tmp.BASE_SIZE = 16;
    tmp.RATIOS = {1.0};
    tmp.ALLOWED_BORDER = 9999;
    tmp.STRIDE = 16;
    cfg.push_back(tmp);

    tmp.SCALES = {16, 32};
    tmp.BASE_SIZE = 16;
    tmp.RATIOS = {1.0};
    tmp.ALLOWED_BORDER = 9999;
    tmp.STRIDE = 32;
    cfg.push_back(tmp);
  }

  std::vector<std::vector<anchor_box>> anchors_fpn =
      generate_anchors_fpn(false, cfg, this->process_);
  std::map<std::string, std::vector<anchor_box>> anchors_fpn_map;
  for (size_t i = 0; i < anchors_fpn.size(); i++) {
    std::string key = "stride" + std::to_string(m_feat_stride_fpn[i]) + suffix_info;
    anchors_fpn_map[key] = anchors_fpn[i];
    m_num_anchors[key] = anchors_fpn[i].size();
  }

  for (size_t i = 0; i < m_feat_stride_fpn.size(); i++) {
    int stride = m_feat_stride_fpn[i];

    std::string key = "stride" + std::to_string(m_feat_stride_fpn[i]) + suffix_info;
    std::string bbox_str = NAME_BBOX + key;
    CVI_SHAPE bbox_shape = getOutputShape(bbox_str.c_str());

    m_anchors[bbox_str] =
        anchors_plane(bbox_shape.dim[2], bbox_shape.dim[3], stride, anchors_fpn_map[key]);
  }
  return CVI_TDL_SUCCESS;
}

int RetinaFace::inference(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_face_t *meta) {
  std::vector<VIDEO_FRAME_INFO_S *> frames;
  CVI_SHAPE shape = getInputShape(0);
  for (uint32_t b = 0; b < (uint32_t)shape.dim[0]; b++) {
    frames.push_back(&srcFrame[b]);
  }
  int ret = run(frames);
  if (ret != CVI_TDL_SUCCESS) {
    return ret;
  }

  int image_width = shape.dim[3];
  int image_height = shape.dim[2];
  outputParser(image_width, image_height, srcFrame->stVFrame.u32Width, srcFrame->stVFrame.u32Height,
               meta);
  model_timer_.TicToc("post");
  return CVI_TDL_SUCCESS;
}

void RetinaFace::outputParser(int image_width, int image_height, int frame_width, int frame_height,
                              cvtdl_face_t *meta) {
  CVI_SHAPE input_shape = getInputShape(0);
  for (uint32_t b = 0; b < (uint32_t)input_shape.dim[0]; b++) {
    std::vector<cvtdl_face_info_t> vec_bbox;
    std::vector<cvtdl_face_info_t> vec_bbox_nms;
    for (size_t i = 0; i < m_feat_stride_fpn.size(); i++) {
      std::string key = "stride" + std::to_string(m_feat_stride_fpn[i]) + suffix_info;

      std::string score_str = NAME_SCORE + key;
      CVI_SHAPE score_shape = getOutputShape(score_str.c_str());
      size_t score_size = score_shape.dim[1] * score_shape.dim[2] * score_shape.dim[3];
      float *score_blob = getOutputRawPtr<float>(score_str.c_str()) + (b * score_size);

      bool add_hardhat_score = (score_shape.dim[1] == 6);
      float *hardhat_score_blob = 0;
      float *nohardhat_score_blob = 0;

      if (add_hardhat_score) {
        // let score as non face, face conf would be 1 - score
        // score_blob = score_blob;
        hardhat_score_blob = score_blob + score_size / 3 * 2;
        nohardhat_score_blob = score_blob + score_size / 3;
      } else {
        score_blob += score_size / 2;
      }

      std::string bbox_str = NAME_BBOX + key;
      CVI_SHAPE blob_shape = getOutputShape(bbox_str.c_str());
      size_t blob_size = blob_shape.dim[1] * blob_shape.dim[2] * blob_shape.dim[3];
      float *bbox_blob = getOutputRawPtr<float>(bbox_str.c_str()) + (b * blob_size);

      size_t output_number = getNumOutputTensor();
      bool add_landmark_process = (output_number == 9);

      float *landmark_blob = 0;
      if (add_landmark_process) {
        std::string landmark_str = NAME_LANDMARK + key;
        CVI_SHAPE landmark_shape = getOutputShape(landmark_str.c_str());
        size_t landmark_size =
            landmark_shape.dim[1] * landmark_shape.dim[2] * landmark_shape.dim[3];
        landmark_blob = getOutputRawPtr<float>(landmark_str.c_str()) + (b * landmark_size);
      };
      int width = blob_shape.dim[3];
      int height = blob_shape.dim[2];
      size_t count = width * height;
      size_t num_anchor = m_num_anchors[key];

      std::vector<anchor_box> anchors = m_anchors[bbox_str];
      for (size_t num = 0; num < num_anchor; num++) {
        for (size_t j = 0; j < count; j++) {
          float conf = -1;
          float hardhat_score = -1;
          if (add_hardhat_score) {
            hardhat_score = hardhat_score_blob[j + count * num];
            conf = hardhat_score + nohardhat_score_blob[j + count * num];
            conf = std::min(float(conf), float(1));
          } else {
            conf = score_blob[j + count * num];
          }
          if (conf <= m_model_threshold) {
            continue;
          }
          cvtdl_face_info_t box;
          memset(&box, 0, sizeof(box));
          box.pts.size = 5;
          box.pts.x = (float *)malloc(sizeof(float) * box.pts.size);
          box.pts.y = (float *)malloc(sizeof(float) * box.pts.size);
          box.bbox.score = conf;
          box.hardhat_score = hardhat_score;

          float dx = bbox_blob[j + count * (0 + num * 4)];
          float dy = bbox_blob[j + count * (1 + num * 4)];
          float dw = bbox_blob[j + count * (2 + num * 4)];
          float dh = bbox_blob[j + count * (3 + num * 4)];
          float regress[4] = {dx, dy, dw, dh};
          bbox_pred(anchors[j + count * num], regress, box.bbox, this->process_);

          if (add_landmark_process) {
            for (size_t k = 0; k < box.pts.size; k++) {
              box.pts.x[k] = landmark_blob[j + count * (num * 10 + k * 2)];
              box.pts.y[k] = landmark_blob[j + count * (num * 10 + k * 2 + 1)];
            }
            landmark_pred(anchors[j + count * num], box.pts);
          }
          vec_bbox.push_back(box);
        }
      }
    }
    // DO nms on output result
    vec_bbox_nms.clear();
    NonMaximumSuppression(vec_bbox, vec_bbox_nms, 0.4, 'u');
    // Init face meta
    cvtdl_face_t *facemeta = &meta[b];
    facemeta->width = image_width;
    facemeta->height = image_height;
    if (vec_bbox_nms.size() == 0) {
      facemeta->size = vec_bbox_nms.size();
      facemeta->info = NULL;
      return;
    }
    CVI_TDL_MemAllocInit(vec_bbox_nms.size(), FACE_POINTS_SIZE, facemeta);
    if (hasSkippedVpssPreprocess()) {
      for (uint32_t i = 0; i < facemeta->size; ++i) {
        clip_boxes(image_width, image_height, vec_bbox_nms[i].bbox);
        facemeta->info[i].bbox.x1 = vec_bbox_nms[i].bbox.x1;
        facemeta->info[i].bbox.x2 = vec_bbox_nms[i].bbox.x2;
        facemeta->info[i].bbox.y1 = vec_bbox_nms[i].bbox.y1;
        facemeta->info[i].bbox.y2 = vec_bbox_nms[i].bbox.y2;
        facemeta->info[i].bbox.score = vec_bbox_nms[i].bbox.score;
        facemeta->info[i].hardhat_score = vec_bbox_nms[i].hardhat_score;

        for (int j = 0; j < FACE_POINTS_SIZE; ++j) {
          facemeta->info[i].pts.x[j] = vec_bbox_nms[i].pts.x[j];
          facemeta->info[i].pts.y[j] = vec_bbox_nms[i].pts.y[j];
        }
      }
    } else {
      // Recover coordinate if internal vpss engine is used.
      facemeta->width = frame_width;
      facemeta->height = frame_height;
      facemeta->rescale_type = m_vpss_config[0].rescale_type;
      for (uint32_t i = 0; i < facemeta->size; ++i) {
        clip_boxes(image_width, image_height, vec_bbox_nms[i].bbox);
        cvtdl_face_info_t info =
            info_rescale_c(image_width, image_height, frame_width, frame_height, vec_bbox_nms[i]);
        facemeta->info[i].bbox.x1 = info.bbox.x1;
        facemeta->info[i].bbox.x2 = info.bbox.x2;
        facemeta->info[i].bbox.y1 = info.bbox.y1;
        facemeta->info[i].bbox.y2 = info.bbox.y2;
        facemeta->info[i].bbox.score = info.bbox.score;
        facemeta->info[i].hardhat_score = info.hardhat_score;
        for (int j = 0; j < FACE_POINTS_SIZE; ++j) {
          facemeta->info[i].pts.x[j] = info.pts.x[j];
          facemeta->info[i].pts.y[j] = info.pts.y[j];
        }
        CVI_TDL_FreeCpp(&info);
      }
    }
    // Clear original bbox. bbox_nms does not need to free since it points to bbox.
    for (size_t i = 0; i < vec_bbox.size(); ++i) {
      CVI_TDL_FreeCpp(&vec_bbox[i].pts);
    }
  }
}

}  // namespace cvitdl
