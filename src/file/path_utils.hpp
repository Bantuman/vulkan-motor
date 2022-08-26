#pragma once

#include <pair.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace PathUtils {

std::string join(const std::string_view& base, const std::string_view& path);
std::string make_relative_path(const std::string_view& base, const std::string_view& path);
std::string canonicalize_path(const std::string_view& path);
std::string enforce_scheme(const std::string_view& path);

Pair<std::string, std::string> split(const std::string_view& path);
Pair<std::string, std::string> split_scheme(const std::string_view& path);

std::string_view get_directory(const std::string_view& path);
std::string_view get_file_name(const std::string_view& path);
std::string_view get_file_extension(const std::string_view& path);

std::vector<std::string_view> tokenize_path(const std::string_view& path);
std::string merge_path(const std::vector<std::string_view>& tokens);

std::string get_executable_path();

}

