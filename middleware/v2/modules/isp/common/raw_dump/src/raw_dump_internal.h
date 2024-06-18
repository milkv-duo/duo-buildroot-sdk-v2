/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: raw_dump_internal.h
 * Description:
 *
 */

#ifndef _RAW_DUMP_INTERNAL_H_
#define _RAW_DUMP_INTERNAL_H_

#include "cvi_type.h"

#if defined(CHIP_ARCH_CV183X) || defined(CHIP_ARCH_CV182X)
#include "cvi_common.h"
#else
#include <linux/cvi_common.h>
#endif //

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

typedef struct _RAW_DUMP_INFO {
	char *pathPrefix;
	CVI_U32 u32TotalFrameCnt;
	RECT_S stRoiRect;
	bool bDump2Remote;
} RAW_DUMP_INFO_S;

CVI_S32 cvi_raw_dump(VI_PIPE ViPipe, const RAW_DUMP_INFO_S *pstRawDumpInfo);

int send_file(const char *path);
int send_raw(const char *path, int totalLength);
int send_raw_data(const uint8_t *data, int length);
int send_status(int status);

void *cvi_raw_dump_run(void *param);

#ifndef FIFO_HEAD

#undef FIFO_INIT
#undef FIFO_EXIT
#undef FIFO_EMPTY
#undef FIFO_FULL
#undef FIFO_CAPACITY
#undef FIFO_SIZE
#undef FIFO_PUSH
#undef FIFO_POP
#undef FIFO_FOREACH
#undef FIFO_GET_FRONT
#undef FIFO_GET_TAIL

/* [FIFO] is implemented by the idea of circular-array.
 *
 * FIFO_HEAD: Declare new struct.
 * FIFO_INIT: Initialize FIFO.
 * FIFO_EXIT: Finalize FIFO.
 * FIFO_EMPTY: If FIFO is empty.
 * FIFO_CAPACITY: How many elements can be placed in FIFO.
 * FIFO_SIZE: Number of elements in FIFO.
 * FIFO_FULL: If FIFO is full.
 * FIFO_PUSH: PUSH elm into FIFO's tail. overwritren old data if full.
 * FIFO_POP: POP elm from FIFO's front.
 * FIFO_FOREACH: For-loop go through FIFO.
 */
#define FIFO_HEAD(name, type)						\
	struct name {							\
		struct type *fifo;					\
		int front, tail, capacity;				\
	}

#define FIFO_INIT(head, _capacity) do {					\
	(head)->fifo = malloc(sizeof(*(head)->fifo) * _capacity);	\
	(head)->front = (head)->tail = -1;				\
	(head)->capacity = _capacity;					\
} while (0)

#define FIFO_EXIT(head) do {						\
	(head)->front = (head)->tail = -1;				\
	(head)->capacity = 0;						\
	if ((head)->fifo)						\
		free((head)->fifo);					\
} while (0)

#define FIFO_EMPTY(head)    ((head)->front == -1)

#define FIFO_FULL(head)     (((head)->front == ((head)->tail + 1))	\
			    || (((head)->front == 0) && ((head)->tail == ((head)->capacity - 1))))

#define FIFO_CAPACITY(head) ((head)->capacity)

#define FIFO_SIZE(head)     (FIFO_EMPTY(head) ?\
	0 : ((((head)->tail + (head)->capacity - (head)->front) % (head)->capacity) + 1))

#define FIFO_PUSH(head, elm) do {					\
	if (FIFO_EMPTY(head))						\
		(head)->front = (head)->tail = 0;			\
	else								\
		(head)->tail = ((head)->tail == (head)->capacity - 1)	\
				? 0 : (head)->tail + 1;			\
	(head)->fifo[(head)->tail] = elm;				\
} while (0)

#define FIFO_POP(head, pelm) do {					\
	*(pelm) = (head)->fifo[(head)->front];				\
	if ((head)->front == (head)->tail)				\
		(head)->front = (head)->tail = -1;			\
	else								\
		(head)->front = ((head)->front == (head)->capacity - 1)	\
				? 0 : (head)->front + 1;		\
} while (0)

#define FIFO_FOREACH(var, head, idx)					\
	for (idx = (head)->front, var = (head)->fifo[idx];		\
		idx < (head)->front + FIFO_SIZE(head);			\
		idx = idx + 1, var = (head)->fifo[idx % (head)->capacity])

#define FIFO_GET_FRONT(head, pelm) (*(pelm) = (head)->fifo[(head)->front])

#define FIFO_GET_TAIL(head, pelm) (*(pelm) = (head)->fifo[(head)->tail])
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _RAW_DUMP_INTERNAL_H_
