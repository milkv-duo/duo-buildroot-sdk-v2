//#define _GNU_SOURCE

#include "cvi_audio.h"
#include "cvi_tdl.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>

#include <pthread.h>
#include <termios.h>
#include <dirent.h>

#include <iostream>
#include <map>
#include <queue>
#include <sstream>
#include <string>
#include <vector>

#include "linux/cvi_defines.h"
#include "cvi_audio.h"


#define ACODEC_ADC "/dev/cvitekaadc"

#define CVI_SAMPLE_RATE 8000            // 采样率8kHz
#define CVI_PERIOD_SIZE 640             // 每帧 - 采样数

//#define CVI_SAMPLE_RATE 16000            // 采样率8kHz
//#define CVI_PERIOD_SIZE 320              // 每帧 - 采样数

#define CVI_GET_FRAME_MODE -1           // AI数据阻塞读取

#define CVI_AUDIO_BITS 2                // 每个采样点16bit = 2B
#define CVI_FRAME_SIZE (CVI_PERIOD_SIZE * CVI_AUDIO_BITS)     // 每次 [AI传入]/[AO传出] 数据大小

#define TPU_SECOND 2                    // AI解析单次传入N秒数据 - 用于计算BUF大小
#define TPU_FRAME_SIZE (TPU_SECOND * CVI_SAMPLE_RATE * CVI_AUDIO_BITS) // N秒 - 缓冲大小

#define AI_RECORD 0                     // 原始录音保存
#define TPU_RECORD 1                    // TPU事件保存

// AI输出 -> 字符串
static const char* enumStr[] = { "background", "help 911", "help me", "emergency", "scream", "gunshot", "glass"};

// 帧缓存队列, 时钟缓冲最后N秒的数据
static std::queue<std::array<int, CVI_FRAME_SIZE>> que;
bool gRun = true;     // signal

// ----------------------------------------------------------------------------
#define MUTEXAUTOLOCK_INIT(mutex) pthread_mutex_t AUTOLOCK_##mutex = PTHREAD_MUTEX_INITIALIZER;

#define MutexAutoLock(mutex, lock)                                                \
  __attribute__((cleanup(AutoUnLock))) pthread_mutex_t *lock = &AUTOLOCK_##mutex; \
  pthread_mutex_lock(lock);

__attribute__((always_inline)) inline void AutoUnLock(void* mutex) {
    pthread_mutex_unlock(*(pthread_mutex_t**)mutex);
}
// ----------------------------------------------------------------------------
// Init cviai handle.
cvitdl_handle_t tdl_handle = NULL;
MUTEXAUTOLOCK_INIT(ResultMutex);
static void SampleHandleSig(CVI_S32 signo) {
    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);

    if (SIGINT == signo || SIGTERM == signo) {
        gRun = false;
    }
}

void* thread_audio(void* arg) {
    CVI_S32 s32Ret;
    AUDIO_FRAME_S stFrame;
    AEC_FRAME_S stAecFrm;

    std::array<int, CVI_FRAME_SIZE> buffer_temp;
#if AI_RECORD
    FILE* fpAi = fopen("./record.pcm", "wb+");
    if (!fpAi) {
        printf("file open fail: record.pcm\n");
    }
#endif
    while (gRun) {
        s32Ret = CVI_AI_GetFrame(0, 0, &stFrame, &stAecFrm, CVI_GET_FRAME_MODE);  // Get audio frame
        if (s32Ret != CVI_TDL_SUCCESS) {
            printf("CVI_AI_GetFrame --> none!!\n");
            continue;
        }
        else {
            MutexAutoLock(ResultMutex, lock);
            std::copy(stFrame.u64VirAddr[0], stFrame.u64VirAddr[0] + CVI_FRAME_SIZE, buffer_temp.begin());
            que.push(buffer_temp);
            que.pop();
#if AI_RECORD
            //printf("write: len = %d, %d\r\n", CVI_FRAME_SIZE, stFrame.u32Len * 2);
            fwrite(stFrame.u64VirAddr[0], 1, stFrame.u32Len * 2, fpAi);
#endif
        }
    }

#if AI_RECORD
    fclose(fpAi);
#endif
    s32Ret = CVI_AI_ReleaseFrame(0, 0, &stFrame, &stAecFrm);
    if (s32Ret != CVI_TDL_SUCCESS) printf("CVI_AI_ReleaseFrame Failed!!\n");

    pthread_exit(NULL);
}

std::string GetFmtTime()
{
    static struct timeval s_tv = {
        .tv_sec = 0,
        .tv_usec = 0
    };
    if (s_tv.tv_sec == 0)
        gettimeofday(&s_tv, NULL);

    char szData[64] = { 0 };
    struct timeval tv;
    gettimeofday(&tv, NULL);
    tv.tv_sec -= s_tv.tv_sec;
    struct tm* t = localtime(&tv.tv_sec);
    //snprintf(szData, sizeof(szData), "%04d-%02d-%02d %02d:%02d:%02d.%03ld",
    //    t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, tv.tv_usec / 1000);
    snprintf(szData, sizeof(szData), "%02d:%02d:%02d.%03ld", t->tm_hour, t->tm_min, t->tm_sec, tv.tv_usec / 1000);
    return szData;
}

// Get frame and set it to global buffer
void* thread_infer(void* arg) {
    uint32_t loop = CVI_SAMPLE_RATE / CVI_PERIOD_SIZE * TPU_SECOND;  // 3 seconds
    uint32_t size = CVI_PERIOD_SIZE * CVI_AUDIO_BITS;       // PCM_FORMAT_S16_LE (2bytes)

    // Set video frame interface
    CVI_U8 buffer[TPU_FRAME_SIZE];  // 3 seconds
    memset(buffer, 0, TPU_FRAME_SIZE);
    VIDEO_FRAME_INFO_S Frame;
    Frame.stVFrame.pu8VirAddr[0] = buffer;      // Global buffer
    Frame.stVFrame.u32Height = 1;
    Frame.stVFrame.u32Width = TPU_FRAME_SIZE;  // TPU

    printf("SAMPLE_RATE %d, TPU_FRAME_SIZE:%d, CVI_FRAME_SIZE:%d \n", CVI_SAMPLE_RATE, TPU_FRAME_SIZE, CVI_FRAME_SIZE);

    // classify the sound result
    int pre_label = -1;
    int maxval_sound = 0;

    std::array<int, CVI_FRAME_SIZE> buffer_temp;

    while (gRun) {
        {
            MutexAutoLock(ResultMutex, lock);
            for (uint32_t i = 0; i < loop; i++) {
                // memcpy(buffer + i * size, (CVI_U8 *)que[i],
                //          size);  // Set the period size date to global buffer
                buffer_temp = que.front();
                que.pop();
                std::copy(std::begin(buffer_temp), std::end(buffer_temp), buffer + i * size);
                que.push(buffer_temp);
            }
        }

        //int16_t* psound = (int16_t*)buffer;
        //float meanval = 0;
        //for (int i = 0; i < SAMPLE_RATE * SECOND; i++) {
        //    meanval += psound[i];
        //    if (psound[i] > maxval_sound) {
        //        maxval_sound = psound[i];
        //    }
        //}
        // printf("maxvalsound:%d,meanv:%f\n", maxval_sound, meanval / (SAMPLE_RATE * SECOND));
        int ret = CVI_TDL_SoundClassification(tdl_handle, &Frame, &pre_label);  // Detect the audio

        if (ret == CVI_TDL_SUCCESS) {
            printf("pre_label: %d\n", pre_label);
            // static const char* enumStr[] = { "background", "help 911", "help me", "emergency", "scream", "gunshot", "glass"};
            //if (pre_label != 0) {
            if (pre_label >= 1 && pre_label <= 3) {
                printf("[%s] esc class: %s\n", GetFmtTime().c_str(), enumStr[pre_label]);
#if TPU_RECORD
                static int index = 0;
                char output_cur[128];
                sprintf(output_cur, "/mnt/data/yzx/infer/sound/err_data/help_me/tpu-%d-%s.raw", index++, enumStr[pre_label]);
                // sprintf(output_cur, "%s%s", output_cur, ".raw");
                FILE* fp = fopen(output_cur, "wb");
                if (fp) {
                    printf("to write:%s\n", output_cur);
                    fwrite((char*)buffer, 1, TPU_FRAME_SIZE, fp);
                    fclose(fp);
                }
#endif
                usleep(2500 * 1000);
            }
            else {
                usleep(250 * 1000);
            }

        }
        else {
            printf("detect failed\n");
            gRun = false;
        }
    }

    pthread_exit(NULL);
}

static CVI_BOOL _update_agc_anr_setting(AI_TALKVQE_CONFIG_S* pstAiVqeTalkAttr)
{
    if (pstAiVqeTalkAttr == NULL)
        return CVI_FALSE;

    pstAiVqeTalkAttr->u32OpenMask |= (NR_ENABLE | AGC_ENABLE | DCREMOVER_ENABLE);

    AUDIO_AGC_CONFIG_S st_AGC_Setting;
    AUDIO_ANR_CONFIG_S st_ANR_Setting;

    st_AGC_Setting.para_agc_max_gain = 0;
    st_AGC_Setting.para_agc_target_high = 2;
    st_AGC_Setting.para_agc_target_low = 72;
    st_AGC_Setting.para_agc_vad_ena = CVI_TRUE;
    st_ANR_Setting.para_nr_snr_coeff = 15;
    st_ANR_Setting.para_nr_init_sile_time = 0;

    pstAiVqeTalkAttr->stAgcCfg = st_AGC_Setting;
    pstAiVqeTalkAttr->stAnrCfg = st_ANR_Setting;

    pstAiVqeTalkAttr->para_notch_freq = 0;
    printf("pstAiVqeTalkAttr:u32OpenMask[0x%x]\n", pstAiVqeTalkAttr->u32OpenMask);
    return CVI_TRUE;
}

static CVI_BOOL _update_aec_setting(AI_TALKVQE_CONFIG_S* pstAiVqeTalkAttr)
{
    if (pstAiVqeTalkAttr == NULL)
        return CVI_FALSE;

    AI_AEC_CONFIG_S default_AEC_Setting;
    memset(&default_AEC_Setting, 0, sizeof(AI_AEC_CONFIG_S));
    default_AEC_Setting.para_aec_filter_len = 13;
    default_AEC_Setting.para_aes_std_thrd = 37;
    default_AEC_Setting.para_aes_supp_coeff = 60;
    pstAiVqeTalkAttr->stAecCfg = default_AEC_Setting;
    pstAiVqeTalkAttr->u32OpenMask = LP_AEC_ENABLE | NLP_AES_ENABLE | NR_ENABLE | AGC_ENABLE;
    printf("pstAiVqeTalkAttr:u32OpenMask[0x%x]\n", pstAiVqeTalkAttr->u32OpenMask);
    return CVI_FALSE;
}

CVI_S32 SET_AUDIO_ATTR(CVI_VOID) {
    int AiDev = 0;/*only support 0 dev */
    int AiChn = 0;
    // STEP 1: cvitek_audin_set
    //_update_audin_config
    AIO_ATTR_S AudinAttr;
    AudinAttr.enSamplerate = (AUDIO_SAMPLE_RATE_E)CVI_SAMPLE_RATE;
    AudinAttr.u32ChnCnt = 1;
    AudinAttr.enSoundmode = AUDIO_SOUND_MODE_MONO;
    AudinAttr.enBitwidth = AUDIO_BIT_WIDTH_16;
    AudinAttr.enWorkmode = AIO_MODE_I2S_MASTER;
    AudinAttr.u32EXFlag = 0;
    AudinAttr.u32FrmNum = 2;                    /* only use in bind mode */
    AudinAttr.u32PtNumPerFrm = CVI_PERIOD_SIZE;  // sample rate / fps
    AudinAttr.u32ClkSel = 0;
    AudinAttr.enI2sType = AIO_I2STYPE_INNERCODEC;

    CVI_S32 s32Ret;
    // STEP 2:cvitek_audin_uplink_start
    //_set_audin_config
    s32Ret = CVI_AI_SetPubAttr(AiDev, &AudinAttr);
    if (s32Ret != CVI_TDL_SUCCESS) printf("CVI_AI_SetPubAttr failed with %#x!\n", s32Ret);

    s32Ret = CVI_AI_Enable(AiDev);
    if (s32Ret != CVI_TDL_SUCCESS) printf("CVI_AI_Enable failed with %#x!\n", s32Ret);

    s32Ret = CVI_AI_EnableChn(AiDev, AiChn);
    if (s32Ret != CVI_TDL_SUCCESS) printf("CVI_AI_EnableChn failed with %#x!\n", s32Ret);

    // 声音增益: 默认0 - 太大会有噪声
    s32Ret = CVI_AI_SetVolume(AiDev, (CVI_SAMPLE_RATE == 8000) ? 8 : 2);
    if (s32Ret != CVI_TDL_SUCCESS) printf("CVI_AI_SetVolume failed with %#x!\n", s32Ret);

    bool bVqe = true;
    if (bVqe == true) {
        AI_TALKVQE_CONFIG_S stAiVqeTalkAttr;
        AI_TALKVQE_CONFIG_S* pstAiVqeTalkAttr = (AI_TALKVQE_CONFIG_S*)&stAiVqeTalkAttr;
        pstAiVqeTalkAttr->s32WorkSampleRate = AudinAttr.enSamplerate;
        _update_agc_anr_setting(pstAiVqeTalkAttr);
        _update_aec_setting(pstAiVqeTalkAttr);

        s32Ret = CVI_AI_SetTalkVqeAttr(AiDev, AiChn, 0, 0, &stAiVqeTalkAttr);
        if (s32Ret != CVI_SUCCESS) {
            printf("[error],[%s],[line:%d],\n", __func__, __LINE__);
        }

        s32Ret = CVI_AI_EnableVqe(AiDev, AiChn);
        if (s32Ret != CVI_SUCCESS) {
            printf("[error],[%s],[line:%d],\n", __func__, __LINE__);
        }
    }

    printf("SET_AUDIO_ATTR success!!\n");
    return CVI_TDL_SUCCESS;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: %s MODEL_PATH \n", argv[0]);
        return CVI_TDL_FAILURE;
    }

    // Set signal catch
    signal(SIGINT, SampleHandleSig);
    signal(SIGTERM, SampleHandleSig);

    uint32_t loop = CVI_SAMPLE_RATE / CVI_PERIOD_SIZE * TPU_SECOND;
    for (uint32_t i = 0; i < loop; i++) {
        std::array<int, CVI_FRAME_SIZE> tmp_arr = { 0 };
        que.push(tmp_arr);
    }

    CVI_S32 ret = CVI_TDL_SUCCESS;
    if (CVI_AUDIO_INIT() == CVI_TDL_SUCCESS) {
        printf("CVI_AUDIO_INIT success!!\n");
    }
    else {
        printf("CVI_AUDIO_INIT failure!!\n");
        return 0;
    }

    SET_AUDIO_ATTR();

    ret = CVI_TDL_CreateHandle3(&tdl_handle);

    if (ret != CVI_TDL_SUCCESS) {
        printf("Create ai handle failed with %#x!\n", ret);
        return ret;
    }

    cvitdl_sound_param param =
        CVI_TDL_GetSoundClassificationParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_SOUNDCLASSIFICATION);
    param.hop_len = 128;
    param.fix = true;

    ret = CVI_TDL_SetSoundClassificationParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_SOUNDCLASSIFICATION,
        param);

    ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_SOUNDCLASSIFICATION, argv[1]);
    if (ret != CVI_TDL_SUCCESS) {
        printf("CVI_TDL_OpenModel Fail: %#x!\n", ret);
        CVI_TDL_DestroyHandle(tdl_handle);
        return ret;
    }
    // 阈值默认值 0.5
    ret = CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_SOUNDCLASSIFICATION, 0.5);
    if (ret != CVI_SUCCESS) {
        printf("set threshold failed %#x!\n", ret);
        return ret;
    }
    float threshold = 0;
    CVI_TDL_GetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_SOUNDCLASSIFICATION, &threshold);
    if (ret == CVI_SUCCESS) {
        printf("tdl threshold %.2f!\n", threshold);
    }

    pthread_t pcm_output_thread, get_sound;
    pthread_create(&pcm_output_thread, NULL, thread_infer, NULL);
    pthread_create(&get_sound, NULL, thread_audio, NULL);

    pthread_join(pcm_output_thread, NULL);
    pthread_join(get_sound, NULL);

    CVI_TDL_DestroyHandle(tdl_handle);
    return 0;
}
