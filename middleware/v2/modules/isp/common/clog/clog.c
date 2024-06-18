
#include "clog.h"
#include <assert.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <sys/syscall.h>

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif

static char *log_buf;
static char log_line_buf[CLOG_LINE_BUF_SIZE];

static FILE *fp;
static size_t file_size;
static char *log_buffer_AB[2];
static uint8_t log_buffer_index;
static size_t log_buffer_write_count[2];
static pthread_t write_thread_tid;
static char path_buf[CLOG_FILE_MAX_PATH_LEN];

static pthread_mutex_t log_buf_lock = PTHREAD_MUTEX_INITIALIZER;

static int clog_lock(void)
{
	pthread_mutex_lock(&log_buf_lock);
	return 0;
}

static int clog_unlock(void)
{
	pthread_mutex_unlock(&log_buf_lock);
	return 0;
}

static int clog_get_time(char *time_str, size_t size)
{
	struct timeval tv;
	struct tm cur_tm;

	gettimeofday(&tv, NULL);
	localtime_r(&tv.tv_sec, &cur_tm);

	snprintf(time_str, size, "%04d%02d%02d %02d:%02d:%02d.%03d ",
		cur_tm.tm_year + 1900,
		cur_tm.tm_mon + 1,
		cur_tm.tm_mday,
		cur_tm.tm_hour,
		cur_tm.tm_min,
		cur_tm.tm_sec,
		(int)(tv.tv_usec / 1000));

	return 0;
}

static const char *clog_get_tid(char *tid_str, size_t size)
{
	snprintf(tid_str, size, "%04d ", (int) syscall(SYS_gettid));
	return 0;
}

static int clog_file_open(void)
{

	static uint32_t file_count;

	if (fp) {
		fclose(fp);
		fp = NULL;
	}

	snprintf(path_buf, CLOG_FILE_MAX_PATH_LEN,
		CLOG_FILE_PATH"isplog-%02d.txt", file_count);

	fp = fopen(path_buf, "w");

	if (fp == NULL) {
		printf("ERROR, clog cannot open: %s\n", path_buf);
	} else {
		file_count++;
	}

	return 0;
}

static void write_file_thread_cleanup(void *param)
{
	UNUSED(param);

	if (fp) {
		fclose(fp);
		fp = NULL;
	}

	free(log_buffer_AB[0]);
	log_buffer_AB[0] = NULL;

	free(log_buffer_AB[1]);
	log_buffer_AB[1] = NULL;

	log_buffer_index = 0;
	log_buffer_write_count[0] = 0;
	log_buffer_write_count[1] = 0;
}

static void *write_file_thread(void *param)
{
	uint8_t log_buffer_index_backup;

	UNUSED(param);

	pthread_cleanup_push(write_file_thread_cleanup, NULL);

	log_buffer_index_backup = log_buffer_index;

	while (1) {

		if (log_buffer_index_backup != log_buffer_index) {

			if (fp == NULL) {
				clog_file_open();
			}

			if (fp) {
				fwrite(log_buffer_AB[log_buffer_index_backup],
					log_buffer_write_count[log_buffer_index_backup], 1, fp);

				fflush(fp);

				file_size += log_buffer_write_count[log_buffer_index_backup];

				if (file_size >= CLOG_FILE_SPLIT_SIZE) {
					clog_file_open();
					file_size = 0x00;
				}

				log_buffer_write_count[log_buffer_index_backup] = 0;
			}

			log_buffer_index_backup = log_buffer_index;
		}

		usleep(100 * 1000);
	}

	pthread_cleanup_pop(0);

	return NULL;
}

static void clog_file_output(const char *log, size_t size)
{
	UNUSED(log);

	if (log_buffer_AB[0] == NULL || log_buffer_AB[1] == NULL) {
		return;
	}

	log_buffer_write_count[log_buffer_index] += size;

	if (log_buffer_write_count[log_buffer_index] >=
		(CLOG_FILE_ASYNC_PINGPONG_BUF_SIZE - CLOG_LINE_BUF_SIZE)) {

		log_buffer_index = log_buffer_index == 0 ? 1 : 0;

		if (log_buffer_write_count[log_buffer_index] != 0) {
			printf("[ERROR], isp log over write!!!\n");
		}

		log_buffer_write_count[log_buffer_index] = 0;

		log_buf = log_buffer_AB[log_buffer_index];

	} else {
		log_buf = log_buffer_AB[log_buffer_index] +
			log_buffer_write_count[log_buffer_index];
	}
}

int clog_file_enable(void)
{
	clog_lock();

	if (log_buf == log_line_buf) {

		log_buffer_AB[0] = (char *) malloc(CLOG_FILE_ASYNC_PINGPONG_BUF_SIZE);
		log_buffer_AB[1] = (char *) malloc(CLOG_FILE_ASYNC_PINGPONG_BUF_SIZE);

		memset(log_buffer_AB[0], 0, CLOG_FILE_ASYNC_PINGPONG_BUF_SIZE);
		memset(log_buffer_AB[1], 0, CLOG_FILE_ASYNC_PINGPONG_BUF_SIZE);

		log_buf = log_buffer_AB[0];

		log_buffer_index = 0;
		log_buffer_write_count[0] = 0;
		log_buffer_write_count[1] = 0;

		pthread_create(&write_thread_tid, NULL, write_file_thread, NULL);
	}

	clog_unlock();

	return 0;
}

int clog_file_disable(void)
{
	clog_lock();

	if (write_thread_tid) {
		pthread_cancel(write_thread_tid);
		pthread_join(write_thread_tid, NULL);
		log_buf = log_line_buf;
	}

	clog_unlock();

	return 0;
}

/*******************************************************************************/
static const char * const level_output_info[] = {
	[CLOG_LVL_ASSERT]  = "A ",
	[CLOG_LVL_ERROR]   = "E ",
	[CLOG_LVL_WARN]    = "W ",
	[CLOG_LVL_INFO]    = "I ",
	[CLOG_LVL_DEBUG]   = "D ",
	[CLOG_LVL_VERBOSE] = "V ",
};

static uint8_t log_level;

int clog_init(void)
{
	int ret = 0;

	clog_lock();

	if (log_buf == NULL) {
		log_buf = log_line_buf;
		log_level = CLOG_LVL_ERROR;
	}

	clog_unlock();

	return ret;
}

int clog_deinit(void)
{
	int ret = 0;

	return ret;
}

int clog_set_level(uint8_t level)
{
	CLOG_ASSERT(level <= CLOG_LVL_VERBOSE);

	log_level = level;

	return 0;
}

static size_t clog_strcpy(size_t cur_len, char *dst, const char *src)
{
	const char *src_old = src;

	while (*src != 0) {
		if (cur_len++ < CLOG_LINE_BUF_SIZE) {
			*dst++ = *src++;
		} else {
			break;
		}
	}

	return src - src_old;
}

void clog_output(uint8_t level, const char *tag, const char *func,
	const long line, const char *format, ...)
{
#define CLOG_TEMP_BUF_SIZE  128

	static char temp_buf[CLOG_TEMP_BUF_SIZE] = { 0 };

	size_t log_len = 0;
	va_list args;
	int fmt_result;

	assert(level <= CLOG_LVL_VERBOSE);

	if (level > log_level) {
		return;
	}

	va_start(args, format);

	clog_lock();

	clog_get_time(temp_buf, CLOG_TEMP_BUF_SIZE);
	log_len += clog_strcpy(log_len, log_buf + log_len, temp_buf);

	clog_get_tid(temp_buf, CLOG_TEMP_BUF_SIZE);
	log_len += clog_strcpy(log_len, log_buf + log_len, temp_buf);

	log_len += clog_strcpy(log_len, log_buf + log_len, level_output_info[level]);
	log_len += clog_strcpy(log_len, log_buf + log_len, tag);

	log_len += clog_strcpy(log_len, log_buf + log_len, " ");

	log_len += clog_strcpy(log_len, log_buf + log_len, func);
	log_len += clog_strcpy(log_len, log_buf + log_len, ":");

	snprintf(temp_buf, CLOG_TEMP_BUF_SIZE, "%ld ", line);
	log_len += clog_strcpy(log_len, log_buf + log_len, temp_buf);

	fmt_result = vsnprintf(log_buf + log_len, CLOG_LINE_BUF_SIZE - log_len, format, args);

	va_end(args);

	log_len += fmt_result;

	if (level <= CLOG_LVL_ERROR) {

		printf("%s", log_buf);

		if (level == CLOG_LVL_ASSERT) {
			assert(0);
		}
	}

	clog_file_output(log_buf, log_len);

	clog_unlock();
}

