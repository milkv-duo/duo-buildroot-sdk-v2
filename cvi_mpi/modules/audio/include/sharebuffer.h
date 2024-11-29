#ifndef __SHARE_BUFFER_H__
#define __SHARE_BUFFER_H__
#include <stdio.h>
#include <stdint.h>

int share_cyclebuffer_init(CVI_U32 length, int bClient);
int share_cyclebuffer_client_write(CVI_U32 index, char *data, CVI_U32 writeLen, CVI_S32 msecs);
int share_cyclebuffer_server_read(CVI_U32 index, char  *outbuf, int readLen);
int share_cyclebuffer_client_read(CVI_U32 index, char  *outbuf, int readLen, CVI_S32 msecs);
int share_cyclebuffer_server_write(CVI_U32 index, char *data, CVI_U32 writeLen);
int share_cyclebuffer_frameready_size(int index);
int share_cyclebuffer_clear(int index);
void share_cyclebuffer_destroy(int index);

#endif //__SHARE_BUFFER_H__
