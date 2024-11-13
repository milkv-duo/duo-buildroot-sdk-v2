#ifndef __CYCLE_BUFFER_H__
#define __CYCLE_BUFFER_H__
#include <stdio.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct _cycleBuffer {
	int size;
	int wroffset;
	int rdoffser;
	unsigned int realWR;
	unsigned int realRD;
	char *buf;
	int buflen;
	int isInit;
	pthread_mutex_t lock;
} cycleBuffer;

int CycleBufferInit(void **ppstCb, int bufLen);
void CycleBufferDestory(void *pstCb);
int CycleBufferRead(void *pstCb, char *outbuf, int readLen);
int CycleBufferWrite(void *pstCb, char *inbuf, int wrireLen);
int CycleBufferDataLen(void *pstCb);
int CycleBufferSize(void *pstCb);
void CycleBufferReset(void *pstCb);
int CycleBufferFreeSize(void *pstCb);
int CycleBufferWriteWait(void *pstCb, char *inbuf, int wrireLen, int timeoutMs);





 #ifdef __cplusplus
} /* extern "C" */
#endif

#endif //__CYCLE_BUFFER_H__



