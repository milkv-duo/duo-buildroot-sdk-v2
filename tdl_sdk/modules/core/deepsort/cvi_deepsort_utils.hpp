#pragma once

#include "cvi_deepsort_types_internal.hpp"

BBOX bbox_tlwh2tlbr(const BBOX &bbox_tlwh);

BBOX bbox_tlwh2xyah(const BBOX &bbox_tlwh);

/* DEBUG CODE */
std::string get_INFO_Vector_Int(const std::vector<int> &idxes, int w = 5);
std::string get_INFO_Vector_Pair_Int_Int(const std::vector<std::pair<int, int>> &pairs, int w = 5);

std::string get_INFO_Match_Pair(const std::vector<std::pair<int, int>> &pairs,
                                const std::vector<int> &idxes, int w = 5);
stRect tlwh2rect(const BBOX &bbox_tlwh);
float get_inter_area(const stRect &box1, const stRect &box2);
float cal_iou(const stRect &box1, const stRect &box2);
float compute_box_sim(stRect box1, stRect box2);

float cal_iou_bbox(const BBOX &box1, const BBOX &box2);
float compute_box_sim_bbox(BBOX box1, BBOX box2);
bool is_bbox_crowded(const std::vector<BBOX> &bboxes, int check_idx, float expand_ratio);
void update_corre(const BBOX &cur_box, const BBOX &pair_box, stCorrelateInfo &prev, float w_cur);
float cal_object_pair_score(stRect boxa, stRect boxb, ObjectType typea, ObjectType typeb,
                            bool strict_order = true);
