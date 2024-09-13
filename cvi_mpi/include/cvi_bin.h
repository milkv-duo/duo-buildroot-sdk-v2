#ifndef __CVI_BIN_H__
#define __CVI_BIN_H__


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include "stdio.h"
#include <linux/cvi_type.h>
#include <linux/cvi_comm_video.h>

// ++++++++ If you want to change these interfaces, please contact the isp team. ++++++++
#define BIN_FILE_LENGTH	256 /*max length of bin file name*/

#define CVI_BIN_NULL_POINT  0xCB000001 /*input pointer is null*/
#define CVI_BIN_REG_ATTR_ERR  0xCB000002 /*Pqbin attribute registration failed*/
#define CVI_BIN_MALLOC_ERR  0xCB000003 /*malloc fail*/
#define CVI_BIN_CHIP_ERR  0xCB000004 /*Chip type of pqbin is error*/
#define CVI_BIN_CRC_ERR  0xCB000005 /*CRC of pqbin is error*/
#define CVI_BIN_SIZE_ERR  0xCB000006 /*size of input data is error.*/
// #define CVI_BIN_LEBLE_ERR  0xCB000007 /*no use*/
#define CVI_BIN_DATA_ERR  0xCB000008 /*data abnormal*/
#define CVI_BIN_SECURITY_SOLUTION_FAILED  0xCB00000A /*Security solution is failed*/
#define CVI_BIN_COMPRESS_ERROR  0xCB00000B /*json compress buffer fail.*/
#define CVI_BIN_UNCOMPRESS_ERROR  0xCB00000C /*json uncompress fail.*/
#define CVI_BIN_SAPCE_ERR  0xCB00000D /*Input buffer space isn't enough.*/
#define CVI_BIN_JSON_ERR  0xCB00000E /*json is inexistence in current bin file.*/
#define CVI_BIN_UPDATE_ERROR  0xCB00000F /*Update bin para fail automatically*/
#define CVI_BIN_FINDBINFILE_ERROR  0xCB000010 /*can't find bin file*/
#define CVI_BIN_NOJSON_ERROR  0xCB000011 /*invalid json in current bin file*/
#define CVI_BIN_JSONHANLE_ERROR  0xCB000012 /*creat json handle fail*/
#define CVI_BIN_ID_ERROR  0xCB000013 /*invalid id error*/
#define CVI_BIN_READ_ERROR  0xCB000014 /*read para from file fail*/
#define CVI_BIN_FILE_ERROR  0xCB000015 /*PQbin file is invalid.*/
#define CVI_BIN_SENSORNUM_ERROR  0xCB000016 /*Sensor number exceeds specified sensor number in the current bin file.*/
#define CVI_BIN_MODULE_NOT_REGISTER_ERROR  0xCB000017 /*Module isn't registered in the current board.*/
#define CVI_BIN_MODULE_IS_EMPTY_ERROR  0xCB000018 /*Current module id is empty in the bin file.*/

enum CVI_BIN_SECTION_ID { /*module id*/
	CVI_BIN_ID_MIN = 0,
	CVI_BIN_ID_HEADER = CVI_BIN_ID_MIN, /*header*/
	CVI_BIN_ID_ISP0,       /*sensor 0*/
	CVI_BIN_ID_ISP1,       /*sensor 1*/
	CVI_BIN_ID_ISP2,       /*sensor 2*/
	CVI_BIN_ID_ISP3,       /*sensor 3*/
	CVI_BIN_ID_VPSS,           /*VPSS*/
	CVI_BIN_ID_VDEC,           /*VDEC*/
	CVI_BIN_ID_VENC,           /*VENC*/
	CVI_BIN_ID_VO,             /* VO */
	CVI_BIN_ID_MAX             /* ALL*/
};

enum CVI_BIN_CREATMODE { /*CREATE MODE*/
	CVI_BIN_AUTO = 0,      /*AUTO MODE*/
	CVI_BIN_MANUAL,      /*MANUAL MODE*/
	CVI_BIN_MODE_MAX        /*MODE MAX*/
};

typedef struct {      /*EXTRA INFO*/
	CVI_UCHAR Author[32]; /*Author*/
	CVI_UCHAR Desc[1024];   /*Desc*/
	CVI_UCHAR Time[32];     /*Time*/
} CVI_BIN_EXTRA_S;

typedef struct {                /*JSON FILE INFO*/
	CVI_U32 u32InitSize;             /*Init Size*/
	CVI_U32 u32CompreSize; /*Size after compress*/
} CVI_JSON_INFO;

typedef struct _CVI_JSON_HEADER { /*JSON FILE INFO for every modules*/
	CVI_JSON_INFO size[CVI_BIN_ID_MAX];
} CVI_JSON_HEADER;

typedef struct _CVI_BIN_HEADER { /*header info*/
	CVI_U32 chipId;               /* chip Id  */
	CVI_BIN_EXTRA_S extraInfo;    /*extra Info*/
	CVI_U32 size[CVI_BIN_ID_MAX]; /*size info for every modules*/
} CVI_BIN_HEADER;

// make params of isp bin to be bypassed
typedef union _ISP_BIN_BYPASS_U {
	CVI_U8 u8Key;
	struct {
		CVI_U8 bitBypassFrameRate : 1; /*FrameRate Bypass [0]*/
		CVI_U8 bitRsv7 : 7;            /*[1:7]*/
	};
} ISP_BIN_BYPASS_U;

// make params bypassed for all modules
typedef struct _ISP_BIN_BYPASS {
	ISP_BIN_BYPASS_U ispBinBypass[VI_MAX_PIPE_NUM];
} ISP_BIN_BYPASS;

/* CVI_BIN_GetBinExtraAttr:
 *   get Author, Desc, Time from bin file
 * [in] fp: file pointer of pqbin
 * [out] extraInfo: pointer of Attr returning Info
 * return: 0: Success;
 *		error codes:
 *		0xCB000001: input pointer is null.
 *		0xCB000003: malloc fail.
 *		0xCB000006: size of inputing data is error.
 *		0xCB000008: data error.
 *		0xCB00000F: update bin para from json fail automatically.
 *		0xCB000010: can't find bin file.
 *		0xCB000011: invalid json in current bin file.
 *		0xCB000012: creat json handle fail.
 *		0xCB000013: invalid id error.
 *		0xCB000014: read para from file fail.
 *		0xCB000015: current PQbin file is invalid.
 *		0xCB000016: Sensor number exceeds specified sensor number in bin file.
 */
CVI_S32 CVI_BIN_GetBinExtraAttr(FILE *fp, CVI_BIN_EXTRA_S *extraInfo);

/* CVI_BIN_SaveParamToBin:
 *   Save Param To Bin file
 * [in] fp: file pointer of pqbin
 *   extraInfo: Info of header in pqbin
 * [out] void
 * return: please refer to CVI_BIN_ExportBinData.
 */
CVI_S32 CVI_BIN_SaveParamToBin(FILE *fp, CVI_BIN_EXTRA_S *extraInfo);

/* CVI_BIN_LoadParamFromBin:
 *   get bin data from buffer
 *
 * [in]	id: module id which selecetd to load
 *   buf: input buf
 * [Out]void
 * return: please refer to CVI_BIN_ImportBinData
 */
CVI_S32 CVI_BIN_LoadParamFromBin(enum CVI_BIN_SECTION_ID id, CVI_U8 *buf);

/* CVI_BIN_SetBinName:
 *   Set the path and file name for storing PQBin
 *
 * [in]	wdrMode: Sensor wdr mode
 *   binName: Set the path and file name of pqbin corresponding to wdr mode
 *            For example, "/mnt/data/cvi-sdr_bin"
 * [Out]void
 * return: please refer to CVI_BIN_ImportBinData
 */
CVI_S32 CVI_BIN_SetBinName(WDR_MODE_E wdrMode, const CVI_CHAR *binName);

/* CVI_BIN_GetBinName:
 *   get the path and file name where PQBin is stored.
 *
 * [in]	void
 * [Out]path and file name where PQBin is stored.
 * return: please refer to CVI_BIN_ImportBinData
 */
CVI_S32 CVI_BIN_GetBinName(CVI_CHAR *binName);

/* CVI_BIN_GetSingleISPBinLen:
 *   get the length of bin data for a given single module.
 *
 * [in]	id: module id which selecetd to export.
 * [Out]void
 * return: length of bin data for a given single module.
 */
CVI_S32 CVI_BIN_GetSingleISPBinLen(enum CVI_BIN_SECTION_ID id);

/* CVI_BIN_ExportSingleISPBinData:
 *   set bin data from buffer for a given single module.
 *
 * [in]	id: module id which selecetd to export.
 *   pu8Buffer:saved bin data
 *   u32DataLength:length of bin data
 * [Out]void
 * return: please refer to CVI_BIN_ExportBinData
 */
CVI_S32 CVI_BIN_ExportSingleISPBinData(enum CVI_BIN_SECTION_ID id, CVI_U8 *pu8Buffer, CVI_U32 u32DataLength);

/* CVI_BIN_LoadParamFromBinEx:
 *   get bin data from buffer
 *
 * [in]	buf: input buf
 *   u32DataLength:length of bin data
 * [Out]void
 * return: please refer to CVI_BIN_ImportBinData
 */
CVI_S32 CVI_BIN_LoadParamFromBinEx(enum CVI_BIN_SECTION_ID id, CVI_U8 *buf, CVI_U32 u32DataLength);

/* CVI_BIN_GetBinTotalLen:
 *   Get length of bin data
 *
 * [in]void
 * [Out]void
 * return: length of bin data
 */
CVI_U32 CVI_BIN_GetBinTotalLen(void);

/* CVI_BIN_ExportBinData:
 *   get bin data from buffer
 *
 * [in]	pu8Buffer:saved bin data
 *      u32DataLength:length of bin data
 * [Out]void.
 * return: 0: Success;
 *		error codes:
 *		0xCB000001:input pointer is null.
 *		0xCB000003: malloc fail.
 *		0xCB000008: data error.
 *		0xCB00000A: security solution fail.
 *		0xCB00000B: json compress fail.
 *		0xCB00000D: Input buffer space isn't enough.
 *		0xCB000012: creat json handle fail.
 *		0xCB000013: invalid id error.
 */
CVI_S32 CVI_BIN_ExportBinData(CVI_U8 *pu8Buffer, CVI_U32 u32DataLength);

/* CVI_BIN_ImportBinData:
 *   set bin data from buffer
 *
 * [in]	pu8Buffer:save bin data
 *		u32DataLength:length of bin data
 * [Out]void
 * return: 0: Success;
 *		error codes:
 *		0xCB000001: input pointer is null.
 *		0xCB000003: malloc fail.
 *		0xCB000006: size of inputing data is error.
 *		0xCB000008: data error.
 *		0xCB00000C: json uncompress fail.
 *		0xCB00000E: json inexistence fail.
 *		0xCB00000F: update bin para from json fail automatically.
 *		0xCB000010: can't find bin file.
 *		0xCB000011: invalid json in current bin file.
 *		0xCB000012: creat json handle fail.
 *		0xCB000013: invalid id error.
 *		0xCB000014: read para from file fail.
 *		0xCB000015: current PQbin file is invalid.
 *		0xCB000016: Sensor number exceeds specified sensor number in bin file.
 */
CVI_S32 CVI_BIN_ImportBinData(CVI_U8 *pu8Buffer, CVI_U32 u32DataLength);

/* CVI_ISP_BIN_SetBypassParams:
 * set the params of ispBinBypass, indicatting which param to be bypassed.
 * [in]	id: sensor id whose params selecetd to be bypassed.
 *    ispBinBypass: the params indicatting which param to be bypassed in sensor of id.
 * [Out]void
 * return: 0: Success;
 *      error codes:
 *      -1: FAILURE;
 *      0xCB000013: invalid id error. The given id is not valid Sensor id.
 */
CVI_S32 CVI_ISP_BIN_SetBypassParams(enum CVI_BIN_SECTION_ID id, ISP_BIN_BYPASS_U *ispBinBypass);

/* CVI_ISP_BIN_GetBypassParams:
 * get the params of ispBinBypass, indicatting which param to be bypassed.
 * [in]	id: sensor id whose params selecetd to be bypassed.
 * [Out]ispBinBypass: the params indicatting which param to be bypassed in sensor of id.
 * return: 0: Success;
 *      error codes:
 *      -1: FAILURE;
 *      0xCB000013: invalid id error. The given id is not valid Sensor id.
 */
CVI_S32 CVI_ISP_BIN_GetBypassParams(enum CVI_BIN_SECTION_ID id, ISP_BIN_BYPASS_U *ispBinBypass);
// -------- If you want to change these interfaces, please contact the isp team. --------

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif


