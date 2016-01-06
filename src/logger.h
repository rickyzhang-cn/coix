#ifndef _LOGGER_H_
#define _LOGGER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdbool.h>

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

bool logger_create(void);
void logger_free(void);
void log_debug(const char *fmt, ...);
void log_info(const char *fmt, ...);
void log_warn(const char *fmt, ...);
void log_error(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
