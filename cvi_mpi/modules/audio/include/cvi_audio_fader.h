#ifndef __CVI_FADER_H__
#define __CVI_FADER_H__

typedef void *CVI_FADER_HANDLE;

typedef enum {
	FADE_MODE_FADEIN,
	FADE_MODE_FADEOUT,
	FADE_MODE_BUTT
} CVI_E_FADE_MODE;


//only for fade out
CVI_FADER_HANDLE cvi_faderInit(CVI_E_FADE_MODE mode, int channels, unsigned long sampleRate, int fadeTimeMs);
int cvi_fader_process(CVI_FADER_HANDLE pFaderHandle, short *data, int frames);
void cvi_fader_destroy(CVI_FADER_HANDLE pFaderHandle);
int cvi_fader_reset(CVI_FADER_HANDLE pFaderHandle);



#endif
