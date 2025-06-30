#include <common.h>
#include <nand.h>
#include <linux/mtd/rawnand.h>
#include "cvsnfc_common.h"
#include "cvsnfc.h"

static struct mtd_info *mtd;
static struct nand_chip nand_chip;

void nand_init(void)
{
	mtd = nand_to_mtd(&nand_chip);
	nand_chip.IO_ADDR_R = nand_chip.IO_ADDR_W = (void  __iomem *)CONFIG_SYS_NAND_BASE;
	board_nand_init(&nand_chip);

	if (nand_scan(mtd, 1)) {
		return;
	}
}

int nand_spl_load_image(uint32_t offs, unsigned int size, void *dst)
{
	struct cvsnfc_chip_info *chip_info = ((struct cvsnfc_host *)nand_chip.priv)->nand_chip_info;
	unsigned int block_size = chip_info->erasesize;
	unsigned int page_size = chip_info->pagesize;
	unsigned int page_count = block_size / page_size;
	unsigned int block = offs / block_size;
	unsigned int lastblock = (offs + size - 1) / block_size;
	unsigned int page = offs / page_size;
	unsigned int page_offset = offs % page_size;

	debug("read block from %d to lastblock %d\n", block, lastblock);

	while (block <= lastblock) {
		if (!nand_block_isbad(mtd, block * block_size)) {
			while (page < (block + 1) * page_count) {
				cvsnfc_read_page_raw(mtd, &nand_chip, dst, 0, page);

				if (unlikely(page_offset)) {
					memmove(dst, dst + page_offset, page_size);
					dst = (void *)((uintptr_t)dst - page_offset);
					page_offset = 0;
				}

				dst += page_size;
				++page;
			}
		} else {
			page += page_count;
			++lastblock;

			debug("skip bad block %d, lastblock is %d\n", block, lastblock);
		}

		++block;
	}

	return 0;
}

u32 nand_spl_adjust_offset(u32 sector, u32 offs)
{
	struct cvsnfc_chip_info *chip_info = ((struct cvsnfc_host *)nand_chip.priv)->nand_chip_info;
	unsigned int block_size = chip_info->erasesize;
	unsigned int block = sector / block_size;
	unsigned int lastblock = (sector + offs) / block_size;

	while (block <= lastblock) {
		if (nand_block_isbad(mtd, block * block_size)) {
			offs += block_size;
			++lastblock;
		}

		++block;
	}

	return offs;
}

void nand_deselect(void)
{
	if (nand_chip.select_chip) {
		nand_chip.select_chip(mtd, -1);
	}
}
