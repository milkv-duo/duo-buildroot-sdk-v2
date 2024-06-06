/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 *   Atish Patra <atish.patra@wdc.com>
 */

#include <sbi/sbi_ecall.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_trap.h>
#include <sbi/sbi_version.h>
#include <sbi/riscv_asm.h>
#include <sbi/riscv_io.h>

static int sbi_ecall_base_probe(unsigned long extid, unsigned long *out_val)
{
	struct sbi_ecall_extension *ext;

	ext = sbi_ecall_find_extension(extid);
	if (!ext) {
		*out_val = 0;
		return 0;
	}

	if (ext->probe)
		return ext->probe(extid, out_val);

	*out_val = 1;
	return 0;
}

#define SEC_SYS_BASE (0x020B0000)
static int sbi_ecall_base_rst_c906l(void)
{
	unsigned int value;

	value = readl((void *)0x3003024);
	writel((value & (~(1 << 6))), (void *)0x3003024);

	return 0;
}

static int sbi_ecall_base_unrst_c906l(const unsigned long address)
{
	unsigned int value;

	value = readl((void *)SEC_SYS_BASE + 0x04);
	writel((value | (1 << 13)), (void *)SEC_SYS_BASE + 0x04);
	writel(address, (void *)SEC_SYS_BASE + 0x20);
	writel((address >> 32), (void *)SEC_SYS_BASE + 0x24);

	value = readl((void *)0x3003024);
	writel((value | (1 << 6)), (void *)0x3003024);

	return 0;
}

static int sbi_ecall_base_reset_c906l(const unsigned long address)
{
	sbi_ecall_base_rst_c906l();

	sbi_ecall_base_unrst_c906l(address);

	return 0;
}

static int sbi_ecall_base_handler(unsigned long extid, unsigned long funcid,
				  const struct sbi_trap_regs *regs,
				  unsigned long *out_val,
				  struct sbi_trap_info *out_trap)
{
	int ret = 0;

	switch (funcid) {
	case SBI_EXT_BASE_GET_SPEC_VERSION:
		*out_val = (SBI_ECALL_VERSION_MAJOR <<
			   SBI_SPEC_VERSION_MAJOR_OFFSET) &
			   (SBI_SPEC_VERSION_MAJOR_MASK <<
			    SBI_SPEC_VERSION_MAJOR_OFFSET);
		*out_val = *out_val | SBI_ECALL_VERSION_MINOR;
		break;
	case SBI_EXT_BASE_GET_IMP_ID:
		*out_val = sbi_ecall_get_impid();
		break;
	case SBI_EXT_BASE_GET_IMP_VERSION:
		*out_val = OPENSBI_VERSION;
		break;
	case SBI_EXT_BASE_GET_MVENDORID:
		*out_val = csr_read(CSR_MVENDORID);
		break;
	case SBI_EXT_BASE_GET_MARCHID:
		*out_val = csr_read(CSR_MARCHID);
		break;
	case SBI_EXT_BASE_GET_MIMPID:
		*out_val = csr_read(CSR_MIMPID);
		break;
	case SBI_EXT_BASE_PROBE_EXT:
		ret = sbi_ecall_base_probe(regs->a0, out_val);
		break;
	case SBI_EXT_BASE_RESET_C906L:
		ret = sbi_ecall_base_reset_c906l(regs->a0);
		*out_val = regs->a0;
		break;
	case SBI_EXT_BASE_RST_C906L:
		ret = sbi_ecall_base_rst_c906l();
		break;
	case SBI_EXT_BASE_UNRST_C906L:
		ret = sbi_ecall_base_unrst_c906l(regs->a0);
		break;
	default:
		ret = SBI_ENOTSUPP;
	}

	return ret;
}

struct sbi_ecall_extension ecall_base = {
	.extid_start = SBI_EXT_BASE,
	.extid_end = SBI_EXT_BASE,
	.handle = sbi_ecall_base_handler,
};
