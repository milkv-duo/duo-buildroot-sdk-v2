#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <semaphore.h>
#include <cvi_rtsp/rtsp.h>

#ifdef BUILD_FILEMODE
#include <cvi_demuxer/cvi_demuxer.h>

bool run = true;
#define PACKET_START_CODE_SIZE 3
static const uint8_t PACKET_START_CODE[PACKET_START_CODE_SIZE] = { 0, 0, 1 };

struct CTX {
    CVI_RTSP_CTX *rtsp;
    CVI_RTSP_SESSION *session;
    CVI_DEMUXER_HANDLE demuxer;
    pthread_t worker;
    bool play;
};


static void handleSig(int signo)
{
    run = false;
}

void client_connect(const char *ip, void *arg)
{
    std::cout << "connect: " << ip << std::endl;
}

void client_disconnect(const char *ip, void *arg)
{
    std::cout << "disconnect: " << ip << std::endl;
}

static bool hasStartCode(const uint8_t* start_code, const uint8_t *data, int max_length)
{
    bool find = false;

    for (int i = 0; i < max_length; ++i) {
        bool match = true;
        for (int j = 0; j < PACKET_START_CODE_SIZE; ++j) {
            if (data[i + j] != start_code[j]) {
                match = false;
                break;
            }
        }
        if (match) {
            find = true;
            break;
        }
    }

    return find;
}

static void replaceStartCode(const uint8_t* start_code, uint8_t *data, unsigned int data_length)
{
    // number of bytes for packet payload length
    const int packet_length_size = 4;
    int length_diff = packet_length_size - PACKET_START_CODE_SIZE;
    uint64_t current_pos = 0;
    uint32_t length = 0;

    // no space left to put start code
    if (length_diff < 0) {
        return;
    }

    while (current_pos < (data_length - packet_length_size)) {
        for (int i = 0; i < packet_length_size; ++i) {
            length = length << 8 | data[current_pos + i];
        }

        for (int i = 0; i < PACKET_START_CODE_SIZE; ++i) {
            data[current_pos + length_diff + i] = start_code[i];
        }

        current_pos += packet_length_size + length;
    }
}

void *play_video(void *arg)
{
    CTX *ctx = (CTX *)arg;

    CVI_DEMUXER_Open(ctx->demuxer);
    while (ctx->play) {
        CVI_DEMUXER_PACKET packet = {0};
        if (0 > CVI_DEMUXER_Read(ctx->demuxer, &packet)) {
            break;
        }

        if (packet.size == 0) {
            break;
        }

        if (!hasStartCode(PACKET_START_CODE, packet.data, PACKET_START_CODE_SIZE)) {
            replaceStartCode(PACKET_START_CODE, packet.data, packet.size);
        }

        CVI_RTSP_DATA data = {0};

        data.dataPtr[0] = packet.data;
        data.dataLen[0] = packet.size;
        data.blockCnt = 1;

        CVI_RTSP_WriteFrame(ctx->rtsp, ctx->session->video, &data);
    }

	CVI_DEMUXER_Close(ctx->demuxer);

    return NULL;
}

void play(int references, void *arg)
{
    CTX *ctx = (CTX *)arg;
    ctx->play = true;
    if (0 > pthread_create(&ctx->worker, NULL, play_video, arg)) {
        std::cout << "fail to play video" << std::endl;
        exit(1);
    }
}

void stop(int references, void *arg)
{
    CTX *ctx = (CTX *)arg;

    ctx->play = false;
    pthread_join(ctx->worker, NULL);

    CVI_DEMUXER_Close(ctx->demuxer);
}

void main_loop()
{
	while (run) {
		sleep(1);
	}

}

int main(int argc, const char *argv[])
{
    signal(SIGPIPE,SIG_IGN);
    signal(SIGINT, handleSig);
    signal(SIGTERM, handleSig);

    CTX *ctx = (CTX *)malloc(sizeof(CTX));
    memset(ctx, 0, sizeof(CTX));

    CVI_DEMUXER_MEDIA_INFO info = {0};

    CVI_DEMUXER_Create(&ctx->demuxer);
    CVI_DEMUXER_SetInput(ctx->demuxer, argv[1]);

    CVI_DEMUXER_Open(ctx->demuxer);
    CVI_DEMUXER_GetMediaInfo(ctx->demuxer, &info);
    CVI_DEMUXER_Close(ctx->demuxer);

    CVI_RTSP_CONFIG config = {0};
    if (0 > CVI_RTSP_Create(&ctx->rtsp, &config)) {
        std::cout << "fail to create rtsp contex" << std::endl;
        return -1;
    }

    CVI_RTSP_SESSION_ATTR attr = {0};
    attr.video.codec = 0 == strcmp("h264", info.video_codec)? RTSP_VIDEO_H264: RTSP_VIDEO_H265;
    snprintf(attr.name, sizeof(attr.name), "%s", "rtsp");

    attr.video.play = play;
    attr.video.playArg = (void *)ctx;

    attr.video.teardown = stop;
    attr.video.teardownArg = (void *)ctx;

    if (0 > CVI_RTSP_CreateSession(ctx->rtsp, &attr, &ctx->session)) {
        std::cout << "create session fail" << std::endl;
        return -1;
    }

    CVI_RTSP_STATE_LISTENER listener = {0};
    listener.onConnect = client_connect;
    listener.argConn = NULL;
    listener.onDisconnect = client_disconnect;
    listener.argDisconn = NULL;

    CVI_RTSP_SetListener(ctx->rtsp, &listener);

    if (0 > CVI_RTSP_Start(ctx->rtsp)) {
        std::cout << "fail to start" << std::endl;
        return -1;
    }

    main_loop();

    CVI_RTSP_Stop(ctx->rtsp);
    CVI_RTSP_DestroySession(ctx->rtsp, ctx->session);
    CVI_RTSP_Destroy(&ctx->rtsp);

    CVI_DEMUXER_Destroy(&ctx->demuxer);

    free(ctx);

    return 0;
}
#else
int main(int argc, const char *argv[])
{
	return 0;
}
#endif
