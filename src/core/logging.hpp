#pragma once

enum LogLevel {
	LOG_LEVEL_ERROR = 0,
	LOG_LEVEL_WARNING,
	LOG_LEVEL_DEBUG,
	LOG_LEVEL_TEMP,
	LOG_LEVEL_COUNT
};

#define LOG(level, category, fmt, ...) \
	log_format(level, category, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_ERROR(category, fmt, ...) \
	LOG(LOG_LEVEL_ERROR, category, fmt, ##__VA_ARGS__)

#define LOG_ERROR2(category, msg) \
	LOG(LOG_LEVEL_ERROR, category, "%s", msg)

#define LOG_WARNING(category, fmt, ...) \
	LOG(LOG_LEVEL_WARNING, category, fmt, ##__VA_ARGS__)

#define LOG_WARNING2(category, msg) \
	LOG(LOG_LEVEL_WARNING, category, "%s", msg)

#define LOG_DEBUG(category, fmt, ...) \
	LOG(LOG_LEVEL_DEBUG, category, fmt, ##__VA_ARGS__)

#define LOG_TEMP(fmt, ...) \
	LOG(LOG_LEVEL_TEMP, "TEMP", fmt, ##__VA_ARGS__)

#define LOG_TEMP2(msg) \
	LOG(LOG_LEVEL_TEMP, "TEMP", "%s", msg)

void log_format(LogLevel logLevel, const char* category, const char* file,
		unsigned line, const char* fmt, ...);
void set_log_level(LogLevel logLevel);

