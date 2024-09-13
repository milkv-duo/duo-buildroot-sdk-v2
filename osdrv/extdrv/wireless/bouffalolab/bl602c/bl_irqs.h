/**
 ******************************************************************************
 *
 *  @file bl_irqs.h
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

#ifndef _BL_IRQS_H_
#define _BL_IRQS_H_

#include <linux/interrupt.h>

/* IRQ handler to be registered by platform driver */
//irqreturn_t bl_irq_hdlr(int irq, void *dev_id);
void bl_irq_hdlr(struct sdio_func *func);
void bl_task(unsigned long data);
void main_wq_hdlr(struct work_struct *work);
void bl_queue_main_work(struct bl_hw *bl_hw);
void bl_get_interrupt_status(struct bl_hw *bl_hw, u8 claim);


#endif /* _BL_IRQS_H_ */
