#pragma once

#include <cstdint>
#include <cstddef>

enum class IterationDecision {
	CONTINUE,
	BREAK
};

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(WIN64)
	#define OPERATING_SYSTEM_WINDOWS
#elif defined(__linux__)
	#define OPERATING_SYSTEM_LINUX
#elif defined(__APPLE__)
	#define OPERATING_SYSTEM_MACOS
#else
	#define OPERATING_SYSTEM_OTHER
#endif

#if defined(__clang__)
	#define COMPILER_CLANG
#elif defined(__GNUC__) || defined(__GNUG__)
	#define COMPILER_GCC
#elif defined(_MSC_VER)
	#define COMPILER_MSVC
#else
	#define COMPILER_OTHER
#endif

#define NULL_COPY_AND_ASSIGN(ClassName) \
	ClassName(const ClassName&) = delete; \
	void operator=(const ClassName&) = delete; \
	ClassName(ClassName&&) = delete; \
	void operator=(ClassName&&) = delete
