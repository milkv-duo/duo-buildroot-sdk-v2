// SPDX-License-Identifier: GPL-2.0-only

#include <linux/clk.h>
#include <linux/syscalls.h>
#include <asm/sbi.h>

SYSCALL_DEFINE2(reset_c906l, int, reset, const unsigned long, address)
{
	int ret = -1;
	struct clk *efuse_clk = NULL;

	efuse_clk = clk_get_sys(NULL, "clk_efuse");
	if (IS_ERR(efuse_clk)) {
		pr_err("%s: efuse clock not found %ld\n", __func__
			, PTR_ERR(efuse_clk));
		return -1;
	}

	clk_prepare_enable(efuse_clk);
	switch (reset) {
	case 0: //rst
		ret = sbi_rst_c906l();
		break;
	case 1: //unrst
		ret = sbi_unrst_c906l(address);
		break;
	case 2: //rst + unrst
		ret = sbi_reset_c906l(address);
		break;
	default:
		break;
	};

	clk_disable_unprepare(efuse_clk);
	return ret;
}

