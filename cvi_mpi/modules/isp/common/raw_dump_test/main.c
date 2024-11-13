#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/param.h>
#include <inttypes.h>

#include <fcntl.h>
#include "cvi_buffer.h"

#include "raw_dump_internal.h"

#include "../inc/pipe_helper.h"

void printfHelp(void)
{
	printf("\r\n\r\n");
	printf("dump one, ./raw_dump path\n");
	printf("\teg: ./raw_dump /mnt/sd\n");
	printf("dump continuous raw, ./raw_dump path count,roi_x,roi_y,roi_w,roi_h\n");
	printf("\teg: ./raw_dump /mnt/sd 10,0,0,960,540\n");
	printf("\r\n\r\n");
}

int execute_raw_dump(int argc, char **argv)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	RAW_DUMP_INFO_S stRawDumpInfo;

	memset(&stRawDumpInfo, 0, sizeof(RAW_DUMP_INFO_S));

	if (argc == 2 || argc == 3) {

		if (access(argv[1], F_OK) == 0) {

			stRawDumpInfo.pathPrefix = argv[1];

		} else {
			printfHelp();
			printf("ERROR: %s not exist!!!\n", argv[1]);
			return CVI_FAILURE;
		}

		if (argc == 2) {

			stRawDumpInfo.u32TotalFrameCnt = 1;

		} else {

			char *pstr = NULL;
			char *ptemp = NULL;

			ptemp = strtok_r(argv[2], ",", &pstr);
			if (ptemp == NULL) {
				printf("param error!!!\n");
				return CVI_FAILURE;
			}

			stRawDumpInfo.u32TotalFrameCnt = atoi(ptemp);

			ptemp = strtok_r(NULL, ",", &pstr);
			if (ptemp == NULL) {
				printf("param error!!!\n");
				return CVI_FAILURE;
			}

			stRawDumpInfo.stRoiRect.s32X = atoi(ptemp);

			ptemp = strtok_r(NULL, ",", &pstr);
			if (ptemp == NULL) {
				printf("param error!!!\n");
				return CVI_FAILURE;
			}

			stRawDumpInfo.stRoiRect.s32Y = atoi(ptemp);

			ptemp = strtok_r(NULL, ",", &pstr);
			if (ptemp == NULL) {
				printf("param error!!!\n");
				return CVI_FAILURE;
			}

			stRawDumpInfo.stRoiRect.u32Width = atoi(ptemp);

			ptemp = strtok_r(NULL, ",", &pstr);
			if (ptemp == NULL) {
				printf("param error!!!\n");
				return CVI_FAILURE;
			}

			stRawDumpInfo.stRoiRect.u32Height = atoi(ptemp);

			printf("count: %d, roi: %d,%d,%d,%d\n",
				stRawDumpInfo.u32TotalFrameCnt,
				stRawDumpInfo.stRoiRect.s32X,
				stRawDumpInfo.stRoiRect.s32Y,
				stRawDumpInfo.stRoiRect.u32Width,
				stRawDumpInfo.stRoiRect.u32Height);
		}

	} else {
		printfHelp();
		return CVI_FAILURE;
	}

	s32Ret = sys_vi_init();
	if (s32Ret != CVI_SUCCESS) {
		return s32Ret;
	}

	sleep(1);

	printf("\r\n\r\n");
	system("cat /proc/cvitek/vi");

	printf("\r\n\r\n");
	system("cat /proc/cvitek/vi_dbg");

	printf("\r\n\r\n");
	s32Ret = cvi_raw_dump(0, &stRawDumpInfo);

	sys_vi_deinit();

	return s32Ret;
}

int main(int argc, char **argv)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	if (argc == 2 || argc == 3) {
		return execute_raw_dump(argc, argv);
	}

	s32Ret = sys_vi_init();
	if (s32Ret != CVI_SUCCESS) {
		return s32Ret;
	}

	sleep(1);

	printf("\r\n\r\n");
	system("cat /proc/cvitek/vi");

	printf("\r\n\r\n");
	system("cat /proc/cvitek/vi_dbg");

	printf("\r\n\r\n");

	cvi_raw_dump_run((void *) 0);

	sys_vi_deinit();

	return s32Ret;
}

