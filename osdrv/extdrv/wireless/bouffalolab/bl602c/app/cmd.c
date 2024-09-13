/**
 ******************************************************************************
 *
 *  @file cmd.c
 *
 *  Copyright (C) BouffaloLab 2017-2024
 *
 *  Licensed under the Apache License, Version 2.0 (the License);
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an ASIS BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************
 */



#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include <signal.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/mman.h>

#define PAGE_SIZE (4*1024)
#define BUF_SIZE (32*PAGE_SIZE)
#define OFFSET (0)

static const char version[] = "0.0.0.1 2019-2-21";

/* sysfs constants */
#define SYSFS_MNT_PATH         "/sys"
#define SYSFS_BUS_NAME         "bus"
#define SYSFS_BUS_TYPE         "mmc"
#define SYSFS_DEVICES_NAME     "devices"

/*TODO: auto recognition*/
#define SYSFS_MMC_NAME				   "mmc0:0001/mmc0:0001:1" 
#define SYSFS_MODULE_ATTR_NAME         "filter_module"
#define SYSFS_SEVERITY_ATTR_NAME       "filter_severity"

static int write_sysfs_attribute(const char *attr_path, const char *new_value,
			  size_t len)
{
	int fd;
	int length;

	fd = open(attr_path, O_WRONLY);
	if (fd < 0) {
		printf("error opening attribute %s", attr_path);
		return -1;
	}

	length = write(fd, new_value, len);
	if (length < 0) {
		printf("error writing to attribute %s", attr_path);
		close(fd);
		return -1;
	}

	close(fd);

	return 0;
}


static void do_change_module(char *arg)
{
	char module_attr_path[255];
	char *end;

	unsigned long int filter_module = strtoul(arg, &end, 16);
	char module[30];

	printf("parsing module arg '%s', filter_module=0x%x(%d)\n", arg, filter_module, filter_module);

	snprintf(module, sizeof(module), "%ld\n", filter_module);

    snprintf(module_attr_path, sizeof(module_attr_path), "%s/%s/%s/%s/%s/%s",
		 SYSFS_MNT_PATH, SYSFS_BUS_NAME, SYSFS_BUS_TYPE,
		 SYSFS_DEVICES_NAME, SYSFS_MMC_NAME, SYSFS_MODULE_ATTR_NAME);
	
	write_sysfs_attribute(module_attr_path, module, strlen(module));
}

static void do_change_level(char *arg)
{
	char severity_attr_path[255];
	char *end;

	unsigned long int filter_severity = strtoul(arg, &end, 16);
	char severity[30];

	printf("parsing severity arg '%s', filter_severity=0x%x(%d)\n", arg, filter_severity, filter_severity);

	snprintf(severity, sizeof(severity), "%ld\n", filter_severity);

    snprintf(severity_attr_path, sizeof(severity_attr_path), "%s/%s/%s/%s/%s/%s",
		 SYSFS_MNT_PATH, SYSFS_BUS_NAME, SYSFS_BUS_TYPE,
		 SYSFS_DEVICES_NAME, SYSFS_MMC_NAME, SYSFS_SEVERITY_ATTR_NAME);
	
	write_sysfs_attribute(severity_attr_path, severity, strlen(severity));
}

static void do_dump_buf()
{
	int fd;
	char *addr = NULL;

	fd = open("/dev/bl_log", O_RDWR);
	if (fd < 0) {
		perror("open failed\n");
		exit(-1);
	}

	addr = mmap(NULL, BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED, fd, OFFSET);
	if (!addr) {
		perror("mmap failed\n");
		exit(-1);
	}

	printf("%s\n", addr);

	munmap(addr, BUF_SIZE);
	close(fd);
}

static const char help_message[] = "\
Usage: [options]				\n\
	-d, --dump				\n\
		dump driver log buf.	\n\
						\n\
	-m, --module				\n\
		change module.	\n\
						\n\
	-l, --level				\n\
		change level			\n\
						\n\
	-h, --help 				\n\
		Print this help.		\n";

static void show_help(void)
{
	printf("%s", help_message);
}

static const struct option longopts[] = {
	{"dump",	no_argument,	NULL, 'd'},
	{"level",	required_argument,	NULL, 'l'},
	{"module",	required_argument,	NULL, 'm'},
	{"version",	no_argument,	NULL, 'v'},
	{"help",	no_argument,	NULL, 'h'},
	{NULL,		0,		NULL,  0}
};


int main(int argc, char *argv[])
{
	char *module = NULL;
	char *severity = NULL;
	enum {
		cmd_dump = 1,
		cmd_change_module,
		cmd_change_level,
		cmd_help,
		cmd_version
	} cmd = cmd_help;

	if (geteuid() != 0)
		printf("not running as root?");

	for (;;) {
		int c;
		int index = 0;

		c = getopt_long(argc, argv, "dm:l:hv", longopts, &index);

		if (c == -1)
			break;

		switch (c) {
		case 'd':
			cmd = cmd_dump;
			break;
		case 'v':
			cmd = cmd_version;
			break;
		case 'h':
			cmd = cmd_help;
			break;
		case 'm':
			module = optarg;
			cmd = cmd_change_module;
			break;
		case 'l':
			severity = optarg;
			cmd = cmd_change_level;
			break;
		case '?':
			show_help();
			exit(EXIT_FAILURE);
		default:
			printf("getopt error\n");
			break;
		}
	}

	switch (cmd) {
	case cmd_dump:
		do_dump_buf();
		break;
	case cmd_change_module:
		do_change_module(module);
		break;
	case cmd_change_level:
		do_change_level(severity);
		break;
	case cmd_version:
		printf("%s\n", version);
		break;
	case cmd_help:
		show_help();
		break;
	default:
		printf("unknown cmd\n");
		show_help();
		break;
	}

	return 0;
}
