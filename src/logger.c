#include "logger.h"

logger * logger_create(void)
{
	logger *l = (logger *)malloc(sizeof(logger));
	if ( l == NULL )
		return NULL;

	l->datetime_format = (char *)"%Y-%m-%d %H:%M:%S";
	l->level = LOG_INFO;
	l->fp    = stdout;

	return l;
}

void logger_free(logger *l)
{
	if ( l != NULL ) {
		if ( fileno(l->fp) != STDOUT_FILENO )
			fclose(l->fp);
		free(l);
	}
}

void log_add(logger *l, int level, const char *msg)
{
	if (level < l->level) return;

	time_t meow = time(NULL);
	char buf[64];

	strftime(buf, sizeof(buf), l->datetime_format, localtime(&meow));
	fprintf(l->fp, "[%d] %c, %s : %s\n",
			(int)getpid(),
			LOG_LEVEL_CHARS[level],
			buf,
			msg);
}

void log_debug(logger *l, const char *fmt, ...)
{
	va_list ap;
	char msg[LOG_MAX_MSG_LEN];

	va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
	log_add(l, LOG_DEBUG, msg);
	va_end(ap);
}

void log_info(logger *l, const char *fmt, ...)
{
	va_list ap;
	char msg[LOG_MAX_MSG_LEN];

	va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
	log_add(l, LOG_INFO, msg);
	va_end(ap);
}

void log_warn(logger *l, const char *fmt, ...)
{
	va_list ap;
	char msg[LOG_MAX_MSG_LEN];

	va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
	log_add(l, LOG_WARN, msg);
	va_end(ap);
}

void log_error(logger *l, const char *fmt, ...)
{
	va_list ap;
	char msg[LOG_MAX_MSG_LEN];

	va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
	log_add(l, LOG_ERROR, msg);
	va_end(ap);
}
