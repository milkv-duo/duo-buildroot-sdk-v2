#ifndef __VOLUME_CTRL_H__
#define __VOLUME_CTRL_H__


typedef void *VOL_HANDLE;

#define MAX_VOLUME (100)
#define MIN_VOLUME (0)

VOL_HANDLE cvitek_volume_ctrl_create(void);
int cvitek_set_volume_index(VOL_HANDLE volHandle, int index);
int cvitek_get_volume_index(VOL_HANDLE volHandle);
int vol_ctrl_process(VOL_HANDLE volHandle, short *sIn, short *sOut, int len);
void cvitek_volume_ctrl_destroy(VOL_HANDLE volHandle);

#endif //__VOLUME_CTRL_H__
