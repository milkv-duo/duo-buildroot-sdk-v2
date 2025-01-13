#pragma once

#ifndef __CV186X__
#include <cviruntime.h>
#endif
#include "cvi_comm.h"

namespace cvitdl {

#ifndef __CV186X__
inline const char *get_tpu_error_msg(int code) {
  switch (code) {
    case CVI_RC_AGAIN:
      return "TPU not ready yet.";
    case CVI_RC_FAILURE:
      return "TPU general failure.";
    case CVI_RC_TIMEOUT:
      return "TPU timeout.";
    case CVI_RC_UNINIT:
      return "TPU uninitialized.";
    case CVI_RC_INVALID_ARG:
      return "invalidate arguments.";
    case CVI_RC_NOMEM:
      return "Not enough memory.";
    case CVI_RC_DATA_ERR:
      return "Data Error.";
    case CVI_RC_BUSY:
      return "TPU is busy.";
    case CVI_RC_UNSUPPORT:
      return "Unsupport operation.";
    default:
      return "Unknown error.";
  };
}
#endif

inline const char *get_vpss_error_msg(int code) {
  switch (code) {
    case CVI_ERR_VPSS_NULL_PTR:
      return "VPSS null pointer.";
    case CVI_ERR_VPSS_NOTREADY:
      return "VPSS not ready yet.";
    case CVI_ERR_VPSS_INVALID_DEVID:
      return "Invalidate vpss device id";
    case CVI_ERR_VPSS_INVALID_CHNID:
      return "Invalidate vpss channel id";
    case CVI_ERR_VPSS_EXIST:
      return "VPSS group id is occupied.";
    case CVI_ERR_VPSS_UNEXIST:
      return "VPSS group id isn't created yet.";
    case CVI_ERR_VPSS_NOT_SUPPORT:
      return "Unsupported VPSS operation.";
    case CVI_ERR_VPSS_NOT_PERM:
      return "VPSS operation is not permitted";
    case CVI_ERR_VPSS_NOMEM:
      return "VPSS cannot get memory.";
    case CVI_ERR_VPSS_NOBUF:
      return "Failed to allocate buffer.";
    case CVI_ERR_VPSS_ILLEGAL_PARAM:
      return "Invalidate VPSS parameter.";
    case CVI_ERR_VPSS_BUSY:
      return "VPSS is busy.";
    case CVI_ERR_VPSS_BUF_EMPTY:
      return "VPSS cannot get buffer from an empty queue.";
    default:
      return "Unknown error.";
  };
}
}  // namespace cvitdl
