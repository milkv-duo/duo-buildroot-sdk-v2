/** Intrusion Detection base on 2-dimensional convex polygons
 *    collision detection (using the Separating Axis Theorem)
 */
#pragma once
#include <memory>
#include <vector>
#include "core/core/cvtdl_core_types.h"

namespace cvitdl {
namespace service {

typedef struct {
  float x;
  float y;
} vertex_t;

class ConvexPolygon {
 public:
  ConvexPolygon() = default;
  ~ConvexPolygon();
  void show();
  bool set_vertices(const cvtdl_pts_t &pts);

  /* member data */
  cvtdl_pts_t vertices;
  cvtdl_pts_t orthogonals;

 private:
  bool is_convex(float *edges_x, float *edges_y, uint32_t size);
};

class IntrusionDetect {
 public:
  IntrusionDetect();
  ~IntrusionDetect();
  int setRegion(const cvtdl_pts_t &pts);
  void getRegion(cvtdl_pts_t ***region_info, uint32_t *size);
  void clean();
  bool run(const cvtdl_bbox_t &bbox);
  void show();

 private:
  bool is_separating_axis(const vertex_t &axis, const cvtdl_pts_t &region_pts,
                          const cvtdl_bbox_t &bbox);
  bool is_point_in_triangle(const vertex_t &o, const vertex_t &v1, const vertex_t &v2,
                            const vertex_t &v3);
  float get_SignedGaussArea(const cvtdl_pts_t &pts);
  bool Triangulate_EC(const cvtdl_pts_t &pts, std::vector<std::vector<int>> &triangle_idxes);
  bool ConvexPartition_HM(const cvtdl_pts_t &pts, std::vector<std::vector<int>> &convex_idxes);

  /* member data */
  std::vector<std::shared_ptr<ConvexPolygon>> regions;
  cvtdl_pts_t base;
};

}  // namespace service
}  // namespace cvitdl