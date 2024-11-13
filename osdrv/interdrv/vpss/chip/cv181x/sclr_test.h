#ifndef _CVI_SCL_TEST_H_
#define _CVI_SCL_TEST_H_

extern CVI_U8 sclr_test_enabled;

CVI_S32 sclr_test_proc_init(struct cvi_vip_dev *dev);
CVI_S32 sclr_test_proc_deinit(void);
void sclr_test_irq_handler(CVI_U32 intr_raw_status);

int sclr_to_vc_sb_ktest(void *data);
int sclr_force_img_in_trigger(void);

#endif /* _CVI_SCL_TEST_H_ */
