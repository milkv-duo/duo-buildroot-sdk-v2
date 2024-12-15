#pragma once

int init_teaisp_drc(SERVICE_CTX *ctx);
int deinit_teaisp_drc(SERVICE_CTX *ctx);

int get_teaisp_drc_video_frame(int pipe, void *video_frame);
int put_teaisp_drc_video_frame(int pipe, void *video_frame);

