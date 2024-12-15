#ifndef __CVI_AUDIO_SHM_H__
#define __CVI_AUDIO_SHM_H__
/* cvi_audio_rpc.h is for share memory mechanism in audio rpc
 * , in order to switch the rpc share memory mode, you need to lessen
 * rpc commands between client & server, the structure and function
 * declaration are list among this files
 */
#include <sys/queue.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <stdbool.h>
#include <linux/cvi_defines.h>
//#include "cvi_common.h"
#include "cvi_comm_aio.h"
#include <linux/cvi_type.h>


#define RPC_AUD_STREAM_SHARED_MEMORY_SIZE (100*1000)
#define RPC_AUD_STREAM_ONEFRMAE_MAX_SIZE (10*1000)
#define MAX_RPC_AUD_STREAM_NAME_LEN (50)
#define MAX_RPC_AUD_STREAM_NAME_LEN (50)
//for ain
#define RPC_AI_STREAM_SEM_READ_NAME "rpc_ai_sharem_sem_read"
#define RPC_AI_STREAM_SEM_WRITE_NAME "rpc_ai_sharem_sem_write"
#define RPC_AI_STREAM_SHM_NAME "/tmp/rpc_ai_stream_sharem_shm"
typedef struct {
	AUDIO_SOUND_MODE_E  enSoundmode;
	CVI_U32 u32Len;
	CVI_U32 u32Seq;
	CVI_U64 u64TimeStamp;
	CVI_U64 u64PhyAddr;
	CVI_S32 s32SyncCode;
} AUD_AIN_FRAME_CBLK;

typedef struct {
	int		Aichn;
	int		AiDev;
	int		g_shm_fd;
	key_t	g_shm_key;
	int		g_shm_id;
	sem_t	*m_pSemRead;
	sem_t	*m_pSemWrite;
	int		m_queue_index;
	int		m_queue_cap;
	char	*g_shm_ptr;
	CVI_U8	*streamPtr;
	int		bRuning;
	int		bInit;
	char cAinStreamShmName[MAX_RPC_AUD_STREAM_NAME_LEN];
	char cAinSemReadName[MAX_RPC_AUD_STREAM_NAME_LEN];
	char cAinSemWriteName[MAX_RPC_AUD_STREAM_NAME_LEN];
} AI_SHARE_MEM_INFO;

extern AI_SHARE_MEM_INFO stAinStreamShmInfo[CVI_AUD_MAX_CHANNEL_NUM];


//for aenc
#define RPC_AUD_STREAM_SEM_READ_NAME "rpc_aenc_shm_sem_read"
#define RPC_AUD_STREAM_SEM_WRITE_NAME "rpc_aenc_shm_sem_write"
#define RPC_AUD_STREAM_SHM_NAME "/tmp/rpc_aenc_getstream_shm"


typedef struct {
	CVI_U32 u32Len;
	CVI_U32 u32Seq;
	CVI_U64 u64TimeStamp;
	CVI_U64 u64PhyAddr;
	CVI_S32 s32SyncCode;
} AUD_AENC_STREAM_CBLK;

typedef struct {
	int	channel;
	int		g_shm_fd;
	key_t	g_shm_key;
	int		g_shm_id;
	sem_t  *m_pSemRead;
	sem_t  *m_pSemWrite;
	int	m_queue_index;
	int	m_queue_cap;
	char	*g_shm_ptr;
	CVI_U8	*streamPtr;
	int	bRuning;
	int	bInit;
	char cAencSemReadName[MAX_RPC_AUD_STREAM_NAME_LEN];
	char cAencSemWriteName[MAX_RPC_AUD_STREAM_NAME_LEN];
	char cAencStrmShmName[MAX_RPC_AUD_STREAM_NAME_LEN];
} AUD_STREAM_SHARE_MEM_INFO;


extern AUD_STREAM_SHARE_MEM_INFO stAencStreamShmInfo[AENC_MAX_CHN_NUM];

//for ao
#define RPC_AO_STREAM_SEM_READ_NAME "rpc_ao_shm_sem_read"
#define RPC_AO_STREAM_SEM_WRITE_NAME "rpc_ao_shm_sem_write"
#define RPC_AO_STREAM_SHM_NAME "/tmp/rpc_ao_getstream_shm"

typedef struct {
	AUDIO_BIT_WIDTH_E   enBitwidth;
	AUDIO_SOUND_MODE_E  enSoundmode;
	CVI_U32 u32Len;
	CVI_U32 u32Seq;
	CVI_U64 u64TimeStamp;
	CVI_U64 u64PhyAddr;
	CVI_S32 s32SyncCode;
} AUD_AO_FRAME_CBLK;

typedef struct {
	int		Aochn;
	int		AoDev;
	int		g_shm_fd;
	key_t	g_shm_key;
	int		g_shm_id;
	sem_t	*m_pSemRead;
	sem_t	*m_pSemWrite;
	int		m_queue_index;
	int		m_queue_cap;
	char	*g_shm_ptr;
	CVI_U8	*streamPtr;
	int		bRuning;
	int		bInit;
	char cAoStreamShmName[MAX_RPC_AUD_STREAM_NAME_LEN];
	char cAoSemReadName[MAX_RPC_AUD_STREAM_NAME_LEN];
	char cAoSemWriteName[MAX_RPC_AUD_STREAM_NAME_LEN];
} AO_SHARE_MEM_INFO;

extern AO_SHARE_MEM_INFO stAoStreamShmInfo[CVI_AUD_MAX_CHANNEL_NUM];


//for adec
#define RPC_ADEC_STREAM_SEM_READ_NAME "rpc_adec_shm_sem_read"
#define RPC_ADEC_STREAM_SEM_WRITE_NAME "rpc_adec_shm_sem_write"
#define RPC_ADEC_STREAM_SHM_NAME "/tmp/rpc_adec_getstream_shm"

typedef struct {
	//AUDIO_SOUND_MODE_E  enSoundmode;
	CVI_U32 u32Len;
	CVI_U32 u32Seq;
	CVI_U64 u64TimeStamp;
	//CVI_U64 u64PhyAddr;
	CVI_S32 s32SyncCode;
} AUD_ADEC_FRAME_CBLK;

typedef struct {
	int		AdChn;
	int		AdDev;
	int		g_shm_fd;
	key_t	g_shm_key;
	int		g_shm_id;
	sem_t	*m_pSemRead;
	sem_t	*m_pSemWrite;
	int		m_queue_index;
	int		m_queue_cap;
	char	*g_shm_ptr;
	CVI_U8	*streamPtr;
	int		bRuning;
	int		bInit;
	char cAdecStreamShmName[MAX_RPC_AUD_STREAM_NAME_LEN];
	char cAdecSemReadName[MAX_RPC_AUD_STREAM_NAME_LEN];
	char cAdecSemWriteName[MAX_RPC_AUD_STREAM_NAME_LEN];
} ADEC_SHARE_MEM_INFO;

extern ADEC_SHARE_MEM_INFO stAdecStreamShmInfo[CVI_AUD_MAX_CHANNEL_NUM];



//function declaration
CVI_U64 rpc_aud_get_boot_time(void);
//for aenc
int checkIsAencStreamHead(AUD_AENC_STREAM_CBLK *pstCblk);
int init_aenc_share_memory(AENC_CHN AeChn);
int deinit_aenc_share_memory(AENC_CHN AeChn);
//for ain
int checkIsAinFrameHead(AUD_AIN_FRAME_CBLK *pstCblk);
int init_ain_share_memory(AI_CHN AiChn);
int deinit_ain_share_memory(AI_CHN AiChn);

int checkIsAoFrameHead(AUD_AO_FRAME_CBLK *pstCblk);
int init_ao_share_memory(AO_CHN AoChn);
int deinit_ao_share_memory(AO_CHN AoChn);

int checkIsAdecFrameHead(AUD_ADEC_FRAME_CBLK *pstCblk);
int init_adec_share_memory(ADEC_CHN AdChn);
int deinit_adec_share_memory(ADEC_CHN AdChn);

#endif
