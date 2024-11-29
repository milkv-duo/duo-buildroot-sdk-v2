
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
#include <dirent.h>

#include <fcntl.h>
#include <sys/stat.h>
#include "cvi_buffer.h"
#include "3A_internal.h"

#include "parse_raw.h"

#define _ENABLE_JPEG 1
#include "../inc/pipe_helper.h"

#include "../raw_replay/raw_replay.h"

#define LOGOUT(fmt, arg...) printf("%s,%d: " fmt, __func__, __LINE__, ##arg)

#define ABORT_IF(EXP)                  \
	do {                               \
		if (EXP) {                     \
			LOGOUT("abort: "#EXP"\n"); \
			abort();                   \
		}                              \
	} while (0)

#define RETURN_FAILURE_IF(EXP)         \
	do {                               \
		if (EXP) {                     \
			LOGOUT("fail: "#EXP"\n");  \
			return CVI_FAILURE;        \
		}                              \
	} while (0)

#define GOTO_FAIL_IF(EXP, LABEL)       \
	do {                               \
		if (EXP) {                     \
			LOGOUT("fail: "#EXP"\n");  \
			goto LABEL;                \
		}                              \
	} while (0)

#define WARN_IF(EXP)                   \
	do {                               \
		if (EXP) {                     \
			LOGOUT("warn: "#EXP"\n");  \
		}                              \
	} while (0)

#define ERROR_IF(EXP)                  \
	do {                               \
		if (EXP) {                     \
			LOGOUT("error: "#EXP"\n"); \
		}                              \
	} while (0)

#define DEBUG_PARSE_RAW 0
#define PATH_MAX_LEN  512

#define _SCRIPT_START        "test_start"
#define _SCRIPT_TEST_DIR     "test_dir"
#define _SCRIPT_PQ_BIN       "test_pq_bin"
#define _SCRIPT_OUTPUT_DIR   "test_output_dir"
#define _SCRIPT_ROI          "test_roi"
#define _SCRIPT_SNS_CFG      "test_sensor_cfg"
#define _SCRIPT_TEST_MD5     "test_md5"
#define _SCRIPT_END          "test_end"

typedef enum {
	MD5_TEST_NONE,
	MD5_TEST_SUCCEED,
	MD5_TEST_FAIL,
	MD5_TEST_STATUS_MAX
} _MD5_TEST_STATUS_E;

typedef struct {
	char *test_dir;
	char *output_dir;
	char *pq_bin_path;
	char *roi_info;
	char *sensor_cfg_path;

	bool bEnableMd5Test;
	_MD5_TEST_STATUS_E enMd5TestStatus;

} RAW_REPLAY_TEST_CTX_S;

static RAW_REPLAY_TEST_CTX_S stTestCtx;

void printfHelp(void)
{
	printf("\r\n\r\n");
	printf("./raw_replay_test path\n");
	printf("\teg: ./raw_replay_test /mnt/sd\n");
	printf("\r\n\r\n");
}

#if DEBUG_PARSE_RAW

int raw_replay_test_main(int argc, char **argv)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	if (argc != 2) {
		LOGOUT("ERROR:\n");
		printfHelp();
		return CVI_FAILURE;
	}

	if (access(argv[1], F_OK) != 0) {
		LOGOUT("ERROR:\n");
		printfHelp();
		return CVI_FAILURE;
	}

	CVI_U32 u32TotalItem = 0;

	parse_raw_init(argv[1], &u32TotalItem);

	LOGOUT("work path: %s, test item: %d\n", argv[1], u32TotalItem);

	CVI_U32 u32TotalFrame = 0;
	char *pathPrefix = NULL;

	void *pHeader = NULL;
	void *pData = NULL;
	CVI_U32 u32RawSize = 0;

	for (CVI_U32 i = 0; i < u32TotalItem; i++) {

		get_raw_data_info(i, &u32TotalFrame, &pathPrefix);

		LOGOUT("item: %d, total frame: %d, pathPrefix: %s\n", i, u32TotalFrame, pathPrefix);

		for (CVI_U32 j = 0; j < u32TotalFrame; j++) {
			s32Ret = get_raw_data(j, &pHeader, &pData, &u32RawSize);
			ERROR_IF(s32Ret != CVI_SUCCESS);
			LOGOUT("get %d, %d\n", j, u32RawSize);
		}
	}

	parse_raw_deinit();

	return s32Ret;
}

#else

static bool bEnableVencThread;
static bool bEnableDumpYuv;
static bool bEnableDumpStream;
static char pathPrefix[PATH_MAX_LEN];

static pthread_t vencThreadTid;

#define WAIT_ISP_STABLE_FRAME (100)
#define RECORD_FRAME_MAX      (25 * 10)
#define WAIT_TIMEOUT_COUNT    (60)
static CVI_U32 u32RecordFrameCount;
static CVI_U32 u32VencWorkFrameCount;

static void load_pq_bin(void)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	FILE *fp = NULL;
	CVI_U8 *buf = NULL;
	struct stat statbuf;

	if (stTestCtx.pq_bin_path == NULL) {
		LOGOUT("skip load pq bin...\n");
		return;
	}

	fp = fopen(stTestCtx.pq_bin_path, "rb");
	if (fp == NULL) {
		LOGOUT("open pq bin faile: %s fail!\n", stTestCtx.pq_bin_path);
		return;
	}

	stat(stTestCtx.pq_bin_path, &statbuf);

	buf = (CVI_U8 *) malloc(statbuf.st_size);
	if (buf == NULL) {
		fclose(fp);
		LOGOUT("malloc buffer size: %ld fail\n", statbuf.st_size);
		return;
	}

	fread(buf, statbuf.st_size, 1, fp);
	fclose(fp);

	for (CVI_U32 idx = CVI_BIN_ID_MIN; idx < CVI_BIN_ID_MAX; idx++) {
		s32Ret = CVI_BIN_LoadParamFromBin((enum CVI_BIN_SECTION_ID) idx, buf);
		if (s32Ret != CVI_SUCCESS) {
			LOGOUT("load param from bin fail, id = %d\n", idx);
		}
	}

	free(buf);
	buf = NULL;
}

static void md5_test_config(void)
{
	CVI_S32 ret = CVI_SUCCESS;
	ISP_DRC_ATTR_S stDRCAttr;
	ISP_TNR_ATTR_S stTNRAttr;

	ret = CVI_ISP_GetDRCAttr(0, &stDRCAttr);
	ERROR_IF(ret != CVI_SUCCESS);
	stDRCAttr.ToneCurveSmooth = 0;
	ret = CVI_ISP_SetDRCAttr(0, &stDRCAttr);
	ERROR_IF(ret != CVI_SUCCESS);

	ret = CVI_ISP_GetTNRAttr(0, &stTNRAttr);
	ERROR_IF(ret != CVI_SUCCESS);
	stTNRAttr.Enable = 0;
	ret = CVI_ISP_SetTNRAttr(0, &stTNRAttr);
	ERROR_IF(ret != CVI_SUCCESS);
}

static void md5_test(void)
{
	LOGOUT("\n\n start yuv md5 test...\n\n");

	FILE *fd = NULL;
	struct stat statbuf;
	char src[PATH_MAX_LEN] = {0};
	char des[PATH_MAX_LEN] = {0};

	char *ptemp = NULL;

	snprintf(src, PATH_MAX_LEN, "md5sum %s.yuv", pathPrefix);

	fd = popen(src, "r");

	if (fd == NULL) {
		LOGOUT("fail, popen: %s\n", src);
		return;
	}

	memset(src, 0, PATH_MAX_LEN);

	fgets(src, PATH_MAX_LEN, fd);

	pclose(fd);
	fd = NULL;

	ptemp = strchr(src, ' ');
	*ptemp = '\0';

	LOGOUT("src md5: %s\n", src);

	snprintf(des, PATH_MAX_LEN, "%s", pathPrefix);

	ptemp = strrchr(des, '/');
	*ptemp = '\0';

	strcat(des, "/md5.txt");

	stat(des, &statbuf);

	if (statbuf.st_size == 0) {
		LOGOUT("md5 txt empty, generate md5 txt...\n");

		fd = fopen(des, "wb");

		if (fd == NULL) {
			LOGOUT("fopen md5 txt fail, %s\n", des);
			return;
		}

		fwrite(src, strlen(src), 1, fd);
		fclose(fd);
		fd = NULL;
		return;
	}

	fd = fopen(des, "rb");

	if (fd == NULL) {
		LOGOUT("fopen md5 txt fail, %s\n", des);
		return;
	}

	memset(des, 0, PATH_MAX_LEN);

	fgets(des, PATH_MAX_LEN, fd);

	fclose(fd);
	fd = NULL;

	LOGOUT("des md5: %s\n", des);

	if (strstr(des, src) == NULL) {
		LOGOUT("\n\n\t md5 test fail...\n\n");
		stTestCtx.enMd5TestStatus = MD5_TEST_FAIL;
	} else {
		LOGOUT("\n\n\t md5 test success...\n\n");
		if (stTestCtx.enMd5TestStatus == MD5_TEST_NONE) {
			stTestCtx.enMd5TestStatus = MD5_TEST_SUCCEED;
		}
	}
}

static void test(CVI_U32 u32ItemIndex)
{
	CVI_S32 ret = CVI_SUCCESS;

	CVI_U32 u32TotalFrame = 0;
	CVI_U32 u32FrameIndex = 0;
	char *prefix = NULL;
	char *temp = NULL;
	CVI_U32 length = 0;

	CVI_U32 u32WaitTimeoutCount = 0;

	ret = get_raw_data_info(u32ItemIndex, &u32TotalFrame, &prefix);
	ERROR_IF(ret != CVI_SUCCESS);

	LOGOUT("\n\nstart, index: %d, total frame: %d, %s\n\n",
		u32ItemIndex, u32TotalFrame, prefix);

	prefix = strrchr(prefix, '/');   // file name

	temp = prefix - 1;
	while (*temp != '/') {  // get dir name
		temp--;
	}

	memset(pathPrefix, 0, PATH_MAX_LEN);

	snprintf(pathPrefix, PATH_MAX_LEN, "%s", stTestCtx.output_dir);

	length = prefix - temp;

	if (length > PATH_MAX_LEN - strlen(pathPrefix)) {
		length = PATH_MAX_LEN - strlen(pathPrefix);
	}

	memcpy(pathPrefix + strlen(pathPrefix), temp, length);

	LOGOUT("output dir: %s\n", pathPrefix);

	if (access(pathPrefix, F_OK) != 0) {
		ERROR_IF(mkdir(pathPrefix, 0755) != 0);
	}

	length = strlen(prefix);

	if (length > PATH_MAX_LEN - strlen(pathPrefix)) {
		length = PATH_MAX_LEN - strlen(pathPrefix);
	}

	memcpy(pathPrefix + strlen(pathPrefix), prefix, length);

	LOGOUT("output path prefix: %s\n", pathPrefix);

	void *pHeader = NULL;
	void *pData = NULL;
	CVI_U32 u32RawSize = 0;

	RAW_REPLAY_INFO rawInfo;

	for (; u32FrameIndex < u32TotalFrame; u32FrameIndex++) {

		LOGOUT("load data, total frame: %d, index: %d\n", u32TotalFrame, u32FrameIndex);

		ret = get_raw_data(u32FrameIndex, &pHeader, &pData, &u32RawSize);
		ERROR_IF(ret != CVI_SUCCESS);

		rawInfo = *((RAW_REPLAY_INFO *) pHeader);

		if (rawInfo.numFrame != rawInfo.roiFrameNum + 1) {
			rawInfo.roiFrameNum = rawInfo.numFrame - 1;
		}

		ret = set_raw_replay_data(&rawInfo, pData, u32TotalFrame, u32FrameIndex, u32RawSize);
		ERROR_IF(ret != CVI_SUCCESS);
	}

	if (!stTestCtx.bEnableMd5Test) {
		raw_replay_ctrl(DISABLE_AWB_UPDATE_CTRL);
	}

	start_raw_replay(0);

	u32WaitTimeoutCount = 0;
	while (!is_raw_replay_ready() && u32WaitTimeoutCount < WAIT_TIMEOUT_COUNT) {
		u32WaitTimeoutCount++;
		usleep(500 * 1000);
	}
	ERROR_IF(u32WaitTimeoutCount >= WAIT_TIMEOUT_COUNT);

	load_pq_bin();

	//auto test should show the result of the final awb effect
	CVI_ISP_SetAwbSimMode(1);

	if (stTestCtx.bEnableMd5Test) {
		md5_test_config();
	}

	for (CVI_U8 i = 0; i < WAIT_ISP_STABLE_FRAME; i++) {
		CVI_ISP_GetVDTimeOut(0, ISP_VD_BE_END, 0);
	}

	if (!stTestCtx.bEnableMd5Test && (u32TotalFrame > 1)) {

		LOGOUT("\n\nstart record stream...\n\n");

		u32RecordFrameCount = 0;
		bEnableDumpStream = true;

		u32WaitTimeoutCount = 0;
		while (bEnableDumpStream && u32WaitTimeoutCount < WAIT_TIMEOUT_COUNT) {
			u32WaitTimeoutCount++;
			usleep(500 * 1000);
		}
		ERROR_IF(u32WaitTimeoutCount >= WAIT_TIMEOUT_COUNT);

		LOGOUT("\n\nrecord stream finish...\n\n");
	}

	LOGOUT("\n\nstart dump yuv...\n\n");

	bEnableDumpYuv = true;

	u32WaitTimeoutCount = 0;
	while (bEnableDumpYuv && u32WaitTimeoutCount < WAIT_TIMEOUT_COUNT) {
		u32WaitTimeoutCount++;
		usleep(500 * 1000);
	}
	ERROR_IF(u32WaitTimeoutCount >= WAIT_TIMEOUT_COUNT);

	if (stTestCtx.bEnableMd5Test) {
		md5_test();
		stTestCtx.bEnableMd5Test = false;
	}
}

static CVI_S32 dump_yuv(char *path, VIDEO_FRAME_INFO_S *pstVideoFrame)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	FILE *fp = fopen(path, "wb");

	RETURN_FAILURE_IF(fp == NULL);

	CVI_U32 u32PlanarCount = 3;
	CVI_U32 plane_offset = 0x00;

	VIDEO_FRAME_S *pstVFrame = &pstVideoFrame->stVFrame;

	CVI_U32 image_size = pstVFrame->u32Length[0] +
					pstVFrame->u32Length[1] +
					pstVFrame->u32Length[2];

	CVI_U8 *vir_addr = (CVI_U8 *) CVI_SYS_MmapCache(pstVFrame->u64PhyAddr[0], image_size);

	for (CVI_U32 u32PlanarIdx = 0; u32PlanarIdx < u32PlanarCount; u32PlanarIdx++) {

		pstVFrame->pu8VirAddr[u32PlanarIdx] = (CVI_U8 *)vir_addr + plane_offset;

		plane_offset += pstVFrame->u32Length[u32PlanarIdx];

		fwrite(pstVFrame->pu8VirAddr[u32PlanarIdx],
					pstVFrame->u32Length[u32PlanarIdx],
					1, fp);
	}

	CVI_SYS_Munmap(vir_addr, image_size);

	fflush(fp);
	fclose(fp);

	return s32Ret;
}

static void *venc_thread(void *params)
{
	params = params;

	CVI_S32 s32Ret = CVI_SUCCESS;

	FILE *fp = NULL;
	char path[PATH_MAX_LEN] = { 0 };
#ifdef _ENABLE_JPEG
	bool bDumpJpeg = false;
#endif
	VENC_CHN VencChn = _H264_VENC_CHN;

	CVI_S32 s32SetFrameMilliSec = 20000;

	VIDEO_FRAME_INFO_S stVencFrame;

	VENC_STREAM_S stStream;
	VENC_CHN_ATTR_S stVencChnAttr;
	VENC_CHN_STATUS_S stStat;

	memset(&stVencFrame, 0, sizeof(VIDEO_FRAME_INFO_S));
	memset(&stVencChnAttr, 0, sizeof(VENC_CHN_ATTR_S));
	memset(&stStat, 0, sizeof(VENC_CHN_STATUS_S));
	memset(&stStream, 0, sizeof(VENC_STREAM_S));

	while (bEnableVencThread) {

tryAgain:

		if (stStream.pstPack != NULL) {
			free(stStream.pstPack);
			stStream.pstPack = NULL;
		}

		memset(&stVencFrame, 0, sizeof(VIDEO_FRAME_INFO_S));
		s32Ret = CVI_VPSS_GetChnFrame(0, 0, &stVencFrame, s32SetFrameMilliSec);
		GOTO_FAIL_IF(s32Ret != CVI_SUCCESS, tryAgain);

		if (bEnableDumpYuv) {
			snprintf(path, PATH_MAX_LEN, "%s.yuv", pathPrefix);
			LOGOUT("save %s\n\n", path);
			dump_yuv(path, &stVencFrame);
#ifdef _ENABLE_JPEG
			bDumpJpeg = true;
			VencChn = _JPEG_VENC_CHN;
#else
			bEnableDumpYuv = false;
#endif
		}

		if (bEnableDumpStream) {

			if (fp == NULL) {
				snprintf(path, PATH_MAX_LEN, "%s.h264", pathPrefix);
				LOGOUT("save %s\n\n", path);
				fp = fopen(path, "wb");
				ERROR_IF(fp == NULL);

				s32Ret = CVI_VENC_RequestIDR(VencChn, CVI_TRUE);
				ERROR_IF(s32Ret != CVI_SUCCESS);
			}
		}

		s32Ret = CVI_VENC_SendFrame(VencChn, &stVencFrame, s32SetFrameMilliSec);
		GOTO_FAIL_IF(s32Ret != CVI_SUCCESS, tryAgain);

		memset(&stVencChnAttr, 0, sizeof(VENC_CHN_ATTR_S));
		s32Ret = CVI_VENC_GetChnAttr(VencChn, &stVencChnAttr);
		GOTO_FAIL_IF(s32Ret != CVI_SUCCESS, tryAgain);

		memset(&stStat, 0, sizeof(VENC_CHN_STATUS_S));
		s32Ret = CVI_VENC_QueryStatus(VencChn, &stStat);
		GOTO_FAIL_IF(s32Ret != CVI_SUCCESS, tryAgain);

		GOTO_FAIL_IF(!stStat.u32CurPacks, tryAgain);

		memset(&stStream, 0, sizeof(VENC_STREAM_S));
		stStream.pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
		GOTO_FAIL_IF(stStream.pstPack == NULL, tryAgain);

		s32Ret = CVI_VENC_GetStream(VencChn, &stStream, -1);
		GOTO_FAIL_IF(s32Ret != CVI_SUCCESS, tryAgain);

		if (bEnableDumpStream) {

			if (fp != NULL) {

				VENC_PACK_S *ppack = NULL;

				for (CVI_U32 i = 0; i < stStream.u32PackCount; i++) {
					ppack = &stStream.pstPack[i];
					fwrite(ppack->pu8Addr + ppack->u32Offset,
						ppack->u32Len - ppack->u32Offset, 1, fp);
				}

				u32RecordFrameCount++;

				if (u32RecordFrameCount > RECORD_FRAME_MAX) {
					fflush(fp);
					fclose(fp);
					fp = NULL;
					u32RecordFrameCount = 0;
					bEnableDumpStream = false;
				}
			}
		}
#ifdef _ENABLE_JPEG
		if (bDumpJpeg) {

			FILE *fp = NULL;

			snprintf(path, PATH_MAX_LEN, "%s.jpg", pathPrefix);
			LOGOUT("save %s\n\n", path);
			fp = fopen(path, "wb");
			ERROR_IF(fp == NULL);

			VENC_PACK_S *ppack = NULL;

			for (CVI_U32 i = 0; i < stStream.u32PackCount; i++) {
				ppack = &stStream.pstPack[i];
				fwrite(ppack->pu8Addr + ppack->u32Offset,
					ppack->u32Len - ppack->u32Offset, 1, fp);
			}

			fflush(fp);
			fclose(fp);
			fp = NULL;

			bDumpJpeg = false;
			bEnableDumpYuv = false;

			s32Ret = CVI_VENC_ReleaseStream(VencChn, &stStream);
			GOTO_FAIL_IF(s32Ret != CVI_SUCCESS, tryAgain);

			VencChn = _H264_VENC_CHN;

		} else {
			s32Ret = CVI_VENC_ReleaseStream(VencChn, &stStream);
			GOTO_FAIL_IF(s32Ret != CVI_SUCCESS, tryAgain);
		}
#else
		s32Ret = CVI_VENC_ReleaseStream(VencChn, &stStream);
		GOTO_FAIL_IF(s32Ret != CVI_SUCCESS, tryAgain);
#endif
		s32Ret = CVI_VPSS_ReleaseChnFrame(0, 0, &stVencFrame);
		GOTO_FAIL_IF(s32Ret != CVI_SUCCESS, tryAgain);

		u32VencWorkFrameCount++;
	}

	if (stStream.pstPack != NULL) {
		free(stStream.pstPack);
		stStream.pstPack = NULL;
	}

	if (fp != NULL) {
		fflush(fp);
		fclose(fp);
		fp = NULL;
	}

	LOGOUT("vencThread end...\n");

	return NULL;
}

//static void printfMemInfo(void)
//{
//	char buff[64] = {0};
//
//	sprintf(buff, "cat /proc/%d/status", getpid());
//	system((const char *) buff);
//	system("free -m");
//}

static void run_test(void)
{
	static bool bReplaceSensorCfg;

	CVI_U32 u32TotalItem = 0;
	CVI_U32 u32ItemIndex = 0;

	char buffer[PATH_MAX_LEN] = {0};

	DIR *dir = NULL;
	struct dirent *ptr = NULL;

	dir = opendir(stTestCtx.test_dir);

	if (dir == NULL) {
		return;
	}

	while ((ptr = readdir(dir)) != NULL) {

		// dir
		if (ptr->d_type == 4) {

			if (strcmp(ptr->d_name, "..") == 0) {
				continue;
			}

			memset(buffer, 0, PATH_MAX_LEN);
			snprintf(buffer, PATH_MAX_LEN, "%s/%s", stTestCtx.test_dir, ptr->d_name);

			parse_raw_init(buffer, stTestCtx.roi_info, &u32TotalItem);

			LOGOUT("work path: %s, test item: %d\n", buffer, u32TotalItem);

			if (u32TotalItem > 0) {

				if (access(stTestCtx.sensor_cfg_path, F_OK) == 0) {

					bReplaceSensorCfg = true;

					stop_raw_replay();

					LOGOUT("replace /mnt/data/sensor_cfg.ini\n");
					system("mv /mnt/data/sensor_cfg.ini /mnt/data/sensor_cfg.ini.bak");

					snprintf(buffer, PATH_MAX_LEN, "cp %s /mnt/data/sensor_cfg.ini",
						stTestCtx.sensor_cfg_path);

					system(buffer);
					system("sync");

				} else if (bReplaceSensorCfg) {

					bReplaceSensorCfg = false;

					stop_raw_replay();
				}

				if (stTestCtx.bEnableMd5Test) {

					LOGOUT("enable md5 test...\n");

					snprintf(buffer, PATH_MAX_LEN, "%s/%s/%s",
						stTestCtx.test_dir, ptr->d_name, "md5.txt");

					if (access(buffer, F_OK) != 0) {
						stTestCtx.bEnableMd5Test = true;
						snprintf(buffer, PATH_MAX_LEN, "touch %s/%s/md5.txt",
							stTestCtx.test_dir,
							ptr->d_name);
						system(buffer);
						system("sync");
					}
				}
			}

			for (u32ItemIndex = 0; u32ItemIndex < u32TotalItem; u32ItemIndex++) {
				LOGOUT("\n\nstart test item: %d, total: %d\n\n", u32ItemIndex, u32TotalItem);
				test(u32ItemIndex);
				//printfMemInfo();
			}

			parse_raw_deinit();

			if (bReplaceSensorCfg) {
				system("mv /mnt/data/sensor_cfg.ini.bak /mnt/data/sensor_cfg.ini");
			}
		}
	}
}

static char *get_str_value(const char *src, const char *str)
{
	int i = 0;
	char *temp = NULL;
	static char value[PATH_MAX_LEN] = {0};

	/* str = value */
	temp = strstr(src, str);
	GOTO_FAIL_IF(temp == NULL, fail);

	temp = strchr(temp, '=');
	GOTO_FAIL_IF(temp == NULL, fail);

	do {
		temp++;
	} while (*temp == ' ');

	memset(value, '\0', PATH_MAX_LEN);

	for (i = 0; i < PATH_MAX_LEN; i++) {

		value[i] = *temp;
		temp++;

		if (*temp == '\r' || *temp == '\n') {
			break;
		}
	}

	//LOGOUT("%s = %s\n", str, value);

	return value;

fail:

	LOGOUT("get %s value fail!\n", str);
	return NULL;
}

static void run_test_script(char *script_file)
{
	FILE *fp = NULL;
	char *script = NULL;
	char *start = NULL;
	char *end = NULL;
	char *value = NULL;
	struct stat statbuf;

	fp = fopen(script_file, "rb");
	GOTO_FAIL_IF(fp == NULL, fail);

	stat(script_file, &statbuf);

	script = (char *) calloc(statbuf.st_size + 1, 1);
	GOTO_FAIL_IF(script == NULL, fail);

	fread(script, statbuf.st_size, 1, fp);

	fclose(fp);
	fp = NULL;

	start = script;
	end = script;

	while (1) {

		if (stTestCtx.test_dir != NULL) {
			free(stTestCtx.test_dir);
			stTestCtx.test_dir = NULL;
		}

		if (stTestCtx.output_dir != NULL) {
			free(stTestCtx.output_dir);
			stTestCtx.output_dir = NULL;
		}

		if (stTestCtx.pq_bin_path != NULL) {
			free(stTestCtx.pq_bin_path);
			stTestCtx.pq_bin_path = NULL;
		}

		if (stTestCtx.roi_info != NULL) {
			free(stTestCtx.roi_info);
			stTestCtx.roi_info = NULL;
		}

		if (stTestCtx.sensor_cfg_path != NULL) {
			free(stTestCtx.sensor_cfg_path);
			stTestCtx.sensor_cfg_path = NULL;
		}

		start = end;

		start = strstr(start, _SCRIPT_START);
		if (start == NULL) {
			//LOGOUT("ERROR: not found script start tag: %s\n", _SCRIPT_START);
			LOGOUT("test script end, finish test...\n");
			break;
		}

		end = strstr(start, _SCRIPT_END);
		if (end == NULL) {
			LOGOUT("ERROR: not found script end tag: %s\n", _SCRIPT_END);
			break;
		}

		if (start > script) {
			char *temp = start - 1;

			if (*temp == '#') {
				LOGOUT("skip test item...\n");
				continue;
			}
		}

		value = strstr(start, _SCRIPT_TEST_DIR);

		if (value != NULL && value < end) {
			value = get_str_value(start, _SCRIPT_TEST_DIR);
			stTestCtx.test_dir = (char *) calloc(strlen(value) + 1, 1);
			strncpy(stTestCtx.test_dir, value, strlen(value));
		} else {
			continue;
		}

		if (access(stTestCtx.test_dir, F_OK) != 0) {
			LOGOUT("ERROR: test dir not exist: %s\n", stTestCtx.test_dir);
			continue;
		}

		LOGOUT("\ntest info:\n");
		LOGOUT("test_dir = %s\n", stTestCtx.test_dir);

		value = strstr(start, _SCRIPT_OUTPUT_DIR);

		if (value != NULL && value < end) {
			value = get_str_value(start, _SCRIPT_OUTPUT_DIR);
			stTestCtx.output_dir = (char *) calloc(strlen(value) + 1, 1);
			strncpy(stTestCtx.output_dir, value, strlen(value));

			if (access(stTestCtx.output_dir, F_OK) != 0) {
				ERROR_IF(mkdir(stTestCtx.output_dir, 0755) != 0);
			}

			LOGOUT("test_output_dir = %s\n", stTestCtx.output_dir);

		} else {
			stTestCtx.output_dir = (char *) calloc(strlen(stTestCtx.test_dir) + 1, 1);
			strncpy(stTestCtx.output_dir, stTestCtx.test_dir, strlen(stTestCtx.test_dir));
		}

		value = strstr(start, _SCRIPT_PQ_BIN);

		if (value != NULL && value < end) {
			value = get_str_value(start, _SCRIPT_PQ_BIN);
			stTestCtx.pq_bin_path = (char *) calloc(strlen(value) + 1, 1);
			strncpy(stTestCtx.pq_bin_path, value, strlen(value));

			LOGOUT("test_pq_bin = %s\n", stTestCtx.pq_bin_path);

		} else {
			stTestCtx.pq_bin_path = NULL;
		}

		value = strstr(start, _SCRIPT_ROI);

		if (value != NULL && value < end) {
			value = get_str_value(start, _SCRIPT_ROI);
			stTestCtx.roi_info = (char *) calloc(strlen(value) + 1, 1);
			strncpy(stTestCtx.roi_info, value, strlen(value));

			LOGOUT("test_roi = %s\n", stTestCtx.roi_info);

		} else {
			stTestCtx.roi_info = NULL;
		}

		value = strstr(start, _SCRIPT_SNS_CFG);

		if (value != NULL && value < end) {
			value = get_str_value(start, _SCRIPT_SNS_CFG);
			stTestCtx.sensor_cfg_path = (char *) calloc(strlen(value) + 1, 1);
			strncpy(stTestCtx.sensor_cfg_path, value, strlen(value));
		} else {
			stTestCtx.sensor_cfg_path = NULL;
		}

		value = strstr(start, _SCRIPT_TEST_MD5);

		if (value != NULL && value < end) {
			value = get_str_value(start, _SCRIPT_TEST_MD5);
			if (atoi(value) != 0) {
				stTestCtx.bEnableMd5Test = true;
			} else {
				stTestCtx.bEnableMd5Test = false;
			}
		} else {
			stTestCtx.bEnableMd5Test = false;
		}

		run_test();
	}

fail:

	if (fp != NULL) {
		fclose(fp);
		fp = NULL;
	}

	if (script != NULL) {
		free(script);
		script = NULL;
	}
}

void printfScriptTemplate(void)
{
	printf("\r\n\r\n");
	printf("./raw_replay_test script_file\n");
	printf("\teg: ./raw_replay_test raw_replay_test_script.txt\n\n");
	printf("script content template:\n");
	printf("%s\n", _SCRIPT_START);
	printf("%s = /mnt/sd/raw_data\n", _SCRIPT_TEST_DIR);
	printf("%s = /mnt/sd/raw_data/cvi_sdr_01.bin\n", _SCRIPT_PQ_BIN);
	printf("%s = 0,0,960,540\n", _SCRIPT_ROI);
	printf("%s = /mnt/sd/raw_data/output_01\n", _SCRIPT_OUTPUT_DIR);
	printf("%s\n", _SCRIPT_END);
	printf("\n");
	printf("%s\n", _SCRIPT_START);
	printf("%s = /mnt/sd/raw_data\n", _SCRIPT_TEST_DIR);
	printf("%s = /mnt/sd/raw_data/cvi_sdr_02.bin\n", _SCRIPT_PQ_BIN);
	printf("%s = /mnt/sd/raw_data/output_02\n", _SCRIPT_OUTPUT_DIR);
	printf("%s\n", _SCRIPT_END);
	printf("\r\n\r\n");
}

int raw_replay_test_main(int argc, char **argv)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	if (argc != 2) {
		LOGOUT("ERROR:\n");
		printfScriptTemplate();
		return CVI_FAILURE;
	}

	if (access(argv[1], F_OK) != 0) {
		LOGOUT("ERROR:\n");
		printfScriptTemplate();
		return CVI_FAILURE;
	}

	bEnableVencThread = true;
	bEnableDumpYuv = false;
	bEnableDumpStream = false;

	memset(&stTestCtx, 0, sizeof(RAW_REPLAY_TEST_CTX_S));

	LOGOUT("init sys vi...\n");

	s32Ret = sys_vi_init();
	RETURN_FAILURE_IF(s32Ret != CVI_SUCCESS);

	LOGOUT("init vpss...\n");

	s32Ret = vpss_init();
	RETURN_FAILURE_IF(s32Ret != CVI_SUCCESS);

	LOGOUT("init venc...\n");

	s32Ret = venc_init();
	RETURN_FAILURE_IF(s32Ret != CVI_SUCCESS);

	LOGOUT("start venc thread...\n");

	s32Ret = pthread_create(&vencThreadTid, NULL, venc_thread, NULL);
	RETURN_FAILURE_IF(s32Ret != 0x00);

	LOGOUT("wait venc thread ready...\n");

	while (u32VencWorkFrameCount < 5) {
		usleep(500 * 1000);
	}

	run_test_script(argv[1]);

	bEnableVencThread = false;

	pthread_join(vencThreadTid, NULL);

	LOGOUT("\n\ntest finish ...\n\n");

	if (stTestCtx.enMd5TestStatus == MD5_TEST_SUCCEED) {
		LOGOUT("\n\n\t MD5 TEST SUCCESS\n\n");
	} else if (stTestCtx.enMd5TestStatus == MD5_TEST_FAIL) {
		LOGOUT("\n\n\t MD5 TEST FAIL\n\n");
	}

	s32Ret = venc_deinit();
	RETURN_FAILURE_IF(s32Ret != CVI_SUCCESS);

	s32Ret = vpss_deinit();
	RETURN_FAILURE_IF(s32Ret != CVI_SUCCESS);

	s32Ret = sys_vi_deinit();
	RETURN_FAILURE_IF(s32Ret != CVI_SUCCESS);

	return s32Ret;
}

#endif

