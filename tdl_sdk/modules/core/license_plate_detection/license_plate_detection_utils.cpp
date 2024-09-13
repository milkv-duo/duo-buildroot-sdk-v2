#include "license_plate_detection_utils.hpp"

ObjectBBox::ObjectBBox() {
  _cl = -1;
  _prob = 0.0;
}

ObjectBBox::ObjectBBox(int cl, Pts tl, Pts br, float prob) {
  _cl = cl;
  _tl = tl;
  _br = br;
  _prob = prob;
}

ObjectBBox::~ObjectBBox() {}

void ObjectBBox::copy(ObjectBBox &obj_bbox) {
  obj_bbox.set_cl(_cl);
  obj_bbox.set_tl(_tl);
  obj_bbox.set_br(_br);
  obj_bbox.set_prob(_prob);
}

int ObjectBBox::cl() const { return _cl; }

float ObjectBBox::prob() const { return _prob; }

Pts ObjectBBox::tl() const { return _tl; }

Pts ObjectBBox::br() const { return _br; }

Pts ObjectBBox::tr() const {
  Pts _tr;
  _tr << _br(0), _tl(1);
  return _tr;
}

Pts ObjectBBox::bl() const {
  Pts _bl;
  _bl << _tl(0), _br(1);
  return _bl;
}

Pts ObjectBBox::wh() const { return _br - _tl; }

Pts ObjectBBox::cc() const { return _tl + 0.5 * this->wh(); }

float ObjectBBox::area() const { return (this->wh()).prod(); }

bool ObjectBBox::operator<(const ObjectBBox &obj_bbox) const { return _prob < obj_bbox.prob(); }

bool ObjectBBox::operator>(const ObjectBBox &obj_bbox) const { return _prob > obj_bbox.prob(); }

bool ObjectBBox::operator==(const ObjectBBox &obj_bbox) const { return _prob == obj_bbox.prob(); }

void ObjectBBox::set_cl(int cl) { _cl = cl; }
void ObjectBBox::set_tl(const Pts &tl) { _tl = tl; }
void ObjectBBox::set_br(const Pts &br) { _br = br; }
void ObjectBBox::set_prob(float prob) { _prob = prob; }

LicensePlateObjBBox::LicensePlateObjBBox() {
  _cl = -1;
  _prob = 0.0;
}

LicensePlateObjBBox::LicensePlateObjBBox(int cl, CornerPts corner_pts, float prob) {
  Pts tl = corner_pts.rowwise().minCoeff();
  Pts br = corner_pts.rowwise().maxCoeff();
  _corner_pts = corner_pts;
  _cl = cl;
  _tl = tl;
  _br = br;
  _prob = prob;
}

LicensePlateObjBBox::~LicensePlateObjBBox() {}

CornerPts LicensePlateObjBBox::getCornerPts(int vehicle_width, int vehicle_height) const {
  Eigen::Matrix<float, 2, 1> vehicle_wh;
  vehicle_wh << vehicle_width, vehicle_height;
  // TODO:
  //   [fix] CornerPts c_pts = _corner_pts.colwise() * vehicle_wh;
  CornerPts c_pts = _corner_pts;
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 4; j++) {
      c_pts(i, j) = c_pts(i, j) * vehicle_wh(i, 0);
    }
  }
  return c_pts;
}

void nms(std::vector<LicensePlateObjBBox> &ObjectBBoxes,
         std::vector<LicensePlateObjBBox> &SelectedBBoxes, float threshold_iou) {
  // TODO:
  //   [fix]
  // std::sort(ObjectBBoxes.begin(), ObjectBBoxes.end(), std::greater<int>());

  // for (std::vector<LicensePlateObjBBox>::iterator it = ObjectBBoxes.begin();
  //      it != ObjectBBoxes.end(); it++){
  //   bool non_overlap = true;
  //   for (std::vector<LicensePlateObjBBox>::iterator it_p = SelectedBBoxes.begin();
  //        it_p != SelectedBBoxes.end(); it_p++){
  //     if(IOU_ObjBBoxes(*it, *it_p) > threshold_iou){
  //       non_overlap = false;
  //       break;
  //     }
  //   }
  //   if (non_overlap){
  //     SelectedBBoxes.push_back(*it);
  //   }
  // }

  /* Only find the max probability for result */
  if (ObjectBBoxes.size() == 0) return;
  float max_prob = ObjectBBoxes[0].prob();
  int max_idx = 0;
  for (size_t i = 1; i < ObjectBBoxes.size(); i++) {
    if (max_prob < ObjectBBoxes[i].prob()) {
      max_prob = ObjectBBoxes[i].prob();
      max_idx = i;
    }
  }
  SelectedBBoxes.push_back(ObjectBBoxes[max_idx]);
}

float IOU_ObjBBoxes(ObjectBBox const &obj1, ObjectBBox const &obj2) {
  return IOU(obj1.tl(), obj1.br(), obj2.tl(), obj2.br());
}

float IOU(Pts const &tl1, Pts const &br1, Pts const &tl2, Pts const &br2) {
  Pts wh1 = br1 - tl1;
  Pts wh2 = br2 - tl2;
  // Matrix OP reference: http://eigen.tuxfamily.org/dox/group__QuickRefPage.html#title6
  Pts intersection_wh = (br1.cwiseMin(br2) - tl1.cwiseMax(tl2)).cwiseMax(0.0);
  float intersection_area = intersection_wh.prod();
  float area1 = wh1.prod();
  float area2 = wh2.prod();
  float union_area = area1 + area2 - intersection_area;
  return intersection_area / union_area;
}