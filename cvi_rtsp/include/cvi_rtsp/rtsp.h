#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "defs.h"

#define SYSLOG(level, fmt, ...) syslog(level, "[%s:%d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__)

/**
 * @brief        create RTSP contex handle
 * @param ctx    Out: context, store the contex for rtsp operation
 * @param config In : config, rtsp config
 * @return         Ok:0, Error:-1
 */
int CVI_RTSP_Create(CVI_RTSP_CTX **ctx, CVI_RTSP_CONFIG *config);

/**
 * @brief        delete RTSP contex handle
 * @param ctx    In: context, store the contex for rtsp operation
 * @return       Ok:0, Error:-1
 */
int CVI_RTSP_Destroy(CVI_RTSP_CTX **ctx);

/**
 * @brief        start rtsp service
 * @param ctx    In: context, store the contex for rtsp operation
 * @return       Ok:0, Error:-1
 */
int CVI_RTSP_Start(CVI_RTSP_CTX *ctx);

/**
 * @brief        stop rtsp service
 * @param ctx    In: context, store the contex for rtsp operation
 * @return       Ok:0, Error:-1
 */
int CVI_RTSP_Stop(CVI_RTSP_CTX *ctx);

/**
 * @brief          Write frame data
 * @param ctx      In: context, store the contex for rtsp operation
 * @param track    In: track info
 * @param data     In: frame data
 * @return         Ok:0, Error:-1
 */

int CVI_RTSP_WriteFrame(CVI_RTSP_CTX *ctx, CVI_RTSP_TRACK track, CVI_RTSP_DATA *data);
/**
 * @brief            Add media stream
 * @param ctx        In: context, store the contex for rtsp operation
 * @param src        In: source info, contain type, codec info
 * @param streamName In: Stream name
 * @return           Ok:0, Error:-1
 */
int CVI_RTSP_CreateSession(CVI_RTSP_CTX *ctx, CVI_RTSP_SESSION_ATTR *attr, CVI_RTSP_SESSION **session);

/**
 * @brief            Remove media stream
 * @param streamName In: stream name
 * @return           Ok:0, Error:-1
 */
int CVI_RTSP_DestroySession(CVI_RTSP_CTX *ctx, CVI_RTSP_SESSION *session);

/**
 * @brief          Set connect and disconnect callback
 * @param listener In: listener info, contains callback and callback arg
 * @return         Ok:0, Error:-1
 */
int CVI_RTSP_SetListener(CVI_RTSP_CTX *ctx, CVI_RTSP_STATE_LISTENER *listener);

/**
 * @brief          for c program to set OutPacketBuffer::maxSize
 * @param rtsp_max_buf_size In: rtsp max buffer size
 * @return         void
 */
void CVI_RTSP_SetOutPckBuf_MaxSize(uint32_t rtsp_max_buf_size);

#ifdef __cplusplus
}
#endif
