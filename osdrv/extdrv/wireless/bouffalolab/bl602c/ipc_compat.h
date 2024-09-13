/**
 ******************************************************************************
 *
 *  @file ipc_compat.h
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


#ifndef _IPC_H_
#define _IPC_H_

#define __INLINE static __attribute__((__always_inline__)) inline

#define __ALIGN4 __aligned(4)

#define ASSERT_ERR(condition)                                                           \
    do {                                                                                \
        if (unlikely(!(condition))) {                                                   \
            printk(KERN_ERR "%s:%d:ASSERT_ERR(" #condition ")\n", __FILE__,  __LINE__); \
        }                                                                               \
    } while(0)

#endif /* _IPC_H_ */
