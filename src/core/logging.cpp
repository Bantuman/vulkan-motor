#include "logging.hpp"

#include <cstdarg>
#include <cstdio>

static const char* LOG_LEVEL_STRING[] = {
	"ERROR",
	"WARNING",
	"DEBUG",
	"TEMP"
};

/*#ifdef NDEBUG
static LogLevel g_logLevel = LOG_LEVEL_ERROR;
#else
static LogLevel g_logLevel = LOG_LEVEL_TEMP;
#endif*/

static LogLevel g_logLevel = LOG_LEVEL_TEMP;

void log_format(LogLevel logLevel, const char* category, const char* file,
		unsigned line, const char* fmt, ...) {
	if (logLevel > g_logLevel) {
		return;
	}

	fprintf(stderr, "[%s] [%s] (%s:%u): ", LOG_LEVEL_STRING[logLevel],
			category, file, line);

	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	fputc('\n', stderr);
}

void set_log_level(LogLevel logLevel) {
	g_logLevel = logLevel;
}

