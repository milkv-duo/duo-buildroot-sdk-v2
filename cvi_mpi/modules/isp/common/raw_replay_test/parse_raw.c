
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
#include "cvi_sys_base.h"
#include "cvi_buffer.h"
#include "cvi_vb.h"
#include "cvi_sys.h"
#include "cvi_defines.h"

#include "parse_raw.h"

#include "dpcm_api.h"

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

#define ENABLE_PARSE_RAW_DEBUG 1

#define PATH_MAX_LEN   512
#define FILE_LIST_MAX  256

static char rootPath[PATH_MAX_LEN];
static char pathPrefix[PATH_MAX_LEN];

static char *fileList[FILE_LIST_MAX];
static CVI_U32 u32FileListTotal;

static RAW_REPLAY_INFO stRawInfo;

static CVI_S16 s16LECropOffsetLeft;
static CVI_S16 s16LECropOffsetTop;
static CVI_U32 u32LECropWidth;
static CVI_U32 u32LECropHeight;

static CVI_S16 s16SECropOffsetLeft;
static CVI_S16 s16SECropOffsetTop;
static CVI_U32 u32SECropWidth;
static CVI_U32 u32SECropHeight;

static CVI_U8 *pFullSizeFrameData;
static CVI_U8 *pRoiFrameData;

static char *pInputRoiInfo;
static RECT_S stOriginalRoiRect;

static CVI_U32 u32AllocVbBlkCnt;

CVI_S32 parse_raw_init(const char *path, const char *roi_info, CVI_U32 *pu32TotalItem)
{
	CVI_S32 ret = CVI_SUCCESS;

	RETURN_FAILURE_IF(path == NULL);
	RETURN_FAILURE_IF(pu32TotalItem == NULL);

	DIR *dir = NULL;
	struct dirent *ptr = NULL;

	dir = opendir(path);
	RETURN_FAILURE_IF(dir == NULL);

	u32FileListTotal = 0;

	while ((ptr = readdir(dir)) != NULL &&
		u32FileListTotal < FILE_LIST_MAX) {

		// file
		if (ptr->d_type == 8) {

			int len = strlen(ptr->d_name);

			if (strcmp(ptr->d_name + (len - 4), ".raw") != 0) {
				continue;
			}

			if (strstr(ptr->d_name, "roi") == NULL) {

				fileList[u32FileListTotal] = (char *) calloc(len + 1, 1);

				snprintf(fileList[u32FileListTotal], len + 1, "%s", ptr->d_name);

				u32FileListTotal++;
			} else {
				continue;
			}
		}
	}

	snprintf(rootPath, PATH_MAX_LEN, "%s", path);
	*pu32TotalItem = u32FileListTotal;

	memset(&stRawInfo, 0, sizeof(RAW_REPLAY_INFO));
	memset(&stOriginalRoiRect, 0, sizeof(RECT_S));

	pFullSizeFrameData = NULL;
	pRoiFrameData = NULL;

#if ENABLE_PARSE_RAW_DEBUG
	LOGOUT("file list: %d\n", u32FileListTotal);
	for (CVI_U32 i = 0; i < u32FileListTotal; i++) {
		LOGOUT("%s\n", fileList[i]);
	}
#endif

	u32AllocVbBlkCnt = 0;

	pInputRoiInfo = (char *) roi_info;

	if (pInputRoiInfo != NULL) {

		char *ptemp = pInputRoiInfo;

		stRawInfo.stRoiRect.s32X = atoi(ptemp);
		stRawInfo.stRoiRect.s32X &= (~0x01);

		ptemp = strchr(ptemp, ','); ptemp++;
		GOTO_FAIL_IF(ptemp == NULL, parse_roi_info_fail);
		stRawInfo.stRoiRect.s32Y = atoi(ptemp);

		ptemp = strchr(ptemp, ','); ptemp++;
		GOTO_FAIL_IF(ptemp == NULL, parse_roi_info_fail);
		stRawInfo.stRoiRect.u32Width = atoi(ptemp);
		stRawInfo.stRoiRect.u32Width &= (~0x01);

		ptemp = strchr(ptemp, ','); ptemp++;
		GOTO_FAIL_IF(ptemp == NULL, parse_roi_info_fail);
		stRawInfo.stRoiRect.u32Height = atoi(ptemp);

		LOGOUT("input roi info: %d,%d,%d,%d\n",
			stRawInfo.stRoiRect.s32X,
			stRawInfo.stRoiRect.s32Y,
			stRawInfo.stRoiRect.u32Width,
			stRawInfo.stRoiRect.u32Height);
	}

	return ret;

parse_roi_info_fail:
	pInputRoiInfo = NULL;

	return ret;
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

	temp = strchr(temp, ' ');
	GOTO_FAIL_IF(temp == NULL, fail);

	temp++;

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

static CVI_S64 get_ion_size_info(const char *path)
{
	FILE *fd = NULL;
	char cmd[256] = {0};

	snprintf(cmd, 256, "cat %s", path);

	fd = popen(cmd, "r");

	if (fd == NULL) {
		LOGOUT("fail, popen: %s\n", cmd);
		return 0;
	}

	memset(cmd, 0, 128);

	fgets(cmd, 128, fd);

	pclose(fd);

	return atol(cmd);
}

static CVI_U32 get_raw_replay_vb_size(void)
{
	FILE *fd = NULL;
	char cmd[256] = {0};
	CVI_U8 status = 0;
	char *ptemp = NULL;

	snprintf(cmd, 256, "cat %s", "/proc/cvitek/vb");

	fd = popen(cmd, "r");

	if (fd == NULL) {
		LOGOUT("fail, popen: %s\n", cmd);
		return 0;
	}

	memset(cmd, 0, sizeof(cmd));

	while (fgets(cmd, sizeof(cmd), fd) != NULL) {
		if (status == 0) {
			if (strstr(cmd, "raw_replay_vb") != NULL) {
				status = 1;
			}
		} else if (status == 1) {
			if (strstr(cmd, "BlkSz") != NULL) {
				status = 2;
				break;
			}
		}
	}

	pclose(fd);

	if (status == 2) {
		ptemp = strrchr(cmd, ' ');
		ptemp++;
		return atol(ptemp);
	} else {
		return 0;
	}
}

static CVI_U32 calculate_blk_cnt(CVI_U32 u32FullBlkSize, CVI_U32 u32RoiBlkSize)
{
	CVI_S64 total_mem = 0;
	CVI_S64 alloc_mem = 0;
	CVI_U32 raw_repaly_vb_size = 0;

	//system("cat /sys/kernel/debug/ion/cvi_carveout_heap_dump/total_mem");
	//system("cat /sys/kernel/debug/ion/cvi_carveout_heap_dump/alloc_mem");
	//system("cat /sys/kernel/debug/ion/cvi_carveout_heap_dump/peak");
	//system("cat /sys/kernel/debug/ion/cvi_carveout_heap_dump/summary");
	//system("cat /proc/cvitek/vb");

	if (u32AllocVbBlkCnt != 0) {
		return u32AllocVbBlkCnt;
	}

	total_mem = get_ion_size_info("/sys/kernel/debug/ion/cvi_carveout_heap_dump/total_mem");
	alloc_mem = get_ion_size_info("/sys/kernel/debug/ion/cvi_carveout_heap_dump/alloc_mem");

	LOGOUT("ion info, total_mem: %lld, alloc_mem: %lld, blkSize: %d\n",
		(long long int) total_mem, (long long int) alloc_mem, u32RoiBlkSize);

	raw_repaly_vb_size = get_raw_replay_vb_size();

	LOGOUT("raw replay vb size: %d\n", raw_repaly_vb_size);

	u32AllocVbBlkCnt = (total_mem - alloc_mem - u32FullBlkSize + raw_repaly_vb_size) / u32RoiBlkSize;

	u32AllocVbBlkCnt += 1; // u32FullBlkSize;

	if (u32AllocVbBlkCnt > VB_POOL_MAX_BLK) {
		u32AllocVbBlkCnt = VB_POOL_MAX_BLK;
	}

	if (u32AllocVbBlkCnt > 2) {
		u32AllocVbBlkCnt -= 1;
	}

	u32AllocVbBlkCnt = u32AllocVbBlkCnt & (~0x01);

	return u32AllocVbBlkCnt;
}

static CVI_S32 parse_raw_info(const char *prefix)
{
	FILE *fp = NULL;
	char *src = NULL;
	char *value = NULL;
	struct stat statbuf;
	char path[PATH_MAX_LEN] = {0};

	LOGOUT("raw info:\n");

	snprintf(path, PATH_MAX_LEN, "%s.txt", prefix);

	fp = fopen(path, "rb");
	GOTO_FAIL_IF(fp == NULL, fail);

	stat(path, &statbuf);

	src = (char *) calloc(statbuf.st_size + 1, 1);
	GOTO_FAIL_IF(src == NULL, fail);

	fread(src, statbuf.st_size, 1, fp);

	fclose(fp);
	fp = NULL;

	value = get_str_value(src, "ISO");

	if (value != NULL) {
		stRawInfo.ISO = atoi(value);
		LOGOUT("ISO = %d\n", stRawInfo.ISO);
	}

	value = get_str_value(src, "Light Value");

	if (value != NULL) {
		stRawInfo.lightValue = atof(value);
		LOGOUT("Light value = %f\n", stRawInfo.lightValue);
	}

	value = get_str_value(src, "Color Temp.");

	if (value != NULL) {
		stRawInfo.colorTemp = atoi(value);
		LOGOUT("Color Temp. = %d\n", stRawInfo.colorTemp);
	}

	value = get_str_value(src, "ISP DGain");

	if (value != NULL) {
		stRawInfo.ispDGain = atoi(value);
		LOGOUT("ISP DGain = %d\n", stRawInfo.ispDGain);
	}

	value = get_str_value(src, "Exposure Time");

	if (value != NULL) {
		stRawInfo.longExposure = atoi(value);
		LOGOUT("Exposure Time = %d\n", stRawInfo.longExposure);
	}

	value = get_str_value(src, "Short Exposure");

	if (value != NULL) {
		stRawInfo.shortExposure = atoi(value);
		LOGOUT("Short Exposure = %d\n", stRawInfo.shortExposure);
	}

	value = get_str_value(src, "Exposure Ratio");

	if (value != NULL) {
		stRawInfo.exposureRatio = atoi(value);
		LOGOUT("Exposure Ratio = %d\n", stRawInfo.exposureRatio);
	}

	value = get_str_value(src, "Exposure AGain");

	if (value != NULL) {
		stRawInfo.exposureAGain = atoi(value);
		LOGOUT("Exposure AGain = %d\n", stRawInfo.exposureAGain);
	}

	value = get_str_value(src, "Exposure DGain");

	if (value != NULL) {
		stRawInfo.exposureDGain = atoi(value);
		LOGOUT("Exposure DGain = %d\n", stRawInfo.exposureDGain);
	}

	value = get_str_value(src, "reg_wbg_rgain");

	if (value != NULL) {
		stRawInfo.WB_RGain = atoi(value);
		LOGOUT("reg_wbg_rgain = %d\n", stRawInfo.WB_RGain);
	}

	value = get_str_value(src, "reg_wbg_bgain");

	if (value != NULL) {
		stRawInfo.WB_BGain = atoi(value);
		LOGOUT("reg_wbg_bgain = %d\n", stRawInfo.WB_BGain);
	}

	value = get_str_value(src, "LE crop");

	if (value != NULL) {

		char *temp = value;

		s16LECropOffsetLeft = atoi(temp);

		temp = strchr(temp, ',');
		temp++;
		s16LECropOffsetTop = atoi(temp);

		temp = strchr(temp, ',');
		temp++;
		u32LECropWidth = atoi(temp);

		temp = strchr(temp, ',');
		temp++;
		u32LECropHeight = atoi(temp);

		LOGOUT("LE crop = %d,%d,%d,%d\n",
			s16LECropOffsetLeft,
			s16LECropOffsetTop,
			u32LECropWidth,
			u32LECropHeight);
	}

	value = get_str_value(src, "SE crop");

	if (value != NULL) {

		char *temp = value;

		s16SECropOffsetLeft = atoi(temp);

		temp = strchr(temp, ',');
		temp++;
		s16SECropOffsetTop = atoi(temp);

		temp = strchr(temp, ',');
		temp++;
		u32SECropWidth = atoi(temp);

		temp = strchr(temp, ',');
		temp++;
		u32SECropHeight = atoi(temp);

		LOGOUT("SE crop = %d,%d,%d,%d\n",
			s16SECropOffsetLeft,
			s16SECropOffsetTop,
			u32SECropWidth,
			u32SECropHeight);
	}

	value = get_str_value(src, "roi");

	if (value != NULL) {

		char *temp = value;

		stRawInfo.roiFrameNum = atoi(temp);

		temp = strchr(temp, ',');
		temp++;
		stOriginalRoiRect.s32X = atoi(temp);

		temp = strchr(temp, ',');
		temp++;
		stOriginalRoiRect.s32Y = atoi(temp);

		temp = strchr(temp, ',');
		temp++;
		stOriginalRoiRect.u32Width = atoi(temp);

		temp = strchr(temp, ',');
		temp++;
		stOriginalRoiRect.u32Height = atoi(temp);

		LOGOUT("original roi = %d,%d,%d,%d,%d\n",
			stRawInfo.roiFrameNum,
			stOriginalRoiRect.s32X,
			stOriginalRoiRect.s32Y,
			stOriginalRoiRect.u32Width,
			stOriginalRoiRect.u32Height);

		if (pInputRoiInfo == NULL) {
			stRawInfo.stRoiRect = stOriginalRoiRect;
		} else {
			CVI_U32 w, h;

			w = stRawInfo.stRoiRect.s32X + stRawInfo.stRoiRect.u32Width;
			h = stRawInfo.stRoiRect.s32Y + stRawInfo.stRoiRect.u32Height;

			if (w > stOriginalRoiRect.u32Width ||
				h > stOriginalRoiRect.u32Height) {
				LOGOUT("ERROR: invalid roi rect, use original roi rect...\n");
				stRawInfo.stRoiRect = stOriginalRoiRect;
			}
		}

	} else {
		stRawInfo.roiFrameNum = 0;
	}

	value = strstr(path, "WDR");

	if (value != NULL) {
		stRawInfo.enWDR = 1;
	} else {
		stRawInfo.enWDR = 0;
	}

	value = strrchr(path, '/');
	value++;
	stRawInfo.width = atoi(value);

	//if (stRawInfo.enWDR != 0) {
	//	stRawInfo.width = stRawInfo.width / 2;
	//}

	value = strchr(path, 'X');
	value++;
	stRawInfo.height = atoi(value);

	value = strstr(path, "color");
	value = strchr(value, '=');
	value++;
	stRawInfo.bayerID = atoi(value);

	stRawInfo.numFrame = stRawInfo.roiFrameNum + 1;

	stRawInfo.size = stRawInfo.width * 3 / 2;
	stRawInfo.size = stRawInfo.size * stRawInfo.height;

	//if (stRawInfo.enWDR != 0) {
	//	stRawInfo.size = stRawInfo.size * 2;
	//}

	stRawInfo.roiFrameSize = stRawInfo.stRoiRect.u32Width * 3 / 2;
	stRawInfo.roiFrameSize = stRawInfo.roiFrameSize * stRawInfo.stRoiRect.u32Height;

	if (stRawInfo.enWDR != 0) {
		stRawInfo.roiFrameSize = stRawInfo.roiFrameSize * 2;
	}

	if (pFullSizeFrameData != NULL) {
		free(pFullSizeFrameData);
		pFullSizeFrameData = NULL;
	}

	pFullSizeFrameData = calloc(stRawInfo.size, 1);
	ERROR_IF(pFullSizeFrameData == NULL);

	if (pRoiFrameData != NULL) {
		free(pRoiFrameData);
		pRoiFrameData = NULL;
	}

	if (stRawInfo.roiFrameSize > 0) {
		pRoiFrameData = calloc(stRawInfo.roiFrameSize, 1);
		ERROR_IF(pRoiFrameData == NULL);
	}

	LOGOUT("%dX%d, numFrame=%d, WDR=%d, color=%d, size=%d, roi_size=%d\n",
		stRawInfo.width,
		stRawInfo.height,
		stRawInfo.numFrame,
		stRawInfo.enWDR,
		stRawInfo.bayerID,
		stRawInfo.size,
		stRawInfo.roiFrameSize);

	if (stRawInfo.roiFrameSize != 0 && stRawInfo.roiFrameNum != 0) {

		CVI_S32 tempVbBlkCnt = calculate_blk_cnt(stRawInfo.size, stRawInfo.roiFrameSize);

		LOGOUT("calculate_vb_blk_cnt: %d\n", tempVbBlkCnt);

		GOTO_FAIL_IF(tempVbBlkCnt < 2, fail);

		if (stRawInfo.numFrame > tempVbBlkCnt) {
			stRawInfo.numFrame = tempVbBlkCnt;
			LOGOUT("\n\nWARN: ion too samle, reduce total frame: %d\n\n", stRawInfo.numFrame);
		}
	}

	free(src);
	src = NULL;

	return CVI_SUCCESS;

fail:

	if (fp != NULL) {
		fclose(fp);
		fp = NULL;
	}

	if (src != NULL) {
		free(src);
		src = NULL;
	}

	return CVI_FAILURE;
}

CVI_S32 get_raw_data_info(CVI_U32 u32ItemIndex, CVI_U32 *pu32TotalFrame, char **prefix)
{
	CVI_S32 ret = CVI_SUCCESS;

	RETURN_FAILURE_IF(u32ItemIndex > u32FileListTotal);
	RETURN_FAILURE_IF(pu32TotalFrame == NULL);
	RETURN_FAILURE_IF(prefix == NULL);

	snprintf(pathPrefix, PATH_MAX_LEN, "%s/%s", rootPath, fileList[u32ItemIndex]);

	LOGOUT("parse %s ...\n", pathPrefix);

	pathPrefix[strlen(pathPrefix) - 4] = '\0';

	*prefix = pathPrefix;

	stRawInfo.isValid = 1;

	ret = parse_raw_info(pathPrefix);

	*pu32TotalFrame = stRawInfo.numFrame;

	return ret;
}

static void crop_image(uint32_t ofs_x, uint32_t ofs_y, uint32_t stride_src, uint32_t width, uint32_t height,
		uint8_t *src, uint8_t *dst)
{
	uint8_t *addr_src = src + ofs_y * stride_src + ofs_x * 2;
	uint8_t *addr_dst = dst;

	for (uint32_t i = 0 ; i < height ; i++) {
		memcpy(addr_dst, addr_src, width * 2);
		addr_src += stride_src;
		addr_dst += width * 2;
	}
}

static void unpacked2packed(CVI_U8 *unpacked, CVI_U8 *packed, CVI_U32 width, CVI_U32 height)
{
	CVI_U16 *pbuffer = (CVI_U16 *) unpacked;
	CVI_U32 size = width * height / 2;

	CVI_U16 s0 = 0;
	CVI_U16 s1 = 0;

	for (CVI_U32 i = 0; i < size; i++) {
		s0 = *pbuffer++;
		s1 = *pbuffer++;
		*packed++ = (CVI_U8) (s0 >> 4);
		*packed++ = (CVI_U8) (s1 >> 4);
		*packed++ = (CVI_U8) (((s1 & 0x0F) << 4) | (s0 & 0x0F));
	}
}

static CVI_S32 parse_full_size_raw(void)
{
	CVI_S32 ret = CVI_SUCCESS;

	FILE *fp = NULL;
	struct stat statbuf;
	char path[PATH_MAX_LEN] = {0};

	CVI_U8 *src = NULL;
	CVI_U8 *dst = NULL;
	CVI_U8 *crop = NULL;

	snprintf(path, PATH_MAX_LEN, "%s.raw", pathPrefix);

	fp = fopen(path, "rb");
	GOTO_FAIL_IF(fp == NULL, fail);

	stat(path, &statbuf);

	src = (CVI_U8 *) malloc(statbuf.st_size);
	GOTO_FAIL_IF(src == NULL, fail);

	fread(src, statbuf.st_size, 1, fp);

	fclose(fp);
	fp = NULL;

	if (statbuf.st_size == stRawInfo.size) {
		/* original raw */
		memcpy(pFullSizeFrameData, src, stRawInfo.size);
		goto finish;
	} else if (statbuf.st_size == (stRawInfo.width * stRawInfo.height * 2)) {
		/* unpacked raw */
		unpacked2packed(src, pFullSizeFrameData, stRawInfo.width, stRawInfo.height);
		goto finish;
	}

	/* compress raw */

	RAW_INFO rawinfo;

	rawinfo.buffer = src;
	rawinfo.width = u32LECropWidth;
	rawinfo.height = u32LECropHeight;

	if (stRawInfo.enWDR != 0) {
		rawinfo.stride = statbuf.st_size / 2 / rawinfo.height;
	} else {
		rawinfo.stride = statbuf.st_size / rawinfo.height;
	}

	CVI_U32 unpackedSize = rawinfo.width * rawinfo.height * 2;

	if (stRawInfo.enWDR != 0) {
		unpackedSize = unpackedSize * 2;
	}

	dst = (CVI_U8 *) malloc(unpackedSize);
	GOTO_FAIL_IF(dst == NULL, fail);

	decoderRaw(rawinfo, dst);

	if (stRawInfo.enWDR != 0) {
		rawinfo.buffer = src + statbuf.st_size / 2;
		decoderRaw(rawinfo, dst + unpackedSize / 2);
	}

	if (s16LECropOffsetLeft != 0 || s16LECropOffsetTop != 0) {
		CVI_U32 unpackedSize = stRawInfo.width * stRawInfo.height * 2;

		crop = (CVI_U8 *) malloc(unpackedSize);
		GOTO_FAIL_IF(crop == NULL, fail);

		crop_image(s16LECropOffsetLeft,
			s16LECropOffsetTop,
			u32LECropWidth * 2,
			stRawInfo.enWDR != 0 ? stRawInfo.width / 2 : stRawInfo.width,
			stRawInfo.height,
			dst,
			crop);

		if (stRawInfo.enWDR != 0) {

			crop_image(s16LECropOffsetLeft,
				s16LECropOffsetTop,
				u32LECropWidth * 2,
				stRawInfo.enWDR != 0 ? stRawInfo.width / 2 : stRawInfo.width,
				stRawInfo.height,
				dst + (rawinfo.width * rawinfo.height * 2),
				crop + unpackedSize / 2);
		}

		free(dst);

		dst = crop;
		crop = NULL;
	}

	if (stRawInfo.enWDR != 0) {

		CVI_U32 unpackedSize = stRawInfo.width * stRawInfo.height * 2;

		CVI_U8 *data = (CVI_U8 *) malloc(unpackedSize);

		GOTO_FAIL_IF(data == NULL, fail);

		CVI_U8 *le = dst;
		CVI_U8 *se = dst + unpackedSize / 2;
		CVI_U32 stride = stRawInfo.width;

		CVI_U8 *temp = data;

		for (CVI_S32 i = 0; i < stRawInfo.height; i++) {
			memcpy(temp, le, stride);
			temp += stride;
			le += stride;

			memcpy(temp, se, stride);
			temp += stride;
			se += stride;
		}

		free(dst);
		dst = data;
		data = NULL;
	}

	unpacked2packed(dst, pFullSizeFrameData, stRawInfo.width, stRawInfo.height);

finish:

	if (src != NULL) {
		free(src);
		src = NULL;
	}

	if (dst != NULL) {
		free(dst);
		dst = NULL;
	}

	if (crop != NULL) {
		free(crop);
		crop = NULL;
	}

	return ret;

fail:

	if (fp != NULL) {
		fclose(fp);
		fp = NULL;
	}

	if (src != NULL) {
		free(src);
		src = NULL;
	}

	if (dst != NULL) {
		free(dst);
		dst = NULL;
	}

	if (crop != NULL) {
		free(crop);
		crop = NULL;
	}

	return CVI_FAILURE;
}

static CVI_S32 parse_roi_raw(CVI_U32 u32FrameIndex)
{
	CVI_S32 ret = CVI_SUCCESS;

	FILE *fp = NULL;
	struct stat statbuf;
	char path[PATH_MAX_LEN] = {0};
	CVI_U32 rawSize = 0;

	CVI_U8 *src = NULL;
	CVI_U8 *dst = NULL;
	CVI_U8 *crop = NULL;

	snprintf(path, PATH_MAX_LEN, "%s_roi=%d,%d,%d,%d,%d.raw",
		pathPrefix,
		stRawInfo.roiFrameNum,
		stOriginalRoiRect.s32X,
		stOriginalRoiRect.s32Y,
		stOriginalRoiRect.u32Width,
		stOriginalRoiRect.u32Height);

	fp = fopen(path, "rb");
	GOTO_FAIL_IF(fp == NULL, fail);

	stat(path, &statbuf);

	rawSize = statbuf.st_size / stRawInfo.roiFrameNum;

	src = (CVI_U8 *) malloc(rawSize);
	GOTO_FAIL_IF(src == NULL, fail);

	fseek(fp, rawSize * u32FrameIndex, SEEK_SET);
	fread(src, rawSize, 1, fp);

	fclose(fp);
	fp = NULL;

	RAW_INFO rawinfo;

	rawinfo.buffer = src;
	rawinfo.width = stOriginalRoiRect.u32Width;
	rawinfo.height = stOriginalRoiRect.u32Height;

	if (stRawInfo.enWDR != 0) {
		rawinfo.stride = rawSize / 2 / rawinfo.height;
	} else {
		rawinfo.stride = rawSize / rawinfo.height;
	}

	CVI_U32 unpackedSize = rawinfo.width * rawinfo.height * 2;

	if (stRawInfo.enWDR != 0) {
		unpackedSize = unpackedSize * 2;
	}

	dst = (CVI_U8 *) malloc(unpackedSize);
	GOTO_FAIL_IF(dst == NULL, fail);

	CVI_BOOL bIsTileMode = CVI_FALSE;

#if defined(__CV181X__) || defined(__CV180X__)
	bIsTileMode = CVI_FALSE;
#else
#error "should check here when porting new chip"
#endif

	decoderRawExtend(rawinfo, dst, bIsTileMode);

	if (stRawInfo.enWDR != 0) {
		rawinfo.buffer = src + rawSize / 2;
		decoderRawExtend(rawinfo, dst + unpackedSize / 2, bIsTileMode);
	}

	if (pInputRoiInfo != NULL) {  // crop roi
		CVI_U32 unpackedSize = stRawInfo.stRoiRect.u32Width *
									stRawInfo.stRoiRect.u32Height * 2;

		if (stRawInfo.enWDR != 0) {
			unpackedSize = unpackedSize * 2;
		}

		crop = (CVI_U8 *) malloc(unpackedSize);
		GOTO_FAIL_IF(crop == NULL, fail);

		crop_image(stRawInfo.stRoiRect.s32X,
			stRawInfo.stRoiRect.s32Y,
			stOriginalRoiRect.u32Width * 2,
			stRawInfo.stRoiRect.u32Width,
			stRawInfo.stRoiRect.u32Height,
			dst,
			crop);

		if (stRawInfo.enWDR != 0) {

			crop_image(stRawInfo.stRoiRect.s32X,
				stRawInfo.stRoiRect.s32Y,
				stOriginalRoiRect.u32Width * 2,
				stRawInfo.stRoiRect.u32Width,
				stRawInfo.stRoiRect.u32Height,
				dst + (stOriginalRoiRect.u32Width * stOriginalRoiRect.u32Height * 2),
				crop + unpackedSize / 2);
		}

		free(dst);

		dst = crop;
		crop = NULL;
	}

	if (stRawInfo.enWDR != 0) {

		CVI_U32 unpackedSize = stRawInfo.stRoiRect.u32Width *
									stRawInfo.stRoiRect.u32Height * 2;

		unpackedSize = unpackedSize * 2;

		CVI_U8 *data = (CVI_U8 *) malloc(unpackedSize);

		GOTO_FAIL_IF(data == NULL, fail);

		CVI_U8 *le = dst;
		CVI_U8 *se = dst + unpackedSize / 2;
		CVI_U32 stride = stRawInfo.stRoiRect.u32Width * 2;

		CVI_U8 *temp = data;

		for (CVI_U32 i = 0; i < stRawInfo.stRoiRect.u32Height; i++) {
			memcpy(temp, le, stride);
			temp += stride;
			le += stride;

			memcpy(temp, se, stride);
			temp += stride;
			se += stride;
		}

		free(dst);
		dst = data;
		data = NULL;

		unpacked2packed(dst, pRoiFrameData,
			stRawInfo.stRoiRect.u32Width * 2, stRawInfo.stRoiRect.u32Height);
	} else {
		unpacked2packed(dst, pRoiFrameData,
			stRawInfo.stRoiRect.u32Width, stRawInfo.stRoiRect.u32Height);
	}

	if (src != NULL) {
		free(src);
		src = NULL;
	}

	if (dst != NULL) {
		free(dst);
		dst = NULL;
	}

	if (crop != NULL) {
		free(crop);
		crop = NULL;
	}

	return ret;

fail:

	if (fp != NULL) {
		fclose(fp);
		fp = NULL;
	}

	if (src != NULL) {
		free(src);
		src = NULL;
	}

	if (dst != NULL) {
		free(dst);
		dst = NULL;
	}

	if (crop != NULL) {
		free(crop);
		crop = NULL;
	}

	return CVI_FAILURE;
}

CVI_S32 get_raw_data(CVI_U32 u32FrameIndex, void **pHeader, void **pData, CVI_U32 *pu32RawSize)
{
	CVI_S32 ret = CVI_SUCCESS;

	stRawInfo.curFrame = u32FrameIndex;
	*pHeader = &stRawInfo;

	if (u32FrameIndex == 0) {
		ret = parse_full_size_raw();
		*pData = pFullSizeFrameData;
	} else {
		ret = parse_roi_raw(u32FrameIndex - 1);
		*pData = pRoiFrameData;
	}

	*pu32RawSize = stRawInfo.size;

	return ret;
}

CVI_S32 parse_raw_deinit(void)
{
	CVI_S32 ret = CVI_SUCCESS;

	for (CVI_U32 i = 0; i < u32FileListTotal; i++) {

		if (fileList[i] != NULL) {
			free(fileList[i]);
			fileList[i] = NULL;
		}
	}

	u32FileListTotal = 0;

	if (pFullSizeFrameData != NULL) {
		free(pFullSizeFrameData);
		pFullSizeFrameData = NULL;
	}

	if (pRoiFrameData != NULL) {
		free(pRoiFrameData);
		pRoiFrameData = NULL;
	}

	pInputRoiInfo = NULL;

	return ret;
}

