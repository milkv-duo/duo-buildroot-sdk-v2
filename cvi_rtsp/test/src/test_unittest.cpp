#include <cvi_rtsp_service/service.h>
#include <unistd.h>
#include "test.h"

int create_service_from_config_default()
{
    RTSP_SERVICE_HANDLE hdl;

    int ret = CVI_RTSP_SERVICE_CreateFromJsonFile(&hdl, nullptr);

    CHECK_EQ(ret, 0);
    sleep(1);

    return CVI_RTSP_SERVICE_Destroy(&hdl);
}

int create_service_from_str_default()
{
    RTSP_SERVICE_HANDLE hdl;

    int ret = CVI_RTSP_SERVICE_CreateFromJsonStr(&hdl, nullptr);
    CHECK_EQ(ret, 0);

    sleep(1);
    return CVI_RTSP_SERVICE_Destroy(&hdl);
}

int create_service_from_config()
{
    RTSP_SERVICE_HANDLE hdl;

    int ret = CVI_RTSP_SERVICE_CreateFromJsonFile(&hdl, "./empty.json");
    CHECK_EQ(ret, 0);

    sleep(1);
    return CVI_RTSP_SERVICE_Destroy(&hdl);
}

int create_service_from_str()
{
    RTSP_SERVICE_HANDLE hdl;

    int ret = CVI_RTSP_SERVICE_CreateFromJsonStr(&hdl, "{}");
    CHECK_EQ(ret, 0);

    sleep(1);
    return CVI_RTSP_SERVICE_Destroy(&hdl);
}

int create_service_from_config_not_exist()
{
    RTSP_SERVICE_HANDLE hdl;

    int ret = CVI_RTSP_SERVICE_CreateFromJsonFile(&hdl, "HelloWorld");
    CHECK_EQ(ret, -1);

    return 0;
}



int main(int argc, char *argv[])
{
    TestCase cases[] = {
        {"create service from config default", create_service_from_config_default},
        {"create service from str default", create_service_from_str_default},
        {"create service from config", create_service_from_config},
        {"create service from str",  create_service_from_str},
        {"create service from config not exist", create_service_from_config_not_exist},
        {NULL, NULL}
    };

    run_all_tests(cases, true);

    return 0;
}
