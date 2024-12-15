/*
 *	cvi_audio_uac.h  --  USB Audio Class Gadget API
 *
 */

#ifndef _CVI_AUDIO_UAC_H_
#define _CVI_AUDIO_UAC_H_

/* ------------------------------------------------------------------------
 * Debugging, printing and logging
 */
int cvi_audio_init_uac(void);
int cvi_audio_deinit_uac(void);
int cvi_audio_uac_uplink(void); //board to pc (treat cv18xx as mic )
int start_downlink_audio(void); //pc to board (treat cv18xx as speaker)
int stop_uplink_audio(void);
int stop_downlink_audio(void);


#endif /* _CVI_AUDIO_UAC_H_ */

