
#ifndef __CVI_MEDIALIST_H__
#define __CVI_MEDIALIST_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <linux/cvi_type.h>

typedef int (*media_malloc_func)(void **dst, void *src);
typedef int (*media_relase_func)(void **src);
typedef void (*media_handler_func)(int venc, void *parm, void *data);
#define MEDIAPROCUNIT_MAXDATASIZE 200

CVI_S32 CVI_MEDIA_ListPushBack(CVI_S32 venc, void *pstream);
CVI_S32 CVI_MEDIA_ListPopFront(CVI_S32 venc, void **pstream);
CVI_S32 CVI_MEDIA_ProcUnitInit(CVI_S32 num, void *pstparm, media_handler_func phandler, media_malloc_func pmalloc,
							   media_relase_func prelase);
CVI_S32 CVI_MEDIA_ProcUnitDeInit(void);
#endif
