/**
 ******************************************************************************
 *
 *  @file lmac_types.h
 *
 *  Copyright (C) BouffaloLab 2017-2024
 *
 *  Licensed under the Apache License, Version 2.0 (the License);
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an ASIS BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************
 */


#ifndef _LMAC_INT_H_
#define _LMAC_INT_H_


/**
 ****************************************************************************************
 * @addtogroup CO_INT
 * @ingroup COMMON
 * @brief Common integer standard types (removes use of stdint)
 *
 * @{
 ****************************************************************************************
 */


/*
 * DEFINES
 ****************************************************************************************
 */


#include <linux/version.h>
#include <linux/types.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)
#include <linux/bits.h>
#else
#include <linux/bitops.h>
#endif


typedef uint8_t u8_l;
typedef int8_t s8_l;
typedef bool bool_l;
typedef uint16_t u16_l;
typedef int16_t s16_l;
typedef uint32_t u32_l;
typedef int32_t s32_l;
typedef uint64_t u64_l;

/// @} CO_INT
#endif // _LMAC_INT_H_
