#include "profiler.hpp"

#include <logging.hpp>

#include <cassert>

#include <vector>
#include <chrono>

using namespace std::chrono;

namespace profiler {
	struct Context {
		std::string_view name;
		high_resolution_clock::time_point timePoint;
	};

	std::vector<Context> g_contextStack;
}

void profiler::context_begin(std::string_view contextName) {
	g_contextStack.push_back({contextName, high_resolution_clock::now()});
}

void profiler::context_end() {
	assert(!g_contextStack.empty());

	auto ctx = g_contextStack.back();
	g_contextStack.pop_back();

	// FIXME: just record this or something instead of domping eet
	long long diff = duration_cast<nanoseconds>(high_resolution_clock::now()
			- ctx.timePoint).count();

	LOG_DEBUG("PROFILE", "[%s] %.2fms (%.2f fps)", ctx.name.data(),
			static_cast<double>(diff) / 1000000.0,
			1000000000.0 / static_cast<double>(diff));
}

profiler::ScopeProfile::ScopeProfile(std::string_view contextName) {
	profiler::context_begin(contextName);
}

profiler::ScopeProfile::~ScopeProfile() {
	profiler::context_end();
}

