#ifndef _LOGGER_H_
#define _LOGGER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <unistd.h>

#define LOG_DEBUG 0
#define LOG_INFO  1
#define LOG_WARN  2
#define LOG_ERROR 3

#define LOG_LEVEL_CHARS "DIWEF"
#define LOG_MAX_MSG_LEN 1024

struct _logger {
	int level;
	char *datetime_format;
	FILE *fp;
};

typedef struct _logger logger;

logger * logger_create(void);
void logger_free(logger *l);
void log_add(logger *l, int level, const char *msg);
void log_debug(logger *l, const char *fmt, ...);
void log_info(logger *l, const char *fmt, ...);
void log_warn(logger *l, const char *fmt, ...);
void log_error(logger *l, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
