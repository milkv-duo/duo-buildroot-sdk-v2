/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_defines.h
 * Description:
 *
 */

#ifndef _ISP_DEFINES_H_
#define _ISP_DEFINES_H_

#include <stdlib.h>
#include <errno.h>
#include "isp_comm_inc.h"
#include "isp_debug.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define VIPIPE_TO_IDX(vi) vi

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN3
#define MIN3(a, b, c) (MIN(MIN(a, b), c))
#endif

#ifndef MINMAX
#define MINMAX(a, b, c) ((a < b) ? b : (a > c) ? c : a)
#endif

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

#define RING_ADD(num, val, max) (((num) + (val)) % (max))
#define CLIP_ADD(num, val, max) (MIN(((num) + (val)), (max)))


#define INIT_V(attr, param, value) {(attr).param = (value); }
#define INIT_V_ARRAY(attr, param, ...) do {\
	typeof((attr).param[0]) arr[] = { __VA_ARGS__ };\
	for (CVI_U32 x = 0; x < ARRAY_SIZE(arr); x++) {\
		(attr).param[x] = arr[x];\
	} \
} while (0)
#define INIT_A(attr, param, manual, ...) do {\
	typeof((attr).stManual.param) arr[] = { __VA_ARGS__ };\
	(attr).stManual.param = manual;\
	for (CVI_U32 x = 0; x < ISP_AUTO_ISO_STRENGTH_NUM; x++) {\
		(attr).stAuto.param[x] = arr[x];\
	} \
} while (0)
#define INIT_LV_A(attr, param, manual, ...) do {\
	typeof((attr).stManual.param) arr[] = { __VA_ARGS__ };\
	(attr).stManual.param = manual;\
	for (CVI_U32 x = 0; x < ISP_AUTO_LV_NUM; x++) {\
		(attr).stAuto.param[x] = arr[x];\
	} \
} while (0)

#define CHECK_DEV_VALUE(dev) do { \
	if (((dev) < 0) || ((dev) >= VI_MAX_DEV_NUM)) \
		return -ENODEV; \
} while (0)

#define ISP_IOCTL_ERR(fd, ec1) \
	ISP_LOG_ERR("IOCTL error fd = %d value = %d ptr = %p id = %x err = %s\n", \
		fd, ec1.value, ec1.ptr, ec1.id, strerror(errno))

#define ISP_CHECK_MIN(val, min) do { \
	if ((val) < (min)) { \
		(val) = (min); \
		printf("%s(%d) < min(%d), set to min\n", (#val), (val), (min)); \
	} \
} while (0)

#define ISP_CHECK_MAX(val, max) do { \
	if ((val) > (max)) { \
		(val) = (max); \
		printf("%s(%d) > max(%d), set to max\n", (#val), (val), (max)); \
	} \
} while (0)

#define	ISP_CHECK_RANGE(val, min, max) do { \
	ISP_CHECK_MIN((val), (min)); \
	ISP_CHECK_MAX((val), (max)); \
} while (0)

#define STRING(s) (#s)

#ifndef API_ATTR_CHECK_LOG_LEVEL
#define API_ATTR_CHECK_LOG_LEVEL	2
#endif // API_ATTR_CHECK_LOG_LEVEL

#if API_ATTR_CHECK_LOG_LEVEL == 0
#define ISP_VALUE_CHECK_ERR_LOG_V(param, value)
#define ISP_VALUE_CHECK_ERR_LOG_A1(param, index1, value)
#define ISP_VALUE_CHECK_ERR_LOG_A2(param, index1, index2, value)
#define ISP_VALUE_CHECK_ERR_LOG_A3(param, index1, index2, index3, value)
#elif API_ATTR_CHECK_LOG_LEVEL == 1
#define ISP_VALUE_CHECK_ERR_LOG_V(param, value) \
	ISP_LOG_ERR("Invalid value : 0x%x\n", value)
#define ISP_VALUE_CHECK_ERR_LOG_A1(param, index1, value) \
	ISP_LOG_ERR("Invalid value : 0x%x\n", value)
#define ISP_VALUE_CHECK_ERR_LOG_A2(param, index1, index2, value) \
	ISP_LOG_ERR("Invalid value : 0x%x\n", value)
#define ISP_VALUE_CHECK_ERR_LOG_A3(param, index1, index2, index3, value) \
	ISP_LOG_ERR("Invalid value : 0x%x\n", value)
#else
#define ISP_VALUE_CHECK_ERR_LOG_V(param, value) \
	ISP_LOG_ERR("Invalid param : %s, value : 0x%x\n", param, value)
#define ISP_VALUE_CHECK_ERR_LOG_A1(param, index1, value) \
	ISP_LOG_ERR("Invalid param : %s[%d], value : 0x%x\n", param, index1, value)
#define ISP_VALUE_CHECK_ERR_LOG_A2(param, index1, index2, value) \
	ISP_LOG_ERR("Invalid param : %s[%d][%d], value : 0x%x\n", param, index1, index2, value)
#define ISP_VALUE_CHECK_ERR_LOG_A3(param, index1, index2, index3, value) \
	ISP_LOG_ERR("Invalid param : %s[%d][%d][%d], value : 0x%x\n", param, index1, index2, index3, value)
#endif // API_ATTR_CHECK_LOG_LEVEL

#define CHECK_VALID_CONST(_struct, _param, _lower_bound, _upper_bound) \
{\
if ((_struct->_param > _upper_bound) || ((_struct->_param+1) < (_lower_bound+1))) {\
	ISP_VALUE_CHECK_ERR_LOG_V(STRING(_struct->_param), _struct->_param); \
	ret = CVI_FAILURE_ILLEGAL_PARAM;\
} \
}

#define CHECK_VALID_ARRAY_1D(_struct, _param, _idx1, _lower_bound, _upper_bound) \
{\
for (CVI_U32 _idx = 0 ; _idx < _idx1 ; _idx++) {\
	if ((_struct->_param[_idx] > _upper_bound) || ((_struct->_param[_idx]+1) < (_lower_bound+1))) {\
		ISP_VALUE_CHECK_ERR_LOG_A1(STRING(_struct->_param), _idx, _struct->_param[_idx]); \
		ret = CVI_FAILURE_ILLEGAL_PARAM;\
	} \
} \
}

#define CHECK_VALID_ARRAY_2D(_struct, _param, _idx1, _idx2, _lower_bound, _upper_bound) \
{\
for (CVI_U32 _i = 0 ; _i < _idx1 ; _i++) {\
	for (CVI_U32 _j = 0 ; _j < _idx2 ; _j++) {\
		if ((_struct->_param[_i][_j] > _upper_bound) || ((_struct->_param[_i][_j]+1) < (_lower_bound+1))) {\
			ISP_VALUE_CHECK_ERR_LOG_A2(STRING(_struct->_param), _i, _j, _struct->_param[_i][_j]); \
			ret = CVI_FAILURE_ILLEGAL_PARAM;\
		} \
	} \
} \
}

#define CHECK_VALID_ARRAY_3D(_struct, _param, _idx1, _idx2, _idx3, _lower_bound, _upper_bound) \
{\
for (CVI_U32 _i = 0 ; _i < _idx1 ; _i++) {\
	for (CVI_U32 _j = 0 ; _j < _idx2 ; _j++) {\
		for (CVI_U32 _k = 0 ; _k < _idx3 ; _k++) {\
			if ((_struct->_param[_i][_j][_k] > _upper_bound) ||\
				((_struct->_param[_i][_j][_k]+1) < (_lower_bound+1))) {\
				ISP_VALUE_CHECK_ERR_LOG_A3(STRING(_struct->_param), \
					_i, _j, _k, _struct->_param[_i][_j][_k]); \
				ret = CVI_FAILURE_ILLEGAL_PARAM;\
			} \
		} \
	} \
} \
}

#define CHECK_VALID_AUTO_LV_1D(_struct, _param, _lower_bound, _upper_bound) \
do {\
CHECK_VALID_CONST(_struct, stManual._param, _lower_bound, _upper_bound);\
CHECK_VALID_ARRAY_1D(_struct, stAuto._param, ISP_AUTO_LV_NUM, _lower_bound, _upper_bound);\
} while (0) \

#define CHECK_VALID_AUTO_LV_2D(_struct, _param, _idx1, _lower_bound, _upper_bound) \
do {\
CHECK_VALID_ARRAY_1D(_struct, stManual._param, _idx1, _lower_bound, _upper_bound);\
CHECK_VALID_ARRAY_2D(_struct, stAuto._param, _idx1, ISP_AUTO_LV_NUM, _lower_bound, _upper_bound);\
} while (0) \

#define CHECK_VALID_AUTO_LV_3D(_struct, _param, _idx1, _idx2, _lower_bound, _upper_bound) \
do {\
CHECK_VALID_ARRAY_2D(_struct, stManual._param, _idx1, _idx2, _lower_bound, _upper_bound);\
CHECK_VALID_ARRAY_3D(_struct, stAuto._param, _idx1, _idx2, ISP_AUTO_LV_NUM, _lower_bound, _upper_bound);\
} while (0) \

#define CHECK_VALID_AUTO_ISO_1D(_struct, _param, _lower_bound, _upper_bound) \
do {\
CHECK_VALID_CONST(_struct, stManual._param, _lower_bound, _upper_bound);\
CHECK_VALID_ARRAY_1D(_struct, stAuto._param, ISP_AUTO_ISO_STRENGTH_NUM, _lower_bound, _upper_bound);\
} while (0) \

#define CHECK_VALID_AUTO_ISO_2D(_struct, _param, _idx1, _lower_bound, _upper_bound) \
do {\
CHECK_VALID_ARRAY_1D(_struct, stManual._param, _idx1, _lower_bound, _upper_bound);\
CHECK_VALID_ARRAY_2D(_struct, stAuto._param, _idx1, ISP_AUTO_ISO_STRENGTH_NUM, _lower_bound, _upper_bound);\
} while (0) \

#define CHECK_VALID_AUTO_ISO_3D(_struct, _param, _idx1, _idx2, _lower_bound, _upper_bound) \
do {\
CHECK_VALID_ARRAY_2D(_struct, stManual._param, _idx1, _idx2, _lower_bound, _upper_bound);\
CHECK_VALID_ARRAY_3D(_struct, stAuto._param, _idx1, _idx2, ISP_AUTO_ISO_STRENGTH_NUM, _lower_bound, _upper_bound);\
} while (0) \

#define ISP_RELEASE_MEMORY(x) do { \
	if (x) { \
		free(x); \
		x = 0; \
	} \
} while (0)

#ifdef ARCH_RTOS_CV181X
#include "malloc.h"

#define ISP_MALLOC(size)\
	({\
		void *memory = malloc(size);\
		if (!memory)\
			ISP_LOG_ERR("can't allocate memory\n");\
		memory;\
	})

#define ISP_CALLOC(n, size)\
	({\
		void *memory = malloc((n) * (size));\
		if (!memory) {\
			ISP_LOG_ERR("can't allocate memory\n");\
		} else { \
			memset(memory, 0, (n) * (size));\
		} \
		memory;\
	})
#else
#define ISP_MALLOC(size)\
	({\
		void *memory = malloc(size);\
		if (!memory)\
			ISP_LOG_ERR("can't allocate memory\n");\
		memory;\
	})

#define ISP_CALLOC(n, size)\
	({\
		void *memory = calloc((n), (size));\
		if (!memory) {\
			ISP_LOG_ERR("can't allocate memory\n");\
		} \
		memory;\
	})
#endif

#define ISP_CREATE_RUNTIME(_instance, _type)\
({\
	if (_instance == NULL) {\
		_instance = (_type *)ISP_CALLOC(1, sizeof(_type));\
		ISP_LOG_DEBUG("%s(%p)\n", "runtime", _instance);\
		if (_instance == NULL) {\
			ISP_LOG_ERR("%s(%p)\n", "can't allocate memory", _instance);\
			ret = CVI_FAILURE;\
		} \
	} else {\
		ISP_LOG_ERR("%s(%p)\n", "initialized", _instance);\
	} \
})

#define	ISP_FORLOOP_SET(target, source, length) do {\
	for (CVI_U32 i = 0; i < length; i++)\
		target[i] = source[i];\
	} while (0)

#define ISP_U8_PTR  CVI_U64
#define ISP_U16_PTR CVI_U64
#define ISP_U32_PTR CVI_U64
#define ISP_U64_PTR CVI_U64

#define ISP_S8_PTR  CVI_U64
#define ISP_S16_PTR CVI_U64
#define ISP_S32_PTR CVI_U64
#define ISP_S64_PTR CVI_U64

#define ISP_VOID_PTR CVI_U64
#define ISP_BOOL_PTR CVI_U64

#if __SIZEOF_POINTER__ == 4
#define ISP_PTR_CAST_U8(x)  ((CVI_U8 *) (CVI_U32) x)
#define ISP_PTR_CAST_U16(x) ((CVI_U16 *) (CVI_U32) x)
#define ISP_PTR_CAST_U32(x) ((CVI_U32 *) (CVI_U32) x)
#define ISP_PTR_CAST_U64(x) ((CVI_U64 *) (CVI_U32) x)

#define ISP_PTR_CAST_S8(x)  ((CVI_S8 *) (CVI_U32) x)
#define ISP_PTR_CAST_S16(x) ((CVI_S16 *) (CVI_U32) x)
#define ISP_PTR_CAST_S32(x) ((CVI_S32 *) (CVI_U32) x)
#define ISP_PTR_CAST_S64(x) ((CVI_S64 *) (CVI_U32) x)

#define ISP_PTR_CAST_FLOAT(x) ((CVI_FLOAT *) (CVI_U32) x)
#define ISP_PTR_CAST_VOID(x) ((CVI_VOID *) (CVI_U32) x)
#define ISP_PTR_CAST_BOOL(x) ((CVI_BOOL *) (CVI_U32) x)

#define ISP_PTR_CAST_PTR(x) ((CVI_U32) x)
#else
#define ISP_PTR_CAST_U8(x)  ((CVI_U8 *) (CVI_U64) x)
#define ISP_PTR_CAST_U16(x) ((CVI_U16 *) (CVI_U64) x)
#define ISP_PTR_CAST_U32(x) ((CVI_U32 *) (CVI_U64) x)
#define ISP_PTR_CAST_U64(x) ((CVI_U64 *) (CVI_U64) x)

#define ISP_PTR_CAST_S8(x)  ((CVI_S8 *) (CVI_U64) x)
#define ISP_PTR_CAST_S16(x) ((CVI_S16 *) (CVI_U64) x)
#define ISP_PTR_CAST_S32(x) ((CVI_S32 *) (CVI_U64) x)
#define ISP_PTR_CAST_S64(x) ((CVI_S64 *) (CVI_U64) x)

#define ISP_PTR_CAST_FLOAT(x) ((CVI_FLOAT *) (CVI_U64) x)
#define ISP_PTR_CAST_VOID(x) ((CVI_VOID *) (CVI_U64) x)
#define ISP_PTR_CAST_BOOL(x) ((CVI_BOOL *) (CVI_U64) x)

#define ISP_PTR_CAST_PTR(x) ((CVI_U64) x)
#endif

#define ATTR __attribute__
#define ISP_PACKED ATTR((packed))
#define ISP_ALIGNED(x) ATTR((aligned(x)))

#define ENABLE_FE_WBG_UPDATE    0   // for rgbmap

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_DEFINES_H_
