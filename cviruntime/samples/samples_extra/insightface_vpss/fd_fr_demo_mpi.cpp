#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <algorithm>
#include <fstream>

#include <opencv2/opencv.hpp>

#include "cvi_media_sdk.h"

#include "cviruntime.h"

#include "FacePreprocess.h"
#include "affine_hw.h"
#include "type_define.h"

#define IMAGE_WIDTH            (1920)
#define IMAGE_HEIGHT           (1080)
#define DISP_WIDTH             (1280)
#define DISP_HEIGHT            (720)
#define DISP_FMT               (PIXEL_FORMAT_YUV_PLANAR_420)

#define FD_IMG_RESIZE_WIDTH    (600)
#define FD_IMG_RESIZE_HEIGHT   (600)

#define FD_THRESHOLD           (0.1)

#define FR_IMG_RESIZE_WIDTH    (112)
#define FR_IMG_RESIZE_HEIGHT   (112)

#define HW_AFFINE

#define INPUT_FMT PIXEL_FORMAT_RGB_888_PLANAR

static int face_detect(CVI_MODEL_HANDLE model,
    VIDEO_FRAME_INFO_S *frame_in, 
    face_rect_t *dets, uint32_t *det_num,
    bool dump) {
  VIDEO_FRAME_INFO_S *frame_preprocessed;
  VIDEO_FRAME_INFO_S frame_preprocessed_local;

  // get input and output size
  CVI_TENSOR *input_tensors, *output_tensors;
  int32_t input_num, output_num;
  CVI_NN_GetInputOutputTensors(model, &input_tensors, &input_num,
      &output_tensors, &output_num);
  CVI_TENSOR *input = CVI_NN_GetTensorByName(CVI_NN_DEFAULT_TENSOR,
      input_tensors, input_num);

  // retinaface model
  // export MODEL_CHANNEL_ORDER = "rgb"
  // export RAW_SCALE = 255.0
  // export MEAN = 0, 0, 0
  // export INPUT_SCALE = 1.0
  // Threshold data 128.000000
  // CVI_MAPI_PREPROCESS_ATTR_T pp_attr;
  // pp_attr.is_rgb          = true;
  // pp_attr.raw_scale       = 255.0f;
  // pp_attr.mean[0]         = 0.0f;
  // pp_attr.mean[1]         = 0.0f;
  // pp_attr.mean[2]         = 0.0f;
  // pp_attr.input_scale[0]  = 1.0f;
  // pp_attr.input_scale[1]  = 1.0f;
  // pp_attr.input_scale[2]  = 1.0f;
  // pp_attr.qscale = CVI_NN_TensorQuantScale(input);
  SAMPLE_CHECK_RET(CVI_MAPI_IPROC_Resize(frame_in, &frame_preprocessed_local,
                                         FD_IMG_RESIZE_WIDTH, FD_IMG_RESIZE_HEIGHT,
                                         INPUT_FMT,
                                         false, NULL, NULL, NULL));

  if (dump) {
      SAMPLE_CHECK_RET(CVI_MAPI_SaveFramePixelData(&frame_preprocessed_local,
                                                   "face_resized"));
  }
  frame_preprocessed = &frame_preprocessed_local;

  CVI_NN_SetTensorPhysicalAddr(input, frame_preprocessed->stVFrame.u64PhyAddr[0]);

  // run inference
  CVI_NN_Forward(model, input_tensors, input_num,
                 output_tensors, output_num);

  SAMPLE_CHECK_RET(CVI_MAPI_IPROC_ReleaseFrame(&frame_preprocessed_local));

  // get output
  // output: [x1,y1,x2,y2,score,landmarks]
  CVI_TENSOR *output = CVI_NN_GetTensorByName("output",
      output_tensors, output_num);
  int32_t num = output->shape.dim[2];
  int32_t dim = output->shape.dim[3];
  //printf("num %d, dim %d\n", num, dim);

  // process output
  int face_idx = 0;
  float *det = (float *)CVI_NN_TensorPtr(output);
  memset(dets, 0, sizeof(face_rect_t) * FD_MAX_DET_NUM);
  for (int i = 0; i < num; ++i) {
    if (det[4] != 0) {
      dets[face_idx].score = det[4];
      dets[face_idx].x1 = det[0];
      dets[face_idx].y1 = det[1];
      dets[face_idx].x2 = det[2];
      dets[face_idx].y2 = det[3];
      memcpy(dets[face_idx].landmarks, &det[5], sizeof(float) * 10);

      face_idx++;

      if (face_idx >= FD_MAX_DET_NUM)
        break;
    }
    det += dim;
  }
  //printf("%d faces detected\n", face_idx);

  float sw = 1.0f * FD_IMG_RESIZE_WIDTH / frame_in->stVFrame.u32Width;
  float sh = 1.0f * FD_IMG_RESIZE_HEIGHT / frame_in->stVFrame.u32Height;

  // mutiply scale to origin image size
  for (int i = 0; i < face_idx; ++i) {
    dets[i].x1 /= sw;
    dets[i].y1 /= sh;
    dets[i].x2 /= sw;
    dets[i].y2 /= sh;

    for (int j = 0; j < 5; j++) {
      dets[i].landmarks[j][0] /= sw;
      dets[i].landmarks[j][1] /= sh;
    }
  }

  *det_num = face_idx;
  return 0;
}

#ifdef HW_AFFINE
static int face_align_by_hw(VIDEO_FRAME_INFO_S *frame_in, face_rect_t *det,
    VIDEO_FRAME_INFO_S *frame_aligned, bool dump) {
  //init frame_aligned
  uint32_t width = FR_IMG_RESIZE_WIDTH;
  uint32_t height = FR_IMG_RESIZE_HEIGHT;
  SAMPLE_CHECK_RET(CVI_MAPI_AllocateFrame(frame_aligned, width, height, frame_in->stVFrame.enPixelFormat));
  SAMPLE_CHECK_RET(face_align_gdc(frame_in, frame_aligned, *det));

  if (dump) {
    SAMPLE_CHECK_RET(CVI_MAPI_SaveFramePixelData(frame_aligned, "face_aligned"));
  }
  return 0;
}
#else
static int copy_mat_to_frame(cv::Mat& mat, void * frame_data,
    int32_t stride) {
  CVI_LOG_ASSERT(frame_data != NULL, "Null Frame!");
  CVI_LOG_ASSERT(mat.cols <= stride, "Error param!");
  if (stride == mat.cols) {
    memcpy(frame_data, mat.data, mat.cols * mat.rows);
    return 0;
  }
  uint8_t * frame_ptr = (uint8_t *)frame_data;
  uint8_t * mat_ptr = mat.data;
  uint32_t frame_step = stride;
  uint32_t mat_step = mat.cols;
  for (int i = 0; i < mat.rows; ++i) {
    memcpy(frame_ptr, mat_ptr, mat_step);
    frame_ptr += frame_step;
    mat_ptr += mat_step;
  }
  return 0;
}

static int face_align_by_sw(VIDEO_FRAME_INFO_S *frame_in, face_rect_t *det,
    VIDEO_FRAME_INFO_S *frame_aligned, bool dump) {
  int x = det->x1;
  int y = det->y1;
  int w = det->x2 - x;
  int h = det->y2 - y;

  //expand the face's rect
  float pad_scale = 0.6;
  cv::Rect frame_rect(0, 0,
      frame_in->stVFrame.u32Width,
      frame_in->stVFrame.u32Height);
  cv::Rect face_pad_rect(
      x - w * pad_scale / 2,
      y - h * pad_scale / 2,
      w * (1 + pad_scale),
      h * (1 + pad_scale));
  face_pad_rect.x = ALIGN(face_pad_rect.x, 2);
  face_pad_rect.y = ALIGN(face_pad_rect.y, 2);
  face_pad_rect.width = ALIGN(face_pad_rect.width, 2);
  face_pad_rect.height = ALIGN(face_pad_rect.height, 2);
  face_pad_rect &= frame_rect;

  //adjust the coordinate
  det->x1 -= face_pad_rect.x;
  det->y1 -= face_pad_rect.y;
  det->x2 -= face_pad_rect.x;
  det->y2 -= face_pad_rect.y;

  for (int i = 0; i < 5; i++) {
    det->landmarks[i][0] = std::max(0.0f, det->landmarks[i][0] - face_pad_rect.x);
    det->landmarks[i][1] = std::max(0.0f, det->landmarks[i][1] - face_pad_rect.y);
  }

  //crop face
  CVI_MAPI_IPROC_RESIZE_CROP_ATTR_T crop_in_attr;
  crop_in_attr.x = face_pad_rect.x;
  crop_in_attr.y = face_pad_rect.y;
  crop_in_attr.w = face_pad_rect.width;
  crop_in_attr.h = face_pad_rect.height;

  //printf("  crop [%d, %d, %d, %d]\n",
  //    crop_in_attr.x, crop_in_attr.y,
  //    crop_in_attr.w, crop_in_attr.h);

  VIDEO_FRAME_INFO_S frame_crop;
  SAMPLE_CHECK_RET(CVI_MAPI_IPROC_Resize(frame_in, &frame_crop,
        crop_in_attr.w, crop_in_attr.h,
        PIXEL_FORMAT_RGB_888_PLANAR,
        false, &crop_in_attr, NULL, NULL));

  if (dump) {
    SAMPLE_CHECK_RET(CVI_MAPI_SaveFramePixelData(&frame_crop, "face_crop"));
  }

  // affine
  // TODO: use hardware warp engine
  //
  float ref_pts[5][2] = {
    { 30.2946f, 51.6963f },
    { 65.5318f, 51.5014f },
    { 48.0252f, 71.7366f },
    { 33.5493f, 92.3655f },
    { 62.7299f, 92.2041f }
  };
  cv::Mat ref(5, 2, CV_32FC1, ref_pts);
  cv::Mat dst(5, 2, CV_32FC1, det->landmarks);

  auto m = FacePreprocess::similarTransform(dst, ref);

  //std::cout << "ref =" << std::endl << ref << std::endl;
  //std::cout << "dst =" << std::endl << dst << std::endl;
  //std::cout << "m =" << std::endl << m << std::endl;

  uint32_t width = FR_IMG_RESIZE_WIDTH;
  uint32_t height = FR_IMG_RESIZE_HEIGHT;
  SAMPLE_CHECK_RET(CVI_MAPI_AllocateFrame(frame_aligned, width, height,
                                   PIXEL_FORMAT_RGB_888_PLANAR));
  CVI_MAPI_FrameMmap(frame_aligned, true);

  CVI_MAPI_FrameMmap(&frame_crop, true);
  CVI_MAPI_FrameInvalidateCache(&frame_crop);

  for (int i = 0; i < 3; ++i) {
    // each channel do warp alone
    cv::Mat channel_in = cv::Mat(frame_crop.stVFrame.u32Height,
                                 frame_crop.stVFrame.u32Width,
                                 CV_8UC1,
                                 frame_crop.stVFrame.pu8VirAddr[i],
                                 frame_crop.stVFrame.u32Stride[i]);
    cv::Mat channel_out;
    cv::warpPerspective(channel_in, channel_out, m,
                        cv::Size(96, 112), cv::INTER_LINEAR);
    cv::resize(channel_out, channel_out,
               cv::Size(FR_IMG_RESIZE_WIDTH, FR_IMG_RESIZE_HEIGHT),
               0, 0, cv::INTER_LINEAR);
    copy_mat_to_frame(channel_out,
                      frame_aligned->stVFrame.pu8VirAddr[i],
                      frame_aligned->stVFrame.u32Stride[i]);
  }
  // Unmap
  CVI_MAPI_FrameMunmap(&frame_crop);

  // Flush cache
  CVI_MAPI_FrameFlushCache(frame_aligned);
  CVI_MAPI_FrameMunmap(frame_aligned);

  if (dump) {
    SAMPLE_CHECK_RET(CVI_MAPI_SaveFramePixelData(frame_aligned, "face_aligned"));
  }

  CVI_MAPI_ReleaseFrame(&frame_crop);

  return 0;
}
#endif

static cv::Mat face_extract(CVI_MODEL_HANDLE model,
    VIDEO_FRAME_INFO_S *frame_in) {
  cv::Mat feature(512, 1, CV_32FC1);

  CVI_TENSOR *input_tensors, *output_tensors;
  int32_t input_num, output_num;
  CVI_NN_GetInputOutputTensors(model, &input_tensors, &input_num, &output_tensors, &output_num);
  CVI_TENSOR *input = CVI_NN_GetTensorByName(CVI_NN_DEFAULT_TENSOR, input_tensors, input_num);
  //printf("fr input tensor size:%ld\n", CVI_NN_TensorSize(input));
  //printf("fr quant scale:%lf\n", CVI_NN_TensorQuantScale(input));

  // IPROC PREPROCESS
  VIDEO_FRAME_INFO_S frame_out;
  //eg. model
  // export MODEL_CHANNEL_ORDER = "rgb"
  // export RAW_SCALE = 255
  // export MEAN = 127.5, 127.5, 127.5
  // export INPUT_SCALE = 1
  // Threshold data 128.0
  // CVI_MAPI_PREPROCESS_ATTR_T pp_attr;
  // pp_attr.is_rgb = true;
  // pp_attr.raw_scale = 255.0f;
  // pp_attr.mean[0] = 127.5f;
  // pp_attr.mean[1] = 127.5f;
  // pp_attr.mean[2] = 127.5f;
  // pp_attr.input_scale[0] = 0.0078125f;
  // pp_attr.input_scale[1] = 0.0078125f;
  // pp_attr.input_scale[2] = 0.0078125f;
  // pp_attr.qscale = CVI_NN_TensorQuantScale(input);

  int ret = 0;
  ret = CVI_MAPI_IPROC_Resize(frame_in, &frame_out,
        frame_in->stVFrame.u32Width, frame_in->stVFrame.u32Height,
        INPUT_FMT,
        false, NULL, NULL, NULL);
  if (ret != 0) {
    printf("CVI_MAPI_IPROC_Resize failed, err %d\n", ret);
    exit(-1);
  }

  // use device memory
  CVI_NN_SetTensorPhysicalAddr(input, frame_out.stVFrame.u64PhyAddr[0]);

  // run inference
  ret = CVI_NN_Forward(model, input_tensors, input_num,
                      output_tensors, output_num);
  if (ret != CVI_RC_SUCCESS) {
    printf("CVI_NN_Forward failed, err %d\n", ret);
    return cv::Mat();
  }
  printf("CVI_NN_Forward succeeded\n");

  CVI_TENSOR *output = CVI_NN_GetTensorByName("fc1_scale_dequant*", output_tensors, output_num);
  memcpy(feature.data, (float*)CVI_NN_TensorPtr(output), CVI_NN_TensorSize(output));
  CVI_MAPI_IPROC_ReleaseFrame(&frame_out);

  return feature;
}

static float cal_similarity(cv::Mat& feature1, cv::Mat& feature2) {
  return feature1.dot(feature2) / (cv::norm(feature1) * cv::norm(feature2));
}

static void save_feature_bin(cv::Mat& feature, const char *filename) {
  FILE *output;
  output = fopen(filename, "wb");
  int len = feature.total() * feature.elemSize();
  fwrite(feature.data, len, 1, output);
  fclose(output);
}

static void save_feature_txt(cv::Mat& feature, const char *filename) {
  FILE *output;
  output = fopen(filename, "w");
  for (int i = 0; i < feature.rows; ++i) {
    for (int j = 0; j < feature.cols; ++j) {
      fprintf(output, "%lf\n", feature.at<float>(i,j));
    }
  }
  fclose(output);
}

static int get_boot_time(uint64_t *time_us)
{
    struct timespec timeo;
    clock_gettime(CLOCK_MONOTONIC, &timeo);
    *time_us = (uint64_t)timeo.tv_sec * 1000000 + timeo.tv_nsec / 1000;
    return 0;
}

static inline uint64_t align_up(uint64_t x, uint64_t n)
{
  return (x + n - 1) / n * n;
}

static int load_image_to_frame(VIDEO_FRAME_INFO_S *in_frame, cv::Mat &mat) {
  VIDEO_FRAME_INFO_S bgr_frame;
  CVI_MAPI_AllocateFrame(&bgr_frame, mat.cols, mat.rows, PIXEL_FORMAT_BGR_888);
  CVI_MAPI_FrameMmap(&bgr_frame, true);
  uint8_t *src_ptr = mat.data;
  uint8_t *dst_ptr = bgr_frame.stVFrame.pu8VirAddr[0];
  for (int h = 0; h < mat.rows; ++h) {
    memcpy(dst_ptr, src_ptr, 3 * mat.cols);
    src_ptr += 3 * mat.cols;
    dst_ptr += bgr_frame.stVFrame.u32Stride[0];
  }
  CVI_MAPI_FrameFlushCache(&bgr_frame);
  CVI_MAPI_FrameMunmap(&bgr_frame);

  SAMPLE_CHECK_RET(CVI_MAPI_IPROC_Resize(&bgr_frame, in_frame,
                                         mat.cols, mat.rows,
                                         INPUT_FMT,
                                         false, NULL, NULL, NULL));
  CVI_MAPI_ReleaseFrame(&bgr_frame);
  return 0;
}

static void usage(char **argv) {
  printf("Usage:\n");
  printf("   %s cvimodel_det cvimodel_rec [yuv_input] [frame_count]\n", argv[0]);
}


int main(int argc, char* argv[]) {
  if (argc != 4 && argc != 5) {
    usage(argv);
    exit(-1);
  }

  char *modelfile_det = argv[1];
  char *modelfile_rec = argv[2];
  const char *inputfile = argv[3];
  cv::Mat image = cv::imread(inputfile);
  if (image.empty()) {
    printf("load image failed! image:%s\n", inputfile);
    return -1;
  }
  int frame_count = 1;
  if (argc >= 5) {
    frame_count = atoi(argv[4]);
  }

  // register model
  CVI_MODEL_HANDLE model_fd, model_fr;
  CVI_NN_RegisterModel(modelfile_det, &model_fd);
  CVI_NN_RegisterModel(modelfile_rec, &model_fr);

  CVI_MAPI_MEDIA_SYS_ATTR_T sys_attr = {0};
  sys_attr.vb_pool[0].is_frame = true;
  sys_attr.vb_pool[0].vb_blk_size.frame.width  = IMAGE_WIDTH;
  sys_attr.vb_pool[0].vb_blk_size.frame.height = IMAGE_HEIGHT;
  sys_attr.vb_pool[0].vb_blk_size.frame.fmt    = INPUT_FMT;
  sys_attr.vb_pool[0].vb_blk_num = 6;
  sys_attr.vb_pool[1].is_frame = true;
  sys_attr.vb_pool[1].vb_blk_size.frame.width  = image.cols;
  sys_attr.vb_pool[1].vb_blk_size.frame.height = image.rows;
  sys_attr.vb_pool[1].vb_blk_size.frame.fmt    = PIXEL_FORMAT_BGR_888;
  sys_attr.vb_pool[1].vb_blk_num = 4;
  sys_attr.vb_pool_num = 2;
  SAMPLE_CHECK_RET(CVI_MAPI_Media_Init(&sys_attr));

  VIDEO_FRAME_INFO_S frame_input;
  load_image_to_frame(&frame_input, image);

  // init vproc/vpss
  CVI_MAPI_VPROC_HANDLE_T vproc;
  CVI_MAPI_VPROC_ATTR_T vproc_attr = CVI_MAPI_VPROC_DefaultAttr_OneChn(
      image.cols, image.rows, INPUT_FMT,
      DISP_WIDTH, DISP_HEIGHT, DISP_FMT);
  SAMPLE_CHECK_RET(CVI_MAPI_VPROC_Init(&vproc, -1, &vproc_attr));
  int vproc_chn_id_disp = 0;

  while (frame_count) {
    bool do_dump = (frame_count == 1);
    uint64_t t0, t1, t_vcap, t_detect, t_align = 0, t_extract = 0;

    get_boot_time(&t0);
    face_rect_t dets[FD_MAX_DET_NUM];
    uint32_t det_num;
    SAMPLE_CHECK_RET(face_detect(model_fd, &frame_input, dets,
                          &det_num, do_dump));
    get_boot_time(&t1);
    t_detect = t1 - t0;

    printf("detected %d faces\n", det_num);
    for (uint32_t i = 0; i < det_num; i++) {
      printf("[%d]: [%.2f, %.2f] -> [%.2f, %.2f], score %.2f\n",
          i, dets[i].x1, dets[i].y1, dets[i].x2, dets[i].y2,
          dets[i].score);

      if (dets[i].score > FD_THRESHOLD) {
        // align frame

        get_boot_time(&t0);

        VIDEO_FRAME_INFO_S frame_aligned;
#ifdef HW_AFFINE
#ifdef CHIP_182x
        assert(0 && "cv182x not support hardware affine\n");
#endif
        SAMPLE_CHECK_RET(face_align_by_hw(&frame_input, &dets[i], &frame_aligned, do_dump));
#else
        SAMPLE_CHECK_RET(face_align_by_sw(&frame_input, &dets[i], &frame_aligned, do_dump));
#endif

        get_boot_time(&t1);
        t_align = t1 - t0;
        get_boot_time(&t0);

        auto feature = face_extract(model_fr, &frame_aligned);

        get_boot_time(&t1);
        t_extract = t1 - t0;

        printf("face feature extract done\n") ;
        //std::cout << "feature: " << std::endl << feature << std::endl;

        if (do_dump) {
          save_feature_bin(feature, "face_feature.bin");
          save_feature_txt(feature, "face_feature.txt");
        }

        // compare with the last saved feature
        static cv::Mat last_feature;
        if (!last_feature.empty()) {
          float similarity = cal_similarity(feature, last_feature);
          printf("Similarity (againt last face): %.2f \n", similarity);
        }
        last_feature = feature;

        SAMPLE_CHECK_RET(CVI_MAPI_ReleaseFrame(&frame_aligned));
      }
    }

    /* send frame to vo
      SAMPLE_CHECK_RET(CVI_MAPI_VPROC_SendFrame(vproc, &frame_input));

      VIDEO_FRAME_INFO_S vproc_frame_disp;
      SAMPLE_CHECK_RET(CVI_MAPI_VPROC_GetChnFrame(vproc, 0, &vproc_frame_disp));

      SAMPLE_CHECK_RET(CVI_MAPI_DISP_SendFrame(disp, &vproc_frame_disp));
      SAMPLE_CHECK_RET(CVI_MAPI_ReleaseFrame(&vproc_frame_disp));
    */


    if (frame_count % 30 == 0) {
      printf("\nPerf:\n");
      printf("      Detect    %2.2f ms\n", (t_detect)/1000.0);
      if (det_num) {
        printf("      Align     %2.2f ms\n", (t_align)/1000.0);
        printf("      Extract   %2.2f ms\n", (t_extract)/1000.0);
      }
    }

    frame_count --;
  }

  // clean up
  SAMPLE_CHECK_RET(CVI_MAPI_ReleaseFrame(&frame_input));
  SAMPLE_CHECK_RET(CVI_MAPI_VPROC_Deinit(vproc));
  SAMPLE_CHECK_RET(CVI_MAPI_IPROC_Deinit());
  SAMPLE_CHECK_RET(CVI_MAPI_Media_Deinit());

  CVI_NN_CleanupModel(model_fr);
  CVI_NN_CleanupModel(model_fd);

  return 0;
}
