/*
 *	cvi_audio_uac.h  --  USB Audio Class Gadget API
 *
 */

#ifndef _CVI_AUDIO_UAC_H_
#define _CVI_AUDIO_UAC_H_
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <pthread.h>
#include <unistd.h>

/* ------------------------------------------------------------------------
 * Debugging, printing and logging
 */
//audio codec define------------start
#define ACODEC_ADC	"/dev/cv182xadc"
#define ACODEC_DAC	"/dev/cv182xdac"

//audio codec define--------------end

#define UAC_SRC_MIC_IN_CARD_ID 0
#define UAC_SRC_USB_IN_CARD_ID 2
#define UAC_DST_SPK_OUT_CARD_ID 1
#define UAC_DST_USB_OUT_CARD_ID 2

#define CVI_LITE_AUDIO_INTERFACE
#ifdef CVI_LITE_AUDIO_INTERFACE

#ifndef CVI_SUCCESS
#define CVI_SUCCESS             0
#endif

#ifndef CVI_TRUE
#define CVI_TRUE                1
#endif

#ifndef CVI_FALSE
#define CVI_FALSE               0
#endif


#ifndef CVI_FAILURE
#define CVI_FAILURE             (-1)
#endif

#ifndef CVI_FAILURE_ILLEGAL_PARAM
#define CVI_FAILURE_ILLEGAL_PARAM (-2)
#endif

#ifndef CVI_FAILURE_NOMEM
#define CVI_FAILURE_NOMEM (-3)
#endif

typedef enum _E_CVIAUDIO_UAC_CODETYPE {
	CVIAUDIO_UAC_TYPE_PCM = 0,
	CVIAUDIO_UAC_TYPE_MP3 = 1,
	CVIAUDIO_UAC_TYPE_AAC = 2,
	CVIAUDIO_UAC_TYPE_MAX
} E_CVIAUDIO_UAC_CODETYPE;

typedef enum _E_CVIAUDIO_UAC_SRC_PATH {
	CVIAUDIO_SRC_MIC_IN = 0,
	CVIAUDIO_SRC_USB_IN = 1,
	CVIAUDIO_SRC_FILE_IN = 2,
	CVIAUDIO_SRC_STREAM_IN = 3,
	CVIAUDIO_SRC_MAX_NUM
} E_CVIAUDIO_UAC_SRC_PATH;

typedef struct _ST_CVI_UAC_PCM_IN_PARAM {
	unsigned int channels;
	unsigned int rate;
	unsigned int period_size;
} ST_CVI_UAC_PCM_IN_PARAM;

typedef struct _ST_CVI_UAC_MIC_IN_PARAM {
	unsigned int channels;
	unsigned int rate;
	unsigned int period_size;
	//if need to connect CVIAUDIO_DST_FILE_SAVE,
	//below pfd_MicIn should update
	FILE *pfd_MicInSave;
} ST_CVI_UAC_MIC_IN_PARAM;


typedef struct _ST_CVI_UAC_USB_PARAM {
	//E_CVIAUDIO_UAC_CODETYPE encodeUsbInType;
	//if need to set CVIAUDIO_DST_FILE_SAVE to dst in file mode
	//pfd_usb_save need to do fopen in user layer
	FILE *pfd_usb_save;
} ST_CVI_UAC_USB_PARAM;

typedef struct _ST_CVI_UAC_FILE_PARAM {
	//E_CVIAUDIO_UAC_CODETYPE encodeFileType;
	FILE *pfd;
	FILE *pfd_save;
	E_CVIAUDIO_UAC_CODETYPE decodeFileType;//mp3, aac, or pcm
	ST_CVI_UAC_PCM_IN_PARAM pcmInfo;

} ST_CVI_UAC_FILE_PARAM;


typedef struct _ST_UAC_SRC_PARAM {
	ST_CVI_UAC_MIC_IN_PARAM stMicInParam;
	ST_CVI_UAC_USB_PARAM stUsbInParam;
	ST_CVI_UAC_FILE_PARAM stFileInParam;
} ST_UAC_SRC_PARAM;

typedef enum _E_CVIAUDIO_UAC_DST_PATH {
	CVIAUDIO_DST_SPK_OUT = 0,
	CVIAUDIO_DST_USB_OUT = 1,
	CVIAUDIO_DST_FILE_SAVE = 2,
	CVIAUDIO_DST_MAX_NUM
} E_CVIAUDIO_UAC_DST_PATH;

#define CHECK_UAC_SRC_VALID(x) \
	((((x) > (CVIAUDIO_SRC_MAX_NUM-1))) ? 1:0)
#define CHECK_UAC_DST_VALID(x) \
	((((x) > (CVIAUDIO_DST_MAX_NUM-1))) ? 1:0)
#define CHECK_UAC_CODEC_VALID(x) \
	((((x) > (CVIAUDIO_UAC_TYPE_MAX-1))) ? 1:0)

//UAC mic in / spk out volume api
int cviaudio_set_spk_volume(int volstep);//0~32
int cviaudio_set_mic_volume(int volstep);//0~24

//UAC api
int cviaudio_uac_init(void);
int cviaudio_uac_createSrc(E_CVIAUDIO_UAC_SRC_PATH eSrcPath,
			E_CVIAUDIO_UAC_CODETYPE eCodeType,
			ST_UAC_SRC_PARAM *pstUacSrcParam);
int cviaudio_uac_createDst(E_CVIAUDIO_UAC_DST_PATH eDstPath);
int cviaudio_uac_bind(E_CVIAUDIO_UAC_SRC_PATH eSrcPath,
			E_CVIAUDIO_UAC_DST_PATH eDstPath);
int cviaudio_uac_unbind(E_CVIAUDIO_UAC_SRC_PATH eSrcPath,
			E_CVIAUDIO_UAC_DST_PATH eDstPath);
int cviaudio_uac_stream_mode(char *input_buffer, int input_size);
int cviaudio_uac_destroy(E_CVIAUDIO_UAC_SRC_PATH eSrcPath,
			E_CVIAUDIO_UAC_DST_PATH eDstPath);
int cviaudio_uac_deinit(void);

#endif


#endif /* _CVI_AUDIO_UAC_H_ */

