/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_test.h
 * Description:
 *
 */

#ifndef _ISP_TEST_H_
#define _ISP_TEST_H_

#include <linux/cvi_type.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

// int isp_block_mpi_getest(int ViPipe, int blockNum);
CVI_S32 isp_block_mpi_setest(VI_PIPE ViPipe, CVI_S32 blockNum, CVI_S32 random);
CVI_VOID isp_test_mpi_test(void);
CVI_S32 isp_test_set_debug_level(void);
CVI_VOID isp_gpio_ctrl(void);

struct isp_test_ops {
	CVI_S32 (*statistic_test)(CVI_VOID);
	CVI_S32 (*module_ctrl_test)(CVI_VOID);
	CVI_S32 (*fw_state_test)(CVI_VOID);
	CVI_S32 (*aaa_bind_test)(CVI_VOID);
	CVI_S32 (*reg_set_test)(CVI_VOID);
	CVI_S32(*dcf_info_test)(CVI_VOID);
	CVI_S32 (*raw_replay_test)(CVI_VOID);
};

extern struct isp_test_ops isp_api_test;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_TEST_H_
