#include <iostream>
#include <memory>
#include <vector>
#include "core/object/cvtdl_object_types.h"
#include "core/utils/vpss_helper.h"
#include "cvi_ive.h"
#include "cvi_tdl_log.hpp"
#include "cvi_tdl_media.h"
#ifndef NO_OPENCV
#include "opencv2/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"

// TODO: use memcpy
inline void BufferRGBPackedCopy(const uint8_t *buffer, uint32_t width, uint32_t height,
                                uint32_t stride, VIDEO_FRAME_INFO_S *frame, bool invert) {
  VIDEO_FRAME_S *vFrame = &frame->stVFrame;
  if (invert) {
    for (uint32_t j = 0; j < height; j++) {
      const uint8_t *ptr = buffer + j * stride;
      for (uint32_t i = 0; i < width; i++) {
        uint32_t offset = i * 3 + j * vFrame->u32Stride[0];
        const uint8_t *ptr_pxl = i * 3 + ptr;
        vFrame->pu8VirAddr[0][offset] = ptr_pxl[2];
        vFrame->pu8VirAddr[0][offset + 1] = ptr_pxl[1];
        vFrame->pu8VirAddr[0][offset + 2] = ptr_pxl[0];
      }
    }
  } else {
    for (uint32_t j = 0; j < height; j++) {
      const uint8_t *ptr = buffer + j * stride;
      for (uint32_t i = 0; i < width; i++) {
        uint32_t offset = i * 3 + j * vFrame->u32Stride[0];
        const uint8_t *ptr_pxl = i * 3 + ptr;
        vFrame->pu8VirAddr[0][offset] = ptr_pxl[0];
        vFrame->pu8VirAddr[0][offset + 1] = ptr_pxl[1];
        vFrame->pu8VirAddr[0][offset + 2] = ptr_pxl[2];
      }
    }
  }
}

inline void BufferRGBPacked2PlanarCopy(const uint8_t *buffer, uint32_t width, uint32_t height,
                                       uint32_t stride, VIDEO_FRAME_INFO_S *frame, bool invert) {
  VIDEO_FRAME_S *vFrame = &frame->stVFrame;
  if (invert) {
    for (uint32_t j = 0; j < height; j++) {
      const uint8_t *ptr = buffer + j * stride;
      for (uint32_t i = 0; i < width; i++) {
        const uint8_t *ptr_pxl = i * 3 + ptr;
        vFrame->pu8VirAddr[0][i + j * vFrame->u32Stride[0]] = ptr_pxl[2];
        vFrame->pu8VirAddr[1][i + j * vFrame->u32Stride[1]] = ptr_pxl[1];
        vFrame->pu8VirAddr[2][i + j * vFrame->u32Stride[2]] = ptr_pxl[0];
      }
    }
  } else {
    for (uint32_t j = 0; j < height; j++) {
      const uint8_t *ptr = buffer + j * stride;
      for (uint32_t i = 0; i < width; i++) {
        const uint8_t *ptr_pxl = i * 3 + ptr;
        vFrame->pu8VirAddr[0][i + j * vFrame->u32Stride[0]] = ptr_pxl[0];
        vFrame->pu8VirAddr[1][i + j * vFrame->u32Stride[1]] = ptr_pxl[1];
        vFrame->pu8VirAddr[2][i + j * vFrame->u32Stride[2]] = ptr_pxl[2];
      }
    }
  }
}
// input is bgr format
inline void BufferRGBPacked2YUVPlanarCopy(const uint8_t *buffer, uint32_t width, uint32_t height,
                                          uint32_t stride, VIDEO_FRAME_INFO_S *frame, bool invert) {
  VIDEO_FRAME_S *vFrame = &frame->stVFrame;
  CVI_U8 *pY = vFrame->pu8VirAddr[0];
  CVI_U8 *pU = vFrame->pu8VirAddr[1];
  CVI_U8 *pV = vFrame->pu8VirAddr[2];

  for (uint32_t j = 0; j < height; j++) {
    const uint8_t *ptr = buffer + j * stride;
    for (uint32_t i = 0; i < width; i++) {
      const uint8_t *ptr_pxl = i * 3 + ptr;
      int b = ptr_pxl[0];
      int g = ptr_pxl[1];
      int r = ptr_pxl[2];
      if (invert) {
        std::swap(b, r);
      }
      pY[i + j * vFrame->u32Stride[0]] = ((66 * r + 129 * g + 25 * b) >> 8) + 16;

      if (j % 2 == 0 && i % 2 == 0) {
        pU[width / 2 + height / 2 * vFrame->u32Stride[1]] =
            ((-38 * r - 74 * g + 112 * b) >> 8) + 128;
        pV[width / 2 + height / 2 * vFrame->u32Stride[2]] =
            ((112 * r - 94 * g - 18 * b) >> 8) + 128;
      }
    }
  }
}
inline void BufferGreyCopy(const uint8_t *buffer, uint32_t width, uint32_t height, uint32_t stride,
                           VIDEO_FRAME_INFO_S *frame) {
  VIDEO_FRAME_S *vFrame = &frame->stVFrame;
  for (uint32_t j = 0; j < height; j++) {
    const uint8_t *ptr = buffer + j * stride;
    for (uint32_t i = 0; i < width; i++) {
      const uint8_t *ptr_pxl = ptr + i;
      vFrame->pu8VirAddr[0][i + j * vFrame->u32Stride[0]] = ptr_pxl[0];
    }
  }
}

template <typename T>
inline void BufferC12C1Copy(const uint8_t *buffer, uint32_t width, uint32_t height, uint32_t stride,
                            VIDEO_FRAME_INFO_S *frame) {
  VIDEO_FRAME_S *vFrame = &frame->stVFrame;
  for (uint32_t j = 0; j < height; j++) {
    const uint8_t *ptr = buffer + j * stride;
    uint8_t *vframec0ptr = vFrame->pu8VirAddr[0] + j * vFrame->u32Stride[0];
    memcpy(vframec0ptr, ptr, width * sizeof(T));
  }
}

void image_buffer_copy(cv::Mat &img, VIDEO_FRAME_INFO_S *frame, PIXEL_FORMAT_E format) {
  if (CREATE_ION_HELPER(frame, img.cols, img.rows, format, "cvitdl/image") != CVI_SUCCESS) {
    LOGE("alloc ion failed, imgwidth:%d,imgheight:%d\n", img.cols, img.rows);
  }
  switch (format) {
    case PIXEL_FORMAT_RGB_888: {
      BufferRGBPackedCopy(img.data, img.cols, img.rows, img.step, frame, true);
    } break;
    case PIXEL_FORMAT_BGR_888: {
      BufferRGBPackedCopy(img.data, img.cols, img.rows, img.step, frame, false);
    } break;
    case PIXEL_FORMAT_RGB_888_PLANAR: {
      BufferRGBPacked2PlanarCopy(img.data, img.cols, img.rows, img.step, frame, true);
    } break;
    case PIXEL_FORMAT_YUV_400: {
      cv::Mat img2;
      cv::cvtColor(img, img2, cv::COLOR_BGR2GRAY);
      BufferGreyCopy(img2.data, img2.cols, img2.rows, img2.step, frame);
    } break;
    case PIXEL_FORMAT_YUV_PLANAR_420:
      BufferRGBPacked2YUVPlanarCopy(img.data, img.cols, img.rows, img.step, frame, false);
      break;
    default:
      LOGE("Unsupported format: %u.\n", format);
      break;
  }
}

CVI_S32 read_resize_image(const char *filepath, VIDEO_FRAME_INFO_S *frame, PIXEL_FORMAT_E format,
                          uint32_t width, uint32_t height) {
  cv::Mat src_img = cv::imread(filepath);
  if (src_img.empty()) {
    LOGE("Cannot read image %s.\n", filepath);
    return CVI_FAILURE;
  }

  cv::Mat img;
  cv::resize(src_img, img, cv::Size(width, height));
  image_buffer_copy(img, frame, format);
  return CVI_SUCCESS;
}

CVI_S32 read_image(const char *filepath, VIDEO_FRAME_INFO_S *frame, PIXEL_FORMAT_E format) {
  cv::Mat img = cv::imread(filepath);
  if (img.empty()) {
    LOGE("Cannot read image %s.\n", filepath);
    return CVI_FAILURE;
  }
  image_buffer_copy(img, frame, format);
  return CVI_SUCCESS;
}

CVI_S32 read_centercrop_resize_image(const char *filepath, VIDEO_FRAME_INFO_S *frame,
                                     PIXEL_FORMAT_E format, uint32_t width, uint32_t height) {
  cv::Mat src_img = cv::imread(filepath);
  if (src_img.empty()) {
    LOGE("Cannot read image %s.\n", filepath);
    return CVI_FAILURE;
  }
  float oriWHRato = static_cast<float>(src_img.cols) / static_cast<float>(src_img.rows);
  float preWHRato = static_cast<float>(width) / static_cast<float>(height);
  int start_h, start_w, lenth_h, lenth_w;
  if (preWHRato > oriWHRato) {
    start_w = 0;
    start_h = static_cast<int>((src_img.rows - preWHRato * src_img.cols) / 2);
    lenth_w = src_img.cols;
    lenth_h = static_cast<int>(preWHRato * src_img.cols);
  } else {
    start_w = static_cast<int>((src_img.cols - preWHRato * src_img.rows) / 2);
    start_h = 0;
    lenth_w = static_cast<int>(preWHRato * src_img.rows);
    lenth_h = src_img.rows;
  }
  cv::Rect roi(start_w, start_h, lenth_w, lenth_h);
  cv::Mat img_cropped = src_img(roi);
  cv::Mat img;
  if (width < img_cropped.cols) {
    cv::resize(img_cropped, img, cv::Size(width, height), 0, 0, cv::INTER_AREA);
  } else {
    cv::resize(img_cropped, img, cv::Size(width, height), 0, 0, cv::INTER_CUBIC);
  }
  image_buffer_copy(img, frame, format);
  return CVI_SUCCESS;
}

CVI_S32 opencv_read_float_image(const char *filepath, VIDEO_FRAME_INFO_S *frame,
                                cvtdl_opencv_params opencv_params) {
  cv::Mat src_img = cv::imread(filepath, cv::IMREAD_COLOR);

  if (src_img.empty()) {
    std::cerr << "Cannot read image: " << filepath << std::endl;
    return -1;
  }

  cv::Mat resized_img;
  cv::resize(src_img, resized_img, cv::Size(opencv_params.width, opencv_params.height), 0, 0,
             opencv_params.interpolationMethod);
  cv::Mat img_float;
  resized_img.convertTo(img_float, CV_32FC3, 1.0 / 255.0);
  cv::Scalar mean = cv::Scalar(opencv_params.mean[0], opencv_params.mean[1], opencv_params.mean[2]);
  cv::Scalar std_dev = cv::Scalar(opencv_params.std[0], opencv_params.std[1], opencv_params.std[2]);
  cv::subtract(img_float, mean, img_float);
  cv::divide(img_float, std_dev, img_float);

  cv::Mat channels[3];
  cv::split(img_float, channels);

  int h = img_float.rows;
  int w = img_float.cols;

  CVI_U8 *buffer = new CVI_U8[h * w * 3 * sizeof(float)];
  if (opencv_params.rgbFormat == 0) {
    for (int i = 0; i < 3; ++i) {
      memcpy(buffer + i * h * w * sizeof(float), channels[2 - i].data, h * w * sizeof(float));
    }
  } else if (opencv_params.rgbFormat == 1) {
    for (int i = 0; i < 3; ++i) {
      memcpy(buffer + i * h * w * sizeof(float), channels[i].data, h * w * sizeof(float));
    }
  }

  frame->stVFrame.pu8VirAddr[0] = buffer;
  frame->stVFrame.u32Height = h;
  frame->stVFrame.u32Width = w;

  return CVI_SUCCESS;
}

CVI_S32 release_image(VIDEO_FRAME_INFO_S *frame) {
  if (frame->stVFrame.u64PhyAddr[0] != 0) {
    CVI_SYS_IonFree(frame->stVFrame.u64PhyAddr[0], frame->stVFrame.pu8VirAddr[0]);
    frame->stVFrame.u64PhyAddr[0] = (CVI_U64)0;
    frame->stVFrame.u64PhyAddr[1] = (CVI_U64)0;
    frame->stVFrame.u64PhyAddr[2] = (CVI_U64)0;
    frame->stVFrame.pu8VirAddr[0] = NULL;
    frame->stVFrame.pu8VirAddr[1] = NULL;
    frame->stVFrame.pu8VirAddr[2] = NULL;
  }
  return CVI_SUCCESS;
}
#endif

class ImageProcessor {
 public:
  virtual ~ImageProcessor() {}
  virtual int read(const char *filepath, VIDEO_FRAME_INFO_S *frame, PIXEL_FORMAT_E format) = 0;
  virtual int read_resize(const char *filepath, VIDEO_FRAME_INFO_S *frame, PIXEL_FORMAT_E format,
                          uint32_t width, uint32_t height) = 0;
  virtual int read_centercrop_resize(const char *filepath, VIDEO_FRAME_INFO_S *frame,
                                     PIXEL_FORMAT_E format, uint32_t width, uint32_t height) = 0;
  virtual int opencv_read_float(const char *filepath, VIDEO_FRAME_INFO_S *frame,
                                cvtdl_opencv_params opencv_params) = 0;
  virtual int release(VIDEO_FRAME_INFO_S *frame) = 0;
};

class ImageProcessorNoOpenCV : public ImageProcessor {
 public:
  ImageProcessorNoOpenCV() {
    ive_handle = CVI_IVE_CreateHandle();
    LOGE("ImageProcessorNoOpenCV created.\n");
  }

  ~ImageProcessorNoOpenCV() override {
    if (ive_handle != nullptr) {
      CVI_IVE_DestroyHandle(ive_handle);
      LOGE("ImageProcessorNoOpenCV destroyed..\n");
    }
  }

  int read(const char *filepath, VIDEO_FRAME_INFO_S *frame, PIXEL_FORMAT_E format) override {
    IVE_IMAGE_TYPE_E ive_format;
    int ret = CVI_SUCCESS;
    switch (format) {
      case PIXEL_FORMAT_E::PIXEL_FORMAT_RGB_888:
        ive_format = IVE_IMAGE_TYPE_E::IVE_IMAGE_TYPE_U8C3_PACKAGE;
        break;
      case PIXEL_FORMAT_E::PIXEL_FORMAT_RGB_888_PLANAR:
        ive_format = IVE_IMAGE_TYPE_E::IVE_IMAGE_TYPE_U8C3_PLANAR;
        break;
      default:
        LOGE("format should be PIXEL_FORMAT_RGB_888 or PIXEL_FORMAT_RGB_888_PLANAR\n");
        ive_format = IVE_IMAGE_TYPE_E::IVE_IMAGE_TYPE_U8C1;
        break;
    }

    if (ive_handle != nullptr) {
      image = CVI_IVE_ReadImage(ive_handle, filepath, ive_format);
      int imgw = image.u32Width;
      if (imgw == 0) {
        printf("Read image failed with %x!\n", ret);
        return CVI_FAILURE;
      }
      ret = CVI_IVE_Image2VideoFrameInfo(&image, frame);
      if (ret != CVI_SUCCESS) {
        LOGE("open img failed with %#x!\n", ret);
        return ret;
      } else {
        printf("image read,width:%d\n", frame->stVFrame.u32Width);
      }
    } else {
      LOGE("ive_handle should valuable.\n");
      return CVI_FAILURE;  // Handle error appropriately
    }
    return CVI_SUCCESS;
  }

  int read_resize(const char *filepath, VIDEO_FRAME_INFO_S *frame, PIXEL_FORMAT_E format,
                  uint32_t width, uint32_t height) {
    return 0;
  }

  int read_centercrop_resize(const char *filepath, VIDEO_FRAME_INFO_S *frame, PIXEL_FORMAT_E format,
                             uint32_t width, uint32_t height) {
    return 0;
  }

  int opencv_read_float(const char *filepath, VIDEO_FRAME_INFO_S *frame,
                        cvtdl_opencv_params opencv_params) {
    return 0;
  }

  int release(VIDEO_FRAME_INFO_S *frame) { return CVI_SYS_FreeI(ive_handle, &image); }

 private:
  IVE_HANDLE ive_handle;
  IVE_IMAGE_S image;
};

#ifndef NO_OPENCV
class ImageProcessorOpenCV : public ImageProcessor {
 public:
  int read(const char *filepath, VIDEO_FRAME_INFO_S *frame, PIXEL_FORMAT_E format) override {
    return read_image(filepath, frame, format);
  }

  int read_resize(const char *filepath, VIDEO_FRAME_INFO_S *frame, PIXEL_FORMAT_E format,
                  uint32_t width, uint32_t height) override {
    return read_resize_image(filepath, frame, format, width, height);
  }

  int read_centercrop_resize(const char *filepath, VIDEO_FRAME_INFO_S *frame, PIXEL_FORMAT_E format,
                             uint32_t width, uint32_t height) override {
    return read_centercrop_resize_image(filepath, frame, format, width, height);
  }

  int opencv_read_float(const char *filepath, VIDEO_FRAME_INFO_S *frame,
                        cvtdl_opencv_params opencv_params) override {
    return opencv_read_float_image(filepath, frame, opencv_params);
  }

  int release(VIDEO_FRAME_INFO_S *frame) override { return release_image(frame); }
};
#endif

CVI_S32 CVI_TDL_Create_ImageProcessor(imgprocess_t *handle) {
#ifdef NO_OPENCV
  auto imageProcessor = new ImageProcessorNoOpenCV();
#else
  auto imageProcessor = new ImageProcessorOpenCV();
#endif
  *handle = static_cast<void *>(imageProcessor);
  return 0;
}

CVI_S32 CVI_TDL_ReadImage(imgprocess_t handle, const char *filepath, VIDEO_FRAME_INFO_S *frame,
                          PIXEL_FORMAT_E format) {
  ImageProcessor *ctx = static_cast<ImageProcessor *>(handle);
  return ctx->read(filepath, frame, format);
}

CVI_S32 CVI_TDL_ReadImage_Resize(imgprocess_t handle, const char *filepath,
                                 VIDEO_FRAME_INFO_S *frame, PIXEL_FORMAT_E format, uint32_t width,
                                 uint32_t height) {
  ImageProcessor *ctx = static_cast<ImageProcessor *>(handle);
  return ctx->read_resize(filepath, frame, format, width, height);
}

CVI_S32 CVI_TDL_ReleaseImage(imgprocess_t handle, VIDEO_FRAME_INFO_S *frame) {
  ImageProcessor *ctx = static_cast<ImageProcessor *>(handle);
  return ctx->release(frame);
}

CVI_S32 CVI_TDL_ReadImage_CenrerCrop_Resize(imgprocess_t handle, const char *filepath,
                                            VIDEO_FRAME_INFO_S *frame, PIXEL_FORMAT_E format,
                                            uint32_t width, uint32_t height) {
  ImageProcessor *ctx = static_cast<ImageProcessor *>(handle);
  return ctx->read_centercrop_resize(filepath, frame, format, width, height);
}

CVI_S32 CVI_TDL_OpenCV_ReadImage_Float(imgprocess_t handle, const char *filepath,
                                       VIDEO_FRAME_INFO_S *frame,
                                       cvtdl_opencv_params opencv_params) {
  ImageProcessor *ctx = static_cast<ImageProcessor *>(handle);
  return ctx->opencv_read_float(filepath, frame, opencv_params);
}

CVI_S32 CVI_TDL_Destroy_ImageProcessor(imgprocess_t handle) {
  ImageProcessor *ctx = static_cast<ImageProcessor *>(handle);
  if (ctx) {
    delete ctx;
    ctx = nullptr;
  }
  return 0;
}
