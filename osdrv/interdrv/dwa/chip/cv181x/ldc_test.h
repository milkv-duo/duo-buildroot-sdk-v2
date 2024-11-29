#ifndef _LDC_TEST_H_
#define _LDC_TEST_H_

#include <linux/cvi_type.h>

extern CVI_U8 ldc_test_enabled;

void ldc_dump_register(void);
CVI_S32 ldc_test_proc_init(void);
CVI_S32 ldc_test_proc_deinit(void);

void ldc_test_irq_handler(CVI_U32 intr_raw_status);

#endif /* _LDC_TEST_H_ */
