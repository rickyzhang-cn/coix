#include <stdarg.h>
#include <stdbool.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "logger.h"

static logger *log_p;

bool logger_create(void)
{
	log_p = (logger *)malloc(sizeof(logger));
	if ( log_p == NULL )
		return false;

	log_p->datetime_format = (char *)"%Y-%m-%d %H:%M:%S";
	log_p->level = LOG_INFO;
	log_p->fp = stdout;

	return true;
}

void logger_free(void)
{
	if ( log_p != NULL ) {
		if ( fileno(log_p->fp) != STDOUT_FILENO )
			fclose(log_p->fp);
		free(log_p);
	}
}

static void log_add(int level, const char *msg)
{
	if (level < log_p->level) return;

	time_t meow = time(NULL);
	char buf[64];

	strftime(buf, sizeof(buf), log_p->datetime_format, localtime(&meow));
	fprintf(log_p->fp, "[%d] %c, %s : %s\n",
			(int)getpid(),
			LOG_LEVEL_CHARS[level],
			buf,
			msg);
}

void log_debug(const char *fmt, ...)
{
	va_list ap;
	char msg[LOG_MAX_MSG_LEN];

	va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
	log_add(LOG_DEBUG, msg);
	va_end(ap);
}

void log_info(const char *fmt, ...)
{
	va_list ap;
	char msg[LOG_MAX_MSG_LEN];

	va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
	log_add(LOG_INFO, msg);
	va_end(ap);
}

void log_warn(const char *fmt, ...)
{
	va_list ap;
	char msg[LOG_MAX_MSG_LEN];

	va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
	log_add(LOG_WARN, msg);
	va_end(ap);
}

void log_error(const char *fmt, ...)
{
	va_list ap;
	char msg[LOG_MAX_MSG_LEN];

	va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
	log_add(LOG_ERROR, msg);
	va_end(ap);
	exit(EXIT_FAILURE);
}
