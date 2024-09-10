#include <cvi_rtsp_service/service.h>
#include "test.h"

int test_enable_retina_face()
{
    RTSP_SERVICE_HANDLE hdl;

    int ret = CVI_RTSP_SERVICE_CreateFromJsonFile(&hdl, "./retinaface.json");
    CHECK_EQ(ret, 0);

    return CVI_RTSP_SERVICE_Destroy(&hdl);
}

int main(int argc, char *argv[])
{

    TestCase cases[] = {
        {"enable retina face", test_enable_retina_face},
        {NULL, NULL}
    };

    run_all_tests(cases, true);

    return 0;
}
