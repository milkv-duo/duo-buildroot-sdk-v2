#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/statvfs.h>
#include <mntent.h>

#include "ovd_get_info.h"

#define CVITPU_GET_USAGE _IOWR('p', 0x0D, IpuInfo_t)

int getIpuUtilization(IpuInfo_t *pstIpuInfo, int time_interval)
{
    const char *device_path = "/dev/cvi-tpu0";
    uint64_t start_time_us, end_time_us;
    int ret, fd;

    if (pstIpuInfo == NULL)
        perror("FpstIpuInfo is NULL");
    fd = open(device_path, O_RDWR);
    if (fd < 0) {
        perror("Failed to open TPU device");
        return errno;
    }

    ret = ioctl(fd, CVITPU_GET_USAGE, &start_time_us);
    if (ret < 0) {
        perror("Failed to get TPU usage");
        close(fd);
        return errno;
    }

    sleep(time_interval);

    ret = ioctl(fd, CVITPU_GET_USAGE, &end_time_us);
    if (ret < 0) {
        perror("Failed to get TPU usage");
        close(fd);
        return errno;
    }
    close(fd);

    pstIpuInfo->ipu0_Utilization = ((float)(end_time_us - start_time_us) / (1000000.0f * time_interval)) * 100.0f;
    pstIpuInfo->ipu1_Utilization = 0.0f;

    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

int readMemInfo(const char *key) {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (fp == NULL) {
        perror("Error opening /proc/meminfo");
        return -1;
    }

    char line[256];
    int value = -1;
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, key, strlen(key)) == 0) {
            char *end;
            value = strtol(line + strlen(key), &end, 10);
            break;
        }
    }

    fclose(fp);
    return value;
}

int getSysMemFreeInfo(int *piSysMemFreeInfo) {
    if (piSysMemFreeInfo == NULL) {
        return -1;
    }

    int available = readMemInfo("MemAvailable:");
    int buffers = readMemInfo("Buffers:");
    int cached = readMemInfo("Cached:");

    if (available == -1 || buffers == -1 || cached == -1) {
        return EXIT_FAILURE;
    }

    *piSysMemFreeInfo = available + buffers + cached;

    *piSysMemFreeInfo /= 1024;

    return EXIT_SUCCESS;
}
///////////////////////////////////////////////////////////////////////////
int getFlashFreeInfo(int *piFlashFreeInfo) {
    FILE *mtab;
    struct mntent *mnt;
    struct statvfs buf;
    uint64_t total_free_space = 0;

    if (piFlashFreeInfo == NULL) {
        fprintf(stderr, "error: piFlashFreeInfo is NULL\n");
        return EXIT_FAILURE;
    }

    mtab = setmntent("/etc/mtab", "r");
    if (mtab == NULL) {
        perror("Error opening /etc/mtab");
        return EXIT_FAILURE;
    }

    while ((mnt = getmntent(mtab)) != NULL) {
        if (strstr(mnt->mnt_fsname, "mmcblk") == NULL) {
            if (statvfs(mnt->mnt_dir, &buf) == 0) {
                total_free_space += (uint64_t)buf.f_bsize * buf.f_bavail;
            } else {
                perror("statvfs error");
            }
        }
    }
    endmntent(mtab);

    *piFlashFreeInfo = total_free_space / (1024 * 1024);

    return EXIT_SUCCESS;
}


/////////////////////////////////////////////////////////////////////////////
int test_000(void)
{
    IpuInfo_t ipuInfo;
    int time_interval = 1;

    int ret = getIpuUtilization(&ipuInfo, time_interval);
    if (ret == 0) {
        printf("IPU0 Utilization: %.2f%%\n", ipuInfo.ipu0_Utilization);
        printf("IPU1 Utilization: %.2f%%\n", ipuInfo.ipu1_Utilization);
    } else {
        fprintf(stderr, "Failed to get IPU utilization.\n");
    }

    return ret;
}

int test_001() {
    int freeMemory;
    int result = getSysMemFreeInfo(&freeMemory);

    if (result == 0) {
        printf("Free System Memory: %d MB\n", freeMemory);
    } else {
        fprintf(stderr, "Failed to get free system memory info\n");
    }

    return result;
}

int test_002() {
    int piFlashFreeInfo;
    int result = getFlashFreeInfo(&piFlashFreeInfo);

    if (result == 0) {
        printf("Free Flash Memory: %d MB\n", piFlashFreeInfo);
    } else {
        fprintf(stderr, "Failed to get free flash memory info\n");
    }

    return result;
}

//////////////////////////////////////////////////
int main(void)
{
    test_000();
    test_001();
    test_002();

    return 0;
}
