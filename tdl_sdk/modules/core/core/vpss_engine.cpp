#include "vpss_engine.hpp"
#include "core/utils/vpss_helper.h"
#include "cvi_tdl_log.hpp"

namespace cvitdl {

VpssEngine::VpssEngine(VPSS_GRP desired_grp_id, CVI_U8 device)
    : m_desired_grp_id(desired_grp_id), m_dev(device) {
  init();
}

VpssEngine::~VpssEngine() { stop(); }

int VpssEngine::init() {
  if (m_is_vpss_init) {
    LOGW("Vpss already init.\n");
    return CVI_FAILURE;
  }
  int s32Ret = CVI_SYS_Init();
  if (s32Ret != CVI_SUCCESS) {
    LOGE("CVI_SYS_Init failed!\n");
    return s32Ret;
  }
#ifndef CV186X
  if (CVI_SYS_GetVPSSMode() == VPSS_MODE_DUAL) {
    // FIXME: Currently hardcoded due to no define in mmf.
    m_available_max_chn = VPSS_MAX_CHN_NUM - 1;
  }
#endif

  VPSS_GRP_ATTR_S vpss_grp_attr;
  VPSS_CHN_ATTR_S vpss_chn_attr;
  // Not magic number, only for init.
  uint32_t width = 100;
  uint32_t height = 100;
  m_enabled_chn = 1;
  VPSS_GRP_DEFAULT_HELPER2(&vpss_grp_attr, width, height, VI_PIXEL_FORMAT, m_dev);
  VPSS_CHN_DEFAULT_HELPER(&vpss_chn_attr, width, height, PIXEL_FORMAT_RGB_888_PLANAR, true);

  /*start vpss*/
  m_grpid = -1;
  if (m_desired_grp_id != (VPSS_GRP)-1) {
    LOGI("use specific  groupid:%d", (int)m_desired_grp_id);
    if (CVI_VPSS_CreateGrp(m_desired_grp_id, &vpss_grp_attr) != CVI_SUCCESS) {
      LOGE("User assign group id %u failed to create vpss instance.\n", m_desired_grp_id);
      return CVI_FAILURE;
    }
    m_grpid = m_desired_grp_id;
  } else {
    int id = CVI_VPSS_GetAvailableGrp();
    LOGI("got available groupid:%d", id);
    if (CVI_VPSS_CreateGrp(id, &vpss_grp_attr) != CVI_SUCCESS) {
      LOGE("User assign group id %u failed to create vpss instance.\n", id);
      return CVI_FAILURE;
    }
    m_grpid = id;
  }
  if (m_grpid == (VPSS_GRP)-1) {
    LOGE("All vpss grp init failed!\n");
    return CVI_FAILURE;
  }

  LOGI("Create Vpss Group(%d) Dev(%d)\n", m_grpid, m_dev);

  s32Ret = CVI_VPSS_ResetGrp(m_grpid);
  if (s32Ret != CVI_SUCCESS) {
    LOGE("CVI_VPSS_ResetGrp(grp:%d) failed with %#x!\n", m_grpid, s32Ret);
    return CVI_FAILURE;
  }

  for (uint32_t i = 0; i < m_enabled_chn; i++) {
    s32Ret = CVI_VPSS_SetChnAttr(m_grpid, i, &vpss_chn_attr);

    if (s32Ret != CVI_SUCCESS) {
      LOGE("CVI_VPSS_SetChnAttr failed with %#x\n", s32Ret);
      return CVI_FAILURE;
    }

    s32Ret = CVI_VPSS_EnableChn(m_grpid, i);

    if (s32Ret != CVI_SUCCESS) {
      LOGE("CVI_VPSS_EnableChn failed with %#x\n", s32Ret);
      return CVI_FAILURE;
    }
  }
  s32Ret = CVI_VPSS_StartGrp(m_grpid);
  if (s32Ret != CVI_SUCCESS) {
    LOGE("start vpss group failed. s32Ret: 0x%x !\n", s32Ret);
    return CVI_FAILURE;
  }

  memset(&m_crop_attr_reset, 0, sizeof(VPSS_CROP_INFO_S));
  m_is_vpss_init = true;
  return CVI_SUCCESS;
}

bool VpssEngine::isInitialized() const { return m_is_vpss_init; }

void VpssEngine::attachVBPool(VB_POOL pool_id) { m_vbpool_id = pool_id; }

VB_POOL VpssEngine::getVBPool() const { return m_vbpool_id; }

int VpssEngine::stop() {
  if (!m_is_vpss_init) {
    LOGI("Cannot stop Vpss because it's not initalized yet.\n");
    return CVI_SUCCESS;
  }

  for (uint32_t j = 0; j < m_enabled_chn; j++) {
    int s32Ret = CVI_VPSS_DisableChn(m_grpid, j);
    if (s32Ret != CVI_SUCCESS) {
      LOGE("CVI_VPSS_DisableChn failed with %#x!\n", s32Ret);
      return CVI_FAILURE;
    }
  }

  int s32Ret = CVI_VPSS_StopGrp(m_grpid);
  if (s32Ret != CVI_SUCCESS) {
    LOGE("CVI_VPSS_StopGrp failed with %#x!\n", s32Ret);
    return CVI_FAILURE;
  }

  s32Ret = CVI_VPSS_DestroyGrp(m_grpid);
  if (s32Ret != CVI_SUCCESS) {
    LOGE("CVI_VPSS_DestroyGrp failed with %#x!\n", s32Ret);
    return CVI_FAILURE;
  }

  m_is_vpss_init = false;
  return CVI_SUCCESS;
}

VPSS_GRP VpssEngine::getGrpId() {
  if (isInitialized()) {
    return m_grpid;
  } else {
    return m_desired_grp_id;
  }
}

int VpssEngine::sendFrameBase(const VIDEO_FRAME_INFO_S *frame,
                              const VPSS_CROP_INFO_S *grp_crop_attr,
                              const VPSS_CROP_INFO_S *chn_crop_attr,
                              const VPSS_CHN_ATTR_S *chn_attr, const VPSS_SCALE_COEF_E *coeffs,
                              const uint32_t enable_chns) {
  if (enable_chns >= m_enabled_chn) {
    for (uint32_t i = m_enabled_chn; i < enable_chns; i++) {
      CVI_VPSS_EnableChn(m_grpid, i);
    }
  } else {
    for (uint32_t i = enable_chns; i < m_enabled_chn; i++) {
      CVI_VPSS_DisableChn(m_grpid, i);
    }
  }
  m_enabled_chn = enable_chns;

  VPSS_GRP_ATTR_S vpss_grp_attr;
  LOGI("framew:%u,frameh:%u,pixelformat:%d\n", frame->stVFrame.u32Width, frame->stVFrame.u32Height,
       frame->stVFrame.enPixelFormat);
  VPSS_GRP_DEFAULT_HELPER2(&vpss_grp_attr, frame->stVFrame.u32Width, frame->stVFrame.u32Height,
                           frame->stVFrame.enPixelFormat, m_dev);

  if (m_enabled_chn > m_available_max_chn) {
    LOGE("Exceed max available channel %u. Current: %u.\n", m_available_max_chn, m_enabled_chn);
    return CVI_FAILURE;
  }
  int ret = CVI_VPSS_SetGrpAttr(m_grpid, &vpss_grp_attr);
  if (ret != CVI_SUCCESS) {
    LOGE("CVI_VPSS_SetGrpAttr failed with %#x\n", ret);
    return ret;
  }
  if (grp_crop_attr != NULL) {
    int ret = CVI_VPSS_SetGrpCrop(m_grpid, grp_crop_attr);
    if (ret != CVI_SUCCESS) {
      LOGE("CVI_VPSS_SetGrpCrop failed with %#x\n", ret);
      return ret;
    }
  } else {
    // Reset crop settings
    CVI_VPSS_SetGrpCrop(m_grpid, &m_crop_attr_reset);
  }

  for (uint32_t i = 0; i < m_enabled_chn; i++) {
    ret = CVI_VPSS_SetChnAttr(m_grpid, i, &chn_attr[i]);
    if (ret != CVI_SUCCESS) {
      LOGE("CVI_VPSS_SetChnAttr failed with %#x\n", ret);
      return ret;
    }
    if (m_vbpool_id != VB_INVALID_POOLID) {
      // Attach vb pool before vpss processing.
      ret = CVI_VPSS_AttachVbPool(m_grpid, i, m_vbpool_id);
      if (ret != CVI_SUCCESS) {
        LOGE("Cannot attach vb pool to vpss(grp: %d, chn: %d), ret=%#x\n", m_grpid, 0, ret);
        return CVI_FAILURE;
      }
    }
  }

  if (chn_crop_attr != NULL) {
    for (uint32_t i = 0; i < m_enabled_chn; i++) {
      int ret = CVI_VPSS_SetChnCrop(m_grpid, i, &chn_crop_attr[i]);
      if (ret != CVI_SUCCESS) {
        LOGE("CVI_VPSS_SetChnCrop failed with %#x\n", ret);
        return ret;
      }
    }
  } else {  // if not enable crop, cleanup crop attributes for all channels.
    for (uint32_t i = 0; i < m_enabled_chn; i++) {
      CVI_VPSS_SetChnCrop(m_grpid, i, &m_crop_attr_reset);
    }
  }

  if (coeffs != NULL) {
    for (uint32_t i = 0; i < m_enabled_chn; i++) {
      int ret = CVI_VPSS_SetChnScaleCoefLevel(m_grpid, i, coeffs[i]);
      if (ret != CVI_SUCCESS) {
        LOGE("CVI_VPSS_GetChnScaleCoefLevel failed with %#x\n", ret);
        return ret;
      }
    }
  } else {
    for (uint32_t i = 0; i < m_enabled_chn; i++) {
      // Default value
      CVI_VPSS_SetChnScaleCoefLevel(m_grpid, i, VPSS_SCALE_COEF_BICUBIC);
    }
  }

  ret = CVI_VPSS_SendFrame(m_grpid, frame, -1);
  // Detach vb pool when process is finished.
  if (ret != CVI_SUCCESS && m_vbpool_id != VB_INVALID_POOLID) {
    for (uint32_t i = 0; i < m_enabled_chn; i++) {
      CVI_VPSS_DetachVbPool(m_grpid, i);
    }
  }
  return ret;
}

int VpssEngine::sendFrame(const VIDEO_FRAME_INFO_S *frame, const VPSS_CHN_ATTR_S *chn_attr,
                          const uint32_t enable_chns) {
  // grp_crop_attr=null,chn_crop_attr=null,coeffs=null
  return sendFrameBase(frame, NULL, NULL, chn_attr, NULL, enable_chns);
}

int VpssEngine::sendFrame(const VIDEO_FRAME_INFO_S *frame, const VPSS_CHN_ATTR_S *chn_attr,
                          const VPSS_SCALE_COEF_E *coeffs, const uint32_t enable_chns) {
  return sendFrameBase(frame, NULL, NULL, chn_attr, coeffs, enable_chns);
}

int VpssEngine::sendCropGrpFrame(const VIDEO_FRAME_INFO_S *frame, const VPSS_CROP_INFO_S *crop_attr,
                                 const VPSS_CHN_ATTR_S *chn_attr, const uint32_t enable_chns) {
  return sendFrameBase(frame, crop_attr, NULL, chn_attr, NULL, enable_chns);
}

int VpssEngine::sendCropChnFrame(const VIDEO_FRAME_INFO_S *frame, const VPSS_CROP_INFO_S *crop_attr,
                                 const VPSS_CHN_ATTR_S *chn_attr, const uint32_t enable_chns) {
  return sendFrameBase(frame, NULL, crop_attr, chn_attr, NULL, enable_chns);
}

int VpssEngine::sendCropChnFrame(const VIDEO_FRAME_INFO_S *frame, const VPSS_CROP_INFO_S *crop_attr,
                                 const VPSS_CHN_ATTR_S *chn_attr, const VPSS_SCALE_COEF_E *coeffs,
                                 const uint32_t enable_chns) {
  return sendFrameBase(frame, NULL, crop_attr, chn_attr, coeffs, enable_chns);
}

int VpssEngine::sendCropGrpChnFrame(const VIDEO_FRAME_INFO_S *frame,
                                    const VPSS_CROP_INFO_S *grp_crop_attr,
                                    const VPSS_CROP_INFO_S *chn_crop_attr,
                                    const VPSS_CHN_ATTR_S *chn_attr, const uint32_t enable_chns) {
  return sendFrameBase(frame, grp_crop_attr, chn_crop_attr, chn_attr, NULL, enable_chns);
}

int VpssEngine::getFrame(VIDEO_FRAME_INFO_S *outframe, int chn_idx, uint32_t timeout) {
  int ret = CVI_VPSS_GetChnFrame(m_grpid, chn_idx, outframe, timeout);
  if (m_vbpool_id != VB_INVALID_POOLID) {
    CVI_VPSS_DetachVbPool(m_grpid, chn_idx);
  }
  return ret;
}

int VpssEngine::releaseFrame(VIDEO_FRAME_INFO_S *frame, int chn_idx) {
  return CVI_VPSS_ReleaseChnFrame(m_grpid, chn_idx, frame);
}
}  // namespace cvitdl
