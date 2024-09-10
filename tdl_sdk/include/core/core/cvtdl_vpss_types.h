#ifndef _CVI_VPSS_TYPES_H_
#define _CVI_VPSS_TYPES_H_

#include "cvi_comm.h"

#include <stdint.h>
#include <stdlib.h>

/**
 * \defgroup core_cvitdlvpss CVI_TDL VPSS Related Module
 */

/** @struct cvtdl_vpssconfig_t
 * @ingroup core_cvitdlvpss
 * @brief A structure wrapper that returns the scale quantization parameters of a loaded model.
 */
typedef struct {
  VPSS_SCALE_COEF_E chn_coeff;
  VPSS_CHN_ATTR_S chn_attr;
} cvtdl_vpssconfig_t;

#endif
