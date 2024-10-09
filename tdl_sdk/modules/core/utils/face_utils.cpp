#include "face_utils.hpp"
#include "cvi_tdl_log.hpp"

#include <cvi_gdc.h>
#include <algorithm>
#include "core/core/cvtdl_errno.h"
#include "cvi_comm.h"
#include "opencv2/imgproc.hpp"

using namespace std;

cv::Mat tformfwd(const cv::Mat &trans, const cv::Mat &uv) {
  cv::Mat uv_h = cv::Mat::ones(uv.rows, 3, CV_64FC1);
  uv.copyTo(uv_h(cv::Rect(0, 0, 2, uv.rows)));
  cv::Mat xv_h = uv_h * trans;
  return xv_h(cv::Rect(0, 0, 2, uv.rows));
}

static cv::Mat find_none_flectives_similarity(const cv::Mat &uv, const cv::Mat &xy) {
  cv::Mat A = cv::Mat::zeros(2 * xy.rows, 4, CV_64FC1);
  cv::Mat b = cv::Mat::zeros(2 * xy.rows, 1, CV_64FC1);
  cv::Mat x = cv::Mat::zeros(4, 1, CV_64FC1);

  xy(cv::Rect(0, 0, 1, xy.rows)).copyTo(A(cv::Rect(0, 0, 1, xy.rows)));  // x
  xy(cv::Rect(1, 0, 1, xy.rows)).copyTo(A(cv::Rect(1, 0, 1, xy.rows)));  // y
  A(cv::Rect(2, 0, 1, xy.rows)).setTo(1.);

  xy(cv::Rect(1, 0, 1, xy.rows)).copyTo(A(cv::Rect(0, xy.rows, 1, xy.rows)));    // y
  (xy(cv::Rect(0, 0, 1, xy.rows))).copyTo(A(cv::Rect(1, xy.rows, 1, xy.rows)));  //-x
  A(cv::Rect(1, xy.rows, 1, xy.rows)) *= -1;
  A(cv::Rect(3, xy.rows, 1, xy.rows)).setTo(1.);

  uv(cv::Rect(0, 0, 1, uv.rows)).copyTo(b(cv::Rect(0, 0, 1, uv.rows)));
  uv(cv::Rect(1, 0, 1, uv.rows)).copyTo(b(cv::Rect(0, uv.rows, 1, uv.rows)));

  cv::solve(A, b, x, cv::DECOMP_SVD);
  cv::Mat trans_inv = (cv::Mat_<double>(3, 3) << x.at<double>(0), -x.at<double>(1), 0,
                       x.at<double>(1), x.at<double>(0), 0, x.at<double>(2), x.at<double>(3), 1);
  cv::Mat trans = trans_inv.inv(cv::DECOMP_SVD);
  trans.at<double>(0, 2) = 0;
  trans.at<double>(1, 2) = 0;
  trans.at<double>(2, 2) = 1;

  return trans;
}

static cv::Mat find_similarity(const cv::Mat &uv, const cv::Mat &xy) {
  cv::Mat trans1 = find_none_flectives_similarity(uv, xy);
  cv::Mat xy_reflect = xy;
  xy_reflect(cv::Rect(0, 0, 1, xy.rows)) *= -1;
  cv::Mat trans2r = find_none_flectives_similarity(uv, xy_reflect);
  cv::Mat reflect = (cv::Mat_<double>(3, 3) << -1, 0, 0, 0, 1, 0, 0, 0, 1);

  cv::Mat trans2 = trans2r * reflect;
  cv::Mat xy1 = tformfwd(trans1, uv);

  double norm1 = cv::norm(xy1 - xy);

  cv::Mat xy2 = tformfwd(trans2, uv);
  double norm2 = cv::norm(xy2 - xy);

  cv::Mat trans;
  if (norm1 < norm2) {
    trans = trans1;
  } else {
    trans = trans2;
  }
  return trans;
}

static cv::Mat get_similarity_transform(const vector<cv::Point2f> &src_pts,
                                        const vector<cv::Point2f> &dest_pts, bool reflective) {
  cv::Mat src((int)src_pts.size(), 2, CV_32FC1, (void *)(&src_pts[0].x));
  src.convertTo(src, CV_64FC1);

  cv::Mat dst((int)dest_pts.size(), 2, CV_32FC1, (void *)(&dest_pts[0].x));
  dst.convertTo(dst, CV_64FC1);

  cv::Mat trans = reflective ? find_similarity(src, dst) : find_none_flectives_similarity(src, dst);
  return trans(cv::Rect(0, 0, 2, trans.rows)).t();
}

namespace cvitdl {

inline int getTfmFromFaceInfo(const cvtdl_face_info_t &face_info, const int width, const int height,
                              cv::Mat *tfm) {
  assert(width == 96 || width == 112 || (width == 64 && height == 64));
  assert(height == 112 || (width == 64 && height == 64));

  if (!((width == 96 && width == 112) || (width == 112 && height == 112) ||
        (width == 64 && height == 64))) {
    return -1;
  }

  int ref_width = width;
  int ref_height = height;
  vector<cv::Point2f> detect_points;
  for (int j = 0; j < 5; ++j) {
    cv::Point2f e;
    e.x = face_info.pts.x[j];
    e.y = face_info.pts.y[j];
    detect_points.emplace_back(e);
  }

  vector<cv::Point2f> reference_points;
  if (96 == width) {
    reference_points = {{30.29459953, 51.69630051},
                        {65.53179932, 51.50139999},
                        {48.02519989, 71.73660278},
                        {33.54930115, 92.3655014},
                        {62.72990036, 92.20410156}};
  } else if (112 == width) {
    reference_points = {{38.29459953, 51.69630051},
                        {73.53179932, 51.50139999},
                        {56.02519989, 71.73660278},
                        {41.54930115, 92.3655014},
                        {70.72990036, 92.20410156}};
  } else {
    reference_points = {{21.88262830, 29.53926611},
                        {42.01607013, 29.42789995},
                        {32.01279921, 40.99029482},
                        {23.74127067, 45.7776475},
                        {40.41506506, 45.68542363}};
  }

  for (auto &e : reference_points) {
    e.x += (width - ref_width) / 2.0f;
    e.y += (height - ref_height) / 2.0f;
  }
  *tfm = get_similarity_transform(detect_points, reference_points, true);
  return 0;
}

int face_align(const cv::Mat &image, cv::Mat &aligned, const cvtdl_face_info_t &face_info) {
  cv::Mat tfm;
  if (getTfmFromFaceInfo(face_info, aligned.cols, aligned.rows, &tfm) != 0) {
    return -1;
  }
  cv::warpAffine(image, aligned, tfm, aligned.size(), cv::INTER_NEAREST);
  return 0;
}

int face_align_gdc(const VIDEO_FRAME_INFO_S *inFrame, VIDEO_FRAME_INFO_S *outFrame,
                   const cvtdl_face_info_t &face_info) {
#if defined(CV183X) || defined(CV186X)
  if (inFrame->stVFrame.enPixelFormat != PIXEL_FORMAT_RGB_888_PLANAR &&
      inFrame->stVFrame.enPixelFormat != PIXEL_FORMAT_YUV_PLANAR_420) {
    return -1;
  }
  cv::Mat tfm;
  if (getTfmFromFaceInfo(face_info, outFrame->stVFrame.u32Width, outFrame->stVFrame.u32Height,
                         &tfm) != 0) {
    return -1;
  }
  double t =
      (tfm.at<double>(0, 0) / tfm.at<double>(0, 1) - tfm.at<double>(1, 0) / tfm.at<double>(1, 1));
  double a = 1 / tfm.at<double>(0, 1) / t;
  double b = -1 / tfm.at<double>(1, 1) / t;
  double c =
      (-tfm.at<double>(0, 2) / tfm.at<double>(0, 1) + tfm.at<double>(1, 2) / tfm.at<double>(1, 1)) /
      t;
  vector<cv::Point2f> search_points;
  const float sp_x = outFrame->stVFrame.u32Width - 1;
  const float sp_y = outFrame->stVFrame.u32Height - 1;
  search_points = {{0.0, 0.0}, {sp_x, 0.0}, {0.0, sp_y}, {sp_x, sp_y}};
  AFFINE_ATTR_S stAffineAttr;
  stAffineAttr.u32RegionNum = 1;
  POINT2F_S *face_box = stAffineAttr.astRegionAttr[0];
  int i = 0;
  for (auto &e : search_points) {
    face_box[i].x = e.x * a + e.y * b + c;
    face_box[i].y =
        (e.x - tfm.at<double>(0, 2) - face_box[i].x * tfm.at<double>(0, 0)) / tfm.at<double>(0, 1);
    ++i;
  }
  stAffineAttr.stDestSize = {outFrame->stVFrame.u32Width, outFrame->stVFrame.u32Height};

  GDC_HANDLE hHandle;
  GDC_TASK_ATTR_S stTask;
  stTask.stImgIn = *inFrame;
  stTask.stImgOut = *outFrame;
  CVI_GDC_BeginJob(&hHandle);
  CVI_GDC_AddAffineTask(hHandle, &stTask, &stAffineAttr);
  if (CVI_GDC_EndJob(hHandle) != CVI_SUCCESS) {
    LOGE("Affine failed.\n");
    return -1;
  }
  return 0;
#else
  LOGE("face_align_gdc only supported on CV183X or CV186X");
  return CVI_TDL_ERR_NOT_YET_IMPLEMENTED;
#endif
}

}  // namespace cvitdl
