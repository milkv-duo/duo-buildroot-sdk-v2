#ifndef _CVI_AUDIO_LOG_H_
#define _CVI_AUDIO_LOG_H_



#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include "cvi_debug.h"
#include "cvi_audio.h"

#define CVI_AUD_MASK_ERR	(0x00)
#define CVI_AUD_MASK_INFO	(0x01)
#define CVI_AUD_MASK_DBG	(0x02)

extern CVI_S32 cviaud_dbg;

static	inline int	_cviAudGetEnv(char const *env, char const  *fmt, void *param)
{
#define NOT_SET -2
#define INPUT_ERR -1
	char *pEnv;
	int val = NOT_SET;

	pEnv = getenv(env);

	if (pEnv) {
		if (strcmp(fmt, "%s") == 0) {
			strcpy((char *)param, pEnv);
		} else {
			if (sscanf(pEnv, fmt, &val) != 1)
				return INPUT_ERR;
		}
	}

	return val;
}

static	inline void cviAudioGetDbgMask(CVI_S32 *debug_level)
{
#define NOT_SET -2
#define INPUT_ERR -1
	CVI_S32  *pAUD_level = debug_level;
	int getValue;
	char const  *str = "cviaudio_level";
	char const  *fmt = "%d";

	getValue = _cviAudGetEnv(str, fmt, NULL);

	if (getValue == NOT_SET || getValue == INPUT_ERR) {
		*pAUD_level  = CVI_AUD_MASK_ERR;
	} else
		*pAUD_level = getValue;
}

static	inline void cviAudioGetCliMask(CVI_S32 *debug_level)
{
#define NOT_SET -2
#define INPUT_ERR -1
	CVI_S32  *pAUD_level = debug_level;
	int getValue;
	char const  *str = "audcli_level";
	char const  *fmt = "%d";

	getValue = _cviAudGetEnv(str, fmt, NULL);

	if (getValue == NOT_SET || getValue == INPUT_ERR) {
		*pAUD_level  = CVI_AUD_MASK_ERR;
	} else
		*pAUD_level = getValue;
}

#ifdef AUDIO_PRINT_WITH_GLOBAL_COMM_LOG  /* use global module id as keyword */

#define log_error(msg, ...)		\
	do { \
		if (cviaud_dbg >= CVI_AUD_MASK_ERR) {	\
			CVI_TRACE(CVI_DBG_ERR, CVI_ID_AUD, "%s:<%s,%d> "msg, \
			__FILENAME__, __func__, __LINE__, ##__VA_ARGS__);\
			} \
	} while (0)
#define log_warn(msg, ...)		\
	do { \
		if (cviaud_dbg >= CVI_AUD_MASK_ERR) { \
			CVI_TRACE(CVI_DBG_WARN, CVI_ID_AUD, "%s:<%s,%d> "msg, \
			__FILENAME__, __func__, __LINE__, ##__VA_ARGS__);\
			} \
	} while (0)

#define log_info(msg, ...)		\
	do { \
		if (cviaud_dbg >= CVI_AUD_MASK_INFO) \
			CVI_TRACE(CVI_DBG_INFO, CVI_ID_AUD, "%s:<%s,%d> "msg, \
			__FILENAME__, __func__, __LINE__, ##__VA_ARGS__); \
	} while (0)
#define log_debug(msg, ...)		\
	do { \
		if (cviaud_dbg >= CVI_AUD_MASK_DBG) \
			CVI_TRACE(CVI_DBG_DEBUG, CVI_ID_AUD, "%s:<%s,%d> "msg, \
			__FILENAME__, __func__, __LINE__, ##__VA_ARGS__); \
	} while (0)


#else  /*  use [cviaudio] key word  as printf macro*/

#define log_error(fmt, args...) \
	do { \
		if (cviaud_dbg >= CVI_AUD_MASK_ERR) \
			printf("[error]<%s,%d> "fmt, __func__, __LINE__, ##args);\
	} while (0)

#define log_warn(fmt, args...) \
	do { \
		if (cviaud_dbg >= CVI_AUD_MASK_ERR) \
			printf("[warn]<%s,%d> "fmt, __func__, __LINE__, ##args);\
	} while (0)

#define log_info(fmt, args...) \
	do { \
		if (cviaud_dbg >= CVI_AUD_MASK_INFO) \
			printf("[info]<%s,%d> "fmt, __func__, __LINE__, ##args);\
	} while (0)

#define log_debug(fmt, args...) \
	do { \
		if (cviaud_dbg >= CVI_AUD_MASK_DBG) \
			printf("[debug]<%s,%d> "fmt, __func__, __LINE__, ##args);\
	} while (0)

#endif


#ifdef __cplusplus
}
#endif


#endif //_CVI_AUDIO_LOG_H_
