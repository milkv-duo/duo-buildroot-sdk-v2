/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 *   Nick Kossifidis <mick@ics.forth.gr>
 */

#include <sbi/riscv_asm.h>
#include <sbi/sbi_bitops.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_hsm.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_system.h>
#include <sbi/sbi_ipi.h>
#include <sbi/sbi_init.h>

static const struct sbi_system_reset_device *reset_dev = NULL;

const struct sbi_system_reset_device *sbi_system_reset_get_device(void)
{
	return reset_dev;
}

void sbi_system_reset_set_device(const struct sbi_system_reset_device *dev)
{
	if (!dev || reset_dev)
		return;

	reset_dev = dev;
}

bool sbi_system_reset_supported(u32 reset_type, u32 reset_reason)
{
	if (reset_dev && reset_dev->system_reset_check &&
	    reset_dev->system_reset_check(reset_type, reset_reason))
		return TRUE;

	return FALSE;
}

#include <sbi/riscv_io.h>
void __noreturn sbi_system_reset(u32 reset_type, u32 reset_reason)
{
	ulong hbase = 0, hmask;
	u32 cur_hartid = current_hartid();
	struct sbi_domain *dom = sbi_domain_thishart_ptr();
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();

	/* Send HALT IPI to every hart other than the current hart */
	while (!sbi_hsm_hart_interruptible_mask(dom, hbase, &hmask)) {
		if (hbase <= cur_hartid)
			hmask &= ~(1UL << (cur_hartid - hbase));
		if (hmask)
			sbi_ipi_send_halt(hmask, hbase);
		hbase += BITS_PER_LONG;
	}

	/* Stop current HART */
	sbi_hsm_hart_stop(scratch, FALSE);

	/* Platform specific reset if domain allowed system reset */
	if (dom->system_reset_allowed &&
	    reset_dev && reset_dev->system_reset)
		reset_dev->system_reset(reset_type, reset_reason);

	writew(0x5555, (void *)0x100000); // qemu poweroff

	/* If platform specific reset did not work then do sbi_exit() */
	sbi_exit(scratch);
}

static const struct sbi_system_suspend_device *suspend_dev;

const struct sbi_system_suspend_device *sbi_system_suspend_get_device(void)
{
	return suspend_dev;
}

void sbi_system_suspend_set_device(struct sbi_system_suspend_device *dev)
{
	if (!dev || suspend_dev)
		return;

	suspend_dev = dev;
}

bool sbi_system_suspend_supported(u32 sleep_type)
{
	return suspend_dev && suspend_dev->system_suspend_check &&
	       suspend_dev->system_suspend_check(sleep_type) == 0;
}

int sbi_system_suspend(u32 sleep_type, ulong resume_addr, ulong opaque)
{
	const struct sbi_domain *dom = sbi_domain_thishart_ptr();
	struct sbi_scratch *scratch  = sbi_scratch_thishart_ptr();
	void (*jump_warmboot)(void)  = (void (*)(void))scratch->warmboot_addr;
	//unsigned int hartid = current_hartid();
	unsigned long prev_mode;
	//unsigned long i, j;
	int ret;

	if (!dom || !dom->system_suspend_allowed)
		return SBI_EFAIL;

	if (!suspend_dev || !suspend_dev->system_suspend ||
			!suspend_dev->system_suspend_check)
		return SBI_EFAIL;

	ret = suspend_dev->system_suspend_check(sleep_type);
	if (ret != SBI_OK)
		return ret;

	prev_mode = (csr_read(CSR_MSTATUS) & MSTATUS_MPP) >> MSTATUS_MPP_SHIFT;
	if (prev_mode != PRV_S && prev_mode != PRV_U)
		return SBI_EFAIL;

	//FIX ME: hartindex to hart
	//sbi_hartmask_for_each_hartindex(j, &dom->assigned_harts) {
	//        //i = sbi_hartindex_to_hartid(j);
	//        if (i == hartid)
	//                continue;
	//        if (__sbi_hsm_hart_get_state(i) != SBI_HSM_STATE_STOPPED)
	//                return SBI_ERR_DENIED;
	//}

	if (!sbi_domain_check_addr(dom, resume_addr, prev_mode,
				   SBI_DOMAIN_EXECUTE))
		return SBI_EINVALID_ADDR;

	if (!sbi_hsm_hart_change_state(scratch, SBI_HSM_STATE_STARTED,
				       SBI_HSM_STATE_SUSPENDED))
		return SBI_EFAIL;

	/* Prepare for resume */
	scratch->next_mode = prev_mode;
	scratch->next_addr = resume_addr;
	scratch->next_arg1 = opaque;

	__sbi_hsm_suspend_non_ret_save(scratch);

	/* Suspend */
	ret = suspend_dev->system_suspend(sleep_type, scratch->warmboot_addr);
	if (ret != SBI_OK) {
		if (!sbi_hsm_hart_change_state(scratch, SBI_HSM_STATE_SUSPENDED,
					       SBI_HSM_STATE_STARTED))
			sbi_hart_hang();
		return ret;
	}

	/* Resume */
	jump_warmboot();

	__builtin_unreachable();
}
