#include <cvi_ive.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "core/utils/vpss_helper.h"
#include "cvi_tdl.h"
#include "cvi_tdl_app/cvi_tdl_app.h"
#include "cvi_tdl_media.h"
#include "sample_comm.h"
#include "stb_image.h"
#include "stb_image_write.h"
#include "sys_utils.h"
#include "cvi_kit.h"

void set_sample_mot_config(cvtdl_deepsort_config_t *ds_conf) {
  ds_conf->ktracker_conf.P_beta[2] = 0.01;
  ds_conf->ktracker_conf.P_beta[6] = 1e-5;

  ds_conf->kfilter_conf.Q_beta[2] = 0.01;
  ds_conf->kfilter_conf.Q_beta[6] = 1e-5;
  ds_conf->kfilter_conf.R_beta[2] = 0.1;
}

char *adas_info(cvitdl_app_handle_t app_handle) {
    if (!app_handle || !app_handle->adas_info) {
        return NULL; // 检查输入有效性
    }

    cvtdl_object_t *obj_meta = &app_handle->adas_info->last_objects;
    cvtdl_lane_t *lane_meta = &app_handle->adas_info->lane_meta;

    // 初始化动态缓冲区
    size_t buffer_size = 1024; // 初始缓冲区大小
    char *buffer = (char *)malloc(buffer_size);
    if (!buffer) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }
    buffer[0] = '\0'; // 初始化为空字符串

    // 拼接对象信息
    for (uint32_t oid = 0; oid < obj_meta->size; oid++) {
        char temp[256]; // 临时缓冲区
        snprintf(temp, sizeof(temp),
                 "%d %d %d %d %d %d %.2f %.2f %d\n",
                 obj_meta->info[oid].unique_id,
                 obj_meta->info[oid].classes,
                 (int)obj_meta->info[oid].bbox.x1,
                 (int)obj_meta->info[oid].bbox.y1,
                 (int)obj_meta->info[oid].bbox.x2,
                 (int)obj_meta->info[oid].bbox.y2,
                 obj_meta->info[oid].adas_properity.dis,
                 obj_meta->info[oid].adas_properity.speed,
                 obj_meta->info[oid].adas_properity.state);

        // 检查缓冲区是否足够
        if (strlen(buffer) + strlen(temp) + 1 > buffer_size) {
            buffer_size *= 2; // 增加缓冲区大小
            char *new_buffer = (char *)realloc(buffer, buffer_size);
            if (!new_buffer) {
                fprintf(stderr, "Memory allocation failed\n");
                free(buffer);
                return NULL;
            }
            buffer = new_buffer;
        }

        strcat(buffer, temp); // 拼接到最终缓冲区
    }

    // 拼接车道信息
    if (app_handle->adas_info->det_type) {
        for (uint32_t i = 0; i < lane_meta->size; i++) {
            char temp[128];
            int x1 = (int)lane_meta->lane[i].x[0];
            int y1 = (int)lane_meta->lane[i].y[0];
            int x2 = (int)lane_meta->lane[i].x[1];
            int y2 = (int)lane_meta->lane[i].y[1];

            snprintf(temp, sizeof(temp), "%d %d %d %d ", x1, y1, x2, y2);

            // 检查缓冲区是否足够
            if (strlen(buffer) + strlen(temp) + 1 > buffer_size) {
                buffer_size *= 2; // 增加缓冲区大小
                char *new_buffer = (char *)realloc(buffer, buffer_size);
                if (!new_buffer) {
                    fprintf(stderr, "Memory allocation failed\n");
                    free(buffer);
                    return NULL;
                }
                buffer = new_buffer;
            }

            strcat(buffer, temp); // 拼接到最终缓冲区
        }

        // 添加 lane_state
        char temp[32];
        snprintf(temp, sizeof(temp), "%d\n", app_handle->adas_info->lane_state);

        if (strlen(buffer) + strlen(temp) + 1 > buffer_size) {
            buffer_size *= 2; // 增加缓冲区大小
            char *new_buffer = (char *)realloc(buffer, buffer_size);
            if (!new_buffer) {
                fprintf(stderr, "Memory allocation failed\n");
                free(buffer);
                return NULL;
            }
            buffer = new_buffer;
        }

        strcat(buffer, temp); // 拼接到最终缓冲区
    }

    return buffer; // 返回最终字符串
}

int main(int argc, char *argv[]) {
  if (argc != 8 && argc != 10 && argc != 12) {
    printf(
        "\nUsage: %s \n person_vehicle_model_path  image_dir_path img_list_path output_dir_path \n"
        "det_type(0: only object, 1: object and lane) \n"
        "sence_type(0: car, 1: motorbike) \n"
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
  app_handle->adas_info->sence_type = atoi(argv[6]);  // 0 for car, 1 for motorbike, defult 1
  app_handle->adas_info->location_type = 1;  // camera location, 0 for left, 1 for middle, 2 for
                                             // right, defult 1, only work when sence_type=1

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
    app_handle->adas_info->lane_model_type = atoi(argv[9]);

    CVI_TDL_SUPPORTED_MODEL_E lane_model_id;
    if (app_handle->adas_info->lane_model_type == 0) {
      lane_model_id = CVI_TDL_SUPPORTED_MODEL_LANE_DET;
    } else if (app_handle->adas_info->lane_model_type == 1) {
      lane_model_id = CVI_TDL_SUPPORTED_MODEL_LSTR;
    } else {
      printf(" err lane_model_type !\n");
      return -1;
    }

    ret = CVI_TDL_OpenModel(tdl_handle, lane_model_id, argv[8]);
    if (ret != CVI_SUCCESS) {
      printf("open lane det model failed with %#x!\n", ret);
      return ret;
    }
  }

  imgprocess_t img_handle;
  CVI_TDL_Create_ImageProcessor(&img_handle);

  char* str_image_root=argv[2];
  char* image_list=argv[3];
  char* str_dst_root=argv[4];
  int save_mode = atoi(argv[7]);

  if (!create_directory(str_dst_root)) {
      printf("Failed to create directory: %s\n", str_dst_root);
  }

  char* str_dst_video = join_path(str_dst_root, get_directory_name(str_image_root));

  if (!create_directory(str_dst_video)) {
      printf("Failed to create directory: %s\n", str_dst_video);
  }

  printf("Reading image list: %s\n", image_list);

  int line_count = 0;
  char** image_files = read_file_lines(image_list, &line_count);

  if (line_count == 0) {
      printf("Image root is empty\n");
      return -1;
  }
  size_t len = strlen(str_image_root); // 获取字符串长度

  // 检查字符串最后一个字符是否为 '/'
  if (len > 0 && str_image_root[len - 1] != '/') {
      // 如果不是 '/'，在末尾追加 '/'
      if (len + 1 < sizeof(str_image_root)) { // 确保不会导致缓冲区溢出
          strcat(str_image_root, "/");
      } else {
          // 如果缓冲区不足，处理错误（例如打印错误信息或退出程序）
          fprintf(stderr, "Error: str_image_root buffer size is too small to append '/'.\n");
      }
  }
  
  int start_idx = 0;
  int end_idx = line_count;

  if (argc == 12) {
    start_idx = atoi(argv[10]);
    end_idx = atoi(argv[11]);
  }
  printf("start_idx: %d, end_idx:%d\n", start_idx, end_idx);

  for (int img_idx = start_idx; img_idx < end_idx; img_idx++) {
    VIDEO_FRAME_INFO_S bg;

    char strf[1024]; 
    snprintf(strf, sizeof(strf), "%s%s", str_image_root, image_files[img_idx]);
    printf("Processing: %d/%d\t%s\n", img_idx + 1, line_count, image_files[img_idx]);

    ret = CVI_TDL_ReadImage(img_handle, strf, &bg, PIXEL_FORMAT_BGR_888);
    if (ret != CVI_SUCCESS) {
      printf("open img failed with %#x!\n", ret);
      return ret;
    } else {
      printf("image read,width:%d\n", bg.stVFrame.u32Width);
    }

    // set FPS and gsensor data
    app_handle->adas_info->FPS = 15;  // infer FPS, only set once if FPS is stable

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

#ifndef NO_OPENCV
    if (save_mode == 0 || save_mode == 2) {
      char save_path[256];
      sprintf(save_path, "%s/%s", str_dst_video, image_files[img_idx]);
      CVI_TDL_Draw_ADAS(app_handle, &bg, save_path);
    }
#endif

    if (save_mode == 1 || save_mode == 2) {
      char dstf[512];
      sprintf(dstf, "%s/%s", str_dst_video, replace_file_ext(image_files[img_idx], "txt"));
      printf("writing file: %s",dstf);
      char* str_res = adas_info(app_handle);
      FILE *fp = fopen(dstf, "w");
      fwrite(str_res, strlen(str_res), 1, fp);
      fclose(fp);
    }

    CVI_TDL_ReleaseImage(img_handle, &bg);
  }

  CVI_TDL_APP_DestroyHandle(app_handle);
  CVI_TDL_DestroyHandle(tdl_handle);
  CVI_TDL_Destroy_ImageProcessor(img_handle);
  return ret;
}
