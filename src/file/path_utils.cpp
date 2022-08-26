#include "path_utils.hpp"

#include <core/common.hpp>

#include <sstream>
#include <algorithm>

#ifdef OPERATING_SYSTEM_WINDOWS
	static constexpr const bool HAS_BACKSLASH = true;

	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#else
	#include <array>

	static constexpr const bool HAS_BACKSLASH = false;
	static constexpr const std::array<const char*, 3> FILE_EXTENSIONS = {
		"exe",
		"file",
		"a.out"
	};

	#include <unistd.h>

	#ifdef OPERATING_SYSTEM_LINUX
		#include <linux/limits.h>
	#endif
#endif

static size_t find_last_slash(const std::string_view& str);

std::string PathUtils::join(const std::string_view& base,
		const std::string_view& path) {
	if (base.empty()) {
		return std::string(path);
	}

	if (path.empty()) {
		return std::string(base);
	}

	std::ostringstream ss;
	ss << base;

	if (find_last_slash(base) != base.length() - 1) {
		ss << '/';
	}

	ss << path;

	return ss.str();
}

std::string PathUtils::make_relative_path(const std::string_view& base,
		const std::string_view& path) {
	return PathUtils::join(PathUtils::get_directory(base), path);
}

std::string PathUtils::canonicalize_path(const std::string_view& path) {
	std::vector<std::string_view> tokens = tokenize_path(path);
	std::vector<std::string_view> result;

	for (auto& token : tokens) {
		if (token == ".") {
			continue;
		}

		if (token == "..") {
			if (!result.empty()) {
				result.pop_back();
			}
		}
		else {
			result.push_back(token);
		}
	}

	return merge_path(result);
}

std::string PathUtils::enforce_scheme(const std::string_view& path) {
	if (path.empty()) {
		return "";
	}

	if (path.find("://") == std::string_view::npos) {
		return std::string("file://") + std::string(path);
	}

	return std::string(path);
}

Pair<std::string, std::string> PathUtils::split(const std::string_view& path) {
	if (path.empty()) {
		return {};
	}

	size_t index = find_last_slash(path);
	
	if (index == std::string::npos) {
		return {std::string(path), ""};
	}

	return {
		std::string(path.substr(0, index)),
		std::string(path.substr(index + 1))
	};
}

Pair<std::string, std::string> PathUtils::split_scheme(const std::string_view& path) {
	if (path.empty()) {
		return {};
	}

	size_t index = path.find("://");

	if (index == std::string::npos) {
		return {"", std::string(path)};
	}

	return {
		std::string(path.substr(0, index)),
		std::string(path.substr(index + 3))
	};
}

std::string_view PathUtils::get_directory(const std::string_view& path) {
	if (path.empty()) {
		return "";
	}

	size_t index = find_last_slash(path);

	if (index == std::string::npos) {
		return path;
	}

	return path.substr(0, index);
}

std::string_view PathUtils::get_file_name(const std::string_view& path) {
	if (path.empty()) {
		return "";
	}

	size_t index = find_last_slash(path);

	if (index == std::string::npos) {
		return path;
	}

	return path.substr(index + 1);
}

std::string_view PathUtils::get_file_extension(const std::string_view& path) {
	if (path.empty()) {
		return "";
	}

	size_t index = path.find_last_of('.');

	if (index == std::string::npos) {
		return "";
	}

	return path.substr(index + 1);
}

std::vector<std::string_view> PathUtils::tokenize_path(const std::string_view& path) {
	if (path.empty()) {
		return {};
	}

	std::vector<std::string_view> result;

	size_t startIndex = 0;
	size_t index;

	constexpr const char* delim = HAS_BACKSLASH ? "/\\" : "/";

	while ((index = path.find_first_of(delim, startIndex)) != std::string_view::npos) {
		if (index > startIndex) {
			result.push_back(path.substr(startIndex, index - startIndex));
		}

		startIndex = index + 1;
	}

	if (startIndex < path.length()) {
		result.push_back(path.substr(startIndex));
	}

	return result;
}

std::string PathUtils::merge_path(const std::vector<std::string_view>& tokens) {
	std::ostringstream ss;

	for (size_t i = 0; i < tokens.size(); ++i) {
		ss << tokens[i];

		if (i != tokens.size() - 1) {
			ss << '/';
		}
	}

	return ss.str();
}

std::string PathUtils::get_executable_path() {
#ifdef OPERATING_SYSTEM_WINDOWS
	char target[4096];
	DWORD ret = GetModuleFileNameA(GetModuleHandle(nullptr), target, sizeof(target));
	target[ret] = '\0';

	return canonicalize_path(target);
#else
	char linkPath[PATH_MAX];
	char target[PATH_MAX];

	for (const char* ext : FILE_EXTENSIONS) {
		std::snprintf(linkPath, sizeof(linkPath), "/proc/self/%s", ext);
		ssize_t ret = readlink(linkPath, target, sizeof(target) - 1);

		if (ret >= 0) {
			target[ret] = '\0';
		}

		return canonicalize_path(target);
	}

	return std::string("");
#endif
}

static size_t find_last_slash(const std::string_view& str) {
	if constexpr (HAS_BACKSLASH) {
		return str.find_last_of("/\\");
	}
	else {
		return str.find_last_of('/');
	}
}

