#ifndef _YOLO_H_
#define _YOLO_H_

typedef enum { v2, v3 } YoloType;

typedef struct {
  float x, y, w, h;
} box;

typedef struct {
  box bbox;
  int classes;
  float *prob;
  float objectness;
  int sort_class;
} detection;

typedef struct {
  float *data;
  int norm;
  int channels;
  int width;
  int height;
} YOLOLayer;

typedef struct {
  int m_classes;
  float m_biases[20];
  float m_nms_threshold;
  int m_anchor_nums;
  int m_coords;
  int m_batch;
  YoloType type;
  int m_mask[3][3];
} YOLOParamter;

/**
 * @breif  calculate index in outputs with some parameter with anchor,
 *         one anchor include [x, y, w, h, c, C1, C2....,Cn] information,
 *         x, y ,w, h using with location, c is mean confidence, and C1 to Cn is classes
 *         which index of anchor we need get, is use the parameter of entry.
 *         when entry = 0,  geting the anchor in outputs index, when entry = 4, geting the
 *confidence of this anchor entry = 5, get the index of C1,
 * @param w feature map width
 * @param h feature map height
 * @param batch batch value
 * @param location use to get Nth anchor and loc(class + coords + 1)
 * @param entry entry of offset]
 **/
int EntryIndex(int w, int h, int classes, int batch, int location, int entry, int outputs);
void ActivateArray(float *x, const int n, bool fast_exp);
void DoNmsSort(detection *dets, int total, int classes, float threshold);
detection *GetNetworkBoxes(YOLOLayer net_output, int classes, int w, int h, float threshold,
                           int relative, int *num, YOLOParamter yolo_param, int index);

#endif