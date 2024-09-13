#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "core/utils/vpss_helper.h"
#include "cvi_tdl.h"
#include "cvi_tdl_media.h"

int ReleaseImage(VIDEO_FRAME_INFO_S *frame) {
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
void export_img_result(const char *sz_dstf, cvtdl_object_t *p_objinfo, int imgw, int imgh) {
  FILE *fp = fopen(sz_dstf, "w");

  for (uint32_t i = 0; i < p_objinfo->size; i++) {
    // if(p_objinfo->info[i].unique_id != 0){
    // sprintf( buf, "\nOD DB File Size = %" PRIu64 " bytes \t"
    char szinfo[128];
    float ctx = (p_objinfo->info[i].bbox.x1 + p_objinfo->info[i].bbox.x2) / 2 / imgw;
    float cty = (p_objinfo->info[i].bbox.y1 + p_objinfo->info[i].bbox.y2) / 2 / imgh;
    float ww = (p_objinfo->info[i].bbox.x2 - p_objinfo->info[i].bbox.x1) / imgw;
    float hh = (p_objinfo->info[i].bbox.y2 - p_objinfo->info[i].bbox.y1) / imgh;
    float score = p_objinfo->info[i].bbox.score;
    sprintf(szinfo, "%d %f %f %f %f %" PRIu64 " %f\n", 4, ctx, cty, ww, hh,
            p_objinfo->info[i].unique_id, score);
    int tid = p_objinfo->info[i].unique_id;
    printf("trackid:%d\n", tid);

    fwrite(szinfo, 1, strlen(szinfo), fp);
  }
  fclose(fp);
}
int main(int argc, char *argv[]) {
  CVI_S32 ret = CVI_SUCCESS;

  cvitdl_handle_t tdl_handle = NULL;

  ret = CVI_TDL_CreateHandle2(&tdl_handle, 1, 0);

  const CVI_S32 vpssgrp_width = 1920;
  const CVI_S32 vpssgrp_height = 1080;

  ret = MMF_INIT_HELPER2(vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888, 1, vpssgrp_width,
                         vpssgrp_height, PIXEL_FORMAT_RGB_888_PLANAR, 1);
  if (ret != CVI_SUCCESS) {
    printf("Init sys failed with %#x!\n", ret);
    return ret;
  }
  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PEDESTRIAN, argv[1]);
  CVI_TDL_DeepSORT_Init(tdl_handle, true);

  cvtdl_deepsort_config_t ds_conf;
  CVI_TDL_DeepSORT_GetDefaultConfig(&ds_conf);
  ds_conf.ktracker_conf.accreditation_threshold = 4;
  ds_conf.ktracker_conf.P_beta[2] = 0.01;
  ds_conf.ktracker_conf.P_beta[6] = 1e-5;
  ds_conf.kfilter_conf.Q_beta[2] = 0.01;
  ds_conf.kfilter_conf.Q_beta[6] = 1e-5;
  ds_conf.kfilter_conf.R_beta[2] = 0.1;
  CVI_TDL_DeepSORT_SetConfig(tdl_handle, &ds_conf, -1, true);

  for (int img_idx = 0; img_idx < 10; img_idx++) {
    char szimg[256];
    sprintf(szimg, "%s/%08d.bin", argv[2], img_idx);
    VIDEO_FRAME_INFO_S fdFrame;
    ret = CVI_TDL_LoadBinImage(szimg, &fdFrame, PIXEL_FORMAT_RGB_888_PLANAR);
    int imgw = fdFrame.stVFrame.u32Width;
    int imgh = fdFrame.stVFrame.u32Width;
    printf("start to process:%s,width:%d,height:%d\n", szimg, imgw, imgh);
    cvtdl_object_t obj_meta;
    memset(&obj_meta, 0, sizeof(cvtdl_object_t));
    cvtdl_tracker_t tracker_meta;
    memset(&tracker_meta, 0, sizeof(cvtdl_tracker_t));
    CVI_TDL_Detection(tdl_handle, &fdFrame, CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PEDESTRIAN,
                      &obj_meta);
    int objnum = obj_meta.size;
    printf("objnum:%d\n", objnum);
    CVI_TDL_DeepSORT_Obj(tdl_handle, &obj_meta, &tracker_meta, false);
    char dstf[256];
    sprintf(dstf, "%s/%08d.txt", argv[3], img_idx);

    export_img_result(dstf, &obj_meta, imgw, imgh);
    ReleaseImage(&fdFrame);
  }

  CVI_TDL_DestroyHandle(tdl_handle);
  CVI_SYS_Exit();
  CVI_VB_Exit();
  return 0;
}
