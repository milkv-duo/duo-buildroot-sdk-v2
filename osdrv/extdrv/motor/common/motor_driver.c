// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) Cvitek Co., Ltd. 2022-2023. All rights reserved.
 *
 * File Name: motor_driver.c
 * Description: motor kernel space driver entry related code

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/sysfs.h>
#include <linux/version.h>
#include <linux/compat.h>
#include <linux/iommu.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/gpio.h>
#include <linux/of_reserved_mem.h>
#include <linux/of_gpio.h>

#include "motor_ioctl.h"
#include "motor_driver.h"
#include "ms41929_driver.h"

#define CVI_MOTOR_CDEV_NAME "cvi-motor"
#define CVI_MOTOR_CLASS_NAME "cvi-motor"

struct cvi_motor_device ndev;
struct spi_device *ptmp;

static int fp_motor_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &ndev;
	return 0;
}

static int fp_motor_close(struct inode *inode, struct file *filp)
{
	filp->private_data = NULL;
	return 0;
}

static long fp_motor_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct cvi_motor_device *ndev = filp->private_data;
	struct cvi_motor_regval reg;
	struct cvi_lens_info info;
	long ret;

	if (!ndev)
		return -EBADF;

	switch (cmd) {
	case CVI_MOTOR_IOC_READ: {
		if (copy_from_user(&reg, (void __user *)arg, sizeof(reg)) == 0) {
			reg.val = read(reg.addr);
			ret = copy_to_user((void __user *)arg, &reg, sizeof(reg));
		}
	} break;
	case CVI_MOTOR_IOC_WRITE: {
		if (copy_from_user(&reg, (void __user *)arg, sizeof(reg)) == 0)
			ret = write(reg.addr, reg.val);
	} break;
	case CVI_MOTOR_IOC_ZOOM_IN: {
		if (copy_from_user(&reg, (void __user *)arg, sizeof(reg)) == 0)
			ret = zoom_in(reg.val);
	} break;
	case CVI_MOTOR_IOC_ZOOM_OUT: {
		if (copy_from_user(&reg, (void __user *)arg, sizeof(reg)) == 0)
			ret = zoom_out(reg.val);
	} break;
	case CVI_MOTOR_IOC_FOCUS_IN: {
		if (copy_from_user(&reg, (void __user *)arg, sizeof(reg)) == 0)
			ret = focus_in(reg.val);
	} break;
	case CVI_MOTOR_IOC_FOCUS_OUT: {
		if (copy_from_user(&reg, (void __user *)arg, sizeof(reg)) == 0)
			ret = focus_out(reg.val);
	} break;
	case CVI_MOTOR_IOC_SET_ZOOM_SPEED: {
		if (copy_from_user(&reg, (void __user *)arg, sizeof(reg)) == 0)
			ret = set_zoom_speed(reg.val);
	} break;
	case CVI_MOTOR_IOC_SET_FOCUS_SPEED: {
		if (copy_from_user(&reg, (void __user *)arg, sizeof(reg)) == 0)
			ret = set_focus_speed(reg.val);
	} break;
	case CVI_MOTOR_IOC_APPLY: {
		if (copy_from_user(&reg, (void __user *)arg, sizeof(reg)) == 0)
			ret = apply();
	} break;
	case CVI_MOTOR_IOC_GET_INFO: {
		if (copy_from_user(&info, (void __user *)arg, sizeof(info)) == 0) {
			get_info(&info);
			ret = copy_to_user((void __user *)arg, &info, sizeof(info));
		}
	} break;
	default:
		return -ENOTTY;
	}
	return ret;
}

static const struct file_operations motor_fops = {
	.owner = THIS_MODULE,
	.open = fp_motor_open,
	.release = fp_motor_close,
	.unlocked_ioctl = fp_motor_ioctl,
};

static int proc_motor_show(struct seq_file *m, void *v)
{
	seq_puts(m, "[MOTOR] motor log have not implemented yet\n");
	return 0;
}

static int proc_motor_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_motor_show, PDE_DATA(inode));
}

static ssize_t proc_motor_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	u32 user_input_param = 0;

	if (kstrtouint_from_user(user_buf, count, 0, &user_input_param)) {
		pr_err("\n[Motor] input parameter incorrect\n");
		return count;
	}

	if (user_input_param == 0) {
		pr_err("motor zoom in\n");
		focus_in(0);
		zoom_in(255);
		apply();
	} else if (user_input_param == 1) {
		pr_err("motor zoom out\n");
		focus_out(0);
		zoom_out(255);
		apply();
	} else if (user_input_param == 2) {
		pr_err("motor focus in\n");
		zoom_in(0);
		focus_in(255);
		apply();
	} else if (user_input_param == 3) {
		pr_err("motor focus out\n");
		zoom_out(0);
		focus_out(255);
		apply();
	}

	return count;
}

static const struct proc_ops motor_proc_ops = {
	.proc_open = proc_motor_open,
	.proc_read = seq_read,
	.proc_write = proc_motor_write,
	.proc_release = single_release,
};

const struct of_device_id cvi_of_match_table[] = {
	{.compatible = "ms41929"},
	{}
};

const struct spi_device_id cvi_id_table[] = {
	{"ms41929"},
	{}
};

struct spi_driver spi_ms41929 = {
	.probe = ms41929_probe,
	.remove = ms41929_remove,
	.driver = {
		.name = "ms41929",
		.owner = THIS_MODULE,
		.of_match_table = cvi_of_match_table,
	},
	.id_table = cvi_id_table,
};

static int __init cvi_motor_init(void)
{
	int ret;

	spin_lock_init(&ndev.lock);

	//register dev number
	if (ndev.major) {
		ndev.devid = MKDEV(ndev.major, 0);
		register_chrdev_region(ndev.devid, 1, CVI_MOTOR_CDEV_NAME);
	} else {
		ret = alloc_chrdev_region(&ndev.devid, 0, 1, CVI_MOTOR_CDEV_NAME);
		ndev.major = MAJOR(ndev.devid);
		ndev.minor = MINOR(ndev.devid);
	}

	if (ret < 0) {
		pr_err("[MOTOR] chr_dev region err\n");
		return -1;
	}

	//register chardev
	ndev.cdev.owner = THIS_MODULE;
	cdev_init(&ndev.cdev, &motor_fops);
	ret = cdev_add(&ndev.cdev, ndev.devid, 1);
	if (ret < 0) {
		pr_err("[MOTOR] cdev_add err\n");
		return -1;
	}

	//create device node
	ndev.class = class_create(THIS_MODULE, CVI_MOTOR_CLASS_NAME);
	if (IS_ERR(ndev.class)) {
		pr_err("[MOTOR] create class failed\n");
		return PTR_ERR(ndev.class);
	}
	ndev.device = device_create(ndev.class, NULL, ndev.devid, NULL, CVI_MOTOR_CDEV_NAME);
	if (IS_ERR(ndev.device))
		return PTR_ERR(ndev.device);

	//create proc node
	ndev.proc_dir = proc_mkdir("motor", NULL);
	if (!proc_create_data("hw_control", 0644, ndev.proc_dir, &motor_proc_ops, &ndev))
		pr_err("[MOTOR] motor hw_control proc creation failed\n");

	//insmod driver
	//motor driver maybe pwm, gpio, uart...............
	//not only spi
	ret = spi_register_driver(&spi_ms41929);

	if (ret < 0) {
		pr_err("spi register driver err\n");
		return ret;
	}
	return 0;
}

static void __exit cvi_motor_exit(void)
{
	spi_unregister_driver(&spi_ms41929);

	cdev_del(&ndev.cdev);

	unregister_chrdev_region(ndev.devid, 1);

	device_destroy(ndev.class, ndev.devid);

	class_destroy(ndev.class);

	proc_remove(ndev.proc_dir);
}

module_init(cvi_motor_init);
module_exit(cvi_motor_exit);

MODULE_DESCRIPTION("Cvitek Motor Driver");
MODULE_AUTHOR("Oliver he<Oliver.he@sophgo.com>");
MODULE_LICENSE("GPL");
