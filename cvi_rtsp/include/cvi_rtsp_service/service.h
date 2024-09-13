#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef void *RTSP_SERVICE_HANDLE;

int CVI_RTSP_SERVICE_CreateFromJsonFile(RTSP_SERVICE_HANDLE *hdl, const char *jsonFile);
int CVI_RTSP_SERVICE_CreateFromJsonStr(RTSP_SERVICE_HANDLE *hdl, const char *jsonStr);
int CVI_RTSP_SERVICE_HupFromJsonFile(RTSP_SERVICE_HANDLE hdl, const char *jsonFile);
int CVI_RTSP_SERVICE_HupFromJsonStr(RTSP_SERVICE_HANDLE hdl, const char *jsonStr);
int CVI_RTSP_SERVICE_Destroy(RTSP_SERVICE_HANDLE *hdl);

#ifdef __cplusplus
}
#endif
