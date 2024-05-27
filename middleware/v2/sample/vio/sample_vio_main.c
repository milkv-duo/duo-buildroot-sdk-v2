#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "sample_comm.h"
#include "sample_vio.h"
#include "cvi_sys.h"
#include <linux/cvi_type.h>

void SAMPLE_VIO_HandleSig(CVI_S32 signo)
{
	signal(SIGINT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);

	if (SIGINT == signo || SIGTERM == signo) {
		//todo for release
		SAMPLE_PRT("Program termination abnormally\n");
	}
	exit(-1);
}

void SAMPLE_VIO_Usage(char *sPrgNm)
{
	printf("Usage : %s <index>\n", sPrgNm);
	printf("index:\n");
	printf("\t 0)VI (Offline) - VPSS(Offline) - VO(Rotation).\n");
	printf("\t 1)VI (Offline) - VPSS(Offline,Keep Aspect Ratio) - VO.\n");
	printf("\t 2)VI (Offline, Rotation) - VPSS(Offline,Keep Aspect Ratio) - VO.\n");
	printf("\t 3)VI (Offline) - VPSS(Offline, Rotation) - VO.\n");
	printf("\t 4)VPSS(Offline, file read/write).\n");
	printf("\t 5)VI (Two devs) - VPSS - VO.\n");
	printf("\t 6)VPSS(Offline, file read/write, combine 2 frame into 1).\n");
}

int main(int argc, char *argv[])
{
	CVI_S32 s32Ret = CVI_FAILURE;
	CVI_S32 s32Index;

	if (argc < 2) {
		SAMPLE_VIO_Usage(argv[0]);
		return CVI_FAILURE;
	}

	if (!strncmp(argv[1], "-h", 2)) {
		SAMPLE_VIO_Usage(argv[0]);
		return CVI_SUCCESS;
	}

	signal(SIGINT, SAMPLE_VIO_HandleSig);
	signal(SIGTERM, SAMPLE_VIO_HandleSig);

	s32Index = atoi(argv[1]);
	switch (s32Index) {
	case 0:
		s32Ret = SAMPLE_VIO_VoRotation();
		break;

	case 1:
		s32Ret = SAMPLE_VIO_ViVpssAspectRatio();
		break;

	case 2:
		s32Ret = SAMPLE_VIO_ViRotation();
		break;

	case 3:
		s32Ret = SAMPLE_VIO_VpssRotation();
		break;

	case 4: {
		SIZE_S stSize = {1920, 1080};

		s32Ret = SAMPLE_VIO_VpssFileIO(stSize);
		break;
	}

	case 5:
		s32Ret = SAMPLE_VIO_TWO_DEV_VO();
		break;

	case 6: {
		SIZE_S stSize = {1920 ,1080};

		s32Ret = SAMPLE_VIO_VpssCombine2File(stSize);
		break;
	}
	default:
		SAMPLE_PRT("the index %d is invaild!\n", s32Index);
		SAMPLE_VIO_Usage(argv[0]);
		return CVI_FAILURE;
	}

	if (s32Ret == CVI_SUCCESS)
		SAMPLE_PRT("sample_vio exit success!\n");
	else
		SAMPLE_PRT("sample_vio exit abnormally!\n");

	return s32Ret;
}

