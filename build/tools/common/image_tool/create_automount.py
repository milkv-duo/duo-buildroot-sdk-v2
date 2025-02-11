#!/usr/bin/python3

# test by:
# build$ python3 tools/common/image_tool/create_automount.py boards/default/partition/partition_spinor.xml .
# build$ python3 tools/common/image_tool/create_automount.py boards/default/partition/partition_spinand_page_2k.xml .
# build$ python3 tools/common/image_tool/create_automount.py boards/default/partition/partition_emmc.xml .

import logging
import argparse
from os import path, chmod
from XmlParser import XmlParser

FORMAT = "%(levelname)s: %(message)s"
logging.basicConfig(level=logging.INFO, format=FORMAT)

header = """#!/bin/sh
# This file is automatically generated by create_automount.py
# Please do not modify this file manually!

${CVI_SHOPTS}
# DL_FLAG=`devmem 0x0e000030`
ENV_DLFLAG=`fw_printenv dl_flag 2>/dev/null | awk -F= '{print $2}'`

case "$1" in
"""

# 挂载分区到xml中指定的 mountpoint. 若挂载失败，
# 通过 flash_erase -j /dev/mtd{dev_no} 0 0 对 mtdx 设备进行全部擦除操作，
# 并在 MTD 设备上创建一个干净的 JFFS2 文件系统。然后重新挂载。
nor_format = """
	printf "############ Mounting {label} partition #############\\n"
	if ! mount -t {filesystem} /dev/mtdblock{dev_no} {mountpoint}; then
		echo 'Mount {label} failed, Try erasing and remounting'
		flash_erase -j /dev/mtd{dev_no} 0 0
		mount -t {filesystem} /dev/mtdblock{dev_no} {mountpoint}
	fi
"""

# 挂载分区到xml中指定的 mountpoint.
nand_format = """
	printf "############ Mounting {label} partition #############\\n"
	ubiattach /dev/ubi_ctrl -m {dev_no}
	if [ $? != 0 ]; then
		ubiformat -y /dev/mtd{dev_no}
		ubiattach /dev/ubi_ctrl -m {dev_no}
		ubimkvol /dev/ubi{ubi_cnt} -N {label} -m
	fi
	if [ ! -c /dev/ubi{ubi_cnt}_0 ]; then
		mdev -s
	fi
	mount -t ubifs -o sync /dev/ubi{ubi_cnt}_0 {mountpoint}
	if [ $? != 0 ]; then
		echo 'Mount {label} failed, Try formatting and remounting'
		ubimkvol /dev/ubi{ubi_cnt} -N {label} -m
		mount -t ubifs -o sync /dev/ubi{ubi_cnt}_0 {mountpoint}
	fi
"""

emmc_data_format = """
	if [ "$DL_FLAG" == '0x50524F47' ] || [ -z $ENV_DLFLAG ] || [ $ENV_DLFLAG == 'prog' ]; then
		printf "OK\\nFix\\n" | parted ---pretend-input-tty /dev/mmcblk0 print
		parted -s /dev/mmcblk0 resizepart {dev_no} 100%
	fi
"""

emmc_format = """
	printf "############ Mounting {label} partition #############\\n"
	{data_partition}
	e2fsck -y /dev/mmcblk0p{dev_no}
	mount -t {type} -o sync /dev/mmcblk0p{dev_no} {mountpoint}
	if [ $? != 0 ]; then
		echo 'Mount {label} failed, Try formatting and remounting'
		mke2fs -T {type} /dev/mmcblk0p{dev_no}
		mount -t {type} -o sync /dev/mmcblk0p{dev_no} {mountpoint}
		resize2fs /dev/mmcblk0p{dev_no}
	elif [ "$DL_FLAG" == '0x50524F47' ] || [ -z $ENV_DLFLAG ] || [ $ENV_DLFLAG == 'prog' ]; then
		resize2fs /dev/mmcblk0p{dev_no}
	fi
"""

def parse_Args():
    parser = argparse.ArgumentParser(description="Create CVITEK device image")
    parser.add_argument("xml", help="path to partition xml")
    parser.add_argument(
        "output",
        metavar="output",
        type=str,
        help="the output folder for saving the S10auto_mount",
    )
    args = parser.parse_args()
    return args


def genCase(case, out, parts, storage):
    out.write("%s)\n" % case)
    ubi_cnt = 0
    for i, p in enumerate(parts):
        if p["label"] in ("BOOT", "MISC", "ROOTFS", "fip"):
            continue
        if not p["mountpoint"]:
            continue
        if case == "start":
            if storage == "emmc":
                data_script = ""
                # 如果是最后一个分区，会调用 parted 扩展分区到 100%
                if i == len(parts) - 1:
                    data_script = emmc_data_format.format(dev_no=i + 1)

                script = emmc_format.format(
                    data_partition=data_script,
                    dev_no=i+1,
                    type=p["type"],
                    mountpoint=p["mountpoint"],
                    label=p["label"]
                )
                out.write(script)

            elif storage == "spinand":
                ubi_cnt += 1
                if p["type"] != "ubifs":
                    assert "Only supoort ubifs"

                script = nand_format.format(
                    dev_no=i,
                    mountpoint=p["mountpoint"],
                    label=p["label"],
                    ubi_cnt=ubi_cnt
                )
                out.write(script)

            elif storage == "spinor":
                script = nor_format.format(
                    filesystem="jffs2",
                    dev_no=i,
                    mountpoint=p["mountpoint"],
                    label=p["label"]
                )
                out.write(script)

        else:
            out.write('printf "Unmounting %s partition\\n"\n' % p["label"])
            out.write("umount %s\n" % p["mountpoint"])

    # Set DL_FLAG flag to complete
    if case == "start" and storage == "emmc":
        # out.write("devmem 0x0e000030 32 0x444F4E45\n")
        out.write("\tfw_setenv dl_flag done\n")
    out.write("\t;;\n")


def main():
    args = parse_Args()
    xmlParser = XmlParser(args.xml)
    parts = xmlParser.parse()
    out_path = path.join(args.output, "S10auto_mount")
    try:
        out = open(out_path, "w")
    except Exception:
        logging.error("Create S10auto_mount failed")
        raise
    out.write(header)
    genCase("start", out, parts, xmlParser.getStorage())
    out.write("esac\n")
    out.close()
    chmod(out_path, 0o755)


if __name__ == "__main__":
    main()
