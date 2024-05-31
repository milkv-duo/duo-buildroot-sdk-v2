#ifndef _TYPE_DEFINE_H_
#define _TYPE_DEFINE_H_

#define FD_MAX_DET_NUM    (10)
#ifndef ALIGN
#define ALIGN(x,y) (x + y -1) & ~(y - 1)
#endif

typedef struct face_rect_s {
  float x1;
  float y1;
  float x2;
  float y2;
  float score;
  float landmarks[5][2];
} face_rect_t;

#endif