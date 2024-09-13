/**
 ******************************************************************************
 *
 *  @file bl_strs.h
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


#ifndef _BL_STRS_H_
#define _BL_STRS_H_

#include "lmac_msg.h"

#define BL_ID2STR(tag) (((MSG_T(tag) < ARRAY_SIZE(bl_id2str)) &&        \
                           (bl_id2str[MSG_T(tag)]) &&          \
                           ((bl_id2str[MSG_T(tag)])[MSG_I(tag)])) ?   \
                          (bl_id2str[MSG_T(tag)])[MSG_I(tag)] : "unknown")

extern const char *const *bl_id2str[TASK_MAX + 1];

#endif /* _BL_STRS_H_ */
