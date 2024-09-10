#include <cvi_ive.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include "core/utils/vpss_helper.h"
#include "cvi_tdl.h"
#include "cvi_tdl_app/cvi_tdl_app.h"
#include "cvi_tdl_media.h"
#include "sample_comm.h"
#include "stb_image.h"
#include "stb_image_write.h"
#include "sys_utils.hpp"

#include <opencv2/opencv.hpp>

static const char *enumStr[] = {"NORMAL", "START", "COLLISION_WARNING", "DANGER"};
int g_count = 0;
std::vector<cv::Scalar> color = {cv::Scalar(51, 153, 255), cv::Scalar(0, 153, 76),
                                 cv::Scalar(255, 215, 0), cv::Scalar(255, 128, 0),
                                 cv::Scalar(0, 255, 0)};

void set_sample_mot_config(cvtdl_deepsort_config_t *ds_conf) {
  ds_conf->ktracker_conf.P_beta[2] = 0.01;
  ds_conf->ktracker_conf.P_beta[6] = 1e-5;

  // ds_conf.kfilter_conf.Q_beta[2] = 0.1;
  ds_conf->kfilter_conf.Q_beta[2] = 0.01;
  ds_conf->kfilter_conf.Q_beta[6] = 1e-5;
  ds_conf->kfilter_conf.R_beta[2] = 0.1;
}

cv::Scalar gen_random_color(uint64_t seed, int min) {
  float scale = (256. - (float)min) / 256.;
  srand((uint32_t)seed);

  int r = (int)((floor(((float)rand() / (RAND_MAX)) * 256.)) * scale) + min;
  int g = (int)((floor(((float)rand() / (RAND_MAX)) * 256.)) * scale) + min;
  int b = (int)((floor(((float)rand() / (RAND_MAX)) * 256.)) * scale) + min;

  return cv::Scalar(r, g, b);
}

void draw_adas(cvitdl_app_handle_t app_handle, VIDEO_FRAME_INFO_S *bg, std::string save_path) {
  bg->stVFrame.pu8VirAddr[0] =
      (CVI_U8 *)CVI_SYS_Mmap(bg->stVFrame.u64PhyAddr[0], bg->stVFrame.u32Length[0]);

  cv::Mat img_rgb(bg->stVFrame.u32Height, bg->stVFrame.u32Width, CV_8UC3,
                  bg->stVFrame.pu8VirAddr[0], bg->stVFrame.u32Stride[0]);

  cvtdl_object_t *obj_meta = &app_handle->adas_info->last_objects;
  cvtdl_tracker_t *track_meta = &app_handle->adas_info->last_trackers;
  cvtdl_lane_t *lane_meta = &app_handle->adas_info->lane_meta;

  cv::Scalar box_color;
  for (uint32_t oid = 0; oid < obj_meta->size; oid++) {
    // if (track_meta->info[oid].state == CVI_TRACKER_NEW) {
    //     box_color = cv::Scalar(0, 255, 0);
    // } else if (track_meta->info[oid].state == CVI_TRACKER_UNSTABLE) {
    //     box_color = cv::Scalar(105, 105, 105);
    // } else {  // CVI_TRACKER_STABLE
    //     box_color = gen_random_color(obj_meta->info[oid].unique_id, 64);
    // }
    box_color = gen_random_color(obj_meta->info[oid].unique_id, 64);

    cv::Point top_left((int)obj_meta->info[oid].bbox.x1, (int)obj_meta->info[oid].bbox.y1);
    cv::Point bottom_right((int)obj_meta->info[oid].bbox.x2, (int)obj_meta->info[oid].bbox.y2);

    cv::rectangle(img_rgb, top_left, bottom_right, box_color, 2);

    char txt_info[256];
    snprintf(txt_info, sizeof(txt_info), "[%d]S:%d m, V:%.1f m/s, [%s]",
             obj_meta->info[oid].classes, (int)obj_meta->info[oid].adas_properity.dis,
             obj_meta->info[oid].adas_properity.speed,
             enumStr[obj_meta->info[oid].adas_properity.state]);

    if (obj_meta->info[oid].adas_properity.state != 0) box_color = cv::Scalar(0, 0, 255);

    cv::putText(img_rgb, txt_info, cv::Point(top_left.x, top_left.y - 10), 0, 0.5, box_color, 1);
  }

  if (app_handle->adas_info->det_type) {
    int size = bg->stVFrame.u32Width >= 1080 ? 6 : 3;

    for (int i = 0; i < lane_meta->size; i++) {
      int x1 = (int)lane_meta->lane[i].x[0];
      int y1 = (int)lane_meta->lane[i].y[0];
      int x2 = (int)lane_meta->lane[i].x[1];
      int y2 = (int)lane_meta->lane[i].y[1];

      cv::line(img_rgb, cv::Point(x1, y1), cv::Point(x2, y2), cv::Scalar(0, 255, 0), size);
    }

    char lane_info[64];

    if (app_handle->adas_info->lane_state == 0) {
      strcpy(lane_info, "NORMAL");
      box_color = cv::Scalar(0, 255, 0);
    } else {
      strcpy(lane_info, "LANE DEPARTURE WARNING !");
      box_color = cv::Scalar(0, 0, 255);
    }

    cv::putText(img_rgb, lane_info,
                cv::Point((int)(0.3 * bg->stVFrame.u32Width), (int)(0.8 * bg->stVFrame.u32Height)),
                0, size / 3, box_color, size / 3);

    snprintf(lane_info, sizeof(lane_info), "%d", g_count);
    cv::putText(img_rgb, lane_info, cv::Point(10, 50), 0, size / 3, cv::Scalar(0, 255, 0),
                size / 3);

    g_count++;
  }

  cv::imwrite(save_path, img_rgb);
  CVI_SYS_Munmap((void *)bg->stVFrame.pu8VirAddr[0], bg->stVFrame.u32Length[0]);
}

std::string adas_info(cvitdl_app_handle_t app_handle) {
  cvtdl_object_t *obj_meta = &app_handle->adas_info->last_objects;
  cvtdl_lane_t *lane_meta = &app_handle->adas_info->lane_meta;

  std::stringstream ss;
  // unique_id class x1 y1 x2 y2 distance speed object_state;......; x1 y1 x2 y2 x1 y1 x2 y2
  // lane_state
  for (uint32_t oid = 0; oid < obj_meta->size; oid++) {
    ss << obj_meta->info[oid].unique_id << " " << obj_meta->info[oid].classes << " "
       << (int)obj_meta->info[oid].bbox.x1 << " " << (int)obj_meta->info[oid].bbox.y1 << " "
       << (int)obj_meta->info[oid].bbox.x2 << " " << (int)obj_meta->info[oid].bbox.y2 << " "
       << std::fixed << std::setprecision(2) << obj_meta->info[oid].adas_properity.dis << " "
       << std::fixed << std::setprecision(2) << obj_meta->info[oid].adas_properity.speed << " "
       << obj_meta->info[oid].adas_properity.state << "\n";
  }

  if (app_handle->adas_info->det_type) {
    for (uint32_t i = 0; i < lane_meta->size; i++) {
      int x1 = (int)lane_meta->lane[i].x[0];
      int y1 = (int)lane_meta->lane[i].y[0];
      int x2 = (int)lane_meta->lane[i].x[1];
      int y2 = (int)lane_meta->lane[i].y[1];

      ss << x1 << " " << y1 << " " << x2 << " " << y2 << " ";
    }
    ss << app_handle->adas_info->lane_state << "\n";
  }
  return ss.str();
}

int main(int argc, char *argv[]) {
  if (argc != 7 && argc != 9 && argc != 11) {
    printf(
        "\nUsage: %s \nperson_vehicle_model_path  image_dir_path img_list_path output_dir_path \n"
        "det_type(0: only object, 1: object and lane) \n"
        "save_mode(0: draw, 1: save txt info, 2: both) \n"
        "[lane_det_model_path](optional, if det_type=1, must exist) \n"
        "[lane_model_type](optional, 0 for lane_det model, 1 for lstr model, if det_type=1, must "
        "exist) \n",
        argv[0]);
    return -1;
  }

  int vpssgrp_width = 1920;
  int vpssgrp_height = 1080;

  CVI_S32 ret = MMF_INIT_HELPER2(vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888, 1,
                                 vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888, 1);
  if (ret != CVI_TDL_SUCCESS) {
    printf("Init sys failed with %#x!\n", ret);
    return ret;
  }

  cvitdl_handle_t tdl_handle = NULL;
  ret = CVI_TDL_CreateHandle(&tdl_handle);
  if (ret != CVI_SUCCESS) {
    printf("Create tdl handle failed with %#x!\n", ret);
    return ret;
  }

  // setup yolo algorithm preprocess
  cvtdl_det_algo_param_t yolov8_param =
      CVI_TDL_GetDetectionAlgoParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION);
  yolov8_param.cls = 7;

  printf("setup yolov8 algorithm param \n");
  ret = CVI_TDL_SetDetectionAlgoParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION,
                                      yolov8_param);
  if (ret != CVI_SUCCESS) {
    printf("Can not set yolov8 algorithm parameters %#x\n", ret);
    return ret;
  }

  uint32_t buffer_size = 20;
  int det_type = atoi(argv[5]);
  cvitdl_app_handle_t app_handle = NULL;
  ret |= CVI_TDL_APP_CreateHandle(&app_handle, tdl_handle);
  ret |= CVI_TDL_APP_ADAS_Init(app_handle, (uint32_t)buffer_size, det_type);

  if (ret != CVI_SUCCESS) {
    printf("failed with %#x!\n", ret);
    // goto setup_tdl_fail;
  }

  CVI_TDL_DeepSORT_Init(tdl_handle, true);
  cvtdl_deepsort_config_t ds_conf;
  CVI_TDL_DeepSORT_GetDefaultConfig(&ds_conf);
  set_sample_mot_config(&ds_conf);
  CVI_TDL_DeepSORT_SetConfig(tdl_handle, &ds_conf, -1, false);

  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION, argv[1]);
  if (ret != CVI_SUCCESS) {
    printf("open CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION failed with %#x!\n", ret);
    return ret;
  }

  if (det_type == 1) {  // detect lane
    app_handle->adas_info->lane_model_type = atoi(argv[8]);

    CVI_TDL_SUPPORTED_MODEL_E lane_model_id;
    if (app_handle->adas_info->lane_model_type == 0) {
      lane_model_id = CVI_TDL_SUPPORTED_MODEL_LANE_DET;
    } else if (app_handle->adas_info->lane_model_type == 1) {
      lane_model_id = CVI_TDL_SUPPORTED_MODEL_LSTR;
    } else {
      printf(" err lane_model_type !\n");
      return -1;
    }

    ret = CVI_TDL_OpenModel(tdl_handle, lane_model_id, argv[7]);
    if (ret != CVI_SUCCESS) {
      printf("open lane det model failed with %#x!\n", ret);
      return ret;
    }
  }

  imgprocess_t img_handle;
  CVI_TDL_Create_ImageProcessor(&img_handle);

  std::string str_image_root(argv[2]);
  std::string image_list(argv[3]);
  std::string str_dst_root = std::string(argv[4]);
  int save_mode = atoi(argv[6]);

  if (!create_directory(str_dst_root)) {
    // std::cout << "create directory:" << str_dst_root << " failed\n";
    std::cout << " \n";
  }
  std::string str_dst_video = join_path(str_dst_root, get_directory_name(str_image_root));
  if (!create_directory(str_dst_video)) {
    // std::cout << "create directory:" << str_dst_video << " failed\n";
    std::cout << " \n";
    // return CVI_FAILURE;
  }

  std::cout << "to read imagelist:" << image_list << std::endl;
  std::vector<std::string> image_files = read_file_lines(image_list);
  if (str_image_root.size() == 0) {
    std::cout << ", imageroot empty\n";
    return -1;
  }
  if (str_image_root.at(str_image_root.size() - 1) != '/') {
    str_image_root = str_image_root + std::string("/");
  }

  int start_idx = 0;
  int end_idx = image_files.size();

  if (argc == 11) {
    start_idx = atoi(argv[9]);
    end_idx = atoi(argv[10]);
  }
  printf("start_idx: %d, end_idx:%d\n", start_idx, end_idx);

  for (int img_idx = start_idx; img_idx < end_idx; img_idx++) {
    VIDEO_FRAME_INFO_S bg;

    // std::cout << "processing:" << img_idx << "/1000\n";
    // char szimg[256];
    // sprintf(szimg, "%s/%08d.jpg", str_image_root.c_str(), img_idx);

    std::string strf = str_image_root + image_files[img_idx];

    // std::cout << "processing:" << img_idx << "/1000,path:" << szimg << std::endl;

    std::cout << "processing :" << img_idx + 1 << "/" << image_files.size() << "\t"
              << image_files[img_idx] << std::endl;

    ret = CVI_TDL_ReadImage(img_handle, strf.c_str(), &bg, PIXEL_FORMAT_BGR_888);
    if (ret != CVI_SUCCESS) {
      printf("open img failed with %#x!\n", ret);
      return ret;
    } else {
      printf("image read,width:%d\n", bg.stVFrame.u32Width);
    }

    // set FPS and gsensor data
    // app_handle->adas_info->FPS = 15;
    // app_handle->adas_info->gsensor_data.x = 0;
    // app_handle->adas_info->gsensor_data.y = 0;
    // app_handle->adas_info->gsensor_data.z = 0;
    // app_handle->adas_info->gsensor_data.counter += 1;

    ret = CVI_TDL_APP_ADAS_Run(app_handle, &bg);

    if (ret != CVI_TDL_SUCCESS) {
      printf("inference failed!, ret=%x\n", ret);
      CVI_TDL_APP_DestroyHandle(app_handle);

      // goto inf_error;
    }

    // cvtdl_object_t *obj_meta = &app_handle->adas_info->last_objects;
    // for (uint32_t i = 0; i < obj_meta->size; i++) {
    //     std::cout << "[" << obj_meta->info[i].bbox.x1 << "," << obj_meta->info[i].bbox.y1 << ","
    //         << obj_meta->info[i].bbox.x2 << "," << obj_meta->info[i].bbox.y2 << "]," <<std::endl;
    // }
    if (save_mode == 0 || save_mode == 2) {
      char save_path[256];
      sprintf(save_path, "%s/%s", str_dst_video.c_str(), image_files[img_idx].c_str());
      draw_adas(app_handle, &bg, save_path);
    }

    if (save_mode == 1 || save_mode == 2) {
      std::string dstf = str_dst_video + "/" + replace_file_ext(image_files[img_idx], "txt");
      std::cout << "writing file:" << dstf << std::endl;
      std::string str_res = adas_info(app_handle);
      FILE *fp = fopen(dstf.c_str(), "w");
      fwrite(str_res.c_str(), str_res.size(), 1, fp);
      fclose(fp);
    }

    CVI_TDL_ReleaseImage(img_handle, &bg);
  }

  CVI_TDL_APP_DestroyHandle(app_handle);
  CVI_TDL_DestroyHandle(tdl_handle);
  CVI_TDL_Destroy_ImageProcessor(img_handle);
  return ret;
}
