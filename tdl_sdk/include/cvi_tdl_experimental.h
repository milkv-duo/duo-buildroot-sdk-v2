#ifndef _CVI_TDL_EXPERIMENTAL_H_
#define _CVI_TDL_EXPERIMENTAL_H_

#include "cvi_tdl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enable GDC hardware acceleration.
 * @ingroup core_tdl_settings
 *
 * @param handle An TDL SDK handle.
 * @param use_gdc Set true to use hardware.
 */
DLL_EXPORT void CVI_TDL_EnableGDC(cvitdl_handle_t handle, bool use_gdc);

#ifdef __cplusplus
}
#endif

#endif  // End of _CVI_TDL_EXPERIMENTAL_H_
