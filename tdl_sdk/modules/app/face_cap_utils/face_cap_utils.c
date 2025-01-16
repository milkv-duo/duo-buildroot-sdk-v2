#include <math.h>
#include <sys/time.h>

#include "cvi_tdl_log.hpp"
#include "cvi_venc.h"
#include "face_cap_utils.h"
#include "service/cvi_tdl_service.h"
#include "vpss_helper.h"

#define ABS(x) ((x) >= 0 ? (x) : (-(x)))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

void face_capture_init_venc(VENC_CHN VeChn) {
  VENC_RECV_PIC_PARAM_S stRecvParam;
  VENC_CHN_ATTR_S stAttr;
  venc_extern_init = 0;
  VENC_JPEG_PARAM_S stJpegParam;

  if (CVI_SUCCESS == CVI_VENC_GetChnAttr(VeChn, &stAttr)) {
    venc_extern_init = 1;
    printf("venc channel %d have been init \n", VeChn);
    return;
  }

  memset(&stAttr, 0, sizeof(VENC_CHN_ATTR_S));
  stAttr.stVencAttr.enType = PT_JPEG;
  stAttr.stVencAttr.u32MaxPicWidth = 1920;
  stAttr.stVencAttr.u32MaxPicHeight = 1080;
  stAttr.stVencAttr.u32BufSize = 1024 * 512;
  stAttr.stVencAttr.u32Profile = H264E_PROFILE_BASELINE;
  stAttr.stVencAttr.bByFrame = CVI_TRUE;

  // not care,will be modify
  stAttr.stVencAttr.u32PicHeight = 64;
  stAttr.stVencAttr.u32PicWidth = 64;

  stAttr.stGopAttr.enGopMode = VENC_GOPMODE_NORMALP;
  stAttr.stGopAttr.stNormalP.s32IPQpDelta = 0;
  stAttr.stVencAttr.stAttrJpege.bSupportDCF = CVI_FALSE;
  stAttr.stVencAttr.stAttrJpege.enReceiveMode = VENC_PIC_RECEIVE_SINGLE;
  stAttr.stVencAttr.stAttrJpege.stMPFCfg.u8LargeThumbNailNum = 0;

  // set channel
  CVI_VENC_CreateChn(VeChn, &stAttr);

  CVI_VENC_GetJpegParam(VeChn, &stJpegParam);
  stJpegParam.u32Qfactor = 20;
  CVI_VENC_SetJpegParam(VeChn, &stJpegParam);
  stRecvParam.s32RecvPicNum = -1;
  CVI_VENC_StartRecvFrame(VeChn, &stRecvParam);
}

void release_venc(VENC_CHN VeChn) {
  CVI_VENC_StopRecvFrame(VeChn);
  CVI_VENC_ResetChn(VeChn);
  CVI_VENC_DestroyChn(VeChn);
}

float get_score(cvtdl_face_info_t *face_info, uint32_t img_w, uint32_t img_h, bool fl_model) {
  cvtdl_bbox_t *bbox = &face_info->bbox;
  cvtdl_pts_t *pts_info = &face_info->pts;
  cvtdl_head_pose_t *pose = &face_info->head_pose;
  float velx = face_info->velx;
  float vely = face_info->vely;
  float blurness = face_info->blurness;
  float nose_x = pts_info->x[2];
  // float nose_y = pts_info->y[2];
  float left_max = MIN(pts_info->x[0], pts_info->x[3]);
  float right_max = MAX(pts_info->x[1], pts_info->x[4]);
  float width = bbox->x2 - bbox->x1;
  float height = bbox->y2 - bbox->y1;
  float l_ = nose_x - left_max;
  float r_ = right_max - nose_x;
  // printf("box:[%.3f,%.3f,%.3f,%.3f],w:%.3f,h:%.3f\n",bbox->x1,bbox->y1,bbox->x2,bbox->y2,width,height);
  // printf("kpts=[");
  // for(int i = 0; i < 5; i++){
  //   printf("%.3f,%.3f,",pts_info->x[i],pts_info->y[i]);
  // }

  float eye_diff_x = pts_info->x[1] - pts_info->x[0];
  float eye_diff_y = pts_info->y[1] - pts_info->y[0];
  float eye_size = sqrt(eye_diff_x * eye_diff_x + eye_diff_y * eye_diff_y);

  float mouth_diff_x = pts_info->x[4] - pts_info->x[3];
  float mouth_diff_y = pts_info->y[4] - pts_info->y[3];
  float mouth_size = sqrt(mouth_diff_x * mouth_diff_x + mouth_diff_y * mouth_diff_y);
  float vel = sqrt(velx * velx + vely * vely);

  if (pts_info->x[1] > bbox->x2 || pts_info->x[2] > bbox->x2 || pts_info->x[4] > bbox->x2 ||
      pts_info->x[0] < bbox->x1 || pts_info->x[2] < bbox->x1 || pts_info->x[3] < bbox->x1) {
    return 0.0;
  } else if ((l_ + 0.01 * width) < 0 || (r_ + 0.01 * width) < 0 || (eye_size / width) < 0.25 ||
             (mouth_size / width) < 0.15) {
    return 0.0;
  } else if ((pts_info->y[0] < bbox->y1 || pts_info->y[1] < bbox->y1 || pts_info->y[3] > bbox->y2 ||
              pts_info->y[4] > bbox->y2)) {
    return 0.0;
  } else if (width * height < (25 * 25)) {
    return 0.0;
  } else if (pose != NULL) {
    float face_size = ((bbox->y2 - bbox->y1) + (bbox->x2 - bbox->x1)) / 2;
    float size_score = 0;
    float pose_score = 1. - (ABS(pose->yaw) + ABS(pose->pitch) + ABS(pose->roll) * 0.5) / 3.;
    // printf("pose_score_angle: %f, pose->yaw: %f, pose->pitch: %f, pose->roll: %f\n", pose_score,
    // pose->yaw, pose->pitch, pose->roll);

    float area_score;
    float wpose = 0.8;
    float wsize = 0.2;

    float h_ratio = face_size / (float)img_h;

    if (h_ratio < 0.06) {  // 64/1080
      wpose = 0.4;
      area_score = 0;
    } else if (h_ratio < 0.0685)  // 74/1080
    {
      wpose = 0.6;
      // area_score = log(face_size/(float)img_h)/log(4.0);
      area_score = log(h_ratio * 20.0) / log(4.0);
      if (pose_score > 0.8) {
        pose_score = 0.8;
      }
      size_score = 0.75;

    } else {
      area_score =
          0.23 + (2.0 - 1.0 / (h_ratio * 4.38 + 0.2)) / 5.0;  // 0.23 ~= log(0.0685*20.0)/log(4.0)
      size_score = eye_size / (bbox->x2 - bbox->x1);
      size_score += mouth_size / (bbox->x2 - bbox->x1);
    }
    if (fl_model && h_ratio > 0.06) {
      wpose = 0.8;
    }

    float velscore = vel * 0.04;
    if (velscore > 0.2) {
      velscore = 0.2;
    }

    // printf("img_h: %d, face_size:%f, size_score:%f, area_score: %f, vel:%f, velscore: %f,
    // blurness:%f\n", img_h, face_size,size_score , area_score, vel, velscore, blurness);

    pose_score = pose_score * wpose + wsize * size_score + area_score - blurness * 0.2;

    if (bbox->x1 < 0.5 * width || img_w - bbox->x2 < 0.5 * width || bbox->y1 < 0.5 * height ||
        img_h - bbox->y2 < 0.5 * height) {
      pose_score -= 0.2;
    }
    return pose_score;
  } else {
    return 0.5;
  }
}

void face_quality_assessment(VIDEO_FRAME_INFO_S *frame, cvtdl_face_t *face, bool *skip,
                             quality_assessment_e qa_method, float thr_laplacian) {
  /* NOTE: Make sure the coordinate is recovered by RetinaFace */
  for (uint32_t i = 0; i < face->size; i++) {
    if (skip != NULL && skip[i]) {
      face->info[i].pose_score = 0;
      LOGD("skip face:%u\n", i);
      continue;
    }
    face->info[i].blurness = 0;
    face->info[i].velx = 0;
    face->info[i].vely = 0;
    if (qa_method == AREA_RATIO) {
      face->info[i].pose_score = get_score(&face->info[i], face->width, face->height, false);
    } else if (qa_method == EYES_DISTANCE) {
      float dx = face->info[i].pts.x[0] - face->info[i].pts.x[1];
      float dy = face->info[i].pts.y[0] - face->info[i].pts.y[1];
      float dist_score = sqrt(dx * dx + dy * dy) / EYE_DISTANCE_STANDARD;
      face->info[i].pose_score = (dist_score >= 1.) ? 1. : dist_score;
    } else if (qa_method == LAPLACIAN) {
      static const float face_area = 112 * 112;
      static const float laplacian_threshold = 8.0; /* tune this value for different condition */
      float score;
      CVI_TDL_Face_Quality_Laplacian(frame, &face->info[i], &score);
      score /= face_area;
      score /= laplacian_threshold;
      if (score > 1.0) score = 1.0;
      face->info[i].pose_score = score;
    } else if (qa_method == MIX) {
      float score1, score2;
      cvtdl_bbox_t *bbox = &face->info[i].bbox;
      score1 = get_score(&face->info[i], face->width, face->height, false);
      face->info[i].pose_score = score1;
      if (score1 >= 0.1) {
        float face_area = (bbox->y2 - bbox->y1) * (bbox->x2 - bbox->x1);
        float laplacian_threshold =
            thr_laplacian; /* tune this value for different condition default:100 */
        CVI_TDL_Face_Quality_Laplacian(frame, &face->info[i], &score2);
        // score2 = sqrt(score2);
        float area_score = MIN(1.0, face_area / (90 * 90));
        score2 = score2 * area_score;
        if (score2 < laplacian_threshold) face->info[i].pose_score = 0.0;
        face->info[i].sharpness_score = score2;
      }
    }
  }
  return;
}

void encode_img2jpg(VENC_CHN VeChn, VIDEO_FRAME_INFO_S *src_frame, VIDEO_FRAME_INFO_S *crop_frame,
                    cvtdl_image_t *dst_image) {
  VENC_STREAM_S stStream;
  VENC_PACK_S *pstPack;
  VENC_CHN_ATTR_S stAttr;

  CVI_VENC_GetChnAttr(VeChn, &stAttr);
  stAttr.stVencAttr.u32PicHeight = src_frame->stVFrame.u32Height;
  stAttr.stVencAttr.u32PicWidth = src_frame->stVFrame.u32Width;
  CVI_VENC_SetChnAttr(VeChn, &stAttr);

  CVI_VENC_SendFrame(VeChn, src_frame, 2000);
  /*do jepg encode*/
  stStream.pstPack = malloc(sizeof(VENC_PACK_S) * 8);
  if (!stStream.pstPack) {
    LOGE("malloc pack fail \n");
    return;
  }

  CVI_VENC_GetStream(VeChn, &stStream, 2000);
  uint32_t total_len = 0;
  for (uint32_t i = 0; i < stStream.u32PackCount; i++) {
    pstPack = &stStream.pstPack[i];
    total_len += (pstPack->u32Len - pstPack->u32Offset);
  }

  if (dst_image->full_length != total_len) {
    if (dst_image->full_img != NULL) {
      free(dst_image->full_img);
      dst_image->full_img = NULL;
    }
    dst_image->full_img = (uint8_t *)malloc(total_len);
    dst_image->full_length = total_len;
  }

  for (uint32_t j = 0; j < stStream.u32PackCount; j++) {
    pstPack = &stStream.pstPack[j];
    memcpy(dst_image->full_img, pstPack->pu8Addr + pstPack->u32Offset, total_len);
  }

  uint32_t dst_h = crop_frame->stVFrame.u32Height;
  uint32_t dst_w = crop_frame->stVFrame.u32Width;
  if (dst_image->height != crop_frame->stVFrame.u32Height ||
      dst_image->width != crop_frame->stVFrame.u32Width) {
    CVI_TDL_Free(dst_image);
    CVI_TDL_CreateImage(dst_image, dst_h, dst_w, PIXEL_FORMAT_RGB_888);
  }
  CVI_TDL_Copy_VideoFrameToImage(crop_frame, dst_image);

  CVI_VENC_ReleaseStream(VeChn, &stStream);
  if (stStream.pstPack != NULL) {
    free(stStream.pstPack);
    stStream.pstPack = NULL;
  }
}

int image_to_video_frame(face_capture_t *face_cpt_info, cvtdl_image_t *image,
                         VIDEO_FRAME_INFO_S *dstFrame) {
  if (image->width > 256 || image->height > 256) {
    LOGE("crop image size over 256,w:%u,h:%u\n", image->width, image->height);
    return CVI_FAILURE;
  }
  memset(dstFrame, 0, sizeof(VIDEO_FRAME_INFO_S));
  VIDEO_FRAME_S *vFrame = &dstFrame->stVFrame;

  vFrame->enCompressMode = COMPRESS_MODE_NONE;
  vFrame->enPixelFormat = PIXEL_FORMAT_RGB_888;
  vFrame->enVideoFormat = VIDEO_FORMAT_LINEAR;
  vFrame->enColorGamut = COLOR_GAMUT_BT709;
  vFrame->u32TimeRef = 0;
  vFrame->u64PTS = 0;
  vFrame->enDynamicRange = DYNAMIC_RANGE_SDR8;

  vFrame->u32Width = image->width;
  vFrame->u32Height = image->height;
  vFrame->u32Stride[0] = image->stride[0];
  vFrame->u32Length[0] = image->length[0];

  vFrame->u64PhyAddr[0] = face_cpt_info->tmp_buf_physic_addr;
  vFrame->u64PhyAddr[1] = vFrame->u64PhyAddr[0] + vFrame->u32Length[0];
  vFrame->u64PhyAddr[2] = vFrame->u64PhyAddr[1] + vFrame->u32Length[1];
  vFrame->pu8VirAddr[0] = face_cpt_info->p_tmp_buf_addr;
  vFrame->pu8VirAddr[1] = vFrame->pu8VirAddr[0] + vFrame->u32Length[0];
  vFrame->pu8VirAddr[2] = vFrame->pu8VirAddr[1] + vFrame->u32Length[1];

  memcpy(vFrame->pu8VirAddr[0], image->pix[0], vFrame->u32Length[0]);
  CVI_SYS_IonFlushCache(vFrame->u64PhyAddr[0], vFrame->pu8VirAddr[0], vFrame->u32Length[0]);
  return CVI_SUCCESS;
}

int cal_crop_box(const float frame_width, const float frame_height, cvtdl_bbox_t crop_box,
                 cvtdl_bbox_t *p_dst_box) {
  cvtdl_bbox_t bbox = crop_box;
  int boxw = (bbox.x2 - bbox.x1);
  int boxh = (bbox.y2 - bbox.y1);

  int ctx = (bbox.x1 + bbox.x2) / 2;
  int cty = (bbox.y1 + bbox.y2) / 2;

  float halfscale = 0.65;
  // bbox new coordinate after extern
  int x1 = ctx - boxw * halfscale;
  int y1 = cty - boxh * halfscale;
  int x2 = ctx + boxw * halfscale;
  int y2 = cty + boxh * halfscale;

  cvtdl_bbox_t new_bbox;
  new_bbox.score = bbox.score;
  new_bbox.x1 = x1 > 0 ? x1 : 0;
  new_bbox.y1 = y1 > 0 ? y1 : 0;
  new_bbox.x2 = x2 < frame_width ? x2 : frame_width - 1;
  new_bbox.y2 = y2 < frame_height ? y2 : frame_height - 1;

  // float ratio, pad_width, pad_height;
  float box_height = new_bbox.y2 - new_bbox.y1;
  float box_width = new_bbox.x2 - new_bbox.x1;

  int mean_hw = (box_width + box_height) / 2;
  int dst_hw = 128;
  if (mean_hw > dst_hw) {
    dst_hw = 256;
  }
  *p_dst_box = new_bbox;
  return dst_hw;
}

float cal_crop_big_box(const float frame_width, const float frame_height, cvtdl_bbox_t crop_box,
                       cvtdl_bbox_t *p_dst_box, int *dts_w, int *dst_h) {
  cvtdl_bbox_t bbox = crop_box;
  int boxw = (bbox.x2 - bbox.x1);
  int boxh = (bbox.y2 - bbox.y1);

  // bbox new coordinate after extern
  int x1 = bbox.x1 - 0.8 * boxw;
  int y1 = bbox.y1 - 0.5 * boxh;
  int x2 = bbox.x2 + 0.8 * boxw;
  int y2 = bbox.y1 + 2.0 * boxh;

  cvtdl_bbox_t new_bbox;
  new_bbox.score = bbox.score;
  new_bbox.x1 = x1 > 0 ? x1 : 0;
  new_bbox.y1 = y1 > 0 ? y1 : 0;
  new_bbox.x2 = x2 < frame_width ? x2 : frame_width - 1;
  new_bbox.y2 = y2 < frame_height ? y2 : frame_height - 1;

  *p_dst_box = new_bbox;

  float box_height = new_bbox.y2 - new_bbox.y1;
  float box_width = new_bbox.x2 - new_bbox.x1;

  int new_width = ((int)box_width / 64 + 1) * 64;
  if (new_width > 256) {
    new_width = 256;
  }

  float scale = box_width / (float)new_width;
  int new_height = box_height / scale;

  *dts_w = new_width;
  *dst_h = new_height;

  return scale;
}

int update_extend_resize_info(const float frame_width, const float frame_height,
                              cvtdl_face_info_t *face_info, cvtdl_bbox_t *p_dst_box,
                              bool update_info) {
  cvtdl_bbox_t bbox = face_info->bbox;
  int boxw = (bbox.x2 - bbox.x1);
  int boxh = (bbox.y2 - bbox.y1);

  int ctx = (bbox.x1 + bbox.x2) / 2;
  int cty = (bbox.y1 + bbox.y2) / 2;

  float halfscale = 0.65;
  // bbox new coordinate after extern
  int x1 = ctx - boxw * halfscale;
  int y1 = cty - boxh * halfscale;
  int x2 = ctx + boxw * halfscale;
  int y2 = cty + boxh * halfscale;

  cvtdl_bbox_t new_bbox;
  new_bbox.score = bbox.score;
  new_bbox.x1 = x1 > 0 ? x1 : 0;
  new_bbox.y1 = y1 > 0 ? y1 : 0;
  new_bbox.x2 = x2 < frame_width ? x2 : frame_width - 1;
  new_bbox.y2 = y2 < frame_height ? y2 : frame_height - 1;

  // float ratio, pad_width, pad_height;
  float box_height = new_bbox.y2 - new_bbox.y1;
  float box_width = new_bbox.x2 - new_bbox.x1;

  int mean_hw = (box_width + box_height) / 2;
  int dst_hw = 128;
  if (mean_hw > dst_hw) {
    dst_hw = 256;
  }
  *p_dst_box = new_bbox;
  if (update_info) {
    float ratio_h = dst_hw / box_height;
    float ratio_w = dst_hw / box_width;
    for (uint32_t j = 0; j < face_info->pts.size; ++j) {
      face_info->pts.x[j] = (face_info->pts.x[j] - new_bbox.x1) * ratio_w;
      face_info->pts.y[j] = (face_info->pts.y[j] - new_bbox.y1) * ratio_h;
    }
    face_info->bbox.x1 = (face_info->bbox.x1 - new_bbox.x1) * ratio_w;
    face_info->bbox.y1 = (face_info->bbox.y1 - new_bbox.y1) * ratio_h;
    face_info->bbox.x2 = (face_info->bbox.x2 - new_bbox.x1) * ratio_w;
    face_info->bbox.y2 = (face_info->bbox.y2 - new_bbox.y1) * ratio_h;
  }

  return dst_hw;
}

CVI_S32 extract_cropped_face(const cvitdl_handle_t tdl_handle, face_capture_t *face_cpt_info) {
  for (uint32_t i = 0; i < face_cpt_info->size; i++) {
    if (face_cpt_info->_output[i] && face_cpt_info->cfg.img_capture_flag == 0) {
      int ret = CVI_TDL_FaceFeatureExtract(
          tdl_handle, face_cpt_info->data[i].image.pix[0], face_cpt_info->data[i].image.width,
          face_cpt_info->data[i].image.height, face_cpt_info->data[i].image.stride[0],
          &face_cpt_info->data[i].info);
      LOGI("extract face feature,trackid:%d,ret:%d\n", (int)face_cpt_info->data[i].info.unique_id,
           ret);
    }
  }
  return CVI_SUCCESS;
}

CVI_S32 _FaceCapture_SetMode(face_capture_t *face_cpt_info, capture_mode_e mode) {
  face_cpt_info->mode = mode;
  return CVI_TDL_SUCCESS;
}

CVI_S32 _FaceCapture_Init(face_capture_t **face_cpt_info, uint32_t buffer_size) {
  if (*face_cpt_info != NULL) {
    LOGW("[APP::FaceCapture] already exist.\n");
    return CVI_TDL_SUCCESS;
  }
  LOGI("[APP::FaceCapture] Initialize (Buffer Size: %u)\n", buffer_size);
  face_capture_t *new_face_cpt_info = (face_capture_t *)malloc(sizeof(face_capture_t));
  memset(new_face_cpt_info, 0, sizeof(face_capture_t));
  new_face_cpt_info->size = buffer_size;
  new_face_cpt_info->data = (face_cpt_data_t *)malloc(sizeof(face_cpt_data_t) * buffer_size);
  memset(new_face_cpt_info->data, 0, sizeof(face_cpt_data_t) * buffer_size);

  new_face_cpt_info->_output = (bool *)malloc(sizeof(bool) * buffer_size);
  memset(new_face_cpt_info->_output, 0, sizeof(bool) * buffer_size);

  _FaceCapture_GetDefaultConfig(&new_face_cpt_info->cfg);
  new_face_cpt_info->_m_limit = MEMORY_LIMIT;

  uint32_t tmp_size = 256 * 256 * 3;
  int ret = CVI_SYS_IonAlloc(&(new_face_cpt_info->tmp_buf_physic_addr),
                             (CVI_VOID **)&(new_face_cpt_info->p_tmp_buf_addr), "cvitdl/tmp_crop",
                             tmp_size);
  if (ret != CVI_SUCCESS) {
    printf("alloc ion failed,ret:%d\n", ret);
    return ret;
  }
  *face_cpt_info = new_face_cpt_info;

  return ret;
}

CVI_S32 _FaceCapture_GetDefaultConfig(face_capture_config_t *cfg) {
  cfg->miss_time_limit = MISS_TIME_LIMIT;
  cfg->thr_size_min = SIZE_MIN_THRESHOLD;
  cfg->thr_size_max = SIZE_MAX_THRESHOLD;
  cfg->qa_method = QUALITY_ASSESSMENT_METHOD;
  cfg->thr_quality = QUALITY_THRESHOLD;
  cfg->thr_yaw = 0.7;
  cfg->thr_pitch = 0.5;
  cfg->thr_roll = 0.65;
  cfg->thr_laplacian = 100;
  cfg->m_interval = 45;
  cfg->m_capture_num = 3;
  cfg->auto_m_fast_cap = false;
  cfg->img_capture_flag = 0;
  cfg->eye_dist_thresh = 20;
  cfg->landmark_score_thresh = 0.5;
  cfg->venc_channel_id = 1;
  return CVI_TDL_SUCCESS;
}

CVI_S32 _FaceCapture_SetConfig(face_capture_t *face_cpt_info, face_capture_config_t *cfg,
                               cvitdl_handle_t tdl_handle) {
  memcpy(&face_cpt_info->cfg, cfg, sizeof(face_capture_config_t));
  if (cfg->img_capture_flag) {
    face_capture_init_venc(cfg->venc_channel_id);
  }
  cvtdl_deepsort_config_t deepsort_conf;
  CVI_TDL_DeepSORT_GetConfig(tdl_handle, &deepsort_conf, -1);
  deepsort_conf.ktracker_conf.max_unmatched_num = cfg->miss_time_limit;
  CVI_TDL_DeepSORT_SetConfig(tdl_handle, &deepsort_conf, -1, true);
  SHOW_CONFIG(&face_cpt_info->cfg);
  return CVI_TDL_SUCCESS;
}

CVI_S32 _FaceCapture_CleanAll(face_capture_t *face_cpt_info) {
  if (face_cpt_info->cfg.img_capture_flag) {
    if (venc_extern_init == 0) {
      release_venc(face_cpt_info->cfg.venc_channel_id);
    }
  }

  /* Release tracking data */
  for (uint32_t j = 0; j < face_cpt_info->size; j++) {
    if (face_cpt_info->data[j].state != IDLE) {
      LOGI("[APP::FaceCapture] Clean Face Info[%u]\n", j);
      CVI_TDL_Free(&face_cpt_info->data[j].image);
      CVI_TDL_Free(&face_cpt_info->data[j].info);
      face_cpt_info->data[j].state = IDLE;
    }
  }
  return CVI_TDL_SUCCESS;
}

CVI_S32 update_data(cvitdl_handle_t tdl_handle, face_capture_t *face_cpt_info,
                    VIDEO_FRAME_INFO_S *frame, cvtdl_face_t *face_meta,
                    cvtdl_tracker_t *tracker_meta, bool with_pts) {
  LOGI("[APP::FaceCapture] Update Data\n");
  for (uint32_t i = 0; i < face_meta->size; i++) {
    /* we only consider the stable tracker in this sample code. */
    if (face_meta->info[i].track_state != CVI_TRACKER_STABLE) {
      for (uint32_t j = 0; j < face_cpt_info->size; j++) {
        if (face_cpt_info->data[j].info.unique_id == face_meta->info[i].unique_id) {
          face_cpt_info->data[j].cap_timestamp = face_cpt_info->_time;
        }
      }
      continue;
    }
    // if is low quality face,do not generate capture data
    bool toskip = face_meta->info[i].face_quality < face_cpt_info->cfg.thr_quality;
    float eye_dist_x = face_meta->info[i].pts.x[0] - face_meta->info[i].pts.x[1];
    float eye_dist_y = face_meta->info[i].pts.y[0] - face_meta->info[i].pts.y[1];
    float eye_dist = sqrt(eye_dist_x * eye_dist_x + eye_dist_y * eye_dist_y);
    bool skip_dist = eye_dist < face_cpt_info->cfg.eye_dist_thresh * 0.8;

    uint64_t trk_id = face_meta->info[i].unique_id;
    LOGD("to update_data,trackid:%d,quality:%.3f,pscore:%.3f\n", (int)trk_id,
         face_meta->info[i].face_quality, face_meta->info[i].pose_score);

    if (toskip || (with_pts && skip_dist)) {
      LOGD("update_data,skip to generate capture data,trackid:%d,pscore:%f,eyedist:%.3f\n",
           (int)face_meta->info[i].unique_id, face_meta->info[i].pose_score, eye_dist);
      continue;
    }

    /* check whether the tracker id exist or not. */
    int match_idx = -1;
    int idle_idx = -1;
    int update_idx = -1;
    for (uint32_t j = 0; j < face_cpt_info->size; j++) {
      if (face_cpt_info->data[j].state == ALIVE &&
          face_cpt_info->data[j].info.unique_id == trk_id) {
        match_idx = (int)j;
        break;
      }
    }

    if (match_idx != -1) {
      float current_quality = face_cpt_info->data[match_idx].info.face_quality + 0.05;
      if (face_meta->info[i].face_quality > current_quality) {
        update_idx = match_idx;
      } else {
        LOGD("curqu:%.3f,facequa:%.3f,posescore1:%.3f\n", current_quality,
             face_meta->info[i].face_quality, face_cpt_info->data[match_idx].info.pose_score1);
      }
    } else {
      for (uint32_t j = 0; j < face_cpt_info->size; j++) {
        if (face_cpt_info->data[j].state == IDLE) {
          idle_idx = (int)j;
          update_idx = idle_idx;
          break;
        }
      }
    }

    if (match_idx == -1 && idle_idx == -1) {
      LOGD("no valid buffer\n");
      continue;
    }
    if (update_idx == -1) {
      continue;
    }
    bool send_ok = face_cpt_info->cfg.m_capture_num != 0 &&
                   face_cpt_info->data[update_idx]._out_counter >= face_cpt_info->cfg.m_capture_num;
    if (send_ok) {
      continue;
    }

    cvtdl_bbox_t crop_big_box;
    int dst_w = 0;
    int dst_h = 0;
    float scale = cal_crop_big_box(
        frame->stVFrame.u32Width, frame->stVFrame.u32Height, face_meta->info[i].bbox, &crop_big_box,
        &dst_w, &dst_h);  // crop_big_box为外扩后的当前人脸框坐标，dst_w
                          // dst_h为外扩后的64位对齐的宽和高，scale为外扩后的宽与64位对齐宽的比值
    VIDEO_FRAME_INFO_S *crop_big_frame = NULL;
    int ret = CVI_TDL_CropResizeImage(tdl_handle, face_cpt_info->fl_model, frame, &crop_big_box,
                                      dst_w, dst_h, PIXEL_FORMAT_RGB_888,
                                      &crop_big_frame);  // crop_big_frame为外扩后的大人脸图
    if (ret != CVI_SUCCESS) {
      LOGE("skip crop failed\n");
      if (crop_big_frame != NULL)
        CVI_TDL_Release_VideoFrame(tdl_handle, face_cpt_info->fl_model, crop_big_frame, true);
      continue;
    }

    cvtdl_bbox_t scale_box;  // 人脸图相对于crop_big_frame的坐标
    scale_box.x1 = (face_meta->info[i].bbox.x1 - crop_big_box.x1) / scale;
    scale_box.y1 = (face_meta->info[i].bbox.y1 - crop_big_box.y1) / scale;
    scale_box.x2 = (face_meta->info[i].bbox.x2 - crop_big_box.x1) / scale;
    scale_box.y2 = (face_meta->info[i].bbox.y2 - crop_big_box.y1) / scale;
    scale_box.score = face_meta->info[i].bbox.score;
    cvtdl_bbox_t crop_box;  // crop_box为小幅度外扩的人脸坐标（相对于crop_big_box）
    int dst_wh = cal_crop_box(crop_big_frame->stVFrame.u32Width, crop_big_frame->stVFrame.u32Height,
                              scale_box, &crop_box);
    VIDEO_FRAME_INFO_S *crop_frame = NULL;
    ret = CVI_TDL_CropResizeImage(tdl_handle, face_cpt_info->fl_model, crop_big_frame, &crop_box,
                                  dst_wh, dst_wh, PIXEL_FORMAT_RGB_888, &crop_frame);
    // printf("to process,index:%d,face:%d\n",update_idx,(int)i);
    if (ret != CVI_SUCCESS) {
      LOGE("skip crop failed\n");
      if (crop_frame != NULL)
        CVI_TDL_Release_VideoFrame(tdl_handle, face_cpt_info->fl_model, crop_frame, true);
      continue;
    }

    // cvtdl_bbox_t scale_box2;  // 人脸图相对于crop_frame的坐标
    // scale_box2.x1 = (scale_box.x1 - crop_box.x1) / ((crop_box.x2 - crop_box.x1) / dst_wh);
    // scale_box2.y1 = (scale_box.y1 - crop_box.y1) / ((crop_box.y2 - crop_box.y1) / dst_wh);
    // scale_box2.x2 = (scale_box.x2 - crop_box.x1) / ((crop_box.x2 - crop_box.x1) / dst_wh);
    // scale_box2.y2 = (scale_box.y2 - crop_box.y1) / ((crop_box.y2 - crop_box.y1) / dst_wh);
    // scale_box2.score = face_meta->info[i].bbox.score;

    cvtdl_bbox_t origin_box;  // 人脸属性分类使用稍微外扩的人脸图，效果较好
    origin_box.x1 = 1;
    origin_box.y1 = 1;
    origin_box.x2 = crop_frame->stVFrame.u32Width - 1;
    origin_box.y2 = crop_frame->stVFrame.u32Height - 1;
    origin_box.score = face_meta->info[i].bbox.score;

    cvtdl_face_t obj_meta = {0};
    if (face_cpt_info->fl_model == CVI_TDL_SUPPORTED_MODEL_LANDMARK_DET3) {
      ret = CVI_TDL_FLDet3(tdl_handle, crop_frame, &obj_meta);
    } else {
      ret = CVI_TDL_FaceLandmarkerDet2(tdl_handle, crop_frame, &obj_meta);
    }
    if (ret != 0) {
      LOGE("det3 failed\n");
      CVI_TDL_Release_VideoFrame(tdl_handle, face_cpt_info->fl_model, crop_frame, true);
      CVI_TDL_Release_VideoFrame(tdl_handle, face_cpt_info->fl_model, crop_big_frame, true);
      continue;
    }

    float *ori_pts_x = NULL;
    float *ori_pts_y = NULL;
    if (face_cpt_info->fr_flag == 2) {
      ori_pts_x = (float *)malloc(obj_meta.info[0].pts.size * sizeof(float));
      ori_pts_y = (float *)malloc(obj_meta.info[0].pts.size * sizeof(float));
      memcpy(ori_pts_x, obj_meta.info[0].pts.x, sizeof(float) * obj_meta.info[0].pts.size);
      memcpy(ori_pts_y, obj_meta.info[0].pts.y, sizeof(float) * obj_meta.info[0].pts.size);
    }

    float boxw = crop_box.x2 - crop_box.x1;
    float boxh = crop_box.y2 - crop_box.y1;
    float scalew = boxw / dst_wh;
    float scaleh = boxh / dst_wh;
    obj_meta.info[0].bbox = crop_box;
    for (int i = 0; i < 5; i++) {
      obj_meta.info[0].pts.x[i] =
          (obj_meta.info[0].pts.x[i] * scalew + crop_box.x1) * scale + crop_big_box.x1;
      obj_meta.info[0].pts.y[i] =
          (obj_meta.info[0].pts.y[i] * scaleh + crop_box.y1) * scale + crop_big_box.y1;
    }

    eye_dist_x = obj_meta.info[0].pts.x[1] - obj_meta.info[0].pts.x[0];
    eye_dist_y = obj_meta.info[0].pts.y[0] - obj_meta.info[0].pts.y[1];
    eye_dist = sqrt(eye_dist_x * eye_dist_x + eye_dist_y * eye_dist_y);
    if (eye_dist < face_cpt_info->cfg.eye_dist_thresh ||
        obj_meta.info[0].pts.score < face_cpt_info->cfg.landmark_score_thresh) {
      LOGD("skip track:%d,eyedist:%.3f,ptscore:%.3f\n", (int)trk_id, eye_dist,
           obj_meta.info[0].pts.score);
      CVI_TDL_Release_VideoFrame(tdl_handle, face_cpt_info->fl_model, crop_frame, true);
      CVI_TDL_Release_VideoFrame(tdl_handle, face_cpt_info->fl_model, crop_big_frame, true);
      CVI_TDL_Free(&obj_meta);
      free(ori_pts_x);
      free(ori_pts_y);
      continue;
    }

    cvtdl_head_pose_t pose;
    CVI_TDL_Service_FaceAngle(&obj_meta.info[0].pts, &pose);
    face_meta->info[i].head_pose.pitch = pose.pitch;
    face_meta->info[i].head_pose.yaw = pose.yaw;
    face_meta->info[i].head_pose.roll = pose.roll;

    face_meta->info[i].blurness = obj_meta.info[0].blurness;

    for (int k = 0; k < 5; k++) {
      face_meta->info[i].pts.x[k] = obj_meta.info[0].pts.x[k];
      face_meta->info[i].pts.y[k] = obj_meta.info[0].pts.y[k];
    }

    face_meta->info[i].pose_score1 =
        get_score(&face_meta->info[i], face_meta->width, face_meta->height, true);

    if (face_meta->info[i].pose_score1 <= face_cpt_info->data[update_idx].info.pose_score1) {
      LOGD("skip track:%d,pose_score1:%.3f\n", (int)trk_id, face_meta->info[i].pose_score1);
      CVI_TDL_Release_VideoFrame(tdl_handle, face_cpt_info->fl_model, crop_frame, true);
      CVI_TDL_Release_VideoFrame(tdl_handle, face_cpt_info->fl_model, crop_big_frame, true);
      CVI_TDL_Free(&obj_meta);
      free(ori_pts_x);
      free(ori_pts_y);
      continue;
    }
    // quality satisfied,update data

    if (match_idx == -1) {
      memcpy(&face_cpt_info->data[update_idx].info, &face_meta->info[i], sizeof(cvtdl_face_info_t));
      face_cpt_info->data[update_idx].info.pts.size = 5;
      face_cpt_info->data[update_idx].info.pts.x = (float *)malloc(5 * sizeof(float));
      face_cpt_info->data[update_idx].info.pts.y = (float *)malloc(5 * sizeof(float));
      face_cpt_info->data[update_idx]._timestamp =
          face_cpt_info->_time;  // new unique_id, now maybe face_cpt_info->_time subtract
                                 // _timestamp (since _timestamp defult 0) is a large number, update
                                 // _timestamp to avoid  output directly
    } else {
      int8_t *p_feature = face_cpt_info->data[update_idx].info.feature.ptr;
      float *p_pts_x = face_cpt_info->data[update_idx].info.pts.x;
      float *p_pts_y = face_cpt_info->data[update_idx].info.pts.y;
      // store matched name
      memcpy(face_meta->info[i].name, face_cpt_info->data[update_idx].info.name,
             sizeof(face_cpt_info->data[update_idx].info.name));
      memcpy(&face_cpt_info->data[update_idx].info, &face_meta->info[i], sizeof(cvtdl_face_info_t));
      face_cpt_info->data[update_idx].info.feature.ptr = p_feature;
      face_cpt_info->data[update_idx].info.pts.x = p_pts_x;
      face_cpt_info->data[update_idx].info.pts.y = p_pts_y;
    }

    face_cpt_info->data[update_idx].state = ALIVE;

    if (face_cpt_info->fr_flag == 2) {
      memcpy(face_cpt_info->data[update_idx].info.pts.x, ori_pts_x,
             sizeof(float) * obj_meta.info[0].pts.size);
      memcpy(face_cpt_info->data[update_idx].info.pts.y, ori_pts_y,
             sizeof(float) * obj_meta.info[0].pts.size);

    } else {
      memcpy(face_cpt_info->data[update_idx].info.pts.x, obj_meta.info[0].pts.x, sizeof(float) * obj_meta.info[0].pts.size);
      memcpy(face_cpt_info->data[update_idx].info.pts.y, obj_meta.info[0].pts.y, sizeof(float) * obj_meta.info[0].pts.size);
    }
    free(ori_pts_x);
    free(ori_pts_y);

    face_cpt_info->data[update_idx]._capture = true;

    if (face_cpt_info->cfg.img_capture_flag == 0 || face_cpt_info->cfg.img_capture_flag == 1) {
      // face_cpt_info->data[update_idx].crop_face_box = scale_box2;
      face_cpt_info->data[update_idx].crop_face_box = origin_box;

    } else if (face_cpt_info->cfg.img_capture_flag == 2 ||
               face_cpt_info->cfg.img_capture_flag == 3) {
      // face_cpt_info->data[update_idx].crop_face_box = scale_box;
      face_cpt_info->data[update_idx].crop_face_box = crop_box;
    }

    if (face_cpt_info->cfg.img_capture_flag == 1 ||
        face_cpt_info->cfg.img_capture_flag == 3) {  // 抓拍模式
      VIDEO_FRAME_INFO_S *p_frame = frame;
      VENC_CHN VeChn = face_cpt_info->cfg.venc_channel_id;
      uint8_t yuv_fmt = false;
      PIXEL_FORMAT_E fmt = frame->stVFrame.enPixelFormat;
      if (fmt != PIXEL_FORMAT_NV21 && fmt != PIXEL_FORMAT_YUV_PLANAR_420) {
        CVI_TDL_Change_Img(tdl_handle, face_cpt_info->fd_model, frame, &p_frame, fmt);
        yuv_fmt = true;
      }
      // TODO: crop image is RGB_PACKED, use venc hardware must be nv21 format.
      // therefore save the source croped image.
      if (face_cpt_info->cfg.img_capture_flag == 1) {
        encode_img2jpg(VeChn, p_frame, crop_frame, &face_cpt_info->data[update_idx].image);
      } else {
        encode_img2jpg(VeChn, p_frame, crop_big_frame, &face_cpt_info->data[update_idx].image);
      }
      if (yuv_fmt) {
        CVI_TDL_Delete_Img(tdl_handle, face_cpt_info->fd_model, p_frame);
      }
    } else {
      if (face_cpt_info->cfg.img_capture_flag == 0) {
        dst_h = dst_wh;
        dst_w = dst_wh;
      }
      if (face_cpt_info->data[update_idx].image.width != dst_w ||
          face_cpt_info->data[update_idx].image.height != dst_h) {
        CVI_TDL_Free(&face_cpt_info->data[update_idx].image);
        CVI_TDL_CreateImage(&face_cpt_info->data[update_idx].image, dst_h, dst_w,
                            PIXEL_FORMAT_RGB_888);
      }
      if (face_cpt_info->cfg.img_capture_flag == 2) {
        ret =
            CVI_TDL_Copy_VideoFrameToImage(crop_big_frame, &face_cpt_info->data[update_idx].image);
      } else {
        ret = CVI_TDL_Copy_VideoFrameToImage(crop_frame, &face_cpt_info->data[update_idx].image);
      }
    }

    face_cpt_info->data[update_idx].cap_timestamp =
        face_cpt_info->_time;  // to update cap_timestamp
    face_cpt_info->data[update_idx].info.face_quality = face_meta->info[i].face_quality;
    memcpy(face_meta->info[i].name, face_cpt_info->data[update_idx].info.name,
           sizeof(face_cpt_info->data[update_idx].info.name));

    CVI_TDL_Release_VideoFrame(tdl_handle, face_cpt_info->fl_model, crop_frame, true);
    CVI_TDL_Release_VideoFrame(tdl_handle, face_cpt_info->fl_model, crop_big_frame, true);
    CVI_TDL_Free(&obj_meta);
  }

  for (uint32_t j = 0; j < face_cpt_info->size; j++) {
    bool found = false;
    for (uint32_t k = 0; k < tracker_meta->size; k++) {
      if (face_cpt_info->data[j].info.unique_id == tracker_meta->info[k].id) {
        found = true;
        break;
      }
    }

    if (!found && face_cpt_info->data[j].info.unique_id != 0) {
      LOGD("to delete track:%u\n", (uint32_t)face_cpt_info->data[j].info.unique_id);
      face_cpt_info->data[j].miss_counter = face_cpt_info->cfg.miss_time_limit;
    }
  }

  return CVI_TDL_SUCCESS;
}

CVI_S32 clean_data(face_capture_t *face_cpt_info) {
  for (uint32_t j = 0; j < face_cpt_info->size; j++) {
    // printf("buf:%u,trackid:%d,state:%d\n",j,(int)face_cpt_info->data[j].info.unique_id,face_cpt_info->data[j].state);
    if (face_cpt_info->data[j].state == MISS) {
      LOGI("[APP::FaceCapture] Clean Face Info[%u]\n", j);
      CVI_TDL_Free(&face_cpt_info->data[j].image);
      CVI_TDL_Free(&face_cpt_info->data[j].info);
      // memset(&face_cpt_info->data[j].info,0,sizeof(face_cpt_info->data[j].info));
      face_cpt_info->data[j].info.unique_id = 0;
      face_cpt_info->data[j].info.face_quality = 0;
      face_cpt_info->data[j].info.pose_score1 = 0;

      face_cpt_info->data[j].state = IDLE;
      face_cpt_info->data[j].miss_counter = 0;
      face_cpt_info->data[j]._out_counter = 0;
      // face_cpt_info->data[j]._capture = false;
    }
  }
  return CVI_TDL_SUCCESS;
}

CVI_S32 update_output_state(cvitdl_handle_t tdl_handle, face_capture_t *face_cpt_info) {
  for (uint32_t j = 0; j < face_cpt_info->size; j++) {
    // only process alive track and good face quality face
    if (face_cpt_info->data[j].state != ALIVE) {
      continue;
    }
#ifdef DEBUG_TRACK
    printf("to update output state,track:%d,miss:%d,out:%d,quality:%f,thresh:%f\n",
           (int)face_cpt_info->data[j].info.unique_id, (int)face_cpt_info->data[j].miss_counter,
           face_cpt_info->data[j]._out_counter, face_cpt_info->data[j].info.face_quality,
           face_cpt_info->cfg.thr_quality);
#endif
    bool num_ok = face_cpt_info->cfg.m_capture_num == 0 ||
                  (face_cpt_info->cfg.m_capture_num != 0 &&
                   face_cpt_info->data[j]._out_counter < face_cpt_info->cfg.m_capture_num);

    if (face_cpt_info->data[j].miss_counter >= face_cpt_info->cfg.miss_time_limit) {
      face_cpt_info->data[j].state = MISS;
      if (face_cpt_info->data[j].info.face_quality < face_cpt_info->cfg.thr_quality) continue;
      if (face_cpt_info->data[j].last_cap_timestamp == face_cpt_info->data[j].cap_timestamp)
        continue;
      // printf("to output miss track:%d\n", (int)face_cpt_info->data[j].info.unique_id);
      if (face_cpt_info->mode == AUTO &&
          num_ok) {  // when leaving,auto mode would output left best face
        face_cpt_info->_output[j] = true;
      } else if ((face_cpt_info->mode == FAST || face_cpt_info->mode == CYCLE) &&
                 face_cpt_info->data[j]._out_counter == 0) {
        face_cpt_info->_output[j] = true;
      }
    } else if (face_cpt_info->data[j].info.face_quality >= face_cpt_info->cfg.thr_quality) {
      int _time = face_cpt_info->_time - face_cpt_info->data[j]._timestamp;
      if (face_cpt_info->mode == AUTO) {
        if (face_cpt_info->cfg.auto_m_fast_cap && _time >= face_cpt_info->cfg.m_interval &&
            face_cpt_info->data[j]._out_counter < 1) {
          face_cpt_info->_output[j] = true;
        }
        if (num_ok && _time >= face_cpt_info->cfg.m_interval) {
          /* Time's up */
          face_cpt_info->_output[j] = true;
          // printf("output,interval:%d,capts:%d,ts:%d\n", _time,
          //       (int)face_cpt_info->data[j].cap_timestamp, (int)face_cpt_info->_time);
        }
      } else if (face_cpt_info->mode == FAST) {
        if (face_cpt_info->data[j]._out_counter < 1) {
          face_cpt_info->_output[j] = true;
        } else if (num_ok && _time >= face_cpt_info->cfg.m_interval) {
          face_cpt_info->_output[j] = true;
        }
      } else if (face_cpt_info->mode == CYCLE && _time >= face_cpt_info->cfg.m_interval && num_ok) {
        face_cpt_info->_output[j] = true;

        LOGD("update output flag,interval:%u\n", face_cpt_info->cfg.m_interval);
      }
    }
    if (face_cpt_info->_output[j]) {
      // if (_FaceCapture_FilterWithLandmark(tdl_handle, face_cpt_info, j)) {
      //   face_cpt_info->_output[j] = false;
      //   face_cpt_info->data[j].info.face_quality = 0;
      //   continue;
      // }

#ifdef DEBUG_TRACK
      printf("to output track:%d,miss:%d,out:%d,quality:%f,thresh:%f\n",
             (int)face_cpt_info->data[j].info.unique_id, (int)face_cpt_info->data[j].miss_counter,
             face_cpt_info->data[j]._out_counter, face_cpt_info->data[j].info.face_quality,
             face_cpt_info->cfg.thr_quality);
#endif
      face_cpt_info->data[j]._timestamp = face_cpt_info->_time;  // update output timestamp
      face_cpt_info->data[j]._out_counter += 1;
      face_cpt_info->data[j].last_cap_timestamp = face_cpt_info->data[j].cap_timestamp;
    }
  }
  return CVI_SUCCESS;
}

CVI_S32 _FaceCapture_Free(face_capture_t *face_cpt_info) {
  LOGI("[APP::FaceCapture] Free FaceCapture Data\n");
  if (face_cpt_info != NULL) {
    _FaceCapture_CleanAll(face_cpt_info);

    free(face_cpt_info->data);
    CVI_TDL_Free(&face_cpt_info->last_faces);
    CVI_TDL_Free(&face_cpt_info->last_trackers);
    CVI_TDL_Free(&face_cpt_info->last_objects);
    CVI_TDL_Free(&face_cpt_info->pet_objects);
    free(face_cpt_info->_output);

    if (face_cpt_info->tmp_buf_physic_addr != 0) {
      CVI_SYS_IonFree(face_cpt_info->tmp_buf_physic_addr, face_cpt_info->p_tmp_buf_addr);
    }
    free(face_cpt_info);
  }
  return CVI_TDL_SUCCESS;
}

void SHOW_CONFIG(face_capture_config_t *cfg) {
  printf("@@@ Face Capture Config @@@\n");
  printf(" - Miss Time Limit:   : %u\n", cfg->miss_time_limit);
  printf(" - Thr Size (Min)     : %i\n", cfg->thr_size_min);
  printf(" - Thr Size (Max)     : %i\n", cfg->thr_size_max);
  printf(" - Quality Assessment Method : %i\n", cfg->qa_method);
  printf(" - Thr Quality        : %.2f\n", cfg->thr_quality);
  printf(" - Thr Yaw    : %.2f\n", cfg->thr_yaw);
  printf(" - Thr Pitch  : %.2f\n", cfg->thr_pitch);
  printf(" - Thr Roll   : %.2f\n", cfg->thr_roll);
  printf("Interval     : %u\n", cfg->m_interval);
  printf("Capture Num  : %u\n", cfg->m_capture_num);
  printf("[Auto] Fast Capture : %s\n\n", cfg->auto_m_fast_cap ? "True" : "False");
  printf(" - Image Capture Flag : %d\n\n", cfg->img_capture_flag);
  printf(" - Store Face Feature   : %s\n\n", cfg->store_feature ? "True" : "False");
  return;
}

uint32_t get_time_in_ms() {
  struct timeval tv;
  if (gettimeofday(&tv, NULL) < 0) {
    return 0;
  }
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

void set_skipFQsignal(face_capture_t *face_cpt_info, cvtdl_face_t *face_meta, bool *skip) {
  memset(skip, 0, sizeof(bool) * face_meta->size);
  bool care_size_min = face_cpt_info->cfg.thr_size_min != -1;
  bool care_size_max = face_cpt_info->cfg.thr_size_max != -1;
  if (!care_size_min && !care_size_max) return;

  for (uint32_t i = 0; i < face_meta->size; i++) {
    float h = face_meta->info[i].bbox.y2 - face_meta->info[i].bbox.y1;
    float w = face_meta->info[i].bbox.x2 - face_meta->info[i].bbox.x1;
    if (care_size_min) {
      if (h < (float)face_cpt_info->cfg.thr_size_min ||
          w < (float)face_cpt_info->cfg.thr_size_min) {
        skip[i] = true;
        continue;
      }
    }
    if (care_size_max) {
      if (h > (float)face_cpt_info->cfg.thr_size_max ||
          w > (float)face_cpt_info->cfg.thr_size_max) {
        skip[i] = true;
        continue;
      }
    }
    if (ABS(face_meta->info[i].head_pose.yaw) > face_cpt_info->cfg.thr_yaw ||
        ABS(face_meta->info[i].head_pose.pitch) > face_cpt_info->cfg.thr_pitch ||
        ABS(face_meta->info[i].head_pose.roll) > face_cpt_info->cfg.thr_roll) {
      skip[i] = true;
    }
  }
}