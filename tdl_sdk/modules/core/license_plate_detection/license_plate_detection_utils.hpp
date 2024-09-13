#pragma once
#include <Eigen/Eigen>
#include <vector>

#define NET_STRIDE 16
#define SIDE (float)7.75

#define LICENSE_PLATE_HEIGHT 24
#define LICENSE_PLATE_WIDTH 94

typedef Eigen::Matrix<float, 2, 1> Pts;
typedef Eigen::Matrix<float, 2, 4> CornerPts;

class ObjectBBox {
 public:
  ObjectBBox();
  ObjectBBox(int cl, Pts tl, Pts br, float prob);
  virtual ~ObjectBBox();
  void copy(ObjectBBox &obj_bbox);
  Pts wh() const;
  Pts cc() const;
  Pts tl() const;
  Pts br() const;
  Pts tr() const;
  Pts bl() const;
  int cl() const;
  float prob() const;
  float area() const;
  void set_cl(int cl);
  void set_tl(const Pts &tl);
  void set_br(const Pts &br);
  void set_prob(float prob);

  bool operator<(const ObjectBBox &obj_bbox) const;
  bool operator>(const ObjectBBox &obj_bbox) const;
  bool operator==(const ObjectBBox &obj_bbox) const;

 protected:
  Pts _tl;      // top-left point
  Pts _br;      // bottom-right point
  int _cl;      // class ID
  float _prob;  // probability
};

class LicensePlateObjBBox : public ObjectBBox {
 public:
  LicensePlateObjBBox();
  LicensePlateObjBBox(int cl, CornerPts corner_pts, float prob);
  virtual ~LicensePlateObjBBox();
  CornerPts getCornerPts(int vehicle_width, int vehicle_height) const;

 protected:
  CornerPts _corner_pts;
};

void nms(std::vector<LicensePlateObjBBox> &ObjectBBoxes,
         std::vector<LicensePlateObjBBox> &SelectedBBoxes, float threshold_iou = 0.1);

float IOU_ObjBBoxes(ObjectBBox const &obj1, ObjectBBox const &obj2);

float IOU(Pts const &tl1, Pts const &br1, Pts const &tl2, Pts const &br2);