
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <functional>
#include <istream>
#include <map>
#include <sstream>
#include <vector>
#include "core/cvi_tdl_types_mem_internal.h"
#include "core/utils/vpss_helper.h"
#include "cvi_tdl.h"
#include "cvi_tdl_media.h"
#include "sys_utils.hpp"

std::string g_model_root;

std::string split_line(std::string line, std::vector<std::vector<int>> &boxes) {
  int pos = line.find(",");
  std::string name = line.substr(0, pos);
  std::string res = line.substr(pos + 1);

  istringstream iss(res);
  std::string box;

  std::string val;
  while (getline(iss, box, ',')) {
    istringstream subiss(box);
    std::vector<int> vec_box;

    while (getline(subiss, val, ' ')) {
      vec_box.push_back(atoi(val.c_str()));
    }
    boxes.push_back(vec_box);
  }

  return name;
}

void clip_boxes(int width, int height, std::vector<int> &box) {
  if (box[0] < 0) {
    box[0] = 0;
  }
  if (box[1] < 0) {
    box[1] = 0;
  }
  if (box[2] > width - 1) {
    box[2] = width - 1;
  }
  if (box[3] > height - 1) {
    box[3] = height - 1;
  }
}

std::string run_image_liveness_detection(VIDEO_FRAME_INFO_S *p_frame, uint32_t image_width,
                                         uint32_t image_height, cvitdl_handle_t tdl_handle,
                                         std::vector<std::vector<int>> boxes) {
  static int model_init = 0;
  CVI_S32 ret;
  if (model_init == 0) {
    ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_IRLIVENESS, g_model_root.c_str());
    if (ret != CVI_SUCCESS) {
      std::cout << "open model failed:" << g_model_root << std::endl;
      return "";
    }
    model_init = 1;
  }

  cvtdl_face_t face_meta;
  memset(&face_meta, 0, sizeof(cvtdl_face_t));
  face_meta.width = image_width;
  face_meta.height = image_height;
  CVI_TDL_MemAllocInit(boxes.size(), 0, &face_meta);

  for (uint32_t i = 0; i < face_meta.size; ++i) {
    clip_boxes(image_width, image_height, boxes[i]);
    face_meta.info[i].bbox.x1 = boxes[i][0];
    face_meta.info[i].bbox.y1 = boxes[i][1];
    face_meta.info[i].bbox.x2 = boxes[i][2];
    face_meta.info[i].bbox.y2 = boxes[i][3];
  }

  ret = CVI_TDL_IrLiveness(tdl_handle, p_frame, &face_meta);
  if (ret != CVI_SUCCESS) {
    std::cout << "run IrLiveness failed:" << ret << std::endl;
  }

  std::stringstream ss;
  for (uint32_t i = 0; i < face_meta.size; i++) {
    ss << " " << face_meta.info[i].liveness_score << " ";
  }
  ss << "\n";
  CVI_TDL_Free(&face_meta);
  return ss.str();
}

int main(int argc, char *argv[]) {
  g_model_root = std::string(argv[1]);
  std::string image_root(argv[2]);
  std::string image_list(argv[3]);
  std::string dst_root(argv[4]);
  int starti = 0;
  if (argc > 5) starti = atoi(argv[5]);

  if (image_root.at(image_root.size() - 1) != '/') {
    image_root = image_root + std::string("/");
  }

  const CVI_S32 vpssgrp_width = 1920;
  const CVI_S32 vpssgrp_height = 1080;

  CVI_S32 ret = MMF_INIT_HELPER2(vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888, 3,
                                 vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888_PLANAR, 3);

  if (ret != CVI_SUCCESS) {
    printf("Init sys failed with %#x!\n", ret);
    return ret;
  }

  cvitdl_handle_t tdl_handle = NULL;
  ret = CVI_TDL_CreateHandle(&tdl_handle);
  if (ret != CVI_SUCCESS) {
    printf("Create tdl handle failed with %#x!\n", ret);
    return ret;
  }
  std::cout << "to read imagelist:" << image_list << std::endl;
  std::vector<std::string> image_files = read_file_lines(image_list);
  if (image_root.size() == 0) {
    std::cout << ", imageroot empty\n";
    return -1;
  }

  std::vector<std::vector<int>> boxes;

  std::string dstf = dst_root;

  FILE *fp = fopen(dstf.c_str(), "w");

  imgprocess_t img_handle;
  CVI_TDL_Create_ImageProcessor(&img_handle);

  for (int i = starti; i < image_files.size(); i++) {
    boxes.clear();
    std::string file_name = split_line(image_files[i], boxes);
    fwrite(file_name.c_str(), file_name.size(), 1, fp);

    if (boxes.size() == 0) {
      std::string endl("\n");
      fwrite(endl.c_str(), endl.size(), 1, fp);
      continue;
    }

    std::cout << "processing :" << i << "/" << image_files.size() << "\t" << file_name;
    std::string strf = image_root + file_name;

    VIDEO_FRAME_INFO_S fdFrame;
    ret = CVI_TDL_ReadImage(img_handle, strf.c_str(), &fdFrame, PIXEL_FORMAT_RGB_888_PLANAR);
    std::cout << " CVI_TDL_ReadImage done\t";
    if (ret != CVI_SUCCESS) {
      std::cout << "Convert to video frame failed with:" << ret << ",file:" << strf << std::endl;

      continue;
    }
    std::string str_res = run_image_liveness_detection(
        &fdFrame, fdFrame.stVFrame.u32Width, fdFrame.stVFrame.u32Height, tdl_handle, boxes);
    // std::cout << "process_funcs done\t";

    std::cout << "str_res.size():" << str_res.size() << std::endl;

    if (str_res.size() > 0) {
      fwrite(str_res.c_str(), str_res.size(), 1, fp);
    }

    // std::cout << "write results done\n";
    CVI_TDL_ReleaseImage(img_handle, &fdFrame);
  }
  fclose(fp);

  CVI_TDL_Destroy_ImageProcessor(img_handle);
  CVI_TDL_DestroyHandle(tdl_handle);
  return ret;
}
