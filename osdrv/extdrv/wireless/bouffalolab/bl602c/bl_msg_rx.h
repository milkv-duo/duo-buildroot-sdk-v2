/**
 ******************************************************************************
 *
 *  @file bl_msg_rx.h
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


#ifndef _BL_MSG_RX_H_
#define _BL_MSG_RX_H_

void bl_rx_handle_msg(struct bl_hw *bl_hw, struct ipc_e2a_msg *msg);

#endif /* _BL_MSG_RX_H_ */
