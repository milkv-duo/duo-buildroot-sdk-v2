#ifndef __STIMER_H__
#define __STIMER_H__
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<termios.h>
#include<errno.h>
#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<sys/time.h>
#include<sys/times.h>
#include<sys/ioctl.h>
#include <pthread.h>
#include<signal.h>
#include<string.h>


#define STIMER_CYCLE	1
#define STIMER_ONCE		2

typedef struct _stime {
	unsigned long msec;
	unsigned long intermsec;
	void (*handle)(unsigned long);
	unsigned long args;
	unsigned long timer_id;
	unsigned int enable;
	struct sigaction act;
	pthread_t tid;
	struct _stime *prev, *next;
} stimer, *pstimer;

pstimer soft_timer_create(void);
int soft_timer_set(pstimer pst, unsigned long msec, void (*handle)(unsigned long), void *arg, int mode);
int soft_timer_feed(pstimer pst);
int soft_timer_pause(pstimer pst);
int soft_timer_continue(pstimer pst);
int soft_timer_del(pstimer pst);
int soft_watchdog_init(void);
void soft_watchdog_exit(void);

#endif
