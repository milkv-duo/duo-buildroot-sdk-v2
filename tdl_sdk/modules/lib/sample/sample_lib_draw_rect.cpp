#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include "core/cvi_tdl_types_mem_internal.h"
#include "core/utils/vpss_helper.h"
#include "cvi_draw_rect.h"
#include "cvi_tdl.h"

int main(int argc, char *argv[]) {
  int vpssgrp_width = 1920;
  int vpssgrp_height = 1080;
  CVI_S32 ret = MMF_INIT_HELPER2(vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888, 1,
                                 vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888, 1);
  if (ret != CVI_TDL_SUCCESS) {
    printf("Init sys failed with %#x!\n", ret);
    return ret;
  }

  VIDEO_FRAME_INFO_S bg_view;

  cvtdl_object_t obj_meta = {0};
  cvtdl_service_brush_t brushi;
  brushi.color.r = 255;
  brushi.color.g = 255;
  brushi.color.b = 255;
  brushi.size = 4;

  obj_meta.size = 1;
  obj_meta.rescale_type = meta_rescale_type_e::RESCALE_CENTER;
  obj_meta.info = (cvtdl_object_info_t *)malloc(sizeof(cvtdl_object_info_t) * obj_meta.size);
  obj_meta.info[0].bbox.x1 = 100;
  obj_meta.info[0].bbox.y1 = 100;
  obj_meta.info[0].bbox.x2 = 150;
  obj_meta.info[0].bbox.y2 = 150;

  CVI_TDL_ObjectDrawRect(&obj_meta, &bg_view, false, brushi);
  free(obj_meta.info);
  return ret;
}
