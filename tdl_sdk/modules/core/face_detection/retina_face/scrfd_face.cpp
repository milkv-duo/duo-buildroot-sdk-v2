#include "scrfd_face.hpp"
#include "retina_face_utils.hpp"

#include <math.h>
#include <iostream>
#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem.h"
#include "core/cvi_tdl_types_mem_internal.h"
#include "object_utils.hpp"
#define FACE_POINTS_SIZE 5

namespace cvitdl {

ScrFDFace::ScrFDFace() {
  std::vector<float> means = {127.5, 127.5, 127.5};
  std::vector<float> scales = {1.0 / 128, 1.0 / 128, 1.0 / 128};

  for (int i = 0; i < 3; i++) {
    m_preprocess_param[0].factor[i] = scales[i];
    m_preprocess_param[0].mean[i] = means[i] * scales[i];
  }
  m_preprocess_param[0].format = PIXEL_FORMAT_BGR_888_PLANAR;
}

int ScrFDFace::onModelOpened() {
  std::vector<anchor_cfg> cfg;
  anchor_cfg tmp;

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
  // std::cout << "start to parse node\n";
  std::map<std::string, std::vector<anchor_box>> anchors_fpn_map;
  CVI_SHAPE input_shape = getInputShape(0);
  for (size_t i = 0; i < cfg.size(); i++) {
    std::vector<std::vector<float>> base_anchors =
        generate_mmdet_base_anchors(cfg[i].BASE_SIZE, 0, cfg[i].RATIOS, cfg[i].SCALES);
    int stride = cfg[i].STRIDE;
    int input_w = input_shape.dim[3];
    int input_h = input_shape.dim[2];
    int feat_w = ceil(input_w / float(stride));
    int feat_h = ceil(input_h / float(stride));
    fpn_anchors_[stride] = generate_mmdet_grid_anchors(feat_w, feat_h, stride, base_anchors);

    int num_feat_branch = 0;
    int num_anchors = int(cfg[i].SCALES.size() * cfg[i].RATIOS.size());
    fpn_grid_anchor_num_[stride] = num_anchors;

    for (size_t j = 0; j < getNumOutputTensor(); j++) {
      CVI_SHAPE oj = getOutputShape(j);
      // std::cout << "stride:" << stride << ",w:" << feat_w << ",feath:" << feat_h
      //           << ",node:" << getOutputTensorInfo(j).tensor_name << ",sw:" << oj.dim[3]
      //           << ",sh:" << oj.dim[2] << ",c:" << oj.dim[1] << std::endl;
      if (oj.dim[2] == feat_h && oj.dim[3] == feat_w) {
        // std::cout << "fpnnode,stride:" << stride << ",w:" << feat_w << ",feath:" << feat_h
        //           << std::endl;
        if (oj.dim[1] == num_anchors * 1) {
          fpn_out_nodes_[stride]["score"] = getOutputTensorInfo(j).tensor_name;
          num_feat_branch++;
        } else if (oj.dim[1] == num_anchors * 4) {
          fpn_out_nodes_[stride]["bbox"] = getOutputTensorInfo(j).tensor_name;
          num_feat_branch++;
        } else if (oj.dim[1] == num_anchors * 10) {
          fpn_out_nodes_[stride]["landmark"] = getOutputTensorInfo(j).tensor_name;
          num_feat_branch++;
        }
      }
    }
    // std::cout << "numfeat:" << num_feat_branch << std::endl;
    for (auto &kv : fpn_out_nodes_[stride]) {
      std::cout << kv.first << ":" << kv.second << std::endl;
    }
    if (num_feat_branch != int(cfg.size())) {
      LOGE("output nodenum error,got:%d,expected:%d at branch:%d", num_feat_branch, int(cfg.size()),
           int(i));
    }
  }
  return CVI_TDL_SUCCESS;
}

int ScrFDFace::inference(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_face_t *meta) {
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
// static void print_dim(CVI_SHAPE sp, std::string strprefix) {
//   std::stringstream ss;
//   ss << "shape:";
//   for (int i = 0; i < 4; i++) {
//     ss << sp.dim[i] << ",";
//   }
//   std::cout << strprefix << "," << ss.str() << "\n";
// }
void ScrFDFace::outputParser(int image_width, int image_height, int frame_width, int frame_height,
                             cvtdl_face_t *meta) {
  CVI_SHAPE input_shape = getInputShape(0);
  for (uint32_t b = 0; b < (uint32_t)input_shape.dim[0]; b++) {
    std::vector<cvtdl_face_info_t> vec_bbox;
    std::vector<cvtdl_face_info_t> vec_bbox_nms;
    for (size_t i = 0; i < m_feat_stride_fpn.size(); i++) {
      int stride = m_feat_stride_fpn[i];
      std::string score_str = fpn_out_nodes_[stride]["score"];
      CVI_SHAPE score_shape = getOutputShape(score_str.c_str());
      // std::cout << "stride:" << m_feat_stride_fpn[i] << "\n";
      // print_dim(score_shape, "score");
      size_t score_size = score_shape.dim[1] * score_shape.dim[2] * score_shape.dim[3];
      float *score_blob = getOutputRawPtr<float>(score_str.c_str()) + (b * score_size);

      std::string bbox_str = fpn_out_nodes_[stride]["bbox"];
      CVI_SHAPE blob_shape = getOutputShape(bbox_str.c_str());
      // print_dim(blob_shape, "bbox");
      size_t blob_size = blob_shape.dim[1] * blob_shape.dim[2] * blob_shape.dim[3];
      float *bbox_blob = getOutputRawPtr<float>(bbox_str.c_str()) + (b * blob_size);

      std::string landmark_str = fpn_out_nodes_[stride]["landmark"];
      CVI_SHAPE landmark_shape = getOutputShape(landmark_str.c_str());
      // print_dim(blob_shape, "bbox");
      size_t landmark_size = landmark_shape.dim[1] * landmark_shape.dim[2] * landmark_shape.dim[3];
      float *landmark_blob = getOutputRawPtr<float>(landmark_str.c_str()) + (b * landmark_size);
      int width = blob_shape.dim[3];
      int height = blob_shape.dim[2];
      size_t count = width * height;
      size_t num_anchor = fpn_grid_anchor_num_[stride];
      // std::cout << "numanchor:" << num_anchor << ",count:" << count << "\n";
      std::vector<std::vector<float>> &fpn_grids = fpn_anchors_[stride];
      for (size_t num = 0; num < num_anchor; num++) {  // anchor index
        for (size_t j = 0; j < count; j++) {           // j:grid index
          float conf = score_blob[j + count * num];
          if (conf <= m_model_threshold) {
            continue;
          }
          std::vector<float> &grid = fpn_grids[j + count * num];
          float grid_cx = (grid[0] + grid[2]) / 2;
          float grid_cy = (grid[1] + grid[3]) / 2;

          cvtdl_face_info_t box;
          memset(&box, 0, sizeof(box));
          box.pts.size = 5;
          box.pts.x = (float *)malloc(sizeof(float) * box.pts.size);
          box.pts.y = (float *)malloc(sizeof(float) * box.pts.size);
          box.bbox.score = conf;
          box.hardhat_score = 0;

          // cv::Vec4f regress;
          // bbox_blob:b x (num_anchors*num_elem) x h x w
          box.bbox.x1 = grid_cx - bbox_blob[j + count * (0 + num * 4)] * stride;
          box.bbox.y1 = grid_cy - bbox_blob[j + count * (1 + num * 4)] * stride;
          box.bbox.x2 = grid_cx + bbox_blob[j + count * (2 + num * 4)] * stride;
          box.bbox.y2 = grid_cy + bbox_blob[j + count * (3 + num * 4)] * stride;

          for (size_t k = 0; k < box.pts.size; k++) {
            box.pts.x[k] = landmark_blob[j + count * (num * 10 + k * 2)] * stride + grid_cx;
            box.pts.y[k] = landmark_blob[j + count * (num * 10 + k * 2 + 1)] * stride + grid_cy;
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
        facemeta->info[i].pts.score = info.bbox.score;
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
