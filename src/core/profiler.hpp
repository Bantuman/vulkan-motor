#pragma once

#include <string_view>

#ifndef PROFILING_DISABLED
	#define PROFILE_BEGIN(...) profiler::context_begin(__VA_ARGS__)
	#define PROFILE_END() profiler::context_end()

	#define PROFILE_BEGIN_SCOPE(...) ScopeProfile(__VA_ARGS__)
#else
	#define PROFILE_BEGIN(...)
	#define PROFILE_END()

	#define PROFILE_BEGIN_SCOPE(...)
#endif

namespace profiler {
	void context_begin(std::string_view contextName);
	void context_end();

	struct ScopeProfile {
		explicit ScopeProfile(std::string_view contextName);
		~ScopeProfile();

		ScopeProfile(ScopeProfile&&) = delete;
		void operator=(ScopeProfile&&) = delete;

		ScopeProfile(const ScopeProfile&) = delete;
		void operator=(const ScopeProfile&) = delete;
	};
}

