#ifndef __CVI_AUDIO_DNVQE_ADP_H__
#define __CVI_AUDIO_DNVQE_ADP_H__

#include <linux/cvi_type.h>

CVI_S32 CVI_AudDnVqe_Dlpath(CVI_CHAR *pChLibPath);

CVI_S32 CVI_AudDnVqe_Dlopen(CVI_VOID **pLibhandle, CVI_CHAR *pChLibName);

CVI_S32 CVI_AudDnVqe_Dlsym(CVI_VOID **pFunchandle, CVI_VOID *Libhandle,
			CVI_CHAR *pChFuncName);

CVI_S32 CVI_AudDnVqe_Dlclose(CVI_VOID *Libhandle);

#endif
