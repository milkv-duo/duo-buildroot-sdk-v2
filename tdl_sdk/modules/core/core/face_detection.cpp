#include "face_detection.hpp"

#include "core/cvi_tdl_types_mem.h"
#include "rescale_utils.hpp"

#include <cmath>
#include "core/core/cvtdl_errno.h"
#include "cvi_sys.h"

#define R_SCALE (1 / (256.0 * 0.229))
#define G_SCALE (1 / (256.0 * 0.224))
#define B_SCALE (1 / (256.0 * 0.225))
#define R_MEAN (0.485 / 0.229)
#define G_MEAN (0.456 / 0.224)
#define B_MEAN (0.406 / 0.225)
#define CROP_PCT 0.875
#define MASK_OUT_NAME "logits_dequant"

namespace cvitdl {}  // namespace cvitdl