/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _CVITEK_SPACC_H_
#define _CVITEK_SPACC_H_

typedef enum SPACC_ALGO {
	SPACC_ALGO_AES,
	SPACC_ALGO_DES,
	SPACC_ALGO_TDES,
	SPACC_ALGO_SM4,
	SPACC_ALGO_SHA1,
	SPACC_ALGO_SHA256,
	SPACC_ALGO_BASE64,
} SPACC_ALGO_E;

typedef enum SPACC_ALGO_MODE {
	SPACC_ALGO_MODE_ECB,
	SPACC_ALGO_MODE_CBC, //cv180x Not Supported
	SPACC_ALGO_MODE_CTR, //cv180x Not Supported
	SPACC_ALGO_MODE_OFB, //cv180x Not Supported
} SPACC_ALGO_MODE_E;

typedef enum SPACC_KEY_SIZE {
	SPACC_KEY_SIZE_64BITS,
	SPACC_KEY_SIZE_128BITS,
	SPACC_KEY_SIZE_192BITS,
	SPACC_KEY_SIZE_256BITS,
} SPACC_KEY_SIZE_E;

typedef enum SPACC_ACTION {
	SPACC_ACTION_ENCRYPTION,
	SPACC_ACTION_DECRYPT,
} SPACC_ACTION_E;

typedef struct spacc_base64_action {
	SPACC_ACTION_E action;
} spacc_base64_action_s;

typedef struct spacc_aes_config {
	// data config
	void *src; //src phy address
	size_t len;

	// spacc config
	unsigned char key[32];
	unsigned char iv[16];
	SPACC_ALGO_MODE_E mode;
	SPACC_KEY_SIZE_E size;
	SPACC_ACTION_E action;
} spacc_aes_config_s;

typedef struct spacc_des_config {
	unsigned char key[24];
	unsigned char iv[16];
	SPACC_ALGO_MODE_E mode;
	SPACC_ACTION_E action;
} spacc_des_config_s;

typedef spacc_aes_config_s spacc_sm4_config_s;
typedef spacc_des_config_s spacc_tdes_config_s;

#define IOCTL_SPACC_BASE					'S'
#define IOCTL_SPACC_CREATE_MEMPOOL			_IOW(IOCTL_SPACC_BASE, 1, unsigned int)
#define IOCTL_SPACC_GET_MEMPOOL_SIZE		_IOR(IOCTL_SPACC_BASE, 2, unsigned int)

#define IOCTL_SPACC_SHA256_ACTION			_IO(IOCTL_SPACC_BASE, 5)
#define IOCTL_SPACC_SHA1_ACTION				_IO(IOCTL_SPACC_BASE, 6)
#define IOCTL_SPACC_BASE64_ACTION			_IOW(IOCTL_SPACC_BASE, 7, spacc_base64_action_s)
#define IOCTL_SPACC_AES_ACTION				_IOW(IOCTL_SPACC_BASE, 8, spacc_aes_config_s)
#define IOCTL_SPACC_SM4_ACTION				_IOW(IOCTL_SPACC_BASE, 9, spacc_sm4_config_s)
#define IOCTL_SPACC_DES_ACTION				_IOW(IOCTL_SPACC_BASE, 10, spacc_des_config_s)
#define IOCTL_SPACC_TDES_ACTION				_IOW(IOCTL_SPACC_BASE, 11, spacc_tdes_config_s)

#endif // _CVITEK_SPACC_H_
