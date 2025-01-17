#ifndef _CVI_DRAW_IVE_H_
#define _CVI_DRAW_IVE_H_
#ifdef CV180X
#include "linux/cvi_type.h"
#else
#include "cvi_type.h"
#endif

/**
 * @brief IVE point structure.
 *
 */
typedef struct IVE_PTS {
  uint16_t x;
  uint16_t y;
} IVE_PTS_S;

/**
 * @brief IVE rectangle structure.
 *
 */
typedef struct IVE_RECT {
  IVE_PTS_S pts[2];
} IVE_RECT_S;

/**
 * @brief IVE color structure.
 *
 */
typedef struct IVE_COLOR {
  CVI_U8 r;
  CVI_U8 g;
  CVI_U8 b;
} IVE_COLOR_S;

/**
 * @brief IVE draw rect control parameters.
 *
 */
typedef struct IVE_DRAW_RECT_CTRL {
  CVI_U8 numsOfRect;
  IVE_RECT_S *rect;
  IVE_COLOR_S color;
  CVI_U8 thickness;  // Reserved variable.
} IVE_DRAW_RECT_CTRL_S;

#endif  // End of _CVI_DRAW_IVE_H_