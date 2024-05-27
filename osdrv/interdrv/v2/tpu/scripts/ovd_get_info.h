#ifndef _GET_INFO_H_
#define _GET_INFO_H_

typedef struct _FlashInfo
{
    int size;   //flash 大小，单位：MB
    int clk;    //flash clk，单位：MHz
}FlashInfo_t;

typedef struct _DdrInfo
{
    int size;   //ddr 大小，单位：MB
    int clk;    //ddr clk，单位：MHz
    int bit_width;  //位宽，单位：bit
}DdrInfo_t;

typedef struct _CpuInfo
{
    int cpu0_clk;    //cpu0 clk，单位：kHz
    int cpu1_clk;    //cpu1 clk，单位：kHz
    int core_power; //cpu上电压，单位：mV
}CpuInfo_t;

typedef struct _HardwareInfo{
	FlashInfo_t flashInfo;//flash信息，flash信息中相关参数如果获取不到则置0；
	DdrInfo_t ddrInfo;//ddr内存信息，ddr信息中相关参数如获取不到则置0；
	CpuInfo_t cpuInfo;//cpu信息，cpu信息中相关参数如获取不到，则置0；
	char chipName[64];//芯片型号名称，如获取不到则为空
	char toolChain[64];//工具链信息，如Linux-arm-GOKE-GK7605V200-gcc6.3.0
	float computePower;//芯片算力值，单位T, TFLOPS（Tera Floating-point Operations Per Second）,如果获取不到，则置0；
}HardwareInfo_t;

typedef struct _ComputingPowerMem{
	int mem_total;//算力分配的总内存,单位为MB
	int mem_free;//算力剩余的内存，单位为MB
}ComputingPowerMem_t;

typedef struct _IpuInfo
{
    float ipu0_Utilization; //ipu 核心 0 的利用率，为百分比
    float ipu1_Utilization; //ipu 核心 1 的利用率，为百分比;如无，则置0
}IpuInfo_t;


/**
 * brief 获取设备硬件信息
 * 获取设备硬件信息
 * param[out] pstHardwareInfo 硬件信息，包括芯片型号名称、flash信息、ddr内存信息、cpu信息、算力信息
 * return 返回值描述
 * retval 成功返回0
 * retval 失败返回 非0
 * note
 */
int getHardwareInfo(HardwareInfo_t *pstHardwareInfo);


/**
 * brief：获取实时算力使用率
 * time_interval为统计ipu利用率的周期，单位为秒，统计的为该time_interval时间内ipu利用率
 * param[out] pstIpuInfo ipu统计信息
 * param[in] time_interval统计周期
 * retval:成功返回0
 * retval:失败返回 非0
 */
int getIpuUtilization(IpuInfo_t *pstIpuInfo, int time_interval);


/**
 * brief：获取算力总内存和剩余内存
 * 获取算力总内存和剩余内存
 * param[out] pstComputingPowerMem总内存和剩余内存信息
 * retval:成功返回0
 * retval:失败返回 非0
 * note：
 */
int getComputingPowerMem(ComputingPowerMem_t *pstComputingPowerMem);

/**
 * brief：获取实时系统剩余内存
 * 获取Linux剩余内存，meminfo=available+buffers+cached
 * param[out] pstSysMemFreeInfo剩余内存信息，单位MB
 * retval:成功返回0
 * retval:失败返回 非0
 * note 无
 */
int getSysMemFreeInfo(int *piSysMemFreeInfo);

/**
 * brief：获取实时flash剩余空间
 * 获取flash剩余空间
 * param[out] piFlashFreeInfo flash剩余空间，单位MB
 * retval:成功返回0
 * retval:失败返回 非0
 * note 无
 */
int getFlashFreeInfo(int *piFlashFreeInfo);

#endif
