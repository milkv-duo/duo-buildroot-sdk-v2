#ifndef __CVIAUDIO_IOCTL_CMD_H__
#define __CVIAUDIO_IOCTL_CMD_H__

#if defined(__CV181X__) || defined(__CV180X__)
#include <linux/cvi_defines.h>
#else
#include <cvi_defines.h>
#endif
#include <cvi_comm_aio.h>
//#include <cvi_defines.h//watch out for corresponding device node minor id
//minor id should map to channel id
#define CVIAUDIO_DRV_MAX_ALL_NUM  (1)


#define CVIAUDIO_DRV_PLATFORM_DEVICE_NAME	"cvi-audio"
#define CVIAUDIO_DRV_CLASS_NAME			"cvi-audio"
#define CVIAUDIO_CEO_DEV_NAME			"cviaudio_core"

typedef struct _st_ssp_msg {
	AI_TALKVQE_CONFIG_S stVqeConfig;
	int bytes_per_period;
	int channel_cnt; //only support 1 or 2 channel(2nd channe should be ref-data)
	//channel should align the setting of the pcm_open setting
} ST_SSP_MSG;

typedef struct _st_ssp_pcm_msg {
	unsigned int channels;
	unsigned int rate;
	unsigned int period_size;
} ST_SSP_PCM_MSG;

typedef struct _st_ssp_data_msg {
	CVI_U64 data_phyaddr;
	char	*data_addr;
	int	required_size_bytes;
	int	data_valid;
} ST_SSP_DATA_MSG;

typedef struct _st_spk_ssp_msg {
	AO_VQE_CONFIG_S stAoVqeConfig;
} ST_SPK_SSP_MSG;


typedef struct RESAMPLE_INFO_T {
	int inSampleRate;
	int outSampleRate;
	double stepDist;
	CVI_U64 fixedFraction;
	double normFixed;
	CVI_U64 step;
	CVI_S16 *last_input;
	CVI_S16 *output_buf;
	CVI_U64 curOffset;
	CVI_U32 inputsamples;
	CVI_U32 prev_inputsamples; //user may not always send the same size
	CVI_U32 channels;
	CVI_U32 output_index;
	int last_delta;
} RESAMPLE_INFO;


typedef struct _st_ain_msg {
	int AiDev;
	int AiChnId;
	//for channel buffer
	CVI_U64 cycbufread_addr;
	int cycbufread_size;
	CVI_U64 vqebuf_addr;
	int vqebuf_size;
	CVI_U64 tinyalsaReadBuf;
	int tinyalsaReadBuf_size;
	//for resample handler
	CVI_U64 resample_handler_addr;
	int resample_handler_size;
	//for share memory
	int sharebuf_index;
	int bsharebuf_first;
	RESAMPLE_INFO ResInfo;
} ST_AIN_MSG;

typedef struct _st_aout_msg {
	int AoDev;
	int AoChn;
	//for channel buffer
	//CVI_U64 cycbufwrite_addr;
	//int cycbufwrite_size;
	int sizebytes;
	//for share memory
	int sharebuf_index;
	int bsharebuf_first;
	//for resample handler
	CVI_U64 resample_handler_addr;
	int resample_handler_size;
	RESAMPLE_INFO ResInfo;
} ST_AOUT_MSG;

typedef struct _st_aenc_msg {
	int AenChn;
	//for userspace mem buffer
	CVI_U64 EncBuff;
	CVI_U64 EncBuff_aec;
	CVI_U64 EncInBuff;
	CVI_U64 EncVqeBuff;

	int EncBuff_size;
	int EncBuff_aec_size;
	int EncInBuff_size;
	int EncVqeBuff_size;
	//for share buffer
	int sharebuf_index;
	int bsharebuf_first;
} ST_AENC_MSG;

typedef struct _st_adec_msg {
	int AdecChn;
	//for userspace mem buffer
	CVI_U64 DecBuff;
	CVI_U64 DecReadBuff;

	int DecBuff_size;
	int DecReadBuff_size;
	//for share memory
	int sharebuf_index;
	int bsharebuf_first;
} ST_ADEC_MSG;
struct cviaudio_drv_ops {
//	void	(*clk_get)(struct cvi_vcodec_device *vdev);
};
#define CVIAUDIO_DRV_IOCTL_MAGIC			'A'
#define CVIAUDIO_CMD_MASK	400

//define for cviaudio_adec_interface  commands
//for share buffer
#define CVIAUDIO_INIT_CHECK				_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 1))
#define CVIAUDIO_DEINIT_CHECK				_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 2))
#define CVIAUDIO_GET_MMAP_SIZE				_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 3))
#define CVIAUDIO_AIN_CHN_BUFFER_ACQUIRE			_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 6))
#define CVIAUDIO_AIN_ALSA_READ_BUFFER_ACQUIRE		_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 7))
#define CVIAUDIO_AIN_SHAREBUFFER_INIT			_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 8))
#define CVIAUDIO_AIN_RESAMPLE_HANDLER_ACQUIRE		_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 9))
#define CVIAUDIO_AIN_RESAMPLE_HANDLER_ATTACH		_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 10))
#define CVIAUDIO_AENC_SHAREBUFFER_INIT			_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 11))
#define CVIAUDIO_AENC_CHN_BUFFER_ACQUIRE		_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 12))
#define CVIAUDIO_AOUT_SHAREBUFFER_INIT			_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 13))
#define CVIAUDIO_ADEC_SHAREBUFFER_INIT			_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 16))
#define CVIAUDIO_ADEC_CHN_BUFFER_ACQUIRE		_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 17))
#define CVIAUDIO_AOUT_RESAMPLE_HANDLER_ACQUIRE		_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 18))
#define CVIAUDIO_AIN_CHN_BUFFER_RELEASE			_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 19))
#define CVIAUDIO_AIN_ALSA_READ_BUFFER_RELEASE		_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 20))
#define CVIAUDIO_AENC_CLEARCHNBUF			_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 21))
#define CVIAUDIO_ADEC_CLEARCHNBUF			_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 22))
//add SSP RTOS cmd
//for SSP algorithm in RTOS
#define CVIAUDIO_IOCTL_SSP_INIT				_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 30))
#define CVIAUDIO_IOCTL_SSP_DEINIT			_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 31))
#define CVIAUDIO_IOCTL_SSP_PROC				_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 32))
#define CVIAUDIO_IOCTL_SSP_SPK_INIT			_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 33))
#define CVIAUDIO_IOCTL_SSP_SPK_DEINIT			_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 34))
#define CVIAUDIO_IOCTL_SSP_SPK_PROC			_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 35))
#define CVIAUDIO_IOCTL_SSP_DEBUG			_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 36))
#define CVIAUDIO_IOCTL_SSP_UNIT_TEST			_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 37))
#define CVIAUDIO_IOCTL_SSP_UNIT_TEST_STOP		_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 38))
#define CVIAUDIO_IOCTL_SSP_UPDATE_PCM_RECORD_CFG	_IO(CVIAUDIO_DRV_IOCTL_MAGIC, (CVIAUDIO_CMD_MASK + 39))
//additional RTOS command
//for block mode command from user space ---- start
typedef struct _st_cviaudio_mailbox_block {
	CVI_U64 u64RevMask;
	CVI_U64 AinVqeCfgPhy;
} ST_CVIAUDIO_MAILBOX_BLOCK;

typedef struct _st_civaudio_block_frame {
	CVI_U64 u64RevMask;
	CVI_U64 mic_in_addr;
	CVI_U64 ref_in_addr;
	CVI_U64 output_addr;
} ST_CVIAUDIO_BLOCK_FRAME;
//for block mode command from user space ---- end
#define CVIAUDIO_RTOS_CMD_SSP_UNIT_TEST_BLOCK_MODE_INIT	0x09
#define CVIAUDIO_RTOS_CMD_SSP_UNIT_TEST_BLOCK_MODE_GET	0x0A
#define CVIAUDIO_RTOS_CMD_SSP_INIT_BLOCK 0x0B
#define CVIAUDIO_RTOS_CMD_SSP_PROC_BLOCK 0x0C
#define CVIAUDIO_RTOS_CMD_SSP_DEINIT_BLOCK 0x0D



#endif	/* __CVIAUDIO_IOCTL_CMD_H__ */
