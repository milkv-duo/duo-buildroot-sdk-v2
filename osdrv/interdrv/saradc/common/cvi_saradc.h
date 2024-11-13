#ifndef __CVI_SARADC_H__
#define __CVI_SARADC_H__

#include <linux/cdev.h>
#include <linux/completion.h>
#include <linux/wait.h>
#include <linux/list.h>

#define SARADC_CTRL		0x004	// control register
#define SARADC_STATUS		0x008	// staus  register
#define SARADC_CYC_SET		0x00c	// saradc waveform setting register
#define SARADC_CH1_RESULT	0x014	// channel 1 result register
#define SARADC_CH2_RESULT	0x018	// channel 2 result register
#define SARADC_CH3_RESULT	0x01c	// channel 3 result register
#define SARADC_INTR_EN		0x020	// interrupt enable register
#define SARADC_INTR_CLR		0x024	// interrupt clear register
#define SARADC_INTR_STA		0x028	// interrupt status register
#define SARADC_INTR_RAW		0x02c	// interrupt raw status register
#define SARADC_TEST			0x030	// Enable self-test mode, active high
#define SARADC_TRIM			0x034	// trim register
#define SARADC_PERIOD_CYCLE	0x038	// bit[0]-bit[23] auto measure in a period
#define SARADC_TEST_FORCE	0x040

#define SARADC_EN_SHIFT		0x0
#define SARADC_SEL_SHIFT	0x4

#define	SARADC_REGS_SIZE (0x2c + 4)
#define	SARADC_REGS_NUM	 (SARADC_REGS_SIZE / 4)

struct cvi_saradc_device {
	struct device *dev;
	struct reset_control *rst_saradc;
	struct clk *clk_saradc;
	dev_t cdev_id;
	struct cdev cdev;
	u8 __iomem *saradc_vaddr;
	u8 __iomem *top_saradc_base_addr;
	u8 __iomem *rtcsys_saradc_base_addr;
	int ic_version_is_new;
	int saradc_irq;
	spinlock_t close_lock;
	int use_count;
	int channel_index;
	void *private_data;
	u32 saradc_ctrl;
	u32 saradc_cyc_set;
	u32 saradc_intr_en;
	u32 saradc_intr_clr;
	u32 saradc_test;
	u32 saradc_trim;
	u32 saradc_period_cycle;
	u32 saradc_test_force;
};

#endif /* __CVI_SARADC_H__ */
