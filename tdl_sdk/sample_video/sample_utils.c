#include "sample_utils.h"
#include "cvi_tdl.h"
#define FACE_FEAT_SIZE 256
CVI_S32 get_od_model_info(const char *model_name, CVI_TDL_SUPPORTED_MODEL_E *model_index,
                          ODInferenceFunc *inference_func) {
  CVI_S32 ret = CVI_SUCCESS;
  *inference_func = CVI_TDL_Detection;
  if (strcmp(model_name, "mobiledetv2-person-vehicle") == 0) {
    *model_index = CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PERSON_VEHICLE;
  } else if (strcmp(model_name, "mobiledetv2-person-pets") == 0) {
    *model_index = CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PERSON_PETS;
  } else if (strcmp(model_name, "mobiledetv2-coco80") == 0) {
    *model_index = CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_COCO80;
  } else if (strcmp(model_name, "mobiledetv2-vehicle") == 0) {
    *model_index = CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_VEHICLE;
  } else if (strcmp(model_name, "mobiledetv2-pedestrian") == 0) {
    *model_index = CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PEDESTRIAN;
  } else if (strcmp(model_name, "yolov3") == 0) {
    *model_index = CVI_TDL_SUPPORTED_MODEL_YOLOV3;
  } else if (strcmp(model_name, "yolox") == 0) {
    *model_index = CVI_TDL_SUPPORTED_MODEL_YOLOX;
  } else {
    ret = CVI_TDL_FAILURE;
  }
  return ret;
}

CVI_S32 get_pd_model_info(const char *model_name, CVI_TDL_SUPPORTED_MODEL_E *model_index,
                          ODInferenceFunc *inference_func) {
  CVI_S32 ret = CVI_SUCCESS;
  *inference_func = CVI_TDL_Detection;
  if (strcmp(model_name, "mobiledetv2-person-vehicle") == 0) {
    *model_index = CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PERSON_VEHICLE;
  } else if (strcmp(model_name, "mobiledetv2-person-pets") == 0) {
    *model_index = CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PERSON_PETS;
  } else if (strcmp(model_name, "mobiledetv2-coco80") == 0) {
    *model_index = CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_COCO80;
  } else if (strcmp(model_name, "mobiledetv2-pedestrian") == 0) {
    *model_index = CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PEDESTRIAN;
  } else if (strcmp(model_name, "yolov3") == 0) {
    *model_index = CVI_TDL_SUPPORTED_MODEL_YOLOV3;
  } else if (strcmp(model_name, "yolox") == 0) {
    *model_index = CVI_TDL_SUPPORTED_MODEL_YOLOX;
  } else {
    ret = CVI_TDL_FAILURE;
  }
  return ret;
}

CVI_S32 get_vehicle_model_info(const char *model_name, CVI_TDL_SUPPORTED_MODEL_E *model_index,
                               ODInferenceFunc *inference_func) {
  CVI_S32 ret = CVI_SUCCESS;
  *inference_func = CVI_TDL_Detection;
  if (strcmp(model_name, "mobiledetv2-person-vehicle") == 0) {
    *model_index = CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PERSON_VEHICLE;
  } else if (strcmp(model_name, "mobiledetv2-coco80") == 0) {
    *model_index = CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_COCO80;
  } else if (strcmp(model_name, "mobiledetv2-vehicle") == 0) {
    *model_index = CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_VEHICLE;
  } else if (strcmp(model_name, "yolov3") == 0) {
    *model_index = CVI_TDL_SUPPORTED_MODEL_YOLOV3;
  } else if (strcmp(model_name, "yolox") == 0) {
    *model_index = CVI_TDL_SUPPORTED_MODEL_YOLOX;
  } else {
    ret = CVI_TDL_FAILURE;
  }
  return ret;
}
int release_image(VIDEO_FRAME_INFO_S *frame) {
  CVI_S32 ret = CVI_SUCCESS;
  if (frame->stVFrame.u64PhyAddr[0] != 0) {
    ret = CVI_SYS_IonFree(frame->stVFrame.u64PhyAddr[0], frame->stVFrame.pu8VirAddr[0]);
    frame->stVFrame.u64PhyAddr[0] = (CVI_U64)0;
    frame->stVFrame.u64PhyAddr[1] = (CVI_U64)0;
    frame->stVFrame.u64PhyAddr[2] = (CVI_U64)0;
    frame->stVFrame.pu8VirAddr[0] = NULL;
    frame->stVFrame.pu8VirAddr[1] = NULL;
    frame->stVFrame.pu8VirAddr[2] = NULL;
  }
  return ret;
}

CVI_S32 register_gallery_feature(cvitdl_handle_t tdl_handle, const char *sz_feat_file,
                                 cvtdl_service_feature_array_t *p_feat_gallery) {
  FILE *fp = fopen(sz_feat_file, "rb");
  if (fp == NULL) {
    printf("read %s failed\n", sz_feat_file);
    return CVI_TDL_FAILURE;
  }
  fseek(fp, 0, SEEK_END);
  int len = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  int8_t *ptr_feat = (int8_t *)malloc(len);
  fread(ptr_feat, 1, len, fp);
  fclose(fp);
  printf("read %s done,len:%d\n", sz_feat_file, len);
  if (len != FACE_FEAT_SIZE) {
    free(ptr_feat);
    return CVI_TDL_FAILURE;
  }
  size_t src_size = 0;
  int8_t *p_new_feat = NULL;
  if (p_feat_gallery->ptr == 0) {
    p_new_feat = (int8_t *)malloc(FACE_FEAT_SIZE);
    p_feat_gallery->type = TYPE_INT8;
    p_feat_gallery->feature_length = FACE_FEAT_SIZE;
    printf("allocate memory,size:%d\n", (int)p_feat_gallery->feature_length);
  } else {
    src_size = p_feat_gallery->feature_length * p_feat_gallery->data_num;
    p_new_feat = (int8_t *)malloc(src_size + FACE_FEAT_SIZE);
    memcpy(p_new_feat, p_feat_gallery->ptr, src_size);
    free(p_feat_gallery->ptr);
  }
  memcpy(p_new_feat + src_size, ptr_feat, FACE_FEAT_SIZE);
  p_feat_gallery->data_num += 1;
  p_feat_gallery->ptr = p_new_feat;
  free(ptr_feat);
  printf("register face sucess,gallery num:%d\n", (int)p_feat_gallery->data_num);
  return CVI_SUCCESS;
}

CVI_S32 register_gallery_face(cvitdl_handle_t tdl_handle, const char *sz_img_file,
                              CVI_TDL_SUPPORTED_MODEL_E *fd_index, FaceDetInferFunc fd_func,
                              FaceRecInferFunc fr_func,
                              cvtdl_service_feature_array_t *p_feat_gallery) {
  VIDEO_FRAME_INFO_S img_frm;
  CVI_S32 ret = CVI_SUCCESS;
  printf("to load:%s\n", sz_img_file);
  if (CVI_SUCCESS != CVI_TDL_LoadBinImage(sz_img_file, &img_frm, PIXEL_FORMAT_RGB_888_PLANAR)) {
    printf("cvi_tdl read image :%s failed.\n", sz_img_file);
    return CVI_TDL_FAILURE;
  }

  cvtdl_face_t faceinfo;
  memset(&faceinfo, 0, sizeof(faceinfo));
  if (CVI_SUCCESS != fd_func(tdl_handle, &img_frm, *fd_index, &faceinfo)) {
    printf("fd_inference failed\n");
    return CVI_FAILURE;
  }
  printf("detect face num:%u\n", faceinfo.size);

  if (CVI_SUCCESS != fr_func(tdl_handle, &img_frm, &faceinfo)) {
    printf("fr inference failed\n");
    return CVI_FAILURE;
  }

  if (faceinfo.size == 0 || faceinfo.info[0].feature.ptr == NULL) {
    printf("face num error,got:%d\n", (int)faceinfo.size);
    ret = CVI_FAILURE;
  } else {
    printf("extract featsize:%u,addr:%p\n", faceinfo.info[0].feature.size,
           (void *)faceinfo.info[0].feature.ptr);
  }

  if (ret == CVI_FAILURE) {
    release_image(&img_frm);
    CVI_TDL_Free(&faceinfo);
    return ret;
  }

  int8_t *p_new_feat = NULL;
  size_t src_size = 0;
  if (p_feat_gallery->ptr == 0) {
    p_new_feat = (int8_t *)malloc(faceinfo.info[0].feature.size);
    p_feat_gallery->type = faceinfo.info[0].feature.type;
    p_feat_gallery->feature_length = faceinfo.info[0].feature.size;
    printf("allocate memory,size:%d\n", (int)p_feat_gallery->feature_length);
  } else {
    if (p_feat_gallery->feature_length != faceinfo.info[0].feature.size) {
      printf("error,featsize not equal,curface:%u,gallery:%u\n", faceinfo.info[0].feature.size,
             p_feat_gallery->feature_length);
      ret = CVI_FAILURE;
    } else {
      src_size = p_feat_gallery->feature_length * p_feat_gallery->data_num;
      p_new_feat = (int8_t *)malloc(src_size + FACE_FEAT_SIZE);
      memcpy(p_new_feat, p_feat_gallery->ptr, src_size);
    }
  }

  if (ret == CVI_SUCCESS) {
    if (p_feat_gallery->ptr != NULL) {
      memcpy(p_new_feat, p_feat_gallery->ptr + src_size, p_feat_gallery->feature_length);
      free(p_feat_gallery->ptr);
    }
    memcpy(p_new_feat + src_size, faceinfo.info[0].feature.ptr, faceinfo.info[0].feature.size);
    p_feat_gallery->data_num += 1;
    p_feat_gallery->ptr = p_new_feat;
    printf("register gallery\n");
  }

  release_image(&img_frm);
  CVI_TDL_Free(&faceinfo);
  return ret;
}
CVI_S32 do_face_match(cvitdl_service_handle_t service_handle, cvtdl_face_info_t *p_face,
                      float thresh) {
  if (p_face->feature.size == 0) {
    return CVI_FAILURE;
  }
  uint32_t ind = 0;
  float score = 0;
  uint32_t size;

  int ret = CVI_TDL_Service_FaceInfoMatching(service_handle, p_face, 1, 0.1, &ind, &score, &size);
  // printf("ind:%u,ret:%d,score:%f\n",ind,ret,score);
  // getchar();

  if (score > thresh) {
    p_face->recog_score = score;
    sprintf(p_face->name, "%d", ind + 1);
    printf("matchname,trackid:%u,index:%d,score:%f,ret:%d,featlen:%d,name:%s\n",
           (uint32_t)p_face->unique_id, ind + 1, score, ret, (int)p_face->feature.size,
           p_face->name);
  }

  return ret;
}