// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright 2022-2023 CVITEK
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <linux/reset.h>
#include <linux/version.h>
#include <linux/jiffies.h>
#include <linux/init.h>
#include <asm/cacheflush.h>
#include <linux/dma-buf.h>
#include <linux/dma-map-ops.h>
#include <linux/uaccess.h>
#include <linux/errno.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/cvitek_spacc.h>
#include <cvitek-spacc-regs.h>

#define DEVICE_NAME    "spacc"

static char flag = 'n';
static DECLARE_WAIT_QUEUE_HEAD(wq);

struct cvi_spacc {
	struct device *dev;
	struct cdev cdev;
	dev_t tdev;
	void __iomem *spacc_base;
	struct class *spacc_class;
	void *buffer;
	u32 buffer_size;
	u32 used_size;

	// for sha256/sha1
	u32 state[8];
	u32 result_size;

#ifdef CONFIG_PM_SLEEP
	struct clk *efuse_clk;
#endif
};

#ifdef CONFIG_PM_SLEEP
static int cvitek_spacc_suspend(struct device *dev)
{
	struct cvi_spacc *spacc = dev_get_drvdata(dev);
	void __iomem *sec_top;

	clk_prepare_enable(spacc->efuse_clk);

	sec_top = ioremap(0x020b0000, 4);
	iowrite32(0x3, sec_top);
	iounmap(sec_top);

	clk_disable_unprepare(spacc->efuse_clk);
	return 0;
}

static int cvitek_spacc_resume(struct device *dev)
{
	struct cvi_spacc *spacc = dev_get_drvdata(dev);
	void __iomem *sec_top;

	clk_prepare_enable(spacc->efuse_clk);

	sec_top = ioremap(0x020b0000, 4);
	iowrite32(0x0, sec_top);
	iounmap(sec_top);

	clk_disable_unprepare(spacc->efuse_clk);
	return 0;
}
#endif /* CONFIG_PM_SLEEP */

static SIMPLE_DEV_PM_OPS(cvitek_spacc_pm_ops, cvitek_spacc_suspend, cvitek_spacc_resume);

static inline void cvi_sha256_init(struct cvi_spacc *spacc)
{
	spacc->state[0] = cpu_to_be32(0x6A09E667);
	spacc->state[1] = cpu_to_be32(0xBB67AE85);
	spacc->state[2] = cpu_to_be32(0x3C6EF372);
	spacc->state[3] = cpu_to_be32(0xA54FF53A);
	spacc->state[4] = cpu_to_be32(0x510E527F);
	spacc->state[5] = cpu_to_be32(0x9B05688C);
	spacc->state[6] = cpu_to_be32(0x1F83D9AB);
	spacc->state[7] = cpu_to_be32(0x5BE0CD19);
}

static inline void cvi_sha1_init(struct cvi_spacc *spacc)
{
	spacc->state[0] = cpu_to_be32(0x67452301);
	spacc->state[1] = cpu_to_be32(0xEFCDAB89);
	spacc->state[2] = cpu_to_be32(0x98BADCFE);
	spacc->state[3] = cpu_to_be32(0x10325476);
	spacc->state[4] = cpu_to_be32(0xC3D2E1F0);
}

static inline void trigger_cryptodma_engine_and_wait_finish(struct cvi_spacc *spacc)
{
	// Set cryptodma control
	iowrite32(0x3, spacc->spacc_base + CRYPTODMA_INT_MASK);

	// Clear interrupt
	// Important!!! must do this
	iowrite32(0x3, spacc->spacc_base + CRYPTODMA_WR_INT);

	// Trigger cryptodma engine
	iowrite32(DMA_WRITE_MAX_BURST << 24 |
			  DMA_READ_MAX_BURST << 16 |
			  DMA_DESCRIPTOR_MODE << 1 | DMA_ENABLE, spacc->spacc_base + CRYPTODMA_DMA_CTRL);

	wait_event_interruptible(wq, flag == 'y');
	flag = 'n';
}

static inline void get_hash_result(struct cvi_spacc *spacc, int count)
{
	u32 i;
	u32 *result = (u32 *)spacc->buffer;

	for (i = 0; i < count; i++)
		result[i] = ioread32(spacc->spacc_base + CRYPTODMA_SHA_PARA + i * 4);
}

static inline void setup_dma_descriptor(struct cvi_spacc *spacc, uint32_t *dma_descriptor)
{
	phys_addr_t descriptor_phys;

	descriptor_phys = virt_to_phys(dma_descriptor);

	arch_sync_dma_for_device(descriptor_phys, /*sizeof(dma_descriptor)*/ 4 * 22, DMA_TO_DEVICE);

	// set dma descriptor addr
	iowrite32((uint32_t)((uint64_t)descriptor_phys & 0xFFFFFFFF), spacc->spacc_base + CRYPTODMA_DES_BASE_L);
	iowrite32((uint32_t)((uint64_t)descriptor_phys >> 32), spacc->spacc_base + CRYPTODMA_DES_BASE_H);
}

static inline void setup_src(u32 *dma_descriptor, uintptr_t src, u32 len)
{
	phys_addr_t src_phys;

	src_phys = virt_to_phys((void *)src);
	arch_sync_dma_for_device(src_phys, len, DMA_TO_DEVICE);

	dma_descriptor[CRYPTODMA_SRC_LEN] = len;
	dma_descriptor[CRYPTODMA_SRC_ADDR_L] = (uint32_t)((uint64_t)src_phys & 0xFFFFFFFF);
	dma_descriptor[CRYPTODMA_SRC_ADDR_H] = (uint32_t)((uint64_t)src_phys >> 32);
}

static void setup_src_dst(u32 *dma_descriptor, phys_addr_t buffer, u32 len)
{
	dma_descriptor[CRYPTODMA_SRC_LEN] = len;
	dma_descriptor[CRYPTODMA_SRC_ADDR_L] = (uint32_t)((uint64_t)buffer & 0xFFFFFFFF);
	dma_descriptor[CRYPTODMA_SRC_ADDR_H] = (uint32_t)((uint64_t)buffer >> 32);

	dma_descriptor[CRYPTODMA_DST_ADDR_L] = (uint32_t)((uint64_t)buffer & 0xFFFFFFFF);
	dma_descriptor[CRYPTODMA_DST_ADDR_H] = (uint32_t)((uint64_t)buffer >> 32);
}

#define setup_dst(dst)\
do {\
	phys_addr_t dst_phys = virt_to_phys((void *)dst);\
	dma_descriptor[CRYPTODMA_DST_ADDR_L] = (uint32_t)((uint64_t)dst_phys & 0xFFFFFFFF);\
	dma_descriptor[CRYPTODMA_DST_ADDR_H] = (uint32_t)((uint64_t)dst_phys >> 32);\
} while (0)

static inline void setup_mode(u32 *dma_descriptor, SPACC_ALGO_MODE_E mode, unsigned char *iv)
{
	switch (mode) {
	case SPACC_ALGO_MODE_CBC:
		dma_descriptor[CRYPTODMA_CTRL] |= DES_USE_DESCRIPTOR_IV;
		dma_descriptor[CRYPTODMA_CIPHER] = CBC_ENABLE << 1;
		memcpy(&dma_descriptor[CRYPTODMA_IV], iv, 16);
		break;
	case SPACC_ALGO_MODE_CTR:
		dma_descriptor[CRYPTODMA_CTRL] |= DES_USE_DESCRIPTOR_IV;
		dma_descriptor[CRYPTODMA_CIPHER] = 0x1 << 2;
		memcpy(&dma_descriptor[CRYPTODMA_IV], iv, 16);
		break;
	case SPACC_ALGO_MODE_ECB:
	default:
		break;
	}
}

static inline void setup_key_size(u32 *dma_descriptor, SPACC_KEY_SIZE_E size, unsigned char *key)
{
	switch (size) {
	case SPACC_KEY_SIZE_64BITS:
		memcpy(&dma_descriptor[CRYPTODMA_KEY], key, 8);
		break;
	case SPACC_KEY_SIZE_128BITS:
		dma_descriptor[CRYPTODMA_CIPHER] |= (0x1 << 5);
		memcpy(&dma_descriptor[CRYPTODMA_KEY], key, 16);
		break;
	case SPACC_KEY_SIZE_192BITS:
		dma_descriptor[CRYPTODMA_CIPHER] |= (0x1 << 4);
		memcpy(&dma_descriptor[CRYPTODMA_KEY], key, 24);
		break;
	case SPACC_KEY_SIZE_256BITS:
		dma_descriptor[CRYPTODMA_CIPHER] |= (0x1 << 3);
		memcpy(&dma_descriptor[CRYPTODMA_KEY], key, 32);
		break;
	default:
		break;
	}
}

static inline void setup_action(u32 *dma_descriptor, SPACC_ACTION_E action)
{
	if (action == SPACC_ACTION_ENCRYPTION)
		dma_descriptor[CRYPTODMA_CIPHER] |= 0x1;
}

static irqreturn_t cvitek_spacc_irq(int irq, void *data)
{
	struct cvi_spacc *spacc = (struct cvi_spacc *)data;

	iowrite32(0x3, spacc->spacc_base + CRYPTODMA_WR_INT);

	flag = 'y';
	wake_up_interruptible(&wq);

	return IRQ_HANDLED;
}

int spacc_sha256(struct cvi_spacc *spacc, uintptr_t src, uint32_t len)
{
	__aligned(32) u32 dma_descriptor[22] = {0};
	u32 i;

	// must mark DES_USE_DESCRIPTOR_KEY flag
	dma_descriptor[CRYPTODMA_CTRL] = DES_USE_DESCRIPTOR_KEY | DES_USE_SHA | 0xF;
	dma_descriptor[CRYPTODMA_CIPHER] = (0x1 << 1) | 0x1;

	for (i = 0; i < 8; i++)
		dma_descriptor[CRYPTODMA_KEY + i] = spacc->state[i];

	setup_src(dma_descriptor, src, len);
	setup_dma_descriptor(spacc, dma_descriptor);

	trigger_cryptodma_engine_and_wait_finish(spacc);
	return 0;
}

int spacc_sha1(struct cvi_spacc *spacc, uintptr_t src, uint32_t len)
{
	__aligned(32) u32 dma_descriptor[22] = {0};
	u32 i;

	// must mark DES_USE_DESCRIPTOR_KEY flag
	dma_descriptor[CRYPTODMA_CTRL] = DES_USE_DESCRIPTOR_KEY | DES_USE_SHA | 0xF;
	dma_descriptor[CRYPTODMA_CIPHER] = 0x1;

	for (i = 0; i < 5; i++)
		dma_descriptor[CRYPTODMA_KEY + i] = spacc->state[i];

	setup_src(dma_descriptor, src, len);
	setup_dma_descriptor(spacc, dma_descriptor);

	trigger_cryptodma_engine_and_wait_finish(spacc);
	return 0;
}

int spacc_base64(struct cvi_spacc *spacc, phys_addr_t src, uint32_t len, SPACC_ACTION_E ation)
{
	__aligned(32) u32 dma_descriptor[22] = {0};

	dma_descriptor[CRYPTODMA_CTRL] = DES_USE_BASE64 | 0xF;

	if (ation == SPACC_ACTION_ENCRYPTION) {
		dma_descriptor[CRYPTODMA_CIPHER] = 0x1;
		spacc->result_size = (len + (3 - 1)) / 3 * 4;
		dma_descriptor[CRYPTODMA_DST_LEN] = spacc->result_size;
	} else {
		spacc->result_size = (len / 4) * 3;
		dma_descriptor[CRYPTODMA_DST_LEN] = spacc->result_size;
	}

	setup_src_dst(dma_descriptor, src, len);
	setup_dma_descriptor(spacc, dma_descriptor);

	trigger_cryptodma_engine_and_wait_finish(spacc);
	return 0;
}

int spacc_aes(struct cvi_spacc *spacc, phys_addr_t src, uint32_t len, spacc_aes_config_s config)
{
	__aligned(32) u32 dma_descriptor[22] = {0};

	spacc->result_size = len;
	dma_descriptor[CRYPTODMA_CTRL] = DES_USE_DESCRIPTOR_KEY | DES_USE_AES | 0xF;

	setup_mode(dma_descriptor, config.mode, config.iv);
	setup_key_size(dma_descriptor, config.size, config.key);
	setup_action(dma_descriptor, config.action);

	setup_src_dst(dma_descriptor, src, len);
	setup_dma_descriptor(spacc, dma_descriptor);

	trigger_cryptodma_engine_and_wait_finish(spacc);
	return 0;
}

int spacc_sm4(struct cvi_spacc *spacc, phys_addr_t src, uint32_t len, spacc_sm4_config_s config)
{
	__aligned(32) u32 dma_descriptor[22] = {0};

	spacc->result_size = len;
	dma_descriptor[CRYPTODMA_CTRL] = DES_USE_DESCRIPTOR_KEY | DES_USE_SM4 | 0xF;

	setup_mode(dma_descriptor, config.mode, config.iv);
	setup_key_size(dma_descriptor, config.size, config.key);
	setup_action(dma_descriptor, config.action);

	setup_src_dst(dma_descriptor, src, len);
	setup_dma_descriptor(spacc, dma_descriptor);

	trigger_cryptodma_engine_and_wait_finish(spacc);
	return 0;
}

int spacc_des(struct cvi_spacc *spacc, phys_addr_t src, uint32_t len, spacc_des_config_s config, int is_tdes)
{
	__aligned(32) u32 dma_descriptor[22] = {0};

	spacc->result_size = len;
	dma_descriptor[CRYPTODMA_CTRL] = DES_USE_DESCRIPTOR_KEY | DES_USE_DES | 0xF;

	setup_mode(dma_descriptor, config.mode, config.iv);
	if (is_tdes) {
		dma_descriptor[CRYPTODMA_CIPHER] |= (0x1 << 3);
		memcpy(&dma_descriptor[CRYPTODMA_KEY], config.key, 24);
	} else {
		memcpy(&dma_descriptor[CRYPTODMA_KEY], config.key, 8);
	}
	setup_action(dma_descriptor, config.action);

	setup_src_dst(dma_descriptor, src, len);
	setup_dma_descriptor(spacc, dma_descriptor);

	trigger_cryptodma_engine_and_wait_finish(spacc);
	return 0;
}

static int cvi_spacc_init_buffer(struct cvi_spacc *spacc, size_t size)
{
	unsigned int order = get_order(size);
	struct page *page;

	if (size == spacc->buffer_size) {
		return 0;
	} else if (spacc->buffer_size) {
		free_pages((unsigned long)spacc->buffer, get_order(spacc->buffer_size));
		spacc->buffer_size = 0;
	}

	page = alloc_pages(GFP_KERNEL | __GFP_ZERO, order);
	if (!page)
		return -ENOMEM;

	spacc->buffer = page_address(page);
	spacc->buffer_size = size;
	return 0;
}

static int spacc_open(struct inode *inode, struct file *file)
{
	struct cvi_spacc *spacc;

	spacc = container_of(inode->i_cdev, struct cvi_spacc, cdev);

	spacc->used_size = 0;
	spacc->result_size = 0;
	file->private_data = spacc;
	return 0;
}

static ssize_t spacc_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	struct cvi_spacc *spacc = filp->private_data;
	int ret = 0;

	if (spacc->result_size == 0) {
		pr_err("spacc result is 0\n");
		return -1;
	}

	if (count < spacc->result_size)
		return -1;

	ret = copy_to_user(buf, spacc->buffer, spacc->result_size);
	if (ret != 0)
		return -1;

	return spacc->result_size;
}

static ssize_t spacc_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	struct cvi_spacc *spacc = filp->private_data;
	int ret = spacc->buffer_size - spacc->used_size;

	if (ret <= 0)
		return spacc->used_size;

	if (count > ret)
		count = ret;

	ret = copy_from_user(((unsigned char *)spacc->buffer + spacc->used_size), buf, count);
	if (ret != 0)
		return -1;

	spacc->used_size += count;
	return spacc->used_size;
}

static int spacc_release(struct inode *inode, struct file *file)
{
	return 0;
}

static long spacc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct cvi_spacc *spacc = filp->private_data;
	int ret;

	switch (cmd) {
	case IOCTL_SPACC_CREATE_MEMPOOL: {
		unsigned int size = 0;

		ret = copy_from_user((unsigned char *)&size, (unsigned char *)arg, sizeof(size));
		if (ret != 0)
			return -1;

		ret = cvi_spacc_init_buffer(spacc, size);
		if (ret != 0)
			return -1;

		break;
	}
	case IOCTL_SPACC_GET_MEMPOOL_SIZE: {
		ret = copy_to_user((unsigned char *)arg, (unsigned char *)&spacc->buffer_size,
				   sizeof(spacc->buffer_size));
		if (ret != 0)
			return -1;

		break;
	}
	case IOCTL_SPACC_SHA256_ACTION: {
		if (spacc->used_size & 0x3F) {
			pr_err("used_size : %d\n", spacc->used_size);
			return -1;
		}

		cvi_sha256_init(spacc);

		ret = spacc_sha256(spacc, (uintptr_t)spacc->buffer, spacc->used_size);
		if (ret < 0) {
			pr_err("plat_cryptodma_do failed\n");
			return -1;
		}

		get_hash_result(spacc, 8);
		spacc->result_size = 32;
		spacc->used_size = 0;
		break;
	}
	case IOCTL_SPACC_SHA1_ACTION: {
		if (spacc->used_size & 0x3F) {
			pr_err("spacc_dev->used_size : %d\n", spacc->used_size);
			return -1;
		}

		cvi_sha1_init(spacc);

		ret = spacc_sha1(spacc, (uintptr_t)spacc->buffer, spacc->used_size);
		if (ret < 0) {
			pr_err("plat_cryptodma_do failed\n");
			return -1;
		}

		get_hash_result(spacc, 5);
		spacc->result_size = 20;
		spacc->used_size = 0;
		break;
	}
	case IOCTL_SPACC_BASE64_ACTION: {
		spacc_base64_action_s action = {0};
		phys_addr_t src_phys;

		ret = copy_from_user((unsigned char *)&action, (unsigned char *)arg, sizeof(action));
		if (ret != 0)
			return -1;

		src_phys = virt_to_phys(spacc->buffer);
		arch_sync_dma_for_device(src_phys, spacc->used_size, DMA_TO_DEVICE);
		ret = spacc_base64(spacc, src_phys, spacc->used_size, action.action);
		if (ret < 0) {
			pr_err("plat_cryptodma_do failed\n");
			return -1;
		}

		arch_sync_dma_for_device(src_phys, spacc->result_size, DMA_FROM_DEVICE);
		spacc->used_size = 0;
		break;
	}
	case IOCTL_SPACC_AES_ACTION: {
		spacc_aes_config_s config = {0};
		phys_addr_t src_phys;
		uint32_t len;

		ret = copy_from_user((unsigned char *)&config, (unsigned char *)arg, sizeof(config));
		if (ret != 0)
			return -1;

		if (config.src) {
			if ((config.len == 0) ||
				(config.len & 0xF)) {
				pr_err("src len [%d] invailed\n", config.len);
				return -1;
			}

			src_phys = (phys_addr_t)config.src;
			len = config.len;
		} else {
			if ((spacc->used_size == 0) ||
				(spacc->used_size & 0xF)) {
				pr_err("used_size : %d\n", spacc->used_size);
				return -1;
			}

			src_phys = virt_to_phys(spacc->buffer);
			len = spacc->used_size;
		}

		arch_sync_dma_for_device(src_phys, len, DMA_TO_DEVICE);
		ret = spacc_aes(spacc, src_phys, len, config);
		if (ret < 0) {
			pr_err("plat_cryptodma_do failed\n");
			return -1;
		}

		arch_sync_dma_for_device(src_phys, spacc->result_size, DMA_FROM_DEVICE);
		spacc->used_size = 0;
		break;
	}
	case IOCTL_SPACC_SM4_ACTION: {
		spacc_sm4_config_s action = {0};
		phys_addr_t src_phys;

		if (spacc->used_size & 0xF) {
			pr_err("used_size : %d\n", spacc->used_size);
			return -1;
		}

		ret = copy_from_user((unsigned char *)&action, (unsigned char *)arg, sizeof(action));
		if (ret != 0)
			return -1;

		src_phys = virt_to_phys(spacc->buffer);
		arch_sync_dma_for_device(src_phys, spacc->used_size, DMA_TO_DEVICE);
		ret = spacc_sm4(spacc, src_phys, spacc->used_size, action);
		if (ret < 0) {
			pr_err("plat_cryptodma_do failed\n");
			return -1;
		}

		arch_sync_dma_for_device(src_phys, spacc->result_size, DMA_FROM_DEVICE);
		spacc->used_size = 0;
		break;
	}
	case IOCTL_SPACC_DES_ACTION: {
		spacc_des_config_s action = {0};
		phys_addr_t src_phys;

		if (spacc->used_size & 0x7) {
			pr_err("spacc_dev->used_size : %d\n", spacc->used_size);
			return -1;
		}

		ret = copy_from_user((unsigned char *)&action, (unsigned char *)arg, sizeof(action));
		if (ret != 0)
			return -1;

		src_phys = virt_to_phys(spacc->buffer);
		arch_sync_dma_for_device(src_phys, spacc->used_size, DMA_TO_DEVICE);
		ret = spacc_des(spacc, src_phys, spacc->used_size, action, 0);
		if (ret < 0) {
			pr_err("plat_cryptodma_do failed\n");
			return -1;
		}

		arch_sync_dma_for_device(src_phys, spacc->result_size, DMA_FROM_DEVICE);
		spacc->used_size = 0;
		break;
	}
	case IOCTL_SPACC_TDES_ACTION: {
		spacc_tdes_config_s action = {0};
		phys_addr_t src_phys;

		if (spacc->used_size & 0x7) {
			pr_err("spacc_dev->used_size : %d\n", spacc->used_size);
			return -EINVAL;
		}

		ret = copy_from_user((unsigned char *)&action, (unsigned char *)arg, sizeof(action));
		if (ret != 0)
			return -1;

		src_phys = virt_to_phys(spacc->buffer);
		arch_sync_dma_for_device(src_phys, spacc->used_size, DMA_TO_DEVICE);
		ret = spacc_des(spacc, src_phys, spacc->used_size, action, 1);
		if (ret < 0) {
			pr_err("plat_cryptodma_do failed\n");
			return -1;
		}

		arch_sync_dma_for_device(src_phys, spacc->result_size, DMA_FROM_DEVICE);
		spacc->used_size = 0;
		break;
	}
	default:
		return -EINVAL;
	}

	return 0;
}

const struct file_operations spacc_fops = {
	.owner  =   THIS_MODULE,
	.open   =   spacc_open,
	.read   =   spacc_read,
	.write  =   spacc_write,
	.release =  spacc_release,
	.unlocked_ioctl = spacc_ioctl,
};

static int cvitek_spacc_drv_probe(struct platform_device *pdev)
{
	struct cvi_spacc *spacc;
	struct device *dev = &pdev->dev;
	int ret = 0;

	spacc = devm_kzalloc(dev, sizeof(*spacc), GFP_KERNEL);
	if (!spacc)
		return -ENOMEM;

	spacc->dev = dev;
	spacc->spacc_base = devm_platform_ioremap_resource(pdev, 0);
	if (!spacc->spacc_base)
		return -ENOMEM;

	ret = platform_get_irq(pdev, 0);
	if (ret <= 0) {
		pr_err("get irq num failed\n");
		return -1;
	}

#ifdef CONFIG_PM_SLEEP
	spacc->efuse_clk = clk_get_sys(NULL, "clk_efuse");
	if (IS_ERR(spacc->efuse_clk)) {
		pr_err("%s: efuse clock not found %ld\n", __func__
				, PTR_ERR(spacc->efuse_clk));
		return -1;
	}
#endif

	ret = devm_request_irq(dev, ret, cvitek_spacc_irq
			, IRQF_SHARED | IRQF_TRIGGER_RISING, pdev->name, spacc);
	if (ret) {
		pr_err("request irq failed\n");
		return -1;
	}

	ret = alloc_chrdev_region(&spacc->tdev, 0, 1, DEVICE_NAME);
	if (ret)
		return ret;

	cdev_init(&spacc->cdev, &spacc_fops);
	spacc->cdev.owner = THIS_MODULE;

	ret = cdev_add(&spacc->cdev, spacc->tdev, 1);
	if (ret)
		goto failed;

	spacc->spacc_class = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(spacc->spacc_class)) {
		pr_err("Err: failed when create class.\n");
		goto failed;
	}

	device_create(spacc->spacc_class, NULL, spacc->tdev, spacc, DEVICE_NAME);
	return ret;
failed:
	unregister_chrdev_region(spacc->tdev, 1);
	return -ENOMEM;
}

static int cvitek_spacc_drv_remove(struct platform_device *pdev)
{
	struct cvi_spacc *spacc = platform_get_drvdata(pdev);

	device_destroy(spacc->spacc_class, spacc->tdev);
	cdev_del(&spacc->cdev);
	unregister_chrdev_region(spacc->tdev, 1);
	class_destroy(spacc->spacc_class);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id cvitek_spacc_of_match[] = {
	{ .compatible = "cvitek,spacc", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, cvitek_spacc_of_match);
#endif

static struct platform_driver cvitek_spacc_driver = {
	.probe		= cvitek_spacc_drv_probe,
	.remove		= cvitek_spacc_drv_remove,
	.driver		= {
		.name	= "cvitek_spacc",
		.of_match_table = of_match_ptr(cvitek_spacc_of_match),
		.pm     = &cvitek_spacc_pm_ops,
	},
};

module_platform_driver(cvitek_spacc_driver);

MODULE_DESCRIPTION("Cvitek Spacc Driver");
MODULE_LICENSE("GPL");
