#include "intrusion_detect.hpp"
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include "assert.h"
#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem.h"
#include "cvi_comm.h"
#include "cvi_tdl_log.hpp"
#include "stdio.h"

#define MIN(a, b) ((a) <= (b) ? (a) : (b))
#define MAX(a, b) ((a) >= (b) ? (a) : (b))

namespace cvitdl {
namespace service {

ConvexPolygon::~ConvexPolygon() {
  CVI_TDL_FreeCpp(&vertices);
  CVI_TDL_FreeCpp(&orthogonals);
}

bool ConvexPolygon::is_convex(float *edges_x, float *edges_y, uint32_t size) {
  for (uint32_t i = 0; i < size; i++) {
    uint32_t j = (i != size - 1) ? (i + 1) : 0;
    /* because the order is clockwise */
    if ((edges_x[i] * edges_y[j]) - (edges_x[j] * edges_y[i]) > 0) {
      return false;
    }
  }
  return true;
}

bool ConvexPolygon::set_vertices(const cvtdl_pts_t &pts) {
  /* calculate edges */
  float *edge_x = new float[pts.size];
  for (uint32_t i = 0; i < pts.size; i++) {
    uint32_t j = ((i + 1) == pts.size) ? 0 : (i + 1);
    edge_x[i] = pts.x[j] - pts.x[i];
  }
  float *edge_y = new float[pts.size];
  for (uint32_t i = 0; i < pts.size; i++) {
    uint32_t j = ((i + 1) == pts.size) ? 0 : (i + 1);
    edge_y[i] = pts.y[j] - pts.y[i];
  }

  /* check the convex condition */
  if (!is_convex(edge_x, edge_y, pts.size)) {
    delete[] edge_x;
    delete[] edge_y;
    return false;
  }

  /* copy vertices */
  this->vertices.size = pts.size;
  this->vertices.x = (float *)malloc(sizeof(float) * pts.size);
  this->vertices.y = (float *)malloc(sizeof(float) * pts.size);
  memcpy(this->vertices.x, pts.x, sizeof(float) * pts.size);
  memcpy(this->vertices.y, pts.y, sizeof(float) * pts.size);

  /* calculate orthogonals */
  this->orthogonals.size = pts.size;
  this->orthogonals.x = (float *)malloc(sizeof(float) * pts.size);
  this->orthogonals.y = (float *)malloc(sizeof(float) * pts.size);
  memcpy(this->orthogonals.x, edge_y, sizeof(float) * pts.size);
  for (uint32_t i = 0; i < pts.size; i++) {
    this->orthogonals.x[i] *= -1.0;
  }
  memcpy(this->orthogonals.y, edge_x, sizeof(float) * pts.size);

  /* delete useless data */
  delete[] edge_x;
  delete[] edge_y;
  return true;
}

IntrusionDetect::IntrusionDetect() {
  base.size = 2;
  base.x = (float *)malloc(sizeof(float) * 2);
  base.y = (float *)malloc(sizeof(float) * 2);
  base.x[0] = 1.;
  base.x[1] = 0.;
  base.y[0] = 0.;
  base.y[1] = 1.;
}

IntrusionDetect::~IntrusionDetect() { CVI_TDL_FreeCpp(&base); }

int IntrusionDetect::setRegion(const cvtdl_pts_t &pts) {
  cvtdl_pts_t new_pts;
  memset(&new_pts, 0, sizeof(cvtdl_pts_t));
  new_pts.size = pts.size;
  new_pts.x = (float *)malloc(sizeof(float) * pts.size);
  new_pts.y = (float *)malloc(sizeof(float) * pts.size);
  memcpy(new_pts.x, pts.x, sizeof(float) * pts.size);
  /* transfer coordinate system from Image to Euclidean */
  for (uint32_t i = 0; i < pts.size; i++) {
    /* (x,y) to (x,-y) */
    new_pts.y[i] = -1. * pts.y[i];
  }
  float area = get_SignedGaussArea(new_pts);
  if (area > 0) {
    /* because input pts is counter-clockwise, it need to be reversed. */
    std::reverse(new_pts.x, new_pts.x + pts.size);
    std::reverse(new_pts.y, new_pts.y + pts.size);
  } else if (area == 0) {
    LOGW("Area = 0\n");
    return CVI_TDL_SUCCESS;
  }

  auto new_region = std::make_shared<ConvexPolygon>();
  if (new_region->set_vertices(new_pts)) {
    regions.push_back(new_region);
    // this->show();
    return CVI_TDL_SUCCESS;
  }

  std::vector<std::vector<int>> convex_idxes;
  if (!ConvexPartition_HM(new_pts, convex_idxes)) {
    LOGE("ConvexPartition_HM Bug (1).\n");
    return CVI_TDL_FAILURE;
  }

  for (size_t i = 0; i < convex_idxes.size(); i++) {
    cvtdl_pts_t new_sub_pts;
    new_sub_pts.size = convex_idxes[i].size();
    new_sub_pts.x = (float *)malloc(sizeof(float) * new_sub_pts.size);
    new_sub_pts.y = (float *)malloc(sizeof(float) * new_sub_pts.size);
    for (size_t j = 0; j < convex_idxes[i].size(); j++) {
      new_sub_pts.x[j] = new_pts.x[convex_idxes[i][j]];
      new_sub_pts.y[j] = new_pts.y[convex_idxes[i][j]];
    }
    auto new_region = std::make_shared<ConvexPolygon>();
    if (!new_region->set_vertices(new_sub_pts)) {
      LOGE("ConvexPartition_HM Bug (2).\n");
      return CVI_TDL_FAILURE;
    }
    regions.push_back(new_region);
    CVI_TDL_FreeCpp(&new_sub_pts);
  }
  CVI_TDL_FreeCpp(&new_pts);

  this->show();
  return CVI_TDL_SUCCESS;
}

void IntrusionDetect::getRegion(cvtdl_pts_t ***region_info, uint32_t *size) {
  *size = regions.size();
  *region_info = (cvtdl_pts_t **)malloc(sizeof(cvtdl_pts_t *) * regions.size());
  for (size_t i = 0; i < regions.size(); i++) {
    (*region_info)[i] = (cvtdl_pts_t *)malloc(sizeof(cvtdl_pts_t));
    cvtdl_pts_t &convex_pts = regions[i]->vertices;
    (*region_info)[i]->size = convex_pts.size;
    (*region_info)[i]->x = (float *)malloc(sizeof(float) * convex_pts.size);
    (*region_info)[i]->y = (float *)malloc(sizeof(float) * convex_pts.size);
    memcpy((*region_info)[i]->x, convex_pts.x, sizeof(float) * convex_pts.size);
    /* because we map (x,y) from Image coordinate system, to (x,-y) */
    for (uint32_t j = 0; j < convex_pts.size; j++) {
      (*region_info)[i]->y[j] = -1 * convex_pts.y[j];
    }
  }
}

void IntrusionDetect::clean() { this->regions.clear(); }

bool IntrusionDetect::run(const cvtdl_bbox_t &bbox) {
  // printf("[RUN] BBox: (%.1f,%.1f,%.1f,%.1f)\n", bbox.x1, bbox.y1, bbox.x2, bbox.y2);
  /* transfer coordinate system from Image to Euclidean */
  cvtdl_bbox_t t_bbox = bbox;
  t_bbox.y1 *= -1;
  t_bbox.y2 *= -1;
  for (size_t i = 0; i < regions.size(); i++) {
    bool separating = false;
    for (size_t j = 0; j < regions[i]->orthogonals.size; j++) {
      vertex_t o;
      o.x = regions[i]->orthogonals.x[j];
      o.y = regions[i]->orthogonals.y[j];
      if (is_separating_axis(o, regions[i]->vertices, t_bbox)) {
        separating = true;
        break;
      }
    }
    if (separating) {
      continue;
    }
    for (size_t j = 0; j < base.size; j++) {
      vertex_t o;
      o.x = base.x[j];
      o.y = base.y[j];
      if (is_separating_axis(o, regions[i]->vertices, t_bbox)) {
        separating = true;
        break;
      }
    }
    if (separating) {
      continue;
    } else {
      return true;
    }
  }
  return false;
}

bool IntrusionDetect::is_separating_axis(const vertex_t &axis, const cvtdl_pts_t &region_pts,
                                         const cvtdl_bbox_t &bbox) {
  float min_1 = std::numeric_limits<float>::max();
  float max_1 = -std::numeric_limits<float>::max();
  float min_2 = std::numeric_limits<float>::max();
  float max_2 = -std::numeric_limits<float>::max();
  float proj;
  for (uint32_t i = 0; i < region_pts.size; i++) {
    proj = axis.x * region_pts.x[i] + axis.y * region_pts.y[i];
    min_1 = MIN(min_1, proj);
    max_1 = MAX(max_1, proj);
  }
  proj = axis.x * bbox.x1 + axis.y * bbox.y1;
  min_2 = MIN(min_2, proj);
  max_2 = MAX(max_2, proj);
  proj = axis.x * bbox.x2 + axis.y * bbox.y1;
  min_2 = MIN(min_2, proj);
  max_2 = MAX(max_2, proj);
  proj = axis.x * bbox.x2 + axis.y * bbox.y2;
  min_2 = MIN(min_2, proj);
  max_2 = MAX(max_2, proj);
  proj = axis.x * bbox.x1 + axis.y * bbox.y2;
  min_2 = MIN(min_2, proj);
  max_2 = MAX(max_2, proj);

  if ((max_1 >= min_2) && (max_2 >= min_1)) {
    return false;
  } else {
    return true;
  }
}

bool IntrusionDetect::is_point_in_triangle(const vertex_t &o, const vertex_t &v1,
                                           const vertex_t &v2, const vertex_t &v3) {
  /* o1 = cross_product(v1-o, v2-o)
   * o2 = cross_product(v2-o, v3-o)
   * o3 = cross_product(v3-o, v1-o)
   */
  float o1 = (v1.x - o.x) * (v2.y - o.y) - (v2.x - o.x) * (v1.y - o.y);
  float o2 = (v2.x - o.x) * (v3.y - o.y) - (v3.x - o.x) * (v2.y - o.y);
  float o3 = (v3.x - o.x) * (v1.y - o.y) - (v1.x - o.x) * (v3.y - o.y);
  bool has_pos = (o1 > 0) || (o2 > 0) || (o3 > 0);
  bool has_neg = (o1 < 0) || (o2 < 0) || (o3 < 0);
  return !(has_pos && has_neg);
}

float IntrusionDetect::get_SignedGaussArea(const cvtdl_pts_t &pts) {
  float area = 0;
  for (uint32_t i = 0; i < pts.size; i++) {
    uint32_t j = (i != pts.size - 1) ? (i + 1) : 0;
    area += pts.x[i] * pts.y[j] - pts.x[j] * pts.y[i];
  }
  return area / 2;
}

bool IntrusionDetect::Triangulate_EC(const cvtdl_pts_t &pts,
                                     std::vector<std::vector<int>> &triangle_idxes) {
  std::vector<uint32_t> active_idxes;
  for (uint32_t i = 0; i < pts.size; i++) {
    active_idxes.push_back(i);
  }
  for (uint32_t t = 0; t < pts.size - 3; t++) {
    int ear_idx = -1;
    int ear_p, ear_i, ear_q;
    for (size_t i = 0; i < active_idxes.size(); i++) {
      size_t p = (i != 0) ? (i - 1) : (active_idxes.size() - 1);
      size_t q = (i != active_idxes.size() - 1) ? (i + 1) : 0;
      uint32_t idx_p = active_idxes[p];
      uint32_t idx_i = active_idxes[i];
      uint32_t idx_q = active_idxes[q];
      /* check convex: cross_product(v[i]-v[p], v[q]-v[i]) */
      if (0 < (pts.x[idx_i] - pts.x[idx_p]) * (pts.y[idx_q] - pts.y[idx_i]) -
                  (pts.x[idx_q] - pts.x[idx_i]) * (pts.y[idx_i] - pts.y[idx_p])) {
        continue;
      }
      vertex_t v1{.x = pts.x[idx_p], .y = pts.y[idx_p]};
      vertex_t v2{.x = pts.x[idx_i], .y = pts.y[idx_i]};
      vertex_t v3{.x = pts.x[idx_q], .y = pts.y[idx_q]};
      /* check point in triangle */
      bool is_ear = true;
      for (size_t j = 0; j < active_idxes.size(); j++) {
        if (j == p || j == i || j == q) {
          continue;
        }
        uint32_t idx_j = active_idxes[j];
        vertex_t v0{.x = pts.x[idx_j], .y = pts.y[idx_j]};
        if (is_point_in_triangle(v0, v1, v2, v3)) {
          is_ear = false;
          break;
        }
      }
      if (is_ear) {
        ear_idx = (int)i;
        ear_p = (int)idx_p;
        ear_i = (int)idx_i;
        ear_q = (int)idx_q;
        break;
      }
    }
    if (ear_idx == -1) {
      LOGE("EAR index not found.");
      return false;
    }
    triangle_idxes.push_back(std::vector<int>({ear_p, ear_i, ear_q}));
    active_idxes.erase(active_idxes.begin() + ear_idx);
  }

  if (active_idxes.size() != 3) {
    LOGE("final active index size != 3.");
    return false;
  }
  triangle_idxes.push_back(
      std::vector<int>({(int)active_idxes[0], (int)active_idxes[1], (int)active_idxes[2]}));

  return true;
}

bool IntrusionDetect::ConvexPartition_HM(const cvtdl_pts_t &pts,
                                         std::vector<std::vector<int>> &convex_idxes) {
  std::vector<std::vector<int>> triangle_idxes;
  if (!Triangulate_EC(pts, triangle_idxes)) {
    LOGE("Triangulate_EC Bug");
    return false;
  }
  bool *valid_triangle = new bool[triangle_idxes.size()];
  std::fill_n(valid_triangle, triangle_idxes.size(), true);
  int a0, a1, a2, b0, b1, b2;
  for (size_t i = 0; i < triangle_idxes.size(); i++) {
    if (!valid_triangle[i]) {
      continue;
    }
    std::cout << std::endl;
    assert(triangle_idxes[i].size() == 3);
    int edge_num = 3;
    a1 = 0;
    while (a1 < edge_num) {
      a0 = (a1 > 0) ? a1 - 1 : edge_num - 1;
      a2 = (a1 + 1) % edge_num;
      bool is_diagonal = false;
      int target_idx = -1;
      for (size_t j = i + 1; j < triangle_idxes.size(); j++) {
        if (!valid_triangle[j]) {
          continue;
        }
        assert(triangle_idxes[j].size() == 3);
        for (b1 = 0; b1 < 3; b1++) {
          b0 = (b1 > 0) ? b1 - 1 : 2;
          b2 = (b1 + 1) % 3;
          if (triangle_idxes[j][b1] == triangle_idxes[i][a2] &&
              triangle_idxes[j][b2] == triangle_idxes[i][a1]) {
            is_diagonal = true;
            target_idx = (int)j;
            break;
          }
        }
        if (is_diagonal) {
          break;
        }
      }
      if (!is_diagonal) {
        a1 += 1;
        continue;
      }
      int a3 = (a2 + 1) % edge_num;
      int b3 = (b2 + 1) % 3;
      assert(b0 == b3);
      vertex_t va_01{.x = pts.x[triangle_idxes[i][a1]] - pts.x[triangle_idxes[i][a0]],
                     .y = pts.y[triangle_idxes[i][a1]] - pts.y[triangle_idxes[i][a0]]};
      vertex_t va_23{.x = pts.x[triangle_idxes[i][a3]] - pts.x[triangle_idxes[i][a2]],
                     .y = pts.y[triangle_idxes[i][a3]] - pts.y[triangle_idxes[i][a2]]};
      vertex_t vb_01{
          .x = pts.x[triangle_idxes[target_idx][b1]] - pts.x[triangle_idxes[target_idx][b0]],
          .y = pts.y[triangle_idxes[target_idx][b1]] - pts.y[triangle_idxes[target_idx][b0]]};
      vertex_t vb_23{
          .x = pts.x[triangle_idxes[target_idx][b3]] - pts.x[triangle_idxes[target_idx][b2]],
          .y = pts.y[triangle_idxes[target_idx][b3]] - pts.y[triangle_idxes[target_idx][b2]]};
      if ((va_01.x * vb_23.y - va_01.y * vb_23.x) < 0 &&
          (vb_01.x * va_23.y - vb_01.y * va_23.x) < 0) {
        triangle_idxes[i].insert(triangle_idxes[i].begin() + a1 + 1,
                                 triangle_idxes[target_idx][b3]);
        edge_num += 1;
        valid_triangle[target_idx] = false;
      }
      a1 += 1;
    }
  }

  for (size_t i = 0; i < triangle_idxes.size(); i++) {
    if (!valid_triangle[i]) {
      continue;
    }
    convex_idxes.push_back(triangle_idxes[i]);
  }

  return true;
}

/* DEBUG CODE*/

void IntrusionDetect::show() {
  std::cout << "Region Num: " << regions.size() << std::endl;
  for (size_t i = 0; i < regions.size(); i++) {
    std::cout << "[" << i << "]\n";
    regions[i]->show();
  }
}

void ConvexPolygon::show() {
  for (size_t i = 0; i < this->vertices.size; i++) {
    std::cout << "(" << std::setw(4) << this->vertices.x[i] << "," << std::setw(4)
              << this->vertices.y[i] << ")";
    if ((i + 1) != this->vertices.size)
      std::cout << "  &  ";
    else
      std::cout << "\n";
  }
  for (size_t i = 0; i < this->orthogonals.size; i++) {
    std::cout << "(" << std::setw(4) << this->orthogonals.x[i] << "," << std::setw(4)
              << this->orthogonals.y[i] << ")";
    if ((i + 1) != this->orthogonals.size)
      std::cout << "  &  ";
    else
      std::cout << "\n";
  }
}

}  // namespace service
}  // namespace cvitdl
