#include "od.h"

CVI_S32 get_od_model_info(const char *model_name, CVI_TDL_SUPPORTED_MODEL_E *model_index,
                          ODInferenceFunc *inference_func) {
  CVI_S32 ret = CVI_SUCCESS;

  if (strcmp(model_name, "mobiledetv2-person-vehicle") == 0) {
    *model_index = CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PERSON_VEHICLE;
    *inference_func = CVI_TDL_MobileDetV2_Person_Vehicle;
  } else if (strcmp(model_name, "mobiledetv2-person-pets") == 0) {
    *model_index = CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PERSON_PETS;
    *inference_func = CVI_TDL_MobileDetV2_Person_Pets;
  } else if (strcmp(model_name, "mobiledetv2-coco80") == 0) {
    *model_index = CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_COCO80;
    *inference_func = CVI_TDL_MobileDetV2_COCO80;
  } else if (strcmp(model_name, "mobiledetv2-vehicle") == 0) {
    *model_index = CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_VEHICLE;
    *inference_func = CVI_TDL_MobileDetV2_Vehicle;
  } else if (strcmp(model_name, "mobiledetv2-pedestrian") == 0) {
    *model_index = CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PEDESTRIAN;
    *inference_func = CVI_TDL_MobileDetV2_Pedestrian;
  } else if (strcmp(model_name, "yolov3") == 0) {
    *model_index = CVI_TDL_SUPPORTED_MODEL_YOLOV3;
    *inference_func = CVI_TDL_Yolov3;
  } else {
    ret = CVI_TDL_FAILURE;
  }
  return ret;
}

CVI_S32 get_pd_model_info(const char *model_name, CVI_TDL_SUPPORTED_MODEL_E *model_index,
                          ODInferenceFunc *inference_func) {
  CVI_S32 ret = CVI_SUCCESS;

  if (strcmp(model_name, "mobiledetv2-person-vehicle") == 0) {
    *model_index = CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PERSON_VEHICLE;
    *inference_func = CVI_TDL_MobileDetV2_Person_Vehicle;
  } else if (strcmp(model_name, "mobiledetv2-person-pets") == 0) {
    *model_index = CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PERSON_PETS;
    *inference_func = CVI_TDL_MobileDetV2_Person_Pets;
  } else if (strcmp(model_name, "mobiledetv2-coco80") == 0) {
    *model_index = CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_COCO80;
    *inference_func = CVI_TDL_MobileDetV2_COCO80;
  } else if (strcmp(model_name, "mobiledetv2-pedestrian") == 0) {
    *model_index = CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PEDESTRIAN;
    *inference_func = CVI_TDL_MobileDetV2_Pedestrian;
  } else if (strcmp(model_name, "yolov3") == 0) {
    *model_index = CVI_TDL_SUPPORTED_MODEL_YOLOV3;
    *inference_func = CVI_TDL_Yolov3;
  } else {
    ret = CVI_TDL_FAILURE;
  }
  return ret;
}