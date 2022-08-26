#pragma once

#include <cstddef>

#include <vector>
#include <string>

namespace Web {
	enum class RequestMethod {
		GET,
		POST
	};

	struct Request {
		const char* url;
		RequestMethod method;
		const char** pHeaders;
		size_t headerCount;
		const char* body;
	};

	struct Response {
		long statusCode;
		std::vector<char> body;
	};

	void init();
	void deinit();

	bool invoke_request(const Request&, Response&);
}

