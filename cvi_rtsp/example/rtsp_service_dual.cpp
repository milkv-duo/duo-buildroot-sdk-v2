#include <cvi_rtsp_service/service.h>
#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <string>

bool run = true;
static void handleSig(int signo)
{
    run = false;
}

int main(int argc, const char *argv[])
{
    signal(SIGPIPE,SIG_IGN);
    signal(SIGINT, handleSig);
    signal(SIGTERM, handleSig);
    signal(SIGHUP, handleSig);

    // std::string json_0 = "{\"chn\":0,\"buf-blk-cnt\":4,\"venc-bind-vpss\":true,\"vi-vpss-mode\":3}"; //vi-vpss: online-online mode
    // std::string json_1 = "{\"chn\":1,\"buf-blk-cnt\":4,\"venc-bind-vpss\":true,\"vi-vpss-mode\":3}"; //vi-vpss: online-online mode

    RTSP_SERVICE_HANDLE hdl = NULL;
    RTSP_SERVICE_HANDLE hdl2 = NULL;

    if (0 > CVI_RTSP_SERVICE_CreateFromJsonFile(&hdl, argv[1])) {
        std::cout << "service hdl create fail" << std::endl;
        return -1;
    }

    if (argc >2 && 0 > CVI_RTSP_SERVICE_CreateFromJsonFile(&hdl2, argv[2])) {
        std::cout << "service hdl2 create fail" << std::endl;
        return -1;
    }

    while (run) {
        sleep(1);
    }

    if (argc >2 && 0 > CVI_RTSP_SERVICE_Destroy(&hdl2)) {
        std::cout << "service hdl2 destroy fail" << std::endl;
        return -1;
    }

    if (0 > CVI_RTSP_SERVICE_Destroy(&hdl)) {
        std::cout << "service hdl destroy fail" << std::endl;
        return -1;
    }

    return 0;
}
