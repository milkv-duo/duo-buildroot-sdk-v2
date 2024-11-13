#ifndef __FAREND_BUFFER_H__
#define __FAREND_BUFFER_H__
#include <stdio.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct _stFarEndBuffer {
	int size;
	int wroffset;
	int rdoffser;
	unsigned int realWR;
	unsigned int realRD;
	char *buf;
	int buflen;
	int isInit;
	pthread_mutex_t lock;
} stFarEndBuffer;

int FarEndBufferInit(void **ppstCb, int bufLen);
void FarEndBufferDestory(void *pstCb);
int FarEndBufferRead(void *pstCb, char *outbuf, int readLen);
int FarEndBufferWrite(void *pstCb, char *inbuf, int wrireLen);
int FarEndBufferDataLen(void *pstCb);
int FarEndBufferSize(void *pstCb);
void FarEndBufferReset(void *pstCb);
int FarEndBufferFreeSize(void *pstCb);
int FarEndBufferWriteWait(void *pstCb, char *inbuf, int wrireLen, int timeoutMs);



 #ifdef __cplusplus
} /* extern "C" */
#endif

#endif //__CYCLE_BUFFER_H__



