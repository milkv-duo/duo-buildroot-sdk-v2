#include <cvi_rtsp_service/service.h>
#include <iostream>
#include <signal.h>
#include <unistd.h>

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

    RTSP_SERVICE_HANDLE hdl = NULL;

    if (0 > CVI_RTSP_SERVICE_CreateFromJsonFile(&hdl, argv[1])) {
        std::cout << "service create fail" << std::endl;
        return -1;
    }

    while (run) {
        usleep(500000);
    }

    if (0 > CVI_RTSP_SERVICE_Destroy(&hdl)) {
        std::cout << "service destroy fail" << std::endl;
        return -1;
    }

    return 0;
}
