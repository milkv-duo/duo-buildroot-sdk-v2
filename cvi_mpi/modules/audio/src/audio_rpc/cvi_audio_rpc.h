#ifndef __CVI_AUDIO_RPC_H__
#define __CVI_AUDIO_RPC_H__

#include <stdbool.h>
#include <linux/cvi_type.h>
#include "cvi_comm_adec.h"
#include "cvi_comm_aenc.h"
#include "cvi_comm_aio.h"
#include "linux/cvi_errno.h"
#include "cvi_debug.h"
#include "cvi_audio.h"


#define AUD_RPC_SERVER_CNT 5 //extend from 3 -> 5 (available aio more channel)
#define AUD_RPC_URL_AIO 0
#define AUD_RPC_URL_AENC 1
#define AUD_RPC_URL_ADEC 2
#define AUD_RPC_URL_AIO_CH1 3
#define AUD_RPC_URL_AIO_CH2 4

#define RPC_URL_AUDCH0 "ipc:///tmp/cvitek_audaio.ipc"
#define RPC_URL_AUDCH1 "ipc:///tmp/cvitek_audaenc.ipc"
#define RPC_URL_AUDCH2 "ipc:///tmp/cvitek_audadec.ipc"
#define RPC_URL_AUDAIO_CH1 "ipc:///tmp/cvitek_audaio_ch1.ipc"
#define RPC_URL_AUDAIO_CH2 "ipc:///tmp/cvitek_audaio_ch2.ipc"

#define AUD_RPC_MAGIC 0x22446688


#define SET_MSG(mode, dev, chn, cmd) ((mode << 24) | (dev << 16) | (chn << 8) | cmd)
#define GET_MODE(msg) (msg >> 24)
#define GET_DEV(msg) ((msg >> 16) & 0xff)
#define GET_CHN(msg) ((msg >> 8) & 0xff)
#define GET_CMD(msg) (msg & 0xff)

#define x_info(format, args...) \
	printf("\033[1m\033[32m[INF|%s|%d] " format "\033[0m\n", __func__, __LINE__, ##args)


enum RPC_AUD_CMD {
	//AIN module command
	RPC_CMD_AI_SET_MOD_PARAM	= 0,//none
	RPC_CMD_AI_GET_MOD_PARAM	= 1,//none
	RPC_CMD_AUD_SYS_BIND	= 2,//ok->need test
	RPC_CMD_AUD_SYS_UNBIND	 = 3,//ok->need test
	RPC_CMD_AI_SET_PUB_ATTR = 4,//ok
	RPC_CMD_AI_GET_PUB_ATTR = 5,//OK
	RPC_CMD_AI_ENABLE = 6,//ok
	RPC_CMD_AI_DISABLE = 7,//ok->need test
	RPC_CMD_AI_ENABLE_CHN = 8,//ok
	RPC_CMD_AI_DISABLE_CHN = 9,//ok->need test
	RPC_CMD_AI_GET_FRAME = 10,//ok
	RPC_CMD_AI_RELEASE_FRAME = 11,//ok->need test
	RPC_CMD_AI_SET_CHN_PARAM = 12,//ok
	RPC_CMD_AI_GET_CHN_PARAM = 13,//ok
	RPC_CMD_AI_SET_VOLUME = 14,//ok
	RPC_CMD_AI_GET_VOLUME = 15,//ok
	RPC_CMD_AI_SET_RECORD_VQE_ATTR = 16,//not available
	RPC_CMD_AI_GET_RECORD_VQE_ATTR = 17,//not available
	RPC_CMD_AI_ENABLE_VQE = 18,//ok
	RPC_CMD_AI_DISABLE_VQE = 19,//ok
	RPC_CMD_AI_ENABLE_RESMP = 20,//ok->need test
	RPC_CMD_AI_DISABLE_RESMP = 21,//ok->need test
	RPC_CMD_AI_SET_TRACK_MODE = 22,//not available
	RPC_CMD_AI_GET_TRACK_MODE = 23,//not available
	RPC_CMD_AI_SAVE_FILE = 24,//not available(buggy)
	RPC_CMD_AI_QUERY_FILE_STATUS = 25,//not available(buggy)
	RPC_CMD_AI_CLR_PUB_ATTR = 26,//ok->need test
	RPC_CMD_AI_GET_FD = 27,//not available
	RPC_CMD_AI_ENABLE_AEC_REF_FRAME = 28,//not available
	RPC_CMD_AI_DISABLE_AEC_REF_FRAME = 29,//not available
	RPC_CMD_AI_SET_TALK_VQE_ATTR = 30,//ok
	RPC_CMD_AI_GET_TALK_VQE_ATTR = 31,//ok->need test
	//AOUT module command
	RPC_CMD_AO_SET_PUB_ATTR = 32,//ok
	RPC_CMD_AO_GET_PUB_ATTR = 33,//ok->need test
	RPC_CMD_AO_ENABLE = 34,//ok
	RPC_CMD_AO_DISABLE = 35,//ok
	RPC_CMD_AO_ENABLE_CHN = 36,//ok
	RPC_CMD_AO_DISABLE_CHN = 37,//ok
	RPC_CMD_AO_SEND_FRAME = 38,//ok
	RPC_CMD_AO_ENABLE_RESMP = 39,//ok need test
	RPC_CMD_AO_DISABLE_RESMP = 40,////ok need test
	RPC_CMD_AO_CLEAR_CHN_BUF = 41,//not available
	RPC_CMD_AO_QUERY_CHN_STAT = 42,//not available
	RPC_CMD_AO_PAUSE_CHN = 43,//not available
	RPC_CMD_AO_RESUME_CHN = 44,//not available
	RPC_CMD_AO_SET_VOLUME = 45,//ok
	RPC_CMD_AO_GET_VOLUME = 46,//ok
	RPC_CMD_AO_SET_MUTE = 47,//ok
	RPC_CMD_AO_GET_MUTE = 48,//ok
	RPC_CMD_AO_SET_TRACK_MODE = 49,//not available
	RPC_CMD_AO_GET_TRACK_MODE = 50,//not available
	RPC_CMD_AO_GET_FD = 51,//not available
	//AENC module command
	RPC_CMD_AENC_CREATE_CHN = 52,//ok->need test
	RPC_CMD_AENC_DESTROY_CHN = 53,//ok->need test
	PRC_CMD_AENC_SEND_FRAME = 54,//ok->need test
	RPC_CMD_AENC_GET_STREAM = 55,//ok
	RPC_CMD_AENC_RELEASE_STREAM = 56,//ok->need test
	RPC_CMD_AENC_GET_FD = 57,//not available
	RPC_CMD_AENC_GET_STREAM_BUF_INFO = 58,//not available now
	RPC_CMD_AENC_SET_MUTE = 59,//not available
	RPC_CMD_AENC_GET_MUTE = 60,//not available
	RPC_CMD_AENC_REGISTER_EXTERNAL_ENCODER = 61,//[TODO:]
	RPC_CMD_AENC_UNREGISTER_EXTERNAL_ENCODER = 62,//[TODO:]
	//ADEC module command
	RPC_CMD_ADEC_CREATE_CHN = 63,//ok->need test
	RPC_CMD_ADEC_DESTROY_CHN = 64,//ok->need test
	RPC_CMD_ADEC_SEND_STREAM = 65,//ok
	RPC_CMD_ADEC_CLEAR_CHN_BUF = 66,//not available
	RPC_CMD_ADEC_REGISTER_EXTERNAL_DECODER = 67,//[TODO:]
	RPC_CMD_ADEC_UNREGISTER_EXTERNAL_DECODER = 68,//[TODO:]
	RPC_CMD_ADEC_GET_FRAME = 69,//ok
	RPC_CMD_ADEC_RELEASE_FRAME = 70,//ok
	RPC_CMD_ADEC_SEND_END_OF_STREAM = 71,//ok->need test
	RPC_CMD_ADEC_QUERY_CHN_STAT = 72,//not available
	RPC_CMD_CHECK_INIT = 73,
	RPC_CMD_MAX
};

struct rpc_aud_msg {
	unsigned int msg;
	unsigned int magic;
	int result;
	int body_len;
	char body[0];
};

typedef int (*RPC_PROCESS_CALLBACK)(int cmd, struct rpc_aud_msg *pstRpcMsg, unsigned char *response, int *responselen);

//int rpc_client_send(unsigned char *buf, int size, int s32MilliSec, char **response, int *responselen);
//int rpc_client_response_proc(char *response, int len, void *args);
//audio mapping rpc api ------start
CVI_S32  rpc_client_aud_sys_bind(const MMF_CHN_S *pstSrcChn, const MMF_CHN_S *pstDestChn);
CVI_S32  rpc_client_aud_sys_unbind(const MMF_CHN_S *pstSrcChn, const MMF_CHN_S *pstDestChn);
CVI_S32 rpc_client_aud_ai_set_pub_attr(AUDIO_DEV AiDevId,
				const AIO_ATTR_S *pstAttr);
CVI_S32 rpc_client_aud_ai_get_pub_attr(AUDIO_DEV AiDevId,
					AIO_ATTR_S *pstAttr);
CVI_S32 rpc_client_aud_ai_enable(AUDIO_DEV AiDevId);
CVI_S32 rpc_client_aud_ai_disable(AUDIO_DEV AiDevId);
CVI_S32 rpc_client_aud_ai_enable_chn(AUDIO_DEV AiDevId, AI_CHN AiChn);
CVI_S32 rpc_client_aud_ai_disable_chn(AUDIO_DEV AiDevId, AI_CHN AiChn);
CVI_S32 rpc_client_aud_ai_get_frame(AUDIO_DEV AiDevId, AI_CHN AiChn,
			AUDIO_FRAME_S *pstFrm, AEC_FRAME_S *pstAecFrm,
			CVI_S32 s32MilliSec);
CVI_S32 rpc_client_aud_ai_release_frame(AUDIO_DEV AiDevId, AI_CHN AiChn,
			const AUDIO_FRAME_S *pstFrm, const AEC_FRAME_S *pstAecFrm);
CVI_S32 rpc_client_aud_aenc_get_stream(AENC_CHN AeChn,
			AUDIO_STREAM_S *pstStream,
			   CVI_S32 s32MilliSec);
CVI_S32 rpc_client_aud_aenc_release_stream(AENC_CHN AeChn,
				const AUDIO_STREAM_S *pstStream);
CVI_S32 rpc_client_aud_adec_send_stream(ADEC_CHN AdChn,
				const AUDIO_STREAM_S *pstStream,
				CVI_BOOL bBlock);
CVI_S32 rpc_client_aud_adec_get_frame(ADEC_CHN AdChn,
				AUDIO_FRAME_INFO_S *pstFrmInfo,
				CVI_BOOL bBlock);
CVI_S32 rpc_client_aud_adec_release_frame(ADEC_CHN AdChn,
				const AUDIO_FRAME_INFO_S *pstFrmInfo);
CVI_S32 rpc_client_aud_aenc_send_frame(AENC_CHN AeChn,
				const AUDIO_FRAME_S *pstFrm,
				const AEC_FRAME_S *pstAecFrm);
CVI_S32 rpc_client_aud_ai_set_volume(AUDIO_DEV AiDevId, CVI_S32 s32VolumeStep);
CVI_S32 rpc_client_aud_ai_get_volume(AUDIO_DEV AiDevId, CVI_S32 *ps32VolumeStep);
CVI_S32 rpc_client_aud_ao_enable(AUDIO_DEV AoDevId);
CVI_S32 rpc_client_aud_ao_disable(AUDIO_DEV AoDevId);
CVI_S32 rpc_client_aud_ao_set_pub_attr(AUDIO_DEV AoDevId,
				const AIO_ATTR_S *pstAttr);
CVI_S32 rpc_client_aud_ao_get_pub_attr(AUDIO_DEV AoDevId, AIO_ATTR_S *pstAttr);
CVI_S32 rpc_client_aud_ao_enable_chn(AUDIO_DEV AoDevId, AO_CHN AoChn);
CVI_S32 rpc_client_aud_ao_disable_chn(AUDIO_DEV AoDevId, AO_CHN AoChn);
CVI_S32 rpc_client_aud_ao_send_frame(AUDIO_DEV AoDevId,
			AO_CHN AoChn,
			const AUDIO_FRAME_S *pstData,
			CVI_S32 s32MilliSec);
CVI_S32 rpc_client_aud_ao_querychn_stat(AUDIO_DEV AoDevId, AO_CHN AoChn,
			    AO_CHN_STATE_S *pstStatus);
CVI_S32 rpc_client_aud_ao_set_volume(AUDIO_DEV AoDevId, CVI_S32 s32VolumeDb);
CVI_S32 rpc_client_aud_ao_get_volume(AUDIO_DEV AoDevId, CVI_S32 *ps32VolumeDb);
CVI_S32 rpc_client_aud_ai_set_chn_param(AUDIO_DEV AiDevId, AI_CHN AiChn,
			   const AI_CHN_PARAM_S *pstChnParam);
CVI_S32 rpc_client_aud_ai_get_chn_param(AUDIO_DEV AiDevId, AI_CHN AiChn,
			   AI_CHN_PARAM_S *pstChnParam);
CVI_S32 rpc_client_aud_ai_enable_vqe(AUDIO_DEV AiDevId, AI_CHN AiChn);
CVI_S32 rpc_client_aud_ai_disable_vqe(AUDIO_DEV AiDevId, AI_CHN AiChn);
CVI_S32 rpc_client_aud_ai_enable_resmp(AUDIO_DEV AiDevId, AI_CHN AiChn,
				AUDIO_SAMPLE_RATE_E enOutSampleRate);
CVI_S32 rpc_client_aud_ai_disable_resmp(AUDIO_DEV AiDevId, AI_CHN AiChn);
CVI_S32 rpc_client_aud_ai_clr_pub_attr(AUDIO_DEV AiDevId);

CVI_S32 rpc_client_aud_ai_set_talk_vqe_attr(AUDIO_DEV AiDevId, AI_CHN AiChn,
			AUDIO_DEV AoDevId, AO_CHN AoChn,
			const AI_TALKVQE_CONFIG_S *pstVqeConfig);

CVI_S32 rpc_client_aud_ai_get_talk_vqe_attr(AUDIO_DEV AiDevId, AI_CHN AiChn,
				AI_TALKVQE_CONFIG_S *pstVqeConfig);
CVI_S32 rpc_client_aud_ao_enable_resmp(AUDIO_DEV AoDevId, AO_CHN AoChn,
			   AUDIO_SAMPLE_RATE_E enInSampleRate);
CVI_S32 rpc_client_aud_ao_disable_resmp(AUDIO_DEV AoDevId, AO_CHN AoChn);
CVI_S32 rpc_client_aud_ao_set_mute(AUDIO_DEV AoDevId, CVI_BOOL bEnable,
			 const AUDIO_FADE_S *pstFade);
CVI_S32 rpc_client_aud_ao_get_mute(AUDIO_DEV AoDevId, CVI_BOOL *pbEnable,
			  AUDIO_FADE_S *pstFade);
CVI_S32 rpc_client_aud_aenc_create_chn(AENC_CHN AeChn, const AENC_CHN_ATTR_S *pstAttr);
CVI_S32 rpc_client_aud_aenc_destroy_chn(AENC_CHN AeChn);
CVI_S32 rpc_client_aud_adec_create_chn(ADEC_CHN AdChn,
				const ADEC_CHN_ATTR_S *pstAttr);
CVI_S32 rpc_client_aud_adec_destroy_chn(ADEC_CHN AdChn);
CVI_S32 rpc_client_aud_adec_send_end_of_stream(ADEC_CHN AdChn, CVI_BOOL bInstant);
CVI_S32 rpc_client_aud_check_init(CVI_BOOL *pbInit);
//audio mapping rpc api ------stop

int rpc_server_audio_init(void);
int rpc_server_audio_deinit(void);
bool isAudioMaster(void);

#endif
