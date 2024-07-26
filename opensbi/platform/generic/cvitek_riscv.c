/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <platform_override.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/fdt/fdt_fixup.h>
#include <sbi/sbi_system.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_string.h>
#include <sbi/sbi_types.h>
#include <sbi/sbi_error.h>
#include <sbi/riscv_asm.h>
#include <sbi/riscv_io.h>

__asm__(".section .rodata\n"
	".global suspend_sram_entry\n"
	".global suspend_sram_end\n"
	".type suspend_sram_entry, @object\n"
	".type suspend_sram_end, @object\n"
	".align 4\n"
	"suspend_sram_entry:\n"
	".incbin \"" STRINGIFY(PM_SRAM_BIN_PATH) "\"\n"
	".align 4\n"
	"suspend_sram_end:\n"
	".text\n");

#define RTC_SRAM_FLAG_ADDR 0x05026ff8
#ifdef CONFIG_CV180X
#define SUSPEND_SRAM_ENTRY 0x3C000000
#endif
#ifdef CONFIG_CV181X
#define SUSPEND_SRAM_ENTRY 0xC030000
#endif
#define memcpy sbi_memcpy

static void rtc_latch_pinmux_settings(void)
{
	writel(0x2, (void *)0x50250ac);
	writel(0x0, (void *)0x5027084);
	//TODO
}

static void rtc_power_saving_settings_for_suspend(void)
{
	//TODO
}

static int cvitek_sbi_system_suspend_check(u32 sleep_type)
{
	return sleep_type == SBI_SUSP_SLEEP_TYPE_SUSPEND ? 0 : SBI_EINVAL;
}

static int cvitek_sbi_system_suspend(u32 sleep_type,
				unsigned long mmode_resume_addr)
{
	void (*suspend)(void) = (void *)SUSPEND_SRAM_ENTRY;

	if (sleep_type != SBI_SUSP_SLEEP_TYPE_SUSPEND)
		return SBI_EINVAL;

	rtc_latch_pinmux_settings();
	rtc_power_saving_settings_for_suspend();

	/* store warmboot entry for resume*/
	writel(mmode_resume_addr, (void *)RTC_SRAM_FLAG_ADDR);
	writel(readl((void *)0x03002000) | 0x10, (void *)0x03002000); //enable TPU clock
	memcpy((void *)SUSPEND_SRAM_ENTRY, suspend_sram_entry, suspend_sram_end - suspend_sram_entry);

	asm volatile("fence.i" ::: "memory");

	asm volatile("fence rw,rw\n\t");

	suspend();

	wfi();

	return SBI_OK;
}

static struct sbi_system_suspend_device cvitek_sbi_suspend_device = {
	.name = "cvi-suspend",
	.system_suspend_check = cvitek_sbi_system_suspend_check,
	.system_suspend = cvitek_sbi_system_suspend,
};

static int cvitek_riscv_early_init(bool cold_boot, const struct fdt_match *match)
{
	sbi_system_suspend_set_device(&cvitek_sbi_suspend_device);

	return 0;
}

static const struct fdt_match cvitek_riscv_match[] = {
	{ .compatible = "cvitek,cv180x" },
	{ .compatible = "cvitek,cv181x" }
};

const struct platform_override cvitek_riscv = {
	.match_table = cvitek_riscv_match,
	.early_init = cvitek_riscv_early_init,
};
