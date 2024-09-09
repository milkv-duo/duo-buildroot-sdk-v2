/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: cvi_mipi_tx.h
 * Description:
 */

#ifndef __CVI_MIPI_TX_H__
#define __CVI_MIPI_TX_H__

#include <linux/cvi_comm_mipi_tx.h>
/**
 * @brief MIPI TX Device attribute settings.
 *
 * @param fd(In), device handle.
 * @param dev_cfg(In), dev attribute.
 * @return CVI_S32 Return CVI_SUCCESS if succeed.
 */
int CVI_MIPI_TX_Cfg(int fd, struct combo_dev_cfg_s *dev_cfg);
/**
 * @brief Display Command Set.
 *
 * @param fd(In), device handle.
 * @param cmd_info(In), cmd info.
 * @return CVI_S32 Return CVI_SUCCESS if succeed.
 */
int CVI_MIPI_TX_SendCmd(int fd, struct cmd_info_s *cmd_info);
/**
 * @brief Display Command Get.
 *
 * @param fd(In), device handle.
 * @param cmd_info(out), cmd info.
 * @return CVI_S32 Return CVI_SUCCESS if succeed.
 */
int CVI_MIPI_TX_RecvCmd(int fd, struct get_cmd_info_s *cmd_info);
/**
 * @brief MIPI TX Device enable.
 *
 * @param fd(In), device handle.
 * @return CVI_S32 Return CVI_SUCCESS if succeed.
 */
int CVI_MIPI_TX_Enable(int fd);
/**
 * @brief MIPI TX Device disable.
 *
 * @param fd(In), device handle.
 * @return CVI_S32 Return CVI_SUCCESS if succeed.
 */
int CVI_MIPI_TX_Disable(int fd);
/**
 * @brief MIPI TX hs prepare/zero/trail time set.
 *
 * @param fd(In), device handle.
 * @param hs_cfg(In), hs_settle attribute.
 * @return CVI_S32 Return CVI_SUCCESS if succeed.
 */
int CVI_MIPI_TX_SetHsSettle(int fd, const struct hs_settle_s *hs_cfg);
/**
 * @brief MIPI TX hs prepare/zero/trail time get.
 *
 * @param fd(In), device handle.
 * @param hs_cfg(out), hs_settle attribute.
 * @return CVI_S32 Return CVI_SUCCESS if succeed.
 */
int CVI_MIPI_TX_GetHsSettle(int fd, struct hs_settle_s *hs_cfg);

#endif // __CVI_MIPI_TX_H__

