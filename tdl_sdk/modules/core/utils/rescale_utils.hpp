#pragma once
#include "core/face/cvtdl_face_types.h"
#include "core/object/cvtdl_object_types.h"

namespace cvitdl {

cvtdl_bbox_t box_rescale_c(const float frame_width, const float frame_height, const float nn_width,
                           const float nn_height, const cvtdl_bbox_t bbox, float *ratio,
                           float *pad_width, float *pad_height);
cvtdl_bbox_t box_rescale_rb(const float frame_width, const float frame_height, const float nn_width,
                            const float nn_height, const cvtdl_bbox_t bbox, float *ratio);

cvtdl_bbox_t __attribute__((visibility("default")))
box_rescale(const float frame_width, const float frame_height, const float nn_width,
            const float nn_height, const cvtdl_bbox_t bbox, const meta_rescale_type_e type);

cvtdl_dms_od_info_t info_rescale_c(const float width, const float height, const float new_width,
                                   const float new_height, const cvtdl_dms_od_info_t &dms_info);
cvtdl_face_info_t info_rescale_c(const float width, const float height, const float new_width,
                                 const float new_height, const cvtdl_face_info_t &face_info);
cvtdl_object_info_t info_rescale_c(const float width, const float height, const float new_width,
                                   const float new_height, const cvtdl_object_info_t &object_info);
cvtdl_face_info_t info_rescale_rb(const float width, const float height, const float new_width,
                                  const float new_height, const cvtdl_face_info_t &face_info);
cvtdl_object_info_t info_extern_crop_resize_img(const float frame_width, const float frame_height,
                                                const cvtdl_object_info_t *obj_info,
                                                float w_pad_ratio = 0.2, float h_pad_ratio = 0.2);
cvtdl_face_info_t info_extern_crop_resize_img(const float frame_width, const float frame_height,
                                              const cvtdl_face_info_t *face_info, int *p_dst_size,
                                              float w_pad_ratio = 0.2, float h_pad_ratio = 0.2);
void info_rescale_nocopy_c(const float width, const float height, const float new_width,
                           const float new_height, cvtdl_face_info_t *face_info);
void info_rescale_nocopy_rb(const float width, const float height, const float new_width,
                            const float new_height, cvtdl_face_info_t *face_info);
cvtdl_face_info_t info_rescale_c(const float new_width, const float new_height,
                                 const cvtdl_face_t &face_meta, const int face_idx);
cvtdl_object_info_t info_rescale_c(const float new_width, const float new_height,
                                   const cvtdl_object_t &object_meta, const int object_idx);
cvtdl_face_info_t info_rescale_rb(const float new_width, const float new_height,
                                  const cvtdl_face_t &face_meta, const int face_idx);

}  // namespace cvitdl
