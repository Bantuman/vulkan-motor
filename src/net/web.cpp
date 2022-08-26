#include "web.hpp"

#include <cassert>
#include <cstdlib>
#include <cstring>

#include <vector>

// Async: https://curl.se/libcurl/c/libcurl-multi.html
#include <curl/curl.h>

static CURL* g_session;

static size_t write_callback(void* ptr, size_t size, size_t nmemb, void* userData);

void Web::init() {
	curl_global_init(CURL_GLOBAL_DEFAULT);
	g_session = curl_easy_init();
}

void Web::deinit() {
	curl_easy_cleanup(g_session);
	curl_global_cleanup();
}

bool Web::invoke_request(const Request& request, Response& response) {
	assert(request.url && "Request must have a url");

	curl_easy_setopt(g_session, CURLOPT_URL, request.url);

	if (request.method == RequestMethod::POST) {
		curl_easy_setopt(g_session, CURLOPT_POST, true);
	}

	struct curl_slist* chunk = NULL;

	for (size_t i = 0; i < request.headerCount; ++i) {
		chunk = curl_slist_append(chunk, request.pHeaders[i]);
	}

	curl_easy_setopt(g_session, CURLOPT_HTTPHEADER, chunk);

	if (request.body && request.method == RequestMethod::POST) {
		curl_easy_setopt(g_session, CURLOPT_POSTFIELDS, request.body);
	}

	response.body.clear();

	curl_easy_setopt(g_session, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(g_session, CURLOPT_WRITEDATA, &response.body);

	//std::vector<char> headerData;
	//curl_easy_setopt(g_session, CURLOPT_HEADERDATA, &headerData);

	CURLcode res = curl_easy_perform(g_session);

	response.statusCode = 0;

	if (res == CURLE_OK) {
		curl_easy_getinfo(g_session, CURLINFO_RESPONSE_CODE, &response.statusCode);

		return response.statusCode >= 200 && response.statusCode <= 299;
	}
	else {
		printf("[ERROR] [CURL]: %s\n", curl_easy_strerror(res));
	}

	return false;
}

static size_t write_callback(void* ptr, size_t size, size_t nmemb, void* userData) {
	auto& buffer = *reinterpret_cast<std::vector<char>*>(userData);

	size *= nmemb;

	size_t prevSize = buffer.size();
	buffer.resize(prevSize + size);

	memcpy(buffer.data() + prevSize, ptr, size);

	return size;
}

