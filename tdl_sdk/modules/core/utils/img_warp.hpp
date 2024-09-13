#pragma once
#include <stdint.h>

namespace cvitdl {
int get_face_transform(const float* landmark_pts, const int width, float* transform);
void warp_affine(const unsigned char* src_data, unsigned int src_step, int src_width,
                 int src_height, unsigned char* dst_data, unsigned int dst_step, int dst_width,
                 int dst_height, float* fM);
}  // namespace cvitdl
