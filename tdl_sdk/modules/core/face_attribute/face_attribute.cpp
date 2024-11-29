#include "face_attribute.hpp"
#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem.h"
#include "core/cvi_tdl_types_mem_internal.h"
#include "core/face/cvtdl_face_helper.h"
#include "core/utils/vpss_helper.h"
#include "core_utils.hpp"
#include "cvi_sys.h"
#include "cvi_tdl_log.hpp"
#include "face_attribute_types.hpp"
#include "img_warp.hpp"
#ifndef NO_OPENCV
#include "face_utils.hpp"
#include "image_utils.hpp"
#include "opencv2/opencv.hpp"
#endif

#define FACE_ATTRIBUTE_FACTOR (1 / 128.f)
#define FACE_ATTRIBUTE_MEAN (0.99609375)

#define ATTRIBUTE_OUT_NAME "BMFace_dense_MatMul_folded"
#define RECOGNITION_OUT_NAME "pre_fc1"
#define AGE_OUT_NAME "Age_Conv_Conv2D"
#define EMOTION_OUT_NAME "Emotion_Conv_Conv2D"
#define GENDER_OUT_NAME "Gender_Conv_Conv2D"
#define RACE_OUT_NAME "Race_Conv_Conv2D"

#define RACE_OUT_THRESH (16.9593105)
#define GENDER_OUT_THRESH (6.53386879)
#define AGE_OUT_THRESH (35.0413513)
#define EMOTION_OUT_THRESH (9.25036811)

static bool IS_SUPPORTED_FORMAT(VIDEO_FRAME_INFO_S *frame) {
  if (frame->stVFrame.enPixelFormat != PIXEL_FORMAT_RGB_888 &&
      frame->stVFrame.enPixelFormat != PIXEL_FORMAT_RGB_888_PLANAR &&
      frame->stVFrame.enPixelFormat != PIXEL_FORMAT_YUV_PLANAR_420 &&
      frame->stVFrame.enPixelFormat != PIXEL_FORMAT_NV21) {
    LOGE("Pixel format [%d] is not supported.\n", frame->stVFrame.enPixelFormat);
    return false;
  }
  return true;
}

namespace cvitdl {

FaceAttribute::FaceAttribute(bool with_attr)
    : Core(CVI_MEM_DEVICE), m_use_wrap_hw(false), m_with_attribute(with_attr) {
  attribute_buffer = new float[ATTR_AGE_FEATURE_DIM];
  for (uint32_t i = 0; i < 3; i++) {
    m_preprocess_param[0].factor[i] = FACE_ATTRIBUTE_FACTOR;
    m_preprocess_param[0].mean[i] = FACE_ATTRIBUTE_MEAN;
  }
}

int FaceAttribute::onModelOpened() { return allocateION(); }

int FaceAttribute::onModelClosed() {
  releaseION();
  return CVI_TDL_SUCCESS;
}

CVI_S32 FaceAttribute::allocateION() {
  CVI_SHAPE shape = getInputShape(0);
  PIXEL_FORMAT_E format = m_use_wrap_hw ? PIXEL_FORMAT_RGB_888_PLANAR : PIXEL_FORMAT_RGB_888;
  if (CREATE_ION_HELPER(&m_wrap_frame, shape.dim[3], shape.dim[2], format, "tpu") != CVI_SUCCESS) {
    LOGE("Cannot allocate ion for preprocess\n");
    LOGE("error Cannot allocate ion for preprocess\n");
    return CVI_TDL_ERR_ALLOC_ION_FAIL;
  }
  LOGI("m_wrap_frame step:%u,width:%u,height:%u\n", m_wrap_frame.stVFrame.u32Stride[0],
       m_wrap_frame.stVFrame.u32Width, m_wrap_frame.stVFrame.u32Height);
  return CVI_TDL_SUCCESS;
}

void FaceAttribute::releaseION() {
  if (m_wrap_frame.stVFrame.u64PhyAddr[0] != 0) {
#ifdef CONFIG_ALIOS
    CVI_SYS_IonFree64Align(m_wrap_frame.stVFrame.u64PhyAddr[0],
                           m_wrap_frame.stVFrame.pu8VirAddr[0]);
#else
    CVI_SYS_IonFree(m_wrap_frame.stVFrame.u64PhyAddr[0], m_wrap_frame.stVFrame.pu8VirAddr[0]);
#endif
    m_wrap_frame.stVFrame.u64PhyAddr[0] = (CVI_U64)0;
    m_wrap_frame.stVFrame.u64PhyAddr[1] = (CVI_U64)0;
    m_wrap_frame.stVFrame.u64PhyAddr[2] = (CVI_U64)0;
    m_wrap_frame.stVFrame.pu8VirAddr[0] = NULL;
    m_wrap_frame.stVFrame.pu8VirAddr[1] = NULL;
    m_wrap_frame.stVFrame.pu8VirAddr[2] = NULL;
    LOGI("release m_wrap_frame\n");
  }
}

FaceAttribute::~FaceAttribute() {
  if (attribute_buffer != nullptr) {
    delete[] attribute_buffer;
    attribute_buffer = nullptr;
  }
}

void FaceAttribute::setHardwareGDC(bool use_wrap_hw) {
  if (isInitialized()) {
    LOGW("Please invoke CVI_TDL_EnableGDC before opening model\n");
    return;
  }

  m_use_wrap_hw = use_wrap_hw;
}
int FaceAttribute::dump_bgr_pack(const char *p_img_file, VIDEO_FRAME_INFO_S *p_img_frm) {
  FILE *fp = fopen(p_img_file, "wb");
  if (fp == nullptr) {
    LOGE("failed to open: %s.\n", p_img_file);
    return CVI_FAILURE;
  }
  VIDEO_FRAME_S *pstVFSrc = &p_img_frm->stVFrame;
  uint32_t width = pstVFSrc->u32Width;
  uint32_t height = pstVFSrc->u32Height;
  fwrite(&width, sizeof(uint32_t), 1, fp);
  fwrite(&height, sizeof(uint32_t), 1, fp);
  fwrite(pstVFSrc->pu8VirAddr[0], height * p_img_frm->stVFrame.u32Stride[0], 1, fp);
  printf("width:%u,stride:%u\n", width, p_img_frm->stVFrame.u32Stride[0]);
  fclose(fp);
  return CVI_TDL_SUCCESS;
}
int FaceAttribute::inference(VIDEO_FRAME_INFO_S *stOutFrame, cvtdl_face_t *meta, int face_idx) {
  if (m_use_wrap_hw) {
#if defined(CV183X) || defined(CV186X)
    if (stOutFrame->stVFrame.enPixelFormat != PIXEL_FORMAT_RGB_888_PLANAR &&
        stOutFrame->stVFrame.enPixelFormat != PIXEL_FORMAT_YUV_PLANAR_420) {
      LOGE(
          "Supported format are PIXEL_FORMAT_RGB_888_PLANAR, PIXEL_FORMAT_YUV_PLANAR_420. Current: "
          "%x\n",
          stOutFrame->stVFrame.enPixelFormat);
      return CVI_TDL_ERR_INVALID_ARGS;
    }

    for (uint32_t i = 0; i < meta->size; ++i) {
      if (face_idx != -1 && i != (uint32_t)face_idx) continue;

      cvtdl_face_info_t face_info =
          info_rescale_c(stOutFrame->stVFrame.u32Width, stOutFrame->stVFrame.u32Height, *meta, i);
      face_align_gdc(stOutFrame, &m_wrap_frame, face_info);
      std::vector<VIDEO_FRAME_INFO_S *> frames = {&m_wrap_frame};
      int ret = run(frames);
      if (ret != CVI_TDL_SUCCESS) {
        return ret;
      }
      outputParser(&meta->info[i]);
      CVI_TDL_FreeCpp(&face_info);
    }
#endif
  } else {
    if (false == IS_SUPPORTED_FORMAT(stOutFrame)) {
      return CVI_TDL_ERR_INVALID_ARGS;
    }

    bool do_unmap = false;
    if (stOutFrame->stVFrame.pu8VirAddr[0] == NULL) {
      mmap_video_frame(stOutFrame);
      do_unmap = true;
    }

    for (uint32_t i = 0; i < meta->size; ++i) {
      if (face_idx != -1 && i != (uint32_t)face_idx) continue;
#ifdef NO_OPENCV
      int dst_size = 0;
      cvtdl_face_info_t face_info =
          info_extern_crop_resize_img(stOutFrame->stVFrame.u32Width, stOutFrame->stVFrame.u32Height,
                                      &(meta->info[i]), &dst_size);
      /*There will crop the image and resize to 256*256, export PIXEL_FORMAT_BGR_888_PACKED format*/
      VIDEO_FRAME_INFO_S *f = new VIDEO_FRAME_INFO_S;
      memset(f, 0, sizeof(VIDEO_FRAME_INFO_S));
      vpssCropImage(stOutFrame, f, face_info.bbox, dst_size, dst_size, PIXEL_FORMAT_RGB_888);
      mmap_video_frame(f);

      float pts[10];
      float transm[6];
      for (uint8_t i = 0; i < 5; i++) {
        pts[2 * i] = face_info.pts.x[i];
        pts[2 * i + 1] = face_info.pts.y[i];
      }

      cvitdl::get_face_transform(pts, 112, transm);
      cvitdl::warp_affine(f->stVFrame.pu8VirAddr[0], f->stVFrame.u32Stride[0], f->stVFrame.u32Width,
                          f->stVFrame.u32Height, m_wrap_frame.stVFrame.pu8VirAddr[0],
                          m_wrap_frame.stVFrame.u32Stride[0], m_wrap_frame.stVFrame.u32Width,
                          m_wrap_frame.stVFrame.u32Height, transm);

      unmap_video_frame(f);
      if (f->stVFrame.u64PhyAddr[0] != 0) {
        mp_vpss_inst->releaseFrame(f, 0);
      }
      delete f;
#else
      cvtdl_face_info_t face_info =
          info_rescale_c(stOutFrame->stVFrame.u32Width, stOutFrame->stVFrame.u32Height, *meta, i);
      ALIGN_FACE_TO_FRAME(stOutFrame, &m_wrap_frame, face_info);
#endif

      std::vector<VIDEO_FRAME_INFO_S *> frames = {&m_wrap_frame};
      int ret = run(frames);
      if (ret != CVI_TDL_SUCCESS) {
        return ret;
      }
      outputParser(&meta->info[i]);
      CVI_TDL_FreeCpp(&face_info);
    }
    if (do_unmap) {
      unmap_video_frame(stOutFrame);
    }
  }
  return CVI_TDL_SUCCESS;
}

int FaceAttribute::extract_face_feature(const uint8_t *p_rgb_pack, uint32_t width, uint32_t height,
                                        uint32_t stride, cvtdl_face_info_t *p_face_info) {
  float pts[10];
  float transm[6];
  mmap_video_frame(&m_wrap_frame);
  if (p_face_info->pts.size == 5) {  // do face alignment
    for (uint8_t i = 0; i < 5; i++) {
      pts[2 * i] = p_face_info->pts.x[i];
      pts[2 * i + 1] = p_face_info->pts.y[i];
    }
    cvitdl::get_face_transform(pts, 112, transm);
    cvitdl::warp_affine(p_rgb_pack, stride, width, height, m_wrap_frame.stVFrame.pu8VirAddr[0],
                        m_wrap_frame.stVFrame.u32Stride[0], m_wrap_frame.stVFrame.u32Width,
                        m_wrap_frame.stVFrame.u32Height, transm);

  } else {
    if (width != m_wrap_frame.stVFrame.u32Width || height != m_wrap_frame.stVFrame.u32Height) {
      LOGE("input image size should be equal to %u\n", m_wrap_frame.stVFrame.u32Width);
    }
    if (stride != m_wrap_frame.stVFrame.u32Stride[0]) {
      LOGE("input image stride not ok,got:%u,expect:%u", stride,
           m_wrap_frame.stVFrame.u32Stride[0]);
    }
    memcpy(m_wrap_frame.stVFrame.pu8VirAddr[0], p_rgb_pack,
           m_wrap_frame.stVFrame.u32Stride[0] * height);
  }
  CVI_SYS_IonFlushCache(m_wrap_frame.stVFrame.u64PhyAddr[0], m_wrap_frame.stVFrame.pu8VirAddr[0],
                        m_wrap_frame.stVFrame.u32Length[0]);
  unmap_video_frame(&m_wrap_frame);
  std::vector<VIDEO_FRAME_INFO_S *> frames = {&m_wrap_frame};
  if (run(frames) != CVI_SUCCESS) {
    return CVI_FAILURE;
  }
  std::string feature_out_name = (m_with_attribute) ? ATTRIBUTE_OUT_NAME : RECOGNITION_OUT_NAME;
  int8_t *face_blob;
  size_t face_feature_size;

  if (m_with_attribute) {
    const TensorInfo &tinfo = getOutputTensorInfo(feature_out_name);
    face_blob = tinfo.get<int8_t>();
    face_feature_size = tinfo.tensor_elem;
  } else {
    const TensorInfo &tinfo = getOutputTensorInfo(0);
    face_blob = tinfo.get<int8_t>();
    face_feature_size = tinfo.tensor_elem;
  }
  // Create feature
  CVI_TDL_MemAlloc(sizeof(int8_t), face_feature_size, TYPE_INT8, &p_face_info->feature);
  memcpy(p_face_info->feature.ptr, face_blob, face_feature_size);

  return CVI_SUCCESS;
}

template <typename U, typename V>
struct ExtractFeatures {
  std::pair<U, V> operator()(float *out, const int size) {
    SoftMaxForBuffer(out, out, size);
    size_t max_idx = 0;
    float max_val = -1e3;

    for (int i = 0; i < size; i++) {
      if (out[i] > max_val) {
        max_val = out[i];
        max_idx = i;
      }
    }

    V features;
    memcpy(features.features.data(), out, sizeof(float) * size);
    return std::make_pair(U(max_idx + 1), features);
  }
};

template <>
struct ExtractFeatures<float, AgeFeature> {
  std::pair<float, AgeFeature> operator()(float *out, const int age_prob_size) {
    float expect_age = 0;
    if (age_prob_size == 1) {
      expect_age = out[0];
    } else if (age_prob_size == 101) {
      SoftMaxForBuffer(out, out, age_prob_size);
      for (size_t i = 0; i < 101; i++) {
        expect_age += (i * out[i]);
      }
    } else {
      expect_age = -1.0;
    }

    AgeFeature features;
    memcpy(features.features.data(), out, sizeof(float) * age_prob_size);
    return std::make_pair(expect_age, features);
  }
};

template <typename U, typename V>
std::pair<U, V> getDequantTensor(const TensorInfo &tinfo, float threshold, float *buffer,
                                 ExtractFeatures<U, V> functor) {
  int8_t *blob = tinfo.get<int8_t>();
  size_t prob_size = tinfo.tensor_elem;
  Dequantize(blob, buffer, threshold, prob_size);
  return functor(buffer, prob_size);
}

void FaceAttribute::outputParser(cvtdl_face_info_t *face_info) {
  FaceAttributeInfo result;

  // feature
  std::string feature_out_name = (m_with_attribute) ? ATTRIBUTE_OUT_NAME : RECOGNITION_OUT_NAME;

  int8_t *face_blob;
  size_t face_feature_size;

  if (m_with_attribute) {
    const TensorInfo &tinfo = getOutputTensorInfo(feature_out_name);
    face_blob = tinfo.get<int8_t>();
    face_feature_size = tinfo.tensor_elem;
  } else {
    const TensorInfo &tinfo = getOutputTensorInfo(0);
    face_blob = tinfo.get<int8_t>();
    face_feature_size = tinfo.tensor_elem;
  }
  // Create feature
  CVI_TDL_MemAlloc(sizeof(int8_t), face_feature_size, TYPE_INT8, &face_info->feature);
  memcpy(face_info->feature.ptr, face_blob, face_feature_size);

  if (!m_with_attribute) {
    return;
  }

  // race
  auto race = getDequantTensor(getOutputTensorInfo(RACE_OUT_NAME), RACE_OUT_THRESH,
                               attribute_buffer, ExtractFeatures<cvtdl_face_race_e, RaceFeature>());
  result.race = race.first;
  result.race_prob = std::move(race.second);

  // gender
  auto gender =
      getDequantTensor(getOutputTensorInfo(GENDER_OUT_NAME), GENDER_OUT_THRESH, attribute_buffer,
                       ExtractFeatures<cvtdl_face_gender_e, GenderFeature>());
  result.gender = gender.first;
  result.gender_prob = std::move(gender.second);

  // age
  auto age = getDequantTensor(getOutputTensorInfo(AGE_OUT_NAME), AGE_OUT_THRESH, attribute_buffer,
                              ExtractFeatures<float, AgeFeature>());
  result.age = age.first;
  result.age_prob = std::move(age.second);

  // emotion
  auto emotion =
      getDequantTensor(getOutputTensorInfo(EMOTION_OUT_NAME), EMOTION_OUT_THRESH, attribute_buffer,
                       ExtractFeatures<cvtdl_face_emotion_e, EmotionFeature>());
  result.emotion = emotion.first;
  result.emotion_prob = std::move(emotion.second);

  face_info->race = result.race;
  face_info->gender = result.gender;
  face_info->emotion = result.emotion;
  face_info->age = result.age;

#ifdef ENABLE_FACE_ATTRIBUTE_DEBUG
  std::cout << "Emotion   |" << getEmotionString(result.emotion) << std::endl;
  std::cout << "Age       |" << result.age << std::endl;
  std::cout << "Gender    |" << getGenderString(result.gender) << std::endl;
  std::cout << "Race      |" << getRaceString(result.race) << std::endl;
#endif
}

}  // namespace cvitdl
